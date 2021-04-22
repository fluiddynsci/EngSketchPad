/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             AIM Utility Functions
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "capsTypes.h"
#include "udunits.h"
#include "capsAIM.h"

/* OpenCSM Defines & Includes */
#include "common.h"
#include "OpenCSM.h"


/*@-incondefs@*/
extern void ut_free(/*@only@*/ ut_unit* const unit);
extern void cv_free(/*@only@*/ cv_converter* const converter);
/*@+incondefs@*/



static int
aim_Data(aimContext     cntxt,
         const char     *aname,
         int            instance,       /* instance index */
         const char     *name,          /* name of transfer */
         enum capsvType *vtype,         /* data type */
         int            *rank,          /* rank of the data */
         int            *nrow,          /* number of rows */
         int            *ncol,          /* number of columns */
         void           **vdata,        /* the returned data */
         char           **units)        /* returned units -- if any */
{
  int i;

  for (i = 0; i < cntxt.aim_nAnal; i++)
    if (strcmp(aname, cntxt.aimName[i]) == 0) break;

  if (i == cntxt.aim_nAnal) return CAPS_NOTFOUND;
  if (cntxt.aimData[i] == NULL) {
    printf("aimData not implemented in AIM %s\n", aname);
    return CAPS_NOTIMPLEMENT;
  }

  return cntxt.aimData[i](instance, name, vtype, rank, nrow, ncol, vdata,
                          units);
}


int
aim_getUnitSys(void *aimStruc, char **unitSys)
{
  aimInfo      *aInfo;
  capsAnalysis *analysis;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                               return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)             return CAPS_BADOBJECT;
  analysis = (capsAnalysis *) aInfo->analysis;

  *unitSys = analysis->unitSys;
  return CAPS_SUCCESS;
}


int
aim_getBodies(void *aimStruc, const char **intents, int *nBody, ego **bodies)
{
  aimInfo      *aInfo;
  capsAnalysis *analysis;

  *nBody  = 0;
  *bodies = NULL;
  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  analysis = (capsAnalysis *) aInfo->analysis;

  *intents = analysis->intents;
  *nBody   = analysis->nBody;
  *bodies  = analysis->bodies;
  return CAPS_SUCCESS;
}


int
aim_newGeometry(void *aimStruc)
{
  aimInfo      *aInfo;
  capsProblem  *problem;
  capsAnalysis *analysis;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                               return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)             return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  if (problem->geometry.sNum < analysis->pre.sNum) return CAPS_CLEAN;
  return CAPS_SUCCESS;
}


int
aim_convert(void *aimStruc, const char  *inUnits, double   inValue,
                            const char *outUnits, double *outValue)
{
  int          status;
  aimInfo      *aInfo;
  capsProblem  *problem;
  ut_unit      *utunit1, *utunit2;
  cv_converter *converter;

  *outValue = inValue;
  if ((inUnits == NULL) || (outUnits == NULL)) return CAPS_NULLNAME;
  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                           return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)         return CAPS_BADOBJECT;
  problem = aInfo->problem;

  utunit1 = ut_parse((ut_system *) problem->utsystem,  inUnits, UT_ASCII);
  utunit2 = ut_parse((ut_system *) problem->utsystem, outUnits, UT_ASCII);
  status  = ut_are_convertible(utunit1, utunit2);
  if (status == 0) {
    ut_free(utunit1);
    ut_free(utunit2);
    return CAPS_UNITERR;
  }

  converter = ut_get_converter(utunit1, utunit2);
  *outValue = cv_convert_double(converter, inValue);
/*
  printf(" aim_convert: %lf %s to %lf %s!\n",
         inValue, inUnits, *outValue, outUnits);
 */

  cv_free(converter);
  ut_free(utunit1);
  ut_free(utunit2);
  return CAPS_SUCCESS;
}


int
aim_unitMultiply(void *aimStruc, const char  *inUnits1, const char *inUnits2,
                 char **outUnits)
{
  int         status;
  size_t      len1, len2, ulen;
  aimInfo     *aInfo;
  capsProblem *problem;
  ut_unit     *utunit1, *utunit2, *utunit;
  char        *units = NULL;

  if ((inUnits1 == NULL) ||
      (inUnits2 == NULL) || (outUnits == NULL)) return CAPS_NULLNAME;
  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                            return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)          return CAPS_BADOBJECT;
  problem = aInfo->problem;

  *outUnits = NULL;

  len1 = strlen(inUnits1);
  len2 = strlen(inUnits2);
  ulen = 10*(len1+len2);
  if (ulen == 0) return CAPS_UNITERR;

  units = (char *) EG_alloc(ulen*sizeof(char));
  if (units == NULL) return EGADS_MALLOC;

  utunit1 = ut_parse((ut_system *) problem->utsystem, inUnits1, UT_ASCII);
  utunit2 = ut_parse((ut_system *) problem->utsystem, inUnits2, UT_ASCII);
  utunit  = ut_multiply(utunit1, utunit2);
  if (ut_get_status() != UT_SUCCESS) {
    ut_free(utunit1);
    ut_free(utunit2);
    ut_free(utunit);
    EG_free(units);
    return CAPS_UNITERR;
  }

  status = ut_format(utunit, units, ulen, UT_ASCII);

  ut_free(utunit1);
  ut_free(utunit2);
  ut_free(utunit);

  if (status < UT_SUCCESS) {
    EG_free(units);
    return CAPS_UNITERR;
  } else {
    *outUnits = units;
    return CAPS_SUCCESS;
  }
}


int
aim_unitDivide(void *aimStruc, const char  *inUnits1, const char *inUnits2,
               char **outUnits)
{
  int         status;
  size_t      len1, len2, ulen;
  aimInfo     *aInfo;
  capsProblem *problem;
  ut_unit     *utunit1, *utunit2, *utunit;
  char        *units = NULL;

  if ((inUnits1 == NULL) ||
      (inUnits2 == NULL) || (outUnits == NULL)) return CAPS_NULLNAME;
  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                            return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)          return CAPS_BADOBJECT;
  problem = aInfo->problem;

  *outUnits = NULL;

  len1 = strlen(inUnits1);
  len2 = strlen(inUnits2);
  ulen = 10*(len1+len2);
  if (ulen == 0) return CAPS_UNITERR;

  units = (char *) EG_alloc(ulen*sizeof(char));
  if (units == NULL) return EGADS_MALLOC;

  utunit1 = ut_parse((ut_system *) problem->utsystem, inUnits1, UT_ASCII);
  utunit2 = ut_parse((ut_system *) problem->utsystem, inUnits2, UT_ASCII);
  utunit  = ut_divide(utunit1, utunit2);
  if (ut_get_status() != UT_SUCCESS) {
    ut_free(utunit1);
    ut_free(utunit2);
    ut_free(utunit);
    EG_free(units);
    return CAPS_UNITERR;
  }

  status = ut_format(utunit, units, ulen, UT_ASCII);

  ut_free(utunit1);
  ut_free(utunit2);
  ut_free(utunit);

  if (status < UT_SUCCESS) {
    EG_free(units);
    return CAPS_UNITERR;
  } else {
    *outUnits = units;
    return CAPS_SUCCESS;
  }
}


int
aim_unitInvert(void *aimStruc, const char  *inUnit, char **outUnits)
{
  int         status;
  size_t      len1, ulen;
  aimInfo     *aInfo;
  capsProblem *problem;
  ut_unit     *utunit1, *utunit;
  char        *units = NULL;

  if ((inUnit == NULL) || (outUnits == NULL)) return CAPS_NULLNAME;
  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                          return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)        return CAPS_BADOBJECT;
  problem = aInfo->problem;

  *outUnits = NULL;

  len1 = strlen(inUnit);
  ulen = 10*len1;
  if (ulen == 0) return CAPS_UNITERR;

  units = (char *) EG_alloc(ulen*sizeof(char));
  if (units == NULL) return EGADS_MALLOC;

  utunit1 = ut_parse((ut_system *) problem->utsystem, inUnit, UT_ASCII);
  utunit  = ut_invert(utunit1);
  if (ut_get_status() != UT_SUCCESS) {
    ut_free(utunit1);
    ut_free(utunit);
    EG_free(units);
    return CAPS_UNITERR;
  }

  status = ut_format(utunit, units, ulen, UT_ASCII);

  ut_free(utunit1);
  ut_free(utunit);

  if (status < UT_SUCCESS) {
    EG_free(units);
    return CAPS_UNITERR;
  } else {
    *outUnits = units;
    return CAPS_SUCCESS;
  }
}


int
aim_unitRaise(void *aimStruc, const char  *inUnit, const int power,
              char **outUnits)
{
  int         status;
  size_t      len1, ulen;
  aimInfo     *aInfo;
  capsProblem *problem;
  ut_unit     *utunit1, *utunit;
  char        *units = NULL;

  if ((inUnit == NULL) || (outUnits == NULL)) return CAPS_NULLNAME;
  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                          return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)        return CAPS_BADOBJECT;
  problem = aInfo->problem;

  *outUnits = NULL;

  len1 = strlen(inUnit);
  ulen = 10*len1;
  if (ulen == 0) return CAPS_UNITERR;

  units = (char *) EG_alloc(ulen*sizeof(char));
  if (units == NULL) return EGADS_MALLOC;

  utunit1 = ut_parse((ut_system *) problem->utsystem, inUnit, UT_ASCII);
  utunit  = ut_raise(utunit1, power);
  if (ut_get_status() != UT_SUCCESS) {
    ut_free(utunit1);
    ut_free(utunit);
    EG_free(units);
    return CAPS_UNITERR;
  }

  status = ut_format(utunit, units, ulen, UT_ASCII);

  ut_free(utunit1);
  ut_free(utunit);

  if (status < UT_SUCCESS) {
    EG_free(units);
    return CAPS_UNITERR;
  } else {
    *outUnits = units;
    return CAPS_SUCCESS;
  }
}


int
aim_getIndex(void *aimStruc, /*@null@*/ const char *name,
             enum capssType subtype)
{
  int          i, nobj;
  aimInfo      *aInfo;
  capsProblem  *problem;
  capsAnalysis *analysis;
  capsObject   **objs;

  aInfo = (aimInfo *) aimStruc;
  if ((subtype != GEOMETRYIN) && (subtype != GEOMETRYOUT) &&
      (subtype != ANALYSISIN) && (subtype != ANALYSISOUT))
                                       return CAPS_BADTYPE;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;
  if (subtype == GEOMETRYIN) {
    nobj = problem->nGeomIn;
    objs = problem->geomIn;
  } else if (subtype == GEOMETRYOUT) {
    nobj = problem->nGeomOut;
    objs = problem->geomOut;
  } else if (subtype == ANALYSISIN) {
    nobj = analysis->nAnalysisIn;
    objs = analysis->analysisIn;
  } else {
    nobj = analysis->nAnalysisOut;
    objs = analysis->analysisOut;
  }
  if (name == NULL) return nobj;

  for (i = 0; i < nobj; i++) {
    if (objs[i]->name == NULL) {
      printf(" aimUtil Info: object %d with NULL name!\n", i+1);
      continue;
    }
    if (strcmp(objs[i]->name, name) == 0) return i+1;
  }

  return CAPS_NOTFOUND;
}


int
aim_getValue(void *aimStruc, int index, enum capssType subtype,
             capsValue **value)
{
  int          nobj;
  aimInfo      *aInfo;
  capsProblem  *problem;
  capsAnalysis *analysis;
  capsObject   **objs;

  aInfo = (aimInfo *) aimStruc;
  if ((subtype != GEOMETRYIN) && (subtype != GEOMETRYOUT) &&
      (subtype != ANALYSISIN) && (subtype != ANALYSISOUT))
                                       return CAPS_BADTYPE;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (index <= 0)                      return CAPS_BADINDEX;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;
  if (subtype == GEOMETRYIN) {
    nobj = problem->nGeomIn;
    objs = problem->geomIn;
  } else if (subtype == GEOMETRYOUT) {
    nobj = problem->nGeomOut;
    objs = problem->geomOut;
  } else if (subtype == ANALYSISIN) {
    nobj = analysis->nAnalysisIn;
    objs = analysis->analysisIn;
  } else {
    nobj = analysis->nAnalysisOut;
    objs = analysis->analysisOut;
  }
  if (index > nobj)                    return CAPS_BADINDEX;

  *value = (capsValue *) objs[index-1]->blind;
  return CAPS_SUCCESS;
}


int
aim_getName(void *aimStruc, int index, enum capssType subtype,
            const char **name)
{
  int          nobj;
  aimInfo      *aInfo;
  capsProblem  *problem;
  capsAnalysis *analysis;
  capsObject   **objs;

  aInfo = (aimInfo *) aimStruc;
  if ((subtype != GEOMETRYIN) && (subtype != GEOMETRYOUT) &&
      (subtype != ANALYSISIN) && (subtype != ANALYSISOUT))
                                       return CAPS_BADTYPE;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (index <= 0)                      return CAPS_BADINDEX;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;
  if (subtype == GEOMETRYIN) {
    nobj = problem->nGeomIn;
    objs = problem->geomIn;
  } else if (subtype == GEOMETRYOUT) {
    nobj = problem->nGeomOut;
    objs = problem->geomOut;
  } else if (subtype == ANALYSISIN) {
    nobj = analysis->nAnalysisIn;
    objs = analysis->analysisIn;
  } else {
    nobj = analysis->nAnalysisOut;
    objs = analysis->analysisOut;
  }
  if (index > nobj)                    return CAPS_BADINDEX;

  *name = objs[index-1]->name;
  return CAPS_SUCCESS;
}


int
aim_getGeomInType(void *aimStruc, int index)
{
  int         stat, nrow, ncol, type;
  char        name[MAX_NAME_LEN];
  aimInfo     *aInfo;
  capsObject  *vobj;
  capsValue   *value;
  capsProblem *problem;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (index <= 0)                      return CAPS_BADINDEX;
  problem = aInfo->problem;
  if (index > problem->nGeomIn)        return CAPS_BADINDEX;
  vobj = problem->geomIn[index-1];
  if (vobj->magicnumber != CAPSMAGIC)  return CAPS_BADOBJECT;
  if (vobj->type        != VALUE)      return CAPS_BADTYPE;
  if (vobj->blind       == NULL)       return CAPS_NULLBLIND;
  value = (capsValue *) vobj->blind;

  stat  = ocsmGetPmtr(problem->modl, value->pIndex, &type, &nrow, &ncol, name);
  if (stat != SUCCESS) return stat;
  
  if (type == OCSM_EXTERNAL) return CAPS_SUCCESS;
  return EGADS_OUTSIDE;
}


static int
aim_analyScan(capsProblem  *problem, capsAnalysis *analysis, const char *name,
              enum capsvType *vtype, int *rank, int *nrow, int *ncol,
              void **data, char **units)
{
  int            i, status, rankx, nrowx, ncolx;
  char           *unitx;
  void           *datax;
  enum capsvType vtypx;
  capsAnalysis   *analpar;
  capsObject     *aobj;

  for (i = 0; i < analysis->nParent; i++) {
    aobj    = analysis->parents[i];
    analpar = (capsAnalysis *) aobj->blind;
    status  = aim_Data(problem->aimFPTR, analpar->loadName,
                       analpar->instance, name, &vtypx, &rankx, &nrowx,
                       &ncolx, &datax, &unitx);
    if ((status == CAPS_SUCCESS) && (datax != NULL)) {
      if (*data != NULL) {
        if (*rank  < 0)     EG_free(*data);
        if (*units != NULL) EG_free(*units);
        return CAPS_SOURCEERR;
      }
      *vtype = vtypx;
      *rank  = rankx;
      *nrow  = nrowx;
      *ncol  = ncolx;
      *data  = datax;
      *units = unitx;
    }
/*  do not recurse
    status = aim_analyScan(problem, analpar, name, vtype, rank, nrow, ncol,
                           data, units);
    if (status == CAPS_SOURCEERR) return status;  */
  }

  if (*data == NULL) return CAPS_NOTFOUND;
  return CAPS_SUCCESS;
}


int
aim_getData(void *aimStruc, const char *name, enum capsvType *vtype, int *rank,
            int *nrow, int *ncol, void **data, char **units)
{
  int          i, status;
  aimInfo      *aInfo;
  capsProblem  *problem;
  capsAnalysis *analysis, *analpar;
  capsObject   *aobj, *source, *object, *last;
  capsValue    *value;

  *vtype = Integer;
  *nrow  = *ncol = *rank = 0;
  *data  = NULL;
  *units = NULL;
  aInfo  = (aimInfo *) aimStruc;
  if (name  == NULL)                   return CAPS_NULLNAME;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  /* check for dirty geometry inputs */
  for (status = i = 0; i < problem->nGeomIn; i++) {
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
      status = 1;
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
      status = 1;
      break;
    }
  }
  if (status != 0) return CAPS_DIRTY;

  /* are any of the analysis dirty? */
  for (i = 0; i < analysis->nParent; i++) {
    aobj    = analysis->parents[i];
    analpar = (capsAnalysis *) aobj->blind;
    if (analpar->pre.sNum == 0) {
      status = 1;
    } else {
      /* check for dirty inputs */
      for (i = 0; i < analpar->nAnalysisIn; i++) {
        source = object = analpar->analysisIn[i];
        do {
          if (source->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
          if (source->type        != VALUE)     return CAPS_BADTYPE;
          if (source->blind       == NULL)      return CAPS_NULLBLIND;
          value = (capsValue *) source->blind;
          if (value->link == object)            return CAPS_CIRCULARLINK;
          last   = source;
          source = value->link;
        } while (value->link != NULL);
        if (last->last.sNum > analpar->pre.sNum) {
          status = 1;
          break;
        }
      }
    }
    /* is geometry new? */
    if (status == 0)
      if (problem->geometry.sNum > analpar->pre.sNum) status = 4;
    /* is post required? */
    if (status == 0)
      if (analpar->pre.sNum > aobj->last.sNum) status = 5;
    if (status != 0) return CAPS_DIRTY;
  }

  return aim_analyScan(problem, analysis, name, vtype, rank, nrow, ncol, data,
                       units);
}


static int
aim_findByName(const char *name, int len, capsObject **objs, capsObject **child)
{
  int i;

  for (i = 0; i < len; i++) {
    if (strcmp(objs[i]->name, name) == 0) {
      *child = objs[i];
      return CAPS_SUCCESS;
    }
  }

  return CAPS_NOTFOUND;
}


static int
aim_parentScan(capsAnalysis *analysis, const char *name, enum capssType stype,
               capsObject **child)
{
  int          i, status;
  capsAnalysis *analpar;
  capsObject   *aobj;

  /* do not recurse
  for (i = 0; i < analysis->nParent; i++) {
    aobj    = analysis->parents[i];
    analpar = (capsAnalysis *) aobj->blind;
    status  = aim_parentScan(analpar, name, stype, child);
    if (status == CAPS_SUCCESS) return status;
  }  */
  for (i = 0; i < analysis->nParent; i++) {
    aobj    = analysis->parents[i];
    analpar = (capsAnalysis *) aobj->blind;
    if (stype == ANALYSISIN) {
      status = aim_findByName(name, analpar->nAnalysisIn,
                              analpar->analysisIn,  child);
    } else {
      status = aim_findByName(name, analpar->nAnalysisOut,
                              analpar->analysisOut, child);
    }
    if (status == CAPS_SUCCESS) return status;
  }

  return CAPS_NOTFOUND;
}


int
aim_link(void *aimStruc, const char *name, enum capssType stype,
         capsValue *value)
{
  int          stat;
  aimInfo      *aInfo;
  capsProblem  *problem;
  capsAnalysis *analysis;
  capsObject   *child = NULL;

  aInfo = (aimInfo *) aimStruc;
  if (name  == NULL)                   return CAPS_NULLNAME;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  if (stype == GEOMETRYIN) {
    stat = aim_findByName(name, problem->nGeomIn,  problem->geomIn,  &child);
    if (stat != CAPS_SUCCESS) return stat;
  } else if (stype == GEOMETRYOUT) {
    stat = aim_findByName(name, problem->nGeomOut, problem->geomOut, &child);
    if (stat != CAPS_SUCCESS) return stat;
  } else if ((stype == ANALYSISIN) || (stype == ANALYSISOUT)) {
    stat = aim_parentScan(analysis, name, stype, &child);
    if (stat != CAPS_SUCCESS) return stat;
  } else {
    return CAPS_BADTYPE;
  }

  value->link = child;

  return CAPS_SUCCESS;
}


int
aim_setTess(void *aimStruc, ego tess)
{
  int          stat, i, state, npts;
  aimInfo      *aInfo;
  capsAnalysis *analysis;
  ego          body;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (tess == NULL)                    return EGADS_NULLOBJ;
  if (tess->magicnumber  != MAGIC)     return EGADS_NOTOBJ;
  analysis = (capsAnalysis *) aInfo->analysis;

  /* remove an existing tessellation? */
  if (tess->oclass == BODY) {
    for (i = 0; i < analysis->nBody; i++) {
      if (tess != analysis->bodies[i]) continue;
      if (analysis->bodies[analysis->nBody+i] != NULL)
        EG_deleteObject(analysis->bodies[analysis->nBody+i]);
      analysis->bodies[analysis->nBody+i] = NULL;
      return CAPS_SUCCESS;
    }
    printf(" CAPS Info: Matching Body not in list! (aim_setTess)\n");
    return CAPS_NOTFOUND;
  }

  /* associate a tessellation object */
  stat = EG_statusTessBody(tess, &body, &state, &npts);
  if (stat <  EGADS_SUCCESS) return stat;
  if (stat == EGADS_OUTSIDE) return CAPS_SOURCEERR;

  for (i = 0; i < analysis->nBody; i++) {
    if (body != analysis->bodies[i]) continue;
    if (analysis->bodies[analysis->nBody+i] != NULL) return EGADS_EXISTS;
    analysis->bodies[analysis->nBody+i] = tess;
    return CAPS_SUCCESS;
  }

  printf(" CAPS Info: Tessellation Body not in list! (aim_setTess)\n");
  return CAPS_NOTFOUND;
}


int
aim_getDiscr(void *aimStruc, const char *bname, capsDiscr **discr)
{
  int           i, j;
  aimInfo       *aInfo;
  capsProblem   *problem;
  capsBound     *bound;
  capsAnalysis  *analysis, *analy;
  capsVertexSet *vertexset;

  *discr = NULL;
  aInfo  = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  /* find bound with same name */
  for (i = 0; i < problem->nBound; i++) {
    if (problem->bounds[i]       == NULL) continue;
    if (problem->bounds[i]->name == NULL) continue;
    if (strcmp(bname, problem->bounds[i]->name) != 0) continue;
    /* find our analysis in the Bound */
    bound = (capsBound *) problem->bounds[i]->blind;
    if (bound == NULL) return CAPS_NULLOBJ;
    for (j = 0; j < bound->nVertexSet; j++) {
      if (bound->vertexSet[j]              == NULL)      continue;
      if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[j]->type        != VERTEXSET) continue;
      if (bound->vertexSet[j]->blind       == NULL)      continue;
      vertexset = (capsVertexSet *) bound->vertexSet[j]->blind;
      if (vertexset           == NULL) continue;
      if (vertexset->analysis == NULL) continue;
      analy = (capsAnalysis *) vertexset->analysis->blind;
      if (analy != analysis) continue;
      *discr = vertexset->discr;
      return CAPS_SUCCESS;
    }
  }

  return CAPS_NOTFOUND;
}


int
aim_getDiscrState(void *aimStruc, const char *bname)
{
  int         i;
  aimInfo     *aInfo;
  capsProblem *problem;
  capsObject  *bobject;
  capsBound   *bound;

  aInfo  = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  problem = aInfo->problem;

  /* find bound with same name */
  for (i = 0; i < problem->nBound; i++) {
    if (problem->bounds[i]       == NULL) continue;
    if (problem->bounds[i]->name == NULL) continue;
    if (strcmp(bname, problem->bounds[i]->name) != 0) continue;
    bobject = problem->bounds[i];
    bound   = (capsBound *) bobject->blind;
    if (bound == NULL) return CAPS_NULLOBJ;
    if (bobject->last.sNum < problem->geometry.sNum) return CAPS_DIRTY;
    return CAPS_SUCCESS;
  }

  return CAPS_NOTFOUND;
}


int
aim_getDataSet(capsDiscr *discr, const char *dname, enum capsdMethod *method,
               int *npts, int *rank, double **data)
{
  int           i, j, k, ii, jj;
  aimInfo       *aInfo;
  capsObject    *foundset;
  capsProblem   *problem;
  capsBound     *bound;
  capsAnalysis  *analysis, *analy;
  capsVertexSet *vertexset, *fvs;
  capsDataSet   *dataset, *otherset;

  *method = BuiltIn;
  *npts   = *rank = 0;
  *data   = NULL;
  if (discr == NULL)                   return CAPS_NULLOBJ;
  aInfo   = discr->aInfo;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  /* find dataset in discr */
  for (i = 0; i < problem->nBound; i++) {
    if (problem->bounds[i] == NULL) continue;
    bound = (capsBound *) problem->bounds[i]->blind;
    if (bound == NULL) continue;
    for (j = 0; j < bound->nVertexSet; j++) {
      if (bound->vertexSet[j]              == NULL)      continue;
      if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[j]->type        != VERTEXSET) continue;
      if (bound->vertexSet[j]->blind       == NULL)      continue;
      vertexset = (capsVertexSet *) bound->vertexSet[j]->blind;
      if (vertexset           == NULL) continue;
      if (vertexset->analysis == NULL) continue;
      analy = (capsAnalysis *) vertexset->analysis->blind;
      if (analy            != analysis) continue;
      if (vertexset->discr != discr)    continue;
      for (k = 0; k < vertexset->nDataSets; k++) {
        if (vertexset->dataSets[k]       == NULL) continue;
        if (vertexset->dataSets[k]->name == NULL) continue;
        if (strcmp(dname, vertexset->dataSets[k]->name) != 0) continue;
        dataset = (capsDataSet *) vertexset->dataSets[k]->blind;
        if (dataset == NULL) return CAPS_NULLOBJ;
        /* are we dirty? */
        if ((dataset->method == Interpolate) || (dataset->method == Conserve)) {
          if ((vertexset->dataSets[k]->last.sNum == 0) &&
              (dataset->startup != NULL)) {
            /* special startup data */
            *method = dataset->method;
            *rank   = dataset->rank;
            *npts   = 1;
            *data   = dataset->startup;
            return CAPS_SUCCESS;
          }
          /* find source in other VertexSets */
          foundset = NULL;
          for (ii = 0; ii < bound->nVertexSet; ii++) {
            if (ii == j) continue;
            if (bound->vertexSet[ii]              == NULL)      continue;
            if (bound->vertexSet[ii]->magicnumber != CAPSMAGIC) continue;
            if (bound->vertexSet[ii]->type        != VERTEXSET) continue;
            if (bound->vertexSet[ii]->blind       == NULL)      continue;
            fvs = (capsVertexSet *) bound->vertexSet[ii]->blind;
            if (fvs == NULL) continue;
            for (jj = 0; jj < fvs->nDataSets; jj++) {
              if (fvs->dataSets[jj]                      == NULL) continue;
              if (fvs->dataSets[jj]->name                == NULL) continue;
              if (strcmp(dname, fvs->dataSets[jj]->name) != 0)    continue;
              otherset = (capsDataSet *) fvs->dataSets[jj]->blind;
              if (otherset == NULL) continue;
              if ((otherset->method == User) || (otherset->method == Analysis)) {
                foundset = fvs->dataSets[j];
                break;
              }
            }
            if (foundset != NULL) break;
          }
          if (foundset == NULL) return CAPS_SOURCEERR;
          if ((foundset->last.sNum > vertexset->dataSets[k]->last.sNum) ||
              (dataset->data == NULL)) return CAPS_DIRTY;
        }
        *method = dataset->method;
        *rank   = dataset->rank;
        *npts   = dataset->npts;
        *data   = dataset->data;
        return CAPS_SUCCESS;
      }
    }
  }

  return CAPS_NOTFOUND;
}


int
aim_getBounds(void *aimStruc, int *nTname, char ***tnames)
{
  int           i, j, k;
  char          **names;
  aimInfo       *aInfo;
  capsProblem   *problem;
  capsBound     *bound;
  capsAnalysis  *analysis, *analy;
  capsVertexSet *vertexset;

  *tnames = NULL;
  *nTname = 0;
  aInfo   = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  if (problem->nBound == 0) return CAPS_SUCCESS;
  names = (char **) EG_alloc(problem->nBound*sizeof(char *));
  if (names == NULL) return EGADS_MALLOC;

  /* look at all bounds and find those with this analysis */
  for (k = i = 0; i < problem->nBound; i++) {
    if (problem->bounds[i]       == NULL) continue;
    if (problem->bounds[i]->name == NULL) continue;
    /* find our analysis in the Bound */
    bound = (capsBound *) problem->bounds[i]->blind;
    if (bound == NULL) continue;
    for (j = 0; j < bound->nVertexSet; j++) {
      if (bound->vertexSet[j]              == NULL)      continue;
      if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[j]->type        != VERTEXSET) continue;
      if (bound->vertexSet[j]->blind       == NULL)      continue;
      vertexset = (capsVertexSet *) bound->vertexSet[j]->blind;
      if (vertexset           == NULL) continue;
      if (vertexset->analysis == NULL) continue;
      analy = (capsAnalysis *) vertexset->analysis->blind;
      if (analy != analysis) continue;
      names[k] = problem->bounds[i]->name;
      k++;
      break;
    }
  }
  if (k == 0) {
    EG_free(names);
    return CAPS_SUCCESS;
  }

  *nTname = k;
  *tnames = names;
  return CAPS_SUCCESS;
}


int
aim_setSensitivity(void *aimStruc, const char *GIname, int irow, int icol)
{
  int          stat, i, ipmtr, nbrch, npmtr, nrow, ncol, type;
  int          buildTo, builtTo, nbody;
  char         name[MAX_NAME_LEN];
  modl_T       *MODL;
  aimInfo      *aInfo;
  capsProblem  *problem;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (GIname == NULL)                  return CAPS_NULLNAME;
  if ((irow < 1) || (icol < 1))        return CAPS_BADINDEX;
  problem  = aInfo->problem;
  MODL     = (modl_T *) problem->modl;
  if (MODL == NULL)                    return CAPS_NOTPARMTRIC;

  /* find the OpenCSM Parameter */
  stat = ocsmInfo(problem->modl, &nbrch, &npmtr, &nbody);
  if (stat != SUCCESS) return stat;
  for (ipmtr = i = 0; i < npmtr; i++) {
    stat = ocsmGetPmtr(problem->modl, i+1, &type, &nrow, &ncol, name);
    if (stat != SUCCESS) continue;
    if (type != OCSM_EXTERNAL) continue;
    if (strcmp(name, GIname) != 0) continue;
    ipmtr = i+1;
    break;
  }
  if (ipmtr == 0)  return CAPS_NOSENSITVTY;
  if (irow > nrow) return CAPS_BADINDEX;
  if (icol > ncol) return CAPS_BADINDEX;

  aInfo->pIndex = 0;
  aInfo->irow   = 0;
  aInfo->icol   = 0;

  /* clear all then set */
  ocsmSetVelD(problem->modl, 0,     0,    0,    0.0);
  ocsmSetVelD(problem->modl, ipmtr, irow, icol, 1.0);
  buildTo = 0;
  nbody   = 0;
  stat    = ocsmBuild(problem->modl, buildTo, &builtTo, &nbody, NULL);
  if (stat != SUCCESS) return stat;

  aInfo->pIndex = ipmtr;
  aInfo->irow   = irow;
  aInfo->icol   = icol;
  return CAPS_SUCCESS;
}


int
aim_getSensitivity(void *aimStruc, ego tess, int ttype, int index, int *npts,
                   double **dxyz)
{
  int          i, stat, ibody, npt, ntri, type;
  double       *dsen;
  const int    *ptype, *pindex, *tris, *tric;
  const double *xyzs, *parms;
  ego          body, oldtess;
  egTessel     *btess;
  modl_T       *MODL;
  aimInfo      *aInfo;
  capsProblem  *problem;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aInfo->pIndex      == 0)         return CAPS_STATEERR;
  if ((ttype < -2) || (ttype > 2))     return CAPS_RANGEERR;
  if (index < 1)                       return CAPS_BADINDEX;
  problem  = aInfo->problem;
  MODL     = (modl_T *) problem->modl;
  if (MODL == NULL)                    return CAPS_NOTPARMTRIC;
  if (tess == NULL)                    return EGADS_NULLOBJ;
  if (tess->magicnumber != MAGIC)      return EGADS_NOTOBJ;
  if (tess->oclass != TESSELLATION)    return EGADS_NOTTESS;
  btess = (egTessel *) tess->blind;
  if (btess == NULL)                   return EGADS_NOTFOUND;
  body = btess->src;
  if (body == NULL)                    return EGADS_NULLOBJ;
  if (body->magicnumber != MAGIC)      return EGADS_NOTOBJ;
  if (body->oclass != BODY)            return EGADS_NOTBODY;

  for (ibody = 1; ibody <= MODL->nbody; ibody++) {
    if (MODL->body[ibody].onstack != 1) continue;
    if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
    if (MODL->body[ibody].ebody   == body) break;
  }
  if (ibody > MODL->nbody) return CAPS_NOTFOUND;

  /* set the OCSM tessellation to the one given & save the old */
  oldtess = MODL->body[ibody].etess;
  MODL->body[ibody].etess = tess;

  /* get the length */
  if (ttype == 0) {
    type = OCSM_NODE;
    npt  = 1;
  } else if (abs(ttype) == 1) {
    type = OCSM_EDGE;
    stat = EG_getTessEdge(MODL->body[ibody].etess, index, &npt, &xyzs, &parms);
    if (stat != EGADS_SUCCESS) {
      MODL->body[ibody].etess = oldtess;
      return stat;
    }
  } else {
    type = OCSM_FACE;
    stat = EG_getTessFace(MODL->body[ibody].etess, index, &npt, &xyzs, &parms,
                          &ptype, &pindex, &ntri, &tris, &tric);
    if (stat != EGADS_SUCCESS) {
      MODL->body[ibody].etess = oldtess;
      return stat;
    }
  }
  if (npt <= 0) {
    MODL->body[ibody].etess = oldtess;
    return CAPS_NULLVALUE;
  }

  /* get the memory to store the sensitivity */
  dsen = (double *) EG_alloc(3*npt*sizeof(double));
  if (dsen == NULL) {
    MODL->body[ibody].etess = oldtess;
    return EGADS_MALLOC;
  }

  /* get and return the requested sensitivity */
  if (ttype <= 0) {
    stat = ocsmGetVel(problem->modl, ibody, type, index, npt, NULL, dsen);
    if (stat != SUCCESS) {
      EG_free(dsen);
      MODL->body[ibody].etess = oldtess;
      return stat;
    }
  } else {
    stat = ocsmGetTessVel(problem->modl, ibody, type, index, &xyzs);
    if (stat != SUCCESS) {
      EG_free(dsen);
      MODL->body[ibody].etess = oldtess;
      return stat;
    }
    for (i = 0; i < 3*npt; i++) dsen[i] = xyzs[i];
  }
  /* reset OCSMs tessellation */
  MODL->body[ibody].etess = oldtess;

  *npts = npt;
  *dxyz = dsen;
  return CAPS_SUCCESS;
}


int
aim_sensitivity(void *aimStruc, const char *GIname, int irow, int icol,
                ego tess, int *npts, double **dxyz)
{
  int          i, j, ipmtr, ibody, stat, nbrch, npmtr, nbody, nrow, ncol, type;
  int          npt, nface, np, ntris, global;
  const int    *ptype, *pindex, *tris, *tric;
  char         name[MAX_NAME_LEN];
  double       *dsen;
  const double *xyzs, *xyz, *uv;
  ego          body, oldtess, *faces;
  modl_T       *MODL;
  aimInfo      *aInfo;
  capsProblem  *problem;

  *npts = 0;
  *dxyz = NULL;
  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                   return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (GIname == NULL)                  return CAPS_NULLNAME;
  if ((irow < 1) || (icol < 1))        return CAPS_BADINDEX;
  problem  = aInfo->problem;
  MODL     = (modl_T *) problem->modl;
  if (MODL == NULL)                    return CAPS_NOTPARMTRIC;
  if (tess == NULL)                    return EGADS_NULLOBJ;
  if (tess->magicnumber != MAGIC)      return EGADS_NOTOBJ;
  if (tess->oclass != TESSELLATION)    return EGADS_NOTTESS;
  stat = EG_statusTessBody(tess, &body, &i, &npt);
  if (stat == EGADS_OUTSIDE)           return EGADS_TESSTATE;
  if (body == NULL)                    return EGADS_NULLOBJ;
  if (body->magicnumber != MAGIC)      return EGADS_NOTOBJ;
  if (body->oclass != BODY)            return EGADS_NOTBODY;
  stat = EG_getBodyTopos(body, NULL, FACE, &nface, &faces);
  if (stat != EGADS_SUCCESS)           return stat;
  EG_free(faces);

  for (ibody = 1; ibody <= MODL->nbody; ibody++) {
    if (MODL->body[ibody].onstack != 1) continue;
    if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
    if (MODL->body[ibody].ebody   == body) break;
  }
  if (ibody > MODL->nbody) return CAPS_NOTFOUND;

  /* find the OpenCSM Parameter */
  stat = ocsmInfo(problem->modl, &nbrch, &npmtr, &nbody);
  if (stat != SUCCESS) return stat;
  for (ipmtr = i = 0; i < npmtr; i++) {
    stat = ocsmGetPmtr(problem->modl, i+1, &type, &nrow, &ncol, name);
    if (stat != SUCCESS) continue;
    if (type != OCSM_EXTERNAL) continue;
    if (strcmp(name, GIname) != 0) continue;
    ipmtr = i+1;
    break;
  }
  if (ipmtr == 0)  return CAPS_NOSENSITVTY;
  if (irow > nrow) return CAPS_BADINDEX;
  if (icol > ncol) return CAPS_BADINDEX;

  /* set the Parameter if not already set */
  if ((aInfo->pIndex != ipmtr) || (aInfo->irow != irow) ||
      (aInfo->icol   != icol)) {
    stat = aim_setSensitivity(aimStruc, GIname, irow, icol);
    if (stat != CAPS_SUCCESS) return stat;
  }

  dsen = (double *) EG_alloc(3*npt*sizeof(double));
  if (dsen == NULL) return EGADS_MALLOC;
  for (i = 0; i < 3*npt; i++) dsen[i] = 0.0;

  /* return the requested sensitivity */
  oldtess = MODL->body[ibody].etess;
  MODL->body[ibody].etess = tess;
  for (i = 1; i <= nface; i++) {
    stat = EG_getTessFace(tess, i, &np, &xyz, &uv, &ptype, &pindex, &ntris,
                          &tris, &tric);
    if (stat != EGADS_SUCCESS) {
      EG_free(dsen);
      return stat;
    }
    stat = ocsmGetTessVel(problem->modl, ibody, OCSM_FACE, i, &xyzs);
    if (stat != SUCCESS) {
      EG_free(dsen);
      return stat;
    }
    for (j = 1; j <= np; j++) {
      stat = EG_localToGlobal(tess, i, j, &global);
      if (stat != EGADS_SUCCESS) {
        EG_free(dsen);
        return stat;
      }
      dsen[3*global-3] = xyzs[3*j-3];
      dsen[3*global-2] = xyzs[3*j-2];
      dsen[3*global-1] = xyzs[3*j-1];
    }
  }
  MODL->body[ibody].etess = oldtess;

  *npts = npt;
  *dxyz = dsen;
  return CAPS_SUCCESS;
}


int
aim_isNodeBody(ego body, double *xyz)
{
  int    status, oclass, mtype, nchild, *psens = NULL;
  double data[4];
  ego    *pchldrn, ref, loop, edge;

  /* Determine of the body is a "node body", i.e. a degenerate wire body */
  status = EG_getTopology(body, &ref, &oclass, &mtype, data, &nchild, &pchldrn,
                          &psens);
  if (status != EGADS_SUCCESS) return status;

  /* should be a body */
  if (oclass != BODY) return EGADS_NOTBODY;

  /* done if it's not a wire body */
  if (mtype != WIREBODY) return EGADS_OUTSIDE;

  /* there should be only one child, and it is a loop */
  if (nchild != 1) return EGADS_OUTSIDE;

  loop = pchldrn[0];

  /* get the edges */
  status = EG_getTopology(loop, &ref, &oclass, &mtype, data, &nchild, &pchldrn,
                          &psens);
  if (status != CAPS_SUCCESS) return status;

  /* there should be only one child, and it is an edge */
  if (nchild != 1) return EGADS_OUTSIDE;

  edge = pchldrn[0];

  /* get the node(s) of the edge */
  status = EG_getTopology(edge, &ref, &oclass, &mtype, data, &nchild, &pchldrn,
                          &psens);
  if (status != CAPS_SUCCESS) return status;

  /* the edge should be degenerate */
  if (mtype != DEGENERATE) return EGADS_OUTSIDE;

  /* something is really wrong if the edge does not have one node */
  if (nchild != 1) return EGADS_GEOMERR;

  /* retrieve the coordinates of the node */
  status = EG_getTopology(pchldrn[0], &ref, &oclass, &mtype, xyz, &nchild,
                          &pchldrn, &psens);
  if (status != EGADS_SUCCESS) return status;

  return EGADS_SUCCESS;
}
