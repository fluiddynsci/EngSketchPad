/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             fun3d/tetgen AIM tester
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
    capsObj          problemObj, meshObj, fun3dObj, tempObj;
    capsErrs         *errors;
    capsOwn          current;

    // CAPS return values
    int   nErr;
    char *name;
    enum capsoType   type;
    enum capssType   subtype;
    capsObj link, parent;


    // Input values
    capsTuple        *tupleVal = NULL;
    int               tupleSize = 3;
    double            doubleVal;
    enum capsBoolean  boolVal;

    char *stringVal = NULL;

    char analysisPath[PATH_MAX] = "./runDirectory";
    char currentPath[PATH_MAX];

    printf("\n\nAttention: fun3dTetgenTest is hard coded to look for ../csmData/cfdMultiBody.csm\n");
    printf("An analysisPath maybe specified as a command line option, if none is given "
            "a directory called \"runDirectory\" in the current folder is assumed to exist!\n\n");

    if (argc > 2) {

        printf(" usage: fun3dTetgenTest analysisDirectoryPath!\n");
        return 1;

    } else if (argc == 2) {

        strncpy(analysisPath,
                argv[1],
                strlen(argv[1])*sizeof(char));

        analysisPath[strlen(argv[1])] = '\0';

    } else {

        printf("Assuming the analysis directory path to be -> %s", analysisPath);
    }

    status = caps_open("../csmData/cfdMultiBody.csm", "FUN3D_Tetgen_Example", &problemObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_info(problemObj, &name, &type, &subtype, &link, &parent, &current);

    // Load the AFLR4 AIM */
    status = caps_load(problemObj, "tetgenAIM", analysisPath, NULL, NULL, 0, NULL, &meshObj);
    if (status != CAPS_SUCCESS)  goto cleanup;

    // Set input variables for Tetgen  - None needed

    // Do the analysis -- actually run Tetgen
    status = caps_preAnalysis(meshObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Everything is done in preAnalysis, so we just do the post
    status = caps_postAnalysis(meshObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get our 1 output -- just a complete flag */
    status = caps_childByName(meshObj, VALUE, ANALYSISOUT, "Done", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Now load the fun3dAIM
    status = caps_load(problemObj, "fun3dAIM", analysisPath, NULL, NULL, 1, &meshObj, &fun3dObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set Boundary_Conditions
    status = caps_childByName(fun3dObj, VALUE, ANALYSISIN, "Boundary_Condition", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    tupleVal = (capsTuple *) EG_alloc(tupleSize*sizeof(capsTuple));
    tupleVal[0].name = EG_strdup("Wing1");
    tupleVal[0].value = EG_strdup("{\"bcType\": \"Inviscid\", \"wallTemperature\": 1}");

    tupleVal[1].name = EG_strdup("Wing2");
    tupleVal[1].value =  EG_strdup("{\"bcType\": \"Inviscid\", \"wallTemperature\": 1.2}");

    tupleVal[2].name = EG_strdup("Farfield");
    tupleVal[2].value = EG_strdup("farfield");

    status = caps_setValue(tempObj, tupleSize, 1,  (void **) tupleVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set Mach number input
    status = caps_childByName(fun3dObj, VALUE, ANALYSISIN, "Mach", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal  = 0.4;
    status = caps_setValue(tempObj, 1, 1, (void *) &doubleVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set Overwrite_NML */
    status = caps_childByName(fun3dObj, VALUE, ANALYSISIN, "Overwrite_NML", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    boolVal = true;
    status = caps_setValue(tempObj, 1, 1, (void *) &boolVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Do the analysis setup for FUN3D;
    status = caps_preAnalysis(fun3dObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Execute FUN3D via system call
    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) {
        printf(" ERROR: Cannot change directory to -> %s\n", analysisPath);
        goto cleanup;
    }

    printf(" NOT Running fun3d!\n");
    //system("nodet_mpi > fun3dOutput.txt");

    (void) chdir(currentPath);

    status = caps_postAnalysis(fun3dObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d", status);

        if (tupleVal != NULL) {
            for (i = 0; i < tupleSize; i++) {

                if (tupleVal[i].name != NULL) EG_free(tupleVal[i].name);
                if (tupleVal[i].value != NULL) EG_free(tupleVal[i].value);
            }
            EG_free(tupleVal);
        }
        if (stringVal != NULL) EG_free(stringVal);

        (void) caps_close(problemObj);

        return status;
}
