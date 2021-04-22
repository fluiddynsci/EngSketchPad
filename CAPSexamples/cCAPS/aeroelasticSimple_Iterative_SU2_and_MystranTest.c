/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             SU2, tetgen, mystran AIM tester
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


/***********************************************************************/
/*                                                                     */
/*   helper functions                                                  */
/*                                                                     */
/***********************************************************************/

static int
setValueByName_Double(capsObj probObj,              /* (in)  problem object */
                      int     type,                 /* (in)  variable type */
                      char    name[],               /* (in)  variable name */
                      int     nrow,                 /* (in)  number of rows */
                      int     ncol,                 /* (in)  number of columns */
                      double *value)                /* (in)  value to set */
{
    int  status = CAPS_SUCCESS;

    int       nErr;
    capsErrs  *errors;

    capsObj          valObj;
    const double     *data;
    int              vlen;
    const char       *units;
    enum capsvType   vtype;

    /* --------------------------------------------------------------- */

    status = caps_childByName(probObj, VALUE, type, name, &valObj);
    if (status != CAPS_SUCCESS) {
        printf("caps_childByName(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

    status = caps_getValue(valObj, &vtype, &vlen, (const void**)(&data), &units, &nErr, &errors);
    if (status != CAPS_SUCCESS) {
        printf("caps_getValue -> status=%d\n", status);
        goto cleanup;
    } else if (vtype != Double) {
        printf("'%s' is not Double data type\n", name);
        status = CAPS_BADTYPE;
        goto cleanup;
    }

    status = caps_setValue(valObj, nrow, ncol, (void*)value);
    if (status != CAPS_SUCCESS) {
        printf("caps_setValue(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

cleanup:
    return status;
}

static int
setValueByName_Integer(capsObj probObj,              /* (in)  problem object */
                       int     type,                 /* (in)  variable type */
                       char    name[],               /* (in)  variable name */
                       int     nrow,                 /* (in)  number of rows */
                       int     ncol,                 /* (in)  number of columns */
                       int    *value)                /* (in)  value to set */
{
    int  status = CAPS_SUCCESS;

    int       nErr;
    capsErrs  *errors;

    capsObj          valObj;
    const double     *data;
    int              vlen;
    const char       *units;
    enum capsvType   vtype;

    /* --------------------------------------------------------------- */

    status = caps_childByName(probObj, VALUE, type, name, &valObj);
    if (status != CAPS_SUCCESS) {
        printf("caps_childByName(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

    status = caps_getValue(valObj, &vtype, &vlen, (const void**)(&data), &units, &nErr, &errors);
    if (status != CAPS_SUCCESS) {
        printf("caps_getValue -> status=%d\n", status);
        goto cleanup;
    } else if (vtype != Integer) {
        printf("'%s' is not Integer data type\n", name);
        status = CAPS_BADTYPE;
        goto cleanup;
    }

    status = caps_setValue(valObj, nrow, ncol, (void*)value);
    if (status != CAPS_SUCCESS) {
        printf("caps_setValue(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

cleanup:
    return status;
}

static int
setValueByName_Boolean(capsObj           probObj,              /* (in)  problem object */
                       int               type,                 /* (in)  variable type */
                       char              name[],               /* (in)  variable name */
                       int               nrow,                 /* (in)  number of rows */
                       int               ncol,                 /* (in)  number of columns */
                       enum capsBoolean *value)                /* (in)  value to set */
{
    int  status = CAPS_SUCCESS;

    int       nErr;
    capsErrs  *errors;

    capsObj          valObj;
    const double     *data;
    int              vlen;
    const char       *units;
    enum capsvType   vtype;

    /* --------------------------------------------------------------- */

    status = caps_childByName(probObj, VALUE, type, name, &valObj);
    if (status != CAPS_SUCCESS) {
        printf("caps_childByName(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

    status = caps_getValue(valObj, &vtype, &vlen, (const void**)(&data), &units, &nErr, &errors);
    if (status != CAPS_SUCCESS) {
        printf("caps_getValue -> status=%d\n", status);
        goto cleanup;
    } else if (vtype != Boolean) {
        printf("'%s' is not Boolean data type\n", name);
        status = CAPS_BADTYPE;
        goto cleanup;
    }

    status = caps_setValue(valObj, nrow, ncol, (void*)value);
    if (status != CAPS_SUCCESS) {
        printf("caps_setValue(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

cleanup:
    return status;
}

static int
setValueByName_Tuple(capsObj    probObj,              /* (in)  problem object */
                     int        type,                 /* (in)  variable type */
                     char       name[],               /* (in)  variable name */
                     int        nrow,                 /* (in)  number of rows */
                     int        ncol,                 /* (in)  number of columns */
                     capsTuple *value)                /* (in)  value to set */
{
    int  status = CAPS_SUCCESS;

    int       nErr;
    capsErrs  *errors;

    capsObj          valObj;
    const double     *data;
    int              vlen;
    const char       *units;
    enum capsvType   vtype;

    /* --------------------------------------------------------------- */

    status = caps_childByName(probObj, VALUE, type, name, &valObj);
    if (status != CAPS_SUCCESS) {
        printf("caps_childByName(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

    status = caps_getValue(valObj, &vtype, &vlen, (const void**)(&data), &units, &nErr, &errors);
    if (status != CAPS_SUCCESS) {
        printf("caps_getValue -> status=%d\n", status);
        goto cleanup;
    } else if (vtype != Tuple) {
        printf("'%s' is not Tuple data type\n", name);
        status = CAPS_BADTYPE;
        goto cleanup;
    }

    status = caps_setValue(valObj, nrow, ncol, (void*)value);
    if (status != CAPS_SUCCESS) {
        printf("caps_setValue(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

cleanup:
    return status;
}

static int
setValueByName_String(capsObj     probObj,              /* (in)  problem object */
                      int         type,                 /* (in)  variable type */
                      char        name[],               /* (in)  variable name */
                      int         nrow,                 /* (in)  number of rows */
                      int         ncol,                 /* (in)  number of columns */
                      const char *value)                /* (in)  value to set */
{
    int  status = CAPS_SUCCESS;

    int       nErr;
    capsErrs  *errors;

    capsObj          valObj;
    const double     *data;
    int              vlen;
    const char       *units;
    enum capsvType   vtype;

    /* --------------------------------------------------------------- */

    status = caps_childByName(probObj, VALUE, type, name, &valObj);
    if (status != CAPS_SUCCESS) {
        printf("caps_childByName(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

    status = caps_getValue(valObj, &vtype, &vlen, (const void**)(&data), &units, &nErr, &errors);
    if (status != CAPS_SUCCESS) {
        printf("caps_getValue -> status=%d\n", status);
        goto cleanup;
    } else if (vtype != String) {
        printf("'%s' is not String data type\n", name);
        status = CAPS_BADTYPE;
        goto cleanup;
    }

    status = caps_setValue(valObj, nrow, ncol, (void*)value);
    if (status != CAPS_SUCCESS) {
        printf("caps_setValue(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

cleanup:
    return status;
}

static void
freeTuple(int tlen,          /* (in)  length of the tuple */
          capsTuple *tuple)  /* (in)  tuple freed */
{
    int i;

    if (tuple != NULL) {
        for (i = 0; i < tlen; i++) {

            EG_free(tuple[i].name);
            EG_free(tuple[i].value);
        }
    }
    EG_free(tuple);
}

/***********************************************************************/
/*                                                                     */
/*   main - main program                                               */
/*                                                                     */
/***********************************************************************/

int main(int argc, char *argv[])
{

    int status; // Function return status;
    int i, iter; // Indexing

    // CAPS objects
    capsObj  problemObj, meshObj, su2Obj, mystranObj;
    capsErrs *errors;
    capsOwn  current;

    // Data transfer
    const char *transfers[3] = {"Skin_Top", "Skin_Bottom", "Skin_Tip"};
    capsObj  boundObj[3], vertexSU2Obj[3], vertexMystranObj[3], pressureSU2Obj[3], pressureMystranObj[3], displacementMystranObj[3], displacementSU2Obj[3];
    int npts, rank;
    double *data;
    char *units;
#ifdef DEBUG
    char filename[256];
#endif

    // CAPS return values
    int   nErr;
    char *name;
    enum capsoType   type;
    enum capssType   subtype;
    capsObj link, parent;


    // Input values
    capsTuple         *su2BC = NULL, *material = NULL, *constraint = NULL, *property = NULL, *load = NULL;
    int               numSU2BC = 3, numMaterial = 1, numConstraint = 1, numProperty = 2, numLoad = 1;

    int               intVal;
    double            doubleVal, tessParams[3];
    enum capsBoolean  boolVal;
    double            displacement0[3] = {0,0,0};

    char analysisPath[PATH_MAX]= "runDirectory", projectName[] = "aeroelasticSimple_Iterative";
    char currentPath[PATH_MAX];

    double speedofSound = 340.0, refVelocity = 100.0, refDensity = 1.2;

    int numIteration = 2;

    int runAnalysis = (int) true;

    printf("\n\nAttention: aeroelasticIterativeTest is hard coded to look for ../csmData/aeroelasticDataTransferSimple.csm\n");
    printf("An analysisPath maybe specified as a command line option, if none is given "
            "a directory called \"runDirectory\" in the current folder is assumed to exist! To "
            "not make system calls to the su2 and mystran executables the third command line option may be "
            "supplied - 0 = no analysis , >0 run analysis (default).\n\n");

    if (argc > 3) {

        printf(" usage: aeroelasticSimple_Iterative_SU2_and_Mystran analysisDirectoryPath runAnalysis!\n");
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

    status = caps_open("../csmData/aeroelasticDataTransferSimple.csm", "SU2_MyStran_Aeroelastic_Interative_Example", &problemObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_info(problemObj, &name, &type, &subtype, &link, &parent, &current);

    /* --------------------------------------------------------------- */

    // Load the AIMs
    status = caps_load(problemObj, "tetgenAIM", analysisPath, NULL, NULL, 0, NULL, &meshObj);
    if (status != CAPS_SUCCESS)  goto cleanup;

    status = caps_load(problemObj, "su2AIM", analysisPath, NULL, NULL, 1, &meshObj, &su2Obj);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_load(problemObj, "mystranAIM", analysisPath, NULL, NULL, 0, NULL, &mystranObj);
    if (status != CAPS_SUCCESS) goto cleanup;

    /* --------------------------------------------------------------- */

    // Create data bounds
    for (i = 0; i < 3; i++) {
        status = caps_makeBound(problemObj, 2, transfers[i], &boundObj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_makeVertexSet(boundObj[i], su2Obj, NULL, &vertexSU2Obj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_makeVertexSet(boundObj[i], mystranObj, NULL, &vertexMystranObj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_makeDataSet(vertexSU2Obj[i], "Pressure", Analysis, 1, &pressureSU2Obj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_makeDataSet(vertexMystranObj[i], "Pressure", Conserve, 1, &pressureMystranObj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_makeDataSet(vertexMystranObj[i], "Displacement", Analysis, 3, &displacementMystranObj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_makeDataSet(vertexSU2Obj[i], "Displacement", Conserve, 3, &displacementSU2Obj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_initDataSet(displacementSU2Obj[i], 3, displacement0);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = caps_completeBound(boundObj[i]);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    /* --------------------------------------------------------------- */

    // Set parameters for SU2
    status = setValueByName_String(su2Obj, ANALYSISIN, "Proj_Name", 1, 1, projectName);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = setValueByName_String(su2Obj, ANALYSISIN, "SU2_Version", 1, 1, "Falcon");
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal = refVelocity/speedofSound;
    status = setValueByName_Double(su2Obj, ANALYSISIN, "Mach", 1, 1, &doubleVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = setValueByName_String(su2Obj, ANALYSISIN, "Equation_Type", 1, 1, "compressible");
    if (status != CAPS_SUCCESS) goto cleanup;

    intVal = 5;
    status = setValueByName_Integer(su2Obj, ANALYSISIN, "Num_Iter", 1, 1, &intVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = setValueByName_String(su2Obj, ANALYSISIN, "Output_Format", 1, 1, "Tecplot");
    if (status != CAPS_SUCCESS) goto cleanup;

    boolVal = true;
    status = setValueByName_Boolean(su2Obj, ANALYSISIN, "Overwrite_CFG", 1, 1, &boolVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    doubleVal  = 0.5*refDensity*refVelocity*refVelocity;
    status = setValueByName_Double(su2Obj, ANALYSISIN, "Pressure_Scale_Factor", 1, 1, &doubleVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Find & set Boundary_Conditions for SU2
    su2BC = (capsTuple *) EG_alloc(numSU2BC*sizeof(capsTuple));
    su2BC[0].name = EG_strdup("Skin");
    su2BC[0].value = EG_strdup("{\"bcType\": \"Inviscid\"}");

    su2BC[1].name = EG_strdup("SymmPlane");
    su2BC[1].value =  EG_strdup("SymmetryY");

    su2BC[2].name = EG_strdup("Farfield");
    su2BC[2].value = EG_strdup("farfield");

    status = setValueByName_Tuple(su2Obj, ANALYSISIN, "Boundary_Condition", numSU2BC, 1, su2BC);
    if (status != CAPS_SUCCESS) goto cleanup;

    /* --------------------------------------------------------------- */

    // Set Mystran inputs - Materials
    material = (capsTuple *) EG_alloc(numMaterial*sizeof(capsTuple));
    material[0].name = EG_strdup("Madeupium");
    material[0].value = EG_strdup("{\"youngModulus\": 72.0E9, \"density\": 2.8E3}");

    status = setValueByName_Tuple(mystranObj, ANALYSISIN, "Material", numMaterial, 1, material);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                    - Properties
    property = (capsTuple *) EG_alloc(numProperty*sizeof(capsTuple));
    property[0].name = EG_strdup("Skin");
    property[0].value = EG_strdup("{\"propertyType\": \"Shell\", \"membraneThickness\": 0.05}");
    property[1].name = EG_strdup("Rib_Root");
    property[1].value = EG_strdup("{\"propertyType\": \"Shell\", \"membraneThickness\": 0.1}");

    status = setValueByName_Tuple(mystranObj, ANALYSISIN, "Property", numProperty, 1, property);
    if (status != CAPS_SUCCESS) goto cleanup;

    //                   - Constraints
    constraint = (capsTuple *) EG_alloc(numConstraint*sizeof(capsTuple));
    constraint[0].name = EG_strdup("edgeConstraint");
    constraint[0].value = EG_strdup("{\"groupName\": \"Rib_Root\", \"dofConstraint\": 123456}");

    status = setValueByName_Tuple(mystranObj, ANALYSISIN, "Constraint", numConstraint, 1, constraint);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = setValueByName_String(mystranObj, ANALYSISIN, "Proj_Name", 1, 1, projectName);
    if (status != CAPS_SUCCESS) goto cleanup;

    intVal = 3;
    status = setValueByName_Integer(mystranObj, ANALYSISIN, "Edge_Point_Min", 1, 1, &intVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    intVal = 10;
    status = setValueByName_Integer(mystranObj, ANALYSISIN, "Edge_Point_Max", 1, 1, &intVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    tessParams[0] = 1.5;
    tessParams[1] = 0.1;
    tessParams[2] = 0.15;
    status = setValueByName_Double(mystranObj, ANALYSISIN, "Tess_Params", 3, 1, tessParams);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = setValueByName_String(mystranObj, ANALYSISIN, "Analysis_Type", 1, 1, "Static");
    if (status != CAPS_SUCCESS) goto cleanup;

    boolVal = (int) false;
    status = setValueByName_Boolean(mystranObj, ANALYSISIN, "Quad_Mesh", 1, 1, &boolVal);
    if (status != CAPS_SUCCESS) goto cleanup;

    load = (capsTuple *) EG_alloc(numLoad*sizeof(capsTuple));
    load[0].name = EG_strdup("pressureAero");
    load[0].value = EG_strdup("{\"loadType\": \"PressureExternal\", \"loadScaleFactor\": -1.0}");

    status = setValueByName_Tuple(mystranObj, ANALYSISIN, "Load", numLoad, 1, load);
    if (status != CAPS_SUCCESS) goto cleanup;

    /* --------------------------------------------------------------- */

    // Do the analysis -- actually run TetGen
    status = caps_preAnalysis(meshObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Everything is done in preAnalysis, so we just do the post
    status = caps_postAnalysis(meshObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) goto cleanup;

    /* --------------------------------------------------------------- */

    // Populate the vertex sets in the bound objects after the grid generation
    for (i = 0; i < 3; i++) {
        status = caps_fillVertexSets(boundObj[i], &nErr, &errors);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Start iteration loop
    for (iter = 0; iter < numIteration; iter++) {

        /* --------------------------------------------------------------- */

        // Get data - Displacement
        // This defaults to values specified in caps_initDataSet on the first iteration
        printf("Transfer - Displacement\n");
        for (i = 0; i < 3; i++) {
            status = caps_getData(displacementSU2Obj[i], &npts, &rank, &data, &units);
            if (status != CAPS_SUCCESS) return status;
        }

#ifdef DEBUG
        /* Dump out vertex sets for debugging */
        for (i = 0; i < 3; i++) {
            sprintf(filename, "SU2_%d.vs", i);
            status = caps_outputVertexSet( vertexSU2Obj[i], filename );
            if (status != CAPS_SUCCESS) goto cleanup;
        }
#endif
        /* --------------------------------------------------------------- */

        // Do the pre-analysis  for SU2
        status = caps_preAnalysis(su2Obj, &nErr, &errors);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Execute SU2 via system call
        (void) getcwd(currentPath, PATH_MAX);

        if (chdir(analysisPath) != 0) {
            printf(" ERROR: Cannot change directory to -> %s\n", analysisPath);
            goto cleanup;
        }

        if (runAnalysis == (int) true) {

            printf("\n\nRunning su2!\n\n");

            if (iter > 0) system("SU2_DEF aeroelasticSimple_Iterative.cfg > su2DEFOut.txt");
            system("SU2_CFD aeroelasticSimple_Iterative.cfg > su2CFDOut.txt");
        } else {

            printf("\n\nNOT Running su2!\n\n");
        }

        (void) chdir(currentPath);

        // Run su2 post - to kick off data transfer
        status = caps_postAnalysis(su2Obj, current, &nErr, &errors);
        if (status != CAPS_SUCCESS) goto cleanup;

        /* --------------------------------------------------------------- */

        // Get data - Pressure
        printf("Transfer - Pressure\n");
        for (i = 0; i < 3; i++) {
            status = caps_getData(pressureMystranObj[i], &npts, &rank, &data, &units);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

#ifdef DEBUG
        /* Dump out vertex sets for debugging */
        for (i = 0; i < 3; i++) {
            sprintf(filename, "mystran_%d.vs", i);
            status = caps_outputVertexSet( vertexMystranObj[i], filename );
            if (status != CAPS_SUCCESS) goto cleanup;
        }
#endif

        /* --------------------------------------------------------------- */

        // Do the analysis  for Mystran
        status = caps_preAnalysis(mystranObj, &nErr, &errors);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Execute Mystran via system call
        (void) getcwd(currentPath, PATH_MAX);

        if (chdir(analysisPath) != 0) {
            printf(" ERROR: Cannot change directory to -> %s\n", analysisPath);
            goto cleanup;
        }

        if (runAnalysis == (int) true) {
            printf("\n\nRunning mystran!\n\n");
            system("mystran.exe aeroelasticSimple_Iterative.dat > mystranOut.txt");
        } else {
            printf("\n\nNOT Running mystran!\n\n");
        }

        (void) chdir(currentPath);

        // Run Mystran post
        status = caps_postAnalysis(mystranObj, current, &nErr, &errors);
        if (status != CAPS_SUCCESS) goto cleanup;

        /* --------------------------------------------------------------- */
    }

    status = CAPS_SUCCESS;

cleanup:
    if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n", status);

    freeTuple(numSU2BC, su2BC);
    freeTuple(numConstraint, constraint);
    freeTuple(numLoad, load);
    freeTuple(numProperty, property);
    freeTuple(numMaterial, material);

    (void) caps_close(problemObj);

    return status;
}
