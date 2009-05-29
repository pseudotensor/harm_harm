

//////////////////
// all things related to FLUXB==FLUXCTSTAG
// Except fluxcalc_standard_4fluxctstag() (and its interpolate_prim_cent2face()) that could be generally used to replace standard method
// and except more general functions with simple FLUXB==FLUXCTSTAG conditions like getplpr()
// and except functions that call these things, of course

#include "decs.h"




///////////////////////////////////////////////////////
//
// OUTLINE of entire staggered field procedure:
//
//
// 1) init.c has user set A_i at CORN_i (and user can control whether A_i set or B^i set via fieldfrompotential[])
// 2) initbase.c computes B^i @ FACE_i and unew=\detg B^i at FACE_i using vpot2field() and B^i and \detg B^i at CENT through interpolation:
//   a) Creates higher-order A_i or flux using:
//     i) vectorpot_useflux() when using flux
//     ii) vectorpot_fluxreconorfvavg() for FV and FLUXRECON methods otherwise
//   b) Obtains point field and conserved field using:
//     i)  vpot2field_staggeredfield() if higher-order method involves higher-order field evolution
//     ii) vpot2field_useflux() if higher-order method involves evolving point fields
//   c) Obtains de-averaged point conserved staggered conserved field using deaverage_ustag2pstag()
//   d) Interpolates staggered FACE1,2,3 field to CENT using interpolate_pfield_face2cent()
// 3) advance.c: Uses Ui=unew, and flux-updates unew and uf, which are used to obtain B^i and \detg B^i at cent:
//   a) Computes \detg B^i [ui] at FACE1,2,3 for ui at FACE1,2,3 for field in advance_standard()  (Ui at CENT from pi for non-field)
//   b) Computes pstag at FACE1,2,3 from updated unew/uf at FACE1,2,3 using deaverage_ustag2pstag() in advance_standard()
//   c) Computes \detg B^i [upoint] at CENT from pstag at FACE1,2,3 using  interpolate_pfield_face2cent() in advance_standard()
//
// So updating conserved \detg B^i at FACE1,2,3 and keeping pstag consistent with this using de-averaging function
// Inversion is peformed on interpolated B^i at CENT obtained from face pstag


// NOTES:

// 1) Presently bound pstag because corner regions need EMF and interpolate face to edge so need boundary face values
//    For example, for EMF3 we need B1 along dir=2 and B2 along dir=1 so can reconstruct field to corner. This requires (a limited) bounding of pstag.  I don't see a way around this without using CENT to get everything and then violating locality of pstag with the EMFs.
// In non-staggered scheme those face values could come from centered values.  Would be inconsistent to do that for stag method, so need to find another way if want to only bound CENT primitives.
// 

// 2) Note that boundary conditions are applied to staggered field in logical way similar to CENT field (with one exception).
//    For MPI this is correct.  For periodic this is correct.  For reflecting BC this is correct as long as boundary conditions supplied and (for polar axis) \gdetg=0.0 exactly.  For analytical setting of BCs this is correct.  For outflow this is *sufficient* since don't really need to evolve that last cell
//    This makes code simpler with this assumption so don't have to extra-evolve the last upper face field value
// For FLUXRECON, this means bound_flux() must set upper flux so staggered field is set by boundary conditions
// For IF3DSPCTHENMPITRANSFERATPOLE, this is no longer true.  In this case must evolve that outer cell to evolve B2 at outer pole for any-all i,k,myid.

// 3) Note that for some problems the staggered method reaches nearly 0 field so that the normalized divb is erroneously large due to hitting machine errors relative to the dominate field strength evolved.  Maybe should output unnormalized divb too
//    TOTH doesn't have this problem because diffusion is high enough to keep field value large.  In 0-field regions TOTH generates checkerboard pattern with much higher field values and so divb appears to stay small.






static void rescale_calc_stagfield_full(int *Nvec, FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR]);







// This vpot2field function is for staggered method to evolve quasi-deaveraged fields (i.e. not point fields) when doing higher order
// compute field at FACE1,2,3 from vector potential A at CORN1,2,3
// assumes normal field p
// assume if 1D then along the 1D direction the field doesn't change and input pfield and ufield are already correct and set
int vpot2field_staggeredfield(FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*pfield)[NSTORE2][NSTORE3][NPR],FTYPE (*ufield)[NSTORE2][NSTORE3][NPR])
{
  int Nvec[NDIM];


#if(FIELDSTAGMEM==0)
  dualfprintf(fail_file,"Set FIELDSTAGMEM==1 to do FLUXCTSTAG\n");
  myexit(7158915);
#endif

  /* flux-staggered */

  // note A_i is always at CORN1,2,3 (edges) for any method, so init.c remains the same
  // all comments same as for centered field -- only change is no averaging of A_i to faces from edges (CORN1,2,3)


  Nvec[0]=0;
  Nvec[1]=N1;
  Nvec[2]=N2;
  Nvec[3]=N3;

  ////////////////////////
  //
  // now with double-quasi-deaveraged A_i compute B^i
  //
  // use Nvec in case 1-D then if Nvec[odir1]==1 && Nvec[odir2]==1 then don't assign dir field since constant in time and not zero as would be determined here
  // pfield should be from de-(laterally)-averaged ufield at FACE1,2,3
  //
  ////////////////////////

  // since nowait'ed above (because each loop sets ufield,pfield for each dir that don't depend upon eachother), need barrier here at end to stop before continuing (implied in parallel reigon)
#pragma omp parallel 
  {

    int i,j,k;
    int dir;
    FTYPE igdetgnosing[NDIM];
    int odir1,odir2;
    int jj;
    struct of_geom geomfdontuse[NDIM];
    struct of_geom *ptrgeomf[NDIM];
    OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUPFULL; // doesn't depend upon dir, but blockijk must be private


    // generally ptr's are different inside parallel block
    DLOOPA(jj) ptrgeomf[jj]=&(geomfdontuse[jj]);



    dir=1;
    get_odirs(dir,&odir1,&odir2);
    if(!(Nvec[odir1]==1 && Nvec[odir2]==1)){
      //////////      COMPFULLLOOP{ // COMPFULLLOOP allows since A_i exists at COMPFULLLOOPP1 and so always accessing valid A_i
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);

	// ufield doesn't require geometry
	MACP0A1(ufield,i,j,k,B1)  = +(NOAVGCORN_1(A[3],i,jp1mac(j),k)-NOAVGCORN_1(A[3],i,j,k))/(dx[2]);
	MACP0A1(ufield,i,j,k,B1) += -(NOAVGCORN_1(A[2],i,j,kp1mac(k))-NOAVGCORN_1(A[2],i,j,k))/(dx[3]);

	get_geometry_gdetonly(i, j, k, FACE1-1+dir, ptrgeomf[dir]);
	set_igdetsimple(ptrgeomf[dir]);
	igdetgnosing[dir] = ptrgeomf[dir]->igdetnosing;
	MACP0A1(pfield,i,j,k,B1-1+dir)  = MACP0A1(ufield,i,j,k,B1-1+dir)*igdetgnosing[dir];

      }/// end 3D LOOP
    }// end if
 
    dir=2;
    get_odirs(dir,&odir1,&odir2);
    if(!(Nvec[odir1]==1 && Nvec[odir2]==1)){
      ///////COMPFULLLOOP{
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);

	MACP0A1(ufield,i,j,k,B2)  = +(NOAVGCORN_2(A[1],i,j,kp1mac(k))-NOAVGCORN_2(A[1],i,j,k))/(dx[3]);
	MACP0A1(ufield,i,j,k,B2) += -(NOAVGCORN_2(A[3],ip1mac(i),j,k)-NOAVGCORN_2(A[3],i,j,k))/(dx[1]);

	get_geometry_gdetonly(i, j, k, FACE1-1+dir, ptrgeomf[dir]);
	set_igdetsimple(ptrgeomf[dir]);
	igdetgnosing[dir] = ptrgeomf[dir]->igdetnosing;
	MACP0A1(pfield,i,j,k,B1-1+dir)  = MACP0A1(ufield,i,j,k,B1-1+dir)*igdetgnosing[dir];

      }
    }

    dir=3;
    get_odirs(dir,&odir1,&odir2);
    if(!(Nvec[odir1]==1 && Nvec[odir2]==1)){
      //////////      COMPFULLLOOP{
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);

	MACP0A1(ufield,i,j,k,B3)  = +(NOAVGCORN_3(A[2],ip1mac(i),j,k)-NOAVGCORN_3(A[2],i,j,k))/(dx[1]);
	MACP0A1(ufield,i,j,k,B3) += -(NOAVGCORN_3(A[1],i,jp1mac(j),k)-NOAVGCORN_3(A[1],i,j,k))/(dx[2]);

	get_geometry_gdetonly(i, j, k, FACE1-1+dir, ptrgeomf[dir]);
	set_igdetsimple(ptrgeomf[dir]);
	igdetgnosing[dir] = ptrgeomf[dir]->igdetnosing;
	MACP0A1(pfield,i,j,k,B1-1+dir)  = MACP0A1(ufield,i,j,k,B1-1+dir)*igdetgnosing[dir];

      }
    }
  }// end full parallel region (with implied barrier)



#if(0)
  bound_prim(STAGEM1,t,ufield, 0);
  bound_prim(STAGEM1,t,pfield, 0);
#endif

  

  return(0);
}








// wrapper for:
// 1) interpolate FACE 2 CORN
// 2) loop over dimensions setting field flux dimension-by-dimension using multi-D interpolated CORN quantities
// At present, original flux as emf is computed like normal flux even if overwritten here, and shouldn't be much more expensive doing that there since primary cost is interpolation whose results are required and used here
int fluxcalc_fluxctstag(int stage, FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
			FTYPE (*pbcorn)[COMPDIM][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
			FTYPE (*pvcorn)[COMPDIM][NUMCS][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
			FTYPE (*wspeed)[NUMCS][NSTORE1][NSTORE2][NSTORE3],
			FTYPE (*prc)[NSTORE2][NSTORE3][NPR2INTERP],
			FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP],
			FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP],
			struct of_state (*fluxstatecent)[NSTORE2][NSTORE3],
			struct of_state (*fluxstate)[NUMLEFTRIGHT][NSTORE1][NSTORE2][NSTORE3],
			FTYPE (*geomcorn)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
			int *Nvec, FTYPE (*dqvec[NDIM])[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*fluxvec[NDIM])[NSTORE2][NSTORE3][NPR], FTYPE CUf, struct of_loop *cent2faceloop, struct of_loop (*face2cornloop)[NDIM][NDIM])
{
  int interpolate_prim_face2corn(FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*primface_l)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*primface_r)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*pbcorn)[COMPDIM][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*pvcorn)[COMPDIM][NUMCS][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], struct of_loop *cent2faceloop, struct of_loop (*face2cornloop)[NDIM][NDIM],
				 FTYPE (*prc)[NSTORE2][NSTORE3][NPR2INTERP],
				 FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP],
				 FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP],
				 int *Nvec, FTYPE (*dqvec[NDIM])[NSTORE2][NSTORE3][NPR2INTERP]);
  int dir;
  int idel, jdel, kdel, face;
  int is, ie, js, je, ks, ke;
  int fluxcalc_fluxctstag_emf_1d(int stage, FTYPE (*pr)[NSTORE2][NSTORE3][NPR], int dir, int odir1, int odir2, int is, int ie, int js, int je, int ks, int ke, FTYPE (*fluxvec[NDIM])[NSTORE2][NSTORE3][NPR], FTYPE CUf, struct of_loop (*face2cornloop)[NDIM],
				 FTYPE (*pbcorn)[COMPDIM][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
				 FTYPE (*pvcorn)[COMPDIM][NUMCS][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
				 FTYPE (*wspeed)[NUMCS][NSTORE1][NSTORE2][NSTORE3]);
  int edgedir, odir1,odir2;
  //  static int firsttime=1;
  int i,j,k;
  int enerregion;
  int *localenerpos;



  // forCOMPZSLOOP:
  // avoid looping over region outside active+ghost grid
  // good because somewhat general and avoid bad inversions, etc.
  enerregion=TRUEGLOBALWITHBNDENERREGION;
  localenerpos=enerposreg[enerregion];


  //////////////////////////
  //
  // first obtain pbinterp and pvinterp (point CORN1,2,3 quantities) from pl_ct and pr_ct (FACE1,2,3 quantities)
  // This can't be done per dimension since flux needs multi-D quantities for 2D Riemann problem...hence why stored and outside dimenloop
  //
  ////////////////////////////
  interpolate_prim_face2corn(pr, pl_ct, pr_ct, pbcorn, pvcorn, cent2faceloop, face2cornloop,prc,pleft,pright,Nvec,dqvec);




  ///////////////////////////////////////////////
  //
  // LOOP OVER EDGES (corresponds to EMF,edge directions, NOT face directions)
  //
  ///////////////////////////////////////////////
  DIMENLOOP(dir){

    edgedir=dir;

    ///////////////////////////////////////////////
    //
    // other dimensions
    // will be setting flux associated with d_t(Bodir1) and d_t(Bodir2) and setting flux(Bdir)->0

    get_odirs(dir,&odir1,&odir2);
    // skip to next dir if 1D such that EMF[dir] not needed since always cancels with itself
    // assumes set to 0 and if set to 0 once then always 0 (assume set to 0 in 
    if(Nvec[odir1]==1 && Nvec[odir2]==1){
      //      if(firsttime){
	// then ensure that really 0 and should remain 0 for entire evolution
	// don't need to set this except once assuming no other code sets to non-zero
	// No!  If those directions are 0 then flux isn't defined nor used
	//COMPZSLOOP( -N1BND, N1-1+N1BND, -N2BND, N2-1+N2BND, -N3BND, N3-1+N3BND ){
	//	  MACP1A1(fluxvec,odir1,i,j,k,B1-1+odir2)=MACP1A1(fluxvec,odir2,i,j,k,B1-1+odir1)=MACP1A1(fluxvec,dir,i,j,k,B1-1+dir)=0.0;
	//	}
      //      }
      continue;
    }

    //loop over the interfaces where fluxes are computed -- atch, useCOMPZSLOOP( is, ie, js, je, ks, ke ) { ... }
    // since looping over edges (emfs) and flux loop different than emf loop, then expand loops so consistent with both fluxes corresponding to that emf
    is=emffluxloop[dir][FIS];
    ie=emffluxloop[dir][FIE];
    js=emffluxloop[dir][FJS];
    je=emffluxloop[dir][FJE];
    ks=emffluxloop[dir][FKS];
    ke=emffluxloop[dir][FKE];


    // dir corrsponds to *edge,emf* NOT face
    MYFUN(fluxcalc_fluxctstag_emf_1d(stage, pr, dir, odir1, odir2, is, ie, js, je, ks, ke, fluxvec, CUf, face2cornloop[edgedir],pbcorn,pvcorn,wspeed),"flux.c:fluxcalc()", "fluxcalc_fluxctstag_1d", dir);

#if(PRODUCTION==0)
    trifprintf("%d",dir);
#endif
  }// end DIMENLOOP(dir)


#if(0)
  bound_flux(STAGEM1,t,fluxvec[1],fluxvec[2],fluxvec[3], 0);
  if(N1>1) bound_prim(STAGEM1,t,fluxvec[1], 0);
  if(N2>1) bound_prim(STAGEM1,t,fluxvec[2], 0);
  if(N3>1) bound_prim(STAGEM1,t,fluxvec[3], 0);
#endif


#if(0)
  // SUPERDEBUG: shouldn't be needed -- test

  dualfprintf(fail_file,"nstep=%ld steppart=%d\n",nstep,steppart);
  LOOPF_23{ // debug only
    dualfprintf(fail_file,"j=%d emf[0]=%21.15g emf[N1]=%21.15g diff=%21.15g\n",j,MACP1A1(fluxvec,1,0,j,k,B2),MACP1A1(fluxvec,1,N1,j,k,B2),MACP1A1(fluxvec,1,0,j,k,B2)-MACP1A1(fluxvec,1,N1,j,k,B2));
    //MACP1A1(fluxvec,1,N1,j,k,B2)=MACP1A1(fluxvec,1,0,j,k,B2);
    //    MACP1A1(fluxvec,2,N1,j,k,B1)=-MACP1A1(fluxvec,1,0,j,k,B2);

    //    MACP1A1(fluxvec,1,N1,j,k,B2)=-MACP1A1(fluxvec,2,N1,j,k,B1);
    //=-MACP1A1(fluxvec,2,N1,j,k,B1)=
  }
#endif



  //  firsttime=0;
  return(0);

}







/////////////////////////
//
// use global pbcorninterp and pvcorninterp at CORN1,2,3 to obtain EMFs at CORN1,2,3
// NO interpolation of quantities (except kinda wavespeeds) done here
//
// assumes 2-D Riemann problem
//
// When time for flux calculation:
//
// 1) Wavespeed calculation:
//
//    MACP2A0(wspeed,dir,CMIN,CMAX,i,j,k) : Use global wspeed located at FACE (as computed by flux_standard() and put at FACE by global_vchar())
//
//    CMIN,CMAX correspond to left,right going waves at FACEdir
//    In Del Zanna et al. (2003) equation 43-45, we have
//    alpha_x^+ = max(0,wspeed[dir corresponding to x][CMAX][index],wspeed[dir corresponding to x][CMAX][index-1 in y direction])
//    That is, wspeed located at FACE means only need to take max over 2 remaining speeds since already got wspeed by max,min of CENT speeds -- so realy are going over all 4 states for max,min each for each direction
//
// 2) Flux calculation
//
//    F1[B1]=F2[B2]=F3[B3]=0
//    F2[B3]=-F3[B2] (+-E1)
//    F1[B3]=-F3[B1] (+-E2)
//    F1[B2]=-F2[B1] (+-E3)
//
//    So only really 3 fluxes to be set corresponding to EMFs E1,E2,E3.
//    If 3D, then all fields use E in orthogonal plane (e.g. Bx uses d_2(E3) and d_3(E2), etc.)
//    If 2D in (say) x-y plane, then B1 only evolves by d_2(E3), B2 only evolves by d_1(E3), and B3 evolves by d_1(E2) and d_2(E1)
//    If 1D in (say) x-dir, then d_1(E3) evolves B2 and d_1(E2) evolves B3 and B1 doesn't evolve.  Hence if 1-D in x-dir don't need to compute E1
//    

// pvcorninterp and pbcorninterp are defined to be used like below initializaiton in set_arrays.c
//    for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=U1;pl<=U3;pl++) for(m=0;m<NUMCS;m++) for(l=0;l<NUMCS;l++)  MACP4A0(pvcorn,pl2,pl,m,l,i,j,k)=valueinit;
//    for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=B1;pl<=B3;pl++) for(l=0;l<NUMCS;l++)  MACP3A0(pbcorn,pl2,pl,l,i,j,k)=valueinit;

int fluxcalc_fluxctstag_emf_1d(int stage, FTYPE (*pr)[NSTORE2][NSTORE3][NPR], int dir, int odir1, int odir2, int is, int ie, int js, int je, int ks, int ke, FTYPE (*fluxvec[NDIM])[NSTORE2][NSTORE3][NPR], FTYPE CUf, struct of_loop (*face2cornloop)[NDIM],
			       FTYPE (*pbcorn)[COMPDIM][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
			       FTYPE (*pvcorn)[COMPDIM][NUMCS][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
			       FTYPE (*wspeed)[NUMCS][NSTORE1][NSTORE2][NSTORE3])
{
  int idel1,jdel1,kdel1;
  int idel2,jdel2,kdel2;
  int Nvec[NDIM];



  ///////////////////////////////////////////////
  //
  // get direction offsets for array accesses
  //
  ///////////////////////////////////////////////
  Nvec[0]=0;
  Nvec[1]=N1;
  Nvec[2]=N2;
  Nvec[3]=N3;

  idel1 = fluxloop[odir1][FIDEL];
  jdel1 = fluxloop[odir1][FJDEL];
  kdel1 = fluxloop[odir1][FKDEL];
  
  idel2 = fluxloop[odir2][FIDEL];
  jdel2 = fluxloop[odir2][FJDEL];
  kdel2 = fluxloop[odir2][FKDEL];

  

  //////////////////////////////////////
  //
  // flux loop : Extra "expand" zone for the purpose of averaging flux to get emf at corner.  Only used by field components, see flux_ct().
  // This loop is over interfaces where fluxes are evaluated -- atch
  //
  ////////////////////////////////////////

  //#if((SIMULBCCALC==2)&&(TYPE2==1))
  //  COMPFZLOOP(is,js,ks)
  //#else
  //#endif
  /////   COMPZSLOOP( is, ie, js, je, ks, ke ){ // slightly expanded compared to normal flux calculation due to needing emf that is for 2 different fluxes

#pragma omp parallel 
  {
    int i,j,k,pl,pliter;
    struct of_geom geomdontuse;
    struct of_geom *ptrgeom=&geomdontuse;
    int m,l;
    FTYPE emf2d[COMPDIM-1][COMPDIM-1],c2d[NUMCS][COMPDIM-1];
    FTYPE dB[COMPDIM-1],ctop[COMPDIM-1];
    FTYPE emffinal;
    FTYPE geomcornodir1,geomcornodir2;
    FTYPE topwave[COMPDIM-1],bottomwave[COMPDIM-1];
    OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);
    
    // generally ptr's are different inside parallel block
    ptrgeom=&geomdontuse;

#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);

    
      // pvcorninterp and pbcorninterp are defined to be used like below initializaiton in set_arrays.c
      //    for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=U1;pl<=U3;pl++) for(m=0;m<NUMCS;m++) for(l=0;l<NUMCS;l++)  MACP4A0(pvcorn,pl2,pl,m,l,i,j,k)=valueinit;
      //    for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=B1;pl<=B3;pl++) for(l=0;l<NUMCS;l++)  MACP3A0(pbcorn,pl2,pl,l,i,j,k)=valueinit;


      /////////////////////////////////
      //
      // pvcorn must contain velocity in same frame as field primitive in order to avoid interpolating yet another set of quantities (even can't do just 1 extra since 2 directions to interpolate from and no way to keep symmetry of interpolations without doing 2 extras and (e.g.) averaging)
      // assume final EMF being too large for the local field ($B^2-E^2>0$ or $\gamma^2\sim B^2/(B^2-E^2)$) will not be a problem since extra EMF at CORN would only increase field
      // not formally inconsistent since didn't interpolate other velocity or field to same location
      //
      //  for each edge/EMF (dir):
      //     1) There are 2 values for fields in each directions (odir1,odir2)
      //     2) There are 2x2 values for velocities in directions (odir1,odir2)
      //
      //     3) Need to work out where interpolated quantities go
      // 


      // unsure if signature and overall sign will be right -- GODMARK
      // i,j,k should already be taken into account (i.e. no offsets to add)
      // emf in dir direction
      for(m=0;m<NUMCS;m++) for(l=0;l<NUMCS;l++){
	  // emf[+- in odir1][+- in odir2]
	  // velocity in same positions as emf
	  // for example, emf3[+-x][+-y] = By[+-x]*vx[+-x][+-y] - Bx[+-y]*vy[+-x][+-y]
	  // below requires velocity to be lab-frame 3-velocity consistent with lab-frame 3-field primitive

	  // see fluxct.c for signature definition of EMF and flux

	  // m%3+1 gives next 1->2,2->3,3->1
	  // 3-(4-m)%3 = (dir+1)%3+1 gives previous 1->3,2->1,3->2
	  // so odir1 is forward cyclic
	  // so odir2 is backward cyclic
	  // notice that the emf in total uses 4 fields and 4*2 velocities for 12 total quantities
	  // not all pbcorn[][?] positions are used (have 3 only need 2 per dir)
	  // pvcorn[which corner][which component in pl form][+-odir1][+-odir2]
	  // pbcorn[which corner][which component in pl form][+-remaining direction that is not corn nor pl-dir]

	  emf2d[m][l] = 
	    + MACP3A0(pbcorn,dir,B1-1+odir2,m,i,j,k)*MACP4A0(pvcorn,dir,U1-1+odir1,m,l,i,j,k) 
	    - MACP3A0(pbcorn,dir,B1-1+odir1,l,i,j,k)*MACP4A0(pvcorn,dir,U1-1+odir2,m,l,i,j,k);
	}

      
      ///////////////////////////
      //
      // get wave speeds (these are not interpolated yet to CORNER, they start at FACE)
      // note wspeed still has sign information as set by global_vchar()
      // c[CMIN,CMAX][0=odir1,1=odir2]
      // need to determine i,j,k to choose based upon odir value
      // -?del? since going from FACE to CORN
      // del2 for c2d[][0] since wspeed[odir1] is wave going in odir1-direction, whereas other wavespeed to MAX with is in other (odir2) direction
      if(Nvec[odir1]>1){
	c2d[CMIN][0] = fabs(MAX(0.,MAX(-MACP2A0(wspeed,odir1,CMIN,i,j,k),-MACP2A0(wspeed,odir1,CMIN,i-idel2,j-jdel2,k-kdel2))));
	c2d[CMAX][0] = fabs(MAX(0.,MAX(+MACP2A0(wspeed,odir1,CMAX,i,j,k),+MACP2A0(wspeed,odir1,CMAX,i-idel2,j-jdel2,k-kdel2))));
      }
      else{
	// GODMARK: shoud just set the offending wavespeed (associated with non-existing dimensional direction) to 1.0 so don't have this conditional
	// then speed doesn't matter, not set, so scale-out
	c2d[CMIN][0] = 1.0;
	c2d[CMAX][0] = 1.0;
      }
      
      if(Nvec[odir2]>1){
	c2d[CMIN][1] = fabs(MAX(0.,MAX(-MACP2A0(wspeed,odir2,CMIN,i,j,k),-MACP2A0(wspeed,odir2,CMIN,i-idel1,j-jdel1,k-kdel1))));
	c2d[CMAX][1] = fabs(MAX(0.,MAX(+MACP2A0(wspeed,odir2,CMAX,i,j,k),+MACP2A0(wspeed,odir2,CMAX,i-idel1,j-jdel1,k-kdel1))));
      }
      else{
	// then speed doesn't matter, not set, so scale-out
	c2d[CMIN][1] = 1.0;
	c2d[CMAX][1] = 1.0;
      }

      ctop[0]    = MAX(c2d[CMIN][0],c2d[CMAX][0]);
      ctop[1]    = MAX(c2d[CMIN][1],c2d[CMAX][1]);


      //////////////////////////////////
      //
      // compute conserved dissipation term 
      //
      /////////////////////////////////


      // "upper" minus "lower" fields
      // dB[?] corresponds to ? meaning field direction, not interpolation or any other direction
      // dB[0] corresponds to dB["odir1"]
      dB[0] = MACP3A0(pbcorn,dir,B1-1+odir1,1,i,j,k) - MACP3A0(pbcorn,dir,B1-1+odir1,0,i,j,k) ;
      // dB[1] corresponds to dB["odir2"]
      dB[1] = MACP3A0(pbcorn,dir,B1-1+odir2,1,i,j,k) - MACP3A0(pbcorn,dir,B1-1+odir2,0,i,j,k) ;



      //////////////////////////////////
      //
      // compute emf
      //
      /////////////////////////////////

      // Del Zanna et al. (2003) has opposite sign in equations 44,45 since they define the electric field (E_i) whereas we define EMF=-gdet E_i
      // see fluxct.c comments
      // Sign of dissipative term is as for HLL flux, which since EMF and flux have different sign relationships for each direction/field, we have:
      // 
      // emf_1 = B^3 v^2 - B^2 v^3 = F2[B3] or -F3[B2]  : Dissipative terms: -(dB3) and +(dB2)
      // emf_2 = B^1 v^3 - B^3 v^1 = F3[B1] or -F1[B3]  : Dissipative terms: -(dB1) and +(dB3)
      // emf_3 = B^2 v^1 - B^1 v^2 = F1[B2] or -F2[B1]  : Dissipative terms: -(dB2) and +(dB1)
      //
      // So since dissipative term order is EMF[edgedir] += dB[odir1]  + dB[odir2]
      // then we have:
      // edgedir=1 odir1=2 odir2=3 , so need -dB[1] and +dB[0]
      // edgedir=2 odir1=3 odir2=1 , so need -dB[1] and +dB[0]
      // edgedir=3 odir1=1 odir2=2 , so need -dB[1] and +dB[0]
      // so same formula for all EMFs


      // below topwave[] are positive definite but could be 0.0 and then HLLFLUX formula not right
      topwave[0]=c2d[CMIN][0] + c2d[CMAX][0];
      topwave[1]=c2d[CMIN][1] + c2d[CMAX][1];


      if( (fluxmethod==HLLFLUX) && (topwave[0]>SMALL) && (topwave[1]>SMALL) ){

	bottomwave[0]=1.0/topwave[0];
	bottomwave[1]=1.0/topwave[1];

	// HLL
	emffinal = 
	  // non-dissipative term
	  + (
	     +c2d[CMAX][0]*c2d[CMAX][1]*emf2d[0][0]  // emf has -odir1 -odir2, so wavespeed has +odir1 +odir2
	     +c2d[CMAX][0]*c2d[CMIN][1]*emf2d[0][1]  // emf has -odir1 +odir2, so wavespeed has +odir1 -odir2
	     +c2d[CMIN][0]*c2d[CMAX][1]*emf2d[1][0]  // emf has +odir1 -odir2, so wavespeed has -odir1 +odir2
	     +c2d[CMIN][0]*c2d[CMIN][1]*emf2d[1][1]  // emf has +odir1 +odir2, so wavespeed has -odir1 -odir2
	     )*bottomwave[0]*bottomwave[1]
	  // dissipative terms
	  - (
	     // dB has d(B[odir2]) so wavespeed has +-odir1  (note d(B[odir1]) for +-odir1 is 0 due to divb=0) (i.e. otherwise would be 4 dissipation terms for 2D Riemann problem)
	     c2d[CMIN][0]*c2d[CMAX][0]*bottomwave[0]*dB[1]
	     )
	  + (
	     // dB has d(B[odir1]) so wavespeed has +-odir2  (note d(B[odir2]) for +-odir2 is 0 due to divb=0) (i.e. otherwise would be 4 dissipation terms for 2D Riemann problem)
	     c2d[CMIN][1]*c2d[CMAX][1]*bottomwave[1]*dB[0]
	     )
	  ;
      }
      else{ // assume if not HLL then LAXF since only 2 methods for this routine
	// LAXF
	// dB and ctop flip odir's due to divb=0(i.e. otherwise would be 4 dissipation terms for 2D Riemann problem)
	emffinal =
	  + 0.25*(emf2d[0][0]+emf2d[0][1]+emf2d[1][0]+emf2d[1][1])
	  - 0.50*(ctop[0]*dB[1] - ctop[1]*dB[0]);
      }

      //////////////////////////////////
      //
      // set the fluxes (final flux is expected to have geometry on it)
      //
      /////////////////////////////////

      // B inside pbcorn was setup with geometry
      // assume that if on singularity where gdet=0 then interpolation gave good value on singularity
      // GODMARK: seems like issue with choice of how to treat gdet is related to presence (or not) of line current on singularity
      //          For example, for POLARAXIS E_z will not be 0 if first used gdet and then interpolated, while if used gdet afterwards then will be 0
      //          Same ambiguity exists in fluxct.c
#if(CORNGDETVERSION==1)
      // even though only want geom.e, geom.e calculation could depend upon many things
      get_geometry_geomeonly(i, j, k, CORN1-1+dir, ptrgeom); // get geometry at CORN[dir] where emf is located

      // GODMARK: why use ptrgeom->e here and ptrgeom->g in most other places?
      geomcornodir1=ptrgeom->EOMFUNCMAC(B1-1+odir1); // which field ptrgeom->e doesn't matter as mentioned below
      geomcornodir2=ptrgeom->EOMFUNCMAC(B1-1+odir2);
#else
      geomcornodir1=1.0;
      geomcornodir2=1.0;
#endif

      // see fluxct.c for definitions of signature
      // e.g. edgedir=3 gives F[1][B2]=E3 and F[2][B1]=-E3  F[3][B3]=0, which is correct.
      // Checked that correct for all edgedir's
      // notice that geometries for field-type primitive must all be same for field since otherwise emf ptrgeom->e factor has a mixed association
      if(Nvec[odir1]>1) MACP1A1(fluxvec,odir1,i,j,k,B1-1+odir2) = + emffinal*geomcornodir2;
      if(Nvec[odir2]>1) MACP1A1(fluxvec,odir2,i,j,k,B1-1+odir1) = - emffinal*geomcornodir1;
      if(Nvec[dir]>1) MACP1A1(fluxvec,dir,i,j,k,B1-1+dir)     =   0.0;

#if((WHICHEOM==WITHNOGDET)&&(NOGDETB1>0)||(NOGDETB2>0)||(NOGDETB3>0))
      dualfprintf(fail_file,"Makes no sense to have field with different geometry factor since flux,emf mixes those factors\n");
      myexit(196826);
#endif



#if(0)
      if(i==N1 || i==0){
	dualfprintf(fail_file,"dir=%d i=%d EMF=%21.15g\n",dir,i,emffinal);
      }
#endif





#if(0)
      // DEBUG:
      if(i<=0){
	for(m=0;m<NUMCS;m++) for(l=0;l<NUMCS;l++){
	    dualfprintf(fail_file,"i=%d j=%d k=%d m=%d l=%d :: dir=%d odir1=%d odir2=%d :: emf2d[m][l]=%21.15g :: %21.15g %21.15g %21.15g %21.15g\n",i,j,k,m,l,dir,odir1,odir2,emf2d[m][l],MACP3A0(pbcorn,dir,B1-1+odir2,m,i,j,k),MACP4A0(pvcorn,dir,U1-1+odir1,m,l,i,j,k),MACP3A0(pbcorn,dir,B1-1+odir1,l,i,j,k),MACP4A0(pvcorn,dir,U1-1+odir2,m,l,i,j,k));
	  }
	dualfprintf(fail_file,"dB[0]=%21.15g dB[1]=%21.15g\n",dB[0],dB[1]);
	if(dir==3){
	  // below are things used to get pbcorn[3][B2][m=1] at 0,0,0
	  dualfprintf(fail_file,"pstag[B2]= %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.51g\n",MACP0A1(pstagglobal,-3,0,0,B2),MACP0A1(pstagglobal,-2,0,0,B2),MACP0A1(pstagglobal,-1,0,0,B2),MACP0A1(pstagglobal,0,0,0,B2),MACP0A1(pstagglobal,1,0,0,B2),MACP0A1(pstagglobal,2,0,0,B2),MACP0A1(pstagglobal,3,0,0,B2));
	  dualfprintf(fail_file,"pl_ct[2][B2]= %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.51g\n",MACP1A1(pl_ct,2,-3,0,0,B2),MACP1A1(pl_ct,2,-2,0,0,B2),MACP1A1(pl_ct,2,-1,0,0,B2),MACP1A1(pl_ct,2,0,0,0,B2),MACP1A1(pl_ct,2,1,0,0,B2),MACP1A1(pl_ct,2,2,0,0,B2),MACP1A1(pl_ct,2,3,0,0,B2));
	  dualfprintf(fail_file,"pr_ct[2][B2]= %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.51g\n",MACP1A1(pr_ct,2,-3,0,0,B2),MACP1A1(pr_ct,2,-2,0,0,B2),MACP1A1(pr_ct,2,-1,0,0,B2),MACP1A1(pr_ct,2,0,0,0,B2),MACP1A1(pr_ct,2,1,0,0,B2),MACP1A1(pr_ct,2,2,0,0,B2),MACP1A1(pr_ct,2,3,0,0,B2));
	  dualfprintf(fail_file,"BADFLUX: %21.15g %21.15g\n",MACP1A1(fluxvec,odir1,i,j,k,B1-1+odir2),MACP1A1(fluxvec,odir2,i,j,k,B1-1+odir1));
	}
	

	// m=1 means odir2 on upper side (for B2 this is same as m=0)
	// so MACP3A0(pbcorn,3,B2,m=1,0,0,0) is nan
	// so MACP4A0(pvcorn,3,U1,m=1,0,1,0,0,0) is nan
	//
	// so MACP3A0(pbcorn,3,B1,l=0,1,0,0,0) is fine
	// so MACP4A0(pvcorn,3,U2,m=0,1,l=0,1,0,0,0) is fine
 

	//i=0 j=0 k=0 m=0 l=0 :: dir=3 odir1=1 odir2=2 :: emf2d[m][l]=                   -0 ::                     0    -0.452342295124171                     0                     0
	//i=0 j=0 k=0 m=0 l=1 :: dir=3 odir1=1 odir2=2 :: emf2d[m][l]=                   -0 ::                     0    -0.452342295124171                     0                     0
	//i=0 j=0 k=0 m=1 l=0 :: dir=3 odir1=1 odir2=2 :: emf2d[m][l]=                  nan ::                   nan                   nan                     0                     0
	//i=0 j=0 k=0 m=1 l=1 :: dir=3 odir1=1 odir2=2 :: emf2d[m][l]=                  nan ::                   nan                   nan                     0                     0
	//BADFLUX:                   nan                   nan

      }
#endif

      // done!

    }// end 3D LOOP
  }// end parallel region





#if(0)
  bound_flux(STAGEM1,t,fluxvec[1],fluxvec[2],fluxvec[3], 0);
  if(N1>1) bound_prim(STAGEM1,t,fluxvec[1], 0);
  if(N2>1) bound_prim(STAGEM1,t,fluxvec[2], 0);
  if(N3>1) bound_prim(STAGEM1,t,fluxvec[3], 0);
#endif




  return (0);
}






// wrapper for *continuous* interpolation for FACE_to_CENT
void slope_lim_continuous_e2z(int realisinterp, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*dq)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP], struct of_loop *face2centloop)
{
  extern void slope_lim_linetype_c2e(int realisinterp, int whichprimtype, int interporflux, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*stencilvar)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP]);
  extern void slope_lim_pointtype(int interporflux, int realisinterp, int pl, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*dq)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP]);
  int pl,pliter;




  if( LINEINTERPTYPE(lim[dir]) ){ // this overrides lim, but lim must still be set properly
    get_loop(INTERPLINETYPE, ENOINTERPTYPE, dir, face2centloop);

    // 1 below means primitives instead of conserved quantities (used for loops)
    // GODMARK: ENOMARK: pstag below may only contain field so can't be used for pressure indicator
    // GODMARK: set_interppoint() inside this function sets starting and ending position for loops, and as set c2e always needs more than e2c for obtaining flux, so leaving as for c2e is fine for now
    slope_lim_linetype_c2e(realisinterp, ENOPRIMITIVE, ENOINTERPTYPE, dir, idel, jdel, kdel, primreal, NULL, p2interp, pleft, pright);
      
    // ENOMARK: for ENO should really  use special _e2c_cont() function that assumes continuity is required GODMARK
    //    slope_lim_linetype_e2c_cont(realisinterp, ENOPRIMITIVE, ENOINTERPTYPE, dir, idel, jdel, kdel, primreal, NULL, p2interp, pcent);
  }
  else{
    // Should really interpolate such that continuous GODMARK
    // GODMARK: set_interppoint() inside this function sets starting and ending position for loops, and as set c2e always needs more than e2c for obtaining flux, so leaving as for c2e is fine for now
    get_loop(INTERPPOINTTYPE, ENOINTERPTYPE, dir, face2centloop);
    PINTERPLOOP(pliter,pl){
      slope_lim_pointtype(ENOINTERPTYPE, realisinterp, pl, dir, idel, jdel, kdel, primreal, p2interp, dq, pleft, pright); // GODMARK: overwritting dq from other type of interpolation
    }
  }

#if(0)
  bound_prim(STAGEM1,t,pleft, 0);
  bound_prim(STAGEM1,t,pright, 0);
#endif

}





// wrapper for taking staggered conserved field quantity and obtaining centered field quantity
// upoint enters as stag and leaves as CENT
// once this function is done we have:
// 1) pstag will have correct staggered point value
// 2) upoint (if needed) will be replaced with center conserved value
// 3) pfield will contain centered field primitive value
// Note that no averaging or deaveraging occurs in this function -- everything is points
int interpolate_ustag2fieldcent(int stage, SFTYPE boundtime, int timeorder, int numtimeorders, FTYPE (*preal)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*upoint)[NSTORE2][NSTORE3][NPR],FTYPE (*pcent)[NSTORE2][NSTORE3][NPR])
{
  int ustagpoint2pstag(FTYPE (*upoint)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR]);
  int interpolate_pfield_face2cent(FTYPE (*preal)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*ucent)[NSTORE2][NSTORE3][NPR],FTYPE (*pcent)[NSTORE2][NSTORE3][NPR], struct of_loop *face2centloop, FTYPE (*dqvec[NDIM])[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*prc)[NSTORE2][NSTORE3][NPR], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP], int *Nvec);
  struct of_loop face2cent[NDIM];
  int finalstep;
  FTYPE (*dqvec[NDIM])[NSTORE2][NSTORE3][NPR2INTERP];
  int Nvec[NDIM];



#if(0)
  bound_prim(STAGEM1,boundtime,upoint, 0);
#endif



  // setup spatial sizes
  Nvec[0]=0;
  Nvec[1]=N1;
  Nvec[2]=N2;
  Nvec[3]=N3;


  // setup dq's if needed
  // called by advance() that doesn't have dq1,dq2,dq3 passed to it (yet), so access global dq1,dq2,dq3
  dqvec[1]=GLOBALPOINT(dq1);
  dqvec[2]=GLOBALPOINT(dq2);
  dqvec[3]=GLOBALPOINT(dq3);



  // Actually updates field using conserved update (and inverts field)
  // next use of pstagscratch by fluxcalc() will be correct (see  setup_rktimestep() commends for RK4)
  ustagpoint2pstag(upoint,pstag);


  if(timeorder==numtimeorders-1) finalstep=1; else finalstep=0;




  /////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // Bound pstag
  //
  // must bound pstag at FACE1,2,3 so enough boundary zones to interpolate to get field at CENT
  // note that unlike other conserved quantities, since field trivially inverted there is no issue with possible failure modes or fixups, so only need to bound once
  // That is, we always update field as required for divb=0
  // bound_pstag() takes care of which quantities to bound (only bounding B1,B2,B3)

  bound_pstag(stage, boundtime, preal, pstag, upoint, finalstep);

  // note that ustag isn't bounded, but is used for divb calculation, which is thus only valid at active CENT cells -- but that's all that's in normal dumps unless FULLOUTPUT is used


  // pstagescratch should contain result of deaverage_ustag2pstag() -- gets utoinvert ready for inversion using guess pb
  // other quantities in utoinvert are unchanged by this function (and if needed de-averaging this didn't do it!)
  interpolate_pfield_face2cent(preal,pstag,upoint,pcent,face2cent,dqvec,GLOBALPOINT(prc),GLOBALPOINT(pleft),GLOBALPOINT(pright),Nvec);


#if(0)
  bound_prim(STAGEM1,boundtime,pcent);
#endif

  // pstagscratch is at FACE1,2,3 and is primary field variable
  // pfield is at CENT and is dependent field variable
  //
  // This means pstagscratch must be bounded as a flux1,2,3
  // In MPI use pstag with boundflux()
  // In user bound, user must treat field differently knowing where field is located

  // pfield at CENT does NOT need to be bounded itself since pstag is bounded and then interpolated in extended off-dir range in interpolate_pfield_face2cent() that uses slope_lim_continuous_e2z()

  // So only need to bound pstag like a flux -- need to bound before FACE_2_CENT interpolation


  // now have field at CENT wherever needed for both EMF at CORN1,2,3  AND  have enough for normal fluxes at faces
  // Note don't use CENT field to get field in direction of flux of any quantity -- revert to existing staggered value
  // This is why don't need to bound the field at CENT




  return(0);

}







#define USTAG2PSTAGCONSTRAINEDLOOP 1 // should be 1

// this function called in initbase.c and during advance()
// here ustag is point value
// if dimension doesn't exist, then ustag and pstag are really at CENT effectively
int ustagpoint2pstag(FTYPE (*ustag)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR])
{
  void ustag2pstag(int dir, int i, int j, int k, FTYPE (*ustag)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR]);
  int Nvec[NDIM];
  

  Nvec[0]=0;
  Nvec[1]=N1;
  Nvec[2]=N2;
  Nvec[3]=N3;


  //////////////////////////
  //
  // get pstag from unew
  // p should be de-averaged field at FACE1,2,3
  //
  ////////////////////////////



#pragma omp parallel 
  {
    extern void get_stag_startendindices(int dir, int *is,int *ie,int *js,int *je,int *ks,int *ke);
    int i,j,k,pl,pliter;
    int dir;
    FTYPE igdetgnosing;
    int is,ie,js,je,ks,ke;
    struct of_geom geomfdontuse[NDIM];
    struct of_geom *ptrgeomf[NDIM];
    int jj;

    OPENMP3DLOOPVARSDEFINE;


    // generally ptr's are different inside parallel block
    DLOOPA(jj) ptrgeomf[jj]=&(geomfdontuse[jj]);


#if(USTAG2PSTAGCONSTRAINEDLOOP==0)


    // Not well-constrained
    DIMENLOOP(dir){
      pl=B1-1+dir;

      OPENMP3DLOOPSETUPFULL;
      /////////    COMPFULLLOOP{
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait // "nowait" ok because each pstag[B1,B2,B3] set independently
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);
      
	// get geometry for face pre-interpolated values
	get_geometry_gdetonly(i, j, k, FACE1-1+dir, ptrgeomf[dir]); // FACE1,FACE2,FACE3 each
	set_igdetsimple(ptrgeomf[dir]);
	igdetgnosing = ptrgeomf[dir]->igdetnosing;      
	MACP0A1(pstag,i,j,k,pl)  = MACP0A1(ustag,i,j,k,pl)*igdetgnosing;
      }
    }



#else // else if constrained loops



    // well-constrained to only update pstag where really want update
    // This ensures don't modify pstag outside well-defined box that defines current computational space
    // do pl==B1
    pl=B1;
    dir=1;
    get_stag_startendindices(dir, &is,&ie,&js,&je,&ks,&ke);
    //  dualfprintf(fail_file,"dir=%d is=%d ie=%d js=%d je=%d ks=%d ke=%d\n",dir,is,ie,js,je,ks,ke);
    //////  COMPZSLOOP(is,ie,js,je,ks,ke){
    OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait // "nowait" ok because each pstag[B1,B2,B3] set independently
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);
    
      ustag2pstag(dir, i, j, k, ustag,pstag);
    }



    // do pl==B2
    pl=B2;
    dir=2;
    get_stag_startendindices(dir, &is,&ie,&js,&je,&ks,&ke);
    //  dualfprintf(fail_file,"dir=%d is=%d ie=%d js=%d je=%d ks=%d ke=%d\n",dir,is,ie,js,je,ks,ke);
    //////  COMPZSLOOP(is,ie,js,je,ks,ke){
    OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);

      ustag2pstag(dir, i, j, k, ustag,pstag);
    }



    // do pl==B3
    pl=B3;
    dir=3;
    get_stag_startendindices(dir, &is,&ie,&js,&je,&ks,&ke);
    //  dualfprintf(fail_file,"dir=%d is=%d ie=%d js=%d je=%d ks=%d ke=%d\n",dir,is,ie,js,je,ks,ke);
    //////  COMPZSLOOP(is,ie,js,je,ks,ke){
    OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);

      ustag2pstag(dir, i, j, k, ustag,pstag);
    }


#endif
  }// end parallel region (with implicit barrier)






#if(0)
  bound_prim(STAGEM1,t,pstag);
#endif


  return(0);
}





// OPTMARK: presume this function gets inlined
void ustag2pstag(int dir, int i, int j, int k, FTYPE (*ustag)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR])
{
  FTYPE igdetgnosing;
  int pl,pliter;
  struct of_geom geomfdontuse[NDIM];
  struct of_geom *ptrgeomf[NDIM];
  int jj;

  // assign memory
  DLOOPA(jj) ptrgeomf[jj]=&(geomfdontuse[jj]);


  pl=B1-1+dir;

  // get geometry for face pre-interpolated values
  get_geometry_gdetonly(i, j, k, FACE1-1+dir, ptrgeomf[dir]); // FACE1,FACE2,FACE3 each
  set_igdetsimple(ptrgeomf[dir]);
  igdetgnosing = ptrgeomf[dir]->igdetnosing;      
  MACP0A1(pstag,i,j,k,pl)  = MACP0A1(ustag,i,j,k,pl)*igdetgnosing;


}






// if not rescaling, then default is to interpolate \detg B^i rather than B^i -- more accurate for field-aligned flows (e.g. monopole)
#define IFNOTRESCALETHENUSEGDET 1 // should be 1

// wrapper for rescale() used for staggered field
static void rescale_calc_stagfield_full(int *Nvec, FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR])
{



#pragma omp parallel OPENMPGLOBALPRIVATEFORSTATEANDGEOMINTERPFULLNPR2INTERP // requires full copyin() changes npr2interp stuff
  {
    int i,j,k;
    int jj;
    int pl,dir;
    struct of_geom geomfdontuse[NDIM];
    struct of_geom *ptrgeomf[NDIM];
    extern int rescale(int which, int dir, FTYPE *pr, struct of_geom *geom,FTYPE*newvar);

    OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUPFULL;


    // generally ptr's are different inside parallel block
    // assign memory
    DLOOPA(jj) ptrgeomf[jj]=&(geomfdontuse[jj]);


    // LOOP over faces (dimensions)
    DIMENLOOP(dir){ // dimen loop because rescale() is at different locations
      //    odir1=dir%3+1;
      //    odir2=(dir+1)%3+1;
    
      //    if(Nvec[dir]==1 || (Nvec[dir]!=1 && (Nvec[odir1]==1 && Nvec[odir2]==1) ) ) continue; // later will copy if no such dimension or such dimension but other dimensions don't exist so field must be constant
      if(Nvec[dir]==1) continue; // later will copy if no such dimension


      pl=B1-1+dir;
      npr2interpstart=npr2interpend=0; npr2interplist[0]=pl; // B1,B2,B3 each


#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait // p2interp[B1,B2,B3] set independently
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);


#if(RESCALEINTERP)
	// get geometry for face pre-interpolated values
	get_geometry(i, j, k, FACE1-1+dir, ptrgeomf[dir]); // FACE1,FACE2,FACE3 each
	rescale(1,dir,MAC(pstag,i,j,k),ptrgeomf[dir],MAC(p2interp,i,j,k));
#elif(IFNOTRESCALETHENUSEGDET)
	// get geometry for face pre-interpolated values
	get_geometry_gdetonly(i, j, k, FACE1-1+dir, ptrgeomf[dir]); // FACE1,FACE2,FACE3 each
	MACP0A1(p2interp,i,j,k,pl) = (ptrgeomf[dir]->gdet)*MACP0A1(pstag,i,j,k,pl);
#endif
	
      }// end COMPFULLLOOP
    }// end over dirs
  }// end parallel region (with implicit barrier)


}










// interpolates field at FACE to field at CENT assuming field along itself is *continuous*

// |=interface
// i=zone center of ith zone
//
// |            |         i                 |
// |         pstag(i)     i                 |
// |            |         i                 |
// |         p2interp(i)  i                 |
// |            |         i                 |
// |          dq(i)       i                 |
// |            |         i                 |
// |pleft(i)    |     pright(i) pleft(i+1)  |    //  notice odd pleft(i) position
// |            |         i                 |
// |            |      p_l p_r              |
// |            |         i                 |
// |            |        pcent(i)           |
// |            |         i                 |
//
// exactly correct for arbitrary order since input is pstag (point quantity) and output is ucent,pcent (point quantities)
// ustag only used to get non-field ucent at end of function... otherwise use pstag
// ucent has ONLY field values updated since ucent may already contain correct values for other quantities (means if not, then outside this function must set ucent to something before using full quantity set!)
// this function called in initbase.c and during advance()
//
// INPUTS: Nvec, preal, pstag[B1,B2,B3]
// OUTPUTS: ucent[B1,B2,B3] (if not NULL), pcent[B1,B2,B3], face2centloop
// TEMPVARS: dqvec(dir,{B1,B2,B3}), prc[B1,B2,B3], pleft[B1,B2,B3], pright[B1,B2,B3]
// OPENMPOPTMARK: pleft,pright only exist per dir but used for all dirs, so can't use  nowait when involving pleft/pright
// We don't use wavespeeds here, so don't worry about wspeedtemp being only for one dir
// 
//
int interpolate_pfield_face2cent(FTYPE (*preal)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*ucent)[NSTORE2][NSTORE3][NPR],FTYPE (*pcent)[NSTORE2][NSTORE3][NPR], struct of_loop *face2centloop, FTYPE (*dqvec[NDIM])[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*prc)[NSTORE2][NSTORE3][NPR], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP], int *Nvec)
{
  void slope_lim_continuous_e2z(int realisinterp, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*dq)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP], struct of_loop *face2centloop);
  extern int choose_limiter(int dir, int i, int j, int k, int pl);
  FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP];
  int nprlocalstart,nprlocalend;
  int nprlocallist[MAXNPR];



  //////////////////////////////////////
  // PROCEDURE:
  // 1) save interp list
  // 2) setup which quantities to interpolate
  // 3) rescale pstag (no need to unrescale since have separate memory for p2interp)
  // 4) interpolate rescaled pstag (p2interp)
  // 5) obtain pcent from dq or pleft/pright for simple interpolation/averaging method
  // 6) copy fields if not interpolated/rescaled
  // 7) restore interp list

  // GODMARK: in 1D don't have to interpolate face2cent for field in dir-direction since has to be constant

  ////////////////////////////////////////////
  //
  // save choice for interpolations so can restore global variables after done
  {
    int pl,pliter;
    nprlocalstart=npr2interpstart;
    nprlocalend=npr2interpend;
    PMAXNPRLOOP(pl) nprlocallist[pl]=npr2interplist[pl];
  }


  //////////////////////////
  //
  // rescale before interpolation
  //
  ////////////////////////////
  if(RESCALEINTERP || IFNOTRESCALETHENUSEGDET){
    p2interp=prc; // it's different
    // rescale or multiply by \sqrt{-g}
    rescale_calc_stagfield_full(Nvec, pstag,p2interp);
  }
  else{
    p2interp=pstag; // it's itself
  }




  //////////////////////////
  //
  // LOOP OVER FACES for each dimension
  //
  // Peform interpolation on point quantity (even for ENO/FV method a2c_perps would be used before here)
  //
  ////////////////////////////

#pragma omp parallel OPENMPGLOBALPRIVATEFORGEOMNPR2INTERP
  {
    int pl,pliter;
    int idel,jdel,kdel;
    int i,j,k;
    int dir;
    int locallim;
    FTYPE pstore_l[NPR2INTERP],pstore_r[NPR2INTERP];
    FTYPE *p2interp_l,*p2interp_r;
    FTYPE p_l[NPR2INTERP], p_r[NPR2INTERP];
    int usedq;
    int realisinterp;
    int odir1,odir2;
    FTYPE igdetgnosing;
    int is,ie,js,je,ks,ke;
    struct of_geom geomcdontuse;
    struct of_geom *ptrgeomc=&geomcdontuse;
    struct of_geom geomfdontuse[NDIM];
    struct of_geom *ptrgeomf[NDIM];
    int jj;

    OPENMP3DLOOPVARSDEFINE;

    // assign memory
    DLOOPA(jj) ptrgeomf[jj]=&(geomfdontuse[jj]);

    // Setup rescale pointer reference
#if(RESCALEINTERP)
    p2interp_l=pstore_l; // then need separate storage
    p2interp_r=pstore_r;
#else
    p2interp_l=p_l; // p2interp_? is final p_?
    p2interp_r=p_r;
#endif




    // GODMARK:  for now use standard interpolation and just force continuity by averaging (probably leads to unstable algorithm)
    DIMENLOOP(dir){

      //   odir1=dir%3+1;
      //   odir2=(dir+1)%3+1;
    
      ////////////
      //
      // for TESTNUMBER 1002, stag, recon, below causes peak of B2/B3 to be smaller -- NO IDEA WHY SUPERGODMARK
      // for same test, stag, fv method has small peak even with Nvec[dir]==1 only
      //
      //if(Nvec[dir]==1 || (Nvec[dir]!=1 && (Nvec[odir1]==1 && Nvec[odir2]==1) ) ) continue; // later will copy if no such dimension or such dimension but other dimensions don't exist so field must be constant
      //
      ////////////


      // setup which quantity to interpolate
      pl = B1-1+dir;
      npr2interpstart=npr2interpend=0; npr2interplist[0]=pl; // B1,B2,B3 each for each dir=1,2,3




      if(Nvec[dir]==1){


	//////////////////////////
	//
	// Copy if no such dimension or such dimension but other dimensions don't exist so field must be constant
	// copy over those things not interpolating (and hence not rescalig either)
	// unlike FACE_to_EDGE, these degenerate cases result in a trivial copy from the fake FACE to CENT
	// what's constant is \detg B^i, not B^i, so need geometry if field along 1D dir with no other dimensions
	// no idel,jdel,kdel for simplicity
	OPENMP3DLOOPSETUPFULL;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait // don't wait for each dir since independent and all memory written to (and used in later loops) is independent for each dir.
	OPENMP3DLOOPBLOCK{
	  OPENMP3DLOOPBLOCK2IJK(i,j,k);

	  MACP0A1(pcent,i,j,k,pl)=MACP0A1(pstag,i,j,k,pl);

	  // get ucent if required
	  if(ucent!=NULL){// OPTMARK: Is this expensive?
	    // Note: If WHICHEOM==WITHNOGDET and turned off \detg for fields, then staggered method doesn't work, so ok to assume gdet below and assume standard primitive field such that \detg B^i = conserved quantity
	    get_geometry(i, j, k, CENT, ptrgeomc); // final quantity is at CENT
	    MACP0A1(ucent,i,j,k,pl)  = MACP0A1(pcent,i,j,k,pl)*(ptrgeomc->gdet); // exactly correct (even for ENO/FV)
	  }// end if ucent!=NULL

	}// end 3D LOOP




      }
      else{ // Then Nvec[dir]!=1, so should treat field generally along this direction


   
	// get loop details
	idel = fluxloop[dir][FIDEL];
	jdel = fluxloop[dir][FJDEL];
	kdel = fluxloop[dir][FKDEL];

	//////////////////////
	// interpolate    
	//////////////////////
	realisinterp=0; // since only dealing with fields
	slope_lim_continuous_e2z(realisinterp, dir, idel, jdel, kdel, preal, p2interp, dqvec[dir], pleft, pright, &(face2centloop[dir]));

  
	///////////////////
	// get p_l p_r
	//////////////////////////////////////
	//
	// interpolate primitive using slope (dq) or directly from pleft and pright
	// For FV: p_left, p_right (indexed by grid cell #) -> p2interp_l, p2interp_r (indexed by interface #) -- atch comment
	//
	// always do since p2interp is good and dq/pleft/pright should have stored quantities or just-computed quantities
	/////////////////////////////////////

	// Assume for now that limiter is not per i,j,k but only per dir (unlike normal interpolation)
	// no HORIZONSUPERFAST here
	locallim=choose_limiter(dir, 0,0,0,pl);
	usedq = (locallim<PARA)&&(LIMADJUST==0);
    
	// using COMPZSLOOP since final CENT quantity is used wherever centered primitive is needed for flux (which is sometimes transverse direction)
	// do maximal loop but avoid going out of bounds when accessing dqvec,pleft,pright
	// NOW do constrained loop
	get_advance_startendindices(&is,&ie,&js,&je,&ks,&ke);
	////////      COMPZSLOOP(is,ie,js,je,ks,ke){
	OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) ///nowait // don't wait since each dir is independent (NO: pleft,pright not functions of dir, so each dir not independent)
	OPENMP3DLOOPBLOCK{
	  OPENMP3DLOOPBLOCK2IJK(i,j,k);



	  // note that interpolation from FACE_to_CENT is different than from FACE_to_EDGE or CENT_to_FACE
	  // if not rescaling or auto-gdet rescaling, then p2interp_? is already really p_?, otherwise p2interp_? is separate memory from p_?
	  if(usedq){
	    p2interp_l[pl] = MACP0A1(p2interp,i,j,k,pl) + 0.5 * MACP1A1(dqvec,dir,i,j,k,pl);
	    p2interp_r[pl] = MACP0A1(p2interp,i + idel,j + jdel,k + kdel,pl) - 0.5 * MACP1A1(dqvec,dir,i + idel,j + jdel,k + kdel,pl);
	  }
	  else{
	    p2interp_l[pl] = MACP0A1(pright,i,j,k,pl);
	    p2interp_r[pl] = MACP0A1(pleft,i+idel,j+jdel,k+kdel,pl);
	  }


	


#if(RESCALEINTERP)
	  /////////////////////////////////////
	  // after interpolation, unrescale from p2interp to normal primitive 
	  get_geometry(i, j, k, CENT, ptrgeomc); // final quantity is at CENT
	  rescale(-1,dir,p_l,ptrgeomc,p2interp_l);
	  rescale(-1,dir,p_r,ptrgeomc,p2interp_r);

	  if(ucent!=NULL) MACP0A1(ucent,i,j,k,pl)  = 0.5*(p2interp_l[pl]+p2interp_r[pl])*(ptrgeomc->gdet); // exactly correct (even for ENO/FV)

#elif(IFNOTRESCALETHENUSEGDET)
	  get_geometry_gdetonly(i, j, k, CENT, ptrgeomc); // final quantity is at CENT
	  set_igdetsimple(ptrgeomc);
	  igdetgnosing=ptrgeomc->igdetnosing;

	  // Assign \detg B^i
	  if(ucent!=NULL) MACP0A1(ucent,i,j,k,pl)=0.5*(p2interp_l[pl]+p2interp_r[pl]); // go ahead and assign ucent if this method

	  // now get B^i from \detg B^i
	  // remove \detg used during interpolation
	  p_l[pl] = p2interp_l[pl]*igdetgnosing;
	  p_r[pl] = p2interp_r[pl]*igdetgnosing;

#else
	  if(ucent!=NULL){
	    // otherwise p2interp_{l,r} are really just pointing to p_l and p_r
	    get_geometry(i, j, k, CENT, ptrgeomc); // final quantity is at CENT
	    // Note: If WHICHEOM==WITHNOGDET and turned off \detg for fields, then staggered method doesn't work, so ok to assume gdet below and assume standard primitive field such that \detg B^i = conserved quantity
	    MACP0A1(ucent,i,j,k,pl)  = 0.5*(p2interp_l[pl]+p2interp_r[pl])*(ptrgeomc->gdet); // exactly correct (even for ENO/FV)
	  }
	  
#endif


      
	  // now set pcent -- GODMARK -- should interpolate such that only 1 continuous value
	  // Must preserve divb in 1D Riemann problem, so B^{dir} must be continuous
	  // Yes, should use larger stencil and interpolate such that constant. Essentially choose
	  // large stencil and effectively choosing which points we trust (not necessarily local points)
	  MACP0A1(pcent,i,j,k,pl)=0.5*(p_l[pl]+p_r[pl]);


	}// endCOMPZSLOOP

      }// end if N>1

    }// end DIMENLOOP
  }// end parallel region (with implicit barrier)  






  ////////////////////////////////////////////
  //
  // restore choice for interpolations from global variables
  {
    int pl,pliter;
    npr2interpstart=nprlocalstart;
    npr2interpend=nprlocalend;
    PMAXNPRLOOP(pl) npr2interplist[pl]=nprlocallist[pl];
  }


#if(0)
  bound_prim(STAGEM1,t,pcent);
  if(ucent!=NULL) bound_prim(STAGEM1,t,ucent);
#endif

  return(0);

}






// slope_lim_face2corn() is provided p2interp and returns pleft/pright
// differs from slope_lim() by range over which quantities required
//
// |=interface
// i=zone center of ith zone
//
// |              |      p2interp(i)   |
// |         pl(i)|pr(i)    i          |
// |         Fl(i)|Fr(i)    i          |
// |         Ul(i)|Ur(i)    i          |
// |              |pleft(i)   pright(i)|
// |              |F(i)                |
//
void slope_lim_face2corn(int realisinterp, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*dq)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP], struct of_loop *face2cornloop)
{
  extern void slope_lim_linetype_c2e(int realisinterp, int whichprimtype, int interporflux, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*stencilvar)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP]);
  extern void slope_lim_pointtype(int interporflux, int realisinterp, int pl, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*dq)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP]);
  int pl,pliter;




  if( LINEINTERPTYPE(lim[dir]) ){ // this overrides lim, but lim must still be set properly
    // ENOPRIMITIVE below means primitives instead of conserved quantities
    slope_lim_linetype_c2e(realisinterp, ENOPRIMITIVE, ENOINTERPTYPE4EMF, dir, idel, jdel, kdel, primreal, NULL, p2interp, pleft, pright);
  }
  else{
    PINTERPLOOP(pliter,pl){
      slope_lim_pointtype(ENOINTERPTYPE4EMF,realisinterp, pl, dir, idel, jdel, kdel, primreal, p2interp, dq, pleft, pright);
    }
  }


#if(0)
  bound_prim(STAGEM1,t,pleft);
  bound_prim(STAGEM1,t,pright);
#endif


}








// interpolates certain B and v at FACE1,2,3 to CORN1,2,3

//
//
// MACP1A1(primface_l,dir,i,j,k,pl), etc.
// pbinterp[l,r][4 things]
// pvinterp[l,r][u,d][4 things]
//
// Interpolation list:
//
// FACE1: B1 -> +-C2 and +-C3   l/r V2 -> lr/+-C3   l/r V3 -> lr/+-C2
// FACE2: B2 -> +-C1 and +-C3   l/r V1 -> lr/+-C3   l/r V3 -> lr/+-C1
// FACE3: B3 -> +-C1 and +-C2   l/r V1 -> lr/+-C2   l/r V2 -> lr/+-C1
//
// Hence taking primface_{l or r} for field and primface_{l and r} for velocity and obtaining
// pfield[l,r][4]
// pvelocity[l,r][u,d][4]  where l,r,u,d are determine in some cyclic way for a given quantity
//
// |=interface
// i=zone center of ith zone
// |            |                     |
// |            |                     |
// |            |                     |
// |            |                     |
// |            |                     |
// |   i-1,j+1  |        i,j+1        |   i+1,j+1
// |   i-1,j    |        i,j          |   i+1,j
// |   i-1,j-1  |        i,j-1        |   i+1,j-1
// |            |                     |
// |            |                     |  ^ dir=2
//  -> dir=1                              |
//
// All inputs are points and all outputs are points
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// below from fluxcalc type function:
//
//      for(m=0;m<NUMCS;m++) for(l=0;l<NUMCS;l++){
// emf[+- in odir1][+- in odir2]
// velocity in same positions as emf
// for example, emf3[+-x][+-y] = By[+-x]*vx[+-x][+-y] - Bx[+-y]*vy[+-x][+-y]
// below requires velocity to be lab-frame 3-velocity consistent with lab-frame 3-field primitive

// see fluxct.c for signature definition of EMF and flux

// m%3+1 gives next 1->2,2->3,3->1
// 3-(4-m)%3 = (dir+1)%3+1 gives previous 1->3,2->1,3->2
// so odir1 is forward cyclic
// so odir2 is backward cyclic
//	emf2d[m][l] = 
//	  + MACP3A0(pbcorn,dir,B1-1+odir2,m,i,j,k)*MACP4A0(pvcorn,dir,U1-1+odir1,m,l,i,j,k) 
//	  - MACP3A0(pbcorn,dir,B1-1+odir1,l,i,j,k)*MACP4A0(pvcorn,dir,U1-1+odir2,m,l,i,j,k);
//      }
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// pvcorn and pbcorn are defined to be used like below initializaiton in set_arrays.c
//    for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=U1;pl<=U3;pl++) for(m=0;m<NUMCS;m++) for(l=0;l<NUMCS;l++)  MACP4A0(pvcorn,pl2,pl,m,l,i,j,k)=valueinit;
//    for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=B1;pl<=B3;pl++) for(l=0;l<NUMCS;l++)  MACP3A0(pbcorn,pl2,pl,l,i,j,k)=valueinit;
//

// LEAVE AS 0 since makes no sense to have as 1
#define INCLUDEGDETINTRANSVERSEINTERPLATIONOFFIELD 0

// INPUTS: Nvec, pr, primface_l[dir], primface_r[dir]
// OUTPUTS: pbcorn[dir][side], pvcorn[dir][side][side], cent2faceloop, face2cornloop
// TEMPVARS: prc pleft pright dqvec[dir]
// OPENMPOPTMARK: prc (used for p2interp in funny way), pleft,pright only exist per dir but used for all dirs, so can't use  nowait when involving pleft/pright/p2interp
// Note: wavespeeds already stored, so don't worry about wspeedtmp being only 1 dir at a time.  Further, don't use wavespeeds here, only used directly in EMF calculation
//
int interpolate_prim_face2corn(FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*primface_l)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*primface_r)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*pbcorn)[COMPDIM][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*pvcorn)[COMPDIM][NUMCS][NUMCS][NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], struct of_loop *cent2faceloop, struct of_loop (*face2cornloop)[NDIM][NDIM],
			       FTYPE (*prc)[NSTORE2][NSTORE3][NPR2INTERP],
			       FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP],
			       FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP],
				 int *Nvec, FTYPE (*dqvec[NDIM])[NSTORE2][NSTORE3][NPR2INTERP])
{
  void slope_lim_face2corn(int realisinterp, int dir, int idel, int jdel, int kdel, FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*dq)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pleft)[NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pright)[NSTORE2][NSTORE3][NPR2INTERP], struct of_loop *face2cornloop);
  FTYPE (*p2interp)[NSTORE2][NSTORE3][NPR2INTERP];
  int nprlocalstart,nprlocalend;
  int nprlocallist[MAXNPR];



  // GODMARK: face2corn duplicates cent2face if in 1D
  // This causes FLUXCTSTAG method to be slightly slower than FLUXCTTOTH method even in 1D, and somewhat still in 2D since comparing simple averaging with full interpolation

  ////////////////////////////////////////////
  //
  // save choice for interpolations
  {
    int pl,pliter;
    nprlocalstart=npr2interpstart;
    nprlocalend=npr2interpend;
    PMAXNPRLOOP(pl) nprlocallist[pl]=npr2interplist[pl];
  }


  ///////////////////////////////////////
  //
  // Procedure: (follow interpolate_pfield_face2cent() above)

  //
  // 1) Take primface_l and primface_2 and convert velocity term (have all components) to lab-frame 3-velocity (can't modify primface!!) -- so need another space? (vconemf?) FTYPE BASEMACP0A1(vconemf,N?M,...,NDIM-1);
  // 2) rescale() can be setup, but only for field since rescale() assumes input is primitive and after interpolation wouldn't be able to recover lab-frame 3-velocity cmponents if were using primitive velocity (unless WHICHVEL=VEL3, rarely used so no code for that special case when rescale() could be used)
  //
  // 3) For each of 3 edges, interpolate up and down (into pleft,pright) from each 1 FACE to 2 different CORNs
  //
  // 4) take interpolated pleft,pright and form p_l and p_r type quantities inside pbcorn[] and pvcorn[]
  //
  // 5) 
  //
  //
  //
  //
  //
  //
  //

  // Interpolation list:
  //
  // FACE1: B1 -> +-C2 and +-C3   l/r V2 -> lr/+-C3   l/r V3 -> lr/+-C2
  // FACE2: B2 -> +-C1 and +-C3   l/r V1 -> lr/+-C3   l/r V3 -> lr/+-C1
  // FACE3: B3 -> +-C1 and +-C2   l/r V1 -> lr/+-C2   l/r V2 -> lr/+-C1

  // loop over each edge(CORN1,2,3), interpolating in both odir1,odir2 directions and if that odir1,2 direction has N[odir1,2]==1, then just copy instead of interpolate
  // only have 1 dq,pleft,pright for NPR2INTERP quantities

  // sort above list by edge(C1,C2,C3):

  //  +-C1: FACE2 B2 interpolated in dir=3 ...........


  ///////////
  // interpolate dimension-by-dimension to allow future optimizations based upon memory localization
  // also this allows feeding in primface_l and primface_r directly
  ///////////

  // total of 6 sets of interpolations
  // note have chosen to mix velocity,field interpolations with certain symmetry.  In the end this means velocity is interpolated along its own direction only -- maybe not best?
  // note primface_l[i][Bi]=primface_r[i][Bi]
  // note not using many of the primface_l and primface_r (e.g. primface_lr[1][B2] -- cross fields since less local compared to directly evolved staggered field), but others are simply not used
  //
  // +-dir=1 : FACE2_to_CORN3 1 B2 & 2 V1 (primface_l[2][B2] and primface_lr[2][U1]) , FACE3_to_CORN2 1 B3, 2 V1 (primface_l[3][B3] and primface_lr[3][U1])
  // +-dir=2 : FACE1_to_CORN3 1 B1 & 2 V2 (primface_l[1][B1] and primface_lr[1][U2]) , FACE3_to_CORN1 1 B3, 2 V2 (primface_l[3][B3] and primface_lr[3][U2])
  // +-dir=3 : FACE1_to_CORN2 1 B1 & 2 V3 (primface_l[1][B1] and primface_lr[1][U3]) , FACE2_to_CORN1 1 B2, 2 V3 (primface_l[2][B2] and primface_lr[2][U3])


  /////////////////////////////////////////////////
  //
  // before computing EMF, need to convert velocity primitive to something consistent with field primitive frame
  // i.e. B^i (lab-frame 3-field) is normal field primitive, so use v^i (lab-frame 3-velocity) for velocity
  // otherwise number of interpolations increases to 2*4 extra per edge(CORN) since need symmetric value of the other velocity or uu0 if sending ucon
  // so just send lab-frame 3-velocity v^i as consistent with lab-frame field B^i in Faraday,Maxwell tensors

  // instead of using rescale(), just assume always better to inteprolate gdet B^i since the flux is conserved
#define NUMSTAGINTERP 5
#define BFACEINTERP 0
#define VLODIR1INTERP 1
#define VRODIR1INTERP 2
#define VLODIR2INTERP 3
#define VRODIR2INTERP 4

  // holds quantities prepared for interpolation with space used as listed above
  p2interp=prc;
 


  // Loop over faces that contains the original interpolation so only have to compute \detg B^i and v^i once instead of multiple times for each +-dir=1,2,3
  // FACE1: B1 -> +-C2 and +-C3   l/r V2 -> lr/+-C3   l/r V3 -> lr/+-C2
  // FACE2: B2 -> +-C1 and +-C3   l/r V1 -> lr/+-C3   l/r V3 -> lr/+-C1
  // FACE3: B3 -> +-C1 and +-C2   l/r V1 -> lr/+-C2   l/r V2 -> lr/+-C1






#pragma omp parallel OPENMPGLOBALPRIVATEFORUCONANDGEOMNPR2INTERP
  {
    int pl,pliter;
    int idel,jdel,kdel;
    int idel1,jdel1,kdel1;
    int idel2,jdel2,kdel2;
    int i,j,k;
    struct of_geom geomfdontuse;
    struct of_geom *ptrgeomf=&geomfdontuse;
    struct of_geom geomcdontuse;
    struct of_geom *ptrgeomc=&geomcdontuse;
    struct of_geom geomcorndontuse;
    struct of_geom *ptrgeomcorn=&geomcorndontuse;
    int dir,interpdir,edgedir;
    int locallim;
    FTYPE pstore_l[NPR2INTERP],pstore_r[NPR2INTERP];
    FTYPE *p2interp_l,*p2interp_r;
    FTYPE p_l[NPR2INTERP], p_r[NPR2INTERP];
    int enerregion;
    int *localenerpos;
    int odir1,odir2;
    int EMFodir1,EMFodir2;
    struct of_state ql,qr;
    struct of_state *ptrql,*ptrqr;
    FTYPE igdetgnosing;
    int usedq;
    int whichodir;
    int Aodir1,Aodir2;
    int Bodir1,Bodir2;
    int Codir1,Codir2;
    int Dodir1,Dodir2;
    extern int choose_limiter(int dir, int i, int j, int k, int pl);
    int realisinterp;
    int is,ie,js,je,ks,ke;

    OPENMP3DLOOPVARSDEFINE;


    // default state pointer (from now on below code should only use ptrql,ptrqr
    ptrql=&ql;
    ptrqr=&qr;

    p2interp_l=pstore_l;
    p2interp_r=pstore_r;


    ////////////////////////////////////////////
    //
    // Loop over faces
    //
    ///////////////////////////////////////////
    DIMENLOOP(dir){




      ///////////////////////////////////////////////
      //
      // other dimensions
      get_odirs(dir,&odir1,&odir2);

      //  if(Nvec[odir1]==1 && Nvec[odir2]==1) then EMF[dir]==0, but still copy over results in accordance with this routine since results here are for other EMFs
      // for example, if 2D in dirs=1,2 and 1-D in dir=3, then FACE3 still gives EMF1,EMF2 whose differences in dirs=1,2 is used for evolution
      // later explicit interpolations are avoided if interpdir is 1-D

      ///////////////////////////////////////////////
      //
      // get direction offsets for array accesses
      //
      ///////////////////////////////////////////////
      idel1 = fluxloop[odir1][FIDEL];
      jdel1 = fluxloop[odir1][FJDEL];
      kdel1 = fluxloop[odir1][FKDEL];
  
      idel2 = fluxloop[odir2][FIDEL];
      jdel2 = fluxloop[odir2][FJDEL];
      kdel2 = fluxloop[odir2][FKDEL];




      ////////////////////////////////////////////
      //
      // Convert and store primitive into p2interp[] -- these are *all* the quantities needed from this face-dir
      //
      ///////////////////////////////////////////

      // DEBUG:
      //    dualfprintf(fail_file,"%d %d : %d %d : %d %d\n",cent2faceloop[dir].is, cent2faceloop[dir].ie, cent2faceloop[dir].js, cent2faceloop[dir].je, cent2faceloop[dir].ks, cent2faceloop[dir].ke);

      // Was using:
      //     COMPZSLOOP( -N1BND, N1-1+N1BND, -N2BND, N2-1+N2BND, -N3BND, N3-1+N3BND ){
      // But, need loops to be controlled to don't access beyond where left-right faces set since ucon can fail with arbitrary input or nan's unlike other types of calculations
      // cent2faceloop is over CENT positions, which now since accessing the faces needs to be transcribed to FACE values interior to those CENT values
      is=cent2faceloop[dir].is + SHIFT1*(dir==1);
      ie=cent2faceloop[dir].ie;
      js=cent2faceloop[dir].js + SHIFT2*(dir==2);
      je=cent2faceloop[dir].je;
      ks=cent2faceloop[dir].ks + SHIFT3*(dir==3);
      ke=cent2faceloop[dir].ke;

      /////////COMPZSLOOP( is, ie, js, je, ks, ke ){
      OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) ///nowait // can't use "nowait" when using p2interp for each dir in next loop
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);
	
	// get geometry for face pre-interpolated values
	
	
#if(STOREFLUXSTATE==0)
	get_geometry(i, j, k, FACE1-1+dir, ptrgeomf); // at face[dir]

	// VELs: note don't use velocity in "dir" direction
	// LEFTVEL: compute and store v^i at face (input to ucon_calc() is primitive list as correct in primface_l,r)
	MYFUN(ucon_calc(MACP1A0(primface_l,dir,i,j,k), ptrgeomf, ptrql->ucon, ptrql->others) ,"flux.c:interpolate_face2corn()", "ucon_calc()", 1);
	// RIGHTVEL: compute and store v^i at face
	MYFUN(ucon_calc(MACP1A0(primface_r,dir,i,j,k), ptrgeomf, ptrqr->ucon, ptrqr->others) ,"flux.c:interpolate_face2corn()", "ucon_calc()", 2);

#else

	// really only need i,j,k in geomf for get_stateforfluxcalc(), unless doing INCLUDEGDETINTRANSVERSEINTERPLATIONOFFIELD==1
	get_geometry_gdetonly(i, j, k, FACE1-1+dir, ptrgeomf); // at face[dir]

	get_stateforfluxcalc(dir, ISLEFT, MACP1A0(primface_l,dir,i,j,k), ptrgeomf, &ptrql);
	get_stateforfluxcalc(dir, ISRIGHT, MACP1A0(primface_r,dir,i,j,k), ptrgeomf, &ptrqr);

#endif



#if(INCLUDEGDETINTRANSVERSEINTERPLATIONOFFIELD)
	// BAD IDEA:
	// BFACE: compute and store \detg B^i and prepare for 2-way interpolation of that single field (notice that primface_l,r same for face field
	// note that since interpolating \detg B^i, don't have to unrescale because can just use this to obtain EMF w/ gdet
	MACP0A1(p2interp,i,j,k,BFACEINTERP) = MACP1A1(primface_l,dir,i,j,k,B1-1+dir) * (ptrgeomf->gdet);
#else
	// BFACE: compute and store B^i and prepare for 2-way interpolation of that single field (notice that primface_l,r same for face field
	// note that for the interpolation in transverse direction of the field it makes no sense to use gdet.  Example is B1 near pole.  B1 is roughly constant typically near pole.
	MACP0A1(p2interp,i,j,k,BFACEINTERP) = MACP1A1(primface_l,dir,i,j,k,B1-1+dir) ;
#endif


	// VELs: note don't use velocity in "dir" direction
	// LEFTVEL: compute and store v^i at face (input to ucon_calc() is primitive list as correct in primface_l,r)
	MACP0A1(p2interp,i,j,k,VLODIR1INTERP) = (ptrql->ucon[odir1])/(ptrql->ucon[TT]);
	MACP0A1(p2interp,i,j,k,VLODIR2INTERP) = (ptrql->ucon[odir2])/(ptrql->ucon[TT]);
	// RIGHTVEL: compute and store v^i at face
	MACP0A1(p2interp,i,j,k,VRODIR1INTERP) = (ptrqr->ucon[odir1])/(ptrqr->ucon[TT]);
	MACP0A1(p2interp,i,j,k,VRODIR2INTERP) = (ptrqr->ucon[odir2])/(ptrqr->ucon[TT]);


      }// end COMPZSLOOP






      ///////////////
      //
      // now send relevant terms in p2interp to interpolator for each odir1,odir2 directions
      //
      ///////////////








      // MACP4A0(pvcorn,corner/emf/edge dir,which velocity,l/r,u/d,i,j,k)
      // note that emf[+- in EMFodir1][+- in EMFodir2] implies pvcorn[l/r][u/d] =  pvcorn[l/r in EMFodir1][u/d in EMFodir2] since have matching [l][m] when accessing emf[] and pvcorn[] where in this comment edgedir=dir and odir1 and odir2 are interpdir and dir (order?)
      //
      // translation:
      //
      // in EMF calculation function:
      //   edgedir=1 odir1=2 odir2=3
      //   edgedir=2 odir1=3 odir2=1
      //   edgedir=3 odir1=1 odir2=2
      //
      // in this function for interpdir=odir1 interpolation:
      //   face dir=1 odir1=interpdir=2 odir2=edgedir=3  EMFodir1=1  EMFodir2=2
      //   face dir=2 odir1=interpdir=3 odir2=edgedir=1  EMFodir1=2  EMFodir2=3
      //   face dir=3 odir1=interpdir=1 odir2=edgedir=2  EMFodir1=3  EMFodir2=1
      //
      //
      // Hence, EMFodir1 is facedir and EMFodir2 is interpdir
      //
      // in this function for interpdir=odir2 interpolation:
      //   face dir=1 odir2=interpdir=3 odir1=edgedir=2  EMFodir1=3  EMFodir2=1
      //   face dir=2 odir2=interpdir=1 odir1=edgedir=3  EMFodir1=1  EMFodir2=2
      //   face dir=3 odir2=interpdir=2 odir1=edgedir=1  EMFodir1=2  EMFodir2=3
      //
      //
      // Hence, EMFodir1 is interpdir and EMFodir2 is facedir
      //
      // So need to define EMFodir1 and EMFodir2 from edgedir



      //////////////////////////////////////////
      //
      // Loop over other directions not in face-dir
      //
      //////////////////////////////////////////

      for(whichodir=0;whichodir<=1;whichodir++){

	if(whichodir==0){ // whichodir==0 arbitrarily corresponds to interpdir=odir1
	  ///////////////////////////
	  // interpolate in odir1 direction (places quantities at edge/emf/corner odir2)

	  npr2interpstart=0;
	  npr2interpend=2; // 3 things
	  npr2interplist[0]=BFACEINTERP;
	  // ODIR? should be same as interpdir
	  npr2interplist[1]=VLODIR1INTERP; // hence [1] is for previous p_l
	  npr2interplist[2]=VRODIR1INTERP; // hence [2] is for previous p_r

	  // face is dir, and interpolation direction and edge direction are orthogonal to that
	  interpdir=odir1;
	  edgedir=odir2;

	  // del's associated with interpolation direction
	  idel=idel1;
	  jdel=jdel1;
	  kdel=kdel1;
	}
	else{

	  ///////////////////////////
	  // interpolate in odir2 direction (places quantities at edge/emf/corner odir1)

	  npr2interpstart=0;
	  npr2interpend=2; // 3 things
	  npr2interplist[0]=BFACEINTERP;
	  // ODIR? should be same as interpdir
	  npr2interplist[1]=VLODIR2INTERP; // hence [1] is for previous p_l
	  npr2interplist[2]=VRODIR2INTERP; // hence [2] is for previous p_r

	  interpdir=odir2;
	  edgedir=odir1;

	  // del's associated with interpolation direction
	  idel=idel2;
	  jdel=jdel2;
	  kdel=kdel2;
	}



	//////////////////////
	//
	// set EMFodir's
	//
	//////////////////////
	get_odirs(edgedir,&EMFodir1,&EMFodir2);



	//////////////////////
	//
	//    Setup access to emf-like quantities that have 3-dimensions and going to 2-dimensions
	//
	//////////////////////

	if(EMFodir1==interpdir){
	  // then first entry should contain 0/1 for *current* interpolation
	  // then EMFodir1 is interpdir corresponding to direction for current interpolation
	  // then EMFodir2 is face-dir corresponding to direction for previous interpolation
	  Aodir1=0; Aodir2=0;
	  Bodir1=1; Bodir2=0;
	  Codir1=0; Codir2=1;
	  Dodir1=1; Dodir2=1;
	}
	else{
	  // then first entry should contain 0/1 for *previous* interpolation
	  // then EMFodir1 is face-dir corresponding to direction for previous interpolation
	  // then EMFodir2 is interpdir corresponding to direction for current interpolation
	  Aodir1=0; Aodir2=0;
	  Bodir1=0; Bodir2=1;
	  Codir1=1; Codir2=0;
	  Dodir1=1; Dodir2=1;
	}

	//////////////////////
	// interpolate    
	//
	// GODMARK: Note put primface_l[dir] (yes, face-dir) into slope_lim as a "good primitive" to be used for shock or other indicators -- should really use average of primface_l and primface_r for symmetry considerations -- otherwise not used -- ENOMARK
	//
	// only really do interpolation if dimension exists ...
	//////////////////////
	//	 (Nvec[interpdir]>1 && (! (Nvec[edgedir]==1 && Nvec[dir]==1) ))      // ... or even if dimension exists but orthogonal dimensions do not then just copy over result -- need to modify more things to do this
	//      if(Nvec[interpdir]>1){
	if(!(Nvec[interpdir]==1)){
	  realisinterp=0; // since only ever limited set of quantities
	  slope_lim_face2corn(realisinterp, interpdir,idel,jdel,kdel,pr,p2interp,dqvec[interpdir],pleft,pright, &(face2cornloop[edgedir][EMFodir1][EMFodir2]));
	}
  
	///////////////////
	// get p_l p_r
	//////////////////////////////////////
	//
	// interpolate primitive using slope (dq) or directly from pleft and pright
	//
	/////////////////////////////////////
	
	// Assume for now that limiter is not per i,j,k but only per dir (unlike normal interpolation) (also no pl dependence)
	// no HORIZONSUPERFAST here
	locallim=choose_limiter(interpdir, 0,0,0,B1);
	usedq=(locallim<PARA)&&(LIMADJUST==0);
	
	// Could use face2cornloop-> to better constrain the below
	///////	COMPZSLOOP( -N1BND+idel, N1-1+N1BND, -N2BND+jdel, N2-1+N2BND, -N3BND+kdel, N3-1+N3BND ){
	OPENMP3DLOOPSETUP( -N1BND+idel, N1-1+N1BND, -N2BND+jdel, N2-1+N2BND, -N3BND+kdel, N3-1+N3BND );
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) //nowait // Don't wait since each direction is independent // NO: use of pleft, pright, and some use of p2interp (that depend on each dir) from loop above makes not possible to use "nowait"
	OPENMP3DLOOPBLOCK{
	  OPENMP3DLOOPBLOCK2IJK(i,j,k);

	  
	  //	if(Nvec[interpdir]>1 && (! (Nvec[edgedir]==1 && Nvec[dir]==1) ))
	  //	if(Nvec[interpdir]>1){
	  if(!(Nvec[interpdir]==1)){
	    if(usedq){
	      PINTERPLOOP(pliter,pl){
		// FACE_to_CORN interpolation is same as if doing CENT_to_EDGE from point of view of indicies to use and pleft,pright assignments
		p2interp_l[pl] = MACP0A1(p2interp,i - idel,j - jdel,k - kdel,pl) + 0.5 * MACP1A1(dqvec,interpdir,i - idel,j - jdel,k - kdel,pl);
		p2interp_r[pl] = MACP0A1(p2interp,i,j,k,pl) - 0.5 * MACP1A1(dqvec,interpdir,i,j,k,pl);
	      }
	    }
	    else{
	      PINTERPLOOP(pliter,pl){
		p2interp_l[pl] = MACP0A1(pright,i-idel,j-jdel,k-kdel,pl);
		p2interp_r[pl] = MACP0A1(pleft,i,j,k,pl);
	      }
	    }
	  }
	  else{
	    // if no interpolation, just copy result from pre-interpolated p2interp[] avoiding,bypassing dq,pleft,pright
	    PINTERPLOOP(pliter,pl){
	      p2interp_r[pl] = p2interp_l[pl] = MACP0A1(p2interp,i,j,k,pl);
	    }
	  }

	  ////////////////////
	  //
	  // Set lab-frame 3-magnetic field at CORN
	  //
	  // MACP3A0(pbcorn,corner/emf/edge dir,which field,+-present interpdir,i,j,k)



#if(INCLUDEGDETINTRANSVERSEINTERPLATIONOFFIELD==1 && CORNGDETVERSION==1 || INCLUDEGDETINTRANSVERSEINTERPLATIONOFFIELD==0 && CORNGDETVERSION==0)
	  get_geometry_gdetonly(i, j, k, CORN1-1+edgedir, ptrgeomcorn); // at CORN[dir]
#endif


#if(INCLUDEGDETINTRANSVERSEINTERPLATIONOFFIELD==1 && CORNGDETVERSION==1)
	  // then unrescale field since will multiply geometry once have final EMF (avoids line currents)
	  set_igdetsimple(ptrgeomcorn);
	  igdetgnosing = ptrgeomcorn->igdetnosing;
#elif(INCLUDEGDETINTRANSVERSEINTERPLATIONOFFIELD==0 && CORNGDETVERSION==0)
	  // then add gdet now since will not multiply geometry once have final EMF
	  igdetgnosing = ptrgeomcorn->gdet; // here igdet really is just geom
#else
	  // nothing to do
	  igdetgnosing = 1.0;
#endif


	  MACP3A0(pbcorn,edgedir,B1-1+dir,0,i,j,k) = p2interp_l[npr2interplist[0]]*igdetgnosing;
	  MACP3A0(pbcorn,edgedir,B1-1+dir,1,i,j,k) = p2interp_r[npr2interplist[0]]*igdetgnosing;



#if(0)
	  // DEBUG:
	  if(edgedir==3 && dir==2 && i==0 && j==0 && k==0){
	    dualfprintf(fail_file,"GOD1: interpdir=%d whichodir=%d list0=%d :: %21.15g %21.15g %21.15g\n",interpdir,whichodir,npr2interplist[0],MACP3A0(pbcorn,edgedir,B1-1+dir,0,i,j,k),MACP3A0(pbcorn,edgedir,B1-1+dir,1,i,j,k),igdetgnosing);
	    dualfprintf(fail_file,"pleft=%21.15g\n",MACP0A1(pleft,i,j,k,npr2interplist[0)]);
	  }
#endif

	  ////////////////////
	  //
	  // Set lab-frame 3-velocity at CORN
	  //
	  //      pvcorn[EMFdir][whichvel][l/r in EMFodir1][u/d in EMFodir2]
	  //
	  // for example:
	  // edgedir=1: EMFodir1=2 EMFodir2=3 then emf[0][0] is emf[left for EMFodir1][left for EMFodir2]
	  // so if interpolated in 2-dir and EMFodir1==2, then emf[0/1 filled with current p_l and p_r][0/1 filled with previous p_l p_r]

	  // npr2interplist[1,2] constains [u,d for velocity in interpdir direction]	

	  MACP4A0(pvcorn,edgedir,U1-1+interpdir,Aodir1,Aodir2,i,j,k) = p2interp_l[npr2interplist[1]]; // current p_l for previous p_l 
	  MACP4A0(pvcorn,edgedir,U1-1+interpdir,Bodir1,Bodir2,i,j,k) = p2interp_r[npr2interplist[1]]; // current p_r for previous p_l
	  MACP4A0(pvcorn,edgedir,U1-1+interpdir,Codir1,Codir2,i,j,k) = p2interp_l[npr2interplist[2]]; // current p_l for previous p_r
	  MACP4A0(pvcorn,edgedir,U1-1+interpdir,Dodir1,Dodir2,i,j,k) = p2interp_r[npr2interplist[2]]; // current p_r for previous p_r
	}// endCOMPZSLOOP
  
      }// end loop over other directions // at end of loop, have pbcorn,pvcorn for this 1 face interpolated to 2 corners
      


    }// end DIMENLOOP // at end of loop, have pbcorn,pvcorn for 3 edges


  }// end over parallel region (and implied barrier)





  ////////////////////////////////////////////
  //
  // restore choice for interpolations
  //
  ///////////////////////////////////////////
  {
    int pl,pliter;
    npr2interpstart=nprlocalstart;
    npr2interpend=nprlocalend;
    PMAXNPRLOOP(pl) npr2interplist[pl]=nprlocallist[pl];
  }









  ////////////////////////////////////////////
  //
  // DEBUG:
  //
  ///////////////////////////////////////////

#if(0)

  int pl2,m,l;
  int ri,rj,rk;

  for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=U1;pl<=U3;pl++) for(m=0;m<NUMCS;m++) for(l=0;l<NUMCS;l++){
	  // periodic x1
	  if ( (mycpupos[1] == 0)&&(mycpupos[1] == ncpux1 - 1) ) {
	    if( (BCtype[X1DN]==PERIODIC)&&(BCtype[X1UP]==PERIODIC) ){
	      // just copy from one side to another

	      LOOPFP12 LOOPFP13{ // debug only

		// copy from upper side to lower boundary zones
		ri=N1;
		rj=j;
		rk=k;
		LOOPBOUND1IN MACP4A0(pvcorn,pl2,pl,m,l,i,j,k) = MACP4A0(pvcorn,pl2,pl,m,l,ri+i,rj,rk);

		// copy from lower side to upper boundary zones
		ri=0;
		rj=j;
		k=k;
		LOOPBOUND1OUT MACP4A0(pvcorn,pl2,pl,m,l,i,j,k) = MACP4A0(pvcorn,pl2,pl,m,l,ri+(i-N1),rj,rk);
	      }
	    }
	  }
	  // periodic x2
	  if ( (mycpupos[2] == 0)&&(mycpupos[2] == ncpux2 - 1) ) {
	    if( (BCtype[X2DN]==PERIODIC)&&(BCtype[X2UP]==PERIODIC) ){
	      // just copy from one side to another

	      LOOPFP11 LOOPFP13{ // debug only

		// copy from upper side to lower boundary zones
		ri=i;
		rj=N2;
		rk=k;
		LOOPBOUND2IN MACP4A0(pvcorn,pl2,pl,m,l,i,j,k) = MACP4A0(pvcorn,pl2,pl,m,l,ri,rj+j,rk);

		// copy from lower side to upper boundary zones
		ri=i;
		rj=0;
		rk=k;
		LOOPBOUND2OUT MACP4A0(pvcorn,pl2,pl,m,l,i,j,k) = MACP4A0(pvcorn,pl2,pl,m,l,ri,rj+(j-N2),rk);
	      }
	    }
	  }
	}

  for(pl2=1;pl2<=COMPDIM;pl2++) for(pl=B1;pl<=B3;pl++) for(l=0;l<NUMCS;l++){
	// periodic x1
	if ( (mycpupos[1] == 0)&&(mycpupos[1] == ncpux1 - 1) ) {
	  if( (BCtype[X1DN]==PERIODIC)&&(BCtype[X1UP]==PERIODIC) ){
	    // just copy from one side to another

	    LOOPFP12 LOOPFP13{ // debug only

	      // copy from upper side to lower boundary zones
	      ri=N1;
	      rj=j;
	      rk=k;
	      LOOPBOUND1IN MACP3A0(pbcorn,pl2,pl,l,i,j,k) = MACP3A0(pbcorn,pl2,pl,l,ri+i,rj,rk);

	      // copy from lower side to upper boundary zones
	      ri=0;
	      rj=j;
	      k=k;
	      LOOPBOUND1OUT MACP3A0(pbcorn,pl2,pl,l,i,j,k) = MACP3A0(pbcorn,pl2,pl,l,ri+(i-N1),rj,rk);
	    }
	  }
	}
	// periodic x2
	if ( (mycpupos[2] == 0)&&(mycpupos[2] == ncpux2 - 1) ) {
	  if( (BCtype[X2DN]==PERIODIC)&&(BCtype[X2UP]==PERIODIC) ){
	    // just copy from one side to another

	    LOOPFP11 LOOPFP13{ // debug only

	      // copy from upper side to lower boundary zones
	      ri=i;
	      rj=N2;
	      rk=k;
	      LOOPBOUND2IN MACP3A0(pbcorn,pl2,pl,l,i,j,k) = MACP3A0(pbcorn,pl2,pl,l,ri,rj+j,rk);

	      // copy from lower side to upper boundary zones
	      ri=i;
	      rj=0;
	      rk=k;
	      LOOPBOUND2OUT MACP3A0(pbcorn,pl2,pl,l,i,j,k) = MACP3A0(pbcorn,pl2,pl,l,ri,rj+(j-N2),rk);
	    }
	  }
	}
      }
#endif






  return(0);

}











