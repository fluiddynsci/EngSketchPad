/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Friction AIM tester
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
#define strcasecmp  stricmp
#else
#include <limits.h>
#include <unistd.h>
#endif

int main(int argc, char *argv[])
{
    int status; // Function return status;
    int i; // Indexing

    // CAPS objects
    capsObj          problemObj, frictionObj, tempObj;
    capsErrs         *errors;
    capsOwn          current;

    // CAPS return values
    int   nErr;
    char *name;
    enum capsoType   type;
    enum capssType   subtype;
    enum capsvType   vtype;
    capsObj link, parent;

    // Input values
    double            doubleVal[2];

    int               integerVal;
    //enum capsBoolean  boolVal;

    // Output values
    const char *valunits;
    const void *data;

    char *stringVal = NULL;

    char analysisPath[PATH_MAX] = "./runDirectory";
    char currentPath[PATH_MAX];

    int runAnalysis = (int) true;

    printf("\n\nAttention: frictionTest is hard coded to look for ../csmData/frictionWingTailFuselage.csm\n");
    printf("An analysisPath maybe specified as a command line option, if none is given "
            "a directory called \"runDirectory\" in the current folder is assumed to exist! To "
            "not make system calls to the friction executable the third command line option may be "
            "supplied - 0 = no analysis , >0 run analysis (default).\n\n");

    if (argc > 3) {

        printf(" usage: frictionTest analysisDirectoryPath noAnalysis!\n");
        return 1;

    } else if (argc == 2 || argc == 3) {

        strncpy(analysisPath,
                argv[1],
                strlen(argv[1])*sizeof(char));

        analysisPath[strlen(argv[1])] = '\0';

    } else {

        printf("Assuming the analysis directory path to be -> %s", analysisPath);
    }

    if (argc == 3) {
        if (strcasecmp(argv[2], "0") == 0) runAnalysis = (int) false;
    }


    status = caps_open("../csmData/frictionWingTailFuselage.csm", "FRICTION_Example", &problemObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_info(problemObj, &name, &type, &subtype, &link, &parent, &current);

    // Now load the frictionAIM
    status = caps_load(problemObj, "frictionAIM", analysisPath, NULL, NULL, 0, NULL, &frictionObj);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Find & set Mach number
    status = caps_childByName(frictionObj, VALUE, ANALYSISIN, "Mach", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal[0] = 0.5;
    doubleVal[1] = 1.5;
    status = caps_setValue(tempObj, 2, 1, (void *) &doubleVal);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Find & set Altitude
    status = caps_childByName(frictionObj, VALUE, ANALYSISIN, "Altitude", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal[0] = 29.52756; // kft
    doubleVal[1] = 59.711286;
    status = caps_setValue(tempObj, 2, 1, (void *) &doubleVal);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Do the analysis setup for FRICTIOn;
    status = caps_preAnalysis(frictionObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Execute friction via system call
    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) {
        printf(" ERROR: Cannot change directory to -> %s\n", analysisPath);
        goto cleanup;
    }

    if (runAnalysis == (int) false) {
        printf("\n\nNot Running FRICTION!\n\n");
    } else {
        printf("\n\nRunning FRICTION!\n\n");

#ifdef WIN32
        system("friction.exe frictionInput.txt frictionOutput.txt > Info.out");
#else
        system("friction frictionInput.txt frictionOutput.txt > Info.out");
#endif
    }


    (void) chdir(currentPath);

    status = caps_postAnalysis(frictionObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get  total Cl
    status = caps_childByName(frictionObj, VALUE, ANALYSISOUT, "CDfric", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_getValue(tempObj, &vtype, &integerVal, &data, &valunits, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    for (i = 0; i < integerVal; i++) {
        printf("\nValue of CDfric = %f\n", ((double *) data)[i]);
    }

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n", status);

        if (stringVal != NULL) EG_free(stringVal);

        (void) caps_close(problemObj);

        return status;
}
