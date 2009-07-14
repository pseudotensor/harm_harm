#include "decs.h"



int setRin_withchecks(FTYPE *rin)
{
  FTYPE Rhor,rminus;
 
  Rhor=rhor_calc(0);
  *rin=setRin(setihor());

  //  (*rin) = 0.98 * Rhor;
  

  trifprintf("R0=%21.15g Rhor=%21.15g Rin=%21.15g Rout=%21.15g ihor=%d\n",R0,Rhor,(*rin),Rout,setihor());

  if((*rin)<R0){
    dualfprintf(fail_file,"Wrong Rin calculation\n");
    myexit(34968346);
  }
  if((*rin)<0.0){
    dualfprintf(fail_file,"Very Poor Rin<0 calculation\n");
    myexit(34968347);
  }
  rminus=rhor_calc(1);
  if((*rin)<rminus){
    dualfprintf(fail_file,"Rin<r_- : Poor Rin calculation\n");
    myexit(34968348);
  }
  if((*rin)>Rhor){
    dualfprintf(fail_file,"Rin>r_+ : Poor Rin calculation\n");
    myexit(34968349);
  }


  return(0);
}

// some very common, but not completely general, user-type functions for init.c

int user1_prepre_init_specific_init(void)
{

  // choice// GODMARK: not convenient location, but needed for init_mpi()
  periodicx1=0;
  periodicx2=0;
#if(USEMPI&&N3!=1)
  periodicx3=1;// GODMARK: periodic in \phi for 3D spherical polar
#else
  periodicx3=0;
#endif


  return(0);

}



int user1_init_conservatives(int *fieldfrompotential, FTYPE (*prim)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*Utemp)[NSTORE2][NSTORE3][NPR], FTYPE (*U)[NSTORE2][NSTORE3][NPR])
{
  int pl,pliter;
  
  PLOOPBONLY(pl) trifprintf("fieldfrompotential[%d]=%d\n",pl-B1+1,fieldfrompotential[pl-B1+1]);


  trifprintf("begin init_conservatives\n");
  pi2Uavg(fieldfrompotential, prim, pstag, Utemp, U);
  trifprintf("end init_conservatives\n");

  return(0);

}


int user1_post_init_specific_init(void)
{
  // globally used parameters set by specific initial condition routines, reran for restart as well *after* all other calculations

  UTOPRIMVERSION = UTOPRIMJONNONRELCOMPAT;

  //  UTOPRIMVERSION =   UTOPRIM5D1;
  //    UTOPRIMVERSION =   UTOPRIM2DFINAL;

  return(0);
}


int user1_init_global(void)
{
  int pl,pliter;

  DODIAGEVERYSUBSTEP = 0;


  SAFE=1.3;
  //  cour = 0.9;
  //  cour=0.8;
  cour = 0.4999; // Changed to cour=0.5 because noticed that with STOREWAVESPEEDS==1 that extremely unstable with 0.8 near pole due to larger or smaller wave speeds from max-averaging from CENT to FACE1/2/3.  While may be inconsistency in Riemann solution, LAXF is simply a diffusive solution so shouldn't be so dependent upon exact wave speed.  Noticed that with one setup where instability appeared (large radii 512x32 type run) that forcing timestep to be like STOREWAVESPEEDS==0 led to much more stable solution, but still kinda unstable to a saturation point.  That was with cour=0.83*0.8=664.  For safety, use cour=0.4999.  In any case, this is motivated by fact that only with cour<0.5 will Riemann waves from interface not intersect by a single timestep, and only with non-intersecting waves does Riemann solution make sense.  So cour>0.5 is really too aggressive.  Still keep multi-dimen effective courant reducer since that's there for different reasons of multi-dimen stability issues.

  ///////////////////////
  //
  // ENO-RELATED STUFF
  //
  ///////////////////////
  
  INVERTFROMAVERAGEIFFAILED = 1;
  LIMIT_AC_PRIM_FRAC_CHANGE = 1;
  MAX_AC_PRIM_FRAC_CHANGE = 0.1;

  LIMIT_AC_FRAC_CHANGE=0; // CHANGINGMARK: avoiding complication for now
  // have to make consistent with weno-minimization for fluxes
  MAX_AC_FRAC_CHANGE=0.2;

  // need MAXBND==17 if not evolving point field and doing full WENO5BND
  // test1103 N1=8 is smallest tried and new simple_ limiting works at 10% very well
  //dofluxreconevolvepointfield=0;
  // only need MAXBND==11 like normal higher-order method (like FV method)
  dofluxreconevolvepointfield=1;



#if(EOMTYPE==EOMGRMHD || EOMTYPE==EOMCOLDGRMHD)


  //avgscheme[1]=avgscheme[2]=avgscheme[3]=WENO5BND;
  //  lim[1] = lim[2] = lim[3] = WENO5BND;
  lim[1] = lim[2] = lim[3] = PARALINE;

  avgscheme[1]=avgscheme[2]=avgscheme[3]=DONOR; // CHANGINGMARK
  //lim[1] = lim[2] = lim[3] = MC; // CHANGINGMARK


  DOENOFLUX = NOENOFLUX;
  //DOENOFLUX = ENOFLUXRECON; // CHANGINGMARK
  //DOENOFLUX = ENOFINITEVOLUME;

  if(DOENOFLUX == ENOFLUXRECON){
    // below applies to all fluxes
    PALLLOOP(pl) do_transverse_flux_integration[pl] = 1;
    PLOOPBONLY(pl) do_transverse_flux_integration[pl] = 1;
    // below used for re-averaging of field in advance.c for dBhat/dt method
    PALLLOOP(pl) do_conserved_integration[pl] = 1;
    PLOOPBONLY(pl) do_conserved_integration[pl] = 1;
  }

  if(DOENOFLUX == ENOFINITEVOLUME){
    PALLLOOP(pl) do_transverse_flux_integration[pl] = 1;
    PLOOPBONLY(pl) do_transverse_flux_integration[pl] = 1;
    PALLLOOP(pl) do_source_integration[pl] = 0;
    PLOOPBONLY(pl) do_source_integration[pl] = 0;
    PALLLOOP(pl) do_conserved_integration[pl] = 1;
    PLOOPBONLY(pl) do_conserved_integration[pl] = 1;
    //    do_conserved_integration[B1-1+DIRZ] = 1;
  }



  FLUXB = FLUXCTSTAG;  // CHANGINGMARK
  //  FLUXB = FLUXCTHLL;
  //FLUXB = FLUXCTTOTH;
  //  TIMEORDER=2;
  TIMEORDER=4;
  //  TIMEORDER=3;
  //  fluxmethod= HLLFLUX;
  fluxmethod= LAXFFLUX; // generally more robust than HLLFLUX for various reasons
  

  //  UTOPRIMVERSION=UTOPRIM5D1;
  UTOPRIMVERSION = UTOPRIMJONNONRELCOMPAT;
  //  UTOPRIMVERSION = UTOPRIM1DFINAL;


#elif(EOMTYPE==EOMFFDE)
  // PARA and TO=4 and HLL not trustable in FFDE so far
  lim[1] =lim[2]=lim[3]= MC;
  TIMEORDER=2;


  // below applies to all fluxes
  PALLLOOP(pl) do_transverse_flux_integration[pl] = 1;
  PLOOPBONLY(pl) do_transverse_flux_integration[pl] = 1;
  // below used for re-averaging of field in advance.c for dBhat/dt method
  PALLLOOP(pl) do_conserved_integration[pl] = 1;
  PLOOPBONLY(pl) do_conserved_integration[pl] = 1;



  fluxmethod=LAXFFLUX; // generally more robust than HLLFLUX
  FLUXB = FLUXCTTOTH;
  UTOPRIMVERSION=UTOPRIM2DFINAL;
  // whether/which ENO used to interpolate fluxes
  DOENOFLUX = ENOFINITEVOLUME;
  //  DOENOFLUX= NOENOFLUX;
  //DOENOFLUX=ENOFLUXRECON;
#endif



  ranc(1,0); // no MPI method yet, so just pure randomization
  /* some physics parameters */
  gam = 4. / 3.;
  cooling=0;


  BCtype[X1DN]=OUTFLOW;
  BCtype[X2UP]=POLARAXIS;
  BCtype[X2DN]=POLARAXIS;
  BCtype[X3UP]=PERIODIC;
  BCtype[X3DN]=PERIODIC;

  BCtype[X1UP]=OUTFLOW;
  //  rescaletype=1;
  rescaletype=4;
  BSQORHOLIMIT=1E2; // was 1E2 but latest BC test had 1E3 // CHANGINGMARK
  BSQOULIMIT=1E3; // was 1E3 but latest BC test had 1E4
  UORHOLIMIT=1E3;
  RHOMIN = 1E-4;
  UUMIN = 1E-6;


  // below floor model is only used if rescaletype!=4
  if(BCtype[X1UP]==FIXEDOUTFLOW){ // then doing bondi inflow
    // avoids constant floor activation -- trying to be more physical
    prfloorcoef[RHO]=RHOMIN/100.0;
    prfloorcoef[UU]=UUMIN/100.0;
  }
  else{
    prfloorcoef[RHO]=RHOMIN;
    prfloorcoef[UU]=UUMIN;
  }

  BCtype[X1UP]=OUTFLOW;
  //  rescaletype=1;
  rescaletype=4;
  BSQORHOLIMIT=1E2; // was 1E2 but latest BC test had 1E3 // CHANGINGMARK
  BSQOULIMIT=1E3; // was 1E3 but latest BC test had 1E4
  UORHOLIMIT=1E3;
  RHOMIN = 1E-4;
  UUMIN = 1E-6;


  /* dumping frequency, in units of M */
  DTdumpgen[DTDISS]=DTdumpgen[DTFLUX]=DTdumpgen[DTOTHER]=DTdumpgen[DTEOS]=DTdumpgen[DTVPOT]=DTdumpgen[DTDUMP] = 50.;
  DTdumpgen[DTAVG] = 50.0;
  // ener period
  DTdumpgen[DTENER] = 2.0;
  /* image file frequ., in units of M */
  DTdumpgen[DTIMAGE] = 2.0;
  // fieldline locked to images so can overlay
  DTdumpgen[DTFIELDLINE] = DTdumpgen[DTIMAGE];

  /* debug file */  
  DTdumpgen[DTDEBUG] = 50.0;
  // DTr = .1 ; /* restart file frequ., in units of M */
  /* restart file period in steps */
  DTr = 3000;


  tf=2E3; // very problem dependent, should override

  return(0);
}


// assumes normalized density
int user1_init_atmosphere(int *whichvel, int*whichcoord,int i, int j, int k, FTYPE *pr)
{
  int pl,pliter;
  struct of_geom realgeomdontuse;
  struct of_geom *ptrrealgeom=&realgeomdontuse;
  FTYPE pratm[NPR];




  get_geometry(i, j, k, CENT, ptrrealgeom); // true coordinate system
  set_atmosphere(-1,WHICHVEL,ptrrealgeom,pratm); // set velocity in chosen WHICHVEL frame in any coordinate system

  if(pr[RHO]<pratm[RHO]){
    PLOOP(pliter,pl) pr[pl]=pratm[pl];
  }
  

  *whichvel=WHICHVEL;
  *whichcoord=PRIMECOORDS;
  return(0);


}



int user1_init_primitives(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*panalytic)[NSTORE2][NSTORE3][NPR], FTYPE (*pstaganalytic)[NSTORE2][NSTORE3][NPR], FTYPE (*vpotanalytic)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhatanalytic)[NSTORE2][NSTORE3][NPR],
			  FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR], FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3])
{
  int whichvel, whichcoord;
  int initreturn;
  int i = 0, j = 0, k = 0, l;
  FTYPE r,th,X[NDIM],V[NDIM];
  int normalize_densities(FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
  int normalize_field(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR]);
  int init_dsandvels(int *whichvel, int *whichcoord, int i, int j, int k, FTYPE *p, FTYPE *pstag);
  int init_atmosphere(int *whichvel, int *whichcoord, int i, int j, int k, FTYPE *pr);
  int pl,pliter;


  ///////////////////////////////////
  //
  // Assign primitive variables
  //
  ///////////////////////////////////
  trifprintf("Assign primitives\n");

  // assume we start in bl coords and convert to KSprim
  // so field defined when get to floor model (fixup)
  init_3dnpr_fullloop(0.0,prim);



  //////////////////////
  //
  // assume we start in bl coords and convert to KSprim
  //
  //////////////////////
#pragma omp parallel private(i,j,k,initreturn,whichvel,whichcoord) OPENMPGLOBALPRIVATEFULL
  {
    OPENMP3DLOOPVARSDEFINE;
  ////////  COMPFULLLOOP{
    OPENMP3DLOOPSETUPFULL;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);

      initreturn=init_dsandvels(&whichvel, &whichcoord,i,j,k,MAC(prim,i,j,k),MAC(pstag,i,j,k)); // request densities for all computational centers
      if(initreturn>0){
	FAILSTATEMENT("init.c:init_primitives()", "init_dsandvels()", 1);
      }
      else MYFUN(transform_primitive_vB(whichvel, whichcoord, i,j,k, prim, pstag),"init.c:init_primitives","transform_primitive_vB()",0);
    }
  }// end parallel region


  /////////////////////////////
  //
  // normalize density if wanted
  //
  /////////////////////////////// 
  // at this point densities are still standard, so just send "prim"
  trifprintf("Normalize densities\n");
  normalize_densities(prim);




  /////////////////////////////
  //
  // Define an atmosphere if wanted
  //
  /////////////////////////////// 

#if(EOMTYPE==EOMGRMHD || EOMTYPE==EOMCOLDGRMHD)
  // normalized atmosphere
  trifprintf("Add atmosphere\n");

#pragma omp parallel private(i,j,k,initreturn,whichvel,whichcoord) OPENMPGLOBALPRIVATEFULL
  {
    OPENMP3DLOOPVARSDEFINE;
    ///////  COMPZLOOP {
    OPENMP3DLOOPSETUPZLOOP;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);

      initreturn=init_atmosphere(&whichvel, &whichcoord,i,j,k,MAC(prim,i,j,k));
      if(initreturn>0){
	FAILSTATEMENT("init.c:init_primitives()", "init_atmosphere()", 1);
      }
      else{
	// transform from whichcoord to MCOORD
	if (bl2met2metp2v(whichvel, whichcoord,MAC(prim,i,j,k), i,j,k) >= 1){
	  FAILSTATEMENT("init.c:init()", "bl2ks2ksp2v()", 1);
	}
      }
    }// end 3D LOOP
  }// end parallel region
#endif// end if EOMTYPE is EOMGRMHD or EOMCOLDGRMHD




  /////////////////////////////
  //
  // Set analytics
  //
  /////////////////////////////// 


#if(ANALYTICMEMORY)
  // copy over initial solution as analytic solution
  // NEEDED FOR BOUND in case uses panalytic
  copy_prim2panalytic(prim,panalytic,pstag,pstaganalytic,vpot,vpotanalytic,Bhat,Bhatanalytic);
#endif

  /////////////////////////////
  //
  // Fixup and Bound variables since some primitive quantities may have changed
  // These may be used to define vector potential below
  // Also setup pre_fixup() type quantities
  //
  /////////////////////////////// 
  trifprintf("Fixup and Bound #1\n");

#if(FIXUPAFTERINIT)
  if(fixup(STAGEM1,prim,ucons,0)>=1) FAILSTATEMENT("init.c:init()", "fixup()", 1);
#endif

  if (bound_prim(STAGEM1,0.0,prim, pstag, Bhat, 1) >= 1) FAILSTATEMENT("init.c:init()", "bound_prim()", 1);

  // now fully bounded, initialize interpolations in case interpolate using prim/pstag data
  pre_interpolate_and_advance(prim);

  if(pre_fixup(STAGEM1,prim)>=1) FAILSTATEMENT("init.c:init()", "pre_fixup()", 1);



  
  /////////////////////////////
  //
  // Initialize field from vector potential
  //
  /////////////////////////////// 
#if(1)
  init_vpot(prim,pstag,ucons,vpot,Bhat,F1,F2,F3,Atemp);
  normalize_field(prim,pstag,ucons,vpot,Bhat); // normalizes p and pstag and unew and vpot if tracked
#else
  // no field
  init_zero_field(prim,pstag,ucons,vpot,Bhat);
#endif




#if(ANALYTICMEMORY)
  // copy over initial solution as analytic solution
  // NEEDED FOR BOUND in case uses panalytic
  copy_prim2panalytic(prim,panalytic,pstag,pstaganalytic,vpot,vpotanalytic,Bhat,Bhatanalytic);
#endif

  /////////////////////////////
  //
  // Fixup and Bound variables since some primitive quantities may have changed
  // These may be used to define vector potential below
  // Also setup pre_fixup() type quantities
  //
  //
  // BOUND AGAIN IN CASE USING PANALYTIC TO BOUND!
  //
  /////////////////////////////// 
  trifprintf("Fixup and Bound #2\n");

#if(FIXUPAFTERINIT)
  if(fixup(STAGEM1,prim,ucons,0)>=1) FAILSTATEMENT("init.c:init()", "fixup()", 1);
#endif

  if (bound_allprim(STAGEM1,0.0,prim,pstag,ucons, 1) >= 1) FAILSTATEMENT("init.c:init()", "bound_allprim()", 1);

  // now fully bounded, initialize interpolations in case interpolate using prim/pstag data
  pre_interpolate_and_advance(prim);


  if(pre_fixup(STAGEM1,prim)>=1) FAILSTATEMENT("init.c:init()", "pre_fixup()", 1);




  return(0);


}







int user1_init_vpot2field_user(int *fieldfrompotential, FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR])
{
  int i,j,k,pl,pliter;
  int toreturn;
  extern int set_fieldfrompotential(int *fieldfrompotential);
   

  // uses ptemparray as temporary variable
  // use PLOOP (not PLOOPBONLY) since rho,u,etc. used for interpolation in some cases
  copy_3dnpr_fullloop(prim,GLOBALPOINT(ptemparray));

  set_fieldfrompotential(fieldfrompotential);

  // obtain primitive magnetic field from vector potential
  // uses ptemparray as temporary variable.  Uses emf as temporary variable.
  toreturn=vpot2field(A,GLOBALPOINT(ptemparray),pstag,ucons,Bhat,GLOBALPOINT(F1),GLOBALPOINT(F2),GLOBALPOINT(F3),GLOBALPOINT(emf),GLOBALPOINT(ulastglobal));


  // Can override vector potential choice for some field components, like B3 in axisymmetry
  // see init.sasha.c

  ////////////////////
  //
  // copy back
  // don't override
  //
  ////////////////////
  copy_3d_fieldonly_fullloop(GLOBALPOINT(ptemparray),prim);

  return(toreturn);

}




// assumes we are fed the true densities
// OPENMPOPTMARK: Too user specific to worry about optimizing
int user1_normalize_densities(int eqline, FTYPE *parms, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE *rhomax, FTYPE *umax)
{
  int i,j,k;
  FTYPE X[NDIM],V[NDIM],r,th;
  FTYPE rin,rhodisk;
  

  *rhomax=0;
  *umax=0;
  rin=parms[0];
  rhodisk=parms[1];


  COMPZLOOP {
    bl_coord_ijk_2(i, j, k, CENT, X, V);
    r=V[1];
    th=V[2];

    if (MACP0A1(prim,i,j,k,RHO) > *rhomax)   *rhomax = MACP0A1(prim,i,j,k,RHO);
    if(eqline){
      if (MACP0A1(prim,i,j,k,UU) > *umax && (r > rin))    *umax = MACP0A1(prim,i,j,k,UU);
    }
    else{
      if (MACP0A1(prim,i,j,k,UU) > *umax )    *umax = MACP0A1(prim,i,j,k,UU);
    }
  }


  ////////////
  //
  // Find max over all MPI procs
  //
  ////////////
  mpimax(rhomax);
  mpimax(umax);
  trifprintf("rhomax: %21.15g umax: %21.15g\n", *rhomax, *umax);


  ////////////
  //
  // Normalize densities
  //
  ////////////

  COMPZLOOP{
    MACP0A1(prim,i,j,k,RHO) *= rhodisk/(*rhomax);
    MACP0A1(prim,i,j,k,UU)  *= rhodisk/(*rhomax);
  }


  ////////////
  //
  // Reset maximum u and rho as consistent with renormalization
  //
  ////////////

  *umax *= rhodisk/(*rhomax);
  *rhomax = rhodisk;

  return(0);
}



// assumes we are fed the true densities
// OPENMPOPTMARK: No benefit from OpenMP due to critical region required
int user1_getmax_densities(FTYPE (*prim)[NSTORE2][NSTORE3][NPR],SFTYPE *rhomax, SFTYPE *umax)
{
  int i,j,k;
  FTYPE X[NDIM],V[NDIM],r,th;
  

  *rhomax=0;
  *umax=0;


  ZLOOP{
    bl_coord_ijk_2(i, j, k, CENT, X, V);
    r=V[1];
    th=V[2];
      
    //    dualfprintf(fail_file,"rho=%g u=%g\n",MACP0A1(prim,i,j,k,RHO),MACP0A1(prim,i,j,k,UU));
     
    if (MACP0A1(prim,i,j,k,RHO) > *rhomax)   *rhomax = MACP0A1(prim,i,j,k,RHO);
    if (MACP0A1(prim,i,j,k,UU) > *umax )    *umax = MACP0A1(prim,i,j,k,UU);
  }


  ////////////
  //
  // Find max over all MPI procs
  //
  ////////////
  mpimax(rhomax);
  mpimax(umax);


  trifprintf("rhomax: %21.15g umax: %21.15g\n", *rhomax, *umax);

  return(0);
}



// get maximum b^2 and p_g
// OPENMPOPTMARK: No benefit from OpenMP due to critical region required and too user specific
int user1_get_maxes(int eqslice, FTYPE *parms, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE *bsq_max, FTYPE *pg_max)
{
  int i,j,k;
  FTYPE bsq_ij,pg_ij;
  struct of_geom geomdontuse;
  struct of_geom *ptrgeom=&geomdontuse;
  FTYPE X[NDIM],V[NDIM];
  FTYPE dxdxp[NDIM][NDIM];
  FTYPE r,th;
  int gotnormal;
  FTYPE rin;
  int loc;

  
  

  rin=parms[0];


  pg_max[0]= 0.;
  bsq_max[0] = SMALL;
  gotnormal=0; // to check if ever was in location where wanted to normalize
  loc=CENT;

  ZLOOP {
    get_geometry(i, j, k, loc, ptrgeom);

    if(eqslice){
      bl_coord_ijk_2(i, j, k, loc, X, V);
      r=V[1];
      th=V[2];
      
      if((r>rin)&&(fabs(th-M_PI*0.5)<4.0*M_PI*dx[2]*hslope)){
	gotnormal=1;
	if (bsq_calc(MAC(prim,i,j,k), ptrgeom, &bsq_ij) >= 1) FAILSTATEMENT("init.c:init()", "bsq_calc()", 1);
	if (bsq_ij > bsq_max[0])      bsq_max[0] = bsq_ij;

	pg_ij=pressure_rho0_u_simple(i,j,k,loc,MACP0A1(prim,i,j,k,RHO),MACP0A1(prim,i,j,k,UU));
	if (pg_ij > pg_max[0])      pg_max[0] = pg_ij;
      }
    }
    else{
      gotnormal=1;
      if (bsq_calc(MAC(prim,i,j,k), ptrgeom, &bsq_ij) >= 1) FAILSTATEMENT("init.c:init()", "bsq_calc()", 1);
      if (bsq_ij > bsq_max[0])      bsq_max[0] = bsq_ij;

      pg_ij=pressure_rho0_u_simple(i,j,k,loc,MACP0A1(prim,i,j,k,RHO),MACP0A1(prim,i,j,k,UU));
      if (pg_ij > pg_max[0])      pg_max[0] = pg_ij;

    }
  }


  mpiisum(&gotnormal);

  if(gotnormal==0){
    dualfprintf(fail_file,"Never found place to normalize field\n");
    if(N2==1 && N3==1){
      ZLOOP {
	bl_coord_ijk_2(i, j, k, loc, X, V);
	dxdxprim_ijk(i, j, k, loc, dxdxp);
	r=V[1];
	th=V[2];

	dualfprintf(fail_file,"i=%d j=%d k=%d V[1]=%21.15g dxdxp[1][1]*dx[1]=%21.15g dx[1]=%21.15g\n",i,j,k,V[1],dxdxp[1][1]*dx[1],dx[1]);
      }
    }
    myexit(111);
  }

  mpimax(bsq_max);
  mpimax(pg_max);


  return(0);

}


// assumes normal field definition
int user1_normalize_field(FTYPE beta, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR])
{
  FTYPE bsq_max, norm, beta_act;
  FTYPE mypgmax;
  int get_maxes(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE *bsq_max, FTYPE *mypgmax);


  // get initial maximum
  get_maxes(prim, &bsq_max, &mypgmax);
  trifprintf("initial bsq_max: %21.15g pgmax: %21.15g\n", bsq_max,mypgmax);

  // get normalization parameter
  beta_act = mypgmax / (0.5 * bsq_max);
  norm = sqrt(beta_act / beta);
  trifprintf("initial beta: %21.15g (should be %21.15g) norm=%21.15g\n", beta_act,beta,norm);

  
  // not quite right since only correct static field energy, not moving field energy
  normalize_field_withnorm(norm, prim, pstag, ucons, vpot, Bhat);

  // get new maxes to check if beta is correct
  get_maxes(prim, &bsq_max, &mypgmax);
  trifprintf("new initial bsq_max: %21.15g\n", bsq_max);

  beta_act = mypgmax / (0.5 * bsq_max);
  trifprintf("new bsq_max: %21.15g\n", bsq_max);
  trifprintf("final beta: %21.15g (should be %21.15g)\n", beta_act,beta);


  return(0);
}


// UUMIN/RHOMIN used for atmosphere

// for each WHICHVEL possibility, set atmosphere state for any coordinate system
// which=0 : initial condition
// which=1 : evolution condition (might also include a specific angular momentum or whatever)
// which==1 assumes pr set to something locally reasonable, and we adjust to that slowly

#define TAUADJUSTATM (10.0) // timescale for boundary to adjust to using preset inflow
int user1_set_atmosphere(int atmospheretype, int whichcond, int whichvel, struct of_geom *ptrgeom, FTYPE *pr)
{
  FTYPE rho,u,ur,uh,up;
  FTYPE X[NDIM],V[NDIM];
  FTYPE r,th;
  FTYPE prlocal[NPR];
  int pl,pliter;

  // Bondi like initial atmosphere
  //    rho = RHOMIN * 1.E-14;
  //    u = UUMIN * 1.E-14;
  bl_coord_ijk_2(ptrgeom->i, ptrgeom->j, ptrgeom->k, ptrgeom->p, X, V);
  r=V[1];
  th=V[2];

  // default
  PLOOP(pliter,pl) prlocal[pl]=pr[pl];

  if((EOMTYPE==EOMGRMHD)||(EOMTYPE==EOMCOLDGRMHD)){
    // Bondi-like atmosphere
    if(rescaletype==4){
      if(atmospheretype==1){
	// couple rescaletype to atmosphere type
	prlocal[RHO] = RHOMIN*pow(r,-2.0);
      }
      else if(atmospheretype==2){
	// couple rescaletype to atmosphere type
	if(r>40.0) prlocal[RHO] = RHOMIN*pow(r,-2.0);
	else prlocal[RHO] = RHOMIN*pow(40.0,-2.0);
      }
    }
    else{
      prlocal[RHO] = RHOMIN*pow(r,-1.5);
    }
  }
  else{
    prlocal[RHO] = 0;
  }


  if(EOMTYPE==EOMGRMHD){
    // Bondi-like atmosphere
    prlocal[UU]  = UUMIN*pow(r,-2.5);
  }
  else{
    prlocal[UU]  = 0;
  }

    
  // bl-normal observer (4-vel components)
  
  // normal observer velocity in atmosphere
  if(whichvel==VEL4){
    prlocal[U1] = -ptrgeom->gcon[GIND(0,1)]/sqrt(-ptrgeom->gcon[GIND(0,0)]) ;
    prlocal[U2] = -ptrgeom->gcon[GIND(0,2)]/sqrt(-ptrgeom->gcon[GIND(0,0)]) ;
    prlocal[U3] = -ptrgeom->gcon[GIND(0,3)]/sqrt(-ptrgeom->gcon[GIND(0,0)]) ;
  }
  else if(whichvel==VEL3){
    prlocal[U1] = ptrgeom->gcon[GIND(0,1)]/ptrgeom->gcon[GIND(0,0)] ;
    prlocal[U2] = ptrgeom->gcon[GIND(0,2)]/ptrgeom->gcon[GIND(0,0)] ;
    prlocal[U3] = ptrgeom->gcon[GIND(0,3)]/ptrgeom->gcon[GIND(0,0)] ;
    // GAMMIE
    //ur = -1./(r*r);
    //uh=up=0.0;
  }
  else if(whichvel==VELREL4){
    prlocal[U1] = 0.0;
    prlocal[U2] = 0.0;
    prlocal[U3] = 0.0;
  }
  
  if(whichcond==1){
    if(100.0*dt>TAUADJUSTATM){
      dualfprintf(fail_file,"dt=%21.15g and TAUADJUSTATM=%21.15g\n",dt,TAUADJUSTATM);
      myexit(1);
    }
    // TAUADJUSTATM must be >> dt always in order for this to make sense (i.e. critical damping to fixed solution)
    PLOOP(pliter,pl) pr[pl] = pr[pl]+(prlocal[pl]-pr[pl])*dt/TAUADJUSTATM;
  }
  else if(whichcond==0){ 
    PLOOP(pliter,pl) pr[pl] = prlocal[pl];
    // very specific
    // always outflow field
    //    pr[B1] = pr[B2] = pr[B3] = 0;
  }
  else if(whichcond==-1){ 
    // t=0, just sets as "floor"
    PLOOP(pliter,pl) pr[pl] = prlocal[pl];
    pr[B1] = pr[B2] = pr[B3] = 0;
  }


  return(0);

}



