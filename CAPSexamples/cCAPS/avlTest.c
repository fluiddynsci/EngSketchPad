/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             avl AIM tester
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
    capsObj          problemObj, avlObj, tempObj;
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
    capsTuple        *surfaceTuple = NULL, *flapTuple = NULL;
    int               surfaceSize = 2, flapSize = 3;
    double            doubleVal;
    int               integerVal;
    //enum capsBoolean  boolVal;

    // Output values
    const char *valunits;
    const void *data;

    char *stringVal = NULL;

    char analysisPath[PATH_MAX] = "./runDirectory";
    char currentPath[PATH_MAX];

    int runAnalysis = (int) true;

    printf("\n\nAttention: avlTest is hard coded to look for ../csmData/avlWingTail.csm\n");
    printf("An analysisPath maybe specified as a command line option, if none is given "
            "a directory called \"runDirectory\" in the current folder is assumed to exist! To "
            "not make system calls to the avl executable the third command line option may be "
            "supplied - 0 = no analysis , >0 run analysis (default).\n\n");

    if (argc > 3) {

        printf(" usage: avlTest analysisDirectoryPath noAnalysis!\n");
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

    status = caps_open("../csmData/avlWingTail.csm", "AVL_Example", &problemObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_info(problemObj, &name, &type, &subtype, &link, &parent, &current);

    // Now load the avlAIM
    status = caps_load(problemObj, "avlAIM", analysisPath, NULL, NULL, 0, NULL, &avlObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set AVL_Surface
    status = caps_childByName(avlObj, VALUE, ANALYSISIN, "AVL_Surface", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    surfaceTuple = (capsTuple *) EG_alloc(surfaceSize*sizeof(capsTuple));
    surfaceTuple[0].name = EG_strdup("Wing");
    surfaceTuple[0].value = EG_strdup("{\"numChord\": 8, \"spaceChord\": 1, \"numSpanPerSection\": 12, \"spaceSpan\": 1}");

    surfaceTuple[1].name = EG_strdup("Vertical_Tail");
    surfaceTuple[1].value = EG_strdup("{\"numChord\": 5, \"spaceChord\": 1, \"numSpanTotal\": 10, \"spaceSpan\": 1}");

    status = caps_setValue(tempObj, surfaceSize, 1,  (void **) surfaceTuple);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Find & set AVL_Control
    status = caps_childByName(avlObj, VALUE, ANALYSISIN, "AVL_Control", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    flapTuple = (capsTuple *) EG_alloc(flapSize*sizeof(capsTuple));
    flapTuple[0].name = EG_strdup("WingRightLE");
    flapTuple[0].value = EG_strdup("{\"controlGain\": 0.5, \"deflectionAngle\": 25}");

    flapTuple[1].name = EG_strdup("WingRightTE");
    flapTuple[1].value = EG_strdup("{\"controlGain\": 1.0, \"deflectionAngle\": 15}");

    flapTuple[2].name = EG_strdup("Tail");
    flapTuple[2].value = EG_strdup("{\"controlGain\": 1.0, \"deflectionAngle\": 15}");

    status = caps_setValue(tempObj, flapSize, 1,  (void **) flapTuple);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set Mach number
    status = caps_childByName(avlObj, VALUE, ANALYSISIN, "Mach", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal  = 0.5;
    status = caps_setValue(tempObj, 1, 1, (void *) &doubleVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set AoA
    status = caps_childByName(avlObj, VALUE, ANALYSISIN, "Alpha", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal  = 1.0;
    status = caps_setValue(tempObj, 1, 1, (void *) &doubleVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Do the analysis setup for AVL;
    status = caps_preAnalysis(avlObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Execute avl via system call
    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) {
        printf(" ERROR: Cannot change directory to -> %s\n", analysisPath);
        goto cleanup;
    }

    if (runAnalysis == (int) false) {
        printf("\n\nNot Running AVL!\n\n");
    } else {
        printf("\n\nRunning AVL!\n\n");

#ifdef WIN32
        system("avl.exe caps < avlInput.txt > avlOutput.txt");
#else
        system("avl caps < avlInput.txt > avlOutput.txt");
#endif
    }

    (void) chdir(currentPath);

    status = caps_postAnalysis(avlObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get  total Cl
    status = caps_childByName(avlObj, VALUE, ANALYSISOUT, "CLtot", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_getValue(tempObj, &vtype, &integerVal, &data, &valunits, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    printf("\nValue of Cltot = %f\n", ((double *) data)[0]);

    // Get strip forces
    status = caps_childByName(avlObj, VALUE, ANALYSISOUT, "StripForces", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_getValue(tempObj, &vtype, &integerVal, &data, &valunits, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    printf("\nStripForces\n\n");
    printf("%s = %s\n\n", ((capsTuple *) data)[0].name, ((capsTuple *) data)[0].value);
    printf("%s = %s\n\n", ((capsTuple *) data)[1].name, ((capsTuple *) data)[1].value);

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n", status);

        if (surfaceTuple != NULL) {
            for (i = 0; i < surfaceSize; i++) {

                if (surfaceTuple[i].name != NULL) EG_free(surfaceTuple[i].name);
                if (surfaceTuple[i].value != NULL) EG_free(surfaceTuple[i].value);
            }
            EG_free(surfaceTuple);
        }

        if (flapTuple != NULL) {
            for (i = 0; i < flapSize; i++) {

                if (flapTuple[i].name != NULL) EG_free(flapTuple[i].name);
                if (flapTuple[i].value != NULL) EG_free(flapTuple[i].value);
            }
            EG_free(flapTuple);
        }

        if (stringVal != NULL) EG_free(stringVal);

        (void) caps_close(problemObj);

        return status;
}
