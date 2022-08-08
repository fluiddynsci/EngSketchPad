/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             hsm AIM tester
 *
 *      Copyright 2014-2022, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include "caps.h"
#include <string.h>
#include <math.h>

#ifdef WIN32
#define unlink     _unlink
#define getcwd     _getcwd
#define PATH_MAX   _MAX_PATH
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
    capsObj  problemObj, hsmObj, tempObj;
    capsErrs *errors;
    capsOwn  current;

    // CAPS return values
    int   nErr, exec;
    char *name;
    enum capsoType   type;
    enum capssType   subtype;
    capsObj link, parent;

    // Input values
    capsTuple        *material = NULL, *constraint = NULL, *property = NULL, *load = NULL;
    int               numMaterial = 1, numConstraint = 1, numProperty = 1, numLoad = 1;

    int               intVal;
    double            doubleVal;
    enum capsBoolean  boolVal;
    char *stringVal = NULL;

    int               state;
    double            plateLength;
    double            youngModulus;
    double            poissonRatio;
    double            shearModulus;
    double            tshell;
    double            dpar, myload;
    char              line[128];

#if !defined(__APPLE__) && !defined(WIN32)
    /* enable floating point exceptions for testing */
    feenableexcept(FE_INVALID | FE_OVERFLOW | FE_DIVBYZERO);
#endif

    printf("\n\nAttention: hsmJoinedPlate is hard coded to look for ../csmData/feaJoinedPlate.csm\n");

    if (argc > 2) {
        printf(" usage: hsmTest!\n");
        return 1;
    } else if (argc == 2) {
        outLevel = atoi(argv[1]);
    }

    status = caps_open("HSM_JoinedPlate_Example", NULL, 0,
                       "../csmData/feaJoinedPlate.csm", outLevel,
                       &problemObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_info(problemObj, &name, &type, &subtype, &link, &parent,
                       &current);
    if (status != CAPS_SUCCESS)  goto cleanup;

    // Change the plate to length = 1, width = 0.2
    status = caps_childByName(problemObj, VALUE, GEOMETRYIN, "plateLength", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    plateLength = 1.0;
    status = caps_setValue(tempObj, Double, 1, 1, (void *) &plateLength, NULL,
                           NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_childByName(problemObj, VALUE, GEOMETRYIN, "plateWidth", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal = 0.1;
    status = caps_setValue(tempObj, Double, 1, 1, (void *) &doubleVal, NULL,
                           NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Load the AIMs
    exec   = 1;
    status = caps_makeAnalysis(problemObj, "hsmAIM", NULL, NULL, NULL, &exec,
                               &hsmObj, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Set HSM inputs - Materials
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Material", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    youngModulus = 10000.0;
    poissonRatio = 0.0;
    shearModulus = youngModulus/(2*(1+poissonRatio));
    snprintf(line, 128,"{\"massPerArea\": 1.0, \"youngModulus\": %lf, \"poissonRatio\": %lf, \"shearModulus\": %lf}",
                       youngModulus, poissonRatio, shearModulus);

    material = (capsTuple *) EG_alloc(numMaterial*sizeof(capsTuple));
    material[0].name = EG_strdup("Madeupium");
    material[0].value = EG_strdup(line);

    status = caps_setValue(tempObj, Tuple, numMaterial, 1,  (void **) material,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                       - Properties
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Property", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    tshell = 0.1 * pow(1.2, 1.0/3.0);

    property = (capsTuple *) EG_alloc(numProperty*sizeof(capsTuple));
    property[0].name  = EG_strdup("plate");
    snprintf(line, 128,"{\"propertyType\": \"Shell\", \"membraneThickness\": %lf}", tshell);
    property[0].value = EG_strdup(line);

    status = caps_setValue(tempObj, Tuple, numProperty, 1,  (void **) property,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                       - Constraints
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Constraint", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    constraint = (capsTuple *) EG_alloc(numConstraint*sizeof(capsTuple));
    constraint[0].name = EG_strdup("edgeConstraint");
    constraint[0].value = EG_strdup("{\"groupName\": \"plateEdge\", \"dofConstraint\": 123456}");

    status = caps_setValue(tempObj, Tuple, numConstraint, 1,  (void **) constraint,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                       - Analysis
    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Load", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    load = (capsTuple *) EG_alloc(numLoad*sizeof(capsTuple));
    load[0].name = EG_strdup("appliedLoad");
    //load[0].value = EG_strdup("{\"groupName\": \"plateTip\", \"loadType\": \"LineForce\", \"forceScaleFactor\": 1.0, \"directionVector\": [0.0, 0.0, 1.0]}");

    // D = t^3 E / [ 12 (1-v^2) ]
    dpar = pow(tshell,3) * youngModulus / (12.0 * (1.0-poissonRatio*poissonRatio));

    // end bending moment to bend beam into a circle segment
    myload = -2.0*atan(1.)*4.*dpar/plateLength;

    snprintf(line, 128,"{\"groupName\": \"plateTip\", \"loadType\": \"LineMoment\", \"momentScaleFactor\": %lf, \"directionVector\": [0.0, 1.0, 0.0]}", myload);
    load[0].value = EG_strdup(line);

    status = caps_setValue(tempObj, Tuple, numLoad, 1,  (void **) load,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Edge_Point_Max", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    intVal = 5;
    status = caps_setValue(tempObj, Integer, 1, 1, (void *) &intVal,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Edge_Point_Min", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    intVal = 2;
    status = caps_setValue(tempObj, Integer, 1, 1, (void *) &intVal,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_childByName(hsmObj, VALUE, ANALYSISIN, "Quad_Mesh", &tempObj,
                              &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    boolVal = (int) true;
    status = caps_setValue(tempObj, Boolean, 1, 1, (void *) &boolVal,
                           NULL, NULL, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Run hsm
    status = caps_execute(hsmObj, &state, &nErr, &errors);
    if (nErr != 0) printErrors(nErr, errors);
    if (status != CAPS_SUCCESS) goto cleanup;


cleanup:
    if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n",
                                       status);

    if (constraint != NULL) {
        for (i = 0; i < numConstraint; i++) {
            if (constraint[i].name  != NULL) EG_free(constraint[i].name);
            if (constraint[i].value != NULL) EG_free(constraint[i].value);
        }
        EG_free(constraint);
    }

    if (load != NULL) {
        for (i = 0; i < numLoad; i++) {
            if (load[i].name  != NULL) EG_free(load[i].name);
            if (load[i].value != NULL) EG_free(load[i].value);
        }
        EG_free(load);
    }

    if (property != NULL) {
        for (i = 0; i < numProperty; i++) {
            if (property[i].name  != NULL) EG_free(property[i].name);
            if (property[i].value != NULL) EG_free(property[i].value);
        }
        EG_free(property);
    }

    if (material != NULL) {
        for (i = 0; i < numMaterial; i++) {
            if (material[i].name  != NULL) EG_free(material[i].name);
            if (material[i].value != NULL) EG_free(material[i].value);
        }
        EG_free(material);
    }

    if (stringVal != NULL) EG_free(stringVal);

    i = 0;
    if (status == CAPS_SUCCESS) i = 1;
    (void) caps_close(problemObj, i, NULL);

    return status;
}
