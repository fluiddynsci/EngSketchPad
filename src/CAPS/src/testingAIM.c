/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Testing AIM Example Code
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <string.h>
#include <math.h>
#include "aimUtil.h"

#define DEBUG
//#define SENSITIVITY

#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


  typedef struct {
    int       ngIn;
    capsValue *gIn;
  } aimStorage;

  static int        nInstance = 0;
  static aimStorage *instance = NULL;




int
aimInitialize(int ngIn, /*@null@*/ capsValue *gIn, int *qeFlag,
              /*@null@*/ const char *unitSys, int *nIn, int *nOut,
              int *nFields, char ***fnames, int **ranks)
{
  int        *ints, ret, flag;
  char       **strs;
  aimStorage *tmp;

#ifdef DEBUG
/*@-nullderef@*/
/*@-nullpass@*/
  printf("\n testingAIM/aimInitialize  ngIn = %d  qeFlag = %d  unitSys = %s!\n",
         ngIn, *qeFlag, unitSys);
/*@+nullpass@*/
/*@+nullderef@*/
#endif
  flag = *qeFlag;
  
  /* specify the number of analysis input and out "parameters" */
  *nIn    = 2;
  *nOut   = 1;
  *qeFlag = 1;
  if (flag == 1) return CAPS_SUCCESS;
  
  if (nInstance == 0) {
    instance = (aimStorage *) EG_alloc(sizeof(aimStorage));
    if (instance == NULL) return EGADS_MALLOC;
  } else {
    tmp = (aimStorage *) EG_reall(instance, (nInstance+1)*sizeof(aimStorage));
    if (tmp == NULL) return EGADS_MALLOC;
    instance = tmp;
  }

  /* specify the field variables this analysis can generate */
  *nFields = 2;
  ints     = (int *) EG_alloc(2*sizeof(int));
  if (ints == NULL) return EGADS_MALLOC;
  ints[0]  = 1;
  ints[1]  = 3;
  *ranks   = ints;
  strs     = (char **) EG_alloc(2*sizeof(char *));
  if (strs == NULL) {
    EG_free(*ranks);
    *ranks   = NULL;
    return EGADS_MALLOC;
  }
  strs[0]  = EG_strdup("scalar");
  strs[1]  = EG_strdup("vector");
  *fnames  = strs;
  
  /* return an index for the instance generated */
  ret = nInstance;
  instance[ret].ngIn = ngIn;
  instance[ret].gIn  = gIn;
  nInstance++;
  return ret;
}


void
aimCleanup()
{
#ifdef DEBUG
  printf(" testingAIM/aimCleanup!\n");
#endif
  
  if (nInstance != 0) {
    /* clean up any allocated data */
    EG_free(instance);
  }
  nInstance = 0;
  instance  = NULL;

}


int
aimFreeDiscr(capsDiscr *discr)
{
  int i;

#ifdef DEBUG
  printf(" testingAIM/aimFreeDiscr instance = %d!\n", discr->instance);
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


int
aimDiscr(char *bname, capsDiscr *discr)
{
  int          i, j, k, kk, n, ibod, stat, inst, nFace, atype, alen, tlen;
  int          npts, ntris, nBody, nGlobal, global, *storage, *pairs = NULL;
  int          *vid = NULL;
  double       size, box[6], params[3];
  const int    *ints, *ptype, *pindex, *tris, *nei;
  const double *reals, *xyz, *uv;
  const char   *string, *intents;
  ego          body, *bodies, *faces, *tess = NULL;

  inst = discr->instance;
#ifdef DEBUG
  printf(" testingAIM/aimDiscr: name = %s, instance = %d!\n", bname, inst);
#endif
  if ((inst < 0) || (inst >= nInstance)) return CAPS_BADINDEX;
  
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" testingAIM/aimDiscr: %d aim_getBodies = %d!\n", inst, stat);
    return stat;
  }
  
  if (nBody <= 0) return CAPS_SUCCESS;

  tess = (ego *) EG_alloc(nBody*sizeof(ego));
  if (tess == NULL) return EGADS_MALLOC;
  for (i = 0; i < nBody; i++) {
    tess[i] = NULL;
    /* look for existing tessellation */
    if (bodies[nBody+i] != NULL) tess[i] = bodies[nBody+i];
  }

  /* find any faces with our boundary marker */
  for (n = i = 0; i < nBody; i++) {
    stat = EG_getBodyTopos(bodies[i], NULL, FACE, &nFace, &faces);
    if (stat != EGADS_SUCCESS) {
      printf(" testingAIM: getBodyTopos (Face) = %d for Body %d!\n", stat, i+1);
      if (tess != NULL) {
        for (j = 0; j < i; j++)
          if (tess[j] != NULL) EG_deleteObject(tess[j]);
        EG_free(tess);
      }
      return stat;
    }
    for (j = 0; j < nFace; j++) {
      stat = EG_attributeRet(faces[j], "capsBound", &atype, &alen,
                             &ints, &reals, &string);
      if (stat  != EGADS_SUCCESS)     continue;
      if (atype != ATTRSTRING)        continue;
      if (strcmp(string, bname) != 0) continue;
#ifdef DEBUG
      printf(" testingAIM/aimDiscr: Body %d/Face %d matches %s!\n",
             i+1, j+1, bname);
#endif
      /* tessellate if necessary -- use EGADS tessellation directly */
      if (tess[i] == NULL) {
        stat = EG_getBoundingBox(bodies[i], box);
        if (stat != EGADS_SUCCESS) {
          printf(" testingAIM: getBoundingBox = %d for Body %d!\n", stat, i+1);
          for (j = 0; j < i; j++)
            if (tess[j] != NULL) EG_deleteObject(tess[j]);
          EG_free(tess);
          return stat;
        }
                                  size = box[3]-box[0];
        if (size < box[4]-box[1]) size = box[4]-box[1];
        if (size < box[5]-box[2]) size = box[5]-box[2];
        params[0] =  0.025*size;
        params[1] =  0.001*size;
        params[2] = 15.0;
        stat = EG_makeTessBody(bodies[i], params, &tess[i]);
        if (stat != EGADS_SUCCESS) {
          printf(" testingAIM: makeTessBody = %d for Body %d!\n", stat, i+1);
          for (j = 0; j < i; j++)
            if (tess[j] != NULL) EG_deleteObject(tess[j]);
          EG_free(tess);
          return stat;
        }
        /* store the tessellation back in CAPS */
        stat = aim_setTess(discr->aInfo, tess[i]);
        if (stat != EGADS_SUCCESS) {
          printf(" testingAIM: aim_setTess = %d for Body %d!\n", stat, i+1);
          EG_deleteObject(tess[i]);
          EG_free(tess);
          return stat;
        }
        printf(" testingAIM/aimDiscr: Tessellation set!\n");
      }
      n++;
    }
    EG_free(faces);
  }
  if (n == 0) {
    printf(" testingAIM/aimDiscr: No Faces match %s!\n", bname);
    if (tess != NULL) EG_free(tess);
    return CAPS_SUCCESS;
  }
  
  /* store away the faces */
  stat  = EGADS_MALLOC;
  pairs = (int *) EG_alloc(2*n*sizeof(int));
  if (pairs == NULL) {
    printf(" testingAIM: alloc on pairs = %d!\n", n);
    goto bail;
  }
  npts  = ntris = 0;
  for (n = i = 0; i < nBody; i++) {
    stat = EG_getBodyTopos(bodies[i], NULL, FACE, &nFace, &faces);
    if (stat != EGADS_SUCCESS) {
      printf(" testingAIM: getBodyTopos (Face) = %d for Body %d!\n", stat, i+1);
      goto bail;
    }
    for (j = 0; j < nFace; j++) {
      stat = EG_attributeRet(faces[j], "capsBound", &atype, &alen,
                             &ints, &reals, &string);
      if (stat  != EGADS_SUCCESS)     continue;
      if (atype != ATTRSTRING)        continue;
      if (strcmp(string, bname) != 0) continue;
      pairs[2*n  ] = i+1;
      pairs[2*n+1] = j+1;
      n++;
      stat = EG_getTessFace(tess[i], j+1, &alen, &xyz, &uv, &ptype, &pindex,
                                          &tlen, &tris, &nei);
      if (stat != EGADS_SUCCESS) {
        printf(" testingAIM: EG_getTessFace %d = %d for Body %d!\n",
               j+1, stat, i+1);
        continue;
      }
      npts  += alen;
      ntris += tlen;
    }
    EG_free(faces);
  }
  if ((npts == 0) || (ntris == 0)) {
#ifdef DEBUG
    printf(" testingAIM/aimDiscr: ntris = %d, npts = %d!\n", ntris, npts);
#endif
    stat = CAPS_SOURCEERR;
    goto bail;
  }
  discr->nElems = ntris;

  /* specify our single element type */
  stat = EGADS_MALLOC;
  discr->nTypes = 1;
  discr->types  = (capsEleType *) EG_alloc(sizeof(capsEleType));
  if (discr->types == NULL) goto bail;
  discr->types[0].nref  = 3;
  discr->types[0].ndata = 0;            /* data at geom reference positions */
  discr->types[0].ntri  = 1;
  discr->types[0].nmat  = 0;            /* match points at geom ref positions */
  discr->types[0].tris  = NULL;
  discr->types[0].gst   = NULL;
  discr->types[0].dst   = NULL;
  discr->types[0].matst = NULL;
  
  discr->types[0].tris   = (int *) EG_alloc(3*sizeof(int));
  if (discr->types[0].tris == NULL) goto bail;
  discr->types[0].tris[0] = 1;
  discr->types[0].tris[1] = 2;
  discr->types[0].tris[2] = 3;
  
  discr->types[0].gst   = (double *) EG_alloc(6*sizeof(double));
  if (discr->types[0].gst == NULL) goto bail;
  discr->types[0].gst[0] = 0.0;
  discr->types[0].gst[1] = 0.0;
  discr->types[0].gst[2] = 1.0;
  discr->types[0].gst[3] = 0.0;
  discr->types[0].gst[4] = 0.0;
  discr->types[0].gst[5] = 1.0;
  
  /* get the tessellation and
     make up a simple linear continuous triangle discretization */
#ifdef DEBUG
  printf(" testingAIM/aimDiscr: ntris = %d, npts = %d!\n", ntris, npts);
#endif
  discr->mapping = (int *) EG_alloc(2*npts*sizeof(int));
  if (discr->mapping == NULL) goto bail;
  discr->elems = (capsElement *) EG_alloc(ntris*sizeof(capsElement));
  if (discr->elems == NULL) goto bail;
  storage = (int *) EG_alloc(6*ntris*sizeof(int));
  if (storage == NULL) goto bail;
  discr->ptrm = storage;
  
  for (ibod = k = kk = i = 0; i < n; i++) {
    while (pairs[2*i] != ibod) {
      stat = EG_statusTessBody(tess[pairs[2*i]-1], &body, &j, &nGlobal);
      if ((stat < EGADS_SUCCESS) || (nGlobal == 0)) {
        printf(" testingAIM/aimDiscr: EG_statusTessBody = %d, nGlobal = %d!\n",
               stat, nGlobal);
        goto bail;
      }
      if (vid != NULL) EG_free(vid);
      stat = EGADS_MALLOC;
      vid  = (int *) EG_alloc(nGlobal*sizeof(int));
      if (vid == NULL) goto bail;
      for (j = 0; j < nGlobal; j++) vid[j] = 0;
      ibod++;
    }
    if (vid == NULL) continue;
    stat = EG_getTessFace(tess[pairs[2*i]-1], pairs[2*i+1], &alen, &xyz, &uv,
                          &ptype, &pindex, &tlen, &tris, &nei);
    if (stat != EGADS_SUCCESS) continue;
    for (j = 0; j < alen; j++) {
      stat = EG_localToGlobal(tess[pairs[2*i]-1], pairs[2*i+1], j+1, &global);
      if (stat != EGADS_SUCCESS) {
        printf(" testingAIM/aimDiscr: %d %d - %d %d EG_localToGlobal = %d!\n",
               pairs[2*i], j+1, ptype[j], pindex[j], stat);
        goto bail;
      }
      if (vid[global-1] != 0) continue;
      discr->mapping[2*k  ] = ibod;
      discr->mapping[2*k+1] = global;
      vid[global-1]         = k+1;
      k++;
    }
    for (j = 0; j < tlen; j++, kk++) {
      discr->elems[kk].bIndex      = ibod;
      discr->elems[kk].tIndex      = 1;
      discr->elems[kk].eIndex      = pairs[2*i+1];
/*@-immediatetrans@*/
      discr->elems[kk].gIndices    = &storage[6*kk];
/*@+immediatetrans@*/
      discr->elems[kk].dIndices    = NULL;
      discr->elems[kk].eTris.tq[0] = j+1;
      stat = EG_localToGlobal(tess[pairs[2*i]-1], pairs[2*i+1], tris[3*j  ],
                              &global);
      if (stat != EGADS_SUCCESS)
        printf(" testingAIM/aimDiscr: tri %d/0 EG_localToGlobal = %d\n",
               j+1, stat);
      storage[6*kk  ] = vid[global-1];
      storage[6*kk+1] = tris[3*j  ];
      stat = EG_localToGlobal(tess[pairs[2*i]-1], pairs[2*i+1], tris[3*j+1],
                              &global);
      if (stat != EGADS_SUCCESS)
        printf(" testingAIM/aimDiscr: tri %d/1 EG_localToGlobal = %d\n",
               j+1, stat);
      storage[6*kk+2] = vid[global-1];
      storage[6*kk+3] = tris[3*j+1];
      stat = EG_localToGlobal(tess[pairs[2*i]-1], pairs[2*i+1], tris[3*j+2],
                              &global);
      if (stat != EGADS_SUCCESS)
        printf(" testingAIM/aimDiscr: tri %d/2 EG_localToGlobal = %d\n",
               j+1, stat);
      storage[6*kk+4] = vid[global-1];
      storage[6*kk+5] = tris[3*j+2];
    }
  }
  discr->nPoints = k;

  /* free up our stuff */
  if (vid   != NULL) EG_free(vid);
  if (pairs != NULL) EG_free(pairs);
  if (tess  != NULL) EG_free(tess);

  return CAPS_SUCCESS;
  
bail:
  if (vid   != NULL) EG_free(vid);
  aimFreeDiscr(discr);
  if (pairs != NULL) EG_free(pairs);
  if (tess  != NULL) EG_free(tess);

  return stat;
}


int
aimLocateElement(capsDiscr *discr, double *params, double *param, int *eIndex,
                 double *bary)
{
  int    i, in[3], stat, ismall;
  double we[3], w, smallw = -1.e300;
  
  if (discr == NULL) return CAPS_NULLOBJ;
  
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
/*    {
        double uv[2];
        uv[0] = we[0]*params[2*in[0]  ] + we[1]*params[2*in[1]  ] +
                we[2]*params[2*in[2]  ];
        uv[1] = we[0]*params[2*in[0]+1] + we[1]*params[2*in[1]+1] +
                we[2]*params[2*in[2]+1];
        printf(" %d: %lf %lf  %lf %lf\n", i+1, param[0], param[1], uv[0], uv[1]);
        printf("        %lf %lf\n", bary[0], bary[1]);
      }  */
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
/*
  printf(" aimLocateElement: extropolate to %d (%lf %lf %lf)  %lf\n",
         ismall, we[0], we[1], we[2], smallw);
*/  
  return CAPS_SUCCESS;
}


int
aimInputs(int inst, void *aimStruc, int index, char **ainame, capsValue *defval)
{
  int       stat;
  capsTuple *tuple;
#ifdef DEBUG
  printf(" testingAIM/aimInputs instance = %d  index = %d!\n", inst, index);
#endif

  if (index == 1) {
    
    *ainame = EG_strdup("testingAIMin");
    if (*ainame == NULL) return EGADS_MALLOC;
    defval->type      = Double;
    defval->vals.real = 5.0 + inst;
    defval->units     = EG_strdup("cm");
    if (inst == 1) {
      stat = aim_link(aimStruc, *ainame, ANALYSISIN, defval);
      if (stat != CAPS_SUCCESS)
        printf(" aimInputs: aim_link = %d\n", stat);
    }
    
  } else {
    
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
    
  }

  return CAPS_SUCCESS;
}


int
aimUsesDataSet(int inst, /*@unused@*/ void *aimStruc, const char *bname,
               const char *dname, /*@unused@*/ enum capsdMethod method)
{
#ifdef DEBUG
  printf(" testingAIM/aimUsesDataSet inst = %d  Bound = %s  DataSet = %s!\n",
         inst, bname, dname);
#endif
  
  if (strcmp(bname, "Interface") != 0) return CAPS_NOTNEEDED;
  if (inst == 0) {
    if (strcmp(dname, "scalar")  != 0) return CAPS_NOTNEEDED;
  } else if (inst == 1) {
    if (strcmp(dname, "vector")  != 0) return CAPS_NOTNEEDED;
  } else {
    return CAPS_NOTNEEDED;
  }
  
  return CAPS_SUCCESS;
}


int
aimPreAnalysis(int inst, void *aimStruc, /*@unused@*/ const char *apath,
               /*@unused@*/ /*@null@*/ capsValue *inputs, capsErrs **errs)
{
  int              nBname, stat, i, npts, rank;
  char             **bNames;
  double           *data;
  capsDiscr        *discr;
  enum capsdMethod method;
#ifdef DEBUG
  printf(" testingAIM/aimPreAnalysis instance = %d!\n", inst);
#endif
  
  *errs = NULL;
  stat  = aim_getBounds(aimStruc, &nBname, &bNames);
  printf(" testingAIM/aimPreAnalysis aim_getBounds = %d\n", stat);
  for (i = 0; i < nBname; i++)
    printf("   Analysis in Bound = %s\n", bNames[i]);
  if (bNames != NULL) EG_free(bNames);
  
  stat = aim_newGeometry(aimStruc);
  printf("     aim_newGeometry = %d!\n", stat);
  
  if (inst == 0) {
    /* look for parent's dependency */
    stat = aim_getDiscr(aimStruc, "Interface", &discr);
    printf("   getDiscr = %d\n", stat);
    if (stat == CAPS_SUCCESS) {
      stat = aim_getDataSet(discr, "scalar", &method, &npts, &rank, &data);
      printf("   getDataSet = %d, rank = %d, method = %d\n", stat, rank, method);
      if (npts == 1) {
        printf("   scalar = %lf\n", data[0]);
      } else {
        printf("   %d scalars!\n", npts);
      }
    }
  } else if (inst == 1) {
    /* look for child's dependency */
    stat = aim_getDiscr(aimStruc, "Interface", &discr);
    printf("   getDiscr = %d\n", stat);
    if (stat == CAPS_SUCCESS) {
      stat = aim_getDataSet(discr, "vector", &method, &npts, &rank, &data);
      printf("   getDataSet = %d, rank = %d, method = %d\n", stat, rank, method);
      if (npts == 1) {
        printf("   vector = %lf %lf %lf\n", data[0], data[1], data[2]);
      } else if (stat != CAPS_SUCCESS) {
        printf("   vectors not setup yet!\n");
      } else {
        printf("   %d vectors!\n", npts);
      }
    }
  }
  
  return CAPS_SUCCESS;
}


int
aimOutputs(/*@unused@*/ int inst, /*@unused@*/ void *aimStruc,
           /*@unused@*/ int index, char **aoname, capsValue *form)
{
#ifdef DEBUG
  printf(" testingAIM/aimOutputs instance = %d  index = %d!\n", inst, index);
#endif
  *aoname = EG_strdup("testingAIMout");

  form->type = Double;
  return CAPS_SUCCESS;
}


int
aimPostAnalysis(int inst, void *aimStruc, /*@unused@*/ const char *apath,
                capsErrs **errs)
{
  int i, stat;
#ifdef DEBUG
  printf(" testingAIM/aimPostAnalysis instance = %d!\n", inst);
#endif
  
  if (inst == 0)
    for (i = 1; i <= instance[inst].ngIn; i++) {
      stat = aim_getGeomInType(aimStruc, i);
      if (stat < CAPS_SUCCESS) {
        printf(" testingAIM/aimPostAnalysis: %d aim_getGeomInType = %d\n",
               i, stat);
      } else if (stat == EGADS_OUTSIDE) {
        printf(" testingAIM/aimPostAnalysis: %d is Config Parameter\n", i);
      }
    }
  
  *errs = NULL;
  
  return CAPS_SUCCESS;
}


int
aimCalcOutput(int inst, void *aimStruc, /*@unused@*/ const char *apath,
              /*@unused@*/ int index, capsValue *val, capsErrs **errors)
{
  int              stat, rank, nrow, ncol, npts;
  double           *dval;
  char             *units;
  void             *data;
  enum capsvType   vtype;
  enum capsdMethod method;
  capsDiscr        *discr;
  const char       *name;
#ifdef SENSITIVITY
  int              nBody,   nFace, i, j, ig;
  double           *dxyz;
  ego              *bodies, *faces;
  const char       *intents;
#endif
#ifdef DEBUG
  
  stat = aim_getName(aimStruc, index, ANALYSISOUT, &name);
  printf(" testingAIM/aimCalcOutput instance = %d  index = %d %s %d!\n",
         inst, index, name, stat);
#endif

  *errors = NULL;

  val->vals.real = 5.5;
  if (inst == 1) {
    stat = aim_getData(aimStruc, "anything", &vtype, &rank, &nrow, &ncol, &data,
                       &units);
#ifdef DEBUG
    printf(" aim_getData = %d   %d %d %d\n", stat, rank, nrow, ncol);
#endif
    if (stat == CAPS_SUCCESS) {
      dval           = (double *) data;
      val->vals.real = dval[0];
    }
  }
  
  /* get a dataset */
  stat = aim_getDiscr(aimStruc, "Interface", &discr);
#ifdef DEBUG
  printf(" aim_getDiscr %d on Interface = %d\n", inst, stat);
#endif
  if (stat == CAPS_SUCCESS) {
    stat = aim_getDataSet(discr, "scalar", &method, &npts, &rank, &dval);
    if (stat == CAPS_SUCCESS) {
      printf(" aim_getDataSet %d for scalar = %d %d %d\n",
             inst, method, npts, rank);
    } else {
      printf(" aim_getDataSet %d on Interface = %d\n", inst, stat);
    }
  }

#ifdef SENSITIVITY
  /* get sensitivity on tessellation */
  ig   = 2;
  name = NULL;
  stat = aim_getName(aimStruc, ig, GEOMETRYIN, &name);
  if (stat == CAPS_SUCCESS) {
    printf(" instance = %d, %d geomIn = %s %d %d\n", inst, ig, name,
           instance[inst].gIn[ig-1].nrow, instance[inst].gIn[ig-1].ncol);
    if (inst == 0) {
      printf("\n");
      stat = aim_setSensitivity(aimStruc, name, 1, 1);
      if (stat != CAPS_SUCCESS) {
        printf(" testingAIM/aimCalcOutput: aim_setSensitivity = %d\n", stat);
      } else {
        stat = aim_getBodies(aimStruc, &intents, &nBody, &bodies);
        if (stat != CAPS_SUCCESS) {
          printf(" testingAIM/aimCalcOutput: aim_getBodies = %d!\n", stat);
        } else {
          for (i = 0; i < nBody; i++) {
            stat = EG_getBodyTopos(bodies[i], NULL, FACE, &nFace, &faces);
            if (stat != EGADS_SUCCESS) {
              printf(" testingAIM/aimCalcOutput: EG_getBodyTopos = %d!\n", stat);
              continue;
            }
            for (j = 0; j < nFace; j++) {
              stat = aim_getSensitivity(aimStruc, bodies[i+nBody], 2, j+1,
                                        &npts, &dxyz);
              if (stat != CAPS_SUCCESS) {
                printf(" testingAIM/aimCalcOutput: %d/%d aim_getSensitivity = %d!\n",
                       i+1, j+1, stat);
                continue;
              }
              printf(" sensitivity: Body %d/Face %d  nPts = %d\n",
                     i+1, j+1, npts);
              EG_free(dxyz);
            }
            EG_free(faces);
          }
        }
      }
      printf("\n");
    }
  }
#endif
  
  return CAPS_SUCCESS;
}


int
aimTransfer(capsDiscr *discr, /*@unused@*/ const char *name, int npts, int rank,
            double *data, /*@unused@*/ char **units)
{
  int        i, j, ptype, pindex, stat, nBody;
  double     xyz[3];
  const char *intents;
  ego        *bodies;

#ifdef DEBUG
  printf(" testingAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n",
         name, discr->instance, npts, rank);
#endif
  
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" testingAIM/aimTransfer: %d aim_getBodies = %d!\n",
           discr->instance, stat);
    return stat;
  }
  
  /* fill in with our coordinates -- for now */
  for (i = 0; i < npts; i++) {
    EG_getGlobal(bodies[discr->mapping[2*i]+nBody-1], discr->mapping[2*i+1],
                 &ptype, &pindex, xyz);
    for (j = 0; j < rank; j++) data[rank*i+j] = xyz[j];
  }

  return CAPS_SUCCESS;
}


int
aimData(/*@unused@*/ int inst, /*@unused@*/ const char *name,
        enum capsvType *vtype, int *rank, int *nrow, int *ncol, void **data,
        char **units)
{
  static double val = 6.5;
  
#ifdef DEBUG
  printf(" testingAIM/aimData instance = %d  name = %s!\n", inst, name);
#endif
  
  *vtype = Double;
  *rank  = *nrow = *ncol = 1;
  *data  = &val;
  *units = NULL;
  
  return CAPS_SUCCESS;
}


int
aimInterpolation(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                 double *bary, int rank, double *data, double *result)
{
  int    in[3], i;
  double we[3];
/*
#ifdef DEBUG
  printf(" testingAIM/aimInterpolation  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" testingAIM/Interpolation: eIndex = %d [1-%d]!\n",
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


int
aimInterpolateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                  double *bary, int rank, double *r_bar, double *d_bar)
{
  int    in[3], i;
  double we[3];
/*
#ifdef DEBUG
  printf(" testingAIM/aimInterpolateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" tetingAIM/InterpolateBar: eIndex = %d [1-%d]!\n",
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


int
aimIntegration(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
               /*@null@*/ double *data, double *result)
{
  int        i, in[3], stat, ptype, pindex, nBody;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;
/*
#ifdef DEBUG
  printf(" testingAIM/aimIntegration  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" testingAIM/aimIntegration: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" testingAIM/aimIntegration: %d aim_getBodies = %d!\n",
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
    printf(" testingAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[0], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                      discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (stat != CAPS_SUCCESS)
    printf(" testingAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[1], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                      discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (stat != CAPS_SUCCESS)
    printf(" testingAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
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


int
aimIntegrateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
                double *r_bar, double *d_bar)
{
  int        i, in[3], stat, ptype, pindex, nBody;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;
/*
#ifdef DEBUG
  printf(" testingAIM/aimIntegrateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" testingAIM/aimIntegrateBar: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" testingAIM/aimIntegrateBar: %d aim_getBodies = %d!\n",
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
    printf(" testingAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[0], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                      discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (stat != CAPS_SUCCESS)
    printf(" testingAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[1], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                      discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (stat != CAPS_SUCCESS)
    printf(" testingAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
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


int
aimBackdoor(int inst, /*@unused@*/ void *aimStruc, const char *JSONin,
            char **JSONout)
{
#ifdef DEBUG
  printf(" testingAIM/aimBackdoor instance = %d: %s!\n", inst, JSONin);
#endif
  
  *JSONout = EG_strdup("aimBackdoor Output");
  
  return CAPS_SUCCESS;
}
