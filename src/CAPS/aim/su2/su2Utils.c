#include <string.h>
#include <stdio.h>

#include "egads.h"     // Bring in egads utilss
#include "capsTypes.h" // Bring in CAPS types
#include "aimUtil.h"   // Bring in AIM utils
#include "miscUtils.h" // Bring in misc. utility functions
#include "meshUtils.h" // Bring in meshing utility functions
#include "cfdTypes.h"  // Bring in cfd specific types
#include "su2Utils.h"  // Bring in su2 utility header

#ifdef WIN32
#define strcasecmp  stricmp
#define strtok_r   strtok_s
#endif

// Extract flow variables from SU2 surface csv file (connectivity is ignored) - dataMatrix = [numVariable][numDataPoint]
int su2_readAeroLoad(char *filename, int *numVariable, char **variableName[], int *numDataPoint, double ***dataMatrix) {

    int status; // Function return
    int i, j; // Indexing

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder
    char *tempStr = NULL; // Temporary strings

    char comma;

    FILE *fp = NULL; // File pointer

    // Open file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        return CAPS_IOERR;
    }

    printf("Reading SU2 AeroLoad File - %s\n", filename);

    *numVariable = 0;
    *numDataPoint = 0;

    // Determine how many variables there are - get the first line
    status = getline(&line, &linecap, fp);
    if (status < 0) {
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Create a temp string to braket our line with '[' and ']'
    tempStr = (char *) EG_alloc((strlen(line) + 4)*sizeof(char));
    if (tempStr == NULL) return EGADS_MALLOC;

    strcpy(tempStr,"[");
    strncat(tempStr, line, strlen(line)-1);
    strcat(tempStr, "]");

    // Sort string into an array of strings
    status =  string_toStringDynamicArray(tempStr, numVariable, variableName);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Count the number of lines to determine number of data points
    while (getline(&line, &linecap, fp) >= 0) {
        *numDataPoint += 1;
    }

    // Go to the start of the file
    rewind(fp);

    printf("\tNumber of variables = %d\n", *numVariable);
    printf("\tNumber of data points = %d\n", *numDataPoint);

    // Re-Grab the first line
    status = getline(&line, &linecap, fp);
    if (status < 0) {
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // If nozero values were found
    if (*numVariable != 0 && *numDataPoint != 0) {

        // Allocate dataMatrix array
        //if (*dataMatrix != NULL) EG_free(*dataMatrix);

        *dataMatrix = (double **) EG_alloc(*numVariable *sizeof(double *));
        if (*dataMatrix == NULL) {
            status = EGADS_MALLOC; // If allocation failed ....
            goto cleanup;
        }

        for (i = 0; i < *numVariable; i++) {

            (*dataMatrix)[i] = (double *) EG_alloc(*numDataPoint*sizeof(double));

            if ((*dataMatrix)[i] == NULL) { // If allocation failed ....

                for (j = 0; j < i; j++) {

                    if ((*dataMatrix)[j] != NULL ) EG_free((*dataMatrix)[j]);
                }

                if ((*dataMatrix) != NULL) EG_free((*dataMatrix));
                *dataMatrix = NULL;

                status = EGADS_MALLOC;
                goto cleanup;
            }
        }

        // Loop through the file and fill up the data matrix
        for (j = 0; j < *numDataPoint; j++) {

            for (i = 0; i < *numVariable; i++) {
                fscanf(fp, "%lf", &(*dataMatrix)[i][j]);

                if (i != *numVariable-1) fscanf(fp, "%c", &comma);
            }
        }

        // Output the first two rows of the dataMatrix
        //for (i = 0; i < *numVariable; i++) printf("Variable %d - %.6f\n", i, (*dataMatrix)[i][0]);
        //for (i = 0; i < *numVariable; i++) printf("Variable %d - %.6f\n", i, (*dataMatrix)[i][1]);

    } else {

        printf("\tNo data values extracted from file - %s",filename);
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("\tError in su2_readAeroLoad, status = %d\n", status);

        if (fp != NULL) fclose(fp);

        if (tempStr != NULL) EG_free(tempStr);
        if (line != NULL) EG_free(line);

        if (status != CAPS_SUCCESS) {

            // Free up the dataMatrix
            if ((*dataMatrix) != NULL) {

                for (i = 0; i < *numVariable; i++) {
                    if ((*dataMatrix)[i] == NULL) EG_free((*dataMatrix)[i]);
                }

                EG_free((*dataMatrix));
                *dataMatrix = NULL;
            }

            // Free of variable array
            string_freeArray(*numVariable, variableName);

            *numVariable = 0;
            *numDataPoint = 0;
        }

        return status;
}


// Write SU2 surface motion file (connectivity is optional) - dataMatrix = [numVariable][numDataPoint], connectMatrix (optional) = [4*numConnect]
//  the formating of the data may be specified through dataFormat = [numVariable] (use capsTypes Integer and Double)- If NULL default to double
int su2_writeSurfaceMotion(char *filename,
                           int numVariable,
                           int numDataPoint,
                           double **dataMatrix,
                           int *dataFormat,
                           int numConnect,
                           int *connectMatrix) {

    int i, j; // Indexing

    FILE *fp; // File pointer


    printf("Writing SU2 Motion File - %s\n", filename);

    // Open file
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        return CAPS_IOERR;
    }

    if (connectMatrix == NULL) numConnect = 0;

    for (i = 0; i < numDataPoint; i++) {

        for (j = 0; j < numVariable; j++) {

            if (dataFormat != NULL) {

                if (dataFormat[j] == (int) Integer) {

                    fprintf(fp, "%d ", (int) dataMatrix[j][i]);

                } else if (dataFormat[j] == (int) Double) {

                    fprintf(fp, "%e ", dataMatrix[j][i]);

                } else {

                    printf("Unrecognized data format requested - %d", (int) dataFormat[j]);
                    fclose(fp);
                    return CAPS_BADVALUE;
                }

            } else {

                fprintf(fp, "%e ", dataMatrix[j][i]);
            }
        }

        fprintf(fp, "\n");
    }

    if (connectMatrix != NULL) {

        for (i = 0; i < numConnect; i++ ) {

            for (j = 0; j < 4; j++) {
                fprintf(fp, "%d ", connectMatrix[4*i+j]);
            }

            fprintf(fp, "\n");
        }
    }

    fclose(fp);

    return CAPS_SUCCESS;
}

// Write SU2 data transfer files
int su2_dataTransfer(void *aimInfo,
                     const char *analysisPath,
                     char *projectName,
                     meshStruct volumeMesh) {

    /*! \page dataTransferSU2 SU2 Data Transfer
     *
     * \section dataToSU2 Data transfer to SU2
     *
     * <ul>
     *  <li> <B>"Displacement"</B> </li> <br>
     *   Retrieves nodal displacements (as from a structural solver)
     *   and updates SU2's surface mesh; a new [project_name]_motion.dat file is written out which may
     *   be loaded into SU2 to update the surface mesh/move the volume mesh.
     * </ul>
     */

    int status; // Function return status
    int i, j, k; // Indexing

    int stringLength = 0;

    char *filename = NULL;

    // Discrete data transfer variables
    capsDiscr *dataTransferDiscreteObj;
    char **transferName = NULL;
    int numTransferName = 0;
    enum capsdMethod dataTransferMethod;
    int numDataTransferPoint;
    //int numDataTransferElement = 0;
    int dataTransferRank;
    double *dataTransferData;

    // Variables used in global node mapping
    int *nodeMap, *storage;
    int globalNodeID;

    // Data transfer Out variables

    double **dataOutMatrix = NULL;
    int *dataOutFormat = NULL;
    int *dataConnectMatrix = NULL;

    int numOutVariable = 7;
    int numOutDataPoint = 0;
    int numOutDataConnect = 0;

    char fileExt[] = "_motion.dat";

    int foundDisplacement = (int) false;

    meshStruct surfaceMesh;

    status = aim_getBounds(aimInfo, &numTransferName, &transferName);
    if (status != CAPS_SUCCESS) return status;

    (void) initiate_meshStruct(&surfaceMesh);

    foundDisplacement = (int) false;
    for (i = 0; i < numTransferName; i++) {

        status = aim_getDiscr(aimInfo, transferName[i], &dataTransferDiscreteObj);
        if (status != CAPS_SUCCESS) continue;

        status = aim_getDataSet(dataTransferDiscreteObj,
                                "Displacement",
                                &dataTransferMethod,
                                &numDataTransferPoint,
                                &dataTransferRank,
                                &dataTransferData);

        if (status == CAPS_SUCCESS) { // If we do have data ready is the rank correct

            foundDisplacement = (int) true;

            if (dataTransferRank != 3) {
                printf("Displacement transfer data found however rank is %d not 3!!!!\n", dataTransferRank);
                status = CAPS_BADRANK;
                goto cleanup;
            }

            break;
        }

    } // Loop through transfer names


    if (foundDisplacement != (int) true ) {

        printf("No recognized data transfer names found!\n");

        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Ok looks like we have displacements to get so lets continue
    printf("Writing SU2 data transfer files\n");

    // Combine surface meshes found in the volumes
    status = mesh_combineMeshStruct(volumeMesh.numReferenceMesh,
                                    volumeMesh.referenceMesh,
                                    &surfaceMesh);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Right now we are just going to output a body containing all surface nodes
    numOutDataPoint = surfaceMesh.numNode;
    numOutDataConnect = surfaceMesh.numElement;

    // Allocate data arrays that are going to be output
    dataOutMatrix = (double **) EG_alloc(numOutVariable*sizeof(double));
    dataOutFormat = (int *) EG_alloc(numOutVariable*sizeof(int));
    dataConnectMatrix = (int *) EG_alloc(4*numOutDataConnect*sizeof(int));

    if (dataOutMatrix == NULL || dataOutFormat == NULL || dataConnectMatrix == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    for (i = 0; i < numOutVariable; i++) dataOutMatrix[i] = NULL;

    for (i = 0; i < numOutVariable; i++) {

        dataOutMatrix[i] = (double *) EG_alloc(numOutDataPoint*sizeof(double));

        if (dataOutMatrix[i] == NULL) { // If allocation failed ....

            status =  EGADS_MALLOC;
            goto cleanup;
        }
    }

    // Set data out formatting
    for (i = 0; i < numOutVariable; i++) {
        if (i == 0) {
            dataOutFormat[i] = (int) Integer;
        } else {
            dataOutFormat[i] = (int) Double;
        }
    }

    // Fill data out matrix with current surface mesh and global id
    for (i = 0; i < numOutDataPoint; i++ ) {

        // Global ID - assumes the surfaceMesh nodes are in the volume at the beginning
        dataOutMatrix[0][i] = (double) surfaceMesh.node[i].nodeID;

        // Coordinates
        dataOutMatrix[1][i] = surfaceMesh.node[i].xyz[0];
        dataOutMatrix[2][i] = surfaceMesh.node[i].xyz[1];
        dataOutMatrix[3][i] = surfaceMesh.node[i].xyz[2];

        // Delta displacements
        dataOutMatrix[4][i] = 0;
        dataOutMatrix[5][i] = 0;
        dataOutMatrix[6][i] = 0;
    }

/*
    for (i = 0; i < numOutDataConnect; i++) {

        if (surfaceMesh.element[i].elementType == Triangle) {
            dataConnectMatrix[4*i+ 0] = surfaceMesh.element[i].connectivity[0];
            dataConnectMatrix[4*i+ 1] = surfaceMesh.element[i].connectivity[1];
            dataConnectMatrix[4*i+ 2] = surfaceMesh.element[i].connectivity[2];
            dataConnectMatrix[4*i+ 3] = surfaceMesh.element[i].connectivity[2];
        }

        if (surfaceMesh.element[i].elementType == Quadrilateral) {
            dataConnectMatrix[4*i+ 0] = surfaceMesh.element[i].connectivity[0];
            dataConnectMatrix[4*i+ 1] = surfaceMesh.element[i].connectivity[1];
            dataConnectMatrix[4*i+ 2] = surfaceMesh.element[i].connectivity[2];
            dataConnectMatrix[4*i+ 3] = surfaceMesh.element[i].connectivity[3];
        }
    }

*/

    // Re-loop through transfers - if we are doing displacements
    if (foundDisplacement == (int) true) {

        for (i = 0; i < numTransferName; i++) {

            status = aim_getDiscr(aimInfo, transferName[i], &dataTransferDiscreteObj);
            if (status != CAPS_SUCCESS) continue;

            status = aim_getDataSet(dataTransferDiscreteObj,
                                    "Displacement",
                                    &dataTransferMethod,
                                    &numDataTransferPoint,
                                    &dataTransferRank,
                                    &dataTransferData);
            if (status != CAPS_SUCCESS) continue; // If no elements in this object skip to next transfer name

            //Get extra node information stored in the discrObj
            storage = (int *) dataTransferDiscreteObj->ptrm;
            nodeMap = storage; // Global indexing on the body

            // A single point means this is an initialization phase
            if (numDataTransferPoint == 1) {
                for (k = 0; k < numOutDataPoint; k++) {
                    dataOutMatrix[4][k] = dataTransferData[0];
                    dataOutMatrix[5][k] = dataTransferData[1];
                    dataOutMatrix[6][k] = dataTransferData[2];
                }
            }
            else {
                for (j = 0; j < numDataTransferPoint; j++) {

                    globalNodeID = nodeMap[j];
                    for (k = 0; k < numOutDataPoint; k++) {

                        // If the global node IDs match store the displacement values in the dataOutMatrix
                        if (globalNodeID  == (int) dataOutMatrix[0][k]) {

                            // A rank of 3 should have already been checked
                            // Delta displacements
                            dataOutMatrix[4][k] = dataTransferData[3*j+0];
                            dataOutMatrix[5][k] = dataTransferData[3*j+1];
                            dataOutMatrix[6][k] = dataTransferData[3*j+2];
                            break;
                        }
                    }
                }
            }
        } // End dataTransferDiscreteObj loop

        // Update surface coordinates based on displacements and decrement grid IDs
        for (i = 0; i < numOutDataPoint; i++ ) {

            dataOutMatrix[0][i] -= 1; // SU2 wants grid ids to start at 0 !!!!

            // Coordinates + displacements at nodes
            dataOutMatrix[1][i] = dataOutMatrix[1][i] + dataOutMatrix[4][i]; // x
            dataOutMatrix[2][i] = dataOutMatrix[2][i] + dataOutMatrix[5][i]; // y
            dataOutMatrix[3][i] = dataOutMatrix[3][i] + dataOutMatrix[6][i]; // z

        }

        stringLength = strlen(projectName) + strlen(analysisPath) + strlen(fileExt) + 1;
        filename = (char *) EG_alloc((stringLength +1)*sizeof(char));
        if (filename == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        strcpy(filename, analysisPath);
        #ifdef WIN32
        strcat(filename, "\\");
        #else
        strcat(filename, "/");
        #endif
        strcat(filename, projectName);
        strcat(filename, fileExt);

        // Write out displacement in tecplot file
        status = su2_writeSurfaceMotion(filename,
                                        4, // Only want the Global id and coordinates
                                        numOutDataPoint,
                                        dataOutMatrix,
                                        dataOutFormat,
                                        0, // numConnectivity
                                        NULL); // connectivity matrix
        if (status != CAPS_SUCCESS) goto cleanup;
    } // End if found displacements

    status = CAPS_SUCCESS;

    goto cleanup;

    // Clean-up
    cleanup:

        (void) destroy_meshStruct(&surfaceMesh);

        if (dataOutMatrix != NULL) {
            for (i = 0; i < numOutVariable; i++) {
                if (dataOutMatrix[i] != NULL)  EG_free(dataOutMatrix[i]);
            }
        }

        if (dataOutMatrix != NULL) EG_free(dataOutMatrix);
        if (dataOutFormat != NULL) EG_free(dataOutFormat);
        if (dataConnectMatrix != NULL) EG_free(dataConnectMatrix);

        if (filename != NULL) EG_free(filename);

        if (transferName != NULL) EG_free(transferName);

        return status;

}

// Extract the boundary condition names that should added to the marker
int su2_marker(void *aimInfo, const char* iname, capsValue *aimInputs, FILE *fp, cfdBCsStruct bcProps) {

    int status = CAPS_SUCCESS;
    int counter = 0;
    int nmarker = 0;

    int i, j; // indexing

    char **markers = NULL;
    char *fullstr = NULL, *rest = NULL, *token = NULL;

    // might not be anything to write in the list
    if (aimInputs[aim_getIndex(aimInfo, iname, ANALYSISIN)-1].nullVal == IsNull) {
        fprintf(fp," NONE )\n");
        return CAPS_SUCCESS;
    }

    fullstr = EG_strdup(aimInputs[aim_getIndex(aimInfo, iname, ANALYSISIN)-1].vals.string);

    // Remove any possible brackets and separators
    while ((token = strstr(fullstr, "["))) *token = ' ';
    while ((token = strstr(fullstr, "]"))) *token = ' ';
    while ((token = strstr(fullstr, "("))) *token = ' ';
    while ((token = strstr(fullstr, ")"))) *token = ' ';
    while ((token = strstr(fullstr, ","))) *token = ' ';
    while ((token = strstr(fullstr, ";"))) *token = ' ';
    while ((token = strstr(fullstr, ":"))) *token = ' ';

    rest = fullstr;
    while ((token = strtok_r(rest, " ", &rest))) {

        markers = (char**)EG_reall(markers, (nmarker+1)*sizeof(char*));
        if ( markers == NULL ) {
            nmarker = 0;
            status = EGADS_MALLOC;
            goto cleanup;
        }

        markers[nmarker] = (char*)EG_alloc((strlen(token)+1)*sizeof(char));
        if ( markers[nmarker] == NULL ) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        sscanf(token, "%s", markers[nmarker]);
        nmarker++;
    }

    if (nmarker == 0) {
        printf("********************************************\n");
        printf("\n");
        printf("ERROR: Could not find any names in string:\n\n");
        printf("\t%s='%s'\n",
               iname, aimInputs[aim_getIndex(aimInfo, "Surface_Monitor", ANALYSISIN)-1].vals.string);
        printf("\n");
        printf("********************************************\n");
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    counter = 0;
    for (j = 0; j < nmarker; j++) {
        for (i = 0; i < bcProps.numBCID ; i++) {
            if (strcmp(bcProps.surfaceProps[i].name, markers[j]) == 0) {

                if (counter > 0) fprintf(fp, ",");
                fprintf(fp," %d", bcProps.surfaceProps[i].bcID);

                counter += 1;
                break;
            }
        }
    }
    fprintf(fp," )\n");

    if (counter != nmarker) {
        printf("********************************************\n");
        printf("\n");
        printf("ERROR: Could not find all '%s' names:\n\n", iname);
        for (j = 0; j < nmarker; j++)
            printf("\t%s\n", markers[j]);
        printf("\n");

        printf("in the list of boundary condition names:\n\n");
        for (i = 0; i < bcProps.numBCID ; i++)
            printf("\t%s\n", bcProps.surfaceProps[i].name);
        printf("\n");
        printf("********************************************\n");

        status = CAPS_NOTFOUND;
    }


cleanup:
    // free the temporary string
    EG_free(fullstr);

    // free the monitoring strings
    for (j = 0; j < nmarker; j++)
        EG_free(markers[j]);
    EG_free(markers);

    return status;
}

#ifdef DEFINED_BUT_NOT_USED
// Use in the future to just the information to carry out the deform.
static int su2_writeConfig_Deform(void *aimInfo, char *analysisPath, capsValue *aimInputs, cfdBCsStruct bcProps) {
    int status; // Function return status

    int i; // Indexing

    int stringLength;

    // For SU2 boundary tagging
    int counter;

    char *filename = NULL;
    FILE *fp = NULL;
    char fileExt[] = "_Deform.cfg";

    printf("Write SU2 configuration file for grid deformation\n");
    stringLength = strlen(analysisPath)
                   + 1
                   + strlen(aimInputs[aim_getIndex(aimInfo, "Proj_Name",  ANALYSISIN)-1].vals.string)
                   + strlen(fileExt);

    filename = (char *) EG_alloc((stringLength +1)*sizeof(char));
    if (filename == NULL) {
        status =  EGADS_MALLOC;
        goto cleanup;
    }

    strcpy(filename, analysisPath);
    #ifdef WIN32
        strcat(filename, "\\");
    #else
        strcat(filename, "/");
    #endif
    strcat(filename, aimInputs[aim_getIndex(aimInfo, "Proj_Name",  ANALYSISIN)-1].vals.string);
    strcat(filename, fileExt);

    fp = fopen(filename,"w");
    if (fp == NULL) {
        status =  CAPS_IOERR;
        goto cleanup;
    }

    fprintf(fp,"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
    fprintf(fp,"%%                                                                              %%\n");
    fprintf(fp,"%% SU2 configuration file - for Grid Deformation                                %%\n");
    fprintf(fp,"%% Created by SU2AIM for Project: \"%s\"\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name",  ANALYSISIN)-1].vals.string);
    fprintf(fp,"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n");

    fprintf(fp,"%% ----------------------- DYNAMIC MESH DEFINITION -----------------------------%%\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Type of dynamic mesh (NONE, RIGID_MOTION, DEFORMING, ROTATING_FRAME,\n");
    fprintf(fp,"%%                       MOVING_WALL, STEADY_TRANSLATION, FLUID_STRUCTURE,\n");
    fprintf(fp,"%%                       AEROELASTIC, ELASTICITY, EXTERNAL,\n");
    fprintf(fp,"%%                       AEROELASTIC_RIGID_MOTION, GUST)\n");
    fprintf(fp,"GRID_MOVEMENT_KIND= DEFORMING\n");

    fprintf(fp,"%% Moving wall boundary marker(s) (NONE = no marker, ignored for RIGID_MOTION)\n");

    fprintf(fp,"MARKER_MOVING= (" );
    counter = 0;
    for (i = 0; i < bcProps.numBCID ; i++) {
        if (bcProps.surfaceProps[i].surfaceType == Inviscid ||
            bcProps.surfaceProps[i].surfaceType == Viscous) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d", bcProps.surfaceProps[i].bcID);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%% ----------------------- DESIGN VARIABLE PARAMETERS --------------------------%%\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Kind of deformation (NO_DEFORMATION, TRANSLATION, ROTATION, SCALE,\n");
    fprintf(fp,"%%                      FFD_SETTING, FFD_NACELLE\n");
    fprintf(fp,"%%                      FFD_CONTROL_POINT, FFD_CAMBER, FFD_THICKNESS, FFD_TWIST\n");
    fprintf(fp,"%%                      FFD_CONTROL_POINT_2D, FFD_CAMBER_2D, FFD_THICKNESS_2D, FFD_TWIST_2D,\n");
    fprintf(fp,"%%                      HICKS_HENNE, SURFACE_BUMP)\n");
    fprintf(fp,"DV_KIND= SURFACE_FILE \n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Marker of the surface in which we are going apply the shape deformation\n");

    fprintf(fp,"DV_MARKER= (");
    counter = 0;
    for (i = 0; i < bcProps.numBCID ; i++) {
        if (bcProps.surfaceProps[i].surfaceType == Inviscid ||
            bcProps.surfaceProps[i].surfaceType == Viscous) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d", bcProps.surfaceProps[i].bcID);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    //fprintf(fp,"DV_MARKER= ( airfoil )\n");
    fprintf(fp,"MOTION_FILENAME=%s_motion.dat\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name",  ANALYSISIN)-1].vals.string);
    fprintf(fp,"\n");

    fprintf(fp,"%% ------------------------- INPUT/OUTPUT INFORMATION --------------------------%%\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Mesh input file\n");
    fprintf(fp,"MESH_FILENAME= %s.su2\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name",  ANALYSISIN)-1].vals.string);
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Mesh input file format (SU2, CGNS)\n");
    fprintf(fp,"MESH_FORMAT= SU2\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Mesh output file\n");
    fprintf(fp,"MESH_OUT_FILENAME= %s.su2\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name",  ANALYSISIN)-1].vals.string);
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Output file format (TECPLOT, TECPLOT_BINARY, PARAVIEW,\n");
    fprintf(fp,"%%                     FIELDVIEW, FIELDVIEW_BINARY)\n");
    string_toUpperCase(aimInputs[aim_getIndex(aimInfo, "Output_Format",  ANALYSISIN)-1].vals.string);
    fprintf(fp,"OUTPUT_FORMAT= %s\n", aimInputs[aim_getIndex(aimInfo, "Output_Format",  ANALYSISIN)-1].vals.string);
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Verbosity of console output: NONE removes minor MPI overhead (NONE, HIGH)\n");
    fprintf(fp,"CONSOLE_OUTPUT_VERBOSITY= HIGH\n");


    fprintf(fp,"%% -------------------- BOUNDARY CONDITION DEFINITION --------------------------%%\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Euler wall boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_EULER= (" );

    counter = 0; // Euler boundary
    for (i = 0; i < bcProps.numBCID; i++) {

        if (bcProps.surfaceProps[i].surfaceType == Inviscid) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d", bcProps.surfaceProps[i].bcID);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%%\n");
    fprintf(fp,"%% Navier-Stokes (no-slip), constant heat flux wall  marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: ( marker name, constant heat flux (J/m^2), ... )\n");
    fprintf(fp,"MARKER_HEATFLUX= (");

    counter = 0; // Viscous boundary w/ heat flux
    for (i = 0; i < bcProps.numBCID; i++) {
        if (bcProps.surfaceProps[i].surfaceType == Viscous &&
            bcProps.surfaceProps[i].wallTemperatureFlag == (int) true &&
            bcProps.surfaceProps[i].wallTemperature < 0) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d, %f", bcProps.surfaceProps[i].bcID, bcProps.surfaceProps[i].wallHeatFlux);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%%\n");
    fprintf(fp,"%% Navier-Stokes (no-slip), isothermal wall marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: ( marker name, constant wall temperature (K), ... )\n");
    fprintf(fp,"MARKER_ISOTHERMAL= (");

    counter = 0; // Viscous boundary w/ isothermal wall
    for (i = 0; i < bcProps.numBCID; i++) {
        if (bcProps.surfaceProps[i].surfaceType == Viscous &&
            bcProps.surfaceProps[i].wallTemperatureFlag == (int) true &&
            bcProps.surfaceProps[i].wallTemperature >= 0) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d, %f", bcProps.surfaceProps[i].bcID, bcProps.surfaceProps[i].wallTemperature);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%%\n");
    fprintf(fp,"%% Far-field boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_FAR= (" );

    counter = 0; // Farfield boundary
    for (i = 0; i < bcProps.numBCID; i++) {
        if (bcProps.surfaceProps[i].surfaceType == Farfield) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d", bcProps.surfaceProps[i].bcID);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%%\n");
    fprintf(fp,"%% Symmetry boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_SYM= (" );

    counter = 0; // Symmetry boundary
    for (i = 0; i < bcProps.numBCID; i++) {
        if (bcProps.surfaceProps[i].surfaceType == Symmetry) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d", bcProps.surfaceProps[i].bcID);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%%\n");
    fprintf(fp,"%% Near-Field boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_NEARFIELD= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Zone interface boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_INTERFACE= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Actuator disk boundary type (VARIABLES_JUMP, NET_THRUST, BC_THRUST,\n");
    fprintf(fp,"%%                              DRAG_MINUS_THRUST, MASSFLOW, POWER)\n");
    fprintf(fp,"ACTDISK_TYPE= VARIABLES_JUMP\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Actuator disk boundary marker(s) with the following formats (NONE = no marker)\n");
    fprintf(fp,"%% Variables Jump: ( inlet face marker, outlet face marker,\n");
    fprintf(fp,"%%                   Takeoff pressure jump (psf), Takeoff temperature jump (R), Takeoff rev/min,\n");
    fprintf(fp,"%%                   Cruise  pressure jump (psf), Cruise temperature jump (R), Cruise rev/min )\n");
    fprintf(fp,"%% Net Thrust: ( inlet face marker, outlet face marker,\n");
    fprintf(fp,"%%               Takeoff net thrust (lbs), 0.0, Takeoff rev/min,\n");
    fprintf(fp,"%%               Cruise net thrust (lbs), 0.0, Cruise rev/min )\n");
    fprintf(fp,"%%BC Thrust: ( inlet face marker, outlet face marker,\n");
    fprintf(fp,"%%             Takeoff BC thrust (lbs), 0.0, Takeoff rev/min,\n");
    fprintf(fp,"%%             Cruise BC thrust (lbs), 0.0, Cruise rev/min )\n");
    fprintf(fp,"%%Drag-Thrust: ( inlet face marker, outlet face marker,\n");
    fprintf(fp,"%%               Takeoff Drag-Thrust (lbs), 0.0, Takeoff rev/min,\n");
    fprintf(fp,"%%               Cruise Drag-Thrust (lbs), 0.0, Cruise rev/min )\n");
    fprintf(fp,"%%MasssFlow: ( inlet face marker, outlet face marker,\n");
    fprintf(fp,"%%               Takeoff massflow (lbs/s), 0.0, Takeoff rev/min,\n");
    fprintf(fp,"%%               Cruise massflowt (lbs/s), 0.0, Cruise rev/min )\n");
    fprintf(fp,"%%Power: ( inlet face marker, outlet face marker,\n");
    fprintf(fp,"%%          Takeoff power (HP), 0.0, Takeoff rev/min\n");
    fprintf(fp,"%%          Cruise power (HP), 0.0, Cruise rev/min )\n");
    fprintf(fp,"MARKER_ACTDISK= ( NONE )\n");
    fprintf(fp,"%%\n");

    fprintf(fp,"%% Inlet boundary type (TOTAL_CONDITIONS, MASS_FLOW)\n");
    fprintf(fp,"INLET_TYPE= TOTAL_CONDITIONS\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Inlet boundary marker(s) with the following formats (NONE = no marker) \n");
    fprintf(fp,"%% Total Conditions: (inlet marker, total temp, total pressure, flow_direction_x, \n");
    fprintf(fp,"%%           flow_direction_y, flow_direction_z, ... ) where flow_direction is\n");
    fprintf(fp,"%%           a unit vector.\n");
    fprintf(fp,"%% Mass Flow: (inlet marker, density, velocity magnitude, flow_direction_x, \n");
    fprintf(fp,"%%           flow_direction_y, flow_direction_z, ... ) where flow_direction is\n");
    fprintf(fp,"%%           a unit vector.\n");
    fprintf(fp,"%% Incompressible: (inlet marker, NULL, velocity magnitude, flow_direction_x,\n");
    fprintf(fp,"%%           flow_direction_y, flow_direction_z, ... ) where flow_direction is\n");
    fprintf(fp,"%%           a unit vector.\n");
    fprintf(fp,"MARKER_INLET= ( ");

    counter = 0; // Subsonic Inflow
    for (i = 0; i < bcProps.numBCID ; i++) {
        if (bcProps.surfaceProps[i].surfaceType == SubsonicInflow) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d, %f, %f, %f, %f, %f", bcProps.surfaceProps[i].bcID,
                                                  bcProps.surfaceProps[i].totalTemperature,
                                                  bcProps.surfaceProps[i].totalPressure,
                                                  bcProps.surfaceProps[i].uVelocity,
                                                  bcProps.surfaceProps[i].vVelocity,
                                                  bcProps.surfaceProps[i].wVelocity);
            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%%\n");
    fprintf(fp,"%% Supersonic inlet boundary marker(s) (NONE = no marker) \n");
    fprintf(fp,"%% Format: (inlet marker, temperature, static pressure, velocity_x, \n");
    fprintf(fp,"%%           velocity_y, velocity_z, ... ), i.e. primitive variables specified.\n");
    fprintf(fp,"MARKER_SUPERSONIC_INLET= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Outlet boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: ( outlet marker, back pressure (static), ... )\n");
    fprintf(fp,"MARKER_OUTLET= ( ");

    counter = 0; // Outlet boundary
    for (i = 0; i < bcProps.numBCID ; i++) {
        if (bcProps.surfaceProps[i].surfaceType == BackPressure ||
            bcProps.surfaceProps[i].surfaceType == SubsonicOutflow) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d, %f", bcProps.surfaceProps[i].bcID,
                                  bcProps.surfaceProps[i].staticPressure);

            counter += 1;
        }
    }

    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    fprintf(fp,"%%\n");
    fprintf(fp,"%% Supersonic outlet boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_SUPERSONIC_OUTLET= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Periodic boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: ( periodic marker, donor marker, rotation_center_x, rotation_center_y, \n");
    fprintf(fp,"%% rotation_center_z, rotation_angle_x-axis, rotation_angle_y-axis, \n");
    fprintf(fp,"%% rotation_angle_z-axis, translation_x, translation_y, translation_z, ... )\n");
    fprintf(fp,"MARKER_PERIODIC= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Engine inflow boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: (engine inflow marker, fan face Mach, ... )\n");
    fprintf(fp,"MARKER_ENGINE_INFLOW= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Engine exhaust boundary marker(s) with the following formats (NONE = no marker) \n");
    fprintf(fp,"%% Format: (engine exhaust marker, total nozzle temp, total nozzle pressure, ... )\n");
    fprintf(fp,"MARKER_ENGINE_EXHAUST= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Displacement boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: ( displacement marker, displacement value normal to the surface, ... )\n");
    fprintf(fp,"MARKER_NORMAL_DISPL= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Load boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: ( load marker, force value normal to the surface, ... )\n");
    fprintf(fp,"MARKER_NORMAL_LOAD= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Pressure boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: ( pressure marker )\n");
    fprintf(fp,"MARKER_PRESSURE= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Neumann bounday marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_NEUMANN= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Dirichlet boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"MARKER_DIRICHLET= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Riemann boundary marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: (marker, data kind flag, list of data)\n");
    fprintf(fp,"MARKER_RIEMANN= ( NONE )\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Non Reflecting boundary conditions marker(s) (NONE = no marker)\n");
    fprintf(fp,"%% Format: (marker, data kind flag, list of data)\n");
    fprintf(fp,"MARKER_NRBC= ( NONE )\n");


    fprintf(fp,"%% ------------------------ SURFACES IDENTIFICATION ----------------------------%%\n");
    fprintf(fp,"%%\n");
    fprintf(fp,"%% Marker(s) of the surface in the surface flow solution file\n");
    fprintf(fp,"MARKER_PLOTTING= (" );

    counter = 0; // Surface marker
    for (i = 0; i < bcProps.numBCID ; i++) {
        if (bcProps.surfaceProps[i].surfaceType == Inviscid ||
            bcProps.surfaceProps[i].surfaceType == Viscous) {

            if (counter > 0) fprintf(fp, ",");
            fprintf(fp," %d", bcProps.surfaceProps[i].bcID);

            counter += 1;
        }
    }
    if(counter == 0) fprintf(fp," NONE");
    fprintf(fp," )\n");

    // write monitoring information
    status = su2_monitor(aimInfo, aimInputs, fp, bcProps);
    if (status != CAPS_SUCCESS) goto cleanup;

    fprintf(fp,"%%\n");
    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:

        if (filename != NULL) EG_free(filename);
        if (fp != NULL) fclose(fp);

        return status;
}
#endif
