
#include "decs.h"


static void set_position_stores(void);
static void set_grid_metrics(void);
static void set_connection(void);
static void set_idxvol(void);


// translate compgeom to some local quantities at loc,i,j,k
int assignmetricstorage_new(struct of_compgeom *mygeom, FTYPE **localgcov, FTYPE **localgcon, FTYPE **localgcovpert, FTYPE **localgdet, FTYPE **localgdetvol, FTYPE **localalphalapse, FTYPE **localbetasqoalphasq, FTYPE **localbeta, FTYPE **localeomfunc)
{

#if(NEWMETRICSTORAGE==1)


  *localgcov=mygeom->gcov;
  *localgcon=mygeom->gcon;
  *localgcovpert=mygeom->gcovpert;
  *localgdet=&(mygeom->gdet);
#if(GDETVOLDIFF)
  *localgdetvol=&(mygeom->gdetvol);
#endif
  *localalphalapse=&(mygeom->alphalapse);
  *localbetasqoalphasq=&(mygeom->betasqoalphasq);
  *localbeta=mygeom->beta;
  *localeomfunc=&(mygeom->EOMFUNCMAC(0));


#endif
  return(0);
}


// old use of arrays: translate compgeom to some local quantities at loc,i,j,k
// uses globals
int assignmetricstorage_old(int loc, int i, int j, int k, FTYPE **localgcov, FTYPE **localgcon, FTYPE **localgcovpert, FTYPE **localgdet, FTYPE **localgdetvol, FTYPE **localalphalapse, FTYPE **localbetasqoalphasq, FTYPE **localbeta, FTYPE **localeomfunc)
{
#if(NEWMETRICSTORAGE==0)


  *localgcov=GLOBALMETMACP1A0(gcov,loc,i,j,k);
  *localgcon=GLOBALMETMACP1A0(gcon,loc,i,j,k);
  *localgcovpert=GLOBALMETMACP1A0(gcovpert,loc,i,j,k);
  *localgdet=&GLOBALMETMACP1A0(gdet,loc,i,j,k);
#if(GDETVOLDIFF)
  *localgdetvol=&GLOBALMETMACP1A0(gdetvol,loc,i,j,k);
#endif
  *localalphalapse=&GLOBALMETMACP1A0(alphalapse,loc,i,j,k);
  *localbetasqoalphasq=&GLOBALMETMACP1A0(betasqoalphasq,loc,i,j,k);
  *localbeta=GLOBALMETMACP1A0(beta,loc,i,j,k);

#if(WHICHEOM!=WITHGDET)
  *localeomfunc=GLOBALMETMACP1A0(eomfunc,loc,i,j,k);
#else
  *localeomfunc=&GLOBALMETMACP1A0(gdet,loc,i,j,k)
#endif


#endif

  return(0);
}

// old use of arrays: translate compgeom to some local quantities at loc,i,j,k
// uses globals
int assignmetricstorage_oldlast(int loc, int i, int j, int k, FTYPE **localgcov, FTYPE **localgcon, FTYPE **localgcovpert, FTYPE **localgdet, FTYPE **localgdetvol, FTYPE **localalphalapse, FTYPE **localbetasqoalphasq, FTYPE **localbeta, FTYPE **localeomfunc)
{
#if(NEWMETRICSTORAGE==0)

  *localgcovlast=GLOBALMETMACP1A0(gcovlast,loc,i,j,k);
  //  *localgconlast=GLOBALMETMACP1A0(gconlast,loc,i,j,k);
  *localgcovpertlast=GLOBALMETMACP1A0(gcovpertlast,loc,i,j,k);
  //  *localgdetlast=&GLOBALMETMACP1A0(gdetlast,loc,i,j,k);
#if(GDETVOLDIFF)
  //  *localgdetvollast=&GLOBALMETMACP1A0(gdetvollast,loc,i,j,k);
#endif
  *localalphalapselast=&GLOBALMETMACP1A0(alphalapselast,loc,i,j,k);
  //  *localbetasqoalphasqlast=&GLOBALMETMACP1A0(betasqoalphasqlast,loc,i,j,k);
  //  *localbetalast=GLOBALMETMACP1A0(betalast,loc,i,j,k);
  //#if(WHICHEOM!=WITHGDET)
  //  *localeomfunclast=GLOBALMETMACP1A0(eomfunclast,loc,i,j,k);
  //#else
  //  *localeomfunclast=&GLOBALMETMACP1A0(gdetlast,loc,i,j,k)
  //#endif


#endif

  return(0);
}




// GODMARK: may want to make grid functions explicitly 2D for axisymmetric space-times when in axisymmetry with space-time axis aligned with grid.

// set up all grid functions
//
// whichtime: 0: setting initial coordinate and metric quantities.  Can be called many times to solve initial value problem of coupled matter-metric system of equations.  Should NOT be treated as a single call for entire simulation.
//            1: Setting a future metric such that old metric can be used to compute connection with temporal changes incorporated
//
// CUf/Cunew: time-step for substeps used to iterate the metric and store into old metric when can take temporal difference and use as slope for present value of connection calculation
//
void set_grid(int whichtime, FTYPE *CUf, FTYPE *Cunew)
{
  extern void set_rvsr(void);
  extern void control_time_store_metric(int whichtime, FTYPE *Cunew);
  extern void set_drsing(void);
  extern void set_rvsr(void);
  extern int bound_spacetime_inside_horizon(void);
  extern int store_old_metric(void);



  // NOTE:
  // don't assume we enter here with whichtime==0 only once since may have to "iteratively" obtain metric from stress-energy tensor and metric
  // this happens at t=0 in init() when DOSELFGRAVVSR==1


  // choose time of metric and whether to store metric before overwritten
  control_time_store_metric(whichtime,Cunew);


  /* set up boundaries, steps in coordinate grid */
  if(whichtime==0){
    dt=1.0; // dummy value that should lead to 0 connection coefficient for derivatives in time
    // just avoids 0/0 that should really be 0
  }
  // set_points() is purely dealing with coordinates, not metric quantities
  // but sets dx[0]=dt
  set_points();



  /////////////////////////////////////////
  //
  // first compute coordinate labels, which never change in time right now
  //
  /////////////////////////////////////////
  if(whichtime==0){
    // set minimum dr used to smooth metric (requires set_points())
    // assume drsing doesn't change (i.e. coordinates don't change)
    trifprintf("set_drsing() START\n");
    set_drsing();
    trifprintf("set_drsing() END\n");
    trifprintf("drsing=%21.15g\n",drsing);
  
    if(DOSELFGRAVVSR){
      // set rvsr
      // assume r doesn't change with time
      trifprintf("Setting r(i)\n");
      set_rvsr();
      trifprintf("Done setting r(i)\n");
    }


    //////////////
    //
    // set positional stores
    //
    //////////////
#if(DOSTOREPOSITIONDATA)
    trifprintf("set_position_stores() BEGIN\n");
    set_position_stores();
    didstorepositiondata=1;
    trifprintf("set_position_stores() END\n");
#endif // end if DOSTOREPOSITIONDATA==1    

  }// end if whichtime==0





  //////////////
  //
  // set metric
  //
  //////////////

  // if here, then doing over again
  if(whichtime==0) trifprintf("set_grid_metrics() BEGIN\n");
  didstoremetricdata=0;
  set_grid_metrics();
  // if here, then did store new metric data
  didstoremetricdata=1;
  if(whichtime==0) trifprintf("set_grid_metrics() END\n");


  //////////////
  //
  // set connection
  //
  //////////////
  if(whichtime==0) trifprintf("set_connection() BEGIN\n");
  set_connection();
  if(whichtime==0) trifprintf("set_connection() END\n");
  

  //////////////
  //
  // set idxvol
  //
  //////////////

  if(VOLUMEDIFF){
    if(whichtime==0) trifprintf("set_idxvol() BEGIN\n");
    set_idxvol();
    if(whichtime==0) trifprintf("set_idxvol() END\n");
  }


  // set boundary conditions on metric
  // this only modifies outside unused computational regions, so can come last.  One might imagine that bounding metric would change connection calculation...true but connection bounded too.  +-1 issues? GODMARK
  if(DOEVOLVEMETRIC){
    if(whichtime==0) trifprintf("bound_spacetime_inside_horizon() BEGIN\n");
    bound_spacetime_inside_horizon();
    if(whichtime==0) trifprintf("bound_spacetime_inside_horizon() END\n");
  }


  if(DOEVOLVEMETRIC && whichtime==0){
    // if first time to call set_grid() (i.e. set_grid(0)), then store present metric as old metric
    // if evolving metric, then store old metric before computing new one
    // store metric, needed to have dg/dt terms in connection coefficients
    // initially dg/dt = 0
    // This should come after metric calculation and after connection calculation so that conneciton has old and new metric rather than new and new metric
    if(whichtime==0) trifprintf("store_old_metric() START\n");
    trifprintf("Storing old metric initially\n");
    store_old_metric();
    if(whichtime==0) trifprintf("store_old_metric() END\n");
  }




  /* done! */
}






// set positional stores
static void set_position_stores(void)
{
  extern void dxdxprim(FTYPE *X, FTYPE *V, FTYPE (*dxdxp)[NDIM]);


#pragma omp parallel 
  {
    int i, j, k;
    int loc;


    // over grid locations needing these quantities
    for (loc = NPG - 1; loc >= 0; loc--) {


      //////////    COMPFULLLOOPP1
      OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUPFULLP1;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);
	
	
	// store X since can be expensive to keep recomputing these things, esp. if bl_coord() involves alot of complicated functions
	// X must be always allowed to vary in space
	coord(i, j, k, loc, GLOBALMACP1A0(Xstore,loc,i,j,k));
	
      }// end internal loop block
    }// end over locations
  }// end parallel region (and implied barrier)




  // separately do other non-X stores since X has no MCOORD==CARTMINKMETRIC conditional
#pragma omp parallel  // no ucon or EOS stuff needed
  {
    int i, j, k;
    int loc;


    // over grid locations needing these quantities
    for (loc = NPG - 1; loc >= 0; loc--) {

      //////////    COMPFULLLOOPP1
      OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUPFULLP1;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize)) nowait
      OPENMP3DLOOPBLOCK{
	OPENMP3DLOOPBLOCK2IJK(i,j,k);


#if(MCOORD==CARTMINKMETRIC)
	// doesn't depend on position, only store/use 1 value
	if(i!=0 || j!=0 || k!=0) continue; // simple way to avoid other i,j,k when doing OpenMP
#endif
      
      

	// store V, dxdxp since can be expensive to keep recomputing these things, esp. if bl_coord() involves alot of complicated functions
	bl_coord(GLOBALMACP1A0(Xstore,loc,i,j,k),GLOBALMETMACP1A0(Vstore,loc,i,j,k));
	dxdxprim(GLOBALMACP1A0(Xstore,loc,i,j,k),GLOBALMETMACP1A0(Vstore,loc,i,j,k),GLOBALMETMACP1A0(dxdxpstore,loc,i,j,k));

	//	SLOOPA(jj) dualfprintf(fail_file,"loc=%d i=%d j=%d k=%d :: V[%d]=%21.15g\n",loc,i,j,k,jj,GLOBALMETMACP1A0(Vstore,loc,i,j,k)[jj]);

	matrix_inverse(PRIMECOORDS, GLOBALMETMACP1A0(dxdxpstore,loc,i,j,k),GLOBALMETMACP1A0(idxdxpstore,loc,i,j,k));
      }// end internal loop block
    }// end over locations
  }// end parallel region (and implied barrier)

}















///////////////////
//
// Grid functions that only exist at many locations and are assigned
// values on all points INCLUDING another value at the outer edges
// so have edge grid data there -- makes setting up certain things
// easier
//
// Notice that coord() and bl_coord() work without this.  So those
// functions that only require those functions can do without this
// extra grid stuff.
//
//////////////////
static void set_grid_metrics(void)
{


  
  //  dualfprintf(fail_file,"Computing metric stuff\n");


#pragma omp parallel 
  {
    int i, j, k, l, m;
    int ii, jj, kk, ll;
    int pl,pliter;
    FTYPE X[NDIM];
    FTYPE V[NDIM];
    struct of_geom geomdontuse;
    struct of_geom *ptrgeom=&geomdontuse;
    int loc;
    extern void gcov_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcovpert);
    extern void gcon_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcon);
    extern void eomfunc_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *EOMFUNCNAME);
    void assign_eomfunc(struct of_geom *geom, FTYPE *EOMFUNCNAME);
    LOCALMETRICTEMPVARS;


    //////////    COMPFULLLOOPP1
    OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUPFULLP1;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);


#if(MCOORD==CARTMINKMETRIC)
      // doesn't depend on position, only store/use 1 value
      if(i!=0 || j!=0 || k!=0) continue; // simple way to avoid other i,j,k when doing OpenMP
#endif
    
      // over grid locations needing these quantities
      for (loc = NPG - 1; loc >= 0; loc--) {

	bl_coord_ijk_2(i,j,k,loc,X, V);
	/////////////////
	//
	// (1,MCOORD) here actually means PRIMCOORDS since the "1" means convert MCOORD to PRIMCOORDS.

	// fake geom just for indices
	ptrgeom->i=i;
	ptrgeom->j=j;
	ptrgeom->k=k;
	ptrgeom->p=loc;

	// get local metric quantities for this loc,i,j,k
	GETLOCALMETRIC(loc,i,j,k);


	// get metric terms and anything in struct of_geom
	gcov_func(ptrgeom,1,MCOORD,X, localgcov,localgcovpert);
	localgdet[0] = gdet_func_metric(MCOORD,V,localgcov);
	gcon_func(ptrgeom,1,MCOORD,X,localgcov,localgcon);
#if(WHICHEOM!=WITHGDET || NEWMETRICSTORAGE==1)
	// this is consistent with EOMFUNCMAC stuff as long as eomfunc_func() has been macrofied
	eomfunc_func(ptrgeom,1,MCOORD,X,localeomfunc);
#endif
	alphalapse_func(ptrgeom,1,MCOORD,X,localgcov,localgcon,localalphalapse);
	betasqoalphasq_func(ptrgeom,1,MCOORD,X,localgcov,localgcon,localbetasqoalphasq);
	beta_func(ptrgeom,1,MCOORD,X,localgcov,localgcon,*localalphalapse,localbeta);
	if(GDETVOLDIFF){
	  // uses X,V (not det) from all locations
	  gdetvol_func(ptrgeom,localgdet,localeomfunc,localgdetvol);
	}


#if(NEWMETRICSTORAGE)
	GLOBALMETMACP1A0(compgeom,loc,i,j,k).i=ptrgeom->i;
	GLOBALMETMACP1A0(compgeom,loc,i,j,k).j=ptrgeom->j;
	GLOBALMETMACP1A0(compgeom,loc,i,j,k).k=ptrgeom->k;
	GLOBALMETMACP1A0(compgeom,loc,i,j,k).p=ptrgeom->p;


	// then store extra things in global geom structure
	// refer to "_old" version so really computs it
	set_igdet_old(&GLOBALMETMACP1A0(compgeom,loc,i,j,k)); // full 1/gdet and 1/EOMFUNCMAC(pl)

	// go ahead and assign eomfunc as per variable as either original eomfunc or gdet
	assign_eomfunc(&GLOBALMETMACP1A0(compgeom,loc,i,j,k), &(GLOBALMETMACP1A0(compgeom,loc,i,j,k).EOMFUNCMAC(0)));


	GLOBALMETMACP0A1(gdetgeom,i,j,k,loc).gdet=GLOBALMETMACP1A0(compgeom,loc,i,j,k).gdet;


	GLOBALMETMACP0A1(gdetgeom,i,j,k,loc).igdetnosing=GLOBALMETMACP1A0(compgeom,loc,i,j,k).igdetnosing;


	PLOOP(pliter,pl){
	  GLOBALMETMACP0A1(gdetgeom,i,j,k,loc).EOMFUNCMAC(pl)=GLOBALMETMACP1A0(compgeom,loc,i,j,k).EOMFUNCMAC(pl);
	  GLOBALMETMACP0A1(gdetgeom,i,j,k,loc).IEOMFUNCNOSINGMAC(pl)=GLOBALMETMACP1A0(compgeom,loc,i,j,k).IEOMFUNCNOSINGMAC(pl);
	}
	//      exit(0);
	
#else
	// GODMARK: then unless want to create independent storage area, computed each time needed
#endif

	/////////////
	//
	// Fill global gdetgeom structure
	


	//      if(loc==FACE2){
	//	dualfprintf(fail_file,"i=%d V1=%21.15g V[2]=%21.15g gdet=%21.15g\n",i,V[1],V[2],localgdet[0]);
	//      }


	//      if(loc==FACE1){
	//dualfprintf(fail_file,"i=%d V1=%21.15g gdet=%21.15g\n",i,V[1],GLOBALMETMACP1A0(gdet,FACE1,i,j,k));
	//	DLOOP(jj,kk) dualfprintf(fail_file,"gcov[%d][%d]=%21.15g\n",jj,kk,GLOBALMETMACP1A2(gcov,FACE1,i,j,k,jj,kk));
	//	DLOOP(jj,kk) dualfprintf(fail_file,"gcon[%d][%d]=%21.15g\n",jj,kk,GLOBALMETMACP1A2(gcon,FACE1,i,j,k,jj,kk));
	//}

	// check if near static limit since can't divide by the below in ucon_calc
	// GODMARK
	if (fabs(localgcon[GIND(TT,TT)]) < SMALL) {
	  bl_coord_ijk(i,j,k,X,V);
	  dualfprintf(fail_file, "grid location too near g_{tt}==0: %d %d %d : r=%21.15g th=%21.15g phi=%21.15g : Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,localgcon[GIND(TT,TT)]);
	  myexit(1);
	}
	if (0 && fabs(localgcon[GIND(RR,RR)]) < SMALL) {
	  bl_coord_ijk(i,j,k,X,V);
	  dualfprintf(fail_file, "grid location too near g^{rr}==0:  %d %d %d : r=%21.15g th=%21.15g phi=%21.15g :  Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,localgcon[GIND(RR,RR)]);
	  myexit(1);
	}
	if (0 && fabs(localgcon[GIND(TH,TH)]) < SMALL) {
	  bl_coord_ijk(i,j,k,X,V);
	  dualfprintf(fail_file,"grid location too near g^{\\theta\\theta}==0:  %d %d %d : r=%21.15g th=%21.15g phi=%21.15g :  Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,localgcon[GIND(TH,TH)]);
	  myexit(1);
	}
	if (0 && fabs(localgcon[GIND(PH,PH)]) < SMALL) {
	  bl_coord_ijk(i,j,k,X,V);
	  dualfprintf(fail_file,"grid location too near g^{\\phi\\phi}==0:  %d %d %d : r=%21.15g th=%21.15g phi=%21.15g :  Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,localgcon[GIND(PH,PH)]);
	  myexit(1);
	}

	// what about g_{tt}==0? Do I ever divide by g_{tt}?
	// Yes, for ucon[TT] for 4 velocity, which is done if WHICHVEL==VEL4 or init.c
	// what about g^{rr}==0? Do I ever divide by g^{rr}?
	// Doesn't appear so
	// g^{pp} inf taken care of in metric.c by avoiding theta=0,Pi

      }// end over location
    }// end 3D LOOP
  }// end parallel region
}











///////////////////
//
// Grid functions that only exist at one location for all grid points
//
//////////////////
 // connection only needed at center, and only has memory on full normal grid (not like gcon/gcov that have extra upper edge)
static void set_connection(void)
{


  //  dualfprintf(fail_file,"Computing connection\n");


#pragma omp parallel 
  {
    int i, j, k;
    FTYPE X[NDIM];
    struct of_geom geomdontuse;
    struct of_geom *ptrgeom=&geomdontuse;
    int loc;


    //////////    COMPFULLLOOP
    OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUPFULL;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);


#if(MCOORD==CARTMINKMETRIC)
      // doesn't depend on position, only store/use 1 value
      if(i!=0 || j!=0 || k!=0) continue; // simple way to avoid other i,j,k when doing OpenMP
#endif

      loc=CENT;
      coord_ijk(i, j, k, loc, X);
      // in geom, only metric thing used is gcon to raise the lower connection.
      get_geometry(i, j, k, loc, ptrgeom);

      //    dualfprintf(fail_file,"conn: i=%d j=%d k=%d\n",i,j,k);
      conn_func(MCOORD,X, ptrgeom, GLOBALMETMAC(conn,i,j,k),GLOBALMETMAC(conn2,i,j,k));
    }// end 3D LOOP
  }// end parallel region

}





///////////////////
//
// Grid functions that only exist at one location AND only on active grid
//
//////////////////
static void set_idxvol(void)
{


#pragma omp parallel 
  {
    int i, j, k;

    //////////    COMPZLOOP
    OPENMP3DLOOPVARSDEFINE; OPENMP3DLOOPSETUPZLOOP;
#pragma omp for schedule(OPENMPSCHEDULE(),OPENMPCHUNKSIZE(blocksize))
    OPENMP3DLOOPBLOCK{
      OPENMP3DLOOPBLOCK2IJK(i,j,k);


#if(MCOORD==CARTMINKMETRIC)
      // doesn't depend on position, only store/use 1 value
      if(i!=0 || j!=0 || k!=0) continue; // simple way to avoid other i,j,k when doing OpenMP
#endif

      // only at 1 location, centered, using surrounding edge values
      if((defcoord==LOGRUNITH)&&(MCOORD==KSCOORDS)){
	mks_unitheta_idxvol_func(i,j,k,GLOBALMETMAC(idxvol,i,j,k));
      }
      else{
	 GLOBALMETMACP0A1(idxvol,i,j,k,TT)=1.0; // really 1/dt, but changes in time      
	 GLOBALMETMACP0A1(idxvol,i,j,k,RR)=1.0/dx[1];
	 GLOBALMETMACP0A1(idxvol,i,j,k,TH)=1.0/dx[2];
	 GLOBALMETMACP0A1(idxvol,i,j,k,PH)=1.0/dx[3];
      }
    }// end 3D LOOP
  }// end parallel region

}