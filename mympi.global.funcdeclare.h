////////////////////////////////////////////
//
// OpenMP and MPI -related function declarations
//
////////////////////////////////////////////





////////////////////////////////////////////
//
// OpenMP-related function declarations
//
////////////////////////////////////////////


int init_OPENMP_general(FILE *out);
int init_OPENMP_sets_fromargs(void);
void get_report_openmp_thread_info(FILE * out);





////////////////////////////////////////////
//
// MPI-related function declarations
//
////////////////////////////////////////////



#if(DOINGLIAISON)
extern void test_nonliaison(void);
#endif




extern void mpi_set_arrays(void);
extern void mpi_set_arrays_multidimen(void);
extern void init_genfiles(int gopp);
extern int init_MPI_general(int *argc, char **argv[]);
extern int init_MPI_GRMHD(int *argc, char **argv[]);
extern int init_default_MPI_GRMHD_myid(void);
extern int init_MPI_GRMHD_myid(void);
extern int report_myid(FILE *out);
extern void init_MPI_setupfilesandgrid(int argc, char *argv[]);
extern void init_placeongrid(void);
extern int myexit(int call_code);
extern int final_myexit(void);

extern int bound_mpi_dir(int boundstage, int whichdir, int boundtype, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);

extern int bound_mpi(int boundstage, int boundtype, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);

extern int bound_mpi_int(int boundstage, int boundtype, PFTYPE (*prim)[NSTORE2][NSTORE3][NUMPFLAGS]);
extern void pack(int dir, int boundtype, FTYPE (*prim)[NSTORE2][NSTORE3][NPR],FTYPE (*workbc)[COMPDIM * 2][NMAXBOUND * NBIGBND * NBIGSM]);
extern void unpack(int dir, int boundtype, FTYPE (*workbc)[COMPDIM * 2][NMAXBOUND * NBIGBND * NBIGSM],FTYPE (*prim)[NSTORE2][NSTORE3][NPR]);
extern void pack_int(int dir, int boundtype, PFTYPE (*prim)[NSTORE2][NSTORE3][NUMPFLAGS],PFTYPE (*workbc_int)[COMPDIM * 2][NUMPFLAGSBOUND * NBIGBND * NBIGSM]);
extern void unpack_int(int dir, int boundtype, PFTYPE (*workbc_int)[COMPDIM * 2][NUMPFLAGSBOUND * NBIGBND * NBIGSM],PFTYPE (*prim)[NSTORE2][NSTORE3][NUMPFLAGS]);
#if(USEMPI)
extern void sendrecv(int dir,int boundtype,FTYPE (*workbc)[COMPDIM * 2][NMAXBOUND * NBIGBND * NBIGSM],MPI_Request *requests);
extern void sendrecv_int(int dir, int boundtype, PFTYPE (*workbc_int)[COMPDIM * 2][NUMPFLAGSBOUND * NBIGBND * NBIGSM],MPI_Request *requests);
extern void recvwait(int dir,MPI_Request *request);
extern void sendwait(int dir,MPI_Request *request);
#endif

extern void mpimax(SFTYPE*maxptr);
extern void mpiimax(int*maxptr);
extern void mpimin(SFTYPE*minptr);
extern void mpiisum(int*maxptr);
extern void mpiisum0(int*sumptr, int recvid);
extern void mpildsum0(long int*sumptr, int recvid);
extern void mpifmin(FTYPE*minptr);

extern int set_sizeofmemory(int numbuff, int sizeofdatatype, int numcolumns, int *sizeofmemory);
extern int set_maxnumsize(int numcolumns, long long int *maxnumsize);
extern int set_numbuffers(int numcolumns, int *numbuffers);
extern int gcountmod(int numcolumns);

extern int getsizeofdatatype(MPI_Datatype datatype);

extern void mpiio_init(int bintxt, int sorted, FILE ** fp, long headerbytesize, int which, char *filename, int numcolumns,
		       MPI_Datatype datatype, void **jonio, void **writebuf);
extern void mpiio_combine(int bintxt, int sorted,
			  int numcolumns, MPI_Datatype datatype,
			  FILE ** fp, void *jonio, void *writebuf);
extern void mpiio_seperate(int bintxt, int sorted, int stage,
			   int numcolumns,
			   MPI_Datatype datatype, FILE ** fp, void *jonio,
			   void *writebuf);
extern void mpiios_init(int bintxt, int sorted, FILE ** fp, int which, int headerbytesize, char *filename, int numcolumns,
			MPI_Datatype datatype, void **jonio, void **writebuf);
extern void mpiiomin_final(int numcolumns,FILE **fp, void *jonio, void *writebuf);
extern void mpiio_minmem(int readwrite, int whichdump, int i, int j, int k, int bintxt, int sorted,
			      int numcolumns, MPI_Datatype datatype,
			      FILE ** fpptr, void *jonio, void *writebuf);


extern void mpiioromio_init_combine(int operationtype, int which, long headerbytesize, char *filename, int numcolumns,MPI_Datatype datatype, void **writebufptr, void *writebuf);

#if(USEMPI)
extern void mpiios_combine(int bintxt, MPI_Datatype datatype, int numcolumns,
			   FILE ** fp, void *jonio, void *writebuf);
extern void mpiios_seperate(int bintxt, int stage, MPI_Datatype datatype, int numcolumns,
			    FILE ** fp, void *jonio,
			    void *writebuf);
extern void mpiiotu_combine(MPI_Datatype datatype, int numcolumns,
			    FILE ** fp, void *writebuf);
extern void mpiiotu_seperate(int stage, MPI_Datatype datatype, int numcolumns,
			     FILE ** fp,void *writebuf);
#endif







// END
//
//////////////////////////

