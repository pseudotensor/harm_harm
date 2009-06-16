#include "decs.h"



// static declarations
static int compute_dt_fromsource(struct of_geom *ptrgeom, struct of_state *state, FTYPE *U, FTYPE *pr, FTYPE *dUevolve, FTYPE *dUgeomevolveUU, FTYPE *dtij, FTYPE *gravitydt);
static int dUtodt(struct of_geom *ptrgeom, struct of_state *state, FTYPE *pr, FTYPE *dUgeom, FTYPE *dUriemann, FTYPE *dUgeomgravity, FTYPE *accdt, FTYPE *gravitydt);
static int check_point_vs_average(int timeorder, int numtimeorders, PFTYPE *lpflag, FTYPE *pb, FTYPE *pf, FTYPE *upoint, FTYPE *uavg, struct of_geom *ptrgeom, struct of_newtonstats *newtonstats);



static int prepare_globaldt(
		     FTYPE ndt1,FTYPE ndt2,FTYPE ndt3,
		     FTYPE accdt,int accdti,int accdtj,int accdtk,
		     FTYPE gravitydt,int gravitydti,int gravitydtj,int gravitydtk,
		     FTYPE *ndt);



static void flux2dUavg(int i, int j, int k, FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE *dUavg1,FTYPE *dUavg2,FTYPE *dUavg3);
static void dUtoU(int i, int j, int k, int loc, FTYPE *dUgeom, FTYPE *dUriemann, FTYPE *CUf, FTYPE *Cunew, FTYPE *Ui,  FTYPE *uf, FTYPE *ucum);
static void dUtoU_check(int i, int j, int k, int loc, int pl, FTYPE *dUgeom, FTYPE *dUriemann, FTYPE *CUf, FTYPE *Cunew, FTYPE *Ui,  FTYPE *Uf, FTYPE *ucum);
static int asym_compute_1(FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
static int asym_compute_2(FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);


static FTYPE fractional_diff( FTYPE a, FTYPE b );



// AVG_2_POINT functions:
static void debugeno_compute(FTYPE (*p)[NSTORE2][NSTORE3][NPR],FTYPE (*U)[NSTORE2][NSTORE3][NPR],FTYPE (*debugvars)[NSTORE2][NSTORE3][NUMENODEBUGS]);

static void show_fluxes(int i, int j, int k, int loc, int pl,FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);


static int advance_standard(int stage, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR], FTYPE (*pf)[NSTORE2][NSTORE3][NPR],
			    FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],
			    FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
			    FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],
			    FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
			    FTYPE (*ui)[NSTORE2][NSTORE3][NPR], FTYPE (*uf)[NSTORE2][NSTORE3][NPR], FTYPE (*ucum)[NSTORE2][NSTORE3][NPR],
			    FTYPE *CUf,FTYPE *Cunew,SFTYPE fluxdt, SFTYPE boundtime, int stagenow, int numstages, FTYPE *ndt);
static int advance_finitevolume(int stage, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR], FTYPE (*pf)[NSTORE2][NSTORE3][NPR],
				FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],
				FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
				FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],
				FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
				FTYPE (*ui)[NSTORE2][NSTORE3][NPR],FTYPE (*uf)[NSTORE2][NSTORE3][NPR], FTYPE (*ucum)[NSTORE2][NSTORE3][NPR],
				FTYPE *CUf,FTYPE *Cunew, SFTYPE fluxdt, SFTYPE boundtime,  int stagenow, int numstages, FTYPE *ndt);




// things to do before any interpolation or advance step
// includes pre-computed things for interpolation and advance that (e.g.) aren't required to perform for each interpolation or advance call or portion of a call
void pre_interpolate_and_advance(FTYPE (*pb)[NSTORE2][NSTORE3][NPR])
{

  /////////////////////////////////////
  //
  // Compute and Store (globally) the get_state() data for the CENT position to avoid computing later and for merged-higher-order method
  //
  /////////////////////////////////////
#if(STOREFLUXSTATE||STORESHOCKINDICATOR)
  // NOTE: This is done before advance since always needed, and only needed once for all dimensions, and don't here instead of inside advance() since needed during fluxcalc() that is called first before any use of get_geometry() that we would use to put this call with
  compute_and_store_fluxstatecent(pb);
  // now flux_compute() and other flux-position-related things will obtain get_state() data for p_l and p_r from global arrays
#endif


}


// pi: initial values at t=t0 to compute Ui
// pb: values used to compute flux/source
// pf: solution using flux(pb) from pi's Ui -> Uf

// pi, pb, and pf can all be the same since
// 1) pb used first on a stencil, not modified, to compute fluxes
// 2) pf=pi is assigned by value at each zone
// 3) pf is modified using Utoprim at each zone using pb for sources (to correspond to fluxes which used pb)
//
// So in the end only pf is modified at each zone, so the loop changing p at previous (i,j) location doesn't affect the any new location in (i,j)
int advance(int stage, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR], FTYPE (*pf)[NSTORE2][NSTORE3][NPR],
	    FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],
	    FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
	    FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],
	    FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
	    FTYPE (*ui)[NSTORE2][NSTORE3][NPR],FTYPE (*uf)[NSTORE2][NSTORE3][NPR], FTYPE (*ucum)[NSTORE2][NSTORE3][NPR],
	    FTYPE *CUf, FTYPE *Cunew, SFTYPE fluxdt, SFTYPE boundtime, int timeorder, int numtimeorders, FTYPE *ndt)
{


  ////////////////
  //
  // setup state and interpolation stuff for interpolation and advance calls
  //
  ////////////////
  pre_interpolate_and_advance(pb);



  ////////////////
  //
  // advance
  //
  ////////////////
  if(DOENOFLUX==ENOFINITEVOLUME){
    MYFUN(advance_finitevolume(stage,pi,pb,pf,pstag,pl_ct, pr_ct, F1, F2, F3, vpot,ui,uf,ucum,CUf,Cunew,fluxdt,boundtime,timeorder,numtimeorders,ndt),"advance.c:advance()", "advance_finitevolume()", 1);
  }
  else if((DOENOFLUX==NOENOFLUX)||(DOENOFLUX==ENOFLUXRECON)||(DOENOFLUX==ENOFLUXSPLIT)){
    // new standard preserves conserved quantities even when metric changes
    MYFUN(advance_standard(stage,pi,pb,pf,pstag,pl_ct, pr_ct, F1, F2, F3, vpot,ui,uf,ucum,CUf,Cunew,fluxdt,boundtime,timeorder,numtimeorders,ndt),"advance.c:advance()", "advance_standard()", 1);
  }
  else{
    dualfprintf(fail_file,"No such DOENOFLUX=%d\n",DOENOFLUX);
    myexit(1);
  }


  return(0);


}



// Notes:
// loop range defined with +SHIFT? so consistent with requirement by IF3DSPCTHENMPITRANSFERATPOLE or consistent with setting upper face flux and using it, which is only used for FLUXBSTAG for evolving field on faces.  This forces field on upper face to be evolved as required in some cases.
// This is bit excessive for non-face quantities, but fake partial evolution of centered value at "N" is ok as long as don't hit NaN's that slow down code.





// this method guarantees conservation of non-sourced conserved quantities when metric is time-dependent
// this method has updated field staggered method
static int advance_standard(
		     int stage,
		     FTYPE (*pi)[NSTORE2][NSTORE3][NPR],
		     FTYPE (*pb)[NSTORE2][NSTORE3][NPR],
		     FTYPE (*pf)[NSTORE2][NSTORE3][NPR],
		     FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],
		     FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
		     FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],
		     FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
		     FTYPE (*ui)[NSTORE2][NSTORE3][NPR],
		     FTYPE (*uf)[NSTORE2][NSTORE3][NPR],
		     FTYPE (*ucum)[NSTORE2][NSTORE3][NPR], 
		     FTYPE *CUf, 
		     FTYPE *Cunew, 
		     SFTYPE fluxdt,
		     SFTYPE boundtime,
		     int timeorder, int numtimeorders,
		     FTYPE *ndt)
{
  FTYPE ndt1, ndt2, ndt3;
  FTYPE dUtot;
  FTYPE idx1,idx2;
  SFTYPE dt4diag,dt4diag_forcomp;
  static SFTYPE dt4diag_forcomp_willbe=0;
  int finalstep;
  FTYPE accdt, accdt_ij;
  int accdti,accdtj,accdtk;
  FTYPE gravitydt, gravitydt_ij;
  int gravitydti,gravitydtj,gravitydtk;
  //  FTYPE (*dUriemannarray)[NSTORE2][NSTORE3][NPR];
  FTYPE (*ucumformetric)[NSTORE2][NSTORE3][NPR];
  int enerregion;
  int *localenerpos;
  int jj;
  FTYPE (*utoinvert)[NSTORE2][NSTORE3][NPR];
  FTYPE (*tempucum)[NSTORE2][NSTORE3][NPR];
  FTYPE (*useducum)[NSTORE2][NSTORE3][NPR];
  FTYPE (*myupoint)[NSTORE2][NSTORE3][NPR];
  int whichpltoavg[NPR];
  int ifnotavgthencopy[NPR];
  int is,ie,js,je,ks,ke;
  int doingextrashiftforstag;






  if(FLUXB==FLUXCTSTAG){
    // fill in tempucum with changes that are not controlled well in space, but later fill in ucum from this in controlled way
    tempucum=GLOBALPOINT(utemparray);
    useducum=tempucum; // unless changed
  }
  else{
    // no special shifting of indices occurs that requires loop shifting
    tempucum=ucum;
    useducum=ucum;
  }


  ucumformetric=GLOBALPOINT(ucumformetric);// temporary space for ucum for metric that is same time as "pb", so not updated yet or is ui



  /////////////////////////////////////////////
  //
  // Setup function tasks
  //
  ////////////////////////////////////////////


  // for ZLOOP:
  // avoid looping over region outside active+ghost grid
  // good because somewhat general and avoid bad inversions, etc.
  enerregion=TRUEGLOBALWITHBNDENERREGION;
  localenerpos=enerposreg[enerregion];


  accdt=BIG; // initially no limit to dt due to acceleration
  accdti=accdtj=accdtk=-100;
  gravitydt=BIG; // initially no limit to dt due to time derivatives in gravity
  gravitydti=gravitydtj=gravitydtk=-100;




#if(ASYMDIAGCHECK)
  dualfprintf(fail_file,"BEGINNING steppart=%d nstep=%ld\n",steppart,nstep);

  dualfprintf(fail_file,"pi in advance\n");
  asym_compute_1(pi);

  dualfprintf(fail_file,"pb in advance\n");
  asym_compute_1(pb);
#endif

  /////////
  //
  // tells diagnostics functions if should be accounting or not
  //
  /////////
  if(timeorder==numtimeorders-1){
    dt4diag=dt;
    finalstep=1;
  }
  else{
    dt4diag=-1.0;
    finalstep=0;
  }

  if(timeorder==numtimeorders-1 && (nstep%DIAGSOURCECOMPSTEP==0) ){
    // every 4 full steps as well as on final step of full step (otherwise diag_source_comp() too expensive)
    dt4diag_forcomp=dt4diag_forcomp_willbe;
    dt4diag_forcomp_willbe=0;
  }
  else{
    dt4diag_forcomp_willbe+=dt;
    dt4diag_forcomp=-1.0;
  }



  /////////////////////////////////////////////
  //
  // Setup loops [+1 extra compared to normal comp region if doing FLUXCTSTAG]
  //
  ////////////////////////////////////////////
  get_flux_startendindices(Uconsevolveloop,&is,&ie,&js,&je,&ks,&ke);



  /////////////////////////////////////////////
  //
  // Set initial ui, temporary space, so ok that COMPZLOOP() goes over +1 in FLUXB==FLUXCTSTAG case
  //
  ////////////////////////////////////////////
  if(timeorder==0){
    // last timestep's final ucum is stored into ucumformetric and ui as initial term in ucum
    // copy ucum -> {ucumformetric,ui}
    if(doingmetricsubstep()) copy_3dnpr_2ptrs(is,ie,js,je,ks,ke,ucum,ucumformetric,ui);
    else copy_3dnpr(is,ie,js,je,ks,ke,ucum,ui); // only need to setup ui then
  }
  else{
    // preserve this time's value of ucum for the metric (for timeorder==0 ucumformetric is assigned from ui)
    // copy ucum -> ucumformetric
    if(doingmetricsubstep()) copy_3dnpr(is,ie,js,je,ks,ke,ucum,ucumformetric);
  }


  /////////////////////////////////////////////
  //
  // Compute flux
  //
  ////////////////////////////////////////////

#if(PRODUCTION==0)
  trifprintf( "#0f");
#endif


  if(dt!=0.0){
    ndt1=ndt2=ndt3=BIG;
    // pb used here on a stencil, so if pb=pf or pb=pi in pointers, shouldn't change pi or pf yet -- don't currently
    MYFUN(fluxcalc(stage,pb,pstag,pl_ct, pr_ct, vpot,F1,F2,F3,CUf[2],fluxdt,&ndt1,&ndt2,&ndt3),"advance.c:advance_standard()", "fluxcalcall", 1);
  }

#if(PRODUCTION==0)
  trifprintf( "1f");
#endif




  // from here on, pi/pb/pf are only used a zone at a time rather than on a stencil



  /////////////////////////////////////////////////////
  /////////////////////////////////////////////////////
  //
  // now update get flux update [loop should go over normal computational region +1 on outer edges so automatically set field if staggered.  Since we only set tempucum so far, ucum in that +1 region is unaffected]
  //
  /////////////////////////////////////////////////////


  // initialize uf and ucum if very first time here since ucum is cumulative (+=) [now tempucum is cumulative]
  // copy 0 -> {uf,tempucum}
  if(timeorder==0) init_3dnpr_2ptrs(is, ie, js, je, ks, ke,0.0, uf,tempucum);


  if(dt!=0.0){

#if(WHICHEOM==WITHNOGDET && (NOGDETB1==1 || NOGDETB2==1 || NOGDETB3==1) )
    // for FLUXB==FLUXCTSTAG, assume no source term for field
    if(FLUXB==FLUXCTSTAG){
      dualfprintf(fail_file,"Not setup for field source term if staggered field\n");
      myexit(176023);
    }
#endif


#pragma omp parallel OPENMPGLOBALPRIVATEFORSTATEANDGEOM
    {
      int pl,pliter,i,j,k;
      struct of_geom geomdontuse;
      struct of_geom *ptrgeom=&geomdontuse;
      FTYPE Uitemp[NPR];
      FTYPE dUgeom[NPR],dUriemann[NPR],dUriemann1[NPR],dUriemann2[NPR],dUriemann3[NPR],dUcomp[NUMSOURCES][NPR];
      struct of_state qdontuse;
      struct of_state *qptr=&qdontuse;
      struct of_state qdontuse2;
      struct of_state *qptr2=&qdontuse2; // different qptr since call normal and special get_state()


      OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);

      
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);


	// set geometry for centered zone to be updated
	get_geometry(i, j, k, CENT, ptrgeom);
      
	// find Ui(pi)
	// force use of primitive to set Ui since otherwise wherever corrected/changed primitive (in fixup, etc.) then would have to change conserved quantity, while same since both are point values
	// only field for staggered method is special point value at faces that needs to come from conserved quantity
	MYFUN(get_state(MAC(pi,i,j,k), ptrgeom, qptr),"step_ch.c:advance()", "get_state()", 1);
	MYFUN(primtoU(UEVOLVE,MAC(pi,i,j,k), qptr, ptrgeom, Uitemp),"step_ch.c:advance()", "primtoU()", 1);

	if(FLUXB==FLUXCTSTAG || DOENOFLUX != NOENOFLUX ){
	  // then field version of ui[] is stored as "conserved" value at FACE, not CENT
	  PLOOPNOB1(pl) MACP0A1(ui,i,j,k,pl)=Uitemp[pl]; // CENT
	  //PLOOPBONLY(pl) MACP0A1(ui,i,j,k,pl) is itself // FACE (see step_ch.c's setup_rktimestep and know that ui=unew for before first substep)
	  PLOOPNOB2(pl) MACP0A1(ui,i,j,k,pl)=Uitemp[pl]; // CENT
	}
	else{
	  PLOOP(pliter,pl) MACP0A1(ui,i,j,k,pl)=Uitemp[pl]; // all at CENT
	}
	
	// dUriemann is actually average quantity, but we treat is as a point quantity at the zone center
	flux2dUavg(i,j,k,F1,F2,F3,dUriemann1,dUriemann2,dUriemann3);
	PLOOP(pliter,pl) dUriemann[pl]=dUriemann1[pl]+dUriemann2[pl]+dUriemann3[pl]; // this addition is one type of avg->point mistake



	/////////////////////////////////////////////////////
	/////////////////////////////////////////////////////
	//
	// now update get source update (only affects stress-energy tensor in general)
	// [Loop goes over up to +1 normal computational region for getting new staggered U if doing FLUXCTSTAG]
	//
	/////////////////////////////////////////////////////
	
	// get state since both source() and dUtodt() need same state
	// From pb, so different than state for Ui(pi)
	MYFUN(get_stateforsource(MAC(pb,i,j,k), ptrgeom, &qptr2) ,"advance.c:()", "get_state() dir=0", 1);
      

	// note that uf and ucum are initialized inside setup_rktimestep() before first substep


	// find dU(pb)
	// source() doesn't actually use CUf[2]=dt right now
	MYFUN(source(MAC(pb,i,j,k), ptrgeom, qptr2, MAC(ui,i,j,k), dUriemann, dUcomp, dUgeom),"step_ch.c:advance()", "source", 1);
	// assumes final dUcomp is nonzero and representative of source term over this timestep
	


#if(SPLITNPR)
	// don't update metric if only doing B1-B3
	if(advancepassnumber==-1 || advancepassnumber==1)
#endif
	  {
#if(ACCURATESOURCEDIAG)
	    if(DODIAGS){
	      diag_source_comp(ptrgeom,dUcomp,fluxdt);
	      diag_source_all(ptrgeom,dUgeom,fluxdt);
	    }
#else
	    if(DODIAGS){
	      diag_source_comp(ptrgeom,dUcomp,dt4diag_forcomp);
	      diag_source_all(ptrgeom,dUgeom,dt4diag);
	    }
#endif
	  }

      

	// Get update
	dUtoU(i,j,k,ptrgeom->p,dUgeom, dUriemann, CUf, Cunew, MAC(ui,i,j,k), MAC(uf,i,j,k), MAC(tempucum,i,j,k));
      
      
      
	// get timestep limit from acceleration
#if(LIMITDTWITHSOURCETERM)
#if(SPLITNPR)
	// don't do dUtodt if only doing B1-B3
	if(advancepassnumber==-1 || advancepassnumber==1)
#endif
	  {
	    // geometry is post-metric update, but should still give good estimate of future dt
	    dUtodt(ptrgeom, qptr2, MAC(pb,i,j,k), dUgeom, dUriemann, dUcomp[GEOMSOURCE], &accdt_ij, &gravitydt_ij);

#pragma omp critical
	    {
	      if(accdt_ij<accdt){
		accdt=accdt_ij;
		accdti=i;
		accdtj=j;
		accdtk=k;
	      }
	      if(gravitydt_ij<gravitydt){
		gravitydt=gravitydt_ij;
		gravitydti=i;
		gravitydtj=j;
		gravitydtk=k;
	      }
	    }// end critical region
	  }// end block that may mean: if not only doing B1-B3

#endif // end if doing LIMITDTWITHSOURCETERM
          





#if(FLUXDUMP)
	// DEBUG - DIAG:
	PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,0*NPR + pl)=dUgeom[pl];

	if(N1>1) PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,1*NPR + pl)=dUriemann1[pl];
	else PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,1*NPR + pl)=0.0;
		
	if(N2>1) PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,2*NPR + pl)=dUriemann2[pl];
	else PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,2*NPR + pl)=0.0;

	if(N3>1) PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,3*NPR + pl)=dUriemann3[pl];
	else PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,3*NPR + pl)=0.0;
#endif

      } // end COMPZLOOP :: end looping to obtain dUriemann and full unew update
    }// end parallel block
  } // end if dt!=0.0
  else{
    // then nothing to do since nothing changed
    // previously updated dU and got new ucum as fed into metric, but now metric has its own ucummetric so all that is not needed
    // SUPERGODMARK: no, I guess I don't recall what was being done for metric and why when passing through with dt==0.0
  }







#if(PRODUCTION==0)
  trifprintf( "#0m");
#endif

  
    
  /////////////////////////////////////////////
  //
  // EVOLVE (update/compute) METRIC HERE
  // In general computes stress-energy tensor (T) from pb and T is then used to compute metric
  // Done here instead of after flux since horizon_flux() updates flux through boundary that would change metric
  // want metric to be at same time as everything else done with pb so RK method is consistent
  //
  // uses unew that's NOT updated yet
  /////////////////////////////////////////////
  if(dt!=0.0){
    if(doingmetricsubstep()){
#if(SPLITNPR)
      // don't update metric if only doing B1-B3
      if(advancepassnumber==-1 || advancepassnumber==1)
#endif
	{
	  compute_new_metric_substep(CUf,Cunew,pb,ucumformetric); // CHANGINGMARK : Not sure if Cunew here is correct
	}
    }// end if doing metric substepping
  }




  ////////////////////////////////
  //
  // compute flux diagnostics (accurately using all substeps)
  //
  ///////////////////////////////
  if(dt!=0.0){
    // must come after metric changes that can change where flux surface is since otherwise when flux surface changes, we won't track this substep's flux through the new surface but the old surface (which could even be at r=0 and have no flux)
    // if using unew, then since metric update above uses old unew, need present flux at new horizon surface
#if(SPLITNPR)
    // don't update metric if only doing B1-B3
    if(advancepassnumber==-1 || advancepassnumber==1)
#endif
      {
#if(ACCURATEDIAG)
	diag_flux_pureflux(pb,F1, F2, F3, fluxdt); // fluxdt is true dt for this flux as added in dUtoU() as part of ucum
#endif
      }
  }






  
#if(PRODUCTION==0)
  trifprintf( "#0s");
#endif


  


 // Do comp full loop in case subset of full grid so that boundary cells of subset domain are defined without having to copy over those cells in bounds.c or initbase.gridsectioning.c
  /////////////
  //
  // Utoprim as initial conditions : can't assume want these to be same in the end, so assign
  //
  // Since final step often sets pointers of pi=pf, in order to use arbitrary guess, must set here once done with pi,pb,pf.
  //
  ////////////
  // copy pb->pf
  copy_3dnpr_fullloop(pb,pf);




  ///////////////////////////////////////
  //
  // Copy over tempucum -> ucum per pl to account for staggered field
  //
  // And choose which RK-quantity to invert
  //
  ///////////////////////////////////////
  if(finalstep){
    if(FLUXB==FLUXCTSTAG){
      // copy over new ucum in only desired locations irrespective of where tempucum was updated
      // copy tempucum -> ucum
      copy_tempucum_finalucum(Uconsevolveloop,tempucum,ucum);
    }
    utoinvert = ucum;
    useducum=ucum;
  }
  else{
    // tempucum cumulates for now
    utoinvert = uf;
    // tempucum cumulates for now
    useducum=tempucum;
  }




  ////////////////////////////
  //
  // split ZLOOP above and below to allow staggered field method
  // [loop only over "centered" cells]
  //
  ////////////////////////////
  if(FLUXB==FLUXCTSTAG){
    // if using staggered grid for magnetic field, then need to convert ucum to pstag to new pb/pf

    // GODMARK: Use of globals
    myupoint=GLOBALPOINT(upointglobal);

    // first copy over all quantities as point, which is true except for fields if FLUXRECON active
    // copy utoinvert -> myupoint
    // copy all pl's since myupoint eventually used to invert rest of non-field quantities
    copy_tempucum_finalucum(Uconsevolveloop,utoinvert,myupoint);


    if(extrazones4emf && dofluxreconevolvepointfield==0){
      //bound_uavg(STAGEM1,utoinvert); // DEBUG
      // uses utoinvert and gets reaveraged field into myupoint
      field_integrate_fluxrecon(stage, pb, utoinvert, myupoint);
    }


    // first pb entry is used for shock indicator, second is filled with new field
    // myupoint goes in as staggered point value of magnetic flux and returned as centered point value of magnetic flux
    interpolate_ustag2fieldcent(stage, boundtime, timeorder, numtimeorders, pb, pstag, myupoint, pf);

    ////////////////////    
    // now myupoint contains centered point conserved quantities ready for inversion
    ////////////////////

  }
  else{
    // utoinvert never reassigned from global a_utoinvert assignment since if here not doing FLUXCTSTAG
    myupoint=utoinvert;
  }



  ////////////////////////////
  //
  // INVERT [loop only over "centered" cells]
  //
  ////////////////////////////


  // get loop range
  get_inversion_startendindices(Uconsevolveloop,&is,&ie,&js,&je,&ks,&ke);

#pragma omp parallel OPENMPGLOBALPRIVATEFORINVERSION
  {
    int pl,pliter,i,j,k;
    struct of_geom geomdontuse;
    struct of_geom *ptrgeom=&geomdontuse;
    FTYPE prbefore[NPR];
    struct of_newtonstats newtonstats; // not pointer
    
    OPENMP3DLOOPVARSDEFINE;  OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);

    // initialize counters
    newtonstats.nstroke=newtonstats.lntries=0;


#pragma omp for schedule(OPENMPVARYENDTIMESCHEDULE(),OPENMPCHUNKSIZE(blocksize)) reduction(+: nstroke)
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);
 
      // set geometry for centered zone to be updated
      get_geometry(i, j, k, CENT, ptrgeom);

      
      // invert U->p
      if(finalstep){ // last call, so ucum is cooked and ready to eat!
	// store guess for diss_compute before changed by normal inversion
	PALLLOOP(pl) prbefore[pl]=MACP0A1(pf,i,j,k,pl);

	MYFUN(Utoprimgen(finalstep,EVOLVEUTOPRIM,UEVOLVE,MAC(myupoint,i,j,k), ptrgeom, MAC(pf,i,j,k),&newtonstats),"step_ch.c:advance()", "Utoprimgen", 1);
	nstroke+=newtonstats.nstroke; newtonstats.nstroke=newtonstats.lntries=0;


#if(DODISS||DODISSVSR)
	// then see what entropy inversion would give
	diss_compute(EVOLVEUTOPRIM,UEVOLVE,MAC(myupoint,i,j,k),ptrgeom,prbefore,MAC(pf,i,j,k),&newtonstats);
#endif
	
      }
      else{ // otherwise still iterating on primitives
	MYFUN(Utoprimgen(finalstep,EVOLVEUTOPRIM,UEVOLVE,MAC(myupoint,i,j,k), ptrgeom, MAC(pf,i,j,k),&newtonstats),"step_ch.c:advance()", "Utoprimgen", 1);
	nstroke+=newtonstats.nstroke; newtonstats.nstroke=newtonstats.lntries=0;
      }


      
#if(SPLITNPR)
      // don't update metric if only doing B1-B3
      if(advancepassnumber==-1 || advancepassnumber==1)
#endif
	{
	  // immediate local (i.e. 1-zone) fix
#if(FIXUPZONES==FIXUP1ZONE)
	  // SUPERGODMARK: Below should differentiate beteween whether want negative densities fixed or not, but right now fixup1zone() does all
	  if((STEPOVERNEGU==0)||(STEPOVERNEGRHO==0)||(STEPOVERNEGRHOU==0)||(finalstep)){
	    MYFUN(fixup1zone(MAC(pf,i,j,k),MAC(useducum,i,j,k), ptrgeom,finalstep),"fixup.c:fixup()", "fixup1zone()", 1);
	  }
#endif
	}


    }// end COMPZLOOP
  }// end parallel section






#if(ASYMDIAGCHECK)
  dualfprintf(fail_file,"useducum in advance\n");
  asym_compute_2(useducum);

  dualfprintf(fail_file,"ENDING steppart=%d nstep=%ld\n",steppart,nstep);
#endif




  /////////////////////////////////
  //
  // If not fixing up primitives after inversion immediately, then fix up all zones at once afterwards
  //
  /////////////////////////////////

#if(SPLITNPR)
  // don't update metric if only doing B1-B3
  if(advancepassnumber==-1 || advancepassnumber==1)
#endif
    {
#if(FIXUPZONES==FIXUPALLZONES)
      fixup(stage,pf,useducum,finalstep);
#endif  
    }


  /////////////////////////////////
  //
  // Determine next timestep from waves, fluxes, and source updates
  //
  /////////////////////////////////


  prepare_globaldt(ndt1,ndt2,ndt3,accdt,accdti,accdtj,accdtk,gravitydt,gravitydti,gravitydtj,gravitydtk,ndt);



#if(PRODUCTION==0)
  trifprintf( "2f");
#endif

  return (0);
}







// finite volume method NOT SETUP FOR CONSISTENT METRIC EVOLUTION YET -- EASY, JUST NOT DOING IT YET -- FOLLOW ABOVE AS EXAMPLE OF WHAT TO DO
// also not setup for staggered field method
static int advance_finitevolume(int stage,
			 FTYPE (*pi)[NSTORE2][NSTORE3][NPR],
			 FTYPE (*pb)[NSTORE2][NSTORE3][NPR],
			 FTYPE (*pf)[NSTORE2][NSTORE3][NPR],
			 FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],
			 FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
			 FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],
			 FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
			 FTYPE (*ui)[NSTORE2][NSTORE3][NPR],
			 FTYPE (*uf)[NSTORE2][NSTORE3][NPR],
			 FTYPE (*ucum)[NSTORE2][NSTORE3][NPR], 
			 FTYPE *CUf, 
			 FTYPE *Cunew, 
			 SFTYPE fluxdt,
			 SFTYPE boundtime,
			 int timeorder, int numtimeorders,
			 FTYPE *ndt)
{
  int sc;
  FTYPE ndt1, ndt2, ndt3;
  FTYPE dUtot;
  FTYPE idx1,idx2;
  SFTYPE dt4diag,dt4diag_forcomp;
  static SFTYPE dt4diag_forcomp_willbe=0;
  int finalstep;
  FTYPE accdt, accdt_ij;
  int accdti,accdtj,accdtk;
  FTYPE gravitydt, gravitydt_ij;
  int gravitydti,gravitydtj,gravitydtk;
  int enerregion;
  int *localenerpos;
  int jj;
  FTYPE (*utoinvert)[NSTORE2][NSTORE3][NPR];
  FTYPE (*tempucum)[NSTORE2][NSTORE3][NPR];
  FTYPE (*useducum)[NSTORE2][NSTORE3][NPR];
  FTYPE (*myupoint)[NSTORE2][NSTORE3][NPR];
  FTYPE (*ucumformetric)[NSTORE2][NSTORE3][NPR];
  FTYPE (*dUgeomarray)[NSTORE2][NSTORE3][NPR];
  // staggered field function:
  int whichpltoavg[NPR];
  int ifnotavgthencopy[NPR];
  int docons,dosource;
  int locpl[NPR];
  int is,ie,js,je,ks,ke;
  int doingextrashiftforstag;


  if(FLUXB==FLUXCTSTAG){
    tempucum=GLOBALPOINT(utemparray);
    useducum=tempucum; // unless changed
  }
  else{
    // no special shifting of indices occurs that requires loop shifting
    tempucum=ucum;
    useducum=ucum;
  }


  ucumformetric=GLOBALPOINT(ucumformetric);// temporary space for ucum for metric that is same time as "pb", so not updated yet or is ui
  dUgeomarray=GLOBALPOINT(dUgeomarray);// temporary space for dUgeomarray


  
  {// begin block
    int pl,pliter;
    // any cons
    docons=0;
    PLOOP(pliter,pl) docons+=do_conserved_integration[pl];
    docons*=(DOENOFLUX == ENOFINITEVOLUME);

    // any source
    dosource=0;
    PLOOP(pliter,pl) dosource+=do_source_integration[pl];
    dosource*=(DOENOFLUX == ENOFINITEVOLUME);
  }// end block


  /////////////////////////////////////////////
  //
  // Setup loops and dt's
  //
  ////////////////////////////////////////////


  // for CZLOOP:
  // avoid looping over region outside active+ghost grid
  // good because somewhat general and avoid bad inversions, etc.
  enerregion=TRUEGLOBALWITHBNDENERREGION;
  localenerpos=enerposreg[enerregion];


  accdt=BIG; // initially no limit to dt due to acceleration
  accdti=accdtj=accdtk=-100;
  gravitydt=BIG; // initially no limit to dt due to time-changes in gravity
  gravitydti=gravitydtj=gravitydtk=-100;
 

  /////////
  //
  // tells diagnostics functions if should be accounting or not
  //
  /////////
  if(timeorder==numtimeorders-1){
    dt4diag=dt;
    finalstep=1;
  }
  else{
    dt4diag=-1.0;
    finalstep=0;
  }

  if(timeorder==numtimeorders-1 && (nstep%DIAGSOURCECOMPSTEP==0) ){
    // every 4 full steps as well as on final step of full step (otherwise diag_source_comp() too expensive)
    dt4diag_forcomp=dt4diag_forcomp_willbe;
    dt4diag_forcomp_willbe=0;
  }
  else{
    dt4diag_forcomp_willbe+=dt;
    dt4diag_forcomp=-1.0;
  }


  /////////////////////////////////////////////
  //
  // Setup loops
  //
  ////////////////////////////////////////////
  get_flux_startendindices(Uconsevolveloop,&is,&ie,&js,&je,&ks,&ke);



  /////////////////////////////////////////////
  //
  // Setup RK stuff
  //
  ////////////////////////////////////////////

  // setup RK's uinitial (needed since sometimes set ui=0 inside advance())
  // unew should be read in, now assign to uinitial for finite volume method or normal method when dt=0.0 or moving grid
  // below can't be CZLOOP since need uinitial in ghost+active region
  // GODMARK: since ui=ucum (average quantities) then if change primitive somehow (fixup, etc.) then must change corresponding average conserved quantity somehow (this is one reason why using average values is annoying, although in some cases we use Sasha's method to treat average like point for *change* in average conserved quantity)
  // otherwise assume ui didn't change.  Present RK schemes assume this.  Otherwise have to keep track of pf/Uf pairs in RK stepping
  /////////////////////////////////////////////
  //
  // Set initial ui, temporary space, so ok that COMPZLOOP() goes over +1 in FLUXB==FLUXCTSTAG case
  //
  ////////////////////////////////////////////
  if(timeorder==0){
    // last timestep's final ucum is stored into ucumformetric and ui as initial term in ucum
    // copy ucum -> {ucumformetric,ui}
    if(doingmetricsubstep()) copy_3dnpr_2ptrs(is,ie,js,je,ks,ke,ucum,ucumformetric,ui);
    else copy_3dnpr(is,ie,js,je,ks,ke,ucum,ui); // only need to setup ui then
  }
  else{
    // preserve this time's value of ucum for the metric (for timeorder==0 ucumformetric is assigned from ui)
    // copy ucum -> ucumformetric
    if(doingmetricsubstep()) copy_3dnpr(is,ie,js,je,ks,ke,ucum,ucumformetric);
  }



  /////////////////////////////////////////////
  //
  // Compute flux
  //
  ////////////////////////////////////////////

#if(PRODUCTION==0)
  trifprintf( "#0f");
#endif

  if(dt!=0.0){
    ndt1=ndt2=ndt3=BIG;
    // pb used here on a stencil, so if pb=pf or pb=pi in pointers, shouldn't change pi or pf yet -- don't currently
    MYFUN(fluxcalc(stage,pb,pstag,pl_ct, pr_ct, vpot,F1,F2,F3,CUf[2],fluxdt,&ndt1,&ndt2,&ndt3),"advance.c:advance_standard()", "fluxcalcall", 1);
  }

#if(PRODUCTION==0)
  trifprintf( "1f");
#endif
  // from here on, pi/pb/pf are only used a zone at a time rather than on a stencil







#if(PRODUCTION==0)
  trifprintf( "#0m");
#endif



	
  /////////////////////////
  //
  // SOURCE TERM
  //
  ////////////////////////

  if(dt!=0.0){
    // GODMARK: other/more special cases?
#if(WHICHEOM==WITHNOGDET && (NOGDETB1==1 || NOGDETB2==1 || NOGDETB3==1) )
    // for FLUXB==FLUXCTSTAG, assume no source term for field
    if(FLUXB==FLUXCTSTAG){
      dualfprintf(fail_file,"Not setup for field source term if staggered field\n");
      myexit(176023);
    }
#endif

    //    COMPZSLOOP(is,ie,js,je,ks,ke){
#if(STOREFLUXSTATE)
    // call get_stateforsource, which just reads existing data instead of generating from EOS, etc.
#pragma omp parallel 
#else
#pragma omp parallel OPENMPGLOBALPRIVATEFORSTATEANDGEOM
#endif
    {
      int i,j,k,pl,pliter;
      FTYPE dUgeom[NPR],dUriemann[NPR],dUriemann1[NPR],dUriemann2[NPR],dUriemann3[NPR],dUcomp[NUMSOURCES][NPR];
      struct of_geom geomdontuse;
      struct of_geom *ptrgeom=&geomdontuse;
      struct of_state qdontuse;
      struct of_state *qptr=&qdontuse;

      OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);

      
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);
	// find dU(pb)
	// only find source term if non-Minkowski and non-Cartesian
	// set geometry for centered zone to be updated
	get_geometry(i, j, k, CENT, ptrgeom);

	// get state
	MYFUN(get_stateforsource(MAC(pb,i,j,k), ptrgeom, &qptr) ,"advance.c:()", "get_state() dir=0", 1);


	// dUriemann is volume averaged quantity (here this calcuation is done in case want to limit sources)
	flux2dUavg(i,j,k,F1,F2,F3,dUriemann1,dUriemann2,dUriemann3);
	PLOOP(pliter,pl) dUriemann[pl]=dUriemann1[pl]+dUriemann2[pl]+dUriemann3[pl]; // this addition is entirely consistent with point->averages

	    
	// get source term (point source, don't use to update diagnostics)
	MYFUN(source(MAC(pb,i,j,k), ptrgeom, qptr, MAC(ui,i,j,k), dUriemann, dUcomp, MAC(dUgeomarray,i,j,k)),"step_ch.c:advance()", "source", 1);
      }// end COMPZLOOP


      // volume integrate dUgeom
      // c2a_1 c2a_2 c2a_3
      if(dosource){
	// need to limit source averaging -- GODMARK
	PALLLOOP(pl) locpl[pl]=CENT;
	PALLLOOP(pl) whichpltoavg[pl]=do_source_integration[pl];// default
	PALLLOOP(pl) ifnotavgthencopy[pl]=1-do_source_integration[pl];// default
	avg2cen_interp(locpl,whichpltoavg, ifnotavgthencopy, ENOSOURCETERM, ENOCENT2AVGTYPE, pb, POINT(dUgeomarray), POINT(dUgeomarray));
      }
    }// end parallel region
  }// end if dt!=0.0




  ///////////////////////////////////////////////////////
  //
  // update Ui to Uf (ultimately to ucum)
  //
  ///////////////////////////////////////////////////////

  //  COMPZSLOOP(is,ie,js,je,ks,ke){
#if(STOREFLUXSTATE)
  // call get_stateforsource, which just reads existing data instead of generating from EOS, etc.
#pragma omp parallel 
#else
#pragma omp parallel OPENMPGLOBALPRIVATEFORSTATEANDGEOM
#endif
  {
    int i,j,k,pl,pliter;
    FTYPE fdummy;
    FTYPE dUgeom[NPR],dUriemann[NPR],dUriemann1[NPR],dUriemann2[NPR],dUriemann3[NPR],dUcomp[NUMSOURCES][NPR];
    struct of_geom geomdontuse;
    struct of_geom *ptrgeom=&geomdontuse;
    struct of_state qdontuse;
    struct of_state *qptr=&qdontuse;

    OPENMP3DLOOPVARSDEFINE;  OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);

      
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);


      // get geometry at center where source is located
      get_geometry(i, j, k, CENT, ptrgeom);

      // get state
      MYFUN(get_stateforsource(MAC(pb,i,j,k), ptrgeom, &qptr) ,"advance.c:()", "get_state() dir=0", 1);
		
      /////////////
      //
      // Utoprim as initial conditions : can't assume want these to be same in the end, so assign
      //
      ////////////
		
      // initialize uf and ucum if very first time here since ucum is cumulative (+=)
      //    if(timeorder==0) PLOOP(pliter,pl) MACP0A1(uf,i,j,k,pl)=MACP0A1(tempucum,i,j,k,pl)=MACP0A1(ucum,i,j,k,pl)=0.0;
      if(timeorder==0) PLOOP(pliter,pl) MACP0A1(uf,i,j,k,pl)=MACP0A1(tempucum,i,j,k,pl)=0.0;
		
      // NEED TO DEFINE Ui on other substeps besides the 0th one
      // find Ui(pi)



      if(dt!=0.0){
	// get source term (volume integrated)
	PLOOP(pliter,pl) dUgeom[pl]=MACP0A1(dUgeomarray,i,j,k,pl);

#if(SPLITNPR)
	// don't update metric if only doing B1-B3
	if(advancepassnumber==-1 || advancepassnumber==1)
#endif
	  {
#pragma omp critical // since diagnostics store in same global cumulative variables
	    {
#if(ACCURATESOURCEDIAG)
	      if(DODIAGS){
		// do diagnostics for volume integrated source term
		diag_source_all(ptrgeom,dUgeom,fluxdt);
	      }
#else
	      if(DODIAGS){
		diag_source_all(ptrgeom,dUgeom,dt4diag);
	      }
#endif
	    }	  
	  }
		  

	// dUriemann is volume averaged quantity
	flux2dUavg(i,j,k,F1,F2,F3,dUriemann1,dUriemann2,dUriemann3);
	PLOOP(pliter,pl) dUriemann[pl]=dUriemann1[pl]+dUriemann2[pl]+dUriemann3[pl]; // this addition is entirely consistent with point->averages
      }
      else{
	PLOOP(pliter,pl){
	  dUriemann[pl]=0.0;
	  dUgeom[pl]=0.0;
	}
      }

      // find uf==Uf and additional terms to ucum
      dUtoU(i,j,k,ptrgeom->p,dUgeom, dUriemann, CUf, Cunew, MAC(ui,i,j,k), MAC(uf,i,j,k), MAC(tempucum,i,j,k));



#if(LIMITDTWITHSOURCETERM)
      // SUPERGODMARK: no longer have access to dUcomp : NEED TO FIX
      // below is correct, but excessive
      // get source term again in order to have dUcomp (NEED TO FIX)
      MYFUN(source(MAC(pb,i,j,k), ptrgeom, qptr, MAC(ui,i,j,k), dUriemann, dUcomp, &fdummy),"step_ch.c:advance()", "source", 2);


      dUtodt(ptrgeom, qptr, MAC(pb,i,j,k), dUgeom, dUriemann, dUcomp[GEOMSOURCE], &accdt_ij, &gravitydt_ij);
      // below is old before grid sectioning
      //#if( (DOEVOLVEMETRIC || DOSELFGRAVVSR) && (RESTRICTDTSETTINGINSIDEHORIZON>=1) )
      //    // avoid limiting dt if inside horizon
      //    if(WITHINENERREGION(enerposreg[OUTSIDEHORIZONENERREGION],i,j,k))
      //#endif
#pragma omp critical
      {
	if(accdt_ij<accdt){
	  accdt=accdt_ij;
	  accdti=i;
	  accdtj=j;
	  accdtk=k;
	}
	if(gravitydt_ij<gravitydt){
	  gravitydt=gravitydt_ij;
	  gravitydti=i;
	  gravitydtj=j;
	  gravitydtk=k;
	}
      }// end critical region
#endif




#if(FLUXDUMP)
      PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,0*NPR + pl)=dUgeom[pl];

      if(N1>1) PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,1*NPR + pl)=dUriemann1[pl];
      else PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,1*NPR + pl)=0.0;
		
      if(N2>1) PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,2*NPR + pl)=dUriemann2[pl];
      else PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,2*NPR + pl)=0.0;

      if(N3>1) PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,3*NPR + pl)=dUriemann3[pl];
      else PLOOP(pliter,pl) GLOBALMACP0A1(fluxdump,i,j,k,3*NPR + pl)=0.0;
#endif
  

    }// end 3D LOOP
  }//end parallel region





  /////////////////////////////////////////////
  //
  // EVOLVE (update/compute) METRIC HERE
  // In general computes stress-energy tensor (T) from pb and T is then used to compute metric
  // Done here instead of after flux since horizon_flux() updates flux through boundary that would change metric
  // want metric to be at same time as everythin else done with pb so RK method is consistent
  //
  // uses unew that's NOT updated yet
  /////////////////////////////////////////////
  if(dt!=0.0){
#if(SPLITNPR)
    // don't update metric if only doing B1-B3
    if(advancepassnumber==-1 || advancepassnumber==1)
#endif
    {
      compute_new_metric_substep(CUf,Cunew,pb,ucumformetric); // CHANGINGMARK : Not sure if Cunew here is correct
    }
  }



  ////////////////////////////////
  //
  // compute flux diagnostics (accurately using all substeps)
  //
  ///////////////////////////////
  if(dt!=0.0){
    // must come after metric changes that can change where flux surface is since otherwise when flux surface changes, we won't track this substep's flux through the new surface but the old surface (which could even be at r=0 and have no flux)
    // if using unew, then since metric update above uses old unew, need present flux at new horizon surface
#if(SPLITNPR)
    // don't update metric if only doing B1-B3
    if(advancepassnumber==-1 || advancepassnumber==1)
#endif
      {
#if(ACCURATEDIAG)
	diag_flux_pureflux(pb,F1, F2, F3, fluxdt); // fluxdt is true dt for this flux as added in dUtoU() as part of ucum update
#endif
      }
  }





  /////////////
  //
  // Utoprim as initial conditions : can't assume want these to be same in the end, so assign
  //
  // Since final step often sets pointers of pi=pf, in order to use arbitrary guess, must set here once done with pi,pb,pf.
  //
  ////////////
  // setup initial guess for inversion
  // use pb since should be closer to pf
  // copy pb->pf
  copy_3dnpr_fullloop(pb,pf);



  ///////////////////////////////////////
  //
  // Copy over tempucum -> ucum per pl to account for staggered field
  //
  // and volume differentiate the conserved quantity
  //
  ///////////////////////////////////////
  if(finalstep){
    // last call, so ucum is cooked and ready to eat!
    if(FLUXB==FLUXCTSTAG){
      copy_tempucum_finalucum(Uconsevolveloop,tempucum,ucum);
    }
    // useducum only used after ucum assigned to ui above
    utoinvert=ucum;
    useducum=ucum;
    //    ubound=utoinvert;
  }
  else{ // otherwise still iterating on primitives
    // don't need to copy over tempucum->ucum yet
    utoinvert=uf;
    useducum=tempucum;
    //    ubound=utoinvert;
  }

  // GODMARK: use of globals
  myupoint=GLOBALPOINT(upointglobal);



  // to debug ENO
  //#if(DOENODEBUG)
  //debugeno_compute(pb,utoinvert,enodebugarray); //SASDEBUG -- OLD USE: now assign debugs inside reconstructeno code
  //#endif





  // a2c_1 a2c_2 a2c_3
  if(docons){
    int pl,pliter;
    // conserved quantity is limited later after primitive is obtained
    PALLLOOP(pl) locpl[pl]=CENT;
    PALLLOOP(pl) whichpltoavg[pl]=do_conserved_integration[pl];// default
    PALLLOOP(pl) ifnotavgthencopy[pl]=1-do_conserved_integration[pl];// default
    avg2cen_interp(locpl,whichpltoavg, ifnotavgthencopy, ENOCONSERVED, ENOAVG2CENTTYPE, pb, utoinvert, myupoint);  //SASMARK:  pb's for shock indicators should be defined on ALL grid, not only on ghost+active.  Maybe should use pi instead because define everywhere?
  }
  else{
    //    myupoint=utoinvert;
    copy_tempucum_finalucum(Uconsevolveloop,utoinvert,myupoint);
  }



  ////////////////////////////
  //
  // split CZLOOP above and below to allow staggered field method
  //
  ////////////////////////////
  if(FLUXB==FLUXCTSTAG){
    // if using staggered grid for magnetic field, then need to convert ucum to pstag to new pb/pf

    // GODMARK: If had c2a/a2c with 3-point outputs, then could do avg2cen_interp and below at once

    // first pb entry is used for shock indicator, second is filled with new field
    interpolate_ustag2fieldcent(stage, boundtime, timeorder, numtimeorders, pb, pstag, myupoint, pf);

    ////////////////////    
    // now utoinvert contains centered point conserved quantities ready for inversion
    ////////////////////

  }




  /////////////////////////////////
  //
  // Invert U -> p
  //
  // and fixup p if inversion bad
  // and compute dissipation rate if requested
  //
  /////////////////////////////////


  // get loop range
  get_inversion_startendindices(Uconsevolveloop,&is,&ie,&js,&je,&ks,&ke);

#pragma omp parallel OPENMPGLOBALPRIVATEFORINVERSION
  {
    int i,j,k,pl,pliter;
    FTYPE prbefore[NPR];
    struct of_geom geomdontuse;
    struct of_geom *ptrgeom=&geomdontuse;
    struct of_newtonstats newtonstats;
    OPENMP3DLOOPVARSDEFINE;  OPENMP3DLOOPSETUP(is,ie,js,je,ks,ke);

    // initialize counters
    newtonstats.nstroke=newtonstats.lntries=0;
    // ptr different when in parallel region
    ptrgeom=&geomdontuse;

#pragma omp for schedule(OPENMPVARYENDTIMESCHEDULE(),OPENMPCHUNKSIZE(blocksize)) reduction(+: nstroke)
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);
 
      // set geometry for centered zone to be updated
      get_geometry(i, j, k, CENT, ptrgeom);


      // store guess for diss_compute before changed by normal inversion
      PALLLOOP(pl) prbefore[pl]=MACP0A1(pf,i,j,k,pl);
  

      // invert point U-> point p
      MYFUN(Utoprimgen(finalstep,EVOLVEUTOPRIM, UEVOLVE, MAC(myupoint,i,j,k), ptrgeom, MAC(pf,i,j,k),&newtonstats),"step_ch.c:advance()", "Utoprimgen", 1);
      nstroke+=newtonstats.nstroke; newtonstats.nstroke=newtonstats.lntries=0;

      //If using a high order scheme, need to choose whether to trust the point value
      if(docons){
	MYFUN(check_point_vs_average(timeorder, numtimeorders, GLOBALMAC(pflag,i,j,k),MAC(pb,i,j,k),MAC(pf,i,j,k),MAC(myupoint,i,j,k),MAC(utoinvert,i,j,k),ptrgeom,&newtonstats),"advance.c:advance_finitevolume()", "check_point_vs_average()", 1);
      }


#if(DODISS||DODISSVSR)
      if(finalstep){
	// then see what entropy inversion would give
	diss_compute(EVOLVEUTOPRIM,UEVOLVE,MAC(useducum,i,j,k),ptrgeom,prbefore, MAC(pf,i,j,k),&newtonstats);
      }
#endif


#if(SPLITNPR)
      // don't update metric if only doing B1-B3
      if(advancepassnumber==-1 || advancepassnumber==1)
#endif
	{
	  // immediate local (i.e. 1-zone) fix
#if(FIXUPZONES==FIXUP1ZONE)
	  // SUPERGODMARK: Below should differentiate beteween whether want negative densities fixed or not, but right now fixup1zone() does all
	  if((STEPOVERNEGU==0)||(STEPOVERNEGRHO==0)||(STEPOVERNEGRHOU==0)||(finalstep)){
	    MYFUN(fixup1zone(MAC(pf,i,j,k), MAC(useducum,i,j,k), ptrgeom, finalstep),"advance.c:advance_finitevolume()", "fixup1zone()", 1);
	  }
#endif
	}
    }// end COMPZLOOP
  }// end parallel region





  /////////////////////////////////
  //
  // If not fixing up primitives after inversion immediately, then fix up all zones at once afterwards
  //
  /////////////////////////////////

#if(SPLITNPR)
  // don't update metric if only doing B1-B3
  if(advancepassnumber==-1 || advancepassnumber==1)
#endif
    {
#if(FIXUPZONES==FIXUPALLZONES)
      fixup(stage,pf,useducum,finalstep);
#endif  
    }



  /////////////////////////////////
  //
  // Determine next timestep from waves, fluxes, and source updates
  //
  /////////////////////////////////

  prepare_globaldt(ndt1,ndt2,ndt3,accdt,accdti,accdtj,accdtk,gravitydt,gravitydti,gravitydtj,gravitydtk,ndt);
 

#if(PRODUCTION==0)
  trifprintf( "2f");
#endif

  return (0);
}





// some dt calculations done at end of each substep
static int prepare_globaldt(
		     FTYPE ndt1,FTYPE ndt2,FTYPE ndt3,
		     FTYPE accdt,int accdti,int accdtj,int accdtk,
		     FTYPE gravitydt,int gravitydti,int gravitydtj,int gravitydtk,
		     FTYPE *ndt)
{
  FTYPE wavedt;
  int jj;



  wavedt = 1. / (1. / ndt1 + 1. / ndt2 + 1. / ndt3);
  *ndt = defcon * MIN(wavedt , accdt);

#if(USEGRAVITYDTINDTLIMIT)
  // use gravitydt (often too small, but sometimes accdt/ndt not small enough)
  *ndt = MIN(*ndt,gravitydt);
#endif

  gravitydtglobal = gravitydt;
  sourcedtglobal  = accdt; // accdt includes gravitydtglobal
  wavedtglobal    = wavedt;

#if(PRODUCTION==0)
  if(dt!=0.0){
    // report per-CPU time-step limited every 100 time steps

    // GODMARK: 1 : do always
    if(1|| nstep%DTr==0){
      fprintf(logdt_file,"nstep=%ld steppart=%d :: dt=%g ndt=%g ndt1=%g ndt2=%g ndt3=%g\n",nstep,steppart,dt,*ndt,ndt1,ndt2,ndt3);
      SLOOPA(jj) fprintf(logdt_file,"dir=%d wavedti=%d wavedtj=%d wavedtk=%d\n",jj,waveglobaldti[jj],waveglobaldtj[jj],waveglobaldtk[jj]);
      fprintf(logdt_file,"accdt=%g (accdti=%d accdtj=%d accdtk=%d) :: gravitydt=%g (gravitydti=%d gravitydtj=%d gravitydtk=%d) :: gravityskipstep=%d\n",accdt,accdti,accdtj,accdtk,gravitydt,gravitydti,gravitydtj,gravitydtk,gravityskipstep);
    }
  }
#endif


  return(0);
}











// check whether point conserved quantity inverted successfully to point primitive.
//   if unsuccessful, then see if want to revert to average conserved quantity and invert that
//   if Uavg->p unsuccessful, then leave as failure
// if Upoint->p is good, then check if p from Upoint is much different than p from Uavg.  If so, limit change

// upoint only needed for diagnostics
static int check_point_vs_average(int timeorder, int numtimeorders, PFTYPE *lpflag, FTYPE *pb, FTYPE *pf, FTYPE *upoint, FTYPE *uavg, struct of_geom *ptrgeom, struct of_newtonstats *newtonstats)
{
  FTYPE pavg[NPR];  //atch for temporary storage of primitives obtained from inverting the averaged conserved quantities
  int invert_from_point_flag, invert_from_average_flag;
  FTYPE frac_avg_used;  //this is be used for flux interpolation limiting
  int pl,pliter;
  int is_convergence_failure;
  int avgschemeatall;
  int finalstep;
  FTYPE limit_prim_correction( FTYPE fractional_difference_threshold, struct of_geom *geom, FTYPE *pin, FTYPE *pout );


  finalstep=timeorder == numtimeorders-1;



  avgschemeatall=(interporder[avgscheme[1]]>3) ||  (interporder[avgscheme[2]]>3) ||  (interporder[avgscheme[3]]>3);
  if(avgschemeatall==0) return(0); // since nothing to do


  invert_from_point_flag = lpflag[FLAGUTOPRIMFAIL];


  if( 0 && debugfail >= 1 && (IFUTOPRIMFAILSOFT(invert_from_point_flag)) ) {
    dualfprintf( fail_file, "t = %g, nstep = %ld, steppart = %d, i = %d, j = %d, rho = %21.15g, u = %21.15g, fracneg = %21.15g\n",
		 t, realnstep, steppart, ptrgeom->i + startpos[1], ptrgeom->j + startpos[2],
		 pf[RHO], pf[UU], (pf[RHO]>0)?(-pf[UU]/(pf[RHO]+DBL_MIN)):(-pf[RHO]/(pf[UU]+DBL_MIN)) );
  }


  //WHAT IF INTERNAL ENERGY BECOMES SLIGHTLY NEGATIVE?  WE STILL CAN DO THE LIMITING IN PRIM QUANTITIES! -- coorrected but check! -- SUPERSASMARK TODO atch
  if( LIMIT_AC_PRIM_FRAC_CHANGE &&
      (
       invert_from_point_flag == UTOPRIMNOFAIL || //atch added the below to still do the pt. vs. avg. check on primitives if the internal energy goes neg.
       ( (IFUTOPRIMFAILSOFTNOTRHORELATED(invert_from_point_flag)) && (0 != STEPOVERNEGU) ) || //intermediate substep with stepping over u < 0
       ( (IFUTOPRIMFAILSOFTRHORELATED(invert_from_point_flag)) && (0 != STEPOVERNEGRHO) ) //intermediate substep with stepping over rho < 0
       )
      ) {

    //make a copy of the initial guess so that not to modify the original pb's
    PLOOP(pliter,pl) pavg[pl] = pb[pl];
    //invert the average U -> "average" p
    MYFUN(Utoprimgen(finalstep,EVOLVEUTOPRIM, UEVOLVE, uavg, ptrgeom, pavg,newtonstats),"step_ch.c:advance()", "Utoprimgen", 3);

    invert_from_average_flag = GLOBALMACP0A1(pflag,ptrgeom->i,ptrgeom->j,ptrgeom->k,FLAGUTOPRIMFAIL);

    //Inversion from the average value succeeded or has a negative density or internal energy
    if( invert_from_average_flag == UTOPRIMNOFAIL  || IFUTOPRIMFAILSOFT(invert_from_average_flag) ) {
      //Inversion from both from the point and the average values succeeded
      //checks if the states' gamma factors and densities are different by more than a certain fraction
      //and if different, modify the point values such that they are not further than by MAX_AC_PRIM_FRAC_CHANGE
      //from the average ones
      frac_avg_used = limit_prim_correction(MAX_AC_PRIM_FRAC_CHANGE, ptrgeom, pavg, pf);

#if( DOENODEBUG )  //atch debug; note that use the location with pl =0 , interporflux = 0, & dir = 1 since limiting the change in prim quantities is not a per-component operation
      if(  frac_avg_used > 0.01 ) {
	MACP0A4(enodebugarray,ptrgeom->i,ptrgeom->j,ptrgeom->k,1-1,0,0,ENODEBUGPARAM_LIMCORR_PRIM)++;
      }
#endif
      
      if( pf[UU] < 0.0 && timeorder == numtimeorders-1 ) {
        lpflag[FLAGUTOPRIMFAIL] = UTOPRIMFAILU2AVG2;
      }

      //lpflag[FLAGUTOPRIMFAIL] = invert_from_point_flag;  //unneeded since it is alrady = UTOPRIMNOFAIL

    } // end if both point and average did NOT fail
    else {
      //Inversion from the point values succeeded but that from the average failed:
      //retain the point value

      //set the inversion error flag to correspond to the inversion from the average value
      lpflag[FLAGUTOPRIMFAIL] = invert_from_point_flag;
      frac_avg_used = 0.0;  //used point value, i.e. zero fracion of the average value
    }
  }
  else if( INVERTFROMAVERAGEIFFAILED && (invert_from_point_flag!=UTOPRIMNOFAIL) ) {  //failure  //atch correct
    //inversion from the point value failed

    // if last substep -> revert to the average value, else if only negative densities then allow for substep.  If other type of failures, then never allow and revert to Uavg


    // GODMARK:
    // CHECK IF INVERSION FAILED.  IF FAILED, TRY TO USE uavg:
    // invert U->p

    // GODMARK: decide whether best to revert to average or not

    //=1 if it is a non-convergence failure; =0 if it is an occurrence of a negative density
    is_convergence_failure = !IFUTOPRIMFAILSOFT(invert_from_point_flag);

    if((timeorder==numtimeorders-1 /*&& (1 == is_nonconvergence_failure || FIXUPZONES == FIXUPNOZONES)*/ ) ||   // last substep, then DO invert from average IF no fixups or non-convergence failure (ADT) SASMARKx

       ( 1 == is_convergence_failure ) || //non-convergence (no solution in primitive quantities) error, so have to fix it up

       ( (timeorder<numtimeorders-1) && (
					 ((IFUTOPRIMFAILSOFTNOTRHORELATED(invert_from_point_flag)) && (0 == STEPOVERNEGU))||
					 ((invert_from_point_flag==UTOPRIMFAILRHOUNEG) && (0 == STEPOVERNEGRHOU)) ||
					 ((IFUTOPRIMFAILSOFTRHORELATED(invert_from_point_flag)) && (0 == STEPOVERNEGRHO))
					 )
	 )  //intermediate substep with no stepping over u < 0, rho<0, or both <0
       ) {
      if(debugfail >= 1) {
	dualfprintf( fail_file, "Inversion from the point value failed.  Using the inversion from the average value.\n" );
      }

      //make a copy of the initial guess so that not to modify the original pb's
      PLOOP(pliter,pl) pf[pl] = pb[pl];
      //invert the average U -> "average" p
      MYFUN(Utoprimgen(finalstep,EVOLVEUTOPRIM, UEVOLVE, uavg, ptrgeom, pf,newtonstats),"step_ch.c:advance()", "Utoprimgen", 3);
      //      invert_from_average_flag = lpflag[FLAGUTOPRIMFAIL];


      //Have the results from the inversion from the average value -- copy the result over
      //      PLOOP(pliter,pl) pf[pl] = pavg[pl];
      //      lpflag[FLAGUTOPRIMFAIL] = invert_from_average_flag;

      //old code:
      //MYFUN(Utoprimgen(finalstep,EVOLVEUTOPRIM, UEVOLVE, avg, ptrgeom, pf,&newtonstats),"step_ch.c:advance()", "Utoprimgen", 2);

      frac_avg_used = 1.0; //reverted to the average value

    }
    else {
      frac_avg_used = 0.0; //used the point value
    }
  }

  return(0);
}





#define COMPARE_GAMMA 0

//If density or gamma-factors are different by more than fractional_difference_threshold for states pin & pout, 
//if different -- correct pout such that it is not more than fractional_difference_threshold away from pin.
// externally referenced
FTYPE limit_prim_correction( FTYPE fractional_difference_threshold, struct of_geom *geom, FTYPE *pin, FTYPE *pout )
{
  FTYPE gammain = 0.0, gammaout = 0.0;
  FTYPE frac_start, frac_end, frac_diff;
  FTYPE fraction_input_value;
  FTYPE comovingenergyin, comovingenergyout;
  int pl,pliter;
  FTYPE bsqin,bsqout;
  struct of_state qin, qout;
  int jj;
  FTYPE bdotuin, bdotuout;


#if( COMPARE_GAMMA ) 
  gamma_calc( pin, geom, &gammain );
  gamma_calc( pout, geom, &gammaout );
#endif

  get_state(pin, geom, &qin);
  get_state(pout, geom, &qout);

  bsqout = dot(qout.bcon, qout.bcov);
  bsqin = dot(qin.bcon, qin.bcov);

  bdotuin = 0.0;   DLOOPA(jj) bdotuin+=(qin.ucov[jj])*(qin.bcon[jj]);
  bdotuout = 0.0;  DLOOPA(jj) bdotuout+=(qout.ucov[jj])*(qout.bcon[jj]);

  // u.T.u comoving energy density
  comovingenergyin = pin[RHO] + pin[UU] + bsqin*0.5 - bdotuin*bdotuin;
  comovingenergyout = pout[RHO] + pout[UU] + bsqout*0.5 - bdotuout*bdotuout;


	
#if( COMPARE_GAMMA ) 
  frac_diff = MAX( fractional_diff(gammain, gammaout), 
		   fractional_diff( comovingenergyin, comovingenergyout ) );
#else
  frac_diff = fractional_diff( comovingenergyin, comovingenergyout );
#endif

  //fractional difference at which the reduction to the input value starts
  frac_start = 0.5 * fractional_difference_threshold;

  //fractional difference after which only the input value is used
  frac_end = fractional_difference_threshold;

  //the fraction of the input value used in the output; increases linearly from 0 to 1 for fractions going from frac_start to frac_end
  fraction_input_value = MAX( 0., MIN(1., (frac_diff - frac_start)/(frac_end - frac_start) ) );

  if( 0.0 != fraction_input_value ){
    //states are too different: reverted to primitives that correspond to average conserved quantities because trust them more than point values
    dualfprintf( fail_file, "States are too different, using %3d%% of the average values: i = %d, j = %d, k = %d, nstep = %ld, steppart = %d, t = %21.15g\n", 
		 (int)(100. * fraction_input_value), geom->i, geom->j, geom->k, nstep, steppart, t );
    if( debugfail >= 2 ){
      dualfprintf( fail_file, "Prim. pt. value (gamma, rho, u): " );
      dualfprintf( fail_file, "%21.15g %21.15g %21.15g\n",  gammaout, pout[RHO], pout[UU] );
      dualfprintf( fail_file, "Prim. avg value (gamma, rho, u): " );
      dualfprintf( fail_file, "%21.15g %21.15g %21.15g\n", gammain, pin[RHO], pin[UU] );
      dualfprintf( fail_file, "Frac. difference(ganna, rho, u): " );
      dualfprintf( fail_file, "%21.15g %21.15g %21.15g\n", 
		   fractional_diff(gammain, gammaout),
		   fractional_diff(pin[RHO], pout[RHO]),  
		   fractional_diff(pin[UU], pout[UU])
		   );
    }
  }

  PLOOP(pliter,pl) {
    pout[pl] = fraction_input_value * pin[pl] + (1. - fraction_input_value) * pout[pl];
  }

  return( fraction_input_value );
}



//Returns the fractional difference between a & b
static FTYPE fractional_diff( FTYPE a, FTYPE b )
{
  FTYPE frac_diff;

  frac_diff = 2. * fabs( a - b ) / ( fabs(a) + fabs(b) + DBL_MIN );

  return( frac_diff );

}

























// get dUavg
static void flux2dUavg(int i, int j, int k, FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE *dU1avg,FTYPE *dU2avg,FTYPE *dU3avg)
{
  FTYPE idx1,idx2,idx3;
  int pl,pliter;

#if(VOLUMEDIFF==0)
  idx1=1.0/dx[RR];
  idx2=1.0/dx[TH];
  idx3=1.0/dx[PH];
#else
  idx1=GLOBALMETMACP0A1(idxvol,i,j,k,RR);
  idx2=GLOBALMETMACP0A1(idxvol,i,j,k,TH);
  idx3=GLOBALMETMACP0A1(idxvol,i,j,k,PH);
#endif

  // initialize for simplicity so don't have to do it conditionally on N>1
  PLOOP(pliter,pl){
    dU1avg[pl]=0;
    dU2avg[pl]=0;
    dU3avg[pl]=0;
  }


  if(FLUXB==FLUXCD){ // don't use volume reg. since differencing is large
    PLOOPNOB1(pl){
#if(N1>1)
      dU1avg[pl]=(
		  - (MACP0A1(F1,ip1mac(i),j,k,pl) - MACP0A1(F1,i,j,k,pl)) *idx1
		  );
#endif
#if(N2>1)
      dU2avg[pl]=(
		  - (MACP0A1(F2,i,jp1mac(j),k,pl) - MACP0A1(F2,i,j,k,pl)) *idx2
		  );
#endif

#if(N3>1)
      dU3avg[pl]=(
		  - (MACP0A1(F3,i,j,kp1mac(k),pl) - MACP0A1(F3,i,j,k,pl)) *idx3
		  );
#endif

    }

    // simple version that assumes Fi[Bi] is set to 0 in flux.c for FLUXCD (which it is currently)
    PLOOPBONLY(pl){
#if(N1>1)
      dU1avg[pl]=(
		  - (MACP0A1(F1,ip1mac(i),j,k,pl) - MACP0A1(F1,im1mac(i),j,k,pl)) *idx1
		  );
#endif
#if(N2>1)
      dU2avg[pl]=(
		  - (MACP0A1(F2,i,jp1mac(j),k,pl) - MACP0A1(F2,i,jm1mac(j),k,pl)) *idx2
		  );
#endif
#if(N3>1)
      dU3avg[pl]=(
		  - (MACP0A1(F3,i,j,kp1mac(k),pl) - MACP0A1(F3,i,j,km1mac(k),pl)) *idx3
		  );
#endif
    }

    /*
    // old version
    // FIELDS are special.  The 0's would have come automatically, but spacing is twice the normal size.
    // the double spacing is accounted for in fluxct().
    pl=B1;
    #if(N1>1)
    dU1avg[pl]=(
    0.0
    );
    #endif
    #if(N2>1)
    dU2avg[pl]=(
    - (MACP0A1(F2,i,jp1mac(j),k,pl) - MACP0A1(F2,i,jm1mac(j),k,pl)) *idx2
    );
    #endif
    #if(N3>1)
    dU3avg[pl]=(
    - (MACP0A1(F3,i,j,kp1mac(k),pl) - MACP0A1(F3,i,j,km1mac(k),pl)) *idx3
    );
    #endif

    pl=B2;
    #if(N1>1)
    dU1avg[pl]=(
    - (MACP0A1(F1,ip1mac(i),j,k,pl) - MACP0A1(F1,im1mac(i),j,k,pl)) *idx1
    );
    #endif
    #if(N2>1)
    dU2avg[pl]=(
    0.0
    );
    #endif
    #if(N3>1)
    dU3avg[pl]=(
    - (MACP0A1(F3,i,j,kp1mac(k),pl) - MACP0A1(F3,i,j,km1mac(k),pl)) *idx3
    );
    #endif


    pl=B3;
    #if(N1>1)
    dU1avg[pl]=(
    - (MACP0A1(F1,ip1mac(i),j,k,pl) - MACP0A1(F1,im1mac(i),j,k,pl)) *idx1
    );
    #endif
    #if(N2>1)
    dU2avg[pl]=(
    - (MACP0A1(F2,i,jp1mac(j),k,pl) - MACP0A1(F2,i,jm1mac(j),k,pl)) *idx2
    );
    #endif
    #if(N3>1)
    dU3avg[pl]=(
    0.0
    );
    #endif
    // end old version
    */


    // rest of variables (if any) are normal
    PLOOPNOB2(pl){
#if(N1>1)
      dU1avg[pl]=(
		  - (MACP0A1(F1,ip1mac(i),j,k,pl) - MACP0A1(F1,i,j,k,pl)) *idx1
		  );
#endif
#if(N2>1)
      dU2avg[pl]=(
		  - (MACP0A1(F2,i,jp1mac(j),k,pl) - MACP0A1(F2,i,j,k,pl)) *idx2
		  );
#endif
#if(N3>1)
      dU3avg[pl]=(
		  - (MACP0A1(F3,i,j,kp1mac(k),pl) - MACP0A1(F3,i,j,k,pl)) *idx3
		  );
#endif

    }
  }

  else{


    // other (normal) FLUXB methods, including FLUXCTSTAG
    PLOOP(pliter,pl) {


#if(N1>1)
      dU1avg[pl]=(
		  - (MACP0A1(F1,ip1mac(i),j,k,pl) - MACP0A1(F1,i,j,k,pl)) *idx1
		  );
#endif
#if(N2>1)
      dU2avg[pl]=(
		  - (MACP0A1(F2,i,jp1mac(j),k,pl) - MACP0A1(F2,i,j,k,pl)) *idx2
		  );
#endif
#if(N3>1)
      dU3avg[pl]=(
		  - (MACP0A1(F3,i,j,kp1mac(k),pl) - MACP0A1(F3,i,j,k,pl)) *idx3
		  );
#endif
    }


  }




}





// convert point versions of U_i^{n} and dU -> U_i^{n+1} and other versions
static void dUtoU(int i, int j, int k, int loc, FTYPE *dUgeom, FTYPE *dUriemann, FTYPE *CUf, FTYPE *Cunew, FTYPE *Ui,  FTYPE *Uf, FTYPE *ucum)
{
  int pl,pliter;
  void dUtoU_check(int i, int j, int k, int loc, int pl, FTYPE *dUgeom, FTYPE *dUriemann, FTYPE *CUf, FTYPE *Cunew, FTYPE *Ui,  FTYPE *Uf, FTYPE *ucum);


  // finally assign new Uf and ucum
  // store uf to avoid recomputing U(pf) used later as pb for advance()
  PLOOP(pliter,pl) Uf[pl] = UFSET(CUf,dt,Ui[pl],Uf[pl],dUriemann[pl],dUgeom[pl]);


  // how much of Ui, dU, and Uf to keep for final solution
  // ultimately ucum is actual solution used to find final pf
  PLOOP(pliter,pl) ucum[pl] += UCUMUPDATE(Cunew,dt,Ui[pl],Uf[pl],dUriemann[pl],dUgeom[pl]);



#if(PRODUCTION==0)
  if(FLUXB!=FLUXCTSTAG){// turned off by default for FLUXB==FLUXCTSTAG since even with PRODUCTION==0, FLUXB==FLUXCTSTAG's extended loop causes output at edges.
    PLOOP(pliter,pl) dUtoU_check(i,j,k,loc,pl, dUgeom, dUriemann, CUf, Cunew, Ui,  Uf, ucum);
  }
#endif


}



// Check result of dUtoU()
static void dUtoU_check(int i, int j, int k, int loc, int pl, FTYPE *dUgeom, FTYPE *dUriemann, FTYPE *CUf, FTYPE *Cunew, FTYPE *Ui,  FTYPE *Uf, FTYPE *ucum)
{
  int showfluxes;
  void show_fluxes(int i, int j, int k, int loc, int pl,FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);


  // default
  showfluxes=0;

  if(!isfinite(Uf[pl])){
    dualfprintf(fail_file,"dUtoU after: nan found for Uf[%d]=%21.15g\n",pl,Uf[pl]);
    dualfprintf(fail_file,"pl=%d Ui=%21.15g dUriemann=%21.15g dugeom=%21.15g\n",pl,Ui[pl],dUriemann[pl],dUgeom[pl]);
    showfluxes=1;
  }
  if(!isfinite(ucum[pl])){
    dualfprintf(fail_file,"dUtoU after: nan found for ucum[%d]=%21.15g\n",pl,ucum[pl]);
    dualfprintf(fail_file,"pl=%d Ui=%21.15g dUriemann=%21.15g dugeom=%21.15g\n",pl,Ui[pl],dUriemann[pl],dUgeom[pl]);
    showfluxes=1;
  }

  if(showfluxes){
    show_fluxes(i,j,k,loc,pl,GLOBALPOINT(F1),GLOBALPOINT(F2),GLOBALPOINT(F3));
  }


}


// Check ucum
// externally referenced.
void ucum_check(int i, int j, int k, int loc, int pl, FTYPE *ucum)
{
  int showfluxes;


  // default
  showfluxes=0;

  if(!isfinite(ucum[pl])){
    dualfprintf(fail_file,"ucum_check: nan found at i=%d j=%d k=%d loc=%d for ucum[pl=%d]=%21.15g\n",i,j,k,loc,pl,ucum[pl]);
    showfluxes=1;
  }

  if(showfluxes){
    show_fluxes(i,j,k,loc,pl,GLOBALPOINT(F1),GLOBALPOINT(F2),GLOBALPOINT(F3));
  }


}

static void show_fluxes(int i, int j, int k, int loc, int pl,FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR])
{
    if(N1>1){
      dualfprintf(fail_file,"pl=%d i=%d j=%d k=%d :: F1[i]=%21.15g F1[i+1]=%21.15g dF1/dx1=%21.15g \n",pl,i,j,k,MACP0A1(F1,i,j,k,pl),MACP0A1(F1,i+SHIFT1,j,k,pl),(MACP0A1(F1,i+SHIFT1,j,k,pl)-MACP0A1(F1,i,j,k,pl))/dx[1]);
    }
    if(N2>1){
      dualfprintf(fail_file,"pl=%d i=%d j=%d k=%d :: F2[j]=%21.15g F2[j+1]=%21.15g dF2/dx2=%21.15g\n",pl,i,j,k,MACP0A1(F2,i,j,k,pl),MACP0A1(F2,i,j+SHIFT2,k,pl),(MACP0A1(F2,i,j+SHIFT2,k,pl)-MACP0A1(F2,i,j,k,pl))/dx[2]);
    }
    if(N3>1){
      dualfprintf(fail_file,"pl=%d i=%d j=%d k=%d :: F3[k]=%21.15g F3[k+1]=%21.15g dF3/dx3=%21.15g \n",pl,i,j,k,MACP0A1(F3,i,j,k,pl),MACP0A1(F3,i,j,k+SHIFT3,pl),(MACP0A1(F3,i,j,k+SHIFT3,pl)-MACP0A1(F3,i,j,k,pl))/dx[3]);
    }

}



// find global dt.  Useful if needed not during evolution, such as at t=0 or for restarting the run if restarting finished run that has a generally smaller dt than should use (including possibly dt=0)
// externally referenced
int set_dt(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], SFTYPE *dt)
{
  struct of_state state;
  struct of_geom geomdontuse;
  struct of_geom *ptrgeom=&geomdontuse;
  int i,j,k;
  int pl,pliter;
  int jj,kk,ll;
  int dir,ignorecourant;
  FTYPE cmax1,cmin1;
  FTYPE cmax2,cmin2;
  FTYPE cmax3,cmin3;
  int wavendti[NDIM],wavendtj[NDIM],wavendtk[NDIM];
  int accdti,accdtj,accdtk;
  int gravitydti,gravitydtj,gravitydtk;
  FTYPE tempwavedt,tempaccdt,tempgravitydt;
  FTYPE dtij[NDIM], wavedt, accdt, gravitydt;
  FTYPE wavendt[NDIM];
  FTYPE ndtfinal;
  FTYPE dUgeom[NPR],dUcomp[NUMSOURCES][NPR];
  int enerregion;
  int *waveenerpos, *sourceenerpos;
  FTYPE X[NDIM],V[NDIM],Vp1[NDIM];
  FTYPE dUriemann[NPR];
  FTYPE Ugeomfree[NPR],U[NPR];
  




  wavedt=accdt=gravitydt=ndtfinal=BIG;
  wavendt[1]=wavendt[2]=wavendt[3]=BIG;
  wavendti[1]=wavendtj[1]=wavendtk[1]=-100;
  wavendti[2]=wavendtj[2]=wavendtk[2]=-100;
  wavendti[3]=wavendtj[3]=wavendtk[3]=-100;
  accdti=accdtj=accdtk=-100;
  gravitydti=gravitydtj=gravitydtk=-100;

  enerregion=OUTSIDEHORIZONENERREGION; // consistent with flux update (except when doing WHAM)
  sourceenerpos=enerposreg[enerregion];

  COMPFULLLOOP{ // want to use boundary cells as well to limit dt (otherwise boundary-induced changes not treated)

    // includes "ghost" zones in case boundary drives solution
    get_geometry(i, j, k, CENT, ptrgeom);
    
    // need full state for vchar()
    MYFUN(get_state(MAC(prim,i,j,k), ptrgeom, &state),"advance.c:set_dt()", "get_state()", 1);
    
    
#if(N1>1)
    dir=1;
    MYFUN(vchar(MAC(prim,i,j,k), &state, dir, ptrgeom, &cmax1, &cmin1,&ignorecourant),"advance.c:set_dt()", "vchar() dir=1", 1);
    dtij[dir] = cour * dx[dir] / MAX(fabs(cmax1),fabs(cmin1));
    if (dtij[dir] < wavendt[dir]){
      wavendt[dir] = dtij[dir];
      wavendti[dir] = i;
      wavendtj[dir] = j;
      wavendtk[dir] = k;
    }
#endif
    
#if(N2>1)
    dir=2;
    MYFUN(vchar(MAC(prim,i,j,k), &state, dir, ptrgeom, &cmax2, &cmin2,&ignorecourant),"advance.c:set_dt()", "vchar() dir=2", 1);
    dtij[dir] = cour * dx[dir] / MAX(fabs(cmax2),fabs(cmin2));
    if (dtij[dir] < wavendt[dir]){
      wavendt[dir] = dtij[dir];
      wavendti[dir] = i;
      wavendtj[dir] = j;
      wavendtk[dir] = k;
    }
#endif

#if(N3>1)
    dir=3;
    MYFUN(vchar(MAC(prim,i,j,k), &state, dir, ptrgeom, &cmax3, &cmin3,&ignorecourant),"restart.c:set_dt()", "vchar() dir=3", 1);
    dtij[dir] = cour * dx[dir] / MAX(fabs(cmax3),fabs(cmin3));
    if (dtij[dir] < wavendt[dir]){
      wavendt[dir] = dtij[dir];
      wavendti[dir] = i;
      wavendtj[dir] = j;
      wavendtk[dir] = k;
    }
#endif



    if(WITHINENERREGION(sourceenerpos,i,j,k)){


#if(LIMITDTWITHSOURCETERM)

      // conserved quantity without geometry
      MYFUN(primtoU(UEVOLVE, MAC(prim,i,j,k), &state, ptrgeom, U),"step_ch.c:advance()", "primtoU()", 1);
      PLOOP(pliter,pl) Ugeomfree[pl] = U[pl]*(ptrgeom->IEOMFUNCNOSINGMAC(pl));

      // get source term
      // GODMARK: here dUriemann=0, although in reality this setting of dt is related to the constraint trying to make
      PLOOP(pliter,pl) dUriemann[pl]=0.0;
      MYFUN(source(MAC(prim,i,j,k), ptrgeom, &state, U, dUriemann, dUcomp, dUgeom),"advance.c:set_dt()", "source", 1);

      // get dt limit
      compute_dt_fromsource(ptrgeom,&state,MAC(prim,i,j,k), Ugeomfree, dUgeom, dUcomp[GEOMSOURCE], &tempaccdt, &tempgravitydt);
      if(accdt>tempaccdt){
	accdt=tempaccdt;
	accdti=i;
	accdtj=j;
	accdtk=k;
      }
      if(gravitydt>tempgravitydt){
	gravitydt=tempgravitydt;
	gravitydti=i;
	gravitydtj=j;
	gravitydtk=k;
      }

#if(0)// DEBUG
      if(i==-1 || i==0){
	dualfprintf(fail_file,"BANG: i=%d\n",i);
	PLOOP(pliter,pl) dualfprintf(fail_file,"prim[%d]=%21.15g\n",pl,MACP0A1(prim,i,j,k,pl));
	PLOOP(pliter,pl) dualfprintf(fail_file,"dUgeom[%d]=%21.15g dUcompgeomsource=%21.15g\n",pl,dUgeom[pl],dUcomp[GEOMSOURCE][pl]);
	if(i==-1){
	  // side-by-side
	  coord(i,j,k,CENT,X);
	  bl_coord(X,V);
	  coord(ip1mac(i),j,k,CENT,X);
	  bl_coord(X,Vp1);
	  dualfprintf(fail_file,"r(i=-1)=%21.15g r(i=0)=%21.15g\n",V[1],Vp1[1]);
	  dualfprintf(fail_file,"gdet(i=-1)=%21.15g gdet(i=0)=%21.15g\n",GLOBALMETMACP1A0(gdet,CENT,i,j,k),GLOBALMETMACP1A0(gdet,CENT,ip1mac(i),j,k));
	  DLOOP(jj,kk) dualfprintf(fail_file,"%d %d gcov(i=-1)=%21.15g gcov(i=0)=%21.15g\n",jj,kk,GLOBALMETMACP1A2(gcov,CENT,i,j,k,jj,kk),GLOBALMETMACP1A2(gcov,CENT,ip1mac(i),j,k,jj,kk));
	  DLOOP(jj,kk) dualfprintf(fail_file,"%d %d gcovlast(i=-1)=%21.15g gcovlast(i=0)=%21.15g\n",jj,kk,GLOBALMETMACP1A2(gcovlast,CENT,i,j,k,jj,kk),GLOBALMETMACP1A2(gcovlast,CENT,ip1mac(i),j,k,jj,kk));
	  DLOOP(jj,kk) dualfprintf(fail_file,"%d %d gcon(i=-1)=%21.15g gcon(i=0)=%21.15g\n",jj,kk,GLOBALMETMACP1A2(gcon,CENT,i,j,k,jj,kk),GLOBALMETMACP1A2(gcon,CENT,ip1mac(i),j,k,jj,kk));
	  DLOOP(jj,kk) DLOOPA(ll) dualfprintf(fail_file,"%d %d %d conn(i=-1)=%21.15g conn(i=0)=%21.15g\n",jj,kk,ll,GLOBALMETMACP0A3(conn,i,j,k,jj,kk,ll),GLOBALMETMACP0A3(conn,ip1mac(i),j,k,jj,kk,ll));
	}
      }
#endif


#endif


    } // end if within source enerregion

  } // end of loop


  // GODMARK: note that in normal advance, wavendt[i] is over each CPU region and wavedt computed for each CPU and then minimized over all CPUs -- so not perfectly consistent with MPI
  // here we preserve perfect MPI domain decomposition
  mpifmin(&wavendt[1]);
  mpifmin(&wavendt[2]);
  mpifmin(&wavendt[3]);
  // single all-CPU wavedt
  wavedt = 1.0/(1.0/wavendt[1]+1.0/wavendt[2]+1.0/wavendt[3]); // wavendt[i] is over entire region for each i

  // single all-CPU accdt and gravitydt
  mpifmin(&accdt);
  mpifmin(&gravitydt);

  wavedtglobal=wavedt;
  sourcedtglobal=accdt;
  gravitydtglobal=gravitydt;


  // find global minimum value of wavendt over all cpus
  ndtfinal=MIN(wavedt,MIN(accdt,gravitydt));

#if(1)
  // below single line only right if 1-CPU
  SLOOPA(jj) dualfprintf(log_file,"dtij[%d]=%21.15g wavendti=%d wavendtj=%d wavendtk=%d\n",jj,wavendt[jj],wavendti[jj],wavendtj[jj],wavendtk[jj]);
  dualfprintf(log_file,"ndtfinal=%21.15g wavedt=%21.15g accdt=%21.15g gravitydt=%21.15g\n",ndtfinal,wavedt,accdt,gravitydt); 
  dualfprintf(log_file,"accdti=%d accdtj=%d accdtk=%d :: gravitydti=%d  gravitydtj=%d  gravitydtk=%d\n",accdti,accdtj,accdtk,gravitydti,gravitydtj,gravitydtk);
#endif

  *dt = ndtfinal;
  // don't step beyond end of run
  if (t + *dt > tf) *dt = tf - t;
  
  return(0);
}


// 0.5 not good enough for pressureless collapse
// normal cour=0.8/4 works for presureless collapse for longsteps, so use 0.1 to be safe since rarely gravity conquers timestep
// but cour=0.8 and GRAVITYCOUR = 0.1 doesn't even work for longsteps!
#define GRAVITYCOUR (0.1)

static int compute_dt_fromsource(struct of_geom *ptrgeom, struct of_state *state, FTYPE *pr, FTYPE *U, FTYPE *dUevolve, FTYPE *dUgeomevolve, FTYPE *dtij, FTYPE *gravitydt)
{
  FTYPE dUd[NDIM],dUu[NDIM];
  int jj,kk;
  FTYPE rhoprime[NDIM];
  FTYPE ag[NDIM],dtsource[NDIM];
  FTYPE rho,u,P,bsq,w,eta;
  FTYPE mydU[NDIM];
  FTYPE mydUgravity, rhoprimegravity, aggravity;
  FTYPE frac;
  int i,j,k,loc;
  extern void compute_dr(int i, int j, int k, FTYPE *dr);
  FTYPE dr,dphidt,phi,tempdt;
  FTYPE veleff;
  FTYPE v1max,v1min;
  FTYPE mydx[NDIM];


  i=ptrgeom->i;
  j=ptrgeom->j;
  k=ptrgeom->k;
  loc=ptrgeom->p;

  // default is no limit on dt due to flux differential or source terms
  *dtij=BIG;
  // default is no limit due to time-dependence of gravity
  *gravitydt=BIG;



  DLOOPA(jj){
    dUd[jj]=dUevolve[UU+jj]*(ptrgeom->IEOMFUNCNOSINGMAC(UU+jj)); // remove geometry
  }
  raise_vec(dUd,ptrgeom,dUu);

  // comparing time update with time value, so keep lower as conserved quantity
  mydU[TT]=dUd[TT];
  mydUgravity = dUgeomevolve[UU]*(ptrgeom->IEOMFUNCNOSINGMAC(UU)); // pure gravity piece

  mydx[TT]=dt; // so in the end dt_time <=C*dt/(dU/rhoprime)
  SLOOPA(jj){
    // treating momentum update as giving dv^i, so for getting Courant condition of moving across grid, need upper
    mydU[jj] = dUu[jj];
    mydx[jj] = dx[jj];
  }
    

  bsq = dot(state->bcon, state->bcov);
  rho=pr[RHO];
  u=pr[UU];
  P=pressure_rho0_u_simple(i,j,k,loc,rho,u);
  w = rho+u+P;
  eta = w+bsq;





  /////////////////////////
  //
  // New method for dealing with source terms
  //
  /////////////////////////
#if(1)
  DLOOPA(jj){
    // U[UU] is like eta but with \gamma^2 factors that are present in the momentum terms
    // account for geometry prefactor of conserved quantities and put in geometry consistent with source term
    // U[UU] \sim eta \gamma^2 when neglecting stress terms
    // GODMARK: Might want to be more careful like in utoprim_jon.c in how normalize errors
    // GODMARK: Consider REMOVERESTMASSFROMUU==2 effects
    rhoprime[jj]=MAX(fabs(eta),fabs(U[UU]));

    // update to 3-velocity v^i is approximately due to this acceleration
    // ag^j \sim (\rho\gamma^2 v^i)/(\rho\gamma^2) \sim dv^i/dt
    // below is really = acceleration * dt
    ag[jj]=SMALL+fabs(mydU[jj]/rhoprime[jj]); // acceleration.  SMALL is so doesn't identically vanish and we get nan below

    // for time, idea is to keep d\rho/\rho\lesssim cour, so dtnew = dtold *\rho/d\rho = dtold/ag[TT]
    if(jj==TT) dtsource[jj]=cour*(mydx[jj]/ag[jj]);
    // dt = dx/ag since ag is really  = dv^i = dx^i/dt
    else dtsource[jj]=cour*(mydx[jj]/ag[jj]); // characteristic time-step for increasing velocity so mass would cross a grid
      
  }




  /////////////////////////
  //
  // Self-gravity
  //
  /////////////////////////

#if(DOSELFGRAVVSR)
  // make sure metric is not varying too fast
  // just look at perturbed part of g_{tt} -- an invariant in stationary metrics, so good indicator of gauge-invariant time-dependence of metric
  // only look at loc=CENT since others should be similar to order unity -- also CENT won't diverge at r=0
  //  frac = (METMACP1A1(gcovpertlast,CENT,i,j,k,TT) - METMACP1A1(gcovpert,CENT,i,j,k,TT))/(fabs(METMACP1A1(gcovpertlast,CENT,i,j,k,TT)) + fabs(METMACP1A1(gcovpert,CENT,i,j,k,TT)));
  // above frac has no dt, so no measure of what dt should be

  // \Gamma^t_{tt} measures dg_{tt}/dt
  // \Gamma^t_{tt} \approx g_{tt},t g^{tt}  so that g_{tt},t = \Gamma^t_{tt}/g^{tt}
  // now form same construct as with (dU/dt)/U as above
  // g^{tt} can't go to 0 unless in bad coordinate system (BL-coords on horizon)
  // frac is approximately g_{tt},t/g^{tt} \aprox 1/dt
  // GODMARK: below might be problem in non-relativistic case



  // get \Gamma_{ttt} = dphi/dt ~ 1/2 g_{tt,t}
  dphidt=0.0;
  DLOOPA(jj) dphidt += GLOBALMETMACP0A3(conn,i,j,k,jj,TT,TT)*(ptrgeom->gcov[GIND(jj,TT)]);
  dphidt = fabs(dphidt); // don't care about sign, just magnitude

  // get \phi ~ -(g_{tt} +1)/2
  // phi by itself has no meaning, but as a reference for changes in phi in time it does
  phi = -(1.0+ptrgeom->gcov[GIND(TT,TT)])*0.5;
  phi = fabs(phi); // sign not important

#if(0)

  // treat dt ~ \phi / (d\phi/dt)
  //  frac = fabs(GLOBALMETMACP0A3(conn,i,j,k,TT,TT,TT)/((1+GLOBALMETMACP1A2(gcon,CENT,i,j,k,TT,TT))*(1+GLOBALMETMACP1A2(gcon,CENT,i,j,k,TT,TT))));
  frac = fabs(dphidt/phi);
  *gravitydt = cour*(GRAVITYCOUR/frac);
  // this dt keeps frac~cour
  //  *gravitydt = cour*(GRAVITYCOUR/frac); // GRAVITYCOUR is additional courant factor on gravitational term


  // treat d\phi/dt as v^2/dt
  compute_dr( i,  j,  k, &dr);
  //dgttdt = fabs(GLOBALMETMACP0A3(conn,i,j,k,TT,TT,TT)/(GLOBALMETMACP1A2(gcon,CENT,i,j,k,TT,TT)));
  //tempdt = GRAVITYCOUR*pow(cour*cour*dr*dr/dgttdt,1.0/3.0); // GRAVITYCOUR in front is effective additional courant factor on gravitational term
  tempdt = GRAVITYCOUR*pow(cour*cour*dr*dr/dphidt,1.0/3.0); // GRAVITYCOUR in front is effective additional courant factor on gravitational term

  if(tempdt<*gravitydt) *gravitydt=tempdt;
#elif(0)

  frac = fabs(ptrgeom->gcon[GIND(TT,TT)]*dphidt);
  *gravitydt = cour*(GRAVITYCOUR/frac);




#elif(1)

  ///////////////////////////////
  //
  // 3 methods to limit dt
  //
  //////////////////////////////

  // LOCAL DPHI/DT LIMIT
  // treat dt ~ \phi / (d\phi/dt)
  //  frac = fabs(GLOBALMETMACP0A3(conn,i,j,k,TT,TT,TT)/((1+GLOBALMETMACP1A2(gcon,CENT,i,j,k,TT,TT))*(1+GLOBALMETMACP1A2(gcon,CENT,i,j,k,TT,TT))));
  frac = fabs(dphidt/phi);
  *gravitydt = cour*(GRAVITYCOUR/frac);
  // this dt keeps frac~cour
  //  *gravitydt = cour*(GRAVITYCOUR/frac); // GRAVITYCOUR is additional courant factor on gravitational term



  compute_dr( i,  j,  k, &dr);


  // LOCAL DU/DT LIMIT BUT TREAT AS VELOCITY LIMITED BY SOL

#if(REMOVERESTMASSFROMUU==2)
  rhoprimegravity=fabs(U[UU]); // gravity affects only \rho \phi -like terms order \rho v^2, not \rho
#else
  // remove rest-mass
  rhoprimegravity=fabs(U[UU]+U[RHO]); // gravity affects only \rho \phi -like terms order \rho v^2, not \rho
#endif
  aggravity=SMALL+fabs(mydUgravity/rhoprimegravity);
  veleff = aggravity*mydx[1];

  // get speed of light in 1-direction (dx^1/dt)
  sol(pr,state,1,ptrgeom,&v1max,&v1min);
  // limit "velocity" to speed of light
  if(veleff>fabs(v1min)) veleff=fabs(v1min);
  if(veleff>fabs(v1max)) veleff=fabs(v1max);

  tempdt=GRAVITYCOUR*cour*mydx[1]/veleff;
  if(tempdt<*gravitydt) *gravitydt=tempdt;


  // LOCAL LIMIT ON DPHI/DT WHERE PHI~V^2
  //dgttdt = fabs(GLOBALMETMACP0A3(conn,i,j,k,TT,TT,TT)/(GLOBALMETMACP1A2(gcon,CENT,i,j,k,TT,TT)));
  //tempdt = GRAVITYCOUR*pow(cour*cour*dr*dr/dgttdt,1.0/3.0); // GRAVITYCOUR in front is effective additional courant factor on gravitational term
  tempdt = GRAVITYCOUR*pow(cour*cour*dr*dr/dphidt,1.0/3.0); // GRAVITYCOUR in front is effective additional courant factor on gravitational term
  if(tempdt<*gravitydt) *gravitydt=tempdt;


#endif




#endif // end if DOSELFGRAVVSR==1





#else // else other older method for gravitydt


  /////////////////////////
  //
  // Old method for dealing with source terms
  //
  /////////////////////////

  // as expected, this method essentialy divides by 0 do dt->0

  SLOOPA(jj){
    // U[UU] is like eta but with \gamma^2 factors that are present in the momentum terms
    // account for geometry prefactor of conserved quantities and put in geometry consistent with source term
    // U[UU] \sim eta \gamma^2 when neglecting stress terms
    // GODMARK: Might want to be more careful like in utoprim_jon.c in how normalize errors
    // GODMARK: Consider REMOVERESTMASSFROMUU==2 effects
    rhoprime[jj]=MAX(fabs(eta),fabs(U[UU]));

    // update to 3-velocity v^i is approximately due to this acceleration
    ag[jj]=SMALL+fabs(mydU[jj]/rhoprime[jj]); // acceleration.  SMALL is so doesn't identically vanish and we get nan below

    dtsource[jj]=cour*sqrt(mydx[jj]/ag[jj]); // characteristic time-step for increasing velocity so mass would cross a grid
      
  }

  // do pure gravity piece (very similar to how full TT source term dealt with)
  //  rhoprimegravity=MAX(fabs(eta),fabs(U[UU]));
  // The below term is \propto \rho Mvsr(r)/r when v=0, so is well-defined
  // GODMARK: Can this quantity go to zero near rotating BH or something?
#if(REMOVERESTMASSFROMUU==2)
  rhoprimegravity=fabs(U[UU]); // gravity affects only \rho \phi -like terms order \rho v^2, not \rho
#else
  // remove rest-mass
  rhoprimegravity=fabs(U[UU]+U[RHO]); // gravity affects only \rho \phi -like terms order \rho v^2, not \rho
#endif
  aggravity=SMALL+fabs(mydUgravity/rhoprimegravity);
  dtsource[TT] = *gravitydt=cour*(mydx[TT]/aggravity);



#endif  





  /////////////////////////
  //
  // Finally store source term's version of limited dt to be used later
  //
  /////////////////////////
  

  //  dualfprintf(fail_file,"i=%d mydUgravity=%21.15g rhoprimegravity=%21.15g rhoprime[TT]=%21.15g mydU[TT]=%21.15g\n",ptrgeom->i,mydUgravity,rhoprimegravity,rhoprime[TT],mydU[TT]);

  // always do time-component
  // accounts for thermal changes if cooling function or geometry changes if metric changing
  if (dtsource[TT] < *dtij) *dtij = dtsource[TT];

#if(N1>1)
  if (dtsource[RR] < *dtij) *dtij = dtsource[RR];
#endif
#if(N2>1)
  if (dtsource[TH] < *dtij) *dtij = dtsource[TH];
#endif
#if(N3>1)
  if (dtsource[PH] < *dtij) *dtij = dtsource[PH];
#endif


  return(0);

}



// compute dt from full coordinate acceleration
static int dUtodt(struct of_geom *ptrgeom, struct of_state *qaddr, FTYPE *pr, FTYPE *dUgeom, FTYPE *dUriemann, FTYPE *dUgeomgravity, FTYPE *accdt, FTYPE *gravitydt)
{
  int pl,pliter;
  FTYPE dUtotal[NPR];
  FTYPE Ugeomfree[NPR],U[NPR];



  PLOOP(pliter,pl) {
    // while each piece may be "large", when summed if small then final change to conserved quantity is small, so that's all that's relevant
    dUtotal[pl]=dUriemann[pl]+dUgeom[pl];
    // GODMARK:
    //    dUtotal[pl]=fabs(dUriemann[pl])+fabs(dUgeom[pl]);
  }

  // conserved quantity without geometry
  MYFUN(primtoU(UEVOLVE, pr, qaddr, ptrgeom, U),"step_ch.c:advance()", "primtoU()", 1);
  PLOOP(pliter,pl) Ugeomfree[pl] = U[pl]*(ptrgeom->IEOMFUNCNOSINGMAC(pl));


  compute_dt_fromsource(ptrgeom, qaddr, pr, Ugeomfree, dUtotal, dUgeomgravity, accdt, gravitydt);
    


  return(0);

}

