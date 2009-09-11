////////////////////////////
//
// EOS from KAZ FULL
//
////////////////////////////


// PIPETODO:
// 1) Streamline existing lookup code to use pointer referneces to functions to avoid conditionals.
// 2) Only go to 1E-10 or something instead of 1E-14 for inversion




// non-dependent and dependent macros
//#include "kazfulleos.global.h" // included at top-level now

// small-sized definitions
#include "kazfulleos.defs.h"



// memory and pointers for EOS tables
#include "kazfulleos.eostablesdefs.h"


///////////////
//
// some globally (within this file) accessed functions
//
///////////////
static int getsingle_eos_fromtable(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPEEOS *answer);
static int get_eos_fromtable(int whichtablesubtype, int *iffun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPEEOS *answers, int *badlookups);
static int which_eostable(int whichtablesubtype, int ifdegencheck, int whichindep, int *vartypearraylocal, FTYPE *qarray, int *whichtable);
static void eos_lookup_degen(int begin, int end, int skip, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray);
static void eos_lookup_degen_utotdegencut23(int begin, int end, int skip, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray);
static void eos_lookup_prepost_degen(int whichdegen, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray);
static int get_whichindep_fromwhichd(int whichd, int *whichindep);
static void eos_lookup_modify_qarray(int whichdegen, FTYPEEOS *myanswers, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray);

static void lookup_yespecial(int whichtable, int vartypearraylocal, FTYPE qarray, FTYPEEOS *indexarray);
static void lookup_log(int whichtable, int vartypearraylocal, FTYPE qarray, FTYPEEOS *indexarray);


// include C files for simplicity
#include "kazfulleos_set_arrays.c"
#include "kazfulleos_read_setup_eostable.c"
#include "kazfulleos_eosfunctions.c"
#include "kazfulleos_lookuptable_interpolation.c"







// called during read_setup_eostable()
void initeos_kazfulleos(void)
{
#if(ALLOWKAZEOS)
  
  // initialize repeated qarray's
  int qi,whichd,extrai;
  for(whichd=0;whichd<NUMEOSDEGENQUANTITIESMEM1;whichd++){
    for(extrai=0;extrai<NUMEXTRATABLETYPES;extrai++) for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMENUSED;qi++) qoldarray[extrai][whichd][qi]=-BIG;
    //    kaziiowhichd[whichd]=kazjjowhichd[whichd]=kazkkowhichd[whichd]=kazllowhichd[whichd]=kazmmowhichd[whichd]=INITKAZINDEX;
  }
  for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMENUSED;qi++) qoldarrayextras[qi]=-BIG; // all same UEOS independent variable, so no need for [whichd] dependence
  doallextrasold=-1;
#endif
  
}



// non-pipelined way to get single value from pipelined lookup
static int getsingle_eos_fromtable(int whichfun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPEEOS *answer)
{
  FTYPEEOS answers[MAXEOSPIPELINE];
  int badlookups[MAXEOSPIPELINE];
  int iffun[MAXEOSPIPELINE];
  int numcols,coli;
  int whichtablesubtype=whichtablesubtypeinquantity[whichfun];
  int whichcol=whichcolinquantity[whichfun];

  ///////////
  //
  // can pipeline up to numcols quantities
  //
  ///////////
  numcols = numcolintablesubtype[whichtablesubtype]; // ok use of numcolintablesubtype
  // default iffun=0
  for(coli=0;coli<numcols;coli++) iffun[coli]=0;
  // correct iffun
  iffun[whichcol]=1;

  get_eos_fromtable(whichtablesubtype,iffun,whichd,EOSextra,quant1,quant2,answers,badlookups);

  *answer = answers[whichcol];

  return(badlookups[whichcol]); // can do this for just 1 quantity

}



////////////////////////////
//
// Choose behavior of loop depending upon whether including degen offset
//
////////////////////////////
// GODMARK: Could set this 
#if(ALLOWDEGENOFFSET)
// then need to lookup degen part first
#define whichdegenstart 1
#define whichdegenend 0
#else
#define whichdegenstart 0
#define whichdegenend 0
#endif


// for now assume TDYNORYE is locked to timestep that is order 1E-6 seconds, so can use lowest leos
// which = which function looking up
// q1 = rhob
// q2 = u or P or \chi
// notice that q1-q2 are pass by value, so internally changed but externally remains unchanged

// iffun[0...] = 0 or 1 depending upon if want that quantity in that subtable
// answers[0...] is filled with answer if iffun==1
// for now assume can only do one whichd type per call, so quantities within table subtypes have to agree on their independent variables
static int get_eos_fromtable(int whichtablesubtype, int *iffun, int whichd, FTYPE *EOSextra, FTYPE quant1, FTYPE quant2, FTYPEEOS *answers, int *badlookups)
{
  int whichindep;
  int whichdegen;
  FTYPEEOS myanswers[2][MAXEOSPIPELINE]; // for [2]: 0 = non-degen answer  1=degen answer
  FTYPE qarray[NUMINDEPDIMENSMEM];
  FTYPE qfloor[NUMINDEPDIMENSMEM];
  int i,qi,coli;
  int repeatedeos;
  int numcols;
  int failreturn=0;
  int vartypearraylocal[NUMINDEPDIMENSMEM];
  int localiffun[MAXEOSPIPELINE];
  int localbadlookups[MAXEOSPIPELINE];
  int localwhichtablesubtype;
  int isextratype;
  int firsteos;
  int localnumcols;

  ///////////////////////////////
  //
  // Ensure that setup table
  //
  ///////////////////////////////
#if(PRODUCTION==0)
  if(didsetupkazeos==0){
    dualfprintf(fail_file,"Requested WHICHEOS=KAZEOS but didn't initialize kaz table by calling read_setup_eostable()\n");
    myexit(151068); 
  }

  // For repeatedeos, don't want coincidence where (e.g.) utotdiff==stotdiff and think repeated.  Need independents to be same.  Now controlled with using [whichd] on qoldarray
  if(whichd!=whichdintablesubtype[whichtablesubtype] && whichdintablesubtype[whichtablesubtype]!=NOSUCHDIFF ){
    dualfprintf(fail_file,"lookup not setup for different whichd=%d in this whichtablesubtype=%d with whichd=%d\n",whichd,whichtablesubtype,whichdintablesubtype[whichtablesubtype]);
  }
#endif



 
  //////////////////////
  //
  // Determine which independent variable to use for "temperature": utot, ptot, or chi (or utotdiff, ptotdiff, chidiff for degen case)
  //
  //////////////////////
  get_whichindep_fromwhichd(whichd,&whichindep);
  // used to translate q1-q5 to INDEP0-INDEP6, and used with limits and sizes of tables
  //  for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMEN;qi++){
  for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMEN;qi++){
    vartypearraylocal[qi]=vartypearray[qi]; // use of global, which is ok since unchanging
  }
  // OVERRIDE global value for temperature-like quantity
  vartypearraylocal[TEMPLIKEINDEP]=whichindep;



  ///////////
  //
  // can pipeline up to numcols quantities
  //
  ///////////
  numcols = numcolintablesubtype[whichtablesubtype]; // ok use of numcolintablesubtype
  isextratype=isextraintablesubtype[whichtablesubtype];
  firsteos=firsteosintablesubtype[whichtablesubtype];

  for(coli=0;coli<numcols;coli++){
    if(iffun[coli]) badlookups[coli]=1; // all bad by default.  Using this to check if need to get this coli during processing
    else badlookups[coli]=0; // unwanted are not bad
    // SO DO NOT CHANGE iffun[] here or in subfunctions.  Also do NOT default badlookups to 1 in subfunctions since already done here and use this to see what other functions yet to do.
    // So can only change badlookups to 0, can't set to 1
  }


  ///////////////////////////////
  //
  // nuclear offset (as consistent with HELM code to obtain correct energy per baryon)
  //
  ///////////////////////////////
  offsetquant2(whichd, EOSextra, quant1, quant2, &quant2);



  ////////////////////////////
  //
  // Set array of independent EOS quantities from qarray and EOSextra[]
  //
  ////////////////////////////
  qarray[RHOINDEP]=quant1;
  qarray[TEMPLIKEINDEP]=quant2;
  // qarray[TEMPLIKEINDEP+?] are always stored in EOSextra
  for(qi=TEMPLIKEINDEP+1;qi<=LASTINDEPDIMENUSED;qi++){
    qarray[qi] = EOSextra[vartypeeosextraarray[qi]];
  }


  //////////////////////////////////////////////////////
  //
  // check if already got here with these q values (done after assignment of q3-q5, so distinct -- done before floors on q since they make less distinct anyways)
  //
  // Note that repeatedeos uses only qarray and whichd, but not based upon whichtablesubtype
  // This is so that in principle input independents could be same but switch to another subtable so don't have to re-get indexarray[]
  //
  // repeated entry can occur as long as:
  // 1) Same whichd
  // 2) Same qarray
  // 3) Resolves to same type of table (i.e. extra version or not extra version) since this is case where "whichd" and "qarray" and "whichtable" are not enough information for a given single function or whichtablesubtype.
  //
  // This is done instead of having (e.g.) qoldarray[MAXEOSPIPELINE] since many subtypes have same whichd and are on same range and want to be able to repeat across table subtypes with same "whichd".  extra generally on different range, which is why it has its own array index
  //
  // Then have old result stored that came from some table as stored in whichtable[]
  //
  // Result is in repeatedfun[] and resultold[] that *are* [MAXEOSPIPELINE] since don't care where answer came from as long as have one in storage
  //
  //////////////////////////////////////////////////////


  repeatedeos=1;
  //repeatedeos=0; // SUPERFUCKMARK
  for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMENUSED;qi++){ // only need to repeat used independent variables, not all
    repeatedeos*=(fabs(qarray[qi]-qoldarray[isextratype][whichd][qi])<OLDTOLERANCE);
  }




  //////////////////////////////////////////////////////
  //
  // Compute result or retrieve already computed result
  //
  //////////////////////////////////////////////////////
  if(repeatedeos){

    //////////
    //
    // grab old result if exists and reset badlookups->0 since no longer need to get that quantity
    //
    //////////
    for(coli=0;coli<numcols;coli++){
      // check if already got the function wanted
      if(repeatedfun[firsteos+coli] && iffun[coli]){
	// choice of table irrelevant if already found solution
	answers[coli]=resultold[firsteos+coli];
	badlookups[coli]=0; // no longer need to get this one
      }
    }


    //////////
    //
    // check if still quantities to get
    //
    //////////
    int stilltoget=0; for(coli=0;coli<numcols;coli++) if(iffun[coli] && badlookups[coli]==1) stilltoget++;


    //////////
    //
    // get quantities not stored in repeated array but grid index is same as previous since repeatedeos
    //
    //////////
    if(stilltoget){
      // then get result

      // whatever is done after degen table lookup and normal table lookup
      whichdegen=whichdegenend;
      
      // check if previously did get table result and previously used degeneracy and non-degeneracy tables fell within same table type (full, simple, simplezoom)
      if(whichtable[isextratype][whichd][ISNOTDEGENTABLE]==NOTABLE || whichtable[isextratype][whichd][ISDEGENTABLE]!=whichtable[isextratype][whichd][ISNOTDEGENTABLE]){
	diag_eosfaillookup((int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
	// badlookups[all coli] will remain for all bad ones
	// return now since nothing else to do
	return(0); // but not hard failure
      }
      else{
	// DEBUG:
	//if(debugfail>=2) dualfprintf(fail_file,"REPEATFROMLOOKUP\n"); // DEBUG
	//if(debugfail>=2) dualfprintf(fail_file,"CHECKLOOKUP:(repeated) %d %d %d : %d %d\n",(int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL],whichdegen,whichfun);


	// tell get_eos_fromlookup() which functions to try and get.  Better than having to look at badlookups inside get_eos_fromlookup()
	for(coli=0;coli<numcols;coli++) if(iffun[coli]&&badlookups[coli]) localiffun[coli]=1; else localiffun[coli]=0;

	// do lookup directly using old indexarray that is valid since independents unchanged
	failreturn += get_eos_fromlookup(repeatedeos,WHICHEOSDIMEN,whichdegen, whichtable[isextratype][whichd][whichdegen], whichtablesubtype, localiffun, whichindep, quant1, vartypearraylocal, indexarray[isextratype][whichd], myanswers[whichdegen],badlookups);

	// see if good lookup
	for(coli=0;coli<numcols;coli++){
	  if(localiffun[coli]){ // only look at new coli's
	    if(badlookups[coli]){
	      dualfprintf(fail_file,"Repeated lookup got no valid lookup in table\n"); // actually bad failure
	      diag_eosfaillookup((int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
	    } // end if bad lookup
	    else{
	      // setup return of answer
	      answers[coli]=myanswers[whichdegen][coli];

	      // now save result if got result so can use next time
	      resultold[firsteos+coli]=myanswers[whichdegen][coli];
	      repeatedfun[firsteos+coli]=1;
	    }// end else if good lookup
	  }// end localiffun==1
	}// end over coli

      }// end else if correct table and did previously get a result from table
    }// end if still quantity to get
    else{

      // then done since nothing left to get -- it was all inside the repeated array

    } // end else if done





  }// end if repreatedeos
  else{ // else if not repeatedeos, so need to do full lookup





    /////////////
    //
    // setup old values for next time since doing new lookup
    //
    /////////////
    for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMENUSED;qi++) qoldarray[isextratype][whichd][qi]=qarray[qi]; // should be qarray *before* any degeneracy subtraction on qarray[TEMPLIKEINDEP]
    // once values of independent variables changes, reset so that none of quantities in subtable are set as repeated, no matter iffun==0 or 1
    for(coli=0;coli<numcols;coli++) repeatedfun[firsteos+coli]=0;
    // if not repeated, then reset whichtable="prior table used" to access the subtabletype
    whichtable[isextratype][whichd][ISNOTDEGENTABLE]=whichtable[isextratype][whichd][ISDEGENTABLE]=NOTABLE;




    ////////////////////////////
    //
    // Obtain interpolated function
    //
    // 1) Get whichtable for degen table -- which determines ieos,keos,leos
    // 2) Determine jeos after have degen offset from interpolation of ieos,keos,leos
    //
    ////////////////////////////
    for(whichdegen=whichdegenstart;whichdegen>=whichdegenend;whichdegen--){


      //////////////
      //
      // tell get_eos_fromlookup() which functions to try and get.  Better than having to look at badlookups inside get_eos_fromlookup()
      // if degen type lookup, get *all* degens
      //
      //////////////
      if(whichdegen==ISNOTDEGENTABLE){
	localnumcols=numcols;
	for(coli=0;coli<localnumcols;coli++) if(iffun[coli]&&badlookups[coli]){ localwhichtablesubtype=whichtablesubtype; localiffun[coli]=1; localbadlookups[coli]=1;} else { localwhichtablesubtype=whichtablesubtype; localiffun[coli]=0; localbadlookups[coli]=0;}
      }
      else{
	localnumcols=get_numcols(whichtable[isextratype][whichd][whichdegen],SUBTYPEDEGEN);
	for(coli=0;coli<localnumcols;coli++) {localwhichtablesubtype=SUBTYPEDEGEN; localiffun[coli]=1; localbadlookups[coli]=1;}
      }


      

      // Once looked-up ieos,keos,leos, don't need to do full lookup again (but do need to do full interpolation "again" that is on different set of quantities)
      // that is, only jeos changes
      // see if within table, and if so lookup interpolated result

      // can't predict that when taking subtraction on utot -> utotdiff that will end up within same table, so always get both tables
      failreturn += which_eostable(whichtablesubtype,whichdegen,whichindep, vartypearraylocal, qarray, &whichtable[isextratype][whichd][whichdegen]);

      // check if got a table, and if so then use it, otherwise return(1) and assume using off-table values
      if(whichtable[isextratype][whichd][whichdegen]==NOTABLE){
	if(debugfail>=3){
	  dualfprintf(fail_file,"qarray not within table\n");
	  dualfprintf(fail_file,"subtype=%d whichtable=%d : whichd=%d whichdegen=%d :: ig=%d jg=%d kg=%d\n",whichtablesubtype,whichtable[isextratype][whichd][whichdegen],whichd,whichdegen,(int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
	  for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMENUSED;qi++) dualfprintf(fail_file,"qarray[%d]=%21.15g\n",qi,qarray[qi]);
	  for(coli=0;coli<localnumcols;coli++) if(iffun[coli]) dualfprintf(fail_file,"coli=%d bad=%d\n",coli,badlookups[coli]);
	}
	diag_eosfaillookup((int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
	// badlookups will be used since didn't change to 0
	// return now since nothing else to do
	return(0); // but not hard failure

      }
      else{

	// check that degen and normal table resolved to same table, since can't use degen value from one table to get values from another table
	if(whichdegen==whichdegenend){

	  if(whichtable[isextratype][whichd][ISNOTDEGENTABLE]==NOTABLE || whichtable[isextratype][whichd][ISNOTDEGENTABLE]!=whichtable[isextratype][whichd][ISDEGENTABLE]){
	    dualfprintf(fail_file,"Degen and normal table selections different: %d %d\n",whichtable[isextratype][whichd][ISNOTDEGENTABLE],whichtable[isextratype][whichd][ISDEGENTABLE]);
	    diag_eosfaillookup((int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
	    // badlookups will be used since didn't change to 0
	    // return now since nothing else to do
	    return(0); // but not hard failure
	  }
	}



	////////////
	//
	// Lookup the positions: ieos,jeos,keos,leos for a given table, independent variable definition, and indep values "qarray"
	//
	// for whichdegen==1, below does set indexarray[][2]=0 for accessing degen table
	//
	////////////
	eos_lookup_prepost_degen(whichdegen, whichtable[isextratype][whichd][whichdegen], whichindep, vartypearraylocal, qarray, indexarray[isextratype][whichd]);



	////////////////////////////
	//
	// now compute result
	//
	// For whichdegen==1, myanswers[whichdegen==1] then contains either {utot/ptot/chitot/stot}offset for utotdegencut==0,1 OR contains 3 quantities corressponding to {utot,ptot,chitot,stot}{0,in,out} used by eos_lookup_modify_qarray() to obtain indexed version of qarray[TEMPLIKEINDEP]
	//
	// For whichdegen==0, presume qarray[TEMPLIKEINDEP] correct for lookup and so lookup quantities within subtable=whichtablesubtype and lookup those quantities listed as iffun[whichfun]=1
	//
	////////////////////////////
	

	//////////////
	//
	// do either non-degen or degen lookup
	//
	//////////////
	failreturn=get_eos_fromlookup(repeatedeos,WHICHEOSDIMEN,whichdegen, whichtable[isextratype][whichd][whichdegen], localwhichtablesubtype, localiffun, whichindep, quant1, vartypearraylocal, indexarray[isextratype][whichd],myanswers[whichdegen],localbadlookups);


	////////////
	//
	// Check if bad lookup
	//
	////////////
	for(coli=0;coli<localnumcols;coli++){
	  if(localiffun[coli] && localbadlookups[coli]==1){
	    if(1||debugfail>=3){
	      dualfprintf(fail_file,"Looked up, but no answer within table: coli=%d out of %d\n",coli,localnumcols);
	      dualfprintf(fail_file,"subtype=%d whichtable=%d : whichd=%d whichdegen=%d :: ig=%d jg=%d kg=%d\n",whichtablesubtype,whichtable[isextratype][whichd][whichdegen],whichd,whichdegen,(int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
	      for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMENUSED;qi++) dualfprintf(fail_file,"qarray[%d]=%21.15g\n",qi,qarray[qi]);
	    }
	    diag_eosfaillookup((int)EOSextra[IGLOBAL],(int)EOSextra[JGLOBAL],(int)EOSextra[KGLOBAL]);
	  }
	}


	// transfer over badlookups if final non-degen quantity
	if(whichdegen==0) for(coli=0;coli<numcols;coli++) if(iffun[coli]&&badlookups[coli] && localbadlookups[coli]==0) badlookups[coli]=0;


	if(whichdegen==1){
	  ////////////////////////////
	  //
	  // translate qarray[TEMPLIKEINDEP] from original value to value required to do final temperature-like quantity lookup
	  //
	  // myanswers[whichdegen] should have multiple values with utotdegencut>=2
	  //
	  ////////////////////////////
	  eos_lookup_modify_qarray(whichdegen, myanswers[whichdegen],whichtable[isextratype][whichd][whichdegen],whichindep,vartypearraylocal,qarray,indexarray[isextratype][whichd]);
	}


      } // end if whichtable!=NOTABLE and so qarray[] within some table's independent variables
    }// end loop over degenerate and then normal table (or if ALLOWDEGENOFFSET==0, then only for normal table)
    

    // finally, normal table lookup gives answer over *all* originally desired coli's
    for(coli=0;coli<numcols;coli++){
      if(iffun[coli] && badlookups[coli]==0){
	answers[coli]=myanswers[ISNOTDEGENTABLE][coli];
	// setup repeated for next time
	resultold[firsteos+coli]=answers[coli];
	repeatedfun[firsteos+coli]=1;

	// DEBUG:
	//	if(whichtablesubtype==SUBTYPEEXTRA){
	//	  dualfprintf(fail_file,"coli=%d answers=%21.15g\n",coli,answers[coli]);
	//	}


      }
    }


    // DEBUG:
    //    if(whichtablesubtype==SUBTYPEEXTRA){
    //      for(qi=FIRSTINDEPDIMEN;qi<=LASTINDEPDIMENUSED;qi++) dualfprintf(fail_file,"qarray[%d]=%21.15g index[ise=%d][whichd=%d][qi=%d]=%21.15g\n",qi,qarray[qi],isextratype,whichd,qi,indexarray[isextratype][whichd][qi]);
    //    }



  }// end else if not repeated




  /////////////////
  //
  // Report failure status
  //
  /////////////////
  return(failreturn);


}






// which_eostable() is written for each type of dataset(s) to used -- too complicated to make general

// check if density and pressure-like quantity are within table
// all quantities are real code units (i.e. linear mapping instead of log10 mapping)
// here we only presume to care about density and internal energy, while H and T are presumed to be truncated rather than extended with some alternative
// ifdegencheck: 0 = normal table check  1 = ignores q2 (u,p,chi) since generally q2 is largest range possible and later will restrict/check if within the full table
// whichindep = which independent variable (based upon which function looking up)
static int which_eostable(int whichtablesubtype, int ifdegencheck, int whichindep, int *vartypearraylocal, FTYPE *qarray, int *whichtable)
{
  int whichtabletry;
  // don't assume tables are setup so q1 and q2 always have same range
  // assume all quantities depend on limits in same way, so "which" doesn't matter
  // assume HEOS and YEEOS are always withing range for now



  /////////////////////////////
  //
  // Note that q3,q4,q5 ((TDYN or YE),(TDYN or YNU), Hcm) are computed such that is forced to be within FULLTABLE limits so consistently used
  //
  // Note that for q2, for utotdegencut>=2 that qarray[TEMPLIKEINDEP] is actually an fractional index "i/N"=lutotdiff, and lineartablelimits[] is correctly that range of "i/N"=lutotdiff from 0..1.0
  /////////////////////////////

#if(ALLOWFULLTABLE)
  if(whichtablesubtype==SUBTYPEEXTRA && WHICHDATATYPEGENERAL==4){
    whichtabletry=EXTRAFULLTABLE;
  }
  else whichtabletry=FULLTABLE;
    
  if(qarray[1]>=lineartablelimits[whichtabletry][vartypearraylocal[1]][0] && qarray[1]<=lineartablelimits[whichtabletry][vartypearraylocal[1]][1]
     &&
     (ifdegencheck || qarray[TEMPLIKEINDEP]>=lineartablelimits[whichtabletry][vartypearraylocal[TEMPLIKEINDEP]][0] && qarray[TEMPLIKEINDEP]<=lineartablelimits[whichtabletry][vartypearraylocal[TEMPLIKEINDEP]][1])
     ){
    *whichtable=whichtabletry;
    return(0);
  }
#endif
#if(ALLOWSIMPLETABLE)
  if(whichtablesubtype==SUBTYPEEXTRA && WHICHDATATYPEGENERAL==4){
    whichtabletry=EXTRASIMPLETABLE;
  }
  else whichtabletry=SIMPLETABLE;
    
  if(qarray[1]>=lineartablelimits[whichtabletry][vartypearraylocal[1]][0] && qarray[1]<=lineartablelimits[whichtabletry][vartypearraylocal[1]][1]
     &&
     (ifdegencheck || qarray[TEMPLIKEINDEP]>=lineartablelimits[whichtabletry][vartypearraylocal[TEMPLIKEINDEP]][0] && qarray[TEMPLIKEINDEP]<=lineartablelimits[whichtabletry][vartypearraylocal[TEMPLIKEINDEP]][1])
     ){
    *whichtable=whichtabletry;
    return(0);
  }
#endif


  if(debugfail>=3){ // DEBUG: was turned on when debugging EOS
    dualfprintf(fail_file,"NOT IN LOOKUP: ifdegencheck=%d whichindep=%d qarray[1]=%21.15g qarray[TEMPLIKEINDEP]=%21.15g\n",ifdegencheck,whichindep,qarray[1],qarray[TEMPLIKEINDEP]);
  }
  *whichtable=NOTABLE;
  return(1);

  // GODMARK:
  // alternative to this function is that we simply place a floor on the linear values of rho, u, H, and T so always within table values
  // Use: lineartablelimits[ii][0,1]
  //
  // problem is that for inversion, truncation still will be an if check so as slow as above anyways, so can just truncate here if wanted and then more general.  Only slows down simpler single-type calls to EOS.

}










// rho0,u,H,T have independent lookups for mapping to i,j,k,l and already done in eos_lookup_degen()
// u here stands for utotdiff,ptotdiff,and chidiff depending upon whichindep
// here only need to look up jeos associated with u
// notice that qarray and vartypearraylocal start at 1 not 0
static void eos_lookup_degen(int begin, int end, int skip, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray)
{
  int qi;

  // is this expensive?
  // log_base(R-R0)
  // avoid nan's by choosing minimal density

  // old simple log10 way:
  // i = (x - x0)*[(N-1)/(x1-x0)] such that if x=x0, then i=0, if x=x1, then i=N-1
  
  // new generalized log way:
  // i = (log_base(R-R0) - x_in)/dx

  for(qi=begin;qi<=end;qi++){
    if(qi==skip) continue;

    if(qi==YEINDEP && whichyelooptype[whichtable]==YELOOPTYPESPECIAL){
      lookup_yespecial(whichtable,vartypearraylocal[qi],qarray[qi],&indexarray[qi]);
    }
    else{
      lookup_log(whichtable,vartypearraylocal[qi],qarray[qi],&indexarray[qi]);
    }

  }


}



// get i(value)
static void lookup_log(int whichtable, int vartypearraylocal, FTYPE qarray, FTYPEEOS *indexarray)
{
  FTYPEEOS logq;
  FTYPEEOS prelogq;


  prelogq = MAX(qarray    -lineartablelimits[whichtable][vartypearraylocal]  [3],SMALL);
  logq    = log10(prelogq)*lineartablelimits[whichtable][vartypearraylocal][2];
  
  *indexarray = (logq   -tablelimits[whichtable][vartypearraylocal]  [0])*tablelimits[whichtable][vartypearraylocal]  [3];

}


// Get i(ye)
// see yetable.nb
static void lookup_yespecial(int whichtable, int vartypearraylocal, FTYPE qarray, FTYPEEOS *indexarray)
{
  FTYPEEOS num = tablesize[whichtable][YEINDEP];
  FTYPEEOS yei = lineartablelimits[whichtable][YEINDEP][0];
  FTYPEEOS yef = lineartablelimits[whichtable][YEINDEP][1];
  FTYPEEOS yegrid1 = eosyegrid1[whichtable];
  FTYPEEOS yegrid2 = eosyegrid2[whichtable];
  FTYPEEOS xgrid1 = eosxgrid1[whichtable];
  FTYPEEOS xgrid2 = eosxgrid2[whichtable];
  FTYPEEOS ye=qarray;

  // first range:
  if(ye<=yegrid1){
    *indexarray = (num-1.0)*xgrid1*log10(ye/yei)/log10(yegrid1/yei);
  }
  else if(ye<=yegrid2){
    *indexarray = (num-1.0)*(ye*(xgrid1-xgrid2) + xgrid2*yegrid1 - xgrid1*yegrid2)/(yegrid1-yegrid2);
  }
  else{
    *indexarray = (num-1.0)*(ye*(xgrid2-1.0) + yegrid2 - xgrid2*yef)/(yegrid2-yef);
  }

}



// for this function, presume qarray[TEMPLIKEINDEP] is in lutotdiff="i/N" form, so simpler conversion to "i" units
static void eos_lookup_degen_utotdegencut23(int begin, int end, int skip, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray)
{
  int qi;

  for(qi=begin;qi<=end;qi++){
    if(qi==skip) continue;
    
    indexarray[qi] = (qarray[qi])*tablelimits[whichtable][vartypearraylocal[qi]]  [3]; // tablelimits[][][3] is N[qi], so gives index

  }


}


// whichdegen: 1 = do degen lookup  0 = do normal lookup without energy indepenent variable lookup
static void eos_lookup_prepost_degen(int whichdegen, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray)
{

  void eos_lookup_degen(int begin, int end, int skip, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray);
  int skip;


  if(whichdegen==ISNOTDEGENTABLE){
    // if normal table after degen table, then only need to get q2 location
    skip = NONEINDEP;// nonsense value to avoid skipping

    if(utotdegencut[whichtable]>DEGENCUTLASTOLDVERSION){
      eos_lookup_degen_utotdegencut23(TEMPLIKEINDEP,TEMPLIKEINDEP,skip, whichtable, whichindep, vartypearraylocal, qarray, indexarray);
    }
    else{
      eos_lookup_degen(TEMPLIKEINDEP,TEMPLIKEINDEP,skip, whichtable, whichindep, vartypearraylocal, qarray, indexarray);
    }
  }
  else{
    // if doing degentable, lookall up except q=UEOS/PEOS/CHIEOS/SEOS
    skip=TEMPLIKEINDEP; 
    eos_lookup_degen(FIRSTINDEPDIMEN,LASTINDEPDIMENUSED,skip,whichtable, whichindep, vartypearraylocal, qarray, indexarray);
    indexarray[TEMPLIKEINDEP]=0.0; // set TEMPLIKEINDEP as 0 since degentable has this as 0 (may be floating -> integer issue in how this floating value is used)
  }



  
  /////////////////
  //
  // enforce floor on temperature-like quantity
  // assume table has low density and internal energy and don't want to go much below this since then implied temperature becomes undefined
  // if density is outside table, can go to lower temperatures than in table
  //
  ///////////////////
#if(ALLOWDEGENOFFSET)
  if(indexarray[TEMPLIKEINDEP]<0) indexarray[TEMPLIKEINDEP]=0;
#endif



  //////////
  //
  // assume bottom of table is default for other untouched dimensions unless changed here
  //
  /////////
  int qi;
  for(qi=LASTINDEPDIMENUSED+1;qi<=NUMINDEPDIMENS;qi++){
    indexarray[qi]=0.0;
  }



}




// If utotdegencut<=1: Translate qarray[TEMPLIKEINDEP] from {utot,ptot,chi,stot} -> {utotdiff,ptotdiff,chidiff,stotdiff}
// If utotdegencut>=2: Translate qarray[TEMPLIKEINDEP] from {utot,ptot,chi,stot} -> {lutotdiff,lptotdiff,lchidiff,lstotdiff} that actually correspond to "i/N" : the fractional index of the grid
// vartypearraylocal[TEMPLIKEINDEP] resolves to one of UEOS,PEOS,CHIEOS,SEOS as set globally when starting lookup
static void eos_lookup_modify_qarray(int whichdegen, FTYPEEOS *myanswers, int whichtable, int whichindep, int *vartypearraylocal, FTYPE *qarray, FTYPEEOS *indexarray)
{

  /////////////////////////
  //      
  // subtract offset from actual code value to see where within table we are in terms of the offset
  // when doing normal table this change won't matter (i.e. q2 not used again, and meaning of q2 is undefined when mixing function with independent variable, but avoid if statement)
  // if whichdegen==1, after below line then q2 contains utotoffset, ptotoffset, or chioffset consistent with independent variables used for EOSQUANTITIES



  if(utotdegencut[whichtable]<=1){
    // always true that ALLOWDEGENOFFSET if here with whichdegen==1
    // don't allow u<utotdiff that would imply T<~0
    // here myanswers[EOSOFFSET] = utotoffset
    FTYPEEOS UTOTOFFSET=myanswers[EOSOFFSET];
    qarray[TEMPLIKEINDEP] = qarray[TEMPLIKEINDEP] - UTOTOFFSET;
    qarray[TEMPLIKEINDEP] = MAX(qarray[TEMPLIKEINDEP],lineartablelimits[whichtable][vartypearraylocal[TEMPLIKEINDEP]][0]); // lowest value of utotdiff = utot-utotoffset = utotoffset-utotoffset = 0.0 or minimum of table


  }
  else{

    // if here, then myanswers[EOSOFFSET]=U0, myanswers[EOSIN]=UIN, myanswers[EOSOUT]=UOUT, so apply inversion to "i/N"
    FTYPEEOS UTOT0=myanswers[EOSOFFSET];
    FTYPEEOS UTOTIN=myanswers[EOSIN];
    FTYPEEOS UTOTOUT=myanswers[EOSOUT];
    FTYPEEOS UTOT=qarray[TEMPLIKEINDEP];
    // e.g. from eos_extract.m: lutotdiff = log10( (utot - utotoffset)./(utotin - utotoffset))./log10( (utotout - utotoffset)./(utotin - utotoffset));

    qarray[TEMPLIKEINDEP] = log10( (UTOT-UTOT0)/(UTOTIN-UTOT0) )/log10( (UTOTOUT-UTOT0)/(UTOTIN-UTOT0) );

    // min of utotdegencut>=2 table format is lutotdiff="i/N"=0 and at this point qarray[TEMPLIKEINDEP]=lutotdiff
    qarray[TEMPLIKEINDEP] = MAX(qarray[TEMPLIKEINDEP],lineartablelimits[whichtable][vartypearraylocal[TEMPLIKEINDEP]][0]);

    // presume ok to let qarray[TEMPLIKEINDEP] be beyond "i/N" after which ideal gas EOS used

    //    dualfprintf(fail_file,"UTOT0=%21.15g UTOTIN=%21.15g UTOTOUT=%21.15g UTOT=%21.15g qarray[2]=%21.15g\n",UTOT0,UTOTIN,UTOTOUT,UTOT,qarray[TEMPLIKEINDEP]);

  }
  



}









// obtain whichd from whichindep
static int get_whichindep_fromwhichd(int whichd, int *whichindep)
{

  if(whichd==UTOTDIFF) *whichindep=UEOS;
  else if(whichd==PTOTDIFF) *whichindep=PEOS;
  else if(whichd==CHIDIFF) *whichindep=CHIEOS;
  else if(whichd==STOTDIFF) *whichindep=SEOS;
  else{
    dualfprintf(fail_file,"Undefined whichd=%d in get_whichindep_fromwhichd()\n",whichd);
    myexit(21937525);
  }

  return(0);

}







