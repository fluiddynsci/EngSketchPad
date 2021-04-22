/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             pointwise AIM tester
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

    // CAPS objects
    capsObj          problemObj, pointwiseObj;
    capsErrs         *errors;
    capsOwn          current;

    // CAPS return values
    int   nErr;
    char *name;
    enum capsoType   type;
    enum capssType   subtype;
    //enum capsvType   vtype;
    capsObj link, parent;


    // Input values
    //double            doubleVal;
    //int               integerVal;
    //enum capsBoolean  boolVal;
    char *stringVal = NULL;

    char analysisPath[PATH_MAX] = "runDirectory";
    char currentPath[PATH_MAX];

    int runAnalysis = (int) true;

    printf("\n\nAttention: pointwiseTest is hard coded to look for ../csmData/cfdMultiBody.csm\n");
    printf("An analysisPath maybe specified as a command line option, if none is given "
            "a directory called \"runDirectory\" in the current folder is assumed to exist! To "
            "not make system calls to the pointwise executable the third command line option may be "
            "supplied - 0 = no analysis , >0 run analysis (default).\n\n");

    if (argc > 3) {

        printf(" usage: pointwiseTest analysisDirectoryPath noAnalysis!\n");
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

    status = caps_open("../csmData/cfdMultiBody.csm", "Pointwise_Example", &problemObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_info(problemObj, &name, &type, &subtype, &link, &parent, &current);

    // Now load the pointwiseAIM
    status = caps_load(problemObj, "pointwiseAIM", analysisPath, NULL, NULL, 0, NULL, &pointwiseObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Do the analysis setup for pointwise;
    status = caps_preAnalysis(pointwiseObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Execute pointwise via system call
    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) {
        printf(" ERROR: Cannot change directory to -> %s\n", analysisPath);
        goto cleanup;
    }

    if (runAnalysis == (int) false) {
        printf("\n\nNot Running pointwise!\n\n");
    } else {
        printf("\n\nRunning pointwise!\n\n");

        system("pointwise AutoMeshBatch.glf caps.nmb capsInput.txt");
    }

    (void) chdir(currentPath);

    status = caps_postAnalysis(pointwiseObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n", status);

        if (stringVal != NULL) EG_free(stringVal);

        (void) caps_close(problemObj);

        return status;
}
