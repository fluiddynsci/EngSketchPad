/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Attribute Functions
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "capsBase.h"

extern /*@null@*/ /*@only@*/ char *EG_strdup(/*@null@*/ const char *str);


int
caps_attrByName(capsObject *cobj, char *name, capsObject **attr)
{
  int            i, status, atype, len;
  const void     *data;
  enum capsvType type;
  capsValue      *value;
  capsObject     *object;

  *attr = NULL;
  if (cobj              == NULL)      return CAPS_NULLOBJ;
  if (name              == NULL)      return CAPS_NULLNAME;
  if (cobj->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (cobj->attrs       == NULL)      return CAPS_NOTFOUND;

  for (i = 0; i < cobj->attrs->nattrs; i++)
    if (strcmp(name, cobj->attrs->attrs[i].name) == 0) {
      len   = cobj->attrs->attrs[i].length;
      atype = cobj->attrs->attrs[i].type;
      if (atype == ATTRINT) {
        type = Integer;
        if (len == 1) {
          data = &cobj->attrs->attrs[i].vals.integer;
        } else {
          data =  cobj->attrs->attrs[i].vals.integers;
        }
      } else if (atype == ATTRREAL) {
        type = Double;
        if (len == 1) {
          data = &cobj->attrs->attrs[i].vals.real;
        } else {
          data =  cobj->attrs->attrs[i].vals.reals;
        }
      } else {
        type = String;
        data = cobj->attrs->attrs[i].vals.string;
      }
      status = caps_makeVal(type, len, data, &value);
      if (status != CAPS_SUCCESS) return status;
      if ((len == 1) || (type == String)) {
        value->nrow = 1;
        value->ncol = 1;
        value->dim  = Scalar;
      } else {
        value->nrow = len;
        value->ncol = 1;
        value->dim  = Vector;
      }

      status = caps_makeObject(&object);
      if (status != CAPS_SUCCESS) {
        if (value->length != 1) {
          if (value->type == Integer) {
            EG_free(value->vals.integers);
          } else if (value->type == Double) {
            EG_free(value->vals.reals);
          } else {
            EG_free(value->vals.string);
          }
        }
        EG_free(value);
        return status;
      }
      object->name    = EG_strdup(name);
      object->type    = VALUE;
      object->subtype = USER;
      object->blind   = value;
      
      *attr  = object;
      return CAPS_SUCCESS;
    }
  
  return CAPS_NOTFOUND;
}


int
caps_attrByIndex(capsObject *cobj, int in, capsObject **attr)
{
  int            status, atype, len;
  const void     *data;
  enum capsvType type;
  capsValue      *value;
  capsObject     *object;
  
  *attr = NULL;
  if (cobj              == NULL)      return CAPS_NULLOBJ;
  if (cobj->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (cobj->attrs       == NULL)      return CAPS_NOTFOUND;
  if ((in < 1) || (in > cobj->attrs->nattrs)) return CAPS_BADINDEX;

  len   = cobj->attrs->attrs[in-1].length;
  atype = cobj->attrs->attrs[in-1].type;
  if (atype == ATTRINT) {
    type = Integer;
    if (len == 1) {
      data = &cobj->attrs->attrs[in-1].vals.integer;
    } else {
      data =  cobj->attrs->attrs[in-1].vals.integers;
    }
  } else if (atype == ATTRREAL) {
    type = Double;
    if (len == 1) {
      data = &cobj->attrs->attrs[in-1].vals.real;
    } else {
      data =  cobj->attrs->attrs[in-1].vals.reals;
    }
  } else {
    type = String;
    data = cobj->attrs->attrs[in-1].vals.string;
  }
  status = caps_makeVal(type, len, data, &value);
  if (status != CAPS_SUCCESS) return status;
  if ((len == 1) || (type == String)) {
    value->nrow = 1;
    value->ncol = 1;
    value->dim  = Scalar;
  } else {
    value->nrow = len;
    value->ncol = 1;
    value->dim  = Vector;
  }
  
  status = caps_makeObject(&object);
  if (status != CAPS_SUCCESS) {
    if (value->length != 1) {
      if (value->type == Integer) {
        EG_free(value->vals.integers);
      } else if (value->type == Double) {
        EG_free(value->vals.reals);
      } else {
        EG_free(value->vals.string);
      }
    }
    EG_free(value);
    return status;
  }
  object->name    = EG_strdup(cobj->attrs->attrs[in-1].name);
  object->type    = VALUE;
  object->subtype = USER;
  object->blind   = value;
  
  *attr  = object;
  return CAPS_SUCCESS;
}


int
caps_setAttr(capsObject *cobj, /*@null@*/ const char *aname, capsObject *aval)
{
  int          atype, i, find = -1;
  const int    *ints  = NULL;
  const double *reals = NULL;
  const char   *str   = NULL, *name;
  egAttr       *attr;
  egAttrs      *attrs;
  capsValue    *value;
  
  if (cobj              == NULL)      return CAPS_NULLOBJ;
  if (cobj->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (aval              == NULL)      return CAPS_NULLVALUE;
  if (aval->type        != VALUE)     return CAPS_BADTYPE;
  if (aval->blind       == NULL)      return CAPS_NULLBLIND;
  name = aname;
  if (name == NULL) name = aval->name;
  if (name              == NULL)      return CAPS_NULLNAME;

  value = aval->blind;
  if ((value->type == Boolean) || (value->type == Value)) return CAPS_BADVALUE;
  
  if (value->type == Integer) {
    atype = ATTRINT;
    if (value->length == 1) {
      ints = &value->vals.integer;
    } else {
      ints =  value->vals.integers;
    }
  } else if (value->type == Double) {
    atype = ATTRREAL;
    if (value->length == 1) {
      reals = &value->vals.real;
    } else {
      reals =  value->vals.reals;
    }
  } else {
    atype = ATTRSTRING;
    str   = value->vals.string;
  }

  attrs = (egAttrs *) cobj->attrs;
  
  if (attrs != NULL)
    for (i = 0; i < attrs->nattrs; i++)
      if (strcmp(attrs->attrs[i].name, name) == 0) {
        find = i;
        break;
      }
  
  if ((find != -1) && (attrs != NULL)) {
    
    /* an existing attribute -- reset the values */
    
    if (attrs->attrs[find].type == ATTRINT) {
      if (attrs->attrs[find].length != 1)
        EG_free(attrs->attrs[find].vals.integers);
    } else if (attrs->attrs[find].type == ATTRREAL) {
      if (attrs->attrs[find].length != 1)
        EG_free(attrs->attrs[find].vals.reals);
    } else {
      EG_free(attrs->attrs[find].vals.string);
    }
    
  } else {
    
    if (attrs == NULL) {
      attrs = (egAttrs *) EG_alloc(sizeof(egAttrs));
      if (attrs == NULL) return EGADS_MALLOC;
      attrs->nattrs = 0;
      attrs->attrs  = NULL;
      attrs->nseqs  = 0;
      attrs->seqs   = NULL;
      cobj->attrs   = attrs;
    }
    if (attrs->attrs == NULL) {
      attr = (egAttr *) EG_alloc((attrs->nattrs+1)*sizeof(egAttr));
    } else {
      attr = (egAttr *) EG_reall(attrs->attrs,
                                 (attrs->nattrs+1)*sizeof(egAttr));
    }
    if (attr == NULL) return EGADS_MALLOC;
    attrs->attrs = attr;
    find = attrs->nattrs;
    attrs->attrs[find].vals.string = NULL;
    attrs->attrs[find].name        = EG_strdup(name);
    if (attrs->attrs[find].name == NULL) return EGADS_MALLOC;
    attrs->nattrs += 1;
  }
  
  attrs->attrs[find].type   = atype;
  attrs->attrs[find].length = value->length;
  if (atype == ATTRINT) {
    if (ints != NULL) {
      if (value->length == 1) {
        attrs->attrs[find].vals.integer = ints[0];
      } else {
        attrs->attrs[find].vals.integers = (int *)
                                           EG_alloc(value->length*sizeof(int));
        if (attrs->attrs[find].vals.integers == NULL) {
          attrs->attrs[find].length = 0;
        } else {
          for (i = 0; i < value->length; i++)
            attrs->attrs[find].vals.integers[i] = ints[i];
        }
      }
    }
  } else if (atype == ATTRREAL) {
    if (reals != NULL) {
      if (value->length == 1) {
        attrs->attrs[find].vals.real = reals[0];
      } else {
        attrs->attrs[find].vals.reals = (double *)
                                        EG_alloc(value->length*sizeof(double));
        if (attrs->attrs[find].vals.reals == NULL) {
          attrs->attrs[find].length = 0;
        } else {
          for (i = 0; i < value->length; i++)
            attrs->attrs[find].vals.reals[i] = reals[i];
        }
      }
    }
  } else {
    attrs->attrs[find].length = 0;
    attrs->attrs[find].vals.string = EG_strdup(str);
    if (attrs->attrs[find].vals.string != NULL)
      attrs->attrs[find].length = strlen(attrs->attrs[find].vals.string);
  }
  
  return CAPS_SUCCESS;
}


int
caps_deleleAttr(capsObject *cobj, /*@null@*/ char *name)
{
  int     i, find = -1;
  egAttrs *attrs;

  if (cobj              == NULL)      return CAPS_NULLOBJ;
  if (cobj->magicnumber != CAPSMAGIC) return CAPS_BADOBJECT;
  if (cobj->attrs       == NULL)      return CAPS_NOTFOUND;
  
  /* delete all? */
  if (name == NULL) {
    caps_freeAttrs(&cobj->attrs);
    return CAPS_SUCCESS;
  }
  
  /* find the attribute */
  attrs  = cobj->attrs;
  for (i = 0; attrs->nattrs; i++)
    if (strcmp(name, attrs->attrs[i].name) == 0) {
      find = i;
      break;
    }
  
  if (find == -1) return CAPS_NOTFOUND;
 
  /* remove it! */
  EG_free(attrs->attrs[find].name);
  if (attrs->attrs[find].type == ATTRINT) {
    if (attrs->attrs[find].length > 1)
      EG_free(attrs->attrs[find].vals.integers);
  } else if (attrs->attrs[find].type == ATTRREAL) {
    if (attrs->attrs[find].length > 1)
      EG_free(attrs->attrs[find].vals.reals);
  } else {
    EG_free(attrs->attrs[find].vals.string);
  }
  for (i = find+1; i < attrs->nattrs; i++)
    attrs->attrs[i-1] = attrs->attrs[i];
  attrs->nattrs -= 1;
  
  return CAPS_SUCCESS;
}
