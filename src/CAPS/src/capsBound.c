/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Bound, VertexSet & DataSet Object Functions
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "capsBase.h"
#include "capsAIM.h"

/* OpenCSM Defines & Includes */
#include "common.h"
#include "OpenCSM.h"


//#define VSOUTPUT


typedef struct {
  int    eIndex;              /* the element index in source quilt */
  double st[3];               /* position in element ref coords -- source */
} capsTarget;

typedef struct {
  capsTarget source;          /* the positions in the source */
  capsTarget target;          /* the positions in the target */
} capsMatch;

typedef struct {
  char       *name;
  aimContext *aimFPTR;
  int        npts;
  int        irank;
  int        nrank;
  int        sindx;           /* source DLL index */
  int        tindx;           /* target DLL index */
  double     afact;           /* area penalty function weight */
  double     area_src;        /* area associated with source (output) */
  double     area_tgt;        /* area associated with target (output) */
  capsDiscr  *src;
  double     *prms_src;
  double     *data_src;
  capsDiscr  *tgt;
  double     *prms_tgt;
  /*@exposed@*/
  double     *data_tgt;
  int        nmat;            /* number of MatchPoints */
  capsMatch  *mat;            /* array  of MatchPoints */
} capsConFit;


extern /*@null@*/ /*@only@*/ char *EG_strdup(/*@null@*/ const char *str);

extern int
  caps_conjGrad(int (*func)(int, double[], void *, double *,
                            /*@null@*/ double[]), /* (in) objective function */
                void   *blind,                    /* (in)  blind pointer to structure */
                int    n,                         /* (in)  number of variables */
                double x[],                       /* (in)  initial set of variables */
                                                  /* (out) optimized variables */
                double ftol,                      /* (in)  convergence tolerance on 
                                                           objective function */
                /*@null@*/
                FILE   *fp,                       /* (in)  FILE to write history */
                double *fopt);                    /* (out) objective function */


int
caps_integrateData(const capsObject *object, enum capstMethod method,
                   int *rank, double **data, char **units)
{
  int           i, j, status;
  double        *result;
  capsProblem   *problem;
  capsDataSet   *dataset;
  capsObject    *vso;
  capsVertexSet *vertexset;
  capsAnalysis  *analysis;
  capsDiscr     *discr;
  
  *rank  = 0;
  *data  = NULL;
  *units = NULL;          /* what do I fill this in with? */

  if (object              == NULL)      return CAPS_NULLOBJ;
  if (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (object->type        != DATASET)   return CAPS_BADTYPE;
  if (object->blind       == NULL)      return CAPS_NULLBLIND;
  dataset = (capsDataSet *) object->blind;
  vso     = object->parent;
  if (vso                 == NULL)      return CAPS_NULLOBJ;
  if (vso->magicnumber    != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vso->type           != VERTEXSET) return CAPS_BADTYPE;
  if (vso->blind          == NULL)      return CAPS_NULLBLIND;
  vertexset = (capsVertexSet *) vso->blind;
  if (vertexset->analysis == NULL)      return CAPS_NULLOBJ;
  analysis  = (capsAnalysis *) vertexset->analysis;
  discr     =                  vertexset->discr;
  if (discr               == NULL)      return CAPS_NULLVALUE;
  problem   = analysis->info.problem;
  if (problem             == NULL)      return CAPS_NULLOBJ;
  
  
  *rank = dataset->rank;
  i     = *rank*2;
  if (method == Average) i = *rank*3;
  result = (double *) EG_alloc(i*sizeof(double));
  if (result == NULL) return EGADS_MALLOC;
  
  for (i = 0; i < *rank; i++) result[i] = 0.0;
  if (method == Average) for (i = 0; i < *rank; i++) result[*rank*2+i] = 0.0;
  
  /* loop over all of the elements */
  for (i = 0; i < discr->nElems; i++) {
    status = aim_Integration(problem->aimFPTR, analysis->loadName, discr,
                             object->name, i+1, *rank, dataset->data,
                             &result[*rank]);
    if (status != CAPS_SUCCESS) {
      printf(" caps_integrateData Warning: status = %d for %s/%s!\n",
             status, analysis->loadName, object->name);
      continue;
    }
    for (j = 0; j < *rank; j++) result[j] += result[*rank+j];
    if (method != Average) continue;
    status = aim_Integration(problem->aimFPTR, analysis->loadName, discr,
                             object->name, i+1, *rank, NULL, &result[*rank]);
    if (status != CAPS_SUCCESS) {
      printf(" caps_integrateData Warning: Status = %d for %s/%s!\n",
             status, analysis->loadName, object->name);
      continue;
    }
    for (j = 0; j < *rank; j++) result[*rank*2+j] += result[*rank+j];
  }
  
  /* make weigted average, if called for */
  if (method == Average) {
    for (j = 0; j < *rank; j++) result[j] /= result[*rank*2+j];
    *units = EG_strdup(dataset->units);
  }
  
  *data = result;
  return CAPS_SUCCESS;
}


int
caps_boundInfo(const capsObject *object, enum capsState *state, int *dim,
               double *plims)
{
  capsBound *bound;

  *state = Empty;
  *dim   = 0;

  if (object              == NULL)      return CAPS_NULLOBJ;
  if (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (object->type        != BOUND)     return CAPS_BADTYPE;
  if (object->blind       == NULL)      return CAPS_NULLBLIND;
  bound = (capsBound *) object->blind;
  
  *dim   = bound->dim;
  *state = bound->state;
  if (*dim >= 1) {
    plims[0] = bound->plimits[0];
    plims[1] = bound->plimits[1];
  }
  if (*dim == 2) {
    plims[2] = bound->plimits[2];
    plims[3] = bound->plimits[3];
  }
  
  return CAPS_SUCCESS;
}


int
caps_makeBound(capsObject *pobject, int dim, const char *bname,
               capsObject **bobj)
{
  int         i, status;
  capsProblem *problem;
  capsBound   *bound;
  capsObject  *object, **tmp;

  *bobj = NULL;
  if (pobject == NULL)                   return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type != PROBLEM)          return CAPS_BADTYPE;
  if (pobject->blind == NULL)            return CAPS_NULLBLIND;
  if (bname == NULL)                     return CAPS_NULLNAME;
  if ((dim < 1) || (dim > 3))            return CAPS_RANGEERR;
  problem = (capsProblem *) pobject->blind;
  
  /* same name? */
  for (i = 0; i < problem->nBound; i++) {
    if (problem->bounds[i]       == NULL) continue;
    if (problem->bounds[i]->name == NULL) continue;
    if (strcmp(bname, problem->bounds[i]->name) == 0) return CAPS_BADNAME;
  }
  
  bound = (capsBound *) EG_alloc(sizeof(capsBound));
  if (bound == NULL) return EGADS_MALLOC;
  bound->dim        = dim;
  bound->state      = Open;
  bound->lunits     = NULL;
  bound->plimits[0] = bound->plimits[1] = bound->plimits[2] =
                      bound->plimits[3] = 0.0;
  bound->geom       = NULL;
  bound->iBody      = 0;
  bound->iEnt       = 0;
  bound->curve      = NULL;
  bound->surface    = NULL;
  bound->nVertexSet = 0;
  bound->vertexSet  = NULL;
  
  /* make the object */
  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    EG_free(bound);
    return status;
  }

  if (problem->bounds == NULL) {
    problem->bounds = (capsObject  **) EG_alloc(sizeof(capsObject *));
    if (problem->bounds == NULL) {
      EG_free(object);
      EG_free(bound);
      return EGADS_MALLOC;
    }
  } else {
    tmp = (capsObject **) EG_reall( problem->bounds,
                                   (problem->nBound+1)*sizeof(capsObject *));
    if (tmp == NULL) {
      EG_free(object);
      EG_free(bound);
      return EGADS_MALLOC;
    }
    problem->bounds = tmp;
  }
  
  object->parent = pobject;
  object->name   = EG_strdup(bname);
  object->type   = BOUND;
  object->blind  = bound;

  problem->bounds[problem->nBound] = object;
  problem->nBound  += 1;
  problem->sNum    += 1;
  object->last.sNum = problem->sNum;
  caps_fillDateTime(object->last.datetime);

  *bobj = object;
  return CAPS_SUCCESS;
}


int
caps_completeBound(capsObject *bobject)
{
  int           i, j, k, l, n;
  char          *name;
  capsBound     *bound;
  capsVertexSet *vertexset, *othervs;
  capsDataSet   *dataset,   *otherds;
  
  if (bobject              == NULL)      return CAPS_NULLOBJ;
  if (bobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (bobject->type        != BOUND)     return CAPS_BADTYPE;
  if (bobject->blind       == NULL)      return CAPS_NULLBLIND;
  bound = (capsBound *) bobject->blind;
  if (bound->state         != Open)      return CAPS_STATEERR;
  
  /* do we have any entries? */
  if (bound->nVertexSet    == 0)         return CAPS_NOTFOUND;
  
  /* are the VertexSets OK? */
  for (i = 0; i < bound->nVertexSet; i++) {
    if (bound->vertexSet[i]              == NULL)      return CAPS_NULLOBJ;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
    if (bound->vertexSet[i]->type        != VERTEXSET) return CAPS_BADTYPE;
    if (bound->vertexSet[i]->blind       == NULL)      return CAPS_NULLBLIND;
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    for (j = 0; j < vertexset->nDataSets; j++) {
      if (vertexset->dataSets[j]              == NULL)      return CAPS_NULLOBJ;
      if (vertexset->dataSets[j]->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
      if (vertexset->dataSets[j]->type        != DATASET)   return CAPS_BADTYPE;
      if (vertexset->dataSets[j]->blind       == NULL)      return CAPS_NULLBLIND;
    }
  }

  /* do all dependent DataSets have a source? */
  for (i = 0; i < bound->nVertexSet; i++) {
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    for (j = 0; j < vertexset->nDataSets; j++) {
      dataset = (capsDataSet *) vertexset->dataSets[j]->blind;
      if ((dataset->method != Interpolate) &&
          (dataset->method != Conserve)) continue;
      name = vertexset->dataSets[j]->name;
      for (n = k = 0; k < bound->nVertexSet; k++) {
        if (k == i) continue;
        othervs = (capsVertexSet *) bound->vertexSet[k]->blind;
        for (l = 0; i < othervs->nDataSets; l++) {
          if (strcmp(name, othervs->dataSets[l]->name) == 0) {
            otherds = (capsDataSet *) othervs->dataSets[l]->blind;
            if ((otherds->method != Interpolate) &&
                (otherds->method != Conserve)) {
              n++;
              break;
            }
          }
        }
      }
      if (n == 0) {
        printf(" caps_completeBound: No source for VertexSet %s, DataSet %s!\n",
               bound->vertexSet[i]->name, vertexset->dataSets[j]->name);
        return CAPS_SOURCEERR;
      }
    }
  }

  bound->state = Empty;

  return CAPS_SUCCESS;
}


int
caps_makeDataSet(capsObject *vobject, const char *dname, enum capsdMethod meth,
                 int rank, capsObject **dobj)
{
  int           i, j, status, len, type, nrow, ncol;
  int           open = 0, irow = 1, icol = 1;
  char          *pname = NULL, name[MAX_NAME_LEN];
  capsObject    *bobject, *pobject, *aobject, *object, **tmp;
  capsVertexSet *vertexset, *otherset;
  capsAnalysis  *analysis;
  capsValue     *value;
  capsBound     *bound;
  capsProblem   *problem;
  capsDataSet   *dataset;
  
  *dobj = NULL;
  if (dname                == NULL)      return CAPS_NULLNAME;
  if (vobject              == NULL)      return CAPS_NULLOBJ;
  if (vobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vobject->type        != VERTEXSET) return CAPS_BADTYPE;
  if (vobject->blind       == NULL)      return CAPS_NULLBLIND;
  vertexset = (capsVertexSet *) vobject->blind;
  aobject   = vertexset->analysis;
  if (aobject              == NULL)      return CAPS_NULLOBJ;
  if (aobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aobject->type        != ANALYSIS)  return CAPS_BADTYPE;
  if (aobject->blind       == NULL)      return CAPS_NULLBLIND;
  analysis  = (capsAnalysis *) aobject->blind;
  bobject   = vobject->parent;
  if (bobject              == NULL)      return CAPS_NULLOBJ;
  if (bobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (bobject->type        != BOUND)     return CAPS_BADTYPE;
  if (bobject->blind       == NULL)      return CAPS_NULLBLIND;
  bound     = (capsBound *) bobject->blind;
  if (bound->state         != Open)      return CAPS_STATEERR;
  pobject   = bobject->parent;
  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;
  
  /* is that a legal name? */

  if (meth == BuiltIn)
    if ((strcmp(dname, "xyz")  != 0) && (strcmp(dname, "param")  != 0) &&
        (strcmp(dname, "xyzd") != 0) && (strcmp(dname, "paramd") != 0))
      return CAPS_BADDSETNAME;

  if (meth == Sensitivity) {
    if (rank != 3) {
      printf("caps_makeDataSet: Sensitivity %s has rank = 3!\n", dname);
      return CAPS_BADRANK;
    }
    len = strlen(dname);
    for (j = 1; j < len; j++)
      if (dname[j] == '[') {
        open = j;
        break;
      }
    if (open != 0) {
      pname = (char *) malloc((len+1)*sizeof(char));
      if (pname == NULL) return EGADS_MALLOC;
      for (i = 0; i < len; i++) pname[i] = dname[i];
      pname[open] = 0;
      pname[len]  = 0;
      for (i = open+1; i < len; i++) {
        if (pname[i] == ' ') {
          EG_free(pname);
          return CAPS_BADNAME;
        }
        if (pname[i] == ',') pname[i] = ' ';
      }
      sscanf(&pname[open+1], "%d%d\n", &irow, &icol);
    } else {
      pname = (char *) dname;
    }
    for (i = 0; i < problem->nGeomIn; i++)
      if (strcmp(pname,problem->geomIn[i]->name) == 0) {
        value  = (capsValue *) problem->geomIn[i]->blind;
        status = ocsmGetPmtr(problem->modl, value->pIndex, &type, &nrow, &ncol,
                             name);
        if (status < SUCCESS) {
          printf("caps_makeDataSet: %s ocsmGetPmtr = %d!\n", dname, status);
          return status;
        }
        if (type != OCSM_EXTERNAL) {
          printf("caps_makeDataSet: %s is NOT a Design Parameter!\n", dname);
          return CAPS_NOSENSITVTY;
        }
        if ((irow != 1) || (icol != 1)) {
          if (value == NULL) {
            if (open != 0) EG_free(pname);
            return CAPS_NULLVALUE;
          }
          if ((irow < 1) || (irow > value->nrow) ||
              (icol < 1) || (icol > value->ncol)) {
            if (open != 0) EG_free(pname);
            return CAPS_BADINDEX;
          }
        }
        break;
      }
    if (open != 0) EG_free(pname);
    if (i == problem->nGeomIn) {
      printf("caps_makeDataSet: %s does NOT matches GeometryInput!\n", dname);
      return CAPS_BADNAME;
    }
  } else {
    for (i = 0; i < problem->nGeomIn; i++)
      if (strcmp(dname,problem->geomIn[i]->name) == 0) {
        printf("caps_makeDataSet: %s matches GeometryInput!\n", dname);
        return CAPS_BADNAME;
      }
  }
  
  /* is this name unique? */
  for (i = 0; i < vertexset->nDataSets; i++)
    if (strcmp(dname,vertexset->dataSets[i]->name) == 0) {
      printf("caps_makeDataSet: %s is already registered!\n", dname);
      if (open != 0) EG_free(pname);
      return CAPS_BADNAME;
    }
  
  /* check rank -- also do we have another source in the bound? */
  if ((meth != Sensitivity) && (meth != BuiltIn))
    for (i = 0; i < bound->nVertexSet; i++) {
      if (vobject == bound->vertexSet[i]) continue;
      if (bound->vertexSet[i]              == NULL)      return CAPS_NULLOBJ;
      if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
      if (bound->vertexSet[i]->type        != VERTEXSET) return CAPS_BADTYPE;
      if (bound->vertexSet[i]->blind       == NULL)      return CAPS_NULLBLIND;
      otherset = (capsVertexSet *) bound->vertexSet[i]->blind;
      
      for (j = 0; j < otherset->nDataSets; j++) {
        if (strcmp(dname, otherset->dataSets[j]->name) != 0) continue;
        dataset = (capsDataSet *) otherset->dataSets[j]->blind;
        if (dataset == NULL) continue;
        if ((meth == Interpolate) || (meth == Conserve)) {
          if (dataset->rank != rank) {
            printf("caps_makeDataSet: %s with rank = %d should be %d!\n",
                   dname, rank, dataset->rank);
            return CAPS_BADRANK;
          }
        } else {
          if (dataset->rank != rank) {
            printf("caps_makeDataSet: %s with rank = %d should be %d!\n",
                   dname, rank, dataset->rank);
            return CAPS_BADRANK;
          }
        }
        if ((meth == User) || (meth == Analysis))
          if ((dataset->method == User) || (dataset->method == Analysis)) {
            printf("caps_makeDataSet: %s already has Source!\n", dname);
            return CAPS_BADMETHOD;
          }
      }
    }
  
  /* fill in the dataset data */
  dataset = (capsDataSet *) EG_alloc(sizeof(capsDataSet));
  if (dataset == NULL) return EGADS_MALLOC;
  dataset->method  = meth;
  dataset->nHist   = 0;
  dataset->history = NULL;
  dataset->npts    = 0;
  dataset->rank    = rank;
  dataset->dflag   = 0;
  dataset->data    = NULL;
  dataset->units   = NULL;
  dataset->startup = NULL;
  if ((meth == Interpolate) || (meth == Conserve)) {
    status = aim_UsesDataSet(problem->aimFPTR, analysis->loadName,
                             analysis->instance, &analysis->info,
                             bobject->name, dname, meth);
    if (status == CAPS_SUCCESS) dataset->dflag = 1;
  }
  
  /* make the object */
  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    EG_free(dataset);
    return status;
  }
  
  if (vertexset->dataSets == NULL) {
    vertexset->dataSets = (capsObject  **) EG_alloc(sizeof(capsObject *));
    if (vertexset->dataSets == NULL) {
      EG_free(object);
      EG_free(dataset);
      return EGADS_MALLOC;
    }
  } else {
    tmp = (capsObject **) EG_reall(vertexset->dataSets,
                                   (vertexset->nDataSets+1)*sizeof(capsObject *));
    if (tmp == NULL) {
      EG_free(object);
      EG_free(dataset);
      return EGADS_MALLOC;
    }
    vertexset->dataSets = tmp;
  }
  
  object->parent = vobject;
  object->name   = EG_strdup(dname);
  object->type   = DATASET;
  object->blind  = dataset;
  
  vertexset->dataSets[vertexset->nDataSets] = object;
  vertexset->nDataSets += 1;
  object->last.sNum     = 0;
  caps_fillDateTime(object->last.datetime);
  
  *dobj = object;
  return CAPS_SUCCESS;
}


int
caps_makeVertexSet(capsObject *bobject, /*@null@*/ capsObject *aobject,
                   /*@null@*/ const char *vname, capsObject **vobj)
{
  int           i, status;
  const char    *name;
  capsAnalysis  *analysis;
  capsBound     *bound;
  capsVertexSet *vertexset;
  capsObject    *object, **tmp, *ds[4] = {NULL, NULL, NULL, NULL};
  
  if (bobject              == NULL)        return CAPS_NULLOBJ;
  if (bobject->magicnumber != CAPSMAGIC)   return CAPS_BADOBJECT;
  if (bobject->type        != BOUND)       return CAPS_BADTYPE;
  if (bobject->blind       == NULL)        return CAPS_NULLBLIND;
  bound = (capsBound *) bobject->blind;
  if (bound->state         != Open)        return CAPS_STATEERR;
  
  name = vname;
  if (aobject != NULL) {
    /* connected vertex set */
    if (aobject              == NULL)      return CAPS_NULLOBJ;
    if (aobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
    if (aobject->type        != ANALYSIS)  return CAPS_BADTYPE;
    if (aobject->blind       == NULL)      return CAPS_NULLBLIND;
    if (vname == NULL) name = aobject->name;
  }
  if (name                   == NULL)      return CAPS_NULLNAME;
 
  /* unique name? */
  for (i = 0; i < bound->nVertexSet; i++)
    if (strcmp(name, bound->vertexSet[i]->name) == 0) return CAPS_BADNAME;

  vertexset = (capsVertexSet *) EG_alloc(sizeof(capsVertexSet));
  if (vertexset == NULL) return EGADS_MALLOC;
  vertexset->analysis  = aobject;
  vertexset->discr     = NULL;
  vertexset->nDataSets = 0;
  vertexset->dataSets  = NULL;
  
  /* make the object */
  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    EG_free(vertexset);
    return status;
  }
  
  if (bound->vertexSet == NULL) {
    bound->vertexSet = (capsObject  **) EG_alloc(sizeof(capsObject *));
    if (bound->vertexSet == NULL) {
      EG_free(object);
      EG_free(vertexset);
      return EGADS_MALLOC;
    }
  } else {
    tmp = (capsObject **) EG_reall( bound->vertexSet,
                                   (bound->nVertexSet+1)*sizeof(capsObject *));
    if (tmp == NULL) {
      EG_free(object);
      EG_free(vertexset);
      return EGADS_MALLOC;
    }
    bound->vertexSet = tmp;
  }

  vertexset->discr = (capsDiscr *) EG_alloc(sizeof(capsDiscr));
  if (vertexset->discr == NULL) {
    EG_free(object);
    EG_free(vertexset);
    return EGADS_MALLOC;
  }
  vertexset->discr->dim      = bound->dim;
  vertexset->discr->instance = -1;
  vertexset->discr->nPoints  = 0;
  vertexset->discr->aInfo    = NULL;
  vertexset->discr->mapping  = NULL;
  vertexset->discr->nVerts   = 0;
  vertexset->discr->verts    = NULL;
  vertexset->discr->celem    = NULL;
  vertexset->discr->nTypes   = 0;
  vertexset->discr->types    = NULL;
  vertexset->discr->nElems   = 0;
  vertexset->discr->elems    = NULL;
  vertexset->discr->nDtris   = 0;
  vertexset->discr->dtris    = NULL;
  vertexset->discr->ptrm     = NULL;

/*@-kepttrans@*/
  object->parent  = bobject;
/*@+kepttrans@*/
  object->name    = EG_strdup(name);
  object->type    = VERTEXSET;
  object->subtype = UNCONNECTED;
  object->blind   = vertexset;
  
  status = caps_makeDataSet(object, "xyz",  BuiltIn, 3, &ds[0]);
  if (status != CAPS_SUCCESS) {
    EG_free(vertexset->discr);
    EG_free(object->blind);
    EG_free(object);
    return EGADS_MALLOC;
  }
  if (aobject != NULL) {
    analysis = (capsAnalysis *) aobject->blind;
    if (analysis != NULL) {
      vertexset->discr->instance =  analysis->instance;
      vertexset->discr->aInfo    = &analysis->info;
    }
    object->subtype = CONNECTED;
    status = caps_makeDataSet(object, "xyzd", BuiltIn, 3, &ds[1]);
    if (status != CAPS_SUCCESS) {
      EG_free(ds[0]->name);
      EG_free(ds[0]->blind);
      EG_free(vertexset->dataSets);
      EG_free(vertexset->discr);
      EG_free(object->blind);
      EG_free(object);
      return EGADS_MALLOC;
    }
  }
  if (bound->dim != 3) {
    status = caps_makeDataSet(object, "param",  BuiltIn, bound->dim, &ds[2]);
    if (status != CAPS_SUCCESS) {
      if (aobject != NULL) {
        EG_free(ds[1]->name);
        EG_free(ds[1]->blind);
      }
      EG_free(ds[0]->name);
      EG_free(ds[0]->blind);
      EG_free(vertexset->dataSets);
      EG_free(vertexset->discr);
      EG_free(object->blind);
      EG_free(object);
      return EGADS_MALLOC;
    }
    if (aobject != NULL) {
      status = caps_makeDataSet(object, "paramd", BuiltIn, bound->dim, &ds[3]);
      if (status != CAPS_SUCCESS) {
        EG_free(ds[2]->name);
        EG_free(ds[2]->blind);
        EG_free(ds[1]->name);
        EG_free(ds[1]->blind);
        EG_free(ds[0]->name);
        EG_free(ds[0]->blind);
        EG_free(vertexset->dataSets);
        EG_free(vertexset->discr);
        EG_free(object->blind);
        EG_free(object);
        return EGADS_MALLOC;
      }
    }
  }

  bound->vertexSet[bound->nVertexSet] = object;
  bound->nVertexSet += 1;
  object->last.sNum  = 0;
  caps_fillDateTime(object->last.datetime);

  *vobj = object;
  return CAPS_SUCCESS;
}


int
caps_vertexSetInfo(const capsObject *vobject, int *nGpts, int *nDpts,
                         capsObject **bobj, capsObject **aobj)
{
  capsVertexSet *vertexset;
  
  *nGpts = *nDpts = 0;
  *bobj  = *aobj  = NULL;
  if (vobject              == NULL)      return CAPS_NULLOBJ;
  if (vobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vobject->type        != VERTEXSET) return CAPS_BADTYPE;
  if (vobject->blind       == NULL)      return CAPS_NULLBLIND;
  vertexset = (capsVertexSet *) vobject->blind;
  
  if (vertexset->discr != NULL) {
    *nGpts = vertexset->discr->nPoints;
    *nDpts = vertexset->discr->nVerts;
  }
  *bobj = vobject->parent;
  *bobj = vertexset->analysis;
  
  return CAPS_SUCCESS;
}


int
caps_fillUnVertexSet(capsObject *vobject, int npts, const double *xyzs)
{
  int           i;
  capsDataSet   *dataset;
  capsVertexSet *vertexset;
  capsObject    *bobject, *pobject, *dobject = NULL;
  capsProblem   *problem;
  
  if (vobject              == NULL)      return CAPS_NULLOBJ;
  if (vobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vobject->type        != VERTEXSET) return CAPS_BADTYPE;
  if (vobject->blind       == NULL)      return CAPS_NULLBLIND;
  vertexset = (capsVertexSet *) vobject->blind;
  if (vertexset->dataSets  == NULL)      return CAPS_BADMETHOD;
  dobject   = vertexset->dataSets[0];
  if (dobject->blind       == NULL)      return CAPS_NULLBLIND;
  dataset   = (capsDataSet *) dobject->blind;
  if (vertexset->analysis  != NULL)      return CAPS_NOTCONNECT;
  bobject   = vobject->parent;
  if (bobject              == NULL)      return CAPS_NULLOBJ;
  if (bobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (bobject->type        != BOUND)     return CAPS_BADTYPE;
  pobject   = bobject->parent;
  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;

  caps_freeOwner(&vobject->last);
  vobject->last.sNum  = 0;
  caps_fillDateTime(vobject->last.datetime);

  /* clear this out */
  if (npts <= 0) {
    if (vertexset->discr == NULL) return CAPS_SUCCESS;
    if (vertexset->discr->verts != NULL) EG_free(vertexset->discr->verts);
    EG_free(vertexset->discr);
    vertexset->discr = NULL;
    if (dataset->data != NULL) EG_free(dataset->data);
    dataset->data = NULL;
    dataset->npts = 0;
    return CAPS_SUCCESS;
  }
  
  if (vertexset->discr->nVerts != npts) {
    vertexset->discr->nVerts = 0;
    if (vertexset->discr->verts != NULL) EG_free(vertexset->discr->verts);
    vertexset->discr->verts = (double *) EG_alloc(3*npts*sizeof(double));
    if (vertexset->discr->verts == NULL) return EGADS_MALLOC;
    vertexset->discr->nVerts = npts;
  }
  if (dataset->npts != npts) {
    dataset->npts = 0;
    if (dataset->data != NULL) EG_free(dataset->data);
    dataset->data = (double *) EG_alloc(3*npts*sizeof(double));
    if (dataset->data != NULL) dataset->npts = npts;
  }

  for (i = 0; i < npts; i++) {
    vertexset->discr->verts[3*i  ] = xyzs[3*i  ];
    vertexset->discr->verts[3*i+1] = xyzs[3*i+1];
    vertexset->discr->verts[3*i+2] = xyzs[3*i+2];
    if (dataset->data == NULL) continue;
    dataset->data[3*i  ] = xyzs[3*i  ];
    dataset->data[3*i+1] = xyzs[3*i+1];
    dataset->data[3*i+2] = xyzs[3*i+2];
  }
  problem->sNum     += 1;
  vobject->last.sNum = problem->sNum;

  return CAPS_SUCCESS;
}


int
caps_initDataSet(capsObject *dobject, int rank, const double *startup)
{
  int         i;
  double      *data;
  capsDataSet *dataset;
  
  if  (dobject              == NULL)      return CAPS_NULLOBJ;
  if  (dobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if  (dobject->type        != DATASET)   return CAPS_BADTYPE;
  if  (dobject->blind       == NULL)      return CAPS_NULLBLIND;
  if  (startup == NULL)                   return CAPS_NULLVALUE;
  dataset = (capsDataSet *) dobject->blind;
  if  (dataset->rank        != rank)      return CAPS_BADRANK;
  if ((dataset->method      != Interpolate) &&
      (dataset->method      != Conserve)) return CAPS_BADMETHOD;
  if  (dataset->startup     != NULL)      return CAPS_EXISTS;
  
  data = (double *) EG_alloc(rank*sizeof(double));
  if (data == NULL) return EGADS_MALLOC;
  
  for (i = 0; i < rank; i++) data[i] = startup[i];
  dataset->startup = data;
  
  return CAPS_SUCCESS;
}


int
caps_setData(capsObject *dobject, int nverts, int rank, const double *data,
             const char *units)
{
  int         OK = 1;
  char        *uarray = NULL;
  double      *darray;
  capsObject  *vobject, *bobject, *pobject;
  capsDataSet *dataset;
  capsBound   *bound;
  capsProblem *problem;
  capsOwn     *tmp;

  if ((nverts <= 0) || (data == NULL))   return CAPS_NULLVALUE;
  if (dobject              == NULL)      return CAPS_NULLOBJ;
  if (dobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (dobject->type        != DATASET)   return CAPS_BADTYPE;
  if (dobject->blind       == NULL)      return CAPS_NULLBLIND;
  dataset = (capsDataSet *) dobject->blind;
  if (dataset->rank        != rank)      return CAPS_BADRANK;
  if (dataset->method      != User)      return CAPS_BADMETHOD;
  vobject = dobject->parent;
  if (vobject              == NULL)      return CAPS_NULLOBJ;
  if (vobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vobject->type        != VERTEXSET) return CAPS_BADTYPE;
  if (vobject->blind       == NULL)      return CAPS_NULLBLIND;
  bobject = vobject->parent;
  if (bobject              == NULL)      return CAPS_NULLOBJ;
  if (bobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (bobject->type        != BOUND)     return CAPS_BADTYPE;
  if (bobject->blind       == NULL)      return CAPS_NULLBLIND;
  bound   = (capsBound *) bobject->blind;
  if (bound->state         == Open)      return CAPS_STATEERR;
  pobject = bobject->parent;
  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;

  darray = (double *) EG_alloc(rank*nverts*sizeof(double));
  if (darray == NULL) return EGADS_MALLOC;
  if (units != NULL) {
    uarray = EG_strdup(units);
    if (uarray == NULL) {
      EG_free(darray);
      return EGADS_MALLOC;
    }
  }
  
  if (dataset->units != NULL) EG_free(dataset->units);
  if (dataset->data  != NULL) EG_free(dataset->data);
  dataset->npts  = nverts;
  dataset->rank  = rank;
  dataset->data  = darray;
  dataset->units = uarray;

  if (dobject->last.sNum != 0) {
    if (dataset->history == NULL) {
      dataset->nHist   = 0;
      dataset->history = (capsOwn *) EG_alloc(sizeof(capsOwn));
      if (dataset->history == NULL) OK = 0;
    } else {
      tmp = (capsOwn *) EG_reall( dataset->history,
                                 (dataset->nHist+1)*sizeof(capsOwn));
      if (tmp == NULL) {
        OK = 0;
      } else {
        dataset->history = tmp;
      }
    }
    if ((OK == 1) && (dataset->history != NULL)) {
      dataset->history[dataset->nHist]       = dobject->last;
      dataset->history[dataset->nHist].pname = EG_strdup(dobject->last.pname);
      dataset->history[dataset->nHist].pID   = EG_strdup(dobject->last.pID);
      dataset->history[dataset->nHist].user  = EG_strdup(dobject->last.user);
      dataset->nHist += 1;
    }
  }

  caps_freeOwner(&dobject->last);
  problem->sNum     += 1;
  dobject->last.sNum = problem->sNum;
  caps_fillDateTime(dobject->last.datetime);
  
  return CAPS_SUCCESS;
}


/*
 * obj_bar: compute objective function and gradient via reverse differentiation
 */
static int
obj_bar(int    n,                     /* (in)  number of design variables */
        double ftgt[],                /* (in)  design variables */
        void   *blind,                /* (in)  blind pointer to structure */
        double *obj,                  /* (out) objective function */
        double ftgt_bar[])            /* (out) gradient of objective function */
{
  int     status = CAPS_SUCCESS;      /* (out) return status */
  
  int        idat, ielms, ielmt, imat, irank, jrank, nrank, sindx, tindx;
  char       *name;
  double     f_src, f_tgt;
  double     area_src, area_tgt, area_tgt_bar, obj_bar1;
  double     *result, *result_bar = NULL, *data_bar = NULL;
  capsConFit *cfit = (capsConFit *) blind;
  
  irank  = cfit->irank;
  nrank  = cfit->nrank;
  sindx  = cfit->sindx;
  tindx  = cfit->tindx;
  name   = cfit->name;
  result = (double *) EG_alloc(nrank*sizeof(double));
  if (result == NULL) return EGADS_MALLOC;
  
  /* store ftgt into tgt structure */
  for (idat = 0; idat < n; idat++)
    cfit->data_tgt[nrank*idat+irank] = ftgt[idat];
  
  /* compute the area for src */
  area_src = 0.0;
  
  for (ielms = 0; ielms < cfit->src->nElems; ielms++) {
    status = aim_IntegrIndex(*cfit->aimFPTR, sindx, cfit->src, name, ielms+1,
                             nrank, cfit->data_src, result);
    if (status != CAPS_SUCCESS) goto cleanup;
    
    area_src += result[irank];
  }
  cfit->area_src = area_src;
  
  /* compute the area for tgt */
  area_tgt = 0.0;
  
  for (ielmt = 0; ielmt < cfit->tgt->nElems; ielmt++) {
    status = aim_IntegrIndex(*cfit->aimFPTR, tindx, cfit->tgt, name, ielmt+1,
                             nrank, cfit->data_tgt, result);
    if (status != CAPS_SUCCESS) goto cleanup;
    
    area_tgt += result[irank];
  }
  cfit->area_tgt = area_tgt;
  
  /* penalty function part of objective function */
  *obj = cfit->afact * pow(area_tgt-area_src, 2);
  
  /* minimize the difference between source and target at the Match points */
  for (imat = 0; imat < cfit->nmat; imat++) {
    ielms  = cfit->mat[imat].source.eIndex;
    ielmt  = cfit->mat[imat].target.eIndex;
    if ((ielms == -1) || (ielmt == -1)) continue;
    status = aim_InterpolIndex(*cfit->aimFPTR, sindx, cfit->src, name, ielms,
                               cfit->mat[imat].source.st, nrank, cfit->data_src,
                               result);
    if (status != CAPS_SUCCESS) goto cleanup;
    f_src  = result[irank];
    
    status = aim_InterpolIndex(*cfit->aimFPTR, tindx, cfit->tgt, name, ielmt,
                               cfit->mat[imat].target.st, nrank, cfit->data_tgt,
                               result);
    if (status != CAPS_SUCCESS) goto cleanup;
    f_tgt  = result[irank];
    
    *obj  += pow(f_tgt-f_src, 2);
  }
  
  /* if we do not need gradient, return now */
  if (ftgt_bar == NULL) goto cleanup;
  
  result_bar = (double *) EG_alloc(nrank*sizeof(double));
  if (result_bar == NULL) {
    status = EGADS_MALLOC;
    goto cleanup;
  }
  data_bar = (double *) EG_alloc(nrank*n*sizeof(double));
  if (data_bar == NULL) {
    status = EGADS_MALLOC;
    goto cleanup;
  }
  
  /* initialize the derivatives */
  obj_bar1  = 1.0;
  for (jrank = 0; jrank < nrank; jrank++) {
    for (idat = 0; idat < n; idat++)
      data_bar[nrank*idat+jrank] = 0.0;
    result_bar[jrank] = 0.0;
  }
  
  /* reverse: minimize the difference between the source and target
              at the Match points */
  for (imat = cfit->nmat-1; imat >= 0; imat--) {
    ielms  = cfit->mat[imat].source.eIndex;
    ielmt  = cfit->mat[imat].target.eIndex;
    if ((ielms == -1) || (ielmt == -1)) continue;
    status = aim_InterpolIndex(*cfit->aimFPTR, sindx, cfit->src, name, ielms,
                               cfit->mat[imat].source.st, nrank, cfit->data_src,
                               result);
    if (status != CAPS_SUCCESS) goto cleanup;
    f_src  = result[irank];

    status = aim_InterpolIndex(*cfit->aimFPTR, tindx, cfit->tgt, name, ielmt,
                               cfit->mat[imat].target.st, nrank, cfit->data_tgt,
                               result);
    if (status != CAPS_SUCCESS) goto cleanup;
    f_tgt  = result[irank];
    
    result_bar[irank] = (f_tgt - f_src) * 2 * obj_bar1;
    
    status = aim_InterpolIndBar(*cfit->aimFPTR, tindx, cfit->tgt, name, ielmt,
                                cfit->mat[imat].target.st, nrank, result_bar,
                                data_bar);
    if (status != CAPS_SUCCESS) goto cleanup;
  }
  
  /* reverse: penalty function part of objective function */
  area_tgt_bar = cfit->afact * 2 * (area_tgt - area_src) * obj_bar1;
  
  result_bar[irank] = area_tgt_bar;
  
  /* reverse: compute the area for tgt */
  for (ielmt = cfit->tgt->nElems-1; ielmt >= 0; ielmt--) {
    status = aim_IntegrIndBar(*cfit->aimFPTR, tindx, cfit->tgt, name, ielmt+1,
                              nrank, result_bar, data_bar);
    if (status != CAPS_SUCCESS) goto cleanup;
  }
  
  for (idat = 0; idat < n; idat++)
    ftgt_bar[idat] = data_bar[nrank*idat+irank];
  
cleanup:
  if (data_bar   != NULL) EG_free(data_bar);
  if (result_bar != NULL) EG_free(result_bar);
  EG_free(result);
  
  return status;
}


static int
caps_Match(capsConFit *fit, int dim)
{
  int       i, j, k, n, t, npts, stat;
  double    pos[3], *st;
  capsMatch *match;

  /* count the positions */
  for (npts = i = 0; i < fit->tgt->nElems; i++) {
    t = fit->tgt->elems[i].tIndex - 1;
    if (fit->tgt->types[t].nmat == 0) {
      npts += fit->tgt->types[t].nref;
    } else {
      npts += fit->tgt->types[t].nmat;
    }
  }

  /* set the match locations */
  match = (capsMatch *) EG_alloc(npts*sizeof(capsMatch));
  if (match == NULL) return EGADS_MALLOC;
  for (i = 0; i < npts; i++) {
    match[i].source.eIndex = -1;
    match[i].source.st[0]  = 0.0;
    match[i].source.st[1]  = 0.0;
    match[i].source.st[2]  = 0.0;
    match[i].target.eIndex = -1;
    match[i].target.st[0]  = 0.0;
    match[i].target.st[1]  = 0.0;
    match[i].target.st[2]  = 0.0;
  }
  
  /* set things up in parameter (or 3) space */
  for (k = i = 0; i < fit->tgt->nElems; i++) {
    t = fit->tgt->elems[i].tIndex - 1;
    if (fit->tgt->types[t].nmat == 0) {
      n  = fit->tgt->types[t].nref;
      st = fit->tgt->types[t].gst;
    } else {
      n  = fit->tgt->types[t].nmat;
      st = fit->tgt->types[t].matst;
    }
    for (j = 0; j < n; j++, k++) {
      match[i].target.eIndex = i+1;
      if (dim == 3) {
        match[i].target.st[0] = st[0];
        match[i].target.st[1] = st[1];
        match[i].target.st[2] = st[2];
        stat = aim_InterpolIndex(*fit->aimFPTR, fit->tindx, fit->tgt, "xyz", i+1,
                                 &st[3*j], 3, fit->prms_tgt, pos);
      } else {
        match[i].target.st[0] = st[0];
        if (dim == 2) match[i].target.st[1] = st[1];
        stat = aim_InterpolIndex(*fit->aimFPTR, fit->tindx, fit->tgt, "param",
                                 i+1, &st[dim*j], dim, fit->prms_tgt, pos);
      }
      if (stat != CAPS_SUCCESS) {
        printf(" caps_getData: %d/%d aim_Interpolation %d = %d (match)!\n",
               k, npts, fit->tindx, stat);
        continue;
      }
      stat = aim_LocateElIndex(*fit->aimFPTR, fit->sindx, fit->src,
                               fit->prms_src, pos, &match[i].source.eIndex,
                               match[i].source.st);
      if (stat != CAPS_SUCCESS)
        printf(" caps_getData: %d/%d aim_LocateElement = %d (match)!\n",
               i, npts, stat);
    }
  }
  
  fit->mat  = match;
  fit->nmat = npts;

  return CAPS_SUCCESS;
}


static int
caps_Conserve(capsConFit *fit, const char *bname, int dim)
{
  int    i, j, stat, *elems;
  double fopt, *ref, *ftgt, *tmp;
  FILE   *fp = NULL;

#ifdef DEBUG
  fp    = stdout;
#endif
  elems = (int *) EG_alloc(fit->npts*sizeof(int));
  if (elems == NULL) {
    printf(" caps_getData Error: Malloc on %d element indices!\n", fit->npts);
    return EGADS_MALLOC;
  }
  ref = (double *) EG_alloc(((dim+1)*fit->npts+fit->nrank)*sizeof(double));
  if (ref == NULL) {
    EG_free(elems);
    printf(" caps_getData Error: Malloc on %d element %d references!\n",
           fit->npts, dim);
    return EGADS_MALLOC;
  }
  ftgt = &ref[dim*fit->npts];
  tmp  = &ftgt[fit->npts];
  for (i = 0; i < fit->npts; i++) {
    elems[i] = 0;
    stat     = aim_LocateElIndex(*fit->aimFPTR, fit->sindx, fit->src,
                                  fit->prms_src, &fit->prms_tgt[dim*i],
                                 &elems[i], &ref[dim*i]);
    if (stat != CAPS_SUCCESS)
      printf(" caps_getData: %d/%d aim_LocateElement = %d!\n",
             i, fit->npts, stat);
  }
  
  stat = 0;
  for (j = 0; j < fit->nrank; j++) {
    fit->irank = j;
    for (i = 0; i < fit->npts; i++) {
      if (elems[i] == 0) continue;
      stat = aim_InterpolIndex(*fit->aimFPTR, fit->sindx, fit->src, fit->name,
                               elems[i], &ref[dim*i], fit->nrank,
                               fit->data_src, tmp);
      if (stat != CAPS_SUCCESS)
        printf(" caps_getData: %d/%d aim_Interpolation = %d!\n",
               i, fit->npts, stat);
      ftgt[i] = tmp[j];
    }
    stat = caps_conjGrad(obj_bar, fit, fit->npts, ftgt, 1e-6, fp, &fopt);
    if (stat != CAPS_SUCCESS) break;

    if (j == 0)
      printf("Bound %s: Normalized Integrated %s\n", bname, fit->name);
    printf("  Rank=%d: src=%le, tgt=%le, diff=%le\n", j,
           fit->area_src, fit->area_tgt, fabs(fit->area_src-fit->area_tgt));
  }
  
  EG_free(ref);
  EG_free(elems);
  return stat;
}


int
caps_triangulate(const capsObject *vobject, int *nGtris, int **gtris,
                 int *nDtris, int **dtris)
{
  int           i, j, n, ntris, eType, *tris;
  capsVertexSet *vertexset;
  capsDiscr     *discr;
  
  *nGtris = 0;
  *nDtris = 0;
  *gtris  = NULL;
  *dtris  = NULL;
  if (vobject              == NULL)      return CAPS_NULLOBJ;
  if (vobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vobject->type        != VERTEXSET) return CAPS_BADTYPE;
  if (vobject->blind       == NULL)      return CAPS_NULLBLIND;
  vertexset = (capsVertexSet *) vobject->blind;
  
  if (vertexset->discr == NULL) return CAPS_SUCCESS;
  discr = vertexset->discr;
  if ((discr->nPoints == 0) && (discr->nVerts == 0)) return CAPS_SUCCESS;
  
  for (ntris = j = 0; j < discr->nElems; j++) {
    eType = discr->elems[j].tIndex;
    if (discr->types[eType-1].tris == NULL) {
      ntris++;
    } else {
      ntris += discr->types[eType-1].ntri;
    }
  }
  if (ntris != 0) {
    tris = (int *) EG_alloc(3*ntris*sizeof(int));
    if (tris == NULL) return EGADS_MALLOC;
    for (ntris = j = 0; j < discr->nElems; j++) {
      eType = discr->elems[j].tIndex;
      if (discr->types[eType-1].tris == NULL) {
        tris[3*ntris  ] = discr->elems[j].gIndices[0];
        tris[3*ntris+1] = discr->elems[j].gIndices[2];
        tris[3*ntris+2] = discr->elems[j].gIndices[4];
        ntris++;
      } else {
        for (i = 0; i < discr->types[eType-1].ntri; i++, ntris++) {
          n = discr->types[eType-1].tris[3*i  ] - 1;
          tris[3*ntris  ] = discr->elems[j].gIndices[2*n];
          n = discr->types[eType-1].tris[3*i+1] - 1;
          tris[3*ntris+1] = discr->elems[j].gIndices[2*n];
          n = discr->types[eType-1].tris[3*i+2] - 1;
          tris[3*ntris+2] = discr->elems[j].gIndices[2*n];
        }
      }
    }
    *nGtris = ntris;
    *gtris  = tris;
  }
  if ((discr->nDtris == 0) || (discr->dtris == NULL) ||
      (discr->nVerts == 0)) return CAPS_SUCCESS;
  
  tris = (int *) EG_alloc(3*discr->nDtris*sizeof(int));
  if (tris == NULL) {
    EG_free(*gtris);
    *nGtris = 0;
    *gtris  = NULL;
    return EGADS_MALLOC;
  }
  for (j = 0; j < 3*discr->nDtris; j++) tris[j] = discr->dtris[j];
  *nDtris = discr->nDtris;
  *dtris  = tris;
  
  return CAPS_SUCCESS;
}


int
caps_outputVertexSet(const capsObject *vobject, const char *filename)
{
  int           i, j, k, stat, nGtris, *gtris, nDtris, *dtris;
  FILE          *fp;
  capsVertexSet *vertexset;
  capsDataSet   *dataset;

  if (vobject              == NULL)      return CAPS_NULLOBJ;
  if (vobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vobject->type        != VERTEXSET) return CAPS_BADTYPE;
  if (vobject->blind       == NULL)      return CAPS_NULLBLIND;
  if (filename             == NULL)      return CAPS_NULLNAME;
  vertexset = (capsVertexSet *) vobject->blind;
  fp = fopen(filename, "w");
  if (fp                   == NULL)      return CAPS_IOERR;
  stat = caps_triangulate(vobject, &nGtris, &gtris, &nDtris, &dtris);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_outputVertexSet Error: caps_triangulate = %d!\n", stat);
    fclose(fp);
    return stat;
  }
  printf(" **** writing VertexSet file: %s ****\n", filename);
  fprintf(fp, "%s\n", vobject->parent->name);
  fprintf(fp, "%8d %8d %8d\n", nGtris, nDtris, vertexset->nDataSets);
  for (i = 0; i < nGtris; i++)
    fprintf(fp, "    %8d %8d %8d\n", gtris[3*i  ], gtris[3*i+1], gtris[3*i+2]);
  for (i = 0; i < nDtris; i++)
    fprintf(fp, "    %8d %8d %8d\n", dtris[3*i  ], dtris[3*i+1], dtris[3*i+2]);
  EG_free(gtris);
  EG_free(dtris);
  for (i = 0; i < vertexset->nDataSets; i++) {
    fprintf(fp, "%s\n", vertexset->dataSets[i]->name);
    dataset = (capsDataSet *) vertexset->dataSets[i]->blind;
    fprintf(fp, " %8d %8d\n", dataset->npts, dataset->rank);
    for (j = 0; j < dataset->npts; j++) {
      for (k = 0; k < dataset->rank; k++)
        fprintf(fp, " %lf", dataset->data[j*dataset->rank+k]);
      fprintf(fp, "\n");
    }
  }
  fclose(fp);
  return CAPS_SUCCESS;
}


int
caps_getData(capsObject *dobject, int *npts, int *rank, double **data,
             char **units)
{
  int           i, j, elem, index, stat, OK = 1;
  double        *values, *params, *param, st[3];
  capsObject    *vobject, *bobject, *pobject, *aobject, *foundset, *foundanl;
  capsDataSet   *dataset, *otherset, *ds;
  capsVertexSet *vertexset, *fvs = NULL;
  capsDiscr     *discr, *founddsr = NULL;
  capsAnalysis  *analysis = NULL, *fanal;
  capsBound     *bound;
  capsProblem   *problem;
  capsOwn       *tmp;
  capsConFit    fit;
#ifdef VSOUTPUT
  char          fileName[20];
  static int    fileCnt = 0;
  capsObject    *ovs    = NULL;
#endif
  
  *npts  = *rank = 0;
  *data  = NULL;
  *units = NULL;
  if (dobject              == NULL)      return CAPS_NULLOBJ;
  if (dobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (dobject->type        != DATASET)   return CAPS_BADTYPE;
  if (dobject->blind       == NULL)      return CAPS_NULLBLIND;
  dataset = (capsDataSet *) dobject->blind;
  vobject = dobject->parent;
  if (vobject              == NULL)      return CAPS_NULLOBJ;
  if (vobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (vobject->type        != VERTEXSET) return CAPS_BADTYPE;
  if (vobject->blind       == NULL)      return CAPS_NULLBLIND;
  vertexset = (capsVertexSet *) vobject->blind;
  discr     = vertexset->discr;
  aobject   = vertexset->analysis;
  bobject   = vobject->parent;
  if (bobject              == NULL)      return CAPS_NULLOBJ;
  if (bobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (bobject->type        != BOUND)     return CAPS_BADTYPE;
  if (bobject->blind       == NULL)      return CAPS_NULLBLIND;
  bound   = (capsBound *) bobject->blind;
  if (bound->state         == Open)      return CAPS_STATEERR;
  pobject = bobject->parent;
  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;
  if (aobject != NULL) {
    if (aobject->magicnumber  != CAPSMAGIC)          return CAPS_BADOBJECT;
    if (aobject->type         != ANALYSIS)           return CAPS_BADTYPE;
    if (aobject->blind        == NULL)               return CAPS_NULLBLIND;
    analysis = (capsAnalysis *) aobject->blind;
    if ((dataset->method == Sensitivity) || (dataset->method == Analysis)) {
      if (problem->geometry.sNum > analysis->pre.sNum) return CAPS_MISMATCH;
      if (analysis->pre.sNum     > aobject->last.sNum) return CAPS_MISMATCH;
    }
  }
  otherset = NULL;
  foundanl = NULL;
  
  /*
   * Sensitivity - filled in preAnalysis
   * Analysis    - taken care of in postAnalysis
   * User        - from explicit calls to setData
   */
  if ((dataset->method == Interpolate) || (dataset->method == Conserve)) {
    /* find source in other VertexSets */
    foundset = NULL;
    for (i = 0; i < bound->nVertexSet; i++) {
      if (vobject == bound->vertexSet[i]) continue;
      if (bound->vertexSet[i]              == NULL)      return CAPS_NULLOBJ;
      if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
      if (bound->vertexSet[i]->type        != VERTEXSET) return CAPS_BADTYPE;
      fvs = (capsVertexSet *) bound->vertexSet[i]->blind;
      if (fvs                              == NULL)      return CAPS_NULLBLIND;
#ifdef VSOUTPUT
      ovs = bound->vertexSet[i];
#endif
      
      for (j = 0; j < fvs->nDataSets; j++) {
        if (strcmp(dobject->name, fvs->dataSets[j]->name) != 0) continue;
        otherset = (capsDataSet *) fvs->dataSets[j]->blind;
        if (otherset == NULL) continue;
        if ((otherset->method == User) || (otherset->method == Analysis)) {
          foundset = fvs->dataSets[j];
          foundanl = fvs->analysis;
          founddsr = fvs->discr;
          break;
        }
      }
      if (foundset != NULL) break;
    }
    if ((fvs == NULL) || (foundset == NULL) || (otherset == NULL) ||
        (founddsr == NULL)) {
      printf(" caps_getData Error: Bound %s -- %s without a source DataSet!\n",
             bobject->name, dobject->name);
      return CAPS_SOURCEERR;
    }
    
    /* have we been updated? */
    if ((foundset->last.sNum == 0) && (dataset->startup != NULL)) {
      /* bypass everything because we are in a startup situation */
      *rank  = dataset->rank;
      *npts  = 1;
      *data  = dataset->startup;
      *units = dataset->units;
      return CAPS_SUCCESS;
    }
    if ((foundset->last.sNum > dobject->last.sNum) || (dataset->data == NULL)) {
      if (foundanl == NULL) {
        printf(" caps_getData Error: Bound %s -- Analysis is NULL!\n",
               bobject->name);
        return CAPS_BADOBJECT;
      }
      fanal = (capsAnalysis *) foundanl->blind;
      if (fanal == NULL) {
        printf(" caps_getData Error: Bound %s -- Analysis %s blind NULL!\n",
               bobject->name, foundanl->name);
        return CAPS_NULLVALUE;
      }
      index = aim_Index(problem->aimFPTR, fanal->loadName);
      if (index < 0) return index;

      *rank  = dataset->rank;
      *npts  = discr->nPoints;
      if (discr->nVerts != 0) *npts = discr->nVerts;
      params = NULL;
      param  = NULL;
      if (bound->dim == 3) {
        for (j = 0; j < vertexset->nDataSets; j++) {
          if (strcmp("xyz", vertexset->dataSets[j]->name) != 0) continue;
          ds    = (capsDataSet *) vertexset->dataSets[j]->blind;
          param = ds->data;
          break;
        }
        for (j = 0; j < fvs->nDataSets; j++) {
          if (strcmp("xyz", fvs->dataSets[j]->name) != 0) continue;
          ds     = (capsDataSet *) fvs->dataSets[j]->blind;
          params = ds->data;
          break;
        }
      } else {
        if (discr->nVerts != 0) {
          for (j = 0; j < vertexset->nDataSets; j++) {
            if (strcmp("paramd", vertexset->dataSets[j]->name) != 0) continue;
            ds    = (capsDataSet *) vertexset->dataSets[j]->blind;
            param = ds->data;
            break;
          }
        } else {
          for (j = 0; j < vertexset->nDataSets; j++) {
            if (strcmp("param", vertexset->dataSets[j]->name) != 0) continue;
            ds    = (capsDataSet *) vertexset->dataSets[j]->blind;
            param = ds->data;
            break;
          }
        }
        for (j = 0; j < fvs->nDataSets; j++) {
          if (strcmp("param", fvs->dataSets[j]->name) != 0) continue;
          ds     = (capsDataSet *) fvs->dataSets[j]->blind;
          params = ds->data;
          break;
        }
      }
      
      if (params == NULL) {
        printf(" caps_getData Error: Bound %s -- %s without source params!\n",
               bobject->name, dobject->name);
        return CAPS_SOURCEERR;
      }
      if (param == NULL) {
        printf(" caps_getData Error: Bound %s -- %s without dst params!\n",
               bobject->name, dobject->name);
        return CAPS_SOURCEERR;
      }
      if (dataset->data != NULL) EG_free(dataset->data);
      j  = *rank;
      j *= *npts;
      values = (double *) EG_alloc(j*sizeof(double));
      if (values == NULL) {
        printf(" caps_getData Error: Malloc on %d %d  Dataset = %s!\n",
               *npts, *rank, dobject->name);
        *npts = *rank = 0;
        return EGADS_MALLOC;
      }
      for (i = 0; i < j; i++) values[i] = 0.0;
      
      /*compute */
      if (otherset->data == NULL) {
        printf(" caps_getData Error: Source for %s is NULL!\n", dobject->name);
        EG_free(values);
        *npts = *rank = 0;
        return CAPS_NULLVALUE;
      }
      if (dataset->method == Interpolate) {
        for (i = 0; i < *npts; i++) {
          stat = aim_LocateElIndex(problem->aimFPTR, index, founddsr, params,
                                   &param[bound->dim*i], &elem, st);
          if (stat != CAPS_SUCCESS) {
            printf(" caps_getData: %d/%d aim_LocateElement = %d for %s!\n",
                   i, *npts, stat, dobject->name);
            continue;
          }
          stat = aim_InterpolIndex(problem->aimFPTR, index, founddsr,
                                   dobject->name, elem, st, *rank,
                                   otherset->data, &values[*rank*i]);
          if (stat != CAPS_SUCCESS)
            printf(" caps_getData: %d/%d aim_Interpolation = %d for %s!\n",
                   i, *npts, stat, dobject->name);
        }
      } else {
        if ((aobject == NULL) || (analysis == NULL)) {
          EG_free(values);
          *npts = *rank = 0;
          return CAPS_BADMETHOD;
        }
        fit.name     = dobject->name;
/*@-immediatetrans@*/
        fit.aimFPTR  = &problem->aimFPTR;
/*@+immediatetrans@*/
        fit.npts     = *npts;
        fit.nrank    = *rank;
        fit.sindx    = index;
        fit.tindx    = aim_Index(problem->aimFPTR, analysis->loadName);
        fit.afact    = 1.e6;
        fit.area_src = 0.0;
        fit.area_tgt = 0.0;
        fit.src      = founddsr;
        fit.prms_src = params;
        fit.data_src = otherset->data;
        fit.tgt      = discr;
        fit.prms_tgt = param;
        fit.data_tgt = values;
        fit.nmat     = 0;
        fit.mat      = NULL;
        stat = caps_Match(&fit, bound->dim);
        if (stat != CAPS_SUCCESS) {
          EG_free(values);
          *npts = *rank = 0;
          return stat;
        }
        stat = caps_Conserve(&fit, bobject->name, bound->dim);
        if (stat != CAPS_SUCCESS) {
          if (fit.mat != NULL) EG_free(fit.mat);
          EG_free(values);
          *npts = *rank = 0;
          return stat;
        }
        
        if (fit.mat != NULL) EG_free(fit.mat);
      }

      dataset->data = values;
      dataset->npts = *npts;

      if (dobject->last.sNum != 0) {
        if (dataset->history == NULL) {
          dataset->nHist   = 0;
          dataset->history = (capsOwn *) EG_alloc(sizeof(capsOwn));
          if (dataset->history == NULL) OK = 0;
        } else {
          tmp = (capsOwn *) EG_reall( dataset->history,
                                     (dataset->nHist+1)*sizeof(capsOwn));
          if (tmp == NULL) {
            OK = 0;
          } else {
            dataset->history = tmp;
          }
        }
        if ((OK == 1) && (dataset->history != NULL)) {
          dataset->history[dataset->nHist]       = dobject->last;
          dataset->history[dataset->nHist].pname = EG_strdup(dobject->last.pname);
          dataset->history[dataset->nHist].pID   = EG_strdup(dobject->last.pID);
          dataset->history[dataset->nHist].user  = EG_strdup(dobject->last.user);
          dataset->nHist += 1;
        }
      }
      caps_freeOwner(&dobject->last);
      problem->sNum     += 1;
      dobject->last.sNum = problem->sNum;
      caps_fillDateTime(dobject->last.datetime);
    }
#ifdef VSOUTPUT
    snprintf(fileName, 20, "Source%d.vs", fileCnt);
    caps_outputVertexSet(ovs,     fileName);
    snprintf(fileName, 20, "Destin%d.vs", fileCnt);
    caps_outputVertexSet(vobject, fileName);
    fileCnt++;
#endif
  }

  *rank  = dataset->rank;
  *npts  = dataset->npts;
  *data  = dataset->data;
  *units = dataset->units;
  
  return CAPS_SUCCESS;
}


int
caps_getHistory(const capsObject *dobject, capsObject **vobject, int *nHist,
                capsOwn **history)
{
  capsDataSet *dataset;
  
  *vobject = NULL;
  *nHist   = 0;
  *history = NULL;
  if (dobject              == NULL)      return CAPS_NULLOBJ;
  if (dobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (dobject->type        != DATASET)   return CAPS_BADTYPE;
  if (dobject->blind       == NULL)      return CAPS_NULLBLIND;
  dataset = (capsDataSet *) dobject->blind;

  *vobject = dobject->parent;
  *nHist   = dataset->nHist;
  *history = dataset->history;
  
  return CAPS_SUCCESS;
}


int
caps_getDataSets(const capsObject *bobject, const char *dname, int *nobj,
                 capsObject ***dobjs)
{
  int           i, j, n;
  capsBound     *bound;
  capsVertexSet *vertexset;
  capsObject    **objs;

  *nobj  = 0;
  *dobjs = NULL;
  if (dname                == NULL)      return CAPS_NULLNAME;
  if (bobject              == NULL)      return CAPS_NULLOBJ;
  if (bobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (bobject->type        != BOUND)     return CAPS_BADTYPE;
  if (bobject->blind       == NULL)      return CAPS_NULLBLIND;
  bound = (capsBound *) bobject->blind;

  for (n = i = 0; i < bound->nVertexSet; i++) {
    if (bound->vertexSet[i]              == NULL)      return CAPS_NULLOBJ;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
    if (bound->vertexSet[i]->type        != VERTEXSET) return CAPS_BADTYPE;
    if (bound->vertexSet[i]->blind       == NULL)      return CAPS_NULLBLIND;
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    for (j = 0; j < vertexset->nDataSets; j++)
      if (strcmp(dname, vertexset->dataSets[i]->name) == 0) n++;
  }
  
  if (n == 0) return CAPS_SUCCESS;
  objs = (capsObject **) EG_alloc(n*sizeof(capsObject *));
  if (objs == NULL) return EGADS_MALLOC;
  
  for (n = i = 0; i < bound->nVertexSet; i++) {
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    for (j = 0; j < vertexset->nDataSets; j++)
      if (strcmp(dname, vertexset->dataSets[j]->name) == 0) {
        objs[n] = vertexset->dataSets[j];
        n++;
      }
  }
  
  *nobj  = n;
  *dobjs = objs;
  return CAPS_SUCCESS;
}


int
caps_snDataSets(const capsObject *aobject, int flag, CAPSLONG *sn)
/* flag = 0 - return lowest sn of dependant destination
   flag = 1 - return lowest sn of dependant source */
{
  int           i, j, k, jj, kk;
  capsObject    *pobject;
  capsProblem   *problem;
  capsBound     *bound;
  capsVertexSet *vs, *vso;
  capsDataSet   *ds, *dso;
  
  *sn = 0;
  if (aobject              == NULL)      return CAPS_NULLOBJ;
  if (aobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aobject->type        != ANALYSIS)  return CAPS_BADTYPE;
  if (aobject->parent      == NULL)      return CAPS_NULLOBJ;
  pobject = (capsObject *)  aobject->parent;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;
  *sn     = -1;
  
  for (i = 0; i < problem->nBound; i++) {
    if (problem->bounds[i]                 == NULL)      continue;
    if (problem->bounds[i]->magicnumber    != CAPSMAGIC) continue;
    if (problem->bounds[i]->type           != BOUND)     continue;
    if (problem->bounds[i]->blind          == NULL)      continue;
    bound = (capsBound *) problem->bounds[i]->blind;
    for (j = 0; j < bound->nVertexSet; j++) {
      if (bound->vertexSet[j]              == NULL)      continue;
      if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[j]->type        != VERTEXSET) continue;
      if (bound->vertexSet[j]->blind       == NULL)      continue;
      vs = (capsVertexSet *) bound->vertexSet[j]->blind;
      if (vs->analysis                     != aobject)   continue;
      for (k = 0; k < vs->nDataSets; k++) {
        if (vs->dataSets[k]                == NULL)      continue;
        if (vs->dataSets[k]->magicnumber   != CAPSMAGIC) continue;
        if (vs->dataSets[k]->type          != DATASET)   continue;
        if (vs->dataSets[k]->blind         == NULL)      continue;
        ds = (capsDataSet *) vs->dataSets[k]->blind;
        if ((ds->method != Interpolate) &&
            (ds->method != Conserve))                    continue;
        if (ds->dflag == 0)                              continue;
        if (flag == 0) {
          /* found destination */
          if (*sn == -1) {
            *sn = vs->dataSets[k]->last.sNum;
          } else {
            if (*sn > vs->dataSets[k]->last.sNum)
              *sn = vs->dataSets[k]->last.sNum;
          }
          continue;
        }
        for (jj = 0; jj < bound->nVertexSet; jj++) {
          if (j == jj) continue;
          if (bound->vertexSet[jj]              == NULL)      continue;
          if (bound->vertexSet[jj]->magicnumber != CAPSMAGIC) continue;
          if (bound->vertexSet[jj]->type        != VERTEXSET) continue;
          if (bound->vertexSet[jj]->blind       == NULL)      continue;
          vso = (capsVertexSet *) bound->vertexSet[jj]->blind;
          for (kk = 0; kk < vso->nDataSets; kk++) {
            if (vso->dataSets[kk]               == NULL)      continue;
            if (vso->dataSets[kk]->magicnumber  != CAPSMAGIC) continue;
            if (vso->dataSets[kk]->type         != DATASET)   continue;
            if (vso->dataSets[kk]->blind        == NULL)      continue;
            dso = (capsDataSet *) vso->dataSets[kk]->blind;
            if (dso->method                     != Analysis)  continue;
            if (strcmp(vs->dataSets[k]->name,
                       vso->dataSets[kk]->name) != 0)         continue;
            /* found source */
            if (*sn == -1) {
              *sn = vso->dataSets[kk]->last.sNum;
            } else {
              if (*sn > vso->dataSets[kk]->last.sNum)
                *sn = vso->dataSets[kk]->last.sNum;
            }
          }
        }
      }
    }
  }

  if (*sn == -1) {
    *sn = 0;
    return CAPS_NOTFOUND;
  }
  return CAPS_SUCCESS;
}
