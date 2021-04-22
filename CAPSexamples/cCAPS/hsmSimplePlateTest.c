/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             hsm AIM tester
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
#define snprintf   _snprintf
#else
#include <limits.h>
#include <unistd.h>
#endif

#if !defined(__APPLE__) && !defined(WIN32)
// floating point exceptions
#define __USE_GNU
#include <fenv.h>
#endif

int main(int argc, char *argv[])
{

    int status; // Function return status;
    int i; // Indexing

    // CAPS objects
    capsObj  problemObj, hsmObj, tempObj;
    capsErrs *errors;
    capsOwn  current;

    // CAPS return values
    int   nErr;
    char *name;
    enum capsoType   type;
    enum capssType   subtype;
    capsObj link, parent;

    // Input values
    capsTuple        *material = NULL, *constraint = NULL, *property = NULL, *load = NULL;
    int               numMaterial = 1, numConstraint = 1, numProperty = 1, numLoad = 1;

    int               intVal;
    //double            doubleVal;
    enum capsBoolean  boolVal;
    char *stringVal = NULL;

    char analysisPath[PATH_MAX] = "./runDirectory";

#if !defined(__APPLE__) && !defined(WIN32)
    /* enable floating point exceptions for testing */
    feenableexcept(FE_INVALID | FE_OVERFLOW | FE_DIVBYZERO);
#endif

    printf("\n\nAttention: hsmTest2 is hard coded to look for ../csmData/feaSimplePlate.csm\n");
    printf("An analysisPath maybe specified as a command line option, if none is given "
            "a directory called \"runDirectory\" in the current folder is assumed to exist!\n\n");

    if (argc > 2) {

        printf(" usage: hsmTest analysisDirectoryPath!\n");
        return 1;

    } else if (argc == 2) {

        strncpy(analysisPath,
                argv[1],
                strlen(argv[1])*sizeof(char));

        analysisPath[strlen(argv[1])] = '\0';

    } else {

        printf("Assuming the analysis directory path to be -> %s", analysisPath);
    }

    status = caps_open("../csmData/feaSimplePlate.csm", "HSM_Example", &problemObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_info(problemObj, &name, &type, &subtype, &link, &parent, &current);

    // Load the AIMs
    status = caps_load(problemObj, "hsmAIM", analysisPath, NULL, NULL, 0, NULL, &hsmObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Set HSM inputs - Materials
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Material", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    material = (capsTuple *) EG_alloc(numMaterial*sizeof(capsTuple));
    material[0].name = EG_strdup("Madeupium");
    material[0].value = EG_strdup("{\"youngModulus\": 2.2E6, \"density\": 7850, \"poissonRatio\": 0.33}");

    status = caps_setValue(tempObj, numMaterial, 1,  (void **) material);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                       - Properties
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Property", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    property = (capsTuple *) EG_alloc(numProperty*sizeof(capsTuple));
    property[0].name = EG_strdup("plate");
    property[0].value = EG_strdup("{\"propertyType\": \"Shell\", \"membraneThickness\": 0.1}");

    status = caps_setValue(tempObj, numProperty, 1,  (void **) property);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                       - Constraints
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Constraint", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    constraint = (capsTuple *) EG_alloc(numConstraint*sizeof(capsTuple));
    constraint[0].name = EG_strdup("edgeConstraint");
    constraint[0].value = EG_strdup("{\"groupName\": \"plateEdge\", \"dofConstraint\": 123}");

    status = caps_setValue(tempObj, numConstraint, 1,  (void **) constraint);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                       - Analysis
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Load", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    load = (capsTuple *) EG_alloc(numLoad*sizeof(capsTuple));
    load[0].name = EG_strdup("appliedLoad");
    load[0].value = EG_strdup("{\"groupName\": \"plate\", \"loadType\": \"Pressure\", \"pressureForce\": 2.0E6}");

    status = caps_setValue(tempObj, numLoad, 1,  (void **) load);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Edge_Point_Max", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    intVal = 10;
    status = caps_setValue(tempObj, 1, 1, (void *) &intVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Edge_Point_Min", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    intVal = 10;
    status = caps_setValue(tempObj, 1, 1, (void *) &intVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Quad_Mesh", &tempObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    boolVal = (int) false;
    status = caps_setValue(tempObj, 1, 1, (void *) &boolVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Run hsm pre-analysis
    status = caps_preAnalysis(hsmObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Run hsm post
    status = caps_postAnalysis(hsmObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n", status);

        if (constraint != NULL) {
            for (i = 0; i < numConstraint; i++) {

                if (constraint[i].name != NULL) EG_free(constraint[i].name);
                if (constraint[i].value != NULL) EG_free(constraint[i].value);
            }
            EG_free(constraint);
        }

        if (load != NULL) {
            for (i = 0; i < numLoad; i++) {

                if (load[i].name != NULL) EG_free(load[i].name);
                if (load[i].value != NULL) EG_free(load[i].value);
            }
            EG_free(load);
        }

        if (property != NULL) {
            for (i = 0; i < numProperty; i++) {

                if (property[i].name != NULL) EG_free(property[i].name);
                if (property[i].value != NULL) EG_free(property[i].value);
            }
            EG_free(property);
        }

        if (material != NULL) {
            for (i = 0; i < numMaterial; i++) {

                if (material[i].name != NULL) EG_free(material[i].name);
                if (material[i].value != NULL) EG_free(material[i].value);
            }
            EG_free(material);
        }

        if (stringVal != NULL) EG_free(stringVal);

        (void) caps_close(problemObj);

        return status;
}

