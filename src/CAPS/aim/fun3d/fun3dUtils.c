#include <string.h>
#include <stdio.h>


#include "egads.h"        // Bring in egads utilss
#include "capsTypes.h"    // Bring in CAPS types
#include "aimUtil.h"      // Bring in AIM utils
#include "miscUtils.h"    // Bring in misc. utility functions
#include "meshUtils.h"    // Bring in meshing utility functions
#include "cfdTypes.h"     // Bring in cfd specific types
#include "tecplotUtils.h" // Bring in tecplot utility functions
#include "fun3dUtils.h"   // Bring in fun3d utility header


#include "errno.h"

#ifdef WIN32
#define strcasecmp  stricmp

// For _mkdir
#include "direct.h"

#else

// For mkdir
#include "sys/stat.h"

#endif


// Extract the FEPOINT Tecoplot data from a FUN3D Aero-Loads file (connectivity is ignored) - dataMatrix = [numVariable][numDataPoint]
int fun3d_readAeroLoad(char *filename, int *numVariable, char **variableName[], int *numDataPoint, double ***dataMatrix) {


    int status; // Function return
    int i, j; // Indexing

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder
    char *tempStr = NULL, *tempStr2 = NULL; // Temporary strings
    int stringLen = 0; // Length of string holder

    FILE *fp = NULL; // File pointer

    // Open file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        return CAPS_IOERR;
    }

    printf("Reading FUN3D AeroLoad File - %s!!!!!!\n", filename);

    *numVariable = 0;
    *numDataPoint = 0;
    // Loop through file line by line until we have determined how many variables and data points there are
    while (*numVariable == 0 || *numDataPoint == 0) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;

        // Get variable list if available in file line
        if (strncmp("variables=", line, strlen("variables=")) == 0) {

            // Pull out substring at first occurrence of "
            tempStr = strstr(line, "\"");

            // Create a temperory string of the variables in a the folling format - ["a","ae"]
            stringLen  = strlen(tempStr)-1 + 2;

            tempStr2 = (char *) EG_alloc((stringLen +1)*sizeof(char *));
            if (tempStr2 == NULL) {
                fclose(fp);
                if (line != NULL) EG_free(line);
                return EGADS_MALLOC;
            }

            tempStr2[0] = '[';
            strncpy(tempStr2+1, tempStr, strlen(tempStr)-1);
            tempStr2[stringLen-1] = ']';
            tempStr2[stringLen] = '\0';

            // Sort string into an array of strings
            status =  string_toStringDynamicArray(tempStr2, numVariable, variableName);

            if (tempStr2 != NULL) EG_free(tempStr2);
            tempStr2 = NULL;

            if (status != CAPS_SUCCESS) return status;

            // Print out list of variables found in load file
            printf("Variables found in file %s:\n",filename);
            for (i = 0; i < *numVariable; i++) printf("Variable %d = %s\n", i, (*variableName)[i]);
        }

        // Get the number of data points in file if available in file line
        if (strncmp("zone t=", line, strlen("zone t=")) == 0) {

            // Pull out substring at first occurrence of i=
            tempStr = strstr(line, "i=");

            // Retrieve the i= value
            sscanf(&tempStr[2], "%d", numDataPoint);

            // Print out the number of data points found in load file
            printf("Number of data points = %d, in file %s\n", *numDataPoint, filename);
        }

    }

    if (*numVariable != 0 && *numDataPoint != 0) {

        // Allocate dataMatrix array
        if (*dataMatrix != NULL) EG_free(*dataMatrix);

        *dataMatrix = (double **) EG_alloc(*numVariable *sizeof(double *));
        if (*dataMatrix == NULL) return EGADS_MALLOC; // If allocation failed ....

        for (i = 0; i < *numVariable; i++) {

            (*dataMatrix)[i] = (double *) EG_alloc(*numDataPoint*sizeof(double));

            if ((*dataMatrix)[i] == NULL) { // If allocation failed ....
                for (j = 0; j < *numVariable; j++) {

                    if ((*dataMatrix)[j] != NULL ) EG_free((*dataMatrix)[j]);
                }

                if ((*dataMatrix) != NULL) EG_free((*dataMatrix));
                fclose(fp);
                if (line != NULL) EG_free(line);
                return EGADS_MALLOC;
            }
        }

        // Loop through the file and fill up the data matrix
        for (j = 0; j < *numDataPoint; j++) {
            for (i = 0; i < *numVariable; i++) {
                fscanf(fp, "%lf", &(*dataMatrix)[i][j]);
            }
        }

        // Output the first row of the dataMatrix
        //for (i = 0; i < *numVariable; i++) printf("Variable %d - %.6f\n", i, (*dataMatrix)[i][0]);

    } else {

        printf("No data values extracted from file - %s",filename);
        fclose(fp);
        if (line != NULL) EG_free(line);
        return CAPS_BADVALUE;
    }

    fclose(fp);

    if (line != NULL) EG_free(line);
    return CAPS_SUCCESS;
}

// Create a 3D mesh for FUN3D from a 2D mesh
int fun3d_2DMesh(meshStruct *surfaceMesh, mapAttrToIndexStruct *attrMap, meshStruct *volumeMesh, int *extrusionBCIndex) {

    int status; // Function return status

    int i; // Indexing

    int faceBCIndex = CAPSMAGIC;

    double extrusion = -1.0; // Extrusion length

    // Flip coordinates
    int xMeshConstant = (int) true, yMeshConstant = (int) true, zMeshConstant= (int) true; // 2D mesh checks
    double tempCoord;

    cfdMeshDataStruct *cfdData;
    int elementIndex;
    int marker;

    if (surfaceMesh->meshQuickRef.useStartIndex == (int) false &&
        surfaceMesh->meshQuickRef.useListIndex  == (int) false) {

        status = mesh_fillQuickRefList(surfaceMesh);
        if (status != CAPS_SUCCESS) goto bail;
    }

    // add boundary elements if they are missing
    if (surfaceMesh->meshQuickRef.numLine == 0) {
        status = mesh_addTess2Dbc(surfaceMesh, attrMap);
        if (status != CAPS_SUCCESS) goto bail;
    }

    // Check to make sure the surface has a consistent boundary index
    for (i = 0; i < surfaceMesh->numElement; i++) {

        if (surfaceMesh->element[i].elementType != Triangle &&
            surfaceMesh->element[i].elementType != Quadrilateral) {
            continue;
        }

        if (surfaceMesh->element[i].analysisType == MeshCFD) {
            cfdData = (cfdMeshDataStruct *) surfaceMesh->element[i].analysisData;
            marker = cfdData->bcID;
        } else {
            marker = surfaceMesh->element[i].markerID;
        }

        if (faceBCIndex == CAPSMAGIC) {
            faceBCIndex = marker;
            continue;
        }

        if (faceBCIndex != marker) {
            printf("All boundary indexes must be the same for the face!!!\n");
            status = CAPS_BADVALUE;
            goto bail;
        }

    }


    // Determine a suitable boundary index of the extruded plane
    *extrusionBCIndex = faceBCIndex;
    for (i = 0; i < surfaceMesh->meshQuickRef.numLine; i++) {

        if (surfaceMesh->meshQuickRef.startIndexLine >= 0) {
            elementIndex = surfaceMesh->meshQuickRef.startIndexLine + i;
        } else {
            elementIndex = surfaceMesh->meshQuickRef.listIndexLine[i];
        }

        if (surfaceMesh->element[elementIndex].analysisType == MeshCFD) {
            cfdData = (cfdMeshDataStruct *) surfaceMesh->element[elementIndex].analysisData;
            marker = cfdData->bcID;
        } else {
            marker = surfaceMesh->element[elementIndex].markerID;
        }


        if (marker > *extrusionBCIndex)  {
            *extrusionBCIndex = marker;
        }
    }

    *extrusionBCIndex += 1;

    // Check to make sure the face is on the y = 0 plane
    for (i = 0; i < surfaceMesh->numNode; i++) {

        if (surfaceMesh->node[i].xyz[1] != 0.0) {
            printf("\nSurface mesh is not on y = 0.0 plane, FUN3D could fail during execution for this 2D mesh!!!\n");
            break;
        }
    }

    // Constant x?
    for (i = 0; i < surfaceMesh->numNode; i++) {
        if ((surfaceMesh->node[i].xyz[0] - surfaceMesh->node[0].xyz[0]) > 1E-7) {
            xMeshConstant = (int) false;
            break;
        }
    }

    // Constant y?
    for (i = 0; i < surfaceMesh->numNode; i++) {
        if ((surfaceMesh->node[i].xyz[1] - surfaceMesh->node[0].xyz[1] ) > 1E-7) {
            yMeshConstant = (int) false;
            break;
        }
    }

    // Constant z?
    for (i = 0; i < surfaceMesh->numNode; i++) {
        if ((surfaceMesh->node[i].xyz[2] - surfaceMesh->node[0].xyz[2]) > 1E-7) {
            zMeshConstant = (int) false;
            break;
        }
    }

    if (yMeshConstant != (int) true) {
        printf("FUN3D expects 2D meshes be in the x-z plane... attempting to rotate mesh!\n");

        if (xMeshConstant == (int) true && zMeshConstant == (int) false) {
            printf("Swapping y and x coordinates!\n");
            for (i = 0; i < surfaceMesh->numNode; i++) {
                tempCoord = surfaceMesh->node[i].xyz[0];
                surfaceMesh->node[i].xyz[0] = surfaceMesh->node[i].xyz[1];
                surfaceMesh->node[i].xyz[1] = tempCoord;
            }

        } else if(xMeshConstant == (int) false && zMeshConstant == (int) true) {

            printf("Swapping y and z coordinates!\n");
            for (i = 0; i < surfaceMesh->numNode; i++) {
                tempCoord = surfaceMesh->node[i].xyz[2];
                surfaceMesh->node[i].xyz[2] = surfaceMesh->node[i].xyz[1];
                surfaceMesh->node[i].xyz[1] = tempCoord;
            }

        } else {
            printf("Unable to rotate mesh!\n");
            status = CAPS_NOTFOUND;
        }
    }

    status = extrude_SurfaceMesh(extrusion, *extrusionBCIndex, surfaceMesh, volumeMesh);
    if (status != CAPS_SUCCESS) goto bail;

    return CAPS_SUCCESS;

    bail:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in fun3d_2DMesh status = %d\n", status);

        // Destroy volume mesh
        (void ) destroy_meshStruct(volumeMesh);

        return status;
}

// Write FUN3D data transfer files
int fun3D_dataTransfer(void *aimInfo,
                       const char *analysisPath,
                       char *projectName,
                       meshStruct volumeMesh,
                       modalAeroelasticStruct *eigenVector){

    /*! \page dataTransferFUN3D FUN3D Data Transfer
     *
     * \section dataToFUN3D Data transfer to FUN3D
     *
     * <ul>
     *  <li> <B>"Displacement"</B> </li> <br>
     *   Retrieves nodal displacements (as from a structural solver)
     *   and updates FUN3D's surface mesh; a new [project_name]_body1.dat file is written out which may
     *   be loaded into FUN3D to update the surface mesh/move the volume mesh using the FUN3D command line option
     *   -\-read_surface_from_file
     * </ul>
     *
     * <ul>
     *  <li> <B>"EigenVector_#"</B> </li> <br>
     *   Retrieves modal eigen-vectors from a structural solver, where "#" should be replaced by the
     *   corresponding mode number for the eigen-vector (eg. EigenVector_3 would correspond to the third mode,
     *   while EigenVector_6 would be the sixth mode) . A [project_name]_body1_mode#.dat file is written
     *   out for each mode.
     * </ul>
     *
     */


    int status; // Function return status
    int i, j, k, eigenIndex; // Indexing

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
    const char *dataOutName[] = {"x","y","z", "id", "dx", "dy", "dz"};

    double **dataOutMatrix = NULL;
    int *dataOutFormat = NULL;
    int *dataConnectMatrix = NULL;

    int numOutVariable = 7;
    int numOutDataPoint = 0;
    int numOutDataConnect = 0;

    char fileExtBody[] = "_body1";
    char fileExt[] = ".dat";
    char fileExtMode[] = "_mode";

    int foundDisplacement = (int) false, foundEigenVector = (int) false;

    meshStruct surfaceMesh;

    status = aim_getBounds(aimInfo, &numTransferName, &transferName);
    if (status != CAPS_SUCCESS) return status;

    (void) initiate_meshStruct(&surfaceMesh);

    foundDisplacement = foundEigenVector = (int) false;
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

        if ( eigenVector != NULL) {
            for (eigenIndex = 0; eigenIndex < eigenVector->numEigenValue; eigenIndex++) {

                status = aim_getDataSet(dataTransferDiscreteObj,
                                        eigenVector->eigenValue[eigenIndex].name,
                                        &dataTransferMethod,
                                        &numDataTransferPoint,
                                        &dataTransferRank,
                                        &dataTransferData);

                if (status == CAPS_SUCCESS) { // If we do have data ready is the rank correct

                    foundEigenVector = (int) true;

                    if (dataTransferRank != 3) {
                        printf("EigenVector transfer data found however rank is %d not 3!!!!\n", dataTransferRank);
                        status = CAPS_BADRANK;
                        goto cleanup;
                    }

                    break;
                }
            } // Loop through EigenValues

            if (foundEigenVector == (int) true) break;

        } // If eigen-vectors provided
    } // Loop through transfer names


    if (foundDisplacement != (int) true && foundEigenVector != (int) true) {

        printf("No recognized data transfer names found!\n");

        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Ok looks like we have displacements/EigenVectors to get so lets continue
    printf("Writing FUN3D data transfer files\n");

    // Combine surface meshes found in the volumes
    status = mesh_combineMeshStruct(volumeMesh.numReferenceMesh,
                                    volumeMesh.referenceMesh,
                                    &surfaceMesh);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Right now we are just going to output a body containing all surface nodes
    numOutDataPoint = surfaceMesh.numNode;

    numOutDataConnect = 0;
    status = mesh_retrieveNumMeshElements(surfaceMesh.numElement,
                                          surfaceMesh.element,
                                          Triangle,
                                          &j);
    if (status != CAPS_SUCCESS) goto cleanup;
    numOutDataConnect += j;

    status = mesh_retrieveNumMeshElements(surfaceMesh.numElement,
                                          surfaceMesh.element,
                                          Quadrilateral,
                                          &j);
    if (status != CAPS_SUCCESS) goto cleanup;
    numOutDataConnect += j;


    /*
	numOutDataPoint = 0;
	numOutDataConnect = 0;

	for (i = 0; i < volumeMesh.numReferenceMesh; i++) {
	    if (volumeMesh.referenceMesh[i].meshType == SurfaceMesh) {
	        numOutDataPoint += volumeMesh.referenceMesh[i].numNode;

	        // Number of data elements for triangles
	        status = mesh_retrieveNumMeshElements(volumeMesh.referenceMesh[i].numElement,
	                                              volumeMesh.referenceMesh[i].element,
	                                              Triangle,
	                                              &j);
	        if (status != CAPS_SUCCESS) goto cleanup;
	        numOutDataConnect += j;

            // Number of data elements for triangles
            status = mesh_retrieveNumMeshElements(volumeMesh.referenceMesh[i].numElement,
                                                  volumeMesh.referenceMesh[i].element,
                                                  Quadrilateral,
                                                  &j);
            if (status != CAPS_SUCCESS) goto cleanup;
            numOutDataConnect += j;
	    }
	}*/

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
        if (strcasecmp(dataOutName[i], "id") == 0) {
            dataOutFormat[i] = (int) Integer;
        } else {
            dataOutFormat[i] = (int) Double;
        }
    }

    // Fill data out matrix with current surface mesh and global id
    for (i = 0; i < numOutDataPoint; i++ ) {

        // Coordinates
        dataOutMatrix[0][i] = surfaceMesh.node[i].xyz[0];
        dataOutMatrix[1][i] = surfaceMesh.node[i].xyz[1];
        dataOutMatrix[2][i] = surfaceMesh.node[i].xyz[2];

        // Global ID - assumes the surfaceMesh nodes are in the volume at the beginning
        dataOutMatrix[3][i] = (double) surfaceMesh.node[i].nodeID;

        // Delta displacements
        dataOutMatrix[4][i] = 0;
        dataOutMatrix[5][i] = 0;
        dataOutMatrix[6][i] = 0;
    }

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
            nodeMap = &storage[0]; // Global indexing on the body

            for (j = 0; j < numDataTransferPoint; j++) {

                globalNodeID = nodeMap[j];
                for (k = 0; k < numOutDataPoint; k++) {

                    // If the global node IDs match store the displacement values in the dataOutMatrix
                    if (globalNodeID  == (int) dataOutMatrix[3][k]) {

                        // A rank of 3 should have already been checked
                        // Delta displacements
                        dataOutMatrix[4][k] = dataTransferData[3*j+0];
                        dataOutMatrix[5][k] = dataTransferData[3*j+1];
                        dataOutMatrix[6][k] = dataTransferData[3*j+2];
                        break;
                    }
                }
            }
        } // End dataTransferDiscreteObj loop

        // Update surface coordinates based on displacements
        for (i = 0; i < numOutDataPoint; i++ ) {

            // Coordinates + displacements at nodes
            dataOutMatrix[0][i] = dataOutMatrix[0][i] + dataOutMatrix[4][i]; // x
            dataOutMatrix[1][i] = dataOutMatrix[1][i] + dataOutMatrix[5][i]; // y
            dataOutMatrix[2][i] = dataOutMatrix[2][i] + dataOutMatrix[6][i]; // z

        }

        stringLength = strlen(projectName) + strlen(analysisPath) + strlen(fileExtBody) +strlen(fileExt) +1;
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
        strcat(filename, fileExtBody);
        strcat(filename, fileExt);

        filename[stringLength] = '\0';

        // Write out displacement in tecplot file
        status = tecplot_writeFEPOINT(filename,
                                      "FUN3D AeroLoads",
                                      NULL,
                                      numOutVariable,
                                      (char **) dataOutName,
                                      numOutDataPoint,
                                      dataOutMatrix,
                                      dataOutFormat,
                                      numOutDataConnect, // numConnectivity
                                      dataConnectMatrix, // connectivity matrix
                                      NULL); // Solution time
        if (status != CAPS_SUCCESS) goto cleanup;
    } // End if found displacements


    // Re-loop through transfers - if we are doing eigen-vectors
    if (foundEigenVector == (int) true) {

        for (eigenIndex = 0; eigenIndex < eigenVector->numEigenValue; eigenIndex++) {

            // Zero out the eigen-vectors each time we are writing out a new one
            for (i = 0; i < numOutDataPoint; i++ ) {

                // Delta eigen-vectors
                dataOutMatrix[4][i] = 0;
                dataOutMatrix[5][i] = 0;
                dataOutMatrix[6][i] = 0;
            }

            for (i = 0; i < numTransferName; i++) {

                status = aim_getDiscr(aimInfo, transferName[i], &dataTransferDiscreteObj);
                if (status != CAPS_SUCCESS) continue;


                status = aim_getDataSet(dataTransferDiscreteObj,
                        eigenVector->eigenValue[eigenIndex].name,
                        &dataTransferMethod,
                        &numDataTransferPoint,
                        &dataTransferRank,
                        &dataTransferData);


                if (status != CAPS_SUCCESS) continue; // If no elements in this object skip to next transfer name

                //Get extra node information stored in the discrObj
                storage = (int *) dataTransferDiscreteObj->ptrm;
                nodeMap = &storage[0]; // Global indexing on the body

                for (j = 0; j < numDataTransferPoint; j++) {

                    globalNodeID = nodeMap[j];
                    for (k = 0; k < numOutDataPoint; k++) {

                        // If the global node IDs match store the displacement values in the dataOutMatrix
                        if (globalNodeID  == (int) dataOutMatrix[3][k]) {

                            // A rank of 3 should have already been checked
                            // Eigen-vector
                            dataOutMatrix[4][k] = dataTransferData[3*j+0];
                            dataOutMatrix[5][k] = dataTransferData[3*j+1];
                            dataOutMatrix[6][k] = dataTransferData[3*j+2];
                            break;
                        }
                    }
                }
            } // End dataTransferDiscreteObj loop

            stringLength = strlen(analysisPath) +
                    strlen("/") +
                    strlen(projectName) +
                    strlen(fileExtBody) +
                    strlen(fileExtMode) +
                    strlen(fileExt) + 5;

            filename = (char *) EG_alloc((stringLength +1)*sizeof(char));
            if (filename == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }


#ifdef WIN32
            sprintf(filename, "%s%s%s%s%s%d%s", analysisPath,
                    "\\",
                    projectName,
                    fileExtBody,
                    fileExtMode,  // Change modeNumber so it always starts at 1!
                    eigenIndex+1, //eigenVector->eigenValue[eigenIndex].modeNumber,
                    fileExt);
#else
            sprintf(filename, "%s%s%s%s%s%d%s", analysisPath,
                    "/",
                    projectName,
                    fileExtBody,
                    fileExtMode,  // Change modeNumber so it always starts at 1!
                    eigenIndex+1, // eigenVector->eigenValue[eigenIndex].modeNumber,
                    fileExt);
#endif

            // Write out eigen-vector in tecplot file
            status = tecplot_writeFEPOINT(filename,
                                          "FUN3D Modal",
                                          NULL,
                                          numOutVariable,
                                          (char **) dataOutName,
                                          numOutDataPoint,
                                          dataOutMatrix,
                                          dataOutFormat,
                                          numOutDataConnect, // numConnectivity
                                          dataConnectMatrix, // connectivity matrix
                                          NULL); // Solution time

            if (status != CAPS_SUCCESS) goto cleanup;

            if (filename != NULL) EG_free(filename);
            filename = NULL;

        } // End eigenvector names
    } // End if found eigenvectors

    status = CAPS_SUCCESS;

    goto cleanup;

    // Clean-up
    cleanup:

    if (status != CAPS_SUCCESS) printf("Error: Premature exit in fun3D_dataTransfer status = %d\n", status);

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

// Write FUN3D fun3d.nml file
int fun3d_writeNML(void *aimInfo, const char *analysisPath, capsValue *aimInputs, cfdBCsStruct bcProps) {

    int status; // Function return status

    int i; // Indexing

    FILE *fnml = NULL;
    char *filename = NULL;
    char fileExt[] ="fun3d.nml";

    int stringLength;

    printf("Writing fun3d.nml\n");

    stringLength = strlen(analysisPath) + strlen(fileExt) + 1;

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

    strcat(filename, fileExt);

    filename[stringLength] = '\0';

    fnml = fopen(filename, "w");
    if (fnml == NULL) {
        printf("Unable to open file - %s\n", filename);
        status = CAPS_IOERR;
        goto cleanup;
    }

    // &project
    fprintf(fnml,"&project\n");
    fprintf(fnml," project_rootname = \"%s\"\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
    fprintf(fnml,"/\n\n");

    // &raw_grid
    fprintf(fnml,"&raw_grid\n");
    fprintf(fnml," grid_format = \"%s\"\n",
            aimInputs[aim_getIndex(aimInfo, "Mesh_Format",  ANALYSISIN)-1].vals.string);

    if (aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag",  ANALYSISIN)-1].vals.integer == (int) true) {
        fprintf(fnml," data_format = \"ascii\"\n");
    } else fprintf(fnml," data_format = \"stream\"\n");

    if (aimInputs[aim_getIndex(aimInfo, "Two_Dimensional",  ANALYSISIN)-1].vals.integer == (int) true) {
        fprintf(fnml," twod_mode = .true.\n");
        //fprintf(fnml," ignore_euler_number = .true.\n");
    }

    fprintf(fnml,"/\n\n");

    // &reference_physical_properties
    fprintf(fnml,"&reference_physical_properties\n");

    if (aimInputs[aim_getIndex(aimInfo, "Mach",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," mach_number = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Mach",  ANALYSISIN)-1].vals.real);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Re",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," reynolds_number = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Re",  ANALYSISIN)-1].vals.real);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Alpha",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," angle_of_attack = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Alpha",  ANALYSISIN)-1].vals.real);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Beta",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," angle_of_yaw = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Beta",  ANALYSISIN)-1].vals.real);
    }

    fprintf(fnml,"/\n\n");

    // &governing_equations
    fprintf(fnml,"&governing_equations\n");

    if (aimInputs[aim_getIndex(aimInfo, "Viscous",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," viscous_terms = \"%s\"\n",
                aimInputs[aim_getIndex(aimInfo, "Viscous",  ANALYSISIN)-1].vals.string);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Equation_Type",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," eqn_type = \"%s\"\n",
                aimInputs[aim_getIndex(aimInfo, "Equation_Type",  ANALYSISIN)-1].vals.string);
    }

    fprintf(fnml,"/\n\n");

    // &nonlinear_solver_parameters
    fprintf(fnml,"&nonlinear_solver_parameters\n");

    if (aimInputs[aim_getIndex(aimInfo, "Time_Accuracy", ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," time_accuracy = \"%s\"\n",
                aimInputs[aim_getIndex(aimInfo, "Time_Accuracy", ANALYSISIN)-1].vals.string);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Time_Step", ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," time_step_nondim = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Time_Step", ANALYSISIN)-1].vals.real);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Num_Subiter", ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," subiterations = %d\n",
                aimInputs[aim_getIndex(aimInfo, "Num_Subiter", ANALYSISIN)-1].vals.integer);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Temporal_Error", ANALYSISIN)-1].nullVal != IsNull) {

        fprintf(fnml," temporal_err_control = .true.\n");
        fprintf(fnml," temporal_err_floor = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Temporal_Error", ANALYSISIN)-1].vals.real);

    }

    if (aimInputs[aim_getIndex(aimInfo, "CFL_Schedule",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," schedule_cfl = %f %f\n",
                aimInputs[aim_getIndex(aimInfo, "CFL_Schedule",  ANALYSISIN)-1].vals.reals[0],
                aimInputs[aim_getIndex(aimInfo, "CFL_Schedule",  ANALYSISIN)-1].vals.reals[1]);
    }

    if (aimInputs[aim_getIndex(aimInfo, "CFL_Schedule_Iter",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," schedule_iteration = %d %d\n",
                aimInputs[aim_getIndex(aimInfo, "CFL_Schedule_Iter",  ANALYSISIN)-1].vals.integers[0],
                aimInputs[aim_getIndex(aimInfo, "CFL_Schedule_Iter",  ANALYSISIN)-1].vals.integers[1]);
    }

    fprintf(fnml,"/\n\n");

    // &code_run_control
    fprintf(fnml,"&code_run_control\n");

    if (aimInputs[aim_getIndex(aimInfo, "Num_Iter",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," steps = %d\n",
                aimInputs[aim_getIndex(aimInfo, "Num_Iter", ANALYSISIN)-1].vals.integer);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Restart_Read",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," restart_read = '%s'\n",
                aimInputs[aim_getIndex(aimInfo, "Restart_Read", ANALYSISIN)-1].vals.string);
    }


    fprintf(fnml,"/\n\n");

    //&force_moment_integ_properties
    fprintf(fnml,"&force_moment_integ_properties\n");

    if (aimInputs[aim_getIndex(aimInfo, "Reference_Area",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," area_reference = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Reference_Area", ANALYSISIN)-1].vals.real);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Moment_Length",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," x_moment_length = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].vals.reals[0]);

        fprintf(fnml," y_moment_length = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].vals.reals[1]);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Moment_Center",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," x_moment_center = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[0]);

        fprintf(fnml," y_moment_center = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[1]);

        fprintf(fnml," z_moment_center = %f\n",
                aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[2]);
    }

    fprintf(fnml,"/\n\n");

    //&boundary_conditions
    fprintf(fnml,"&boundary_conditions\n");

    // Loop through boundary conditions
    for (i = 0; i < bcProps.numBCID ; i++) {

        // Temperature
        if (bcProps.surfaceProps[i].wallTemperatureFlag == (int) true) {
            fprintf(fnml," wall_temperature(%d) = %f\n",bcProps.surfaceProps[i].bcID,
                    bcProps.surfaceProps[i].wallTemperature);
            fprintf(fnml," wall_temp_flag(%d) = .true.\n",bcProps.surfaceProps[i].bcID);
        }

        // Total pressure and temperature
        if (bcProps.surfaceProps[i].surfaceType == SubsonicInflow) {

            fprintf(fnml, " total_pressure_ratio(%d) = %f\n", bcProps.surfaceProps[i].bcID,
                    bcProps.surfaceProps[i].totalPressure);

            fprintf(fnml, " total_temperature_ratio(%d) = %f\n", bcProps.surfaceProps[i].bcID,
                    bcProps.surfaceProps[i].totalTemperature);
        }

        // Static pressure
        if (bcProps.surfaceProps[i].surfaceType == BackPressure ||
                bcProps.surfaceProps[i].surfaceType == SubsonicOutflow) {

            fprintf(fnml, " static_pressure_ratio(%d) = %f\n", bcProps.surfaceProps[i].bcID,
                    bcProps.surfaceProps[i].staticPressure);
        }

        // Mach number
        if (bcProps.surfaceProps[i].surfaceType == MachOutflow ||
                bcProps.surfaceProps[i].surfaceType == MassflowOut) {

            fprintf(fnml, " mach_bc(%d) = %f\n", bcProps.surfaceProps[i].bcID,
                    bcProps.surfaceProps[i].machNumber);
        }

        // Massflow
        if (bcProps.surfaceProps[i].surfaceType == MassflowIn ||
                bcProps.surfaceProps[i].surfaceType == MassflowOut) {

            fprintf(fnml, " massflow(%d) = %f\n", bcProps.surfaceProps[i].bcID,
                    bcProps.surfaceProps[i].massflow);
        }

        // Fixed inflow and outflow
        /*if (bcProps.surfaceProps[i].surfaceType == FixedInflow ||
            bcProps.surfaceProps[i].surfaceType == FixedOutflow) {

            fprintf(fnml, " qset(%d,1) = %f\n", bcProps.surfaceProps[i].bcID,
                                                bcProps.surfaceProps[i].staticDensity);

            fprintf(fnml, " qset(%d,2) = %f\n", bcProps.surfaceProps[i].bcID,
                                                bcProps.surfaceProps[i].uVelocity);

            fprintf(fnml, " qset(%d,3) = %f\n", bcProps.surfaceProps[i].bcID,
                                                bcProps.surfaceProps[i].vVelocity);

            fprintf(fnml, " qset(%d,4) = %f\n", bcProps.surfaceProps[i].bcID,
                                                bcProps.surfaceProps[i].wVelocity);

            fprintf(fnml, " qset(%d,5) = %f\n", bcProps.surfaceProps[i].bcID,
                                                bcProps.surfaceProps[i].staticDensity);
        }*/
    }

    fprintf(fnml,"/\n\n");

    // &noninertial_reference_frame
    fprintf(fnml,"&noninertial_reference_frame\n");


    if (aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].nullVal   != IsNull ||
            aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Center",  ANALYSISIN)-1].nullVal != IsNull) {

        fprintf(fnml," noninertial = .true.\n");
    }

    if (aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Center",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," rotation_center_x = %f\n",aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Center",  ANALYSISIN)-1].vals.reals[0]);
        fprintf(fnml," rotation_center_y = %f\n",aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Center",  ANALYSISIN)-1].vals.reals[1]);
        fprintf(fnml," rotation_center_z = %f\n",aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Center",  ANALYSISIN)-1].vals.reals[2]);
    }

    if (aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].nullVal != IsNull) {
        fprintf(fnml," rotation_rate_x = %f\n",aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].vals.reals[0]);
        fprintf(fnml," rotation_rate_y = %f\n",aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].vals.reals[1]);
        fprintf(fnml," rotation_rate_z = %f\n",aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].vals.reals[2]);

    }

    fprintf(fnml,"/\n\n");

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:

    if (status != CAPS_SUCCESS) printf("Error: Premature exit in fun3d_writeNML status = %d\n", status);

    if (fnml != NULL) fclose(fnml);

    if (filename != NULL) EG_free(filename);

    return status;
}

// Write FUN3D movingbody.input file
int fun3d_writeMovingBody( const char *analysisPath, cfdBCsStruct bcProps, modalAeroelasticStruct *modalAeroelastic) {


    int status; // Function return status

    int i, eigenIndex; // Indexing
    int counter = 0;
    FILE *fp = NULL;
    char *filename = NULL;
    char fileExt[] ="moving_body.input";

    int bodyIndex = 1;
    int stringLength;

    printf("Writing moving_body.input");

    stringLength = strlen(analysisPath) + strlen(fileExt) + 1;

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
    strcat(filename, fileExt);

    filename[stringLength] = '\0';

    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Unable to open file - %s\n", filename);
        status = CAPS_IOERR;
        goto cleanup;
    }

    // &body_definitions
    fprintf(fp,"&body_definitions\n");

    fprintf(fp," n_moving_bodies = %d\n", bodyIndex);

    counter = 0;
    for (i = 0; i < bcProps.numBCID; i++) {

        if (bcProps.surfaceProps[i].surfaceType == Viscous ||
                bcProps.surfaceProps[i].surfaceType == Inviscid) {

            fprintf(fp," defining_bndry(%d,%d) = %d\n", counter+1,
                    bodyIndex,
                    bcProps.surfaceProps[i].bcID);

            counter += 1;
        }
    }

    fprintf(fp," n_defining_bndry(%d) = %d\n", bodyIndex, counter);

    fprintf(fp," motion_driver(%d) = ", bodyIndex);
    if (modalAeroelastic != NULL) {
        fprintf(fp,"\"aeroelastic\"\n");
    }

    fprintf(fp," mesh_movement(%d) = ", bodyIndex);
    if (modalAeroelastic != NULL) {
        fprintf(fp,"\"deform\"\n");
    }

    fprintf(fp,"/\n\n");

    if (modalAeroelastic != NULL) {

        // &aeroelastic_modal_data
        fprintf(fp,"&aeroelastic_modal_data\n");

        fprintf(fp," nmode(%d) = %d\n", bodyIndex, modalAeroelastic->numEigenValue);
        fprintf(fp," uinf(%d) = %f\n", bodyIndex, modalAeroelastic->freestreamVelocity);
        fprintf(fp," qinf(%d) = %f\n", bodyIndex, modalAeroelastic->freestreamDynamicPressure);
        fprintf(fp," grefl(%d) = %f\n", bodyIndex, modalAeroelastic->lengthScaling);

        fprintf(fp, "\n");


        for (i = 0; i < modalAeroelastic->numEigenValue; i++) {

            eigenIndex = i + 1; // Change mode number so that it always starts at 1
            // modalAeroelastic->eigenValue[i].modeNumber

            fprintf(fp, " ! Mode %d of %d (structural mode %d)\n", eigenIndex,
                    modalAeroelastic->numEigenValue,
                    modalAeroelastic->eigenValue[i].modeNumber);

            fprintf(fp," freq(%d,%d) = %f\n", eigenIndex, bodyIndex, modalAeroelastic->eigenValue[i].frequency);
            fprintf(fp," damp(%d,%d) = %f\n", eigenIndex, bodyIndex, modalAeroelastic->eigenValue[i].damping);

            fprintf(fp," gmass(%d,%d) = %f\n"  , eigenIndex, bodyIndex, modalAeroelastic->eigenValue[i].generalMass);
            fprintf(fp," gdisp0(%d,%d) = %f\n" , eigenIndex, bodyIndex, modalAeroelastic->eigenValue[i].generalDisplacement);
            fprintf(fp," gvel0(%d,%d) = %f\n"  , eigenIndex, bodyIndex, modalAeroelastic->eigenValue[i].generalVelocity);
            fprintf(fp," gforce0(%d,%d) = %f\n", eigenIndex, bodyIndex, modalAeroelastic->eigenValue[i].generalForce);

            fprintf(fp, "\n");
        }
        fprintf(fp,"/\n\n");
    }


    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
    if (status != CAPS_SUCCESS) printf("Error: Premature exit in fun3d_writeMovingBody status = %d\n", status);

    if (fp != NULL) fclose(fp);

    if (filename != NULL) EG_free(filename);

    return status;
}

// Write FUN3D parameterization/sensitivity file
int  fun3d_writeParameterization(void *aimInfo,
        const char *analysisPath,
        meshStruct *volumeMesh,
        int numGeomIn,
        capsValue *geomInVal){

    int status; // Function return status

    int i, j, k, m, row, col; // Indexing

    int stringLength = 7;
    meshStruct *surfaceMesh; // Temporary holder for the reference mesh

    // Data transfer Out variables
    char **dataOutName= NULL; //= {"x","y","z", "id", "dx", "dy", "dz"};

    double **dataOutMatrix = NULL;
    int *dataOutFormat = NULL;
    int *dataConnectMatrix = NULL;

    int numOutVariable = 4; // x, y, z, id
    int numOutDataPoint = 0;
    int numOutDataConnect = 0;

    int nodeOffSet = 0; //Keep track of global node indexing offset due to combining multiple surface meshes

    const char *geomInName;
    int numPoint;
    double *xyz = NULL;

    char message[100];
    char filePre[] = "model.tec.";
    char fileExt[] = ".sd1";
    char *filename = NULL;
    char folder[]  = "Rubberize";
    char zoneTitle[100];

    if (numGeomIn == 0) {

        printf("No geometry in values for sensitivities\n");

        status = CAPS_SUCCESS;
        goto cleanup;

    } else {
        printf("All geometry in values are being used in sensitivity file!\n");
    }

    // Determine number of geometry input variables
    for (i = 0; i < numGeomIn; i++) {
        if(aim_getGeomInType(aimInfo, i+1) == EGADS_OUTSIDE) continue;

        numOutVariable += 3*geomInVal[i].length; // xD1, yD1, zD1, ...
    }

    if (numOutVariable > 99999) {
        printf("Array of design variable names will be over-run!");
        status = CAPS_RANGEERR;
        goto cleanup;
    }

    // Allocate our names
    dataOutName = (char **) EG_alloc(numOutVariable*sizeof(char *));
    if (dataOutName == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    stringLength = 7;
    j = 1;
    k = 1;
    for (i = 0; i < numOutVariable; i++) {
        dataOutName[i] = (char *) EG_alloc((stringLength+1)*sizeof(char));

        if (dataOutName[i] == NULL) {

            (void) string_freeArray(i, &dataOutName);

            status = EGADS_MALLOC;
            goto cleanup;
        }

        // Set names
        if      (i == 0) sprintf(dataOutName[i], "%s", "x");
        else if (i == 1) sprintf(dataOutName[i], "%s", "y");
        else if (i == 2) sprintf(dataOutName[i], "%s", "z");
        else if (i == 3) sprintf(dataOutName[i], "%s", "id");
        else {

            if      (j == 1) sprintf(dataOutName[i], "%s%d", "xD", k);
            else if (j == 2) sprintf(dataOutName[i], "%s%d", "yD", k);
            else if (j == 3) {
                sprintf(dataOutName[i], "%s%d", "zD", k);
                j = 0;
                k += 1;
            }

            j += 1;
        }
    }

    // Allocate data arrays that are going to be output
    dataOutMatrix = (double **) EG_alloc(numOutVariable*sizeof(double));
    dataOutFormat = (int *) EG_alloc(numOutVariable*sizeof(int));

    if (dataOutMatrix == NULL || dataOutFormat == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    for (i = 0; i < numOutVariable; i++) dataOutMatrix[i] = NULL;

    // Set data out formatting
    for (i = 0; i < numOutVariable; i++) {
        if (strcasecmp(dataOutName[i], "id") == 0) {
            dataOutFormat[i] = (int) Integer;
        } else {
            dataOutFormat[i] = (int) Double;
        }
    }

    // Write sensitivity files for each surface mesh
    for (i = 0; i < volumeMesh->numReferenceMesh; i++) {

        surfaceMesh = &volumeMesh->referenceMesh[i];

        if (surfaceMesh->meshType != SurfaceMesh) {
            status = CAPS_BADVALUE;
            printf("Error: Reference mesh is not a surface mesh!\n");
            goto cleanup;
        }

        // Right now we are just going to output a body containing all surface nodes
        numOutDataPoint = surfaceMesh->numNode;

        numOutDataConnect = 0;
        status = mesh_retrieveNumMeshElements(surfaceMesh->numElement,
                surfaceMesh->element,
                Triangle,
                &j);
        if (status != CAPS_SUCCESS) goto cleanup;
        numOutDataConnect += j;

        status = mesh_retrieveNumMeshElements(surfaceMesh->numElement,
                surfaceMesh->element,
                Quadrilateral,
                &j);
        if (status != CAPS_SUCCESS) goto cleanup;
        numOutDataConnect += j;

        // Allocate data arrays that are going to be output
        dataConnectMatrix = (int *) EG_reall(dataConnectMatrix, 4*numOutDataConnect*sizeof(int));

        if (dataConnectMatrix == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (j = 0; j < numOutVariable; j++) {

            dataOutMatrix[j] = (double *) EG_reall(dataOutMatrix[j], numOutDataPoint*sizeof(double));

            if (dataOutMatrix[j] == NULL) { // If allocation failed ....

                status =  EGADS_MALLOC;
                goto cleanup;
            }
        }

        // Populate nodal coordinates and global id
        for (j = 0; j < surfaceMesh->numNode; j++) {


            dataOutMatrix[0][j] = surfaceMesh->node[j].xyz[0]; // x
            dataOutMatrix[1][j] = surfaceMesh->node[j].xyz[1]; // y
            dataOutMatrix[2][j] = surfaceMesh->node[j].xyz[2]; // z

            dataOutMatrix[3][j] = surfaceMesh->node[j].nodeID + nodeOffSet; // global ID
        }

        // Populate connectivity
        k = 0;
        for (j = 0; j < surfaceMesh->numElement; j++) {

            if (surfaceMesh->element[j].elementType == Triangle) {

                dataConnectMatrix[4*k+ 0] = surfaceMesh->element[j].connectivity[0];
                dataConnectMatrix[4*k+ 1] = surfaceMesh->element[j].connectivity[1];
                dataConnectMatrix[4*k+ 2] = surfaceMesh->element[j].connectivity[2];
                dataConnectMatrix[4*k+ 3] = surfaceMesh->element[j].connectivity[2];

            } else if (surfaceMesh->element[j].elementType == Quadrilateral) {

                dataConnectMatrix[4*k+ 0] = surfaceMesh->element[j].connectivity[0];
                dataConnectMatrix[4*k+ 1] = surfaceMesh->element[j].connectivity[1];
                dataConnectMatrix[4*k+ 2] = surfaceMesh->element[j].connectivity[2];
                dataConnectMatrix[4*k+ 3] = surfaceMesh->element[j].connectivity[3];

            } else {
                continue;
            }

            k += 1;
        }

        // Loop over the geometry in values
        m = 4;
        for (j = 0; j < numGeomIn; j++) {
            if(aim_getGeomInType(aimInfo, j+1) == EGADS_OUTSIDE) continue;

            status = aim_getName(aimInfo, j+1, GEOMETRYIN, &geomInName);
            if (status != CAPS_SUCCESS) goto cleanup;

            printf("Geometric sensitivity name = %s\n", geomInName);

            if (xyz != NULL) EG_free(xyz);
            xyz = NULL;

            if (geomInVal[j].length == 1) {

                status = aim_sensitivity(aimInfo,
                        geomInName,
                        1, 1,
                        surfaceMesh->bodyTessMap.egadsTess,
                        &numPoint, &xyz);
                if (status == CAPS_NOTFOUND) {
                    numPoint = surfaceMesh->numNode;
                    xyz = (double *) EG_reall(xyz, 3*numPoint*sizeof(double));
                    if (xyz == NULL) {
                        status = EGADS_MALLOC;
                        goto cleanup;
                    }

                    for (k = 0; k < 3*numPoint; k++) xyz[k] = 0.0;

                    printf("Warning: Sensitivity not found for %s, defaulting to 0.0s\n", geomInName);

                } else if (status != CAPS_SUCCESS) {

                    goto cleanup;

                }

                if (numPoint != surfaceMesh->numNode) {
                    printf("Error: the number of nodes returned by aim_senitivity does NOT match the surface mesh!\n");
                    status = CAPS_MISMATCH;
                    goto cleanup;
                }

                for (k = 0; k < surfaceMesh->numNode; k++) {

                    if (surfaceMesh->node[k].nodeID != k+1) {
                        printf("Error: Node Id %d is out of order (%d). No current fix!\n", surfaceMesh->node[k].nodeID, k+1);
                        status = CAPS_MISMATCH;
                        goto cleanup;
                    }

                    dataOutMatrix[m+0][k] = xyz[3*k + 0]; // x
                    dataOutMatrix[m+1][k] = xyz[3*k + 1]; // y
                    dataOutMatrix[m+2][k] = xyz[3*k + 2]; // z
                }

                m += 3;

            } else {

                for (row = 0; row < geomInVal[j].nrow; row++) {
                    for (col = 0; col < geomInVal[j].ncol; col++) {

                        if (xyz != NULL) EG_free(xyz);
                        xyz = NULL;

                        status = aim_sensitivity(aimInfo,
                                geomInName,
                                row+1, col+1, // row, col
                                surfaceMesh->bodyTessMap.egadsTess,
                                &numPoint, &xyz);
                        if (status == CAPS_NOTFOUND) {
                            numPoint = surfaceMesh->numNode;
                            xyz = (double *) EG_reall(xyz, 3*numPoint*sizeof(double));
                            if (xyz == NULL) {
                                status = EGADS_MALLOC;
                                goto cleanup;
                            }

                            for (k = 0; k < 3*numPoint; k++) xyz[k] = 0.0;

                            printf("Warning: Sensitivity not found for %s, defaulting to 0.0s\n", geomInName);

                        } else if (status != CAPS_SUCCESS) {

                            goto cleanup;

                        }

                        if (numPoint != surfaceMesh->numNode) {
                            printf("Error: the number of nodes returned by aim_senitivity does NOT match the surface mesh!\n");
                            status = CAPS_MISMATCH;
                            goto cleanup;
                        }

                        for (k = 0; k < surfaceMesh->numNode; k++) {

                            if (surfaceMesh->node[k].nodeID != k+1) {
                                printf("Error: Node Id %d is out of order (%d). No current fix!\n", surfaceMesh->node[k].nodeID, k+1);
                                status = CAPS_MISMATCH;
                                goto cleanup;
                            }

                            dataOutMatrix[m+0][k] = xyz[3*k + 0]; // x
                            dataOutMatrix[m+1][k] = xyz[3*k + 1]; // y
                            dataOutMatrix[m+2][k] = xyz[3*k + 2]; // z
                        }

                        m += 3;
                    }
                }
            }
        }

        sprintf(message,"%s %d,", "sensitivity file for body", i+1);
        //                                    /                    /                    bodyNumber          endCharacter
        stringLength = strlen(analysisPath) + 1 + strlen(folder) + 1 + strlen(filePre) + 7 + strlen(fileExt) + 1 ;

        filename = (char *) EG_reall(filename, stringLength*sizeof(char));
        if (filename == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

#ifdef WIN32
        sprintf(filename, "%s\\%s\\%s%d%s", analysisPath, folder, filePre, i+1, fileExt);
#else
        sprintf(filename, "%s/%s/%s%d%s", analysisPath, folder, filePre, i+1, fileExt);
#endif

        sprintf(zoneTitle, "%s_%d", "Body", i+1);

        status = tecplot_writeFEPOINT(filename,
                                      message,
                                      zoneTitle,
                                      numOutVariable,
                                      dataOutName,
                                      numOutDataPoint,
                                      dataOutMatrix,
                                      dataOutFormat,
                                      numOutDataConnect,
                                      dataConnectMatrix,
                                      NULL);
        if (status != CAPS_SUCCESS) goto cleanup;

        nodeOffSet += surfaceMesh->numNode;
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
    if (status != CAPS_SUCCESS) printf("Error: Premature exit in fun3d_writeParameterization status = %d\n", status);

    (void) string_freeArray(numOutVariable, &dataOutName);

    if (dataOutMatrix != NULL) {
        for (i = 0; i < numOutVariable; i++) {
            if (dataOutMatrix[i] != NULL)  EG_free(dataOutMatrix[i]);
        }
    }

    if (dataOutMatrix != NULL) EG_free(dataOutMatrix);
    if (dataOutFormat != NULL) EG_free(dataOutFormat);
    if (dataConnectMatrix != NULL) EG_free(dataConnectMatrix);

    if (xyz != NULL) EG_free(xyz);

    if (filename != NULL) EG_free(filename);
    return status;
}


// Write FUN3D  rubber.data file
int fun3d_writeRubber(void *aimInfo,
        capsValue *aimInputs,
        const char *analysisPath,
        meshStruct *volumeMesh,
        int numGeomIn,
        capsValue *geomInVal) {

    int status; // Function return status

    int i, j, k, m; // Indexing

    int numBody = 0, numShapeVar = 0;

    int stringLength;
    char file[] = "rubber.data";
    char *filename = NULL;
    FILE *fp = NULL;

    double fun3dVersion;

    stringLength = strlen(analysisPath) + strlen(file) + 2;
    filename = (char *) EG_alloc(stringLength*sizeof(char));
    if (filename == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

#ifdef WIN32
    sprintf(filename, "%s\\%s", analysisPath, file);
#else
    sprintf(filename, "%s/%s", analysisPath, file);
#endif

    printf("Writing %s \n", filename);

    // Open file
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        status = CAPS_IOERR;
        goto cleanup;
    }

    numBody = volumeMesh->numReferenceMesh;

    for (i = 0; i < volumeMesh->numReferenceMesh; i++) {

        if (volumeMesh->referenceMesh[i].meshType != SurfaceMesh) {
            status = CAPS_BADVALUE;
            printf("Error: Reference mesh is not a surface mesh!\n");
            goto cleanup;
        }
    }

    // Determine number of geometry input variables
    for (i = 0; i < numGeomIn; i++) {
        if(aim_getGeomInType(aimInfo, i+1) == EGADS_OUTSIDE) continue;

        numShapeVar += geomInVal[i].length; // xD1, yD1, zD1, ...
    }

    fun3dVersion = aimInputs[aim_getIndex(aimInfo, "FUN3D_Version",  ANALYSISIN)-1].vals.real;

    fprintf(fp, "################################################################################\n");
    fprintf(fp, "########################### Design Variable Information ########################\n");
    fprintf(fp, "################################################################################\n");
    fprintf(fp, "Global design variables (Mach number, AOA, Yaw, Noninertial rates)\n");
    fprintf(fp, "Var Active         Value               Lower Bound            Upper Bound\n");

    if (aimInputs[aim_getIndex(aimInfo, "Mach",  ANALYSISIN)-1].nullVal != IsNull){
        fprintf(fp, "Mach    1   %.15E  0.000000000000000E+00  0.900000000000000E+00\n", aimInputs[aim_getIndex(aimInfo, "Mach",  ANALYSISIN)-1].vals.real);
    } else {
        fprintf(fp, "Mach    1   0.000000000000000E+00  0.000000000000000E+00  1.200000000000000E+00\n");
    }

    if (aimInputs[aim_getIndex(aimInfo, "Alpha",  ANALYSISIN)-1].nullVal != IsNull){
        fprintf(fp, "AOA     1   %.15E  0.000000000000000E+00  0.900000000000000E+00\n", aimInputs[aim_getIndex(aimInfo, "Alpha",  ANALYSISIN)-1].vals.real);
    } else {
        fprintf(fp, "AOA     1   0.000000000000000E+00  0.000000000000000E+00  10.00000000000000E+00\n");
    }

    if (fun3dVersion > 12.4) {
        // FUN3D version 13.1 - version 12.4 doesn't have these available

        if (aimInputs[aim_getIndex(aimInfo, "Beta",  ANALYSISIN)-1].nullVal != IsNull){
            fprintf(fp, "Yaw     1   %.15E  0.000000000000000E+00  0.900000000000000E+00\n", aimInputs[aim_getIndex(aimInfo, "Beta",  ANALYSISIN)-1].vals.real);
        } else {
            fprintf(fp, "Yaw     1   0.000000000000000E+00  0.000000000000000E+00  10.00000000000000E+00\n");
        }

        if (aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].nullVal != IsNull){
            fprintf(fp, "xrate   0   %.15E  0.000000000000000E+00  0.900000000000000E+00\n", aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].vals.reals[0]);
        } else {
            fprintf(fp, "xrate   0   0.000000000000000E+00  0.000000000000000E+00  10.00000000000000E+00\n");
        }
        if (aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].nullVal != IsNull){
            fprintf(fp, "yrate   0   %.15E  0.000000000000000E+00  0.900000000000000E+00\n", aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].vals.reals[1]);
        } else {
            fprintf(fp, "yrate   0   0.000000000000000E+00  0.000000000000000E+00  10.00000000000000E+00\n");
        }
        if (aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].nullVal != IsNull){
            fprintf(fp, "zrate   0   %.15E  0.000000000000000E+00  0.900000000000000E+00\n", aimInputs[aim_getIndex(aimInfo, "NonInertial_Rotation_Rate",  ANALYSISIN)-1].vals.reals[2]);
        } else {
            fprintf(fp, "zrate   0   0.000000000000000E+00  0.000000000000000E+00  10.00000000000000E+00\n");
        }
    }

    fprintf(fp, "Number of bodies\n");
    fprintf(fp, "%d\n", numBody);
    for (i = 0; i < numBody; i++) {
        fprintf(fp, "Rigid motion design variables for 'Body %d'\n", i+1);
        fprintf(fp, "Var Active         Value               Lower Bound            Upper Bound\n");
        fprintf(fp, "RotRate  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotFreq  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotAmpl  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotOrgx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotOrgy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotOrgz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotVecx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotVecy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "RotVecz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "TrnRate  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "TrnFreq  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "TrnAmpl  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "TrnVecx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "TrnVecy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        fprintf(fp, "TrnVecz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00\n");
        if (fun3dVersion > 12.4) {
            // FUN3D version 13.1 - version 12.4 doesn't have option 5
            fprintf(fp, "Parameterization Scheme (Massoud=1 Bandaids=2 Sculptor=4 User-Defined=5)\n");
            fprintf(fp, "5\n");
        } else {

            fprintf(fp, "Parameterization Scheme (Massoud=1 Bandaids=2 Sculptor=4)\n");
            fprintf(fp, "1\n");
        }

        fprintf(fp, "Number of shape variables for 'Body %d'\n", i+1);
        fprintf(fp, "%d\n", numShapeVar);
        fprintf(fp, "Index Active         Value               Lower Bound            Upper Bound\n");

        m = 1;
        for (j = 0; j < numGeomIn; j++) {
            if(aim_getGeomInType(aimInfo, j+1) == EGADS_OUTSIDE) continue;
            for (k = 0; k < geomInVal[j].length; k++ ) {

                if (geomInVal[j].length == 1) {

                    fprintf(fp, "%d    1   %.15E  0.000000000000000E+00  0.000000000000000E+00\n", m, geomInVal[j].vals.real);
                } else {

                    fprintf(fp, "%d    1   %.15E  0.000000000000000E+00  0.000000000000000E+00\n", m, geomInVal[j].vals.reals[k]);
                }
                m += 1;
            }
        }
    }

    fprintf(fp, "################################################################################\n");
    fprintf(fp, "############################### Function Information ###########################\n");
    fprintf(fp, "################################################################################\n");
    fprintf(fp, "Number of composite functions for design problem statement\n");
    fprintf(fp, "1\n");
    fprintf(fp, "################################################################################\n");
    fprintf(fp, "Cost function (1) or constraint (2)\n");
    fprintf(fp, "1\n");
    fprintf(fp, "If constraint, lower and upper bounds\n");
    fprintf(fp, "0.0 0.0\n");
    fprintf(fp, "Number of components for function   1\n");
    fprintf(fp, "1\n");
    fprintf(fp, "Physical timestep interval where function is defined\n");
    fprintf(fp, "1 1\n");
    fprintf(fp, "Composite function weight, target, and power\n");
    fprintf(fp, "1.0 0.0 1.0\n");
    fprintf(fp, "Components of function   1: boundary id (0=all)/name/value/weight/target/power\n");
    fprintf(fp, "0 clcd          0.000000000000000    1.000   10.00000 2.000\n");
    fprintf(fp, "Current value of function   1\n");
    fprintf(fp, "0.000000000000000\n");
    fprintf(fp, "Current derivatives of function wrt global design variables\n");
    fprintf(fp, "0.000000000000000\n");
    fprintf(fp, "0.000000000000000\n");

    if (fun3dVersion > 12.4) {
        // FUN3D version 13.1 - version 12.4 doesn't have these available
        fprintf(fp, "0.000000000000000\n"); // Yaw
        fprintf(fp, "0.000000000000000\n"); // xrate
        fprintf(fp, "0.000000000000000\n"); // yrate
        fprintf(fp, "0.000000000000000\n"); // zrate
    }

    for (i = 0; i < numBody; i++) {
        fprintf(fp, "Current derivatives of function wrt rigid motion design variables of body %d\n", i+1);
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "0.000000000000000\n");
        fprintf(fp, "Current derivatives of function wrt shape design variables of body %d\n", i+1);

        for (j = 0; j < numGeomIn; j++) {
            if(aim_getGeomInType(aimInfo, j+1) == EGADS_OUTSIDE) continue;
            for (k = 0; k < geomInVal[j].length; k++ ) {
                fprintf(fp, "0.000000000000000\n");
            }
        }
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
    if (status != CAPS_SUCCESS) printf("Error: Premature exit in fun3d_writeRubber status = %d\n", status);

    if (filename != NULL) EG_free(filename);
    if (fp != NULL) fclose(fp);

    return status;

    /* Version 13.1 - template
	################################################################################
	########################### Design Variable Information ########################
	################################################################################
	Global design variables (Mach number, AOA, Yaw, Noninertial rates)
	Var Active         Value               Lower Bound            Upper Bound
	Mach    0   0.800000000000000E+00  0.000000000000000E+00  0.900000000000000E+00
	AOA    1   1.000000000000000E+00  0.000000000000000E+00  5.000000000000000E+00
	Yaw    0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	xrate    0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	yrate    0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	zrate    0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	Number of bodies
	2
	Rigid motion design variables for 'wing'
	Var Active         Value               Lower Bound            Upper Bound
	RotRate  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotFreq  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotAmpl  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotOrgx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotOrgy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotOrgz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotVecx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotVecy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotVecz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnRate  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnFreq  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnAmpl  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnVecx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnVecy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnVecz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	Parameterization Scheme (Massoud=1 Bandaids=2 Sculptor=4 User-Defined=5)
	1
	Number of shape variables for 'wing'
	3
	Index Active         Value               Lower Bound            Upper Bound
	1    1   1.000000000000000E+00  0.000000000000000E+00  2.000000000000000E+00
	2    1   1.000000000000000E+00  0.000000000000000E+00  2.000000000000000E+00
	3    1   1.000000000000000E+00  0.000000000000000E+00  2.000000000000000E+00
	Rigid motion design variables for 'tail'
	Var Active         Value               Lower Bound            Upper Bound
	RotRate  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotFreq  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotAmpl  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotOrgx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotOrgy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotOrgz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotVecx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotVecy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	RotVecz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnRate  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnFreq  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnAmpl  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnVecx  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnVecy  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	TrnVecz  0   0.000000000000000E+00  0.000000000000000E+00  0.000000000000000E+00
	Parameterization Scheme (Massoud=1 Bandaids=2 Sculptor=4 User-Defined=5)
	2
	Number of shape variables for 'tail'
	2
	Index Active         Value               Lower Bound            Upper Bound
	1    1   2.000000000000000E+00 -1.000000000000000E+00  5.000000000000000E+00
	2    1   2.000000000000000E+00 -1.000000000000000E+00  5.000000000000000E+00
	################################################################################
	############################### Function Information ###########################
	################################################################################
	Number of composite functions for design problem statement
	2
	################################################################################
	Cost function (1) or constraint (2)
	1
	If constraint, lower and upper bounds
	0.0 0.0
	Number of components for function   1
	1
	Physical timestep interval where function is defined
	1 1
	Composite function weight, target, and power
	1.0 0.0 1.0
	Components of function   1: boundary id (0=all)/name/value/weight/target/power
	0 cl            0.000000000000000    1.000   10.00000 2.000
	Current value of function   1
	0.000000000000000
	Current derivatives of function wrt global design variables
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt rigid motion design variables of body   1
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt shape design variables of body   1
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt rigid motion design variables of body   2
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt shape design variables of body   2
	0.000000000000000
	0.000000000000000
	################################################################################
	Cost function (1) or constraint (2)
	2
	If constraint, lower and upper bounds
	-0.03 -0.01
	Number of components for function   2
	1
	Physical timestep interval where function is defined
	1 1
	Composite function weight, target, and power
	1.0 0.0 1.0
	Components of function   2: boundary id (0=all)/name/value/weight/target/power
	0 cmy           0.000000000000000    1.000    0.00000 1.000
	Current value of function   2
	0.000000000000000
	Current derivatives of function wrt global design variables
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt rigid motion design variables of body   1
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt shape design variables of body   1
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt rigid motion design variables of body   2
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	0.000000000000000
	Current derivatives of function wrt shape design variables of body   2
	0.000000000000000
	0.000000000000000
     */
}

// Make FUN3D directory structure/tree
int fun3d_makeDirectory(const char *analysisPath) {

    int status; // Function return status

    int stringLength;
    char *filename = NULL;

    char flow[] = "Flow";
    char datafile[] = "datafiles";
    char adjoint[] = "Adjoint";
    char rubber[] = "Rubberize";

    printf("Creating FUN3D directory tree\n");

    stringLength = strlen(analysisPath) + 1 + strlen(rubber) + 1+ strlen(datafile) + 1;

    filename = (char *) EG_alloc(stringLength*sizeof(char));
    if (filename == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    // Flow
#ifdef WIN32
    sprintf(filename, "%s\\%s", analysisPath, flow);
    status = _mkdir(filename);
#else
    sprintf(filename, "%s/%s", analysisPath, flow);
    status = mkdir(filename, S_IRWXU);
#endif

    if (status != 0) {
        if (errno != EEXIST) {

            printf("Unable to make %s folder!\n", flow);
            status = CAPS_IOERR;
            goto cleanup;
        }
    }

    // Datafiles
#ifdef WIN32
    sprintf(filename, "%s\\%s\\%s", analysisPath, flow, datafile);
    status = _mkdir(filename);
#else
    sprintf(filename, "%s/%s/%s", analysisPath, flow, datafile);
    status = mkdir(filename, S_IRWXU);
#endif

    if (status != 0) {
        if (errno != EEXIST) {

            printf("Unable to make %s folder!\n", datafile);
            status = CAPS_IOERR;
            goto cleanup;
        }
    }

    // Adjoint
#ifdef WIN32
    sprintf(filename, "%s\\%s", analysisPath, adjoint);
    status = _mkdir(filename);
#else
    sprintf(filename, "%s/%s", analysisPath, adjoint);
    status = mkdir(filename, S_IRWXU);
#endif

    if (status != 0) {
        if (errno != EEXIST) {

            printf("Unable to make %s folder!\n", adjoint);
            status = CAPS_IOERR;
            goto cleanup;
        }
    }

    // Rubber
#ifdef WIN32
    sprintf(filename, "%s\\%s", analysisPath, rubber);
    status = _mkdir(filename);
#else
    sprintf(filename, "%s/%s", analysisPath, rubber);
    status = mkdir(filename, S_IRWXU);
#endif

    if (status != 0) {
        if (errno != EEXIST) {

            printf("Unable to make %s folder!\n", rubber);
            status = CAPS_IOERR;
            goto cleanup;
        }
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
    if (status != CAPS_SUCCESS) printf("Error: Premature exit in fun3d_makeDirectory status = %d\n", status);

    if (filename != NULL) EG_free(filename);

    return status;
}
