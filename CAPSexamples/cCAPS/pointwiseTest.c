/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             pointwise AIM tester
 *
 *      Copyright 2014-2021, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include "caps.h"
#include <string.h>

#ifdef WIN32
#define unlink     _unlink
#define getcwd     _getcwd
#define PATH_MAX   _MAX_PATH
#define strcasecmp  stricmp
#else
#include <limits.h>
#include <unistd.h>
#endif


static void
printErrors(int nErr, capsErrs *errors)
{
  int         i, j, stat, eType, nLines;
  char        **lines;
  capsObj     obj;
  static char *type[5] = {"Cont:   ", "Info:   ", "Warning:", "Error:  ",
                          "Status: "};

  if (errors == NULL) return;

  for (i = 1; i <= nErr; i++) {
    stat = caps_errorInfo(errors, i, &obj, &eType, &nLines, &lines);
    if (stat != CAPS_SUCCESS) {
      printf(" printErrors: %d/%d caps_errorInfo = %d\n", i, nErr, stat);
      continue;
    }
    for (j = 0; j < nLines; j++) {
      if (j == 0) {
        printf(" CAPS %s ", type[eType+1]);
      } else {
        printf("               ");
      }
      printf("%s\n", lines[j]);
    }
  }
  
  caps_freeError(errors);
}


int main(int argc, char *argv[])
{

    int i, status; // Function return status;

    // CAPS objects
    capsObj          problemObj, pointwiseObj;
    capsErrs         *errors;

    // CAPS return values
    int   nErr;

    // Input values
    //double            doubleVal;
    //int               integerVal;
    //enum capsBoolean  boolVal;
    char *stringVal = NULL;

    int major, minor, nFields, *ranks, *fInOut, dirty, exec;
    char *analysisPath, *unitSystem, *intents, **fnames;

    char currentPath[PATH_MAX];

    int runAnalysis = (int) true;

    printf("\n\nAttention: pointwiseTest is hard coded to look for ../csmData/cfdMultiBody.csm\n");
    printf("An analysisPath maybe specified as a command line option, if none is given "
            "a directory called \"runDirectory\" in the current folder is assumed to exist! To "
            "not make system calls to the pointwise executable the third command line option may be "
            "supplied - 0 = no analysis , >0 run analysis (default).\n\n");

    if (argc > 2) {
      
        printf(" usage: pointwiseTest noAnalysis!\n");
        return 1;
    }

    if (argc == 2) {
        if (strcasecmp(argv[2], "0") == 0) runAnalysis = (int) false;
    }

    status = caps_open("Pointwise_Example", NULL, 0,
                       "../csmData/cfdMultiBody.csm", 1,
                       &problemObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Now load the pointwiseAIM
    status = caps_makeAnalysis(problemObj, "pointwiseAIM", "pointwise",
                               NULL, NULL, 0, &pointwiseObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Do the analysis setup for pointwise;
    status = caps_preAnalysis(pointwiseObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    /* get analysis info */
    status = caps_analysisInfo(pointwiseObj, &analysisPath, &unitSystem, &major,
                               &minor, &intents, &nFields, &fnames, &ranks,
                               &fInOut, &exec, &dirty);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Execute pointwise via system call
    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) {
        status = CAPS_DIRERR;
        printf(" ERROR: Cannot change directory to -> %s\n", analysisPath);
        goto cleanup;
    }

    if (runAnalysis == (int) false) {
        printf("\n\nNot Running pointwise!\n\n");
    } else {
        printf("\n\nRunning pointwise!\n\n");

        system("pointwise -b  $CAPS_GLYPH/GeomToMesh.glf caps.egads capsUserDefaults.glf");
    }

    (void) chdir(currentPath);

    status = caps_postAnalysis(pointwiseObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);

cleanup:
    if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n",
                                       status);

    if (stringVal != NULL) EG_free(stringVal);

    i = 0;
    if (status == CAPS_SUCCESS) i = 1;
    (void) caps_close(problemObj, i, NULL);

    return status;
}
