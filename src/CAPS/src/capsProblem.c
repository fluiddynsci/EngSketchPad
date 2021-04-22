/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Problem Object Functions
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <strings.h>
#include <unistd.h>
#else
#define strcasecmp  stricmp
#define snprintf   _snprintf
#define unlink     _unlink
#endif

#include "udunits.h"

#include "capsBase.h"
#include "capsAIM.h"

/* OpenCSM Defines & Includes */
#include "common.h"
#include "OpenCSM.h"
#include "udp.h"

extern /*@null@*/ /*@only@*/ char *EG_strdup(/*@null@*/ const char *str);

extern int  caps_checkDiscr(capsDiscr *discr, int l, char *line);
extern int  caps_Aprx1DFree(/*@only@*/ capsAprx1D *approx);
extern int  caps_Aprx2DFree(/*@only@*/ capsAprx2D *approx);
extern int  caps_filter(capsProblem *problem, capsAnalysis *analysis);
extern int  caps_checkIntent(int intent);

extern int  caps_analysisInfo(const capsObj aobject, char **apath, char **unSys,
                              char **intents, int *nparent, capsObj **parents,
                              int *nField, char ***fnames, int **ranks,
                              int *execution, int *status);



static int
caps_writeString(FILE *fp, /*@null@*/ const char *string)
{
  int    len = 0;
  size_t n;

  if (string != NULL) len = strlen(string) + 1;
  n = fwrite(&len, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;

  if (string == NULL) return CAPS_SUCCESS;
  n = fwrite(string, sizeof(char), len, fp);
  if (n != len) return CAPS_IOERR;

  return CAPS_SUCCESS;
}


static int
caps_readString(FILE *fp, char **string)
{
  int    len;
  size_t n;

  *string = NULL;
  n = fread(&len, sizeof(int), 1, fp);
  if (n   != 1) return CAPS_IOERR;
  if (len <  0) return CAPS_IOERR;
  if (len == 0) return CAPS_SUCCESS;

  *string = (char *) EG_alloc(len*sizeof(char));
  if (*string == NULL) return EGADS_MALLOC;

  n = fread(*string, sizeof(char), len, fp);
  if (n != len) {
    EG_free(*string);
    *string = NULL;
    return CAPS_IOERR;
  }

  return CAPS_SUCCESS;
}


static int
caps_writeTuple(FILE *fp, int len, enum capsNull nullVal,
                /*@null@*/ capsTuple *tuple)
{
  int i, stat;

  if (len < 0) return CAPS_BADINDEX;
  if ((tuple == NULL) && (len != 0) && (nullVal != IsNull))
    return CAPS_NULLVALUE;

  if (tuple == NULL) return CAPS_SUCCESS;
  for (i = 0; i < len; i++) {
    stat = caps_writeString(fp, tuple[i].name);
    if (stat != CAPS_SUCCESS) return stat;
    stat = caps_writeString(fp, tuple[i].value);
    if (stat != CAPS_SUCCESS) return stat;
  }

  return CAPS_SUCCESS;
}


static int
caps_readTuple(FILE *fp, int len, enum capsNull nullVal, capsTuple **tuple)
{
  int       i, stat;
  capsTuple *tmp;

  *tuple = NULL;
  if (nullVal == IsNull) return CAPS_SUCCESS;
  stat = caps_makeTuple(len, &tmp);
  if (stat != CAPS_SUCCESS) return stat;

  for (i = 0; i < len; i++) {
    stat = caps_readString(fp, &tmp[i].name);
    if (stat != CAPS_SUCCESS) {
      caps_freeTuple(len, tmp);
      return stat;
    }
    stat = caps_readString(fp, &tmp[i].value);
    if (stat != CAPS_SUCCESS) {
      caps_freeTuple(len, tmp);
      return stat;
    }
  }
  *tuple = tmp;

  return CAPS_SUCCESS;
}


static int
caps_writeDoubles(FILE *fp, int len, /*@null@*/ const double *reals)
{
  size_t n;

  if (len < 0) return CAPS_BADINDEX;
  if ((reals == NULL) && (len != 0)) return CAPS_NULLVALUE;
  n = fwrite(&len, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;

  if (reals == NULL) return CAPS_SUCCESS;
  n = fwrite(reals, sizeof(double), len, fp);
  if (n != len) return CAPS_IOERR;

  return CAPS_SUCCESS;
}


static int
caps_readDoubles(FILE *fp, int *len, double **reals)
{
  size_t n;

  *reals = NULL;
  n = fread(len, sizeof(int), 1, fp);
  if (n    != 1) return CAPS_IOERR;
  if (*len <  0) return CAPS_IOERR;
  if (*len == 0) return CAPS_SUCCESS;

  *reals = (double *) EG_alloc(*len*sizeof(double));
  if (*reals == NULL) return EGADS_MALLOC;

  n = fread(*reals, sizeof(double), *len, fp);
  if (n != *len) {
    EG_free(*reals);
    *reals = NULL;
    return CAPS_IOERR;
  }

  return CAPS_SUCCESS;
}


static int
caps_writeAttrs(FILE *fp, egAttrs *attrs)
{
  int    nattr, i, stat;
  size_t n;
  egAttr *attr;

  nattr = 0;
  if (attrs != NULL) nattr = attrs->nattrs;
  n = fwrite(&nattr, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if ((nattr == 0) || (attrs == NULL)) return CAPS_SUCCESS;

  attr = attrs->attrs;
  for (i = 0; i < nattr; i++) {
    n = fwrite(&attr[i].type,   sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    n = fwrite(&attr[i].length, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    stat = caps_writeString(fp, attr[i].name);
    if (stat != CAPS_SUCCESS) return CAPS_IOERR;
    if (attr[i].type == ATTRINT) {
      n = attr[i].length;
      if (attr[i].length == 1) {
        n = fwrite(&attr[i].vals.integer, sizeof(int),              1, fp);
      } else if (attr[i].length > 1) {
        n = fwrite(attr[i].vals.integers, sizeof(int), attr[i].length, fp);
      }
      if (n != attr[i].length) return CAPS_IOERR;
    } else if (attr[i].type == ATTRREAL) {
      n = attr[i].length;
      if (attr[i].length == 1) {
        n = fwrite(&attr[i].vals.real, sizeof(double),              1, fp);
      } else if (attr[i].length > 1) {
        n = fwrite(attr[i].vals.reals, sizeof(double), attr[i].length, fp);
      }
      if (n != attr[i].length) return CAPS_IOERR;
    } else {
      stat = caps_writeString(fp, attr[i].vals.string);
      if (stat != CAPS_SUCCESS) return CAPS_IOERR;
    }
  }

  return CAPS_SUCCESS;
}


static int
caps_readAttrs(FILE *fp, egAttrs **attrx)
{
  int     nattr, i, stat;
  size_t  n;
  egAttr  *attr;
  egAttrs *attrs;

  *attrx = NULL;
  n = fread(&nattr, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (nattr == 0) return CAPS_SUCCESS;

  attrs = (egAttrs *) EG_alloc(sizeof(egAttrs));
  if (attrs == NULL) return EGADS_MALLOC;
  attr  = (egAttr *)  EG_alloc(nattr*sizeof(egAttr));
  if (attr == NULL) {
    EG_free(attrs);
    return EGADS_MALLOC;
  }
  attrs->nattrs = nattr;
  attrs->attrs  = attr;
  attrs->nseqs  = 0;
  attrs->seqs   = NULL;
  for (i = 0; i < nattr; i++) {
    attr[i].name   = NULL;
    attr[i].length = 1;
    attr[i].type   = ATTRINT;
  }

/*@-mustfreefresh@*/
  for (i = 0; i < nattr; i++) {
    n = fread(&attr[i].type,   sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeAttrs(&attrs);
      return CAPS_IOERR;
    }
    n = fread(&attr[i].length, sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeAttrs(&attrs);
      return CAPS_IOERR;
    }
    stat = caps_readString(fp, &attr[i].name);
    if (stat != CAPS_SUCCESS) {
      caps_freeAttrs(&attrs);
      return CAPS_IOERR;
    }
    if (attr[i].type == ATTRINT) {
      n = attr[i].length;
      if (attr[i].length == 1) {
        n = fread(&attr[i].vals.integer, sizeof(int),              1, fp);
      } else if (attr[i].length > 1) {
        attr[i].vals.integers = (int *) EG_alloc(attr[i].length*sizeof(int));
        if (attr[i].vals.integers == NULL) {
          caps_freeAttrs(&attrs);
          return EGADS_MALLOC;
        }
        n = fread(attr[i].vals.integers, sizeof(int), attr[i].length, fp);
      }
      if (n != attr[i].length) {
        caps_freeAttrs(&attrs);
        return CAPS_IOERR;
      }
    } else if (attr[i].type == ATTRREAL) {
      n = attr[i].length;
      if (attr[i].length == 1) {
        n = fread(&attr[i].vals.real, sizeof(double),              1, fp);
      } else if (attr[i].length > 1) {
        attr[i].vals.reals = (double *) EG_alloc(attr[i].length*sizeof(double));
        if (attr[i].vals.reals == NULL) {
          caps_freeAttrs(&attrs);
          return EGADS_MALLOC;
        }
        n = fread(attr[i].vals.reals, sizeof(double), attr[i].length, fp);
      }
      if (n != attr[i].length) return CAPS_IOERR;
    } else {
      stat = caps_readString(fp, &attr[i].vals.string);
      if (stat != CAPS_SUCCESS) return CAPS_IOERR;
    }
  }
/*@+mustfreefresh@*/

  *attrx = attrs;
  return CAPS_SUCCESS;
}


static int
caps_writeOwn(FILE *fp, capsOwn writer, capsOwn own)
{
  int    stat;
  size_t n;

  if (own.pname == NULL) {
    stat = caps_writeString(fp, writer.pname);
  } else {
    stat = caps_writeString(fp, own.pname);
  }
  if (stat != CAPS_SUCCESS) return stat;
  if (own.pID == NULL) {
    stat = caps_writeString(fp, writer.pID);
  } else {
    stat = caps_writeString(fp, own.pID);
  }
  if (stat != CAPS_SUCCESS) return stat;
  if (own.user == NULL) {
    stat = caps_writeString(fp, writer.user);
  } else {
    stat = caps_writeString(fp, own.user);
  }
  if (stat != CAPS_SUCCESS) return stat;

  n = fwrite(own.datetime, sizeof(short), 6, fp);
  if (n != 6) return CAPS_IOERR;

  n = fwrite(&own.sNum, sizeof(CAPSLONG), 1, fp);
  if (n != 1) return CAPS_IOERR;

  return CAPS_SUCCESS;
}


static int
caps_readOwn(FILE *fp, capsOwn *own)
{
  int    stat;
  size_t n;

  own->pname = NULL;
  own->pID   = NULL;
  own->user  = NULL;
  stat = caps_readString(fp, &own->pname);
  if (stat != CAPS_SUCCESS) return stat;
  stat = caps_readString(fp, &own->pID);
  if (stat != CAPS_SUCCESS) {
    caps_freeOwner(own);
    return stat;
  }
  stat = caps_readString(fp, &own->user);
  if (stat != CAPS_SUCCESS) {
    caps_freeOwner(own);
    return stat;
  }

  n = fread(own->datetime, sizeof(short), 6, fp);
  if (n != 6) {
    caps_freeOwner(own);
    return CAPS_IOERR;
  }

  n = fread(&own->sNum, sizeof(CAPSLONG), 1, fp);
  if (n != 1) {
    caps_freeOwner(own);
    return CAPS_IOERR;
  }

  return CAPS_SUCCESS;
}


static int
caps_writeDataSet(FILE *fp, capsOwn writer, capsDataSet *ds)
{
  int    stat, i;
  size_t n;

  n = fwrite(&ds->method, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&ds->nHist,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&ds->npts,   sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  i = ds->rank;
  if (ds->dflag != 0) i = -i;
  n = fwrite(&i,          sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  stat = caps_writeString(fp, ds->units);
  if (stat != CAPS_SUCCESS) return stat;

  for (i = 0; i < ds->nHist; i++) {
    stat = caps_writeOwn(fp, writer, ds->history[i]);
    if (stat != CAPS_SUCCESS) return stat;
  }

  i = ds->npts*ds->rank;
  if (i > 0) {
    stat = caps_writeDoubles(fp, i, ds->data);
    if (stat != CAPS_SUCCESS) return stat;
  }

  return CAPS_SUCCESS;
}


static void
caps_freeDataSet(capsDataSet *ds)
{
  int i;

  if (ds->history != NULL) {
    for (i = 0; i < ds->nHist; i++) caps_freeOwner(&ds->history[i]);
    EG_free(ds->history);
  }
  if (ds->data    != NULL) EG_free(ds->data);
  if (ds->units   != NULL) EG_free(ds->units);
  if (ds->startup != NULL) EG_free(ds->startup);
  ds->history = NULL;
  ds->data    = NULL;
  ds->units   = NULL;
  ds->startup = NULL;
  ds->nHist   = 0;
  ds->npts    = 0;
  ds->rank    = 0;
  ds->dflag   = 0;
}


static int
caps_readDataSet(FILE *fp, capsDataSet *ds)
{
  int    stat, i;
  size_t n;

  ds->history = NULL;
  ds->data    = NULL;
  ds->units   = NULL;
  ds->startup = NULL;
  ds->nHist   = 0;
  ds->npts    = 0;
  ds->rank    = 0;
  ds->dflag   = 0;
  n = fread(&ds->method, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&ds->nHist,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&ds->npts,   sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&ds->rank,   sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (ds->rank < 0) {
    ds->rank  = -ds->rank;
    ds->dflag = 1;
  }
  stat = caps_readString(fp, &ds->units);
  if (stat != CAPS_SUCCESS) return stat;

  ds->history = NULL;
  if (ds->nHist > 0) {
    ds->history = (capsOwn *) EG_alloc(ds->nHist*sizeof(capsOwn));
    if (ds->history == NULL) {
      caps_freeDataSet(ds);
      return EGADS_MALLOC;
    }
    for (i = 0; i < ds->nHist; i++) {
      ds->history[i].pname = NULL;
      ds->history[i].pID   = NULL;
      ds->history[i].user  = NULL;
    }
    for (i = 0; i < ds->nHist; i++) {
      stat = caps_readOwn(fp, &ds->history[i]);
      if (stat != CAPS_SUCCESS) {
        caps_freeDataSet(ds);
        return stat;
      }
    }
  }

  i = ds->npts*ds->rank;
  if (i > 0) {
    stat = caps_readDoubles(fp, &i, &ds->data);
    if (stat != CAPS_SUCCESS) {
      caps_freeDataSet(ds);
      return stat;
    }
  }

  return CAPS_SUCCESS;
}


static int
caps_writeVertexSet(FILE *fp, capsOwn writer, capsVertexSet *vs)
{
  int    dim, status, i = -1;
  size_t n;

  if (vs->analysis != NULL) i = vs->analysis->sn;
  n = fwrite(&i,            sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&vs->nDataSets, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  for (i = 0; i < vs->nDataSets; i++) {
    n = fwrite(&vs->dataSets[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    status = caps_writeDataSet(fp, writer,
                               (capsDataSet *) vs->dataSets[i]->blind);
    if (status != CAPS_SUCCESS) return status;
  }

  /* write needed discr info */
  dim = 0;
  if (vs->discr != NULL) dim = vs->discr->dim;
  n = fwrite(&dim, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (vs->discr    == NULL) return CAPS_SUCCESS;
  if (vs->analysis != NULL) return CAPS_SUCCESS;

  n = fwrite(&vs->discr->nVerts, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  for (i = 0; i < vs->discr->nVerts; i++) {
    n = fwrite(&vs->discr->verts[3*i], sizeof(double), 3, fp);
    if (n != 3) return CAPS_IOERR;
  }

  return CAPS_SUCCESS;
}


static void
caps_freeVertexSet(capsVertexSet *vs)
{
  int i;

  if (vs->dataSets != NULL) {
    for (i = 0; i < vs->nDataSets; i++) {
      if (vs->dataSets[i]        == NULL) continue;
      if (vs->dataSets[i]->blind == NULL) continue;
      caps_freeDataSet((capsDataSet *) vs->dataSets[i]->blind);
    }
    EG_free(vs->dataSets);
  }

  vs->analysis = NULL;
  vs->discr    = NULL;
  vs->dataSets = NULL;
}


static int
caps_readVertexSet(FILE *fp, capsObject **lookup, capsVertexSet *vs)
{
  int          i, dim, oIndex, status;
  size_t       n;
  capsAnalysis *analysis;

  vs->analysis = NULL;
  vs->discr    = NULL;
  vs->dataSets = NULL;
  n = fread(&oIndex, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (oIndex >= 0) vs->analysis = lookup[oIndex];

  n = fread(&vs->nDataSets, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (vs->nDataSets != 0) {
    vs->dataSets = (capsObject **) EG_alloc(vs->nDataSets*sizeof(capsObject *));
    if (vs->dataSets == NULL) return EGADS_MALLOC;
    for (i = 0; i < vs->nDataSets; i++) vs->dataSets[i] = NULL;
    for (i = 0; i < vs->nDataSets; i++) {
      n = fread(&oIndex, sizeof(int), 1, fp);
      if (n != 1) {
        caps_freeVertexSet(vs);
        return CAPS_IOERR;
      }
      vs->dataSets[i]        = lookup[oIndex];
      vs->dataSets[i]->blind = (capsDataSet *) EG_alloc(sizeof(capsDataSet));
      if (vs->dataSets[i]->blind == NULL) {
        caps_freeVertexSet(vs);
        return EGADS_MALLOC;
      }
      status = caps_readDataSet(fp, (capsDataSet *) vs->dataSets[i]->blind);
      if (status != CAPS_SUCCESS) {
        printf(" CAPS Error: DataSet %d readDataSet = %d\n", i, status);
        caps_freeVertexSet(vs);
        return status;
      }
    }
  }

  /* read and create discr */
  n = fread(&dim, sizeof(int), 1, fp);
  if (n   != 1) return CAPS_IOERR;
  if (dim == 0) return CAPS_SUCCESS;

  vs->discr = (capsDiscr *) EG_alloc(sizeof(capsDiscr));
  if (vs->discr == NULL) {
    caps_freeVertexSet(vs);
    return EGADS_MALLOC;
  }
  vs->discr->dim      = dim;
  vs->discr->instance = -1;
  vs->discr->aInfo    = NULL;
  vs->discr->nPoints  = 0;
  vs->discr->mapping  = NULL;
  vs->discr->nVerts   = 0;
  vs->discr->verts    = NULL;
  vs->discr->celem    = NULL;
  vs->discr->nTypes   = 0;
  vs->discr->types    = NULL;
  vs->discr->nElems   = 0;
  vs->discr->elems    = NULL;
  vs->discr->nDtris   = 0;
  vs->discr->dtris    = NULL;
  vs->discr->ptrm     = NULL;

  if (vs->analysis == NULL) {
    n = fread(&vs->discr->nVerts, sizeof(int), 1, fp);
    if (n != 1) {
      EG_free(vs->discr);
      caps_freeVertexSet(vs);
      return CAPS_IOERR;
    }
    vs->discr->verts = (double *)
                        EG_alloc(3*vs->discr->nVerts*sizeof(double));
    if (vs->discr->verts == NULL) {
      EG_free(vs->discr);
      caps_freeVertexSet(vs);
      return EGADS_MALLOC;
    }
    for (i = 0; i < vs->discr->nVerts; i++) {
      n = fread(&vs->discr->verts[3*i], sizeof(double), 3, fp);
      if (n != 3) {
        EG_free(vs->discr->verts);
        EG_free(vs->discr);
        caps_freeVertexSet(vs);
        return CAPS_IOERR;
      }
    }
  } else {
    analysis = (capsAnalysis *) vs->analysis->blind;
    if (analysis != NULL) vs->discr->aInfo = &analysis->info;
  }

  return CAPS_SUCCESS;
}


static int
caps_writeBound(FILE *fp, capsOwn writer, capsBound *bound)
{
  int    i, status;
  size_t n;

  n = fwrite(&bound->dim,     sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&bound->state,   sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&bound->plimits, sizeof(double), 4, fp);
  if (n != 4) return CAPS_IOERR;
  n = fwrite(&bound->iBody,   sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&bound->iEnt,    sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;

  i = 0;
  if (bound->curve != NULL) i = bound->curve->nrank;
  n = fwrite(&i, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (bound->curve != NULL) {
    n = fwrite(&bound->curve->periodic, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    n = fwrite(&bound->curve->nts,      sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    n = 2*bound->curve->nts*bound->curve->nrank;
    status = caps_writeDoubles(fp, n, bound->curve->interp);
    if (status != CAPS_SUCCESS) return status;
    n = fwrite(&bound->curve->trange, sizeof(double), 2, fp);
    if (n != 2) return CAPS_IOERR;
    n = fwrite(&bound->curve->ntm, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    if (bound->curve->ntm != 0) {
      n = 2*bound->curve->ntm;
      status = caps_writeDoubles(fp, n, bound->curve->tmap);
      if (status != CAPS_SUCCESS) return status;
    }
  }

  i = 0;
  if (bound->surface != NULL) i = bound->surface->nrank;
  n = fwrite(&i, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (bound->surface != NULL) {
    n = fwrite(&bound->surface->periodic, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    n = fwrite(&bound->surface->nus,      sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    n = fwrite(&bound->surface->nvs,      sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    n = 4*bound->surface->nus*bound->surface->nvs*bound->surface->nrank;
    status = caps_writeDoubles(fp, n, bound->surface->interp);
    if (status != CAPS_SUCCESS) return status;
    n = fwrite(&bound->surface->urange, sizeof(double), 2, fp);
    if (n != 2) return CAPS_IOERR;
    n = fwrite(&bound->surface->vrange, sizeof(double), 2, fp);
    if (n != 2) return CAPS_IOERR;
    n = fwrite(&bound->surface->num, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    n = fwrite(&bound->surface->nvm, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    if (bound->surface->num*bound->surface->nvm != 0) {
      n = 8*bound->surface->num*bound->surface->nvm;
      status = caps_writeDoubles(fp, n, bound->surface->uvmap);
      if (status != CAPS_SUCCESS) return status;
    }
  }

  n = fwrite(&bound->nVertexSet, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  for (i = 0; i < bound->nVertexSet; i++) {
    n = fwrite(&bound->vertexSet[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    status = caps_writeVertexSet(fp, writer,
                                 (capsVertexSet *) bound->vertexSet[i]->blind);
    if (status != CAPS_SUCCESS) return status;
  }

  return CAPS_SUCCESS;
}


static void
caps_freeBound(capsBound *bound)
{
  int i;

  if (bound->curve   != NULL) caps_Aprx1DFree(bound->curve);
  if (bound->surface != NULL) caps_Aprx2DFree(bound->surface);
  if (bound->lunits  != NULL) EG_free(bound->lunits);

  if (bound->vertexSet != NULL) {
    for (i = 0; i < bound->nVertexSet; i++) {
      if (bound->vertexSet[i]        == NULL) continue;
      if (bound->vertexSet[i]->blind == NULL) continue;
      caps_freeVertexSet((capsVertexSet *) bound->vertexSet[i]->blind);
    }
    EG_free(bound->vertexSet);
  }

  bound->curve      = NULL;
  bound->surface    = NULL;
  bound->vertexSet  = NULL;
  bound->nVertexSet = 0;
}


static int
caps_readBound(FILE *fp, capsObject **lookup, capsBound *bound)
{
  int    i, nc, status, oIndex;
  size_t n;

  bound->lunits     = NULL;
  bound->curve      = NULL;
  bound->surface    = NULL;
  bound->vertexSet  = NULL;
  bound->nVertexSet = 0;

  n = fread(&bound->dim,     sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&bound->state,   sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&bound->plimits, sizeof(double), 4, fp);
  if (n != 4) return CAPS_IOERR;
  n = fread(&bound->iBody,   sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&bound->iEnt,    sizeof(int),    1, fp);
  if (n != 1) return CAPS_IOERR;

  bound->curve = NULL;
  n = fread(&i, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (i != 0) {
    bound->curve = (capsAprx1D *) EG_alloc(sizeof(capsAprx1D));
    if (bound->curve == NULL) return EGADS_MALLOC;
    bound->curve->nrank  = i;
    bound->curve->interp = NULL;
    bound->curve->tmap   = NULL;
    n = fread(&bound->curve->periodic, sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    n = fread(&bound->curve->nts,      sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    nc = 2*bound->curve->nts*bound->curve->nrank;
    status = caps_readDoubles(fp, &nc, &bound->curve->interp);
    if (status != CAPS_SUCCESS) {
      caps_freeBound(bound);
      return status;
    }
    n = fread(&bound->curve->trange, sizeof(double), 2, fp);
    if (n != 2) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    n = fread(&bound->curve->ntm, sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    bound->curve->tmap = NULL;
    if (bound->curve->ntm != 0) {
      nc = 2*bound->curve->ntm;
      status = caps_readDoubles(fp, &nc, &bound->curve->tmap);
      if (status != CAPS_SUCCESS) {
        caps_freeBound(bound);
        return status;
      }
    }
  }

  bound->surface = NULL;
  n = fread(&i, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  if (i != 0) {
    bound->surface = (capsAprx2D *) EG_alloc(sizeof(capsAprx2D));
    if (bound->surface == NULL) return EGADS_MALLOC;
    bound->surface->nrank  = i;
    bound->surface->interp = NULL;
    bound->surface->uvmap  = NULL;
    n = fread(&bound->surface->periodic, sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    n = fread(&bound->surface->nus,      sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    n = fread(&bound->surface->nvs,      sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    nc = 4*bound->surface->nus*bound->surface->nvs*bound->surface->nrank;
    status = caps_readDoubles(fp, &nc, &bound->surface->interp);
    if (status != CAPS_SUCCESS) {
      caps_freeBound(bound);
      return status;
    }
    n = fread(&bound->surface->urange, sizeof(double), 2, fp);
    if (n != 2) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    n = fread(&bound->surface->vrange, sizeof(double), 2, fp);
    if (n != 2) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    n = fread(&bound->surface->num, sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    n = fread(&bound->surface->nvm, sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    bound->surface->uvmap = NULL;
    if (bound->surface->num*bound->surface->nvm != 0) {
      nc = 8*bound->surface->num*bound->surface->nvm;
      status = caps_readDoubles(fp, &nc, &bound->surface->uvmap);
      if (status != CAPS_SUCCESS) {
        caps_freeBound(bound);
        return status;
      }
    }
  }

  n = fread(&bound->nVertexSet, sizeof(int), 1, fp);
  if (n != 1) {
    caps_freeBound(bound);
    return CAPS_IOERR;
  }
  bound->vertexSet = (capsObject **) EG_alloc(bound->nVertexSet*
                                              sizeof(capsObject *));
  if (bound->vertexSet == NULL) {
    caps_freeBound(bound);
    return EGADS_MALLOC;
  }
  for (i = 0; i < bound->nVertexSet; i++) bound->vertexSet[i] = NULL;

  for (i = 0; i < bound->nVertexSet; i++) {
    n = fread(&oIndex, sizeof(int), 1, fp);
    if (n != 1) {
      caps_freeBound(bound);
      return CAPS_IOERR;
    }
    bound->vertexSet[i]        = lookup[oIndex];
    bound->vertexSet[i]->blind = (capsVertexSet *)
                                 EG_alloc(sizeof(capsVertexSet));
    if (bound->vertexSet[i]->blind == NULL) {
      caps_freeBound(bound);
      return EGADS_MALLOC;
    }

    status = caps_readVertexSet(fp, lookup,
                                (capsVertexSet *) bound->vertexSet[i]->blind);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: VertexSet %d readVertexSet = %d\n", i, status);
      caps_freeBound(bound);
      return status;
    }
  }

  return CAPS_SUCCESS;
}


static int
caps_writeValue(FILE *fp, capsValue *value)
{
  int       i, stat, oIndex;
  size_t    n;
  capsValue *child;

  if (value == NULL) return CAPS_NULLVALUE;

  n = fwrite(&value->type,    sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->length,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->dim,     sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->nrow,    sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->ncol,    sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->lfixed,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->sfixed,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->nullVal, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->pIndex,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;

  if (value->type == Integer) {
    n = fwrite(value->limits.ilims, sizeof(int),    2, fp);
    if (n != 2) return CAPS_IOERR;
  } else if (value->type == Double) {
    n = fwrite(value->limits.dlims, sizeof(double), 2, fp);
    if (n != 2) return CAPS_IOERR;
  }

  stat = caps_writeString(fp, value->units);
  if (stat != CAPS_SUCCESS) return stat;
  oIndex = -1;
  if (value->link != NULL) oIndex = value->link->sn;
  n = fwrite(&oIndex, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&value->linkMethod, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;

  if ((value->length == 1) && (value->type != String) &&
                              (value->type != Tuple)) {
    if (value->type == Value) {
      n = fwrite(&value->vals.object->sn, sizeof(int), 1, fp);
      if (n != 1) return CAPS_IOERR;
      child = (capsValue *) value->vals.object->blind;
      stat  = caps_writeValue(fp, child);
      if (stat != CAPS_SUCCESS) return stat;
    } else if (value->type == Double) {
      n = fwrite(&value->vals.real, sizeof(double), 1, fp);
      if (n != 1) return CAPS_IOERR;
    } else {
      n = fwrite(&value->vals.integer, sizeof(int), 1, fp);
      if (n != 1) return CAPS_IOERR;
    }
  } else {
    if (value->type == Value) {
      for (i = 0; i < value->length; i++) {
        n = fwrite(&value->vals.objects[i]->sn, sizeof(int), 1, fp);
        if (n != 1) return CAPS_IOERR;
      }
      for (i = 0; i < value->length; i++) {
        child = (capsValue *) value->vals.objects[i]->blind;
        stat  = caps_writeValue(fp, child);
        if (stat != CAPS_SUCCESS) return stat;
      }
    } else if (value->type == Double) {
      n = fwrite(value->vals.reals, sizeof(double), value->length, fp);
      if (n != value->length) return CAPS_IOERR;
    } else if (value->type == String) {
      stat = caps_writeString(fp, value->vals.string);
      if (stat != CAPS_SUCCESS) return stat;
    } else if (value->type == Tuple) {
      stat = caps_writeTuple(fp, value->length, value->nullVal,
                             value->vals.tuple);
      if (stat != CAPS_SUCCESS) return stat;
    } else {
      n = fwrite(value->vals.integers, sizeof(int), value->length, fp);
      if (n != value->length) return CAPS_IOERR;
    }
  }

  return CAPS_SUCCESS;
}


void
caps_freeValue(capsValue *value)
{
  int       i;
  capsValue *val;

  if (value == NULL) return;

  if (value->units != NULL) EG_free(value->units);
  if ((value->type == Boolean) || (value->type == Integer)) {
    if (value->length > 1) EG_free(value->vals.integers);
  } else if (value->type == Double) {
    if (value->length > 1) EG_free(value->vals.reals);
  } else if (value->type == String) {
    if (value->length > 1) EG_free(value->vals.string);
  } else if (value->type == Tuple) {
    caps_freeTuple(value->length, value->vals.tuple);
  } else {
    if (value->length > 1) {
      if (value->vals.objects != NULL) {
        for (i = 0; i < value->length; i++) {
          if (value->vals.objects[i] == NULL) continue;
          val = (capsValue *) value->vals.objects[i]->blind;
          value->vals.objects[i]->blind = NULL;
          caps_freeValue(val);
          if (val != NULL) EG_free(val);
        }
        EG_free(value->vals.objects);
      }
    } else {
      if (value->vals.object != NULL) {
        val = (capsValue *) value->vals.object->blind;
        value->vals.object->blind = NULL;
        caps_freeValue(val);
        if (val != NULL) EG_free(val);
      }
    }
  }
}


static int
caps_readValue(FILE *fp, capsObject **lookup, capsValue *value)
{
  int       i, stat, oIndex;
  size_t    n;
  capsValue *child;

  if (value == NULL) return CAPS_NULLVALUE;

  value->length          = value->nrow = value->ncol = 0;
  value->type            = Integer;
  value->dim             = value->pIndex = Scalar;
  value->lfixed          = value->sfixed = Fixed;
  value->nullVal         = NotAllowed;
  value->units           = NULL;
  value->link            = NULL;
  value->limits.dlims[0] = value->limits.dlims[1] = 0.0;
  value->linkMethod      = Copy;
  value->vals.object     = NULL;

  n = fread(&value->type,    sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->length,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->dim,     sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->nrow,    sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->ncol,    sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->lfixed,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->sfixed,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->nullVal, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&value->pIndex,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;

  if (value->type == Integer) {
    n = fread(value->limits.ilims, sizeof(int),    2, fp);
    if (n != 2) return CAPS_IOERR;
  } else if (value->type == Double) {
    n = fread(value->limits.dlims, sizeof(double), 2, fp);
    if (n != 2) return CAPS_IOERR;
  }

  stat = caps_readString(fp, &value->units);
  if (stat != CAPS_SUCCESS) return stat;
  n = fread(&oIndex, sizeof(int), 1, fp);
  if (n != 1) {
    caps_freeValue(value);
    return CAPS_IOERR;
  }
  if (oIndex >= 0) value->link = lookup[oIndex];
  n = fread(&value->linkMethod, sizeof(int), 1, fp);
  if (n != 1) {
    caps_freeValue(value);
    return CAPS_IOERR;
  }

  if ((value->length == 1) && (value->type != String)
                           && (value->type != Tuple)) {
    if (value->type == Value) {
      n = fread(&oIndex, sizeof(int), 1, fp);
      if (n != 1) {
        caps_freeValue(value);
        return CAPS_IOERR;
      }
      if (oIndex >= 0) {
        value->vals.object = lookup[oIndex];
        child = (capsValue *) EG_alloc(sizeof(capsValue));
        if (child == NULL) {
          caps_freeValue(value);
          return EGADS_MALLOC;
        }
        value->vals.object->blind = child;
        stat = caps_readValue(fp, lookup, child);
        if (stat != CAPS_SUCCESS) {
          caps_freeValue(value);
          return stat;
        }
      }
    } else if (value->type == Double) {
      n = fread(&value->vals.real, sizeof(double), 1, fp);
      if (n != 1) {
        caps_freeValue(value);
        return CAPS_IOERR;
      }
    } else {
      n = fread(&value->vals.integer, sizeof(int), 1, fp);
      if (n != 1) {
        caps_freeValue(value);
        return CAPS_IOERR;
      }
    }
  } else {
    if (value->type == Value) {
      value->vals.objects = (capsObject **)
                            EG_alloc(value->length*sizeof(capsObject *));
      if (value->vals.objects == NULL) {
        caps_freeValue(value);
        return EGADS_MALLOC;
      }
      for (i = 0; i < value->length; i++) value->vals.objects[i] = NULL;
      for (i = 0; i < value->length; i++) {
        n = fread(&oIndex, sizeof(int), 1, fp);
        if (n != 1) {
          caps_freeValue(value);
          return CAPS_IOERR;
        }
        if (oIndex < 0) {
          caps_freeValue(value);
          return CAPS_NULLBLIND;
        }
        value->vals.objects[i] = lookup[oIndex];
      }
      for (i = 0; i < value->length; i++) {
        child = (capsValue *) EG_alloc(sizeof(capsValue));
        if (child == NULL) {
          caps_freeValue(value);
          return EGADS_MALLOC;
        }
        value->vals.objects[i]->blind = child;
        stat = caps_readValue(fp, lookup, child);
        if (stat != CAPS_SUCCESS) {
          caps_freeValue(value);
          return stat;
        }
      }
    } else if (value->type == Double) {
      value->vals.reals = (double *) EG_alloc(value->length*sizeof(double));
      if (value->vals.reals == NULL) {
        caps_freeValue(value);
        return EGADS_MALLOC;
      }
      n = fread(value->vals.reals, sizeof(double), value->length, fp);
      if (n != value->length) return CAPS_IOERR;
    } else if (value->type == String) {
      stat = caps_readString(fp, &value->vals.string);
      if (stat != CAPS_SUCCESS) return stat;
    } else if (value->type == Tuple) {
      value->vals.tuple = NULL;
      if (value->length != 0) {
        stat = caps_readTuple(fp, value->length, value->nullVal,
                              &value->vals.tuple);
        if (stat != CAPS_SUCCESS) return stat;
      }
    } else {
      value->vals.integers = (int *) EG_alloc(value->length*sizeof(int));
      if (value->vals.integers == NULL) {
        caps_freeValue(value);
        return EGADS_MALLOC;
      }
      n = fread(value->vals.integers, sizeof(int), value->length, fp);
      if (n != value->length) return CAPS_IOERR;
    }
  }

  return CAPS_SUCCESS;
}


static int
caps_writeAnalysis(FILE *fp, capsOwn writer, capsAnalysis *analysis)
{
  int       i, stat;
  size_t    n;
  capsValue *value;

  stat = caps_writeString(fp, analysis->loadName);
  if (stat != CAPS_SUCCESS) return stat;
  stat = caps_writeString(fp, analysis->path);
  if (stat != CAPS_SUCCESS) return stat;
  stat = caps_writeString(fp, analysis->unitSys);
  if (stat != CAPS_SUCCESS) return stat;
  stat = caps_writeString(fp, analysis->intents);
  if (stat != CAPS_SUCCESS) return stat;
  n = fwrite(&analysis->nField,       sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&analysis->nAnalysisIn,  sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&analysis->nAnalysisOut, sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&analysis->nParent,      sizeof(int), 1, fp);
  if (n != 1) return CAPS_IOERR;
  stat = caps_writeOwn(fp, writer, analysis->pre);
  if (stat != CAPS_SUCCESS) return stat;

  for (i = 0; i < analysis->nAnalysisIn; i++) {
    if (analysis->analysisIn[i]->sn < 0) {
      printf(" CAPS Error: %s Analysis In [%d] sn = %d (caps_save)!\n",
             analysis->loadName, i+1, analysis->analysisIn[i]->sn);
      return CAPS_BADINDEX;
    }
    n     = fwrite(&analysis->analysisIn[i]->sn,  sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    value = (capsValue *) analysis->analysisIn[i]->blind;
    stat  = caps_writeValue(fp, value);
    if (stat != CAPS_SUCCESS) return stat;
  }

  for (i = 0; i < analysis->nAnalysisOut; i++) {
    if (analysis->analysisOut[i]->sn < 0) {
      printf(" CAPS Error: %s Analysis Out [%d] sn = %d (caps_save)!\n",
             analysis->loadName, i+1, analysis->analysisOut[i]->sn);
      return CAPS_BADINDEX;
    }
    n     = fwrite(&analysis->analysisOut[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    value = (capsValue *) analysis->analysisOut[i]->blind;
    stat  = caps_writeValue(fp, value);
    if (stat != CAPS_SUCCESS) return stat;
  }

  for (i = 0; i < analysis->nParent; i++) {
    if (analysis->parents[i]->sn < 0) {
      printf(" CAPS Error: %s Analysis Parent [%d] sn = %d (caps_save)!\n",
             analysis->loadName, i+1, analysis->parents[i]->sn);
      return CAPS_BADINDEX;
    }
    n = fwrite(&analysis->parents[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
  }

  return CAPS_SUCCESS;
}


static int
caps_readAnalysis(FILE *fp, capsObject **lookup, capsProblem *problem,
                  capsAnalysis *analysis)
{
  int        i, j, stat, oIndex, eFlag, nIn, nOut, nField, *ranks;
  size_t     n;
  char       **fields;
  capsObject *object;
  capsValue  *valueI, *valueO, *geomIn = NULL;

  analysis->loadName         = NULL;
  analysis->path             = NULL;
  analysis->unitSys          = NULL;
  analysis->intents          = NULL;
  analysis->eFlag            = 0;
  analysis->info.magicnumber = CAPSMAGIC;
  analysis->info.problem     = problem;
  analysis->info.analysis    = analysis;
  analysis->info.pIndex      = 0;
  analysis->info.irow        = 0;
  analysis->info.icol        = 0;
  analysis->nField           = 0;
  analysis->fields           = NULL;
  analysis->ranks            = 0;
  analysis->nAnalysisIn      = 0;
  analysis->analysisIn       = NULL;
  analysis->nAnalysisOut     = 0;
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

  stat = caps_readString(fp, &analysis->loadName);
  if (stat != CAPS_SUCCESS) return stat;
  if (analysis->loadName == NULL) return CAPS_NULLNAME;
  stat = caps_readString(fp, &analysis->path);
  if (stat != CAPS_SUCCESS) {
    caps_freeAnalysis(1, analysis);
    return stat;
  }
  stat = caps_readString(fp, &analysis->unitSys);
  if (stat != CAPS_SUCCESS) {
    caps_freeAnalysis(1, analysis);
    return stat;
  }
  stat = caps_readString(fp, &analysis->intents);
  if (stat != CAPS_SUCCESS) {
    caps_freeAnalysis(1, analysis);
    return stat;
  }
  n = fread(&analysis->nField,       sizeof(int), 1, fp);
  if (n != 1) {
    caps_freeAnalysis(1, analysis);
    return CAPS_IOERR;
  }
  n = fread(&analysis->nAnalysisIn,  sizeof(int), 1, fp);
  if (n != 1) {
    caps_freeAnalysis(1, analysis);
    return CAPS_IOERR;
  }
  n = fread(&analysis->nAnalysisOut, sizeof(int), 1, fp);
  if (n != 1) {
    caps_freeAnalysis(1, analysis);
    return CAPS_IOERR;
  }
  n = fread(&analysis->nParent,      sizeof(int), 1, fp);
  if (n != 1) {
    caps_freeAnalysis(1, analysis);
    return CAPS_IOERR;
  }
  stat = caps_readOwn(fp, &analysis->pre);
  if (stat != CAPS_SUCCESS) {
    caps_freeAnalysis(1, analysis);
    return stat;
  }

  /* try to load the AIM */
  if (problem->nGeomIn > 0) {
    object = problem->geomIn[0];
    geomIn = (capsValue *) object->blind;
  }
  eFlag  = 0;
  nField = 0;
  fields = NULL;
  ranks  = NULL;
  stat   = aim_Initialize(&problem->aimFPTR, analysis->loadName, problem->nGeomIn,
                          geomIn, &eFlag, analysis->unitSys, &nIn, &nOut,
                          &nField, &fields, &ranks);
  if (stat < CAPS_SUCCESS) {
    if (fields != NULL) {
      for (i = 0; i < nField; i++) EG_free(fields[i]);
      EG_free(fields);
    }
    EG_free(ranks);
    caps_freeAnalysis(1, analysis);
    return stat;
  }
  if (nIn <= 0) {
    if (fields != NULL) {
      for (i = 0; i < nField; i++) EG_free(fields[i]);
      EG_free(fields);
    }
    EG_free(ranks);
    caps_freeAnalysis(1, analysis);
    return CAPS_BADINIT;
  }
  analysis->instance = stat;
  analysis->eFlag    = eFlag;
  analysis->fields   = fields;
  analysis->ranks    = ranks;
  if (nField != analysis->nField) {
    printf(" CAPS Error: %s # Fields = %d -- from file = %d (caps_open)!\n",
           analysis->loadName, nField, analysis->nField);
    analysis->nField = nField;
    caps_freeAnalysis(1, analysis);
    return CAPS_MISMATCH;
  }
  if (nIn != analysis->nAnalysisIn) {
    printf(" CAPS Error: %s # Inputs = %d -- from file = %d (caps_open)!\n",
           analysis->loadName, nIn, analysis->nAnalysisIn);
    caps_freeAnalysis(1, analysis);
    return CAPS_MISMATCH;
  }
  if (nOut != analysis->nAnalysisOut) {
    printf(" CAPS Error: %s # Outputs = %d -- from file = %d (caps_open)!\n",
           analysis->loadName, nOut, analysis->nAnalysisOut);
    caps_freeAnalysis(1, analysis);
    return CAPS_MISMATCH;
  }

  valueI = NULL;
  if (analysis->nAnalysisIn != 0) {
    analysis->analysisIn = (capsObject **)
                           EG_alloc(analysis->nAnalysisIn*sizeof(capsObject *));
    if (analysis->analysisIn == NULL) {
      caps_freeAnalysis(1, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < analysis->nAnalysisIn; i++) analysis->analysisIn[i] = NULL;
    valueI = (capsValue *) EG_alloc(analysis->nAnalysisIn*sizeof(capsValue));
    if (valueI == NULL) {
      caps_freeAnalysis(1, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < analysis->nAnalysisIn; i++) {
      n = fread(&oIndex, sizeof(int), 1, fp);
      if (n != 1) {
        for (j = 0; j < i; j++) caps_freeValue(analysis->analysisIn[j]->blind);
        EG_free(valueI);
        caps_freeAnalysis(1, analysis);
        return CAPS_IOERR;
      }
/*@-immediatetrans@*/
      if (i == 0) lookup[oIndex]->blind = valueI;
      analysis->analysisIn[i]           = lookup[oIndex];
      analysis->analysisIn[i]->blind    = &valueI[i];
/*@+immediatetrans@*/
      stat = caps_readValue(fp, lookup, analysis->analysisIn[i]->blind);
      if (stat != CAPS_SUCCESS) {
        for (j = 0; j < i; j++) caps_freeValue(analysis->analysisIn[j]->blind);
/*@-kepttrans@*/
        EG_free(valueI);
/*@+kepttrans@*/
        caps_freeAnalysis(1, analysis);
        return CAPS_IOERR;
      }
    }
  }

  if (analysis->nAnalysisOut != 0) {
    analysis->analysisOut = (capsObject **)
                            EG_alloc(analysis->nAnalysisOut*sizeof(capsObject *));
    if (analysis->analysisOut == NULL) {
      if (valueI != NULL) {
        if (analysis->analysisIn != NULL)
          for (j = 0; j < analysis->nAnalysisIn; j++)
            caps_freeValue(analysis->analysisIn[j]->blind);
/*@-kepttrans@*/
        EG_free(valueI);
/*@+kepttrans@*/
      }
      caps_freeAnalysis(1, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < analysis->nAnalysisOut; i++) analysis->analysisOut[i] = NULL;
    valueO = (capsValue *) EG_alloc(analysis->nAnalysisOut*sizeof(capsValue));
    if (valueO == NULL) {
      if (valueI != NULL) {
        if (analysis->analysisIn != NULL)
          for (j = 0; j < analysis->nAnalysisIn; j++)
            caps_freeValue(analysis->analysisIn[j]->blind);
/*@-kepttrans@*/
        EG_free(valueI);
/*@+kepttrans@*/
      }
      caps_freeAnalysis(1, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < analysis->nAnalysisOut; i++) {
      n = fread(&oIndex, sizeof(int), 1, fp);
      if (n != 1) {
        for (j = 0; j < i; j++)
          caps_freeValue(analysis->analysisOut[j]->blind);
        if (valueI != NULL) {
          if (analysis->analysisIn != NULL)
            for (j = 0; j < analysis->nAnalysisIn; j++)
              caps_freeValue(analysis->analysisIn[j]->blind);
/*@-kepttrans@*/
          EG_free(valueI);
/*@+kepttrans@*/
        }
        EG_free(valueO);
        caps_freeAnalysis(1, analysis);
        return CAPS_IOERR;
      }
/*@-immediatetrans@*/
      if (i == 0) lookup[oIndex]->blind = valueO;
      analysis->analysisOut[i]          = lookup[oIndex];
      analysis->analysisOut[i]->blind   = &valueO[i];
/*@+immediatetrans@*/
      stat = caps_readValue(fp, lookup, analysis->analysisOut[i]->blind);
      if (stat != CAPS_SUCCESS) {
        for (j = 0; j < i; j++)
          caps_freeValue(analysis->analysisOut[j]->blind);
        if (valueI != NULL) {
          if (analysis->analysisIn != NULL)
            for (j = 0; j < analysis->nAnalysisIn; j++)
              caps_freeValue(analysis->analysisIn[j]->blind);
/*@-kepttrans@*/
          EG_free(valueI);
/*@+kepttrans@*/
        }
/*@-kepttrans@*/
        EG_free(valueO);
/*@+kepttrans@*/
        caps_freeAnalysis(1, analysis);
        return CAPS_IOERR;
      }
    }
  }

  if (analysis->nParent != 0) {
    analysis->parents = (capsObject **)
                        EG_alloc(analysis->nParent*sizeof(capsObject *));
    if (analysis->parents == NULL) {
      if (valueI != NULL) {
        if (analysis->analysisIn != NULL)
          for (j = 0; j < analysis->nAnalysisIn; j++)
            caps_freeValue(analysis->analysisIn[j]->blind);
/*@-kepttrans@*/
        EG_free(valueI);
/*@+kepttrans@*/
      }
      caps_freeAnalysis(1, analysis);
      return EGADS_MALLOC;
    }
    for (i = 0; i < analysis->nParent; i++) analysis->parents[i] = NULL;

    for (i = 0; i < analysis->nParent; i++) {
      n = fread(&oIndex, sizeof(int), 1, fp);
      if (n != 1) {
        if (valueI != NULL) {
          if (analysis->analysisIn != NULL)
            for (j = 0; j < analysis->nAnalysisIn; j++)
              caps_freeValue(analysis->analysisIn[j]->blind);
/*@-kepttrans@*/
          EG_free(valueI);
/*@+kepttrans@*/
        }
        caps_freeAnalysis(1, analysis);
        return CAPS_IOERR;
      }
      analysis->parents[i] = lookup[oIndex];
    }
  }

  return CAPS_SUCCESS;
}


static int
caps_writeProblem(FILE *fp, capsProblem *problem)
{
  int          i, stat;
  size_t       n;
  capsOwn      writer;
  capsAnalysis *analysis;
  capsBound    *bound;
  capsValue    *value;

  writer = problem->writer;
  stat   = caps_writeString(fp, problem->filename);
  if (stat != CAPS_SUCCESS) return stat;
  n = fwrite(&problem->sNum,      sizeof(CAPSLONG), 1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&problem->fileLen,   sizeof(CAPSLONG), 1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(problem->file,       sizeof(char),     problem->fileLen, fp);
  if (n != problem->fileLen) return CAPS_IOERR;
  n = fwrite(&problem->outLevel,  sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&problem->nParam,    sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&problem->nBranch,   sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&problem->nGeomIn,   sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&problem->nGeomOut,  sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&problem->nAnalysis, sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fwrite(&problem->nBound,    sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;

  for (i = 0; i < problem->nParam; i++) {
    n = fwrite(&problem->params[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    value = (capsValue *) problem->params[i]->blind;
    stat  = caps_writeValue(fp, value);
    if (stat != CAPS_SUCCESS) return stat;
  }

  /* geometry related objects */
  for (i = 0; i < problem->nBranch; i++) {
    n = fwrite(&problem->branchs[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
  }
  for (i = 0; i < problem->nBranch; i++) {
    value = (capsValue *) problem->branchs[i]->blind;
    stat  = caps_writeValue(fp, value);
    if (stat != CAPS_SUCCESS) return stat;
  }
  for (i = 0; i < problem->nGeomIn; i++) {
    n = fwrite(&problem->geomIn[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
  }
  for (i = 0; i < problem->nGeomIn; i++) {
    value = (capsValue *) problem->geomIn[i]->blind;
    stat  = caps_writeValue(fp, value);
    if (stat != CAPS_SUCCESS) return stat;
  }
  for (i = 0; i < problem->nGeomOut; i++) {
    n = fwrite(&problem->geomOut[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
  }
  for (i = 0; i < problem->nGeomOut; i++) {
    value = (capsValue *) problem->geomOut[i]->blind;
    stat  = caps_writeValue(fp, value);
    if (stat != CAPS_SUCCESS) return stat;
  }

  /* analysis objects */
  for (i = 0; i < problem->nAnalysis; i++) {
    n = fwrite(&problem->analysis[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    analysis = (capsAnalysis *) problem->analysis[i]->blind;
    stat     = caps_writeAnalysis(fp, writer, analysis);
    if (stat != CAPS_SUCCESS) return stat;
  }

  /* bound objects */
  for (i = 0; i < problem->nBound; i++) {
    n = fwrite(&problem->bounds[i]->sn, sizeof(int), 1, fp);
    if (n != 1) return CAPS_IOERR;
    bound = (capsBound *) problem->bounds[i]->blind;
    stat  = caps_writeBound(fp, writer, bound);
    if (stat != CAPS_SUCCESS) return stat;
  }

  return CAPS_SUCCESS;
}


int
caps_outLevel(capsObject *pobject, int outLevel)
{
  int          old;
  capsProblem  *problem;

  if (pobject == NULL)                   return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type != PROBLEM)          return CAPS_BADTYPE;
  if (pobject->blind == NULL)            return CAPS_NULLBLIND;
  if ((outLevel < 0) || (outLevel > 2))  return CAPS_RANGEERR;
  problem = (capsProblem *) pobject->blind;

  if (pobject->subtype == PARAMETRIC) {
    ocsmSetOutLevel(outLevel);
    old = problem->outLevel;
  } else {
    old = EG_setOutLevel(problem->context, outLevel);
  }
  if (old >= 0) problem->outLevel = outLevel;

  return old;
}


int
caps_close(capsObject *pobject)
{
  int          i, j, closeEGADS = 0;
  ego          model;
  capsProblem  *problem;
  capsAnalysis *analysis;

  if (pobject == NULL)                   return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type != PROBLEM)          return CAPS_BADTYPE;
  if (pobject->blind == NULL)            return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;

  if (problem->filename != NULL) {
    EG_free(problem->filename);
    closeEGADS = 1;
  }
  if (problem->pfile    != NULL) EG_free(problem->pfile);
  if (problem->file     != NULL) EG_free(problem->file);
  if (problem->lunits   != NULL) {
    for (i = 0; i < problem->nBodies; i++)
      if (problem->lunits[i] != NULL) EG_free(problem->lunits[i]);
    EG_free(problem->lunits);
  }
  caps_freeOwner(&problem->writer);
  caps_freeOwner(&problem->geometry);

  /* deal with geometry */
  if (problem->modl != NULL) {
    if (pobject->subtype == PARAMETRIC) {
      if (problem->analysis != NULL)
        for (i = 0; i < problem->nAnalysis; i++) {
          analysis = (capsAnalysis *) problem->analysis[i]->blind;
          if (analysis == NULL) continue;
          if (analysis->bodies != NULL)
            for (j = 0; j < analysis->nBody; j++)
              if (analysis->bodies[j+analysis->nBody] != NULL) {
                EG_deleteObject(analysis->bodies[j+analysis->nBody]);
                analysis->bodies[j+analysis->nBody] = NULL;
              }
        }
      /* close up OpenCSM */
      ocsmFree(problem->modl);
      /* remove tmp files (if they exist) and cleanup udp storage */
      ocsmFree(NULL);
      if (problem->bodies != NULL) EG_free(problem->bodies);
    } else {
      if (closeEGADS == 1) {
        model = (ego) problem->modl;
        EG_deleteObject(model);
      }
    }
  }

  /* deal with the CAPS Problem level Objects */
  caps_freeValueObjects(1, problem->nParam,   problem->params);
  caps_freeValueObjects(0, problem->nBranch,  problem->branchs);
  caps_freeValueObjects(0, problem->nGeomIn,  problem->geomIn);
  caps_freeValueObjects(0, problem->nGeomOut, problem->geomOut);

  if (problem->bounds != NULL) {
    for (i = 0; i < problem->nBound; i++) caps_delete(problem->bounds[i]);
    EG_free(problem->bounds);
  }

  /* close up the AIM */
  aim_cleanupAll(&problem->aimFPTR);

  if (problem->analysis != NULL) {
    for (i = 0; i < problem->nAnalysis; i++) {
      analysis = (capsAnalysis *) problem->analysis[i]->blind;
      caps_freeAnalysis(0, analysis);
      caps_freeOwner(&problem->analysis[i]->last);
      caps_freeAttrs(&problem->analysis[i]->attrs);
      problem->analysis[i]->magicnumber = 0;
      EG_free(problem->analysis[i]->name);
      problem->analysis[i]->name = NULL;
      EG_free(problem->analysis[i]);
    }
    EG_free(problem->analysis);
  }

  /* close up units interfaces */
  ut_free_system((ut_system *) problem->utsystem);

  /* close up EGADS and free the problem */
  if ((problem->context != NULL) && (closeEGADS == 1))
    EG_close(problem->context);
  EG_free(problem);

  /* cleanup and invalidate the object */
  caps_freeAttrs(&pobject->attrs);
  caps_freeOwner(&pobject->last);
  pobject->magicnumber = 0;
  EG_free(pobject->name);
  pobject->name = NULL;
  EG_free(pobject);

  return CAPS_SUCCESS;
}


static int
caps_readProblem(FILE *fp, capsObject **lookup, capsObject *pobject)
{
  int          i, stat, oIndex, oclass, mtype, *senses;
  size_t       n;
  CAPSLONG     nc;
  double       data[4];
  ego          model, ref;
  capsValue    *value;
  capsProblem  *problem;
  capsAnalysis *analysis;
  capsBound    *bound;
  FILE         *ofp;

  problem = (capsProblem *) pobject->blind;

  stat = caps_readString(fp, &problem->filename);
  if (stat != CAPS_SUCCESS) return stat;
  n = fread(&problem->sNum,      sizeof(CAPSLONG), 1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&problem->fileLen,   sizeof(CAPSLONG), 1,                fp);
  if (n != 1) return CAPS_IOERR;
  problem->file = (char *) EG_alloc(problem->fileLen*sizeof(char));
  if (problem->file == NULL) {
    caps_close(pobject);
    return EGADS_MALLOC;
  }
  n = fread(problem->file,       sizeof(char),     problem->fileLen, fp);
  if (n != problem->fileLen) return CAPS_IOERR;
  n = fread(&problem->outLevel,  sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&problem->nParam,    sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&problem->nBranch,   sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&problem->nGeomIn,   sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&problem->nGeomOut,  sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&problem->nAnalysis, sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;
  n = fread(&problem->nBound,    sizeof(int),      1,                fp);
  if (n != 1) return CAPS_IOERR;

  if (problem->nParam > 0) {
    for (i = 0; i < problem->nParam; i++) {
      n = fread(&oIndex, sizeof(int), 1, fp);
      if (n != 1) {
        caps_close(pobject);
        return CAPS_IOERR;
      }
      problem->params[i] = lookup[oIndex];
      value = (capsValue *) EG_alloc(sizeof(capsValue));
      if (value == NULL) {
        caps_close(pobject);
        return EGADS_MALLOC;
      }
      stat = caps_readValue(fp, lookup, value);
      if (stat != CAPS_SUCCESS) {
        EG_free(value);
        caps_close(pobject);
        return stat;
      }
      problem->params[i]->blind = value;
    }
  }

  /* get geometry */
  if ((problem->nBranch != 0) || (problem->nGeomIn != 0) ||
      (problem->nGeomOut != 0)) {

    if (problem->nBranch != 0) {
      for (i = 0; i < problem->nBranch; i++) {
        n = fread(&oIndex, sizeof(int), 1, fp);
        if (n != 1) {
          caps_close(pobject);
          return CAPS_IOERR;
        }
        problem->branchs[i] = lookup[oIndex];
      }
      value = (capsValue *) EG_alloc(problem->nBranch*sizeof(capsValue));
      if (value == NULL) {
        caps_close(pobject);
        return EGADS_MALLOC;
      }
      problem->branchs[0]->blind = value;
      for (i = 0; i < problem->nBranch; i++) {
        stat = caps_readValue(fp, lookup, &value[i]);
        if (stat != CAPS_SUCCESS) {
          caps_close(pobject);
          return stat;
        }
/*@-immediatetrans@*/
        problem->branchs[i]->blind = &value[i];
/*@+immediatetrans@*/
      }
    }
    if (problem->nGeomIn != 0) {
      for (i = 0; i < problem->nGeomIn; i++) {
        n = fread(&oIndex, sizeof(int), 1, fp);
        if (n != 1) {
          caps_close(pobject);
          return CAPS_IOERR;
        }
        problem->geomIn[i] = lookup[oIndex];
      }
      value = (capsValue *) EG_alloc(problem->nGeomIn*sizeof(capsValue));
      if (value == NULL) {
        caps_close(pobject);
        return EGADS_MALLOC;
      }
      problem->geomIn[0]->blind = value;
      for (i = 0; i < problem->nGeomIn; i++) {
        stat = caps_readValue(fp, lookup, &value[i]);
        if (stat != CAPS_SUCCESS) {
          caps_close(pobject);
          return stat;
        }
/*@-immediatetrans@*/
        problem->geomIn[i]->blind = &value[i];
/*@+immediatetrans@*/
      }
    }
    if (problem->nGeomOut != 0) {
      for (i = 0; i < problem->nGeomOut; i++) {
        n = fread(&oIndex, sizeof(int), 1, fp);
        if (n != 1) {
          caps_close(pobject);
          return CAPS_IOERR;
        }
        problem->geomOut[i] = lookup[oIndex];
      }
      value = (capsValue *) EG_alloc(problem->nGeomOut*sizeof(capsValue));
      if (value == NULL) {
        caps_close(pobject);
        return EGADS_MALLOC;
      }
      problem->geomOut[0]->blind = value;
      for (i = 0; i < problem->nGeomOut; i++) {
        stat = caps_readValue(fp, lookup, &value[i]);
        if (stat != CAPS_SUCCESS) {
          caps_close(pobject);
          return stat;
        }
/*@-immediatetrans@*/
        problem->geomOut[i]->blind = &value[i];
/*@+immediatetrans@*/
      }
    }

  } else {
    /* static geometry */
    unlink("capsTmp.egads");
    ofp = fopen("capsTmp.egads", "wb");
    if (ofp == NULL) {
      caps_close(pobject);
      return CAPS_IOERR;
    }
    n = fwrite(problem->file, sizeof(char), problem->fileLen, ofp);
    fclose(ofp);
    if (n != problem->fileLen) {
      nc = n;
#ifdef WIN32
      printf(" CAPS Error: fwrite problem -> %lld requested, %lld written!\n",
             nc, problem->fileLen);
#else
      printf(" CAPS Error: fwrite problem -> %ld requested, %ld written!\n",
             nc, problem->fileLen);
#endif
      unlink("capsTmp.egads");
      caps_close(pobject);
      return CAPS_MISMATCH;
    }
    stat = EG_loadModel(problem->context, 1, "capsTmp.egads", &model);
    unlink("capsTmp.egads");
    if (stat != EGADS_SUCCESS) {
      caps_close(pobject);
      return stat;
    }
    problem->modl = model;
    stat = EG_getTopology(model, &ref, &oclass, &mtype, data,
                          &problem->nBodies, &problem->bodies, &senses);
    if (stat != EGADS_SUCCESS) {
      caps_close(pobject);
      return stat;
    }
  }

  /* analyses objects */
  for (i = 0; i < problem->nAnalysis; i++) {
    n = fread(&oIndex, sizeof(int), 1, fp);
    if (n != 1) {
      caps_close(pobject);
      return CAPS_IOERR;
    }
    problem->analysis[i] = lookup[oIndex];
    analysis = (capsAnalysis *) EG_alloc(sizeof(capsAnalysis));
    if (analysis == NULL) {
      caps_close(pobject);
      return EGADS_MALLOC;
    }
    stat = caps_readAnalysis(fp, lookup, problem, analysis);
    if (stat != CAPS_SUCCESS) {
      printf(" CAPS Error: Analysis %d readAnalysis = %d\n", i, stat);
      EG_free(analysis);
      caps_close(pobject);
      return stat;
    }
    problem->analysis[i]->blind = analysis;
  }

  /* bound objects */
  for (i = 0; i < problem->nBound; i++) {
    n = fread(&oIndex, sizeof(int), 1, fp);
    if (n != 1) {
      caps_close(pobject);
      return CAPS_IOERR;
    }
    problem->bounds[i] = lookup[oIndex];
    bound = (capsBound *) EG_alloc(sizeof(capsBound));
    if (bound == NULL) {
      caps_close(pobject);
      return EGADS_MALLOC;
    }
    stat = caps_readBound(fp, lookup, bound);
    if (stat != CAPS_SUCCESS) {
      printf(" CAPS Error: Bound %d readBound = %d\n", i, stat);
      EG_free(bound);
      caps_close(pobject);
      return stat;
    }
    problem->bounds[i]->blind = bound;
  }

  return CAPS_SUCCESS;
}


static void
caps_countValueObjs(capsObject *vobject, int *nobj,
                    /*@null@*/ capsObject **lookup)
{
  int       i;
  capsValue *value;

  if (lookup == NULL) {
    vobject->sn   = *nobj;
  } else {
    lookup[*nobj] = vobject;
  }
  *nobj       += 1;
  value        = (capsValue *) vobject->blind;
  if (value == NULL) return;

  if (value->type != Value) return;
  if (value->length == 1) {
    caps_countValueObjs(value->vals.object, nobj, lookup);
  } else {
    for (i = 0; i < value->length; i++)
      caps_countValueObjs(value->vals.objects[i], nobj, lookup);
  }
}


int
caps_save(capsObject *pobject, /*@null@*/ const char *filename)
{
  int           i, j, k, stat, nobj, sizes[2], gstatus, rev[2] = {1, 1};
  int           execute, nparent, nField, *ranks;
  size_t        n;
  char          *apath, *unitSys, *intents, **fnames;
  const char    *name;
  capsObject    **lookup, *object, *source, *last, **parents;
  capsProblem   *problem;
  capsAnalysis  *analysis;
  capsBound     *bound;
  capsValue     *value;
  capsVertexSet *vs;
  FILE          *fp;

  if (pobject == NULL)                   return CAPS_NULLOBJ;
  if (pobject->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (pobject->type != PROBLEM)          return CAPS_BADTYPE;
  if (pobject->blind == NULL)            return CAPS_NULLBLIND;
  problem = (capsProblem *) pobject->blind;
  if (problem->filename == NULL)         return CAPS_BADNAME;

  /* make sure we are not dirty */
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
  if (gstatus == 1) {
    printf(" CAPS Error: Geometry is Dirty!\n");
    return CAPS_DIRTY;
  }
  if (problem->outLevel > 0)
    for (i = 0; i < problem->nAnalysis; i++) {
      stat = caps_analysisInfo(problem->analysis[i], &apath, &unitSys, &intents,
                               &nparent, &parents,
                               &nField, &fnames, &ranks, &execute, &gstatus);
      if (stat != CAPS_SUCCESS) {
        printf(" CAPS Error: Analysis[%d] %s caps_analysisInfo = %d!\n",
               i, problem->analysis[i]->name, stat);
        return stat;
      }
      if (gstatus != 0)
        printf(" CAPS Info: Analysis[%d] %s is Dirty!\n",
               i, problem->analysis[i]->name);
    }

  /* open the file */
  if (filename == NULL) {
    name = problem->pfile;
  } else {
    fp = fopen(filename, "rb");
    if (fp != NULL) {
      fclose(fp);
      return CAPS_EXISTS;
    }
    name = filename;
  }

  /* collect all objects */
  nobj = 1;
  for (i = 0; i < problem->nParam; i++)
    caps_countValueObjs(problem->params[i],  &nobj, NULL);
  for (i = 0; i < problem->nBranch; i++)
    caps_countValueObjs(problem->branchs[i], &nobj, NULL);
  for (i = 0; i < problem->nGeomIn; i++)
    caps_countValueObjs(problem->geomIn[i],  &nobj, NULL);
  for (i = 0; i < problem->nGeomOut; i++)
    caps_countValueObjs(problem->geomOut[i], &nobj, NULL);
  for (i = 0; i < problem->nAnalysis; i++) {
    problem->analysis[i]->sn = nobj;
    nobj++;
  }
  for (i = 0; i < problem->nAnalysis; i++) {
    analysis = (capsAnalysis *) problem->analysis[i]->blind;
    if (analysis == NULL) continue;
    for (j = 0; j < analysis->nAnalysisIn; j++)
      caps_countValueObjs(analysis->analysisIn[j],  &nobj, NULL);
    for (j = 0; j < analysis->nAnalysisOut; j++)
      caps_countValueObjs(analysis->analysisOut[j], &nobj, NULL);
  }
  for (i = 0; i < problem->nBound; i++) {
    problem->bounds[i]->sn = nobj;
    nobj++;
  }
  for (i = 0; i < problem->nBound; i++) {
    bound = (capsBound *) problem->bounds[i]->blind;
    if (bound == NULL) continue;
    for (j = 0; j < bound->nVertexSet; j++) {
      bound->vertexSet[j]->sn = nobj;
      nobj++;
    }
    for (j = 0; j < bound->nVertexSet; j++) {
      vs = (capsVertexSet *) bound->vertexSet[j]->blind;
      if (vs == NULL) continue;
      for (k = 0; k < vs->nDataSets; k++) {
        vs->dataSets[k]->sn = nobj;
        nobj++;
      }
    }
  }
  lookup = (capsObject **) EG_alloc(nobj*sizeof(capsObject *));
  if (lookup == NULL) return EGADS_MALLOC;
  lookup[0] = pobject;
  nobj      = 1;
  for (i = 0; i < problem->nParam; i++)
    caps_countValueObjs(problem->params[i],  &nobj, lookup);
  for (i = 0; i < problem->nBranch; i++)
    caps_countValueObjs(problem->branchs[i], &nobj, lookup);
  for (i = 0; i < problem->nGeomIn; i++)
    caps_countValueObjs(problem->geomIn[i],  &nobj, lookup);
  for (i = 0; i < problem->nGeomOut; i++)
    caps_countValueObjs(problem->geomOut[i], &nobj, lookup);
  for (i = 0; i < problem->nAnalysis; i++) {
    lookup[nobj] = problem->analysis[i];
    nobj++;
  }
  for (i = 0; i < problem->nAnalysis; i++) {
    analysis = (capsAnalysis *) problem->analysis[i]->blind;
    if (analysis == NULL) continue;
    for (j = 0; j < analysis->nAnalysisIn; j++)
      caps_countValueObjs(analysis->analysisIn[j],  &nobj, lookup);
    for (j = 0; j < analysis->nAnalysisOut; j++)
      caps_countValueObjs(analysis->analysisOut[j], &nobj, lookup);
  }
  for (i = 0; i < problem->nBound; i++) {
    lookup[nobj] = problem->bounds[i];
    nobj++;
  }
  for (i = 0; i < problem->nBound; i++) {
    bound = (capsBound *) problem->bounds[i]->blind;
    if (bound == NULL) continue;
    for (j = 0; j < bound->nVertexSet; j++) {
      lookup[nobj] = bound->vertexSet[j];
      nobj++;
    }
    for (j = 0; j < bound->nVertexSet; j++) {
      vs = (capsVertexSet *) bound->vertexSet[j]->blind;
      if (vs == NULL) continue;
      for (k = 0; k < vs->nDataSets; k++) {
        lookup[nobj] = vs->dataSets[k];
        nobj++;
      }
    }
  }

  /* write the file */
  fp = fopen(name, "wb");
  if (fp == NULL) {
    EG_free(lookup);
    return CAPS_NOTFOUND;
  }

  /* output the header info */
  i = CAPSMAGIC;
  n = fwrite(&i,                  sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  n = fwrite(rev,                 sizeof(int), 2, fp);
  if (n != 2) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  n = fwrite(&nobj,               sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  n = fwrite(&problem->nParam,    sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  n = fwrite(&problem->nBranch,   sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  n = fwrite(&problem->nGeomIn,   sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  n = fwrite(&problem->nGeomOut,  sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  n = fwrite(&problem->nAnalysis, sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }
  for (i = 0; i < problem->nAnalysis; i++) {
    analysis = (capsAnalysis *) problem->analysis[i]->blind;
    sizes[0] = sizes[1] = 0;
    if (analysis != NULL) {
      sizes[0] = analysis->nAnalysisIn;
      sizes[1] = analysis->nAnalysisOut;
    }
    n = fwrite(sizes,             sizeof(int), 2, fp);
    if (n != 2) {
      fclose(fp);
      EG_free(lookup);
      return CAPS_IOERR;
    }
  }
  n = fwrite(&problem->nBound,    sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    EG_free(lookup);
    return CAPS_IOERR;
  }

  /* output the objects themselves */
  for (i = 0; i < nobj; i++) {
    sizes[0] = lookup[i]->type;
    sizes[1] = lookup[i]->subtype;
    n = fwrite(sizes,             sizeof(int), 2, fp);
    if (n != 2) {
      fclose(fp);
      EG_free(lookup);
      return CAPS_IOERR;
    }
    stat = caps_writeString(fp, lookup[i]->name);
    if (stat != CAPS_SUCCESS) {
      fclose(fp);
      EG_free(lookup);
      return CAPS_IOERR;
    }
    stat = caps_writeAttrs(fp, lookup[i]->attrs);
    if (stat != CAPS_SUCCESS) {
      fclose(fp);
      EG_free(lookup);
      return CAPS_IOERR;
    }
    stat = caps_writeOwn(fp, problem->writer, lookup[i]->last);
    if (stat != CAPS_SUCCESS) {
      fclose(fp);
      EG_free(lookup);
      return CAPS_IOERR;
    }
    j = -1;
    if (lookup[i]->parent != NULL) j = lookup[i]->parent->sn;
    n = fwrite(&j, sizeof(int), 1, fp);
    if (n != 1) {
      fclose(fp);
      EG_free(lookup);
      return CAPS_IOERR;
    }
  }
  EG_free(lookup);

  /* write the data within the objects */
  stat = caps_writeProblem(fp, problem);
  fclose(fp);

  return stat;
}


static int
caps_readFile(capsObject *pobject)
{
  int         stat, i, j, nobj, iobj, *asizes, sizes[2], rev[2];
  size_t      n;
  capsProblem *problem;
  capsObject  **lookup;
  FILE        *fp;

  problem = (capsProblem *) pobject->blind;

  fp = fopen(problem->pfile, "rb");
  if (fp == NULL) return CAPS_NOTFOUND;

  /* get header */
  n = fread(&i,                  sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    return CAPS_IOERR;
  }
  if (i != CAPSMAGIC) {
    printf(" CAPS Error: Not a CAPS Problem file!\n");
    fclose(fp);
    return CAPS_MISMATCH;
  }
  n = fread(rev,                 sizeof(int), 2, fp);
  if (n != 2) {
    fclose(fp);
    return CAPS_IOERR;
  }
  if ((rev[0] != 1) || (rev[1] != 1)) {
    printf(" CAPS Error: CAPS file revision = %d %d!\n", rev[0], rev[1]);
    fclose(fp);
    return CAPS_MISMATCH;
  }
  n = fread(&nobj,               sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    return CAPS_IOERR;
  }
  n = fread(&problem->nParam,    sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    return CAPS_IOERR;
  }
  n = fread(&problem->nBranch,   sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    return CAPS_IOERR;
  }
  n = fread(&problem->nGeomIn,   sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    return CAPS_IOERR;
  }
  n = fread(&problem->nGeomOut,  sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    return CAPS_IOERR;
  }
  n = fread(&problem->nAnalysis, sizeof(int), 1, fp);
  if (n != 1) {
    fclose(fp);
    return CAPS_IOERR;
  }
  asizes = NULL;
  if (problem->nAnalysis > 0) {
    asizes = (int *) EG_alloc(2*problem->nAnalysis*sizeof(int));
    if (asizes == NULL) {
      fclose(fp);
      return EGADS_MALLOC;
    }
    for (i = 0; i < problem->nAnalysis; i++) {
      n = fread(&asizes[2*i],    sizeof(int), 2, fp);
      if (n != 2) {
        EG_free(asizes);
        fclose(fp);
        return CAPS_IOERR;
      }
    }
  }
  n = fread(&problem->nBound,    sizeof(int), 1, fp);
  if (n != 1) {
    EG_free(asizes);
    fclose(fp);
    return CAPS_IOERR;
  }

  lookup = (capsObject **) EG_alloc(nobj*sizeof(capsObject *));
  if (lookup == NULL) {
    EG_free(asizes);
    fclose(fp);
    return EGADS_MALLOC;
  }
  for (i = 1; i < nobj; i++) lookup[i] = NULL;
  for (i = 1; i < nobj; i++) {
    stat = caps_makeObject(&lookup[i]);
    if (stat != CAPS_SUCCESS) {
      for (j = 1; j < i; j++) EG_free(lookup[j]);
      EG_free(asizes);
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return stat;
    }
  }

  lookup[0]       = pobject;
  iobj            = 1;
  problem->params = NULL;
  if (problem->nParam > 0) {
    problem->params = (capsObject **)
                      EG_alloc(problem->nParam*sizeof(capsObject *));
    if (problem->params == NULL) {
      for (j = 1; j < nobj; j++) EG_free(lookup[j]);
      EG_free(asizes);
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return EGADS_MALLOC;
    }
    for (i = 0; i < problem->nParam; i++, iobj++)
      problem->params[i] = lookup[iobj];
  }

  if (problem->nBranch > 0) {
    problem->branchs = (capsObject **)
                       EG_alloc(problem->nBranch*sizeof(capsObject *));
    if (problem->branchs == NULL) {
      for (j = 1; j < nobj; j++) EG_free(lookup[j]);
      EG_free(asizes);
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return EGADS_MALLOC;
    }
    for (i = 0; i < problem->nBranch; i++, iobj++)
      problem->branchs[i] = lookup[iobj];
  }

  if (problem->nGeomIn > 0) {
    problem->geomIn = (capsObject **)
                      EG_alloc(problem->nGeomIn*sizeof(capsObject *));
    if (problem->geomIn == NULL) {
      for (j = 1; j < nobj; j++) EG_free(lookup[j]);
      EG_free(asizes);
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return EGADS_MALLOC;
    }
    for (i = 0; i < problem->nGeomIn; i++, iobj++)
      problem->geomIn[i] = lookup[iobj];
  }

  if (problem->nGeomOut > 0) {
    problem->geomOut = (capsObject **)
                       EG_alloc(problem->nGeomOut*sizeof(capsObject *));
    if (problem->geomOut == NULL) {
      for (j = 1; j < nobj; j++) EG_free(lookup[j]);
      EG_free(asizes);
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return EGADS_MALLOC;
    }
    for (i = 0; i < problem->nGeomOut; i++, iobj++)
      problem->geomOut[i] = lookup[iobj];
  }

  if (problem->nAnalysis > 0) {
    problem->analysis = (capsObject **)
                        EG_alloc(problem->nAnalysis*sizeof(capsObject *));
    if (problem->analysis == NULL) {
      for (j = 1; j < nobj; j++) EG_free(lookup[j]);
      EG_free(asizes);
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return EGADS_MALLOC;
    }
    for (i = 0; i < problem->nAnalysis; i++, iobj++)
      problem->analysis[i] = lookup[iobj];
  }
  if (asizes != NULL) EG_free(asizes);

  if (problem->nBound > 0) {
    problem->bounds = (capsObject **)
                      EG_alloc(problem->nBound*sizeof(capsObject *));
    if (problem->bounds == NULL) {
      for (j = 1; j < nobj; j++) EG_free(lookup[j]);
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return EGADS_MALLOC;
    }
    for (i = 0; i < problem->nBound; i++, iobj++)
      problem->bounds[i] = lookup[iobj];
  }

  /* get object information */
  for (i = 0; i < nobj; i++) {
    n = fread(sizes, sizeof(int), 2, fp);
    if (n != 2) {
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return CAPS_IOERR;
    }
    lookup[i]->type    = sizes[0];
    lookup[i]->subtype = sizes[1];

    stat = caps_readString(fp, &lookup[i]->name);
    if (stat != CAPS_SUCCESS) {
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return CAPS_IOERR;
    }
    stat = caps_readAttrs(fp, &lookup[i]->attrs);
    if (stat != CAPS_SUCCESS) {
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return CAPS_IOERR;
    }
    stat = caps_readOwn(fp, &lookup[i]->last);
    if (stat != CAPS_SUCCESS) {
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return CAPS_IOERR;
    }
    lookup[i]->parent = NULL;
    n = fread(sizes, sizeof(int), 1, fp);
    if (n != 1) {
      EG_free(lookup);
      caps_close(pobject);
      fclose(fp);
      return CAPS_IOERR;
    }
    if (sizes[0] >= 0) lookup[i]->parent = lookup[sizes[0]];
  }

  /* read object data */
  stat = caps_readProblem(fp, lookup, pobject);
  fclose(fp);
  EG_free(lookup);

  return stat;
}


int
caps_open(const char *filename, const char *pname, capsObject **pobject)
{
  int           i, j, k, n, len, status, oclass, mtype, idot, *senses;
  int           type, class, actv, ichld, ileft, irite, nattr, ngIn, ngOut;
  int           narg, nrow, ncol, nbrch, npmtr, buildTo, builtTo, ibody, nbody;
  char          byte, *units, *env;
  char          name[MAX_NAME_LEN], bname[MAX_STRVAL_LEN], line[129];
  CAPSLONG      fileLen, ret;
  double        dot, lower, upper, data[4], *reals;
  ego           model, ref;
  capsObject    *object, *objs;
  capsProblem   *problem;
  capsValue     *value;
  capsAnalysis  *analysis;
  capsBound     *bound;
  capsVertexSet *vertexset;
  modl_T        *MODL;
  FILE          *fp;
  const char    *aname, *astring;
  const int     *aints;
  const double  *areals;
#ifdef BUILDONLOAD
  int           jj, m;
  char          vstr[MAX_STRVAL_LEN];
  double        *values;
#endif

  *pobject = NULL;

  /* does file exist? */
  fp = fopen(filename, "rb");
  if (fp == NULL) return CAPS_NOTFOUND;
  fileLen = 0;
  do {
    ret = fread(&byte, sizeof(char), 1, fp);
    fileLen++;
  } while (ret != 0);
  fclose(fp);
  if (fileLen >  0) fileLen--;
  if (fileLen == 0) return CAPS_BADVALUE;

  /* find the file extension */
  len = strlen(filename);
  for (idot = len-1; idot > 0; idot--)
    if (filename[idot] == '.') break;
  if (idot == 0) return CAPS_BADNAME;
/*@-unrecog@*/
  if ((strcasecmp(&filename[idot],".caps")  != 0) &&
      (strcasecmp(&filename[idot],".csm")   != 0) &&
      (strcasecmp(&filename[idot],".egads") != 0)) return CAPS_BADTYPE;
/*@+unrecog@*/

  problem = (capsProblem *) EG_alloc(sizeof(capsProblem));
  if (problem == NULL) return EGADS_MALLOC;

  /* initialize all members */
  problem->signature      = NULL;
  problem->context        = NULL;
  problem->utsystem       = NULL;
  problem->pfile          = NULL;
  problem->filename       = NULL;
  problem->file           = NULL;
  problem->fileLen        = 0;
  problem->outLevel       = 1;
  problem->modl           = NULL;
  problem->nParam         = 0;
  problem->params         = NULL;
  problem->nBranch        = 0;
  problem->branchs        = NULL;
  problem->nGeomIn        = 0;
  problem->geomIn         = NULL;
  problem->nGeomOut       = 0;
  problem->geomOut        = NULL;
  problem->nAnalysis      = 0;
  problem->analysis       = NULL;
  problem->nBound         = 0;
  problem->bounds         = NULL;
  problem->geometry.pname = NULL;
  problem->geometry.pID   = NULL;
  problem->geometry.user  = NULL;
  problem->geometry.sNum  = 0;
  for (j = 0; j < 6; j++) problem->geometry.datetime[j] = 0;
  problem->nBodies        = 0;
  problem->bodies         = NULL;
  problem->lunits         = NULL;
  problem->sNum           = problem->writer.sNum = 1;
  problem->writer.pname   = EG_strdup(pname);
  caps_getStaticStrings(&problem->signature, &problem->writer.pID,
                        &problem->writer.user);
  for (j = 0; j < 6; j++) problem->writer.datetime[j] = 0;
  problem->aimFPTR.aim_nAnal = 0;
  for (j = 0; j < MAXANAL; j++) {
    problem->aimFPTR.aimName[j] = NULL;
    problem->aimFPTR.aimDLL[j] = NULL;
    problem->aimFPTR.aimInit[j] = NULL;
    problem->aimFPTR.aimDiscr[j] = NULL;
    problem->aimFPTR.aimFreeD[j] = NULL;
    problem->aimFPTR.aimLoc[j] = NULL;
    problem->aimFPTR.aimInput[j] = NULL;
    problem->aimFPTR.aimUsesDS[j] = NULL;
    problem->aimFPTR.aimPAnal[j] = NULL;
    problem->aimFPTR.aimPost[j] = NULL;
    problem->aimFPTR.aimOutput[j] = NULL;
    problem->aimFPTR.aimCalc[j] = NULL;
    problem->aimFPTR.aimXfer[j] = NULL;
    problem->aimFPTR.aimIntrp[j] = NULL;
    problem->aimFPTR.aimIntrpBar[j] = NULL;
    problem->aimFPTR.aimIntgr[j] = NULL;
    problem->aimFPTR.aimIntgrBar[j] = NULL;
    problem->aimFPTR.aimData[j] = NULL;
    problem->aimFPTR.aimBdoor[j] = NULL;
    problem->aimFPTR.aimClean[j] = NULL;
  }

  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    EG_free(problem);
    return status;
  }
  object->type  = PROBLEM;
  object->blind = problem;

/*@-nullpass@*/
  problem->utsystem = ut_read_xml(NULL);
/*@+nullpass@*/
  if (problem->utsystem == NULL) {
    caps_close(object);
    return CAPS_UNITERR;
  }

  /* open up EGADS */
  status = EG_open(&problem->context);
  if (status != EGADS_SUCCESS) {
    caps_close(object);
    return status;
  }
  if (problem->context == NULL) {
    caps_close(object);
    return EGADS_NOTCNTX;
  }

  /* load the file */
  if (strcasecmp(&filename[idot],".caps") == 0) {
    problem->pfile = EG_strdup(filename);
    status = caps_readFile(object);
    if (status < SUCCESS) return status;
    if (problem->file == NULL) return CAPS_NULLNAME;

    /* reload the Geometry */
    if (object->subtype == PARAMETRIC) {
      if (problem->outLevel != 1) ocsmSetOutLevel(problem->outLevel);
      unlink("capsTmp.cpc");
      fp = fopen("capsTmp.cpc", "wb");
      if (fp == NULL) {
        printf(" CAPS Error: Cannot Open Temp File (caps_open)!\n");
        caps_close(object);
        return CAPS_IOERR;
      }
      ret = fwrite(problem->file, sizeof(char), problem->fileLen, fp);
      fclose(fp);
      if (ret != problem->fileLen) {
        unlink("capsTmp.cpc");
#ifdef WIN32
        printf(" CAPS Error: File IO mismatch %lld %lld (caps_open)!\n", ret,
               problem->fileLen);
#else
        printf(" CAPS Error: File IO mismatch %ld %ld (caps_open)!\n", ret,
               problem->fileLen);
#endif
        caps_close(object);
        return CAPS_MISMATCH;
      }
      status = ocsmLoad((char *) "capsTmp.cpc", &problem->modl);
      unlink("capsTmp.cpc");
      if (status < SUCCESS) {
        printf(" CAPS Error: Cannot Load Temp File (caps_open)!\n");
        caps_close(object);
        return status;
      }
      MODL = (modl_T *) problem->modl;
      if (MODL == NULL) {
        printf(" CAPS Error: Cannot get OpenCSM MODL (caps_open)!\n");
        caps_close(object);
        return CAPS_NOTFOUND;
      }
      MODL->context   = problem->context;
      MODL->tessAtEnd = 0;
      /* check that Branches are properly ordered */
      status = ocsmCheck(problem->modl);
      if (status < SUCCESS) {
        printf(" CAPS Error: ocsmCheck = %d (caps_open)!\n", status);
        caps_close(object);
        return status;
      }
      /* reset the GeomIns & Branchs */
      if (problem->branchs != NULL)
        for (i = 0; i < problem->nBranch; i++) {
          if (problem->branchs[i] == NULL) continue;
          value  = (capsValue *) problem->branchs[i]->blind;
          status = ocsmSetBrch(problem->modl, i+1, value->vals.integer);
          if ((status != SUCCESS) && (status != OCSM_CANNOT_BE_SUPPRESSED)) {
            caps_close(object);
            printf(" caps_open Error: ocsmSetBrch[%d] fails with %d!",
                   i, status);
            return status;
          }
        }
      if (problem->geomIn != NULL)
        for (i = 0; i < problem->nGeomIn; i++) {
          if (problem->geomIn[i] == NULL) continue;
          value = (capsValue *) problem->geomIn[i]->blind;
          reals = value->vals.reals;
          if (value->length == 1) reals = &value->vals.real;
          status = ocsmGetPmtr(problem->modl, value->pIndex, &type,
                               &nrow, &ncol, name);
          if (status != SUCCESS) {
            caps_close(object);
            printf(" caps_open Error: ocsmGetPmtr %d fails with %d!",
                   value->pIndex, status);
            return status;
          }
          if ((ncol != value->ncol) || (nrow != value->nrow)) {
            caps_close(object);
            printf(" caps_open Error: %s ncol = %d %d, nrow = %d %d!",
                   name, ncol, value->ncol, nrow, value->nrow);
            return CAPS_MISMATCH;
          }
          /* flip storage
          for (n = j = 0; j < ncol; j++)
            for (k = 0; k < nrow; k++, n++) {  */
          for (n = k = 0; k < nrow; k++)
            for (j = 0; j < ncol; j++, n++) {
              status = ocsmSetValuD(problem->modl, value->pIndex,
                                    k+1, j+1, reals[n]);
              if (status != SUCCESS) {
                caps_close(object);
                printf(" caps_open Error: %d ocsmSetValuD[%d,%d] fails with %d!",
                       value->pIndex, k+1, j+1, status);
                return status;
              }
            }
        }

      buildTo = 0;                          /* all */
      nbody   = 0;
      status  = ocsmBuild(problem->modl, buildTo, &builtTo, &nbody, NULL);
      if (status != SUCCESS) {
        caps_close(object);
        printf(" caps_open Error: ocsmBuild to %d fails with %d!",
               builtTo, status);
        return status;
      }
      /* reset geometry serial number -- avoid rebuild */
      problem->geometry.sNum = problem->sNum;
      caps_fillDateTime(problem->geometry.datetime);
      nbody = 0;
      for (ibody = 1; ibody <= MODL->nbody; ibody++) {
        if (MODL->body[ibody].onstack != 1) continue;
        if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
        nbody++;
      }

      if (nbody > 0) {
        problem->bodies = (ego *)   EG_alloc(nbody*sizeof(ego));
        problem->lunits = (char **) EG_alloc(nbody*sizeof(char *));
        if ((problem->bodies != NULL) && (problem->lunits != NULL)) {
          problem->nBodies = nbody;
          i = 0;
          for (ibody = 1; ibody <= MODL->nbody; ibody++) {
            if (MODL->body[ibody].onstack != 1) continue;
            if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
            problem->bodies[i] = MODL->body[ibody].ebody;
            caps_fillLengthUnits(problem, problem->bodies[i],
                                 &problem->lunits[i]);
            i++;
          }
        } else {
          if (problem->lunits != NULL) EG_free(problem->lunits);
          problem->lunits = NULL;
          caps_close(object);
          printf(" caps_open: Error on %d Body memory allocation!", nbody);
          return EGADS_MALLOC;
        }
      }
      status = ocsmInfo(problem->modl, &nbrch, &npmtr, &nbody);
      if (status != SUCCESS) {
        caps_close(object);
        printf(" caps_open: ocsmInfo returns %d!", status);
        return status;
      }
      for (ngIn = ngOut = i = 0; i < npmtr; i++) {
        status = ocsmGetPmtr(problem->modl, i+1, &type, &nrow, &ncol, name);
        if (status != SUCCESS) {
          caps_close(object);
          return status;
        }
        if (type == OCSM_OUTPUT)   ngOut++;
        if (type == OCSM_EXTERNAL) ngIn++;
        if (type == OCSM_CONFIG)   ngIn++;
      }
      if (nbrch != problem->nBranch) {
        printf(" CAPS Error: # Branch = %d -- from %s = %d (caps_open)!\n",
               nbrch, filename, problem->nBranch);
        caps_close(object);
        return CAPS_MISMATCH;
      }
      if (ngIn != problem->nGeomIn) {
        printf(" CAPS Error: # Design Vars = %d -- from %s = %d (caps_open)!\n",
               ngIn, filename, problem->nGeomIn);
        caps_close(object);
        return CAPS_MISMATCH;
      }
      if (ngOut != problem->nGeomOut) {
        printf(" CAPS Error: # Geometry Outs = %d -- from %s = %d (caps_open)!\n",
               ngOut, filename, problem->nGeomOut);
        caps_close(object);
        return CAPS_MISMATCH;
      }
      /* check geomOuts */
      if (problem->geomOut != NULL)
        for (i = j = 0; j < npmtr; j++) {
          ocsmGetPmtr(problem->modl, j+1, &type, &nrow, &ncol, name);
          if (type != OCSM_OUTPUT) continue;
          if (strcmp(name, problem->geomOut[i]->name) != 0) {
            printf(" CAPS Error: %d Geometry Outs %s != %s (caps_open)!\n",
                   i+1, name, problem->geomOut[i]->name);
            caps_close(object);
            return CAPS_MISMATCH;
          }
          i++;
        }

      /* set the bodies for the AIMs */
      if ((nbody > 0) && (problem->bodies != NULL) &&
          (problem->analysis != NULL))
        for (i = 0; i < problem->nAnalysis; i++) {
          analysis = (capsAnalysis *) problem->analysis[i]->blind;
          if (analysis == NULL) continue;
          status   = caps_filter(problem, analysis);
          if (status != CAPS_SUCCESS)
            printf(" CAPS Warning: %s caps_filter = %d!\n",
                   problem->analysis[i]->name, status);
        }

      /* get the capsDiscr structures */
      if (problem->bounds != NULL)
        for (i = 0; i < problem->nBound; i++) {
          if (problem->bounds[i]              == NULL)      continue;
          if (problem->bounds[i]->magicnumber != CAPSMAGIC) continue;
          if (problem->bounds[i]->type        != BOUND)     continue;
          if (problem->bounds[i]->blind       == NULL)      continue;
          bound = (capsBound *) problem->bounds[i]->blind;
          for (j = 0; j < bound->nVertexSet; j++) {
            if (bound->vertexSet[j]              == NULL)      continue;
            if (bound->vertexSet[j]->magicnumber != CAPSMAGIC) continue;
            if (bound->vertexSet[j]->type        != VERTEXSET) continue;
            if (bound->vertexSet[j]->blind       == NULL)      continue;
            vertexset = (capsVertexSet *) bound->vertexSet[j]->blind;
            if (vertexset->analysis != NULL)
              if (vertexset->analysis->blind != NULL) {
                analysis = (capsAnalysis *) vertexset->analysis->blind;
                vertexset->discr->dim      = bound->dim;
                vertexset->discr->instance = analysis->instance;
                status  = aim_Discr(problem->aimFPTR, analysis->loadName,
                                    problem->bounds[i]->name, vertexset->discr);
                if (status != CAPS_SUCCESS) {
                  printf(" CAPS Error: Bound = %s, Analysis = %s aimDiscr = %d\n",
                         problem->bounds[i]->name, analysis->loadName, status);
                  caps_close(object);
                  return status;
                } else {
                  /* check the validity of the discretization just returned */
                  status = caps_checkDiscr(vertexset->discr, 129, line);
                  if (status != CAPS_SUCCESS) {
                    printf(" CAPS Error: Bound = %s, Analysis = %s chkDiscr=%d\n",
                           problem->bounds[i]->name, analysis->loadName, status);
                    printf("             %s\n", line);
                    aim_FreeDiscr(problem->aimFPTR, analysis->loadName,
                                  vertexset->discr);
                    caps_close(object);
                    return status;
                  }
                }
              }
          }
        }
    } else {
      /* Problem is static */
      if (problem->outLevel != 1)
        EG_setOutLevel(problem->context, problem->outLevel);
    }

  } else if (strcasecmp(&filename[idot],".csm") == 0) {
    object->subtype    = PARAMETRIC;
    object->name       = EG_strdup(pname);
    object->last.pname = EG_strdup(pname);
    object->last.sNum  = problem->sNum;
    caps_getStaticStrings(&problem->signature, &object->last.pID,
                          &object->last.user);

    /* Quiet initial ocsm by default */
    ocsmSetOutLevel(0);

    /* do an OpenCSM load */
    status = ocsmLoad((char *) filename, &problem->modl);
    if (status < SUCCESS) {
      printf(" CAPS Error: Cannot Load %s (caps_open)!\n", filename);
      caps_close(object);
      return status;
    }
    MODL = (modl_T *) problem->modl;
    if (MODL == NULL) {
      printf(" CAPS Error: Cannot get OpenCSM MODL (caps_open)!\n");
      caps_close(object);
      return CAPS_NOTFOUND;
    }
    MODL->context   = problem->context;
    MODL->tessAtEnd = 0;

    /* Restore ocsm outLevel */
    ocsmSetOutLevel(1);

    env = getenv("DUMPEGADS");
    if (env != NULL) {
      MODL->dumpEgads = 1;
      MODL->loadEgads = 1;
    }

    /* check that Branches are properly ordered */
    status = ocsmCheck(problem->modl);
    if (status < SUCCESS) {
      printf(" CAPS Error: ocsmCheck = %d (caps_open)!\n", status);
      caps_close(object);
      return status;
    }
#ifdef BUILDONLOAD
    buildTo = 0;                          /* all */
    nbody   = 0;
    status  = ocsmBuild(problem->modl, buildTo, &builtTo, &nbody, NULL);
    if (status != SUCCESS) {
      caps_close(object);
      printf(" caps_open Error: ocsmBuild to %d fails with %d!",
             builtTo, status);
      return status;
    }
    nbody = 0;
    for (ibody = 1; ibody <= MODL->nbody; ibody++) {
      if (MODL->body[ibody].onstack != 1) continue;
      if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
      nbody++;
    }

    printf(" CAPS Info: # bodies = %d\n", nbody);
    if (nbody > 0) {
      problem->bodies = (ego *)   EG_alloc(nbody*sizeof(ego));
      problem->lunits = (char **) EG_alloc(nbody*sizeof(char *));
      if ((problem->bodies != NULL) && (problem->lunits != NULL)) {
        problem->nBodies = nbody;
        i = 0;
        for (ibody = 1; ibody <= MODL->nbody; ibody++) {
          if (MODL->body[ibody].onstack != 1) continue;
          if (MODL->body[ibody].botype  == OCSM_NULL_BODY) continue;
          problem->bodies[i] = MODL->body[ibody].ebody;
          caps_fillLengthUnits(problem, problem->bodies[i],
                               &problem->lunits[i]);
          i++;
        }
      } else {
        if (problem->lunits != NULL) EG_free(problem->lunits);
        problem->lunits = NULL;
        caps_close(object);
        printf(" caps_open: Error on %d Body memory allocation!", nbody);
        return EGADS_MALLOC;
      }
    }
    problem->geometry.sNum = problem->sNum;
    caps_fillDateTime(problem->geometry.datetime);
#endif

    status = ocsmInfo(problem->modl, &nbrch, &npmtr, &nbody);
    if (status != SUCCESS) {
      caps_close(object);
      printf(" caps_open: ocsmInfo returns %d!", status);
      return status;
    }
    /* allocate the objects for the branches */
    if (nbrch != 0) {
      problem->branchs = (capsObject **) EG_alloc(nbrch*sizeof(capsObject *));
      if (problem->branchs == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = 0; i < nbrch; i++) problem->branchs[i] = NULL;
      value = (capsValue *) EG_alloc(nbrch*sizeof(capsValue));
      if (value == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = 0; i < nbrch; i++) {
        value[i].length          = value[i].nrow = value[i].ncol = 1;
        value[i].type            = Integer;
        value[i].dim             = value[i].pIndex = 0;
        value[i].lfixed          = value[i].sfixed = Fixed;
        value[i].nullVal         = NotAllowed;
        value[i].units           = NULL;
        value[i].link            = NULL;
        value[i].vals.integer    = 0;
        value[i].limits.ilims[0] = OCSM_ACTIVE;
        value[i].limits.ilims[1] = OCSM_DEFERRED;
        value[i].linkMethod      = Copy;

        status = caps_makeObject(&objs);
        if (status != CAPS_SUCCESS) {
          EG_free(value);
          caps_close(object);
          return EGADS_MALLOC;
        }
        if (i == 0) objs->blind = value;
        objs->parent    = object;
        objs->name      = NULL;
        objs->type      = VALUE;
        objs->subtype   = BRANCH;
        objs->last.sNum = 1;
/*@-immediatetrans@*/
        objs->blind     = &value[i];
/*@+immediatetrans@*/
        problem->branchs[i] = objs;
      }
      problem->nBranch = nbrch;
      for (i = 0; i < nbrch; i++) {
        status = ocsmGetName(problem->modl, i+1, bname);
        if (status != SUCCESS) {
          caps_close(object);
          return status;
        }
        problem->branchs[i]->name = EG_strdup(bname);
        status = ocsmGetBrch(problem->modl, i+1, &type, &class, &actv, &ichld,
                             &ileft, &irite, &narg, &nattr);
        if (status != CAPS_SUCCESS) {
          caps_close(object);
          return status;
        }
        value[i].vals.integer = actv;
      }
    }

    /* count the GeomIns and GeomOuts */
    for (ngIn = ngOut = i = 0; i < npmtr; i++) {
      status = ocsmGetPmtr(problem->modl, i+1, &type, &nrow, &ncol, name);
      if (status != SUCCESS) {
        caps_close(object);
        return status;
      }
      if (type == OCSM_OUTPUT)   ngOut++;
      if (type == OCSM_EXTERNAL) ngIn++;
      if (type == OCSM_CONFIG)   ngIn++;
    }

    /* allocate the objects for the geometry inputs */
    if (ngIn != 0) {
      problem->geomIn = (capsObject **) EG_alloc(ngIn*sizeof(capsObject *));
      if (problem->geomIn == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = 0; i < ngIn; i++) problem->geomIn[i] = NULL;
      value = (capsValue *) EG_alloc(ngIn*sizeof(capsValue));
      if (value == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = j = 0; j < npmtr; j++) {
        ocsmGetPmtr(problem->modl, j+1, &type, &nrow, &ncol, name);
        if ((type != OCSM_EXTERNAL) && (type != OCSM_CONFIG)) continue;
        if ((nrow == 0) || (ncol == 0)) continue;    /* ignore string pmtrs */
        value[i].nrow            = nrow;
        value[i].ncol            = ncol;
        value[i].type            = Double;
        value[i].dim             = Scalar;
        value[i].pIndex          = j+1;
        value[i].lfixed          = value[i].sfixed = Fixed;
        value[i].nullVal         = NotAllowed;
        value[i].units           = NULL;
        value[i].link            = NULL;
        value[i].vals.real       = 0.0;
        value[i].limits.dlims[0] = value[i].limits.dlims[1] = 0.0;
        value[i].linkMethod      = Copy;
        value[i].length          = value[i].nrow*value[i].ncol;
        if ((ncol > 1) && (nrow > 1)) {
          value[i].dim           = Array2D;
        } else if ((ncol > 1) || (nrow > 1)) {
          value[i].dim           = Vector;
        }

        status = caps_makeObject(&objs);
        if (status != CAPS_SUCCESS) {
          EG_free(value);
          caps_close(object);
          return EGADS_MALLOC;
        }
        if (i == 0) objs->blind = value;
/*@-kepttrans@*/
        objs->parent       = object;
/*@+kepttrans@*/
        objs->name         = NULL;
        objs->type         = VALUE;
        objs->subtype      = GEOMETRYIN;
        objs->last.sNum    = 1;
/*@-immediatetrans@*/
        objs->blind        = &value[i];
/*@+immediatetrans@*/
        problem->geomIn[i] = objs;
        i++;
      }
      problem->nGeomIn = ngIn;
      for (i = 0; i < ngIn; i++) {
        ocsmGetPmtr(problem->modl, value[i].pIndex, &type, &nrow, &ncol, name);
        if (nrow*ncol > 1) {
          reals = (double *) EG_alloc(nrow*ncol*sizeof(double));
          if (reals == NULL) {
            caps_close(object);
            return EGADS_MALLOC;
          }
          value[i].vals.reals = reals;
        } else {
          reals = &value[i].vals.real;
        }
        problem->geomIn[i]->name = EG_strdup(name);
/*      flip storage
        for (n = j = 0; j < ncol; j++)
          for (k = 0; k < nrow; k++, n++) {  */
        for (n = k = 0; k < nrow; k++)
          for (j = 0; j < ncol; j++, n++) {
            status = ocsmGetValu(problem->modl, value[i].pIndex, k+1, j+1,
                                 &reals[n], &dot);
            if (status != SUCCESS) {
              caps_close(object);
              return status;
            }
          }
        if (type == OCSM_CONFIG) continue;
        status = ocsmGetBnds(problem->modl, value[i].pIndex, 1, 1,
                             &lower, &upper);
        if (status != SUCCESS) continue;
        if ((lower != -HUGEQ) || (upper != HUGEQ)) {
          value[i].limits.dlims[0] = lower;
          value[i].limits.dlims[1] = upper;
        }
      }
    }

    /* allocate the objects for the geometry outputs */
    if (ngOut != 0) {
      units = NULL;
      if (problem->lunits != NULL) units = problem->lunits[problem->nBodies-1];
      problem->geomOut = (capsObject **) EG_alloc(ngOut*sizeof(capsObject *));
      if (problem->geomOut == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = 0; i < ngOut; i++) problem->geomOut[i] = NULL;
      value = (capsValue *) EG_alloc(ngOut*sizeof(capsValue));
      if (value == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = j = 0; j < npmtr; j++) {
        ocsmGetPmtr(problem->modl, j+1, &type, &nrow, &ncol, name);
        if (type != OCSM_OUTPUT) continue;
        value[i].length          = 1;
        value[i].type            = Double;
        value[i].nrow            = 1;
        value[i].ncol            = 1;
        value[i].dim             = Scalar;
        value[i].pIndex          = j+1;
        value[i].lfixed          = value[i].sfixed = Change;
        value[i].nullVal         = IsNull;
        value[i].units           = NULL;
        value[i].link            = NULL;
        value[i].vals.real       = 0.0;
        value[i].limits.dlims[0] = value[i].limits.dlims[1] = 0.0;
        value[i].linkMethod      = Copy;
        caps_geomOutUnits(name, units, &value[i].units);
#ifdef BUILDONLOAD
        if ((nrow == 0) || (ncol == 0)) {
          status = ocsmGetValuS(problem->modl, j+1, vstr);
          if (status != SUCCESS) {
            printf(" caps_open: %d ocsmGetValuS returns %d!", j+1, status);
            continue;
          }
          value[i].nullVal   = NotNull;
          value[i].type      = String;
          value->vals.string = EG_strdup(vstr);
        } else {
          value[i].nullVal = NotNull;
          value[i].nrow    = nrow;
          value[i].ncol    = ncol;
          value[i].length  = nrow*ncol;
          if ((nrow > 1) || (ncol > 1)) value[i].dim = Vector;
          if ((nrow > 1) && (ncol > 1)) value[i].dim = Array2D;
          if (value->length == 1) {
            values = &value[i].vals.real;
          } else {
            values = (double *) EG_alloc(value[i].length*sizeof(double));
            if (values == NULL) {
              printf(" caps_open: %d MALLOC on %d Doubles!",
                     j+1, value[i].length);
              continue;
            }
            value[i].vals.reals = values;
          }
          /* flip storage
          for (n = m = jj = 0; jj < ncol; jj++)
          for (k = 0; k < nrow; k++, n++) {  */
          for (n = m = k = 0; k < nrow; k++)
            for (jj = 0; jj < ncol; jj++, n++) {
              status = ocsmGetValu(problem->modl, j+1, k+1, jj+1,
                                   &values[n], &dot);
              if (status != SUCCESS) {
                printf(" caps_open: %d ocsmGetValu returns %d!",
                       j+1, status);
                continue;
              }
              if (values[n] == -HUGEQ) m++;
            }
          if (m != 0) value->nullVal = IsNull;
        }
#endif

        status = caps_makeObject(&objs);
        if (status != CAPS_SUCCESS) {
          for (k = 0; k < i; k++)
            if (value[k].length > 1) EG_free(value[k].vals.reals);
          EG_free(value);
          caps_close(object);
          return EGADS_MALLOC;
        }
/*@-kepttrans@*/
        objs->parent                   = object;
/*@+kepttrans@*/
        objs->name                     = EG_strdup(name);
        objs->type                     = VALUE;
        objs->subtype                  = GEOMETRYOUT;
        objs->last.sNum                = 0;
/*@-immediatetrans@*/
        objs->blind                    = &value[i];
/*@+immediatetrans@*/
        problem->geomOut[i]            = objs;
        problem->geomOut[i]->last.sNum = problem->sNum;
        i++;
      }
      problem->nGeomOut = ngOut;
    }

    /* write an OpenCSM checkpoint file */
    unlink("capsTmp.cpc");
    status = ocsmSave(problem->modl, "capsTmp.cpc");
    if (status != CAPS_SUCCESS) {
      caps_close(object);
      return status;
    }
    fp = fopen("capsTmp.cpc", "rb");
    if (fp == NULL) {
      caps_close(object);
      return CAPS_NOTFOUND;
    }
    fileLen = 0;
    do {
      ret = fread(&byte, sizeof(char), 1, fp);
      fileLen++;
    } while (ret != 0);
    fclose(fp);
    if (fileLen >  0) fileLen--;
    if (fileLen == 0) {
      caps_close(object);
      printf(" CAPS Error: capsTmp.cpc has zero length!\n");
      return CAPS_BADVALUE;
    }
    problem->filename = EG_strdup(filename);
    if (problem->file != NULL) EG_free(problem->file);
    problem->fileLen  = fileLen;
    problem->file     = (char *) EG_alloc(fileLen*sizeof(char));
    if (problem->file == NULL) {
      caps_close(object);
      printf(" CAPS Error: capsTmp.cpc Malloc problem!\n");
      return EGADS_MALLOC;
    }
    fp = fopen("capsTmp.cpc", "rb");
    if (fp == NULL) {
      caps_close(object);
      printf(" CAPS Error: Cannot Open File capsTmp.cpc for read!\n");
      return CAPS_NOTFOUND;
    } else {
      ret = fread(problem->file, sizeof(char), fileLen, fp);
      fclose(fp);
      if (ret != fileLen) {
        caps_close(object);
#ifdef WIN32
        printf(" CAPS Error: capsTmp.cpc readLen = %lld %lld!\n", ret, fileLen);
#else
        printf(" CAPS Error: capsTmp.cpc readLen = %ld %ld!\n", ret, fileLen);
#endif
        return CAPS_MISMATCH;
      }
    }
    unlink("capsTmp.cpc");

  } else {
    object->subtype    = STATIC;
    object->name       = EG_strdup(pname);
    object->last.pname = EG_strdup(pname);
    object->last.sNum  = problem->sNum;
    caps_getStaticStrings(&problem->signature, &object->last.pID,
                          &object->last.user);
    status = EG_loadModel(problem->context, 1, filename, &model);
    if (status != EGADS_SUCCESS) {
      caps_close(object);
      return status;
    }
    problem->modl = model;
    status = EG_getTopology(model, &ref, &oclass, &mtype, data,
                            &problem->nBodies, &problem->bodies, &senses);
    if (status != EGADS_SUCCESS) {
      caps_close(object);
      return status;
    }
    /* get length units if exist */
    if (problem->nBodies > 0) {
      problem->lunits = (char **) EG_alloc(problem->nBodies*sizeof(char *));
      if ((problem->lunits != NULL) && (problem->bodies != NULL))
        for (i = 0; i < problem->nBodies; i++)
          caps_fillLengthUnits(problem, problem->bodies[i],
                               &problem->lunits[i]);
    }
    
    /* get parameter values if saved by OpenCSM */
    status = EG_attributeNum(model, &nattr);
    if ((status == EGADS_SUCCESS) && (nattr != 0)) {
      for (ngIn = ngOut = i = 0; i < nattr; i++) {
        status = EG_attributeGet(model, i+1, &aname, &type, &len, &aints,
                                 &areals, &astring);
        if (status != EGADS_SUCCESS) continue;
        if (type   != ATTRREAL)      continue;
        if (strncmp(aname, "_outpmtr_", 9) == 0) ngOut++;
        if (strncmp(aname, "_despmtr_", 9) == 0) ngIn++;
        if (strncmp(aname, "_cfgpmtr_", 9) == 0) ngIn++;
      }
      
      /* allocate the objects for the geometry inputs */
      if (ngIn != 0) {
        problem->geomIn = (capsObject **) EG_alloc(ngIn*sizeof(capsObject *));
        if (problem->geomIn == NULL) {
          caps_close(object);
          return EGADS_MALLOC;
        }
        for (i = 0; i < ngIn; i++) problem->geomIn[i] = NULL;
        value = (capsValue *) EG_alloc(ngIn*sizeof(capsValue));
        if (value == NULL) {
          caps_close(object);
          return EGADS_MALLOC;
        }
        for (i = j = 0; j < nattr; j++) {
          status = EG_attributeGet(model, j+1, &aname, &type, &len, &aints,
                                   &areals, &astring);
          if (status != EGADS_SUCCESS) continue;
          if (type   != ATTRREAL)      continue;
          if ((strncmp(aname, "_despmtr_", 9) != 0) &&
              (strncmp(aname, "_cfgpmtr_", 9) != 0)) continue;
          value[i].nrow             = len;
          value[i].ncol             = 1;
          value[i].type             = Double;
          value[i].dim              = Scalar;
          value[i].pIndex           = j+1;
          value[i].lfixed           = value[i].sfixed = Fixed;
          value[i].nullVal          = NotAllowed;
          value[i].units            = NULL;
          value[i].link             = NULL;
          value[i].vals.real        = 0.0;
          value[i].limits.dlims[0]  = value[i].limits.dlims[1] = 0.0;
          value[i].linkMethod       = Copy;
          value[i].length           = len;
          if (len > 1) value[i].dim = Vector;
          
          status = caps_makeObject(&objs);
          if (status != CAPS_SUCCESS) {
            EG_free(value);
            caps_close(object);
            return EGADS_MALLOC;
          }
          if (i == 0) objs->blind = value;
          /*@-kepttrans@*/
          objs->parent       = object;
          /*@+kepttrans@*/
          objs->name         = NULL;
          objs->type         = VALUE;
          objs->subtype      = GEOMETRYIN;
          objs->last.sNum    = 1;
          /*@-immediatetrans@*/
          objs->blind        = &value[i];
          /*@+immediatetrans@*/
          problem->geomIn[i] = objs;
          i++;
        }
        problem->nGeomIn = ngIn;
        for (i = 0; i < ngIn; i++) {
          EG_attributeGet(model, value[i].pIndex, &aname, &type, &len, &aints,
                          &areals, &astring);
          if (len > 1) {
            reals = (double *) EG_alloc(len*sizeof(double));
            if (reals == NULL) {
              caps_close(object);
              return EGADS_MALLOC;
            }
            value[i].vals.reals = reals;
          } else {
            reals = &value[i].vals.real;
          }
          problem->geomIn[i]->name = EG_strdup(&aname[9]);
          for (j = 0; j < len; j++) reals[j] = areals[j];
        }
      }
      
      /* allocate the objects for the geometry outputs */
      if (ngOut != 0) {
        problem->geomOut = (capsObject **) EG_alloc(ngOut*sizeof(capsObject *));
        if (problem->geomOut == NULL) {
          caps_close(object);
          return EGADS_MALLOC;
        }
        for (i = 0; i < ngOut; i++) problem->geomOut[i] = NULL;
        value = (capsValue *) EG_alloc(ngOut*sizeof(capsValue));
        if (value == NULL) {
          caps_close(object);
          return EGADS_MALLOC;
        }
        for (i = j = 0; j < nattr; j++) {
          status = EG_attributeGet(model, j+1, &aname, &type, &len, &aints,
                                   &areals, &astring);
          if (status != EGADS_SUCCESS) continue;
          if (type   != ATTRREAL)      continue;
          if (strncmp(aname, "_outpmtr_", 9) != 0) continue;
          value[i].nrow             = len;
          value[i].ncol             = 1;
          value[i].type             = Double;
          value[i].dim              = Scalar;
          value[i].pIndex           = j+1;
          value[i].lfixed           = value[i].sfixed = Fixed;
          value[i].nullVal          = NotAllowed;
          value[i].units            = NULL;
          value[i].link             = NULL;
          value[i].vals.real        = 0.0;
          value[i].limits.dlims[0]  = value[i].limits.dlims[1] = 0.0;
          value[i].linkMethod       = Copy;
          value[i].length           = len;
          if (len > 1) value[i].dim = Vector;
          
          status = caps_makeObject(&objs);
          if (status != CAPS_SUCCESS) {
            EG_free(value);
            caps_close(object);
            return EGADS_MALLOC;
          }
          if (i == 0) objs->blind = value;
          /*@-kepttrans@*/
          objs->parent        = object;
          /*@+kepttrans@*/
          objs->name          = NULL;
          objs->type          = VALUE;
          objs->subtype       = GEOMETRYOUT;
          objs->last.sNum     = 1;
          /*@-immediatetrans@*/
          objs->blind         = &value[i];
          /*@+immediatetrans@*/
          problem->geomOut[i] = objs;
          i++;
        }
        problem->nGeomOut = ngOut;
        for (i = 0; i < ngOut; i++) {
          EG_attributeGet(model, value[i].pIndex, &aname, &type, &len, &aints,
                          &areals, &astring);
          if (len > 1) {
            reals = (double *) EG_alloc(len*sizeof(double));
            if (reals == NULL) {
              caps_close(object);
              return EGADS_MALLOC;
            }
            value[i].vals.reals = reals;
          } else {
            reals = &value[i].vals.real;
          }
          problem->geomOut[i]->name = EG_strdup(&aname[9]);
          for (j = 0; j < len; j++) reals[j] = areals[j];
        }
      }

    }
    
    problem->filename = EG_strdup(filename);
    problem->fileLen  = fileLen;
    problem->file     = (char *) EG_alloc(fileLen*sizeof(char));
    if (problem->file == NULL) {
      caps_close(object);
      return EGADS_MALLOC;
    }
    /* save away the input file */
    fp = fopen(filename, "rb");
    if (fp == NULL) {
      caps_close(object);
      printf(" CAPS Error: Cannot Open File %s for read!\n", filename);
      return CAPS_NOTFOUND;
    } else {
      ret = fread(problem->file, sizeof(char), fileLen, fp);
      fclose(fp);
      if (ret != fileLen) {
        caps_close(object);
#ifdef WIN32
        printf(" CAPS Error: CAPS readLen = %lld %lld!\n", ret, fileLen);
#else
        printf(" CAPS Error: CAPS readLen = %ld %ld!\n", ret, fileLen);
#endif
        return CAPS_MISMATCH;
      }
    }
  }
  problem->writer.sNum = problem->sNum;
  caps_fillDateTime(problem->writer.datetime);

  *pobject = object;
  return CAPS_SUCCESS;
}


int
caps_start(ego model, const char *pname, capsObject **pobject)
{
  int          i, j, len, status, oclass, mtype, type, nattr, ngIn, ngOut;
  int          *senses;
  double       data[4], *reals;
  ego          ref;
  capsObject   *object, *objs;
  capsProblem  *problem;
  capsValue    *value;
  const char   *aname, *astring;
  const int    *aints;
  const double *areals;
  
  *pobject = NULL;
  if (model == NULL)               return EGADS_NULLOBJ;
  if (model->magicnumber != MAGIC) return EGADS_NOTOBJ;
  if (model->oclass != MODEL)      return EGADS_NOTMODEL;
  
  problem = (capsProblem *) EG_alloc(sizeof(capsProblem));
  if (problem == NULL) return EGADS_MALLOC;
  
  /* initialize all members */
  problem->signature      = NULL;
  problem->context        = NULL;
  problem->utsystem       = NULL;
  problem->pfile          = NULL;
  problem->filename       = NULL;
  problem->file           = NULL;
  problem->fileLen        = 0;
  problem->outLevel       = 1;
  problem->modl           = NULL;
  problem->nParam         = 0;
  problem->params         = NULL;
  problem->nBranch        = 0;
  problem->branchs        = NULL;
  problem->nGeomIn        = 0;
  problem->geomIn         = NULL;
  problem->nGeomOut       = 0;
  problem->geomOut        = NULL;
  problem->nAnalysis      = 0;
  problem->analysis       = NULL;
  problem->nBound         = 0;
  problem->bounds         = NULL;
  problem->geometry.pname = NULL;
  problem->geometry.pID   = NULL;
  problem->geometry.user  = NULL;
  problem->geometry.sNum  = 0;
  for (j = 0; j < 6; j++) problem->geometry.datetime[j] = 0;
  problem->nBodies        = 0;
  problem->bodies         = NULL;
  problem->lunits         = NULL;
  problem->sNum           = problem->writer.sNum = 1;
  problem->writer.pname   = EG_strdup(pname);
  caps_getStaticStrings(&problem->signature, &problem->writer.pID,
                        &problem->writer.user);
  for (j = 0; j < 6; j++) problem->writer.datetime[j] = 0;
  problem->aimFPTR.aim_nAnal = 0;
  for (j = 0; j < MAXANAL; j++) {
    problem->aimFPTR.aimName[j] = NULL;
    problem->aimFPTR.aimDLL[j] = NULL;
    problem->aimFPTR.aimInit[j] = NULL;
    problem->aimFPTR.aimDiscr[j] = NULL;
    problem->aimFPTR.aimFreeD[j] = NULL;
    problem->aimFPTR.aimLoc[j] = NULL;
    problem->aimFPTR.aimInput[j] = NULL;
    problem->aimFPTR.aimUsesDS[j] = NULL;
    problem->aimFPTR.aimPAnal[j] = NULL;
    problem->aimFPTR.aimPost[j] = NULL;
    problem->aimFPTR.aimOutput[j] = NULL;
    problem->aimFPTR.aimCalc[j] = NULL;
    problem->aimFPTR.aimXfer[j] = NULL;
    problem->aimFPTR.aimIntrp[j] = NULL;
    problem->aimFPTR.aimIntrpBar[j] = NULL;
    problem->aimFPTR.aimIntgr[j] = NULL;
    problem->aimFPTR.aimIntgrBar[j] = NULL;
    problem->aimFPTR.aimData[j] = NULL;
    problem->aimFPTR.aimBdoor[j] = NULL;
    problem->aimFPTR.aimClean[j] = NULL;
  }
  
  /* make the Problem Object */
  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    EG_free(problem);
    return status;
  }
  object->type  = PROBLEM;
  object->blind = problem;
 
/*@-nullpass@*/
  problem->utsystem = ut_read_xml(NULL);
/*@+nullpass@*/
  if (problem->utsystem == NULL) {
    caps_close(object);
    return CAPS_UNITERR;
  }
  
  /* Get the existing EGADS Context */
  status = EG_getContext(model, &problem->context);
  if (status != EGADS_SUCCESS) {
    caps_close(object);
    return status;
  }
  
  object->subtype    = STATIC;
  object->name       = EG_strdup(pname);
  object->last.pname = EG_strdup(pname);
  object->last.sNum  = problem->sNum;
  caps_getStaticStrings(&problem->signature, &object->last.pID,
                        &object->last.user);
  problem->modl = model;
  status = EG_getTopology(model, &ref, &oclass, &mtype, data,
                          &problem->nBodies, &problem->bodies, &senses);
  if (status != EGADS_SUCCESS) {
    caps_close(object);
    return status;
  }
  /* get length units if exist */
  if (problem->nBodies > 0) {
    problem->lunits = (char **) EG_alloc(problem->nBodies*sizeof(char *));
    if ((problem->lunits != NULL) && (problem->bodies != NULL))
      for (i = 0; i < problem->nBodies; i++)
        caps_fillLengthUnits(problem, problem->bodies[i], &problem->lunits[i]);
  }
  
  /* get parameter values if saved by OpenCSM */
  status = EG_attributeNum(model, &nattr);
  if ((status == EGADS_SUCCESS) && (nattr != 0)) {
    for (ngIn = ngOut = i = 0; i < nattr; i++) {
      status = EG_attributeGet(model, i+1, &aname, &type, &len, &aints,
                               &areals, &astring);
      if (status != EGADS_SUCCESS) continue;
      if (type   != ATTRREAL)      continue;
      if (strncmp(aname, "_outpmtr_", 9) == 0) ngOut++;
      if (strncmp(aname, "_despmtr_", 9) == 0) ngIn++;
      if (strncmp(aname, "_cfgpmtr_", 9) == 0) ngIn++;
    }
    
    /* allocate the objects for the geometry inputs */
    if (ngIn != 0) {
      problem->geomIn = (capsObject **) EG_alloc(ngIn*sizeof(capsObject *));
      if (problem->geomIn == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = 0; i < ngIn; i++) problem->geomIn[i] = NULL;
      value = (capsValue *) EG_alloc(ngIn*sizeof(capsValue));
      if (value == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = j = 0; j < nattr; j++) {
        status = EG_attributeGet(model, j+1, &aname, &type, &len, &aints,
                                 &areals, &astring);
        if (status != EGADS_SUCCESS) continue;
        if (type   != ATTRREAL)      continue;
        if ((strncmp(aname, "_despmtr_", 9) != 0) &&
            (strncmp(aname, "_cfgpmtr_", 9) != 0)) continue;
        value[i].nrow             = len;
        value[i].ncol             = 1;
        value[i].type             = Double;
        value[i].dim              = Scalar;
        value[i].pIndex           = j+1;
        value[i].lfixed           = value[i].sfixed = Fixed;
        value[i].nullVal          = NotAllowed;
        value[i].units            = NULL;
        value[i].link             = NULL;
        value[i].vals.real        = 0.0;
        value[i].limits.dlims[0]  = value[i].limits.dlims[1] = 0.0;
        value[i].linkMethod       = Copy;
        value[i].length           = len;
        if (len > 1) value[i].dim = Vector;
        
        status = caps_makeObject(&objs);
        if (status != CAPS_SUCCESS) {
          EG_free(value);
          caps_close(object);
          return EGADS_MALLOC;
        }
        if (i == 0) objs->blind = value;
        /*@-kepttrans@*/
        objs->parent       = object;
        /*@+kepttrans@*/
        objs->name         = NULL;
        objs->type         = VALUE;
        objs->subtype      = GEOMETRYIN;
        objs->last.sNum    = 1;
        /*@-immediatetrans@*/
        objs->blind        = &value[i];
        /*@+immediatetrans@*/
        problem->geomIn[i] = objs;
        i++;
      }
      problem->nGeomIn = ngIn;
      for (i = 0; i < ngIn; i++) {
        EG_attributeGet(model, value[i].pIndex, &aname, &type, &len, &aints,
                        &areals, &astring);
        if (len > 1) {
          reals = (double *) EG_alloc(len*sizeof(double));
          if (reals == NULL) {
            caps_close(object);
            return EGADS_MALLOC;
          }
          value[i].vals.reals = reals;
        } else {
          reals = &value[i].vals.real;
        }
        problem->geomIn[i]->name = EG_strdup(&aname[9]);
        for (j = 0; j < len; j++) reals[j] = areals[j];
      }
    }
    
    /* allocate the objects for the geometry outputs */
    if (ngOut != 0) {
      problem->geomOut = (capsObject **) EG_alloc(ngOut*sizeof(capsObject *));
      if (problem->geomOut == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = 0; i < ngOut; i++) problem->geomOut[i] = NULL;
      value = (capsValue *) EG_alloc(ngOut*sizeof(capsValue));
      if (value == NULL) {
        caps_close(object);
        return EGADS_MALLOC;
      }
      for (i = j = 0; j < nattr; j++) {
        status = EG_attributeGet(model, j+1, &aname, &type, &len, &aints,
                                 &areals, &astring);
        if (status != EGADS_SUCCESS) continue;
        if (type   != ATTRREAL)      continue;
        if (strncmp(aname, "_outpmtr_", 9) != 0) continue;
        value[i].nrow             = len;
        value[i].ncol             = 1;
        value[i].type             = Double;
        value[i].dim              = Scalar;
        value[i].pIndex           = j+1;
        value[i].lfixed           = value[i].sfixed = Fixed;
        value[i].nullVal          = NotAllowed;
        value[i].units            = NULL;
        value[i].link             = NULL;
        value[i].vals.real        = 0.0;
        value[i].limits.dlims[0]  = value[i].limits.dlims[1] = 0.0;
        value[i].linkMethod       = Copy;
        value[i].length           = len;
        if (len > 1) value[i].dim = Vector;
        
        status = caps_makeObject(&objs);
        if (status != CAPS_SUCCESS) {
          EG_free(value);
          caps_close(object);
          return EGADS_MALLOC;
        }
        if (i == 0) objs->blind = value;
        /*@-kepttrans@*/
        objs->parent        = object;
        /*@+kepttrans@*/
        objs->name          = NULL;
        objs->type          = VALUE;
        objs->subtype       = GEOMETRYOUT;
        objs->last.sNum     = 1;
        /*@-immediatetrans@*/
        objs->blind         = &value[i];
        /*@+immediatetrans@*/
        problem->geomOut[i] = objs;
        i++;
      }
      problem->nGeomOut = ngOut;
      for (i = 0; i < ngOut; i++) {
        EG_attributeGet(model, value[i].pIndex, &aname, &type, &len, &aints,
                        &areals, &astring);
        if (len > 1) {
          reals = (double *) EG_alloc(len*sizeof(double));
          if (reals == NULL) {
            caps_close(object);
            return EGADS_MALLOC;
          }
          value[i].vals.reals = reals;
        } else {
          reals = &value[i].vals.real;
        }
        problem->geomOut[i]->name = EG_strdup(&aname[9]);
        for (j = 0; j < len; j++) reals[j] = areals[j];
      }
    }
  }

  problem->writer.sNum = problem->sNum;
  caps_fillDateTime(problem->writer.datetime);
  
  *pobject = object;
  return CAPS_SUCCESS;
}
