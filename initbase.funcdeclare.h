
// initialize stuff
// specific to init.c's and used in initbase.c and init.c, so leave global
extern int post_init_specific_init(void);
extern int pre_init_specific_init(void);
extern int prepre_init_specific_init(void);
extern int init_consts(void);
extern int init_grid(void);
extern int init_global(void);
extern int init_defcoord(void);

extern int init_primitives(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*panalytic)[NSTORE2][NSTORE3][NPR], FTYPE (*pstaganalytic)[NSTORE2][NSTORE3][NPR], FTYPE (*vpotanalytic)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhatanalytic)[NSTORE2][NSTORE3][NPR],FTYPE (*F1)[NSTORE2][NSTORE3][NPR],FTYPE (*F2)[NSTORE2][NSTORE3][NPR],FTYPE (*F3)[NSTORE2][NSTORE3][NPR], FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3]);

extern int copy_prim2panalytic(FTYPE (*prim)[NSTORE2][NSTORE3][NPR],FTYPE (*panalytic)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*pstaganalytic)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*vpotanalytic)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*Bhatanalytic)[NSTORE2][NSTORE3][NPR]);


extern int init_vpot(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR], FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3]);

extern int vpot2field(FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*pfield)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR], FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR]);

extern int update_vpot(int stage, FTYPE (*pr)[NSTORE2][NSTORE3][NPR],FTYPE (*ptrfluxvec[NDIM])[NSTORE2][NSTORE3][NPR], FTYPE CUf,FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3]);

extern int normalize_field_withnorm(FTYPE norm, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR]);
extern int assign_fieldconservatives_pointvalues(FTYPE (*prim)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*ucons)[NSTORE2][NSTORE3][NPR]);
extern void setfdivb(FTYPE *divb, FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*U)[NSTORE2][NSTORE3][NPR], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], int i, int j, int k);

extern int copy_vpot2flux(FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);
extern int evolve_withvpot(FTYPE (*prim)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*unew)[NSTORE2][NSTORE3][NPR],FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR], FTYPE (*Atemp)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*uconstemp)[NSTORE2][NSTORE3][NPR]);

extern int init_vpot_user(int *whichcoord, int l, int i, int j, int k, FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE *V, FTYPE *A);
extern int init_vpot2field_user(FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR]);

extern int field_Bhat_fluxrecon(FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*pointfield)[NSTORE2][NSTORE3][NPR], FTYPE (*quasifield)[NSTORE2][NSTORE3][NPR]);


extern int transform_primitive_vB(int whichvel, int whichcoord, int i,int j, int k, FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR]);
extern int transform_primitive_pstag(int whichvel, int whichcoord, int i,int j, int k, FTYPE (*p)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR]);
extern int init_zero_field(FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*ucons)[NSTORE2][NSTORE3][NPR],FTYPE (*vpot)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*Bhat)[NSTORE2][NSTORE3][NPR]);

extern int pi2Uavg(int *fieldfrompotential, FTYPE (*prim)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR], FTYPE (*Upoint)[NSTORE2][NSTORE3][NPR], FTYPE (*Uavg)[NSTORE2][NSTORE3][NPR]);

extern void set_default_nprlists(void);

extern int addremovefieldfromnpr(int doadd, int *whichpltoavg, int *ifnotavgthencopy, int interptype, int dir, int *nprlocalstart, int *nprlocalend, int *nprlocallist, FTYPE (*current_in)[NSTORE2][NSTORE3][NPR], FTYPE (*current_out)[NSTORE2][NSTORE3][NPR]);
extern int addremovefromnpr(int doadd, int *whichpltoavg, int *ifnotavgthencopy, int *nprlocalstart, int *nprlocalend, int *nprlocallist, FTYPE (*in)[NSTORE2][NSTORE3][NPR], FTYPE (*out)[NSTORE2][NSTORE3][NPR]);

extern int addremovefromanynpr(int doadd, int *whichpltoavg, int *ifnotavgthencopy, int *anynprstart, int *anynprend, int *anynprlist, int *nprlocalstart, int *nprlocalend, int *nprlocallist, FTYPE (*in)[NSTORE2][NSTORE3][NPR], FTYPE (*out)[NSTORE2][NSTORE3][NPR]);

