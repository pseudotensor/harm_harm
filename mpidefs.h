


#ifndef USINGLIAISON
// for only grmhd code
#include "supermpidefs.h"
#endif


// for both grmhd and liaison codes whether or not DOINGGRMHDLIAISON==1
#include "mpidefs.mpi_grmhd_grray_liaison.h"




int periodicx1, periodicx2, periodicx3;
int mpiperiodicx1, mpiperiodicx2, mpiperiodicx3;
int skipix1, reflectix1, reflectox1;
int skipix2, reflectix2, reflectox2;
int skipix3, reflectix3, reflectox3;
int intix1, intox1, intix2, intox2, intix3, intox3;
int skipintix1, skipintix2, skipintix3;
int ncpux1, ncpux2, ncpux3;
int truenumprocs;
int myid_world, numprocs;
int myid; // GRMHD CODE rank for non-MPI commands
int MPIid[MAXCPUS]; // GRMHD MPI internal rank for MPI commands
char myidtxt[MAXFILENAME];
long long int totalzones, realtotalzones,realtotalcompzones;
long long int rtotalzones;
long long int itotalzones;
//int sizes[COMPDIM + 1][MAXCPUS];
//int isizes[COMPDIM + 1][MAXCPUS];
long long int totalsize[COMPDIM + 1];
long long int itotalsize[COMPDIM + 1];
int mycpupos[COMPDIM + 1];		// my position amongst the cpus
int primgridpos[NUMBOUNDTYPES][COMPDIM*2][MAXNPR];
// NUMPACKUNPACK+1 because start at 1
FTYPE primfactor[NUMBOUNDTYPES][COMPDIM*2][NUMPRIMGRIDPOS][1+NUMPACKUNPACK][MAXNPR];
int dirgenset[NUMBOUNDTYPES][COMPDIM*2][DIRGENNUMVARS];
int dirloopset[NUMBOUNDTYPES][COMPDIM*2][NUMPRIMGRIDPOS][DIRLOOPNUMVARS];
int srdir[3*2];			// which direction this cpu
				// sends/receives normal interior data
int startpos[COMPDIM + 1];
int endpos[COMPDIM + 1];		// startj and endj are where this CPU
				// located on full grid 
int *startpos0[COMPDIM+1];
int *endpos0[COMPDIM+1];
int *mycpupos0[COMPDIM+1];

int plmpiglobal;


int procnamelen;
#if(USEMPI)
MPI_Group MPI_GROUP_WORLD;
char processor_name[MPI_MAX_PROCESSOR_NAME];
MPI_Status mpichstatus;
#endif

// MPI transmit vars, so minimum local code changes
FTYPE ndtsend, bsq_maxsend;

// for data output
// required to be long long int in case integer is only 4 bytes and want >2GB files.
long long int nextbuf;
int numcolumns;
long long int bufferoffset;
long long int joniosize,writebufsize;
int romiocoliter;


/////////////////////////////
//
// OpenMP stuff
//
/////////////////////////////

int numopenmpthreads;
int numopenmpthreadsorig;
