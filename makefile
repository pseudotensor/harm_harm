##################
#
# Makefile for various systems
# makehead.inc is where most user chocies are made
# maketail.inc is where work for compiling is mostly done
#
# Only should change things here if adding/fixing a system
#
##################


############################
# header
include makehead.inc







# default
USEMCCSWITCH=0
USEMCCSWITCHFORGCC=0
AVOIDFORK=0
AVOIDMKDIR=0
USESPECIAL4GENERATE=0
CCGENERATE=gcc
ECHOSWITCH=-e




ifeq ($(ENFORCEPRECISION),1)
PRECISE=-mp -pc64
endif
ifeq ($(ENFORCEPRECISION),0)
PRECISE=
endif




#################### IF USEMPI

ifeq ($(USEMPI),1)

# override with new default
USEMCCSWITCH=1
USEMCCSWITCHFORGCC=1

ifeq ($(USEQB),1)
USEICCGENERIC=1
AVOIDMKDIR=1
#AVOIDFORK=1
endif

ifeq ($(USEBG),1)
# override again
USEMCCSWITCH=0
AVOIDFORK=1
MCC=mpicc
CCGENERATE=gcc
USESPECIAL4GENERATE=1
endif

ifeq ($(USETACCLONESTAR),1)
# override again
AVOIDFORK=1
MCC=mpicc
endif

ifeq ($(USENERSC),1)
# override again
USEMCCSWITCH=0
AVOIDFORK=1
MCC=cc
endif

ifeq ($(USEICCGENERIC),1)
# uses -static for secure library usage
# MCC=/usr/local/p4mpich-1.2.5-icc-noshmem/bin/mpicc
MCC=mpicc
endif

ifeq ($(USEICCINTEL),1)
# uses -static for secure library usage
# MCC=/usr/local/p4mpich-1.2.5-icc-noshmem/bin/mpicc
MCC=mpicc
endif

ifeq ($(USETACCRANGER),1)
# don't have to avoid fork/system calls
USEMCCSWITCHFORGCC=0
AVOIDFORK=0
MCC=mpicc
endif

ifeq ($(USEORANGE),1)
# orange can't have -static
AVOIDFORK=1 # recently seems to be a problem, but not before
MCC=mpicc -I/afs/slac/package/OpenMPI/include/ -L/afs/slac/package/OpenMPI/lib/
endif

ifeq ($(USEGCC),1)
MCC=mpicc
#.gcc
endif

ifeq ($(USEPGCC),1)
MCC=mpicc
endif    

ifeq ($(USEUB),1)
MCC=/usr/bin/mpicc.mpich
USEGCC=1
ECHOSWITCH=
USELAPACK=0
endif

endif    
#################### DONE IF USEMPI


#################### IF NOT USEMPI

ifeq ($(USEMPI),0)

ifeq ($(USEQB),1)
USEICCGENERIC=1
endif

ifeq ($(USEBG),1)
CCGENERATE=gcc
USESPECIAL4GENERATE=1
endif

ifeq ($(USETACCLONESTAR),1)
endif

ifeq ($(USENERSC),1)
endif

ifeq ($(USEICCGENERIC),1)
endif

ifeq ($(USEICCINTEL),1)
endif

ifeq ($(USETACCRANGER),1)
endif

ifeq ($(USEORANGE),1)
endif

ifeq ($(USEGCC),1)
endif

ifeq ($(USEPGCC),1)
endif    

ifeq ($(USEUB),1)
USEGCC=1
ECHOSWITCH=
USELAPACK=0
endif

endif    
#################### DONE IF USEMPI


# USEMPI OR NOT:
ifeq ($(USEUB),1)
USEGCC=1
ECHOSWITCH=
USELAPACK=0
endif


#################### IF USEGPROF
ifeq ($(USEGPROF),1)
# Noticed problem sometimes with mpi_init.c:myexit():system() calls where program doesn't exit and just stals on fork() with -g -pg
AVOIDFORK=1
#-pg
#-pg -g  source lines
#-pg -g -a   line count
# gprof -l <file> > out.txt
# gprof -A -I<sourcedir>
# gprof -l -A -x s
GPROFFLAG=-g -pg

ifeq ($(USETACCRANGER),1)
GPROFFLAG=-g -p
endif


endif

ifeq ($(USEGPROF),0)
GPROFFLAG=
endif


#################### IF USEDEBUG
ifeq ($(USEDEBUG),1)
DEBUGFLAG=-g
endif
ifeq ($(USEDEBUG),0)
DEBUGFLAG=
endif



ifeq ($(USELAPACK),1)
#	below gives blas and lapack support
	LAPACKLDFLAGS=-lmkl_lapack -lmkl -lguide -lpthread
else
	LAPACKLDFLAGS=
endif

ifeq ($(USEOPENMP),1)
	OPMPFLAGS=-openmp
else
	OPMPFLAGS=
endif


# default extra flags to pass to compiler
EXTRA=$(DEBUGFLAG) $(GPROFFLAG) -DUSINGMPI=$(USEMPI) -DUSINGOPENMP=$(USEOPENMP) -DUSINGMPIAVOIDFORK=$(AVOIDFORK) -DUSINGMPIAVOIDMKDIR=$(AVOIDMKDIR) -DUSINGLAPACK=$(USELAPACK)






#############################################################################
#
# Define preprocessor and compile flags, and linked libraries for each compiler or system type
#
#############################################################################



ifeq ($(USEGCC),1)
LONGDOUBLECOMMAND=-m128bit-long-double
#LONGDOUBLECOMMAND=-m96bit-long-double


DFLAGS=-DUSINGICC=0 -DUSINGORANGE=0 $(EXTRA)


COMP=gcc $(DFLAGS)
# COMP=gcc3

# gcc type flags
#
########################
# UES BELOW
########################
#CFLAGSPRE = -Wall -mpentium -O3 -pipe  -malign-loops=2 -malign-jumps=2 -malign-functions=2 -DCPU=686 -DNEED_GETOPT -DLINUX
#
#
#

CFLAGSPRE = -Wall -O3 $(DFLAGS)
CFLAGSPRENONPRECISE=-O3 $(DFLAGS)

#
#CFLAGS = -Wall -mpentium -O3 -pipe  -malign-loops=2 -malign-jumps=2 -malign-functions=2 -DCPU=686 -DNEED_GETOPT -DLINUX -ffast-math -pg
#CFLAGS = -Wall -mcpu=pentiumpro -march=pentiumpro -O4 -pipe  -malign-loops=2 -malign-jumps=2 -malign-functions=2 -malign-double -mstack-align-double -ffast-math -pg
#CFLAGS = -Wall -mcpu=pentiumpro -march=pentiumpro -O4 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -malign-double -mstack-align-double -ffast-math -finline-functions -pg
#CFLAGS = -Wall -mcpu=pentiumpro -march=pentiumpro -O4 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -malign-double -ffast-math -finline-functions -pg
#CFLAGS = -Wall -mcpu=pentiumpro -march=pentiumpro -O3 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -ffast-math -finline-functions -pg -g -a
#CFLAGS = -Wall -mcpu=pentiumpro -march=pentiumpro -O3 -ffast-math -finline-functions -funroll-loops
#CFLAGS=-O0 -g -Wall -wunused-label -wunused-parameter
#CFLAGS=-O0 -g -Wall
#CFLAGS = -Wall -mcpu=pentiumpro -march=pentiumpro -O4 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -ffast-math -finline-functions -g

#below is typical flags for double precision...can take -pg off for no profile
#add -mstack-align-double if using pgcc
#CFLAGS = -Wall -O0 -g
#  -fomit-frame-pointer



#CFLAGS = -Wall -O0
#CFLAGS = -O6 -g
#CFLAGS = -O0 -pg -g
LDFLAGS = -lm $(LAPACKLDFLAGS)
# -l$(LAPACKLIB) -l$(BLASLIB)  -L/usr/lib/gcc-lib/i386-redhat-linux/2.96/ -l$(F2CLIB) 

#CC = cc
#AR	=	ar r
#RANLIB	=	ranlib

GCCCFLAGSPRE= -Wall -O2 $(DFLAGS)


endif




ifeq ($(USEICCGENERIC),1)

DFLAGS=-DUSINGICC=1  -DUSINGORANGE=0 $(EXTRA)
LONGDOUBLECOMMAND=-long_double


COMP=icc $(DFLAGS) $(OPMPFLAGS)

CFLAGSPRENONPRECISE=-O2 -no-prec-div -no-prec-sqrt -fp-speculation=fast -finline -finline-functions -ip -fno-alias -unroll -Wall -Wcheck -Wshadow -w2 -wd=1419,869,177,310,593,810,981,1418 $(DFLAGS)

CFLAGSPRE=$(PRECISE) $(CFLAGSPRENONPRECISE)

GCCCFLAGSPRE= -Wall -O2 $(DFLAGS)


LDFLAGS=-lm  $(LAPACKLDFLAGS)
LDFLAGSOTHER=

endif



# Intel machine specific
ifeq ($(USEICCINTEL),1)

DFLAGS=-DUSINGICC=1  -DUSINGORANGE=0 $(EXTRA)
LONGDOUBLECOMMAND=-long_double


COMP=icc $(DFLAGS) $(OPMPFLAGS)
# -ipo is not same for multiple names in c files, unless static names
# -ipo_wp is generally not safe, and not for HARM
# -ip is safe, always
# -ipo not safe for icc v8.1 for connection coefficients for some reason
#CFLAGSPRE=-O3 -tpp7 -axKW -mp -unroll -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 -g -pg
# from older code
#CFLAGSPRE=-O3 -tpp7 -axiMKW -unroll -Wall -w2 -wd=175,177,279,593,869,810,981,1418,1419,310
# newer code below
#CFLAGSPRE=-O3 -tpp7 -axKW -unroll -Wall -Wcheck -Wshadow -pc64 -mp -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 -mp

#CFLAGSPRENONPRECISE=-O3 -tpp7 -axKW -unroll -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572
#
#########################
# NORMALLY USE BELOW
#########################
#
# autoparallelization doesn't work except for very trivial loops and not even all of those
#CFLAGSPRENONPRECISE=-O2 -parallel -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 $(DFLAGS)


# NORMAL:
#CFLAGSPRENONPRECISE=-O2 -finline -finline-functions -ip -fno-alias -unroll -openmp -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 $(DFLAGS)
# new normal:
#CFLAGSPRENONPRECISE=-O2 -static -xP -no-prec-div -no-prec-sqrt -fp-speculation=fast -finline -finline-functions -ip -fno-alias -unroll -openmp -Wall -Wcheck -Wshadow -w2 -wd=1419,869,177,310,593,810,981,1418 $(DFLAGS)
#CFLAGSPRENONPRECISE=-O2 -static -xP -no-prec-div -no-prec-sqrt -fp-speculation=fast -finline -finline-functions -ip -fno-alias -unroll -pthread -Wall -Wcheck -Wshadow -w2 -wd=1419,869,177,310,593,810,981,1418 $(DFLAGS)
#CFLAGSPRENONPRECISE=-O2 -xP -no-prec-div -no-prec-sqrt -fp-speculation=fast -finline -finline-functions -ip -fno-alias -unroll -parallel -par-report=2 -par-threshold=10 -Wall -Wcheck -Wshadow -w2 -wd=1419,869,177,310,593,810,981,1418 $(DFLAGS)

CFLAGSPRENONPRECISE=-O2 -xP -no-prec-div -no-prec-sqrt -fp-speculation=fast -finline -finline-functions -ip -fno-alias -unroll -Wall -Wcheck -Wshadow -w2 -wd=1419,869,177,310,593,810,981,1418 $(DFLAGS)


# FOR CHECKING OPTIMIZATIONS:
#CFLAGSPRENONPRECISE=-O2 -openmp -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 -g -pg $(DFLAGS)
#
#########################
# DEBUG BELOW
#########################
#CFLAGSPRENONPRECISE=-O0 -g -openmp $(DFLAGS)
#
#
#
CFLAGSPRE=$(PRECISE) $(CFLAGSPRENONPRECISE)

GCCCFLAGSPRE= -Wall -O2 $(DFLAGS)

#CFLAGSPRE=-O0 -g
#CFLAGSPRE=-O0 -g
#CFLAGSPRE=-O3 -mp -unroll -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572
#CFLAGSPRE=-O3 -mp -unroll -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 -g
#CFLAGSPRE=-O0 -g
#CFLAGSPRE=-O3 -tpp7 -axKW -ip -unroll -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572
#CFLAGS=-O3 -unroll -Wall -w2 -wd=981,279,869,1572,1418,177,1419,593,810,310

#GCCCFLAGSPRE = -Wall -mpentium -O3 -pipe  -malign-loops=2 -malign-jumps=2 -malign-functions=2 -DCPU=686 -DNEED_GETOPT -DLINUX -ffast-math
#GCCCFLAGSPRE = -O0 -g -pg
#CFLAGSPRE = -O3 -g -Wall -w2 -wd=175,177,279,593,869,810,981,1418,1419,310

# P4 (don't use -wp_ipo -- incompat code)
#CFLAGS=-O3 -tpp7 -axiMKW -wp_ipo -unroll -w1
#CFLAGS=-O3 -tpp7 -axiMKW -ipo -unroll -Wall -w2 -wd=175,177,279,593,869,810,981,1418,1419,310 -pg
#CFLAGS=-O3 -unroll -axiMKW -unroll -pg
# -rcd causes problems, like asymetries in interp program I've noticed
#CFLAGS = -Wall -w2 -O3 -axiMKW -unroll -ipo -tpp7 -march=pentium4 -mcpu=pentium4# -p\
#arallel
# -w2 displays more info for warnings
#CFLAGS=-O3 -axiMKW -g -ipo -pg
# P3
# CFLAGS=-O3 -tpp6 -axiMK -ipo -unroll -w1
# GAMMIE
# CFLAGS = -O3 -ipo




LDFLAGS=-lm  $(LAPACKLDFLAGS)
LDFLAGSOTHER=




endif






ifeq ($(USETACCLONESTAR),1)
LONGDOUBLECOMMAND=-long_double
DFLAGS=-DUSINGICC=1  -DUSINGORANGE=0 $(EXTRA)
COMP=icc $(DFLAGS)  $(OPMPFLAGS) -I$(TACC_MKL_INC)
CFLAGSPRENONPRECISE=-O2 -xT -finline -finline-functions -ip -fno-alias -unroll -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 $(DFLAGS)
CFLAGSPRE=$(PRECISE) $(CFLAGSPRENONPRECISE)
# below only needed if compiling main() function file with gcc
#GCCCFLAGSPRE= -Wall -O2 -L$ICC_LIB -lirc $(DFLAGS)
GCCCFLAGSPRE= -Wall -O2 $(DFLAGS)
LDFLAGS = -lm -Wl,-rpath,$(TACC_MKL_LIB) -L$(TACC_MKL_LIB) -lmkl_em64t
LDFLAGSOTHER=
endif


ifeq ($(USETACCRANGER),1)
# http://services.tacc.utexas.edu/index.php/ranger-user-guide#Compiling_Code
# and see batch.qsub.taccranger
LONGDOUBLECOMMAND=-long_double
DFLAGS=-DUSINGICC=1  -DUSINGORANGE=0 $(EXTRA)
COMP=icc $(DFLAGS) $(OPMPFLAGS) -I$(TACC_MKL_INC)
CFLAGSPRENONPRECISE=-xW -O2 -finline -finline-functions -ip -fno-alias -unroll -Wall -Wcheck -Wshadow -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572 $(DFLAGS)
CFLAGSPRE=$(PRECISE) $(CFLAGSPRENONPRECISE)
# below only needed if compiling main() function file with gcc
#GCCCFLAGSPRE= -Wall -O2 -L$ICC_LIB -lirc $(DFLAGS)
GCCCFLAGSPRE= -Wall -O2 $(DFLAGS)
LDFLAGS = -lm  -Wl,-rpath,$(TACC_MKL_LIB) -L$(TACC_MKL_LIB) -lmkl_em64t
LDFLAGSOTHER=
endif


ifeq ($(USECCC),1)

LONGDOUBLECOMMAND=
DFLAGS=-DUSINGICC=0 -DUSINGORANGE=0 $(EXTRA)
COMP=ccc $(DFLAGS)
LDFLAGS =  -lm -lcxml  $(LAPACKLDFLAGS)

#CDEBUG = -g3 # -g3 for higher opts than -O0
#CDEBUG = -g
# do profile (profile screws up speed for loops, doesn't unroll them, etc.)
#CDEBUG = -pg -g3
# production level
CFLAGSPRE = -Wall -O4 -fast -msg_disable badsubscript -msg_disable subscrbounds -msg_disable unreachcode -msg_disable noparmlist -msg_disable subscrbounds2 -msg_disable longlongtype -finline-functions -funroll-loops  $(DFLAGS)
CFLAGSPRENONPRECISE=$(CFLAGSPRE)
#CFLAGS3 = -Wall -O0
# super annoying develop level
#CFLAGS3 = -Wall -O2 -fast
#CFLAGS3 = -fast -arch ev67
# debug level
#CFLAGS3 = -Wall -O0 -msg_disable badsubscript -msg_disable subscrbounds -msg_disable unreachcode -msg_disable noparmlist -msg_disable subscrbounds2
#CFLAGS3 = -Wall -O0
#CFLAGS3 = -Wall -O2

endif


ifeq ($(USEBG),1)
LONGDOUBLECOMMAND=-m128bit-long-double
#LONGDOUBLECOMMAND=-m96bit-long-double
DFLAGS=-DUSINGICC=0  -DUSINGORANGE=0 $(EXTRA)
COMP=gcc $(DFLAGS)
# don't use -ffast_math, causes asymmetries in calculations
CFLAGSPRE= -O3 -funroll-loops  $(DFLAGS)
CFLAGSPRENONPRECISE= $(CFLAGSPRE)
GCCCFLAGSPRE= -O3 $(DFLAGS)
#-funroll-loops -fargument-noalias -mcpu=k8 -msse2 -mfpmath=sse -static
LDFLAGS= -lm  $(LAPACKLDFLAGS)
endif



ifeq ($(USEORANGE),1)
LONGDOUBLECOMMAND=-long_double
DFLAGS=-DUSINGICC=1  -DUSINGORANGE=1 $(EXTRA)
COMP=icc -I/afs/slac/package/OpenMPI/include/ -L/afs/slac/package/OpenMPI/lib/ $(DFLAGS)  $(OPMPFLAGS)
#CFLAGSPRE=-O3 -fno-alias -ftz -unroll -Wall -w2 -wd=175,177,279,593,869,810,981,1418,1419,310,1572
CFLAGSPRE=-O2 -fno-alias -ftz -unroll -Wall -w2 -wd=175,177,279,593,869,810,981,1418,1419,310 $(DFLAGS)
CFLAGSPRENONPRECISE=$(CFLAGSPRE)
LDFLAGS = -lm  $(LAPACKLDFLAGS)
#GCCCFLAGSPRE= -Wall -O3 -m32
GCCCFLAGSPRE= -Wall -O3 $(DFLAGS)
endif



ifeq ($(USENERSC),1)
LONGDOUBLECOMMAND=
DFLAGS=-DUSINGICC=1  -DUSINGORANGE=0 $(EXTRA)
COMP=cc $(DFLAGS)
CFLAGSPRE = -O3 $(DFLAGS)
CFLAGSPRENONPRECISE = $(CFLAGSPRE)
GCCCFLAGSPRE=  $(CFLAGSPRE)
LDFLAGS = -lm  $(LAPACKLDFLAGS)
endif











#########################
#
# Setup flags to pass to compiler/linker
#
#########################


ifeq ($(MYMAKECMDGOALS),$(CMD2))
CFLAGSNONPRECISE=$(LONGDOUBLECOMMAND) $(CFLAGSPRENONPRECISE)
CFLAGS=$(LONGDOUBLECOMMAND) $(CFLAGSPRE)
GCCCFLAGS=$(LONGDOUBLECOMMAND) $(GCCCFLAGSPRE)
else
CFLAGS=$(CFLAGSPRE)
CFLAGSNONPRECISE=$(CFLAGSPRENONPRECISE)
GCCCFLAGS=$(GCCCFLAGSPRE)
endif

# for for normal installation of v5d and hdf
BIN2TXTLIBS=-I /usr/include/hdf/ -L /usr/lib64/hdf/ -lmfhdf -ldf -ljpeg -lz -lv5d
# below for ki-rh39 and likeness
# BIN2TXTLIBS=-I ~/include/ -L ~/lib/ -lmfhdf -ldf -ljpeg -lz -lv5d
# must also change #include "hdf" stuff to have the forward hdf/ removed


#############################
#
# Include primary makefile that includes "prep" and "clean" and all other build operations
#
#############################
include maketail.inc


