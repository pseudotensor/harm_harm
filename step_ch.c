
//////////////////////////
//
// Code to take full step
//
//////////////////////////

#include "decs.h"


static void setup_rktimestep(int *numtimeorders,
			     FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR],
			     FTYPE (*pk)[NSTORE1][NSTORE2][NSTORE3][NPR],
			     FTYPE (*pii[4])[NSTORE2][NSTORE3][NPR],FTYPE (*pbb[4])[NSTORE2][NSTORE3][NPR],FTYPE (*pff[4])[NSTORE2][NSTORE3][NPR],
			     FTYPE (*uii[4])[NSTORE2][NSTORE3][NPR],FTYPE (*uff[4])[NSTORE2][NSTORE3][NPR],FTYPE (*ucum[4])[NSTORE2][NSTORE3][NPR],
			     FTYPE (*CUf)[4],FTYPE (*Cunew)[4]);




static int pre_stepch(int *dumpingnext, FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
static int post_stepch(int *dumpingnext, FTYPE fullndt, FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
static int step_ch(int *dumpingnext, FTYPE *fullndt,FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR]);
static int post_advance(int *dumpingnext, int timeorder, int numtimeorders, int finalstep, SFTYPE boundtime, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR],FTYPE (*pf)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR], FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR]);









// take full step (called from main.c)
int step_ch_full(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR])
{
  FTYPE fullndt;
  // dumpingnext[0] = dumping just after this step
  // dumpingnext[1] = dumping just after the step after this step
  int dumpingnext[2];



  // things to do before taking full step
  if(dt!=0.0) pre_stepch(dumpingnext,prim);

  // take full step
  step_ch(dumpingnext, &fullndt,prim,pstag,ucons,vpot,Bhat,pl_ct, pr_ct, F1, F2, F3,Atemp,uconstemp);

  // things to do after taking full step
  if(dt!=0.0) post_stepch(dumpingnext, fullndt, prim);


  if(dt!=0.0){ // don't do if just passing through -- otherwise would end up looping endlessly!
    // add up contributions to flux through horizon (really inner boundary)
    if(DOEVOLVEMETRIC && (EVOLVEMETRICSUBSTEP==0 || EVOLVEMETRICSUBSTEP==2) ){
      compute_new_metric_longsteps(prim,pstag,ucons,vpot,Bhat,pl_ct, pr_ct, F1, F2, F3,Atemp,uconstemp);
    }
    
    // below must come after longstep update so that don't change before metric step taken!
    control_metric_update();
    
    postdt(); // here one can alter variables and try to restart, or implement any post step operations


  }
  
  
  // must check before MPI operation (since asymmetries would desynchronize cpus)
  MYFUN(error_check(2),"main.c","error_check",1);
  //^^ otherwise ok
  // eventually all cpus come here, either in failure mode or not, and cleanly tell others if failed/exit/dump/etc.


  
  
  
  return(0);
}



// take step itself
int step_ch(int *dumpingnext, FTYPE *fullndt,FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR])
{
  int step_ch_simplempi(int *dumpingnext, FTYPE *fullndt,FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR]);
  int step_ch_supermpi(int *dumpingnext, FTYPE *fullndt,FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR]);



#if(SIMULBCCALC==-1)
  MYFUN(step_ch_simplempi(dumpingnext, fullndt,prim,pstag,ucons,vpot,Bhat,pl_ct, pr_ct,F1,F2,F3,Atemp,uconstemp),"step_ch.c:step_ch()", "step_ch_simplempi()", 1);
#else
  MYFUN(step_ch_supermpi(dumpingnext, fullndt,prim,pstag,ucons,vpot,Bhat,pl_ct, pr_ct,F1,F2,F3,Atemp,uconstemp),"step_ch.c:step_ch()", "step_ch_supermpi()", 1);
#endif


  /* done! */
  return (0);
}



// things to do before taking a full timestep
// assume not called when dt==0.0
int pre_stepch(int *dumpingnext, FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{
  


#if(PRODUCTION==0)
  trifprintf( "\n#step=%ld",realnstep);
#endif



  // if not doing diagnostics, then dumpingnext will be 0
  // first check if dumping next step
  // note dt is already set to correct value here
  dumpingnext[0]=(diag(FUTURE_OUT,t+dt,nstep+1,realnstep+1)==DOINGFUTUREOUT);
  dumpingnext[1]=(diag(FUTURE_OUT,t+dt+dt/2,nstep+2,realnstep+2)==DOINGFUTUREOUT); // estimate of t+dt+dt/2 good with 1<SAFEFACTOR<2



    
  if(dumpingnext[0] || dumpingnext[1]){
#if(CALCFARADAYANDCURRENTS)
    if((WHICHCURRENTCALC==CURRENTCALC0)||(WHICHCURRENTCALC==CURRENTCALC2)){
      // for this method, all J components are located back in time
      current_doprecalc(CURTYPEX,prim);
      current_doprecalc(CURTYPEY,prim);
      current_doprecalc(CURTYPEZ,prim);
      // get faraday in case used for time averaging or dumping it
      current_doprecalc(CURTYPEFARADAY,prim);
    }
#endif
  }


  return(0);


}




// things to do after taking a full timestep
// assume not called when dt==0.0
int post_stepch(int *dumpingnext, FTYPE fullndt, FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{




  if(dumpingnext[0]){
#if(CALCFARADAYANDCURRENTS)
    // compute final faradays
    if(WHICHCURRENTCALC==CURRENTCALC1){
      // compute faraday components needed for time centering of J
      current_doprecalc(CURTYPET,prim);
      // J is located at this time
      current_doprecalc(CURTYPEX,prim);
      current_doprecalc(CURTYPEY,prim);
      current_doprecalc(CURTYPEZ,prim);
      current_doprecalc(CURTYPEFARADAY,prim); // for diagnostics
    }
    // compute current
    current_calc(GLOBALPOINT(cfaraday));
#endif
  }


#if(ACCURATEDIAG==0)
  // compute flux diagnostics
  // this doesn't exactly make conservation work -- should have in middle step point using full step.  But if PARA, no middle point that's exact.
  // think about where to put this
  // GODMARK : use of globals
  diag_flux(prim,F1, F2, F3, dt); // should use REAL dt, not within a timeorderd RK integration step
  //    horizon_flux(F1,dt); // subsumed
#endif

  // general flux only done on full steps since no requirement for accuracy and code can be expensive computationally
  diag_flux_general(prim,dt);// Should be full dt, not substep dt.




#if(PRODUCTION==0)
  trifprintf( "[d]");
#endif



  //////////////////////////
  //
  // increment time
  //
  //////////////////////////
  t += dt;
  tstepparti = tsteppartf = t;
    
  realnstep++;
    
  ////////////////////////////
  //
  // set next timestep
  //
  ////////////////////////////
  // find global minimum value of ndt over all cpus
  mpifmin(&fullndt);
  // note that this assignment to fullndt doesn't get returned outside function
  if (fullndt > SAFE * dt)    fullndt = SAFE * dt;
  dt = fullndt;

  ///////////////////////////////////
  //
  // don't step beyond end of run
  //
  ////////////////////////////////////
  if(onemorestep){
    // check if previous step was onemorestep==1
    reallaststep=1;
    dt=SMALL;
  }
  else{
    if (t + dt > tf){
      dt = tf - t;
      onemorestep=1;
    }
    else if (t + dt == tf){
      reallaststep=1;
    }
    // make sure don't get pathological case of dt=0 on last step
    if(dt<SMALL){
      reallaststep=1;
      dt=SMALL;
    }
  }




  if(dt==0.0 && t>=tf){
    // somehow got here, then finish
    dualfprintf(fail_file,"SOMEHOW GOT HERE\n");
    reallaststep=1;
  }



  return(0);



}





// things to do during substep before advance()
int pre_advance(int timeorder, int numtimeorders, int finalstep, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR],FTYPE (*pf)[NSTORE2][NSTORE3][NPR])
{


  //////////////////
  //
  // prefixup
  //
  //////////////////
#if(EOMTYPE!=EOMFFDE)
  // force-free and cold GRMHD don't use pre_fixup, currently, even if could
  MYFUN(pre_fixup(STAGEM1, pi),"step_ch.c:step_ch_simple()", "pre_fixup()", 1);
#endif


  return(0);

}






// things to do after advance()
int post_advance(int *dumpingnext, int timeorder, int numtimeorders, int finalstep, SFTYPE boundtime, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR],FTYPE (*pf)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR], FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR])
{


  //////////////////////
  //
  // need to compute various things for EOS
  // use primitive located where EOS will be used (pb)
  //
  //////////////////////
  compute_EOS_parms(WHICHEOS,GLOBALPOINT(EOSextraglobal),pb);


  /////////////////////////////////////
  //
  // post-advance bound and fixup
  //
  /////////////////////////////////////

#if( (1 == CHECKSOLUTION || UTOPRIMADJUST == UTOPRIMAVG) )
  // if CHECKSOLUTION==1, then need values to be bounded right now, since use them to check whether even good solutions are really good.
  // post_fixup() will use previous time step pff boundary values to fixup_utoprim() if this is not called.
  // bound advanced values before post_fixup() so fixup_utoprim() has updated boundary values to base fixup on.



  ////////////
  //
  // bounding 1 layer so fixups have that layer available for fixing up inversion failures
  // not done if don't care about perfect MPI consistency with failures -- then fixups don't use values across MPI boundaries.  Will be less robust in general, but still consistent behavior across MPI boundaries.
  //
  ////////////

#if(PRODUCTION==0)
  trifprintf("[b1");
#endif

  MYFUN(bound_beforeevolveprim(STAGEM1,boundtime,pf,pstag,ucons,finalstep,UTOPRIMFIXMPICONSISTENT>0),"step_ch.c:step_ch_simplempi()", "bound_evolveprim()", 1);

#if(PRODUCTION==0)
  trifprintf("]");
#endif






#if(PRODUCTION==0)
  trifprintf("[x");
#endif


  /////////////
  // done when all timeorders are completed, so stencil used doesn't matter
  //
  // postfixup
  // post_fixup: If this modifies values and then uses the modified values for other modified values, then must bound_prim() after this
  // if one doesn't care about MPI being same as non-MPI, then can move bound_prim() above to below error_check() and then remove prc<-pv in fixup_utoprim()
  //
  /////////////
  MYFUN(post_fixup(STAGEM1,boundtime, pf,pb,ucons,finalstep),"step_ch.c:advance()", "post_fixup()", 1);

#if(PRODUCTION==0)
  trifprintf( "]");
#endif




#else

  ////////////
  //
  // just report problems, don't fix them
  // Then no need for boundary calls
  //
  ////////////
  MYFUN(post_fixup_nofixup(STAGEM1,boundtime, pf,pb,ucons,finalstep),"step_ch.c:advance()", "post_fixup_nofixup()", 1);

#endif


  /////////////////////////////////////
  //
  // Synch CPUs
  //
  /////////////////////////////////////

  // must check before MPI operation (since asymmetries would
  // desynchronize cpus
  MYFUN(error_check(1),"step_ch.c", "error_check", 1);

  // bound final values (comes after post_fixup() since changes made by post_fixup)
  //#if(MPIEQUALNONMPI)



#if(ASYMDIAGCHECK)
  dualfprintf(fail_file,"1before bound\n");
  asym_compute_2(pf);
  dualfprintf(fail_file,"2before bound\n");
#endif




#if(PRODUCTION==0)
  trifprintf("[rf");
#endif



  if( DOGRIDSECTIONING){
    // this must come before last bound() call so boundary conditions set consistently with new section so next step has all values needed in ghost cells
    // redo all such enerregions since may be time-dependent
    recompute_fluxpositions(0,timeorder, numtimeorders, nstep,t);
  }

#if(PRODUCTION==0)
  trifprintf("]");
#endif





#if(PRODUCTION==0)
  trifprintf( "[b2");
#endif


  /////////////////////////////////////
  //
  // normal bondary call
  // required in general
  //
  /////////////////////////////////////
  MYFUN(bound_evolveprim(STAGEM1,boundtime,pf,pstag,ucons,finalstep,USEMPI),"step_ch.c:step_ch_simplempi()", "bound_evolveprim()", 2);

  //#endif

#if(PRODUCTION==0)
  trifprintf( "]");
#endif


  ////////////////
  //
  // Use tracked A_i to update magnetic field
  // Required to keep A_i in synch with field and only is different at the machine error level (which grows over long times, so why this is required and hardless)
  //
  ////////////////
  if(EVOLVEWITHVPOT && TRACKVPOT){
    // if evolving with vpot, then assume good enough to use final full timestep A_i to obtain new point fields
    // less expensive than doing every substep and only wrong at machine error with factors extra for number of substeps
    // SUPERGODMARK: Check convergence rate and check errors!!  SUPERCHANGINGMARK
    // GODMARK: for now only do at the end of the full timestep to avoid being expensive -- doesn't really matter that didn't do it per substep since only machine differences
    if(finalstep) evolve_withvpot(pf, pstag, ucons, vpot, Bhat, F1, F2, F3, Atemp,uconstemp);
  }

  /////////////////////////////////////
  //
  // Compute divb for point-field method
  //
  /////////////////////////////////////
  if(dt!=0.0){
    // update Bhat so later can compute divb
    if(FLUXB==FLUXCTSTAG && extrazones4emf==0 && dofluxreconevolvepointfield==1 && DOENOFLUX == ENOFLUXRECON){
      //bound_uavg(STAGEM1,unew); // DEBUG
      // here utoinvert is point conserved field at staggered FACE position
      field_Bhat_fluxrecon(pf,ucons,Bhat);
    }
  }


  /////////////////////////////////////
  //
  // Compute current update
  //
  /////////////////////////////////////
  if(dt!=0.0){ // don't do if just passing through

    if(dumpingnext[0] || dumpingnext[1]){
#if(CALCFARADAYANDCURRENTS)

#if(PRODUCTION==0)
      trifprintf( "[cu");
#endif

      if((WHICHCURRENTCALC==CURRENTCALC0)||(WHICHCURRENTCALC==CURRENTCALC2)){
	// puts J at the time center, but hard to know if RK is at mid point in time except for midpoint method
	// compute current_doprecalc if near half-step in time
	if(
	   ((numtimeorders>=3)&&(timeorder==1))
	   ||((numtimeorders<=2)&&(timeorder==0))
	   )
	  current_doprecalc(CURTYPET,pf); // should be called using half-time step data
      }

#if(PRODUCTION==0)
      trifprintf( "]");
#endif

#endif
    }

  }// end if dt!=0.0


  /////////////////////////////////////
  //
  // Diagnostics
  //
  /////////////////////////////////////
  if(dt!=0.0){ // don't do if just passing through
    /* perform diagnostics */
    // no error check since assume if step_ch passed, diag(1) will pass
    if (DODIAGS && DODIAGEVERYSUBSTEP ){ //SASMARK -- moved the diags calls here
      GLOBALPOINT(pdump) = pf;
      diag(DUMP_OUT,t,nstep,realnstep);
#if(PRODUCTION==0)
      trifprintf( "D");
#endif
    }
  }

  return(0);
}






// take full step using normal MPI method
int step_ch_simplempi(int *dumpingnext, FTYPE *fullndt, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR])
{
  int advance(int stage, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR], FTYPE (*pf)[NSTORE2][NSTORE3][NPR],
	      FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],
	      FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
	      FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],
	      FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
	      FTYPE (*ui)[NSTORE2][NSTORE3][NPR],FTYPE (*uf)[NSTORE2][NSTORE3][NPR], FTYPE (*ucum)[NSTORE2][NSTORE3][NPR],
	      FTYPE *CUf,FTYPE *Cunew,SFTYPE fluxdt, SFTYPE boundtime, int timeorder, int numtimeorders, FTYPE *ndt);
  
  int pre_advance(int timeorder, int numtimeorders, int finalstep, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR],FTYPE (*pf)[NSTORE2][NSTORE3][NPR]);
  int asym_compute_2(FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
  int pl,pliter;
  //
  int boundstage;
  SFTYPE mydt;
  int stage, stagei,stagef;
  FTYPE ndt,lastndt;
  FTYPE (*pi)[NSTORE2][NSTORE3][NPR];
  FTYPE (*pb)[NSTORE2][NSTORE3][NPR];
  FTYPE (*pf)[NSTORE2][NSTORE3][NPR];
  FTYPE (*prevpf)[NSTORE2][NSTORE3][NPR];
  FTYPE (*pii[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*pbb[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*pff[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*uii[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*uff[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*ucum[4])[NSTORE2][NSTORE3][NPR];
  //  FTYPE alphaik[MAXSTAGES][MAXSTAGES],betaik[MAXSTAGES];
  FTYPE CUf[MAXTIMEORDER][4],Cunew[MAXTIMEORDER][4];
  int i, j, k;
  int numtimeorders;
  int timeorder;
  int finalstep;
  SFTYPE fluxdt[MAXTIMEORDER], boundtime[MAXTIMEORDER];





  ////////////////////////
  //
  // setup time-stepping
  //
  ////////////////////////
  setup_rktimestep(&numtimeorders,prim,pstag,ucons,vpot,Bhat,GLOBALPOINT(pk),pii,pbb,pff,uii,uff,ucum,CUf,Cunew);


  /////////////////////////////////////
  //
  // Obtain initial time of substep, final time of substep, and true dt used for flux conservation that is used to iterate ucum in advance.c
  //
  /////////////////////////////////////
  get_truetime_fluxdt(numtimeorders, dt, CUf, Cunew, fluxdt, boundtime, NULL,NULL);


  // global debug tracking var
  numstepparts=numtimeorders;
  // Initialize old and new dt to be very large so minimization procedure always obtains new dt
  ndt=lastndt=BIG;




  /////////////////////////////
  //
  // general temporal loop
  //
  /////////////////////////////
  for(timeorder=0;timeorder<numtimeorders;timeorder++){
    

    /////////////////////////////////////
    //
    //the starting and the ending times of the current substep
    //
    /////////////////////////////////////
    if(timeorder!=numtimeorders-1){
      tstepparti = t + CUf[timeorder][3] * dt;
      tsteppartf = t + CUf[timeorder][2] * dt +  CUf[timeorder][3] * dt;
    }
    else{
      tstepparti=t;
      tsteppartf=t+dt;
    }


    /////////////////////////////////////
    //
    // global debug tracking var
    //
    /////////////////////////////////////
    steppart=timeorder;
    if(timeorder==numtimeorders-1) finalstep=1; else finalstep=0;

#if(PRODUCTION==0)
    trifprintf("|%ds",timeorder);
#endif



    /////////////////////////////////////
    //
    // BEFORE ADVANCE
    //
    /////////////////////////////////////
    pre_advance(timeorder, numtimeorders, finalstep, pii[timeorder],pbb[timeorder],pff[timeorder]);
 


    /////////////////////////////////////
    //
    // ADVANCE
    //
    /////////////////////////////////////
    if(SPLITNPR){


      /////////////////////////////////////
      //
      // SPLITNPR ADVANCE
      //
      /////////////////////////////////////
    

#if(SPLITNPR&&(USESTOREDSPEEDSFORFLUX==0 || STOREWAVESPEEDS==0))
#error Not correct use of SPLITNPR and wavespeeds
#endif
      // Note that even though PLOOP is split-up, some things are still repeated for these 2 advance() calls:
      // 1) vchar for Riemann solver still needed -- could store vchar on first pass and use on second pass, but then using old B for vchar() on second pass and want to use new B everywhere for second pass
      // 2) dt is minimized twice -- seems second pass will overwrite first pass
      // 3) getting of geometry and state everywhere is duplicated
      //    NOTE OF IMPORTANCE: get_state needs all primitives, so must feed non-PLOOP quantities
      // 4) get_wavespeeds requires ALL quantities during interpolation -- must set USESTOREDSPEEDSFORFLUX==1 and STOREWAVESPEEDS>0 so computes wave speeds across 2 advance() calls and uses centered speed for interface to avoid needing during interpolation for p_l,p_r->wavespeed@interface
      //    Basically flux_compute_general(p_l,p_r needs velocities too) for first pass
      // 5) p2SFUevolve's get_state needs velocity -- that is, flux_B(B,v) so have to still interpolate v with B -- save v-interpolation (all interpolations) and avoid interpolations on second pass.
      //    Made pleft and pright [3]X in size to store across advance(), dq is already this way
      // 6) checked NPR and NPR2INTERP uses.  All such explicit uses are ok
      //    e.g. grep -e "[^\[]NPR" *.c

      // Things NOT done on first pass:
      // 1) source term if NOGDETBi=0
      // 2) metric update if evolving metric
      // 3) all non-B quantities if doing PLOOP or PLOOPINTERP
      // 4) diag_flux, diag_source, diag_source_all -- all use PDIAGLOOP
      // 5) fixup1zone() or fixup()

      // Things to fix:
      // 3) compute_df_line_new during interpolation needs get_state for u and b -- only for pressure indicator for WENO
      // 4) calculation of bsq requires b^\mu requires B and v/u/urel -- called PLOOP or what?
      // 5) Check all PLOOP's everywhere to ensure doing ALL NPR if required
      // 6) could bound only field when bounding between



      // choice for range of PLOOP
      // choose to only update magnetic field
      nprstart=0;
      nprend=2;
      nprlist[0]=B1;
      nprlist[1]=B2;
      nprlist[2]=B3;
      // choice for range of PLOOPINTERP
      // but interpolate everything on first pass (GODMARK: in reality only need to do field on timeorder=0 since last field update is next field pbb??? -- probably only true if doing simple RK methods (1,2))
#pragma omp parallel private(pl)
      {
	npr2interpstart=0;
	npr2interpend=NPR2INTERP-1;
	for(pl=npr2interpstart;pl<=npr2interpend;pl++)  npr2interplist[pl]=pl;
	// choice for range of PLOOPNOTINTERP
	npr2notinterpstart=0;
	npr2notinterpend=-1;
	npr2notinterplist[0]=0;
      }
      advancepassnumber=0;


      // advance (field only)
      // Only field parts of pff, uff, and ucum updated
      // on final timeorder, ucum used to get final B that will be used as B for final timeorder for non-field quantities
      MYFUN(advance(STAGEM1, pii[timeorder], pbb[timeorder], pff[timeorder], pstag, pl_ct, pr_ct, F1, F2, F3, vpot, uii[timeorder], uff[timeorder], ucum[timeorder], CUf[timeorder], Cunew[timeorder], fluxdt[timeorder], boundtime[timeorder], timeorder,numtimeorders,&ndt),"step_ch.c:step_ch_simplempi()", "advance()", 1);

      // only need to bound field, so control PLOOPMPI
      nprboundstart=0;
      nprboundend=2;
      nprboundlist[0]=B1;
      nprboundlist[1]=B2;
      nprboundlist[2]=B3;
      // with magnetic field, only  need to bound
      MYFUN(bound_evolveprim(STAGEM1,boundtime[timeorder],pff[timeorder],pstag,uff[timeorder],finalstep,USEMPI),"step_ch.c:step_ch_simplempi()", "bound_evolveprim()", 1);



      // now copy over pff to pbb so Bnew is used in flux calculation of non-field quantities
      COMPFULLLOOP{
	PLOOPBONLY(pl) MACP1A1(pbb,timeorder,i,j,k,pl)=MACP1A1(pff,timeorder,i,j,k,pl);
      }


      // choice for range of PLOOP
      // don't update field on second pass (result overwritten in utoprimgen.c even if done)
      nprstart=0;
      nprend=NPR-1-3; // no field
      for(pl=nprstart;pl<=nprend;pl++){
	if(pl>=B1) nprlist[pl]=pl+3; // skip field
	else nprlist[pl]=pl;
      }
      // choice for range of PLOOPINTERP
      // Interpolation of only fields on second pass since want to use updated field to compute flux and so at end of both steps first updated field and non-field quantites are fed to inversion for consistent P(Bnew) and Bnew is used
#pragma omp parallel private(pl)
      {
	npr2interpstart=0;
	npr2interpend=2;
	npr2interplist[0]=B1;
	npr2interplist[1]=B2;
	npr2interplist[2]=B3;
	// choice for range of PLOOPNOTINTERP
	// what not interpolating (all non-field):
	npr2notinterpstart=0;
	npr2notinterpend=NPR-1-3; // no field
	for(pl=npr2notinterpstart;pl<=npr2notinterpend;pl++){
	  if(pl>=B1) npr2notinterplist[pl]=pl+3; // skip field
	  else npr2notinterplist[pl]=pl;
	}
      }
      advancepassnumber=1;


      // advance (non-field quantities)
      // only non-field parts of pff, uff, ucum updated
      MYFUN(advance(STAGEM1, pii[timeorder], pbb[timeorder], pff[timeorder], pstag, pl_ct, pr_ct, F1, F2, F3, vpot, uii[timeorder], uff[timeorder], ucum[timeorder], CUf[timeorder], Cunew[timeorder], fluxdt[timeorder], boundtime[timeorder], timeorder,numtimeorders,&ndt),"step_ch.c:step_ch_simplempi()", "advance()", 1);



      //////////////////////////
      // go back to defaults

      // choice for range of PLOOP
      nprstart=0;
      nprend=NPR-1;
      for(pl=nprstart;pl<=nprend;pl++) nprlist[pl]=pl;
#pragma omp parallel private(pl)
      {
	// choice for range of PLOOPINTERP
	npr2interpstart=0;
	npr2interpend=NPR2INTERP-1;
	for(pl=npr2interpstart;pl<=npr2interpend;pl++) npr2interplist[pl]=pl;
	// choice for range of PLOOPNOTINTERP
	npr2notinterpstart=0;
	npr2notinterpend=-1;
	npr2notinterplist[0]=0;
      }
      advancepassnumber=-1;

      // default choice for range of PLOOPMPI
      nprboundstart=0;
      nprboundend=NPRBOUND-1;
      for(pl=nprboundstart;pl<=nprboundend;pl++) nprboundlist[pl]=pl;




    }
    else{


      /////////////////////////////////////
      //
      // NORMAL (not SPLITNPR) ADVANCE
      //
      /////////////////////////////////////

      advancepassnumber=-1; // implies do all things, no split
      // advance (all)
      MYFUN(advance(STAGEM1, pii[timeorder], pbb[timeorder], pff[timeorder], pstag, pl_ct, pr_ct, F1, F2, F3, vpot, uii[timeorder], uff[timeorder], ucum[timeorder], CUf[timeorder], Cunew[timeorder], fluxdt[timeorder], boundtime[timeorder], timeorder,numtimeorders,&ndt),"step_ch.c:step_ch_simplempi()", "advance()", 1);



    } // end else if normal advance()



    ///////////////
    //    
    // need to minimize dt over all substeps rather than just last step
    // lastndt holds cumulative-all-substep ndt
    //
    ///////////////
    if(ndt<lastndt) lastndt=ndt;



    /////////////////////////////////////
    //
    // POST ADVANCE
    //
    /////////////////////////////////////
    post_advance(dumpingnext, timeorder, numtimeorders, finalstep, boundtime[timeorder], pii[timeorder],pbb[timeorder],pff[timeorder],pstag,ucons,vpot,Bhat, F1, F2, F3, Atemp, uconstemp);



  }// end timeorder


  
  /////////////////////////
  //
  // pass back the final minimal dt over all substeps
  //
  /////////////////////////
  *fullndt = lastndt;



  /* done! */
  return (0);
}



// Obtain initial time of substep, final time of substep, and true dt used for flux conservation that is used to iterate ucum in advance.c
void get_truetime_fluxdt(int numtimeorders, SFTYPE localdt, FTYPE (*CUf)[4], FTYPE (*Cunew)[4], SFTYPE *fluxdt, SFTYPE *boundtime, SFTYPE *tstepparti, SFTYPE *tsteppartf)
{
  int timeorder;
  SFTYPE ufdt[MAXTIMEORDER],ucumdt[MAXTIMEORDER];
  SFTYPE oldufdt,olducumdt;
  FTYPE Ui, dUriemann, dUgeom;


  //  NOT YET:
  /////////////////////////////////////
  //
  //the starting and the ending times of the current substep
  //
  /////////////////////////////////////
  //  if(timeorder!=numtimeorders-1){
  //    *tstepparti = t + CUf[timeorder][3] * localdt;
  //    *tsteppartf = t + CUf[timeorder][2] * localdt +  CUf[timeorder][3] * localdt;
  //  }
  //  else{
  //    *tstepparti=t;
  //    *tsteppartf=t+localdt;
  //  }




  //////////////////////
  //
  // Get fluxdt
  //
  //////////////////////
  fluxdt[0] = 0.0; // initialize
  boundtime[0] = 0.0; // initialize
  Ui=dUgeom=0.0; // don't care about update from non-flux terms
  dUriemann=1.0; // indicates want dt applied on flux update

  // loop up to current substep to get current fluxdt
  for(timeorder=0;timeorder<numtimeorders;timeorder++){
    if(timeorder==0){
      oldufdt=0.0;
      olducumdt=0.0;
    }
    else{
      oldufdt=ufdt[timeorder-1];
      olducumdt=ucumdt[timeorder-1];
    }

    // follows dUtoU() in advance.c
    ufdt[timeorder] = UFSET(CUf[timeorder],localdt,Ui,oldufdt,dUriemann,dUgeom);
    // below is NOT += since just want current change, not all prior changes
    // if did +=, then get 1 for timeorder==numtimeorders-1 as required!
    ucumdt[timeorder] = UCUMUPDATE(Cunew[timeorder],localdt,Ui,ufdt[timeorder],dUriemann,dUgeom);

    // assuming fluxdt used before any calls in advance.c
    // then represents *amount* of flux'ed stuff in time dt
    // This is NOT the time of the flux evaluation
    // This is to be used with diag_flux and other things multiplied by dt for diagnostics only
    // Other things (e.g. sources()) use same dt as dUriemann automatically, so fluxdt is not for anything but diagnostics
    // Only exception is updating vpot that is out of place and uses ucum-type updates
    fluxdt[timeorder] = ucumdt[timeorder];

    //  time of pf at end of substep
    boundtime[timeorder] = t + ufdt[timeorder];
  }


#if(0)
  //////////////////////
  //
  // Get boundtime
  //
  // Get current time for pf
  // The below works because pi=p always and so always just use 
  // assuming bound called after advance(), then need to get time of next flux evaluation
  // This corresponds to time where pb *will be* located
  // 
  //
  //////////////////////
  for(timeorder=0;timeorder<numtimeorders;timeorder++){
    if(timeorder<numtimeorders-1){
      boundtime[timeorder] = t + localdt*CUf[timeorder][3]+ localdt*CUf[timeorder][2];
    }
    else{
      // if timeorder==numtimeorders-1, then final step and bound should be set for time of flux of next step, which is full t+dt
      boundtime[timeorder] = t + localdt;
    }
  }
#endif


#if(0)
  // DEBUG:
  dualfprintf(fail_file,"FLUXDT/BOUNDTIME: nstep=%ld ",nstep);
  for(timeorder=0;timeorder<numtimeorders;timeorder++){
    dualfprintf(fail_file,"to=%d fluxdt=%21.15g fluxdtperdt=%21.15g boundtime=%21.15g\n",timeorder,fluxdt[timeorder],fluxdt[timeorder]/dt,boundtime[timeorder]);
  }
  dualfprintf(fail_file,"\n");
#endif


}










int step_ch_supermpi(int *dumpingnext, FTYPE *fullndt, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR])
{
  int advance(int stage, FTYPE (*pi)[NSTORE2][NSTORE3][NPR],FTYPE (*pb)[NSTORE2][NSTORE3][NPR], FTYPE (*pf)[NSTORE2][NSTORE3][NPR],
	      FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],
	      FTYPE (*pl_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP], FTYPE (*pr_ct)[NSTORE1][NSTORE2][NSTORE3][NPR2INTERP],
	      FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR],
	      FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],
	      FTYPE (*ui)[NSTORE2][NSTORE3][NPR],FTYPE (*uf)[NSTORE2][NSTORE3][NPR], FTYPE (*ucum)[NSTORE2][NSTORE3][NPR],
	      FTYPE *CUf,FTYPE *Cunew,SFTYPE fluxdt, SFTYPE boundtime, int timeorder, int numtimeorders, FTYPE *ndt);
  int boundstage;
  SFTYPE mydt;
  int stage, stagei,stagef;
  int timeorder;
  FTYPE ndt,lastndt;
  FTYPE (*pi)[NSTORE2][NSTORE3][NPR];
  FTYPE (*pb)[NSTORE2][NSTORE3][NPR];
  FTYPE (*pf)[NSTORE2][NSTORE3][NPR];
  FTYPE (*prevpf)[NSTORE2][NSTORE3][NPR];
  FTYPE (*pii[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*pbb[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*pff[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*uii[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*uff[4])[NSTORE2][NSTORE3][NPR];
  FTYPE (*ucum[4])[NSTORE2][NSTORE3][NPR];
  //  FTYPE alphaik[MAXSTAGES][MAXSTAGES],betaik[MAXSTAGES];
  FTYPE CUf[MAXTIMEORDER][4],Cunew[MAXTIMEORDER][4];
  int i, j, k, pl, pliter;
  int numtimeorders;
  int finalstep;
  //  extern int horizon_flux(FTYPE (*F1)[NSTORE2][NSTORE3][NPR], SFTYPE Dt);
  SFTYPE fluxdt[MAXTIMEORDER], boundtime[MAXTIMEORDER];





  ////////////////////////
  //
  // setup time-stepping
  //
  ////////////////////////
  setup_rktimestep(&numtimeorders,prim,pstag,ucons,vpot,Bhat,GLOBALPOINT(pk),pii,pbb,pff,uii,uff,ucum,CUf,Cunew);


  /////////////////////////////////////
  //
  // Obtain initial time of substep, final time of substep, and true dt used for flux conservation that is used to iterate ucum in advance.c
  //
  /////////////////////////////////////
  get_truetime_fluxdt(numtimeorders, dt, CUf, Cunew, fluxdt, boundtime, NULL, NULL);



  // SPECIAL BOUNDARY/COMPUTATION MPI METHOD (out of date, and doesn't yet work right even if essentially complete code)
  /* check timestep */
  //  if (dt < MINDT) {
  //  trifprintf( "timestep too small\n");
  //  myexit(11);
  // }
  
  lastndt=1E30; // initialize lastndt
  for(timeorder=1;timeorder<=numtimeorders;timeorder++){


    /////////////////////////////////////
    //
    //the starting and the ending times of the current substep
    //
    /////////////////////////////////////
    if(timeorder!=numtimeorders-1){
      tstepparti = t + CUf[timeorder][3] * dt;
      tsteppartf = t + CUf[timeorder][2] * dt +  CUf[timeorder][3] * dt;
    }
    else{
      tstepparti=t;
      tsteppartf=t+dt;
    }

    if(timeorder==numtimeorders-1) finalstep=1; else finalstep=0;



#if(PRODUCTION==0)
    trifprintf("-to%d/%d-",timeorder,numtimeorders);
#endif
    if(numtimeorders==2){
      // note that pb (used for flux calc which has a stencil
      // calculation) must be different from pf so new stencils in
      // different stages won't affect stencil calculations -- must
      // use old values, not new from most previous temporary stage
      //
      // pi however can be the same as pf since each pi is replaced 1
      // zone at a time with a 0 stencil.
      if(timeorder==1){
	pi=prim;
	pb=prim;
	pf=GLOBALPOINT(pk)[0]; // different already, so good for simulbccalc
	prevpf=prim; // previous final true array
	mydt=0.5*dt;
      }
      else if(timeorder==2){
	pi=prim;
	pb=GLOBALPOINT(pk)[0];
	pf=prim;
	prevpf=GLOBALPOINT(pk)[0];
	mydt=dt;
      }
    }
    else if(numtimeorders==1){
      pi=prim;
      pb=prim;
      if(SIMULBCCALC<=0) pf=prim; else pf=GLOBALPOINT(pk)[0]; // need to be different if doing simulbccalc
      prevpf=prim;
      mydt=dt;
    }
    if(SIMULBCCALC<=0){ stagei=STAGEM1; stagef=STAGEM1; }
    else if(SIMULBCCALC==1) { stagei=STAGE0; stagef=STAGE2;}
    else if(SIMULBCCALC==2) { stagei=STAGE0; stagef=STAGE5;}

    
    // initialize bound stage
    if(SIMULBCCALC>=1) boundstage=STAGE0;
    else boundstage=STAGEM1;
    for(stage=stagei;stage<=stagef;stage++){
#if(PRODUCTION==0)
      if(SIMULBCCALC>=1) trifprintf("!s%d!",stage);
#endif
      // setup stage loop
#if(SIMULBCCALC==2)
#if(TYPE2==1)
      // GODMARK: isf1, etc. are NOT defined?!
	STAGECONDITION(0,N1-1,0,N2-1,isc,iec,jsc,jec);
	STAGECONDITION(0,N1,-1,N2,isf1,ief1,jsf1,jef1);
	STAGECONDITION(-1,N1,0,N2,isf2,ief2,jsf2,jef2);
	STAGECONDITION(0,N1,0,N2,ise,iee,jse,jee);
	STAGECONDITION(0,N1,0,N2-1,isf1ct,ief1ct,jsf1ct,jef1ct);
	STAGECONDITION(0,N1-1,0,N2,isf2ct,ief2ct,jsf2ct,jef2ct);
	STAGECONDITION(-1,N1,-1,N2,isdq,iedq,jsdq,jedq);
	STAGECONDITION(-2,N1+1,-2,N2+1,ispdq,iepdq,jspdq,jepdq);
	// GODMARK : probably not right for general boundary condition size
#endif
#endif

      // only bounding if safe zones, unsafe needs bz complete
      if(stage<STAGE2){
	bound_beforeevolveprim(boundstage, boundtime[timeorder], prevpf,pstag,ucons,finalstep,USEMPI); // pstag,ucons not right for supermpi
	if(stage!=STAGEM1) boundstage++;
      }

      // done here instead of local since pseudo-complicated calculation that might slow the dq calculation if done locally per zone
      MYFUN(pre_fixup(stage, prevpf),"step_ch.c:advance()", "pre_fixup()", 1);

      // go from previous solution to new solution
      partialstep=timeorder;      
      // not right for numtimeorders==4 // GODMARK
      // advance
      MYFUN(advance(-1, pii[timeorder], pbb[timeorder], pff[timeorder], pstag, pl_ct, pr_ct, F1, F2, F3, vpot, uii[timeorder], uff[timeorder], ucum[timeorder],CUf[timeorder], Cunew[timeorder], fluxdt[timeorder], boundtime[timeorder], timeorder,numtimeorders,&ndt),"step_ch.c:step_ch_supermpi()", "advance()", 1);
      // must check before MPI operation (since asymmetries would desynchronize cpus)
      if(stage<STAGE2){
	MYFUN(error_check(1),"step_ch.c", "error_check", 1);
      }
      if(stage!=STAGEM1){
	if(stage<STAGE2){
	  bound_evolveprim(boundstage, boundtime[timeorder], prevpf,pstag,ucons,finalstep,USEMPI);
	  boundstage++;
	}
      }
      if(timeorder==numtimeorders){
	if(ndt>lastndt) ndt=lastndt; // don't change if last was lower
	else lastndt=ndt; // new is lower, keep it
      }
    }
    if(timeorder==numtimeorders){// only do on full step
      // find global minimum value of ndt over all cpus
      mpifmin(&ndt);
    }
    // done when all stages are completed, so stencil used doesn't matter
    MYFUN(post_fixup(STAGEM1,boundtime[timeorder], pf,pb,ucons,1),"step_ch.c:advance()", "post_fixup()", 1);
  }


  // pass back the final minimal dt over all substeps
  *fullndt = lastndt;


  // copy the contents to the final working array
  if((numtimeorders==1)&&(SIMULBCCALC>=1)) COMPFULLLOOP PLOOP(pliter,pl) MACP0A1(prim,i,j,k,pl)=MACP0A1(pf,i,j,k,pl);
  

  /* done! */
  return (0);
}









// for the ith stage:

// Uf^i = ulast^i = CUf^{i0} Ui^i + CUf^{i1} ulast^i + CUf^{i2} dU^i

// unew^i = Cunew^{i0} Ui^i + Cunew^{i1} dU^i + Cunew^{i2} Uf^i

// (how also used) CUf[timeorder][2] : t + dt*CUf[timeorder][3]+ dt*CUf[timeorder][2] = time of pf

// CUf[timeorder][3] : t + dt*CUf[timeorder][3] = time of pi

// Cunew[timeorder][3] : t + dt*Cunew[timeorder][3] = approximate time of pb

// GODMARK: Currently RK methods all feed pf into pb on next step, so only need one pstag.  In general should have multiple pstag's.

void setup_rktimestep(int *numtimeorders,
		      FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR],
		      FTYPE (*pk)[NSTORE1][NSTORE2][NSTORE3][NPR],
		      FTYPE (*pii[4])[NSTORE2][NSTORE3][NPR],FTYPE (*pbb[4])[NSTORE2][NSTORE3][NPR],FTYPE (*pff[4])[NSTORE2][NSTORE3][NPR],
		      FTYPE (*uii[4])[NSTORE2][NSTORE3][NPR],FTYPE (*uff[4])[NSTORE2][NSTORE3][NPR],FTYPE (*ucum[4])[NSTORE2][NSTORE3][NPR],
		      FTYPE (*CUf)[4],FTYPE (*Cunew)[4])
{




  // to avoid special copying of final pff->p, always use p as final pff
  if(TIMEORDER==4 && dt!=0.0){
    // RK4 stepping
    *numtimeorders=4;

    // Ui ulast dU(pb)
    CUf[0][0]=1.0;  CUf[0][1]=0.0;      CUf[0][2]=0.5;  CUf[0][3] = 0.0;
    CUf[1][0]=1.0;  CUf[1][1]=0.0;      CUf[1][2]=0.5;  CUf[1][3] = 0.0;
    CUf[2][0]=1.0;  CUf[2][1]=0.0;      CUf[2][2]=1.0;  CUf[2][3] = 0.0;
    CUf[3][0]=1.0;  CUf[3][1]=0.0;      CUf[3][2]=1.0;  CUf[3][3] = 0.0;

    // Ui dU(Ub) Uf
    Cunew[0][0]=1.0;  Cunew[0][1]=1.0/6.0;      Cunew[0][2]=0.0;  Cunew[0][3] = 0.0;
    Cunew[1][0]=0.0;  Cunew[1][1]=1.0/3.0;      Cunew[1][2]=0.0;  Cunew[1][3] = 0.5;
    Cunew[2][0]=0.0;  Cunew[2][1]=1.0/3.0;      Cunew[2][2]=0.0;  Cunew[2][3] = 0.5;
    Cunew[3][0]=0.0;  Cunew[3][1]=1.0/6.0;      Cunew[3][2]=0.0;  Cunew[3][3] = 1.0;

    //primitive values used for initial state, fluxes, final state (where you output)
    pii[0]=p;    pbb[0]=p;       pff[0]=pk[0]; // produces U1
    pii[1]=p;    pbb[1]=pk[0];   pff[1]=pk[1]; // produces U2
    pii[2]=p;    pbb[2]=pk[1];   pff[2]=pk[0]; // produces U3
    pii[3]=p;    pbb[3]=pk[0];   pff[3]=p; // produces U4 (only dU part used)
    
    // GODMARK: use of globals: just scratch anyways
    uii[0]=GLOBALPOINT(uinitialglobal);  uff[0]=GLOBALPOINT(ulastglobal); ucum[0]=ucons;
    uii[1]=GLOBALPOINT(uinitialglobal);  uff[1]=GLOBALPOINT(ulastglobal); ucum[1]=ucons;
    uii[2]=GLOBALPOINT(uinitialglobal);  uff[2]=GLOBALPOINT(ulastglobal); ucum[2]=ucons;
    uii[3]=GLOBALPOINT(uinitialglobal);  uff[3]=GLOBALPOINT(ulastglobal); ucum[3]=ucons;

    // GODMARK: note that pbstag (staggered field from conserved de-averaging and inversion to primitive no geometry version) is always same memory space and comes from operating on final inverted quantity (ulastglobal or ucons), so just use same quantity for now and avoid adding extra code for pbstag[]
    //    pbstag[0]=pstagglobal;
    //    pbstag[1]=pstagglobal;
    //    pbstag[2]=pstagglobal;
    //    pbstag[3]=pstagglobal;

  }
  else if(TIMEORDER==3  && dt!=0.0){
    // TVD optimal RK3 method as in Shu's report
    *numtimeorders=3;
    
    // Ui ulastglobal dU(pb)
    CUf[0][0]=1.0;      CUf[0][1]=0.0;      CUf[0][2]=1.0;      CUf[0][3] = 0.0;
    CUf[1][0]=3.0/4.0;  CUf[1][1]=1.0/4.0;  CUf[1][2]=1.0/4.0;  CUf[1][3] = 0.0;
    CUf[2][0]=1.0/3.0;  CUf[2][1]=2.0/3.0;  CUf[2][2]=2.0/3.0;  CUf[2][3] = 0.0;
    
    // Ui dU(Ub) Uf
    // ucons=U3
    Cunew[0][0]=0.0;   Cunew[0][1]=0.0;      Cunew[0][2]=0.0;  Cunew[0][3] = 0.0;
    Cunew[1][0]=0.0;   Cunew[1][1]=0.0;      Cunew[1][2]=0.0;  Cunew[1][3] = 1.0;
    Cunew[2][0]=0.0;   Cunew[2][1]=0.0;      Cunew[2][2]=1.0;  Cunew[2][3] = 1.0/4.0;
    
    //always starting the substeps from the initial time
    pii[0]=p;      pbb[0]=p;       pff[0]=pk[0]; // produces U1
    pii[1]=p;      pbb[1]=pk[0];   pff[1]=pk[1]; // produces U2
    pii[2]=p;      pbb[2]=pk[1];   pff[2]=p; // produces U3

    // GODMARK: use of globals: just scratch anyways
    uii[0]=GLOBALPOINT(uinitialglobal);  uff[0]=GLOBALPOINT(ulastglobal); ucum[0]=ucons;
    uii[1]=GLOBALPOINT(uinitialglobal);  uff[1]=GLOBALPOINT(ulastglobal); ucum[1]=ucons;
    uii[2]=GLOBALPOINT(uinitialglobal);  uff[2]=GLOBALPOINT(ulastglobal); ucum[2]=ucons;

  }
  else if(TIMEORDER==2  && dt!=0.0){
#if(1)
    // midpoint method

    *numtimeorders=2;

    // old ucons not used for this method (i.e. [?][1]=0)
    CUf[0][0]=1.0; CUf[0][1]=0.0; CUf[0][2]=0.5; CUf[0][3] = 0.0;
    CUf[1][0]=1.0; CUf[1][1]=0.0; CUf[1][2]=1.0; CUf[1][3] = 0.0;

    // Ui dU(Ub) Uf
    // ucons=U2
    Cunew[0][0]=0.0;   Cunew[0][1]=0.0;      Cunew[0][2]=0.0;  Cunew[0][3] = 0.0;
    Cunew[1][0]=0.0;   Cunew[1][1]=0.0;      Cunew[1][2]=1.0;  Cunew[1][3] = 0.5;

    pii[0]=p;    pbb[0]=p;       pff[0]=pk[0];
    pii[1]=p;    pbb[1]=pk[0];   pff[1]=p;

    // GODMARK: use of globals: just scratch anyways
    uii[0]=GLOBALPOINT(uinitialglobal);  uff[0]=GLOBALPOINT(ulastglobal); ucum[0]=ucons;
    uii[1]=GLOBALPOINT(uinitialglobal);  uff[1]=GLOBALPOINT(ulastglobal); ucum[1]=ucons;

#else
    *numtimeorders=2;
    // TVD RK2 (Chi-Wang Shu 1997 - eq 4.10)
    // actually less robust than generic midpoint method above

    CUf[0][0]=1.0; CUf[0][1]=0.0; CUf[0][2]=1.0; CUf[0][3] = 0.0;
    CUf[1][0]=0.5; CUf[1][1]=0.5; CUf[1][2]=0.5; CUf[1][3] = 0.0;

    // Ui dU(Ub) Uf
    // ucons=U2
    Cunew[0][0]=0.0;   Cunew[0][1]=0.0;      Cunew[0][2]=0.0;   Cunew[0][3] = 0.0;
    Cunew[1][0]=0.0;   Cunew[1][1]=0.0;      Cunew[1][2]=1.0;   Cunew[1][3] = 1.0;

    pii[0]=p;    pbb[0]=p;       pff[0]=pk[0];
    pii[1]=p;    pbb[1]=pk[0];   pff[1]=p;

    // GODMARK: use of globals: just scratch anyways
    uii[0]=GLOBALPOINT(uinitialglobal);  uff[0]=GLOBALPOINT(ulastglobal); ucum[0]=ucons;
    uii[1]=GLOBALPOINT(uinitialglobal);  uff[1]=GLOBALPOINT(ulastglobal); ucum[1]=ucons;

#endif
  }
  else if(TIMEORDER==1 ||  dt==0.0){ // dt==0.0 case is case when just passing through
    // Euler method
    *numtimeorders=1;

    CUf[0][0]=1.0; CUf[0][1]=0.0; CUf[0][2]=1.0; CUf[0][3] = 0.0;

    // Ui dU(Ub) Uf
    // ucons=U1
    Cunew[0][0]=0.0;   Cunew[0][1]=0.0;      Cunew[0][2]=1.0;   Cunew[1][3] = 0.0;

    pii[0]=p;    pbb[0]=p;    pff[0]=p;

    // GODMARK: use of globals: just scratch anyways
    uii[0]=GLOBALPOINT(uinitialglobal);  uff[0]=GLOBALPOINT(ulastglobal); ucum[0]=ucons;

  }

}




// normal full bounding during evolution
int bound_evolveprim(int boundstage, SFTYPE boundtime, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  bound_allprim(boundstage, boundtime, prim,pstag,ucons,finalstep,doboundmpi);

  return(0);

}


// simple bounding during evolution
int bound_beforeevolveprim(int boundstage, SFTYPE boundtime, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR],int finalstep, int doboundmpi)
{
  int boundvartype=BOUNDPRIMSIMPLETYPE;

  bound_anyallprim(boundstage, boundtime, boundvartype, prim,pstag,ucons,finalstep,doboundmpi);

  return(0);

}

// normal bounding of non-staggered primitives
int bound_prim(int boundstage, SFTYPE boundtime, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  int boundvartype=BOUNDPRIMTYPE;

  bound_anyprim(boundstage, boundtime, boundvartype, prim, pstag, ucons, finalstep,doboundmpi);

  return(0);

}

// normal bounding of staggered primitives
int bound_pstag(int boundstage, SFTYPE boundtime, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  int boundvartype=BOUNDPSTAGTYPE;

  bound_anypstag(boundstage, boundtime, boundvartype, prim, pstag, ucons, finalstep,doboundmpi);

  return(0);

}


// normal bounding of all primitives
int bound_allprim(int boundstage, SFTYPE boundtime, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  int boundvartype=BOUNDPRIMTYPE;

  bound_anyallprim(boundstage, boundtime, boundvartype, prim, pstag,ucons, finalstep,doboundmpi);

  return(0);

}




// bound all prims
int bound_anyallprim(int boundstage, SFTYPE boundtime, int boundvartype, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR],int finalstep, int doboundmpi)
{
  int mystagboundvar;

  if(FLUXB==FLUXCTSTAG){
    bound_anyprim(boundstage, boundtime, boundvartype, prim, pstag, ucons, finalstep,doboundmpi);
    if(boundvartype==BOUNDPRIMTYPE) mystagboundvar=BOUNDPSTAGTYPE;
    else if(boundvartype==BOUNDPRIMSIMPLETYPE) mystagboundvar=BOUNDPSTAGSIMPLETYPE;
    else  mystagboundvar=boundvartype;
    bound_anypstag(boundstage, boundtime, mystagboundvar, prim, pstag, ucons, finalstep,doboundmpi);
  }
  else{
    bound_anyprim(boundstage, boundtime, boundvartype, prim, pstag, ucons, finalstep,doboundmpi);
  }


  if(unewisavg && BOUNDUNEW){
    // SUPERGODMARK:
    // if not outflow boundary, then can bound u as p
    // desirable since machine errors can be different and lead to secular changes
    // esp. in MPI
    bound_uavg(boundstage, boundtime, boundvartype, ucons,pstag, ucons, finalstep,doboundmpi);
  }

  return(0);

}



// bound uavg
int bound_uavg(int boundstage, SFTYPE boundtime, int boundvartype, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  int mystagboundvar;
  FTYPE (*uavg)[NSTORE2][NSTORE3][NPR];


  // assign uavg
  uavg=ucons;


  // for now only worry about bounding staggered fields since that's what can do given "general" boundary condition code
  if(DOENOFLUX != NOENOFLUX){

    // CHANGINGMARK: DEBUG:
    // can modify for diag call if choose to avoid if outflow condition
    // or do something simple for outflow just for diagnostics
#if(FULLOUTPUT!=0 && PRODUCTION==0)
    bound_anyprim(boundstage, boundtime, boundvartype, uavg,pstag, uavg, finalstep,doboundmpi);
#endif


    if(DOENOFLUX == ENOFINITEVOLUME){
      // then need to bound all conservative quantities
      // SUPERGODMARK -- CHANGINGMARK -- need to generalize so bound knows if p or U quantity
      // other methods have only field "averaged" so only need to modify it
      bound_anyprim(boundstage, boundtime, boundvartype, uavg, pstag, uavg, finalstep,doboundmpi);
    }

    if(FLUXB==FLUXCTSTAG){
      // bound unew for staggered fields
      // SUPERGODMARK -- CHANGINGMARK: need to tell bound if p or U
      if(boundvartype==BOUNDPRIMTYPE) mystagboundvar=BOUNDPSTAGTYPE;
      else if(boundvartype==BOUNDPRIMSIMPLETYPE) mystagboundvar=BOUNDPSTAGSIMPLETYPE;
      else  mystagboundvar=boundvartype;

      bound_anypstag(boundstage, boundtime, mystagboundvar, uavg, pstag, uavg, finalstep,doboundmpi);
    }
  }

  return(0);

}





// bound any prim type
// CALLS directly real and MPI boundary functions
// pstag = pstagglobal
// ucons = unewglobal
int bound_anyprim(int boundstage, SFTYPE boundtime, int boundvartype, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  int whichpltoavg[NPR];
  int ifnotavgthencopy[NPR];
  int nprlocalstart,nprlocalend;
  int nprlocallist[MAXNPR];
  int pl,pliter;
  int dir;




  if(DOGRIDSECTIONING){
    if((boundstage==STAGE0)||(boundstage==STAGEM1)){
      MYFUN(bound_gridsectioning(CENTEREDPRIM,prim,pstag,ucons,finalstep),"step_ch.c:bound_pstag()", "bound_pstag_user()", 1);
    }
  }



  DIMENLOOP(dir){

    if(0 && FLUXB==FLUXCTSTAG){ // apparently need to bound both fields SUPERGODMARK

      PALLLOOP(pl) whichpltoavg[pl]=1;
      PLOOPBONLY(pl) whichpltoavg[pl]=0;

      PALLLOOP(pl) ifnotavgthencopy[pl]=0;
      PLOOPBONLY(pl) ifnotavgthencopy[pl]=0;

      addremovefromanynpr(REMOVEFROMNPR, whichpltoavg, ifnotavgthencopy, &nprboundstart, &nprboundend, nprboundlist, &nprlocalstart, &nprlocalend, nprlocallist, NULL, NULL);
    }
    else{
      // no change then, doing all variables
    }



    // real boundary zones
    if((boundstage==STAGE0)||(boundstage==STAGEM1)){
      MYFUN(bound_prim_user_dir(boundstage, boundtime, dir, boundvartype, prim),"step_ch.c:bound_prim()", "bound_prim_user()", 1);
    }// end if stage0 or stagem1
  
    if(doboundmpi){
      MYFUN(bound_mpi_dir(boundstage, dir, boundvartype, prim, NULL, NULL, NULL),"step_ch.c:bound_prim()", "bound_mpi()", 1);
    }

    // real boundary zones
    if((boundstage==STAGE0)||(boundstage==STAGEM1)){
      MYFUN(bound_prim_user_after_mpi_dir(boundstage, boundtime, dir, prim),"step_ch.c:bound_prim()", "bound_prim_user_after_mpi()", 1);
    }// end if stage0 or stagem1


    ////////////////////////////////////////////
    //
    // restore choice for bounding

    if(0 && FLUXB==FLUXCTSTAG){ // apparently need to bound both fields SUPERGODMARK
      addremovefromanynpr(RESTORENPR, whichpltoavg, ifnotavgthencopy, &nprboundstart, &nprboundend, nprboundlist, &nprlocalstart, &nprlocalend, nprlocallist, NULL, NULL);
    }
  }

  
  return(0);
}


// bound any pstag type
// CALLS directly real and MPI boundary functions
// pstag is at FACE1,2,3 for fields, so user bound is different
// MPI bounding is the same as CENT quantities
int bound_anypstag(int boundstage, SFTYPE boundtime, int boundvartype, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  int pl,pliter;
  int nprlocalstart,nprlocalend;
  int nprlocallist[MAXNPR];
  int dir;
  int mystagboundvar;


  if(boundvartype==BOUNDPRIMTYPE) mystagboundvar=BOUNDPSTAGTYPE;
  else if(boundvartype==BOUNDPRIMSIMPLETYPE) mystagboundvar=BOUNDPSTAGSIMPLETYPE;
  else  mystagboundvar=boundvartype;





  if(DOGRIDSECTIONING){
    if((boundstage==STAGE0)||(boundstage==STAGEM1)){
      MYFUN(bound_gridsectioning(STAGGEREDPRIM,prim,pstag,ucons,finalstep),"step_ch.c:bound_pstag()", "bound_pstag_user()", 1);
    }
  }




  DIMENLOOP(dir){
  
    ////////////////////////////////////////////
    //
    // save choice for bounding
    nprlocalstart=nprboundstart;
    nprlocalend=nprboundend;
    PMAXNPRLOOP(pl) nprlocallist[pl]=nprboundlist[pl];

    // choose range of PBOUNDLOOP and PLOOPMPI
    nprboundstart=0;
    nprboundend=2;
    nprboundlist[0]=B1;
    nprboundlist[1]=B2;
    nprboundlist[2]=B3;


    // real boundary zones
    if((boundstage==STAGE0)||(boundstage==STAGEM1)){
      MYFUN(bound_pstag_user_dir(boundstage,boundtime, dir,mystagboundvar,pstag),"step_ch.c:bound_pstag()", "bound_pstag_user()", 1);
    }// end if stage0 or stagem1


    if(doboundmpi){
      MYFUN(bound_mpi_dir(boundstage, dir, mystagboundvar, pstag, NULL, NULL, NULL),"step_ch.c:bound_pstag()", "bound_mpi()", 1);
    }

    // real boundary zones
    if((boundstage==STAGE0)||(boundstage==STAGEM1)){
      MYFUN(bound_prim_user_after_mpi_dir(boundstage,boundtime, dir,pstag),"step_ch.c:bound_pstag()", "bound_prim_user_after_mpi()", 1);
    }// end if stage0 or stagem1


    ////////////////////////////////////////////
    //
    // restore choice for bounding
    nprboundstart=nprlocalstart;
    nprboundend=nprlocalend;
    PMAXNPRLOOP(pl) nprboundlist[pl]=nprlocallist[pl];
  }


  return(0);
}


// bound any flux type
// CALLS directly real and MPI boundary functions
// special bound for flux that only bounds in direction of flux itself (so not full cross-flux information)
// used for finite difference version of ENO
int bound_flux(int boundstage, SFTYPE boundtime, int boundvartype, FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR], int finalstep, int doboundmpi)
{
  int dir;



  if(boundvartype==BOUNDFLUXTYPE || boundvartype==BOUNDFLUXSIMPLETYPE){
    // then can stay
  }
  else{
    dualfprintf(fail_file,"Shouldn't be in bound_flux() with boundvartype=%d\n",boundvartype);
    myexit(2467367);
  }


  if(DOENOFLUX==ENOFLUXRECON && BOUNDFLUXRECON==0){
    dualfprintf(fail_file,"DEBUG: Assuming bound_flux() called only for debugging purposes\n");
  }


  // real boundary zones
  if((boundstage==STAGE0)||(boundstage==STAGEM1)){
    MYFUN(bound_flux_user(boundstage,boundtime, boundvartype,F1,F2,F3),"step_ch.c:bound_flux()", "bound_flux_user()", 1);
  }// end if stage0 or stagem1


  if(doboundmpi){
    MYFUN(bound_mpi(boundstage, boundvartype, NULL, F1, F2, F3),"step_ch.c:bound_flux()", "bound_mpi()", 1);
  }


  return(0);
}




// bound pflag type
// CALLS directly real and MPI boundary functions
int bound_pflag(int boundstage, SFTYPE boundtime, PFTYPE (*prim)[NSTORE2][NSTORE3][NUMPFLAGS], int finalstep, int doboundmpi)
{
  int boundvartype=BOUNDINTTYPE;
  

  if((boundstage==STAGE0)||(boundstage==STAGEM1)){

    MYFUN(bound_pflag_user(boundstage, boundtime, boundvartype, prim),"step_ch.c:bound_pflag()", "bound_pflag_user()", 1);

  }// end bound stage


  if(doboundmpi){

    if(UTOPRIMFIXMPICONSISTENT==1){
      MYFUN(bound_mpi_int(boundstage, boundvartype, prim),"step_ch.c:bound_pflag()", "bound_mpi_int()", 1);
    }
    else{
      // need to fill boundary cells with failure
      // otherwise, would have to go deeper into fixups and dependency chain for the UTOPRIMFIXMPICONSISTENT chocie would become too deep
      MYFUN(bound_mpi_int_fakeutoprimmpiinconsisent(boundstage, boundvartype, prim,UTOPRIMFAILFAKEVALUE),"step_ch.c:bound_pflag()", "bound_mpi_int()", 1);
    }

  }

  return(0);

}









