/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             TSFOIL AIM
 *
 *      Written by Ryan Durscher AFLR/RQVC
 */

/*!\mainpage Introduction
 * \tableofcontents
 * \section overviewTSFOIL TSFOIL AIM Overview
 *
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact (through input
 * files) with the transonic airfoil analysis tool TSFOIL. TSFOIL can be downloaded from
 * http://www.dept.aoe.vt.edu/~mason/Mason_f/MRsoft.html .
 *
 * Note: In the tsfoil2.f file is may be necessary to comment out line 38 - "USE DFPORT"
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsTSFOIL and \ref aimOutputsTSFOIL, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentTSFOIL.
 *
 * Upon running preAnalysis the AIM generates two files: 1. "tsfoilInput.txt" which contains instructions for
 * TSFOIL to execute and 2. "caps.tsfoil" which contains the geometry to be analyzed.
 *
 * \section assumptionsTSFOIL Assumptions
 * TSFOIL inherently assumes the airfoil cross-section is in the x-y plane, if it isn't an attempt is made
 * to automatically rotate the provided body.
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "aimUtil.h"

#include "miscUtils.h" // Bring in miscellaneous utilities
#include "meshUtils.h"
#include "vlmUtils.h"

#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define NUMINPUT   3

#define NUMOUTPUT  5

#define MXCHAR  255

//typedef struct {
//
//	// Flag to see if we need to get a CFD mesh
//	int data;
//
//} aimStorage;

//static aimStorage *tsfoilInstance = NULL;
static int         numInstance  = 0;



/* ********************** Exposed AIM Functions ***************************** */
int aimInitialize(/*@unused@*/ int ngIn, /*@unused@*/ /*@null@*/ capsValue *gIn,
                  int *qeFlag, /*@null@*/ const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{

    int flag;

#ifdef DEBUG
    printf("\n tsfoilAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif

    flag     = *qeFlag;
    *qeFlag  = 0;

    // Specify the number of analysis input and out "parameters"
    *nIn     = NUMINPUT;
    *nOut    = NUMOUTPUT;

    if (flag == 1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate */
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Increment number of instances
    numInstance += 1;

    return (numInstance -1);
}


int aimInputs( int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{
    /*! \page aimInputsTSFOIL AIM Inputs
     * The following list outlines the TSFOIL inputs along with their default values available
     * through the AIM interface.
     */

#ifdef DEBUG
    printf(" tsfoilAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
#endif

    if (index == 1) {
        *ainame           = EG_strdup("Mach");
        defval->type      = Double;
        defval->vals.real = 0.75;
        defval->units     = NULL;
        defval->limits.dlims[0] = 0.5; // Limits on Mach for TSFOIL
        defval->limits.dlims[1] = 2.0;

        /*! \page aimInputsTSFOIL
         * - <B> Mach = 0.75 </B> <br>
         *  Mach number. Valid range for TSFOIL is 0.5 to 2.0 .
         */

    } else if (index == 2) {
        *ainame              = EG_strdup("Re");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsTSFOIL
         * - <B> Re = 0.0 </B> <br>
         *  Reynolds number based on chord length.
         */
    } else if (index == 3) {
        *ainame           = EG_strdup("Alpha");
        defval->type      = Double;
        defval->lfixed    = Fixed;
        defval->units     = EG_strdup("degree");

        /*! \page aimInputsTSFOIL
         * - <B> Alpha = 0.0 </B> <br>
         *  Angle of attack [degree].
         */
    }


    return CAPS_SUCCESS;
}


int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath, capsValue *aimInputs, capsErrs **errs)
{
    int status; // Function return status

    int i, bodyIndex, loopIndex, edgeIndex; // Indexing

    // Bodies
    const char *intents;
    int numBody;
    ego *bodies = NULL;

    // Topology
    int numLoop, numEdge;
    ego *loops = NULL, *edges = NULL, geom;
    int oclass, mtype;
    //double uvbox[4];
    int *edgeSense = NULL;

    // Attribute to index map
    //mapAttrToIndexStruct attrMap;

    // Tessellation
    double size, params[3], paramsScaled[3];
    int numEdgePoint = 40;
    double rPos[40]; //numEdgePoint
    ego egadsTess;
    int numPoints;
    const double *points = NULL, *uv = NULL;
    double  box[6];

    // Proper orientation
    int xMeshConstant = (int) true, yMeshConstant = (int) true, zMeshConstant= (int) true;
    int swapZX = (int) false, swapZY = (int) false;

    // File I/O
    FILE *fp = NULL;
    char inputFilename[] = "tsfoilInput.txt";
    char outputFilename[] = "tsfoilOutput.txt";
    char tsfoilFilename[] = "caps.tsfoil";

    char currentPath[PATH_MAX];

    // NULL out errs
    *errs = NULL;

    if (aimInputs == NULL) {
#ifdef DEBUG
        printf("\ttsfoilAIM/aimPreAnalysis aimInputs == NULL!\n");
#endif

        return CAPS_NULLVALUE;
    }

    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
#ifdef DEBUG
        printf("\ttsfoilAIM/aimPreAnalysis getBodies = %d!\n", status);
#endif
        return status;
    }

    if (numBody == 0 || bodies == NULL) {
        printf("\tError: tsfoilAIM/aimPreAnalysis No Bodies!\n");
        return CAPS_SOURCEERR;
    }

    if (numBody != 1) {
        printf("\tError: Only one body should be provided to the tsfoilAIM at this time!!");
        return CAPS_SOURCEERR;
    }

    // Setup even distribution for grid points along edge
    for (i = 0; i < numEdgePoint; i++) {
        rPos[i] = (double) (i+1) / (double)(numEdgePoint-1);
    }

    //  Loop through bodies and set spacing
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

        // Get edges on the body - there should only be 2 - checked later
        status = EG_getBodyTopos(bodies[bodyIndex], NULL, EDGE, &numEdge, &edges);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (numEdge != 2) {
            if (edges != NULL) EG_free(edges);
            edges = NULL;
            printf("\tError: The airfoil cross-section (of body %d) should consist of two edges!\n", bodyIndex+1);
            goto cleanup;
        }

        for (i = 0; i < numEdge; i++) {
            status = EG_attributeAdd(edges[i],
                    ".rPos", ATTRREAL, numEdgePoint-2, NULL, rPos, NULL);
        }

        if (edges != NULL) EG_free(edges);
        edges = NULL;

        if (status != EGADS_SUCCESS) goto cleanup;
    }

    // Get where we are and set the path to our input
    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) {
        status = CAPS_DIRERR;
        goto cleanup;
    }

    // Open and write the input to control the TSFOIL session
    fp = fopen(tsfoilFilename, "w");
    chdir(currentPath);
    if (fp == NULL) {
        printf("\tUnable to open file %s\n!", tsfoilFilename);
        status = CAPS_IOERR;
        goto cleanup;
    }

    fprintf(fp,"CAPS generated TSFOIL input\n");
    fprintf(fp,"$INP\n"); // Start input namelist

    fprintf(fp, "EMACH=%f\n", aimInputs[aim_getIndex(aimInfo, "Mach", ANALYSISIN)-1].vals.real);

    fprintf(fp, "DELTA=0.115\n");

    fprintf(fp, "ALPHA=%f\n", aimInputs[aim_getIndex(aimInfo, "Alpha", ANALYSISIN)-1].vals.real);

    fprintf(fp, "GAM=1.4\n");

    fprintf(fp, "WE=1.8,1.9,1.95\n");
    fprintf(fp, "EPS=0.2\n");

    fprintf(fp, "AMESH=T\n");

    fprintf(fp, "RIGF=0.0\n");
    fprintf(fp, "CVERGE=0.00001\n");
    fprintf(fp, "BCFOIL=4\n");
    fprintf(fp, "MAXIT=800\n");

    if (aimInputs[aim_getIndex(aimInfo, "Re", ANALYSISIN)-1].vals.real > 0) {
        fprintf(fp, "REYNLD=%e\n", aimInputs[aim_getIndex(aimInfo, "Re", ANALYSISIN)-1].vals.real);
    }

    fprintf(fp, "$END\n"); // End input namelist

    params[0] = 0.025;
    params[1] = 0.001;
    params[2] = 15.00;

    // Accumulate cross coordinates of airfoil and write out data file
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

        // Reset orientation information for each body
        xMeshConstant = (int) true;
        yMeshConstant = (int) true;
        zMeshConstant= (int) true;
        swapZX = (int) false;
        swapZY = (int) false;

        fprintf(fp,"capsBody_%d\n",bodyIndex+1);
        fprintf(fp,"%10.5f%10.5f%10.5f\n", 0.0, (float) numEdgePoint, (float) numEdgePoint);

        // Check for x-y plane data
        status = EG_getBoundingBox(bodies[bodyIndex], box);
        if (status != EGADS_SUCCESS) goto cleanup;

        // Constant x?
        if (box[3] == box[0]) xMeshConstant = (int) false; // This is not that robust - may cause issues in the future

        // Constant y?
        if (box[4] == box[1]) yMeshConstant = (int) false;

        // Constant z?
        if (box[5] == box[2]) zMeshConstant = (int) false;

        //for (i = 0; i < 6; i++) printf("Box[%d] = %f\n", i, box[i]);

        if (zMeshConstant != (int) true) {
            printf("TSFOIL expects airfoil cross sections to be in the x-y plane... attempting to rotate body %d!\n", bodyIndex);

            if (xMeshConstant == (int) true && yMeshConstant == (int) false) {
                printf("\tSwapping z and x coordinates!\n");
                swapZX = (int) true;

            } else if(xMeshConstant == (int) false && yMeshConstant == (int) true) {
                printf("\tSwapping z and y coordinates!\n");
                swapZY = (int) true;

            } else {
                printf("\tUnable to rotate mesh!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        // Get tessellation size
        size = sqrt((box[0]-box[3])*(box[0]-box[3]) +
                    (box[1]-box[4])*(box[1]-box[4]) +
                    (box[2]-box[5])*(box[2]-box[5]));

        paramsScaled[0] = params[0]*size;
        paramsScaled[1] = params[1]*size;
        paramsScaled[2] = params[2];

        // Make tessellation
        status = EG_makeTessBody(bodies[bodyIndex], paramsScaled, &egadsTess);
        if (status != EGADS_SUCCESS) {
            printf("\tProblem during tessellation of body %d\n", bodyIndex+1);
            goto cleanup;
        }

        // Get loops on the body - there should only be 1
        status = EG_getBodyTopos(bodies[bodyIndex], NULL, LOOP, &numLoop, &loops);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (numLoop != 1) {
            printf("\tError: The number of loops on body %d is more than 1!\n", bodyIndex+1);
            goto cleanup;
        }

        for (loopIndex = 0; loopIndex < numLoop; loopIndex++) {

            // Get edges in loop
            status = EG_getTopology(loops[loopIndex], &geom, &oclass, &mtype,
                                    NULL, &numEdge, &edges, &edgeSense);
            if (status != EGADS_SUCCESS) {
                printf("\tEG_getTopology status = %d\n",status);
                goto cleanup;
            }

            if (numEdge != 2) {
                printf("\tError: The airfoil cross-section (of body %d) should consist of two edges!\n", bodyIndex+1);
                goto cleanup;
            }

            // Loop through edges on the loop
            for (edgeIndex = 0; edgeIndex < numEdge; edgeIndex++) {

                fprintf(fp,"Edge_%d\n", edgeIndex+1);

                // The particular body relative edge index
                //printf("edgeIndex = %d\n", edgeIndex);
                i = EG_indexBodyTopo(bodies[bodyIndex], edges[edgeIndex]);
                if (i < EGADS_SUCCESS) {
                    status = CAPS_BADINDEX;
                    goto cleanup;
                }

                // Get the edge tessellation
                //printf("EdgeBodyIndex = %d %d\n", i, edgeSense[edgeIndex]);
                status = EG_getTessEdge(egadsTess, i, &numPoints, &points, &uv);
                if (status != EGADS_SUCCESS) {
                    printf("\tEG_getTessEdge status = %d\n",status);
                    goto cleanup;
                }

                if (edgeIndex == 1) { // if (edgeSense[edgeIndex] > 0) {
                    for (i = 0; i < numPoints; i++) {

                        if (swapZX == (int) true) {
                            // x = z
                            fprintf(fp, "%10.5f%10.5f\n", points[3*i + 2],
                                    points[3*i + 1]);

                        } else if (swapZY == (int) true) {
                            // y = z
                            fprintf(fp, "%10.5f%10.5f\n", points[3*i + 0],
                                    points[3*i + 2]);
                        } else {
                            fprintf(fp, "%10.5f%10.5f\n", points[3*i + 0],
                                    points[3*i + 1]);
                        }
                    }

                } else {

                    for (i = numPoints-1; i >= 0; i--) {

                        if (swapZX == (int) true) {
                            // x = z
                            fprintf(fp, "%10.5f%10.5f\n", points[3*i + 2],
                                    points[3*i + 1]);

                        } else if (swapZY == (int) true) {
                            // y = z
                            fprintf(fp, "%10.5f%10.5f\n", points[3*i + 0],
                                    points[3*i + 2]);
                        } else {
                            fprintf(fp, "%10.5f%10.5f\n", points[3*i + 0],
                                    points[3*i + 1]);
                        }
                    }
                } // End sense loop
            } // End edge loop
        } // End loop loop

        // Close file
        if (fp != NULL) {
            fclose(fp);
            fp = NULL;
        }

        // Free loop
        if (loops != NULL) EG_free(loops);
        loops = NULL;

    } // End body loop

    // Get where we are and set the path to our input
    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) {
        status = CAPS_DIRERR;
        goto cleanup;
    }

    // Open and write the input to control the TSFOIL session
    fp = fopen(inputFilename, "w");
    if (fp == NULL) {
        printf("Unable to open file %s\n!", inputFilename);
        status = CAPS_IOERR;
        chdir(currentPath);
        goto cleanup;
    }

    // Print the session file for the TSFOIL run
    fprintf(fp, "default\n");            // Set directory
    fprintf(fp, "%s\n", outputFilename); // Set outputFilename
    fprintf(fp, "%s\n", tsfoilFilename); // Set inputfile
    fprintf(fp, "\n"); // Exit
    fprintf(fp, "\n"); // 3 times
    fprintf(fp, "\n"); // just to be safe

    fclose(fp);
    fp = NULL;
    chdir(currentPath);

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("tsfoil/preAnalysis status = %d", status);
        if (fp != NULL) fclose(fp);

        if (loops != NULL) EG_free(loops);

        return status;
}


int aimOutputs(int iIndex, void *aimStruc, int index, char **aoname, capsValue *form)
{

    /*! \page aimOutputsTSFOIL AIM Outputs
     * The following list outlines the TSFOIL outputs available through the AIM interface.
     */

#ifdef DEBUG
    printf(" tsfoilAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
#endif


    if (index == 1) {
        *aoname = EG_strdup("CL");

        /*! \page aimOutputsTSFOIL
         * - <B> CL = </B> Coefficient of lift value.
         */

    } else if (index == 2) {
        *aoname = EG_strdup("CD");

        /*! \page aimOutputsTSFOIL
         * - <B> CD = </B> Coefficient of drag value. (Calculated from momentum integral)
         */

    } else if (index == 3) {
        *aoname = EG_strdup("CD_Wave");

        /*! \page aimOutputsTSFOIL
         * - <B> CD_Wave = </B> Wave drag coefficient value.
         */

    }  else if (index == 4) {
        *aoname = EG_strdup("CM");

        /*! \page aimOutputsTSFOIL
         * - <B> CM = </B> Moment coefficient value.
         */

    } else if (index == 5) {
        *aoname = EG_strdup("Cp_Critical");

        /*! \page aimOutputsTSFOIL
         * - <B> Cp_Critical = </B> Critical pressure coefficient (M = 1).
         */
    }

    form->type   = Double;
    form->dim    = Vector;
    form->length = 1;
    form->nrow   = 1;
    form->ncol   = 1;
    form->units  = NULL;
    form->lfixed = Fixed;

    form->vals.reals = NULL;
    form->vals.real = 0;

    return CAPS_SUCCESS;
}


int aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath, int index, capsValue *val, capsErrs **errors)
{
    int status; // Function return status

    size_t     linecap = 0;
    char       currentPath[PATH_MAX], *line = NULL;
    FILE       *fp;

    //char beginData[] = "                     FINAL MESH";

    char *valstr = NULL;

    *errors        = NULL;

    // Get where we are and set the path to our output
    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) {

#ifdef DEBUG
        printf(" tsfoilAIM/aimCalcOutput Cannot chdir to %s!\n", analysisPath);
#endif

        return CAPS_DIRERR;
    }

    // open the TSFOIL output file
    fp = fopen("tsfoilOutput.txt", "r");
    chdir(currentPath);
    if (fp == NULL) return CAPS_DIRERR;

    status = CAPS_NOTFOUND;
    while (getline(&line, &linecap, fp) >= 0) {

        valstr = NULL;

        if (index == aim_getIndex(aimInfo, "CL", ANALYSISOUT) ||
            index == aim_getIndex(aimInfo, "CM", ANALYSISOUT) ||
            index == aim_getIndex(aimInfo, "Cp_Critical", ANALYSISOUT)) {

            valstr = strstr(line, "FINAL MESH"); // Look for final mesh values
            if (valstr != NULL) {

                status = getline(&line, &linecap, fp);
                if (status < 0) {
                    status = CAPS_NOTFOUND;
                    goto cleanup;
                }


                if (index == aim_getIndex(aimInfo, "CL", ANALYSISOUT)) { // Get CL variable

                    valstr = strstr(line, "CL =");

                    sscanf(&valstr[5], "%lf", &val->vals.real);
                    status = CAPS_SUCCESS;
                    break;
                }

                status = getline(&line, &linecap, fp); // Skip to next line
                if (status < 0) {
                    status = CAPS_NOTFOUND;
                    goto cleanup;
                }

                if (index == aim_getIndex(aimInfo, "CM", ANALYSISOUT)) { // Get CM variable

                    valstr = strstr(line, "CM =");

                    sscanf(&valstr[5], "%lf", &val->vals.real);
                    status = CAPS_SUCCESS;
                    break;
                }

                status = getline(&line, &linecap, fp); // Skip to next line
                if (status < 0) {
                    status = CAPS_NOTFOUND;
                    goto cleanup;
                }

                if (index == aim_getIndex(aimInfo, "Cp_Critical", ANALYSISOUT)) { // Get Cp critical

                    valstr = strstr(line, "CP* =");
                    sscanf(&valstr[5], "%lf", &val->vals.real);
                    status = CAPS_SUCCESS;
                    break;
                }

            }
        }

        if (index == aim_getIndex(aimInfo, "CD_Wave", ANALYSISOUT)) { // Get wave drag
            valstr = strstr(line, "TOTAL CDWAVE =");

            if (valstr != NULL) {
                sscanf(&valstr[15], "%lf", &val->vals.real);
                status = CAPS_SUCCESS;
                break;
            }
        }

        if (index == aim_getIndex(aimInfo, "CD", ANALYSISOUT)) { //Get drag value
            valstr = strstr(line, "CD     =");

            if (valstr != NULL) {
                sscanf(&valstr[8], "%lf", &val->vals.real);
                status = CAPS_SUCCESS;
                break;
            }
        }

    }

    goto cleanup;

    cleanup:
        if (fp != NULL) fclose(fp);
        if (line != NULL) EG_free(line);

        return status;
}

void aimCleanup()
{

#ifdef DEBUG
    printf(" tsfoilAIM/aimCleanup!\n");
#endif

    numInstance = 0;

}
