/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             XFOIL AIM
 *
 *      Written by Ryan Durscher AFLR/RQVC
 */

/*!\mainpage Introduction
 * \tableofcontents
 * \section overviewXFOIL xFoil AIM Overview
 *
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact (through input
 * files) with the subsonic airfoil analysis tool xFoil \cite Drela1989. xFoil is an open-source tool and
 * may be freely downloaded from http://web.mit.edu/drela/Public/web/xfoil/ . At this time only a subsection
 * of xFoil's capabilities are exposed through the AIM. Furthermore, only versions 6.97 and 6.99 of xFoil
 * have been tested against (for Windows only 6.99).
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsXFOIL and \ref aimOutputsXFOIL, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentXFOIL.
 *
 * Upon running preAnalysis the AIM generates two files: 1. "xfoilInput.txt" which contains instructions for
 * xFoil to execute and 2. "caps.xfoil" which contains the geometry to be analyzed.
 *
 * \section assumptionsXFOIL Assumptions
 * xFoil inherently assumes the airfoil cross-section is in the x-y plane, if it isn't an attempt is made
 * to automatically rotate the provided body.
 *
 * Within <b> OpenCSM</b>, there are a number of airfoil generation UDPs (User Defined Primitives). These include NACA 4
 * series, a more general NACA 4/5/6 series generator, Sobieczky's PARSEC parameterization and Kulfan's CST
 * parameterization. All of these UDPs generate <b> EGADS</b> <em> FaceBodies</em> where the <em>Face</em>'s underlying
 * <em>Surface</em>  is planar and the bounds of the <em>Face</em> is a closed set of <em>Edges</em> whose
 * underlying <em>Curves</em> contain the airfoil shape. In all cases there is a <em>Node</em> that represents
 * the <em>Leading Edge</em> point and one or two <em>Nodes</em> at the <em>Trailing Edge</em> -- one if the
 * representation is for a sharp TE and the other if the definition is open or blunt. If there are 2 <em>Nodes</em>
 * at the back, then there are 3 <em>Edges</em> all together and closed, even though the airfoil definition
 * was left open at the TE. All of this information will be used to automatically fill in the xFoil geometry
 * description.
 *
 * It should be noted that general construction in either <b> OpenCSM</b> or even <b> EGADS</b> will be supported
 * as long as the topology described above is used. But care should be taken when constructing the airfoil shape
 * so that a discontinuity (i.e.,  simply <em>C<sup>0</sup></em>) is not generated at the <em>Node</em> representing
 * the <em>Leading Edge</em>. This can be done by splining the entire shape as one and then intersecting the single
 *  <em>Edge</em> to place the LE <em>Node</em>.
 *
 * \section exampleXFOIL Examples
 * An example problem using the xFoil AIM may be found at \ref xfoilExample .
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
#define strtok_r   strtok_s
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define NUMINPUT   10

#define NUMOUTPUT  8

#define MAXPOINT  200

#define MXCHAR  255

//typedef struct {
//
//	// Flag to see if we need to get a CFD mesh
//	int data;
//
//} aimStorage;

//static aimStorage *xfoilInstance = NULL;
static int         numInstance  = 0;



/* ********************** Exposed AIM Functions ***************************** */
int aimInitialize(/*@unused@*/ int ngIn, /*@unused@*/ /*@null@*/ capsValue *gIn,
                  int *qeFlag, /*@null@*/ const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{

    int flag;

#ifdef DEBUG
    printf("\n xfoilAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif
    flag     = *qeFlag;
    *qeFlag  = 0;

    /* specify the number of analysis input and out "parameters" */
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
    /*! \page aimInputsXFOIL AIM Inputs
     * The following list outlines the xFoil inputs along with their default values available
     * through the AIM interface.
     */

#ifdef DEBUG
    printf(" xfoilAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
#endif

    if (index == 1) {
        *ainame           = EG_strdup("Mach");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsXFOIL
         * - <B> Mach = 0.0 </B> <br>
         *  Mach number.
         */

    } else if (index == 2) {
        *ainame              = EG_strdup("Re");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsXFOIL
         * - <B> Re = 0.0 </B> <br>
         *  Reynolds number.
         */
    } else if (index == 3) {
        *ainame           = EG_strdup("Alpha");
        defval->type      = Double;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->vals.real = 0.0;
        defval->nullVal   = IsNull;
        defval->lfixed    = Change;
        defval->units     = EG_strdup("degree");

        /*! \page aimInputsXFOIL
         * - <B> Alpha = NULL </B> <br>
         *  Angle of attack, either a single value or an array of values ( [0.0, 4.0, ...] ) may be provided [degree].
         */
    } else if (index == 4) {
        *ainame            = EG_strdup("Alpha_Increment");
        defval->type       = Double;
        defval->dim        = Vector;
        defval->length     = 3;
        defval->nrow       = 3;
        defval->ncol       = 1;
        defval->units      = NULL;
        defval->vals.reals = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals == NULL) return EGADS_MALLOC;
        defval->vals.reals[0] = 0.0;
        defval->vals.reals[1] = 0.0;
        defval->vals.reals[2] = 0.0;
        defval->nullVal    = IsNull;
        defval->lfixed     = Fixed;
        defval->units      = EG_strdup("degree");

        /*! \page aimInputsXFOIL
         * - <B> Alpha_Increment = NULL</B> <br>
         *  Angle of attack [degree] sequence - [first value, last value, increment].
         */
    }else if (index == 5) {
        *ainame           = EG_strdup("CL");
        defval->type      = Double;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->vals.real = 0.0;
        defval->lfixed    = Change;
        defval->nullVal    = IsNull;

        /*! \page aimInputsXFOIL
         * - <B> CL =  NULL</B> <br>
         *  Prescribed coefficient of lift, either a single value or an array of values ( [0.1, 0.5, ...] ) may be provided.
         */
    } else if (index == 6) {
        *ainame            = EG_strdup("CL_Increment");
        defval->type       = Double;
        defval->dim        = Vector;
        defval->length     = 3;
        defval->nrow       = 3;
        defval->ncol       = 1;
        defval->units      = NULL;
        defval->vals.reals = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals == NULL) return EGADS_MALLOC;
        defval->vals.reals[0] = 0.0;
        defval->vals.reals[1] = 0.0;
        defval->vals.reals[2] = 0.0;
        defval->nullVal    = IsNull;
        defval->lfixed     = Fixed;

        /*! \page aimInputsXFOIL
         * - <B> CL_Increment = NULL</B> <br>
         *  Prescribed coefficient of lift sequence - [first value, last value, increment].
         */
    } else if (index == 7) {
        *ainame           = EG_strdup("CL_Inviscid");
        defval->type      = Double;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->vals.real = 0.0;
        defval->lfixed    = Change;
        defval->nullVal    = IsNull;

        /*! \page aimInputsXFOIL
         * - <B> CL_Inviscid =  NULL</B> <br>
         *  Prescribed inviscid coefficient of lift, either a single value or an array of values ( [0.1, 0.5, ...] ) may be provided.
         */
    } else if (index == 8) {
        *ainame           = EG_strdup("Append_PolarFile");
        defval->type      = Boolean;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->vals.integer = (int) false;

        /*! \page aimInputsXFOIL
         * - <B> Append_PolarFile =  False</B> <br>
         *  Append the file (xfoilPolar.dat) that polar data is written to.
         */
    } else if (index == 9) {
        *ainame           = EG_strdup("Viscous_Iteration");
        defval->type      = Integer;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->lfixed    = Fixed;
        defval->vals.integer = 20;

        /*! \page aimInputsXFOIL
         * - <B> Viscous_Iteration =  20</B> <br>
         *  Viscous solution iteration limit. Only set if a Re isn't 0.0 .
         */
    } else if (index == 10) {
        *ainame           = EG_strdup("Write_Cp");
        defval->type      = Boolean;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->vals.integer = (int) false;

        /*! \page aimInputsXFOIL
         * - <B> Write_Cp =  False</B> <br>
         *  Have xFoil write the coefficient of pressure (Cp) distribution to a file - xfoilCp.dat.
         */

    }

    return CAPS_SUCCESS;
}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath, capsValue *aimInputs, capsErrs **errs)
{
    int status; // Function return status

    int i, bodyIndex; // Indexing

    // Bodies
    const char *intents;
    int numBody;
    ego *bodies = NULL;

    vlmSectionStruct vlmSection;

    // File I/O
    FILE *fp = NULL, *fpTemp = NULL;
    char inputFilename[] = "xfoilInput.txt";
    char xfoilFilename[] = "caps.xfoil";
    char polarFilename[] = "xfoilPolar.dat";
    char cpFilename[] = "xfoilCp.dat";
    char currentPath[PATH_MAX];

    // NULL out errs
    *errs = NULL;

    if (aimInputs == NULL) {
#ifdef DEBUG
        printf("\txfoilAIM/aimPreAnalysis aimInputs == NULL!\n");
#endif

        return CAPS_NULLVALUE;
    }

    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
#ifdef DEBUG
        printf("\txfoilAIM/aimPreAnalysis getBodies = %d!\n", status);
#endif
        return status;
    }

    if (numBody == 0 || bodies == NULL) {
        printf("\tError: xfoilAIM/aimPreAnalysis No Bodies!\n");
        return CAPS_SOURCEERR;
    }

    if (numBody != 1) {
        printf("\tError: Only one body should be provided to the xfoilAIM at this time!!");
        return CAPS_SOURCEERR;
    }

    status = initiate_vlmSectionStruct(&vlmSection);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Accumulate cross coordinates of airfoil and write out data file
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

        // Get where we are and set the path to our input
        (void) getcwd(currentPath, PATH_MAX);
        if (chdir(analysisPath) != 0) {
            status = CAPS_DIRERR;
            goto cleanup;
        }

        // Open and write the input to control the XFOIL session
        fp = fopen(xfoilFilename, "w");
        chdir(currentPath);
        if (fp == NULL) {
            printf("\tUnable to open file %s\n!", xfoilFilename);
            status = CAPS_IOERR;
            goto cleanup;
        }

        fprintf(fp,"capsBody_%d\n",bodyIndex+1);

        status = EG_copyObject(bodies[bodyIndex], NULL, &vlmSection.ebody);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = finalize_vlmSectionStruct(&vlmSection);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Write out the airfoil cross-section given an ego body
        status = vlm_writeSection(fp,
                                  &vlmSection,
                                  (int) false, // Normalize by chord (true/false)
                                  (int) MAXPOINT);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = destroy_vlmSectionStruct(&vlmSection);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Close file
        if (fp != NULL) {
            fclose(fp);
            fp = NULL;
        }

    } // End body loop

    // Get where we are and set the path to our input
    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) {
        status = CAPS_DIRERR;
        goto cleanup;
    }

    // Open and write the input to control the XFOIL session
    fp = fopen(inputFilename, "w");
    if (fp == NULL) {
        printf("Unable to open file %s\n!", inputFilename);
        status = CAPS_IOERR;
        chdir(currentPath);
        goto cleanup;
    }

    // Print the session file for the XFOIL run
    fprintf(fp, "PLOP\n"); // Enter PLOP
    fprintf(fp, "G F\n"); // Turn off graphics
    fprintf(fp, "\n"); // Exit PLOP

    fprintf(fp, "LOAD\n"); // Load airfoil
    fprintf(fp, "%s\n", xfoilFilename);

    fprintf(fp, "PANE\n"); // USE PANE Option to clean up airfoil mesh

    fprintf(fp, "OPER\n"); // Enter OPER

    fprintf(fp, "Mach %f\n", aimInputs[aim_getIndex(aimInfo, "Mach", ANALYSISIN)-1].vals.real); // Set Mach number

    if (aimInputs[aim_getIndex(aimInfo, "Re", ANALYSISIN)-1].vals.real > 0) {
        fprintf(fp, "Viscr\n");
        fprintf(fp,	"%f\n", aimInputs[aim_getIndex(aimInfo, "Re", ANALYSISIN)-1].vals.real); // Set Reynolds number

        fprintf(fp, "ITER\n");
        fprintf(fp,	"%d\n", aimInputs[aim_getIndex(aimInfo, "Viscous_Iteration", ANALYSISIN)-1].vals.integer); // Set iteration limit
    }

    fprintf(fp,"CINC\n"); // Turn of minimum Cp inclusion in polar data
    fprintf(fp,"PACC\n"); // Turn of polar data accmulation

    fprintf(fp,"%s\n", polarFilename);
    fprintf(fp,"\n");

    // Check to see if polar file exist
    fpTemp = fopen(polarFilename, "r");
    if( fpTemp != NULL ) { // File does exists
        fclose(fpTemp); fpTemp = NULL;

        if (aimInputs[aim_getIndex(aimInfo, "Append_PolarFile", ANALYSISIN)-1].vals.integer == (int) false) {
            status = remove(polarFilename); // Remove the file

            if(status != CAPS_SUCCESS) {
                printf("\tError: unable to delete the file - %s\n", polarFilename);
                status = CAPS_IOERR;
                goto cleanup;
            }
        } else {
            fprintf(fp,"n\n");
        }
    }

    //fprintf(fp,"xfoilPolar.dump\n");

    // Alpha
    if (aimInputs[aim_getIndex(aimInfo, "Alpha", ANALYSISIN)-1].nullVal ==  NotNull) {
        if (aimInputs[aim_getIndex(aimInfo, "Alpha", ANALYSISIN)-1].length == 1) {
            fprintf(fp, "Alfa %f\n", aimInputs[aim_getIndex(aimInfo, "Alpha", ANALYSISIN)-1].vals.real);
        } else {
            for (i = 0; i < aimInputs[aim_getIndex(aimInfo, "Alpha", ANALYSISIN)-1].length; i++) {
                fprintf(fp, "Alfa %f\n", aimInputs[aim_getIndex(aimInfo, "Alpha", ANALYSISIN)-1].vals.reals[i]);
            }
        }
    }

    // Alpha sequence
    if (aimInputs[aim_getIndex(aimInfo, "Alpha_Increment", ANALYSISIN)-1].nullVal ==  NotNull) {

        fprintf(fp, "ASeq %f %f %f\n", aimInputs[aim_getIndex(aimInfo, "Alpha_Increment", ANALYSISIN)-1].vals.reals[0],
                                       aimInputs[aim_getIndex(aimInfo, "Alpha_Increment", ANALYSISIN)-1].vals.reals[1],
                                       aimInputs[aim_getIndex(aimInfo, "Alpha_Increment", ANALYSISIN)-1].vals.reals[2]);
    }

    // CL
    if (aimInputs[aim_getIndex(aimInfo, "CL", ANALYSISIN)-1].nullVal ==  NotNull) {

        if (aimInputs[aim_getIndex(aimInfo, "CL", ANALYSISIN)-1].length == 1) {
            fprintf(fp, "CL %f\n", aimInputs[aim_getIndex(aimInfo, "CL", ANALYSISIN)-1].vals.real);
        } else {
            for (i = 0; i < aimInputs[aim_getIndex(aimInfo, "CL", ANALYSISIN)-1].length; i++) {
                fprintf(fp, "CL %f\n", aimInputs[aim_getIndex(aimInfo, "CL", ANALYSISIN)-1].vals.reals[i]);
            }
        }
    }

    // CL sequence
    if (aimInputs[aim_getIndex(aimInfo, "CL_Increment", ANALYSISIN)-1].nullVal ==  NotNull) {

        fprintf(fp, "CSeq %f %f %f\n", aimInputs[aim_getIndex(aimInfo, "CL_Increment", ANALYSISIN)-1].vals.reals[0],
                                       aimInputs[aim_getIndex(aimInfo, "CL_Increment", ANALYSISIN)-1].vals.reals[1],
                                       aimInputs[aim_getIndex(aimInfo, "CL_Increment", ANALYSISIN)-1].vals.reals[2]);
    }

    // CL Invicid
    if (aimInputs[aim_getIndex(aimInfo, "CL_Inviscid", ANALYSISIN)-1].nullVal ==  NotNull) {

        if (aimInputs[aim_getIndex(aimInfo, "CL_Inviscid", ANALYSISIN)-1].length == 1) {
            fprintf(fp, "CLI %f\n", aimInputs[aim_getIndex(aimInfo, "CL_Inviscid", ANALYSISIN)-1].vals.real);
        } else {
            for (i = 0; i < aimInputs[aim_getIndex(aimInfo, "CL_Inviscid", ANALYSISIN)-1].length; i++) {
                fprintf(fp, "CLI %f\n", aimInputs[aim_getIndex(aimInfo, "CL_Inviscid", ANALYSISIN)-1].vals.reals[i]);
            }
        }
    }

    // Write Cp distribution to a file
    if (aimInputs[aim_getIndex(aimInfo, "Write_Cp", ANALYSISIN)-1].vals.integer == (int) true) {

        fprintf(fp, "CPWR\n");
        fprintf(fp, "%s\n", cpFilename);
    }

    fprintf(fp,"\n"); // Exit OPER

    fprintf(fp, "Quit\n"); // Quit XFOIL

    fclose(fp);
    fp = NULL;
    chdir(currentPath);

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("xfoil/preAnalysis status = %d", status);
        if (fp != NULL) fclose(fp);

        return status;
}


int aimOutputs(int iIndex, void *aimStruc, int index, char **aoname, capsValue *form)
{

    /*! \page aimOutputsXFOIL AIM Outputs
     * The following list outlines the xFoil outputs available through the AIM interface.
     */

#ifdef DEBUG
    printf(" xfoilAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
#endif


    if (index == 1) {
        *aoname = EG_strdup("Alpha");

        /*! \page aimOutputsXFOIL
         * - <B> Alpha = </B> Angle of attack value(s).
         */

    } else if (index == 2) {
        *aoname = EG_strdup("CL");

        /*! \page aimOutputsXFOIL
         * - <B> CL = </B> Coefficient of lift value(s).
         */

    } else if (index == 3) {
        *aoname = EG_strdup("CD");

        /*! \page aimOutputsXFOIL
         * - <B> CD = </B> Coefficient of drag value(s).
         */

    }  else if (index == 4) {
        *aoname = EG_strdup("CD_p");

        /*! \page aimOutputsXFOIL
         * - <B> CD_p = </B> Coefficient of drag value(s), pressure contribution.
         */

    }  else if (index == 5) {
        *aoname = EG_strdup("CM");

        /*! \page aimOutputsXFOIL
         * - <B> CM = </B> Moment coefficient value(s).
         */

    } else if (index == 6) {
        *aoname = EG_strdup("Cp_Min");

        /*! \page aimOutputsXFOIL
         * - <B> Cp_Min = </B> Minimum coefficient of pressure value(s).
         */
    } else if (index == 7) {
        *aoname = EG_strdup("Transition_Top");

        /*! \page aimOutputsXFOIL
         * - <B> Transition_Top = </B> Value(s) of x- transition location on the top of the airfoil.
         */
    } else if (index == 8) {
        *aoname = EG_strdup("Transition_Bottom");

        /*! \page aimOutputsXFOIL
         * - <B> Transition_Bottom = </B> Value(s) of x- transition location on the bottom of the airfoil.
         */
    }

    form->type   = Double;
    form->dim    = Vector;
    form->length = 1;
    form->nrow   = 1;
    form->ncol   = 1;
    form->units  = NULL;
    form->lfixed = Change;

    form->vals.reals = NULL;
    form->vals.real = 0;

    return CAPS_SUCCESS;
}


int aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath,
        int index, capsValue *val, capsErrs **errors)
{
    int status; // Function return status
    int i, j, k; // Indexing
    int skipLine = (int) false;
    int valCount = 0; // Number of values in return array

    int valIndex = 0;

    // excess data entries in case future versions of xfoil change
#define MAX_DATA_ENTRY 20
    int numDataEntry = 0;
    double dataLine[MAX_DATA_ENTRY];

    size_t     linecap = 0;
    char       currentPath[PATH_MAX], *line = NULL, *rest = NULL, *token = NULL;
    char       headers[MAX_DATA_ENTRY][20];
    const char *valHeader;
    FILE       *fp;

    *errors        = NULL;

    if (val->length > 1) {
        if (val->vals.reals != NULL) EG_free(val->vals.reals);
        val->vals.reals = NULL;
    } else {
        val->vals.real = 0.0;
    }

    val->nrow = 1;
    val->ncol = 1;
    val->length = val->nrow*val->ncol;

    // Get where we are and set the path to our output
    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) {

#ifdef DEBUG
        printf(" xfoilAIM/aimCalcOutput Cannot chdir to %s!\n", analysisPath);
#endif

        return CAPS_DIRERR;
    }

    // Open the XFOIL output file
    fp = fopen("xfoilPolar.dat", "r");
    chdir(currentPath);
    if (fp == NULL) return CAPS_IOERR;

    // Move to beginning of data
    for (i = 0; i < 11; i++) {
        status = getline(&line, &linecap, fp);
        if (status < 0){
            status = CAPS_IOERR;
            goto cleanup;
        }
    }

    // Parse the header information,
    // both headers can come from xfoil 6.99 (lax version control), i.e.
    //    alpha    CL        CD       CDp       CM      Cpmin   XCpmin   Top_Xtr  Bot_Xtr
    // or
    //    alpha    CL        CD       CDp       CM      Cpmin   Top_Xtr  Bot_Xtr  Top_Itr  Bot_Itr
    rest = line;

    numDataEntry = 0;
    while ((token = strtok_r(rest, " ", &rest))) {
        strcpy(headers[numDataEntry], token);
        //printf("%s\n", headers[numDataEntry]);
        numDataEntry++;
        if (numDataEntry == MAX_DATA_ENTRY) {
            printf("More than %d columns in xfoilPolar.dat is not expected!\n", numDataEntry);
            status = CAPS_IOERR;
            goto cleanup;
        }
    }

    // line skip the heading just above the data, i.e. ---- ---- ----...
    status = getline(&line, &linecap, fp);
    if (status < 0){
        status = CAPS_IOERR;
        goto cleanup;
    }

    // Headers expected in xfoilPolar.dat that correspond to the AIM output names
         if (index == 1)
        valHeader = "alpha";
    else if (index == 2)
        valHeader = "CL";
    else if (index == 3)
        valHeader = "CD";
    else if (index == 4)
        valHeader = "CDp";
    else if (index == 5)
        valHeader = "CM";
    else if (index == 6)
        valHeader = "Cpmin";
    else if (index == 7)
        valHeader = "Top_Xtr";
    else if (index == 8)
        valHeader = "Bot_Xtr";
    else {
        printf("Developer error: Unkown variable index %d", index);
        status = CAPS_BADINDEX;
        goto cleanup;
    }

    // Find which column contains the requested data
    valIndex = 0;
    while( valIndex < numDataEntry && strncmp(headers[valIndex], valHeader, strlen(valHeader)) != 0 ) valIndex++;
    if (valIndex == numDataEntry) {
        printf("Could not find '%s' header in xfoilPolar.dat\n", valHeader);
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Count how many line of data we have
    valCount = 0;
    while (getline(&line, &linecap, fp) >= 0) valCount += 1;

    // Reset file
    rewind(fp);

    if (valCount == 0) {
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    if (valCount > 1) {
        val->vals.reals = (double *) EG_alloc(valCount*sizeof(double));
        if (val->vals.reals == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }
    }

    // Move back to the beginning of data
    for (i = 0; i < 12; i++) {
        (void) getline(&line, &linecap, fp);
    }

    k = 0;
    for (j = 0; j < valCount; j++) {
        skipLine = (int) false;

        for (i = 0; i < numDataEntry; i++) {
            status = fscanf(fp, "%lf", &dataLine[i]);

            if (status == CAPS_SUCCESS) {
                printf("Invalid data line, %d, skipping data entry!\n", j+1);
                (void) getline(&line, &linecap, fp);

                skipLine = (int) true;
                break;
            }
        }

        if (skipLine == (int) true) continue;

        if (valCount == 1){
            val->vals.real = dataLine[valIndex];
        } else {
            //printf("k = %d, Val (%d) - %f\n", k, valIndex,dataLine[valIndex]);
            val->vals.reals[k] = dataLine[valIndex];
        }
        k += 1;
    }

    if (k != valCount) { // Realloc our array - shorten it
        val->vals.reals = (double *) EG_reall(val->vals.reals,
                k*sizeof(double));
        if (val->vals.reals == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }
    }

    valCount = k;
    val->dim    = Vector;
    val->length = valCount;
    val->nrow   = valCount;
    val->ncol   = 1;

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (fp != NULL) fclose(fp);

        if (line != NULL) EG_free(line);

        return status;
}

void aimCleanup()
{

#ifdef DEBUG
    printf(" xfoilAIM/aimCleanup!\n");
#endif

    numInstance = 0;
}
