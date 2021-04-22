/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             AFLR3 AIM
 *
 *      Written by Dr. Ryan Durscher AFRL/RQVC
 *
 *      This software has been cleared for public release on 25 Jul 2018, case number 88ABW-2018-3794.
 *
 */


/*!\mainpage Introduction
 *
 * \section overviewAFLR3 AFLR3 AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact with the
 * unstructured, volumetric grid generator AFLR3 \cite Marcum1995 \cite Marcum1998.
 *
 * The AFLR3 AIM provides the CAPS users with the ability to generate "unstructured tetrahedral element grids" using an
 * "Advancing-Front/Local-Reconnection (AFLR) procedure." Additionally, an "Advancing-Normal Boundary-Layer (ANBL) procedure" may be
 * used "to generate a tetrahedral/pentahedral/hexahedral BL grid adjacent to" specified surfaces.
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsAFLR3 and \ref aimOutputsAFLR3, respectively.
 * The complete AFLR documentation is available at the <a href="http://www.simcenter.msstate.edu/software/downloads/doc/system/index.html">SimCenter</a>.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentAFLR3.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataAFLR3 if connecting this AIM to other AIMs in a
 * parent-child like manner.
 *
 * Example volumes meshes:
 *  \image html multiBodyAFRL3.png "AFLR3 meshing example - Multiple Airfoils with Boundary Layer" width=20px
 *  \image latex multiBodyAFRL3.png "AFLR3 meshing example - Multiple Airfoils with Boundary Layer" width=5cm
 *
 * \section clearanceAFLR3 Clearance Statement
 *  This software has been cleared for public release on 25 Jul 2018, case number 88ABW-2018-3794.
 */

#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"       // Collection of helper functions for meshing
#include "cfdUtils.h"        // Collection of helper functions for cfd analysis
#include "miscUtils.h"

#include "aflr3_Interface.h" // Bring in AFLR3 'interface' functions

#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif


#define NUMINPUT  12  // number of mesh aimInputs
#define NUMOUT    1  // number of outputs

#define MXCHAR 255

//#define DEBUG

////// Global variables //////
typedef struct {

    // Container for volume mesh
    int numVolumeMesh;
    meshStruct *volumeMesh;

    int numSurfaceMesh;
    meshStruct *surfaceMesh;
    int surfaceMeshInherited;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

    // Container for mesh input
    meshInputStruct meshInput;

} aimStorage;

static aimStorage *aflr3Instance = NULL;
static int         numInstance  = 0;


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

static int destroy_aimStorage(int iIndex) {

    int i; // Indexing

    int status; // Function return status

    #if  _MSC_VER >= 1900
        _iob[0] = *stdin;
        _iob[1] = *stdout;
        _iob[2] = *stderr;
    #endif

    // Destroy meshInput
    status = destroy_meshInputStruct(&aflr3Instance[iIndex].meshInput);
    if (status != CAPS_SUCCESS) printf("Status = %d, aflr3AIM instance %d, meshInput cleanup!!!\n", status, iIndex);

    // Destroy surfaceMesh allocated arrays
    if (aflr3Instance[iIndex].surfaceMeshInherited == (int) false) { // Did the parent aim already destroy the surfaces?

        for (i = 0; i < aflr3Instance[iIndex].numSurfaceMesh; i++) {
            status = destroy_meshStruct(&aflr3Instance[iIndex].surfaceMesh[i]);
            if (status != CAPS_SUCCESS) printf("Status = %d, aflr3AIM instance %d, destroy_meshStruct cleanup!!!\n", status, iIndex);
        }
        if (aflr3Instance[iIndex].surfaceMesh != NULL) EG_free(aflr3Instance[iIndex].surfaceMesh);

    }

    aflr3Instance[iIndex].numSurfaceMesh = 0;
    aflr3Instance[iIndex].surfaceMeshInherited = (int) false;
    aflr3Instance[iIndex].surfaceMesh = NULL;

    // Destroy volumeMesh allocated arrays
    if (aflr3Instance[iIndex].volumeMesh != NULL) {
        for (i = 0; i < aflr3Instance[iIndex].numVolumeMesh; i++) {
            status = destroy_meshStruct(&aflr3Instance[iIndex].volumeMesh[i]);
            if (status != CAPS_SUCCESS) printf("Status = %d, aflr3AIM instance %d, destroy_meshStruct cleanup!!!\n", status, iIndex);
        }

        EG_free(aflr3Instance[iIndex].volumeMesh);
    }

    aflr3Instance[iIndex].numVolumeMesh = 0;
    aflr3Instance[iIndex].volumeMesh = NULL;

    // Destroy attribute to index map
    status = destroy_mapAttrToIndexStruct(&aflr3Instance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Status = %d, aflr3AIM instance %d, destroy_mapAttrToIndexStruct cleanup!!!\n", status, iIndex);

    return CAPS_SUCCESS;
}


/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(int ngIn, capsValue *gIn, int *qeFlag, const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int status; // Function status return
    int flag;   // query/execute flag

    aimStorage *tmp;

    #ifdef WIN32
        #if  _MSC_VER >= 1900
            _iob[0] = *stdin;
            _iob[1] = *stdout;
            _iob[2] = *stderr;
        #endif
    #endif

    #ifdef DEBUG
        printf("\n aflr3AIM/aimInitialize   ngIn = %d  qeFlag = %d!\n",
               ngIn, *eqFlag);
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

    // Allocate aflr3Instance
    if (aflr3Instance == NULL) {
        aflr3Instance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (aflr3Instance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(aflr3Instance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        aflr3Instance = tmp;
    }

    // Set initial values for aflr3Instance //

    // Container for volume mesh
    aflr3Instance[numInstance].numVolumeMesh = 0;
    aflr3Instance[numInstance].volumeMesh = NULL;

    // Container for surface meshes
    aflr3Instance[numInstance].numSurfaceMesh = 0;
    aflr3Instance[numInstance].surfaceMesh = NULL;
    aflr3Instance[numInstance].surfaceMeshInherited = (int) false;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&aflr3Instance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for mesh input
    status = initiate_meshInputStruct(&aflr3Instance[numInstance].meshInput);

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}


int aimInputs(int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{
    /*! \page aimInputsAFLR3 AIM Inputs
     * The following list outlines the AFLR3 meshing options along with their default value available
     * through the AIM interface. By default, these values will be linked to any parent AIMs with variables
     * of the same name, with the exception of the ...
     */

    #ifdef DEBUG
        printf(" aflr3AIM/aimInputs instance = %d  index = %d!\n", inst, index);
    #endif

    // Meshing Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name"); // If NULL a volume grid won't be written by the AIM
        defval->type         = String;
        defval->nullVal      = IsNull;
        defval->vals.string  = NULL;
        //defval->vals.string  = EG_strdup("CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsAFLR3
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

        /*! \page aimInputsAFLR3
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

        /*! \page aimInputsAFLR3
         * - <B> Mesh_Quiet_Flag = False</B> <br>
         * Suppression of mesh generator (not including errors)
         */

    } else if (index == 4) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->vals.string   = EG_strdup("AFLR3"); // AFLR3, SU2, VTK
        defval->lfixed        = Change;

        /*! \page aimInputsAFLR3
         * - <B> Mesh_Format = "AFLR3"</B> <br>
         * Mesh output format. Available format names  include: "AFLR3", "SU2", "Nastran", "Tecplot", and "VTK".
         */

    } else if (index == 5) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsAFLR3
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file, if applicable.
         */

    } else if (index == 6) {
        *ainame               = EG_strdup("Mesh_Gen_Input_String");
        defval->type          = String;
        defval->nullVal       = IsNull;
        defval->vals.string   = NULL;

        /*! \page aimInputsAFLR3
         * - <B> Mesh_Gen_Input_String = NULL</B> <br>
         * Meshing program command line string (as if called in bash mode). Use this to specify more
         * complicated options/use features of the mesher not currently exposed through other AIM input
         * variables. Note that this is the exact string that will be provided to the volume mesher; no
         * modifications will be made. If left NULL an input string will be created based on default values
         * of the relevant AIM input variables.
         */

    } else if (index == 7) {
          *ainame               = EG_strdup("Multiple_Mesh");
          defval->type          = Boolean;
          defval->vals.integer  = (int) false;

          /*! \page aimInputsAFLR3
           * - <B> Multiple_Mesh = False</B> <br>
           * If set to True a volume will be generated for each body. When set to False (default value) only a single volume
           * mesh will be created.
           */

    } else if (index == 8) {
        *ainame              = EG_strdup("Mesh_Sizing");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsAFLR3
         * - <B>Mesh_Sizing = NULL </B> <br>
         * See \ref meshSizingProp for additional details.
         */
    } else if (index == 9) {
        *ainame               = EG_strdup("BL_Initial_Spacing");
        defval->type          = Double;
        defval->nullVal       = NotNull;
        defval->vals.real     = 0.0;

        /*! \page aimInputsAFLR3
         * - <B> BL_Initial_Spacing = 0.0</B> <br>
         * Initial mesh spacing when growing a boundary layer that is applied to all bodies. <br><br>
         * Note: Both "BL_Initial_Spacing" and "BL_Thickness" must be non-zero for values to be applied. If "Multiple_Mesh" is
         * False (default value) these values will not be applied to the largest body (if more than 1 body exist in the AIM),
         * as that body is assumed to be a bounding box (e.g. a farfield boundary in a CFD simulation). Boundary spacing
         * and thickness specified through the use of the "Mesh_Sizing" input (see \ref meshSizingProp for additional details)
         * will take precedence over the values specified for "BL_Initial_Spacing" and "BL_Thickness".
         */

    } else if (index == 10) {
        *ainame               = EG_strdup("BL_Thickness");
        defval->type          = Double;
        defval->nullVal       = NotNull;
        defval->vals.real     = 0.0;

        /*! \page aimInputsAFLR3
         * - <B> BL_Thickness = 0.0</B> <br>
         * Total boundary layer thickness that is applied to all bodies. <br>
         * This is a lower bound on the desired thickness. The height can be limited with "nbl".<br><br>
         * Note: see "BL_Initial_Spacing" and "BL_Max_Layers" for additional details
         */

    } else if (index == 11) {
        *ainame               = EG_strdup("BL_Max_Layers");
        defval->type          = Integer;
        defval->nullVal       = IsNull;
        defval->vals.integer  = 10000;

        /*! \page aimInputsAFLR3
         * - <B> BL_Max_Layers = 10000</B> <br>
         * Maximum BL grid layers to generate.
         */

    } else if (index == 12) {
        *ainame               = EG_strdup("BL_Max_Layer_Diff");
        defval->type          = Integer;
        defval->nullVal       = IsNull;
        defval->vals.integer  = 0;

        /*! \page aimInputsAFLR3
         * - <B> BL_Max_Layer_Diff = 0</B> <br>
         * Maximum difference in BL levels.<br>
         * If BL_Max_Layer_Diff > 0 then the maximum difference between the number of BL levels for<br>
         * the BL nodes on a given BL boundary surface face is limited to BL_Max_Layer_Diff. <br>
         * Any active BL node that would allow the number of levels to be greater is <br>
         * terminated. <br>
         * If BL_Max_Layer_Diff = 0 then the difference in BL levels is ignored.
         */
    }

#if NUMINPUT != 12
#error "NUMINPUTS is inconsistent with the list of inputs"
#endif

   /* if (index != 1 && index != 6){
        // Link variable(s) to parent(s) if available
        status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
        //printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n", status, index, defval->type, defval->link);
    }
    */

    return CAPS_SUCCESS;
}

int aimData(int iIndex, const char *name, enum capsvType *vtype, int *rank,
            int *nrow, int *ncol, void **data, char **units)
{

    /*! \page sharableDataAFLR3 AIM Shareable Data
     * The AFLR3 AIM has the following shareable data types/values with its children AIMs if they are so inclined.
     * - <B> Surface_Mesh</B> <br>
     * The returned surface mesh in meshStruct (see meshTypes.h) format.
     * - <B> Volume_Mesh</B> <br>
     * The returned volume mesh after AFLR3 execution is complete in meshStruct (see meshTypes.h) format.
     * - <B> Attribute_Map</B> <br>
     * An index mapping between capsGroups found on the geometry in mapAttrToIndexStruct (see miscTypes.h) format.
     */

    #ifdef DEBUG
        printf(" aflr3AIM/aimData instance = %d  name = %s!\n", inst, name);
    #endif

    // The returned surface mesh from AFLR3
    if (strcasecmp(name, "Surface_Mesh") == 0){
        *vtype = Value;
        *rank  = *ncol = 1;
        *nrow = aflr3Instance[iIndex].numSurfaceMesh;
        *data  = &aflr3Instance[iIndex].surfaceMesh;
        *units = NULL;

        return CAPS_SUCCESS;
    }

    // The returned Volume mesh from AFLR3
    if (strcasecmp(name, "Volume_Mesh") == 0){
        *vtype = Value;
        *rank  = *ncol = 1;
        *nrow = aflr3Instance[iIndex].numVolumeMesh;
        if (aflr3Instance[iIndex].numVolumeMesh == 1) {
            *data  = &aflr3Instance[iIndex].volumeMesh[0];
        } else {
            *data  = aflr3Instance[iIndex].volumeMesh;
        }

        *units = NULL;

        return CAPS_SUCCESS;
    }

    // Share the attribute map
    if (strcasecmp(name, "Attribute_Map") == 0){
        *vtype = Value;
        *rank  = *nrow = *ncol = 1;
        *data  = &aflr3Instance[iIndex].attrMap;
        *units = NULL;

        return CAPS_SUCCESS;
    }


   return CAPS_NOTFOUND;
}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath, capsValue *aimInputs,
                   capsErrs **errs)
{
    int i, propIndex, bodyIndex, pointIndex[4] = {-1,-1,-1,-1}; // Indexing

    int status; // Return status

    const char *intents;

    // Incoming bodies
    ego *bodies = NULL;
    int numBody = 0;

    // Data transfer related variables
    int            nrow, ncol, rank;
    void           *dataTransfer;
    char           *units;
    enum capsvType vtype;

    // Inherit surface mesh related variables
    int numSurfMesh = 0;

    // Mesh attribute parameters
    int numMeshProp = 0;
    meshSizingStruct *meshProp = NULL;

    // Boundary Layer meshing related variables
    double globalBLThickness = 0.0, globalBLSpacing = 0.0, capsMeshLength = 0;
    int createBL = (int) false; // Indicate if boundary layer meshes should be generated
    int *blFlag = NULL ;        // Boundary layer flag [numElement] - Total elements for all bodies
    double *blThickness = NULL; // Boundary layer thickness [numNode] - Total nodes for all bodies
    double *blSpacing = NULL;   // Boundary layer spacing [numNode] - Total nodes for all bodies

    int elementOffSet = 0, nodeOffSet = 0;

    // Bounding box variables
    int boundingBoxIndex = CAPSMAGIC;
    double  box[6], boxMax[6] = {0,0,0,0,0,0}, refLen = -1.0;

    // Output grid
    char *filename = NULL;
    char bodyIndexNumber[11];

    // NULL out errors
    *errs = NULL;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    #ifdef DEBUG
        printf(" aflr3AIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, numBody);
    #endif

    if (numBody <= 0 || bodies == NULL) {
        #ifdef DEBUG
            printf(" aflr3AIM/aimPreAnalysis No Bodies!\n");
        #endif
        return CAPS_SOURCEERR;
    }

    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the same instance
    status = destroy_aimStorage(iIndex);
    if (status != CAPS_SUCCESS) {
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr3AIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
        return status;
    }

    // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
    status = aim_getData(aimInfo, "Attribute_Map", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
    if (status == CAPS_SUCCESS) {

        printf("Found link for attrMap (Attribute_Map) from parent\n");

        status = copy_mapAttrToIndexStruct( (mapAttrToIndexStruct *) dataTransfer, &aflr3Instance[iIndex].attrMap);
        if (status != CAPS_SUCCESS) return status;

    } else {

        if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
        else printf("Linking status during attrMap (Attribute_Map) = %d\n",status);

        printf("Didn't find a link to attrMap from parent - getting it ourselves\n");

        // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
        status = create_CAPSGroupAttrToIndexMap(numBody,
                                                bodies,
                                                2, // Only search down to the face level of the EGADS bodyIndex
                                                &aflr3Instance[iIndex].attrMap);
        if (status != CAPS_SUCCESS) return status;
    }

    // Setup meshing input structure -> -1 because aim_getIndex is 1 bias

    // Get Tessellation parameters -Tess_Params -> -1 because of aim_getIndex 1 bias
    aflr3Instance[iIndex].meshInput.paramTess[0] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[0];
    aflr3Instance[iIndex].meshInput.paramTess[1] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[1];
    aflr3Instance[iIndex].meshInput.paramTess[2] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[2];


    aflr3Instance[iIndex].meshInput.quiet            = aimInputs[aim_getIndex(aimInfo, "Mesh_Quiet_Flag",    ANALYSISIN)-1].vals.integer;
    aflr3Instance[iIndex].meshInput.outputASCIIFlag  = aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag",    ANALYSISIN)-1].vals.integer;


    // Mesh Format
    aflr3Instance[iIndex].meshInput.outputFormat = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].vals.string);
    if (aflr3Instance[iIndex].meshInput.outputFormat == NULL) return EGADS_MALLOC;


    // Project Name
    if (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].nullVal != IsNull) {

        aflr3Instance[iIndex].meshInput.outputFileName = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
        if (aflr3Instance[iIndex].meshInput.outputFileName == NULL) return EGADS_MALLOC;
    }

    // Output directory
    aflr3Instance[iIndex].meshInput.outputDirectory = EG_strdup(analysisPath);
    if (aflr3Instance[iIndex].meshInput.outputDirectory == NULL) return EGADS_MALLOC;

    // Set aflr3 specific mesh aimInputs  -1 because aim_getIndex is 1 bias
    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].nullVal != IsNull) {

        aflr3Instance[iIndex].meshInput.aflr3Input.meshInputString = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].vals.string);
        if (aflr3Instance[iIndex].meshInput.aflr3Input.meshInputString == NULL) return EGADS_MALLOC;
    }

    status = populate_bndCondStruct_from_mapAttrToIndexStruct(&aflr3Instance[iIndex].attrMap,
                                                              &aflr3Instance[iIndex].meshInput.bndConds);
    if (status != CAPS_SUCCESS) return status;

    // Get mesh sizing parameters
    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].nullVal != IsNull) {

        status = mesh_getSizingProp(aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].length,
                                    aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].vals.tuple,
                                    &aflr3Instance[iIndex].attrMap,
                                    &numMeshProp,
                                    &meshProp);
        if (status != CAPS_SUCCESS) return status;
    }

    // Get surface mesh
    status = aim_getData(aimInfo, "Surface_Mesh", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
    if (status == CAPS_SUCCESS) {

        printf("Found link for a surface mesh (Surface_Mesh) from parent\n");

        numSurfMesh = 0;
        if      (nrow == 1 && ncol != 1) numSurfMesh = ncol;
        else if (nrow != 1 && ncol == 1) numSurfMesh = nrow;
        else if (nrow == 1 && ncol == 1) numSurfMesh = nrow;
        else {

            printf("Can not except 2D matrix of surface meshes\n");
            return  CAPS_BADVALUE;
        }

        if (numSurfMesh != numBody) {
            printf("Number of inherited surface meshes does not match the number of bodies\n");
            return CAPS_SOURCEERR;
        }
        aflr3Instance[iIndex].numSurfaceMesh =  numSurfMesh;
        aflr3Instance[iIndex].surfaceMesh = (meshStruct * ) dataTransfer;
        aflr3Instance[iIndex].surfaceMeshInherited = (int) true;

    } else { // Use EGADS bodyIndex tessellation for surface mesh

        aflr3Instance[iIndex].numSurfaceMesh = numBody;
        aflr3Instance[iIndex].surfaceMesh = (meshStruct * ) EG_alloc(aflr3Instance[iIndex].numSurfaceMesh*sizeof(meshStruct));
        if (aflr3Instance[iIndex].surfaceMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {
            status = initiate_meshStruct(&aflr3Instance[iIndex].surfaceMesh[bodyIndex]);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        aflr3Instance[iIndex].surfaceMeshInherited = (int) false;

        // Modify the EGADS bodyIndex tessellation based on given aimInputs if we aren't inheriting a mesh
        status =  mesh_modifyBodyTess(numMeshProp,
                                      meshProp,
                                      -1,          //minEdgePointGlobal
                                      -1,          //maxEdgePointGlobal
                                      (int)false,  //quadMesh
                                      &refLen,
                                      aflr3Instance[iIndex].meshInput.paramTess,
                                      aflr3Instance[iIndex].attrMap,
                                      numBody,
                                      bodies);
        if (status != CAPS_SUCCESS) goto cleanup;

        for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

            printf("Getting surface mesh for body %d (of %d)\n", bodyIndex+1, numBody);

            status = mesh_surfaceMeshEGADSBody(bodies[bodyIndex],
                                               refLen,
                                               aflr3Instance[iIndex].meshInput.paramTess,
                                               (int)false, //quadMesh
                                               &aflr3Instance[iIndex].attrMap,
                                               &aflr3Instance[iIndex].surfaceMesh[bodyIndex]);
            if (status != CAPS_SUCCESS) return status;
        }
    }


    // If we are only going to have one volume determine the bounding box body so that boundary layer
    // parameters wont be applied to it
    if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) false) {

        // Determine which body is the bounding body based on size
        if (numBody > 1) {
            for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

                // Get bounding box for the body
                status = EG_getBoundingBox(bodies[bodyIndex], box);
                if (status != EGADS_SUCCESS) {
                    printf(" EG_getBoundingBox = %d\n\n", status);
                    return status;
                }

                // Just copy the box coordinates on the first go around
                if (bodyIndex == 0) {

                    memcpy(boxMax, box, sizeof(box));

                    // Set body as the bounding box (ie. farfield)
                    boundingBoxIndex = bodyIndex;

                    // Else compare with the "max" box size
                } else if (boxMax[0] >= box[0] &&
                           boxMax[1] >= box[1] &&
                           boxMax[2] >= box[2] &&
                           boxMax[3] <= box[3] &&
                           boxMax[4] <= box[4] &&
                           boxMax[5] <= box[5]) {

                    // If bigger copy coordinates
                    memcpy(boxMax, box, sizeof(box));

                    // Set body as the bounding box (ie. farfield)
                    boundingBoxIndex = bodyIndex;
                }
            }
        }
    }

    // Get global
    globalBLThickness = aimInputs[aim_getIndex(aimInfo, "BL_Thickness", ANALYSISIN)-1].vals.real;
    globalBLSpacing = aimInputs[aim_getIndex(aimInfo, "BL_Initial_Spacing", ANALYSISIN)-1].vals.real;;

    // check that both
    //   globalBLThickness == 0 && globalBLSpacing == 0
    // or
    //   globalBLThickness != 0 && globalBLSpacing != 0
    if (!((globalBLThickness == 0.0 && globalBLSpacing == 0.0) ||
          (globalBLThickness != 0.0 && globalBLSpacing != 0.0))) {
        printf("Both BL_Thickness = %le and BL_Initial_Spacing = %le\n", globalBLThickness, globalBLSpacing);
        printf("must be zero or non-zero.\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // check if boundary layer meshing is requested
    if (globalBLThickness != 0.0 &&
        globalBLSpacing   != 0.0) {

        // Turn on BL flags if globals are set
        createBL = (int) true;
    }

    // Loop through surface meshes to set boundary layer parameters
    for (bodyIndex = 0; bodyIndex < aflr3Instance[iIndex].numSurfaceMesh && createBL == (int) false; bodyIndex++) {

        // Loop through meshing properties and see if boundaryLayerThickness and boundaryLayerSpacing have been specified
        for (propIndex = 0; propIndex < numMeshProp && createBL == (int) false; propIndex++) {

            // If no boundary layer specified in meshProp continue
            if (meshProp[propIndex].boundaryLayerThickness == 0 || meshProp[propIndex].boundaryLayerSpacing == 0) continue;

            // Loop through Elements and see if marker match
            for (i = 0; i < aflr3Instance[iIndex].surfaceMesh[bodyIndex].numElement && createBL == (int) false; i++) {

                //If they don't match continue
                if (aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].markerID != meshProp[propIndex].attrIndex) continue;

                // Set face "bl" flag
                createBL = (int) true;
            }
        }
    }

    // Get the capsMeshLenght if boundary layer meshing has been requested
    if (createBL == (int) true) {
        status = check_CAPSMeshLength(numBody, bodies, &capsMeshLength);

        if (capsMeshLength <= 0 || status != CAPS_SUCCESS) {
            printf("**********************************************************\n");
            if (status != CAPS_SUCCESS)
              printf("capsMeshLength is not set on any body.\n");
            else
              printf("capsMeshLength: %f\n", capsMeshLength);
            printf("\n");
            printf("The capsMeshLength attribute must present on at least\n"
                   "one body for boundary layer generation.\n"
                   "\n"
                   "capsMeshLength should be a a positive value representative\n"
                   "of a characteristic length of the geometry,\n"
                   "e.g. the MAC of a wing or diameter of a fuselage.\n");
            printf("**********************************************************\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }
    }

    // Loop through surface meshes to set boundary layer parameters
    for (bodyIndex = 0; bodyIndex < aflr3Instance[iIndex].numSurfaceMesh; bodyIndex++) {
        // Allocate blThickness and blSpacing
        blThickness = (double *) EG_reall(blThickness,
                                         (aflr3Instance[iIndex].surfaceMesh[bodyIndex].numNode+nodeOffSet)*sizeof(double));
        blSpacing   = (double *) EG_reall(blSpacing,
                                         (aflr3Instance[iIndex].surfaceMesh[bodyIndex].numNode+nodeOffSet)*sizeof(double));
        if (blThickness == NULL || blSpacing == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        // Set default to globals
        for (i = 0; i < aflr3Instance[iIndex].surfaceMesh[bodyIndex].numNode; i++) {
            blThickness[i + nodeOffSet] = globalBLThickness*capsMeshLength;
            blSpacing  [i + nodeOffSet] = globalBLSpacing*capsMeshLength;
        }

        // Allocate blFlag
        blFlag = (int *) EG_reall(blFlag,
                                  (aflr3Instance[iIndex].surfaceMesh[bodyIndex].numElement+elementOffSet)*sizeof(int));
        if (blFlag == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        // Set BL flag
        for (i = 0; i < aflr3Instance[iIndex].surfaceMesh[bodyIndex].numElement; i++) {

            // Set default value to non-viscous
            blFlag[i + elementOffSet] = 0;

            if (globalBLThickness != 0.0 &&
                globalBLSpacing   != 0.0) {

                if (bodyIndex == boundingBoxIndex) {
                    if (i == 0) {
                        printf("Global boundary layer parameters will not be specified on surface %d as it is the bounding box!\n", bodyIndex+1);
                    }
                    continue;
                }

                // Turn on BL flags if globals are set
                blFlag[i + elementOffSet] = -1;
            }
        }

        // Loop through meshing properties and see if boundaryLayerThickness and boundaryLayerSpacing have been specified
        for (propIndex = 0; propIndex < numMeshProp; propIndex++) {

            // If no boundary layer specified in meshProp continue
            if (meshProp[propIndex].boundaryLayerThickness == 0 || meshProp[propIndex].boundaryLayerSpacing == 0) continue;

            // Loop through Elements and see if marker match
            for (i = 0; i < aflr3Instance[iIndex].surfaceMesh[bodyIndex].numElement; i++) {

                //If they don't match continue
                if (aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].markerID != meshProp[propIndex].attrIndex) continue;

                // Set face "bl" flag
                blFlag[i + elementOffSet] = -1;

                if (aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].elementType != Triangle &&
                    aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].elementType != Quadrilateral) continue;

                // Get face indexing for Triangles - 1 bias
                if (aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].elementType == Triangle) {
                    pointIndex[0] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[0] -1;
                    pointIndex[1] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[1] -1;
                    pointIndex[2] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[2] -1;
                    pointIndex[3] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[2] -1; //Repeat last point for simplicity

                }

                if (aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].elementType == Quadrilateral) {
                    pointIndex[0] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[0] -1;
                    pointIndex[1] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[1] -1;
                    pointIndex[2] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[2] -1;
                    pointIndex[3] = aflr3Instance[iIndex].surfaceMesh[bodyIndex].element[i].connectivity[3] -1;
                }

                // Set boundary layer spacing
                blSpacing[pointIndex[0] + nodeOffSet] = meshProp[propIndex].boundaryLayerSpacing*capsMeshLength;
                blSpacing[pointIndex[1] + nodeOffSet] = meshProp[propIndex].boundaryLayerSpacing*capsMeshLength;
                blSpacing[pointIndex[2] + nodeOffSet] = meshProp[propIndex].boundaryLayerSpacing*capsMeshLength;
                blSpacing[pointIndex[3] + nodeOffSet] = meshProp[propIndex].boundaryLayerSpacing*capsMeshLength;

                // Set boundary layer thickness
                blThickness[pointIndex[0] + nodeOffSet] = meshProp[propIndex].boundaryLayerThickness*capsMeshLength;
                blThickness[pointIndex[1] + nodeOffSet] = meshProp[propIndex].boundaryLayerThickness*capsMeshLength;
                blThickness[pointIndex[2] + nodeOffSet] = meshProp[propIndex].boundaryLayerThickness*capsMeshLength;
                blThickness[pointIndex[3] + nodeOffSet] = meshProp[propIndex].boundaryLayerThickness*capsMeshLength;

            }
        }

        elementOffSet += aflr3Instance[iIndex].surfaceMesh[bodyIndex].numElement;
        nodeOffSet += aflr3Instance[iIndex].surfaceMesh[bodyIndex].numNode;
    }

    // Create/setup volume meshes
    if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {

        aflr3Instance[iIndex].numVolumeMesh = numBody;
        aflr3Instance[iIndex].volumeMesh = (meshStruct *) EG_alloc(aflr3Instance[iIndex].numVolumeMesh *sizeof(meshStruct));
        if (aflr3Instance[iIndex].volumeMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (bodyIndex = 0; bodyIndex < aflr3Instance[iIndex].numVolumeMesh; bodyIndex++) {
            status = initiate_meshStruct(&aflr3Instance[iIndex].volumeMesh[bodyIndex]);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Set reference mesh - One surface per body
            aflr3Instance[iIndex].volumeMesh[bodyIndex].numReferenceMesh = 1;
            aflr3Instance[iIndex].volumeMesh[bodyIndex].referenceMesh = (meshStruct *) EG_alloc(aflr3Instance[iIndex].volumeMesh[bodyIndex].numReferenceMesh*
                                                                                                sizeof(meshStruct));
            if (aflr3Instance[iIndex].volumeMesh[bodyIndex].referenceMesh == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            aflr3Instance[iIndex].volumeMesh[bodyIndex].referenceMesh[0] = aflr3Instance[iIndex].surfaceMesh[bodyIndex];
        }

    } else {

        aflr3Instance[iIndex].numVolumeMesh = 1;
        aflr3Instance[iIndex].volumeMesh = (meshStruct *) EG_alloc(aflr3Instance[iIndex].numVolumeMesh*sizeof(meshStruct));
        if (aflr3Instance[iIndex].volumeMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        status = initiate_meshStruct(&aflr3Instance[iIndex].volumeMesh[0]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Combine mesh - temporary story the combined mesh in the volume mesh
        status = mesh_combineMeshStruct(aflr3Instance[iIndex].numSurfaceMesh,
                                        aflr3Instance[iIndex].surfaceMesh,
                                        &aflr3Instance[iIndex].volumeMesh[0]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Set reference meshes - All surfaces
        aflr3Instance[iIndex].volumeMesh[0].numReferenceMesh = aflr3Instance[iIndex].numSurfaceMesh;
        aflr3Instance[iIndex].volumeMesh[0].referenceMesh = (meshStruct *) EG_alloc(aflr3Instance[iIndex].volumeMesh[0].numReferenceMesh*
                                                                                    sizeof(meshStruct));
        if (aflr3Instance[iIndex].volumeMesh[0].referenceMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (bodyIndex = 0; bodyIndex < aflr3Instance[iIndex].numSurfaceMesh; bodyIndex++) {
            aflr3Instance[iIndex].volumeMesh[0].referenceMesh[bodyIndex] = aflr3Instance[iIndex].surfaceMesh[bodyIndex];
        }

        // Report surface mesh
        printf("Number of surface nodes - %d\n",aflr3Instance[iIndex].volumeMesh[0].numNode);
        printf("Number of surface elements - %d\n",aflr3Instance[iIndex].volumeMesh[0].numElement);

    }

    // Run AFLR3
    elementOffSet = 0;
    nodeOffSet = 0;
    for (i = 0; i < aflr3Instance[iIndex].numVolumeMesh; i++) {

        if (aflr3Instance[iIndex].numVolumeMesh > 1) {
            printf("Getting volume mesh for body %d (of %d)\n", i+1, aflr3Instance[iIndex].numVolumeMesh);

            status = aflr3_Volume_Mesh ( aimInfo, aimInputs,
                                         aflr3Instance[iIndex].meshInput,
                                         &aflr3Instance[iIndex].volumeMesh[i].referenceMesh[0],
                                         createBL,
                                         &blFlag[elementOffSet],
                                         &blSpacing[nodeOffSet],
                                         &blThickness[nodeOffSet],
                                         &aflr3Instance[iIndex].volumeMesh[i]);

            elementOffSet += aflr3Instance[iIndex].volumeMesh[i].referenceMesh[0].numElement;
            nodeOffSet += aflr3Instance[iIndex].volumeMesh[i].referenceMesh[0].numNode;
        } else {
            printf("Getting volume mesh\n");

            status = aflr3_Volume_Mesh ( aimInfo, aimInputs,
                                         aflr3Instance[iIndex].meshInput,
                                         &aflr3Instance[iIndex].volumeMesh[i],
                                         createBL,
                                         blFlag,
                                         blSpacing,
                                         blThickness,
                                         &aflr3Instance[iIndex].volumeMesh[i]);
        }

        if (status != CAPS_SUCCESS) {
            printf("Problem during volume meshing of bodyIndex %d\n", i+1);
            goto cleanup;
        }

        if (aflr3Instance[iIndex].numVolumeMesh > 1) {
            printf("Volume mesh for body %d (of %d):\n", i+1, aflr3Instance[iIndex].numVolumeMesh);
        } else {
            printf("Volume mesh:\n");
        }
        printf("\tNumber of nodes = %d\n",  aflr3Instance[iIndex].volumeMesh[i].numNode);
        printf("\tNumber of elements = %d\n",aflr3Instance[iIndex].volumeMesh[i].numElement);
        printf("\tNumber of triangles = %d\n",aflr3Instance[iIndex].volumeMesh[i].meshQuickRef.numTriangle);
        printf("\tNumber of quadrilatarals = %d\n",aflr3Instance[iIndex].volumeMesh[i].meshQuickRef.numQuadrilateral);
        printf("\tNumber of tetrahedrals = %d\n",aflr3Instance[iIndex].volumeMesh[i].meshQuickRef.numTetrahedral);
        printf("\tNumber of pyramids= %d\n",aflr3Instance[iIndex].volumeMesh[i].meshQuickRef.numPyramid);
        printf("\tNumber of prisms= %d\n",aflr3Instance[iIndex].volumeMesh[i].meshQuickRef.numPrism);
        printf("\tNumber of hexahedrals= %d\n",aflr3Instance[iIndex].volumeMesh[i].meshQuickRef.numHexahedral);

    }

    // If filename is not NULL write the mesh
    if (aflr3Instance[iIndex].meshInput.outputFileName != NULL) {

        for (bodyIndex = 0; bodyIndex < aflr3Instance[iIndex].numVolumeMesh; bodyIndex++) {

            if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {
                sprintf(bodyIndexNumber, "%d", bodyIndex);
                filename = (char *) EG_alloc((strlen(aflr3Instance[iIndex].meshInput.outputFileName) +
                                              strlen(aflr3Instance[iIndex].meshInput.outputDirectory)+2+strlen("_Vol")+strlen(bodyIndexNumber))*sizeof(char));
            } else {
                filename = (char *) EG_alloc((strlen(aflr3Instance[iIndex].meshInput.outputFileName) +
                                              strlen(aflr3Instance[iIndex].meshInput.outputDirectory)+2)*sizeof(char));

            }

            if (filename == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            strcpy(filename, aflr3Instance[iIndex].meshInput.outputDirectory);
            #ifdef WIN32
                strcat(filename, "\\");
            #else
                strcat(filename, "/");
            #endif
            strcat(filename, aflr3Instance[iIndex].meshInput.outputFileName);

            if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {
                strcat(filename,"_Vol");
                strcat(filename,bodyIndexNumber);
            }

            if (strcasecmp(aflr3Instance[iIndex].meshInput.outputFormat, "AFLR3") == 0) {

                status = mesh_writeAFLR3(filename,
                                         aflr3Instance[iIndex].meshInput.outputASCIIFlag,
                                         &aflr3Instance[iIndex].volumeMesh[bodyIndex],
                                         1.0);

            } else if (strcasecmp(aflr3Instance[iIndex].meshInput.outputFormat, "VTK") == 0) {

                status = mesh_writeVTK(filename,
                                       aflr3Instance[iIndex].meshInput.outputASCIIFlag,
                                       &aflr3Instance[iIndex].volumeMesh[bodyIndex],
                                       1.0);

            } else if (strcasecmp(aflr3Instance[iIndex].meshInput.outputFormat, "SU2") == 0) {

                status = mesh_writeSU2(filename,
                                       aflr3Instance[iIndex].meshInput.outputASCIIFlag,
                                       &aflr3Instance[iIndex].volumeMesh[bodyIndex],
                                       aflr3Instance[iIndex].meshInput.bndConds.numBND,
                                       aflr3Instance[iIndex].meshInput.bndConds.bndID,
                                       1.0);

            } else if (strcasecmp(aflr3Instance[iIndex].meshInput.outputFormat, "Tecplot") == 0) {

                status = mesh_writeTecplot(filename,
                                           aflr3Instance[iIndex].meshInput.outputASCIIFlag,
                                           &aflr3Instance[iIndex].volumeMesh[bodyIndex],
                                           1.0);


            } else if (strcasecmp(aflr3Instance[iIndex].meshInput.outputFormat, "Nastran") == 0) {

                status = mesh_writeNASTRAN(filename,
                                           aflr3Instance[iIndex].meshInput.outputASCIIFlag,
                                           &aflr3Instance[iIndex].volumeMesh[bodyIndex],
                                           LargeField,
                                           1.0);
            } else {
                printf("Unrecognized mesh format, \"%s\", the volume mesh will not be written out\n", aflr3Instance[iIndex].meshInput.outputFormat);
            }

            if (filename != NULL) EG_free(filename);
            filename = NULL;
        }
    } else {
        printf("No project name (\"Proj_Name\") provided - A volume mesh will not be written out\n");
    }

    if (status != CAPS_SUCCESS) goto cleanup;

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:

        // Clean up meshProps
        if (meshProp != NULL) {

            for (i = 0; i < numMeshProp; i++) {

                (void) destroy_meshSizingStruct(&meshProp[i]);
            }

            EG_free(meshProp);

            meshProp = NULL;
        }

        // Free filename
        if (filename != NULL) EG_free(filename);

        // Free boundary layer parameters
        if (blFlag != NULL) EG_free(blFlag);
        if (blThickness != NULL) EG_free(blThickness);
        if (blSpacing != NULL) EG_free(blSpacing);  // Boundary layer spacing [numNode] - Total nodes for all bodies

        return status;
}

int aimOutputs(int iIndex, void *aimStruc,int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsAFLR3 AIM Outputs
     * The following list outlines the AFLR3 AIM outputs available through the AIM interface.
     *
     * - <B>Done</B> = True if a volume mesh(es) was created, False if not.
     */

    #ifdef DEBUG
        printf(" aflr3AIM/aimOutputs instance = %d  index = %d!\n", inst, index);
    #endif

    *aoname = EG_strdup("Done");
    form->type = Boolean;
    form->vals.integer = (int) false;

    return CAPS_SUCCESS;
}

int aimCalcOutput(int iIndex, void *aimInfo, const char *apath,
                  int index, capsValue *val, capsErrs **errors)
{

    int i; // Indexing

    #ifdef DEBUG
        printf(" aflr3AIM/aimCalcOutput instance = %d  index = %d!\n", inst, index);
    #endif

    *errors           = NULL;
    val->vals.integer = (int) false;

    // Check to see if a volume mesh was generated - maybe a file was written, maybe not
    for (i = 0; i < aflr3Instance[iIndex].numVolumeMesh; i++) {
        // Check to see if a volume mesh was generated
        if (aflr3Instance[iIndex].volumeMesh[i].numElement != 0 &&
            aflr3Instance[iIndex].volumeMesh[i].meshType == VolumeMesh) {

            val->vals.integer = (int) true;

        } else {
            val->vals.integer = (int) false;

            if (aflr3Instance[iIndex].numVolumeMesh > 1) {
                printf("No tetrahedral, pryamids, prisms and/or hexahedral elements were generated for volume mesh %d\n", i+1);
            } else {
                printf("No tetrahedral, pryamids, prisms and/or hexahedral elements were generated\n");
            }

            return CAPS_SUCCESS;
        }
    }

    return CAPS_SUCCESS;
}

void aimCleanup()
{
    int iIndex, status;

    #ifdef DEBUG
        printf(" aflr3AIM/aimCleanup!\n");
    #endif


    // Clean up aflr3Instance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up aflr3Instance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr3AIM instance %d, aimStorage cleanup!!!\n", status, iIndex);

    }

    if (aflr3Instance != NULL) EG_free(aflr3Instance);
    aflr3Instance = NULL;
    numInstance = 0;

}

int aimLocateElement(/*@unused@*/ capsDiscr *discr, /*@unused@*/ double *params,
                 /*@unused@*/ double *param,    /*@unused@*/ int *eIndex,
                 /*@unused@*/ double *bary)
{
    #ifdef DEBUG
        printf(" aflr3AIM/aimLocateElement instance = %d!\n", discr->instance);
    #endif

    return CAPS_SUCCESS;
}

int aimTransfer(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
            /*@unused@*/ int npts, /*@unused@*/ int rank,
            /*@unused@*/ double *data, /*@unused@*/ char **units)
{
    #ifdef DEBUG
        printf(" aflr3AIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n",
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
        printf(" aflr3AIM/aimInterpolation  %s  instance = %d!\n",name, discr->instance);
    #endif

    return CAPS_SUCCESS;
}


int aimInterpolateBar(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
                  /*@unused@*/ int eIndex, /*@unused@*/ double *bary,
                  /*@unused@*/ int rank, /*@unused@*/ double *r_bar,
                  /*@unused@*/ double *d_bar)
{
    #ifdef DEBUG
        printf(" aflr3AIM/aimInterpolateBar  %s  instance = %d!\n", name, discr->instance);
    #endif

    return CAPS_SUCCESS;
}


int aimIntegration(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
               /*@unused@*/ int eIndex, /*@unused@*/ int rank,
               /*@unused@*/ /*@null@*/ double *data, /*@unused@*/ double *result)
{
    #ifdef DEBUG
        printf(" aflr3AIM/aimIntegration  %s  instance = %d!\n", name, discr->instance);
    #endif

    return CAPS_SUCCESS;
}


int aimIntegrateBar(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
                /*@unused@*/ int eIndex, /*@unused@*/ int rank,
                /*@unused@*/ double *r_bar, /*@unused@*/ double *d_bar)
{
    #ifdef DEBUG
        printf(" aflr3AIM/aimIntegrateBar  %s  instance = %d!\n", name, discr->instance);
    #endif

    return CAPS_SUCCESS;
}
