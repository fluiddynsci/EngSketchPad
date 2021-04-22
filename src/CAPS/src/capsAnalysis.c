/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Analysis Object Functions
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

#ifdef WIN32
#define snprintf _snprintf
#define strtok_r strtok_s
#endif

#include "udunits.h"

#include "capsBase.h"
#include "capsAIM.h"
#include "prm.h"

/* OpenCSM Defines & Includes */
#include "common.h"
#include "OpenCSM.h"

#include "egadsTris.h"

#define NOTFILLED        -1
#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


typedef struct {
  ego geom;                     /* geometry object */
  int gIndex;                   /* geometry object index */
  int n;                        /* number of objects */
  ego *objs;                    /* objects */
  int *indices;                 /* markers */
} bodyObjs;


/*@-incondefs@*/
extern void ut_free(/*@only@*/ ut_unit* const unit);
/*@+incondefs@*/

extern /*@null@*/ /*@only@*/
       char *EG_strdup(/*@null@*/ const char *str);
extern void  EG_makeConnect(int k1, int k2, int *tri, int *kedge, int *ntable,
                            connect *etable, int face);

extern int  caps_dupValues(capsValue *val1, capsValue *val2);
extern int  caps_transferValues(const capsObject *source, enum capstMethod meth,
                                capsObject *trgt, int *nErr, capsErrs **errors);
extern int  caps_snDataSets(const capsObject *aobject, int flag, CAPSLONG *sn);
extern int  caps_fillCoeff2D(int nrank, int nux, int nvx, double *fit,
                             double *coeff, double *r);
extern int  caps_Aprx2DFree(/*@only@*/ capsAprx2D *approx);
extern int  caps_invInterpolate1D(capsAprx1D *interp, double *sv, double *t);
extern int  caps_invInterpolate2D(capsAprx2D *interp, double *sv, double *uv);
extern int  caps_size(const capsObject *object, enum capsoType type,
                      enum capssType stype, int *size);


static int
caps_checkAnalysis(capsProblem *problem, int nObject, capsObject **objects)
{
  int       i, j, k, m, n, len1, len2, *level;
  char      *name;
  capsValue *values;
  ut_unit   *utunit;

  values = (capsValue *) objects[0]->blind;
  
  /* check units */
  for (i = 0; i < nObject; i++) {
    if (values[i].units == NULL) continue;
    utunit = ut_parse((ut_system *) problem->utsystem, values[i].units,
                      UT_ASCII);
    if (utunit == NULL) return CAPS_UNITERR;
    ut_free(utunit);
  }
  
  /* fixup hierarchical Values -- allocate and nullify */
  for (i = 0; i < nObject; i++) {
    if (values[i].type != Value) continue;
    if (values[i].length == 1) {
      values[i].vals.object = NULL;
    } else {
      if (values[i].vals.objects == NULL) {
        values[i].vals.objects = (capsObject **)
                                EG_alloc(values[i].length*sizeof(capsObject *));
        if (values[i].vals.objects == NULL) return EGADS_MALLOC;
      }
      for (j = 0; j < values[i].length; j++) values[i].vals.objects[j] = NULL;
    }
  }
  
  /* fill in the parents */
  for (i = 0; i < nObject; i++) {
    if (values[i].type   == Value) continue;
    if (values[i].pIndex == 0)     continue;
    k = values[i].pIndex - 1;
    if (values[k].type   != Value) return CAPS_HIERARCHERR;
    if (values[k].length == 1) {
      if (values[k].vals.object == NULL) {
        values[k].vals.object = objects[i];
        j = 0;
      } else {
        j = 1;
      }
    } else {
      for (j = 0; j < values[k].length; j++)
        if (values[k].vals.objects[j] == NULL) {
          values[k].vals.objects[j] = objects[i];
          break;
        }
    }
    if (j == values[k].length) return CAPS_HIERARCHERR;
  }
  
  /* set the length */
  for (i = 0; i < nObject; i++) {
    values[i].length = values[i].ncol*values[i].nrow;
    if (values[i].type == String) {
      if (values[i].vals.string == NULL) values[i].length = 0;
    } else {
      if (values[i].length <= 0) return CAPS_SHAPEERR;
    }
  }
  
  /* check for no fills */
  for (i = 0; i < nObject; i++) {
    if (values[i].type   != Value) continue;
    if (values[i].length == 1) {
      if (values[i].vals.object == NULL) return CAPS_HIERARCHERR;
    } else {
      if (values[i].vals.objects[values[i].length-1] == NULL)
        return CAPS_HIERARCHERR;
    }
  }
  
  /* look at shapes */
  for (i = 0; i < nObject; i++)
    if (values[i].dim == 0) {
      if (values[i].length > 1) return CAPS_SHAPEERR;
    } else if (values[i].dim == 1) {
      if ((values[i].ncol != 1) && (values[i].nrow != 1)) return CAPS_SHAPEERR;
    } else if (values[i].dim != 2) {
      return CAPS_BADINDEX;
    }
  
  /* fixup hierarchical object names */
  level = (int *) EG_alloc(nObject*sizeof(int));
  if (level == NULL) return EGADS_MALLOC;
  for (i = 0; i < nObject; i++) level[i] = 0;
  for (k = i = 0; i < nObject; i++) {
    if (values[i].type != Value) continue;
    if (values[i].pIndex == 0) level[i] = 1;
  }
  do {
    k++;
    for (j = i = 0; i < nObject; i++) {
      if (values[i].pIndex          == 0) continue;
      if (level[values[i].pIndex-1] != k) continue;
      level[i] = k+1;
      j++;
    }
  } while (j != 0);
  for (j = 2; j < k; j++)
    for (i = 0; i < nObject; i++) {
      if (values[i].pIndex == 0) continue;
      if (level[i]         != j) continue;
      len1 = strlen(objects[values[i].pIndex-1]->name);
      len2 = strlen(objects[i]->name);
      name = (char *) EG_alloc((len1+len2+2)*sizeof(char));
      if (name == NULL) {
        EG_free(level);
        return EGADS_MALLOC;
      }
      for (m = 0; m < len1; m++) name[m] = objects[values[i].pIndex-1]->name[m];
      name[m] = ':';
      m++;
      for (n = 0; n <= len2; n++, m++) name[m] = objects[i]->name[n];
      EG_free(objects[i]->name);
      objects[i]->name = name;
    }
  
  EG_free(level);

  return CAPS_SUCCESS;
}


int
caps_queryAnalysis(capsObject *pobject, const char *aname, int *nIn, int *nOut,
                   int *execute)
{
  int          nField, *ranks;
  char         **fields;
  capsProblem  *problem;

  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  if (aname                == NULL)      return CAPS_NULLNAME;
  problem = (capsProblem *) pobject->blind;
  
  /* try to load the AIM and get the info */
  *execute = 1;
  return aim_Initialize(&problem->aimFPTR, aname, 0, NULL, execute, NULL,
                        nIn, nOut, &nField, &fields, &ranks);
}


int
caps_getBodies(const capsObject *aobject, int *nBody, ego **bodies)
{
  int          stat, n;
  capsAnalysis *analysis;
  
  *nBody  = 0;
  *bodies = NULL;
  if (aobject              == NULL)      return CAPS_NULLOBJ;
  if (aobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aobject->type        != ANALYSIS)  return CAPS_BADTYPE;
  if (aobject->blind       == NULL)      return CAPS_NULLBLIND;
  
  stat = caps_size(aobject, BODIES, NONE, &n);
  if (stat != CAPS_SUCCESS) return stat;

  analysis = (capsAnalysis *) aobject->blind;
  *nBody   = analysis->nBody;
  *bodies  = analysis->bodies;
  return CAPS_SUCCESS;
}


int
caps_getInput(capsObject *pobject, const char *aname, int index, char **ainame,
              capsValue *defaults)
{
  int         stat;
  capsProblem *problem;
  
  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  if (aname                == NULL)      return CAPS_NULLNAME;
  problem = (capsProblem *) pobject->blind;
  
  defaults->length          = defaults->nrow = defaults->ncol = 1;
  defaults->type            = Integer;
  defaults->dim             = defaults->pIndex = 0;
  defaults->lfixed          = defaults->sfixed = Fixed;
  defaults->nullVal         = NotAllowed;
  defaults->units           = NULL;
  defaults->link            = NULL;
  defaults->vals.integer    = 0;
  defaults->limits.dlims[0] = defaults->limits.dlims[1] = 0.0;
  defaults->linkMethod      = Copy;

  stat = aim_Inputs(problem->aimFPTR, aname, -1, NULL, index, ainame, defaults);
  if (stat == CAPS_SUCCESS) defaults->length = defaults->ncol*defaults->nrow;
  return stat;
}


int
caps_getOutput(capsObject *pobject, const char *aname, int index, char **aoname,
               capsValue *form)
{
  int         stat;
  capsProblem *problem;
  
  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  if (aname                == NULL)      return CAPS_NULLNAME;
  problem = (capsProblem *) pobject->blind;
  
  form->length          = form->nrow = form->ncol = 1;
  form->type            = Integer;
  form->dim             = form->pIndex = 0;
  form->lfixed          = form->sfixed = Fixed;
  form->nullVal         = NotAllowed;
  form->units           = NULL;
  form->link            = NULL;
  form->vals.integer    = 0;
  form->limits.dlims[0] = form->limits.dlims[1] = 0.0;
  form->linkMethod      = Copy;

  stat = aim_Outputs(problem->aimFPTR, aname, -1, NULL, index, aoname, form);
  if (stat == CAPS_SUCCESS) form->length = form->ncol*form->nrow;
  return stat;
}


int
caps_AIMbackdoor(const capsObject *aobject, const char *JSONin, char **JSONout)
{
  capsAnalysis *analysis;
  capsObject   *pobject;
  capsProblem  *problem;
  
  if (aobject              == NULL)      return CAPS_NULLOBJ;
  if (aobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aobject->type        != ANALYSIS)  return CAPS_BADTYPE;
  if (aobject->blind       == NULL)      return CAPS_NULLBLIND;
  analysis = (capsAnalysis *) aobject->blind;
  if (aobject->parent      == NULL)      return CAPS_NULLOBJ;
  pobject  = (capsObject *)   aobject->parent;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  problem  = (capsProblem *)  pobject->blind;

  return aim_Backdoor(problem->aimFPTR, analysis->loadName, analysis->instance,
                      &analysis->info, JSONin, JSONout);
}


int
caps_load(capsObject *pobject, const char *aname, const char *apath,
          /*@null@*/ const char *unitSys, /*@null@*/ const char *intents,
          int nparent, /*@null@*/ capsObject **parents, capsObject **aobject)
{
  int          i, j, status, len, nIn, nOut, *ranks, instance, nField, eFlag;
  char         **fields, *oname;
  capsObject   *object, **tmp;
  capsProblem  *problem;
  capsAnalysis *analysis;
  capsValue    *value, *geomIn = NULL;
 
  *aobject = NULL;
  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  if (aname                == NULL)      return CAPS_NULLNAME;
  if (apath                == NULL)      return CAPS_NULLNAME;
  problem = (capsProblem *) pobject->blind;
  if (problem->nGeomIn > 0) {
    object = problem->geomIn[0];
    geomIn = (capsValue *) object->blind;
  }
  
  /* are our parents the correct objects? */
  if (parents != NULL)
    for (i = 0; i < nparent; i++) {
      if (parents[i]              == NULL)      return CAPS_NULLOBJ;
      if (parents[i]->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
      if (parents[i]->type        != ANALYSIS)  return CAPS_BADTYPE;
      if (parents[i]->blind       == NULL)      return CAPS_NULLBLIND;
    }
  
  /* is the name unique? */
  len = strlen(aname) + strlen(apath) + 2;
  oname = (char *) EG_alloc(len*sizeof(char));
  if (oname == NULL) return EGADS_MALLOC;
  i = strlen(aname);
  for (j = 0; j < i; j++) oname[j] = aname[j];
  oname[i] = ':';
  i++;
  for (j = 0; j <= strlen(apath); j++, i++) oname[i] = apath[j];
  for (i = 0; i < problem->nAnalysis; i++) {
    if (problem->analysis[i]       == NULL) continue;
    if (problem->analysis[i]->name == NULL) continue;
    if (strcmp(oname, problem->analysis[i]->name) == 0) {
      EG_free(oname);
      return CAPS_BADNAME;
    }
  }
  EG_free(oname);

  /* try to load the AIM */
  eFlag    = 0;
  nField   = 0;
  fields   = NULL;
  ranks    = NULL;
  instance = aim_Initialize(&problem->aimFPTR, aname, problem->nGeomIn, geomIn,
                            &eFlag, unitSys, &nIn, &nOut, &nField, &fields,
                            &ranks);
  if (instance <  CAPS_SUCCESS) return instance;
  if (nIn      <= 0) {
    if (fields != NULL) {
      for (i = 0; i < nField; i++) EG_free(fields[i]);
      EG_free(fields);
    }
    EG_free(ranks);
    return CAPS_BADINIT;
  }

  /* initialize the analysis structure */
  analysis = (capsAnalysis *) EG_alloc(sizeof(capsAnalysis));
  if (analysis == NULL) {
    if (fields != NULL) {
      for (i = 0; i < nField; i++) EG_free(fields[i]);
      EG_free(fields);
    }
    if (ranks != NULL) EG_free(ranks);
    return EGADS_MALLOC;
  }

  analysis->loadName         = EG_strdup(aname);
  analysis->path             = EG_strdup(apath);
  analysis->unitSys          = NULL;
  analysis->instance         = instance;
  analysis->eFlag            = eFlag;
  analysis->intents          = EG_strdup(intents);
  analysis->info.magicnumber = CAPSMAGIC;
  analysis->info.problem     = problem;
  analysis->info.analysis    = analysis;
  analysis->info.pIndex      = 0;
  analysis->info.irow        = 0;
  analysis->info.icol        = 0;
  analysis->nField           = nField;
  analysis->fields           = fields;
  analysis->ranks            = ranks;
  analysis->nAnalysisIn      = nIn;
  analysis->analysisIn       = NULL;
  analysis->nAnalysisOut     = nOut;
  analysis->analysisOut      = NULL;
  analysis->nParent          = 0;
  analysis->parents          = NULL;
  analysis->nBody            = 0;
  analysis->bodies           = NULL;
  analysis->pre.pname        = NULL;
  analysis->pre.pID          = NULL;
  analysis->pre.user         = NULL;
  analysis->pre.sNum         = 0;
  for (i = 0; i < 6; i++) analysis->pre.datetime[i] = 0;
  if ((nparent > 0) && (parents != NULL)) {
    analysis->nParent = nparent;
    analysis->parents = (capsObject **) EG_alloc(nparent*sizeof(capsObject *));
    if (analysis->parents == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < nparent; i++) analysis->parents[i] = parents[i];
  }
  
  /* allocate the objects for input */
  if (nIn != 0) {
    analysis->analysisIn = (capsObject **) EG_alloc(nIn*sizeof(capsObject *));
    if (analysis->analysisIn == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < nIn; i++) analysis->analysisIn[i] = NULL;
    value = (capsValue *) EG_alloc(nIn*sizeof(capsValue));
    if (value == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    problem->sNum += 1;
    for (i = 0; i < nIn; i++) {
      value[i].length          = value[i].nrow = value[i].ncol = 1;
      value[i].type            = Integer;
      value[i].dim             = value[i].pIndex = 0;
      value[i].lfixed          = value[i].sfixed = Fixed;
      value[i].nullVal         = NotAllowed;
      value[i].units           = NULL;
      value[i].link            = NULL;
      value[i].vals.integer    = 0;
      value[i].limits.dlims[0] = value[i].limits.dlims[1] = 0.0;
      value[i].linkMethod      = Copy;

      status = caps_makeObject(&object);
      if (status != CAPS_SUCCESS) {
        EG_free(value);
        caps_freeAnalysis(0, analysis);
        return EGADS_MALLOC;
      }
      if (i == 0) object->blind = value;
      object->parent    = NULL;
      object->name      = NULL;
      object->type      = VALUE;
      object->subtype   = ANALYSISIN;
      object->last.sNum = problem->sNum;
/*@-immediatetrans@*/
      object->blind     = &value[i];
/*@+immediatetrans@*/
      analysis->analysisIn[i] = object;
    }

    for (i = 0; i < nIn; i++) {
      status = aim_Inputs(problem->aimFPTR, aname, analysis->instance,
                          &analysis->info, i+1, &analysis->analysisIn[i]->name,
                          &value[i]);
      if (status != CAPS_SUCCESS) {
        caps_freeAnalysis(0, analysis);
        return status;
      }
    }
    
    status = caps_checkAnalysis(problem, nIn, analysis->analysisIn);
    if (status != CAPS_SUCCESS) {
      caps_freeAnalysis(0, analysis);
      printf(" CAPS Info: checkAnalysis returns %d\n", status);
      return status;
    }
  }

  /* allocate the objects for output */
  if (nOut != 0) {
    analysis->analysisOut = (capsObject **) EG_alloc(nOut*sizeof(capsObject *));
    if (analysis->analysisOut == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < nOut; i++) analysis->analysisOut[i] = NULL;
    value = (capsValue *) EG_alloc(nOut*sizeof(capsValue));
    if (value == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    problem->sNum += 1;
    for (i = 0; i < nOut; i++) {
      value[i].length          = value[i].nrow = value[i].ncol = 1;
      value[i].type            = Integer;
      value[i].dim             = value[i].pIndex = 0;
      value[i].lfixed          = value[i].sfixed = Fixed;
      value[i].nullVal         = NotAllowed;
      value[i].units           = NULL;
      value[i].link            = NULL;
      value[i].vals.integer    = 0;
      value[i].limits.dlims[0] = value[i].limits.dlims[1] = 0.0;
      value[i].linkMethod      = Copy;

      status = caps_makeObject(&object);
      if (status != CAPS_SUCCESS) {
        EG_free(value);
        caps_freeAnalysis(0, analysis);
        return EGADS_MALLOC;
      }
      if (i == 0) object->blind = value;
      object->parent    = NULL;
      object->name      = NULL;
      object->type      = VALUE;
      object->subtype   = ANALYSISOUT;
      object->last.sNum = problem->sNum;
/*@-immediatetrans@*/
      object->blind     = &value[i];
/*@+immediatetrans@*/
      analysis->analysisOut[i] = object;
    }
    
    for (i = 0; i < nOut; i++) {
      status = aim_Outputs(problem->aimFPTR, aname, analysis->instance,
                           &analysis->info, i+1,
                           &analysis->analysisOut[i]->name, &value[i]);
      if (status != CAPS_SUCCESS) {
        caps_freeAnalysis(0, analysis);
        return status;
      }
    }
    
    status = caps_checkAnalysis(problem, nOut, analysis->analysisOut);
    if (status != CAPS_SUCCESS) {
      caps_freeAnalysis(0, analysis);
      return status;
    }
  }
  
  /* get a place in the problem to store the data away */
  if (problem->analysis == NULL) {
    problem->analysis = (capsObject  **) EG_alloc(sizeof(capsObject *));
    if (problem->analysis == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
  } else {
    tmp = (capsObject  **) EG_reall( problem->analysis,
                                    (problem->nAnalysis+1)*sizeof(capsObject *));
    if (tmp == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    problem->analysis = tmp;
  }
  
  /* object name is concat of name & path */
  len = strlen(aname) + strlen(apath) + 2;
  oname = (char *) EG_alloc(len*sizeof(char));
  if (oname == NULL) {
    caps_freeAnalysis(0, analysis);
    return EGADS_MALLOC;
  }
  i = strlen(aname);
  for (j = 0; j < i; j++) oname[j] = aname[j];
  oname[i] = ':';
  i++;
  for (j = 0; j <= strlen(apath); j++, i++) oname[i] = apath[j];
  
  /* get the analysis object */
  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    EG_free(oname);
    caps_freeAnalysis(0, analysis);
    return status;
  }
  /* leave sNum 0 to flag we are unexecuted */
  object->parent = pobject;
  object->name   = oname;
  object->type   = ANALYSIS;
  object->blind  = analysis;
/*@-nullderef@*/
/*@-kepttrans@*/
  for (i = 0; i < nIn;  i++) analysis->analysisIn[i]->parent  = object;
  for (i = 0; i < nOut; i++) analysis->analysisOut[i]->parent = object;
/*@+kepttrans@*/
/*@+nullderef@*/
  *aobject = object;
  
  problem->analysis[problem->nAnalysis] = object;
  problem->nAnalysis += 1;

  return CAPS_SUCCESS;
}


int
caps_dupAnalysis(capsObject *from, const char *apath, int nparent,
                 /*@null@*/ capsObject **parents,capsObject **aobject)
{
  int          i, j, status, len, nIn, nOut, *ranks, instance, eFlag, nField;
  char         **fields, *oname;
  capsObject   *pobject, *object, **tmp;
  capsAnalysis *froma, *analysis;
  capsProblem  *problem;
  capsValue    *value, *src, *geomIn = NULL;
  
  *aobject = NULL;
  if (from              == NULL)      return CAPS_NULLOBJ;
  if (from->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (from->type        != ANALYSIS)  return CAPS_BADTYPE;
  if (from->blind       == NULL)      return CAPS_NULLBLIND;
  froma   = (capsAnalysis *) from->blind;
  if (apath             == NULL)      return CAPS_NULLNAME;
  if (from->parent      == NULL)      return CAPS_NULLOBJ;
  pobject = (capsObject *)  from->parent;
  if (pobject->blind    == NULL)      return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;
  if (problem->nGeomIn > 0) {
    object = problem->geomIn[0];
    geomIn = (capsValue *) object->blind;
  }
  
  /* is the name unique? */
  len = strlen(froma->loadName) + strlen(apath) + 2;
  oname = (char *) EG_alloc(len*sizeof(char));
  if (oname == NULL) return EGADS_MALLOC;
  i = strlen(froma->loadName);
  for (j = 0; j < i; j++) oname[j] = froma->loadName[j];
  oname[i] = ':';
  i++;
  for (j = 0; j <= strlen(apath); j++, i++) oname[i] = apath[j];
  for (i = 0; i < problem->nAnalysis; i++) {
    if (problem->analysis[i]       == NULL) continue;
    if (problem->analysis[i]->name == NULL) continue;
    if (strcmp(oname, problem->analysis[i]->name) == 0) {
      EG_free(oname);
      return CAPS_BADNAME;
    }
  }
  EG_free(oname);
  
  /* get a new instance AIM */
  eFlag    = 0;
  nField   = 0;
  fields   = NULL;
  ranks    = NULL;
  instance = aim_Initialize(&problem->aimFPTR, froma->loadName, problem->nGeomIn,
                            geomIn, &eFlag, froma->unitSys, &nIn, &nOut,
                            &nField, &fields, &ranks);
  if (instance <  CAPS_SUCCESS) return instance;
  if (nIn      <= 0) {
    if (fields != NULL) {
      for (i = 0; i < nField; i++) EG_free(fields[i]);
      EG_free(fields);
    }
    EG_free(ranks);
    return CAPS_BADINIT;
  }
  
  /* initialize the analysis structure */
  analysis = (capsAnalysis *) EG_alloc(sizeof(capsAnalysis));
  if (analysis == NULL) {
    if (fields != NULL) {
      for (i = 0; i < nField; i++) EG_free(fields[i]);
      EG_free(fields);
    }
    if (ranks != NULL) EG_free(ranks);
    return EGADS_MALLOC;
  }
  
  analysis->loadName         = EG_strdup(froma->loadName);
  analysis->path             = EG_strdup(apath);
  analysis->unitSys          = EG_strdup(froma->unitSys);
  analysis->instance         = instance;
  analysis->eFlag            = eFlag;
  analysis->intents          = EG_strdup(froma->intents);
  analysis->info.magicnumber = CAPSMAGIC;
  analysis->info.problem     = problem;
  analysis->info.analysis    = analysis;
  analysis->info.pIndex      = 0;
  analysis->info.irow        = 0;
  analysis->info.icol        = 0;
  analysis->nField           = nField;
  analysis->fields           = fields;
  analysis->ranks            = ranks;
  analysis->nAnalysisIn      = nIn;
  analysis->analysisIn       = NULL;
  analysis->nAnalysisOut     = nOut;
  analysis->analysisOut      = NULL;
  analysis->nParent          = 0;
  analysis->parents          = NULL;
  analysis->nBody            = 0;
  analysis->bodies           = NULL;
  analysis->pre.pname        = NULL;
  analysis->pre.pID          = NULL;
  analysis->pre.user         = NULL;
  analysis->pre.sNum         = 0;
  for (i = 0; i < 6; i++) analysis->pre.datetime[i] = 0;
  if ((nparent > 0) && (parents != NULL)) {
    analysis->nParent = nparent;
    analysis->parents = (capsObject **) EG_alloc(nparent*sizeof(capsObject *));
    if (analysis->parents == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < nparent; i++) analysis->parents[i] = parents[i];
  }
  
  /* allocate the objects for input */
  if (nIn != 0) {
    analysis->analysisIn = (capsObject **) EG_alloc(nIn*sizeof(capsObject *));
    if (analysis->analysisIn == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < nIn; i++) analysis->analysisIn[i] = NULL;
    value = (capsValue *) EG_alloc(nIn*sizeof(capsValue));
    if (value == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    problem->sNum += 1;
    for (i = 0; i < nIn; i++) {
      value[i].length          = value[i].nrow = value[i].ncol = 1;
      value[i].type            = Integer;
      value[i].dim             = value[i].pIndex = 0;
      value[i].lfixed          = value[i].sfixed = Fixed;
      value[i].nullVal         = NotAllowed;
      value[i].units           = NULL;
      value[i].link            = NULL;
      value[i].vals.integer    = 0;
      value[i].limits.dlims[0] = value[i].limits.dlims[1] = 0.0;
      value[i].linkMethod      = Copy;
      
      status = caps_makeObject(&object);
      if (status != CAPS_SUCCESS) {
        EG_free(value);
        caps_freeAnalysis(0, analysis);
        return EGADS_MALLOC;
      }
      if (i == 0) object->blind = value;
      object->parent    = NULL;
      object->name      = NULL;
      object->type      = VALUE;
      object->subtype   = ANALYSISIN;
      object->last.sNum = problem->sNum;
/*@-immediatetrans@*/
      object->blind     = &value[i];
/*@+immediatetrans@*/
      analysis->analysisIn[i] = object;
    }
    
    src = (capsValue *) froma->analysisIn[0]->blind;
    if ((nIn != 0) && (src == NULL)) {
      caps_freeAnalysis(0, analysis);
      return CAPS_NULLBLIND;
    }
    for (i = 0; i < nIn; i++) {
      status = caps_dupValues(&src[i], &value[i]);
      if (status != CAPS_SUCCESS) {
        caps_freeAnalysis(0, analysis);
        return status;
      }
    }
  }

  /* allocate the objects for output */
  if (nOut != 0) {
    analysis->analysisOut = (capsObject **) EG_alloc(nOut*sizeof(capsObject *));
    if (analysis->analysisOut == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < nOut; i++) analysis->analysisOut[i] = NULL;
    value = (capsValue *) EG_alloc(nOut*sizeof(capsValue));
    if (value == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    problem->sNum += 1;
    for (i = 0; i < nOut; i++) {
      value[i].length          = value[i].nrow = value[i].ncol = 1;
      value[i].type            = Integer;
      value[i].dim             = value[i].pIndex = 0;
      value[i].lfixed          = value[i].sfixed = Fixed;
      value[i].nullVal         = NotAllowed;
      value[i].units           = NULL;
      value[i].link            = NULL;
      value[i].vals.integer    = 0;
      value[i].limits.dlims[0] = value[i].limits.dlims[1] = 0.0;
      value[i].linkMethod      = Copy;
      
      status = caps_makeObject(&object);
      if (status != CAPS_SUCCESS) {
        EG_free(value);
        caps_freeAnalysis(0, analysis);
        return EGADS_MALLOC;
      }
      if (i == 0) object->blind = value;
      object->parent    = NULL;
      object->name      = NULL;
      object->type      = VALUE;
      object->subtype   = ANALYSISOUT;
      object->last.sNum = problem->sNum;
/*@-immediatetrans@*/
      object->blind     = &value[i];
/*@+immediatetrans@*/
      analysis->analysisOut[i] = object;
    }
    
    for (i = 0; i < nOut; i++) {
      status = aim_Outputs(problem->aimFPTR, froma->loadName,
                           analysis->instance, &analysis->info, i+1,
                           &analysis->analysisOut[i]->name, &value[i]);
      if (status != CAPS_SUCCESS) {
        caps_freeAnalysis(0, analysis);
        return status;
      }
    }
    
    status = caps_checkAnalysis(problem, nOut, analysis->analysisOut);
    if (status != CAPS_SUCCESS) {
      caps_freeAnalysis(0, analysis);
      return status;
    }
  }

  /* get a place in the problem to store the data away */
  if (problem->analysis == NULL) {
    problem->analysis = (capsObject  **) EG_alloc(sizeof(capsObject *));
    if (problem->analysis == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
  } else {
    tmp = (capsObject  **) EG_reall( problem->analysis,
                                    (problem->nAnalysis+1)*sizeof(capsObject *));
    if (tmp == NULL) {
      caps_freeAnalysis(0, analysis);
      return EGADS_MALLOC;
    }
    problem->analysis = tmp;
  }
  
  /* object name is concat of name & path */
  len = strlen(froma->loadName) + strlen(apath) + 2;
  oname = (char *) EG_alloc(len*sizeof(char));
  if (oname == NULL) {
    caps_freeAnalysis(0, analysis);
    return EGADS_MALLOC;
  }
  i = strlen(froma->loadName);
  for (j = 0; j < i; j++) oname[j] = froma->loadName[j];
  oname[i] = ':';
  i++;
  for (j = 0; j <= strlen(apath); j++, i++) oname[i] = apath[j];
  
  /* get the analysis object */
  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    EG_free(oname);
    caps_freeAnalysis(0, analysis);
    return status;
  }
  /* leave sNum 0 to flag we are unexecuted */
  object->parent = pobject;
  object->name   = oname;
  object->type   = ANALYSIS;
  object->blind  = analysis;
/*@-nullderef@*/
/*@-kepttrans@*/
  for (i = 0; i < nIn;  i++) analysis->analysisIn[i]->parent  = object;
  for (i = 0; i < nOut; i++) analysis->analysisOut[i]->parent = object;
/*@+kepttrans@*/
/*@+nullderef@*/
  
  *aobject = object;
  
  problem->analysis[problem->nAnalysis] = object;
  problem->nAnalysis += 1;
  
  return CAPS_SUCCESS;
}


int
caps_analysisInfo(const capsObject *aobject, char **apath, char **unitSys,
                  char **intents, int *nparent, capsObject ***parents,
                  int *nField, char ***fnames, int **ranks, int *execute,
                  int *status)
{
  int          i, stat, gstatus;
  CAPSLONG     sn;
  capsAnalysis *analysis;
  capsObject   *pobject, *source, *object, *last;
  capsValue    *value;
  capsProblem  *problem;
  
  *nField  = *status = *nparent = 0;
  *apath   = NULL;
  *unitSys = NULL;
  *parents = NULL;
  *fnames  = NULL;
  *ranks   = NULL;
  if (aobject              == NULL)      return CAPS_NULLOBJ;
  if (aobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aobject->type        != ANALYSIS)  return CAPS_BADTYPE;
  if (aobject->blind       == NULL)      return CAPS_NULLBLIND;
  analysis = (capsAnalysis *) aobject->blind;
  if (aobject->parent      == NULL)      return CAPS_NULLOBJ;
  pobject  = (capsObject *)   aobject->parent;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  problem  = (capsProblem *)  pobject->blind;
  
  *apath   = analysis->path;
  *unitSys = analysis->unitSys;
  *execute = analysis->eFlag;
  *intents = analysis->intents;
  *nparent = analysis->nParent;
  *parents = analysis->parents;
  *nField  = analysis->nField;
  *fnames  = analysis->fields;
  *ranks   = analysis->ranks;

  /* are we "geometry" clean? */
  gstatus = 0;
  if (pobject->subtype == PARAMETRIC) {
    /* check for dirty geometry inputs */
    for (i = 0; i < problem->nGeomIn; i++) {
      source = object = problem->geomIn[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last->last.sNum > problem->geometry.sNum) {
        gstatus = 1;
        break;
      }
    }
    /* check for dirty branchs */
    for (i = 0; i < problem->nBranch; i++) {
      source = object = problem->branchs[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last->last.sNum > problem->geometry.sNum) {
        gstatus = 1;
        break;
      }
    }
  }
  
  /* are we "analysis" clean? */
  if (analysis->pre.sNum == 0) {
    *status = 1;
  } else {
    /* check for dirty inputs */
    for (i = 0; i < analysis->nAnalysisIn; i++) {
      source = object = analysis->analysisIn[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last->last.sNum > analysis->pre.sNum) {
        *status = 1;
        break;
      }
    }
    if (*status == 0) {
      stat = caps_snDataSets(aobject, 1, &sn);
      if (stat == CAPS_SUCCESS)
        if (sn > analysis->pre.sNum) *status = 1;
    }
  }
  *status += gstatus*2;
 
  /* is geometry new? */
  if (*status == 0)
    if (problem->geometry.sNum > analysis->pre.sNum) *status = 4;
  
  /* is post required? */
  if (*status == 0)
    if (analysis->pre.sNum > aobject->last.sNum) {
      *status = 5;
      if (analysis->eFlag == 0) *status = 6;
    }

  return CAPS_SUCCESS;
}


int
caps_checkDiscr(capsDiscr *discr, int l, char *line)
{
  int          i, j, len, typ, stat, bIndex, nEdge, nFace, last, state;
  int          nGlobal, npts, ntris;
  ego          body, *objs;
  aimInfo      *aInfo;
  capsAnalysis *analysis;
  const int    *ptype, *pindex, *tris, *tric;
  const double *xyz, *prms;
  
  aInfo    = (aimInfo *)      discr->aInfo;
  analysis = (capsAnalysis *) aInfo->analysis;

  if (discr->mapping == NULL) {
    snprintf(line, l, "caps_checkDiscr: mapping is NULL!\n");
    return CAPS_NULLBLIND;
  }
  if (discr->types == NULL) {
    snprintf(line, l, "caps_checkDiscr: types is NULL!\n");
    return CAPS_NULLBLIND;
  }
  if (discr->elems == NULL) {
    snprintf(line, l, "caps_checkDiscr: elems is NULL!\n");
    return CAPS_NULLBLIND;
  }
  
  /* do the element types */
  for (i = 0; i < discr->nTypes; i++) {
    if (discr->types[i].gst == NULL) {
      snprintf(line, l, "caps_checkDiscr: types[%d].gst = NULL!", i+1);
      return CAPS_NULLVALUE;
    }
    if ((discr->types[i].dst == NULL) && (discr->types[i].ndata != 0)) {
      snprintf(line, l, "caps_checkDiscr: types[%d].dst = NULL!", i+1);
      return CAPS_NULLVALUE;
    }
    if ((discr->types[i].matst == NULL) && (discr->types[i].nmat != 0)) {
      snprintf(line, l, "caps_checkDiscr: types[%d].matst = NULL!", i+1);
      return CAPS_NULLVALUE;
    }
    if (discr->dim == 2)
      if (discr->types[i].tris == NULL) {
        snprintf(line, l, "caps_checkDiscr: types[%d].tris = NULL!", i+1);
        return CAPS_NULLVALUE;
      }
    for (j = 0; j < 3*discr->types[i].ntri; j++)
      if ((discr->types[i].tris[j] < 1) ||
          (discr->types[i].tris[j] > discr->types[i].nref)) {
        snprintf(line, l,
                 "caps_checkDiscr: types[%d].tris[%d] = %d out of range [1-%d] ",
                 i+1, j+1, discr->types[i].tris[j], discr->types[i].nref);
        return CAPS_BADINDEX;
      }
  }
  
  /* look at body indices */
  for (i = 0; i < discr->nPoints; i++)
    if ((discr->mapping[2*i] < 1) || (discr->mapping[2*i] > analysis->nBody)) {
      snprintf(line, l, "caps_checkDiscr: body mapping %d = %d [1,%d]!\n",
               i+1, discr->mapping[2*i], analysis->nBody);
      return CAPS_BADINDEX;
    }
  for (i = 0; i < discr->nElems; i++)
    if ((discr->elems[i].bIndex < 1) ||
        (discr->elems[i].bIndex > analysis->nBody)) {
      snprintf(line, l, "caps_checkDiscr: body element %d = %d [1,%d]!\n",
               i+1, discr->elems[i].bIndex, analysis->nBody);
      return CAPS_BADINDEX;
    }
  
  /* check vert element indices */
  if (discr->nVerts != 0) {
    if (discr->verts == NULL) {
      snprintf(line, l, "caps_checkDiscr: nVert = %d but verts = NULL!",
               discr->nVerts);
      return CAPS_NULLVALUE;
    }
    if (discr->celem == NULL) {
      snprintf(line, l, "caps_checkDiscr: nVert = %d but celem = NULL!",
               discr->nVerts);
      return CAPS_NULLVALUE;
    }
    for (i = 0; i < discr->nVerts; i++)
      if ((discr->celem[i] < 1) || (discr->celem[i] > discr->nElems)) {
        snprintf(line, l,
                 "caps_checkDiscr: celem[%d] = %d out of range [1-%d] ",
                 i+1, discr->celem[i], discr->nElems);
        return CAPS_BADINDEX;
      }
  }
  
  /* look at the data associated with each body */
  for (bIndex = 1; bIndex <= analysis->nBody; bIndex++) {
    stat = EG_getBodyTopos(analysis->bodies[bIndex-1], NULL, FACE, &nFace,
                           &objs);
    if (stat != EGADS_SUCCESS) {
      snprintf(line, l, "caps_checkDiscr: getBodyTopos (Face) = %d for %d!\n",
               stat, bIndex);
      return stat;
    }
    EG_free(objs);
    stat = EG_getBodyTopos(analysis->bodies[bIndex-1], NULL, FACE, &nEdge,
                           &objs);
    if (stat != EGADS_SUCCESS) {
      snprintf(line, l, "caps_checkDiscr: getBodyTopos (Edge) = %d for %d!\n",
               stat, bIndex);
      return stat;
    }
    EG_free(objs);

    /* only check point mapping if tessellation exists on this body */
    if (analysis->bodies[bIndex-1+analysis->nBody] != NULL) {
      stat = EG_statusTessBody(analysis->bodies[bIndex-1+analysis->nBody], &body,
                               &state, &nGlobal);
      if (stat < EGADS_SUCCESS) {
        snprintf(line, l, "caps_checkDiscr: statusTessBody = %d for %d!\n",
                 stat, bIndex);
        return stat;
      }
      if (state == 0) {
        snprintf(line, l, "caps_checkDiscr: Tessellation is Open for %d!\n",
                 bIndex);
        return EGADS_TESSTATE;
      }

      /* check point mapping is withing the expected range for this body */
      for (i = 0; i < discr->nPoints; i++)
        if (discr->mapping[2*i] == bIndex)
          if ((discr->mapping[2*i+1] < 1) || (discr->mapping[2*i+1] > nGlobal)) {
            snprintf(line, l, "caps_checkDiscr: global mapping %d = %d [1,%d] for %d!\n",
                     i+1, discr->mapping[2*i+1], nGlobal, bIndex);
            return CAPS_BADINDEX;
          }

    } else {

      /* check that the body is not in the mapping because it lacks tessellation */
      for (i = 0; i < discr->nPoints; i++)
        if (discr->mapping[2*i] == bIndex) {
          snprintf(line, l, "caps_checkDiscr: global mapping exists for body %d which lacks tessellation!\n",
                   bIndex);
          return CAPS_BADINDEX;
        }
    }

    /* do the elements */
    last = 0;
    for (i = 1; i <= discr->nElems; i++)
      if (discr->elems[i-1].bIndex == bIndex) {
        if ((discr->elems[i-1].tIndex < 1) ||
            (discr->elems[i-1].tIndex > discr->nTypes)) {
          snprintf(line, l,
                   "caps_checkDiscr: elems[%d].tIndex = %d out of range [1-%d] ",
                   i, discr->elems[i-1].tIndex, discr->nTypes);
          return CAPS_BADINDEX;
        }
        if (discr->dim == 1) {
          if ((discr->elems[i-1].eIndex < 1) ||
              (discr->elems[i-1].eIndex > nEdge)) {
            snprintf(line, l,
                     "caps_checkDiscr: elems[%d].eIndex = %d out of range [1-%d] ",
                     i, discr->elems[i-1].eIndex, nEdge);
            return CAPS_BADINDEX;
          }
          if (discr->elems[i-1].eIndex != last) {
/*@-nullpass@*/
            stat = EG_getTessEdge(analysis->bodies[bIndex-1+analysis->nBody],
                                  discr->elems[i-1].eIndex, &npts, &xyz, &prms);
/*@+nullpass@*/
            if (stat != EGADS_SUCCESS) {
              snprintf(line, l, "caps_checkDiscr: getTessEdge %d = %d for %d!\n",
                       discr->elems[i-1].eIndex, stat, bIndex);
              return stat;
            }
            ntris = npts-1;
            last  = discr->elems[i-1].eIndex;
          }
        } else if (discr->dim == 2) {
          if ((discr->elems[i-1].eIndex < 1) ||
              (discr->elems[i-1].eIndex > nFace)) {
            snprintf(line, l,
                     "caps_checkDiscr: elems[%d].eIndex = %d out of range [1-%d] ",
                     i, discr->elems[i-1].eIndex, nFace);
            return CAPS_BADINDEX;
          }
          if (discr->elems[i-1].eIndex != last) {
/*@-nullpass@*/
            stat = EG_getTessFace(analysis->bodies[bIndex-1+analysis->nBody],
                                  discr->elems[i-1].eIndex, &npts, &xyz, &prms,
                                  &ptype, &pindex, &ntris, &tris, &tric);
/*@+nullpass@*/
            if (stat != EGADS_SUCCESS) {
              snprintf(line, l, "caps_checkDiscr: getTessFace %d = %d for %d!\n",
                       discr->elems[i-1].eIndex, stat, bIndex);
              return stat;
            }
            last = discr->elems[i-1].eIndex;
          }
        }
        typ = discr->elems[i-1].tIndex;
        len = discr->types[typ-1].nref;
        for (j = 0; j < len; j++) {
          if ((discr->elems[i-1].gIndices[2*j] < 1) ||
              (discr->elems[i-1].gIndices[2*j] > discr->nPoints)) {
            snprintf(line, l,
                     "caps_checkDiscr: elems[%d].gIndices[%d]p = %d out of range [1-%d] ",
                     i, j+1, discr->elems[i-1].gIndices[2*j], discr->nPoints);
            return CAPS_BADINDEX;
          }
#ifndef __clang_analyzer__
          if ((discr->elems[i-1].gIndices[2*j+1] < 1) ||
              (discr->elems[i-1].gIndices[2*j+1] > npts)) {
            snprintf(line, l,
                     "caps_checkDiscr: elems[%d].gIndices[%d]t = %d out of range [1-%d] ",
                     i, j+1, discr->elems[i-1].gIndices[2*j+1], npts);
            return CAPS_BADINDEX;
          }
#endif
        }
        if (discr->verts != NULL) {
          len = discr->types[typ-1].ndata;
          if ((len != 0) && (discr->elems[i-1].dIndices == NULL)) {
            snprintf(line, l,
                     "caps_checkDiscr: elems[%d].dIndices[%d] == NULL ",
                     i, j+1);
            return CAPS_NULLVALUE;
          }
          for (j = 0; j < len; j++)
            if ((discr->elems[i-1].dIndices[j] < 1) ||
                (discr->elems[i-1].dIndices[j] > discr->nVerts)) {
              snprintf(line, l,
                       "caps_checkDiscr: elems[%d].dIndices[%d] = %d out of range [1-%d] ",
                       i, j+1, discr->elems[i-1].dIndices[j], discr->nPoints);
              return CAPS_BADINDEX;
            }
        }
        len = discr->types[typ-1].ntri;
        if (len > 2) {
          if (discr->elems[i-1].eTris.poly == NULL) {
            snprintf(line, l, "caps_checkDiscr: elems[%d].poly = NULL!", i);
            return CAPS_NULLVALUE;
          }
          for (j = 0; j < len; j++) {
#ifndef __clang_analyzer__
            if ((discr->elems[i-1].eTris.poly[j] < 1) ||
                (discr->elems[i-1].eTris.poly[j] > ntris)) {
              snprintf(line, l,
                       "caps_checkDiscr: elems[%d].eTris[%d] = %d out of range [1-%d] ",
                       i, j+1, discr->elems[i-1].eTris.poly[j], ntris);
              return CAPS_BADINDEX;
            }
#endif
          }
        } else {
          for (j = 0; j < len; j++) {
#ifndef __clang_analyzer__
            if ((discr->elems[i-1].eTris.tq[j] < 1) ||
                (discr->elems[i-1].eTris.tq[j] > ntris)) {
              snprintf(line, l,
                       "caps_checkDiscr: elems[%d].eTris[%d] = %d out of range [1-%d] ",
                       i, j+1, discr->elems[i-1].eTris.tq[j], ntris);
              return CAPS_BADINDEX;
            }
#endif
          }
        }
      }

  }

  /* check data triangulation */
  if ((discr->nDtris != 0) && (discr->dtris != NULL) &&
      (discr->nVerts != 0))
    for (i = 0; i < discr->nDtris; i++)
      for (j = 0; j < 3; j++)
        if ((discr->dtris[3*i+j] < 1) ||
            (discr->dtris[3*i+j] > discr->nVerts)) {
          snprintf(line, l,
                   "caps_checkDiscr: dtris[%d %d] = %d out of range [1-%d] ",
                   i+1, j+1, discr->dtris[3*i+j], discr->nVerts);
          return CAPS_BADINDEX;
        }
  
  return CAPS_SUCCESS;
}


static void
caps_fillBuiltIn(capsObject *bobject, capsDiscr *discr, capsObject *dobject,
                 CAPSLONG sNum)
{
  int           i, j, k, m, stat, npts, rank, bIndex, pt, pi, typ, len, OK = 1;
  int           last, ntris, nptx, index;
  double        *values, xyz[3], st[2];
  capsObject    *pobject, *vobject;
  capsProblem   *problem;
  capsBound     *bound;
  aimInfo       *aInfo;
  capsAnalysis  *analysis;
  capsDataSet   *dataset, *ds;
  capsVertexSet *vertexset;
  capsOwn       *tmp;
  const int     *ptype, *pindex, *tris, *tric;
  const double  *xyzs, *prms;
  
  aInfo     = (aimInfo *)       discr->aInfo;
  analysis  = (capsAnalysis *)  aInfo->analysis;
  bound     = (capsBound *)     bobject->blind;
  dataset   = (capsDataSet *)   dobject->blind;
  vobject   =                   dobject->parent;
  vertexset = (capsVertexSet *) vobject->blind;
  pobject   =                   bobject->parent;
  problem   = (capsProblem *)   pobject->blind;
  if ((strcmp(dobject->name, "xyzd")   == 0) && (discr->verts == NULL)) return;
  if ((strcmp(dobject->name, "pamamd") == 0) && (discr->verts == NULL)) return;
  
  rank = dataset->rank;
  if (strcmp(dobject->name, "xyz") == 0) {
    npts = discr->nPoints;
  } else if (strcmp(dobject->name, "xyzd") == 0) {
    npts = discr->nVerts;
  } else if (strcmp(dobject->name, "param") == 0) {
    if (bound->state == MultipleError) return;
    npts = discr->nPoints;
  } else if (strcmp(dobject->name, "paramd") == 0) {
    if (bound->state == MultipleError) return;
    npts = discr->nVerts;
  } else {
    printf(" CAPS Internal: Unknown BuiltIn DataSet = %s\n", dobject->name);
    return;
  }
  if (npts == 0) return;
  
  values = (double *) EG_alloc(npts*rank*sizeof(double));
  if (values == NULL) {
    printf(" CAPS Internal: Malloc on %d %d  Dataset = %s\n", npts, rank,
           dobject->name);
    return;
  }

  if (strcmp(dobject->name, "xyz") == 0) {
    
    for (bIndex = 1; bIndex <= analysis->nBody; bIndex++)
      for (i = 0; i < npts; i++) {
        if (discr->mapping[2*i] != bIndex) continue;
        stat = EG_getGlobal(analysis->bodies[bIndex-1+analysis->nBody],
                            discr->mapping[2*i+1], &pt, &pi, &values[3*i]);
        if (stat != EGADS_SUCCESS)
          printf(" CAPS Internal: %d EG_getGlobal %d for %s = %d\n",
                 bIndex, i+1, dobject->name, stat);
      }
    if (bound->lunits != NULL) {
      if (dataset->units != NULL) EG_free(dataset->units);
      dataset->units = EG_strdup(bound->lunits);
    }
    
  } else if (strcmp(dobject->name, "xyzd") == 0) {
    
    for (i = 0; i < npts; i++) {
      values[3*i  ] = discr->verts[3*i  ];
      values[3*i+1] = discr->verts[3*i+1];
      values[3*i+2] = discr->verts[3*i+2];
    }
    if (bound->lunits != NULL) {
      if (dataset->units != NULL) EG_free(dataset->units);
      dataset->units = EG_strdup(bound->lunits);
    }
    
  } else if (strcmp(dobject->name, "param") == 0) {

    if (bound->dim == 2) {
      for (i = 0; i < npts; i++) {
        values[2*i  ] = 0.0;
        values[2*i+1] = 0.0;
      }
      if (bound->state != Multiple) {
        for (bIndex = 1; bIndex <= analysis->nBody; bIndex++) {
          for (last = j = 0; j < discr->nElems; j++) {
            if (discr->elems[j].bIndex != bIndex) continue;
            if (discr->elems[j].eIndex != last) {
              stat = EG_getTessFace(analysis->bodies[bIndex-1+analysis->nBody],
                                    discr->elems[j].eIndex, &nptx, &xyzs, &prms,
                                    &ptype, &pindex, &ntris, &tris, &tric);
              if (stat != EGADS_SUCCESS) {
                printf(" CAPS Internal: getTessFace %d = %d for %d\n",
                         discr->elems[j].eIndex, stat, bIndex);
                continue;
              }
              last = discr->elems[j].eIndex;
            }
            typ = discr->elems[j].tIndex;
            len = discr->types[typ-1].nref;
            for (k = 0; k < len; k++) {
              i  = discr->elems[j].gIndices[2*k  ]-1;
              pt = discr->elems[j].gIndices[2*k+1]-1;
              values[2*i  ] = prms[2*pt  ];
              values[2*i+1] = prms[2*pt+1];
            }
          }
        }
      } else {
        for (bIndex = 1; bIndex <= analysis->nBody; bIndex++)
          for (i = 0; i < npts; i++) {
            if (discr->mapping[2*i] != bIndex) continue;
            stat = EG_getGlobal(analysis->bodies[bIndex-1+analysis->nBody],
                                discr->mapping[2*i+1], &pt, &pi, xyz);
            if (stat != EGADS_SUCCESS) {
              printf(" CAPS Internal: %d EG_getGlobal %d for %s = %d\n",
                     bIndex, i+1, dobject->name, stat);
              continue;
            }
            stat = caps_invInterpolate2D(bound->surface, xyz, &values[2*i]);
            if (stat != EGADS_SUCCESS)
              printf(" CAPS Internal: caps_invInterpolate2D %d for %s = %d\n",
                     i+1, dobject->name, stat);
          }
      }

    } else {

      if (bound->state == Single) {
        for (bIndex = 1; bIndex <= analysis->nBody; bIndex++) {
          for (last = j = 0; j < discr->nElems; j++) {
            if (discr->elems[j].bIndex != bIndex) continue;
            if (discr->elems[j].eIndex != last) {
              stat = EG_getTessEdge(analysis->bodies[bIndex-1+analysis->nBody],
                                    discr->elems[j].eIndex, &nptx, &xyzs, &prms);
              if (stat != EGADS_SUCCESS) {
                printf(" CAPS Internal: getTessEdge %d = %d for %d\n",
                       discr->elems[j].eIndex, stat, bIndex);
                continue;
              }
              last = discr->elems[j].eIndex;
            }
            typ = discr->elems[j].tIndex;
            len = discr->types[typ-1].nref;
            for (k = 0; k < len; k++) {
              i  = discr->elems[j].gIndices[2*k  ]-1;
              pt = discr->elems[j].gIndices[2*k+1]-1;
              values[i] = prms[pt];
            }
          }
        }
      } else {
        for (bIndex = 1; bIndex <= analysis->nBody; bIndex++)
          for (i = 0; i < npts; i++) {
            if (discr->mapping[2*i] != bIndex) continue;
            stat = EG_getGlobal(analysis->bodies[bIndex-1+analysis->nBody],
                                discr->mapping[2*i+1], &pt, &pi, xyz);
            if (stat != EGADS_SUCCESS) {
              printf(" CAPS Internal: %d EG_getGlobal %d for %s = %d\n",
                     bIndex, i+1, dobject->name, stat);
              continue;
            }
            stat = caps_invInterpolate1D(bound->curve, xyz, &values[i]);
            if (stat != EGADS_SUCCESS)
              printf(" CAPS Internal: caps_invInterpolate1D %d for %s = %d\n",
                     i+1, dobject->name, stat);
          }
      
      }
    }
    
  } else {

    for (i = 0; i < npts; i++) values[i] = 0.0;
    index = aim_Index(problem->aimFPTR, analysis->loadName);
    if (index < 0)
      printf(" CAPS Internal: aim_Index %s = %d\n", dobject->name, index);
    
    /* assume that we have the dataset "param" */
    ds = NULL;
    if (vertexset->dataSets[2] == NULL) {
      printf(" CAPS Internal: params obj == NULL for %s\n", dobject->name);
      index = -1;
    } else {
      ds = (capsDataSet *) vertexset->dataSets[2]->blind;
      if (ds == NULL) {
        printf(" CAPS Internal: params ds == NULL for %s\n", dobject->name);
        index = -1;
      }
    }

    if (bound->dim == 2) {
      for (i = 0; i < npts; i++) {
        values[2*i  ] = 0.0;
        values[2*i+1] = 0.0;
      }
      if (bound->state == Single) {
        for (i = 0; i < npts; i++) {
          if (index < 0) continue;
          k   = discr->celem[i] - 1;
          m   = discr->elems[k].tIndex - 1;
          len = discr->types[m].ndata;
          for (j = 0; j < len; j++)
            if (discr->elems[k].dIndices[j] == i+1) {
              st[0] = discr->types[m].dst[2*j  ];
              st[1] = discr->types[m].dst[2*j+1];
              break;
            }
          if ((j == len) || (ds == NULL)) {
            printf(" CAPS Internal: data ref %d for %s not found!\n",
                   i+1, dobject->name);
            continue;
          }
          stat  = aim_InterpolIndex(problem->aimFPTR, index, discr, "paramd",
                                    discr->celem[i], st, 2, ds->data,
                                    &values[2*i]);
          if (stat != CAPS_SUCCESS)
            printf(" CAPS Internal: aim_InterpolIndex %d for %s = %d\n",
                   i+1, dobject->name, stat);
        }
      } else {
        for (i = 0; i < npts; i++) {
          xyz[0] = discr->verts[3*i  ];
          xyz[1] = discr->verts[3*i+1];
          xyz[2] = discr->verts[3*i+2];
          stat   = caps_invInterpolate2D(bound->surface, xyz, &values[2*i]);
          if (stat != EGADS_SUCCESS)
            printf(" CAPS Internal: caps_invInterpolate2D %d for %s = %d\n",
                   i+1, dobject->name, stat);
        }
      }
      
    } else {
      for (i = 0; i < npts; i++) values[i] = 0.0;
      if (bound->state == Single) {
        for (i = 0; i < npts; i++) {
          if (index < 0) continue;
          k   = discr->celem[i] - 1;
          m   = discr->elems[k].tIndex - 1;
          len = discr->types[m].ndata;
          for (j = 0; j < len; j++)
            if (discr->elems[k].dIndices[j] == i+1) {
              st[0] = discr->types[m].dst[j];
              break;
            }
          if ((j == len) || (ds == NULL)) {
            printf(" CAPS Internal: data ref %d for %s not found!\n",
                   i+1, dobject->name);
            continue;
          }
          stat  = aim_InterpolIndex(problem->aimFPTR, index, discr, "paramd",
                                    discr->celem[i], st, 1, ds->data,
                                    &values[i]);
          if (stat != CAPS_SUCCESS)
            printf(" CAPS Internal: aim_InterpolIndex %d for %s = %d\n",
                   i+1, dobject->name, stat);
        }
      } else {
        for (i = 0; i < npts; i++) {
          xyz[0] = discr->verts[3*i  ];
          xyz[1] = discr->verts[3*i+1];
          xyz[2] = discr->verts[3*i+2];
          stat = caps_invInterpolate1D(bound->curve, xyz, &values[i]);
          if (stat != EGADS_SUCCESS)
            printf(" CAPS Internal: caps_invInterpolate1D %d for %s = %d\n",
                   i+1, dobject->name, stat);
        }
      }
    }

  }
  
  dataset->data = values;
  dataset->npts = npts;
  
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
  dobject->last.sNum = sNum;
  caps_fillDateTime(dobject->last.datetime);
}


static void
caps_fillSensit(capsProblem *problem, capsDiscr *discr, capsDataSet *dataset)
{
  int          i, j, k, bIndex, index, ibody, stat;
  int          ii, ni, oclass, mtype, nEdge, nFace, len, ntri, *bins;
  const int    *ptype, *pindex, *tris, *tric;
  const double *dxyz, *xyzs, *uvs;
  ego          topRef, prev, next, tess, oldtess, *objs;
  modl_T       *MODL;
  aimInfo      *aInfo;
  capsAnalysis *analysis;
  
  aInfo    = (aimInfo *) discr->aInfo;
  analysis = (capsAnalysis *) aInfo->analysis;

  MODL = (modl_T *) problem->modl;
  for (bIndex = 1; bIndex <= analysis->nBody; bIndex++) {
    stat = EG_getInfo(analysis->bodies[bIndex-1], &oclass, &mtype, &topRef,
                      &prev, &next);
    if (stat != EGADS_SUCCESS) {
      printf(" caps_fillSensit abort: getInfo = %d for %d!\n", stat, bIndex);
      return;
    }
    nEdge = nFace = 0;
    if (mtype == WIREBODY) {
      stat = EG_getBodyTopos(analysis->bodies[bIndex-1], NULL, EDGE, &nEdge,
                             &objs);
      if (stat != EGADS_SUCCESS) {
        printf(" caps_fillSensit abort: getBodyTopos (Edge) = %d for %d!\n",
               stat, bIndex);
        return;
      }
    } else {
      stat = EG_getBodyTopos(analysis->bodies[bIndex-1], NULL, FACE, &nFace,
                             &objs);
      if (stat != EGADS_SUCCESS) {
        printf(" caps_fillSensit abort: getBodyTopos (Face) = %d for %d!\n",
               stat, bIndex);
        return;
      }
    }
    EG_free(objs);
    for (ibody = 1; ibody <= MODL->nbody; ibody++) {
      if (MODL->body[ibody].onstack != 1) continue;
      if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
      if (MODL->body[ibody].ebody   == analysis->bodies[bIndex-1]) break;
    }
    if (ibody > MODL->nbody) {
      printf(" caps_fillSensit abort: Body Not Found in OpenCSM stack!\n");
      return;
    }
    oldtess = MODL->body[ibody].etess;
    tess    = analysis->bodies[analysis->nBody+bIndex-1];
    if (tess == NULL) {
      printf(" caps_fillSensit abort: Body Tess %d Not Found!\n",
             ibody);
      return;
    }
    MODL->body[ibody].etess = tess;
  
    bins = (int *) EG_alloc((nEdge+nFace)*sizeof(int));
    if (bins == NULL) {
      printf(" caps_fillSensit abort: %d allocating = %d ints!\n",
             ibody, nEdge+nFace);
      MODL->body[ibody].etess = oldtess;
      return;
    }
    for (ii = 0; ii < discr->nElems; ii++)
      if (discr->elems[ii].bIndex == bIndex) {
        j = discr->elems[ii].eIndex - 1;
        bins[j]++;
      }
    
    if (nFace == 0) {
      
      for (index = 1; index <= nEdge; index++) {
        if (bins[index-1] == 0) continue;
        stat = EG_getTessEdge(tess, index, &len, &xyzs, &uvs);
        if (stat != SUCCESS) {
          printf(" caps_fillSensit EG_getTessFace Edge = %d for %d!\n",
                 stat, index);
          continue;
        }
        stat = ocsmGetTessVel(problem->modl, ibody, OCSM_EDGE, index, &dxyz);
        if (stat != SUCCESS) {
          printf(" caps_fillSensit ocsmGetTessVel Edge = %d for %d!\n",
                 stat, index);
          continue;
        }
        for (ii = 0; ii < discr->nElems; ii++) {
          if (discr->elems[ii].bIndex != bIndex) continue;
          if (discr->elems[ii].eIndex !=  index) continue;
          ni = discr->types[discr->elems[ii].tIndex-1].nref;
          for (k = 0; k < ni; k++) {
            i = discr->elems[ii].gIndices[2*k  ]-1;
            j = discr->elems[ii].gIndices[2*k+1]-1;
            dataset->data[3*i  ] = dxyz[3*j  ];
            dataset->data[3*i+1] = dxyz[3*j+1];
            dataset->data[3*i+2] = dxyz[3*j+2];
          }
        }
      }
      
    } else {
      
      for (index = 1; index <= nFace; index++) {
        if (bins[index-1] == 0) continue;
        stat = EG_getTessFace(tess, index, &len, &xyzs, &uvs, &ptype,
                              &pindex, &ntri, &tris, &tric);
        if (stat != SUCCESS) {
          printf(" caps_fillSensit EG_getTessFace Face = %d for %d!\n",
                 stat, index);
          continue;
        }
        stat = ocsmGetTessVel(problem->modl, ibody, OCSM_FACE, index, &dxyz);
        if (stat != SUCCESS) {
          printf(" caps_fillSensit ocsmGetTessVel Face = %d for %d!\n",
                 stat, index);
          continue;
        }
        for (ii = 0; ii < discr->nElems; ii++) {
          if (discr->elems[ii].bIndex != bIndex) continue;
          if (discr->elems[ii].eIndex !=  index) continue;
          ni = discr->types[discr->elems[ii].tIndex-1].nref;
          for (k = 0; k < ni; k++) {
            i = discr->elems[ii].gIndices[2*k  ]-1;
            j = discr->elems[ii].gIndices[2*k+1]-1;
            dataset->data[3*i  ] = dxyz[3*j  ];
            dataset->data[3*i+1] = dxyz[3*j+1];
            dataset->data[3*i+2] = dxyz[3*j+2];
          }
        }
      }
      
    }
    EG_free(bins);
  
    MODL->body[ibody].etess = oldtess;
  }

}


static void
caps_freeBodyObjs(int nBodies, /*@only@*/ bodyObjs *bodies)
{
  int i;
  
  for (i = 0; i < nBodies; i++) {
    if (bodies[i].objs    != NULL) EG_free(bodies[i].objs);
    if (bodies[i].indices != NULL) EG_free(bodies[i].indices);
  }
  EG_free(bodies);
}


static double
caps_triangleArea3D(const double *xyz0, const double *xyz1, const double *xyz2)
{
  double x1[3], x2[3], n[3];
  
  x1[0] = xyz1[0] - xyz0[0];
  x2[0] = xyz2[0] - xyz0[0];
  x1[1] = xyz1[1] - xyz0[1];
  x2[1] = xyz2[1] - xyz0[1];
  x1[2] = xyz1[2] - xyz0[2];
  x2[2] = xyz2[2] - xyz0[2];
  CROSS(n, x1, x2);
  return 0.5*sqrt(DOT(n, n));
}


static int
caps_paramQuilt(capsBound *bound, int l, char *line)
{
  int           i, j, k, m, n, stat, npts, ntris, own, nu, nv, per, eType;
  int           ntrx, nptx, last, bIndex, pt, pi, *ppnts, *vtab, count, iVS;
  int           i0, i1, i2;
  double        coord[3], box[6], tol, rmserr, maxerr, dotmin, area, d;
  double        *grid, *r, *xyzs;
  capsAnalysis  *analysis;
  capsDiscr     *quilt;
  capsAprx2D    *surface;
  capsVertexSet *vertexset;
  capsObject    *aobject;
  prmXYZ        *xyz;
  prmTri        *tris;
  prmUVF        *uvf;
  prmUV         *uv;
  connect       *etab;
  const int     *ptype, *pindex, *trix, *tric;
  const double  *xyzx, *prms;

  for (npts = ntris = i = 0; i < bound->nVertexSet; i++) {
    if (bound->vertexSet[i]              == NULL)      continue;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[i]->type        != VERTEXSET) continue;
    if (bound->vertexSet[i]->blind       == NULL)      continue;
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    if (vertexset->analysis              == NULL)      continue;
    if (vertexset->discr                 == NULL)      continue;
    quilt  = vertexset->discr;
    for (j = 0; j < quilt->nElems; j++) {
      eType = quilt->elems[j].tIndex;
      if (quilt->types[eType-1].tris == NULL) {
        ntris++;
      } else {
        ntris += quilt->types[eType-1].ntri;
      }
    }
    npts += quilt->nPoints;
  }
  
  if ((ntris == 0) || (npts == 0)) {
    snprintf(line, l,
             "caps_paramQuilt Error: nPoints = %d  nTris = %d", npts, ntris);
    return CAPS_NOTCONNECT;
  }

  uv    = (prmUV *)  EG_alloc(npts*sizeof(prmUV));
  if (uv   == NULL) {
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on = %d prmUV", npts);
    return EGADS_MALLOC;
  }
  uvf   = (prmUVF *) EG_alloc(ntris*sizeof(prmUVF));
  if (uvf  == NULL) {
    EG_free(uv);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on = %d prmUVF", ntris);
    return EGADS_MALLOC;
  }
  xyz   = (prmXYZ *) EG_alloc(npts*sizeof(prmXYZ));
  if (xyz  == NULL) {
    EG_free(uvf);
    EG_free(uv);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on = %d prmXYZ", npts);
    return EGADS_MALLOC;
  }
  xyzs  = (double *) xyz;
  tris  = (prmTri *) EG_alloc(ntris*sizeof(prmTri));
  if (tris  == NULL) {
    EG_free(xyz);
    EG_free(uvf);
    EG_free(uv);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on = %d ints", 3*ntris);
    return EGADS_MALLOC;
  }

  /* find the best candidate VertexSet for fitting */
  iVS  = -1;
  area =  0.0;
  for (i = 0; i < bound->nVertexSet; i++) {
    if (bound->vertexSet[i]              == NULL)      continue;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[i]->type        != VERTEXSET) continue;
    if (bound->vertexSet[i]->blind       == NULL)      continue;
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    if (vertexset->analysis              == NULL)      continue;
    if (vertexset->discr                 == NULL)      continue;
    aobject  = vertexset->analysis;
    analysis = (capsAnalysis *) aobject->blind;
    quilt    = vertexset->discr;
#ifdef VSOUTPUT
    {
      extern int caps_triangulate(const capsObject *vobject, int *nGtris,
                                  int **gtris, int *nDtris, int **dtris);
      int  nGtris, *gtris, nDtris, *dtris;
      char fileName[121];
      FILE *fp;
      
      snprintf(fileName, 120, "%s%d.vs", bound->vertexSet[i]->parent->name, i);
      fp = fopen(fileName, "w");
      if (fp != NULL) {
        stat = caps_triangulate(bound->vertexSet[i], &nGtris, &gtris,
                                                     &nDtris, &dtris);
        if (stat == CAPS_SUCCESS) {
          printf(" **** writing VertexSet file: %s ****\n", fileName);
          fprintf(fp, "%s\n", bound->vertexSet[i]->parent->name);
          fprintf(fp, "%8d %8d %8d\n", nGtris, nDtris, 1);
          for (k = 0; k < nGtris; k++)
            fprintf(fp, "    %8d %8d %8d\n", gtris[3*k  ], gtris[3*k+1],
                                             gtris[3*k+2]);
          for (k = 0; k < nDtris; k++)
            fprintf(fp, "    %8d %8d %8d\n", dtris[3*k  ], dtris[3*k+1],
                                             dtris[3*k+2]);
          EG_free(gtris);
          EG_free(dtris);
          fprintf(fp, "%s\n", "xyz");
          fprintf(fp, " %8d %8d\n", quilt->nPoints, 3);
          for (bIndex = 1; bIndex <= analysis->nBody; bIndex++)
            for (j = 0; j < quilt->nPoints; j++) {
              if (quilt->mapping[2*j] != bIndex) continue;
              stat = EG_getGlobal(analysis->bodies[bIndex-1+analysis->nBody],
                                  quilt->mapping[2*j+1], &pt, &pi, coord);
              if (stat != EGADS_SUCCESS) {
                printf(" CAPS Internal: %d EG_getGlobal %d = %d\n",
                       bIndex, j+1, stat);
                fprintf(fp, " 0.0 0.0 0.0\n");
              } else {
                fprintf(fp, " %lf %lf %lf\n", coord[0], coord[1], coord[2]);
              }
            }
        }
        fclose(fp);
      }
    }
#endif
    d     = 0.0;
    ntris = 0;
    for (bIndex = 1; bIndex <= analysis->nBody; bIndex++)
      for (last = j = 0; j < quilt->nElems; j++) {
        if (quilt->elems[j].bIndex != bIndex) continue;
        eType = quilt->elems[j].tIndex;
        own   = quilt->elems[j].eIndex;
        if (own != last) {
          stat = EG_getTessFace(analysis->bodies[bIndex-1+analysis->nBody],
                                own, &nptx, &xyzx, &prms, &ptype, &pindex,
                                &ntrx, &trix, &tric);
          if (stat != EGADS_SUCCESS) {
            printf(" CAPS Internal: EG_getTessFace %d = %d for %d\n",
                   own, stat, bIndex);
            continue;
          }
/*        printf(" VS = %s\n   Face %d: ntris = %d  quilt eles = %d!\n",
                 bound->vertexSet[i]->parent->name, own, nptx, quilt->nElems); */
          last = own;
        }
        if (quilt->types[eType-1].tris == NULL) {
          i0 = quilt->elems[j].gIndices[1] - 1;
          i1 = quilt->elems[j].gIndices[3] - 1;
          i2 = quilt->elems[j].gIndices[5] - 1;
          d += caps_triangleArea3D(&xyzx[3*i0], &xyzx[3*i1], &xyzx[3*i2]);
          ntris++;
        } else {
#ifndef __clang_analyzer__
          for (k = 0; k < quilt->types[eType-1].ntri; k++, ntris++) {
            n  = quilt->types[eType-1].tris[3*k  ] - 1;
            i0 = quilt->elems[j].gIndices[2*n+1]   - 1;
            n  = quilt->types[eType-1].tris[3*k+1] - 1;
            i1 = quilt->elems[j].gIndices[2*n+1]   - 1;
            n  = quilt->types[eType-1].tris[3*k+2] - 1;
            i2 = quilt->elems[j].gIndices[2*n+1]   - 1;
            d += caps_triangleArea3D(&xyzx[3*i0], &xyzx[3*i1], &xyzx[3*i2]);
          }
#endif
        }
      }
#ifdef DEBUG
    printf(" VertexSet %d: area = %lf  ntris = %d\n", i+1, d, ntris);
#endif
    if (d > area) {
      iVS  = i;
      area = d;
    }
  }
  if (iVS == -1) {
    EG_free(tris);
    EG_free(xyz);
    EG_free(uvf);
    EG_free(uv);
    snprintf(line, l,
             "caps_paramQuilt Error: No VertexSet Selected!");
    return EGADS_NOTFOUND;
  }
#ifdef DEBUG
  printf(" selected VertexSet = %d\n", iVS+1);
#endif

/*
  for (count = npts = ntris = i = 0; i < bound->nVertexSet; i++) {  */
  count = npts = ntris = 0;
  for (i = iVS; i <= iVS; i++) {
    if (bound->vertexSet[i]              == NULL)      continue;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[i]->type        != VERTEXSET) continue;
    if (bound->vertexSet[i]->blind       == NULL)      continue;
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    if (vertexset->analysis              == NULL)      continue;
    if (vertexset->discr                 == NULL)      continue;
    aobject  = vertexset->analysis;
    analysis = (capsAnalysis *) aobject->blind;
    quilt    = vertexset->discr;
    prms     = NULL;
    for (bIndex = 1; bIndex <= analysis->nBody; bIndex++)
      for (last = j = 0; j < quilt->nElems; j++) {
        if (quilt->elems[j].bIndex != bIndex) continue;
        eType = quilt->elems[j].tIndex;
        own   = quilt->elems[j].eIndex;
        if (own != last) {
          count++;
/*        printf(" Analysis %d/%d Body %d/%d Face %d: count = %d, npts = %d\n",
                 i+1, bound->nVertexSet, bIndex, analysis->nBody, own, count,
                 npts);  */
          stat = EG_getTessFace(analysis->bodies[bIndex-1+analysis->nBody],
                                own, &nptx, &xyzx, &prms, &ptype, &pindex,
                                &ntrx, &trix, &tric);
          if (stat != EGADS_SUCCESS) {
            printf(" CAPS Internal: getTessFace %d = %d for %d\n",
                   own, stat, bIndex);
            continue;
          }
          last = own;
        }
        if (prms == NULL) continue;
        if (quilt->types[eType-1].tris == NULL) {
          m = quilt->elems[j].gIndices[1] - 1;
          uvf[ntris].u0          = prms[2*m  ];
          uvf[ntris].v0          = prms[2*m+1];
          tris[ntris].indices[0] = quilt->elems[j].gIndices[0] + npts;
          m = quilt->elems[j].gIndices[3] - 1;
          uvf[ntris].u1          = prms[2*m  ];
          uvf[ntris].v1          = prms[2*m+1];
          tris[ntris].indices[1] = quilt->elems[j].gIndices[2] + npts;
          m = quilt->elems[j].gIndices[5] - 1;
          uvf[ntris].u2          = prms[2*m  ];
          uvf[ntris].v2          = prms[2*m+1];
          tris[ntris].indices[2] = quilt->elems[j].gIndices[4] + npts;
          tris[ntris].own        = count;
          ntris++;
        } else {
          for (k = 0; k < quilt->types[eType-1].ntri; k++, ntris++) {
            n = quilt->types[eType-1].tris[3*k  ] - 1;
            m = quilt->elems[j].gIndices[2*n+1]   - 1;
            uvf[ntris].u0          = prms[2*m  ];
            uvf[ntris].v0          = prms[2*m+1];
            tris[ntris].indices[0] = quilt->elems[j].gIndices[2*n] + npts;
            n = quilt->types[eType-1].tris[3*k+1] - 1;
            m = quilt->elems[j].gIndices[2*n+1]   - 1;
            uvf[ntris].u1          = prms[2*m  ];
            uvf[ntris].v1          = prms[2*m+1];
            tris[ntris].indices[1] = quilt->elems[j].gIndices[2*n] + npts;
            n = quilt->types[eType-1].tris[3*k+2] - 1;
            m = quilt->elems[j].gIndices[2*n+1]   - 1;
            uvf[ntris].u2          = prms[2*m  ];
            uvf[ntris].v2          = prms[2*m+1];
            tris[ntris].indices[2] = quilt->elems[j].gIndices[2*n] + npts;
            tris[ntris].own        = count;
          }
        }
      }
    for (bIndex = 1; bIndex <= analysis->nBody; bIndex++)
      for (j = 0; j < quilt->nPoints; j++) {
        if (quilt->mapping[2*j] != bIndex) continue;
        stat = EG_getGlobal(analysis->bodies[bIndex-1+analysis->nBody],
                            quilt->mapping[2*j+1], &pt, &pi, coord);
        if (stat != EGADS_SUCCESS) {
          printf(" CAPS Internal: %d EG_getGlobal %d = %d\n", bIndex, j+1, stat);
        } else {
          xyz[npts+j].x = coord[0];
          xyz[npts+j].y = coord[1];
          xyz[npts+j].z = coord[2];
        }
      }
    npts += quilt->nPoints;
  }
  
  /* make the neighbors */
  
  vtab = (int *) EG_alloc(npts*sizeof(int));
  if (vtab == NULL) {
    EG_free(tris);
    EG_free(xyz);
    EG_free(uvf);
    EG_free(uv);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on table = %d ints", npts);
    return EGADS_MALLOC;
  }
  etab = (connect *) EG_alloc(ntris*3*sizeof(connect));
  if (etab == NULL) {
    EG_free(vtab);
    EG_free(tris);
    EG_free(xyz);
    EG_free(uvf);
    EG_free(uv);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on side table = %d connect",
             3*ntris);
    return EGADS_MALLOC;
  }
  n = NOTFILLED;
  for (j = 0; j < npts; j++) vtab[j] = NOTFILLED;
  for (i = 0; i < ntris;  i++) {
    tris[i].neigh[0] = tris[i].neigh[1] = tris[i].neigh[2] = i+1;
    EG_makeConnect( tris[i].indices[1], tris[i].indices[2],
                   &tris[i].neigh[0], &n, vtab, etab, 0);
    EG_makeConnect( tris[i].indices[0], tris[i].indices[2],
                   &tris[i].neigh[1], &n, vtab, etab, 0);
    EG_makeConnect( tris[i].indices[0], tris[i].indices[1],
                   &tris[i].neigh[2], &n, vtab, etab, 0);
  }
  /* find any unconnected triangle sides */
  for (j = 0; j <= n; j++) {
    if (etab[j].tri == NULL) continue;
/*  printf(" CAPS Info: Unconnected Side %d %d = %d\n",
           etab[j].node1+1, etab[j].node2+1, *etab[j].tri); */
    *etab[j].tri = 0;
  }
  EG_free(etab);
  EG_free(vtab);

  /* get tolerance */

  box[0] = box[3] = xyz[0].x;
  box[1] = box[4] = xyz[0].y;
  box[2] = box[5] = xyz[0].z;
  for (j = 1; j < npts; j++) {
    if (xyz[j].x < box[0]) box[0] = xyz[j].x;
    if (xyz[j].x > box[3]) box[3] = xyz[j].x;
    if (xyz[j].y < box[1]) box[1] = xyz[j].y;
    if (xyz[j].y > box[4]) box[4] = xyz[j].y;
    if (xyz[j].z < box[2]) box[2] = xyz[j].z;
    if (xyz[j].z > box[5]) box[5] = xyz[j].z;
  }
  n     = 1;
  grid  = NULL;
  ppnts = NULL;
  tol   = 1.e-7*sqrt((box[3]-box[0])*(box[3]-box[0]) +
                     (box[4]-box[1])*(box[4]-box[1]) +
                     (box[5]-box[2])*(box[5]-box[2]));
  
  /* reparameterize */
  
  stat  = prm_CreateUV(0, ntris, tris, uvf, npts, NULL, NULL, uv, xyz,
                       &per, &ppnts);
#ifdef DEBUG
  printf(" caps_paramQuilt: prm_CreateUV = %d  per = %d\n", stat, per);
#endif
  if (stat > 0) {
    n    = 2;
    stat = prm_SmoothUV(3, per, ppnts, ntris, tris, npts, 3, uv, xyzs);
#ifdef DEBUG
    printf(" caps_paraQuilt: prm_SmoothUV = %d\n", stat);
#endif
    if (stat == CAPS_SUCCESS) {
      n    = 3;
      stat = prm_NormalizeUV(0.05, per, npts, uv);
#ifdef DEBUG
      printf(" caps_paraQuilt: prm_NormalizeUV = %d\n", stat);
#endif
      if (stat == CAPS_SUCCESS) {
        n    = 4;
        nu   = 2*npts;
        nv   = 0;
        stat = prm_BestGrid(npts, 3, uv, xyzs, ntris, tris, tol, per, ppnts,
                            &nu, &nv, &grid, &rmserr, &maxerr, &dotmin);
        if (stat == PRM_TOLERANCEUNMET) {
          printf(" caps_paramQuilt: Tolerance not met: %lf (%lf)!\n",
                 maxerr, tol);
          stat = CAPS_SUCCESS;
        }
#ifdef DEBUG
        printf(" caps_paramQuilt: prm_BestGrid = %d  %d %d  %lf %lf (%lf)\n",
               stat, nu, nv, rmserr, maxerr, tol);
#endif
      }
    }
  }
  if (ppnts != NULL) EG_free(ppnts);
  EG_free(tris);
  EG_free(uvf);
  EG_free(xyz);
  EG_free(uv);
  if ((stat != CAPS_SUCCESS) || (grid == NULL)) {
    snprintf(line, l,
             "caps_paramQuilt: Create/Smooth/Normalize/BestGrid %d = %d!",
             n, stat);
    return stat;
  }
  
  /* make the surface approximation */
  
  surface = (capsAprx2D *) EG_alloc(sizeof(capsAprx2D));
  if (surface == NULL) {
    EG_free(grid);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on Surface!");
    return EGADS_MALLOC;
  }
  surface->nrank     = 3;
  surface->periodic  = per;
  surface->nus       = nu;
  surface->nvs       = nv;
  surface->urange[0] = 0.0;
  surface->urange[1] = nu-1;
  surface->vrange[0] = 0.0;
  surface->vrange[1] = nv-1;
  surface->num       = 0;
  surface->nvm       = 0;
  surface->uvmap     = NULL;
  surface->interp    = (double *) EG_alloc(3*4*nu*nv*sizeof(double));
  if (surface->interp == NULL) {
    EG_free(surface);
    EG_free(grid);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on Approx nu = %d, nv = %d",
             nu, nv);
    return EGADS_MALLOC;
  }
  n = nu;
  if (nv > nu) n = nv;
  r = (double *) EG_alloc(6*n*sizeof(double));
  if (r == NULL) {
    caps_Aprx2DFree(surface);
    EG_free(grid);
    snprintf(line, l,
             "caps_paramQuilt Error: Malloc on temp, size = %d", n);
    return EGADS_MALLOC;
  }
  stat = caps_fillCoeff2D(3, nu, nv, grid, surface->interp, r);
  EG_free(r);
  EG_free(grid);
  if (stat == 1) {
    caps_Aprx2DFree(surface);
    snprintf(line, l,
             "caps_paramQuilt Error: Failure in producing interpolant!");
    return CAPS_PARAMBNDERR;
  }
  bound->surface    = surface;
  bound->plimits[0] = bound->plimits[2] = 0.0;
  bound->plimits[1] = nu-1;
  bound->plimits[3] = nv-1;
  
  return CAPS_SUCCESS;
}


static int
caps_parameterize(capsProblem *problem, capsObject *bobject, int l, char *line)
{
  int           i, j, k, stat, last, top;
  char          *units;
  double        plims[4];
  ego           entity;
  capsBound     *bound;
  capsAnalysis  *analysis;
  capsVertexSet *vertexset;
  capsDiscr     *discr;
  bodyObjs      *bodies;
  
  bound = (capsBound *) bobject->blind;
  if ((bound->dim != 1) && (bound->dim != 2)) {
    snprintf(line, l, "caps_parameterize Error: Dimension = %d", bound->dim);
    return CAPS_BADINDEX;
  }
  bodies = (bodyObjs *) EG_alloc(problem->nBodies*sizeof(bodyObjs));
  if (bodies == NULL) {
    snprintf(line, l, "caps_parameterize Error: Allocating %d bodyObjs",
             problem->nBodies);
    return EGADS_MALLOC;
  }
  for (i = 0; i < problem->nBodies; i++) {
    bodies[i].n       = 0;
    bodies[i].gIndex  = 0;
    bodies[i].geom    = NULL;
    bodies[i].objs    = NULL;
    bodies[i].indices = NULL;
  }
  for (i = 0; i < problem->nBodies; i++) {
    if (bound->dim == 1) {
      stat = EG_getBodyTopos(problem->bodies[i], NULL, EDGE, &bodies[i].n,
                             &bodies[i].objs);
      if (stat != EGADS_SUCCESS) {
        snprintf(line, l,
                 "caps_parameterize Error: getBodyTopos EDGE for Body #%d",
                 i+1);
        caps_freeBodyObjs(problem->nBodies, bodies);
        return stat;
      }
    } else {
      stat = EG_getBodyTopos(problem->bodies[i], NULL, FACE, &bodies[i].n,
                             &bodies[i].objs);
      if (stat != EGADS_SUCCESS) {
        snprintf(line, l,
                 "caps_parameterize Error: getBodyTopos FACE for Body #%d",
                 i+1);
        caps_freeBodyObjs(problem->nBodies, bodies);
        return stat;
      }
    }
    if (bodies[i].n != 0) {
      bodies[i].indices = (int *) EG_alloc(bodies[i].n*sizeof(int));
      if (bodies[i].indices == NULL) {
        snprintf(line, l, "caps_parameterize Error: malloc %d ints for Body #%d",
                 bodies[i].n, i+1);
        caps_freeBodyObjs(problem->nBodies, bodies);
        return EGADS_MALLOC;
      }
      for (j = 0; j < bodies[i].n; j++) bodies[i].indices[j] = 0;
    }
  }

  for (i = 0; i < bound->nVertexSet; i++) {
    if (bound->vertexSet[i]              == NULL)      continue;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[i]->type        != VERTEXSET) continue;
    if (bound->vertexSet[i]->blind       == NULL)      continue;
    vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
    if (vertexset->analysis              == NULL)      continue;
    if (vertexset->discr                 == NULL)      continue;
    discr = vertexset->discr;
    if (discr->dim != bound->dim) {
      snprintf(line, l,
               "caps_parameterize Error: VertexSet %s Dimension = %d not %d",
               bound->vertexSet[i]->name, discr->dim, bound->dim);
      caps_freeBodyObjs(problem->nBodies, bodies);
      return CAPS_BADINDEX;
    }
    analysis = (capsAnalysis *) vertexset->analysis->blind;
    for (j = 0; j < discr->nElems; j++) {
      for (k = 0; k < problem->nBodies; k++)
        if (problem->bodies[k] == analysis->bodies[discr->elems[j].bIndex-1]) {
          bodies[k].indices[discr->elems[j].eIndex-1]++;
          break;
        }
    }
  }

  /* get the parameterization */
  entity            = NULL;
  bound->plimits[0] = bound->plimits[1] = bound->plimits[2] =
                      bound->plimits[3] = 0.0;

  /* do we have only a single Geometry entity -- in body? */
  for (last = i = 0; i < problem->nBodies; i++) {
    for (j = 0; j < bodies[i].n; j++) {
      if (bodies[i].indices[j] != 0) {
        if (bodies[i].gIndex == 0) {
          bodies[i].gIndex    = j+1;
          bodies[i].geom      = bodies[i].objs[j];
          plims[0] = plims[1] = plims[2] = plims[3] = 0.0;
          stat = EG_getRange(bodies[i].geom, plims, &top);
          if (stat != EGADS_SUCCESS) {
            snprintf(line, l,
                     "caps_parameterize Error: getRange for Body #%d %d",
                     i+1, bodies[i].gIndex);
            caps_freeBodyObjs(problem->nBodies, bodies);
            return stat;
          }
          if (last == 0) {
            bound->plimits[0] = plims[0];
            bound->plimits[1] = plims[1];
            bound->plimits[2] = plims[2];
            bound->plimits[3] = plims[3];
            entity            = bodies[i].geom;
            last++;
          } else {
            if (plims[0] < bound->plimits[0]) bound->plimits[0] = plims[0];
            if (plims[1] > bound->plimits[1]) bound->plimits[1] = plims[1];
            if (plims[2] < bound->plimits[2]) bound->plimits[2] = plims[2];
            if (plims[3] > bound->plimits[3]) bound->plimits[3] = plims[3];
          }
        } else {
          stat = EG_isSame(bodies[i].geom, bodies[i].objs[j]);
          if (stat < 0) {
            snprintf(line, l,
                     "caps_parameterize Error: isSame for Body #%d %d %d",
                     i+1, bodies[i].gIndex, j+1);
            caps_freeBodyObjs(problem->nBodies, bodies);
            return stat;
          }
          if (stat != EGADS_SUCCESS) {
            bodies[i].gIndex = -1;
            break;
          }
          plims[0] = plims[1] = plims[2] = plims[3] = 0.0;
          stat = EG_getRange(bodies[i].objs[j], plims, &top);
          if (stat != EGADS_SUCCESS) {
            snprintf(line, l,
                     "caps_parameterize Error: getRange for Body #%d %d",
                     i+1, j+1);
            caps_freeBodyObjs(problem->nBodies, bodies);
            return stat;
          }
          if (plims[0] < bound->plimits[0]) bound->plimits[0] = plims[0];
          if (plims[1] > bound->plimits[1]) bound->plimits[1] = plims[1];
          if (plims[2] < bound->plimits[2]) bound->plimits[2] = plims[2];
          if (plims[3] > bound->plimits[3]) bound->plimits[3] = plims[3];
        }
      }
    }
  }

  /* cross body */
  for (last = i = 0; i < problem->nBodies; i++) {
    if (bodies[i].gIndex ==  0) continue;
    if (bodies[i].gIndex == -1) {
      last = -1;
      break;
    }
    if (last == 0) {
      last = i+1;
    } else {
      stat = EG_isSame(bodies[last-1].geom, bodies[i].geom);
      if (stat < 0) {
        snprintf(line, l,
                 "caps_parameterize Error: isSame for Body #%d %d - #%d %d",
                 last, bodies[last-1].gIndex, i+1, bodies[i].gIndex);
        caps_freeBodyObjs(problem->nBodies, bodies);
        return stat;
      }
      if (stat != EGADS_SUCCESS) {
        last = -1;
        break;
      }
    }
  }
  if (last == 0) {
    bound->state = Empty;
    printf(" CAPS Info: No geometry for Bound -> %s!\n", bobject->name);
    caps_freeBodyObjs(problem->nBodies, bodies);
    return CAPS_SUCCESS;
  }

  /* single geometric entity */
  if (last != -1) {
    if (bound->lunits != NULL) EG_free(bound->lunits);
    bound->geom   = entity;
    bound->iBody  = last;
    bound->iEnt   = bodies[last-1].gIndex;
    bound->state  = Single;
    bound->lunits = EG_strdup(problem->lunits[last-1]);
    caps_freeBodyObjs(problem->nBodies, bodies);
    return CAPS_SUCCESS;
  }

  /* need to reparameterize */
  units = NULL;
  for (i = 0; i < problem->nBodies; i++) {
    if (bodies[i].gIndex == 0) continue;
    if (units == NULL) {
      units = problem->lunits[i];
      continue;
    }
    /* what do we do about units that don't match? */
    if (strcmp(problem->lunits[i], units) != 0) {
      printf(" CAPS Info: Units don't match for Bound -> %s -- %s %s\n",
             bobject->name, problem->lunits[i], units);
    }
  }
  if (bound->lunits != NULL) EG_free(bound->lunits);
  bound->geom   = NULL;
  bound->iBody  = 0;
  bound->iEnt   = 0;
  bound->state  = Multiple;
  bound->lunits = EG_strdup(units);
  caps_freeBodyObjs(problem->nBodies, bodies);

  if (bound->dim == 1) {
    
    stat = CAPS_SUCCESS;
    
  } else {
    stat = caps_paramQuilt(bound, l, line);
    if (stat == CAPS_SUCCESS)
      printf(" CAPS Info: Reparameterization Bound -> %s -- nu, nv = %d %d\n",
             bobject->name, bound->surface->nus, bound->surface->nvs);
  }
  if (stat != CAPS_SUCCESS) bound->state = MultipleError;
  
  return stat;
}


static int
caps_refillBound(capsProblem *problem, capsObject *bobject,
                 int *nErr, capsErrs **errors)
{
  int           i, j, k, m, n, irow, icol, status, len, OK;
  int           buildTo, builtTo, nbody, open;
  char          name[129], error[129], *str, **names, **ntmp;
  capsValue     *value;
  capsAnalysis  *anal;
  capsObject    *dobject;
  capsBound     *bound;
  capsVertexSet *vertexset;
  capsDataSet   *dataset;
  capsDiscr     *discr;
  capsErrs      *errs;
  capsOwn       *tmp;

  /* invalidate/cleanup any geometry dependencies & remake the bound */
  bound = (capsBound *) bobject->blind;
  for (j = 0; j < bound->nVertexSet; j++) {
    if (bound->vertexSet[j]              == NULL)      continue;
    if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[j]->type        != VERTEXSET) continue;
    if (bound->vertexSet[j]->blind       == NULL)      continue;
    if (bound->vertexSet[j]->last.sNum   >=
        problem->geometry.sNum)                        continue;
    vertexset = (capsVertexSet *) bound->vertexSet[j]->blind;
    for (k = 0; k < vertexset->nDataSets; k++) {
      if (vertexset->dataSets[k]              == NULL)      continue;
      if (vertexset->dataSets[k]->magicnumber != CAPSMAGIC) continue;
      if (vertexset->dataSets[k]->type        != DATASET)   continue;
      if (vertexset->dataSets[k]->blind       == NULL)      continue;
      dobject = vertexset->dataSets[k];
      dataset = (capsDataSet *) dobject->blind;
      if ((dataset->method == User) &&
          (strcmp(dobject->name, "xyz") == 0)) continue;
      if (dataset->data != NULL) EG_free(dataset->data);
      dataset->npts = 0;
      dataset->data = NULL;
    }
    if (vertexset->analysis != NULL)
      if (vertexset->analysis->blind != NULL) {
        anal = (capsAnalysis *) vertexset->analysis->blind;
        aim_FreeDiscr(problem->aimFPTR, anal->loadName, vertexset->discr);
        status = aim_Discr(problem->aimFPTR, anal->loadName,
                           bobject->name, vertexset->discr);
        if (status != CAPS_SUCCESS) {
          snprintf(error, 129, "Bound = %s and Analysis = %s",
                   bobject->name, anal->loadName);
          caps_makeSimpleErr(bound->vertexSet[j],
                             "caps_preAnalysis Error: aimDiscr fails!",
                             error, NULL, NULL, errors);
          if (*errors != NULL) {
            errs  = *errors;
            *nErr = errs->nError;
          }
          return status;
        } else {
          /* check the validity of the discretization just returned */
          status = caps_checkDiscr(vertexset->discr, 129, name);
          if (status != CAPS_SUCCESS) {
            snprintf(error, 129, "Bound = %s and Analysis = %s",
                     bobject->name, anal->loadName);
            caps_makeSimpleErr(bound->vertexSet[j], name, error,
                               NULL, NULL, errors);
            if (*errors != NULL) {
              errs  = *errors;
              *nErr = errs->nError;
            }
            aim_FreeDiscr(problem->aimFPTR, anal->loadName, vertexset->discr);
            return status;
          }
          caps_freeOwner(&bound->vertexSet[j]->last);
          bound->vertexSet[j]->last.sNum = problem->sNum;
          caps_fillDateTime(bound->vertexSet[j]->last.datetime);
        }
      }
  }
  /* reparameterize the existing bounds (dim=1&2) for multiple entities */
  if (bound->dim != 3) {
    status = caps_parameterize(problem, bobject, 129, name);
    if (status != CAPS_SUCCESS) {
      snprintf(error, 129, "Bound = %s", bobject->name);
      caps_makeSimpleErr(bobject,
                         "caps_preAnalysis: Bound Parameterization fails!",
                         error, NULL, NULL, errors);
      if (*errors != NULL) {
        errs  = *errors;
        *nErr = errs->nError;
      }
      return status;
    }
  }
  caps_freeOwner(&bobject->last);
  bobject->last.sNum = problem->sNum;
  caps_fillDateTime(bobject->last.datetime);

  /* populate any sensitivities in DataSets */
  names = NULL;
  for (n = j = 0; j < bound->nVertexSet; j++) {
    if (bound->vertexSet[j]              == NULL)      continue;
    if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[j]->type        != VERTEXSET) continue;
    if (bound->vertexSet[j]->blind       == NULL)      continue;
    vertexset = (capsVertexSet *) bound->vertexSet[j]->blind;
    if (vertexset->analysis              == NULL)      continue;
    if (vertexset->discr                 == NULL)      continue;
    discr = vertexset->discr;
    if (discr->nPoints                   == 0)         continue;
    if (discr->dim                       == 3)         continue;
    for (k = 0; k < vertexset->nDataSets; k++) {
      if (vertexset->dataSets[k]              == NULL)        continue;
      if (vertexset->dataSets[k]->magicnumber != CAPSMAGIC)   continue;
      if (vertexset->dataSets[k]->type        != DATASET)     continue;
      if (vertexset->dataSets[k]->blind       == NULL)        continue;
      dataset = (capsDataSet *) vertexset->dataSets[k]->blind;
      if (dataset->method                     != Sensitivity) continue;
      if (vertexset->dataSets[k]->last.sNum   >=
          problem->geometry.sNum)                             continue;
      if (names == NULL) {
        names = (char **) EG_alloc(sizeof(char *));
        if (names == NULL) continue;
        names[0] = vertexset->dataSets[k]->name;
        n++;
      } else {
        for (m = 0; m < n; m++)
          if (strcmp(names[m], vertexset->dataSets[k]->name) == 0) break;
        if (m == n) {
          ntmp = (char **) EG_reall(names, (n+1)*sizeof(char *));
          if (ntmp == NULL) continue;
          names    = ntmp;
          names[n] = vertexset->dataSets[k]->name;
          n++;
        }
      }
    }
  }
  if (names != NULL) {
    
    /* go through the list of sensitivity names */
    for (m = 0; m < n; m++) {
      irow = icol = 1;
      open = 0;
      str  = EG_strdup(names[m]);
      if (str == NULL) continue;
      len = strlen(str);
      for (j = 1; j < len; j++)
        if (str[j] == '[') {
          open = j;
          break;
        }
      if (open != 0) {
        str[open] = 0;
        for (i = open+1; i < len; i++)
          if (str[i] == ',') str[i] = ' ';
        sscanf(&str[open+1], "%d%d\n", &irow, &icol);
      }
      open = -1;
      for (i = 0; i < problem->nGeomIn; i++)
        if (strcmp(str,problem->geomIn[i]->name) == 0) {
          value = (capsValue *) problem->geomIn[i]->blind;
          if (value == NULL) continue;
          open  = value->pIndex;
          break;
        }
      EG_free(str);
      if (open == -1) continue;
      
      /* clear all then set */
      ocsmSetVelD(problem->modl, 0,    0,    0,    0.0);
      ocsmSetVelD(problem->modl, open, irow, icol, 1.0);
      buildTo = 0;
      nbody   = 0;
      status  = ocsmBuild(problem->modl, buildTo, &builtTo, &nbody, NULL);
      printf(" parameter %d %d %d sensitivity status = %d\n",
             open, irow, icol, status);
      if (status != SUCCESS) continue;
      
      for (j = 0; j < bound->nVertexSet; j++) {
        if (bound->vertexSet[j]              == NULL)      continue;
        if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
        if (bound->vertexSet[j]->type        != VERTEXSET) continue;
        if (bound->vertexSet[j]->blind       == NULL)      continue;
        vertexset = (capsVertexSet *) bound->vertexSet[j]->blind;
        if (vertexset->analysis              == NULL)      continue;
        if (vertexset->discr                 == NULL)      continue;
        discr = vertexset->discr;
        if (discr->nPoints                   == 0)         continue;
        if (discr->dim                       == 3)         continue;
        for (k = 0; k < vertexset->nDataSets; k++) {
          if (vertexset->dataSets[k]              == NULL)        continue;
          if (vertexset->dataSets[k]->magicnumber != CAPSMAGIC)   continue;
          if (vertexset->dataSets[k]->type        != DATASET)     continue;
          if (vertexset->dataSets[k]->blind       == NULL)        continue;
          dobject = vertexset->dataSets[k];
          dataset = (capsDataSet *) dobject->blind;
          if (dataset->method                     != Sensitivity) continue;
          if (vertexset->dataSets[k]->last.sNum   >=
              problem->geometry.sNum)                             continue;
          if (strcmp(names[m],dobject->name)      != 0)           continue;
          dataset->data = (double *)
          EG_alloc(3*discr->nPoints*sizeof(double));
          if (dataset->data                       == NULL)        continue;
          caps_fillSensit(problem, discr, dataset);
          dataset->npts = discr->nPoints;
          OK = 1;
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
          vertexset->dataSets[k]->last.sNum = problem->sNum;
          caps_fillDateTime(dobject->last.datetime);
        }
      }
    }
    EG_free(names);
  }

  return CAPS_SUCCESS;
}


static int
caps_toktokcmp(const char *str1, const char *str2)
{
  char copy1[1025], copy2[1025], *word1, *word2, *last1, *last2, sep[3] = " ;";
  
  /* do a token/token compare against 2 strings */
  strncpy(copy1, str1, 1024);
  copy1[1024] = 0;
/*@-unrecog@*/
  word1 = strtok_r(copy1, sep, &last1);
  while (word1 != NULL) {
    strncpy(copy2, str2, 1024);
    copy2[1024] = 0;
    word2 = strtok_r(copy2, sep, &last2);
    while (word2 != NULL) {
      if (strcmp(word2, word1) == 0) return 0;
      word2 = strtok_r(NULL, sep, &last2);
    }
    word1 = strtok_r(NULL, sep, &last1);
  }
/*@+unrecog@*/

  return 1;
}


int
caps_filter(capsProblem *problem, capsAnalysis *analysis)
{
  int          i, nBody, alen, atype, status;
  ego          *bodies;
  const int    *aints;
  const double *areals;
  const char   *astr;

  bodies = (ego *) EG_alloc(2*problem->nBodies*sizeof(ego));
  if (bodies == NULL) return EGADS_MALLOC;
  for (i = 0; i < 2*problem->nBodies; i++) bodies[i] = NULL;
  
  /* do any bodies have our "capsAIM" attribute? */
  for (nBody = i = 0; i < problem->nBodies; i++) {
    status = EG_attributeRet(problem->bodies[i], "capsAIM", &atype,
                             &alen, &aints, &areals, &astr);
    if (status != EGADS_SUCCESS) continue;
    if (atype  != ATTRSTRING)    continue;
    if (caps_toktokcmp(analysis->loadName, astr) != 0) continue;
    if (analysis->intents != NULL) {
      status = EG_attributeRet(problem->bodies[i], "capsIntent", &atype,
                               &alen, &aints, &areals, &astr);
      if (status != EGADS_SUCCESS) continue;
      if (atype  != ATTRSTRING)    continue;
      if (caps_toktokcmp(analysis->intents, astr) != 0) continue;
    }
    bodies[nBody] = problem->bodies[i];
    nBody++;
  }

  if (nBody == 0)
    if (analysis->intents == NULL) {
      printf(" caps_filter Warning: No bodies with capsAIM = %s!\n",
             analysis->loadName);
    } else {
      printf(" caps_filter Warning: No bodies with capsAIM = %s and capsIntent = %s!\n",
             analysis->loadName, analysis->intents);
    }
  analysis->bodies = bodies;
  analysis->nBody  = nBody;
  return CAPS_SUCCESS;
}


int
caps_preAnalysis(capsObject *aobject, int *nErr, capsErrs **errors)
{
  int              i, j, k, m, n, irow, icol, stat, status, gstatus;
  int              type, nrow, ncol, buildTo, builtTo, nbody, ibody;
  char             name[MAX_NAME_LEN], error[129], vstr[MAX_STRVAL_LEN], *units;
  double           dot, *values;
  enum capstMethod method;
  CAPSLONG         sn;
  modl_T           *MODL;
  capsValue        *valIn, *value;
  capsProblem      *problem;
  capsAnalysis     *analysis, *analy;
  capsObject       *pobject, *source, *object, *last;
  capsErrs         *errs;
  
  *nErr    = 0;
  *errors  = NULL;
  analysis = NULL;
  valIn    = NULL;
  if (aobject              == NULL)      return CAPS_NULLOBJ;
  if (aobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aobject->type        == PROBLEM) {
    if (aobject->blind     == NULL)      return CAPS_NULLBLIND;
    pobject  = aobject;
    problem  = (capsProblem *)  pobject->blind;
  } else {
    if (aobject->type      != ANALYSIS)  return CAPS_BADTYPE;
    if (aobject->blind     == NULL)      return CAPS_NULLBLIND;
    analysis = (capsAnalysis *) aobject->blind;
    if (aobject->parent    == NULL)      return CAPS_NULLOBJ;
    pobject  = (capsObject *)   aobject->parent;
    if (pobject->blind     == NULL)      return CAPS_NULLBLIND;
    problem  = (capsProblem *)  pobject->blind;
    valIn    = (capsValue *)    analysis->analysisIn[0]->blind;
    if (valIn              == NULL)      return CAPS_NULLVALUE;
  }

  /* do we need new geometry? */
  gstatus = 0;
  if (pobject->subtype == PARAMETRIC) {
    /* check for dirty geometry inputs */
    for (i = 0; i < problem->nGeomIn; i++) {
      source = object = problem->geomIn[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last->last.sNum > problem->geometry.sNum) {
        gstatus = 1;
        break;
      }
    }
    /* check for dirty branchs */
    for (i = 0; i < problem->nBranch; i++) {
      source = object = problem->branchs[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last->last.sNum > problem->geometry.sNum) {
        gstatus = 1;
        break;
      }
    }
  }
  if ((analysis == NULL) && (gstatus == 0)) return CAPS_CLEAN;

  /* are we "analysis" clean? */
  status = 0;
  if (analysis != NULL) {
    if (analysis->pre.sNum == 0) {
      status = 1;
    } else {
      /* check for dirty analysis inputs */
      for (i = 0; i < analysis->nAnalysisIn; i++) {
        source = object = analysis->analysisIn[i];
        do {
          if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
          if (source->type        != VALUE)     return CAPS_BADTYPE;
          if (source->blind       == NULL)      return CAPS_NULLBLIND;
          value = (capsValue *) source->blind;
          if (value->link == object)            return CAPS_CIRCULARLINK;
          last   = source;
          source = value->link;
        } while (value->link != NULL);
        if (last->last.sNum > analysis->pre.sNum) {
          status = 1;
          break;
        }
      }
      if (status == 0) {
        stat = caps_snDataSets(aobject, 0, &sn);
        if (stat == CAPS_SUCCESS)
          if (sn > analysis->pre.sNum) status = 1;
      }
    }
    if ((status == 0) && (gstatus == 0) &&
        (problem->geometry.sNum < analysis->pre.sNum)) return CAPS_CLEAN;
    
    /* fill in any values that have links */
    for (i = 0; i < analysis->nAnalysisIn; i++) {
      source = object = analysis->analysisIn[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last != object) {
        value  = (capsValue *) object->blind;
        source = value->link;
        method = value->linkMethod;
        status = caps_transferValues(last, method, object, nErr, errors);
        value->link       = source;
        value->linkMethod = method;
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Info: transferValues for %s from %s = %d\n",
                 object->name, source->name, status);
          return status;
        }
        caps_freeOwner(&object->last);
        object->last       = last->last;
        object->last.pname = EG_strdup(last->last.pname);
        object->last.pID   = EG_strdup(last->last.pID);
        object->last.user  = EG_strdup(last->last.user);
      }
    }
  }

  /* generate new geometry if required */
  if (gstatus == 1) {
    MODL          = (modl_T *) problem->modl;
    MODL->context = problem->context;

    /* update the dirty values in OpenCSM */
    for (i = 0; i < problem->nGeomIn; i++) {
      source = object = problem->geomIn[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last != object) {
        value  = (capsValue *) object->blind;
        source = value->link;
        method = value->linkMethod;
        status = caps_transferValues(last, method, object, nErr, errors);
        value->link       = source;
        value->linkMethod = method;
        if (status != CAPS_SUCCESS) return status;
        caps_freeOwner(&object->last);
        object->last       = last->last;
        object->last.pname = EG_strdup(last->last.pname);
        object->last.pID   = EG_strdup(last->last.pID);
        object->last.user  = EG_strdup(last->last.user);
      }
      if (object->last.sNum > problem->geometry.sNum) {
        value  = (capsValue *) object->blind;
        if (value       == NULL)   return CAPS_NULLVALUE;
        if (value->type != Double) return CAPS_BADTYPE;
        values = value->vals.reals;
        if (value->length == 1) values = &value->vals.real;
        irow   = value->nrow;
        icol   = value->ncol;
        status = ocsmGetPmtr(problem->modl, value->pIndex, &type,
                             &nrow, &ncol, name);
        if (status != SUCCESS) {
          snprintf(error, 129, "Cannot get info on %s", object->name);
          caps_makeSimpleErr(object,
                             "caps_preAnalysis Error: ocsmGetPmtr fails!",
                             error, NULL, NULL, errors);
          if (*errors != NULL) {
            errs  = *errors;
            *nErr = errs->nError;
          }
          return status;
        }
        if ((nrow != irow) || (ncol != icol)) {
          snprintf(error, 129, "nrow = %d irow = %d  ncol = %d icol = %d on %s",
                   nrow, irow, ncol, icol, object->name);
          caps_makeSimpleErr(object,
                             "caps_preAnalysis Error: shape problem!",
                             error, NULL, NULL, errors);
          if (*errors != NULL) {
            errs  = *errors;
            *nErr = errs->nError;
          }
          return CAPS_SHAPEERR;
        }
        /* flip storage
        for (n = j = 0; j < ncol; j++)
          for (k = 0; k < nrow; k++, n++) {  */
        for (n = k = 0; k < nrow; k++)
          for (j = 0; j < ncol; j++, n++) {
            status = ocsmSetValuD(problem->modl, value->pIndex,
                                  k+1, j+1, values[n]);
            if (status != SUCCESS) {
              snprintf(error, 129, "Cannot change %s[%d,%d] to %lf!",
                       object->name, k+1, j+1, values[n]);
              caps_makeSimpleErr(object,
                                 "caps_preAnalysis Error: ocsmSetValuD fails!",
                                 error, NULL, NULL, errors);
              if (*errors != NULL) {
                errs  = *errors;
                *nErr = errs->nError;
              }
              return status;
            }
          }
      }
    }
    /* do the branches */
    for (i = 0; i < problem->nBranch; i++) {
      source = object = problem->branchs[i];
      do {
        if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
        if (source->type        != VALUE)     return CAPS_BADTYPE;
        if (source->blind       == NULL)      return CAPS_NULLBLIND;
        value = (capsValue *) source->blind;
        if (value->link == object)            return CAPS_CIRCULARLINK;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last != object) {
        value  = (capsValue *) object->blind;
        source = value->link;
        method = value->linkMethod;
        status = caps_transferValues(last, method, object, nErr, errors);
        value->link       = source;
        value->linkMethod = method;
        if (status != CAPS_SUCCESS) return status;
        caps_freeOwner(&object->last);
        object->last       = last->last;
        object->last.pname = EG_strdup(last->last.pname);
        object->last.pID   = EG_strdup(last->last.pID);
        object->last.user  = EG_strdup(last->last.user);
      }
      if (object->last.sNum > problem->geometry.sNum) {
        value = (capsValue *) object->blind;
        if (value         == NULL)    return CAPS_NULLVALUE;
        if (value->type   != Integer) return CAPS_BADTYPE;
        if (value->length != 1)       return CAPS_BADVALUE;
        status = ocsmSetBrch(problem->modl, i+1, value->vals.integer);
        if ((status != SUCCESS) && (status != OCSM_CANNOT_BE_SUPPRESSED)) {
          snprintf(error, 129, "Cannot change %d Branch %s to %d!",
                   i+1, object->name, value->vals.integer);
          caps_makeSimpleErr(object,
                             "caps_preAnalysis Error: ocsmSetBrch fails!",
                             error, NULL, NULL, errors);
          if (*errors != NULL) {
            errs  = *errors;
            *nErr = errs->nError;
          }
          return status;
        }
      }
    }

    /* have OpenCSM do the rebuild */
    if (problem->bodies != NULL) {
      for (i = 0; i < problem->nBodies; i++)
        if (problem->lunits[i] != NULL) EG_free(problem->lunits[i]);
      /* remove old bodies & tessellations for all Analyses */
      for (i = 0; i < problem->nAnalysis; i++) {
        analy = (capsAnalysis *) problem->analysis[i]->blind;
        if (analy == NULL) continue;
        if (analy->bodies != NULL) {
          for (j = 0; j < analy->nBody; j++)
            if (analy->bodies[j+analy->nBody] != NULL) {
              EG_deleteObject(analy->bodies[j+analy->nBody]);
              analy->bodies[j+analy->nBody] = NULL;
            }
          EG_free(analy->bodies);
          analy->bodies = NULL;
          analy->nBody  = 0;
        }
      }
      EG_free(problem->bodies);
      EG_free(problem->lunits);
      problem->nBodies       = 0;
      problem->bodies        = NULL;
      problem->lunits        = NULL;
      problem->geometry.sNum = 0;
    }
    buildTo = 0;                          /* all */
    nbody   = 0;
    status  = ocsmBuild(problem->modl, buildTo, &builtTo, &nbody, NULL);
    if (status != SUCCESS) {
      caps_makeSimpleErr(pobject,
                         "caps_preAnalysis Error: ocsmBuild fails!",
                         NULL, NULL, NULL, errors);
      if (*errors != NULL) {
        errs  = *errors;
        *nErr = errs->nError;
      }
      return status;
    }
    nbody = 0;
    for (ibody = 1; ibody <= MODL->nbody; ibody++) {
      if (MODL->body[ibody].onstack != 1) continue;
      if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
      nbody++;
    }

    units = NULL;
    if (nbody > 0) {
      problem->lunits = (char **) EG_alloc(nbody*sizeof(char *));
      problem->bodies = (ego *)   EG_alloc(nbody*sizeof(ego));
      if ((problem->bodies == NULL) || (problem->lunits == NULL)) {
        if (problem->bodies != NULL) EG_free(problem->bodies);
        if (problem->lunits != NULL) EG_free(problem->lunits);
        for (ibody = 1; ibody <= MODL->nbody; ibody++) {
          if (MODL->body[ibody].onstack != 1) continue;
          if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
          EG_deleteObject(MODL->body[ibody].ebody);
        }
        caps_makeSimpleErr(aobject,
                           "caps_preAnalysis: Error on Body memory allocation!",
                           NULL, NULL, NULL, errors);
        if (*errors != NULL) {
          errs  = *errors;
          *nErr = errs->nError;
        }
        return EGADS_MALLOC;
      }
      problem->nBodies = nbody;
      i = 0;
      for (ibody = 1; ibody <= MODL->nbody; ibody++) {
        if (MODL->body[ibody].onstack != 1) continue;
        if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
        problem->bodies[i] = MODL->body[ibody].ebody;
        caps_fillLengthUnits(problem, problem->bodies[i], &problem->lunits[i]);
        i++;
      }
      units = problem->lunits[nbody-1];
    }
    caps_freeOwner(&problem->geometry);
    problem->sNum         += 1;
    problem->geometry.sNum = problem->sNum;
    caps_fillDateTime(problem->geometry.datetime);
    
    /* get geometry outputs */
    for (i = 0; i < problem->nGeomOut; i++) {
      if (problem->geomOut[i]->magicnumber != CAPSMAGIC) continue;
      if (problem->geomOut[i]->type        != VALUE)     continue;
      if (problem->geomOut[i]->blind       == NULL)      continue;
      value = (capsValue *) problem->geomOut[i]->blind;
      if (value->type == String) {
        if (value->vals.string != NULL) EG_free(value->vals.string);
        value->vals.string = NULL;
      } else {
        if (value->length != 1)
          if (value->vals.reals != NULL) EG_free(value->vals.reals);
        value->vals.reals = NULL;
      }
      status = ocsmGetPmtr(problem->modl, value->pIndex, &type, &nrow, &ncol,
                           name);
      if (status != SUCCESS) {
        snprintf(error, 129, "Cannot get info on Output %s",
                 problem->geomOut[i]->name);
        caps_makeSimpleErr(problem->geomOut[i],
                           "caps_preAnalysis Error: ocsmGetPmtr fails!",
                           error, NULL, NULL, errors);
        if (*errors != NULL) {
          errs  = *errors;
          *nErr = errs->nError;
        }
        return status;
      }
      if (strcmp(name,problem->geomOut[i]->name) != 0) {
        snprintf(error, 129, "Cannot Geom Output[%d] %s != %s",
                 i, problem->geomOut[i]->name, name);
        caps_makeSimpleErr(problem->geomOut[i],
                           "caps_preAnalysis Error: ocsmGetPmtr MisMatch!",
                           error, NULL, NULL, errors);
        if (*errors != NULL) {
          errs  = *errors;
          *nErr = errs->nError;
        }
        return CAPS_MISMATCH;
      }
      if ((nrow == 0) || (ncol == 0)) {
        status = ocsmGetValuS(problem->modl, value->pIndex, vstr);
        if (status != SUCCESS) {
          snprintf(error, 129, "Cannot get string on Output %s",
                   problem->geomOut[i]->name);
          caps_makeSimpleErr(problem->geomOut[i],
                             "caps_preAnalysis Error: ocsmGetValuSfails!",
                             error, NULL, NULL, errors);
          if (*errors != NULL) {
            errs  = *errors;
            *nErr = errs->nError;
          }
          return status;
        }
        value->nullVal     = NotNull;
        value->type        = String;
        value->length      = 1;
        value->nrow        = 1;
        value->ncol        = 1;
        value->dim         = Scalar;
        value->vals.string = EG_strdup(vstr);
        if (value->vals.string == NULL) value->nullVal = IsNull;
      } else {
        value->nullVal = NotNull;
        value->type    = Double;
        value->nrow    = nrow;
        value->ncol    = ncol;
        value->length  = nrow*ncol;
        value->dim     = Scalar;
        if ((nrow > 1) || (ncol > 1)) value->dim = Vector;
        if ((nrow > 1) && (ncol > 1)) value->dim = Array2D;
        if (value->length == 1) {
          values = &value->vals.real;
        } else {
          values = (double *) EG_alloc(value->length*sizeof(double));
          if (values == NULL) {
            value->nullVal = IsNull;
            snprintf(error, 129, "length = %d doubles for %s",
                     value->length, problem->geomOut[i]->name);
            caps_makeSimpleErr(problem->geomOut[i],
                               "caps_preAnalysis Error: Memory Allocation fails!",
                               error, NULL, NULL, errors);
            if (*errors != NULL) {
              errs  = *errors;
              *nErr = errs->nError;
            }
            return EGADS_MALLOC;
          }
          value->vals.reals = values;
        }
        /* flip storage
         for (n = m = j = 0; j < ncol; j++)
         for (k = 0; k < nrow; k++, n++) {  */
        for (n = m = k = 0; k < nrow; k++)
          for (j = 0; j < ncol; j++, n++) {
            status = ocsmGetValu(problem->modl, value->pIndex, k+1, j+1,
                                 &values[n], &dot);
            if (status != SUCCESS) {
              snprintf(error, 129, "irow = %d icol = %d on %s",
                       k+1, j+1, problem->geomOut[i]->name);
              caps_makeSimpleErr(problem->geomOut[i],
                                 "caps_preAnalysis Error: Output ocsmGetValu fails!",
                                 error, NULL, NULL, errors);
              if (*errors != NULL) {
                errs  = *errors;
                *nErr = errs->nError;
              }
              return status;
            }
            if (values[n] == -HUGEQ) m++;
          }
        if (m != 0) value->nullVal = IsNull;
      }
      
      if (value->units != NULL) EG_free(value->units);
      value->units = NULL;
      caps_geomOutUnits(name, units, &value->units);

      caps_freeOwner(&problem->geomOut[i]->last);
      problem->geomOut[i]->last.sNum = problem->sNum;
      caps_fillDateTime(problem->geomOut[i]->last.datetime);
    }
  }
  
  if (analysis == NULL) {
    if (problem->nBodies == 0)
      printf(" caps_preAnalysis Warning: No bodies generated!\n");
    return CAPS_SUCCESS;
  }
  
  if ((problem->nBodies <= 0) || (problem->bodies == NULL)) {
    printf(" caps_preAnalysis Warning: No bodies to %s!\n", analysis->loadName);
    if (analysis->bodies != NULL) EG_free(analysis->bodies);
    analysis->bodies = NULL;
    analysis->nBody  = 0;
  } else if (analysis->bodies == NULL) {
    status = caps_filter(problem, analysis);
    if (status != CAPS_SUCCESS) return status;
  }
  
  /* do it! */
  status = aim_PreAnalysis(problem->aimFPTR, analysis->loadName,
                           analysis->instance, &analysis->info, analysis->path,
                           valIn, errors);
  if (*errors != NULL) {
    errs  = *errors;
    *nErr = errs->nError;
    for (i = 0; i < errs->nError; i++) {
      errs->errors[i].errObj = NULL;
      if ((errs->errors[i].index < 1) ||
          (errs->errors[i].index > analysis->nAnalysisIn)) {
        printf(" caps_preAnalysis Warning: Bad Index %d for %s!\n",
               errs->errors[i].index, analysis->loadName);
        continue;
      }
      errs->errors[i].errObj = analysis->analysisIn[errs->errors[i].index-1];
    }
  }
  if (status == CAPS_SUCCESS) {
    caps_freeOwner(&analysis->pre);
    problem->sNum     += 1;
    analysis->pre.sNum = problem->sNum;
    caps_fillDateTime(analysis->pre.datetime);
  }

  return status;
}


static int
caps_fillAnaLinkages(capsProblem *problem, capsAnalysis *analysis, int nObj,
                     capsObject **objs, int *nErr, capsErrs **errors)
{
  int        i, j, k, status;
  capsObject *source, *last;
  capsValue  *value, *valu0;
  capsErrs   *errs;
  
  for (i = 0; i < nObj; i++) {
    source = objs[i];
    do {
      if (source->magicnumber != CAPSMAGIC)  return CAPS_BADOBJECT;
      if (source->type        != VALUE)      return CAPS_BADTYPE;
      if (source->blind       == NULL)       return CAPS_NULLBLIND;
      value = (capsValue *) source->blind;
      if (value->link == objs[i])            return CAPS_CIRCULARLINK;
      last   = source;
      source = value->link;
    } while (value->link != NULL);
    if (last != objs[i]) {
      valu0 = (capsValue *) analysis->analysisOut[0]->blind;
      value = (capsValue *) last->blind;
      j = value - valu0;
      if ((j >= 0) && (j < analysis->nAnalysisOut)) {
        if (analysis->analysisOut[j]->last.sNum <= problem->sNum) {
          value = (capsValue *) analysis->analysisOut[j]->blind;
          if ((value->type == Boolean) || (value->type == Integer)) {
            if (value->length > 1) {
              EG_free(value->vals.integers);
              value->vals.integers = NULL;
            }
          } else if (value->type == Double) {
            if (value->length > 1) {
              EG_free(value->vals.reals);
              value->vals.reals = NULL;
            }
          } else if (value->type == String) {
            if (value->length > 1) {
              EG_free(value->vals.string);
              value->vals.string = NULL;
            }
          } else if (value->type == Tuple) {
            caps_freeTuple(value->length, value->vals.tuple);
          } else {
            return CAPS_BADTYPE;
          }
          caps_freeOwner(&analysis->analysisOut[j]->last);
          analysis->analysisOut[j]->last.sNum = 0;
          status = aim_CalcOutput(problem->aimFPTR, analysis->loadName,
                                  analysis->instance, &analysis->info,
                                  analysis->path, j+1, value, errors);
          if (*errors != NULL) {
            errs  = *errors;
            *nErr = errs->nError;
            for (k = 0; k < errs->nError; k++)
              errs->errors[k].errObj = analysis->analysisOut[j];
          }
          if (status != CAPS_SUCCESS) return status;
          analysis->analysisOut[j]->last.sNum = problem->sNum + 1;
          caps_fillDateTime(analysis->analysisOut[j]->last.datetime);
        }
      }
    }
  }
  
  return CAPS_SUCCESS;
}


static void
caps_fillAnaErrs(capsProblem *problem, capsAnalysis *analysis)
{
  int       i;
  capsValue *value;
  
  for (i = 0; i < analysis->nAnalysisOut; i++) {
    if (analysis->analysisOut[i]->last.sNum != problem->sNum + 1) continue;
    value = (capsValue *) analysis->analysisOut[i]->blind;
    if ((value->type == Boolean) || (value->type == Integer)) {
      if (value->length > 1) {
        EG_free(value->vals.integers);
        value->vals.integers = NULL;
      }
    } else if (value->type == Double) {
      if (value->length > 1) {
        EG_free(value->vals.reals);
        value->vals.reals = NULL;
      }
    } else if (value->type == String) {
      if (value->length > 1) {
        EG_free(value->vals.string);
        value->vals.string = NULL;
      }
    } else if (value->type == Tuple) {
      caps_freeTuple(value->length, value->vals.tuple);
    }
    caps_freeOwner(&analysis->analysisOut[i]->last);
    analysis->analysisOut[i]->last.sNum = 0;
  }
}


static int
caps_boundDependent(capsProblem *problem, capsObject *aobject,
                    capsObject  *oobject)
{
  int           i, j, jj, k, kk;
  capsBound     *bound;
  capsVertexSet *vs, *vso;
  capsDataSet   *ds, *dso;

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
        for (jj = 0; jj < bound->nVertexSet; jj++) {
          if (j == jj) continue;
          if (bound->vertexSet[jj]              == NULL)       continue;
          if (bound->vertexSet[jj]->magicnumber != CAPSMAGIC)  continue;
          if (bound->vertexSet[jj]->type        != VERTEXSET)  continue;
          if (bound->vertexSet[jj]->blind       == NULL)       continue;
          vso = (capsVertexSet *) bound->vertexSet[jj]->blind;
          if (vso->analysis                     != oobject)    continue;
          for (kk = 0; kk < vso->nDataSets; kk++) {
            if (vso->dataSets[kk]                == NULL)      continue;
            if (vso->dataSets[kk]->magicnumber   != CAPSMAGIC) continue;
            if (vso->dataSets[kk]->type          != DATASET)   continue;
            if (vso->dataSets[kk]->blind         == NULL)      continue;
            dso = (capsDataSet *) vso->dataSets[kk]->blind;
            if (dso->method                      != Analysis)  continue;
            if (strcmp(vs->dataSets[k]->name,vso->dataSets[kk]->name) == 0)
              return CAPS_SUCCESS;
          }
        }
      }
    }
  }
  
  return CAPS_NOTFOUND;
}


int
caps_postAnalysis(capsObject *aobject, capsOwn current, int *nErr,
                  capsErrs **errors)
{
  int           i, j, k, OK, nparent, nField, status, exec, dirty, deferred;
  int           *ranks;
  char          name[129], error[129], *intents, *apath, *unitSys, **fnames;
  capsProblem   *problem;
  capsAnalysis  *analysis, *other;
  capsObject    *pobject, *bobject, *object, *dso, **parents;
  capsBound     *bound;
  capsVertexSet *vs;
  capsDataSet   *ds;
  capsOwn       *tmp;
  capsErrs      *errs;
  ut_unit       *utunit;
  
  *nErr   = 0;
  *errors = NULL;
  if (aobject              == NULL)            return CAPS_NULLOBJ;
  if (aobject->magicnumber != CAPSMAGIC)       return CAPS_BADOBJECT;
  if (aobject->type        != ANALYSIS)        return CAPS_BADTYPE;
  if (aobject->blind       == NULL)            return CAPS_NULLBLIND;
  analysis = (capsAnalysis *) aobject->blind;
  if (aobject->parent      == NULL)            return CAPS_NULLOBJ;
  pobject  = (capsObject *)   aobject->parent;
  if (pobject->blind       == NULL)            return CAPS_NULLBLIND;
  problem  = (capsProblem *)  pobject->blind;
  
  /* check to see if we need to do post */
  status = caps_analysisInfo(aobject, &apath, &unitSys, &intents, &nparent,
                             &parents, &nField, &fnames, &ranks, &exec, &dirty);
  if (status != CAPS_SUCCESS) return status;
  if (dirty == 0) return CAPS_CLEAN;
  if (dirty <  5) return CAPS_DIRTY;

  /* call post in the AIM */
  status = aim_PostAnalysis(problem->aimFPTR, analysis->loadName,
                            analysis->instance, &analysis->info, analysis->path,
                            errors);
  if (status != CAPS_SUCCESS) {
    if (*errors != NULL) *nErr = (*errors)->nError;
    return status;
  }
  
  /* look for linkages to our outputs */
  status = caps_fillAnaLinkages(problem, analysis, problem->nParam,
                                problem->params, nErr, errors);
  if (status != CAPS_SUCCESS) {
    caps_fillAnaErrs(problem, analysis);
    return status;
  }
  status = caps_fillAnaLinkages(problem, analysis, problem->nGeomIn,
                                problem->geomIn, nErr, errors);
  if (status != CAPS_SUCCESS) {
    caps_fillAnaErrs(problem, analysis);
    return status;
  }
  for (i = 0; i < problem->nAnalysis; i++) {
    if (problem->analysis[i] == aobject) continue;
    if (problem->analysis[i] == NULL) {
      caps_fillAnaErrs(problem, analysis);
      return CAPS_NULLOBJ;
    }
    other  = (capsAnalysis *) problem->analysis[i]->blind;
    if (other == NULL) {
      caps_fillAnaErrs(problem, analysis);
      return CAPS_NULLBLIND;
    }
    status = caps_fillAnaLinkages(problem, analysis, other->nAnalysisIn,
                                  other->analysisIn, nErr, errors);
    if (status != CAPS_SUCCESS) {
      caps_fillAnaErrs(problem, analysis);
      return status;
    }
  }
  
  /* deal with any bounds dependent on this analysis that can be updated */
  for (i = 0; i < problem->nBound; i++) {
    if (problem->bounds[i]              == NULL)         continue;
    if (problem->bounds[i]->magicnumber != CAPSMAGIC)    continue;
    if (problem->bounds[i]->type        != BOUND)        continue;
    if (problem->bounds[i]->blind       == NULL)         continue;
    bobject  = problem->bounds[i];
    bound    = (capsBound *) bobject->blind;
    deferred = 0;
    if (bobject->last.sNum < problem->geometry.sNum) {
      deferred = 1;
      for (OK = k = j = 0; j < bound->nVertexSet; j++) {
        if (bound->vertexSet[j]              == NULL)      continue;
        if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
        if (bound->vertexSet[j]->type        != VERTEXSET) continue;
        if (bound->vertexSet[j]->blind       == NULL)      continue;
        vs     = (capsVertexSet *) bound->vertexSet[j]->blind;
        object = vs->analysis;
        if (object                           == NULL)      continue;
        if (object == aobject) {
          k = 1;
        } else {
          if (object->last.sNum < problem->geometry.sNum) OK++;
        }
      }
      if ((k == 0) || (OK != 0)) continue;
/*    printf(" *** Bound %s -- good to go! ***\n", bobject->name);  */

      /* bring the bound up-to-date */
      status = caps_refillBound(problem, bobject, nErr, errors);
      if (status != CAPS_SUCCESS) return status;
    }
    
    /* populate any built-in DataSet entries */
    for (j = 0; j < bound->nVertexSet; j++) {
      if (bound->vertexSet[j]              == NULL)      continue;
      if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[j]->type        != VERTEXSET) continue;
      if (bound->vertexSet[j]->blind       == NULL)      continue;
      vs    = (capsVertexSet *) bound->vertexSet[j]->blind;
      if ((deferred == 0) && (vs->analysis != aobject))  continue;
      other = (capsAnalysis *)  vs->analysis->blind;
      if (other                            == NULL)      continue;
      if (bound->vertexSet[j]->last.sNum < problem->geometry.sNum) {
        aim_FreeDiscr(problem->aimFPTR, other->loadName, vs->discr);
        status = aim_Discr(problem->aimFPTR, other->loadName,
                           bobject->name, vs->discr);
        if (status != CAPS_SUCCESS) {
          snprintf(error, 129, "Bound = %s and Analysis = %s",
                   bobject->name, other->loadName);
          caps_makeSimpleErr(bound->vertexSet[j],
                             "caps_postAnalysis Error: aimDiscr fails!",
                             error, NULL, NULL, errors);
          if (*errors != NULL) {
            errs  = *errors;
            *nErr = errs->nError;
          }
          return status;
        } else {
          /* check the validity of the discretization just returned */
          status = caps_checkDiscr(vs->discr, 129, name);
          if (status != CAPS_SUCCESS) {
            snprintf(error, 129, "Bound = %s and Analysis = %s",
                     bobject->name, other->loadName);
            caps_makeSimpleErr(bound->vertexSet[j], name, error,
                               NULL, NULL, errors);
            if (*errors != NULL) {
              errs  = *errors;
              *nErr = errs->nError;
            }
            aim_FreeDiscr(problem->aimFPTR, other->loadName, vs->discr);
            return status;
          }
          caps_freeOwner(&bound->vertexSet[j]->last);
          bound->vertexSet[j]->last.sNum = problem->sNum;
          caps_fillDateTime(bound->vertexSet[j]->last.datetime);
        }
      }
      if (vs->discr                         == NULL)      continue;
      if (vs->discr->nPoints                == 0)         continue;
      for (k = 0; k < vs->nDataSets; k++) {
        if (vs->dataSets[k]                 == NULL)      continue;
        if (vs->dataSets[k]->magicnumber    != CAPSMAGIC) continue;
        if (vs->dataSets[k]->type           != DATASET)   continue;
        if (vs->dataSets[k]->blind          == NULL)      continue;
        ds = (capsDataSet *) vs->dataSets[k]->blind;
        if (ds->method                      != BuiltIn)   continue;
        if (vs->dataSets[k]->last.sNum < problem->geometry.sNum)
          caps_fillBuiltIn(bobject, vs->discr, vs->dataSets[k],
                           analysis->pre.sNum);
      }
    }
    
    /* fill in the other DataSets -- method == Analysis */
    for (j = 0; j < bound->nVertexSet; j++) {
      if (bound->vertexSet[j]              == NULL)      continue;
      if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[j]->type        != VERTEXSET) continue;
      if (bound->vertexSet[j]->blind       == NULL)      continue;
      vs    = (capsVertexSet *) bound->vertexSet[j]->blind;
      if ((deferred == 0) && (vs->analysis != aobject))  continue;
      other = (capsAnalysis *)  vs->analysis->blind;
      if (other                            == NULL)      continue;
      for (k = 0; k < vs->nDataSets; k++) {
        dso = vs->dataSets[k];
        if (dso        == NULL) continue;
        if (dso->blind == NULL) continue;
        ds = (capsDataSet *) dso->blind;
        if (ds->method != Analysis) continue;
        if (bound->vertexSet[j]->last.sNum < problem->geometry.sNum) {
          aim_FreeDiscr(problem->aimFPTR, other->loadName, vs->discr);
          status = aim_Discr(problem->aimFPTR, other->loadName,
                             bobject->name, vs->discr);
          if (status != CAPS_SUCCESS) {
            snprintf(error, 129, "Bound = %s and Analysis = %s",
                     bobject->name, other->loadName);
            caps_makeSimpleErr(bound->vertexSet[j],
                               "caps_postAnalysis Error: aimDiscr fails!",
                               error, NULL, NULL, errors);
            if (*errors != NULL) {
              errs  = *errors;
              *nErr = errs->nError;
            }
            return status;
          } else {
            /* check the validity of the discretization just returned */
            status = caps_checkDiscr(vs->discr, 129, name);
            if (status != CAPS_SUCCESS) {
              snprintf(error, 129, "Bound = %s and Analysis = %s",
                       bobject->name, other->loadName);
              caps_makeSimpleErr(bound->vertexSet[j], name, error,
                                 NULL, NULL, errors);
              if (*errors != NULL) {
                errs  = *errors;
                *nErr = errs->nError;
              }
              aim_FreeDiscr(problem->aimFPTR, other->loadName, vs->discr);
              return status;
            }
            caps_freeOwner(&bound->vertexSet[j]->last);
            bound->vertexSet[j]->last.sNum = analysis->pre.sNum;
            caps_fillDateTime(bound->vertexSet[j]->last.datetime);
          }
        }
        if ((dso->last.sNum < analysis->pre.sNum) ||
            (dso->last.sNum == 0) || (ds->npts == 0)) {
          ds->npts = vs->discr->nVerts;
          if (ds->npts == 0) ds->npts = vs->discr->nPoints;
          if (ds->npts == 0) continue;
          if (ds->data != NULL) EG_free(ds->data);
          ds->data = (double *) EG_alloc(ds->npts*ds->rank*sizeof(double));
          if (ds->data == NULL) {
            ds->npts = 0;
            printf(" CAPS Warning: Post Analysis %s -- DataSet %s Malloc Error!\n",
                   vs->analysis->name, dso->name);
            continue;
          }
          if (ds->units != NULL) EG_free(ds->units);
          ds->units = NULL;
          status = aim_Transfer(problem->aimFPTR, other->loadName, vs->discr,
                                dso->name, ds->npts, ds->rank, ds->data,
                                &ds->units);
          if (status != CAPS_SUCCESS) {
            EG_free(ds->data);
            ds->data = NULL;
            ds->npts = 0;
            printf(" CAPS Warning: Post Analysis %s -- DataSet %s returns %d!\n",
                   vs->analysis->name, dso->name, status);
            continue;
          } else {
            OK = 1;
            if (dso->last.sNum != 0) {
              if (ds->history == NULL) {
                ds->nHist   = 0;
                ds->history = (capsOwn *) EG_alloc(sizeof(capsOwn));
                if (ds->history == NULL) OK = 0;
              } else {
                tmp = (capsOwn *) EG_reall( ds->history,
                                           (ds->nHist+1)*sizeof(capsOwn));
                if (tmp == NULL) {
                  OK = 0;
                } else {
                  ds->history = tmp;
                }
              }
              if ((OK == 1) && (ds->history != NULL)) {
                ds->history[ds->nHist]       = dso->last;
                ds->history[ds->nHist].pname = EG_strdup(dso->last.pname);
                ds->history[ds->nHist].pID   = EG_strdup(dso->last.pID);
                ds->history[ds->nHist].user  = EG_strdup(dso->last.user);
                ds->nHist += 1;
              }
            }
            caps_freeOwner(&dso->last);
            dso->last.sNum = problem->sNum + 1;
            caps_fillDateTime(dso->last.datetime);
          }
          if (ds->units != NULL) {
            utunit = ut_parse((ut_system *) problem->utsystem, ds->units,
                              UT_ASCII);
            if (utunit == NULL) {
              printf(" CAPS Warning: Post Analysis %s -- DataSet %s Units Error!\n",
                     vs->analysis->name, dso->name);
              EG_free(ds->units);
              ds->units = NULL;
            } else {
              ut_free(utunit);
            }
          }
        }
      }
    }
  }
  
  /* set the time/date stamp */
  caps_freeOwner(&aobject->last);
  if (current.pname != NULL) aobject->last.pname = EG_strdup(current.pname);
  if (current.pID   != NULL) aobject->last.pID   = EG_strdup(current.pID);
  if (current.user  != NULL) aobject->last.user  = EG_strdup(current.user);
  problem->sNum     += 1;
  aobject->last.sNum = problem->sNum;
  caps_fillDateTime(aobject->last.datetime);

  return CAPS_SUCCESS;
}


int
caps_fillVertexSets(capsObject *bobject, int *nErr, capsErrs **errors)
{
  int           i, k, stat;
  capsObject    *pobject;
  capsProblem   *problem;
  capsBound     *bound;
  capsAnalysis  *analysis;
  capsVertexSet *vs;
  capsDataSet   *ds;

  *nErr   = 0;
  *errors = NULL;
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
  
  stat = caps_preAnalysis(pobject, nErr, errors);
  if ((stat != CAPS_CLEAN) && (stat != CAPS_SUCCESS)) return stat;
  if (bound->state != Empty)
    if (bobject->last.sNum >= problem->geometry.sNum) return CAPS_CLEAN;
  
  for (i = 0; i < bound->nVertexSet; i++) {
    if (bound->vertexSet[i]              == NULL)      continue;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[i]->type        != VERTEXSET) continue;
    if (bound->vertexSet[i]->blind       == NULL)      continue;
    vs       = (capsVertexSet *) bound->vertexSet[i]->blind;
    analysis = (capsAnalysis *)  vs->analysis->blind;
    if (analysis                         == NULL)      continue;
    if (analysis->bodies                 != NULL)      continue;
    stat     = caps_filter(problem, analysis);
    if (stat != CAPS_SUCCESS)
      printf(" CAPS Warning: caps_filter = %d (caps_fillVertexSets)!\n", stat);
  }

  stat = caps_refillBound(problem, bobject, nErr, errors);
  if (stat != CAPS_SUCCESS) return stat;
  
  /* populate any built-in DataSet entries */
  for (i = 0; i < bound->nVertexSet; i++) {
    if (bound->vertexSet[i]              == NULL)      continue;
    if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
    if (bound->vertexSet[i]->type        != VERTEXSET) continue;
    if (bound->vertexSet[i]->blind       == NULL)      continue;
    vs       = (capsVertexSet *) bound->vertexSet[i]->blind;
    analysis = (capsAnalysis *)  vs->analysis->blind;
    if (analysis                         == NULL)      continue;
    if (vs->discr                        == NULL)      continue;
    if (vs->discr->nPoints               == 0)         continue;
    for (k = 0; k < vs->nDataSets; k++) {
      if (vs->dataSets[k]                == NULL)      continue;
      if (vs->dataSets[k]->magicnumber   != CAPSMAGIC) continue;
      if (vs->dataSets[k]->type          != DATASET)   continue;
      if (vs->dataSets[k]->blind         == NULL)      continue;
      ds = (capsDataSet *) vs->dataSets[k]->blind;
      if (ds->method                     != BuiltIn)   continue;
      if (vs->dataSets[k]->last.sNum < problem->geometry.sNum)
        caps_fillBuiltIn(bobject, vs->discr, vs->dataSets[k], problem->sNum);
    }
  }
  problem->sNum += 1;
  
  return CAPS_SUCCESS;
}


int
caps_dirtyAnalysis(capsObject *object, int *nAobj, capsObject ***aobjs)
{
  int           i, j, execute, nPar, nField, *ranks, stat, dirty;
  char          *intents, *apath, *unitSys, **fnames;
  capsProblem   *problem;
  capsBound     *bound;
  capsVertexSet *vertexset;
  capsObject    *pobject, *aobject, **par;

  *nAobj = 0;
  *aobjs = NULL;
  if  (object              == NULL)      return CAPS_NULLOBJ;
  if  (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if ((object->type        != PROBLEM) && (object->type != ANALYSIS) &&
      (object->type        != BOUND))    return CAPS_BADTYPE;
  if  (object->blind       == NULL)      return CAPS_NULLBLIND;
  
  if (object->type == PROBLEM) {
    problem = (capsProblem *) object->blind;
    if (problem->nAnalysis   == 0)         return CAPS_SUCCESS;
    
    for (i = 0; i < problem->nAnalysis; i++) {
      stat = caps_analysisInfo(problem->analysis[i], &apath, &unitSys, &intents,
                               &nPar, &par, &nField, &fnames, &ranks, &execute,
                               &dirty);
      if (stat != CAPS_SUCCESS) {
        if (*aobjs != NULL) {
          EG_free(*aobjs);
          *nAobj = 0;
          *aobjs = NULL;
        }
        return stat;
      }
      if (dirty == 0) continue;
      if (*aobjs == NULL) {
        *aobjs = (capsObject **)
                 EG_alloc(problem->nAnalysis*sizeof(capsObject *));
        if (*aobjs == NULL) return EGADS_MALLOC;
      }
      (*aobjs)[*nAobj] = problem->analysis[i];
      *nAobj += 1;
    }
    
  } else if (object->type == BOUND) {
    
    /* for Bound Objects -- find dependent Analysis Objects */
    bound   = (capsBound *)  object->blind;
    pobject = (capsObject *) object->parent;
    if (pobject->blind == NULL) return CAPS_NULLBLIND;
    problem = (capsProblem *)  pobject->blind;
    for (i = 0; i < bound->nVertexSet; i++) {
      if (bound->vertexSet[i]              == NULL)      continue;
      if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[i]->type        != VERTEXSET) continue;
      if (bound->vertexSet[i]->blind       == NULL)      continue;
      vertexset = (capsVertexSet *) bound->vertexSet[i]->blind;
      aobject   = vertexset->analysis;
      if (aobject                          == NULL)      continue;
      stat = caps_analysisInfo(aobject, &apath, &unitSys, &intents, &nPar, &par,
                               &nField, &fnames, &ranks, &execute, &dirty);
      if (stat != CAPS_SUCCESS) {
        if (*aobjs != NULL) {
          EG_free(*aobjs);
          *nAobj = 0;
          *aobjs = NULL;
        }
        return stat;
      }
      if (dirty == 0) continue;
      if (*aobjs == NULL) {
        *aobjs = (capsObject **)
                 EG_alloc(problem->nAnalysis*sizeof(capsObject *));
        if (*aobjs == NULL) return EGADS_MALLOC;
      }
      for (j = 0; j < *nAobj; j++)
        if (aobject == (*aobjs)[j]) break;
      if (j != *nAobj) continue;
      (*aobjs)[*nAobj] = aobject;
      *nAobj += 1;
    }

  } else {
    
    /* for Analysis Objects -- find dependent Analysis Objects in Bounds */
    if (object->parent == NULL) return CAPS_NULLOBJ;
    pobject  = (capsObject *)   object->parent;
    if (pobject->blind == NULL) return CAPS_NULLBLIND;
    problem  = (capsProblem *)  pobject->blind;
    for (i = 0; i < problem->nAnalysis; i++) {
      if (problem->analysis[i] == object) continue;
      stat = caps_boundDependent(problem, object, problem->analysis[i]);
      if (stat != CAPS_SUCCESS) continue;
      stat = caps_analysisInfo(problem->analysis[i], &apath, &unitSys, &intents,
                               &nPar, &par, &nField, &fnames, &ranks, &execute,
                               &dirty);
      if (stat != CAPS_SUCCESS) {
        if (*aobjs != NULL) {
          EG_free(*aobjs);
          *nAobj = 0;
          *aobjs = NULL;
        }
        return stat;
      }
      if (dirty == 0) continue;
      if (*aobjs == NULL) {
        *aobjs = (capsObject **)
                 EG_alloc(problem->nAnalysis*sizeof(capsObject *));
        if (*aobjs == NULL) return EGADS_MALLOC;
      }
      (*aobjs)[*nAobj] = problem->analysis[i];
      *nAobj += 1;
    }
  }
  
  return CAPS_SUCCESS;
}
