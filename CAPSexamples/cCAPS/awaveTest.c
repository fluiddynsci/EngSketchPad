/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             awave AIM tester
 *
 * *      Copyright 2014-2023, Massachusetts Institute of Technology
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

    int status; // Function return status;
    int i; // Indexing
    int outLevel = 1;

    // CAPS objects
    capsObj          problemObj, awaveObj, tempObj;
    capsErrs         *errors;

    // CAPS return values
    int   nErr;
    enum capsvType   vtype;

    // Input values
    double    doubleVal[2];
    //enum capsBoolean  boolVal;

    // Output values
    int nrow, ncol;
    const char *valunits;
    const void *data;
    const int  *partial;

    char *stringVal = NULL;

    int exec;

    printf("\n\nAttention: awaveTest is hard coded to look for ../csmData/awaveWingTailFuselage.csm\n");

    if (argc > 2) {
        printf(" usage: awaveTest outLevel\n");
        return 1;
    } else if (argc == 2) {
        outLevel = atoi(argv[1]);
    }

    status = caps_open("awave_Example", NULL, 0, "../csmData/awaveWingTailFuselage.csm",
                       outLevel, &problemObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Now load the awaveAIM (with auto execution)
    exec   = 1;
    status = caps_makeAnalysis(problemObj, "awaveAIM", NULL, NULL, NULL, &exec,
                               &awaveObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set Mach number
    status = caps_childByName(awaveObj, VALUE, ANALYSISIN, "Mach", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal[0] = 1.2;
    doubleVal[1] = 1.5;
    status = caps_setValue(tempObj, Double, 2, 1, (void *) doubleVal,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set AoA
    status = caps_childByName(awaveObj, VALUE, ANALYSISIN, "Alpha", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal[0] = 0.0;
    doubleVal[1] = 2.0;
    status = caps_setValue(tempObj, Double, 2, 1, (void *) doubleVal,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Awave analysis executes automatically

    // Get total CdWave
    status = caps_childByName(awaveObj, VALUE, ANALYSISOUT, "CDwave", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_getValue(tempObj, &vtype, &nrow, &ncol, &data, &partial,
                           &valunits, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    printf("\nValue of CdWave = %f %f\n", ((double *) data)[0], ((double *) data)[1]);

cleanup:
    if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n",
                                       status);

    EG_free(stringVal);

    i = 0;
    if (status == CAPS_SUCCESS) i = 1;
    (void) caps_close(problemObj, i, NULL);

    return status;
}
