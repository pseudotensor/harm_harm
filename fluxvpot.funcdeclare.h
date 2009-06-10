extern int vpot2field_useflux(int *fieldloc,FTYPE (*pfield)[NSTORE2][NSTORE3][NPR],FTYPE (*ufield)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);
extern int vpot2field_centeredfield(FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*pfield)[NSTORE2][NSTORE3][NPR],FTYPE (*ufield)[NSTORE2][NSTORE3][NPR]);
extern int vpot2field_staggeredfield(FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3],FTYPE (*pfield)[NSTORE2][NSTORE3][NPR],FTYPE (*ufield)[NSTORE2][NSTORE3][NPR]);
extern int interpolate_ustag2fieldcent(int stage, SFTYPE boundtime, int timeorder, int numtimeorders, FTYPE (*preal)[NSTORE2][NSTORE3][NPR],FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*ucent)[NSTORE2][NSTORE3][NPR],FTYPE (*pcent)[NSTORE2][NSTORE3][NPR]);
extern int vectorpot_fluxreconorfvavg(int stage, FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*A)[NSTORE1+SHIFTSTORE1][NSTORE2+SHIFTSTORE2][NSTORE3+SHIFTSTORE3], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);
extern int deaverage_fields_fv(FTYPE (*primreal)[NSTORE2][NSTORE3][NPR], FTYPE (*in)[NSTORE2][NSTORE3][NPR], FTYPE (*out)[NSTORE2][NSTORE3][NPR]);
extern int field_integrate_fluxrecon(int stage, FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*quasifield)[NSTORE2][NSTORE3][NPR], FTYPE (*pointfield)[NSTORE2][NSTORE3][NPR]);
extern int vectorpot_useflux(int stage, FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*F1)[NSTORE2][NSTORE3][NPR], FTYPE (*F2)[NSTORE2][NSTORE3][NPR], FTYPE (*F3)[NSTORE2][NSTORE3][NPR]);
extern int field_Bhat_fluxrecon(FTYPE (*pr)[NSTORE2][NSTORE3][NPR], FTYPE (*pointfield)[NSTORE2][NSTORE3][NPR], FTYPE (*quasifield)[NSTORE2][NSTORE3][NPR]);
extern int ucons2upointppoint(SFTYPE boundtime, FTYPE (*pfield)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR],FTYPE (*unew)[NSTORE2][NSTORE3][NPR],FTYPE (*ulast)[NSTORE2][NSTORE3][NPR],FTYPE (*pcent)[NSTORE2][NSTORE3][NPR]);

extern int deaverage_ustag2pstag(FTYPE (*preal)[NSTORE2][NSTORE3][NPR], FTYPE (*ustag)[NSTORE2][NSTORE3][NPR], FTYPE (*pstag)[NSTORE2][NSTORE3][NPR]);

