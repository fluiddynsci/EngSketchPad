#
# Written by Dr. Ryan Durscher AFRL/RQVC
# 
# This software has been cleared for public release on 25 Jul 2018, case number 88ABW-2018-3793.

cimport cEGADS
cimport cCAPS

cdef extern from "aimUtil.h":
    int aim_getBodies( void *aimInfo, int *nBody, cEGADS.ego **bodies )
  
    int aim_newGeometry( void *aimInfo )

    int aim_convert( void *aimInfo, const char  *inUnits, double   inValue,
                     const char *outUnits, double *outValue )
                     
    int aim_getIndex( void *aimInfo, const char *name, cCAPS.capssType subtype )
  
    int aim_getName( void *aimInfo, int index, cCAPS.capssType subtype,
                     const char **name )
  
    int aim_getData( void *aimInfo, const char *name, cCAPS.capsvType *vtype,
                     int *rank, int *nrow, int *ncol, void **data, char **units )
  
    int aim_link( void *aimInfo, const char *name, cCAPS.capssType stype,
                  cCAPS.capsValue *val )
  
    int aim_setTess( void *aimInfo, cEGADS.ego tess )
  
    int aim_getDiscr( void *aimInfo, const char *tname, cCAPS.capsDiscr **discr )
  
    int aim_getDataSet( cCAPS.capsDiscr *discr, const char *dname, cCAPS.capsdMethod *method,
                        int *npts, int *rank, double **data )

    int aim_getBounds( void *aimInfo, int *nTname, char ***tnames )
  
    int aim_setSensitivity( void *aimInfo, const char *GIname, int irow, int icol )
  
    int aim_getSensitivity( void *aimInfo, cEGADS.ego body, int ttype, int index, int *npts,
                            double **dxyz )

    int aim_sensitivity( void *aimInfo, const char *GIname, int irow, int icol,
                         cEGADS.ego tess, int *npts, double **dxyz )
