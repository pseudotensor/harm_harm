#include "decs.h"



#if(0) // uniformly low values although not always lower than original version
#define MAXITER 5
#define NRANSI
#define CON 1.1
#define CON2 (CON*CON)
//#define NTAB 130 // number of function evaluations is 2XNTAB
#define NTAB 30 // number of function evaluations is 2XNTAB

#define SAFE 2.0
#define TRYTOL (trytollocal) // attempted tolerance
#define OKTOL 1e-5 // error must be below this to avoid report
//#define HSTARTCHANGE 10.0
#endif

#if(1) // original version (gets pretty damn low for many, but not all, derivatives -- some 1E-6 rather than 1E-13)
#define MAXITER 5
#define NRANSI
#define CON 1.3
#define CON2 (CON*CON)
#define NTAB 10 // number of function evaluations is 2XNTAB

#define SAFE 2.0
#define TRYTOL 1E-10 // attempted tolerance
#define OKTOL 1e-5 // error must be below this to avoid report
#endif

// whether to turn on extensive recent debugging
#define DEBUGDF 0

// whether to output matrix of derivative and extrapolations of various orders
#define DEBUGOUTPUTAMATRIX 0

// whether to use Jon's additional starting "h" checks [about 2X-3X more expensive in typical cases]
#define USEJONEXTENSION 1



// jon's version of NR's dfridr modified to accept more general, needed, function
FTYPE dfridr(FTYPE (*func)(struct of_geom *, FTYPE*,int,int), struct of_geom *ptrgeom, FTYPE *X,int ii, int jj, int kk)
{
  int i,j,k;
  FTYPE errt,fac,hh,**a,ans;
  FTYPE dX[NDIM],Xh[NDIM],Xl[NDIM];
  FTYPE h,err;
  FTYPE firsthstart,hstart;
  FTYPE newdx,temp;
  int shit;
  int iter;
  FTYPE errlist[MAXITER];
  FTYPE hhlist[MAXITER];
  FTYPE anslist[MAXITER];
  FTYPE minerror,minerrorhstart;
  int miniter;
  int iterdebug;
  FTYPE minans;
  FTYPE trytollocal;
  int lasti;
  int Nvec[NDIM];
  int goodi,goodj;
  FTYPE goodhh;
  FTYPE truecon,truecon2;
  int nrlasti,nrgoodi,nrgoodj;
  FTYPE nrerr,nrans,nrgoodhh;



  trytollocal=NUMEPSILONPOW23;
  //  minhlocal=NUMEPSILONPOW23;
	
  // allocate memory
  a=dmatrix(1,NTAB,1,NTAB);

  // starting delta shouldn't be so large to cross interesting boundaries (like pole or r=0), but should be large enough to allow convergence to answer at smaller intervals.
  Nvec[0]=0;
  Nvec[1]=N1;
  Nvec[2]=N2;
  Nvec[3]=N3;
  if(kk==TT) hstart=CONNDELTA;
  else if(Nvec[kk]==1) hstart=CONNDELTA;
  else hstart=0.5*dx[kk];



  firsthstart=hstart;
  miniter=0; // didn't find minimum is assumed, which means first is minimum!
  iter=0;
  minerror=BIG;
  minans=BIG;
  truecon=CON;
  truecon2=CON2;
  goodhh=-1.0; // -1.0 indicates if ever found goodhh


  //////////////////////
  //
  // START BIG LOOP over Jon's iterations
  //
  //////////////////////
  while(1){



#if(DEBUGDF)
    dualfprintf(fail_file,"iter=%d\n",iter);
#endif

    // Set initial differential size
    h=hstart;
    if (h <= 0.0) nrerror("h must be positive definite in dfridr.");
    hh=h;


    // HARM STUFF
    for(k=0;k<NDIM;k++) dX[k]=0.0; // other components will remains 0 for this function
    dX[kk]=hh;
    for(k=0;k<NDIM;k++){
      //      Xl[k]=X[k]-dX[k];
      temp=X[k]-dX[k];
      newdx=temp-X[k];
      Xl[k]=X[k]+newdx;
      //      dualfprintf(fail_file,"newdx[k=%d] for low = %21.15g : %21.15g\n",k,newdx,Xl[k]);
    }
    for(k=0;k<NDIM;k++){
      //      Xh[k]=X[k]+dX[k];
      temp=X[k]+dX[k];
      newdx=temp-X[k];
      Xh[k]=X[k]+newdx;
      //      dualfprintf(fail_file,"newdx[k=%d] for high = %21.15g : %21.15g\n",k,newdx,Xh[k]);
    }
    //    dualfprintf(fail_file,"a[1][1]=%21.15g dX[kk=%d]=%21.15g\n",a[1][1],kk,dX[kk]);
    //    for(k=0;k<NDIM;k++) dualfprintf(fail_file,"X[%d]=%21.15g Xh[%d]=%21.15g Xl[%d]=%21.15g dXhl=%21.15g\n",k,X[k],k,Xh[k],k,Xl[k],Xh[k]-Xl[k]);
    //    for(k=0;k<NDIM;k++) dualfprintf(fail_file,"funch=%21.15g funcl=%21.15g\n",k,(*func)(ptrgeom,Xh,ii,jj),k,(*func)(ptrgeom,Xl,ii,jj));
    // end HARM STUFF


    // compute df/dx
    a[1][1]=((*func)(ptrgeom,Xh,ii,jj)-(*func)(ptrgeom,Xl,ii,jj))/(2.0*hh);
    err=BIG;
    for (i=2;i<=NTAB;i++) {
      hh /= truecon;

      // HARM STUFF
      dX[kk]=hh;
      for(k=0;k<NDIM;k++) Xl[k]=X[k]-dX[k];
      for(k=0;k<NDIM;k++) Xh[k]=X[k]+dX[k];
      //      for(k=0;k<NDIM;k++) dualfprintf(fail_file,"i=%d k=%d %21.15g %21.15g\n",i,k,X[k],dX[k]);
      // end HARM STUFF

      // compute df/dx with dx here as dx*dx in dx size
      a[1][i]=((*func)(ptrgeom,Xh,ii,jj)-(*func)(ptrgeom,Xl,ii,jj))/(2.0*hh);
      fac=truecon2;

      // compute Neville table for each smaller step size
      // Contains extrapolation to dx->0 for each possible order of extrapolation
      for (j=2;j<=i;j++) {
	// extrapolate for dx->0
	a[j][i]=(a[j-1][i]*fac-a[j-1][i-1])/(fac-1.0);
	fac=truecon2*fac;
	//unnormalized error
	//	errt=MAX(fabs(a[j][i]-a[j-1][i]),fabs(a[j][i]-a[j-1][i-1]));
	//normalized error
	//	errt=MAX(fabs(a[j][i]-a[j-1][i]),fabs(a[j][i]-a[j-1][i-1]))/((*func)(ptrgeom,X,ii,jj)+SMALL);
	// normalized error
	errt=MAX(fabs(a[j][i]-a[j-1][i]),fabs(a[j][i]-a[j-1][i-1]));
	errt/=MAX(MAX(MAX(fabs(a[j][i]),fabs(a[j-1][i])),MAX(fabs(a[j][i]),fabs(a[j-1][i-1]))),SMALL);

	if (errt <= err) {
	  err=errt;
	  ans=a[j][i];
	  goodj=j;
	  goodi=i;
	  goodhh=hh;
	}
      }// end over j derivatives

      // unnormalize error
      //      if (fabs((a[i][i]-a[i-1][i-1])) >= SAFE*(err)) break;
      // normalized error
      //      if (fabs((a[i][i]-a[i-1][i-1])/( (*func)(ptrgeom,X,ii,jj)+SMALL)) >= SAFE*(err)) break;
      //      if (fabs((a[i][i]-a[i-1][i-1]))/(fabs(ans)+SMALL) >= SAFE*(err)) break;
      // normalized error
      if (fabs((a[i][i]-a[i-1][i-1]))/MAX(SMALL,MAX(fabs(a[i][i]),fabs(a[i-1][i-1]))) >= SAFE*(err)){
	nrlasti=i;
	nrerr=err;
	nrans=ans;
	nrgoodj=goodj;
	nrgoodi=goodi;
	nrgoodhh=goodhh;
#if(0) // NR breaks a bit early -- found solution can sometimes be better if wait -- so just do whole NTAB
	break;
#endif
      }// end NR early termination check

      lasti=i;

    }// end over NTAB entries for i









#if(DEBUGOUTPUTAMATRIX) // debug report to see exactly what table contained
    dualfprintf(fail_file,"BEGIN DFRIDR table for i=%d j=%d k=%d :: ii=%d jj=%d kk=%d hstart=%21.15g lasti=%d nrlasti=%d\n",ptrgeom->i,ptrgeom->j,ptrgeom->k,ii,jj,kk,hstart,lasti,nrlasti);
    dualfprintf(fail_file,"   %21s ","ORDERS");
    for(i=1;i<=lasti;i++){
      dualfprintf(fail_file,"%21d",i);
    }
    dualfprintf(fail_file,"\n");
    for(i=1;i<=lasti;i++){
      dualfprintf(fail_file,"hh=%21.15g ",hstart/pow(truecon,i-1));
      for(j=1;j<=lasti;j++){
	if(j<=i){
	  if(i==goodi && j==goodj){
	    dualfprintf(fail_file,"*%21.15g*",a[j][i]);
	  }
	  else if(i==nrgoodi && j==nrgoodj){
	    dualfprintf(fail_file,"?%21.15g?",a[j][i]);
	  }
	  else{
	    dualfprintf(fail_file," %21.15g ",a[j][i]);
	  }
	}
	else{
	  dualfprintf(fail_file,"%21s"," ");
	}
	if(j==lasti) dualfprintf(fail_file,"\n");
	else dualfprintf(fail_file," ");
      }
      if(i==lasti) dualfprintf(fail_file,"\n");
    }
    dualfprintf(fail_file,"final error=%21.15g goodhh=%21.15g ans=%21.15g\n",err,goodhh,ans);
    dualfprintf(fail_file,"NR error=%21.15g goodhh=%21.15g ans=%21.15g\n",nrerr,nrgoodhh,nrans);
    dualfprintf(fail_file,"END DFRIDR table for i=%d j=%d k=%d ii=%d jj=%d kk=%d\n",ptrgeom->i,ptrgeom->j,ptrgeom->k,ii,jj,kk);
#endif







#if(USEJONEXTENSION==0)
    break;
#else

    ///////////
    //
    // Entire below section is new JCM additions
    //
    ///////////

    //////////////////////
    //	  
    // now check error is not crazy with the starting large h, decrease h if crazy until not crazy and get good error
    //
    //////////////////////

    errlist[iter]=err; // store final error from NR method
    //    hhlist[iter]=hstart;
    hhlist[iter]=goodhh; // current hh to focus around
    anslist[iter]=ans; // store result

    if(err>TRYTOL){ // TRYTOL is error we are attempting to reach

      if(errlist[iter]<minerror){
	// store min error event
	minerror=errlist[iter];
	minerrorhstart=hhlist[iter];
	miniter=iter;
	minans=ans;
#if(DEBUGDF)
	dualfprintf(fail_file,"minerr=%21.15g minhstart=%21.15g miniter=%d minans=%21.15g\n",minerror,minerrorhstart,miniter,minans);
#endif
      }
      else{
	// if did no better through bisecting hhgood, then probably done
	ans=minans;
#if(DEBUGDF)
	if(iter==1) dualfprintf(fail_file,"Done with no better error (iter=%d)\n",iter);
	else dualfprintf(fail_file,"Did better on multiple iterations (iter=%d)\n",iter);
#endif
	break;
      }

      // if here, then not done yet with bisecting on hh
      // try h around goodh, but narrow down the factor by which hh changes so don't skip over better hh than goodhh
      // NTAB entries, and want to go back to prior hh before goodhh, but need to cross to goodhh/truecon
      // so change on truecon is fixed to be:
      FTYPE htop,hbottom;
      htop=goodhh*truecon;
      hbottom=goodhh/truecon;
      truecon= pow(hbottom,-1.0/NTAB) * pow(htop,1.0/NTAB);
      truecon2=truecon*truecon; // reset truecon2
      // now set new hstart:
      hstart=htop;
      // so start at htop and will exponentially approach hbottom after NTAB entries.
#if(DEBUGDF)
      dualfprintf(fail_file,"iter=%d err=%21.15g goodhh=%21.15g lasti=%d truecon=%21.15g htop=%21.15g hbottom=%21.15g\n",iter,err,goodhh,lasti,truecon,htop,hbottom);
#endif

      if(truecon==1.0){
	ans=minans;
	break; // can't do any better
      }

    }// end if err > TRYTOL
    else break; // then error<TRYTOL!  GOOD!




    iter++;
    if(iter>=MAXITER){
      if(err<OKTOL) break;
      else{ // then maybe problems
	if(minerror<OKTOL){
	  ans=minans;
	  break;	      // then accept as answer
	}
	else{
	  // then must fail
	  dualfprintf(fail_file,"iter>MAXITER: never found error below %21.15g: err=%21.15g : ii=%d jj=%d kk=%d\n",OKTOL,err,ii,jj,kk);
	  dualfprintf(fail_file,"gi=%d gj=%d gk=%d\n",ptrgeom->i,ptrgeom->j,ptrgeom->k);
	  dualfprintf(fail_file,"miniter=%d errlist[miniter]=%21.15g hhlist[miniter]=%21.15g\n",miniter,errlist[miniter],hhlist[miniter]);
	  dualfprintf(fail_file,"minerror=%21.15g minans=%21.15g\n",minerror,minans);
	  for(iterdebug=0;iterdebug<iter;iterdebug++){
	    dualfprintf(fail_file,"h[%d]=%21.15g err[%d]=%21.15g ans[%d]=%21.15g\n",iterdebug,hhlist[iterdebug],iterdebug,errlist[iterdebug],iterdebug,anslist[iterdebug]);
	  }
	  myexit(67);
	}
      }      
    }
#endif // end JCM addition



  }// end while loop


  // done
  free_dmatrix(a,1,NTAB,1,NTAB);


  if(err<OKTOL){
    // then good, nothing to report
  }
  else{
    if(debugfail>=2) dualfprintf(fail_file,"Bad NUMREC error at i=%d j=%d k=%d ii=%d jj=%d kk=%d error=%21.15g ans=%21.15g :: iter=%d lasti=%d minerrorhstart=%21.15g firsthstart=%21.15g\n",ptrgeom->i,ptrgeom->j,ptrgeom->k,ii,jj,kk,err,ans,iter,lasti,minerrorhstart,firsthstart);
  }

  return ans;

       

}
#undef CON
#undef CON2
#undef BIG
#undef NTAB
#undef SAFE
#undef NRANSI
// (C) Copr. 1986-92 Numerical Recipes Software *1.@Q.. 






/* 
   FTYPE delta(int i, int j) { if(i == j) return(1.) ; else return(0.) 
   ; } */

/* Minkowski metric; signature +2 */
/* 
   FTYPE mink(int i, int j) { if(i == j) { if(i == 0) return(-1.) ;
   else return(1.) ; } else return(0.) ; } */





// continuous mod function like Mathematica
int contmod(int x, int y)
{
  int ans;

  ans = x%y;
  if(ans<0) ans+=y;

  return(ans);
}

// below 2 mysin/mycos preserve symmetry for arg=-M_PI/2 .. 2M_PI by restricting arg to 0..PI/2
FTYPE mysin(FTYPE th)
{
  int contmod(int x, int y);
  int nmod4;
  FTYPE ans;

#if(ACCURATESINCOS)
  nmod4=contmod((int)(th/(M_PI*0.5)),4);
  
  switch(nmod4){
  case 0:
    ans=sin(th);
    break;
  case 1:
    ans=sin(M_PI-th);
    break;
  case 2:
    ans=-sin(th-M_PI);
    break;
  case 3:
    ans=-sin(2.0*M_PI-th);
    break;
  default:
    dualfprintf(fail_file,"No such case for mysin with nmod4=%d\n",nmod4);
    ans=sqrt(-1.0);
    myexit(206983462);
    break;
  }
#else
  ans=sin(th);
#endif

  return(ans);

}


FTYPE mycos(FTYPE th)
{
  

#if(ACCURATESINCOS)
  return(mysin(th+M_PI*0.5));
#else
  return(cos(th));
#endif

}



//#ifdef WIN32
// cot = 1/tan = cos/sin
FTYPE cot(FTYPE arg)
{
#if(ACCURATESINCOS)
  return(mycos(arg)/mysin(arg));
#else
  return(1.0/tan(arg));
#endif
}
//#endif 

FTYPE csc(FTYPE arg)
{
#if(ACCURATESINCOS)
  return(1.0/mysin(arg));
#else
  return(1.0/sin(arg));
#endif
}


FTYPE sec(FTYPE arg)
{
#if(ACCURATESINCOS)
  return(1.0/mycos(arg));
#else
  return(1.0/cos(arg));
#endif
}




// fromwhere==0 or 1, then assume horizoni on downside of actual location
// fromwhere==2, then assume horizoni on upside of actual location
int find_horizon(int fromwhere)
{
  int i, j, k, ii;
  FTYPE r1, r2;
  FTYPE X[NDIM],V[NDIM];
  int gotit;
  FTYPE horizonvalue;
  // called after grid is setup for all cpus


  // first compute where horizon is

  // these 2 below are used prior, but not initialized otherwised on restart
  // some calculations
  // these 2 below are also used by find_horizon() below
  Rhor=rhor_calc(0);
  Risco=rmso_calc(PROGRADERISCO);


  // need to find horizon and place horizoni on right-hand-side of location



  // was testing to make sure if held horizoni fixed that conserved mass if using conservation of baryon number as basis for masses in Mvsr and accretion of energy
  // GODMARK DEBUG DEBUG DEBUG
  //  if(fromwhere!=2){
  //  return(0);
  // }


  // definition of horizoni must be consistent so fluxes are consistent and have conservation
  fromwhere=2; // force to be on upside unless Rhor=0, which is caught first





  if(fromwhere==0) trifprintf("begin: find_horizon ... ");

  // find cpu column that brackets the horizon and determine the
  // i-offset of horizon
  // notice that only 1 CPU will get horizon since stop process once found
  // notice that radius(horizoni) is below or equal to actual horizon radius

  


  horizonvalue = Rhor;
  horizoni = -100;
  horizoncpupos1=-1;
  gotit = 0;
  for (ii = numprocs - 1; ii >= 0; ii--) { // should get done by first row
    if (ii == myid) {
      for (i = N1-1; i >= 0; i--) {


	j = N2 / 2;             // doesn't matter (spherical polar assumed)
	k = N3 / 2;             // doesn't matter (spherical polar assumed)
	coord_ijk(i, j, k, FACE1, X);
	bl_coord_ijk(i, j, k, FACE1, V);
	r1=V[1];
	coord_ijk(ip1mac(i), j, k, FACE1, X);
	bl_coord_ijk(ip1mac(i), j, k, FACE1, V);
	r2=V[1];
	// looking between FACE1's r value and upper FACE1's r value, so loop is from i=N1-1..i=0

	if(ii==myid && myid==0 && i==0){
	  // special check in case horizon inside inner-most radial grid
	  if(horizonvalue<=r1 || horizonvalue<SMALL){ // GODMARK: this means horizon can't be chosen to <SMALL and mean there is a black hole there
	    // then horizon off grid or right on edge, but still ok
	    // treat as if horizon is off grid if right on edge
	    horizoni = 0;
	    horizoncpupos1=mycpupos[1];
	    break;
	  }
	}


	//        if (fabs(r1 - horizonvalue) <= (r2 - r1)) {     // find horizon
	if (fromwhere!=2){
	  if(horizonvalue >= r1 && horizonvalue < r2){ // note that if strictly on r2, then next CPU should pick it up
	    horizoni = i;
	    horizoncpupos1 = mycpupos[1];
	    break;
	  }
	}
	else if (fromwhere==2){
	  if(horizonvalue >= r1 && horizonvalue < r2){
	    horizoni = ip1mac(i);
	    horizoncpupos1 = mycpupos[1];
	    if(horizoni>=N1){
	      horizoni=0;
	      horizoncpupos1++;
	    }
	    else{
	      // then on original CPU
	      horizoncpupos1 = mycpupos[1];
	    }
	    //dualfprintf(fail_file,"horizon: %d %d\n",horizoni,horizoncpupos1);
	    break;
	  }
	  //	  dualfprintf(fail_file,"horizonnot: %d %d :: %21.15g %21.15g %21.15g\n",horizoni,horizoncpupos1,r1,Rhor,r2);
	}
      }
    }

    if (numprocs > 0) {
#if(USEMPI)
      MPI_Bcast(&horizoni, 1, MPI_INT, MPIid[ii], MPI_COMM_GRMHD);
      MPI_Bcast(&horizoncpupos1, 1, MPI_INT, MPIid[ii], MPI_COMM_GRMHD);
#endif
    }
    if (horizoni >= 0) gotit = 1;                // can stop entire process

    // keep horizoni as relative to CPU with horizon so any CPU knows where horizon is
    //    if (mycpupos[1] != horizoncpupos1) {
    //  horizoni = -100;
    //}                           // reset if not right cpu group
    if (gotit) break;
  }




  if(gotit==0){
    dualfprintf(fail_file,"Never found horizon: fromwhere=%d :: MBH=%21.15g a=%21.15g :: Rhor=%21.15g Risco=%21.15g\n",fromwhere,MBH,a,Rhor,Risco);
    myexit(6246);
  }


  /////////////////////////////////
  //
  // report some information
  if(fromwhere==0) {
    trifprintf("horizoni: %d horizoncpupos1: %d\n", horizoni, horizoncpupos1);
    // just a check
    dualfprintf(log_file,"horizoni: %d mycpupos[1]: %d horizoncpupos1: %d\n", horizoni, mycpupos[1], horizoncpupos1);
    
    trifprintf("end: find_horizon\n");
  }


  return(0);
}











int find_RinRout(FTYPE *localRin, FTYPE *localRout)
{
  FTYPE X[NDIM],V[NDIM];
  int whichcpu;


  // assume outer radius is on outer CPU
  // only 1 CPU needs to get this
  whichcpu=ncpux1-1;

  if(myid==whichcpu){

    coord_ijk(N1, 0, 0, FACE1, X);
    bl_coord_ijk(N1, 0, 0, FACE1, V);
    *localRout=V[1];
  }

  if (numprocs > 0) {
#if(USEMPI)
    MPI_Bcast(localRout, 1, MPI_FTYPE, MPIid[whichcpu], MPI_COMM_GRMHD);
#endif
  }


  // assume inner radius is on inner CPU
  // only 1 CPU needs to get this
  whichcpu=0;

  if(myid==whichcpu){

    coord_ijk(0, 0, 0, FACE1, X);
    bl_coord_ijk(0, 0, 0, FACE1, V);
    *localRin=V[1];
  }

  if (numprocs > 0) {
#if(USEMPI)
    MPI_Bcast(localRin, 1, MPI_FTYPE, MPIid[whichcpu], MPI_COMM_GRMHD);
#endif
  }


  return(0);
}








// get dr(i=0)
// assumes black hole is at r=0
void set_drsing(void)
{
  FTYPE dxdxp[NDIM][NDIM];
  FTYPE V[NDIM],X[NDIM];
  FTYPE dr;

  // assume BH r=0 is at inner radial boundary
  if(mycpupos[1]==0){
    coord_ijk(0,0,0,CENT,X);
    bl_coord_ijk(0,0,0,CENT,V);
    dxdxprim_ijk(0,0,0,CENT,dxdxp);
    
    dr = (dxdxp[1][1]*dx[1] + dxdxp[1][2]*dx[2])/10.0; // divide by 10 so doesn't dominate

    //    dualfprintf(fail_file,"%21.15g %21.15g %21.15g %21.15g\n",dxdxp[1][1],dx[1],dxdxp[1][2],dx[2]);
  }
  else dr=0;

#if(USEMPI)
  MPI_Allreduce(&dr, &drsing,1, MPI_FTYPE, MPI_MAX,MPI_COMM_GRMHD);
#else
  drsing=dr;
#endif

}

// get 1-D line over all CPUs that has the radius (only applicable if r(x_1) and not r(x_1,x_2)
void set_rvsr(void)
{
  FTYPE X[NDIM],V[NDIM];
  int i,j,k;
  int ii;
  FTYPE r;


  //////////////////////////////////////
  //
  // get radius for this CPU

  // initialize full rcent for all CPUs, choosing value so that can use MAX over all CPUs and get answer we want
  GRAVLOOP(ii) rcent[ii]=-1E30;

  j=0; // assumes j==0 has same radial dependence as all other j (true if r(x_1))
  k=0; // as with j
  COMPLOOPFP11{
    coord_ijk(i, j, k, CENT, X);
    bl_coord_ijk(i, j, k, CENT, V);
    r=V[1];
    ii=startpos[1]+i;
    rcent[ii] = r;
  }

  // send information to myid=0 since only this processor needs rcent
#if(USEMPI)
  MPI_Reduce(&(rcent[-N1BND]),&(rcent_tot[-N1BND]),NUMGRAVPOS,MPI_FTYPE,MPI_MAX,MPIid[0], MPI_COMM_GRMHD);
  //  MPI_Reduce(rcent,rcent_tot,ncpux1*N1,MPI_FTYPE,MPI_MAX,MPIid[0], MPI_COMM_GRMHD);
#else
  GRAVLOOP(ii) rcent_tot[ii]=rcent[ii];
#endif


}


// determinant not simply transformed from analytic function -- so no analytic form possible yet
FTYPE gdet_func_metric(int whichcoord, FTYPE *V,FTYPE *gcov)
{
  int jj,kk;
  FTYPE generalmatrixlower[NDIM][NDIM];

  // copy to full 2D space for NR functions
  DLOOP(jj,kk) generalmatrixlower[jj][kk] = gcov[GIND(jj,kk)];

  return(gdet_func_singcheck(whichcoord, V,generalmatrixlower));



}

// determinant not simply transformed from analytic function -- so no analytic form possible yet
FTYPE gdet_func_singcheck(int whichcoord, FTYPE *V,FTYPE (*generalmatrixlower)[NDIM])
{
  FTYPE gdet_func_orig(int whichcoord,FTYPE (*generalmatrixlower)[NDIM]);
  FTYPE gdet;



  gdet=gdet_func_orig(whichcoord, generalmatrixlower);


#if(FLIPGDETAXIS)
  if(ISSPCMCOORD(whichcoord)){
    if(V[2]<0.0) gdet*=-1.0;
    if(V[2]>M_PI) gdet*=-1.0;
  }
#endif


  return(gdet);

}

// find determinant in general of a metric
/* assumes gcov has been set first; returns determinant */

// determinant not simply transformed from analytic function -- so no analytic form possible yet
FTYPE gdet_func_orig(int whichcoord, FTYPE (*generalmatrixlower)[NDIM])
{
  FTYPE d;
  int j, k, indx[NDIM];
  int singfix;
  int anglesing,centersing,truedim;
  FTYPE finalvalue;



#if(USEOPENMP)
  // maintain thread safety
  FTYPE **tmp;
  tmp = dmatrix(1, NDIM, 1, NDIM);
#else
  static int firstc = 1;
  static FTYPE **tmp;
  if (firstc) {
    tmp = dmatrix(1, NDIM, 1, NDIM);
    firstc = 0;
  }
#endif


  singfix=0;

#if(1)
  // check for coordinate singularity (using this avoids bad input to ludcmp())
  metric_sing_check(whichcoord, generalmatrixlower, &anglesing, &centersing, &truedim);
#else
  truedim=NDIM;
#endif




  DLOOP(j,k) tmp[j + 1][k + 1] = generalmatrixlower[j][k];


  if(ludcmp(tmp, truedim, indx - 1, &d)>=1){
    if(debugfail>=2) dualfprintf(fail_file,"ludcmp failure in gdet_func_orig\n");
#if(1)
    if(ISSPCMCOORD(whichcoord)){
      // super hack
      if(debugfail>=2) dualfprintf(fail_file,"Assuming on polar axis\n");
      singfix=1;
    }
    else{
      dualfprintf(fail_file,"ludcmp failure 2: whichcoord=%d\n",whichcoord);
      myexit(3477);
    }
#else
    dualfprintf(fail_file,"ludcmp failure: Can't assume on polar axis for whichcoord=%d\n",whichcoord);
    myexit(246);
#endif

  }

  if(singfix==0 || truedim<NDIM){
    // below from 1..NDIM due to ludcmp requiring 1..N
    for (j = 1; j <= NDIM; j++) d *= tmp[j][j];

    if(d>0.0 && d<SMALL){
      finalvalue=0.0;
    }
    else if(d>0.0){
      dualfprintf(fail_file,"Metric has bad signature: d=%21.15g\n",d);
      DLOOP(j,k) dualfprintf(fail_file,"generalmatrixlower[%d][%d]=%21.15g\n",j,k,generalmatrixlower[GIND(j,k)]);
      myexit(3478);
    }
    else{
      finalvalue=sqrt(-d);
    }
  }
  else{
    finalvalue=0.0;
  }


#if(USEOPENMP)
  // maintain thread safety
  free_dmatrix(tmp, 1, NDIM, 1, NDIM);
#endif


  return(finalvalue);
}




// t-r inverse only (for r=0 coordinate singularity)
// NDIM in size, but invert submatrix that's 2x2 in size
void matrix_inverse_2d(FTYPE (*genmatrixlower)[NDIM], FTYPE (*genmatrixupper)[NDIM])
{
  int jj,kk;

  DLOOP(jj,kk) genmatrixupper[jj][kk] = 0.0;


  genmatrixupper[TT][TT] = 1.0/(-genmatrixlower[TT][RR]*genmatrixlower[TT][RR]/genmatrixlower[RR][RR] + genmatrixlower[TT][TT]);

  genmatrixupper[TT][RR] = genmatrixupper[RR][TT] = -genmatrixlower[TT][RR]/(-genmatrixlower[TT][RR]*genmatrixlower[TT][RR] + genmatrixlower[RR][RR]*genmatrixlower[TT][TT]);

  genmatrixupper[RR][RR] = 1.0/(-genmatrixlower[TT][RR]*genmatrixlower[TT][RR]/genmatrixlower[TT][TT] + genmatrixlower[RR][RR]);

}

// t-r-\theta inverse only (for \theta={0,\pi} coordinate singularities)
// NDIM in size, but invert submatrix that's 3x3 in size
void matrix_inverse_3d(FTYPE (*genmatrixlower)[NDIM], FTYPE (*genmatrixupper)[NDIM])
{
  int jj,kk;
  FTYPE genmatrixlowerrhsq,genmatrixlowertrsq,genmatrixlowerthsq;


  DLOOP(jj,kk) genmatrixupper[jj][kk] = 0.0;


  genmatrixlowerrhsq = genmatrixlower[RR][TH]*genmatrixlower[RR][TH];
  genmatrixlowertrsq = genmatrixlower[TT][RR]*genmatrixlower[TT][RR];
  genmatrixlowerthsq = genmatrixlower[TT][TH]*genmatrixlower[TT][TH];
  
  genmatrixupper[TT][TT]=(genmatrixlowerrhsq - genmatrixlower[TH][TH]*genmatrixlower[RR][RR])/(genmatrixlower[RR][RR]*genmatrixlowerthsq - 2.0*genmatrixlower[RR][TH]*genmatrixlower[TT][TH]*genmatrixlower[TT][RR] + genmatrixlower[TH][TH]*genmatrixlowertrsq + genmatrixlowerrhsq*genmatrixlower[TT][TT] - genmatrixlower[TH][TH]*genmatrixlower[RR][RR]*genmatrixlower[TT][TT]);
  genmatrixupper[TT][RR]=genmatrixupper[RR][TT]=(-(genmatrixlower[RR][TH]*genmatrixlower[TT][TH]) + genmatrixlower[TH][TH]*genmatrixlower[TT][RR])/(-2.0*genmatrixlower[RR][TH]*genmatrixlower[TT][TH]*genmatrixlower[TT][RR] + genmatrixlower[TH][TH]*genmatrixlowertrsq + genmatrixlowerrhsq*genmatrixlower[TT][TT] + genmatrixlower[RR][RR]*(genmatrixlowerthsq - genmatrixlower[TH][TH]*genmatrixlower[TT][TT]));
  genmatrixupper[TT][TH]=genmatrixupper[TH][TT]=(genmatrixlower[RR][RR]*genmatrixlower[TT][TH] - genmatrixlower[RR][TH]*genmatrixlower[TT][RR])/(genmatrixlower[RR][RR]*genmatrixlowerthsq - 2.0*genmatrixlower[RR][TH]*genmatrixlower[TT][TH]*genmatrixlower[TT][RR] + genmatrixlower[TH][TH]*genmatrixlowertrsq + genmatrixlowerrhsq*genmatrixlower[TT][TT] - genmatrixlower[TH][TH]*genmatrixlower[RR][RR]*genmatrixlower[TT][TT]);
  genmatrixupper[RR][RR]=(genmatrixlowerthsq - genmatrixlower[TH][TH]*genmatrixlower[TT][TT])/(genmatrixlower[RR][RR]*genmatrixlowerthsq - 2.0*genmatrixlower[RR][TH]*genmatrixlower[TT][TH]*genmatrixlower[TT][RR] + genmatrixlower[TH][TH]*genmatrixlowertrsq + genmatrixlowerrhsq*genmatrixlower[TT][TT] - genmatrixlower[TH][TH]*genmatrixlower[RR][RR]*genmatrixlower[TT][TT]);
  genmatrixupper[RR][TH]=genmatrixupper[TH][RR]=(-(genmatrixlower[TT][TH]*genmatrixlower[TT][RR]) + genmatrixlower[RR][TH]*genmatrixlower[TT][TT])/(-2.0*genmatrixlower[RR][TH]*genmatrixlower[TT][TH]*genmatrixlower[TT][RR] + genmatrixlower[TH][TH]*genmatrixlowertrsq + genmatrixlowerrhsq*genmatrixlower[TT][TT] + genmatrixlower[RR][RR]*(genmatrixlowerthsq - genmatrixlower[TH][TH]*genmatrixlower[TT][TT]));
  genmatrixupper[TH][TH]=(genmatrixlowertrsq - genmatrixlower[RR][RR]*genmatrixlower[TT][TT])/(genmatrixlower[RR][RR]*genmatrixlowerthsq - 2.0*genmatrixlower[RR][TH]*genmatrixlower[TT][TH]*genmatrixlower[TT][RR] + genmatrixlower[TH][TH]*genmatrixlowertrsq + genmatrixlowerrhsq*genmatrixlower[TT][TT] - genmatrixlower[TH][TH]*genmatrixlower[RR][RR]*genmatrixlower[TT][TT]);


}


// wrapper for symmetric matrix
void matrix_inverse_metric(int whichcoord, FTYPE *gcov, FTYPE *gcon)
{
  FTYPE genmatrixlower[NDIM][NDIM];
  FTYPE genmatrixupper[NDIM][NDIM];
  int jj,kk;

  // copy both in case both needed for some reason
  DLOOP(jj,kk) genmatrixlower[jj][kk]=gcov[GIND(jj,kk)];
  DLOOP(jj,kk) genmatrixupper[jj][kk]=gcon[GIND(jj,kk)];

  matrix_inverse(whichcoord, genmatrixlower, genmatrixupper);

  // translate back
  DLOOP(jj,kk) gcon[GIND(jj,kk)]=genmatrixupper[jj][kk];

}

/* invert genmatrixlower to get genmatrixupper */
// can be used to invert any 2nd rank tensor (symmetric or not)
// actually returns the inverse transpose, so if
// genmatrixlower=T^j_k then out pops (iT)^k_j such that T^j_k (iT)^k_l = \delta^j_l
void matrix_inverse(int whichcoord, FTYPE (*genmatrixlower)[NDIM], FTYPE (*genmatrixupper)[NDIM])
{
  int pl,pliter;
  int j, k;
  int anglesing,centersing,truedim;
  void metric_sing_check(int whichcoord, FTYPE (*genmatrixlower)[NDIM], int *anglesing, int*centersing, int *truedim);
  void matrix_inverse_2d(FTYPE (*genmatrixlower)[NDIM], FTYPE (*genmatrixupper)[NDIM]);
  void matrix_inverse_3d(FTYPE (*genmatrixlower)[NDIM], FTYPE (*genmatrixupper)[NDIM]);  


#if(USEOPENMP)
  // maintain thread safety
  FTYPE **tmp;
  tmp = dmatrix(1, NDIM, 1, NDIM);
#else
  static int firstc = 1;
  static FTYPE **tmp;
  if (firstc) {
    tmp = dmatrix(1, NDIM, 1, NDIM);
    firstc = 0;
  }
#endif




  DLOOP(j,k) tmp[j + 1][k + 1] = genmatrixlower[j][k];
  
#if(1)
  // check for singularities
  // only truedim used
  // allow avoiding of gaussj fail as detection of singularity
  metric_sing_check(whichcoord, genmatrixlower, &anglesing, &centersing, &truedim);
  //  dualfprintf(fail_file,"anglesing=%d centersing=%d truedim=%d\n",anglesing,centersing,truedim);
#else
  truedim=NDIM;
#endif

  // 0-out all genmatrixupper
  DLOOP(j,k) genmatrixupper[j][k]=0.0;


  //  DLOOP(j,k) dualfprintf(fail_file,"tmp[%d][%d]=%21.15g\n",j+1,k+1,tmp[j+1][k+1]);

  // GODMARK: Feeding in nan's results in gaussj segfaulting with erroneous access.  Seems bad behavior!
  if(gaussj(tmp, truedim, NULL, 0)){
    // then singular
#if(0) // new singularity check before if(gaussj) should work
    if(ISSPCMCOORD(whichcoord)){
      // super hack
      //if(centersing)  matrix_inverse_2d(genmatrixlower,genmatrixupper);
      //      else if(anglesing) matrix_inverse_3d(genmatrixlower,genmatrixupper);
      matrix_inverse_2d(genmatrixlower,genmatrixupper);
    }
    else{
      dualfprintf(fail_file,"whichcoord=%d\n",whichcoord);
      myexit(6243);
    }
#else
    dualfprintf(fail_file,"Singularity check didn't work\n");
    dualfprintf(fail_file,"whichcoord=%d anglesing=%d centersing=%d truedim=%d\n",whichcoord,anglesing,centersing,truedim);
    DLOOP(j,k) dualfprintf(fail_file,"inputmatrix[%d][%d]=%21.15g\n",j,k,genmatrixlower[j][k]);
    myexit(2714);
#endif

  }
  else{
    // assign but also transpose (shouldn't do in general, confusing)
    //DLOOP(j,k) genmatrixupper[j][k] = tmp[k + 1][j + 1];
    DLOOP(j,k) genmatrixupper[j][k] = tmp[j + 1][k + 1];
  }


#if(1) // check for nan's
  DLOOP(j,k) if(!finite(genmatrixupper[j][k])){
    dualfprintf(fail_file,"Came out of matrix_inverse with inf/nan for genmatrixupper at j=%d k=%d\n",j,k);
    myexit(5);
  }
#endif




#if(USEOPENMP)
  // maintain thread safety
  free_dmatrix(tmp, 1, NDIM, 1, NDIM);
#endif



}





void metric_sing_check(int whichcoord, FTYPE (*genmatrixlower)[NDIM], int *anglesing, int*centersing, int *truedim)
{


  // check for singularity (always removes highest coordinates -- so can use same matrix inverse with lower dimension when on coordinate singularities)
  *anglesing=0;
  *centersing=0;
  if(ISSPCMCOORD(whichcoord)){
    if(fabs(genmatrixlower[PH][PH])<10.0*SMALL){
      *anglesing=1;
    }
    if(fabs(genmatrixlower[TH][TH])<10.0*SMALL){
      *centersing=1;
    }
    if(*centersing){
      *truedim=NDIM-2;
    }
    else if(*anglesing){
      *truedim=NDIM-1;
    }
    else *truedim=NDIM;
  }
  else *truedim=NDIM;


}






// compute the radius of the inner most stable circular orbit
FTYPE rmso_calc(int which)
{
  FTYPE rmso,Z1,Z2,sign ;
  FTYPE j;

  j=a/(MBH+SMALL); // so doesn't nan for MBH=0 since "a" should be "0" if MBH=0

  if(which==PROGRADERISCO) sign=1; else sign=-1;

  Z1 = 1. + pow(1. - j*j,1./3.)*(pow(1. + j,1./3.) +
                                 pow(1. - j, 1./3.)) ;
  Z2 = sqrt(fabs(3.*j*j + Z1*Z1)) ;
  rmso=3. + Z2-sign*sqrt(fabs(3. - Z1)*fabs(3. + Z1 + 2.*Z2)) ;

  return(MBH*rmso) ;
}

FTYPE uphi_isco_calc(int which,FTYPE rold)
{
  FTYPE uphi;
  FTYPE sign;
  FTYPE Z1,Z2;
  FTYPE j,rnew;
  
  rnew=rold/MBH;
  j=a/MBH;

  if(which==PROGRADERISCO) sign=1; else sign=-1;

  Z1=rnew*rnew-sign*2.*j*sqrt(rnew)+j*j;
  Z2=rnew*(rnew*rnew-3.*rnew+sign*2.*j*sqrt(rnew));

  uphi=sign*Z1/sqrt(Z2);

  return(MBH*uphi);

}

FTYPE rhor_calc(int which)
{
  FTYPE sign,rhor;
  FTYPE j,jsq,disc;
  
  j=a/(MBH+SMALL); // so doesn't nan for MBH=0 since "a" should be "0" if MBH=0
  jsq=j*j;

  if(which==0) sign=1; else sign=-1;

  disc=MAX(1.0 - jsq,0.0);
  
  

  rhor=1. +sign*sqrt(disc);

  return(rhor*MBH);
}



/////////////////////////////////////////////////////////////
// 
// below are independent of user choice of metric/coords/grid
// 
/////////////////////////////////////////////////////////////


// find the con/cov forms of the chosen metric
void gset(int getprim, int whichcoord, int i, int j, int k, struct of_geom *ptrgeom)
{
  // assumes loc=CENT
  int loc;

  loc=CENT;
  gset_genloc(getprim, whichcoord, i, j, k, loc, ptrgeom);

}

// find the con/cov forms of the chosen metric
// fills in information like get_geometry but for arbitrary metric not just PRIMECOORDS
// GODMARK: doesn't yet set igdet's
// NOTE:
// This function returns contents of ptrgeom when not necessarily internal coordinate system
// So this function should be quite similar to set_grid.c for that part setting ptrgeom contents, but really setting contents rather than looking them up
void gset_genloc(int getprim, int whichcoord, int i, int j, int k, int loc, struct of_geom *ptrgeom)
{
  FTYPE X[NDIM],V[NDIM];
  struct of_geom tempgeom;
  extern void assign_eomfunc(struct of_geom *geom, FTYPE *EOMFUNCNAME);
  void gcov_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcovpert);
  void gcon_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcon);

  FTYPE *gcovptr;
  FTYPE *gconptr;
  FTYPE *gcovpertptr;


  ptrgeom->i=i;
  ptrgeom->j=j;
  ptrgeom->k=k;
  ptrgeom->p=loc;


#if(GETGEOMUSEPOINTER==0 || NEWMETRICSTORAGE==1)
  // then ptrgeom->gcov,gcon,gcovpert are real memory spaces
  gcovptr=ptrgeom->gcov;
  gconptr=ptrgeom->gcon;
  gcovpertptr=ptrgeom->gcovpert;
#else
  // then need to use dummy pointer space that has real memory assigned
  gcovptr=ptrgeom->gengcov;
  gconptr=ptrgeom->gengcon;
  gcovpertptr=ptrgeom->gengcovpert;

  ptrgeom->gcov=ptrgeom->gengcov; // pointer
  ptrgeom->gcon=ptrgeom->gengcon; // pointer
  ptrgeom->gcovpert=ptrgeom->gengcovpert; // pointer
#endif


  if(whichcoord>=0){
    coord_ijk(i, j, k, loc, X);
    bl_coord_ijk(i, j, k, loc, V);
    gcov_func(ptrgeom,getprim,whichcoord,X,gcovptr,gcovpertptr);
    ptrgeom->gdet=gdet_func_metric(whichcoord,V,gcovptr); // must come after gcov_func() above
    gcon_func(ptrgeom,getprim,whichcoord,X,gcovptr,gconptr); // must come after gcov_func() above
    alphalapse_func(ptrgeom,getprim,whichcoord,X,gcovptr,gconptr,&(ptrgeom->alphalapse));
    betasqoalphasq_func(ptrgeom,getprim,whichcoord,X,gcovptr,gconptr,&(ptrgeom->betasqoalphasq));
    beta_func(ptrgeom,getprim,whichcoord,X,gcovptr,gconptr,ptrgeom->alphalapse,ptrgeom->beta);
    eomfunc_func(ptrgeom, getprim, whichcoord,X,&(ptrgeom->EOMFUNCMAC(0)));
    assign_eomfunc(ptrgeom,&(ptrgeom->EOMFUNCMAC(0))); // must come after assigning ptrgeom->g above (won't use eomfuncgen if WHICHEOM==WITHGDET)
#if(GDETVOLDIFF)
    // uses X,V (not det) from all locations
    gdetvol_func(ptrgeom,ptrgeom->gdet,&(ptrgeom->EOMFUNCMAC(0)),ptrgeom->gdetvol);
#endif
    set_igdet_old(ptrgeom); // full 1/gdet and 1/EOMFUNCMAC(pl)

  }
  else if(whichcoord==PRIMECOORDS){ // special case
    get_and_copy_geometry(i,j,k,loc,ptrgeom);
  }
  else{
    dualfprintf(fail_file,"gset(): no such whichcoord=%d\n",whichcoord);
    myexit(3466);
  }

}


//void SHOULDNOTREACHHEREEVERBUGYOUHAVE(void)
//{
//  exit(1);
//}

#define OPTMETRICLOOP 1 // whether to use highly optmized loop (assumes metric is symmetric)
#define COMPUTEPERTURBEDMETRIC 0 // GODMARK: NOT CORRECT RIGHT NOW, so do NOT do it

void gcov2gcovprim(struct of_geom *ptrgeom, FTYPE *X, FTYPE *V, FTYPE *gcov, FTYPE *gcovpert, FTYPE *gcovprim, FTYPE *gcovpertprim)
{
  int j, k, l, m;
  FTYPE dxdxp[NDIM][NDIM];
  FTYPE tmpgcov[SYMMATRIXNDIM];
  FTYPE ftemp1,ftemp2;
  int q;
  void transgcov(FTYPE *gcov, FTYPE (*dxdxp)[NDIM], FTYPE *gcovprim);
  

  // now take term by term:
  // g_{u v} = \vec{e_{\mu}}\cdot\vec{e_{\nu}} 
  //           * (dx/dx')_{mu} * (dx/dx')_{\nu} =
  //          \vec{e'_{\mu}}\cdot\vec{e'_{\nu}} 

  // dx/dx' where '=prim coords (i.e. nonuni coords)
  //  dualfprintf(fail_file,"gcov2gcovprim: i=%d p=%d\n",ptrgeom->i,ptrgeom->p);
  dxdxprim_ijk_2(ptrgeom,X,V,dxdxp);

#if(OPTMETRICLOOP==0)
  DLOOP(j,k){
    tmpgcov[GIND(j,k)] = 0.;
    for(l=0;l<NDIM;l++) for(m=0;m<NDIM;m++){
      // g_{mup nup} = g_{mu nu} T^mu_mup T^nu_nup
      // where T^mu_mup == dx^mu[BL]/dx^mup[KSP uni grid]
	tmpgcov[GIND(j,k)] += GINDASSIGNFACTOR(j,k)*gcov[GIND(l,m)] * dxdxp[l][j] * dxdxp[m][k];
    }
    // use tmpgcov since gcon might be same address as gconprim
    gcovprim[GIND(j,k)] = tmpgcov[GIND(j,k)];
  }
#else
  transgcov(gcov,dxdxp,gcovprim);
#endif

  //  DLOOP(j,k) dualfprintf(fail_file,"prim gcov[%d][%d]=%21.15g\n",j,k,gcov[GIND(j,k)]);
  //  DLOOP(j,k) dualfprintf(fail_file,"prim gcovprim[%d][%d]=%21.15g\n",j,k,gcovprim[GIND(j,k)]);
  //  DLOOP(j,k) dualfprintf(fail_file,"prim dxdxp[%d][%d]=%21.15g\n",j,k,dxdxp[j][k]);

  ///////////////////////////////
  //
  // perturbed terms
  //
  //////////////////////////////

#if(COMPUTEPERTURBEDMETRIC)
  // SUPERGODMARK GODMARK: This only works for non-rel gravity if dxdxp is diagonal.  Not sure what to do in general
  DLOOPA(q){
    //    dualfprintf(fail_file,"gcovpert[%d]=%21.15g\n",q,gcovpert[q]);


    // get q-q term
    // -1 + g_{q'q'} for q!=TT, else  SOMECONSTANT + g_{q'q'} for q=TT

    // for spatial parts, only care about deviations from constant in getting connection coefficients
    // avoids catastrophic cancellation (GODMARK)
    if( (q!=TT)&&(defcoord==UNIFORMCOORDS)) ftemp1=0.0;
    // then unsure how to handle in general, so just leave alone (SUPERGODMARK GODMARK)
    // problem is with machine error in dxdxp leading to apparently large connection coefficients due to that machine error
    // also don't know ahead of time what constant to choose for prim coordinate quantities
    // and don't know how to handle variations in dxdxp, since then no constant to subtract off, but grid accelerations apparently wash out non-rel gravity accelerations?
    // except for time term, which is solid
    else ftemp1=(-1.0 + dxdxp[q][q] * dxdxp[q][q]);
    if(q==TT) ftemp1 *=-1.0;
    gcovpertprim[q]  = (gcovpert[q] * dxdxp[q][q] * dxdxp[q][q]) + ftemp1;

    // now add 15 other terms
    ftemp2 = 0.;
    for(l=0;l<NDIM;l++) for(m=0;m<NDIM;m++){
      if((l!=q)&&(m!=q)) ftemp2+= gcov[GIND(l,m)] * dxdxp[l][q] * dxdxp[m][q];
    }
    // add other 15 terms to answer for total of 16 terms
    gcovpertprim[q]+=ftemp2;

    
    //    dualfprintf(fail_file,"dxdxp[%d][%d]=%21.15g\n",q,q,dxdxp[q][q]);
    //    dualfprintf(fail_file,"ftemp1[%d]=%21.15g ftemp2[%d]=%21.15g gcovpertprim[%d]=%21.15g\n",q,ftemp1,q,ftemp2,q,gcovpertprim[q]);
  }
#elif(0)
  // override for now
  gcovpertprim[TT]=gcovprim[GIND(TT,TT)]-mink(TT,TT);
  SLOOPA(q){
    gcovpertprim[q]=gcovprim[GIND(q,q)]-mink(q,q);
  }
#elif(1)
  // override for now
  gcovpertprim[TT]=gcovprim[GIND(TT,TT)]+1.0;
  gcovpertprim[RR]=gcovprim[GIND(RR,RR)]-1.0;
  gcovpertprim[TH]=gcovprim[GIND(TH,TH)]-1.0;
  gcovpertprim[PH]=gcovprim[GIND(PH,PH)]-1.0;
#endif

 

}

void transgcov_old(FTYPE *gcov, FTYPE (*dxdxp)[NDIM], FTYPE *gcovprim)
{
  int j, k, l, m;
  FTYPE tmpgcov[SYMMATRIXNDIM];

  DLOOP(j,k){
    tmpgcov[GIND(j,k)] = 0.;
    for(l=0;l<NDIM;l++) for(m=0;m<NDIM;m++){
      // g_{mup nup} = g_{mu nu} T^mu_mup T^nu_nup
      // where T^mu_mup == dx^mu[BL]/dx^mup[KSP uni grid]
      tmpgcov[GIND(j,k)] += gcov[GIND(l,m)] * dxdxp[l][j] * dxdxp[m][k];
    }
    // use tmpgcov since gcon might be same address as gconprim
    gcovprim[GIND(j,k)] = tmpgcov[GIND(j,k)];
  }

}

// gcov might be same memory address as gcovprim, so use tmp
// assumes metric is symmetric 2nd rank tensor
void transgcov(FTYPE *gcov, FTYPE (*dxdxp)[NDIM], FTYPE *gcovprim)
{
  int j, k, l, m;
  FTYPE tmp[NDIM][NDIM];

  // g_{\alpha \beta} = g_{\mu \nu} \Lambda^\mu_\alpha \Lambda^\nu_\beta


  /*
  // 4 along diagonal and 6 off-diagonal with 6 other identical values
#define GCOV_DOT_DXDXP_DOT_DXDXP(a,b)\
          gcov[GIND(0,0)] * dxdxp[0][a]* dxdxp[0][b]\
    +     gcov[GIND(1,1)] * dxdxp[1][a]* dxdxp[1][b]\
    +     gcov[GIND(2,2)] * dxdxp[2][a]* dxdxp[2][b]\
    +     gcov[GIND(3,3)] * dxdxp[3][a]* dxdxp[3][b]\
    + 2.0*gcov[GIND(0,1)] * dxdxp[0][a]* dxdxp[1][b]\
    + 2.0*gcov[GIND(0,2)] * dxdxp[0][a]* dxdxp[2][b]\
    + 2.0*gcov[GIND(0,3)] * dxdxp[0][a]* dxdxp[3][b]\
    + 2.0*gcov[GIND(1,2)] * dxdxp[1][a]* dxdxp[2][b]\
    + 2.0*gcov[GIND(1,3)] * dxdxp[1][a]* dxdxp[3][b]\
    + 2.0*gcov[GIND(2,3)] * dxdxp[2][a]* dxdxp[3][b]
  */


  // 4 along diagonal and 6 off-diagonal with 6 other identical values
#define GCOV_DOT_DXDXP_DOT_DXDXP(a,b)\
          gcov[GIND(0,0)] * dxdxp[0][a]* dxdxp[0][b]\
    +     gcov[GIND(1,1)] * dxdxp[1][a]* dxdxp[1][b]\
    +     gcov[GIND(2,2)] * dxdxp[2][a]* dxdxp[2][b]\
    +     gcov[GIND(3,3)] * dxdxp[3][a]* dxdxp[3][b]\
    +     gcov[GIND(0,1)] * (dxdxp[0][a]* dxdxp[1][b] + dxdxp[1][a]* dxdxp[0][b])\
    +     gcov[GIND(0,2)] * (dxdxp[0][a]* dxdxp[2][b] + dxdxp[2][a]* dxdxp[0][b])\
    +     gcov[GIND(0,3)] * (dxdxp[0][a]* dxdxp[3][b] + dxdxp[3][a]* dxdxp[0][b])\
    +     gcov[GIND(1,2)] * (dxdxp[1][a]* dxdxp[2][b] + dxdxp[2][a]* dxdxp[1][b])\
    +     gcov[GIND(1,3)] * (dxdxp[1][a]* dxdxp[3][b] + dxdxp[3][a]* dxdxp[1][b])\
    +     gcov[GIND(2,3)] * (dxdxp[2][a]* dxdxp[3][b] + dxdxp[3][a]* dxdxp[2][b])


  // first do 4 along diagonal
  DLOOPA(j) tmp[j][j] = GCOV_DOT_DXDXP_DOT_DXDXP(j,j);

  // now do 6 others off-diagonal
  j=0; k=1; tmp[j][k] = GCOV_DOT_DXDXP_DOT_DXDXP(j,k);
  j=0; k=2; tmp[j][k] = GCOV_DOT_DXDXP_DOT_DXDXP(j,k);
  j=0; k=3; tmp[j][k] = GCOV_DOT_DXDXP_DOT_DXDXP(j,k);
  j=1; k=2; tmp[j][k] = GCOV_DOT_DXDXP_DOT_DXDXP(j,k);
  j=1; k=3; tmp[j][k] = GCOV_DOT_DXDXP_DOT_DXDXP(j,k);
  j=2; k=3; tmp[j][k] = GCOV_DOT_DXDXP_DOT_DXDXP(j,k);
    

  // copy over result assuming tmp on upper-diagonal only, but filling gcovprim fully
  // 4 along diagonal (00,11,22,33) and 6 off-diagonal (01, 02, 03, 12, 13, 23), 6 more as copies of off-diagonal
  DLOOPA(j) gcovprim[GIND(j,j)] = tmp[j][j];
  gcovprim[GIND(0,1)]=gcovprim[GIND(1,0)]=tmp[0][1];
  gcovprim[GIND(0,2)]=gcovprim[GIND(2,0)]=tmp[0][2];
  gcovprim[GIND(0,3)]=gcovprim[GIND(3,0)]=tmp[0][3];
  gcovprim[GIND(1,2)]=gcovprim[GIND(2,1)]=tmp[1][2];
  gcovprim[GIND(1,3)]=gcovprim[GIND(3,1)]=tmp[1][3];
  gcovprim[GIND(2,3)]=gcovprim[GIND(3,2)]=tmp[2][3];

}


void gcon2gconprim(struct of_geom *ptrgeom, FTYPE *X, FTYPE *V, FTYPE *gcon,FTYPE *gconprim)
{
  int j, k, l, m;
  //FTYPE dxdxp[NDIM][NDIM],idxdxp[NDIM][NDIM];
  FTYPE idxdxp[NDIM][NDIM];
  FTYPE tmpgcon[SYMMATRIXNDIM];

  // see transforms.c and mettometp() and see gcov2gcovprim()
  idxdxprim_ijk_2(ptrgeom, X, V, idxdxp);

  //  dualfprintf(fail_file,"mi in gcon2gconprim\n");
  // PRIMECOORDS indicates no special considerations if fails to get inverse
  //  matrix_inverse(PRIMECOORDS, dxdxp,idxdxp);
  
  DLOOP(j,k) tmpgcon[GIND(j,k)] = 0.;
  DLOOP(j,k){
    for(l=0;l<NDIM;l++){
      for(m=0;m<NDIM;m++){
	tmpgcon[GIND(j,k)] += GINDASSIGNFACTOR(j,k)*idxdxp[j][l] * idxdxp[k][m] * gcon[GIND(l,m)] ;
      }
    }
  }
  // use tmpgcon since gcon might be same address as gconprim
  DLOOP(j,k) gconprim[GIND(j,k)] = tmpgcon[GIND(j,k)];

}








void setup_delta(int whichfun,int whichdifftype, FTYPE defaultdelta, struct of_geom *geom, struct of_geom (*localptrgeoml)[NDIM], struct of_geom (*localptrgeomh)[NDIM], FTYPE *truedelta)
{
  int jj;
  int localwhichdifftype;
  int N[NDIM];

  N[0]=2; // if evolving metric then 2, but doesn't matter since at CENT always
  N[1]=N1;
  N[2]=N2;
  N[3]=N3;



  if(whichfun==0 || whichfun==1){ // connection or dxdxp
    if(whichdifftype==DIFFGAMMIE || whichdifftype==DIFFNUMREC){
      localwhichdifftype=0; // infinitesimal
    }
    else if(whichdifftype==DIFFFINITE){
      localwhichdifftype=1; // finite
    }
  }
  else{
    dualfprintf(fail_file,"no such whichfun=%d for whichdifftype=%d",whichfun,whichdifftype);
    myexit(915);
  }



  if(localwhichdifftype==0){ // infinitesimal
    DLOOPA(jj){
      // specify NOWHERE so that won't use gridded position
      ((*localptrgeoml)[jj]).p=((*localptrgeomh)[jj]).p=NOWHERE;
      
      ((*localptrgeoml)[jj]).i=((*localptrgeomh)[jj]).i=(geom->i);
      ((*localptrgeoml)[jj]).j=((*localptrgeomh)[jj]).j=(geom->j);
      ((*localptrgeoml)[jj]).k=((*localptrgeomh)[jj]).k=(geom->k);
      
      truedelta[jj]=defaultdelta;
    }
#if(DOEVOLVEMETRIC)
    // if evolving metric, then use CENT and gcov_func() will use X[0] to choose if present time or not
    // GODMARK: assume didstoremetricdata==1 is set when presenttime==1 is set so doesn't reach bl_coord_ijk_2() and do coord(i,j,k...) that would be wrong
    jj=TT;
    ((*localptrgeoml)[jj]).p=((*localptrgeomh)[jj]).p=CENT; // when taking temporal differences, values are always spatially located at loc=CENT
#endif

  }
  else if(localwhichdifftype==1){ // finite
    // use gridded data
    // assumes connection at CENT
    if(geom->p==CENT){


      DLOOPA(jj){
	((*localptrgeoml)[jj]).p=((*localptrgeomh)[jj]).p=CENT + jj*(N[jj]!=1); // jj=0 -> CENT , jj=1 -> FACE1 , jj=2 -> FACE2 , jj=3 -> FACE3
	
	((*localptrgeoml)[jj]).i=(geom->i);
	((*localptrgeoml)[jj]).j=(geom->j);
	((*localptrgeoml)[jj]).k=(geom->k);
	
	// GODMARK: Note that infinitesimal version obtains correct connection even in reduced dimensions, while the finite version just reduces to no connection if reduced dimension (so this assumes something about the position or may not even be right in general)
	((*localptrgeomh)[jj]).i=(geom->i) + (jj==1 ? SHIFT1 : 0);
	((*localptrgeomh)[jj]).j=(geom->j) + (jj==2 ? SHIFT2 : 0);
	((*localptrgeomh)[jj]).k=(geom->k) + (jj==3 ? SHIFT3 : 0);
	
	truedelta[jj]=0.5*dx[jj];
      }
    }
    else{
      dualfprintf(fail_file,"No localptrgeom defined for p=%d\n",geom->p);
      myexit(916);
    }
  }
  else{
    dualfprintf(fail_file,"No such difftype=%d\n",localwhichdifftype);
    myexit(917);
  }

#if(0)
  dualfprintf(fail_file,"whichfun=%d whichdifftype=%d localwhichdifftype=%d\n",whichfun,whichdifftype,localwhichdifftype);
  DLOOPA(jj) dualfprintf(fail_file,"%d :: truedelta=%21.15g\n",jj,truedelta[jj]);
  DLOOPA(jj) dualfprintf(fail_file,"%d :: low: i=%d j=%d k=%d p=%d :: high: i=%d j=%d k=%d p=%d\n",jj,((*localptrgeoml)[jj]).i,((*localptrgeoml)[jj]).j,((*localptrgeoml)[jj]).k,((*localptrgeoml)[jj]).p,((*localptrgeomh)[jj]).i,((*localptrgeomh)[jj]).j,((*localptrgeomh)[jj]).k,((*localptrgeomh)[jj]).p);
#endif

}












/* 
   this gives the connection coefficient \Gamma^{i}_{j,k} =
   GLOBALMETMACP1A0(conn,..,i,j,k) where i,j,k = {0,1,2,3} corresponds to {t,r,theta,phi} 
*/

/*
  we also compute the 2nd connection:
  -d/dj(ln(\detg))
*/




/*

Based upon EOM:

$$
(f_{(\nu)} T^t_\nu)_{,t} 
= -(f_{(\nu)}T^j_\nu)_{,j} 
+  f_{(\nu)} [
     T^\lambda_\nu[\ln(f_{(\nu)}/\detg)]_{,\lambda} 
   + T^\mu_\lambda \Gamma^\lambda_{\mu\nu} 
   + \ln(f_{(\nu)})_{,t} T^t_\nu
]
$$

More compactly (JCM: 05/07/08):

$$
d_\mu (f T^\mu_\nu) = f[ T^\lambda_\kappa \Gamma^\kappa_{\nu\lambda} - d_\mu ln (\detg/f) T^\mu_\nu]
$$

SUPERGODMARK: for  if(WHICHEOM!=WITHGDET) don't yet compute correct conn2 I believe

// to avoid body forces in general, must compute (e.g. through correction):

$$
\Gamma^\kappa_{\nu\lambda}[new] = \Gamma^\kappa_{\nu\lambda}[old] - (1/4)[\Gamma^\alpha_{\nu\alpha} - ( (d_\nu f)/f + d_\nu \ln(\detg/f) )]\delta^\kappa_\lambda
$$

or:

$$
\Gamma^\kappa_{\nu\lambda} += - (1/4)[ \Gamma^\alpha_{\nu\alpha} - ( (d_\nu f)/f - conn2_\nu )]\delta^\kappa_\lambda
$$

For the WHICHEOM!=WITHGDET version, in general this would require a different \Gamma for each EOM.  For simplicity just assume f=\detg and don't allow this non-body version unless WHICHEOM==WITHGDET, and so then one has:

$$
\Gamma^\kappa_{\nu\lambda} += - (1/4)[ \Gamma^\alpha_{\nu\alpha} - (d_\nu \detg)/\detg ]\delta^\kappa_\lambda
$$

The above is only true for second order scheme.  For higher order FLUXRECON scheme one has:

$$
\Gamma^\kappa_{\nu\lambda} += - (1/4)[ \Gamma^\alpha_{\nu\alpha} - (d_\nu a2c_\nu \detg)/\detg ]\delta^\kappa_\lambda
$$

Problems:
1) But a2c_\nu would normally be chosen adaptively and so one wouldn't have good cancellation.
2) Fact that \detg is there means difficult to generally have cancellation otherwise unless use NOGDET
3) NOGDET not yet setup for EVOLVEMETRIC, but otherwise for spherical polar coordinates should just use NOGDET for r and \theta.
4) Alternatively, can perform correction every timestep and use same a2c as used for each flux -- expensive
   Problem is b^2/2 and p will be treated differently for SPLITMAEM unless constant all weights
5) So seems only solution apart from NOGDET is to use constant all weights that is unstable

*/



// if evolving in time, then metric changed from Xmetricold[TT] to now (t)
// by shifting time in past we tell metric to use old metric
// if t==Xmetricold[0], then feeding the below tells gcov to assume standard difference at present time
#define DELTAFORTIMEh(DELTA) (Xmetricnew[TT]!=Xmetricold[TT] ? (0.0) : 2.0*DELTA)
#define DELTAFORTIMEl(DELTA) (Xmetricnew[TT]!=Xmetricold[TT] ? (-(Xmetricnew[TT]-Xmetricold[TT])) : DELTA)

// finite volume form
// doesn't make sense for pole with singularity
//#define MYDELTAh(DELTA,k) ( k==TT ? DELTAFORTIMEh(DELTA) :  dx[k]*0.5 )
//#define MYDELTAl(DELTA,k) ( k==TT ? DELTAFORTIMEl(DELTA) : -dx[k]*0.5 )

// finite difference form
#define MYDELTAh(DELTA,k) ( k==TT ? DELTAFORTIMEh(DELTA) :  DELTA*Diffx[k] )
#define MYDELTAl(DELTA,k) ( k==TT ? DELTAFORTIMEl(DELTA) : -DELTA*Diffx[k] )


/* NOTE: parameter hides global variable */
// note that inputted geom is used not only for i,j,k but for actual CENT gcon, etc.
void conn_func_numerical1(FTYPE DELTA, FTYPE *X, struct of_geom *geom,
			  FTYPE (*conn)[NDIM][NDIM],FTYPE *conn2)
{
  int i, j, k, l;
  int kk,jj;
  FTYPE conndiag[NDIM],conndiag2[NDIM];
  FTYPE gdethgen[NDIM],gdetlgen[NDIM];
  FTYPE lngdethgen[NDIM],lngdetlgen[NDIM];
  //  FTYPE *gdeth,*gdetl;
  //  FTYPE *lngdeth,*lngdetl;
  FTYPE gdetmid;
  FTYPE tmp[NDIM][NDIM][NDIM];
  FTYPE Xhgen[NDIM][NDIM];
  FTYPE Xlgen[NDIM][NDIM];
  FTYPE signdXgen[NDIM];
  //  FTYPE *Xh, *Xl;
  //  FTYPE signdX;
  FTYPE V[NDIM];
  FTYPE gmid[SYMMATRIXNDIM];
  FTYPE ghgen[NDIM][SYMMATRIXNDIM];
  FTYPE glgen[NDIM][SYMMATRIXNDIM];
  //  FTYPE (*gh)[NDIM];
  //  FTYPE (*gl)[NDIM];
  FTYPE gcovpertmid[NDIM];
  FTYPE gcovperthgen[NDIM][NDIM],gcovpertlgen[NDIM][NDIM];
  //  FTYPE *gcovperth,*gcovpertl;
  void gcov_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcovpert);
  FTYPE dfridr(FTYPE (*func)(struct of_geom *,FTYPE*,int,int), struct of_geom *geom, FTYPE *X,int ii, int jj, int kk);
  FTYPE gcov_func_mcoord(struct of_geom *ptrgeom, FTYPE* X, int i, int j);
  FTYPE lngdet_func_mcoord(struct of_geom *ptrgeom, FTYPE* X, int i, int j);
  //
  // below variables used for setup_delta()
  // and used for defining whether infinitesimal or finite difference
  FTYPE truedelta[NDIM];
  struct of_geom localgeoml[NDIM];
  struct of_geom localgeomh[NDIM];
  struct of_geom (*localptrgeoml)[NDIM];
  struct of_geom (*localptrgeomh)[NDIM];
  struct of_geom localgeom;
  struct of_geom *localptrgeom=&localgeom;
  void setup_delta(int whichfun,int whichdifftype, FTYPE defaultdelta, struct of_geom *geom, struct of_geom (*localptrgeoml)[NDIM], struct of_geom (*localptrgeomh)[NDIM], FTYPE *truedelta);
  FTYPE gdet_func_mcoord_usegcov(FTYPE *gcovmcoord, struct of_geom *ptrgeom, FTYPE* X, int i, int j);
  int doingmachinebody;
  int setup_XlXh(FTYPE *X,FTYPE *truedelta, FTYPE (*Xlgen)[NDIM],FTYPE (*Xhgen)[NDIM],FTYPE *signdXgen);
  int compute_metricquantities_midlh(int donormal, int domachinebody
				     ,struct of_geom *geom, FTYPE *X, FTYPE *gmid, FTYPE *gcovpertmid,FTYPE *gdetmid,FTYPE *gdetlgen,FTYPE *gdethgen
				     ,struct of_geom (*localptrgeoml)[NDIM],struct of_geom (*localptrgeomh)[NDIM],FTYPE (*Xlgen)[NDIM],FTYPE (*Xhgen)[NDIM]
				     ,FTYPE *lngdetlgen, FTYPE *lngdethgen, FTYPE (*glgen)[SYMMATRIXNDIM], FTYPE (*ghgen)[SYMMATRIXNDIM], FTYPE (*gcovpertlgen)[NDIM], FTYPE (*gcovperthgen)[NDIM]
				     );





  // setup conditional
  doingmachinebody = CONNMACHINEBODY && WHICHEOM==WITHGDET;
  




  localptrgeoml=&localgeoml;
  localptrgeomh=&localgeomh;

  ///////////////////////////
  //
  // Setup delta
  //
  ///////////////////////////
  // 0 indicates connection type
  setup_delta(0,CONNDERTYPE,DELTA,geom,localptrgeoml,localptrgeomh,truedelta);



#if(DOEVOLVEMETRIC==1 && CONNDERTYPE==DIFFNUMREC)
  // no choice in sense that NUMREC is too slow and unstable
  dualfprintf(fail_file,"Not good idea to use DOEVOLVEMETRIC==1 with CONNDERTYPE==DIFFNUMREC\n");
  myexit(7225);
#endif



  ////////////////////////////////
  //
  // gabc_{ijk}=dg_{ij}/dx^k
  // gammie derivative (attempt to get analytical value) or finite difference (true finite difference with given cell size)
  //
  ////////////////////////////////
  if(CONNDERTYPE==DIFFGAMMIE || CONNDERTYPE==DIFFFINITE){


    // setup Xl and Xh and signdX
    setup_XlXh(X,truedelta,Xlgen,Xhgen,signdXgen);

    // get metric quantities
    compute_metricquantities_midlh(1,doingmachinebody
				   ,geom, X, gmid, gcovpertmid,&gdetmid,gdetlgen,gdethgen
				   ,localptrgeoml,localptrgeomh,Xlgen,Xhgen
				   ,lngdetlgen, lngdethgen, glgen, ghgen, gcovpertlgen, gcovperthgen
				   );


    ////////////
    //
    // Now compute derivatives
    //
    ////////////
    
    for (k = 0; k < NDIM; k++) {
      
      
      if(WHICHEOM!=WITHGDET){
	//$$
	//d_\mu (f T^\mu_\nu) = f[ T^\lambda_\kappa \Gamma^\kappa_{\nu\lambda} - d_\mu ln (\detg/f) T^\mu_\nu]
	//$$
	conn2[k]= signdXgen[k]*(lngdethgen[k] - lngdetlgen[k]) / (Xhgen[k][k] - Xlgen[k][k]);
      }
      else{
	conn2[k]=0.0; // no 2nd connection then
      }


      // answer is symmetric on i,j since uses g_{ij}, so only do part of work
      for (i = 0; i < NDIM; i++){
	for (j = 0; j <=i; j++){
	  // d(1+g_{tt}) -> dg_{tt}, so can use 1+g_{tt} for accurate non-relativistic gravity
	  //if(i==j) conn[i][j][k] = (gcovperth[i] - gcovpertl[i]) / (Xh[k] - Xl[k]);
	  // else 
	  conn[i][j][k] = signdXgen[k]*(ghgen[k][GIND(i,j)] - glgen[k][GIND(i,j)]) / (Xhgen[k][k] - Xlgen[k][k]);
	}
      }
      
    }// end over k


  }
  else if(CONNDERTYPE==DIFFNUMREC){
    // use local copy so don't overwrite original, which could be pointing to global storage
    localptrgeom->i=geom->i;
    localptrgeom->j=geom->j;
    localptrgeom->k=geom->k;
    localptrgeom->p=NOWHERE; // informs rest of calls that X will generally be arbitrary

    //    dualfprintf(fail_file,"DIFFNUMREC: doing i=%d j=%d\n",geom->i,geom->j);
    for (k = 0; k < NDIM; k++) {

      if(WHICHEOM!=WITHGDET){
	conn2[k]= dfridr(lngdet_func_mcoord,localptrgeom,X,0,0,k); // 0,0 not used
      }
      else conn2[k]=0.0; // then no 2nd connection

      // answer is symmetric on i,j since uses g_{ij}, so only do part of work
      for (i = 0; i < NDIM; i++){
	for (j = 0; j <=i; j++){
	  conn[i][j][k] = dfridr(gcov_func_mcoord,localptrgeom,X,i,j,k);
	}
      }

    }// end over k

  }// end if CONNDERTYPE==DIFFNUMREC


  ////////////////////////////////////////////////////
  //  
  // fill in rest of conn[i][j][k] (or enforce symmetry of connection)
  //
  ////////////////////////////////////////////////////
  for (k = 0; k < NDIM; k++) {
    for (i = 0; i < NDIM; i++){
      for (j = i+1; j <NDIM; j++){
	conn[i][j][k] = conn[j][i][k]; 
      }
    }
  }


  ////////////////////////////////////////////////////
  //
  // now rearrange to find \Gamma_{ijk}=1/2*(gabc_{jik}+gabc_{kij}-gabc_{kji})
  //
  ////////////////////////////////////////////////////
  for (i = 0; i < NDIM; i++)
    for (j = 0; j < NDIM; j++)
      for (k = 0; k < NDIM; k++)
	tmp[i][j][k] =
	  0.5 * (conn[j][i][k] + conn[k][i][j] - conn[k][j][i]);


  ////////////////////////////////////////////////////
  //
  // finally, raise first index
  //
  ////////////////////////////////////////////////////
  for (i = 0; i < NDIM; i++)
    for (j = 0; j < NDIM; j++)
      for (k = 0; k < NDIM; k++) {
	conn[i][j][k] = 0.;
	for (l = 0; l < NDIM; l++){
	  conn[i][j][k] += geom->gcon[GIND(i,l)] * tmp[l][j][k];
	}
      }


  //////////////////////////////////////////////////
  //
  // now correct for accurate body forces
  // only makes sense for no a2c on flux right now
  //
  //////////////////////////////////////////////////
  if(doingmachinebody){

    if(CONNDERTYPE!=DIFFFINITE){
      // Setup finite difference for correction
      // 0 indicates connection type
      // correction always uses DIFFFINITE
      setup_delta(0,DIFFFINITE,DELTA,geom,localptrgeoml,localptrgeomh,truedelta);

      // setup Xl and Xh and signdX
      setup_XlXh(X,truedelta,Xlgen,Xhgen,signdXgen);

      // 0 means not normal calculations      
      compute_metricquantities_midlh(0,doingmachinebody
				     ,geom, X, gmid, gcovpertmid,&gdetmid,gdetlgen,gdethgen
				     ,localptrgeoml,localptrgeomh,Xlgen,Xhgen
				     ,lngdetlgen, lngdethgen, glgen, ghgen, gcovpertlgen, gcovperthgen
				     );
      
    }

    //////////////////
    // form contracted connection
    DLOOPA(kk){
      conndiag[kk]=0.0;
      DLOOPA(jj) conndiag[kk] += conn[jj][kk][jj];
    }

    /////////////////
    // form differential \detg
    for (k = 0; k < NDIM; k++) {
      conndiag2[k] = signdXgen[k]*(gdethgen[k] - gdetlgen[k]) / (Xhgen[k][k] - Xlgen[k][k]);
      conndiag2[k] /= gdetmid;
    }

    //////////////////
    // now obtain correction 
    // for Xh-Xl->0 this vanishes as required
    // Plugging this new conn into EOM for T^\mu_\nu = p \delta^\mu_\nu gives exactly cancellation between source and flux differencing of pressure
    //    for (i = 0; i < NDIM; i++) for (j = 0; j < NDIM; j++) for (k = 0; k < NDIM; k++) conn[i][j][k] += -0.25*(conndiag[j] - conndiag2[j])*delta(i,k);
    // apply delta(i,k) directly by setting i=k
    for (i = 0; i < NDIM; i++) for (j = 0; j < NDIM; j++) conn[i][j][i] += -0.25*(conndiag[j] - conndiag2[j]);

  } // end if correcting body forces




#if(0) // DEBUG
  if(geom->i==0 || geom->i==-1){
    for (i = 0; i < NDIM; i++) for (l = 0; l < NDIM; l++){
      dualfprintf(fail_file,"i=%d gcon[%d][%d]=%21.15g\n",geom->i,i,l,geom->gcon[GIND(i,l)]);
    }
    dualfprintf(fail_file,"i=%d conn[0][0][0]=%21.15g\n",geom->i,conn[0][0][0]);
  }
#endif
	
  /* done! */
}



int setup_XlXh(FTYPE *X,FTYPE *truedelta, FTYPE (*Xlgen)[NDIM],FTYPE (*Xhgen)[NDIM],FTYPE *signdXgen)
{
  int k,l;

    ////////////
    // first form high and low positions for function locations
    ////////////
    for (k = 0; k < NDIM; k++) {

      for (l = 0; l < NDIM; l++){
	Xhgen[k][l] = X[l];
	Xlgen[k][l] = X[l];
      }

      // normal case
      Xhgen[k][k] += MYDELTAh(truedelta[k],k);
      Xlgen[k][k] += MYDELTAl(truedelta[k],k);
      signdXgen[k]=1.0;

#if(0)// debug
      if(k==TT){
	if(Xlgen[k][k]>X[k]){
	  dualfprintf(fail_file,"Xl in future! Xl=%21.15g Xh=%21.15g\n",Xlgen[k][k],Xhgen[k][k]);
	}
      }
#endif


#if(0)
      // not really wanted since want "force" to be in same "radial" direction so sign SHOULD flip
      if(k==1 && ISSPCMCOORD(MCOORD)){
	// then check if r<0 and invert Xh and Xl if so
	bl_coord(X, V);
	if(V[k]<0.0){
	  signdXgen[k]=-1.0;
	}
      }
#endif
      //			if(k==TT) dualfprintf(fail_file,"k=%d DELTAl=%21.15g DELTAl=%21.15g\n",k,MYDELTAl(DELTA,k),MYDELTAh(DELTA,k));
      // DEBUG
      //Xhgen[k][k] += DELTA;
      //			Xlgen[k][k] -= DELTA;

    }

    return(0);

}



// compute low/high metric quantities
int compute_metricquantities_midlh(int donormal, int domachinebody
				   ,struct of_geom *geom, FTYPE *X, FTYPE *gmid, FTYPE *gcovpertmid,FTYPE *gdetmid,FTYPE *gdetlgen,FTYPE *gdethgen
				   ,struct of_geom (*localptrgeoml)[NDIM],struct of_geom (*localptrgeomh)[NDIM],FTYPE (*Xlgen)[NDIM],FTYPE (*Xhgen)[NDIM]
				   ,FTYPE *lngdetlgen, FTYPE *lngdethgen, FTYPE (*glgen)[SYMMATRIXNDIM], FTYPE (*ghgen)[SYMMATRIXNDIM], FTYPE (*gcovpertlgen)[NDIM], FTYPE (*gcovperthgen)[NDIM]
				   )
{
  int k;
  FTYPE gcov_func_mcoord(struct of_geom *ptrgeom, FTYPE* X, int i, int j);
  FTYPE lngdet_func_mcoord(struct of_geom *ptrgeom, FTYPE* X, int i, int j);
  FTYPE gdet_func_mcoord_usegcov(FTYPE *gcovmcoord, struct of_geom *ptrgeom, FTYPE* X, int i, int j);

    // get gdet in cell center
  if(domachinebody){
    gcov_func(geom,1,MCOORD,X, gmid,gcovpertmid);
    *gdetmid=gdet_func_mcoord_usegcov(gmid, geom, X, 0,0);
  }

  // get k-dependent things
  for (k = 0; k < NDIM; k++) {

    if(donormal){
      if(WHICHEOM!=WITHGDET){
	lngdethgen[k]=lngdet_func_mcoord(&((*localptrgeomh)[k]),Xhgen[k],0,0); // doesn't use 0,0
	lngdetlgen[k]=lngdet_func_mcoord(&((*localptrgeoml)[k]),Xlgen[k],0,0); // doesn't use 0,0
      }
    }
    if(donormal || domachinebody){
      // must come before below gdet_func_mcoord_usegcov() call
      gcov_func(&((*localptrgeomh)[k]),1,MCOORD,Xhgen[k], ghgen[k],gcovperthgen[k]);
      gcov_func(&((*localptrgeoml)[k]),1,MCOORD,Xlgen[k], glgen[k],gcovpertlgen[k]);
    }

    if(domachinebody){
      gdethgen[k]=gdet_func_mcoord_usegcov(ghgen[k], &((*localptrgeomh)[k]), Xhgen[k], 0,0);
      gdetlgen[k]=gdet_func_mcoord_usegcov(glgen[k], &((*localptrgeoml)[k]), Xlgen[k], 0,0);
    }

  }

  return(0);

}





// returns MCOORD gcov value for i,j element
// excessive to compute other elements, but ok for now
// input ptrgeom is only used for i,j,k,loc positions
FTYPE gcov_func_mcoord(struct of_geom *ptrgeom, FTYPE* X, int i, int j)
{
  FTYPE gcovmcoord[SYMMATRIXNDIM];
  FTYPE gcovpert[NDIM];
  void gcov_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcovpert);

  gcov_func(ptrgeom,1,MCOORD,X,gcovmcoord,gcovpert);
  //  if(i==j) return(gcovpert[i]); // for accurate non-rel gravity (works since d(1+g_{tt}) = dg_{tt})
  //  else
  return(gcovmcoord[GIND(i,j)]);
}


// returns MCOORD  value for log(f/gdet).  Doesn't use i,j (these are not grid locations)
// input ptrgeom is only used for i,j,k,loc positions
FTYPE lngdet_func_mcoord(struct of_geom *ptrgeom, FTYPE* X, int i, int j)
{
  FTYPE gcovmcoord[SYMMATRIXNDIM];
  FTYPE gcovpertcoord[NDIM];
  FTYPE toreturn;
  FTYPE eomfunc[NPR]; // don't change this as related to WHICHEOM==WITHGDET
  FTYPE *eomfuncptr=eomfunc; // don't change this as related to WHICHEOM==WITHGDET
  FTYPE gdet;
  FTYPE V[NDIM];
  void gcov_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcovpert);

  gcov_func(ptrgeom,1,MCOORD,X,gcovmcoord,gcovpertcoord);
  eomfunc_func(ptrgeom,1,MCOORD,X,EOMFUNCPTR);

  bl_coord_ijk_2(ptrgeom->i,ptrgeom->j,ptrgeom->k,ptrgeom->p, X, V);
  // GODMARK: assumes all RHO, etc. use same eomfunc if using 2nd connection
  toreturn=log(EOMFUNCMAC(RHO)/gdet_func_metric(MCOORD,V,gcovmcoord));

  return(toreturn);
}

// returns MCOORD  value for gdet.  Doesn't use i,j (these are not grid locations)
FTYPE gdet_func_mcoord(struct of_geom *ptrgeom, FTYPE* X, int i, int j)
{
  FTYPE gcovmcoord[SYMMATRIXNDIM];
  FTYPE gcovpertcoord[NDIM];
  FTYPE toreturn;
  FTYPE V[NDIM];
  void gcov_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcovpert);

  gcov_func(ptrgeom,1,MCOORD,X,gcovmcoord,gcovpertcoord);

  bl_coord_ijk_2(ptrgeom->i,ptrgeom->j,ptrgeom->k,ptrgeom->p, X, V);
  toreturn=gdet_func_metric(MCOORD,V,gcovmcoord);

  return(toreturn);
}

// returns MCOORD  value for gdet using gcovmcoord as input to avoid repeated computations of gcovmcoord.  Doesn't use i,j (these are not grid locations)
FTYPE gdet_func_mcoord_usegcov(FTYPE *gcovmcoord, struct of_geom *ptrgeom, FTYPE* X, int i, int j)
{
  //  FTYPE gcovmcoord[SYMMATRIXNDIM];
  //  FTYPE gcovpertcoord[NDIM];
  FTYPE toreturn;
  FTYPE V[NDIM];
  //void gcov_func(struct of_geom *ptrgeom, int getprim, int whichcoord, FTYPE *X, FTYPE *gcov, FTYPE *gcovpert);

  //  gcov_func(ptrgeom,1,MCOORD,X,gcovmcoord,gcovpertcoord);

  bl_coord_ijk_2(ptrgeom->i,ptrgeom->j,ptrgeom->k,ptrgeom->p, X, V);
  toreturn=gdet_func_metric(MCOORD,V,gcovmcoord);

  return(toreturn);
}





















int metric_checks(struct of_geom *ptrgeom)
{
  FTYPE delta[NDIM][NDIM];
  FTYPE V[NDIM],X[NDIM];
  int i,j,k,loc;
  int jj,kk,pp;


  i=ptrgeom->i;
  j=ptrgeom->j;
  k=ptrgeom->k;
  loc=ptrgeom->p;

  ////////////////
  //
  // delta^jj_kk = gcov_{kk pp} gcon^{pp jj}
  //
  ////////////////
  DLOOP(jj,kk){
    delta[jj][kk]=0.0;
    DLOOPA(pp){
      delta[jj][kk]+= ptrgeom->gcov[GIND(jj,pp)]* ptrgeom->gcon[GIND(pp,kk)];
    }

    if(PRODUCTION==0){
      if(fabs(delta[jj][kk]-delta(jj,kk))>NUMEPSILON*100.0){
	if(ISSPCMCOORD(MCOORD)==0 || (ISSPCMCOORD(MCOORD)==1 && j!=totalsize[2] && j!=0 && loc!=FACE2 && loc!=CORN1 && loc!=CORN3 && loc!=CORNT) ){
	  dualfprintf(fail_file,"Problem with metric at i=%d j=%d k=%d loc=%d delta[%d][%d]=%21.15g\n",ptrgeom->i,ptrgeom->j,ptrgeom->k,ptrgeom->p,jj,kk,delta[jj][kk]);
	}
      }
    }

    if(fabs(delta[jj][kk]-delta(jj,kk))>NUMEPSILON*1000.0){
      if(ISSPCMCOORD(MCOORD)==0 || (ISSPCMCOORD(MCOORD)==1 && j!=totalsize[2] && j!=0 && loc!=FACE2 && loc!=CORN1 && loc!=CORN3 && loc!=CORNT) ){
	dualfprintf(fail_file,"MAJOR Problem with metric at i=%d j=%d k=%d loc=%d delta[%d][%d]=%21.15g\n",ptrgeom->i,ptrgeom->j,ptrgeom->k,ptrgeom->p,jj,kk,delta[jj][kk]);
      }
    }
  }

  

  // check if near static limit since can't divide by the below in ucon_calc
  // GODMARK
  if (fabs(ptrgeom->gcon[GIND(TT,TT)]) < SMALL) {
    bl_coord_ijk_2(i,j,k,loc,X,V);
    dualfprintf(fail_file, "grid location too near g_{tt}==0: %d %d %d : r=%21.15g th=%21.15g phi=%21.15g : Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,ptrgeom->gcon[GIND(TT,TT)]);
    myexit(1);
  }
  if (0 && fabs(ptrgeom->gcon[GIND(RR,RR)]) < SMALL) {
    bl_coord_ijk_2(i,j,k,loc,X,V);
    dualfprintf(fail_file, "grid location too near g^{rr}==0:  %d %d %d : r=%21.15g th=%21.15g phi=%21.15g :  Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,ptrgeom->gcon[GIND(RR,RR)]);
    myexit(1);
  }
  if (0 && fabs(ptrgeom->gcon[GIND(TH,TH)]) < SMALL) {
    bl_coord_ijk_2(i,j,k,loc,X,V);
    dualfprintf(fail_file,"grid location too near g^{\\theta\\theta}==0:  %d %d %d : r=%21.15g th=%21.15g phi=%21.15g :  Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,ptrgeom->gcon[GIND(TH,TH)]);
    myexit(1);
  }
  if (0 && fabs(ptrgeom->gcon[GIND(PH,PH)]) < SMALL) {
    bl_coord_ijk_2(i,j,k,loc,X,V);
    dualfprintf(fail_file,"grid location too near g^{\\phi\\phi}==0:  %d %d %d : r=%21.15g th=%21.15g phi=%21.15g :  Rin=%21.15g %21.15g\n", i,j,k,V[1],V[2],V[3],Rin,ptrgeom->gcon[GIND(PH,PH)]);
    myexit(1);
  }

  // what about g_{tt}==0? Do I ever divide by g_{tt}?
  // Yes, for ucon[TT] for 4 velocity, which is done if WHICHVEL==VEL4 or init.c
  // what about g^{rr}==0? Do I ever divide by g^{rr}?
  // Doesn't appear so
  // g^{pp} inf taken care of in metric.c by avoiding theta=0,Pi


  return(0);

}
