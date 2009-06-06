// superdefs.h : large data structures not needed by liaison code


/*

to add a new variable:

0) global.h: setup macro to turn on/off memory (BASEMAC??(name)) below.

1) Add a variable called BASEMAC???(name) that is the *definition* of the memory space

2) Lower down in this file, define a pointer of the same type

3) set_arrays.c: add pointer shifting

4) set_arrays.c: down lower, assign to 0 or valueinit

5) set_array.c: use global.h macro to avoid assignments to memory is turned off

6) Use it!

*/


// SUPERNOTE: Ensure that name always starts with "BASEMAC??()" so can easily recall that these are the original arrays
// SUPERNOTE: Arrays should always have order: N1M,N2M,N3M -- these are remapped using the macro

// OPTMARK: Note that while clustering some arrays to be per (i,j,k) point may have benefits due to spatial-temporal localization, if that variable is later interpolated or differenced, then lose benefit since that procedure has to pull out a single object out of the built-up structure.
// OPTMARK:
// OPTMARK: Reasonable things to pull together (balance of above issues):
// 1) pk,unewglobal,ulastglobal,uinitialglobal,dUgeomarray,upointglobal
// 2) pleft,pright,prc, dq1,dq2,dq3 [maybe]
// 3) gp_l gp_r
// 4) ptemp utemp
// 5) pstagglobal pvcorninterp pbcorninterp
// 6) dxdxpstore, idxdxpstore, Xstore, Vstore
// 7) conn, conn2, idxvol
//
// Things not reasonable to put together because treated separately:
// 1) F1,F2,F3,F1EM,F2EM,F3EM
// 2) fluxstate, fluxstatecent
// 3) compgeom and anything
// 4) Bhatglobal pstagglobal [maybe]
// Anything related to p, pstagglobal, emf
// Useless: wspeed, fluxvectemp, Fa,Fb,stencilvarptemp, geomcornglobal


// memory and pointer shift declarations/definitions used globally in code
#include "kazfulleos.superdefs.h"


// below is always used -- never use as temp
FTYPE BASEMACP0A1(pglobal,N1M,N2M,N3M,NPR);	/* space for primitive vars */

// below is always used -- never use as temp
#if(ANALYTICMEMORY)
FTYPE BASEMACP0A1(panalytic,N1M,N2M,N3M,NPR);       /* space for primitive vars */
#if(FIELDSTAGMEM)
FTYPE BASEMACP0A1(pstaganalytic,N1M,N2M,N3M,NPR);       /* space for primitive vars */
#endif
#endif

// below is always used -- never use as temp
#if(NUMPOTHER>0)
FTYPE BASEMACP1A0(pother,NUMPOTHER,N1M,N2M,N3M);	/* space for primitive vars */
#endif

// arbitrary temporary storage array
FTYPE BASEMACP0A1(ptemparray,N1M,N2M,N3M,NPR);

// arbitrary temporary storage array
FTYPE BASEMACP0A1(utemparray,N1M,N2M,N3M,NPR);

#if(DOEVOLVEMETRIC)
FTYPE BASEMACP0A1(ucumformetric,N1M,N2M,N3M,NPR);
#endif


// emf has extra zone on upper end since corner quantity and some components exist that are needed for cell centered quantities
FTYPE BASEMACP1A0(emf,NDIM,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);	/* space for emf */

#if(STOREFLUXSTATE)
struct of_state BASEMACP1A1(fluxstate,COMPDIM,N1M,N2M,N3M,NUMLEFTRIGHT);
struct of_state BASEMACP0A0(fluxstatecent,N1M,N2M,N3M); // pb-like (not pi-like)
#endif

#if(FIELDTOTHMEM)
// emf and vconemf assumed not used across substeps so can use as temp var
// below was [COMPDIM] but wanted to use emf as simple temp space for 4D
FTYPE BASEMACP0A1(vconemf,N1M,N2M,N3M,NDIM-1);	/* used for Athena EMFs */
#endif

#if(TRACKVPOT)
FTYPE BASEMACP1A0(vpotarrayglobal,NDIM,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE BASEMACP1A0(vpotanalytic,NDIM,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
#endif



#if(STOREWAVESPEEDS)
// below is used within substeps but not across
FTYPE BASEMACP0A1(wspeedtemp,N1M,N2M,N3M,NUMCS); // temporarily store wspeed in case not just copying but averaging before putting into wspeed array
FTYPE BASEMACP2A0(wspeed,COMPDIM,NUMCS,N1M,N2M,N3M); // wave speeds (left/right)
#endif


#if(STORESHOCKINDICATOR)
// below is used within substeps but not across
FTYPE BASEMACP1A0(shockindicatorarray,NUMSHOCKPLS,N1M,N2M,N3M);
#endif




////////////////////////////////////////////////
//
// TIME-STEPPING
//
////////////////////////////////////////////////

// below is used within substeps AND across substeps but not across full timesteps
FTYPE BASEMACP1A1(pk,MAXDTSTAGES,N1M,N2M,N3M,NPR);	/* next-step primitives */
// for higher order RK time stepping integrations
FTYPE BASEMACP0A1(unewglobal,N1M,N2M,N3M,NPR); // used across substeps and across full time steps
FTYPE BASEMACP0A1(ulastglobal,N1M,N2M,N3M,NPR); // used across substeps but not across full time steps
FTYPE BASEMACP0A1(uinitialglobal,N1M,N2M,N3M,NPR); // used across substeps but not across full time steps
FTYPE BASEMACP0A1(dUgeomarray,N1M,N2M,N3M,NPR); // assume not used across substeps so can use as temp var

#if(HIGHERORDERMEM||FIELDSTAGMEM)
// below is used within substeps but not across
FTYPE BASEMACP0A1(upointglobal,N1M,N2M,N3M,NPR);
#endif




////////////////////////////////////////////////
//
// SPATIAL INTERPOLATION
//
////////////////////////////////////////////////

// Below F1,F2,F3,F1EM,F2EM,F3EM have extra shift ONLY to have extra memory to avoid seg fault, not to be used for actual calculations!  Hence, pointer to these does NOT have such a shift.  This allows flux_ct() to go over F1,F2,F3 sumes for emf in a single loop rather than multiple loops.
// For FLUXCTTOTH, need space at bottom, while for FLUXCD need space at top.  So add 2 extra spaces.
#if(N1>1)
// below is used within substeps but not across if doing ACCURATEDIAG
FTYPE BASEMACP0A1(F1,SHIFT1+N1M+SHIFT1,SHIFT2+N2M+SHIFT2,SHIFT3+N3M+SHIFT3,NPR);	/* fluxes */
#endif
#if(N2>1)
// below is used within substeps but not across if doing ACCURATEDIAG
FTYPE BASEMACP0A1(F2,SHIFT1+N1M+SHIFT1,SHIFT2+N2M+SHIFT2,SHIFT3+N3M+SHIFT3,NPR);	/* fluxes */
#endif
#if(N3>1)
// below is used within substeps but not across if doing ACCURATEDIAG
FTYPE BASEMACP0A1(F3,SHIFT1+N1M+SHIFT1,SHIFT2+N2M+SHIFT2,SHIFT3+N3M+SHIFT3,NPR);	/* fluxes */
#endif

#if(SPLITMAEMMEM)
#if(N1>1)
// below is used within substeps but not across if doing ACCURATEDIAG
FTYPE BASEMACP0A1(F1EM,SHIFT1+N1M+SHIFT1,SHIFT2+N2M+SHIFT2,SHIFT3+N3M+SHIFT3,NPR);	/* fluxes */
#endif
#if(N2>1)
// below is used within substeps but not across if doing ACCURATEDIAG
FTYPE BASEMACP0A1(F2EM,SHIFT1+N1M+SHIFT1,SHIFT2+N2M+SHIFT2,SHIFT3+N3M+SHIFT3,NPR);	/* fluxes */
#endif
#if(N3>1)
// below is used within substeps but not across if doing ACCURATEDIAG
FTYPE BASEMACP0A1(F3EM,SHIFT1+N1M+SHIFT1,SHIFT2+N2M+SHIFT2,SHIFT3+N3M+SHIFT3,NPR);	/* fluxes */
#endif
#endif


#if(SPLITNPR||FIELDSTAGMEM)
// below 2 assume not used across substeps so can use as temp var
FTYPE BASEMACP1A1(gp_l,NDIM-1,N1M,N2M,N3M,NPR2INTERP);
FTYPE BASEMACP1A1(gp_r,NDIM-1,N1M,N2M,N3M,NPR2INTERP);
#endif

FTYPE BASEMACP0A1(pleft,N1M,N2M,N3M,NPR2INTERP); /* for parabolic interpolation */
FTYPE BASEMACP0A1(pright,N1M,N2M,N3M,NPR2INTERP); /* for parabolic interpolation */
// below is used within substeps but not across
FTYPE BASEMACP0A1(prc,N1M,N2M,N3M,NPR2INTERP);	/* rescaled primitives, also used for temporary storage in fixup_utoprim() */



#if(FIELDSTAGMEM)
// use unew,ulast,uinitial to store conserved staggered field even for non-finite-volume methods since always available
// need to set unew/uinitial with staggered field initially at t=0
// note fixup mods to unew when doing finite volume never change field, so no conflict with staggered use
// at some point pleft/pright to staggered B field from u(new/last/initial?) so pleft/pright well-defined
// at some point need to fill p[] with interpolated version of staggered field so p[] well-defined
// pleftcorn/prightcorn are interpolated from pleft/pright
// ensure STOREWAVESPEEDS is on
// assume need not store separate wavespeed for corn, just use stored wspeed when needed during specific flux calculation (no interpolation)
//FTYPE BASEMACP2A0(wspeedcorn,COMPDIM,NUMCS,N1M,N2M,N3M); // wave speeds (left/right) at corner (not true corner)
FTYPE BASEMACP0A1(pstagglobal,N1M,N2M,N3M,NPR); // for interpolate_pfield_face2cent() -- only contains fields
// below has more memory than needed for 2nd COMPDIM (can be 2) but leave as 3 for simplicity in accessing array
//FTYPE BASEMACP3A0(pbcorninterp,COMPDIM,COMPDIM,NUMCS,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3); // holds field corner interpolations
FTYPE BASEMACP1A3(pvbcorninterp,COMPDIM,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,COMPDIM,NUMCS+1,NUMCS); // holds velocity corner interpolations (NUMCS+1) has +1 part that holds old pbcorninterp
FTYPE BASEMACP1A0(geomcornglobal,COMPDIM,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3); // really for merged method with FLUXCTSTAG

#if(HIGHERORDERMEM)
// below used to store Bhat that satisfies divBhat=0 for fluxrecon method when evolving point field at higher order since then unew doesn't contain Bhat
FTYPE BASEMACP0A1(Bhatglobal,N1M,N2M,N3M,NPR);
FTYPE BASEMACP0A1(Bhatanalytic,N1M,N2M,N3M,NPR);
#endif

// below is used to store de-averaged field in each direction for UNSPLIT scheme or 2D
// used in general, but only different if FV method
// first 3 are directions orthogonal to field given by B1,B2,B3 in NDIM
//FTYPE BASEMACP1A1(pb1davg,3,N1M,N2M,N3M,3);

#endif



////////////////////////////////////////////////
//
// OLD SPATIAL INTERPOLATION
//
////////////////////////////////////////////////


#if(DODQMEMORY)
#if(N1>1)
// below is used within substeps but not across
FTYPE BASEMACP0A1(dq1,N1M,N2M,N3M,NPR2INTERP);	/* slopes */
#endif
#if(N2>1)
// below is used within substeps but not across
FTYPE BASEMACP0A1(dq2,N1M,N2M,N3M,NPR2INTERP);	/* slopes */
#endif
#if(N3>1)
// below is used within substeps but not across
FTYPE BASEMACP0A1(dq3,N1M,N2M,N3M,NPR2INTERP);	/* slopes */
#endif
#endif



////////////////////////////////////////////////
//
// HIGHER ORDER STUFF
//
////////////////////////////////////////////////

#if(HIGHERORDERMEM)

// termporary storage for flux
FTYPE BASEMACP0A1(fluxvectemp,N1M,N2M,N3M,NPR);

FTYPE BASEMACP0A1(Fa,N1M,N2M,N3M,NPR);	/* fluxes */
FTYPE BASEMACP0A1(Fb,N1M,N2M,N3M,NPR);	/* fluxes */

FTYPE BASEMACP0A1(stencilvartemp,N1M,N2M,N3M,NPR);

#endif














////////////////////////////////////////////////
//
// DEBUG STUFF USUALLY OFF
//
////////////////////////////////////////////////

#if(DOENODEBUG)
// 3: 1,2,3 directions
// 5: c2e on P, a2c on U, c2a for U, c2a for Fperp1, c2a for Fperp2
CTYPE BASEMACP0A4(enodebugarray,N1M,N2M,N3M,3-1,NUMENOINTERPTYPES,NPR-4,NUMENODEBUGS);// space for debugging eno
#endif

#if(FLUXDUMP)
FTYPE BASEMACP0A1(fluxdump,N1M,N2M,N3M,NUMFLUXDUMP);
#endif


////////////////////////////////////////////////
//
// DEBUG STUFF USUALLY ON
//
////////////////////////////////////////////////

PFTYPE BASEMACP0A1(pflag,N1M,N2M,N3M,NUMPFLAGS);	/* space for flag */

PFTYPE BASEMACP0A0(pflagfailorig,N1M,N2M,N3M);	/* used to keep original pflag so don't induce order-dependence when doing fixup_utoprim() and changing pflag */

/* for debug */
#if(DODEBUG)
CTYPE BASEMACP0A2(failfloorcount,N1M,N2M,N3M,NUMTSCALES,NUMFAILFLOORFLAGS); // number of failures and floor adjustments for each zone
#endif


////////////////////////////////////////////////
//
// other diagnostics
//
////////////////////////////////////////////////


#if(DODISS)
FTYPE BASEMACP0A1(dissfunpos,N1M,N2M,N3M,NUMDISSFUNPOS); // storage for dissipation function
#endif

#if(CALCFARADAYANDCURRENTS)
// current density stuff
// below 3 are used within and across substeps and full steps (for diagnostics)
FTYPE BASEMACP0A2(cfaraday,N1M,N2M,N3M,NUMCURRENTSLOTS,3);
FTYPE BASEMACP0A1(fcon,N1M,N2M,N3M,NUMFARADAY);
FTYPE BASEMACP0A1(jcon,N1M,N2M,N3M,NDIM);
#endif


////////////////////////////////////////////////
//
// AVG diagnostics
//
////////////////////////////////////////////////
#if(DOAVG)
// time average stuff
FTYPE BASEMACP0A1(normalvarstavg,N1M,N2M,N3M,NUMNORMDUMP);
FTYPE BASEMACP0A1(anormalvarstavg,N1M,N2M,N3M,NUMNORMDUMP);

#if(CALCFARADAYANDCURRENTS)
FTYPE BASEMACP0A1(jcontavg,N1M,N2M,N3M,NDIM);
FTYPE BASEMACP0A1(jcovtavg,N1M,N2M,N3M,NDIM);
FTYPE BASEMACP0A1(ajcontavg,N1M,N2M,N3M,NDIM);
FTYPE BASEMACP0A1(ajcovtavg,N1M,N2M,N3M,NDIM);
#endif

FTYPE BASEMACP0A1(massfluxtavg,N1M,N2M,N3M,NDIM);
FTYPE BASEMACP0A1(amassfluxtavg,N1M,N2M,N3M,NDIM);

FTYPE BASEMACP0A1(othertavg,N1M,N2M,N3M,NUMOTHER);
FTYPE BASEMACP0A1(aothertavg,N1M,N2M,N3M,NUMOTHER);

#if(CALCFARADAYANDCURRENTS)
FTYPE BASEMACP0A1(fcontavg,N1M,N2M,N3M,NUMFARADAY);
FTYPE BASEMACP0A1(fcovtavg,N1M,N2M,N3M,NUMFARADAY);
FTYPE BASEMACP0A1(afcontavg,N1M,N2M,N3M,NUMFARADAY);
FTYPE BASEMACP0A1(afcovtavg,N1M,N2M,N3M,NUMFARADAY);
#endif

FTYPE BASEMACP0A1(tudtavg,N1M,N2M,N3M,NUMSTRESSTERMS);
FTYPE BASEMACP0A1(atudtavg,N1M,N2M,N3M,NUMSTRESSTERMS);
#endif



///////////////////////////////
//
// grid functions (+1 size larger so can have geometry at upper corners -- say for vector potential or whatever)
//
///////////////////////////////

#if(NEWMETRICSTORAGE)

// below used for cases when need gdet or eomfunc[] at multiple grid locations:  Created to avoid cache misses with flux_ct()
struct of_gdetgeom BASEMETMACP0A1(gdetgeom,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NPG);

// below created for simple gdet calls since faster to have storage like this sometimes
struct of_gdetgeom BASEMETMACP1A0(gdetgeomnormal,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);

// new memory-local way (avoids major cache misses)
struct of_compgeom BASEMETMACP1A0(compgeom,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);

#if(DOEVOLVEMETRIC)
struct of_compgeom BASEMETMACP1A0(compgeomlast,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
#endif

#else
// grid functions that exist at multiple locations within a cell
FTYPE BASEMETMACP1A1(gcon,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,SYMMATRIXNDIM);
FTYPE BASEMETMACP1A1(gcov,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,SYMMATRIXNDIM);
FTYPE BASEMETMACP1A1(gcovpert,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);
FTYPE BASEMETMACP1A0(gdet,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
#if(WHICHEOM!=WITHGDET)
FTYPE BASEMETMACP1A1(eomfunc,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NPR);
#endif
#if(GDETVOLDIFF)
FTYPE BASEMETMACP1A0(gdetvol,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3); // for volume regularization of CENT quantities only
#endif
FTYPE BASEMETMACP1A0(alphalapse,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE BASEMETMACP1A0(betasqoalphasq,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
FTYPE BASEMETMACP1A1(beta,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);

#if(DOEVOLVEMETRIC)
// used to evolve metric in time, so that connection has terms due to dg/dt
FTYPE BASEMETMACP1A1(gcovlast,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,SYMMATRIXNDIM);
FTYPE BASEMETMACP1A1(gcovpertlast,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);
FTYPE BASEMETMACP1A0(alphalapselast,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3);
#endif
#endif // end if old way



#if(DOSTOREPOSITIONDATA)
FTYPE BASEMETMACP1A2(dxdxpstore,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM,NDIM);
FTYPE BASEMETMACP1A2(idxdxpstore,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM,NDIM);
FTYPE BASEMETMACP1A1(Xstore,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);
FTYPE BASEMETMACP1A1(Vstore,NPG,N1M+SHIFT1,N2M+SHIFT2,N3M+SHIFT3,NDIM);
#endif

// rest of grid functions always at center
FTYPE BASEMETMACP0A3(conn,N1M,N2M,N3M,NDIM,NDIM,NDIM);
FTYPE BASEMETMACP0A1(conn2,N1M,N2M,N3M,NDIM);
#if(VOLUMEDIFF)
FTYPE BASEMETMACP0A1(idxvol,N1M,N2M,N3M,NDIM); // volume regularization 1/dx for each direction
#endif







