// N1M,N2M,N3M here are used correctly w.r.t. global.storage.h


void kazfulleos_set_arrays(void)
{
  void kazfulleos_set_arrays_multidimen(void);
  void kazfulleos_set_arrays_perpoint_perline(void);


  kazfulleos_set_arrays_perpoint_perline();
  kazfulleos_set_arrays_multidimen();


}


void kazfulleos_set_arrays_perpoint_perline(void)
{

  ///////////////////////////////////////////////
  //
  // assign global pointer to eostable (static assignment)
  //
  // Take stuff in within WHICHEOS==KAZFULL in kazfulleos.eostablesdefs.h and regexp:   FTYPEEOS BASEEOSMAC(\([a-zA-Z]+\),\([a-zA-Z0-9,]+\));    -> EOSPOINT(\1) = (FTYPEEOS PTREOSMAC(\1,\2)) (&(BASEEOSMAC(\1,0,0,0,0,0,0,0)));
  //
  // Note: arrays accessed using "coli" starting at 0, not "whichfun".  Simplifies code here so don't have to include offsets
  //
  // SUPERNOTE: Below must be consistent with kazfulleos.eostablesdefs.h and kazfulleos.global.?.h and *.c associated with reading and writing to arrays
  //
  //////////////////////////////////////////////
#if(WHICHEOS==KAZFULL)

  // full
  EOSPOINT(eosfulltabledegen) = (FTYPEEOS PTREOSMAC(eosfulltabledegen,NUMEOSDEGENQUANTITIESMEM1,EOSFULLDEGENN5,EOSFULLDEGENN4,EOSFULLDEGENN3,EOSFULLDEGENN2,EOSFULLDEGENN1,NUMEOSDEGENQUANTITIESMEM2)) (&(BASEEOSMAC(eosfulltabledegen,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltablestandard) = (FTYPEEOS PTREOSMAC(eosfulltablestandard,1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSSTANDARDQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltablestandard,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltableguess) = (FTYPEEOS PTREOSMAC(eosfulltableguess,1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSGUESSQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltableguess,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltablediss) = (FTYPEEOS PTREOSMAC(eosfulltablediss,1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSDISSQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltablediss,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltabledp) = (FTYPEEOS PTREOSMAC(eosfulltabledp,1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSDPQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltabledp,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltablesden) = (FTYPEEOS PTREOSMAC(eosfulltablesden,1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSSDENQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltablesden,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltablesspec) = (FTYPEEOS PTREOSMAC(eosfulltablesspec,1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSSSPECQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltablesspec,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltablepofchi) = (FTYPEEOS PTREOSMAC(eosfulltablepofchi,1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSPOFCHIQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltablepofchi,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltabletemp) = (FTYPEEOS PTREOSMAC(eosfulltabletemp,NUMEOSTEMPQUANTITIESMEM1,EOSFULLN5,EOSFULLN4,EOSFULLN3,EOSFULLN2,EOSFULLN1,NUMEOSTEMPQUANTITIESMEM2)) (&(BASEEOSMAC(eosfulltabletemp,0,0,0,0,0,0,0)));

  // different size for extra table
  EOSPOINT(eosfulltableextra) = (FTYPEEOS PTREOSMAC(eosfulltableextra,1,EOSEXTRAFULLN5,EOSEXTRAFULLN4,EOSEXTRAFULLN3,EOSEXTRAFULLN2,EOSEXTRAFULLN1,NUMEOSEXTRAQUANTITIESMEM)) (&(BASEEOSMAC(eosfulltableextra,0,0,0,0,0,0,0)));
#if(WHICHDATATYPEGENERAL==4)
  EOSPOINT(eosfulltableextradegen) = (FTYPEEOS PTREOSMAC(eosfulltableextradegen,NUMEOSDEGENQUANTITIESMEM1,EOSEXTRAFULLDEGENN5,EOSEXTRAFULLDEGENN4,EOSEXTRAFULLDEGENN3,EOSEXTRAFULLDEGENN2,EOSEXTRAFULLDEGENN1,NUMEOSDEGENQUANTITIESMEM2)) (&(BASEEOSMAC(eosfulltableextradegen,0,0,0,0,0,0,0)));
  EOSPOINT(eosfulltableextratemp) = (FTYPEEOS PTREOSMAC(eosfulltableextratemp,NUMEOSTEMPQUANTITIESMEM1,EOSEXTRAFULLTEMPN5,EOSEXTRAFULLTEMPN4,EOSEXTRAFULLTEMPN3,EOSEXTRAFULLTEMPN2,EOSEXTRAFULLTEMPN1,NUMEOSTEMPQUANTITIESMEM2)) (&(BASEEOSMAC(eosfulltableextratemp,0,0,0,0,0,0,0)));
#endif

  // simple
  EOSPOINT(eossimpletabledegen) = (FTYPEEOS PTREOSMAC(eossimpletabledegen,NUMEOSDEGENQUANTITIESMEM1,EOSSIMPLEDEGENN5,EOSSIMPLEDEGENN4,EOSSIMPLEDEGENN3,EOSSIMPLEDEGENN2,EOSSIMPLEDEGENN1,NUMEOSDEGENQUANTITIESMEM2)) (&(BASEEOSMAC(eossimpletabledegen,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletablestandard) = (FTYPEEOS PTREOSMAC(eossimpletablestandard,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSSTANDARDQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletablestandard,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletableguess) = (FTYPEEOS PTREOSMAC(eossimpletableguess,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSGUESSQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletableguess,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletablediss) = (FTYPEEOS PTREOSMAC(eossimpletablediss,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSDISSQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletablediss,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletabledp) = (FTYPEEOS PTREOSMAC(eossimpletabledp,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSDPQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletabledp,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletablesden) = (FTYPEEOS PTREOSMAC(eossimpletablesden,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSSDENQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletablesden,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletablesspec) = (FTYPEEOS PTREOSMAC(eossimpletablesspec,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSSSPECQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletablesspec,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletablepofchi) = (FTYPEEOS PTREOSMAC(eossimpletablepofchi,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSPOFCHIQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletablepofchi,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletabletemp) = (FTYPEEOS PTREOSMAC(eossimpletabletemp,NUMEOSTEMPQUANTITIESMEM1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSTEMPQUANTITIESMEM2)) (&(BASEEOSMAC(eossimpletabletemp,0,0,0,0,0,0,0)));

  // different size for extra table
  EOSPOINT(eossimpletableextra) = (FTYPEEOS PTREOSMAC(eossimpletableextra,1,EOSEXTRASIMPLEN5,EOSEXTRASIMPLEN4,EOSEXTRASIMPLEN3,EOSEXTRASIMPLEN2,EOSEXTRASIMPLEN1,NUMEOSEXTRAQUANTITIESMEM)) (&(BASEEOSMAC(eossimpletableextra,0,0,0,0,0,0,0)));
#if(WHICHDATATYPEGENERAL==4)
  EOSPOINT(eossimpletableextradegen) = (FTYPEEOS PTREOSMAC(eossimpletableextradegen,NUMEOSDEGENQUANTITIESMEM1,EOSEXTRASIMPLEDEGENN5,EOSEXTRASIMPLEDEGENN4,EOSEXTRASIMPLEDEGENN3,EOSEXTRASIMPLEDEGENN2,EOSEXTRASIMPLEDEGENN1,NUMEOSDEGENQUANTITIESMEM2)) (&(BASEEOSMAC(eossimpletableextradegen,0,0,0,0,0,0,0)));
  EOSPOINT(eossimpletableextratemp) = (FTYPEEOS PTREOSMAC(eossimpletableextratemp,NUMEOSTEMPQUANTITIESMEM1,EOSEXTRASIMPLETEMPN5,EOSEXTRASIMPLETEMPN4,EOSEXTRASIMPLETEMPN3,EOSEXTRASIMPLETEMPN2,EOSEXTRASIMPLETEMPN1,NUMEOSTEMPQUANTITIESMEM2)) (&(BASEEOSMAC(eossimpletableextratemp,0,0,0,0,0,0,0)));
#endif



  // simple zoom
  EOSPOINT(eossimplezoomtabledegen) = (FTYPEEOS PTREOSMAC(eossimplezoomtabledegen,NUMEOSDEGENQUANTITIESMEM1,EOSSIMPLEZOOMDEGENN5,EOSSIMPLEZOOMDEGENN4,EOSSIMPLEZOOMDEGENN3,EOSSIMPLEZOOMDEGENN2,EOSSIMPLEZOOMDEGENN1,NUMEOSDEGENQUANTITIESMEM2)) (&(BASEEOSMAC(eossimplezoomtabledegen,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtablestandard) = (FTYPEEOS PTREOSMAC(eossimplezoomtablestandard,1,EOSSIMPLEZOOMN5,EOSSIMPLEZOOMN4,EOSSIMPLEZOOMN3,EOSSIMPLEZOOMN2,EOSSIMPLEZOOMN1,NUMEOSSTANDARDQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtablestandard,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtableguess) = (FTYPEEOS PTREOSMAC(eossimplezoomtableguess,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSGUESSQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtableguess,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtablediss) = (FTYPEEOS PTREOSMAC(eossimplezoomtablediss,1,EOSSIMPLEN5,EOSSIMPLEN4,EOSSIMPLEN3,EOSSIMPLEN2,EOSSIMPLEN1,NUMEOSDISSQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtablediss,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtabledp) = (FTYPEEOS PTREOSMAC(eossimplezoomtabledp,1,EOSSIMPLEZOOMN5,EOSSIMPLEZOOMN4,EOSSIMPLEZOOMN3,EOSSIMPLEZOOMN2,EOSSIMPLEZOOMN1,NUMEOSDPQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtabledp,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtablesden) = (FTYPEEOS PTREOSMAC(eossimplezoomtablesden,1,EOSSIMPLEZOOMN5,EOSSIMPLEZOOMN4,EOSSIMPLEZOOMN3,EOSSIMPLEZOOMN2,EOSSIMPLEZOOMN1,NUMEOSSDENQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtablesden,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtablesspec) = (FTYPEEOS PTREOSMAC(eossimplezoomtablesspec,1,EOSSIMPLEZOOMN5,EOSSIMPLEZOOMN4,EOSSIMPLEZOOMN3,EOSSIMPLEZOOMN2,EOSSIMPLEZOOMN1,NUMEOSSSPECQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtablesspec,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtablepofchi) = (FTYPEEOS PTREOSMAC(eossimplezoomtablepofchi,1,EOSSIMPLEZOOMN5,EOSSIMPLEZOOMN4,EOSSIMPLEZOOMN3,EOSSIMPLEZOOMN2,EOSSIMPLEZOOMN1,NUMEOSPOFCHIQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtablepofchi,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtabletemp) = (FTYPEEOS PTREOSMAC(eossimplezoomtabletemp,NUMEOSTEMPQUANTITIESMEM1,EOSSIMPLEZOOMN5,EOSSIMPLEZOOMN4,EOSSIMPLEZOOMN3,EOSSIMPLEZOOMN2,EOSSIMPLEZOOMN1,NUMEOSTEMPQUANTITIESMEM2)) (&(BASEEOSMAC(eossimplezoomtabletemp,0,0,0,0,0,0,0)));

  // different size for extra table
  EOSPOINT(eossimplezoomtableextra) = (FTYPEEOS PTREOSMAC(eossimplezoomtableextra,1,EOSEXTRASIMPLEZOOMN5,EOSEXTRASIMPLEZOOMN4,EOSEXTRASIMPLEZOOMN3,EOSEXTRASIMPLEZOOMN2,EOSEXTRASIMPLEZOOMN1,NUMEOSEXTRAQUANTITIESMEM)) (&(BASEEOSMAC(eossimplezoomtableextra,0,0,0,0,0,0,0)));
#if(WHICHDATATYPEGENERAL==4)
  EOSPOINT(eossimplezoomtableextradegen) = (FTYPEEOS PTREOSMAC(eossimplezoomtableextradegen,NUMEOSDEGENQUANTITIESMEM1,EOSEXTRASIMPLEZOOMDEGENN5,EOSEXTRASIMPLEZOOMDEGENN4,EOSEXTRASIMPLEZOOMDEGENN3,EOSEXTRASIMPLEZOOMDEGENN2,EOSEXTRASIMPLEZOOMDEGENN1,NUMEOSDEGENQUANTITIESMEM2)) (&(BASEEOSMAC(eossimplezoomtableextradegen,0,0,0,0,0,0,0)));
  EOSPOINT(eossimplezoomtableextratemp) = (FTYPEEOS PTREOSMAC(eossimplezoomtableextratemp,NUMEOSTEMPQUANTITIESMEM1,EOSEXTRASIMPLEZOOMTEMPN5,EOSEXTRASIMPLEZOOMTEMPN4,EOSEXTRASIMPLEZOOMTEMPN3,EOSEXTRASIMPLEZOOMTEMPN2,EOSEXTRASIMPLEZOOMTEMPN1,NUMEOSTEMPQUANTITIESMEM2)) (&(BASEEOSMAC(eossimplezoomtableextratemp,0,0,0,0,0,0,0)));
#endif

#endif // end if(WHICHEOS==KAZFULL)


}




void kazfulleos_set_arrays_multidimen(void)
{

#if(WHICHEOS==KAZFULL)

  GLOBALPOINT(EOSextraglobal) = (FTYPE PTRMACP0A1(EOSextraglobal,N1M,N2M,N3M,NUMEOSGLOBALS)) (&(BASEMACP0A1(EOSextraglobal,N1BND,N2BND,N3BND,-FIRSTEOSGLOBAL))); // -FIRSTEOSGLOBAL so EOSextraglobal[FIRSTEOSGLOBAL] is a_EOSextraglobal[0]

  // indicate that IGLOBAL and other things have not been set yet.
  // Used to indicate if starting iterating on Ynu0 yet
  int i,j,k;
  COMPFULLLOOP{
    GLOBALMACP0A1(EOSextraglobal,i,j,k,IGLOBAL) = -100;
    GLOBALMACP0A1(EOSextraglobal,i,j,k,JGLOBAL) = -100;
    GLOBALMACP0A1(EOSextraglobal,i,j,k,KGLOBAL) = -100;
  }


#endif





}
