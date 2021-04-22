/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Skeleton AIM Example Code
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <string.h>
#include <math.h>
#include "aimUtil.h"

#ifdef WIN32
#define strcasecmp stricmp
#endif

#define DEBUG

#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


  /* AIM "local" per instance storage
     needed data should be added here & cleaned up in aimCleanup */
  typedef struct {
    int       ngIn;
    capsValue *gIn;
  } aimStorage;


  /* AIM instance counter & storage */
  static int        nInstance = 0;
  static aimStorage *instance = NULL;



/****************** exposed AIM entry points -- Analysis **********************/

/* aimInitialize: Initialization Information for the AIM */
int
aimInitialize(int ngIn, /*@null@*/ capsValue *gIn, int *qeFlag,
              /*@unused@*/ const char *unitSys, int *nIn, int *nOut,
              int *nFields, char ***fnames, int **ranks)
{
  int        *ints, ret, flag, status, i;
  char       **strs = NULL;
  aimStorage *tmp;

#ifdef DEBUG
  printf("\n skeletonAIM/aimInitialize  ngIn = %d  qeFlag = %d!\n",
         ngIn, *qeFlag);
#endif
  
  /* on Input: 1 indicates a query and not an analysis instance */
  flag = *qeFlag;

  /* on Output: 1 specifies that the AIM executes the analysis
   *              (i.e. no external executable is called)
   *            0 relies on external execution */
  *qeFlag = 1;

  /* specify the number of analysis inputs  defined in aimInputs
   *     and the number of analysis outputs defined in aimOutputs */
  *nIn    = 5;
  *nOut   = 1;

  /* return if "query" only */
  if (flag == 1) return CAPS_SUCCESS;

  /* specify the field variables this analysis can generate */
  *nFields = 2;

  /* specify the dimension of each field variable */
  ints     = (int *) EG_alloc(*nFields*sizeof(int));
  if (ints == NULL) {
    status = EGADS_MALLOC;
    goto cleanup;
  }
  ints[0]  = 1;
  ints[1]  = 3;
  *ranks   = ints;

  /* specify the name of each field variable */
  strs     = (char **) EG_alloc(*nFields*sizeof(char *));
  if (strs == NULL) {
    status = EGADS_MALLOC;
    goto cleanup;
  }
  strs[0]  = EG_strdup("scalar");
  strs[1]  = EG_strdup("coordinates");
  for (i = 0; i < *nFields; i++)
    if (strs[i] == NULL) {
      status = EGADS_MALLOC;
      goto cleanup;
    }
  *fnames  = strs;

  /* create our "local" storage for anything that needs to be persistent
     remember there can be multiple instances of the AIM */
  if (nInstance == 0) {
    tmp = (aimStorage *) EG_alloc(sizeof(aimStorage));
  } else {
    tmp = (aimStorage *) EG_reall(instance, (nInstance+1)*sizeof(aimStorage));
  }
  if (tmp == NULL) {
    status = EGADS_MALLOC;
    goto cleanup;
  }
  instance = tmp;

  /* return the index for the instance generated & store our local data */
  ret = nInstance;
  instance[ret].ngIn = ngIn;
  instance[ret].gIn  = gIn;
  nInstance++; /* increment the instance count */
  return ret;

cleanup:
  /* release all possibly allocated memory on error */
  printf("skeletonAIM/aimInitialize: failed to allocate memory\n");
  *nFields = 0;

  EG_free(*ranks);
  *ranks = NULL;
  if (*fnames != NULL)
    for (i = 0; i < *nFields; i++) EG_free(strs[i]);
  EG_free(*fnames);
  *fnames = NULL;
  
  return status;
}


/* aimInputs: Input Information for the AIM */
int
aimInputs(int inst, /*@unused@*/ void *aimInfo, int index, char **ainame,
          capsValue *defval)
{
  capsTuple *tuple;
#ifdef DEBUG
  printf(" skeletonAIM/aimInputs  instance = %d  index  = %d!\n", inst, index);
#endif
  if ((inst < 0) || (inst >= nInstance)) return CAPS_BADINDEX;

  /* fill in the required members based on the index */
  if (index == 1) {
    *ainame               = EG_strdup("InputVariable");
    defval->type          = Boolean;
    defval->vals.integer  = false;

  } else if (index == 2) {
    *ainame = EG_strdup("skeletonAIMin");
    if (*ainame == NULL) return EGADS_MALLOC;
    defval->type      = Double;
    defval->vals.real = 5.0;
    defval->units     = EG_strdup("cm");
    if (defval->units == NULL) return EGADS_MALLOC;

  } else if (index == 3) {
    *ainame              = EG_strdup("Mach"); // Mach number
    defval->type         = Double;
    defval->nullVal      = IsNull;
    defval->units        = NULL;
    defval->lfixed       = Change;
    defval->dim          = Scalar;

  } else if (index == 4) {
    *ainame              = EG_strdup("Mesh_Format");
    defval->type         = String;
    defval->vals.string  = EG_strdup("AFLR3");
    defval->lfixed       = Change;

  } else if (index == 5) {

    /* an example of filling in a Tuple */
    *ainame = EG_strdup("table");
    if (*ainame == NULL) return EGADS_MALLOC;
    tuple = (capsTuple *) EG_alloc(3*sizeof(capsTuple));
    if (tuple == NULL) return EGADS_MALLOC;
    tuple[0].name  = EG_strdup("Entry1");
    tuple[1].name  = EG_strdup("Entry2");
    tuple[2].name  = EG_strdup("Entry3");
    tuple[0].value = EG_strdup("Value1");
    tuple[1].value = EG_strdup("Value2");
    tuple[2].value = EG_strdup("Value3");

    defval->type       = Tuple;
    defval->dim        = Vector;
    defval->nrow       = 1;
    defval->ncol       = 3;
    defval->vals.tuple = tuple;

  } else {
    printf(" skeletonAIM/aimInputs: unknown input index = %d for instance = %d!\n",
           index, inst);
    return CAPS_BADINDEX;
  }

  return CAPS_SUCCESS;
}

/* aimPreAnalysis: Parse Inputs, Generate Input File(s) & Possibly Tessellate */
int
aimPreAnalysis(int inst, void *aimInfo, /*@unused@*/ const char *apath,
               capsValue *inputs, capsErrs **errs)
{
  int        i, n, status, nBody, state, npts;
  const char *name, *intents;
  double     size, box[6], params[3];
  ego        *bodies, dum, tess;
  capsValue  *val;
#ifdef DEBUG
  printf("\n skeletonAIM/aimPreAnalysis instance = %d!\n", inst);
#endif

  *errs = NULL;
  if ((inst < 0) || (inst >= nInstance)) return CAPS_BADINDEX;

  /* look at the CSM design parameters */
  printf("   GeometryIn:\n");
  for (i = 0; i < instance[inst].ngIn; i++) {
      status = aim_getName(aimInfo, i+1, GEOMETRYIN, &name);
    if (status == CAPS_SUCCESS)
      printf("       %d: %s  %d  (%d,%d)\n", i+1, name,
             instance[inst].gIn[i].type, instance[inst].gIn[i].nrow,
             instance[inst].gIn[i].ncol);
  }
  printf("\n   GeometryOut:\n");
  /* look at the CSM output parameters */
  n = aim_getIndex(aimInfo, NULL, GEOMETRYOUT);
  for (i = 0; i < n; i++) {
    status = aim_getName(aimInfo, i+1, GEOMETRYOUT, &name);
    if (status != CAPS_SUCCESS) continue;
    status = aim_getValue(aimInfo, i+1, GEOMETRYOUT, &val);
    if (status == CAPS_SUCCESS)
      printf("       %d: %s  %d  (%d,%d)\n", i+1, name,
             val->type, val->nrow, val->ncol);
  }

  /* write out input list of values */
  if (inputs != NULL) {
    printf("\n   AnalysisIn:\n");
    for (i = 0; i < 5; i++) {
      status = aim_getName(aimInfo, i+1, ANALYSISIN, &name);
      if (status != CAPS_SUCCESS) break;
      printf("       %d: %s  %d  (%d,%d) %s\n", i+1, name,
             inputs[i].type, inputs[i].nrow, inputs[i].ncol, inputs[i].units);
    }
  }

  /* create the tessellation */
  
  status = aim_getBodies(aimInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) {
    printf(" skeletonAIM/aimPreAnalysis aim_getBodies = %d\n", status);
    return CAPS_SUCCESS;
  }
  if ((bodies == NULL) || (nBody == 0)) return CAPS_SUCCESS;
  
  for (i = 0; i < nBody; i++) {
    /* if we have a tessellation -- skip it */
    if (bodies[i+nBody] != NULL) continue;
    
    /* tessellate with EGADS tessellation directly in this example */
    status = EG_getBoundingBox(bodies[i], box);
    if (status != EGADS_SUCCESS) {
      printf(" skeletonAIM/aimPreAnalysis EG_getBoundingBox = %d\n", status);
      continue;
    }
    size = box[3]-box[0];
    if (size < box[4]-box[1]) size = box[4]-box[1];
    if (size < box[5]-box[2]) size = box[5]-box[2];
    params[0] =  0.025*size;
    params[1] =  0.001*size;
    params[2] = 15.0;
    status = EG_makeTessBody(bodies[i], params, &tess);
    if (status != EGADS_SUCCESS) {
      printf(" skeletonAIM/aimPreAnalysis EG_makeTessBody = %d\n", status);
      continue;
    }
    status = EG_statusTessBody(tess, &dum, &state, &npts);
    if (status != EGADS_SUCCESS) {
      printf(" skeletonAIM/aimPreAnalysis EG_statusTessBody = %d\n", status);
      continue;
    }
#ifdef DEBUG
    printf(" skeletonAIM/aimPreAnalysis tess %d has %d vertices!\n", i, npts);
#endif
    
    /* store the tessellation in CAPS */
    status = aim_setTess(aimInfo, tess);
    if (status != CAPS_SUCCESS)
      printf(" skeletonAIM/aimPreAnalysis aim_setTess = %d\n", status);
  }
  printf("\n");
  
  return CAPS_SUCCESS;
}

/* aimPostAnalysis: Perform any processing after the Analysis is run – Optional */
int
aimPostAnalysis(/*@unused@*/ int iIndex, /*@unused@*/ void *aimInfo,
                /*@unused@*/ const char *analysisPath,
                /*@unused@*/ capsErrs **errs)
{
  int status = CAPS_SUCCESS;

  return status;
}

/* aimOutputs: Output Information for the AIM */
int
aimOutputs(int inst, /*@unused@*/ void *aimInfo, /*@unused@*/ int index,
           char **aoname, capsValue *form)
{
#ifdef DEBUG
  printf(" skeletonAIM/aimOutputs instance = %d  index  = %d!\n", inst, index);
#endif
  if ((inst < 0) || (inst >= nInstance)) return CAPS_BADINDEX;
  
  *aoname = EG_strdup("skeletonAIMout");
  if (*aoname == NULL) return EGADS_MALLOC;
  form->type = Double;

  return CAPS_SUCCESS;
}

/* aimCalcOutput: Calculate/Retrieve Output Information */
int
aimCalcOutput(int inst, /*@unused@*/ void *aimInfo, /*@unused@*/ const char *ap,
              /*@unused@*/ int index, capsValue *val, capsErrs **errors)
{
#ifdef DEBUG
  int        status;
  const char *name;

  status = aim_getName(aimInfo, index, ANALYSISOUT, &name);
  printf(" skeletonAIM/aimCalcOutput instance = %d  index = %d %s %d!\n",
         inst, index, name, status);
#endif

  *errors = NULL;
  if ((inst < 0) || (inst >= nInstance)) return CAPS_BADINDEX;
  val->vals.real = 3.1415926;

  return CAPS_SUCCESS;
}


/* aimCleanup: Free up the AIMs storage */
void aimCleanup()
{
#ifdef DEBUG
  printf(" skeletonAIM/aimCleanup!\n");
#endif

  if (nInstance != 0) {
    /* clean up any allocated data
     :
     */

    EG_free(instance);
  }
  nInstance = 0;
  instance  = NULL;

}


/************ exposed AIM entry points -- Discretization Structure ************/

/* aimFreeDiscr: Frees up data in a Discrete Structure */
int
aimFreeDiscr(capsDiscr *discr)
{
  int i;

#ifdef DEBUG
  printf(" skeletonAIM/aimFreeDiscr instance = %d!\n", discr->instance);
#endif

  /* free up this capsDiscr */
  if (discr->mapping != NULL) EG_free(discr->mapping);
  if (discr->verts   != NULL) EG_free(discr->verts);
  if (discr->celem   != NULL) EG_free(discr->celem);
  if (discr->types   != NULL) {
    for (i = 0; i < discr->nTypes; i++) {
      if (discr->types[i].gst   != NULL) EG_free(discr->types[i].gst);
      if (discr->types[i].dst   != NULL) EG_free(discr->types[i].dst);
      if (discr->types[i].matst != NULL) EG_free(discr->types[i].matst);
      if (discr->types[i].tris  != NULL) EG_free(discr->types[i].tris);
    }
    EG_free(discr->types);
  }
  if (discr->elems   != NULL) EG_free(discr->elems);
  if (discr->dtris   != NULL) EG_free(discr->dtris);
  if (discr->ptrm    != NULL) EG_free(discr->ptrm);

  discr->nPoints  = 0;
  discr->mapping  = NULL;
  discr->nVerts   = 0;
  discr->verts    = NULL;
  discr->celem    = NULL;
  discr->nTypes   = 0;
  discr->types    = NULL;
  discr->nElems   = 0;
  discr->elems    = NULL;
  discr->nDtris   = 0;
  discr->dtris    = NULL;
  discr->ptrm     = NULL;

  return CAPS_SUCCESS;
}

/* aimDiscr: Fill-in the Discrete data for a Bound Object -- Optional */
int
aimDiscr(char *tname, capsDiscr *discr)
{
  int          ibody, iface, i, vID, itri, ielem, ibound, nbound, status;
  int          inst, nFace, atype, alen, tlen;
  int          npts, ntris, nBody, nGlobal, global, *storage, *pairs = NULL;
  int          *vid = NULL;
  const int    *ints, *ptype, *pindex, *tris, *nei;
  const double *reals, *xyz, *uv;
  const char   *string, *intents;
  ego          body, *bodies, *faces, *tess = NULL;

  inst = discr->instance;
#ifdef DEBUG
  printf(" skeletonAIM/aimDiscr: tname = %s, instance = %d!\n", tname, inst);
#endif
  if ((inst < 0) || (inst >= nInstance)) return CAPS_BADINDEX;

  /* create the discretization structure for this capsBound */

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) goto cleanup;
  status = aimFreeDiscr(discr);
  if (status != CAPS_SUCCESS) goto cleanup;

  /* bodies is 2*nBodies long where the last nBodies
     are the tess objects for the first nBodies */
  tess = bodies + nBody;

  /* find any faces with our boundary marker */
  for (nbound = ibody = 0; ibody < nBody; ibody++) {
    status = EG_getBodyTopos(bodies[ibody], NULL, FACE, &nFace, &faces);
    if (status != EGADS_SUCCESS) goto cleanup;
    if (tess[ibody] == NULL) continue;
    for (iface = 0; iface < nFace; iface++) {
      status = EG_attributeRet(faces[iface], "capsBound", &atype, &alen,
                               &ints, &reals, &string);
      if (status  != EGADS_SUCCESS)   continue;
      if (atype != ATTRSTRING)        continue;
      if (strcmp(string, tname) != 0) continue;
#ifdef DEBUG
      printf(" skeletonAIM/aimDiscr: Body %d/Face %d matches %s!\n",
             ibody+1, iface+1, tname);
#endif
      /* count the number of face with capsBound */
      nbound++;
    }
    EG_free(faces);
  }
  if (nbound == 0) {
    printf(" skeletonAIM/aimDiscr: No Faces match %s!\n", tname);
    return CAPS_SUCCESS;
  }

  /* store away pairs of body/face to identify the bounds */
  pairs = (int *) EG_alloc(2*nbound*sizeof(int));
  if (pairs == NULL) {
    printf(" skeletonAIM: alloc on pairs = %d!\n", nbound);
    status  = EGADS_MALLOC;
    goto cleanup;
  }
  npts  = ntris = 0;
  for (nbound = ibody = 0; ibody < nBody; ibody++) {
    status = EG_getBodyTopos(bodies[ibody], NULL, FACE, &nFace, &faces);
    if (status != EGADS_SUCCESS) {
      printf(" skeletonAIM: getBodyTopos (Face) = %d for Body %d!\n",
             status, ibody+1);
      goto cleanup;
    }
    if (tess == NULL) continue;
    for (iface = 0; iface < nFace; iface++) {
      status = EG_attributeRet(faces[iface], "capsBound", &atype, &alen,
                               &ints, &reals, &string);
      if (status  != EGADS_SUCCESS)     continue;
      if (atype != ATTRSTRING)        continue;
      if (strcmp(string, tname) != 0) continue;
      pairs[2*nbound  ] = ibody+1;
      pairs[2*nbound+1] = iface+1;
      nbound++;
      status = EG_getTessFace(tess[ibody], iface+1, &alen, &xyz, &uv,
                              &ptype, &pindex, &tlen, &tris, &nei);
      if (status != EGADS_SUCCESS) {
        printf(" skeletonAIM: EG_getTessFace %d = %d for Body %d!\n",
               iface+1, status, ibody+1);
        continue;
      }
      npts  += alen;
      ntris += tlen;
    }
    EG_free(faces);
  }
  if ((npts == 0) || (ntris == 0)) {
#ifdef DEBUG
    printf(" skeletonAIM/aimDiscr: ntris = %d, npts = %d!\n", ntris, npts);
#endif
    status = CAPS_SOURCEERR;
    goto cleanup;
  }
  discr->nElems = ntris;

  /* specify our single triangle element type */
  status = EGADS_MALLOC;
  discr->nTypes = 1;
  discr->types  = (capsEleType *) EG_alloc(discr->nTypes*sizeof(capsEleType));
  if (discr->types == NULL) goto cleanup;
  discr->types[0].nref  = 3;
  discr->types[0].ndata = 0;         /* data at geom reference positions
                                        (i.e. vertex centered/iso-parametric) */
  discr->types[0].ntri  = 1;
  discr->types[0].nmat  = 0;         /* match points at geom ref positions */
  discr->types[0].tris  = NULL;
  discr->types[0].gst   = NULL;
  discr->types[0].dst   = NULL;
  discr->types[0].matst = NULL;

  /* specify the numbering for the points on the triangle */
  discr->types[0].tris  = (int *) EG_alloc(discr->types[0].nref*sizeof(int));
  if (discr->types[0].tris == NULL) goto cleanup;
  discr->types[0].tris[0] = 1;
  discr->types[0].tris[1] = 2;
  discr->types[0].tris[2] = 3;

  /* specify the reference coordinates for each point on the triangle */
  discr->types[0].gst = (double *) EG_alloc(2*discr->types[0].nref*sizeof(double));
  if (discr->types[0].gst == NULL) goto cleanup;
  discr->types[0].gst[0] = 0.0;   /* s = 0, t = 0 */
  discr->types[0].gst[1] = 0.0;
  discr->types[0].gst[2] = 1.0;   /* s = 1, t = 0 */
  discr->types[0].gst[3] = 0.0;
  discr->types[0].gst[4] = 0.0;   /* s = 0, t = 1 */
  discr->types[0].gst[5] = 1.0;

  /* get the tessellation and
   make up a simple linear continuous triangle discretization */
#ifdef DEBUG
  printf(" skeletonAIM/aimDiscr: ntris = %d, npts = %d!\n", ntris, npts);
#endif
  discr->mapping = (int *) EG_alloc(2*npts*sizeof(int));
  if (discr->mapping == NULL) goto cleanup;

  discr->elems = (capsElement *) EG_alloc(ntris*sizeof(capsElement));
  if (discr->elems == NULL) goto cleanup;

  storage = (int *) EG_alloc(6*ntris*sizeof(int));
  if (storage == NULL) goto cleanup;
  discr->ptrm = storage;

  for (vID = ielem = ibound = 0; ibound < nbound; ibound++) {
    ibody = pairs[2*ibound+0]; /* 1-based indexed */
    iface = pairs[2*ibound+1]; /* 1-based indexed */

    if (tess[ibody] == NULL) {
      printf(" skeletonAIM/aimDiscr: tessellation missing on body = %d!\n",
             ibody);
      status = CAPS_SOURCEERR;
      goto cleanup;
    }

    status = EG_statusTessBody(tess[ibody-1], &body, &iface, &nGlobal);
    if ((status < EGADS_SUCCESS) || (nGlobal == 0)) {
      printf(" skeletonAIM/aimDiscr: EG_statusTessBody = %d, nGlobal = %d!\n",
             status, nGlobal);
      goto cleanup;
    }
    if (vid != NULL) EG_free(vid);
    status = EGADS_MALLOC;
    vid  = (int *) EG_alloc(nGlobal*sizeof(int));
    if (vid == NULL) goto cleanup;
    for (i = 0; i < nGlobal; i++) vid[i] = 0;

    status = EG_getTessFace(tess[ibody-1], iface, &alen, &xyz, &uv,
                          &ptype, &pindex, &tlen, &tris, &nei);
    if (status != EGADS_SUCCESS) continue;
    for (i = 0; i < alen; i++) {
      status = EG_localToGlobal(tess[ibody-1], iface, i+1, &global);
      if (status != EGADS_SUCCESS) {
        printf(" skeletonAIM/aimDiscr: %d %d - %d %d EG_localToGlobal = %d!\n",
                ibody, i+1, ptype[i], pindex[i], status);
        goto cleanup;
      }
      if (vid[global-1] != 0) continue;
      discr->mapping[2*vID  ] = ibody;
      discr->mapping[2*vID+1] = global;
      vid[global-1]           = vID+1;
      vID++;
    }
    for (itri = 0; itri < tlen; itri++, ielem++) {
      discr->elems[ielem].bIndex      = ibody;
      discr->elems[ielem].tIndex      = 1;
      discr->elems[ielem].eIndex      = pairs[2*ibound+1];
/*@-immediatetrans@*/
      discr->elems[ielem].gIndices    = &storage[6*ielem];
/*@+immediatetrans@*/
      discr->elems[ielem].dIndices    = NULL;
      discr->elems[ielem].eTris.tq[0] = itri+1;
      status = EG_localToGlobal(tess[ibody-1], iface, tris[3*itri  ],
                              &global);
      if (status != EGADS_SUCCESS)
        printf(" skeletonAIM/aimDiscr: tri %d/0 EG_localToGlobal = %d\n",
               itri+1, status);
      discr->elems[ielem].gIndices[0] = vid[global-1];
      discr->elems[ielem].gIndices[1] = tris[3*itri  ];
      status = EG_localToGlobal(tess[ibody-1], iface, tris[3*itri+1],
                              &global);
      if (status != EGADS_SUCCESS)
        printf(" skeletonAIM/aimDiscr: tri %d/1 EG_localToGlobal = %d\n",
               itri+1, status);
      discr->elems[ielem].gIndices[2] = vid[global-1];
      discr->elems[ielem].gIndices[3] = tris[3*itri+1];
      status = EG_localToGlobal(tess[ibody-1], iface, tris[3*itri+2],
                              &global);
      if (status != EGADS_SUCCESS)
        printf(" skeletonAIM/aimDiscr: tri %d/2 EG_localToGlobal = %d\n",
               itri+1, status);
      discr->elems[ielem].gIndices[4] = vid[global-1];
      discr->elems[ielem].gIndices[5] = tris[3*itri+2];
    }
  }
  discr->nPoints = vID;

  status = CAPS_SUCCESS;
cleanup:

  /* free up our stuff */
  if (vid   != NULL) EG_free(vid);
  if (pairs != NULL) EG_free(pairs);

  if (status != CAPS_SUCCESS) {
    printf(" skeletonAIM/aimDiscr: failed with status = %d\n", status);
    aimFreeDiscr(discr);
  }

  return status;
}

/* aimUsesDataSet: Is the DataSet required by aimPreAnalysis -- Optional */
int aimUsesDataSet(/*@unused@*/ int inst, /*@unused@*/ void *aimInfo,
                   /*@unused@*/ const char *bname,
                   const char *dname, /*@unused@*/ enum capsdMethod dMethod)
{
  /* This function checks if a data set name can be consumed by this aim.
   *
   * Return CAPS_SUCCESS if the aim can consume the data set. Otherwise
   * return CAPS_NOTNEEDED
   */

/*@-unrecog@*/
  if (strcasecmp(dname, "scalar") == 0) {
      return CAPS_SUCCESS;
  }
/*@+unrecog@*/

  return CAPS_NOTNEEDED;
}

/* aimTransfer: Data Transfer using the Discrete Structure -- Optional */
int
aimTransfer(capsDiscr *discr, const char *fname, int npts, int rank,
            double *data, /*@unused@*/ char **units)
{
  int        i, j, ptype, pindex, status, ibody, nBody, global;
  double     xyz[3];
  const char *intents;
  ego        *bodies, *tess;

#ifdef DEBUG
  printf(" skeletonAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n",
         fname, discr->instance, npts, rank);
#endif

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) {
    printf(" skeletonAIM/aimTransfer: %d aim_getBodies = %d!\n",
           discr->instance, status);
    return status;
  }

  if (strcmp(fname, "scalar") == 0 ) {

    /* fill in a simple scalar field */
    for (i = 0; i < npts; i++) {
      data[i] = i;
    }

  } else if (strcmp(fname, "coordinates") == 0 ) {

    /* bodies is 2*nBodies long where the last nBodies are the tess objects
              for the first bodies */
    tess = bodies + nBody;

    /* fill in with our coordinates -- as an example */
    for (i = 0; i < npts; i++) {
      ibody  = discr->mapping[2*i+0];
      global = discr->mapping[2*i+1];
      EG_getGlobal(tess[ibody-1], global, &ptype, &pindex, xyz);
      for (j = 0; j < rank; j++) data[rank*i+j] = xyz[j];
    }
  }

  return CAPS_SUCCESS;
}


/********* exposed AIM entry points -- Interpolation Functions ***************/

/* aimLocateElement: Element in the Mesh -- Optional */
int
aimLocateElement(capsDiscr *discr, double *params, double *param, int *eIndex,
                 double *bary)
{
  int    i, in[3], stat, ismall;
  double we[3], w, smallw = -1.e300;

  if (discr == NULL) return CAPS_NULLOBJ;

  /* locate the element that contains param and return the bary centric
     coordinates of that element */

  for (ismall = i = 0; i < discr->nElems; i++) {
    in[0] = discr->elems[i].gIndices[0] - 1;
    in[1] = discr->elems[i].gIndices[2] - 1;
    in[2] = discr->elems[i].gIndices[4] - 1;
    stat  = EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]],
                          param, we);
    if (stat == EGADS_SUCCESS) {
      *eIndex = i+1;
      bary[0] = we[1];
      bary[1] = we[2];
      return CAPS_SUCCESS;
    }
    w = we[0];
    if (we[1] < w) w = we[1];
    if (we[2] < w) w = we[2];
    if (w > smallw) {
      ismall = i+1;
      smallw = w;
    }
  }

  /* must extrapolate! */
  if (ismall == 0) return CAPS_NOTFOUND;
  in[0] = discr->elems[ismall-1].gIndices[0] - 1;
  in[1] = discr->elems[ismall-1].gIndices[2] - 1;
  in[2] = discr->elems[ismall-1].gIndices[4] - 1;
  EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]],
                param, we);
  *eIndex = ismall;
  bary[0] = we[1];
  bary[1] = we[2];

  return CAPS_SUCCESS;
}

/* aimInterpolation: Interpolation on the Bound -- Optional */
int
aimInterpolation(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                 double *bary, int rank, double *data, double *result)
{
  int    in[3], i;
  double we[3];

  /* interpolate data to barycentric coordinates of the element */

  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" skeletonAIM/Interpolation: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);

  we[0] = 1.0 - bary[0] - bary[1];
  we[1] = bary[0];
  we[2] = bary[1];
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  for (i = 0; i < rank; i++)
    result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2];

  return CAPS_SUCCESS;
}

/* aimInterpolateBar: Interpolation on the Bound -- Optional */
int
aimInterpolateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                  double *bary, int rank, double *r_bar, double *d_bar)
{
  int    in[3], i;
  double we[3];

  /* reverse differentiation of aimInterpolation */

  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" skeletonAIM/InterpolateBar: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);

  we[0] = 1.0 - bary[0] - bary[1];
  we[1] = bary[0];
  we[2] = bary[1];
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  for (i = 0; i < rank; i++) {
/*  result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2];  */
    d_bar[rank*in[0]+i] += we[0]*r_bar[i];
    d_bar[rank*in[1]+i] += we[1]*r_bar[i];
    d_bar[rank*in[2]+i] += we[2]*r_bar[i];
  }

  return CAPS_SUCCESS;
}

/* aimIntegration: Element Integration on the Bound -- Optional */
int
aimIntegration(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
               int rank, /*@null@*/ double *data, double *result)
{
  int        i, in[3], stat, ptype, pindex, nBody;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;

  /* computes the integral of the data on a given element */

  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" skeletonAIM/aimIntegration: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" skeletonAIM/aimIntegration: %d aim_getBodies = %d!\n",
           discr->instance, stat);
    return stat;
  }

  /* element indices */

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                      discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (stat != CAPS_SUCCESS)
    printf(" skeletonAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[0], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                      discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (stat != CAPS_SUCCESS)
    printf(" skeletonAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[1], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                      discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (stat != CAPS_SUCCESS)
    printf(" skeletonAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[2], stat);

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];
  CROSS(x3, x1, x2);
  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */
  if (data == NULL) {
    *result = 3.0*area;
    return CAPS_SUCCESS;
  }

  for (i = 0; i < rank; i++)
    result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area;

  return CAPS_SUCCESS;
}

/* aimIntegration: Element Integration on the Bound -- Optional */
int
aimIntegrateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                int rank, double *r_bar, double *d_bar)
{
  int        i, in[3], stat, ptype, pindex, nBody;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;

  /* reverse differentiation of aimIntegration */

  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" skeletonAIM/aimIntegrateBar: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" skeletonAIM/aimIntegrateBar: %d aim_getBodies = %d!\n",
           discr->instance, stat);
    return stat;
  }

  /* element indices */

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                      discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (stat != CAPS_SUCCESS)
    printf(" skeletonAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[0], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                      discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (stat != CAPS_SUCCESS)
    printf(" skeletonAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[1], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                      discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (stat != CAPS_SUCCESS)
    printf(" skeletonAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[2], stat);

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];
  CROSS(x3, x1, x2);
  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  for (i = 0; i < rank; i++) {
/*  result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area;  */
    d_bar[rank*in[0]+i] += area*r_bar[i];
    d_bar[rank*in[1]+i] += area*r_bar[i];
    d_bar[rank*in[2]+i] += area*r_bar[i];
  }

  return CAPS_SUCCESS;
}
