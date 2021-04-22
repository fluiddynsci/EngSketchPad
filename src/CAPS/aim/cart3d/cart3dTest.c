/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Cart3D AIM tester
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include "caps.h"
#include <string.h>

#ifdef WIN32
#define unlink   _unlink
#define getcwd   _getcwd
#define PATH_MAX _MAX_PATH
#else
#include <limits.h>
#include <unistd.h>
#endif


int main(int argc, char *argv[])
{
  int            i, j, n, stat, nFields, *ranks, dirty, nErr, nLines;
  int            naobj, npts, rank, exec, imm[2];
  char           *apath, *intents, **fnames, **lines, *name, *lunits, *us;
  char           cpath[PATH_MAX];
  const char     *tname;
  double         val, minmax[2], *data, ptess[3] = {0.003, 0.001, 7.5};
  capsObj        pobj, cobj, tobj, obj, bobj, vobj, dobj, link, parent;
  capsObj        *aobjs;
  capsErrs       *errors;
  capsOwn        current;
  ego            body;
  enum capsoType type;
  enum capssType subtype;

  if ((argc < 2) || (argc > 4)) {
    printf(" usage: cart3dTest filename apath [capsname]!\n");
    return 1;
  }

  stat = caps_open(argv[1], "cart3dTest", &pobj);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_open = %d\n", stat);
    if (stat == -308)
      printf("You forgot to include the working directory as the last input arg.\n");
    return 1;
  }

  /* look at the bodies and report units */
  stat = caps_size(pobj, BODIES, NONE, &n);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_size on Bodies = %d\n", stat);
  } else {
    for (i = 1; i <= n; i++) {
      stat = caps_bodyByIndex(pobj, i, &body, &lunits);
      if (stat != CAPS_SUCCESS) {
        printf(" caps_bodyByIndex = %d for Body %d!\n", stat, i);
      } else {
        printf(" Body %d has length units = %s\n", i, lunits);
      }
    }
  }

  /* load the Cart3D AIM */
  stat = caps_load(pobj, "cart3dAIM", argv[2], NULL, NULL, 0, NULL, &cobj);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_load = %d\n", stat);
    caps_close(pobj);
    return 1;
  }
  stat = caps_info(pobj, &name, &type, &subtype, &link, &parent, &current);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_info on Problem = %d\n", stat);
    caps_close(pobj);
    return 1;
  }
  
  /* find & set tessellation input */
  stat = caps_childByName(cobj, VALUE, ANALYSISIN, "Tess_Params", &tobj);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_childByName = %d\n", stat);
  } else {
    stat = caps_setValue(tobj, 1, 3, (void *) &ptess);
    if (stat != CAPS_SUCCESS)
      printf(" caps_setValue = %d\n", stat);
  }
  
  /* find & set alpha input */
  stat = caps_childByName(cobj, VALUE, ANALYSISIN, "alpha", &tobj);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_childByName tParams = %d\n", stat);
  } else {
    val  = 2.0;
    stat = caps_setValue(tobj, 1, 1, (void *) &val);
    if (stat != CAPS_SUCCESS)
      printf(" caps_setValue alpha = %d\n", stat);
  }
  
  /* find & set maxR input */
  stat = caps_childByName(cobj, VALUE, ANALYSISIN, "maxR", &tobj);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_childByName maxR = %d\n", stat);
  } else {
    j    = 12;
    stat = caps_setValue(tobj, 1, 1, (void *) &j);
    if (stat != CAPS_SUCCESS)
      printf(" caps_setValue maxR = %d\n", stat);
  }
  
  /* make a bound */
  tname = "Top";
  stat = caps_makeBound(pobj, 2, tname, &bobj);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_makeBound: %s = %d\n", tname, stat);
    bobj = NULL;
    vobj = NULL;
    dobj = NULL;
  } else {
    stat = caps_makeVertexSet(bobj, cobj, NULL, &vobj);
    if (stat != CAPS_SUCCESS) {
      printf(" caps_makeVertexSet %s = %d\n", tname, stat);
      vobj = NULL;
      dobj = NULL;
    } else {
      stat = caps_makeDataSet(vobj, "Pressure", Analysis, 1, &dobj);
/*    stat = caps_makeDataSet(vobj, "Velocity", Analysis, 3, &dobj);  */
      if (stat != CAPS_SUCCESS)
        printf(" caps_makeDataSet Pressure = %d\n", stat);
    }
    stat = caps_completeBound(bobj);
    if (stat != CAPS_SUCCESS)
      printf(" caps_completeBound %s = %d\n", tname, stat);
  }

  /* get Cart3D AIM analysis info */
  stat = caps_analysisInfo(cobj, &apath, &us, &intents, &naobj, &aobjs,
                           &nFields, &fnames, &ranks, &exec, &dirty);
  if (stat != CAPS_SUCCESS) {
    printf(" caps_analysisInfo = %d\n", stat);
    caps_close(pobj);
    return 1;
  }
  printf("\n Cart3D Intent   = %s", intents);
  printf("\n APath           = %s", apath);
  printf("\n Num. of parents = %d", naobj);
  printf("\n Fields          =");
  for (i = 0; i < nFields; i++) printf("  %s (%d)", fnames[i], ranks[i]);
  printf("\n Dirty           = %d\n", dirty);

  /* do the analysis */
  if (dirty != 0) {
    stat = caps_preAnalysis(cobj, &nErr, &errors);

    if (stat != CAPS_SUCCESS)
      printf(" caps_preAnalysis Cart3D = %d\n", stat);
    if (errors != NULL) {
      for (i = 0; i < nErr; i++) {
        stat = caps_errorInfo(errors, i+1, &obj, &nLines, &lines);
        if (stat != CAPS_SUCCESS) {
          printf(" CAPS Error: caps_errorInfo[%d] = %d!\n", i+1, stat);
          continue;
        }
        for (j = 0; j < nLines; j++) printf(" %s\n", lines[j]);
      }
      caps_freeError(errors);
    }

    dirty = 0;
    stat = caps_analysisInfo(cobj, &apath, &us, &intents, &naobj, &aobjs,
                                   &nFields, &fnames, &ranks, &exec, &dirty);
    if (stat == CAPS_SUCCESS) printf("\n Dirty           = %d\n", dirty);
    
    /* execute flowCart and run the post */
    if (dirty == 5) {
      
      (void) getcwd(cpath, PATH_MAX);
      if (chdir(apath) != 0) {
        printf(" ERROR: Cannot change directory to -> %s\n", apath);
        caps_close(pobj);
        return 1;
      }
      printf(" Running flowCart!\n");
      stat = system("flowCart");
      /* check if ran correctly before invoking postAnalysis */
      printf(" flowCart = %d\n", stat);
      chdir(cpath);
      
      if (stat == 0) {
        /* run post if we are OK */
        stat = caps_postAnalysis(cobj, current, &nErr, &errors);
        if (stat != CAPS_SUCCESS)
          printf(" caps_postAnalysis = %d\n", stat);
        if (errors != NULL) {
          for (i = 0; i < nErr; i++) {
            stat = caps_errorInfo(errors, i+1, &obj, &nLines, &lines);
            if (stat != CAPS_SUCCESS) {
              printf(" CAPS Error: caps_errorInfo[%d] = %d!\n", i+1, stat);
              continue;
            }
            for (j = 0; j < nLines; j++) printf(" %s\n", lines[j]);
          }
          caps_freeError(errors);
        }
      }
      dirty = 0;
      stat  = caps_analysisInfo(cobj, &apath, &us, &intents, &naobj, &aobjs,
                                &nFields, &fnames, &ranks, &exec, &dirty);
      if (stat == CAPS_SUCCESS) printf(" Dirty    = %d\n", dirty);
    }
  }
  
  /* output what we have */
  printf("\n");
  printObjects(pobj, 0);
  printf("\n");
  
  /* min and max on our dataset */
  if (dobj != NULL) {
    stat = caps_getData(dobj, &npts, &rank, &data, &lunits);
    if (stat != CAPS_SUCCESS) {
      printf(" caps_getData = %d\n", stat);
    } else {
      printf(" DataSet has %d points with rank = %d\n", npts, rank);
      for (i = 0; i < rank; i++) {
        minmax[0] = data[i];
        minmax[1] = data[i];
        imm[0]    = imm[1] = 1;
        for (j = 1; j < npts; j++) {
          if (data[rank*j+i] < minmax[0]) {
            minmax[0] = data[rank*j+i];
            imm[0]    = j+1;
          }
          if (data[rank*j+i] > minmax[1]) {
            minmax[1] = data[rank*j+i];
            imm[1]    = j+1;
          }
        }
        printf("     %d: min = %lf (%d), max = %lf (%d)\n",
               i+1, minmax[0], imm[0], minmax[1], imm[1]);
      }
      printf("\n");
    }
  }

  /* checkpoint the Problem */
  if (argc == 4) {
    stat = caps_save(pobj, argv[3]);
    if (stat != CAPS_SUCCESS) printf(" caps_save = %d\n", stat);
  }

  stat = caps_close(pobj);
  if (stat != CAPS_SUCCESS) printf(" caps_close = %d\n", stat);

  return 0;
}
