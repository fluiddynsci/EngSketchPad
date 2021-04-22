#ifndef CAPS_H
#define CAPS_H
/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Function Prototypes
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


/* base-level object functions */

__ProtoExt__ int
  caps_info( const capsObj object, char **name, enum capsoType *type,
             enum capssType *subtype, capsObj *link, capsObj *parent,
             capsOwn *last );
  
__ProtoExt__ int
  caps_size( const capsObj object, enum capsoType type, enum capssType stype,
             int *size );
  
__ProtoExt__ int
  caps_childByIndex( const capsObj object, enum capsoType type,
                     enum capssType stype, int index, capsObj *child );

__ProtoExt__ int
  caps_childByName( const capsObj object, enum capsoType typ,
                    enum capssType styp, const char *name, capsObj *child );
  
__ProtoExt__ int
  caps_bodyByIndex( const capsObj pobject, int index, ego *body, char **units );
  
__ProtoExt__ int
  caps_ownerInfo( const capsOwn owner, char **pname, char **pID, char **userID,
                  short *datetime, CAPSLONG *sNum );
  
__ProtoExt__ int
  caps_setOwner( const capsObj pobject, const char *pname, capsOwn *owner );
  
__ProtoExt__ void
  caps_freeOwner( capsOwn *owner );

__ProtoExt__ int
  caps_delete( capsObj object );
  
__ProtoExt__ int
  caps_errorInfo( capsErrs *errs, int eIndex, capsObj *errObj, int *nLines,
                  char ***lines );

__ProtoExt__ int
  caps_freeError( /*@only@*/ capsErrs *errs );
  
__ProtoExt__ void
  caps_freeValue( capsValue *value );


/* attribute functions */

__ProtoExt__ int
  caps_attrByName( capsObj cobj, char *name, capsObj *attr );
  
__ProtoExt__ int
  caps_attrByIndex( capsObj cobj, int index, capsObj *attr );
  
__ProtoExt__ int
  caps_setAttr( capsObj cobj, /*@null@*/ const char *name, capsObj aval );
  
__ProtoExt__ int
  caps_deleleAttr( capsObj cobj, /*@null@*/ char *name );

  
/* problem functions */

__ProtoExt__ int
  caps_open( const char *filename, const char *pname, capsObj *pobject );
  
__ProtoExt__ int
  caps_save( capsObj pobject, /*@null@*/ const char *filename );
  
__ProtoExt__ int
  caps_close( capsObj pobject );
  
__ProtoExt__ int
  caps_outLevel( capsObj pobject, int outLevel );
  

/* analysis functions */

__ProtoExt__ int
  caps_queryAnalysis( capsObj pobj, const char *aname, int *nIn, int *nOut,
                      int *execution );
  
__ProtoExt__ int
  caps_getBodies( const capsObject *aobject, int *nBody, ego **bodies );
  
__ProtoExt__ int
  caps_getInput( capsObj pobj, const char *aname, int index, char **ainame,
                 capsValue *defaults );
  
__ProtoExt__ int
  caps_getOutput( capsObj pobj, const char *aname, int index, char **aoname,
                  capsValue *form );
  
__ProtoExt__ int
  caps_AIMbackdoor( const capsObject *aobject, const char *JSONin,
                    char **JSONout );
  
__ProtoExt__ int
  caps_load( capsObj pobj, const char *anam, const char *apath,
             /*@null@*/ const char *unitSys, /*@null@*/ const char *intents,
             int nparent, /*@null@*/ capsObj *parents, capsObj *aobj );

__ProtoExt__ int
  caps_dupAnalysis( capsObj from, const char *apath, int nparent,
                    /*@null@*/ capsObj *parents,capsObj *aobj );
  
__ProtoExt__ int
  caps_dirtyAnalysis( capsObj pobj, int *nAobj, capsObj **aobjs );
  
__ProtoExt__ int
  caps_analysisInfo( const capsObj aobject, char **apath, char **unitSys,
                     char **intents, int *nparent, capsObj **parents,
                     int *nField, char ***fnames, int **ranks, int *execution,
                     int *status );
  
__ProtoExt__ int
  caps_preAnalysis( capsObj aobject, int *nErr, capsErrs **errors );
  
__ProtoExt__ int
  caps_postAnalysis( capsObj aobject, capsOwn current, int *nErr,
                     capsErrs **errors );
  
  
/* bound, vertexset and dataset functions */
  
__ProtoExt__ int
  caps_makeBound( capsObj pobject, int dim, const char *bname, capsObj *bobj );
  
__ProtoExt__ int
  caps_boundInfo( const capsObj object, enum capsState *state, int *dim,
                  double *plims );

__ProtoExt__ int
  caps_completeBound( capsObj bobject );
  
__ProtoExt__ int
  caps_fillVertexSets( capsObj bobject, int *nErr, capsErrs **errors );

__ProtoExt__ int
  caps_makeVertexSet( capsObj bobject, /*@null@*/ capsObj aobject,
                      /*@null@*/ const char *vname, capsObj *vobj );

__ProtoExt__ int
  caps_vertexSetInfo( const capsObj vobject, int *nGpts, int *nDpts,
                      capsObj *bobj, capsObj *aobj );
  
__ProtoExt__ int
  caps_outputVertexSet( const capsObj vobject, const char *filename );
 
__ProtoExt__ int
  caps_fillUnVertexSet( capsObj vobject, int npts, const double *xyzs );
  
__ProtoExt__ int
  caps_makeDataSet( capsObj vobject, const char *dname, enum capsdMethod meth,
                    int rank, capsObj *dobj );
  
__ProtoExt__ int
  caps_initDataSet( capsObj dobject, int rank, const double *data );
  
__ProtoExt__ int
  caps_setData( capsObj dobject, int nverts, int rank, const double *data,
                const char *units );

__ProtoExt__ int
  caps_getData( capsObj dobject, int *npts, int *rank, double **data,
                char **units );

__ProtoExt__ int
  caps_getHistory( const capsObj dobject, capsObj *vobject, int *nHist,
                   capsOwn **history );
  
__ProtoExt__ int
  caps_getDataSets( const capsObj bobject, const char *dname, int *nobj,
                    capsObj **dobjs );
  
__ProtoExt__ int
  caps_triangulate( const capsObj vobject, int *nGtris, int **gtris,
                                           int *nDtris, int **dtris );

  
/* value functions */
  
__ProtoExt__ int
  caps_getValue( capsObj object, enum capsvType *type, int *vlen,
                 /*@null@*/ const void **data, const char **units, int *nErr,
                 capsErrs **errors );
  
__ProtoExt__ int
  caps_makeValue( capsObj pobject, const char *vname, enum capssType stype,
                  enum capsvType vtype, int nrow, int ncol, const void *data,
                  /*@null@*/ const char *units, capsObj *vobj );
  
__ProtoExt__ int
  caps_setValue( capsObj object, int nrow, int ncol, const void *data );
  
__ProtoExt__ int
  caps_getLimits( const capsObj object, const void **limits );
  
__ProtoExt__ int
  caps_setLimits( capsObj object, void *limits );
  
__ProtoExt__ int
  caps_getValueShape( const capsObj object, int *dim, enum capsFixed *lfix,
                      enum capsFixed *sfix, enum capsNull *ntype,
                      int *nrow, int *ncol );

__ProtoExt__ int
  caps_setValueShape( capsObj object, int dim, enum capsFixed lfixed,
                      enum capsFixed sfixed, enum capsNull ntype );
  
__ProtoExt__ int
  caps_convert( const capsObj object, const char *units, double inp,
                double *outp );

__ProtoExt__ int
  caps_transferValues( capsObj source, enum capstMethod method,
                       capsObj target, int *nErr, capsErrs **errors );
  
__ProtoExt__ int
  caps_makeLinkage( /*@null@*/ capsObj link, enum capstMethod method,
                    capsObj target );
  

/* others */
  
__ProtoExt__ void
  printObjects( capsObj object, int indent );

#ifdef __cplusplus
}
#endif

#endif
