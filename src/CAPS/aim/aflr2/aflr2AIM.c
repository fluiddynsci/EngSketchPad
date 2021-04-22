/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             AFLR2 AIM
 *
 *      Modified from code written by Dr. Ryan Durscher AFRL/RQVC
 *
 */

/*!\mainpage Introduction
 *
 * \section overviewAFLR2 AFLR2 AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact with the
 * unstructured, surface grid generator AFLR2 \cite Marcum1995 \cite Marcum1998.
 *
 * The AFLR2 AIM provides the CAPS users with the ability to generate "unstructured, 2D grids" using an
 * "Advancing-Front/Local-Reconnection (AFLR) procedure." Both triangular and quadrilateral elements may be generated.
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsAFLR2 and \ref aimOutputsAFLR2, respectively.
 * The complete AFLR documentation is available at the <a href="http://www.simcenter.msstate.edu/software/downloads/doc/system/index.html">SimCenter</a>.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentAFLR2.
 */

#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"       // Collection of helper functions for meshing
#include "cfdUtils.h"        // Collection of helper functions for cfd analysis
#include "miscUtils.h"       // Collection of helper functions for miscellaneous analysis
#include "aflr2_Interface.h" // Bring in AFLR2 'interface' functions

#ifdef WIN32
#define getcwd     _getcwd
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define NUMINPUT  9       // Number of mesh inputs
#define NUMOUT    1       // Number of outputs

#define MXCHAR  255

//#define DEBUG

typedef struct {

    // Container for surface mesh
    int numSurface;
    meshStruct *surfaceMesh;

    // Container for mesh input
    meshInputStruct meshInput;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

} aimStorage;

static aimStorage *aflr2Instance = NULL;
static int         numInstance  = 0;

static int destroy_aimStorage(int iIndex) {

    int i; // Indexing

    int status; // Function return status

    // Destroy meshInput
    status = destroy_meshInputStruct(&aflr2Instance[iIndex].meshInput);
    if (status != CAPS_SUCCESS) printf("Status = %d, aflr2AIM instance %d, meshInput cleanup!!!\n", status, iIndex);

    // Destroy surface mesh allocated arrays
    for (i = 0; i < aflr2Instance[iIndex].numSurface; i++) {

        status = destroy_meshStruct(&aflr2Instance[iIndex].surfaceMesh[i]);
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr2AIM instance %d, surfaceMesh cleanup!!!\n", status, iIndex);

    }
    aflr2Instance[iIndex].numSurface = 0;

    if (aflr2Instance[iIndex].surfaceMesh != NULL) EG_free(aflr2Instance[iIndex].surfaceMesh);
    aflr2Instance[iIndex].surfaceMesh = NULL;

    // Destroy attribute to index map
    status = destroy_mapAttrToIndexStruct(&aflr2Instance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Status = %d, aflr2AIM instance %d, attributeMap cleanup!!!\n", status, iIndex);

    return CAPS_SUCCESS;
}


#ifdef WIN32
    #if  _MSC_VER >= 1900
    // FILE _iob[] = {*stdin, *stdout, *stderr};
    FILE _iob[3];

    extern FILE * __cdecl __iob_func(void)
    {
        return _iob;
    }
    #endif
#endif

/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(/*@unused@*/ int ngIn, /*@unused@*/ capsValue *gIn,
                  int *qeFlag, /*@null@*/ const char *unitSys, int *nIn,
                  int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int flag;  // query/execute flag
    aimStorage *tmp;

    #ifdef WIN32
        #if  _MSC_VER >= 1900
            _iob[0] = *stdin;
            _iob[1] = *stdout;
            _iob[2] = *stderr;
        #endif
    #endif

    int status; // Function return

    #ifdef DEBUG
        printf("\n aflr2AIM/aimInitialize   ngIn = %d!\n", ngIn);
    #endif
    flag    = *qeFlag;
    // Set that the AIM executes itself
    *qeFlag = 1;

    /* specify the number of analysis input and out "parameters" */
    *nIn     = NUMINPUT;
    *nOut    = NUMOUT;
    if (flag == 1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate */
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Allocate aflrInstance
    if (aflr2Instance == NULL) {
        aflr2Instance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (aflr2Instance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(aflr2Instance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        aflr2Instance = tmp;
    }

    // Set initial values for aflrInstance //

    // Container for surface meshes
    aflr2Instance[numInstance].surfaceMesh = NULL;
    aflr2Instance[numInstance].numSurface = 0;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&aflr2Instance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for mesh input
    status = initiate_meshInputStruct(&aflr2Instance[numInstance].meshInput);
    if (status != CAPS_SUCCESS) return status;

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}

int aimInputs(/*@unused@*/ int iIndex, void *aimInfo, int index, char **ainame,
              capsValue *defval)
{

    /*! \page aimInputsAFLR2 AIM Inputs
     * The following list outlines the AFLR2 meshing options along with their default value available
     * through the AIM interface.
     */

    #ifdef DEBUG
        printf(" aflr2AIM/aimInputs instance = %d  index = %d!\n", inst, index);
    #endif

    // Meshing Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name"); // If NULL a volume grid won't be written by the AIM
        defval->type         = String;
        defval->nullVal      = IsNull;
        defval->vals.string  = NULL;
        //defval->vals.string  = EG_strdup("CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsAFLR2
         * - <B> Proj_Name = NULL</B> <br>
         * This corresponds to the output name of the mesh. If left NULL, the mesh is not written to a file.
         */

    } else if (index == 2) {
        *ainame               = EG_strdup("Tess_Params");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->length        = 3;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->lfixed        = Fixed;
        defval->vals.reals    = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals != NULL) {
            defval->vals.reals[0] = 0.025;
            defval->vals.reals[1] = 0.001;
            defval->vals.reals[2] = 15.00;
        } else return EGADS_MALLOC;

        /*! \page aimInputsAFLR2
         * - <B> Tess_Params = [0.025, 0.001, 15.0]</B> <br>
         * Body tessellation parameters. Tess_Params[0] and Tess_Params[1] get scaled by the bounding
         * box of the body. (From the EGADS manual) A set of 3 parameters that drive the EDGE discretization
         * and the FACE triangulation. The first is the maximum length of an EDGE segment or triangle side
         * (in physical space). A zero is flag that allows for any length. The second is a curvature-based
         * value that looks locally at the deviation between the centroid of the discrete object and the
         * underlying geometry. Any deviation larger than the input value will cause the tessellation to
         * be enhanced in those regions. The third is the maximum interior dihedral angle (in degrees)
         * between triangle facets (or Edge segment tangents for a WIREBODY tessellation), note that a
         * zero ignores this phase.
         */

    } else if (index == 3) {
        *ainame               = EG_strdup("Mesh_Quiet_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = false;

        /*! \page aimInputsAFLR2
         * - <B> Mesh_Quiet_Flag = False</B> <br>
         * Complete suppression of mesh generator (not including errors)
         */

    } else if (index == 4) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->vals.string   = EG_strdup("AFLR3"); // TECPLOT, VTK, AFLR3, STL, FAST
        defval->lfixed        = Change;

        /*! \page aimInputsAFLR2
         * - <B> Mesh_Format = "AFLR3"</B> <br>
         * Mesh output format. Available format names include: "AFLR3", "VTK", "TECPLOT", "STL" (quadrilaterals will be
         * split into triangles), "FAST".
         */

    } else if (index == 5) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsAFLR2
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file if applicable.
         */

    } else if (index == 6) {
        *ainame               = EG_strdup("Mesh_Gen_Input_String");
        defval->type          = String;
        defval->nullVal       = IsNull;
        defval->vals.string   = NULL;

        /*! \page aimInputsAFLR2
         * - <B> Mesh_Gen_Input_String = NULL</B> <br>
         * Meshing program command line string (as if called in bash mode). Use this to specify more
         * complicated options/use features of the mesher not currently exposed through other AIM input
         * variables. Note that this is the exact string that will be provided to the volume mesher; no
         * modifications will be made. If left NULL an input string will be created based on default values
         * of the relevant AIM input variables.
         */

    } else if (index == 7) {
        *ainame               = EG_strdup("Edge_Point_Min");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsAFLR2
         * - <B> Edge_Point_Min = NULL</B> <br>
         * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 8) {
        *ainame               = EG_strdup("Edge_Point_Max");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->length        = 1;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsAFLR2
         * - <B> Edge_Point_Max = NULL</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 9) {
        *ainame              = EG_strdup("Mesh_Sizing");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsAFLR2
         * - <B>Mesh_Sizing = NULL </B> <br>
         * See \ref meshSizingProp for additional details.
         */
    }

#if NUMINPUT != 9
#error "NUMINPUTS is inconsistent with the list of inputs"
#endif

    /*if (index != 1 && index != 6){
        // Link variable(s) to parent(s) if available
        status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
        //printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n", status, index, defval->type, defval->link);
    }*/

    return CAPS_SUCCESS;
}

int aimData(int iIndex, const char *name, enum capsvType *vtype,
            int *rank, int *nrow, int *ncol, void **data, char **units)
{
    /*! \page sharableDataAFLR2 AIM Shareable Data
     * The AFLR2 AIM has the following shareable data types/values with its children AIMs if they are so inclined.
     * - <B> Surface_Mesh</B> <br>
     * The returned surface mesh after AFLR2 execution is complete in meshStruct (see meshTypes.h) format.
     * - <B> Attribute_Map</B> <br>
     * An index mapping between capsGroups found on the geometry in mapAttrToIndexStruct (see miscTypes.h) format.
     */

    #ifdef DEBUG
        printf(" aflr2AIM/aimData instance = %d  name = %s!\n", inst, name);
    #endif

    // The returned surface mesh from aflr2

    if (strcasecmp(name, "Surface_Mesh") == 0){
        *vtype = Value;
        *rank = *ncol = 1;
        *nrow  = aflr2Instance[iIndex].numSurface;
        *data  = aflr2Instance[iIndex].surfaceMesh;
        *units = NULL;

        return CAPS_SUCCESS;
    }

    // Share the attribute map
    if (strcasecmp(name, "Attribute_Map") == 0){
         *vtype = Value;
         *rank  = *nrow = *ncol = 1;
         *data  = &aflr2Instance[iIndex].attrMap;
         *units = NULL;

         return CAPS_SUCCESS;
    }

    return CAPS_NOTFOUND;

}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath, capsValue *aimInputs, capsErrs **errs)
{
    int status; // Status return

    int i, bodyIndex; // Indexing

    int Message_Flag;

    // Body parameters
    const char *intents;
    int numBody = 0; // Number of bodies
    ego *bodies = NULL; // EGADS body objects
    double refLen = -1.0;

    // Global settings
    int minEdgePoint = -1, maxEdgePoint = -1;

    // Mesh attribute parameters
    int numMeshProp = 0;
    meshSizingStruct *meshProp = NULL;

    // File output
    char *filename = NULL;
    char bodyNumber[11];

    // NULL out errors
    *errs = NULL;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    #ifdef DEBUG
        printf(" aflr2AIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, numBody);
    #endif

    if (numBody <= 0 || bodies == NULL) {
        #ifdef DEBUG
            printf(" aflr2AIM/aimPreAnalysis No Bodies!\n");
        #endif
        return CAPS_SOURCEERR;
    }

    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the same instance
    status = destroy_aimStorage(iIndex);
    if (status != CAPS_SUCCESS) {
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr2AIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
        return status;
    }

    // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
    status = create_CAPSGroupAttrToIndexMap(numBody,
                                            bodies,
                                            2, // Edge level
                                            &aflr2Instance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Allocate surfaceMesh from number of bodies
    aflr2Instance[iIndex].numSurface = numBody;
    aflr2Instance[iIndex].surfaceMesh = (meshStruct *) EG_alloc(aflr2Instance[iIndex].numSurface*sizeof(meshStruct));
    if (aflr2Instance[iIndex].surfaceMesh == NULL) {
        aflr2Instance[iIndex].numSurface = 0;
        return EGADS_MALLOC;
    }

    // Initiate surface meshes
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++){
        status = initiate_meshStruct(&aflr2Instance[iIndex].surfaceMesh[bodyIndex]);
        if (status != CAPS_SUCCESS) return status;
    }

    // Setup meshing input structure -> -1 because aim_getIndex is 1 bias

    // Get Tessellation parameters -Tess_Params -> -1 because of aim_getIndex 1 bias
    aflr2Instance[iIndex].meshInput.paramTess[0] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[0]; // Gets multiplied by bounding box size
    aflr2Instance[iIndex].meshInput.paramTess[1] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[1]; // Gets multiplied by bounding box size
    aflr2Instance[iIndex].meshInput.paramTess[2] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[2];

    aflr2Instance[iIndex].meshInput.quiet            = aimInputs[aim_getIndex(aimInfo, "Mesh_Quiet_Flag",    ANALYSISIN)-1].vals.integer;
    aflr2Instance[iIndex].meshInput.outputASCIIFlag  = aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag",    ANALYSISIN)-1].vals.integer;

    // Mesh Format
    aflr2Instance[iIndex].meshInput.outputFormat = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].vals.string);
    if (aflr2Instance[iIndex].meshInput.outputFormat == NULL) return EGADS_MALLOC;

    // Project Name
    if (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].nullVal != IsNull) {

        aflr2Instance[iIndex].meshInput.outputFileName = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
        if (aflr2Instance[iIndex].meshInput.outputFileName == NULL) return EGADS_MALLOC;
    }

    // Output directory
    aflr2Instance[iIndex].meshInput.outputDirectory = EG_strdup(analysisPath);
    if (aflr2Instance[iIndex].meshInput.outputDirectory == NULL) return EGADS_MALLOC;

    // Set aflr2 specific mesh inputs  -1 because aim_getIndex is 1 bias
    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].nullVal != IsNull) {

        aflr2Instance[iIndex].meshInput.aflr4Input.meshInputString = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].vals.string);
        if (aflr2Instance[iIndex].meshInput.aflr4Input.meshInputString == NULL) return EGADS_MALLOC;
    }

    // Max and min number of points
    if (aimInputs[aim_getIndex(aimInfo, "Edge_Point_Min", ANALYSISIN)-1].nullVal != IsNull) {
        minEdgePoint = aimInputs[aim_getIndex(aimInfo, "Edge_Point_Min", ANALYSISIN)-1].vals.integer;
        if (minEdgePoint < 2) {
          printf("**********************************************************\n");
          printf("Edge_Point_Min = %d must be greater or equal to 2\n", minEdgePoint);
          printf("**********************************************************\n");
          status = CAPS_BADVALUE;
          goto cleanup;
        }
    }

    if (aimInputs[aim_getIndex(aimInfo, "Edge_Point_Max", ANALYSISIN)-1].nullVal != IsNull) {
        maxEdgePoint = aimInputs[aim_getIndex(aimInfo, "Edge_Point_Max", ANALYSISIN)-1].vals.integer;
        if (maxEdgePoint < 2) {
          printf("**********************************************************\n");
          printf("Edge_Point_Max = %d must be greater or equal to 2\n", maxEdgePoint);
          printf("**********************************************************\n");
          status = CAPS_BADVALUE;
          goto cleanup;
        }
    }

    if (maxEdgePoint >= 2 && minEdgePoint >= 2 && minEdgePoint > maxEdgePoint) {
      printf("**********************************************************\n");
      printf("Edge_Point_Max must be greater or equal Edge_Point_Min\n");
      printf("Edge_Point_Max = %d, Edge_Point_Min = %d\n",maxEdgePoint,minEdgePoint);
      printf("**********************************************************\n");
      status = CAPS_BADVALUE;
      goto cleanup;
    }

    // Get mesh sizing parameters
    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].nullVal != IsNull) {

        status = mesh_getSizingProp(aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].length,
                                    aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].vals.tuple,
                                    &aflr2Instance[iIndex].attrMap,
                                    &numMeshProp,
                                    &meshProp);
        if (status != CAPS_SUCCESS) return status;
    }

    // Modify the EGADS body tessellation based on given inputs
    status =  mesh_modifyBodyTess(numMeshProp,
                                  meshProp,
                                  minEdgePoint,
                                  maxEdgePoint,
                                  (int)false, // quadMesh
                                  &refLen,
                                  aflr2Instance[iIndex].meshInput.paramTess,
                                  aflr2Instance[iIndex].attrMap,
                                  numBody,
                                  bodies);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Run AFLR2 for each body
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

        printf("Getting 2D mesh for body %d (of %d)\n", bodyIndex+1, numBody);

        Message_Flag = aflr2Instance[iIndex].meshInput.quiet == (int)true ? 0 : 1;

        status = aflr2_Surface_Mesh(Message_Flag, bodies[bodyIndex],
                                    aflr2Instance[iIndex].meshInput,
                                    aflr2Instance[iIndex].attrMap,
                                    numMeshProp, meshProp,
                                    &aflr2Instance[iIndex].surfaceMesh[bodyIndex]);
        if (status != CAPS_SUCCESS) {
            printf("Problem during surface meshing of body %d\n", bodyIndex+1);
            goto cleanup;
        }

        printf("Number of nodes = %d\n", aflr2Instance[iIndex].surfaceMesh[bodyIndex].numNode);
        printf("Number of elements = %d\n", aflr2Instance[iIndex].surfaceMesh[bodyIndex].numElement);
        if (aflr2Instance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.useStartIndex == (int) true ||
            aflr2Instance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.useListIndex == (int) true) {
            printf("Number of tris = %d\n", aflr2Instance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.numTriangle);
            printf("Number of quad = %d\n", aflr2Instance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.numQuadrilateral);
        }

    }

    // Clean up meshProps
    if (meshProp != NULL) {

        for (i = 0; i < numMeshProp; i++) {

            (void) destroy_meshSizingStruct(&meshProp[i]);
        }
        EG_free(meshProp);

        meshProp = NULL;
    }

    if (aflr2Instance[iIndex].meshInput.outputFileName != NULL) {

        for (bodyIndex = 0; bodyIndex < aflr2Instance[iIndex].numSurface; bodyIndex++) {

            if (aflr2Instance[iIndex].numSurface > 1) {
                sprintf(bodyNumber, "%d", bodyIndex);
                filename = (char *) EG_alloc((strlen(aflr2Instance[iIndex].meshInput.outputFileName)  +
                                              strlen(aflr2Instance[iIndex].meshInput.outputDirectory) +
                                              2 +
                                              strlen("_2D_") +
                                              strlen(bodyNumber))*sizeof(char));
            } else {
                filename = (char *) EG_alloc((strlen(aflr2Instance[iIndex].meshInput.outputFileName) +
                                              strlen(aflr2Instance[iIndex].meshInput.outputDirectory)+2)*sizeof(char));

            }

            if (filename == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            strcpy(filename, aflr2Instance[iIndex].meshInput.outputDirectory);
            #ifdef WIN32
                strcat(filename, "\\");
            #else
                strcat(filename, "/");
            #endif
            strcat(filename, aflr2Instance[iIndex].meshInput.outputFileName);

            if (aflr2Instance[iIndex].numSurface > 1) {
                strcat(filename,"_2D_");
                strcat(filename, bodyNumber);
            }

            if (strcasecmp(aflr2Instance[iIndex].meshInput.outputFormat, "AFLR3") == 0) {

                status = mesh_writeAFLR3(filename,
                                         aflr2Instance[iIndex].meshInput.outputASCIIFlag,
                                         &aflr2Instance[iIndex].surfaceMesh[bodyIndex],
                                         1.0);

            } else if (strcasecmp(aflr2Instance[iIndex].meshInput.outputFormat, "VTK") == 0) {

                status = mesh_writeVTK(filename,
                                       aflr2Instance[iIndex].meshInput.outputASCIIFlag,
                                       &aflr2Instance[iIndex].surfaceMesh[bodyIndex],
                                       1.0);

            } else if (strcasecmp(aflr2Instance[iIndex].meshInput.outputFormat, "Tecplot") == 0) {

                status = mesh_writeTecplot(filename,
                                           aflr2Instance[iIndex].meshInput.outputASCIIFlag,
                                           &aflr2Instance[iIndex].surfaceMesh[bodyIndex],
                                           1.0);

            } else if (strcasecmp(aflr2Instance[iIndex].meshInput.outputFormat, "STL") == 0) {

                status = mesh_writeSTL(filename,
                                       aflr2Instance[iIndex].meshInput.outputASCIIFlag,
                                       &aflr2Instance[iIndex].surfaceMesh[bodyIndex],
                                       1.0);

            } else if (strcasecmp(aflr2Instance[iIndex].meshInput.outputFormat, "FAST") == 0) {

                status = mesh_writeFAST(filename,
                                        aflr2Instance[iIndex].meshInput.outputASCIIFlag,
                                        &aflr2Instance[iIndex].surfaceMesh[bodyIndex],
                                        1.0);

            } else {
                printf("Unrecognized mesh format, \"%s\", the volume mesh will not be written out\n", aflr2Instance[iIndex].meshInput.outputFormat);
            }

            if (filename != NULL) EG_free(filename);
            filename = NULL;

            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }


    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("Error: aflr2AIM (instance = %d) status %d\n", iIndex, status);

        if (meshProp != NULL) {

            for (i = 0; i < numMeshProp; i++) {

                (void) destroy_meshSizingStruct(&meshProp[i]);
            }

            EG_free(meshProp);
        }

        if (filename != NULL) EG_free(filename);
        return status;
}


int aimOutputs(/*@unused@*/ int iIndex, /*@unused@*/ void *aimStruc,
               /*@unused@*/ int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsAFLR2 AIM Outputs
     * The following list outlines the AFLR2 AIM outputs available through the AIM interface.
     *
     * - <B>Done</B> = True if a surface mesh was created on all surfaces, False if not.
     */

    #ifdef DEBUG
        printf(" aflr2AIM/aimOutputs instance = %d  index = %d!\n", inst, index);
    #endif

    *aoname = EG_strdup("Done");
    form->type = Boolean;
    form->vals.integer = (int) false;

    return CAPS_SUCCESS;
}

// See if a surface mesh was generated for each body
int aimCalcOutput(int iIndex, void *aimInfo,
                  const char *apath, int index,
                  capsValue *val, capsErrs **errors)
{
    int surf; // Indexing

    #ifdef DEBUG
        printf(" aflr2AIM/aimCalcOutput instance = %d  index = %d!\n", inst, index);
    #endif

    *errors           = NULL;
    val->vals.integer = (int) false;

    // Check to see if surface meshes was generated
    for (surf = 0; surf < aflr2Instance[iIndex].numSurface; surf++ ) {

        if (aflr2Instance[iIndex].surfaceMesh[surf].numElement != 0) {

            val->vals.integer = (int) true;

        } else {

            val->vals.integer = (int) false;
            printf("No surface Tris and/or Quads were generated for surface - %d\n", surf);
            return CAPS_SUCCESS;
        }
    }

    return CAPS_SUCCESS;
}

/************************************************************************/

/*
 * since this AIM does not support field variables or CAPS bounds, the
 * following functions are not required to be filled in except for aimDiscr
 * which just stores away our bodies and aimFreeDiscr that does some cleanup
 * when CAPS terminates
 */


void aimCleanup()
{
    int iIndex; // Indexing

    int status; // Returning status

    #ifdef DEBUG
        printf(" aflr2AIM/aimCleanup!\n");
    #endif

    // Clean up aflr2Instance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up aflr2Instance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr2AIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
    }

    if (aflr2Instance != NULL) EG_free(aflr2Instance);
    aflr2Instance = NULL;
    numInstance = 0;

}

int aimLocateElement(/*@unused@*/ capsDiscr *discr, /*@unused@*/ double *params,
                 /*@unused@*/ double *param,    /*@unused@*/ int *eIndex,
                 /*@unused@*/ double *bary)
{
#ifdef DEBUG
  printf(" aflr2AIM/aimLocateElement instance = %d!\n", discr->instance);
#endif

  return CAPS_SUCCESS;
}

int aimTransfer(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
            /*@unused@*/ int npts, /*@unused@*/ int rank,
            /*@unused@*/ double *data, /*@unused@*/ char **units)
{
#ifdef DEBUG
  printf(" aflr2AIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n",
         name, discr->instance, npts, rank);
#endif

  return CAPS_SUCCESS;
}

int aimInterpolation(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
                 /*@unused@*/ int eIndex, /*@unused@*/ double *bary,
                 /*@unused@*/ int rank, /*@unused@*/ double *data,
                 /*@unused@*/ double *result)
{
#ifdef DEBUG
  printf(" aflr2AIM/aimInterpolation  %s  instance = %d!\n",
         name, discr->instance);
#endif

  return CAPS_SUCCESS;
}


int aimInterpolateBar(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
                  /*@unused@*/ int eIndex, /*@unused@*/ double *bary,
                  /*@unused@*/ int rank, /*@unused@*/ double *r_bar,
                  /*@unused@*/ double *d_bar)
{
#ifdef DEBUG
  printf(" aflr2AIM/aimInterpolateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif

  return CAPS_SUCCESS;
}


int aimIntegration(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
               /*@unused@*/ int eIndex, /*@unused@*/ int rank,
               /*@unused@*/ /*@null@*/ double *data, /*@unused@*/ double *result)
{
#ifdef DEBUG
  printf(" aflr2AIM/aimIntegration  %s  instance = %d!\n",
         name, discr->instance);
#endif

  return CAPS_SUCCESS;
}


int aimIntegrateBar(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
                /*@unused@*/ int eIndex, /*@unused@*/ int rank,
                /*@unused@*/ double *r_bar, /*@unused@*/ double *d_bar)
{
#ifdef DEBUG
  printf(" aflr2AIM/aimIntegrateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif

  return CAPS_SUCCESS;
}
