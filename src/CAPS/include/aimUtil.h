#ifndef AIMU_H
#define AIMU_H
/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             AIM Utility Function Prototypes
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "capsTypes.h"

#ifdef __ProtoExt__
#undef __ProtoExt__
#endif
#ifdef __cplusplus
extern "C" {
#define __ProtoExt__
#else
#define __ProtoExt__ extern
#endif

__ProtoExt__ int
  aim_getBodies( void *aimInfo, const char **intent, int *nBody, ego **bodies );
  
__ProtoExt__ int
  aim_newGeometry( void *aimInfo );
  
__ProtoExt__ int
  aim_getUnitSys( void *aimInfo, char **unitSys );

__ProtoExt__ int
  aim_convert( void *aimInfo, const char  *inUnits, double   inValue,
                              const char *outUnits, double *outValue );

__ProtoExt__ int
  aim_unitMultiply( void *aimInfo, const char  *inUnits1, const char *inUnits2,
                    char **outUnits );

__ProtoExt__ int
  aim_unitDivide( void *aimInfo, const char  *inUnits1, const char *inUnits2,
                  char **outUnits );

__ProtoExt__ int
  aim_unitInvert( void *aimInfo, const char  *inUnits,
                  char **outUnits );

__ProtoExt__ int
  aim_unitRaise( void *aimInfo, const char  *inUnits, const int power,
                 char **outUnits );

__ProtoExt__ int
  aim_getIndex( void *aimInfo, /*@null@*/ const char *name,
                enum capssType subtype );
  
__ProtoExt__ int
  aim_getValue( void *aimInfo, int index, enum capssType subtype,
                capsValue ** value );
  
__ProtoExt__ int
  aim_getName( void *aimInfo, int index, enum capssType subtype,
               const char **name );

__ProtoExt__ int
  aim_getGeomInType( void *aimInfo, int index );
  
__ProtoExt__ int
  aim_getData( void *aimInfo, const char *name, enum capsvType *vtype,
               int *rank, int *nrow, int *ncol, void **data, char **units );
  
__ProtoExt__ int
  aim_link( void *aimInfo, const char *name, enum capssType stype,
            capsValue *val );
  
__ProtoExt__ int
  aim_setTess( void *aimInfo, ego tess );
  
__ProtoExt__ int
  aim_getDiscr( void *aimInfo, const char *bname, capsDiscr **discr );
  
__ProtoExt__ int
  aim_getDiscrState( void *aimInfo, const char *bname );
  
__ProtoExt__ int
  aim_getDataSet( capsDiscr *discr, const char *dname, enum capsdMethod *method,
                  int *npts, int *rank, double **data );

__ProtoExt__ int
  aim_getBounds( void *aimInfo, int *nTname, char ***tnames );
  
__ProtoExt__ int
  aim_setSensitivity( void *aimInfo, const char *GIname, int irow, int icol );
  
__ProtoExt__ int
  aim_getSensitivity( void *aimInfo, ego body, int ttype, int index, int *npts,
                      double **dxyz );

__ProtoExt__ int
  aim_sensitivity( void *aimInfo, const char *GIname, int irow, int icol,
                   ego tess, int *npts, double **dxyz );

__ProtoExt__ int
  aim_isNodeBody( ego body, double *xyz );


  /* utility functions for nodal and cell centered data transfers */
__ProtoExt__ int
  aim_nodalTriangleType(capsEleType *eletype);

__ProtoExt__ int
  aim_nodalQuadType(capsEleType *eletype);

__ProtoExt__ int
  aim_cellTriangleType(capsEleType *eletype);

__ProtoExt__ int
  aim_cellQuadType(capsEleType *eletype);

__ProtoExt__ void
  aim_freeEleType(capsEleType *eletype);

__ProtoExt__ int
  aim_FreeDiscr(capsDiscr *discr);

__ProtoExt__ int
  aim_locateElement( capsDiscr *discr, double *params,
                     double *param,    int *eIndex,
                     double *bary);
__ProtoExt__ int
  aim_interpolation(capsDiscr *discr, const char *name, int eIndex,
                    double *bary, int rank, double *data, double *result);

__ProtoExt__ int
  aim_interpolateBar(capsDiscr *discr, const char *name, int eIndex,
                     double *bary, int rank, double *r_bar, double *d_bar);

__ProtoExt__ int
  aim_integration(capsDiscr *discr, const char *name, int eIndex, int rank,
                  double *data, double *result);

__ProtoExt__ int
  aim_integrateBar(capsDiscr *discr, const char *name, int eIndex, int rank,
                   double *r_bar, double *d_bar);


/*************************************************************************/
/* Prototypes of aim entry points to catch incorrect function signatures */

int
aimInitialize( int ngIn, /*@null@*/ capsValue *gIn, int *qeFlag,
               /*@null@*/ const char *unitSys, int *nIn, int *nOut,
               int *nFields, char ***fnames, int **ranks );

int
aimInputs( int inst, void *aimInfo, int index, char **ainame,
           capsValue *defval );

int
aimOutputs( int inst, void *aimInfo, int index, char **aoname,
            capsValue *form );

int
aimPreAnalysis( int inst, void *aimInfo, const char *apath,
                /*@null@*/ capsValue *inputs, capsErrs **errs );

int
aimPostAnalysis( int inst, void *aimInfo, const char *apath, capsErrs **errs );

void
aimCleanup( );

int
aimCalcOutput( int inst, void *aimInfo, const char *apath, int index,
               capsValue *val, capsErrs **errors );

int
aimDiscr( char *tname, capsDiscr *discr );

int
aimFreeDiscr( capsDiscr *discr );

int
aimLocateElement( capsDiscr *discr, double *params, double *param, int *eIndex,
                  double *bary );

int
aimUsesDataSet( int inst, void *aimInfo, const char *bname, const char *dname,
                enum capsdMethod dMethod );

int
aimTransfer( capsDiscr *discr, const char *fname, int npts, int rank,
             double *data, char **units );

int
aimInterpolation( capsDiscr *discr, const char *name, int eIndex,
                  double *bary, int rank, double *data, double *result );

int
aimInterpolateBar( capsDiscr *discr, const char *name, int eIndex,
                   double *bary, int rank, double *r_bar, double *d_bar );

int
aimIntegration( capsDiscr *discr, const char *name, int eIndex, int rank,
                /*@null@*/ double *data, double *result );

int
aimIntegrateBar( capsDiscr *discr, const char *name, int eIndex, int rank,
                 double *r_bar, double *d_bar );

int
aimData( int inst, const char *name, enum capsvType *vtype, int *rank,
         int *nrow, int *ncol, void **data, char **units );

int
aimBackdoor( int inst, void *aimInfo, const char *JSONin, char **JSONout );

/*************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
