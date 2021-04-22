/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Base Object Functions
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
/* needs Advapi32.lib & Ws2_32.lib */
#include <Windows.h>
#define getpid   _getpid
#define snprintf _snprintf
#else
#include <unistd.h>
#endif

#include "udunits.h"

#include "capsTypes.h"
#include "capsAIM.h"


#define STRING(a)       #a
#define STR(a)          STRING(a)

static char *CAPSprop[2] = {STR(CAPSPROP),
                        "\nCAPSprop: Copyright 2014-2020 MIT. All Rights Reserved."};

extern /*@null@*/ /*@only@*/
       char *EG_strdup(/*@null@*/ const char *str);

extern int  caps_filter(capsProblem *problem, capsAnalysis *analysis);
extern int  caps_Aprx1DFree(/*@only@*/ capsAprx1D *approx);
extern int  caps_Aprx2DFree(/*@only@*/ capsAprx2D *approx);



void
caps_getStaticStrings(char ***signature, char **pID, char **user)
{
#ifdef WIN32
  int     len;
  WSADATA wsaData;
#endif
  char    name[257], ID[301];

  *signature = CAPSprop;
  
#ifdef WIN32
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
  gethostname(name, 257);
#ifdef WIN32
  WSACleanup();
#endif
  snprintf(ID, 301, "%s:%d", name, getpid());
  *pID = EG_strdup(ID);

#ifdef WIN32
  len = 256;
  GetUserName(name, &len);
#else
  /*@-mustfreefresh@*/
  snprintf(name, 257, "%s", getlogin());
  /*@+mustfreefresh@*/
#endif
  *user = EG_strdup(name);
}


void
caps_fillDateTime(short *datetime)
{
#ifdef WIN32
  SYSTEMTIME sysTime;

  GetLocalTime(&sysTime);
  datetime[0] = sysTime.wYear;
  datetime[1] = sysTime.wMonth;
  datetime[2] = sysTime.wDay;
  datetime[3] = sysTime.wHour;
  datetime[4] = sysTime.wMinute;
  datetime[5] = sysTime.wSecond;

#else
  time_t     tloc;
  struct tm  tim, *dum;
  
  tloc = time(NULL);
/*@-unrecog@*/
  dum  = localtime_r(&tloc, &tim);
/*@+unrecog@*/
  if (dum == NULL) {
    datetime[0] = 1900;
    datetime[1] = 0;
    datetime[2] = 0;
    datetime[3] = 0;
    datetime[4] = 0;
    datetime[5] = 0;
  } else {
    datetime[0] = tim.tm_year + 1900;
    datetime[1] = tim.tm_mon  + 1;
    datetime[2] = tim.tm_mday;
    datetime[3] = tim.tm_hour;
    datetime[4] = tim.tm_min;
    datetime[5] = tim.tm_sec;
  }
#endif
}


void
caps_fillLengthUnits(capsProblem *problem, ego body, char **lunits)
{
  int          status, atype, alen;
  const int    *aints;
  const double *areals;
  const char   *astr;
  ut_unit      *utunit;

  *lunits = NULL;
  status  = EG_attributeRet(body, "capsLength", &atype, &alen, &aints, &areals,
                            &astr);
  if ((status != EGADS_SUCCESS) && (status != EGADS_NOTFOUND)) {
    printf(" CAPS Warning: EG_attributeRet = %d in fillLengthUnits!\n", status);
    return;
  }
  if (status == EGADS_NOTFOUND) return;
  if (atype != ATTRSTRING) {
    printf(" CAPS Warning: capsLength w/ incorrect type in fillLengthUnits!\n");
    return;
  }
  
  utunit = ut_parse((ut_system *) problem->utsystem, astr, UT_ASCII);
  if (utunit == NULL) {
    printf(" CAPS Warning: capsLength %s is not a valid unit!\n", astr);
    return;
  }
/*
  printf(" Body with length units = %s\n", astr);
 */
  ut_free(utunit);
  *lunits = EG_strdup(astr);
}


void
caps_geomOutUnits(char *name, /*@null@*/ char *lunits, char **units)
{
  int         i, len;
  static char *names[21] = {"@xmin", "@xmax", "@ymin", "@ymax", "@zmin",
                            "@zmax", "@length", "@area", "@volume", "@xcg",
                            "@ycg", "@zcg", "@Ixx", "@Ixy", "@Ixz", "@Iyx",
                            "@Iyy", "@Iyz", "@Izx", "@Izy", "@Izz" };
  static int  power[21]  = {1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 4, 4, 4, 4, 4,
                            4, 4, 4, 4};

  *units = NULL;
  if (lunits == NULL) return;
  
  for (i = 0; i < 21; i++)
    if (strcmp(name, names[i]) == 0) {
      if (power[i] == 1) {
        *units = EG_strdup(lunits);
      } else {
        len    = strlen(lunits) + 3;
        *units = (char *) EG_alloc(len*sizeof(char));
        if (*units == NULL) return;
        snprintf(*units, len, "%s^%d", lunits, power[i]);
      }
      return;
    }
}


int
caps_makeTuple(int n, capsTuple **tuple)
{
  int       i;
  capsTuple *tmp;
  
  *tuple = NULL;
  if (n < 1) return CAPS_RANGEERR;
  
  tmp = (capsTuple *) EG_alloc(n*sizeof(capsTuple));
  if (tmp == NULL) return EGADS_MALLOC;
  
  for (i = 0; i < n; i++) tmp[i].name = tmp[i].value = NULL;
  *tuple = tmp;
  
  return CAPS_SUCCESS;
}


void
caps_freeTuple(int n, /*@only@*/ capsTuple *tuple)
{
  int i;
  
  if (tuple == NULL) return;
  for (i = 0; i < n; i++) {
    if (tuple[i].name  != NULL) EG_free(tuple[i].name);
    if (tuple[i].value != NULL) EG_free(tuple[i].value);
  }
  EG_free(tuple);
}


void
caps_freeOwner(capsOwn *own)
{
  if (own->pname != NULL) {
    EG_free(own->pname);
    own->pname = NULL;
  }
  if (own->pID != NULL) {
    EG_free(own->pID);
    own->pID = NULL;
  }
  if (own->user != NULL) {
    EG_free(own->user);
    own->user = NULL;
  }
}


void
caps_freeAttrs(egAttrs **attrx)
{
  int     i;
  egAttrs *attrs;
  
  attrs = *attrx;
  if (attrs == NULL) return;
  
  *attrx = NULL;
  /* remove any attributes */
  for (i = 0; i < attrs->nseqs; i++) {
    EG_free(attrs->seqs[i].root);
    EG_free(attrs->seqs[i].attrSeq);
  }
  if (attrs->seqs != NULL) EG_free(attrs->seqs);
  for (i = 0; i < attrs->nattrs; i++) {
    if (attrs->attrs[i].name != NULL) EG_free(attrs->attrs[i].name);
    if (attrs->attrs[i].type == ATTRINT) {
      if (attrs->attrs[i].length > 1) EG_free(attrs->attrs[i].vals.integers);
    } else if (attrs->attrs[i].type == ATTRREAL) {
      if (attrs->attrs[i].length > 1) EG_free(attrs->attrs[i].vals.reals);
    } else {
      EG_free(attrs->attrs[i].vals.string);
    }
  }
  EG_free(attrs->attrs);
  EG_free(attrs);
}


void
caps_freeValueObjects(int vflag, int nObjs, capsObject **objects)
{
  int       i;
  capsValue *value, *vArray;

  if (objects == NULL) return;
  vArray = (capsValue *) objects[0]->blind;

  for (i = 0; i < nObjs; i++) {

    /* clean up any allocated data arrays */
    value = (capsValue *) objects[i]->blind;
    if (value != NULL) {
      objects[i]->blind = NULL;
      if ((value->type == Boolean) || (value->type == Integer)) {
        if (value->length > 1) EG_free(value->vals.integers);
      } else if (value->type == Double) {
        if (value->length > 1) EG_free(value->vals.reals);
      } else if (value->type == String) {
        if (value->vals.string != NULL) EG_free(value->vals.string);
      } else if (value->type == Tuple) {
        caps_freeTuple(value->length, value->vals.tuple);
      } else {
        if (value->length > 1) {
          caps_freeValueObjects(vflag, value->length, value->vals.objects);
          EG_free(value->vals.objects);
        } else {
          caps_freeValueObjects(vflag, 1, &value->vals.object);
        }
      }
      if (value->units != NULL) EG_free(value->units);
      if (vflag == 1) EG_free(value);
    }
    
    /* cleanup and invalidate the object */
    caps_freeAttrs(&objects[i]->attrs);
    caps_freeOwner(&objects[i]->last);
    objects[i]->magicnumber = 0;
    EG_free(objects[i]->name);
    objects[i]->name = NULL;
    EG_free(objects[i]);
  }
  
  if (vflag == 0) EG_free(vArray);
  EG_free(objects);
}


void
caps_freeAnalysis(int flag, /*@only@*/ capsAnalysis *analysis)
{
  int i;

  if (analysis == NULL) return;
  for (i = 0; i < analysis->nField; i++) EG_free(analysis->fields[i]);
  EG_free(analysis->fields);
  EG_free(analysis->ranks);
  if (analysis->intents  != NULL) EG_free(analysis->intents);
  if (analysis->loadName != NULL) EG_free(analysis->loadName);
  if (analysis->path     != NULL) EG_free(analysis->path);
  if (analysis->parents  != NULL) EG_free(analysis->parents);
  if (analysis->bodies   != NULL) {
    for (i = 0; i < analysis->nBody; i++)
      if (analysis->bodies[i+analysis->nBody] != NULL)
        EG_deleteObject(analysis->bodies[i+analysis->nBody]);
    EG_free(analysis->bodies);
  }
  if (flag == 1) return;

  if (analysis->analysisIn != NULL)
    caps_freeValueObjects(0, analysis->nAnalysisIn,  analysis->analysisIn);
  
  if (analysis->analysisOut != NULL)
    caps_freeValueObjects(0, analysis->nAnalysisOut, analysis->analysisOut);
  
  caps_freeOwner(&analysis->pre);
  EG_free(analysis);
}


int
caps_makeObject(capsObject **objs)
{
  capsObject *objects;

  *objs   = NULL;
  objects = (capsObject *) EG_alloc(sizeof(capsObject));
  if (objects == NULL) return EGADS_MALLOC;

  objects->magicnumber = CAPSMAGIC;
  objects->type        = UNUSED;
  objects->subtype     = NONE;
  objects->sn          = 0;
  objects->name        = NULL;
  objects->attrs       = NULL;
  objects->blind       = NULL;
  objects->parent      = NULL;
  objects->last.pname  = NULL;
  objects->last.pID    = NULL;
  objects->last.user   = NULL;
  objects->last.sNum   = 0;
  caps_fillDateTime(objects->last.datetime);

  *objs = objects;
  return CAPS_SUCCESS;
}


int
caps_makeVal(enum capsvType type, int len, const void *data, capsValue **val)
{
  char             *chars;
  int              *ints, j;
  double           *reals;
  enum capsBoolean *bools;
  capsValue        *value;
  capsObject       **objs;
  capsTuple        *tuple;
  
  *val  = NULL;
  if (data  == NULL) return CAPS_NULLVALUE;
  value = (capsValue *) EG_alloc(sizeof(capsValue));
  if (value == NULL) return EGADS_MALLOC;
  value->length  = len;
  value->type    = type;
  value->nrow    = value->ncol   = value->dim = value->pIndex = 0;
  value->lfixed  = value->sfixed = Fixed;
  value->nullVal = NotAllowed;
  value->units   = NULL;
  value->link    = NULL;
  value->limits.dlims[0] = value->limits.dlims[1] = 0.0;
  value->linkMethod      = Copy;
 
  if (data == NULL) {
    value->nullVal = IsNull;
    if (type == Boolean) {
      if (value->length <= 1) {
        value->vals.integer = false;
      } else {
        value->vals.integers = (int *) EG_alloc(value->length*sizeof(int));
        if (value->vals.integers == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.integers[j] = false;
      }
    } else if (type == Integer) {
      if (value->length <= 1) {
        value->vals.integer = 0;
      } else {
        value->vals.integers = (int *) EG_alloc(value->length*sizeof(int));
        if (value->vals.integers == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.integers[j] = 0;
      }
    } else if (type == Double) {
      if (value->length <= 1) {
        value->vals.real = 0.0;
      } else {
        value->vals.reals = (double *) EG_alloc(value->length*sizeof(double));
        if (value->vals.reals == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.reals[j] = 0.0;
      }
    } else if (type == String) {
      if (len <= 0) value->length = 1;
      value->vals.string = (char *) EG_alloc(value->length*sizeof(char));
      if (value->vals.string == NULL) {
        EG_free(value);
        return EGADS_MALLOC;
      }
      for (j = 0; j < value->length; j++) value->vals.string[j] = 0;
      if (value->length == 1) value->length = 2;
    } else if (type == Tuple) {
      value->vals.tuple = NULL;
      if (len > 0) {
        j = caps_makeTuple(len, &value->vals.tuple);
        if ((j != CAPS_SUCCESS) || (value->vals.tuple == NULL)) {
          if (value->vals.tuple == NULL) j = CAPS_NULLVALUE;
          EG_free(value);
          return j;
        }
      }
    } else {
      if (value->length <= 1) {
        value->vals.object = NULL;
      } else {
        value->vals.objects = (capsObject **)
                              EG_alloc(value->length*sizeof(capsObject *));
        if (value->vals.objects == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.objects[j] = NULL;
      }
    }
  } else {
    if (type == Boolean) {
      bools = (enum capsBoolean *) data;
      if (value->length == 1) {
        value->vals.integer = bools[0];
      } else {
        value->vals.integers = (int *) EG_alloc(value->length*sizeof(int));
        if (value->vals.integers == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.integers[j] = bools[j];
      }
    } else if (type == Integer) {
      ints = (int *) data;
      if (value->length == 1) {
        value->vals.integer = ints[0];
      } else {
        value->vals.integers = (int *) EG_alloc(value->length*sizeof(int));
        if (value->vals.integers == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.integers[j] = ints[j];
      }
    } else if (type == Double) {
      reals = (double *) data;
      if (value->length == 1) {
        value->vals.real = reals[0];
      } else {
        value->vals.reals = (double *) EG_alloc(value->length*sizeof(double));
        if (value->vals.reals == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.reals[j] = reals[j];
      }
    } else if (type == String) {
      chars = (char *) data;
      value->length      = strlen(chars)+1;
      value->vals.string = (char *) EG_alloc(value->length*sizeof(char));
      if (value->vals.string == NULL) {
        EG_free(value);
        return EGADS_MALLOC;
      }
      for (j = 0; j < value->length; j++) value->vals.string[j] = chars[j];
      if (value->length == 1) value->length = 2;
    } else if (type == Tuple) {
      value->vals.tuple = NULL;
      if (len > 0) {
        j = caps_makeTuple(len, &value->vals.tuple);
        if ((j != CAPS_SUCCESS) || (value->vals.tuple == NULL)) {
          if (value->vals.tuple == NULL) j = CAPS_NULLVALUE;
          EG_free(value);
          return j;
        }
        tuple = (capsTuple *) data;
        for (j = 0; j < len; j++) {
          value->vals.tuple[j].name  = EG_strdup(tuple[j].name);
          value->vals.tuple[j].value = EG_strdup(tuple[j].value);
          if ((tuple[j].name  != NULL) &&
              (value->vals.tuple[j].name  == NULL)) {
            EG_free(value);
            return EGADS_MALLOC;
          }
          if ((tuple[j].value != NULL) &&
              (value->vals.tuple[j].value == NULL)) {
            EG_free(value);
            return EGADS_MALLOC;
          }
        }
      }
    } else {
      objs = (capsObject **) data;
      if (value->length == 1) {
        value->vals.object = objs[0];
      } else {
        value->vals.objects = (capsObject **)
        EG_alloc(value->length*sizeof(capsObject *));
        if (value->vals.objects == NULL) {
          EG_free(value);
          return EGADS_MALLOC;
        }
        for (j = 0; j < value->length; j++) value->vals.objects[j] = objs[j];
      }
    }
  }
  if (value->length > 1) value->dim = Vector;
    
  *val = value;
  return CAPS_SUCCESS;
}


int
caps_findProblem(const capsObject *object, capsObject **pobject)
{
  capsObject *pobj;
  
  *pobject = NULL;
  if (object == NULL) return CAPS_NULLOBJ;

  pobj = (capsObject *) object;
  do {
    if (pobj->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
    if (pobj->type        == PROBLEM) {
      if (pobj->blind == NULL) return CAPS_NULLBLIND;
      *pobject = pobj;
      return CAPS_SUCCESS;
    }
    pobj = pobj->parent;
  } while (pobj != NULL);
  
  return CAPS_NOTPROBLEM;
}


int
caps_makeSimpleErr(/*@null@*/ capsObject *object, const char *line1,
                   /*@null@*/ const char *line2, /*@null@*/ const char *line3,
                   /*@null@*/ const char *line4, capsErrs **errs)
{
  int      i;
  capsErrs *error;
  
  *errs = NULL;
  error = (capsErrs *) EG_alloc(sizeof(capsErrs));
  if (error == NULL) return EGADS_MALLOC;
  error->nError = 1;
  error->errors = (capsError *) EG_alloc(sizeof(capsError));
  if (error->errors == NULL) {
    EG_free(error);
    return EGADS_MALLOC;
  }
  
  i = 0;
  if (line1 != NULL) i++;
  if (line2 != NULL) i++;
  if (line3 != NULL) i++;
  if (line4 != NULL) i++;
  error->errors->errObj = object;
  error->errors->nLines = i;
  error->errors->lines  = NULL;
  if (i != 0) {
    error->errors->lines = (char **) EG_alloc(i*sizeof(char *));
    if (error->errors->lines == NULL) {
      EG_free(error->errors);
      EG_free(error);
      return EGADS_MALLOC;
    }
    i = 0;
    if (line1 != NULL) {
      error->errors->lines[i] = EG_strdup(line1);
      i++;
    }
    if (line2 != NULL) {
      error->errors->lines[i] = EG_strdup(line2);
      i++;
    }
    if (line3 != NULL) {
      error->errors->lines[i] = EG_strdup(line3);
      i++;
    }
    if (line4 != NULL) {
      error->errors->lines[i] = EG_strdup(line4);
      i++;
    }
  }
  
  *errs = error;
  return CAPS_SUCCESS;
}


/************************* CAPS exposed functions ****************************/


int
caps_info(const capsObject *object, char **name, enum capsoType *type,
          enum capssType *subtype, capsObject **link, capsObject **parent,
          capsOwn *last)
{
  int        i, stat;
  capsValue  *value;
  capsObject *pobj;
  
  *name      = NULL;
  *type      = UNUSED;
  *subtype   = NONE;
  *link      = *parent     = NULL;
  last->user = last->pname = last->pID = NULL;
  for (i = 0; i < 6; i++) last->datetime[i] = 0;

  if (object              == NULL)      return CAPS_NULLOBJ;
  if (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (object->blind       == NULL)      return CAPS_NULLBLIND;

  stat     = caps_findProblem(object, &pobj);
  *type    = object->type;
  *subtype = object->subtype;
  *name    = object->name;
  *parent  = object->parent;
  if (object->type == VALUE) {
    value  = (capsValue *) object->blind;
    *link  = value->link;
  }
  if (object->last.pname == NULL) {
    if (stat == CAPS_SUCCESS) {
      last->pname = pobj->last.pname;
      last->user  = pobj->last.user;
      last->pID   = pobj->last.pID;
    }
  } else {
    last->pname = object->last.pname;
    last->user  = object->last.user;
    last->pID   = object->last.pID;
  }
  last->sNum = object->last.sNum;
  for (i = 0; i < 6; i++) last->datetime[i] = object->last.datetime[i];
  
  return CAPS_SUCCESS;
}


int
caps_size(const capsObject *object, enum capsoType type, enum capssType stype,
          int *size)
{
  int           i, status;
  egAttrs       *attrs;
  capsObject    *pobject;
  capsProblem   *problem;
  capsValue     *value;
  capsAnalysis  *analysis;
  capsBound     *bound;
  capsVertexSet *vertexSet;
  
  *size = 0;
  if (object              == NULL)      return CAPS_NULLOBJ;
  if (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (object->blind       == NULL)      return CAPS_NULLBLIND;
  
  if (object->type == PROBLEM) {
  
    problem = (capsProblem *) object->blind;
    if (type == BODIES) {
      *size = problem->nBodies;
    } else if (type == ATTRIBUTES) {
      attrs = object->attrs;
      if (attrs != NULL) *size = attrs->nattrs;
    } else if (type == VALUE) {
      if (stype == GEOMETRYIN)  *size = problem->nGeomIn;
      if (stype == GEOMETRYOUT) *size = problem->nGeomOut;
      if (stype == BRANCH)      *size = problem->nBranch;
      if (stype == PARAMETER)   *size = problem->nParam;
    } else if (type == ANALYSIS) {
      *size = problem->nAnalysis;
    } else if (type == BOUND) {
      *size = problem->nBound;
    }
    
  } else if (object->type == VALUE) {

    value = (capsValue *) object->blind;
    if (type == ATTRIBUTES) {
      attrs = object->attrs;
      if (attrs != NULL) *size = attrs->nattrs;
    } else if ((type == VALUE) && (value->type == Value)) {
      *size = value->length;
    }
    
  } else if (object->type == ANALYSIS) {
    
    analysis = (capsAnalysis *) object->blind;
    if (type == ATTRIBUTES) {
      attrs = object->attrs;
      if (attrs != NULL) *size = attrs->nattrs;
    } else if (type == VALUE) {
      if (stype == ANALYSISIN)  *size = analysis->nAnalysisIn;
      if (stype == ANALYSISOUT) *size = analysis->nAnalysisOut;
    } else if (type == BODIES) {
      pobject = (capsObject *) object->parent;
      if (pobject->blind == NULL) return CAPS_NULLBLIND;
      problem = (capsProblem *) pobject->blind;
      if ((problem->nBodies > 0) && (problem->bodies != NULL)) {
        if (analysis->bodies == NULL) {
          status = caps_filter(problem, analysis);
          if (status != CAPS_SUCCESS) return status;
        }
        *size = analysis->nBody;
      }
    }
    
  } else if (object->type == BOUND) {
    
    if (type == ATTRIBUTES) {
      attrs = object->attrs;
      if (attrs != NULL) *size = attrs->nattrs;
    } else if (type == VERTEXSET) {
      bound = (capsBound *) object->blind;
      for (*size = i = 0; i < bound->nVertexSet; i++)
        if (bound->vertexSet[i]->subtype == stype) *size += 1;
    }
    
  } else if (object->type == VERTEXSET) {
    
    if (type == ATTRIBUTES) {
      attrs = object->attrs;
      if (attrs != NULL) *size = attrs->nattrs;
    } else if (type == DATASET) {
      vertexSet = (capsVertexSet *) object->blind;
      *size = vertexSet->nDataSets;
    }
    
  } else if (object->type == DATASET) {

    if (type == ATTRIBUTES) {
      attrs = object->attrs;
      if (attrs != NULL) *size = attrs->nattrs;
    }

  }
  
  return CAPS_SUCCESS;
}


int
caps_childByIndex(const capsObject *object, enum capsoType type,
                  enum capssType stype, int index, capsObject **child)
{
  int           i, j;
  capsProblem   *problem;
  capsValue     *value;
  capsAnalysis  *analysis;
  capsBound     *bound;
  capsVertexSet *vertexSet;
  
  *child = NULL;
  if (object              == NULL)      return CAPS_NULLOBJ;
  if (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (object->blind       == NULL)      return CAPS_NULLBLIND;
  if (index               <= 0)         return CAPS_RANGEERR;
  
  if (object->type == PROBLEM) {
    
    problem = (capsProblem *) object->blind;
    if (type == VALUE) {
      if (stype == GEOMETRYIN) {
        if (index > problem->nGeomIn)   return CAPS_RANGEERR;
        *child = problem->geomIn[index-1];
      }
      if (stype == GEOMETRYOUT) {
        if (index > problem->nGeomOut)  return CAPS_RANGEERR;
        *child = problem->geomOut[index-1];
      }
      if (stype == BRANCH) {
        if (index > problem->nBranch)   return CAPS_RANGEERR;
        *child = problem->branchs[index-1];
      }
      if (stype == PARAMETER) {
        if (index > problem->nParam)    return CAPS_RANGEERR;
        *child = problem->params[index-1];
      }
    } else if (type == ANALYSIS) {
      if (index > problem->nAnalysis)   return CAPS_RANGEERR;
      *child = problem->analysis[index-1];
    } else if (type == BOUND) {
      if (index  > problem->nBound)     return CAPS_RANGEERR;
      *child = problem->bounds[index-1];
    }
    
  } else if (object->type == VALUE) {
    
    value = (capsValue *) object->blind;
    if ((type == VALUE) && (value->type == Value)) {
      if (index > value->length)        return CAPS_RANGEERR;
      if (value->length == 1) *child = value->vals.object;
      if (value->length != 1) *child = value->vals.objects[index-1];
    }
    
  } else if (object->type == ANALYSIS) {
    
    analysis = (capsAnalysis *) object->blind;
    if (type == VALUE) {
      if (stype == ANALYSISIN) {
        if (index > analysis->nAnalysisIn) return CAPS_RANGEERR;
        *child = analysis->analysisIn[index-1];
      }
      if (stype == ANALYSISOUT) {
        if (index > analysis->nAnalysisOut) return CAPS_RANGEERR;
        *child = analysis->analysisOut[index-1];
      }
    }
    
  } else if (object->type == BOUND) {
    
    bound = (capsBound *) object->blind;
    for (j = i = 0; i < bound->nVertexSet; i++) {
      if (bound->vertexSet[i]->subtype == stype) j++;
      if (j != index) continue;
      *child = bound->vertexSet[i];
      break;
    }
    
  } else if (object->type == VERTEXSET) {
    
    vertexSet = (capsVertexSet *) object->blind;
    if (type == DATASET) {
      if (index > vertexSet->nDataSets) return CAPS_RANGEERR;
      *child = vertexSet->dataSets[index-1];
    }
    
  }
  
  if (*child == NULL) return CAPS_NOTFOUND;
  return CAPS_SUCCESS;
}



static int
caps_findByName(const char *name, int len, capsObject **objs,
                capsObject **child)
{
  int i;
  
  if (objs == NULL) return CAPS_NOTFOUND;
  
  for (i = 0; i < len; i++) {
    if (objs[i]       == NULL) continue;
    if (objs[i]->name == NULL) continue;
    if (strcmp(objs[i]->name, name) == 0) {
      *child = objs[i];
      return CAPS_SUCCESS;
    }
  }
  
  return CAPS_NOTFOUND;
}


int
caps_childByName(const capsObject *object, enum capsoType type,
                 enum capssType stype, const char *name, capsObject **child)
{
  capsObject    **objs;
  capsProblem   *problem;
  capsValue     *value;
  capsAnalysis  *analysis;
  capsVertexSet *vertexSet;
  
  *child = NULL;
  if (object              == NULL)      return CAPS_NULLOBJ;
  if (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (object->blind       == NULL)      return CAPS_NULLBLIND;
  if (name                == NULL)      return CAPS_NULLNAME;
  
  if (object->type == PROBLEM) {
    
    problem = (capsProblem *) object->blind;
    if (type == VALUE) {
      if (stype == GEOMETRYIN)
        return caps_findByName(name, problem->nGeomIn,
                                     problem->geomIn,  child);
      if (stype == GEOMETRYOUT)
        return caps_findByName(name, problem->nGeomOut,
                                     problem->geomOut, child);
      if (stype == BRANCH)
        return caps_findByName(name, problem->nBranch,
                                     problem->branchs, child);
      if (stype == PARAMETER)
        return caps_findByName(name, problem->nParam,
                                     problem->params,  child);
    } else if (type == ANALYSIS) {
      return caps_findByName(name, problem->nAnalysis,
                                   problem->analysis,  child);
    } else if (type == BOUND) {
      return caps_findByName(name, problem->nBound,
                                   problem->bounds,    child);
    }
    
  } else if (object->type == VALUE) {
    
    value = (capsValue *) object->blind;
    if ((type == VALUE) && (value->type == Value)) {
      if (value->length == 1) objs = &value->vals.object;
      if (value->length != 1) objs =  value->vals.objects;
      return caps_findByName(name, value->length, objs, child);
    }
    
  } else if (object->type == ANALYSIS) {
    
    analysis = (capsAnalysis *) object->blind;
    if (type == VALUE) {
      if (stype == ANALYSISIN)
        return caps_findByName(name, analysis->nAnalysisIn,
                                     analysis->analysisIn,  child);
      if (stype == ANALYSISOUT) {
        return caps_findByName(name, analysis->nAnalysisOut,
                                     analysis->analysisOut, child);
      }
    }
    
  } else if (object->type == VERTEXSET) {
    
    vertexSet = (capsVertexSet *) object->blind;
    if (type == DATASET)
      return caps_findByName(name, vertexSet->nDataSets,
                                   vertexSet->dataSets, child);
    
  }

  return CAPS_NOTFOUND;
}


int
caps_bodyByIndex(const capsObject *object, int index, ego *body, char **lunits)
{
  int          i, ind, status;
  capsObject   *pobject;
  capsProblem  *problem;
  capsAnalysis *analysis;
  
  *body   = NULL;
  *lunits = NULL;
  if  (object              == NULL)      return CAPS_NULLOBJ;
  if  (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if ((object->type        != PROBLEM) &&
      (object->type        != ANALYSIS)) return CAPS_BADTYPE;
  if  (object->blind       == NULL)      return CAPS_NULLBLIND;
  
  if (object->type == PROBLEM) {
    
    if (index               <= 0)        return CAPS_RANGEERR;
    problem = (capsProblem *) object->blind;
    if (index > problem->nBodies)        return CAPS_RANGEERR;
  
    *body   = problem->bodies[index-1];
    *lunits = problem->lunits[index-1];
    
  } else {
    
    if (index             == 0)          return CAPS_RANGEERR;
    analysis = (capsAnalysis *) object->blind;
    pobject  = (capsObject *) object->parent;
    if (pobject->blind == NULL)          return CAPS_NULLBLIND;
    problem = (capsProblem *) pobject->blind;
    if ((problem->nBodies > 0) && (problem->bodies != NULL)) {
      if (analysis->bodies == NULL) {
        status = caps_filter(problem, analysis);
        if (status != CAPS_SUCCESS) return status;
      }
      ind = index;
      if (ind < 0) ind = -index;
      if (ind > analysis->nBody)         return CAPS_RANGEERR;
      *body = analysis->bodies[ind-1];
      for (i = 0; i < problem->nBodies; i++)
        if (*body == problem->bodies[i]) {
          *lunits = problem->lunits[i];
          break;
        }
      if (index < 0) *body = analysis->bodies[analysis->nBody + ind-1];
    }
    
  }
  
  return CAPS_SUCCESS;
}


int
caps_ownerInfo(const capsOwn owner, char **pname, char **pID, char **userID,
               short *datetime, CAPSLONG *sNum)
{
  int i;
  
  *pname  = owner.pname;
  *pID    = owner.pID;
  *userID = owner.user;
  *sNum   = owner.sNum;
  for (i = 0; i < 6; i++) datetime[i] = owner.datetime[i];
  
  return CAPS_SUCCESS;
}


int
caps_setOwner(const capsObject *pobject, const char *pname, capsOwn *owner )
{
  char        **signature;
  capsProblem *problem;

  if (pobject              == NULL)      return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type        != PROBLEM)   return CAPS_BADTYPE;
  if (pobject->blind       == NULL)      return CAPS_NULLBLIND;
  if (pname                == NULL)      return CAPS_NULLNAME;
  problem = (capsProblem *) pobject->blind;

  problem->sNum++;
  caps_getStaticStrings(&signature, &owner->pID, &owner->user);
  owner->pname = EG_strdup(pname);
  owner->sNum  = problem->sNum;
  caps_fillDateTime(owner->datetime);
  return CAPS_SUCCESS;
}


int
caps_delete(capsObject *object)
{
  int           i, j, k;
  capsObject    *pobject;
  capsProblem   *problem;
  capsValue     *value;
  capsBound     *bound;
  capsAnalysis  *analysis;
  capsVertexSet *vertexSet;
  capsDataSet   *dataSet;
  
  if  (object              == NULL)      return CAPS_NULLOBJ;
  if  (object->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if ((object->type        != VALUE) &&
      (object->type        != BOUND))    return CAPS_BADTYPE;
  if ((object->type        == VALUE) &&
      (object->subtype     != USER))     return CAPS_BADTYPE;
  if  (object->blind       == NULL)      return CAPS_NULLBLIND;
  
  /* clean up any allocated data arrays */
  if (object->type == VALUE) {
    
    value = (capsValue *) object->blind;
    object->blind = NULL;
    if ((value->type == Boolean) || (value->type == Integer)) {
      if (value->length > 1) EG_free(value->vals.integers);
    } else if (value->type == Double) {
      if (value->length > 1) EG_free(value->vals.reals);
    } else if (value->type == String) {
      if (value->length > 1) EG_free(value->vals.string);
    } else if (value->type == Tuple) {
      caps_freeTuple(value->length, value->vals.tuple);
    } else {
      if (value->length > 1) EG_free(value->vals.objects);
    }
    if (value->units != NULL) EG_free(value->units);
    EG_free(value);
    
  } else {
    
    pobject = object->parent;
    if (pobject == NULL) return CAPS_NULLOBJ;
    problem = (capsProblem *) pobject->blind;
    if (problem == NULL) return CAPS_NULLBLIND;
    bound   = (capsBound *) object->blind;
    if (bound->curve   != NULL) caps_Aprx1DFree(bound->curve);
    if (bound->surface != NULL) caps_Aprx2DFree(bound->surface);
    if (bound->lunits  != NULL) EG_free(bound->lunits);
    for (i = 0; i < bound->nVertexSet; i++) {
      if (bound->vertexSet[i]->magicnumber != CAPSMAGIC) continue;
      if (bound->vertexSet[i]->blind       == NULL)      continue;

      vertexSet = (capsVertexSet *) bound->vertexSet[i]->blind;
      if ((vertexSet->analysis != NULL) && (vertexSet->discr != NULL))
        if (vertexSet->analysis->blind != NULL) {
          analysis = (capsAnalysis *) vertexSet->analysis->blind;
          aim_FreeDiscr(problem->aimFPTR, analysis->loadName, vertexSet->discr);
          EG_free(vertexSet->discr);
        }
      for (j = 0; j < vertexSet->nDataSets; j++) {
        if (vertexSet->dataSets[j]->magicnumber != CAPSMAGIC) continue;
        if (vertexSet->dataSets[j]->blind       == NULL)      continue;

        dataSet = (capsDataSet *) vertexSet->dataSets[j]->blind;
        for (k = 0; k < dataSet->nHist; k++)
          caps_freeOwner(&dataSet->history[k]);
        if (dataSet->data    != NULL) EG_free(dataSet->data);
        if (dataSet->units   != NULL) EG_free(dataSet->units);
        if (dataSet->history != NULL) EG_free(dataSet->history);
        if (dataSet->startup != NULL) EG_free(dataSet->startup);
        EG_free(dataSet);
        
        caps_freeAttrs(&vertexSet->dataSets[j]->attrs);
        caps_freeOwner(&vertexSet->dataSets[j]->last);
        vertexSet->dataSets[j]->magicnumber = 0;
        EG_free(vertexSet->dataSets[j]->name);
        vertexSet->dataSets[j]->name = NULL;
        EG_free(vertexSet->dataSets[j]);
      }
      EG_free(vertexSet->dataSets);
      vertexSet->dataSets = NULL;
      EG_free(vertexSet);

      caps_freeAttrs(&bound->vertexSet[i]->attrs);
      caps_freeOwner(&bound->vertexSet[i]->last);
      bound->vertexSet[i]->magicnumber = 0;
      EG_free(bound->vertexSet[i]->name);
      bound->vertexSet[i]->name = NULL;
      EG_free(bound->vertexSet[i]);
    }
    EG_free(bound->vertexSet);
    bound->vertexSet = NULL;
    EG_free(bound);
  
  }
  
  /* cleanup and invalidate the object */
  
  caps_freeAttrs(&object->attrs);
  caps_freeOwner(&object->last);
  object->magicnumber = 0;
  EG_free(object->name);
  object->name = NULL;
  EG_free(object);
  
  return CAPS_SUCCESS;
}


int
caps_errorInfo(capsErrs *errs, int eIndex, capsObject **errObj,
               int *nLines, char ***lines)
{
  *errObj = NULL;
  *nLines = 0;
  *lines  = NULL;
  if ((eIndex < 1) || (eIndex > errs->nError)) return CAPS_BADINDEX;
  
  *errObj = errs->errors[eIndex-1].errObj;
  *nLines = errs->errors[eIndex-1].nLines;
  *lines  = errs->errors[eIndex-1].lines;
  return CAPS_SUCCESS;
}


int
caps_freeError(/*@only@*/ capsErrs *errs)
{
  int i, j;
  
  for (i = 0; i < errs->nError; i++)
    for (j = 0; j < errs->errors[i].nLines; j++)
      EG_free(errs->errors[i].lines[j]);
  
  EG_free(errs->errors);
  errs->nError = 0;
  errs->errors = NULL;
  EG_free(errs);

  return CAPS_SUCCESS;
}
