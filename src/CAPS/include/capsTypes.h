#ifndef CAPSTYPES_H
#define CAPSTYPES_H
/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             General Object Header
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include "egads.h"
#include "capsErrors.h"


#ifdef WIN32
#define CAPSLONG unsigned long long
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DLL HINSTANCE
#if _MSC_VER >= 1800
#include <stdbool.h>
#else
#define false 0
#define true  (!false)
#endif
#else
#include <stdbool.h>
#define CAPSLONG unsigned long
#include <dlfcn.h>
#define DLL void *
#endif

#define CAPSMAJOR      1
#define CAPSMINOR     18
#define CAPSPROP      CAPSprop: Revision 1.18

#define CAPSMAGIC     1234321
#define MAXANAL       64



enum capsoType   {BODIES=-2, ATTRIBUTES, UNUSED, PROBLEM, VALUE, ANALYSIS,
                  BOUND, VERTEXSET, DATASET};
enum capssType   {NONE, STATIC, PARAMETRIC, GEOMETRYIN, GEOMETRYOUT, BRANCH,
                  PARAMETER, USER, ANALYSISIN, ANALYSISOUT, CONNECTED,
                  UNCONNECTED};
enum capsBoolean {False=false, True=true};
enum capsvType   {Boolean, Integer, Double, String, Tuple, Value};
enum capsvDim    {Scalar, Vector, Array2D};
enum capsFixed   {Change, Fixed};
enum capsNull    {NotAllowed, NotNull, IsNull};
enum capstMethod {Copy, Integrate, Average};
enum capsdMethod {BuiltIn, Sensitivity, Analysis, Interpolate, Conserve, User};
enum capsState   {MultipleError=-2, Open, Empty, Single, Multiple};


/*
 * defines the tuple structure
 */
typedef struct {
  char *name;                   /* the name */
  char *value;                  /* the value for the pair */
} capsTuple;


/*
 * defines the owning information
 */
typedef struct {
  char      *pname;             /* the process name -- NULL from Problem */
  char      *pID;               /* the process ID   -- NULL from Problem */
  char      *user;              /* the user name    -- NULL from Problem */
  short     datetime[6];        /* the date/time stamp */
  CAPSLONG  sNum;               /* the CAPS sequence number */
} capsOwn;


/*
 * defines approximated spline data
 */
typedef struct {
  int    nrank;                 /* number of members in the state-vector */
  int    periodic;              /* 0 nonperiodic, 1 periodic */
  int    nts;                   /* number of spline points */
  double *interp;               /* spline data */
  double trange[2];             /* mapped t range */
  int    ntm;                   /* number of t mapping points */
  double *tmap;                 /* mapping data -- NULL unmapped */
} capsAprx1D;


typedef struct {
  int    nrank;                 /* number of members in the state-vector */
  int    periodic;              /* 0 non, 1 U, 2 V, 3 periodic in both U&V */
  int    nus;                   /* number of spline points in U */
  int    nvs;                   /* number of spline points in V */
  double *interp;               /* spline data */
  double urange[2];             /* mapped U range */
  double vrange[2];             /* mapped V range */
  int    num;                   /* number of U mapping points */
  int    nvm;                   /* number of V mapping points */
  double *uvmap;                /* mapping data -- NULL unmapped */
} capsAprx2D;


/*
 * defines the element discretization type by the number of reference positions
 * (for geometry and optionally data) within the element. For example:
 * simple tri:  nref = 3; ndata = 0; st = {0.0,0.0, 1.0,0.0, 0.0,1.0}
 * simple quad: nref = 4; ndata = 0; st = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0}
 * internal triangles are used for the in/out predicates and represent linear
 *   triangles in [u,v] space.
 * ndata is the number of data referece positions, which can be zero for simple
 *   nodal or isoparametric discretizations.
 * match points are used for conservative transfers. Must be set when data
 *   and geometry positions differ, specifically for discontinuous mappings.
 */
typedef struct {
  int    nref;                  /* number of geometry reference points */
  int    ndata;                 /* number of data ref points -- 0 data at ref */
  int    nmat;                  /* number of match points (0 -- match at
                                   geometry reference points) */
  int    ntri;                  /* number of triangles to represent the elem */
  double *gst;                  /* [s,t] geom reference coordinates in the
                                   element -- 2*nref in length */
  double *dst;                  /* [s,t] data reference coordinates in the
                                   element -- 2*ndata in length */
  double *matst;                /* [s,t] positions for match points - NULL
                                   when using reference points (2*nmat long) */
  int    *tris;                 /* the triangles defined by reference indices
                                   (bias 1) -- 3*ntri in length */
} capsEleType;


/*
 * defines the element discretization for geometric and optionally data
 * positions.
 */
typedef struct {
  int   bIndex;                 /* the body index (bias 1) */
  int   tIndex;                 /* the element type index (bias 1) */
  int   eIndex;                 /* element owning index -- dim 1 Edge, 2 Face */
  int   *gIndices;              /* local indices (bias 1) geom ref positions,
                                   tess index -- 2*nref in length */
  int   *dIndices;              /* the vertex indices (bias 1) for data ref
                                   positions -- ndata in length or NULL */
  union {
    int tq[2];                  /* tri or quad (bias 1) for ntri <= 2 */
    int *poly;                  /* the multiple indices (bias 1) for ntri > 2 */
  } eTris;                      /* triangle indices that make up the element */
} capsElement;


/*
 * defines a discretized collection of Elements
 *
 * specifies the connectivity based on a collection of Element Types and the
 * elements referencing the types. nPoints refers to the number of indices
 * referenced by the geometric positions in the element which may be different
 * from nVerts which is the number of positions used for the data representation
 * in the element. For simple nodal or isoparametric discretizations, nVerts is
 * zero and verts is set to NULL.
 */
typedef struct {
  int         dim;              /* dimensionality [1-3] */
  int         instance;         /* analysis instance */
  /*@dependent@*/
  void        *aInfo;           /* AIM info */
                                /* below handled by the AIMs: */
  int         nPoints;          /* number of entries in the geom positions */
  int         *mapping;         /* tessellation indices to this local space
                                   2*nPoints in len (body, global tess index) */
  int         nVerts;           /* number data ref positions or unconnected */
  double      *verts;           /* data ref positions -- NULL if same as geom */
  int         *celem;           /* element containing vert or NULL */
  int         nTypes;           /* number of Element Types */
  capsEleType *types;           /* the Element Types (nTypes in length) */
  int         nElems;           /* number of Elements */
  capsElement *elems;           /* the Elements (nElems in length) */
  int         nDtris;           /* number of triangles to plot data */
  int         *dtris;           /* NULL for NULL verts -- indices into verts */
  void        *ptrm;            /* pointer for optional AIM use */
} capsDiscr;


/*
 * defines the CAPS object
 */
typedef struct capsObject {
  int     magicnumber;          /* must be set to validate the object */
  int     type;                 /* object type */
  int     subtype;              /* object subtype */
  int     sn;                   /* object serial number for I/O */
  char    *name;                /* object name */
  egAttrs *attrs;               /* object attributes */
  void    *blind;               /* blind pointer to object data */
  capsOwn last;                 /* last to modify the object */
  struct  capsObject *parent;
} capsObject;
typedef struct capsObject* capsObj;


/*
 * defines the error structures
 */
typedef struct {
  capsObject *errObj;           /* the offending object pointer */
  int        index;             /* index to offending struct -- AIM */
  int        nLines;            /* the number of error strings */
  char       **lines;           /* the error strings */
} capsError;

typedef struct {
  int       nError;             /* number of errors in this structure */
  capsError *errors;            /* the errors */
} capsErrs;


/*
 * structure for CAPS object -- VALUE
 */
typedef struct {
  int          type;            /* value type */
  int          length;          /* number of values */
  int          dim;             /* the dimension */
  int          nrow;            /* number of rows */
  int          ncol;            /* the number of columns */
  int          lfixed;          /* length is fixed */
  int          sfixed;          /* shape is fixed */
  int          nullVal;         /* NULL handling */
  int          pIndex;          /* parent index for vType = Value */
  union {
    int        integer;         /* single int -- length == 1 */
    int        *integers;       /* multiple ints */
    double     real;            /* single double -- length == 1 */
    double     *reals;          /* mutiple doubles */
    char       *string;         /* character string (no single char) */
    capsTuple  *tuple;          /* tuple (no single tuple) */
    capsObject *object;         /* single object */
    capsObject **objects;       /* multiple objects */
  } vals;
  union {
    int        ilims[2];        /* integer limits */
    double     dlims[2];        /* double limits */
  } limits;
  char         *units;          /* the units for the values */
  capsObject   *link;           /* the linked object (or NULL) */
  int          linkMethod;      /* the link method */
} capsValue;


/*
 * AIM declarations
 */

typedef int  (*aimI) (int, /*@null@*/ capsValue *, int *, /*@null@*/ const char *,
                      int *, int *, int *, char ***, int **);
typedef int  (*aimD) (char *, capsDiscr *);
typedef int  (*aimF) (/*@null@*/ capsDiscr *);
typedef int  (*aimL) (capsDiscr *, double *, double *, int *, double *);
typedef int  (*aimIn)(int, /*@null@*/ void *, int, char **, capsValue *);
typedef int  (*aimU) (int, void *, const char *, const char *, enum capsdMethod);
typedef int  (*aimA) (int, void *, const char *, /*@null@*/ capsValue *,
                      capsErrs **);
typedef int  (*aimPo)(int, void *, const char *, capsErrs **);
typedef int  (*aimO) (int, /*@null@*/ void *, int, char **, capsValue *);
typedef int  (*aimC) (int, void *, const char *, int, capsValue *, capsErrs **);
typedef int  (*aimT) (capsDiscr *, const char *, int, int, double *, char **);
typedef int  (*aimP) (capsDiscr *, const char *, int, double *, int, double *,
                      double *);
typedef int  (*aimG) (capsDiscr *, const char *, int, int, /*@null@*/ double *,
                      double *);
typedef int  (*aimDa)(int, const char *, enum capsvType *, int *, int *,
                      int *, void **, char **);
typedef int  (*aimBd)(int, void *, const char *, char **);
typedef void (*aimCU)(void);

typedef struct {
  int   aim_nAnal;
  char *aimName[MAXANAL];
  DLL   aimDLL[MAXANAL];
  aimI  aimInit[MAXANAL];
  aimD  aimDiscr[MAXANAL];
  aimF  aimFreeD[MAXANAL];
  aimL  aimLoc[MAXANAL];
  aimIn aimInput[MAXANAL];
  aimU  aimUsesDS[MAXANAL];
  aimA  aimPAnal[MAXANAL];
  aimPo aimPost[MAXANAL];
  aimO  aimOutput[MAXANAL];
  aimC  aimCalc[MAXANAL];
  aimT  aimXfer[MAXANAL];
  aimP  aimIntrp[MAXANAL];
  aimP  aimIntrpBar[MAXANAL];
  aimG  aimIntgr[MAXANAL];
  aimG  aimIntgrBar[MAXANAL];
  aimDa aimData[MAXANAL];
  aimBd aimBdoor[MAXANAL];
  aimCU aimClean[MAXANAL];
} aimContext;


/*
 * structure for CAPS object -- PROBLEM
 */
typedef struct {
  char       **signature;
  ego        context;            /* the EGADS context object */
  void       *utsystem;          /* the units system */
  aimContext aimFPTR;            /* the aim Function Pointers */
  char       *pfile;             /* problem file name */
  char       *filename;          /* Problem original geometry file name */
  char       *file;              /* a copy of the actual file */
  CAPSLONG   fileLen;            /* number of bytes in the file */
  capsOwn    writer;             /* the owning info of a Problem writer */
  int        outLevel;		 /* output level for messages
                                    0 none, 1 minimal, 2 verbose, 3 debug */
  void       *modl;              /* OpenCSM model void pointer or static ego */
  int        nParam;             /* number of parameters */
  capsObject **params;           /* list of parameter objects */
  int        nBranch;            /* number of branches -- 0 static */
  capsObject **branchs;          /* list of branch objects */
  int        nGeomIn;            /* number of geometryIn (params) -- 0 static */
  capsObject **geomIn;           /* list of geometryIn objects */
  int        nGeomOut;           /* number of geometryOut objects */
  capsObject **geomOut;          /* list of geometryOut objects */
  int        nAnalysis;          /* number of Analysis objects */
  capsObject **analysis;         /* list of Analysis objects */
  int        nBound;             /* number of Bound objects */
  capsObject **bounds;           /* list of Bound objects */
  capsOwn    geometry;           /* the owning info of the geometry */
  int        nBodies;            /* number of current geometric bodies */
  ego        *bodies;            /* the EGADS bodies */
  char       **lunits;           /* the body-based length units */
  CAPSLONG   sNum;               /* sequence number */
} capsProblem;


/*
 * structure for AIM Information
 */
typedef struct {
  int         magicnumber;     /* the magic number */
  int         pIndex;          /* the OpenCSM parameter index - sensitivities */
  int         irow;            /* the parameter row index */
  int         icol;            /* the parameter column index */
  capsProblem *problem;        /* problem structure */
  /*@dependent@*/
  void        *analysis;       /* specific analysis structure */
} aimInfo;


/*
 * structure for CAPS object -- ANALYSIS
 */
typedef struct {
  char       *loadName;         /* so/DLL name */
  char       *path;             /* filesystem path to read/write files */
  char       *unitSys;          /* the unit system used */
  int        instance;          /* this objects index into the so/DLL */
  int        eFlag;             /* execution flag - 1 AIM executes analysis */
  /*@null@*/
  char       *intents;          /* the intents requested for the instance
                                   NULL - all bodies given to the AIM */
  aimInfo    info;              /* data to pass to AIMs to connect with CAPS */
  int        nField;            /* number of fields analysis will respond to */
  char       **fields;          /* the field names for DataSet filling */
  int        *ranks;            /* the ranks associated with each field */
  int        nAnalysisIn;       /* number of Analysis Input objects */
  capsObject **analysisIn;      /* list of Analysis Input objects */
  int        nAnalysisOut;      /* number of Analysis Output objects */
  capsObject **analysisOut;     /* list of Analysis Output objects */
  int        nParent;           /* number of Parent Analysis objects */
  capsObject **parents;         /* list of Parent Analysis objects */
  int        nBody;             /* number of Bodies for this Analysis */
  ego        *bodies;           /* the bodies */
  capsOwn    pre;               /* preAnalysis time/date stamp */
} capsAnalysis;


/*
 * structure for CAPS object -- BOUND
 */
typedef struct {
  int        dim;               /* dimensionality -- 1, 2 or 3 */
  int        state;             /* the current state of the Bound */
  char       *lunits;           /* the length units used */
  double     plimits[4];        /* parameter limits (dim 1 - 2, dim 2 - 4) */
  ego        geom;              /* the geometry for a single entity */
  int        iBody;             /* body index for a single entity */
  int        iEnt;              /* entity index for a single entity */
  capsAprx1D *curve;            /* dim == 1; NULL for single Edge */
  capsAprx2D *surface;          /* dim == 2; NULL for single Face */
  int        nVertexSet;        /* number of VertexSets */
  capsObject **vertexSet;       /* the VertexSets */
} capsBound;


/*
 * structure for CAPS object -- VETREXSET
 */
typedef struct {
  capsObject *analysis;         /* owning analysis object */
  capsDiscr  *discr;            /* the discretization information for the VS */
  int        nDataSets;         /* number of datasets */
  capsObject **dataSets;        /* the datasets */
} capsVertexSet;


/*
 * structure for CAPS object -- DATASET
 */
typedef struct {
  int     method;               /* the source method for the data in the set */
  int     nHist;                /* history length */
  capsOwn *history;             /* the history */
  int     npts;                 /* number of points */
  int     rank;                 /* rank */
  int     dflag;                /* dirty flag */
  double  *data;                /* data -- rank*npts in length */
  char    *units;               /* the units for the data */
  double  *startup;             /* the startup values for cyclic situations */
} capsDataSet;

#endif
