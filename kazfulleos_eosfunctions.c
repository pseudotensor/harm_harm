// actual EOS physics stuff
// stuff that should never directly access global EOS arrays
// all uses of lineartablelimits[] should be on non-temperature-like quantities (i.e. only on rho,ye,ynu,h)

// TODO: create and use pipelined EOS lookup!


static int offsetquant2(int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2in, FTYPE *quant2out);
static int offsetquant2_general(int whichd, FTYPE quant1, FTYPE quant2in, FTYPE *quant2out);
static int offsetquant2_general_inverse(int whichd, FTYPE quant1, FTYPE quant2in, FTYPE *quant2out);

static void constrainH(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
static void get_lambdatot(FTYPE *EOSextra, FTYPE rho0, FTYPE u, FTYPE *lambdatotptr);
static FTYPE pressure_dp_rho0_u_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u, FTYPE *dp);
static FTYPE pressure_dp_wmrho0_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0, FTYPE *dp);


static int get_dquant2(int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE *dquant2, FTYPE *unu, FTYPE *pnu, FTYPE *chinu, FTYPE *snu, FTYPE *quant2nu);
static int usereduced_eos(int domod, int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE *final);


static FTYPE dfun2fun_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE *dfinalreturn);
static int dfun2fun_inputdquant2_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE dquant2, FTYPE unu, FTYPE pnu, FTYPE chinu, FTYPE snu, int badlookup, FTYPE *dfinal, FTYPE *final);


static int prepare_fudgefrac_kazfull(
				     int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, // inputs
				     FTYPE unu, FTYPE pnu, FTYPE chinu, FTYPE snu, // inputs
				     FTYPE *utot, FTYPE *ugas, // outputs
				     FTYPE *ptot, FTYPE *pgas, // outputs
				     FTYPE *chitot, FTYPE *chigas // outputs
				     );
static int preparepart2_fudgefrac_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, // inputs
					  FTYPE quant2nu, FTYPE quant2, // inputs
					  FTYPE *fakeneutrino, FTYPE *faketotal // outputs
					  );
static int finish_fudgefrac_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, // inputs
				    FTYPE utot, FTYPE ugas, FTYPE unu, // inputs
				    FTYPE ptot, FTYPE pgas, FTYPE pnu, // inputs
				    FTYPE chitot, FTYPE chigas, FTYPE chinu, // inputs
				    FTYPE fakeneutrino, FTYPE faketotal, // inputs
				    int badlookup, FTYPE nonneutrino, // inputs
				    FTYPE *final // outputs
				    );
static FTYPE fudgefracsingle_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2);
static FTYPE compute_tabulated_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE rho0, FTYPE u);


// general offset for energy per baryon to be applied always before accessing table with nuclear EOS
// quant2out is the \delta u that is tabulated in the EOS table itself.
// inputted quant2in is some true internal energy (maybe no neutrinos)
// So we add the offset before looking up the results from the table that has the offset
static int offsetquant2(int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2in, FTYPE *quant2out)
{


  if(whichd==UTOTDIFF || whichd==CHIDIFF){
    // nuclear offset
    // assumes lsoffset in helm/jon_lsbox.f is offsetting only energy/baryon = u/\rho_0
    *quant2out = quant2in + TRUENUCLEAROFFSET*quant1;
  }


  return(0);

}

static int offsetquant2_general(int whichd, FTYPE quant1, FTYPE quant2in, FTYPE *quant2out)
{

  *quant2out = quant2in + DEGENNUCLEAROFFSET*quant1;

  return(0);

}

static int offsetquant2_general_inverse(int whichd, FTYPE quant1, FTYPE quant2in, FTYPE *quant2out)
{

  *quant2out = quant2in - DEGENNUCLEAROFFSET*quant1;

  return(0);

}


// get dquant2 = quant2 - quant2nu, the difference between full temperature-like quantity and neutrino version
// dquant2 used directly for lookup
static int get_dquant2(int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE *dquant2, FTYPE *unu, FTYPE *pnu, FTYPE *chinu, FTYPE *snu, FTYPE *quant2nu)
{

  if(whichdatatype[primarytable]==4){

    *unu = EOSextra[UNUGLOBAL];
    *pnu = EOSextra[PNUGLOBAL];
    *chinu = (*unu + *pnu);
    *snu = EOSextra[SNUGLOBAL];

    if(whichd==UTOTDIFF){
      *quant2nu = *unu;
    }
    else if(whichd==PTOTDIFF){
      *quant2nu = *pnu;
    }
    else if(whichd==CHIDIFF){
      *quant2nu = *chinu;
    }
    else if(whichd==STOTDIFF){
      *quant2nu = *snu;
    }

    // get dquant2
    *dquant2  = quant2 - *quant2nu;

  }
  else{
    // then neutrino inside table already
    *unu= *pnu= *chinu= *snu=0.0;
    *dquant2 = quant2;
    *quant2nu = *chinu = *pnu = *unu = 0.0;
  }



  return(0);

}



// tabulated dfun[du] and need to get u=u+unu and then fun= dfun+fun_nu
static FTYPE dfun2fun_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE *dfinalreturn)
{
  FTYPE final,dfinal;
  FTYPE unu,pnu,chinu,snu;
  FTYPE dquant2,quant2nu;
  int badlookup;

  ///////////////////
  //
  // get dquant2 for lookup and get associated \nu quantities for the final answer
  //
  ///////////////////
  get_dquant2(whichd, EOSextra, quant1, quant2, &dquant2, &unu, &pnu, &chinu, &snu, &quant2nu);


  badlookup=getsingle_eos_fromtable(whichfun,whichd,EOSextra,quant1,dquant2,&dfinal); // input quant1,dquant2 and get dfinal

  // get final and possibly different dfinal results
  dfun2fun_inputdquant2_kazfull(whichfun, whichd, EOSextra, quant1, quant2, dquant2, unu, pnu, chinu, snu, badlookup, &dfinal, &final);

  //////////////
  //
  // sometimes need pure "gas" (non-neutrino) part of answer
  //
  //////////////
  *dfinalreturn=dfinal;
  
  //////////////
  //
  // return final full answer
  //
  //////////////
  return(final);

}







// tabulated dfun[du] and need to get u=u+unu and then fun= dfun+fun_nu
// no lookup here!  Just process to get "final" full EOS value
static int dfun2fun_inputdquant2_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE dquant2, FTYPE unu, FTYPE pnu, FTYPE chinu, FTYPE snu, int badlookup, FTYPE *dfinal, FTYPE *final)
{


  ///////////////////
  //
  // do lookup
  //
  ///////////////////
  if(badlookup){
    usereduced_eos(1,whichfun,whichd,EOSextra,quant1,quant2,final);
    *dfinal=*final; // correct dfinal
  }
  else{
    // sum up to get final answer for function value
    if(whichfun==PofRHOU)          *final = *dfinal + pnu;
    else if(whichfun==UofRHOP)     *final = *dfinal + unu;
    else if(whichfun==UofRHOS)     *final = *dfinal + unu;
    else if(whichfun==SofRHOU)     *final = *dfinal + snu;
    else if(whichfun==SSofRHOCHI)  *final = *dfinal + snu/quant1; // specific entropy is snu/rho0 in code units
    else if(whichfun==PofRHOCHI)   *final = *dfinal + pnu;
  }


  return(0);
}



// use reduced ideal-like EOS since (probably) bad table lookup
static int usereduced_eos(int domod, int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE *final)
{
  FTYPE quant2mod;


  if(domod){
    if(whichd==UTOTDIFF || whichd==CHIDIFF){
      quant2mod = quant2 + FAKE2IDEALNUCLEAROFFSET*quant1; // set nuclear per baryon offset so can smoothly connect to ideal gas EOS
    }
    else if(whichd==STOTDIFF){
      // SUPERGODMARK: Need to offset by u/(kb*t)
      //      quant2mod = quant2 + FAKE2IDEALNUCLEAROFFSET*quant1; // set nuclear per baryon offset so can smoothly connect to ideal gas EOS     
    }
  }
  else{
    // no mod
    quant2mod = quant2;
  }


    // otherwise use TM EOS
#if(REDUCE2WHICHEOS==MIGNONE)
    if(whichfun==PofRHOU)        *final = pressure_rho0_u_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==UofRHOP)   *final = u_rho0_p_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==UofRHOS)   *final = compute_u_from_entropy_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==SofRHOU)   *final = compute_entropy_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==SSofRHOCHI)   *final = compute_specificentropy_wmrho0_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==PofRHOCHI) *final = pressure_wmrho0_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==CS2ofRHOU)           *final = cs2_compute_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==DPDUofRHOU)     *final = dpdu_rho0_u_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==DPDRHOofRHOU)   *final = dpdrho0_rho0_u_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==IDRHO0DP)       *final = compute_idrho0dp_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==IDCHIDP)        *final = compute_idwmrho0dp_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==DSDRHOofRHOU)   *final = compute_dSdrho_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==DSDUofRHOU)     *final = compute_dSdu_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==DSSDRHOofRHOCHI) *final = compute_dspecificSdrho_wmrho0_mignone(EOSextra,quant1, quant2mod);
    else if(whichfun==DSSDCHIofRHOCHI) *final = compute_dspecificSdwmrho0_wmrho0_mignone(EOSextra,quant1, quant2mod);
#elif(REDUCE2WHICHEOS==IDEALGAS)
    // use ideal EOS
    if(whichfun==PofRHOU)        *final = pressure_rho0_u_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==UofRHOP)   *final = u_rho0_p_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==UofRHOS)   *final = compute_u_from_entropy_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==SofRHOU)   *final = compute_entropy_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==SSofRHOCHI)   *final = compute_specificentropy_wmrho0_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==PofRHOCHI) *final = pressure_wmrho0_idealgas(EOSextra,quant1, quant2mod);    
    else if(whichfun==CS2ofRHOU)           *final = cs2_compute_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==DPDUofRHOU)     *final = dpdu_rho0_u_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==DPDRHOofRHOU)   *final = dpdrho0_rho0_u_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==IDRHO0DP)       *final = compute_idrho0dp_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==IDCHIDP)        *final = compute_idwmrho0dp_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==DSDRHOofRHOU)   *final = compute_dSdrho_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==DSDUofRHOU)     *final = compute_dSdu_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==DSSDRHOofRHOCHI) *final = compute_dspecificSdrho_wmrho0_idealgas(EOSextra,quant1, quant2mod);
    else if(whichfun==DSSDCHIofRHOCHI) *final = compute_dspecificSdwmrho0_wmrho0_idealgas(EOSextra,quant1, quant2mod);
#endif

    return(0);
}





// setup for fudgefrac()
// GODMARK: introduces another lookup, so kinda expensive for something like the derivative that probably does not often have dominant neutrino contribution
static int prepare_fudgefrac_kazfull(
				     int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, // inputs
				     FTYPE unu, FTYPE pnu, FTYPE chinu, FTYPE snu, // inputs
				     FTYPE *utot, FTYPE *ugas, // outputs
				     FTYPE *ptot, FTYPE *pgas, // outputs
				     FTYPE *chitot, FTYPE *chigas // outputs
				     )
{

  // first get total pressure in order to frac-fudge the answer
  if(whichd==UTOTDIFF){
    // pressure_dp_rho0_u() takes in P(rho0,u) not P(rho0,du), so feed in quant1,quant2
    *ptot = pressure_dp_rho0_u_kazfull(EOSextra, quant1, quant2, pgas); // need pgas to form chi since otherwise don't have it
    *utot = quant2;
    *ugas = *utot - unu; // *ugas = *utot - unu
    *chigas = quant2 + *pgas;
    *chitot = *chigas + chinu;
  }
  else if(0&&whichd==PTOTDIFF){
    // Not setup.  Not needed yet.  Would require (utot,ugas)[rho0,p]
  }
  else if(whichd==CHIDIFF){
    // pressure_dp_rho0_u() takes in P(rho0,u) not P(rho0,du), so feed in quant1,quant2
    *ptot =pressure_dp_wmrho0_kazfull(EOSextra, quant1, quant2, pgas); // don't need separate pgas
    *chitot = quant2;
    *chigas = *chitot - chinu;
    *utot = *chitot - *ptot;
    *ugas = *utot - unu;
  }
  else if(0&&whichd==STOTDIFF){
    // don't need fudgefrac(STOTDIFF) since only used for complicated functions like derivatives that have mixed derivatives between gas and neutrino terms
    // pressure_dp_rho0_s() takes in P(rho0,Sden) not P(rho0,dSden), so feed in quant1,quant2
    //    utot =u_du_rho0_s_kazfull(EOSextra, quant1, quant2, &ugas); // need ugas to form chi since otherwise don't have it
    //    ptot =pressure_dp_rho0_s_kazfull(EOSextra, quant1, quant2, &pgas); // need pgas to form chi since otherwise don't have it
    //    chigas = ugas+pgas;
  }
  else{
    dualfprintf(fail_file,"prepare_fudgefrac_kazfull not setup for whichd=%d\n",whichd);
    myexit(19672606);
  }





  return(0);

}


// setup for fudgefrac()
// input dpfun instead of extra lookup
static int prepare_fudgefrac_input_dpofwhichd_kazfull(
				     int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, // inputs
				     FTYPE unu, FTYPE pnu, FTYPE chinu, FTYPE snu, // inputs
				     FTYPE dpfun, // inputs
				     FTYPE *utot, FTYPE *ugas, // outputs
				     FTYPE *ptot, FTYPE *pgas, // outputs
				     FTYPE *chitot, FTYPE *chigas // outputs
				     )
{

  // first get total pressure in order to frac-fudge the answer
  if(whichd==UTOTDIFF){
    // pressure_dp_rho0_u() takes in P(rho0,u) not P(rho0,du), so feed in quant1,quant2
    *pgas = dpfun;
    *ptot = *pgas + pnu;

    *utot = quant2;
    *ugas = *utot - unu; // *ugas = *utot - unu
    *chigas = quant2 + *pgas;
    *chitot = *chigas + chinu;
  }
  else if(0&&whichd==PTOTDIFF){
    // Not setup.  Not needed yet.  Would require (utot,ugas)[rho0,p]
  }
  else if(whichd==CHIDIFF){
    // pressure_dp_rho0_u() takes in P(rho0,u) not P(rho0,du), so feed in quant1,quant2
    *pgas = dpfun;
    *ptot = *pgas + pnu;

    *chitot = quant2;
    *chigas = *chitot - chinu;
    *utot = *chitot - *ptot;
    *ugas = *utot - unu;
  }
  else if(0&&whichd==STOTDIFF){
    // don't need fudgefrac(STOTDIFF) since only used for complicated functions like derivatives that have mixed derivatives between gas and neutrino terms
    // pressure_dp_rho0_s() takes in P(rho0,Sden) not P(rho0,dSden), so feed in quant1,quant2
    //    utot =u_du_rho0_s_kazfull(EOSextra, quant1, quant2, &ugas); // need ugas to form chi since otherwise don't have it
    //    ptot =pressure_dp_rho0_s_kazfull(EOSextra, quant1, quant2, &pgas); // need pgas to form chi since otherwise don't have it
    //    chigas = ugas+pgas;
  }
  else{
    dualfprintf(fail_file,"prepare_fudgefrac_input_dpofwhichd_kazfull not setup for whichd=%d\n",whichd);
    myexit(19672606);
  }





  return(0);

}





static int preparepart2_fudgefrac_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, // inputs
					  FTYPE quant2nu, FTYPE quant2, // inputs
					  FTYPE *fakeneutrino, FTYPE *faketotal // outputs
					  )
{


  ////////////////////////////////////////
  //
  // determine fake-neutrino part
  // GODMARK: Assume at least that if neutrino pressure is dominant then treat like mixture of gas+neutrinos
  //
  ////////////////////////////////////////
  usereduced_eos(0,whichfun,whichd, EOSextra, quant1, quant2nu, fakeneutrino);

  // reduce to using totals if not within table
  usereduced_eos(1,whichfun,whichd, EOSextra, quant1, quant2, faketotal);

  return(0);
}





// finish-up for fudgefrac()
static int finish_fudgefrac_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, // inputs
				    FTYPE utot, FTYPE ugas, FTYPE unu, // inputs
				    FTYPE ptot, FTYPE pgas, FTYPE pnu, // inputs
				    FTYPE chitot, FTYPE chigas, FTYPE chinu, // inputs
				    FTYPE fakeneutrino, FTYPE faketotal, // inputs
				    int badlookup, FTYPE nonneutrino, // inputs
				    FTYPE *final // outputs
				     )
{
  FTYPE frac;



  if(badlookup){
    *final=faketotal;
  }
  else{

    // then estimate importance of neutrinos and its control over sound speed
    if(whichfun==CS2ofRHOU){
      // for sound speed more accurate to do:
      // c_s^2[total] = c_s^2[gas]*(hgas/htotal) + c_s^2[neutrino]*(hneutrino/htotal)
      // hgas = (rho_0 + ugas + pgas)/rho_0
      // hneutrino = (rho_0 + unu + pnu)/rho_0
      *final = (nonneutrino*(quant1+chigas) + fakeneutrino*(quant1+chinu))/(quant1+chitot);

      //      if(!isfinite(final)){
      //	dualfprintf(fail_file,"IF: final=%21.15g chitot=%21.15g\n",final,chitot);
      //	dualfprintf(fail_file,"%21.15g%21.15g%21.15g%21.15g%21.15g%21.15g%21.15g%21.15g\n",nonneutrino,quant1,chigas,fakeneutrino,quant1,chinu,quant1,chitot);
      //      }
      
      
    }
    else{
      //    frac = (pnu/(ptot-pnu));
      frac = pnu/ptot; // -> 1 if ptot=pnu.  Suppose photons+electrons+neutrinos equally dominate.  Then cs2 still correct.
      // choose cs2total if neutrino-dominated, otherwise choose non-neutrino cs2
      *final = faketotal*frac + nonneutrino*(1.0-frac);
    }
  }// end else if good lookup

  ///////////////
  //
  // return that was success
  //
  ///////////////
  return(0);


}





// general function to interpolate between non-neutrino and neutrino values
// used for those quantities not yet setup for exactly correct answer (more specifically, fudgefrac() used for derivatives that would otherwise involved mixed derivatives between gas and neutrino terms that are not easily computed/stored without many extra things to store
// never used on pressure itself, but pressure looked-up in prepare_fudgefrac_kazfull() so expensive in that sense
static FTYPE fudgefracsingle_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2)
{
  FTYPE ptot, pgas, pnu;
  FTYPE utot, ugas, unu;
  FTYPE chitot, chigas, chinu;
  FTYPE snu;
  FTYPE dquant2,quant2nu,quant2mod;
  FTYPE fakeneutrino,faketotal;

  FTYPE nonneutrino;
  int badlookup;

  FTYPE final;



  ///////////////////
  //
  // get dquant2 for lookup and get associated \nu quantities for the final answer
  //
  ///////////////////
  get_dquant2(whichd, EOSextra, quant1, quant2, &dquant2, &unu, &pnu, &chinu, &snu, &quant2nu);


  /////////////////////////////////
  //
  // now get non-neutrino value
  // nonneutrino will then be only "gas" part without neutrinos
  //
  /////////////////////////////////
  badlookup=getsingle_eos_fromtable(whichfun,whichd,EOSextra,quant1,dquant2,
				    &nonneutrino);


  ////////////////////////
  //
  // setup fudgefrac()
  //
  ////////////////////////
  prepare_fudgefrac_kazfull(whichd, EOSextra, quant1, quant2,
			    unu, pnu, chinu, snu,
			    &utot, &ugas,
			    &ptot, &pgas,
			    &chitot, &chigas
			    );

  ////////////////////////
  //
  // get estimated full neutrino and fake total values
  //
  ////////////////////////
  preparepart2_fudgefrac_kazfull(whichfun, whichd, EOSextra, quant1,
				 quant2nu, quant2,
				 &fakeneutrino, &faketotal
				 );

  ////////////////////////
  //
  // finish fudgefrac()
  //
  ////////////////////////
  finish_fudgefrac_kazfull(whichfun, whichd, EOSextra, quant1, quant2,
			   utot, ugas, unu,
			   ptot, pgas, pnu,
			   chitot, chitot, chinu,
			   fakeneutrino, faketotal,
			   badlookup, nonneutrino,
			   &final);

  ////////////////////////
  //
  // return single value
  //
  ////////////////////////
  return(final);

}




// returns all 3 quantities for inversion based upon p(\rho_0,\chi) or sspec(\rho_0,\chi)
// note that no optimization for 5D method based upon p(\rho_0,u), but could set that up.  Would have to add P to dP table for simplicity.
int get_stuff_forinversion_kazfull(int eomtype, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPE *fun, FTYPE *dfunofrho, FTYPE *dfunofu)
{
  FTYPE utot, ugas, unu;
  FTYPE ptot, pgas, pnu;
  FTYPE chitot, chigas, chinu;
  FTYPE snu;
  FTYPE dquant2,quant2nu;
  FTYPE fakeneutrino,faketotal;

  FTYPE deltafun;

  FTYPE nonneutrinos[MAXEOSPIPELINE];
  FTYPE finals[MAXEOSPIPELINE];
  int badlookups[MAXEOSPIPELINE];
  int iffun[MAXEOSPIPELINE];
  int whichtablesubtype;
  int whichd;
  int numcols,coli;

  int returnfun,returndfunofrho,returndfunofu;
  int failreturn;
  int whichfun;
  int simplefudge;



  ///////////////
  //
  // get number of columns possible for this whichtablesubtype
  //
  ///////////////
  if(eomtype==EOMGRMHD){
    whichtablesubtype=SUBTYPEPOFCHI;
    returnfun = PofRHOCHI;
    returndfunofrho = IDRHO0DP;
    returndfunofu = IDCHIDP;
    whichd=CHIDIFF;
  }
  else if(eomtype==EOMENTROPYGRMHD){
    whichtablesubtype=SUBTYPESSPEC;
    returnfun = SSofRHOCHI;
    returndfunofrho = DSSDRHOofRHOCHI;
    returndfunofu = DSSDCHIofRHOCHI;
    whichd=CHIDIFF;
  }
  numcols = numcolintablesubtype[whichtablesubtype];

  // setup iffun
  for(coli=0;coli<numcols;coli++) iffun[coli]=1; // want all!


  ///////////////////
  //
  // get dquant2 for lookup and get associated \nu quantities for the final answer
  //
  ///////////////////
  get_dquant2(whichd, EOSextra, quant1, quant2, &dquant2, &unu, &pnu, &chinu, &snu, &quant2nu);






  /////////////////////////////////
  //
  // now get non-neutrino value
  // nonneutrino will then be only "gas" part without neutrinos
  //
  /////////////////////////////////
  failreturn=get_eos_fromtable(whichtablesubtype, iffun, whichd,EOSextra,quant1,dquant2,nonneutrinos,badlookups);
  

  if(whichtablesubtype==SUBTYPEPOFCHI){
    // get p - pnu
    deltafun = nonneutrinos[returnfun];
    simplefudge=1;
  }
  else{
    simplefudge=0;
  }

  ////////////////////////
  //
  // get final results for function and its derivatives
  //
  ////////////////////////
  for(coli=0;coli<numcols;coli++){ // don't need to deal with iffun since want all

    whichfun = coli+firsteosintablesubtype[whichtablesubtype];

    if(whichfun==returndfunofrho || whichfun==returndfunofu){

      ////////////////////////
      //
      // setup fudgefrac() for dpofchidrho0 and dpofchidchi
      //
      ////////////////////////

      if(simplefudge){
	// special prepare_fudgefrac that inputs already-looked-up p-pnu
	prepare_fudgefrac_input_dpofwhichd_kazfull(whichd, EOSextra, quant1, quant2,
						   unu, pnu, chinu, snu,
						   deltafun,
						   &utot, &ugas,
						   &ptot, &pgas,
						   &chitot, &chigas
						   );
      }
      else{
	// then need to look up pressure still
	prepare_fudgefrac_kazfull(whichd, EOSextra, quant1, quant2,
				  unu, pnu, chinu, snu,
				  &utot, &ugas,
				  &ptot, &pgas,
				  &chitot, &chigas
				  );

      }

      // get preparepart2_fudgefrac(), which is done per final derivative quantity
      preparepart2_fudgefrac_kazfull(whichfun, whichd, EOSextra, quant1,
				     quant2nu, quant2,
				     &fakeneutrino, &faketotal
				     );
      
      if(failreturn==0){
	// finish fudgefrac(), which is done per final derivative quantity
	finish_fudgefrac_kazfull(whichfun, whichd, EOSextra, quant1, quant2,
				 utot, ugas, unu,
				 ptot, pgas, pnu,
				 chitot, chitot, chinu,
				 fakeneutrino, faketotal,
				 badlookups[coli], nonneutrinos[coli],
				 &finals[coli]);
      }
      else{
	// reduce to fake total if true failure in lookup
	finals[coli]=faketotal;
      }

      ////////
      //
      // assign to final values
      //
      ///////
      if(whichfun==returndfunofrho){
	*dfunofrho=finals[coli];
      }
      else if(whichfun==returndfunofu){
	*dfunofu=finals[coli];
      }
    }// end if a derivative
    else if(whichfun==returnfun){
      // then process non-derivative using finish_dfun2fun()

      // finish dffun2fun(), which is done for the non-derivative quantity
      FTYPE dfun; // don't need dfun
      dfun2fun_inputdquant2_kazfull(whichfun, whichd, EOSextra, quant1, quant2, dquant2, unu, pnu, chinu, snu, badlookups[coli], &dfun, fun);

    }
  }// end over coli


  ////////////////////////
  //
  // return whether successful
  //
  ////////////////////////
  return(failreturn);

}





// used for fully tabulated quantities that are functions of du/dp/dchi with whichdatatype==4
FTYPE compute_tabulated_kazfull(int whichfun, int whichd, FTYPE *EOSextra, FTYPE rho0, FTYPE quant2)
{
  FTYPE final;
  FTYPE dquant2;

  if(whichdatatype[primarytable]==4){
    if(whichd==UTOTDIFF)      dquant2 = quant2 - EOSextra[UNUGLOBAL];
    else if(whichd==PTOTDIFF) dquant2 = quant2 - EOSextra[PNUGLOBAL];
    else if(whichd==CHIDIFF)  dquant2 = quant2 - (EOSextra[UNUGLOBAL]+EOSextra[PNUGLOBAL]);
    else if(whichd==STOTDIFF) dquant2 = quant2 - EOSextra[SNUGLOBAL];
  }
  else{
    dquant2 = quant2;
  }

  
  // do lookup
  if(getsingle_eos_fromtable(whichfun,whichd,EOSextra,rho0,dquant2,&final)){ // uses dquant2
    if(whichfun==TEMPU || whichfun==TEMPP || whichfun==TEMPCHI || whichfun==TEMPS) final=0.0;
    else if(whichfun==QDOTNU) final=0.0;
    else final=0.0; // use mignone?
  }  

  return(final);

}










// p(rho0, u) (needed to get initial guess for W)
FTYPE pressure_rho0_u_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  FTYPE dp;
  return(dfun2fun_kazfull(PofRHOU, UTOTDIFF, EOSextra, rho0, u, &dp));
}

// p(rho0, u) (used internally)
static FTYPE pressure_dp_rho0_u_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u, FTYPE *dp)
{
  return(dfun2fun_kazfull(PofRHOU, UTOTDIFF, EOSextra, rho0, u, dp));
}



// u(rho0, p) (used for initial conditions)
FTYPE u_rho0_p_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE p)
{
  FTYPE du;
  return(dfun2fun_kazfull(UofRHOP, PTOTDIFF, EOSextra, rho0, p, &du));
}


// p(rho0, w-rho0 = u+p)
// Notice that using EOSextraglobal bypasses need to have other quantities as functions of wmrho0 unless want more direct (by iteration) result compared to what EOSextraglobal gives
FTYPE pressure_wmrho0_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0)
{
  FTYPE dp;
  return(dfun2fun_kazfull(PofRHOCHI, CHIDIFF, EOSextra, rho0, wmrho0, &dp));
}

// used internally
static FTYPE pressure_dp_wmrho0_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0, FTYPE *dp)
{
  return(dfun2fun_kazfull(PofRHOCHI, CHIDIFF, EOSextra, rho0, wmrho0, dp));
}






// frac-fudged because dpdu is complicated with p=dp+pnu and u=du+unu
FTYPE dpdu_rho0_u_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  return(fudgefracsingle_kazfull(DPDRHOofRHOU,UTOTDIFF,EOSextra,rho0, u));
}


// for this could store dp_\nu/drho0, which for whichdatatype==4 is 0 given other independent variables are rho0,T,Y_e,Y_\nu,H
// for whichdatatype==4 check that dpnu/drho0 = 0 for whichdatatype==4 if ever want to use that method (not now)
// dp(rho0, u)/drho0
// frac-fudged since dp/drho0 would otherwise involve dpnu/drho0 that would require storing many neutrino-related derivatives
FTYPE dpdrho0_rho0_u_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  return(fudgefracsingle_kazfull(DPDUofRHOU,UTOTDIFF,EOSextra,rho0, u));
}


// sound speed squared (for vchar.c) -- important for treatment of shocks
// frac-fudged
FTYPE cs2_compute_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  return(fudgefracsingle_kazfull(CS2ofRHOU,UTOTDIFF,EOSextra,rho0, u));
}


// entropy as function of rho0 and internal energy (u)
// Sden(rho0,u)
// tabulated ds(du), so first compute du and then ds and then s
FTYPE compute_entropy_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  FTYPE ds;
  return(dfun2fun_kazfull(SofRHOU, UTOTDIFF, EOSextra, rho0, u, &ds));
}

// u(rho0,Sden)
// here S is entropy is entropy per unit volume
FTYPE compute_u_from_entropy_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE entropy)
{
  FTYPE du;
  return(dfun2fun_kazfull(UofRHOS, STOTDIFF, EOSextra, rho0, entropy, &du));
}


// used for dudp_calc
// frac-fudged
FTYPE compute_dSdrho_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  return(fudgefracsingle_kazfull(DSDRHOofRHOU,UTOTDIFF,EOSextra,rho0, u));
}


// used for dudp_calc
// frac-fudged
FTYPE compute_dSdu_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  return(fudgefracsingle_kazfull(DSDUofRHOU,UTOTDIFF, EOSextra, rho0, u));
}

// exactly correct answer (not frac-fudged)
// entropy as function of rho0 and internal energy (u)
// S(rho0,\chi)
// tabulated ds(d\chi), so first compute d\chi and then ds and then s
FTYPE compute_specificentropy_wmrho0_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0)
{
  FTYPE ds;
  return(dfun2fun_kazfull(SSofRHOCHI, CHIDIFF, EOSextra, rho0, wmrho0, &ds));
}

// used for utoprim_jon entropy inversion
// frac-fudged
FTYPE compute_dspecificSdrho_wmrho0_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0)
{
  return(fudgefracsingle_kazfull(DSSDRHOofRHOCHI,CHIDIFF,EOSextra,rho0, wmrho0));
}


// used for utoprim_jon entropy inversion
// frac-fudged
FTYPE compute_dspecificSdwmrho0_wmrho0_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0)
{
  return(fudgefracsingle_kazfull(DSSDCHIofRHOCHI,CHIDIFF, EOSextra, rho0, wmrho0));
}




// 1 / (drho0/dp) holding wmrho0 fixed
// frac-fudged
FTYPE compute_idrho0dp_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0)
{
  return(fudgefracsingle_kazfull(IDRHO0DP,CHIDIFF, EOSextra, rho0, wmrho0));
}



// 1 / (d(u+p)/dp) holding rho0 fixed
// frac-fudged
FTYPE compute_idwmrho0dp_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0)
{
  return(fudgefracsingle_kazfull(IDRHO0DP,CHIDIFF, EOSextra, rho0, wmrho0));
}




// volume heating rate(rho0,u)
FTYPE compute_qdot_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  int whichfun;

  if(whichdatatype[primarytable]==1){
    whichfun=EXTRA1;
  }
  else if(whichdatatype[primarytable]==3){
    whichfun=EXTRA1;
  }
  else{
    dualfprintf(fail_file,"Shouldn't request whichfun0=%d if primarytable=%d\n",QDOTNU,primarytable);
  }

  return(compute_tabulated_kazfull(whichfun, UTOTDIFF, EOSextra, rho0, u));
}


// temperature(rho0,u)
FTYPE compute_temp_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u)
{
  return(compute_tabulated_kazfull(TEMPGEN, UTOTDIFF, EOSextra, rho0, u));
}


// temperature (direct lookup from differential quant2: dquant2)
// whichfun = TEMPU + 0,1,2,3 for TEMPU,TEMPP,TEMPCHI,TEMPS
FTYPE compute_temp_whichd_kazfull(int whichd, FTYPE *EOSextra, FTYPE rho0, FTYPE dquant2)
{
  FTYPE temp;


  if(getsingle_eos_fromtable(TEMPGEN,whichd,EOSextra,rho0,dquant2,&temp)){
    temp=0.0;
  }

  return(temp);



}


// uses global variable "numextras" that should be set before this function is called
// f(rho0,du)
// direct lookup
void compute_allextras_du_kazfull(int justnum, FTYPE *EOSextra, FTYPE rho0, FTYPE du, int *numextrasreturn, FTYPE *extras)
{
  int i;

  if(justnum==0){

    *numextrasreturn=numextras[primarytable]; // by default assume tablulated data exists


    // assume all tables have same number of extras or else this doesn't make sense in general
    for(i=0;i<numextras[primarytable];i++){
      if(getsingle_eos_fromtable(EXTRA1+i,UTOTDIFF,EOSextra,rho0,du,&(extras[i]))){
	extras[i]=0.0;
	*numextrasreturn=0; // tells calling function that no tabulated extras
      }
    }
    // set rest to 0
    for(i=numextras[primarytable];i<MAXNUMEXTRAS;i++){
      extras[i] = 0.0;
    }
  }

}


// uses global variable "numextras" that should be set before this function is called
// f(rho0,u)
void compute_allextras_kazfull(int justnum, FTYPE *EOSextra, FTYPE rho0, FTYPE u, int *numextrasreturn, FTYPE *extras)
{
  void compute_allextras_du_kazfull(int justnum, FTYPE *EOSextra, FTYPE rho0, FTYPE du, int *numextrasreturn, FTYPE *extras);
  int i;
  FTYPE du;

  if(whichdatatype[primarytable]==4){
    du = u - EOSextra[UNUGLOBAL];
  }
  else{
    du = u;
  }

  compute_allextras_du_kazfull(justnum, EOSextra, rho0, du, numextrasreturn, extras);

}










// f2c prototype
#include "f2c.h"
#include "tau_neededbyharm.P"
// not linking with libf2c since don't want that dependence and conversion doesn't need it since the original code was simple

// get neutrino terms (and return extras also)
// here function is (rho0,u) -- never used as function of p or chi
int get_extrasprocessed_kazfull(int doall, FTYPE *EOSextra, FTYPE *pr, FTYPE *extras, FTYPE *processed)
{
  FTYPE quant1,quant2;
  // assume all tables have same number of extras or else this doesn't make sense in general
  FTYPE tempk,kbtk,rho0,u,H,ye,ynu;
  int numextrasreturn,ei;
  void compute_allextras_du_kazfull(int justnum, FTYPE *EOSextra, FTYPE rho0, FTYPE du, int *numextrasreturn, FTYPE *extras);
  FTYPE compute_temp_kazfull(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
  FTYPE unue0,unuebar0,unumu0,
    qtautnueohcm,qtautnuebarohcm,qtautmuohcm,
    qtauanueohcm,qtauanuebarohcm,qtauamuohcm,
    nnue0,nnuebar0,nnumu0,
    ntautnueohcm,ntautnuebarohcm,ntautmuohcm,
    ntauanueohcm,ntauanuebarohcm,ntauamuohcm,
    lambdatot,lambdaintot,
    tauphotonohcm,tauphotonabsohcm,
    nnueth0,nnuebarth0;
  FTYPE qphoton,qm,graddotrhouyl,tthermaltot,tdifftot,rho_nu,p_nu,s_nu,ynulocal,Ynuthermal,enu,enue,enuebar;
  FTYPE qphoton_a[NUMHDIRECTIONS],qm_a[NUMHDIRECTIONS],graddotrhouyl_a[NUMHDIRECTIONS],tthermaltot_a[NUMHDIRECTIONS],tdifftot_a[NUMHDIRECTIONS],rho_nu_a[NUMHDIRECTIONS],p_nu_a[NUMHDIRECTIONS],s_nu_a[NUMHDIRECTIONS],ynulocal_a[NUMHDIRECTIONS],Ynuthermal_a[NUMHDIRECTIONS],enu_a[NUMHDIRECTIONS],enue_a[NUMHDIRECTIONS],enuebar_a[NUMHDIRECTIONS];
  FTYPE dquant2;
  FTYPE qarray[NUMINDEPDIMENS+1];
  int repeatedeos;
  int qi;
  int hi;
  FTYPE frac;
  int notintable;
  int whichd;
  FTYPE testextras[MAXNUMEXTRAS];


 
#if(0)
  // BEGIN DEBUG LOOP
  int loopit,numloops=1000;
  //int loopit,numloops=0;
  FTYPE logynu0;
  for(loopit=0;loopit<=numloops;loopit++){
    //if(doall) EOSextra[YNU0GLOBAL]=1E-15 + (1.0-1E-15)/(1000.0)*loopit;
    if(doall){
      logynu0=-15 + (1.0-(-15))/(1000.0)*loopit;
      logynu0=-7 + (1.0-(-7))/(1000.0)*loopit;
      EOSextra[YNU0GLOBAL]=pow(10.0,logynu0);
    }
    else numloops=0;
#endif


  if((int)EOSextra[IGLOBAL]==0) dualfprintf(fail_file,"doall=%d\n",doall);
  //  else dualfprintf(fail_file,"ijk: %d %d %d\n",(int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
  
  
  if(whichdatatype[primarytable]!=4){
    dualfprintf(fail_file,"Neutrino calculation not setup for other datatypes=%d primarytable=%d\n",whichdatatype[primarytable],primarytable);
    myexit(2954654);
  }

  whichd=UTOTDIFF; // only one allowed for now
  quant1=pr[RHO];
  quant2=pr[UU];

  // first get dquant2 from quant2
  // avoid getting densities effectively twice, so just get UNUGLOBAL directly
  // Get dquant2 = u - u_\nu
  dquant2 = quant2 - EOSextra[UNUGLOBAL];


  // setup array
  qarray[1]=quant1;
  qarray[2]=dquant2;
  // qarray[3+] are always stored in EOSextra
  for(qi=3;qi<=NUMINDEPDIMENS;qi++){
    qarray[qi] = EOSextra[vartypeeosextraarray[qi]];
  }


  if(doallextrasold==doall){
    // check if repeated case (doesn't matter if i,j,k same since result only depends on all qarray values)
    repeatedeos=1;
    for(qi=1;qi<=WHICHEOSDIMEN;qi++){ // only need to repeat used independent variables, not all
      repeatedeos*=(fabs(qarray[qi]-qoldarrayextras[qi])<OLDTOLERANCE);
      if((int)EOSextra[IGLOBAL]==0) dualfprintf(fail_file,"qi=%d %21.15g %21.15g\n",qi,qarray[qi],qoldarrayextras[qi]);
    }

  }
  else{
    repeatedeos=0; // can't repeat if doall changed (i.e. if did doall=0, then doall=1 with repeated, then that info isn't there to repeat!)
    doallextrasold=doall; // store old version
  }

  if((int)EOSextra[IGLOBAL]==0) dualfprintf(fail_file,"repeatedeos=%d doall=%d doallextrasold=%d\n",repeatedeos,doall,doallextrasold);


  if(repeatedeos){
    if(doall){
      // then repeated case, so just return old result
      for(ei=0;ei<MAXNUMEXTRAS;ei++) extras[ei]=extrasold[ei];
      for(ei=0;ei<MAXPROCESSEDEXTRAS;ei++) processed[ei]=processedold[ei];
    }
    else{
      processed[RHONU]=processedold[RHONU];
      processed[PNU]=processedold[PNU];
      processed[SNU]=processedold[SNU];
    }    
  }// end if repeatedeos
  else{// else if not repeated

    // set extra parameters
    rho0=qarray[1];
    u=qarray[2]; // "u" not used
    ye=qarray[3]; // "ye" not used
    ynu=pr[YNU]; // pr[YNU] = Ynu[orig] = Ynu (i.e. not Ynu0 so that conservation law equation for Y_\nu is correctly using radiative transfer version of Y_\nu, while EOSextra[YNU] is Ynu0 used for table lookup
    H=qarray[5];

    // not normal temperature(rho0,u), but temp(rho0,d), so direct lookup as tabulated
    // if used temp(rho0,u) then would get densities and this is already being done here, so avoid extra iteration
    // tempk is always stored in dimensionless form of: T[k] k_b/(m_b c^2), so since need kbtk in energy units, then always multiply tempk by m_b c^2 no matter what rho0unittype is
    tempk=compute_temp_whichd_kazfull(whichd, EOSextra, quant1, dquant2);
    kbtk=tempk*mbcsq; // in code energy units


    if(doall){
      // get extras (function of rho0,du as tabulated)
      compute_allextras_du_kazfull(0, EOSextra, quant1, dquant2, &numextrasreturn, extras);


#if(0)
      // DEBUG:
      ei=-1;
      ei++;  testextras[ei]=2.313057286241873E-009/(1.0/Lunit);
      ei++;  testextras[ei]=2.713713102758627E-011/(1.0/Lunit);
      ei++;  testextras[ei]=1.569556488872701E-008/(1.0/Lunit);
      ei++;  testextras[ei]=5.342111503769025E-014/(1.0/Lunit);
      ei++;  testextras[ei]=2.057814079059060E-009/(1.0/Lunit);
      ei++;  testextras[ei]=1.096473627400667E-014/(1.0/Lunit);
      ei++;  testextras[ei]=1.481348570684876E-009/(1.0/Lunit);
      ei++;  testextras[ei]=1.238443974759171E-011/(1.0/Lunit);
      ei++;  testextras[ei]=1.176101873401969E-008/(1.0/Lunit);
      ei++;  testextras[ei]=3.017955829870171E-014/(1.0/Lunit);
      ei++;  testextras[ei]=1.293887908702487E-009/(1.0/Lunit);
      ei++;  testextras[ei]=5.894535354846674E-015/(1.0/Lunit);
      ei++;  testextras[ei]=1.484470216153997E+025/(energyunit/pow(Lunit,3.0));
      ei++;  testextras[ei]=1.484470216153997E+025/(energyunit/pow(Lunit,3.0));
      ei++;  testextras[ei]=2.968941988288588E+025/(energyunit/pow(Lunit,3.0));
      ei++;  testextras[ei]=4.169196104458902E+030/(1.0/pow(Lunit,3.0));
      ei++;  testextras[ei]=4.169196104458902E+030/(1.0/pow(Lunit,3.0));
      ei++;  testextras[ei]=8.338413581438167E+030/(1.0/pow(Lunit,3.0));
      ei++;  testextras[ei]=664039105.329363/(Lunit);
      ei++;  testextras[ei]=4304671623.94283/(Lunit);
      ei++;  testextras[ei]=3818185478.20807/(1.0/Lunit);
      ei++;  testextras[ei]=3183162641.50992/(1.0/Lunit);
      ei++;  testextras[ei]=1.170948788427859E+032/(1.0/pow(Lunit,3.0));
      ei++;  testextras[ei]=4.111239894021138E+028/(1.0/pow(Lunit,3.0));
      //      for(ei=0;ei<MAXNUMEXTRAS;ei++) extras[ei]=testextras[ei];
#endif


      if(numextrasreturn!=0){
	notintable=0;
	// MAXNUMEXTRAS entries
	// not same order as passed to function: computefinal_fromhcm()
	ei=0;
	qtautnueohcm=extras[ei]; ei++;
	qtauanueohcm=extras[ei]; ei++;
	qtautnuebarohcm=extras[ei]; ei++;
	qtauanuebarohcm=extras[ei]; ei++;
	qtautmuohcm=extras[ei]; ei++;
	qtauamuohcm=extras[ei]; ei++;

	ntautnueohcm=extras[ei]; ei++;
	ntauanueohcm=extras[ei]; ei++;
	ntautnuebarohcm=extras[ei]; ei++;
	ntauanuebarohcm=extras[ei]; ei++;
	ntautmuohcm=extras[ei]; ei++;
	ntauamuohcm=extras[ei]; ei++;

	unue0=extras[ei]; ei++;
	unuebar0=extras[ei]; ei++;
	unumu0=extras[ei]; ei++;

	nnue0=extras[ei]; ei++;
	nnuebar0=extras[ei]; ei++;
	nnumu0=extras[ei]; ei++;

	lambdatot=extras[ei]; ei++; // EXTRA19
	lambdaintot=extras[ei]; ei++; // EXTRA20

	tauphotonohcm=extras[ei]; ei++;
	tauphotonabsohcm=extras[ei]; ei++;

	nnueth0=extras[ei]; ei++;
	nnuebarth0=extras[ei]; ei++;
      }
      else{
	notintable=1;
	// then not in table, so revert to estimated non-tabulated values (i.e. optically thin)
	// extra quantities were assumed set to 0 if not in table, so only need to change non-zero things
	// optical depths and densities are complicated and if out of table then they aren't determined easily
	lambdatot=lambdaintot=BIG; // optically thin
      }

    }// end if doall
    else{// else if not doing all
      notintable=0;
      // get extras (only those needed)
      if(getsingle_eos_fromtable(EXTRA1,UTOTDIFF,EOSextra,quant1,dquant2,&qtautnueohcm)){
	qtautnueohcm=0.0; // not in tables, assume optically thin

	qtauanueohcm=0.0;
	qtautnuebarohcm=0.0;
	qtauanuebarohcm=0.0;
	qtautmuohcm=0.0;
	qtauamuohcm=0.0;
	unue0=0.0;
	unuebar0=0.0;
	unumu0=0.0;
	notintable=1;
      }
      if(!notintable){
	// assume if one is in table, rest are.
	// EXTRA1:  qtautnueohcm
	// EXTRA2:  qtauanueohcm
	// EXTRA3:  qtautnuebarohcm
	// EXTRA4:  qtauanuebarohcm
	// EXTRA5:  qtautmuohcm
	// EXTRA6:  qtauamuohcm
	// EXTRA13:  unue0
	// EXTRA14:  unuebar0
	// EXTRA15:  unumu0
	getsingle_eos_fromtable(EXTRA2,UTOTDIFF,EOSextra,quant1,dquant2,&qtauanueohcm);
	getsingle_eos_fromtable(EXTRA3,UTOTDIFF,EOSextra,quant1,dquant2,&qtautnuebarohcm);
	getsingle_eos_fromtable(EXTRA4,UTOTDIFF,EOSextra,quant1,dquant2,&qtauanuebarohcm);
	getsingle_eos_fromtable(EXTRA5,UTOTDIFF,EOSextra,quant1,dquant2,&qtautmuohcm);
	getsingle_eos_fromtable(EXTRA6,UTOTDIFF,EOSextra,quant1,dquant2,&qtauamuohcm);
	getsingle_eos_fromtable(EXTRA13,UTOTDIFF,EOSextra,quant1,dquant2,&unue0);
	getsingle_eos_fromtable(EXTRA14,UTOTDIFF,EOSextra,quant1,dquant2,&unuebar0);
	getsingle_eos_fromtable(EXTRA15,UTOTDIFF,EOSextra,quant1,dquant2,&unumu0);
      }
      else{// not in table, then non-zero quantities need to be set to something else
	// so far choosing all to be 0.0 is correct for optically thin approximation to off-table quantities
      }
    }




    for(hi=0;hi<NUMHDIRECTIONS;hi++){
      // now process neutrino variables into final form
      H = EOSextra[vartypeheightarray[hi+1]]; // vartypeheightarray[] index starts at 1

      if(doall){

	if(!notintable){
	  // calling f2c generated code, which assumes a certain dimensionless form for inputs
	  // rhob/mb should be a number density, and mb*rate/cc should be in rhounit*rate
	  // this means if rho0 in c^2 units, then mb should be too
	  computefinal_fromhcm__(&Ccode,&mbwithrhounit,&rho0,&kbtk,&H,
				 &unue0,&unuebar0,&unumu0,
				 &qtautnueohcm,&qtautnuebarohcm,&qtautmuohcm,
				 &qtauanueohcm,&qtauanuebarohcm,&qtauamuohcm,
				 &nnue0,&nnuebar0,&nnumu0,
				 &ntautnueohcm,&ntautnuebarohcm,&ntautmuohcm,
				 &ntauanueohcm,&ntauanuebarohcm,&ntauamuohcm,
				 &lambdatot,&lambdaintot,
				 &tauphotonohcm,&tauphotonabsohcm,
				 &nnueth0,&nnuebarth0,
				 // outputs below
				 &qphoton_a[hi],&qm_a[hi],&graddotrhouyl_a[hi],&tthermaltot_a[hi],&tdifftot_a[hi],&rho_nu_a[hi],&p_nu_a[hi],&s_nu_a[hi],&ynulocal_a[hi],&Ynuthermal_a[hi],&enu_a[hi],&enue_a[hi],&enuebar_a[hi]);
	}
	else{
	  // then set to approximate optically thin values (i.e. ~0)
	  qphoton_a[hi]=qm_a[hi]=graddotrhouyl_a[hi]=rho_nu_a[hi]=p_nu_a[hi]=s_nu_a[hi]=ynulocal_a[hi]=Ynuthermal_a[hi]=enu_a[hi]=enue_a[hi]=enuebar_a[hi]=0.0;
	  tthermaltot_a[hi]=tdifftot_a[hi]=BIG; // force non-evolution of Ynu
	}
      }// end if doall
      else{// else if not doall
	// DEBUG:
	//	dualfprintf(fail_file,"%21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g\n",qtautnueohcm,qtauanueohcm,qtautnuebarohcm,qtauanuebarohcm,qtautmuohcm,qtauamuohcm,unue0,unuebar0,unumu0);
	//	dualfprintf(fail_file,"CCODE: :: %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g %21.15g\n",Ccode,mbwithrhounit,rho0,kbtk,H,unue0,unuebar0,unumu0,qtautnueohcm,qtautnuebarohcm,qtautmuohcm,qtauanueohcm,qtauanuebarohcm,qtauamuohcm);
	//	dualfprintf(fail_file,"mb[cgs]=%21.15g T[K] = %21.15g\n",mbwithrhounit*Munit/Vunit/Vunit,kbtk*energyunit/kb);

	if(!notintable){
	  // get rho_nu, p_nu, s_nu from extras
	  computefinal_justdensities_fromhcm__(&Ccode,&mbwithrhounit,
					       &rho0,&kbtk,&H,
					       &unue0,&unuebar0,&unumu0,
					       &qtautnueohcm,&qtautnuebarohcm,&qtautmuohcm,
					       &qtauanueohcm,&qtauanuebarohcm,&qtauamuohcm,
					       // outputs below
					       &rho_nu_a[hi],&p_nu_a[hi],&s_nu_a[hi]); // notice no & since already pointer
	}
	else{
	  // not in table
	  rho_nu_a[hi]=p_nu_a[hi]=s_nu_a[hi]=0.0;
	}
      }
    }
    

    // now get appropriate sum
    // if all h's same then get normal result, otherwise optical depth supression occurs per direction
    // hack for real method
    // GODMARK: a heating method might want access to these different directional quantities
    qphoton=qm=graddotrhouyl=tthermaltot=tdifftot=rho_nu=p_nu=s_nu=ynulocal=Ynuthermal=enu=enue=enuebar=0.0;
    frac=1.0/((FTYPE)NUMHDIRECTIONS);
    for(hi=0;hi<NUMHDIRECTIONS;hi++){

      if(doall){
      	qphoton += qphoton_a[hi]*frac;
	qm += qm_a[hi]*frac;
	graddotrhouyl += graddotrhouyl_a[hi]*frac;
	tthermaltot += tthermaltot_a[hi]*frac;
	tdifftot += tdifftot_a[hi]*frac;
	ynulocal += ynulocal_a[hi]*frac;
	Ynuthermal += Ynuthermal_a[hi]*frac;
	enu += enu_a[hi]*frac;
	enue += enue_a[hi]*frac;
	enuebar += enuebar_a[hi]*frac;
      }
      
      rho_nu += rho_nu_a[hi]*frac;
      p_nu += p_nu_a[hi]*frac;
      s_nu += s_nu_a[hi]*frac;

    }

  
    if(doall){
      // MAXPROCESSEDEXTRAS entries
      ei=-1; // just so easy copy/paste form
      ei++; processed[ei]=qphoton;
      ei++; processed[ei]=qm;
      ei++; processed[ei]=graddotrhouyl;
      ei++; processed[ei]=tthermaltot;
      ei++; processed[ei]=tdifftot;
      ei++; processed[ei]=rho_nu;
      ei++; processed[ei]=p_nu;
      ei++; processed[ei]=s_nu;
      ei++; processed[ei]=ynulocal;
      ei++; processed[ei]=Ynuthermal;
      // below are energies of *escaping* neutrinos
      ei++; processed[ei]=enu;
      ei++; processed[ei]=enue;
      ei++; processed[ei]=enuebar;
    }
    else{
      processed[RHONU]=rho_nu;
      processed[PNU]=p_nu;
      processed[SNU]=s_nu;
    }

    //  MAXPROCESSEDEXTRAS entries
    // below not used right now


    // setup old values
    for(qi=1;qi<=NUMINDEPDIMENS;qi++) qoldarrayextras[qi]=qarray[qi];
    if(doall){
      for(ei=0;ei<MAXNUMEXTRAS;ei++) extrasold[ei]=extras[ei];
      for(ei=0;ei<MAXPROCESSEDEXTRAS;ei++) processedold[ei]=processed[ei];
    }
    else{
      processedold[RHONU]=processed[RHONU];
      processedold[PNU]=processed[PNU];
      processedold[SNU]=processed[SNU];
    }


  }// end if not repeated lookup

#if(0)
  // qarray[3+] are always stored in EOSextra
  for(qi=1;qi<=NUMINDEPDIMENS;qi++){
    dualfprintf(fail_file,"qi=%d qarray[qi] =%21.15g\n",qi,qarray[qi]);
  }

  if(doall) dualfprintf(fail_file,"TOPLOT %21.15g %21.15g\n",EOSextra[YNU0GLOBAL],processed[YNULOCAL]);
  
  if(doall){
    for(ei=0;ei<MAXNUMEXTRAS;ei++) dualfprintf(fail_file,"extras[%d]=%21.15g %21.15g\n",ei,extras[ei],testextras[ei]);
    for(ei=0;ei<MAXPROCESSEDEXTRAS;ei++) dualfprintf(fail_file,"processed[%d]=%21.15g\n",ei,processed[ei]);
  }

  }// end DEBUG LOOP
  
  myexit(0);
#endif


  return(0);


}




static int iterateupsnu(FTYPE *processed,FTYPE *pr,FTYPE *EOSextra);
static int iterateynu0(FTYPE *processed,FTYPE *pr,FTYPE *EOSextra);
static int compute_and_iterate_upsnu(int whichd, FTYPE *EOSextra, FTYPE *pr);
static int compute_and_iterate_ynu0_upsnu(int whichd, FTYPE *EOSextra, FTYPE *pr);




// assumes this is computed every timestep (or substep) or at least on some timescale that H changes
static void compute_upsnu_global(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{
  FTYPE rho0,u;
  FTYPE rho_nu, p_nu, s_nu;
  int i,j,k;



  if(whichdatatype[primarytable]!=4) return; // doesn't need to be done if !=4

  COMPFULLLOOP{
    compute_and_iterate_upsnu(UTOTDIFF, MAC(EOSextra,i,j,k), MAC(prim,i,j,k));
  }

}

// assumes this is computed every timestep (or substep) or at least on some timescale that H changes
static void compute_ynu0_upsnu_global(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{
  FTYPE rho0,u;
  FTYPE rho_nu, p_nu, s_nu;
  int i,j,k;



  if(whichdatatype[primarytable]!=4) return; // doesn't need to be done if !=4

  COMPFULLLOOP{
    compute_and_iterate_ynu0_upsnu(UTOTDIFF, MAC(EOSextra,i,j,k), MAC(prim,i,j,k));
  }

}





// whichd = UTOTDIFF,PTOTDIFF,CHIDIFF
// get neutrino rho_nu, p_nu, s_nu
// function is (rho0,u/p/chi)
// for now assume only needed as function of quant2=u.  Assumes this function called infrequently outside iterative routines
// Point is that anyways we are doing an approximation, and save memory and gain simplicity by avoiding tabulating other f(rho0,chi) quantities
static int compute_and_iterate_upsnu(int whichd, FTYPE *EOSextra, FTYPE *pr)
{
  int get_extrasprocessed_kazfull(int doall, FTYPE *EOSextra, FTYPE *pr, FTYPE *extras, FTYPE *processed);
  int toreturn;
  FTYPE extras[MAXNUMEXTRAS];
  FTYPE processed[MAXPROCESSEDEXTRAS];


  // assume WHICHD==UTOTDIFF only
  toreturn=get_extrasprocessed_kazfull(0, EOSextra, pr, extras, processed);

  // iterate u_nu, p_nu, and s_nu
  iterateupsnu(processed,pr,EOSextra);

  return(toreturn);
}


// call this if not calling sources so can iterate Ynu0
static int compute_and_iterate_ynu0_upsnu(int whichd, FTYPE *EOSextra, FTYPE *pr)
{
  //  int whichd=UTOTDIFF;
  int get_extrasprocessed_kazfull(int doall, FTYPE *EOSextra, FTYPE *pr, FTYPE *extras, FTYPE *processed);
  int toreturn;
  FTYPE extras[MAXNUMEXTRAS];
  FTYPE processed[MAXPROCESSEDEXTRAS];

  // assume WHICHD==UTOTDIFF only
  // 1 below means doall extras and processed
  toreturn=get_extrasprocessed_kazfull(1, EOSextra, pr, extras, processed);

  // iterate u_nu, p_nu, and s_nu
  iterateupsnu(processed,pr,EOSextra);

  // iterate Ynu0
  iterateynu0(processed,pr,EOSextra);

  return(toreturn);

}




// iterate u_nu, p_nu, and s_nu
static int iterateupsnu(FTYPE *processed,FTYPE *pr,FTYPE *EOSextra)
{

  /////////////////
  //
  // Iterate rho_nu, p_nu, and s_nu for table lookup
  //
  // assume DO WANT to update EOSextraglobal[UNUGLOBAL,PNUGLOBAL,SNUGLOBAL][i][j][k] since input is actual value on grid
  EOSextra[UNUGLOBAL] = processed[RHONU];
  EOSextra[PNUGLOBAL] = processed[PNU];
  EOSextra[SNUGLOBAL] = processed[SNU];

  return(0);

}


// iterate Ynu0
// only can be done if doall was used when creating processed[], since otherwise processed[YNULOCAL] has not been recalled or computed
static int iterateynu0(FTYPE *processed,FTYPE *pr,FTYPE *EOSextra)
{
  FTYPE damp;
  FTYPE ynu0old=EOSextra[YNU0GLOBAL];
  FTYPE ynu0older=EOSextra[YNU0OLDGLOBAL];
  FTYPE ynuold=processed[YNULOCAL];
  FTYPE ynuolder=EOSextra[YNUOLDGLOBAL];
  FTYPE odRdYnu0,R,errR;



  /////////////////
  //
  // Iterate Ynu0 for table lookup
  //
    
  // Take fake-simple Newton step (assumes monotonically increasing function and that Ynu0~Ynu)
  // Will use below to compute Ynu0[new] = Ynu[new][Ynu0,rho,du,Ye,H] + dYnu, where dYnu = -Ynu[old][Ynu0,rho,du,Ye,H] + Ynu0[old]
  // Recall that ynulocal = Ynu after table lookup (i.e. not Ynu0, but radiative transfer version of Ynu just computed)
  // Recall that ynu=pr[YNU] that is updated (i.e. new) Ynu from last evolution step of Ynu.
  // Recall that EOSextra[YNU0GLOBAL] = Ynu0 used for table lookup
  // Recall that ynulocal is estimate of Ynu using last Ynu0 but current rho,u,Ye.  So it corresponds to OLD value of Ynu
  // EOSextra[YNU0GLOBAL] = EOSextra[YNU0GLOBAL] + (ynu - ynulocal);    
  // EOSextra[YNU0GLOBAL] += (pr[ynu] - processed[YNULOCAL]);


  // TAKE NEWTON STEP:
  // In general, Newton's method is:
  // Resid = R = (-Ynu[new] + Ynu[Ynu0old])
  // damp = starts at 1.0 and can decrease to avoid jumping too far (e.g. out of table)
  // dYnu0 = -damp*R/(dR/dYnu0)
  // But dR/dYnu0 = dYnu[Ynu0old]/dYnu0 \approx (Ynu[Ynu0old] - Ynu[Ynu0older])/(Ynu0old-Ynu0older)
  // So dYnu0 = damp*(Ynu[new] - Ynu[Ynu0old])*(Ynu0old-Ynu0older)/(Ynu[Ynu0old] - Ynu[Ynu0older])
  // So need to store Ynu0older and Ynu[Ynu0older].  Have at first Ynu0old and computed above Ynu[Ynu0old]
  // Then Ynu0new = Ynu0old + dYnu0



  
  //    damp=1.0;
  damp=0.20;
  R = (-pr[YNU] + ynuold);
  errR = fabs(R/(fabs(pr[YNU])+fabs(ynuold)+SMALL));

  if((int)EOSextra[IGLOBAL]==0) dualfprintf(fail_file,"MARK: steppart=%d nstep=%ld R=%21.15g errR=%21.15g\n",steppart,nstep,R,errR);

  if(errR<0.05){
    // don't try to do better if already accurate to less than 5%
      
    if((int)EOSextra[IGLOBAL]==0) dualfprintf(fail_file,"MARK: ynu0old=%21.15g ynu0older=%21.15g : ynuold=%21.15g ynuolder=%21.15g : final=%21.15g\n",ynu0old,ynu0older,ynuold,ynuolder,EOSextra[YNU0GLOBAL]);

  }
  else{// then try to get better Ynu0

    if(ynuold==ynuolder || ynu0old==ynu0older){
      // If just first iteration, so no older information, so just use small slope to change so next iteration can get slope
      // If reached here after first iteration, then don't change Ynu0 much.  Just push so can get slope in case evolves
      // ok that I push it in same sense of Ynu-Ynulocal since just need a push to somewhere else

      // odRdYnu0 = 1/ (dR/dYnu0)
      odRdYnu0=0.1*errR;
    }
    else{
      // standard derivative
      // odRdYnu0 = 1/ (dR/dYnu0)
      odRdYnu0=(ynu0old-ynu0older)/(ynuold-ynuolder);
    }

    // iterate
    EOSextra[YNU0GLOBAL] += -damp*R*odRdYnu0;

    if((int)EOSextra[IGLOBAL]==0) dualfprintf(fail_file,"MARK: odRdYnu0=%21.15g ynu0old=%21.15g ynu0older=%21.15g : ynuold=%21.15g ynuolder=%21.15g : finalYnu0=%21.15g\n",odRdYnu0,ynu0old,ynu0older,ynuold,ynuolder,EOSextra[YNU0GLOBAL]);
      
    // need to limit Ynu0 to table (assumes reasonable behavior for above iteration!)
    if(EOSextra[YNU0GLOBAL]>0.999*lineartablelimits[primarytable][YNUEOS][1]) EOSextra[YNU0GLOBAL] = 0.999*lineartablelimits[primarytable][YNUEOS][1];
    if(EOSextra[YNU0GLOBAL]<1.001*lineartablelimits[primarytable][YNUEOS][0]) EOSextra[YNU0GLOBAL] = 1.001*lineartablelimits[primarytable][YNUEOS][0];
  }// end else if error large enough to need correction


  // store old value into older values
  EOSextra[YNU0OLDGLOBAL] = ynu0old;
  EOSextra[YNUOLDGLOBAL] = ynuold;


  return(0);
}






// compute sources to equations of motion for HARM
// duplicate of compute_neutrino() in sources.c
// Ui and dUother in UNOTHING form
// Calling this iterates Ynu0 when doing whichdatatype==4 (compute_EOS_parms_kazfull() also iterates Ynu0)
int compute_sources_EOS_kazfull(FTYPE *EOSextra, FTYPE *pr, struct of_geom *geom, struct of_state *q, FTYPE *Ui, FTYPE *dUother, FTYPE(*dUcomp)[NPR])
{
  int ii,jj,kk,pp;
  int j;
  FTYPE X[NDIM],V[NDIM],r,th,R;
  FTYPE du;
  FTYPE rho,u,yl,ynu;
  FTYPE cofactor;
  FTYPE dUdtau;
  FTYPE rph,photoncapture;
  FTYPE extras[MAXNUMEXTRAS];
  void compute_allextras_kazfull(int justnum, FTYPE *EOSextra, FTYPE rho0, FTYPE u, int *numextrasreturn, FTYPE *extras);
  int get_extrasprocessed_kazfull(int doall, FTYPE *EOSextra, FTYPE *pr, FTYPE *extras, FTYPE *processed);
  FTYPE processed[MAXPROCESSEDEXTRAS];
  int ei;
  FTYPE qphoton,qm,graddotrhouyl,tthermaltot,tdifftot,rho_nu,p_nu,s_nu,ynulocal,enu,enue,enuebar;
  FTYPE Ynuthermal,lambdatot,lambdaintot;
  FTYPE graddotrhouynu;
  FTYPE dtau,dtlimit;
  FTYPE gradUU[NDIM];
  FTYPE Unewlocal[NPR];
  FTYPE Uylupdate;
  FTYPE Urhoupdate;
  FTYPE graddotrhouynulimit;
  int numextrasreturn;




  ii=geom->i;
  jj=geom->j;
  kk=geom->k;
  pp=geom->p;

  coord_ijk(ii,jj,kk,pp,X) ;
  bl_coord_ijk(ii,jj,kk,pp,V) ;

  r=V[1];
  th=V[2];
  R = r*sin(th) ;
  rho = pr[RHO];
  u = pr[UU];
#if(DOYNU!=DONOYNU)
  ynu = pr[YNU];
#else
  ynu = 0.0;
#endif
#if(DOYL!=DONOYL)
  yl = pr[YL];
#else
  yl = 0.0;
#endif


  // approximately account for photon capture by black hole
  // 50% of masslesss particles from **stationary** isotropic emitters are captured by black hole
  // See Shapiro & Teukolsky (1983) and equation 2.81 and 2.82 in Shibata, Sekiguchi, Takahashi (2007)
  // Note that if MBH=0 then rph=0, so works for NS

  // For now, treat capture process as suppression of cooling/heating rates or any changes in fluid
  // In this way, capture by BH is treated as assuming fluid advected them in since assume if trapping photons then trapping fluid for sure.
  // In reality have to follow those photons/neutrinos to check if this is true, but good assumption.
  rph = 2.0*MBH*(1.0 + cos(2.0/3.0*(acos(-a/MBH))));
  photoncapture = (r>rph) ? 1.0 : 0.0;


  if(whichdatatype[primarytable]==4){
    // get neutrino and photon quantities
    get_extrasprocessed_kazfull(1, EOSextra, pr, extras, processed); // (rho0,u)


    // MAXPROCESSEDEXTRAS entries
    qphoton=processed[QPHOTON];
    qm = processed[QNEUTRINO]; 
    graddotrhouyl = processed[GRADDOTRHOUYL]; 
    tthermaltot = processed[TTHERMAL]; 
    tdifftot = processed[TDIFF]; 
    rho_nu = processed[RHONU]; 
    p_nu = processed[PNU]; 
    s_nu = processed[SNU]; 
    ynulocal = processed[YNULOCAL];  // don't actually need this
    Ynuthermal = processed[YNUTHERMAL];
    // below are energies of *escaping* neutrinos
    enu = processed[ENUAVG]; 
    enue = processed[ENUE]; 
    enuebar = processed[ENUEBAR]; 

    // also store below additional as directly wanted quantities from extras
    lambdatot = extras[EXTRA19-EXTRA1]; // extras[0] is start
    lambdaintot = extras[EXTRA20-EXTRA1];
  }
  else if(whichdatatype[primarytable]==3){

    compute_allextras_kazfull(0, EOSextra, rho, u, &numextrasreturn, extras);

    qphoton = extras[EXTRA1-EXTRA1];
    qm = extras[EXTRA2-EXTRA1];
    graddotrhouyl = extras[EXTRA3-EXTRA1];
    tthermaltot = extras[EXTRA4-EXTRA1];
    tdifftot = extras[EXTRA5-EXTRA1];
    lambdatot = extras[EXTRA6-EXTRA1];
    lambdaintot = extras[EXTRA7-EXTRA1];
    enu = extras[EXTRA8-EXTRA1];
    enue = extras[EXTRA9-EXTRA1];
    enuebar = extras[EXTRA10-EXTRA1];
    Ynuthermal = extras[EXTRA11-EXTRA1];
  }


  // compute some other things
  //  dtau = dt/(q->ucon[TT]);

  /////////////
  //
  // Neutrino thermalization
  // Neutrino thermalization source term (due to neutrinos thermalizing fluid)
  //
  /////////////
  // regardless of the proper Lorentz transformation, this is the right dtau to use to limit ynu to Ynuthermal in the fluid frame
  // well, not really quite true due to advection terms
  // since U[RHO] will get updated, no point in trying to strictly limit using present U[RHO] due to advection terms
  // limit graddotrhouynu so that ynu can't go beyond Ynuthermal (haven't yet accounted for flux, so is not quite right constraint)
  // GODMARK: could add constraints later once flux known -- new function
  //  graddotrhouynu=rho*(Ynuthermal-ynu)/MAX(dtau,tthermaltot);
  //  if(rho*ucon[TT]*ynu+graddotrhouynu*dt>rho*ucon[TT]*Ynuthermal){
  //    graddotrhouynu = rho*(Ynuthermal - ynu)/dtau;
  //  }

  // equation is \nablda_\mu (\rho u^\mu Y_\nu) = \rho dY_\nu/d\tau = \rho (Y_{\nu,thermal} - Y_\nu)/\tau_{thermal} = -graddotrhouynu
  //  graddotrhouynu=-rho*(Ynuthermal-ynu)/MAX(dtau,tthermaltot);

  Urhoupdate = Ui[RHO] + (dUother[RHO])*dt; // assumes no physics source here for RHO (should add it if there is)
  // then based upon this criterion that doesn't necessarily imply exact dtau, strictly limit
  // Y_nu[new] = Unew[Ynu]/Unew[RHO] = (Ui[Ynu] + dUother[Ynu]*dt + dUsource[Ynu]*dt)/Unew[RHO] and solved for dUsource[Ynu]
  // this guarantees that post-advection an overshoot is truncated and leads to Ynu=Ynuthermal
  graddotrhouynulimit = (Ynuthermal*Urhoupdate - Ui[YNU])/dt - dUother[YNU];
  
  // dtlimit is chosen so the expression for graddotrhouynu is consistent
  // that is graddotrhoynu(original) = graddotrhouynulimit and solve for dtlimit
  dtlimit = -rho*(Ynuthermal-ynu)/graddotrhouynulimit;
  // GODMARK: should compare dtlimit with estimate of dtau -- for example can dtlimit<0?
  graddotrhouynu = -rho*(Ynuthermal-ynu)/MAX(dtlimit,tthermaltot);


  /////////////
  //
  // Lepton losses
  // Lepton source term (due to neutrinos escaping fluid)
  //
  // equation is:
  //
  // \nablda_\mu (\rho u^\mu Y_l) = \rho dY_l/d\tau = graddotrhouyl
  //
  // Equation is Unew = Ui + dUother*dt + gradU*dt
  //
  /////////////
  // predicted update
  Uylupdate = Ui[YL] + (dUother[YL] + graddotrhouyl)*dt;
  // limit update so Uylupdate<=0
  // limit Y_l update (Am I sure Y_l can't go negative? GODMARK)
  // here limit is perfect since any change in U[RHO] doesn't matter since U[YL]=0 -> Y_l=0 for all U[RHO]
  if(Uylupdate<0.0){
    graddotrhouyl = -Ui[YL]/dt - dUother[YL]; // so yl=0 after full Riemann + geometry + this source update
  }



  /////////////
  //
  // Lepton+Photon Cooling
  //
  /////////////
  // No obvious limit on gradUU since energy can be negative in ergosphere? and momentum and be + or - . GODMARK
  DLOOPA(j){
    gradUU[j]=-(qphoton+qm)*(q->ucov[j]);
  }





  /////////////////////////////
  //
  // now apply source terms
  //
  ////////////////////////////
  DLOOPA(j){
    // provide energy density per second cooled
    // source term has: \detg dU/d\tau u_\mu
    // dUcomp[][j+1] because j=0 implies UU as required (i.e. dUcomp in NPR form, not NDIM form)
    if(t > 0.){
      dUcomp[RADSOURCE][j+1]=photoncapture*gradUU[j]; // photons+neutrino energy lost in fluid frame.  Some may be captured by black hole, rest reaches infinity
    }
    else{
      dUcomp[RADSOURCE][j+1]=0.0;
    }
  }

  // GODMARK: need a way to track photons that reach infinity vs. black hole
  //*photoncapture


#if(DOYL!=DONOYL)
  j=YL; // accessing NPR-type quantity, so acces dUcomp[][j]
  if(t > 0.){
    dUcomp[RADSOURCE][j]=photoncapture*graddotrhouyl; // GODMARK: check sign of graddotrhouyl
  }
  else{
    dUcomp[RADSOURCE][j]=0;
  }
#endif

#if(DOYNU!=DONOYNU)
  j=YNU; // accessing NPR-type quantity, so acces dUcomp[][j]
  if(t > 0.){
    dUcomp[RADSOURCE][j]=photoncapture*graddotrhouynu; // GODMARK: check sign
  }
  else{
    dUcomp[RADSOURCE][j]=0;
  }
#endif
  


  return(0) ;




}















void store_EOS_parms_kazfull(int numparms, FTYPE *EOSextra, FTYPE *parlist)
{
  FTYPE TDYNORYEtouse;
  FTYPE YNUtouse,YNUOLDtouse;
  FTYPE Htouse[NUMHDIRECTIONS];
  FTYPE unu,pnu,snu;
  int hi;


  if(numparms!=NUMEOSGLOBALS){
    dualfprintf(fail_file,"numparms=%d NUMEOSGLOBALS=%d\n",numparms,NUMEOSGLOBALS);
    myexit(917692);
  }

  // must be right order when this function is used

  // YE assign
  TDYNORYEtouse=parlist[TDYNORYEGLOBAL-FIRSTEOSGLOBAL];

  // YNU0 assign
  YNUtouse=parlist[YNU0GLOBAL-FIRSTEOSGLOBAL];
  YNUOLDtouse=parlist[YNU0OLDGLOBAL-FIRSTEOSGLOBAL];
  
  // H assign
  for(hi=0;hi<NUMHDIRECTIONS;hi++){
    Htouse[hi]=parlist[HGLOBAL-FIRSTEOSGLOBAL+hi]; // 5-3+hi and so for hi=0 gives 2 as required
  }
 
  // YE constrain
  if(TDYNORYEtouse>lineartablelimits[primarytable][YEEOS][1]) TDYNORYEtouse = lineartablelimits[primarytable][YEEOS][1];
  if(TDYNORYEtouse<lineartablelimits[primarytable][YEEOS][0]) TDYNORYEtouse = lineartablelimits[primarytable][YEEOS][0];

  // YNU0 (whichdatatype==4) or YNU (others) constrain
  if(YNUtouse>lineartablelimits[primarytable][YNUEOS][1]) YNUtouse = lineartablelimits[primarytable][YNUEOS][1];
  if(YNUtouse<lineartablelimits[primarytable][YNUEOS][0]) YNUtouse = lineartablelimits[primarytable][YNUEOS][0];
  if(YNUOLDtouse>lineartablelimits[primarytable][YNUEOS][1]) YNUOLDtouse = lineartablelimits[primarytable][YNUEOS][1];
  if(YNUOLDtouse<lineartablelimits[primarytable][YNUEOS][0]) YNUOLDtouse = lineartablelimits[primarytable][YNUEOS][0];

  // H constrain
  if(whichdatatype[primarytable]!=4){
    for(hi=0;hi<NUMHDIRECTIONS;hi++){
      if(Htouse[hi]>lineartablelimits[primarytable][HEOS][1]) Htouse[hi] = lineartablelimits[primarytable][HEOS][1];
      if(Htouse[hi]<lineartablelimits[primarytable][HEOS][0]) Htouse[hi] = lineartablelimits[primarytable][HEOS][0];
    }
  }

  // YE set  
  EOSextra[TDYNORYEGLOBAL] = TDYNORYEtouse;

  if((int)EOSextra[IGLOBAL]==-100){
    // then not yet iterating on Ynu0, so start iteration

    // YNU0 set
    EOSextra[YNU0GLOBAL] = YNUtouse;
    EOSextra[YNU0OLDGLOBAL] = YNUOLDtouse;
    // YNUold set/assign
    EOSextra[YNUOLDGLOBAL]  = parlist[YNUOLDGLOBAL-FIRSTEOSGLOBAL]; // no particular limit on this quantity
  }

  // H set
  for(hi=0;hi<NUMHDIRECTIONS;hi++){
    EOSextra[vartypeheightarray[hi+1]]=Htouse[hi];
  }

  // Unu,Pnu,Snu assign/set
  EOSextra[UNUGLOBAL]=parlist[UNUGLOBAL-FIRSTEOSGLOBAL];
  EOSextra[PNUGLOBAL]=parlist[PNUGLOBAL-FIRSTEOSGLOBAL];
  EOSextra[SNUGLOBAL]=parlist[SNUGLOBAL-FIRSTEOSGLOBAL];

  // IJKGLOBAL assign/set
  EOSextra[IGLOBAL]=parlist[IGLOBAL-FIRSTEOSGLOBAL];
  EOSextra[JGLOBAL]=parlist[JGLOBAL-FIRSTEOSGLOBAL];
  EOSextra[KGLOBAL]=parlist[KGLOBAL-FIRSTEOSGLOBAL];


}

void get_EOS_parms_kazfull(int*numparms, FTYPE *EOSextra, FTYPE *parlist)
{
  int hi;

  // expected use of output should be know this ordering
  // resulting array parlist[] should start at index 0 and have "*numparms" elements
  parlist[TDYNORYEGLOBAL-FIRSTEOSGLOBAL]=EOSextra[TDYNORYEGLOBAL];

  parlist[YNU0GLOBAL-FIRSTEOSGLOBAL]=EOSextra[YNU0GLOBAL];
  parlist[YNU0OLDGLOBAL-FIRSTEOSGLOBAL]=EOSextra[YNU0OLDGLOBAL];
  parlist[YNUOLDGLOBAL-FIRSTEOSGLOBAL]=EOSextra[YNUOLDGLOBAL];

  for(hi=0;hi<NUMHDIRECTIONS;hi++){
    parlist[HGLOBAL-FIRSTEOSGLOBAL+hi]=EOSextra[vartypeheightarray[hi+1]];
  }
  parlist[UNUGLOBAL-FIRSTEOSGLOBAL]=EOSextra[UNUGLOBAL];
  parlist[PNUGLOBAL-FIRSTEOSGLOBAL]=EOSextra[PNUGLOBAL];
  parlist[SNUGLOBAL-FIRSTEOSGLOBAL]=EOSextra[SNUGLOBAL];

  parlist[IGLOBAL-FIRSTEOSGLOBAL]=EOSextra[IGLOBAL];
  parlist[JGLOBAL-FIRSTEOSGLOBAL]=EOSextra[JGLOBAL];
  parlist[KGLOBAL-FIRSTEOSGLOBAL]=EOSextra[KGLOBAL];

  *numparms=MAXPARLIST; // Only Ye -> ???

}


static void compute_IJKglobal(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS]);
static void compute_Hglobal(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
static void compute_TDYNORYE_YNU_global(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
static void compute_upsnu_global(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);


// compute things beyond simple EOS independent variables
void compute_EOS_parms_kazfull(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{

  ////////////////////////
  //
  // set grid indices
  //
  ////////////////////////
  compute_IJKglobal(EOSextra);

  /////////////////////////
  //  
  // compute H
  //
  /////////////////////////
  compute_Hglobal(EOSextra,prim);

  /////////////////////////
  //  
  // compute Tdyn or Ye depending upon whichrnpmethod
  //  
  /////////////////////////
  compute_TDYNORYE_YNU_global(EOSextra,prim);

  /////////////////////////
  //  
  // 1) Iterate Ynu0 if whichdatatype==4
  // 2) Compute neutriono u_\nu, p_\nu, and s_\nu
  //  
  /////////////////////////
  compute_ynu0_upsnu_global(EOSextra,prim);


}

// compute things beyond simple EOS independent variables
// "full" really means at t=0 for now, so do multiple times.
void compute_EOS_parms_full_kazfull(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{

  ////////////////////
  //
  // do normal update 10 times so converged solution at t=0
  //
  ////////////////////
  int i;
  for(i=0;i<10;i++){
    compute_EOS_parms_kazfull(EOSextra,prim);
  }


}


static void compute_IJKglobal(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS])
{
  int i,j,k;
  COMPFULLLOOP{
    MACP0A1(EOSextra,i,j,k,IGLOBAL) = i;
    MACP0A1(EOSextra,i,j,k,JGLOBAL) = j;
    MACP0A1(EOSextra,i,j,k,KGLOBAL) = k;
  }
}


static void compute_Hglobal_new(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{

  // 1) Take EOSextra, rho0, u from full overall-CPUs grid to a single smaller grid
  // 2) Ray-trace out from emission zones to determine their optical depth : each CPU gets some fraction of rays
  // 3) Take final per-CPU rays' resulting \tau and feed back to all CPUs.
  // 4) Interpolate back \tau onto hydro grid.


  // get lambdatot
  //get_lambdatot(MAC(EOSextra,i,j,k),rho0,u,&lambdatot);



}




// assumes this is computed every timestep (or substep) or at least on some timescale that H changes
// EOSextra is not input here -- rather part of output.
static void compute_Hglobal(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{
  int i,j,k;
  int ii,jj,kk;
  int pl,pliter;
  FTYPE r,th,phi;
  FTYPE X[NDIM],V[NDIM],dxdxp[NDIM][NDIM];
  FTYPE dr,rdth,rsinthdphi,lambdatot;
  FTYPE rho0,u;
  FTYPE Htest1,Htest2,Htest3,Htest4;
  int lambdatotextra;


  //  return; //assume initial H is good -- use to compare input and output for EOS

  // this calculation makes sense only if using spherical polar coordinates
  if(!ISSPCMCOORD(MCOORD)){
    dualfprintf(fail_file,"Hglobal not setup for anything except spherical polar coordinates: %d\n",ISSPCMCOORD(MCOORD));
    myexit(2673);
  }


  // first store lookups and computations per point
  COMPFULLLOOP{

    //////////////////////////
    // get geometry stuff
    coord_ijk(i,j,k,CENT,X); // doesn't matter if CENT or anyother thing is used since just an estimate
    bl_coord_ijk(i,j,k,CENT,V);
    dxdxprim_ijk(i,j,k,CENT,dxdxp);
    r=V[1];
    th=V[2];
    phi=V[3];
    // assumes original coordinates are r,\theta,\phi
    dr = MAX(fabs(dxdxp[1][1]*dx[1]),fabs(dxdxp[1][2]*dx[2])); // just an estimate -- assume dxdxp[1][1]~1
    rdth = r*MAX(fabs(dxdxp[2][1]*dx[1]),fabs(dxdxp[2][2]*dx[2])); // just an estimate -- assume dxdxp[2][2]~1
#if(0)
    rsinthdphi = r*sin(th)*fabs(dxdxp[3][3]*dx[3]); // just an estimate -- assume dxdxp[3][3]~1
#endif

    rho0=MACP0A1(prim,i,j,k,RHO);
    u=MACP0A1(prim,i,j,k,UU);


    // get lambdatot
    get_lambdatot(MAC(EOSextra,i,j,k),rho0,u,&lambdatot);


    // get d\tau/dL
    GLOBALMACP0A1(ptemparray,i,j,k,0) = lambdatot;
    GLOBALMACP0A1(ptemparray,i,j,k,1) = dr/(lambdatot+SMALL); // dtau for radial integral
    GLOBALMACP0A1(ptemparray,i,j,k,2) = rdth/(lambdatot+SMALL); // dtau for angular integral
#if(0)
    GLOBALMACP0A1(ptemparray,i,j,k,3) = rsinthddphi/(lambdatot+SMALL); // dtau for radial integral
#endif

    
  }





  // GODMARK : NOT YET FOR MPI
  COMPFULLLOOP{ 


    //////////////////////////////
    //
    // get +- r direction H
    Htest1 = Htest2 = 0.0;
    jj=j;
    kk=k;
    for(ii=i;ii<N1+N1BND;ii++){// outward pointing integral (in MPI, should include other integrals that go to r=infinity)
      Htest1 +=GLOBALMACP0A1(ptemparray,ii,jj,kk,1);
    }
#if(0)
    // convergence of neutrinos to r=0 makes no sense, so only use outgoing rays as estimate
    Htest2 = GLOBALMACP0A1(ptemparray,0,jj,kk,1); // double count ii=0
    for(ii=0;ii<i;ii++){
      Htest2 +=GLOBALMACP0A1(ptemparray,ii,jj,kk,1);
    } 
    MACP0A1(EOSextra,i,j,k,H2GLOBAL) = (Htest1+Htest2*2.0)*GLOBALMACP0A1(ptemparray,i,j,k,0);
#endif
    // This is scale-height used by Kaz's EOS
    MACP0A1(EOSextra,i,j,k,HGLOBAL)  = Htest1*GLOBALMACP0A1(ptemparray,i,j,k,0);
    MACP0A1(EOSextra,i,j,k,H2GLOBAL) = Htest1*GLOBALMACP0A1(ptemparray,i,j,k,0);



    //////////////////////////////
    //
    // get +- \theta direction H (assume spherical polar)
    // GODMARK : NOT YET FOR MPI
    Htest3 = Htest4 = 0.0;
    ii=i;
    kk=k;
    for(jj=j;jj<=N2-1;jj++){
      Htest3 +=GLOBALMACP0A1(ptemparray,ii,jj,kk,2);
    }
    for(jj=j;jj>=0;jj--){
      Htest4 +=GLOBALMACP0A1(ptemparray,ii,jj,kk,2);
    }
    // This is scale-height used by Kaz's EOS
    // add angular part to hack photon trajectory for +-z for disk near BH or NS
    MACP0A1(EOSextra,i,j,k,H3GLOBAL) = (Htest1 + Htest3)*GLOBALMACP0A1(ptemparray,i,j,k,0);;

    MACP0A1(EOSextra,i,j,k,H4GLOBAL) = (Htest1 + Htest4)*GLOBALMACP0A1(ptemparray,i,j,k,0);;

    //    MACP0A1(EOSextra,i,j,k,H3GLOBAL) = (Htest3 + SHIFT2*MACP0A1(EOSextra,i,SHIFT2,k,HGLOBAL)/(GLOBALMACP0A1(ptemparray,i,SHIFT2,k,0)+SMALL))*GLOBALMACP0A1(ptemparray,i,j,k,0);
    //    MACP0A1(EOSextra,i,j,k,H4GLOBAL) = (Htest4 + SHIFT2*MACP0A1(EOSextra,i,N2-1-SHIFT2,k,HGLOBAL)/(GLOBALMACP0A1(ptemparray,i,N2-1-SHIFT2,k,0)+SMALL))*GLOBALMACP0A1(ptemparray,i,j,k,0);

  }




#if(0)
  //////////////////////////////
  //
  // get +- \phi direction H (assume spherical polar)
  // GODMARK : NOT YET FOR MPI
  COMPFULLLOOP{ 
    Htest1 = Htest2 = 0.0;
    ii=i;
    jj=j;
    for(kk=k;kk<N3;kk++){
      Htest1 +=GLOBALMACP0A1(ptemparray,ii,jj,kk,1);
    }
    for(kk=k;kk>=0;kk--){
      Htest2 +=GLOBALMACP0A1(ptemparray,ii,jj,kk,1);
    }
    // This is scale-height used by Kaz's EOS
    MACP0A1(EOSextra,i,j,k,H5GLOBAL) = Htest1*GLOBALMACP0A1(ptemparray,i,j,k,0);
    MACP0A1(EOSextra,i,j,k,H6GLOBAL) = Htest2*GLOBALMACP0A1(ptemparray,i,j,k,0);
  }
#endif




  ///////////////
  //
  // constrain resulting H
  //
  ///////////////
  constrainH(EOSextra,prim);



}



// get lambdatot(rho0,u)
static void get_lambdatot(FTYPE *EOSextra, FTYPE rho0, FTYPE u, FTYPE *lambdatotptr)
{
  int whichd=UTOTDIFF;
  int whichfun;
  FTYPE unu,snu,pnu,du;



  /////////////////////
  //
  // determine what whichfun0 means for this table
  // could put this in lookup with primarytable->whichtable, but expensive to put there and assume all tables same whichdatatype, so this is ok
  //
  /////////////////////
  if(whichdatatype[primarytable]==3){
    whichfun=EXTRA5;
  }
  else if(whichdatatype[primarytable]==4){
    whichfun=EXTRA19;
  }
  else{
    dualfprintf(fail_file,"Shouldn't request whichfun0=%d if primarytable=%d\n",LAMBDATOT,primarytable);
    myexit(46763463);
  }

  if(whichdatatype[primarytable]==4){
    unu = EOSextra[UNUGLOBAL];
    pnu = EOSextra[PNUGLOBAL];
    snu = EOSextra[SNUGLOBAL];
    du  = u - unu;
  }
  else{
    du = u;
  }



  if(getsingle_eos_fromtable(whichfun,whichd,EOSextra,rho0,du,lambdatotptr)){ // uses utotdiff=du
    *lambdatotptr=BIG; // then assume optically thin (worry if outside when rho>>the limit in the table?) GODMARK
  }

}




static void constrainH(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{
  int hi;
  int i,j,k;

  if(whichdatatype[primarytable]!=4){ // otherwise no need to limit since not using table lookup for H
    for(hi=0;hi<NUMHDIRECTIONS;hi++){
      COMPFULLLOOP{ 
	
	
	// since there exists no information beyond bounds of table, limit H to table values so return value is consistent
	// This is also needed since H is used with other EOS table values and needs to be consistent
	// Note that when using simpletable that these restrictions won't matter
	// offset from strict linear table limits because log-linear conversion leads to error
	if(MACP0A1(EOSextra,i,j,k,HGLOBAL+hi)>0.999*lineartablelimits[primarytable][HEOS][1]) MACP0A1(EOSextra,i,j,k,HGLOBAL+hi) = 0.999*lineartablelimits[primarytable][HEOS][1];
	if(MACP0A1(EOSextra,i,j,k,HGLOBAL+hi)<1.001*lineartablelimits[primarytable][HEOS][0]) MACP0A1(EOSextra,i,j,k,HGLOBAL+hi) = 1.001*lineartablelimits[primarytable][HEOS][0];
      }
    }
  }


}










// assumes this is computed every timestep (or substep) before anything else computed
static void compute_TDYNORYE_YNU_global(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS], FTYPE (*prim)[NSTORE2][NSTORE3][NPR])
{
  int i,j,k;
  //  int jj;
  //  int pl,pliter;
  //  FTYPE r,th;
  //  FTYPE X[NDIM],V[NDIM],dxdxp[NDIM][NDIM];
  FTYPE TDYNORYEtouse, YNUtouse;


#if(DOYL==DONOYL || DOYNU==DONOYNU)
  return;
#endif


  COMPFULLLOOP{
    
    //    coord_ijk(i,j,k,CENT,X); // doesn't matter if CENT or anyother thing is used since just an estimate
    //    bl_coord_ijk(i,j,k,CENT,V);
    //    dxdxprim_ijk(i,j,k,CENT,dxdxp);
    //    r=V[1];

    
#if(! (DOYL==DONOYL && DOYNU==DONOYNU) )
    TDYNORYEtouse = MACP0A1(prim,i,j,k,YL) - MACP0A1(prim,i,j,k,YNU);

    if(whichdatatype[primarytable]==4){
      // Below really YNU0
      YNUtouse = MACP0A1(EOSextra,i,j,k,YNU0GLOBAL);
    }
    else{
      YNUtouse = MACP0A1(prim,i,j,k,YNU);
    }
#endif  

    //    dualfprintf(fail_file,"COMPUTEYE: %21.15g %21.15g %21.15g\n",MACP0A1(prim,i,j,k,YL),MACP0A1(prim,i,j,k,YNU),TDYNORYEtouse);


    // since there exists no information beyond bounds of table, limit TDYNORYE to table values so return value is consistent
    // This is also needed since TDYNORYE is used with other EOS table values and needs to be consistent
    // Note that when using simpletable that these restrictions won't matter
    // offset from strict linear table limits because log-linear conversion leads to error
    if(TDYNORYEtouse>0.999*lineartablelimits[primarytable][YEEOS][1]) TDYNORYEtouse = 0.999*lineartablelimits[primarytable][YEEOS][1];
    if(TDYNORYEtouse<1.001*lineartablelimits[primarytable][YEEOS][0]) TDYNORYEtouse = 1.001*lineartablelimits[primarytable][YEEOS][0];

    if(YNUtouse>0.999*lineartablelimits[primarytable][YNUEOS][1]) YNUtouse = 0.999*lineartablelimits[primarytable][YNUEOS][1];
    if(YNUtouse<1.001*lineartablelimits[primarytable][YNUEOS][0]) YNUtouse = 1.001*lineartablelimits[primarytable][YNUEOS][0];

    // make final assignment
    MACP0A1(EOSextra,i,j,k,TDYNORYEGLOBAL)=TDYNORYEtouse;
    MACP0A1(EOSextra,i,j,k,YNU0GLOBAL)=YNUtouse;        // Below really YNU0 if whichdatatype==4

  }


}






// Change the primitives to be limited by constraints on Y_e
// Do this so if out of bounds, Y_e can still recover eventually if fluid goes out of region where Y_\nu is large
void fix_primitive_eos_scalars_kazfull(FTYPE *EOSextra, FTYPE *pr)
{
  FTYPE yemin,yemax;
  FTYPE ynumin,ynumax;
  FTYPE ylmin,ylmax;
  FTYPE ye;


  yemin=1.001*lineartablelimits[primarytable][YEEOS][0];
  yemax=0.999*lineartablelimits[primarytable][YEEOS][1];
  ynumin=1.001*lineartablelimits[primarytable][YNUEOS][0];
  ynumax=0.999*lineartablelimits[primarytable][YNUEOS][1];
  ylmin=yemin+ynumin;
  ylmax=yemax+ynumax;


  ye = pr[YL] - pr[YNU];

  // have pr[YL] and pr[YNU] share the blame
  if(ye<yemin){
    pr[YL]  += -0.5*ye;
    pr[YNU] += +0.5*ye;
    // now effective ye is yemin
    ye=yemin;
  }

  if(ye>yemax){
    pr[YL]  += -0.5*ye+0.5;
    pr[YNU] += +0.5*ye-0.5;
    // Now effective ye is yemax
    ye=yemax;
  }

  if(pr[YL]<ylmin){
    pr[YL]  = ylmin;
    pr[YNU] = ynumin;
    // Now effective ye is ~0.0=1.001*lineartablelimits[primarytable][YEEOS][0]
    ye=yemin;
    
  }

  // check result:
  if(pr[YNU]<ynumin){
    pr[YL]  = ye; // Y_l = Y_e in this case
    pr[YNU] = ynumin;
  }


  

}

