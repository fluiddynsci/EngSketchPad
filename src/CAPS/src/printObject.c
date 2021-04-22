/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Object Output Utility
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include "caps.h"


extern void printObjects(capsObj object, int indent);


static int
printValues(capsObj object, int indent)
{
  int            i, j, status, len, nerr, dim, nrow, ncol, range, *ints;
  char           *str, *pname, *pID, *uID;
  const char     *units;
  double         *reals;
  const void     *data;
  short          datetime[6];
  CAPSLONG       sNum;
  enum capsoType otype;
  enum capssType stype;
  enum capsvType type;
  enum capsFixed lfixed, sfixed;
  enum capsNull  nval;
  capsTuple      *tuple;
  capsObj        *objs, link, parent;
  capsErrs       *errors;
  capsOwn        own;

  status = caps_info(object, &pname, &otype, &stype, &link, &parent, &own);
  if (status != CAPS_SUCCESS) return status;
  status = caps_ownerInfo(own, &pname, &pID, &uID, datetime, &sNum);
  if (status != CAPS_SUCCESS) return status;
  if ((sNum == 0) && (stype != USER)) {
    for (i = 0; i < indent; i++) printf(" ");
    printf(" value = UNITITIALIZED\n");
    return CAPS_SUCCESS;
  }
  status = caps_getValue(object, &type, &len, &data, &units, &nerr, &errors);
  if (status != CAPS_SUCCESS) {
    if (errors != NULL) caps_freeError(errors);
    return status;
  }
  status = caps_getValueShape(object, &dim, &lfixed, &sfixed, &nval,
                              &nrow, &ncol);
  if (status != CAPS_SUCCESS) {
    if (errors != NULL) caps_freeError(errors);
    return status;
  }
  
  range = 0;
  for (i = 0; i < indent; i++) printf(" ");
  if (len == 1) {
    printf(" value =");
  } else {
    printf(" value (%dx%d) =", nrow, ncol);
  }
  if (data == NULL) {
    printf(" NULL with len = %d", len);
  } else {
    if ((type == Boolean) || (type == Integer)) {
      ints = (int *) data;
      for (i = 0; i < len; i++) printf(" %d", ints[i]);
      if (type == Integer) {
        data   = NULL;
        status = caps_getLimits(object, &data);
        if ((data != NULL) && (status == CAPS_SUCCESS)) range = 1;
      }
    } else if (type == Double) {
      reals = (double *) data;
      for (i = 0; i < len; i++) printf(" %lf", reals[i]);
      data   = NULL;
      status = caps_getLimits(object, &data);
      if ((data != NULL) && (status == CAPS_SUCCESS)) range = 2;
    } else if (type == String) {
      str = (char *) data;
      printf(" %s", str);
    } else if (type == Tuple) {
      printf("\n");
      tuple = (capsTuple *) data;
      for (j = 0; j < len; j++) {
        for (i = 0; i < indent+2; i++) printf(" ");
        printf("%d: %s -> %s\n", j+1, tuple[j].name, tuple[j].value);
      }
    } else if (type == Value) {
      objs = (capsObj *) data;
      for (i = 0; i < len; i++) printObjects(objs[i], indent+2);
    } else {
      return CAPS_BADTYPE;
    }
  }
  if (type == Tuple) return CAPS_SUCCESS;
  
  if (type != Value) printf(" %s", units);
  if (range == 1) {
    ints = (int *) data;
    if (ints  != NULL) printf(" lims=[%d-%d]", ints[0], ints[1]);
  } else if (range == 2) {
    reals = (double *) data;
    if (reals != NULL) printf(" lims=[%lf-%lf]", reals[0], reals[1]);
  }
  printf("\n");
  
  return CAPS_SUCCESS;
}


void
printObjects(capsObj object, int indent)
{
  int            i, j, status, nAttr, nBody, nParam, nGeomIn, nGeomOut, nBranch;
  int            nAnalysis, nBound, nAnalIn, nAnalOut, nConnect, nUnConnect;
  int            nDataSet, npts, rank;
  char           *name, *units, *pname, *pID, *userID, *oname;
  double         *data;
  short          datetime[6];
  CAPSLONG       sn, lsn;
  capsObj        link, parent, obj, attr;
  capsOwn        own;
  enum capsoType type, otype;
  enum capssType subtype, osubtype;
  static char    *oType[9]  = { "BODIES", "ATTRIBUTES", "UNUSED", "PROBLEM",
                                "VALUE", "ANALYSIS", "BOUND", "VERTEXSET",
                                "DATASET" };
  static char    *sType[12] = { "NONE", "STATIC", "PARAMETRIC", "GEOMETRYIN",
                                "GEOMETRYOUT", "BRANCH", "PARAMETER", "USER",
                                "ANALYSISIN", "ANALYSISOUT", "CONNECTED",
                                "UNCONNECTED" };

  status = caps_info(object, &name, &type, &subtype, &link, &parent, &own);
  if (status != CAPS_SUCCESS) {
#if defined(WIN32) && defined(_OCC64)
    printf(" CAPS Error: Object %llx returns %d from caps_info!\n",
           (long long) object, status);
#else
    printf(" CAPS Error: Object %lx returns %d from caps_info!\n",
           (long) object, status);
#endif
    return;
  }
  status = caps_size(object, ATTRIBUTES, NONE, &nAttr);
  if (status != CAPS_SUCCESS) {
    printf(" CAPS Error: Object %s returns %d from caps_size(Attribute)!\n",
           name, status);
    return;
  }
  for (i = 0; i < indent; i++) printf(" ");
  printf(" %s has type %s, subtype %s with %d attributes\n",
         name, oType[type+2], sType[subtype], nAttr);
  /* output owner */
  status = caps_ownerInfo(own, &pname, &pID, &userID, datetime, &sn);
  if (status != CAPS_SUCCESS) {
    printf(" CAPS Error: Object %s returns %d from caps_ownerInfo!\n",
           name, status);
  } else {
    if (datetime[0] != 0) {
      for (i = 0; i < indent; i++) printf(" ");
#ifdef WIN32
      printf("   last: %s %s %s  %d/%02d/%02d  %02d:%02d:%02d  %lld\n", pname,
             pID, userID, datetime[0], datetime[1], datetime[2], datetime[3],
             datetime[4], datetime[5], sn);
#else
      printf("   last: %s %s %s  %d/%02d/%02d  %02d:%02d:%02d  %ld\n", pname,
             pID, userID, datetime[0], datetime[1], datetime[2], datetime[3],
             datetime[4], datetime[5], sn);
#endif
    }
  }
  
  /* output attributes */
  for (j = 1; j <= nAttr; j++) {
    status = caps_attrByIndex(object, j, &attr);
    if (status != CAPS_SUCCESS) {
      for (i = 0; i < indent+2; i++) printf(" ");
      printf(" CAPS Error: Object %s Attr %d ret=%d from caps_attrByIndex!\n",
             name, j, status);
      continue;
    }
    printObjects(attr, indent+2);
    status = caps_delete(attr);
    if (status != CAPS_SUCCESS)
      printf(" CAPS Error: Object %s returns %d from caps_delete(Attr)!\n",
             attr->name, status);
  }
  
  if (type == PROBLEM) {
    status = caps_size(object, BODIES, NONE, &nBody);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(Body)!\n",
             name, status);
      return;
    }
    status = caps_size(object, VALUE, PARAMETER, &nParam);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(Parameter)!\n",
             name, status);
      return;
    }
    status = caps_size(object, VALUE, GEOMETRYIN, &nGeomIn);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(GeomIn)!\n",
             name, status);
      return;
    }
    status = caps_size(object, VALUE, GEOMETRYOUT, &nGeomOut);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(GeomOut)!\n",
             name, status);
      return;
    }
    status = caps_size(object, VALUE, BRANCH, &nBranch);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(Branch)!\n",
             name, status);
      return;
    }
    status = caps_size(object, ANALYSIS, NONE, &nAnalysis);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(Analysis)!\n",
             name, status);
      return;
    }
    status = caps_size(object, BOUND, NONE, &nBound);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(Bound)!\n",
             name, status);
      return;
    }
    for (i = 0; i < indent; i++) printf(" ");
    printf("   %d Bodies, %d Parameters, %d GeomIns, %d GeomOuts, %d Branchs,",
           nBody, nParam, nGeomIn, nGeomOut, nBranch);
    printf(" %d Analyses, %d Bounds\n", nAnalysis, nBound);

    if (nParam > 0) {
      printf("\n");
      for (i = 0; i < nParam; i++) {
        status = caps_childByIndex(object, VALUE, PARAMETER, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(Param,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
  
    if (nBranch > 0) {
      printf("\n");
      for (i = 0; i < nBranch; i++) {
        status = caps_childByIndex(object, VALUE, BRANCH, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(Branch,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
  
    if (nGeomIn > 0) {
      printf("\n");
      for (i = 0; i < nGeomIn; i++) {
        status = caps_childByIndex(object, VALUE, GEOMETRYIN, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(GeomIn,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
    if (nGeomOut > 0) {
      printf("\n");
      for (i = 0; i < nGeomOut; i++) {
        status = caps_childByIndex(object, VALUE, GEOMETRYOUT, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(GeomOut,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
    
    if (nAnalysis > 0) {
      printf("\n");
      for (i = 0; i < nAnalysis; i++) {
        status = caps_childByIndex(object, ANALYSIS, NONE, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(Analysis,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
    
    if (nBound > 0) {
      printf("\n");
      for (i = 0; i < nBound; i++) {
        status = caps_childByIndex(object, BOUND, NONE, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(Bound,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }

  } else if (type == VALUE) {
    if (link != NULL) {
      for (i = 0; i < indent+2; i++) printf(" ");
      printf(" linked to %s\n", link->name);
    } else {
      status = printValues(object, indent+2);
      if (status != CAPS_SUCCESS) {
        for (i = 0; i < indent; i++) printf(" ");
        printf(" CAPS Error: printVal returns %d!\n", status);
      }
    }
    
  } else if (type == ANALYSIS) {
    status = caps_size(object, VALUE, ANALYSISIN, &nAnalIn);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(AnalysisIn)!\n",
             name, status);
      return;
    }
    status = caps_size(object, VALUE, ANALYSISOUT, &nAnalOut);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(AnalysisIn)!\n",
             name, status);
      return;
    }
    for (i = 0; i < indent; i++) printf(" ");
    printf("   %d AnalysisIns, %d AnalysisOuts\n", nAnalIn, nAnalOut);

    if (nAnalIn > 0) {
      printf("\n");
      for (i = 0; i < nAnalIn; i++) {
        status = caps_childByIndex(object, VALUE, ANALYSISIN, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(AnalIn,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
    
    if (nAnalOut > 0) {
      printf("\n");
      for (i = 0; i < nAnalOut; i++) {
        status = caps_childByIndex(object, VALUE, ANALYSISOUT, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(AnalOut,%d)!\n",
                 name, status, i+1);
          return;
        }
        sn     = 0;
        status = caps_info(obj, &oname, &otype, &osubtype, &link, &parent, &own);
        if (status == CAPS_SUCCESS)
          caps_ownerInfo(own, &pname, &pID, &userID, datetime, &sn);
        if (sn == 0) continue;
        
        printObjects(obj, indent+2);

        status = caps_info(obj, &oname, &otype, &osubtype, &link, &parent, &own);
        if (status == CAPS_SUCCESS) {
          status = caps_ownerInfo(own, &pname, &pID, &userID, datetime, &lsn);
          if (status == CAPS_SUCCESS)
            if (lsn != sn) {
              for (j = 0; j < indent+2; j++) printf(" ");
#ifdef WIN32
              printf("   lazy: %s %s %s  %d/%02d/%02d  %02d:%02d:%02d  %lld\n",
                     pname, pID, userID, datetime[0], datetime[1], datetime[2],
                     datetime[3], datetime[4], datetime[5], lsn);
#else
              printf("   lazy: %s %s %s  %d/%02d/%02d  %02d:%02d:%02d  %ld\n",
                     pname, pID, userID, datetime[0], datetime[1], datetime[2],
                     datetime[3], datetime[4], datetime[5], lsn);
#endif
            }
        }
      }
    }
    
  } else if (type == BOUND) {
    status = caps_size(object, VERTEXSET, CONNECTED, &nConnect);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(VSconnected)!\n",
             name, status);
      return;
    }
    status = caps_size(object, VERTEXSET, UNCONNECTED, &nUnConnect);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(VSconnected)!\n",
             name, status);
      return;
    }
    for (i = 0; i < indent; i++) printf(" ");
    printf("   %d Connecteds, %d UnConnecteds\n", nConnect, nUnConnect);

    if (nConnect > 0) {
      printf("\n");
      for (i = 0; i < nConnect; i++) {
        status = caps_childByIndex(object, VERTEXSET, CONNECTED, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(Connect,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
    
    if (nUnConnect > 0) {
      printf("\n");
      for (i = 0; i < nUnConnect; i++) {
        status = caps_childByIndex(object, VERTEXSET, UNCONNECTED, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(UnConnect,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
  
  } else if (type == VERTEXSET) {
    status = caps_size(object, DATASET, NONE, &nDataSet);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s returns %d from caps_size(DataSet)!\n",
             name, status);
      return;
    }
    for (i = 0; i < indent; i++) printf(" ");
    printf("   %d DataSets\n", nDataSet);
    
    if (nDataSet > 0) {
      printf("\n");
      for (i = 0; i < nDataSet; i++) {
        status = caps_childByIndex(object, DATASET, NONE, i+1, &obj);
        if (status != CAPS_SUCCESS) {
          printf(" CAPS Error: Object %s ret=%d from caps_child(DataSet,%d)!\n",
                 name, status, i+1);
          return;
        }
        printObjects(obj, indent+2);
      }
    }
    
  } else if (type == DATASET) {
    status = caps_getData(object, &npts, &rank, &data, &units);
    if (status != CAPS_SUCCESS) {
      printf(" CAPS Error: Object %s return=%d from caps_getData!\n",
             name, status);
      return;
    }
    for (i = 0; i < indent; i++) printf(" ");
    printf("   %d points, rank=%d, units=%s\n", npts, rank, units);
    
  }
  
}
