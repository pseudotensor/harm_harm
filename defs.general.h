// BELOW USED BY makeopenmpsharedlist.sh to generate OPENMPSHAREDLIST, so should come last among all things in this file.
int BEGINOPENMPSHAREDLIST;



#include "mpidefs.h"


#include "kazfulleos.defsglobalprivate.h" // put here so OpenMP private globals are defined before global.nondepnmemonics.h sets up thread private pragma's



/////////////////////////////////////////////////////////////////
//
//
// GLOBAL VARIABLES that are not for every point in space
//
// OPENMPNOTE: Note that any global variable *written to* inside parallel region must be made private and dealt with or neglected in value.
//             Otherwise race condition occurs and if results depend upon that variable, then result will be undefined and wrong in general.
//
//
/////////////////////////////////////////////////////////////////

#include "rancdefs.h"

FTYPE Xmetricnew[NDIM],Xmetricold[NDIM]; // used to store time of latest and oldest metric

SFTYPE *lumvsr,*lumvsr_tot;

SFTYPE *dissvsr[NUMDISSVERSIONS],*dissvsr_tot[NUMDISSVERSIONS];

FTYPE *rcent,*rcent_tot;

SFTYPE *dVvsr,*dVvsr_tot,*vrsqvsr,*vrsqvsr_tot,*dMvsr,*dMvsr_tot,*dTrrvsr, *dTrrvsr_tot,*Mvsr_tot,*Mvsrface1_tot,*MOrvsr_tot,*phivsr_tot,*dJvsr,*dJvsr_tot,*Jvsr_tot,*Jvsrface1_tot;


/** GLOBAL PARAMETERS SECTION **/

/* physics parameters */
FTYPE gam,gamideal;

/* numerical parameters */
int defcoord;
FTYPE Rin, R0, Rout, hslope, Zin, Zout;
FTYPE Rin_array[NDIM], Rout_array[NDIM];  //atch -- arrays for a more general way of handling the grid dimensions
FTYPE Risco,Rhor;
FTYPE cour;
FTYPE dV, dVF, dx[NDIM], startx[NDIM], endx[NDIM], Diffx[NDIM];
SFTYPE dt,t,tf,tstepparti,tsteppartf;
SFTYPE fluxdumpdt;
long fluxdumprealnstep;
FTYPE TDYNORYEglobal,Hglobal;
FTYPE rcurr, hcurr;
FTYPE drsing;

//int istart, istop, jstart, jstop;
#if(SIMULBCCALC!=-1) 
int isc,iec,jsc,jec;
int isf1,ief1,jsf1,jef1,ksf1,kef1;
int isf2,ief2,jsf2,jef2,ksf2,kef2;
int isf3,ief3,jsf3,jef3,ksf3,kef3;
int ise,iee,jse,jee;
int isf1ct,ief1ct,jsf1ct,jef1ct;// GODMARK: other stage type requires more
int isf2ct,ief2ct,jsf2ct,jef2ct;
int isf3ct,ief3ct,jsf3ct,jef3ct;
int isdq,iedq,jsdq,jedq;
int ispdq,iepdq,jspdq,jepdq;
#endif

FTYPE mydminarg1, mydminarg2;
long nstep;
int specialstep;

int steppart,numstepparts;

int gocont; // used to continue running(runtype, directinput, timereenter)
int runtype;

/* output parameters */
FILE *log_file;
FILE *fail_file;
FILE *logfull_file;
FILE *logdt_file;
FILE *logdtfull_file;
FILE *logstep_file;
FILE *logperf_file;
SFTYPE DTstep,DTstepdot,DTperf,DTgocheck,DTtimecheck,DTperfdump;
int reallaststep,onemorestep;

// file version stuff:
int PVER,GRIDVER,DVER,FLVER,NPVER,AVG1DVER,AVG2DVER,ENERVER,MODEVER,LOSSVER,SPVER,TSVER,LOGDTVER,STEPVER,PERFVER,ADVER,PDVER,CALCVER,FLINEVER;

int PTYPE,GRIDTYPE,DTYPE,FLTYPE,NPTYPE,AVG2DTYPE,AVG1DTYPE,ENERTYPE,LOSSTYPE,SPTYPE,TSTYPE,LOGDTTYPE,STEPTYPE,PERFTYPE,ADTYPE,PDTYPE,CALCTYPE,FLINETYPE,MODETYPE,EXPANDTYPE,NPCOMPUTETYPE;


SFTYPE DTdumpgen[NUMDTDS];
long dumpcntgen[NUMDTDS];
//SFTYPE DTd;
//SFTYPE DTavg;
//SFTYPE DTener;
//SFTYPE DTi;
//SFTYPE DTdebug;
long DTr;
//long dump_cnt;
//long avg_cnt;
//long debug_cnt;
//long image_cnt;
long rdump_cnt;
//long fieldline_cnt; // assumed to keep track with images (as in diag.c), so no need to include in restart()

int nstroke; // OPENMPMARK: Bad in some inversion codes


//for holding absolute values of indices of regions -- for restarting
FTYPE t_transition_in,t_transition_out;
int global_enerregiondef[NUMENERREGIONS][NUMUPDOWN][NDIM];




/* global flags */
int failed;
int lim[NDIM],fluxmethod,FLUXB,UTOPRIMVERSION,TIMEORDER,DOENOFLUX,avgscheme[NDIM];
int dofluxreconevolvepointfield,emffixedstencil,extrazones4emf,splitmaem,unewisavg;
int do_transverse_flux_integration[NPR],do_conserved_integration[NPR],do_source_integration[NPR];
int useghostplusactive;
FTYPE defcon;

/* diagnostics */
// don't track this separately in other regions except global region
SFTYPE frdot[N1][NPR];
SFTYPE pdottermsjet2[COMPDIM*2][NUMFLUXTERMS][NPR];
CTYPE failfloorcountlocal[NUMTSCALES][NUMFAILFLOORFLAGS]; // don't track this separately in jet
CTYPE failfloorcountlocal_tot[NUMTSCALES][NUMFAILFLOORFLAGS]; // don't track this separately in jet

// general stuff for ener.out file for regions to completely track, including terms within flux
int dothisenerreg[NUMENERREGIONS];
int dofluxreg[NUMENERREGIONS][COMPDIM*2];
int enerposreg[NUMENERREGIONS][COMPDIM*2];
// these quantities contain diagnostics
// all these require writing to restart file
// other _tot quantities appear in dump_ener.c that don't need to be written to restart file since easily computed from existing data.
SFTYPE fladdreg[NUMENERREGIONS][NPR];
SFTYPE fladdreg_tot[NUMENERREGIONS][NPR];
SFTYPE fladdtermsreg[NUMENERREGIONS][NUMFAILFLOORFLAGS][NPR];
SFTYPE fladdtermsreg_tot[NUMENERREGIONS][NUMFAILFLOORFLAGS][NPR];
SFTYPE Ureg_init[NUMENERREGIONS][NPR];
SFTYPE Ureg_init_tot[NUMENERREGIONS][NPR];
SFTYPE pcumreg[NUMENERREGIONS][COMPDIM*2][NPR];
SFTYPE pcumreg_tot[NUMENERREGIONS][COMPDIM*2][NPR];
SFTYPE pdotreg[NUMENERREGIONS][COMPDIM*2][NPR];
SFTYPE pdottermsreg[NUMENERREGIONS][COMPDIM*2][NUMFLUXTERMS][NPR];
SFTYPE sourceaddreg[NUMENERREGIONS][NPR];
SFTYPE sourceaddreg_tot[NUMENERREGIONS][NPR];
SFTYPE sourceaddtermsreg[NUMENERREGIONS][NUMSOURCES][NPR];
SFTYPE sourceaddtermsreg_tot[NUMENERREGIONS][NUMSOURCES][NPR];
SFTYPE dissreg[NUMENERREGIONS][NUMDISSVERSIONS];
SFTYPE dissreg_tot[NUMENERREGIONS][NUMDISSVERSIONS];
//SFTYPE horizonflux[NPR];
//SFTYPE horizoncum[NPR];
//SFTYPE horizonflux_tot[NPR];
//SFTYPE horizoncum_tot[NPR];

// below quantities are not kept in restart file since easily recomputed
// kept global so can always access current value throughout code
SFTYPE pdotreg_tot[NUMENERREGIONS][COMPDIM*2][NPR];
SFTYPE pdottermsreg_tot[NUMENERREGIONS][COMPDIM*2][NUMFLUXTERMS][NPR];

// used for each region, related to global quantities
// _tot quantities here are global since used in restart.
// dangerous as global quantity since could overlap if nesting use of enerregion stuff! GODMARK
int *doflux;
int *enerpos;
SFTYPE *fladd;
SFTYPE *fladd_tot;
SFTYPE (*fladdterms)[NPR];
SFTYPE (*fladdterms_tot)[NPR];
SFTYPE *U_init;
SFTYPE *U_init_tot;
SFTYPE (*pcum)[NPR];
SFTYPE (*pcum_tot)[NPR];
SFTYPE (*pdot)[NPR];
SFTYPE (*pdotterms)[NUMFLUXTERMS][NPR];
SFTYPE *sourceadd;
SFTYPE *sourceadd_tot;
SFTYPE (*sourceaddterms)[NPR];
SFTYPE (*sourceaddterms_tot)[NPR];
SFTYPE *diss;
SFTYPE *diss_tot;

// kept global
SFTYPE (*pdot_tot)[NPR];
SFTYPE (*pdotterms_tot)[NUMFLUXTERMS][NPR];


// end changes after ...


/* Jon's addition */
int horizoni,horizoncpupos1;
long realnstep;
int partialstep;
int mpicombine;
int mpicombinetype;
int truempicombinetype;
int halftimep;
int whichrestart;
int appendold;
int whocalleducon; // OPENMPNOTE: Ensure those are set as threadprivate [noted only called outside parallel regions]
// global flags
long restartsteps[2];
int binaryoutput,sortedoutput;
int CHECKCONT,DOTSTEPDIAG,DOLOGSTEP,DOLOGPERF;
int NDTCCHECK,NZCCHECK,NDTDOTCCHECK,NGOCHECK,NTIMECHECK,NDTPERFDUMPCHECK;
SFTYPE PERFWALLTIME,ZCPSESTIMATE;

long steptofaildump,steptofailmap;
int ifail,jfail,kfail; // OPENMPNOTE: Ensure those are set as private [noted only for diagnostics outside parallel regions]
int dofailmap,dofaildump,restartonfail;
// IC
FTYPE h_over_r;
// BC
FTYPE h_over_r_jet;
int BCtype[COMPDIM*2];
int rescaletype;
int cooling;
int DOENERDIAG,DOGDUMPDIAG,DORDUMPDIAG,DODUMPDIAG,DOAVGDIAG, DOIMAGEDIAG,DOAREAMAPDIAG;
int GAMMIEDUMP,GAMMIEIMAGE,GAMMIEENER,DODIAGS,RESTARTMODE,WHICHFILE,POSDEFMETRIC,DOENODEBUGEVERYSUBSTEP,DODIAGEVERYSUBSTEP;
int INVERTFROMAVERAGEIFFAILED,LIMIT_AC_PRIM_FRAC_CHANGE,LIMIT_AC_FRAC_CHANGE; //atch
int PARAMODWENO;
FTYPE MAX_AC_FRAC_CHANGE,MAX_AC_PRIM_FRAC_CHANGE;
FTYPE RHOMIN,UUMIN,RHOMINLIMIT,UUMINLIMIT;
FTYPE prMAX[NPR];
FTYPE prfloorcoef[NPR];
FTYPE BSQORHOLIMIT,BSQOULIMIT,UORHOLIMIT,GAMMAMAX,GAMMADAMP,GAMMAFAIL;
FTYPE SAFE;
int debugfail;
FTYPE uttdiscr;  // OPENMPNOTE: Ensure those are set as threadprivate [noted only for WHICHVEL==VEL3]
int jonchecks;
int dnumcolumns[NUMDUMPTYPES];
struct blink * blinkptr0[NUMDUMPTYPES];
struct blink * cpulinkptr0[NUMDUMPTYPES];
int DOCOLSPLIT[NUMDUMPTYPES];
int docolsplit; // global var for now CHANGINGMARK
int nextcol;
int doevolvemetricsubsteps, gravityskipstep;
FTYPE gravitydtglobal, sourcedtglobal, wavedtglobal;
int waveglobaldti[NDIM],waveglobaldtj[NDIM],waveglobaldtk[NDIM];
int didstorepositiondata,didstoremetricdata;

/* physical consts */
FTYPE msun,lsun,rsun,G,H,C,qe,Na,malpha,mn,me,kb,arad,sigmasb,sigmamat,mevocsq,ergPmev,mp,Q,R,Re,hpl,hbar,K,K2;
SFTYPE a,MBH,QBH;
FTYPE Mfactor,Jfactor,rhofactor;
SFTYPE dabh,dE,dJ,dEold,dJold;
FTYPE mb,mbcsq,mbwithrhounit,amu,a0,MBH0,QBH0,Mdot,Mdotc,Mcgs,Ccode;
FTYPE Lunit,Tunit,Vunit,rhounit,rhomassunit,Munit,mdotunit,energyunit,edotunit,Pressureunit,Tempunit,Bunit,massunitPmsun;
int rho0unittype;
FTYPE ledd,leddcode;

int NUMBUFFERS;

int advancepassnumber;

// OPENMPNOTE: Ensure all pl's are set as private as required [only npr2interp and npr2notinterp changed in parallel regions] and removed use of "plglobal" type globals
// for SPLITNPR (NOW generally used):
// for choosing range of PLOOP type arrays
int nprstart,nprend; // normally 0 and NPR-1
int nprlist[MAXNPR]; // maximum is NPR elements

// for choosing range of PLOOPINTERP type arrays
int npr2interpstart,npr2interpend; // normally 0 and NPR2INTERP-1
int npr2interplist[MAXNPR]; // maximum is NPR2INTERP elements

// for choosing range of PLOOPNOTINTERP type arrays
int npr2notinterpstart,npr2notinterpend; // normally 0 and -1
int npr2notinterplist[MAXNPR]; // maximum is NPR2INTERP elements

// for choosing range of PBOUNDLOOP and PLOOPMPI type arrays
int nprboundstart,nprboundend; // normally 0 and NPRBOUND-1
int nprboundlist[MAXNPR]; // maximum is NPRBOUND elements

// for choosing range of PFLUXBOUNDLOOP and PLOOPMPI type arrays
int nprfluxboundstart,nprfluxboundend; // normally 0 and NPRFLUXBOUND-1
int nprfluxboundlist[MAXNPR]; // maximum is NPRFLUXBOUND elements

// for choosing range of PDUMPLOOP
int nprdumpstart,nprdumpend; // normally 0 and NPRDUMP-1
int nprdumplist[MAXNPR]; // maximum is NPRDUMP elements

// for choosing range of PINVERTLOOP
int nprinvertstart,nprinvertend; // normally 0 and NPRINVERT-1
int nprinvertlist[MAXNPR]; // maximum is NPRINVERT elements


int fluxloop[NDIM][NUMFLUXLOOPNUMBERS];
int emffluxloop[NDIM][NUMFLUXLOOPNUMBERS];
int Uconsloop[NUMFLUXLOOPNUMBERS];
int emfUconsloop[NUMFLUXLOOPNUMBERS];
int Uconsevolveloop[NUMFLUXLOOPNUMBERS];
int a_interporder[NUMINTERPS];
int *interporder;


// ENO DEBUG GLOBAL VARS
//int dirglobal,locglobal,iglobal,jglobal,kglobal,iterglobal,interporfluxglobal;

// SOME GEOMETRIC VARIABLES
int special3dspc;

int numbercpu[ 3+1 ];

// Ramesh stuff
FTYPE nu,ss,ucrit,Ttpow,jetalpha;


// EOS related functions
FTYPE (*ptr_pressure_rho0_u[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
FTYPE (*ptr_compute_u_from_entropy[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE entropy);
FTYPE (*ptr_u_rho0_p[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE p);
FTYPE (*ptr_dpdu_rho0_u[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
FTYPE (*ptr_dpdrho0_rho0_u[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
FTYPE (*ptr_cs2_compute[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);

FTYPE (*ptr_compute_entropy[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
FTYPE (*ptr_compute_dSdrho[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
FTYPE (*ptr_compute_dSdu[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);

FTYPE (*ptr_compute_specificentropy_wmrho0[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0);
FTYPE (*ptr_compute_dspecificSdrho_wmrho0[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0);
FTYPE (*ptr_compute_dspecificSdwmrho0_wmrho0[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0);

FTYPE (*ptr_pressure_wmrho0[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0);
FTYPE (*ptr_compute_idwmrho0dp[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0);
FTYPE (*ptr_compute_idrho0dp[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE wmrho0);
FTYPE (*ptr_compute_qdot[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
int (*ptr_compute_sources_EOS[NUMEOSS])(FTYPE *EOSextra, FTYPE *pr, struct of_geom *geom, struct of_state *q, FTYPE *Ui, FTYPE *dUother, FTYPE(*dUcomp)[NPR]);
void (*ptr_compute_allextras[NUMEOSS])(int justnum, FTYPE *EOSextra, FTYPE rho0, FTYPE u, int *numextrasreturn, FTYPE *extras);
int (*ptr_get_extrasprocessed[NUMEOSS])(int doall, FTYPE *EOSextra, FTYPE *pr, FTYPE *extras, FTYPE *processed);
FTYPE (*ptr_compute_temp[NUMEOSS])(FTYPE *EOSextra, FTYPE rho0, FTYPE u);
void (*ptr_compute_EOS_parms[NUMEOSS])(FTYPE (*EOSextra)[NSTORE2][NSTORE3][NUMEOSGLOBALS],  FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
void (*ptr_store_EOS_parms[NUMEOSS])(int numparms, FTYPE *EOSextra, FTYPE *parlist);
void (*ptr_get_EOS_parms[NUMEOSS])(int*numparms, FTYPE *EOSextra, FTYPE *parlist);

FTYPE SQRTMINNUMREPRESENT;

FTYPE NUMEPSILONPOW23;

// some grid section or loop things
int AVOIDADVANCESHIFTX1DN,AVOIDADVANCESHIFTX1UP,AVOIDADVANCESHIFTX2DN,AVOIDADVANCESHIFTX2UP,AVOIDADVANCESHIFTX3DN,AVOIDADVANCESHIFTX3UP,GLOBALBCMOVEDWITHACTIVESECTION;


#include "defs.user.h"



int crapdebug;



// BELOW USED BY makeopenmpsharedlist.sh to generate OPENMPSHAREDLIST, so should come last among all things in this file.
int ENDOPENMPSHAREDLIST;


#pragma omp threadprivate(OPENMPGLOBALPRIVATELIST)
