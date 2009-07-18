
////////////////
//
// globals (OPENMPMARK: that are fixed in value for all threads)
//
////////////////
static int numeosquantities[NUMTBLS]; // true number
static int numeosdegenquantities[NUMTBLS]; // true number

// first [4] : above 4 types of independent variables
// second [4] : 0 = lower log_base limit, 1 = upper log_base limit, 2=step, 3 = divisor of grid position 4=base of log, 5 = log10 value of offset for log_base stepping so can control how resolved
static FTYPE inputtablelimits[NUMTBLS][NUMEOSINDEPS][TBLITEMS];
// below is same as input, but converted for easy use
static FTYPE tablelimits[NUMTBLS][NUMEOSINDEPS][TBLITEMS];
// first [4] as above, second [2] : 0=lower linear limit, 1=upper linear limit, 2 = log(-base) 3 = linear offset
static FTYPE lineartablelimits[NUMTBLS][NUMEOSINDEPS][TBLLINEARITEMS];
// first [4] : as first [4] above
static int tablesize[NUMTBLS][NUMEOSINDEPS];

static int vartypearray[NUMINDEPDIMENS+1];
static int varnormalcompare2degentypearray[NUMEOSDEGENINDEPS+1]; // to be used to compare against degen table
static int vardegentypearray[NUMEOSDEGENINDEPS+1];


// code value of invalid temperature and log10 version
static FTYPE invalidtempcode,invalidlogtempcode;

static int whichrnpmethod[NUMTBLS], whichynumethod[NUMTBLS], whichhcmmethod[NUMTBLS];
static int whichdatatype[NUMTBLS],numc[NUMTBLS],numextras[NUMTBLS];

static int primarytable=-1; // -1 indicates no EOS setup/read-in yet

static int extralimits[MAXNUMDATATYPES][2];


static FTYPE FAKE2IDEALNUCLEAROFFSET;
static FTYPE TRUENUCLEAROFFSET, DEGENNUCLEAROFFSET;
static FTYPE lsoffset, fakelsoffset;
static int didsetupkazeos;







