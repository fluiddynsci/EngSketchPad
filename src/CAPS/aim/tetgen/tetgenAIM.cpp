/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             TetGen AIM
 *
 *      Written by Dr. Ryan Durscher AFRL/RQVC
 *
 */


/*! \mainpage Introduction
 *
 * \section overviewTetGen TetGen AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact with
 * the open-source volume mesh generator, TetGen \cite Hang2015. TetGen is capable of generating exact constrained
 * Delaunay tetrahedralizations, boundary conforming Delaunay meshes, and Voronoi partitions.
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsTetGen and \ref aimOutputsTetGen, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentTetGen.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataTetGen if connecting this AIM to other AIMs in a
 * parent-child like manner.
 *
 *
 * Current issues include:
 *  - The holes or seed points provided to TetGen by creating an taking the cnetroid of a tetrahedron from an 'empty' mesh.
 *    This is guaranteed to work with solid bodies, but sheet bodies with multiple segregated regions where some
 *    regions are holes require manual seed points to indicate the hole.
 *  - (<b>Important</b>) If Tetgen is allowed to added Steiner points (see "Preserve_Surf_Mesh" in \ref aimInputsTetGen)
 *  discrete data transfer will <b>NOT</b> be possible.
 *
 * \section tetgenInterface TetGen Interface
 * In order to use TetGen, CAPS will automatically build the TetGen source code supplied in "library" mode. The directory
 * in which the source code exists is set in the ESP configuration script. The C++ API is interfaced within the AIM
 * through an interface function that takes the body tessellation and transfers the data to a "tetgenio" object
 * in PLCs format (Piecewise Linear Complexes). After volume meshing is complete the mesh can be output in various
 *  mesh formats (see \ref aimInputsTetGen for additional details).
 */

#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"  // Collection of helper functions for meshing
#include "cfdUtils.h"   // Collection of helper functions for cfd analysis
#include "miscUtils.h"  // Collection of helper functions for miscellaneous use
#include "feaUtils.h"  // Collection of helper functions for fea use

#include "tetgen_Interface.hpp" // Bring in TetGen 'interface' function

#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif


#define NUMINPUT  18  // number of mesh inputs
#define NUMOUT     1  // number of outputs

//#define DEBUG


typedef struct {

    int surfaceMeshInherited;

    // Container for volume mesh
    int numVolumeMesh;
    meshStruct *volumeMesh;

    // Container for surface mesh
    int numSurfaceMesh;
    meshStruct *surfaceMesh;

    // Container for mesh input
    meshInputStruct meshInput;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

} aimStorage;

static aimStorage *tetgenInstance = NULL;
static int         numInstance  = 0;

static int destroy_aimStorage(int iIndex) {

    int i; // Indexing

    int status; // Function return status

    // Destroy meshInput
    status = destroy_meshInputStruct(&tetgenInstance[iIndex].meshInput);
    if (status != CAPS_SUCCESS) printf("Status = %d, tetgenAIM instance %d, meshInput cleanup!!!\n", status, iIndex);

    // Destroy surfaceMesh allocated arrays
    if (tetgenInstance[iIndex].surfaceMeshInherited == (int) false) { // Did the parent aim already destroy the surfaces?

        if (tetgenInstance[iIndex].surfaceMesh != NULL) {
            for (i = 0; i < tetgenInstance[iIndex].numSurfaceMesh; i++) {
                status = destroy_meshStruct(&tetgenInstance[iIndex].surfaceMesh[i]);
                if (status != CAPS_SUCCESS) printf("Error during destroy_meshStruct, status = %d\n", status);
            }
            EG_free(tetgenInstance[iIndex].surfaceMesh);
        }
    }

    tetgenInstance[iIndex].numSurfaceMesh = 0;
    tetgenInstance[iIndex].surfaceMeshInherited = (int) false;
    tetgenInstance[iIndex].surfaceMesh = NULL;

    // Destroy volumeMesh allocated arrays
    if (tetgenInstance[iIndex].volumeMesh != NULL) {
        for (i = 0; i < tetgenInstance[iIndex].numVolumeMesh; i++) {
            status = destroy_meshStruct(&tetgenInstance[iIndex].volumeMesh[i]);
            if (status != CAPS_SUCCESS) printf("Error during destroy_meshStruct, status = %d\n", status);
        }

        EG_free(tetgenInstance[iIndex].volumeMesh);
    }
    tetgenInstance[iIndex].numVolumeMesh = 0;
    tetgenInstance[iIndex].volumeMesh = NULL;

    // Destroy attribute to index map
    status = destroy_mapAttrToIndexStruct(&tetgenInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Status = %d, tetgenAIM instance %d, attrMap cleanup!!!\n", status, iIndex);

    return CAPS_SUCCESS;
}



/* ********************** Exposed AIM Functions ***************************** */

extern "C" int
aimInitialize(int ngIn, capsValue *gIn, int *qeFlag, const char *unitSys,
              int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int status; // Function status return
    int flag;   // query/execute flag

    aimStorage *tmp;

#ifdef DEBUG
    printf("\n tetgenAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif
    flag     = *qeFlag;
    // Set that the AIM executes itself
    *qeFlag  = 1;

    /* specify the number of analysis input and out "parameters" */
    *nIn     = NUMINPUT;
    *nOut    = NUMOUT;
    if (flag == 1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate */
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Allocate tetgenInstance
    if (tetgenInstance == NULL) {
        tetgenInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (tetgenInstance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(tetgenInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        tetgenInstance = tmp;
    }

    // Set initial values for tetgenInstance //

    // Container for volume mesh
    tetgenInstance[numInstance].numVolumeMesh = 0;
    tetgenInstance[numInstance].volumeMesh = NULL;

    // Container for surface meshes
    tetgenInstance[numInstance].surfaceMeshInherited = (int) false;

    tetgenInstance[numInstance].numSurfaceMesh = 0;
    tetgenInstance[numInstance].surfaceMesh = NULL;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&tetgenInstance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for mesh input
    status = initiate_meshInputStruct(&tetgenInstance[numInstance].meshInput);

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}


extern "C" int
aimInputs(int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{
    /*! \page aimInputsTetGen AIM Inputs
     * The following list outlines the TetGen meshing options along with their default value available
     * through the AIM interface.
     */

#ifdef DEBUG
    printf(" tetgenAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
#endif

    // Meshing Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name"); // If NULL a volume grid won't be written by the AIM
        defval->type         = String;
        defval->nullVal      = IsNull;
        defval->vals.string  = NULL;
        //defval->vals.string  = EG_strdup("CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsTetGen
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

        /*! \page aimInputsTetGen
         * - <B> Tess_Params = [0.025, 0.001, 15.0]</B> <br>
         * Body tessellation parameters. Tess_Params[0] and Tess_Params[1] get scaled by the bounding
         * box of the body. (From the EGADS manual) A set of 3 parameters that drive the EDGE discretization
         * and the FACE triangulation. The first is the maximum length of an EDGE segment or triangle side
         * (in physical space). A zero is flag that allows for any length. The second is a curvature-based
         * value that looks locally at the deviation between the centroid of the discrete object and the
         * underlying geometry. Any deviation larger than the input value will cause the tessellation to
         * be enhanced in those regions. The third is the maximum interior dihedral angle (in degrees)
         * between triangle facets (or Edge segment tangents for a WIREBODY tessellation), note that a
         * zero ignores this phase
         */
    } else if (index == 3) {
        *ainame               = EG_strdup("Preserve_Surf_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsTetGen
         * - <B> Preserve_Surf_Mesh = True</B> <br>
         * Tells TetGen to preserve the surface mesh provided (i.e. do not add Steiner points on the surface). Discrete data transfer
         * will <b>NOT</b> be possible if Steiner points are added.
         */
    } else if (index == 4) {
        *ainame               = EG_strdup("Mesh_Verbose_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = false;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Verbose_Flag = False</B> <br>
         * Verbose output from TetGen.
         */
    } else if (index == 5) {
        *ainame               = EG_strdup("Mesh_Quiet_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = false;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Quiet_Flag = False</B> <br>
         * Complete suppression of all TetGen output messages (not including errors).
         */
    } else if (index == 6) {
        *ainame               = EG_strdup("Quality_Rad_Edge"); // TetGen specific parameters
        defval->type          = Double;
        defval->vals.real     = 1.5;

        /*! \page aimInputsTetGen
         * - <B> Quality_Rad_Edge = 1.5</B> <br>
         * TetGen maximum radius-edge ratio.
         */
    } else if (index == 7) {
        *ainame               = EG_strdup("Quality_Angle"); // TetGen specific parameters
        defval->type          = Double;
        defval->vals.real     = 0.0;

        /*! \page aimInputsTetGen
         * - <B> Quality_Angle = 0.0</B> <br>
         * TetGen minimum dihedral angle (in degrees).
         */
    } else if (index == 8) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->vals.string   = EG_strdup("AFLR3"); // TECPLOT, VTK, AFLR3, SU2, NASTRAN
        defval->lfixed        = Change;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Format = "AFLR3"</B> <br>
         * Mesh output format. Available format names  include: "AFLR3", "TECPLOT", "SU2", "VTK", and "NASTRAN".
         */
    } else if (index == 9) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsTetGen
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file if applicable.
         */
    } else if (index == 10) {
        *ainame               = EG_strdup("Mesh_Gen_Input_String");
        defval->type          = String;
        defval->nullVal       = IsNull;
        defval->vals.string   = NULL;
        defval->lfixed       = Change;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Gen_Input_String = NULL</B> <br>
         * Meshing program command line string (as if called in bash mode). Use this to specify more
         * complicated options/use features of the mesher not currently exposed through other AIM input
         * variables. Note that this is the exact string that will be provided to the volume mesher; no
         * modifications will be made. If left NULL an input string will be created based on default values
         * of the relevant AIM input variables. See \ref tetgenCommandLineInput for options to include
         * in the input string.
         */
    } else if (index == 11) { // Deprecated
        *ainame               = EG_strdup("Ignore_Surface_Mesh_Extraction");
        defval->type          = Boolean;
        defval->vals.integer  = true;

//        /*! \page aimInputsTetGen
//         * - <B> Ignore_Surface_Mesh_Extraction = True</B> <br>
//         * If TetGen doesn't preserve the surface mesh provided (i.e. Steiner points are added) a simple search algorithm may be
//         * used to reconstruct a separate (not dependent on the volume mesh node numbering) representation of the surface mesh. In
//         * general, this has little use and can add a significant computational penalty. The default value of "True" is recommended.
//         */
    } else if (index == 12) {
        *ainame               = EG_strdup("Mesh_Tolerance"); // TetGen specific parameters
        defval->type          = Double;
        defval->vals.real     = 1E-16;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Tolerance = 1E-16</B> <br>
         * Sets the tolerance for coplanar test in TetGen.
         */
    } else if (index == 13) {
        *ainame               = EG_strdup("Multiple_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = (int) false;

        /*! \page aimInputsTetGen
         * - <B> Multiple_Mesh = False</B> <br>
         * If set to True a volume will be generated for each body. When set to False (default value) only a single volume
         * mesh will be created.
         */
    } else if (index == 14) {
        *ainame               = EG_strdup("Edge_Point_Min");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsTetGen
         * - <B> Edge_Point_Min = NULL</B> <br>
         * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 15) {
        *ainame               = EG_strdup("Edge_Point_Max");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->length        = 1;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsTetGen
         * - <B> Edge_Point_Max = NULL</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 16) {
        *ainame              = EG_strdup("Mesh_Sizing");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsTetGen
         * - <B>Mesh_Sizing = NULL </B> <br>
         * See \ref meshSizingProp for additional details.
         */
    } else if (index == 17) {
        *ainame              = EG_strdup("Regions");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsTetGen
         * - <B>Regions = NULL</B><br>
         * If this input is set, the volume mesh will be divided into regions,
         * each bounded by surface mesh and identified by an interior `seed`
         * point.  If a seed appears in the `Regions` input, then its region
         * will be meshed and the markers for all cells in the region will be
         * set to the region's `id`.  The markers for cells that fall outside
         * of any user-defined region or hole will be numbered automatically.
         * The input is a vector of tuples.  The tuple keys are ignored and the
         * tuple values are dictionaries; each requires an integer `id` entry
         * and a 3-vector `seed` point.  For example, from within a pyCAPS
         * script,
         *
         *  \code{.py}
         *  tetgen.setAnalysisVal('Regions', [
         *    ( 'A', { 'id': 10, 'seed': [0, 0,  1] } ),
         *    ( 'B', { 'id': 20, 'seed': [0, 0, -1] } )
         *  ])
         *  \endcode
         *
         * Automatic hole detection will be disabled if one or both of the
         * `Regions` and `Holes` inputs is not NULL.
         */
    } else if (index == 18) {
      *ainame              = EG_strdup("Holes");
      defval->type         = Tuple;
      defval->nullVal      = IsNull;
      defval->dim          = Vector;
      defval->lfixed       = Change;
      defval->vals.tuple   = NULL;

      /*! \page aimInputsTetGen
       * - <B>Holes = NULL</B><br>
       * If this input is set, the volume mesh will be divided into regions,
       * each bounded by surface mesh and identified by an interior `seed`
       * point.  If a seed appears in the `Holes` input, then its region will
       * not be meshed.  The input is a vector of tuples.  The tuple keys are
       * ignored and the tuple values are dictionaries; each requires a
       * 3-vector `seed` point.  For example, from within a pyCAPS script,
       *
       *  \code{.py}
       *  tetgen.setAnalysisVal('Holes', [
       *    ( 'A', { 'seed': [ 1, 0, 0] } ),
       *    ( 'B', { 'seed': [-1, 0, 0] } )
       *  ])
       *  \endcode
       *
       * Automatic hole detection will be disabled if one or both of the
       * `Regions` and `Holes` inputs is not NULL.
       */
    }


    /* if (index != 1 && index != 10 && index != 13 ){
        // Link variable(s) to parent(s) if available
        status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
        //printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n", status, index, defval->type, defval->link);
    }*/

    return CAPS_SUCCESS;
}

extern "C" int
aimData(int iIndex, const char *name, enum capsvType *vtype, int *rank, int *nrow,
        int *ncol, void **data, char **units)
{

    /*! \page sharableDataTetGen AIM Shareable Data
     * The TetGen AIM has the following shareable data types/values with its children AIMs if they are so inclined.
     * - <B> Volume_Mesh</B> <br>
     * The returned volume mesh after TetGen execution is complete in meshStruct (see meshTypes.h) format.
     * - <B> Attribute_Map</B> <br>
     * An index mapping between capsGroups found on the geometry in mapAttrToIndexStruct (see miscTypes.h) format.
     */

#ifdef DEBUG
    printf(" tetgenAIM/aimData instance = %d  name = %s!\n", iIndex, name);
#endif

//    * - <B> Surface_Mesh</B> <br>
//    * The returned surface mesh in meshStruct (see meshTypes.h) format.

//    // The returned surface mesh from TetGen
//    if (strcasecmp(name, "Surface_Mesh") == 0){
//        *vtype = Value;
//        *rank  = *ncol = 1;
//        *nrow = tetgenInstance[iIndex].numSurfaceMesh;
//        *data  = &tetgenInstance[iIndex].surfaceMesh;
//        *units = NULL;
//
//        return CAPS_SUCCESS;
//    }

    // The returned Volume mesh from TetGen
    if (strcasecmp(name, "Volume_Mesh") == 0){
        *vtype = Value;
        *rank  = *ncol = 1;
        *nrow = tetgenInstance[iIndex].numVolumeMesh;
        if (tetgenInstance[iIndex].numVolumeMesh == 1) {
            *data  = &tetgenInstance[iIndex].volumeMesh[0];
        } else {
            *data  = tetgenInstance[iIndex].volumeMesh;
        }

        *units = NULL;

        return CAPS_SUCCESS;
    }

    // Share the attribute map
    if (strcasecmp(name, "Attribute_Map") == 0){
        *vtype = Value;
        *rank  = *nrow = *ncol = 1;
        *data  = &tetgenInstance[iIndex].attrMap;
        *units = NULL;

        return CAPS_SUCCESS;
    }

    return CAPS_NOTFOUND;
}


extern "C" int
aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath, capsValue *aimInputs,
        capsErrs **errs)
{
    int i, elem, body; // Indexing

    int status; // Return status

    // Incoming bodies
    const char *intents;
    ego *bodies = NULL;
    int numBody = 0;

    // Data transfer related variables
    int            nrow, ncol, rank;
    void           *dataTransfer;
    char           *units;
    enum capsvType vtype;

    // Meshing related variables
    double  tessParams[3], refLen = -1.0;
    meshElementStruct *element;

    // Global settings
    int minEdgePoint = -1, maxEdgePoint = -1;

    // Mesh attribute parameters
    int numMeshProp = 0;
    meshSizingStruct *meshProp = NULL;

    double  box[6], boxMax[6] = {0,0,0,0,0,0};

    int     bodyBoundingBox = 0;

    // Inherit surface mesh related variables
    int numSurfMesh = 0;

    // File ouput
    char *filename;
    char bodyNumber[11];

    // NULL out errors
    *errs = NULL;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

#ifdef DEBUG
    printf(" tetgenAIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex,
            numBody);
#endif

    if (numBody <= 0 || bodies == NULL) {
#ifdef DEBUG
        printf(" tetgenAIM/aimPreAnalysis No Bodies!\n");
#endif
        return CAPS_SOURCEERR;
    }

    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the same instance
    status = destroy_aimStorage(iIndex);
    if (status != CAPS_SUCCESS) {
        if (status != CAPS_SUCCESS) printf("Status = %d, tetgenAIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
        return status;
    }

    // Get capsGroup name and index mapping
    status = aim_getData(aimInfo, "Attribute_Map", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
    if (status == CAPS_SUCCESS) {

        printf("Found link for attrMap (Attribute_Map) from parent\n");

        status = copy_mapAttrToIndexStruct( (mapAttrToIndexStruct *) dataTransfer, &tetgenInstance[iIndex].attrMap);
        if (status != CAPS_SUCCESS) return status;

    } else {

        if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
        else printf("Linking status during attrMap (Attribute_Map) = %d\n",status);

        printf("Didn't find a link to attrMap from parent - getting it ourselves\n");

        // Get capsGroup name and index mapping
        status = create_CAPSGroupAttrToIndexMap(numBody,
                                                bodies,
                                                2, // Only search down to the edge level of the EGADS body
                                                &tetgenInstance[iIndex].attrMap);
        if (status != CAPS_SUCCESS) return status;
    }

    // Tessellate body(ies)
    // Tess_Params -> -1 because of aim_getIndex 1 bias
    tessParams[0] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[0]; // Gets multiplied by bounding box size
    tessParams[1] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[1]; // Gets multiplied by bounding box size
    tessParams[2] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[2];

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
        tetgenInstance[iIndex].numSurfaceMesh =  numSurfMesh;
        tetgenInstance[iIndex].surfaceMesh = (meshStruct * ) dataTransfer;
        tetgenInstance[iIndex].surfaceMeshInherited = (int) true;

    } else { // Use EGADS body tessellation for surface mesh

        tetgenInstance[iIndex].numSurfaceMesh = numBody;
        tetgenInstance[iIndex].surfaceMesh = (meshStruct * ) EG_alloc(tetgenInstance[iIndex].numSurfaceMesh*sizeof(meshStruct));
        if (tetgenInstance[iIndex].surfaceMesh == NULL) return EGADS_MALLOC;

        for (body = 0; body < numBody; body++) {
            status = initiate_meshStruct(&tetgenInstance[iIndex].surfaceMesh[body]);
            if (status != CAPS_SUCCESS) return status;
        }

        tetgenInstance[iIndex].surfaceMeshInherited = (int) false;

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
                                        &tetgenInstance[iIndex].attrMap,
                                        &numMeshProp,
                                        &meshProp);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        // Modify the EGADS body tessellation based on given inputs
        status =  mesh_modifyBodyTess(numMeshProp,
                                      meshProp,
                                      minEdgePoint,
                                      maxEdgePoint,
                                      0, // quadMesh
                                      &refLen,
                                      tessParams,
                                      tetgenInstance[iIndex].attrMap,
                                      numBody,
                                      bodies);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Clean up meshProps
        if (meshProp != NULL) {

            for (i = 0; i < numMeshProp; i++) {

                (void) destroy_meshSizingStruct(&meshProp[i]);
            }
            EG_free(meshProp);
            meshProp = NULL;
        }

        for (body = 0; body < numBody; body++) {

            printf("Getting surface mesh for body %d (of %d)\n", body+1, numBody);

            status = mesh_surfaceMeshEGADSBody(bodies[body],
                                               refLen,
                                               tessParams,
                                               (int)false, //quadMesh
                                               &tetgenInstance[iIndex].attrMap,
                                               &tetgenInstance[iIndex].surfaceMesh[body]);
            if (status != CAPS_SUCCESS) return status;

        }
    }

    // Create/setup volume meshes
    if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {

        tetgenInstance[iIndex].numVolumeMesh = numBody;
        tetgenInstance[iIndex].volumeMesh = (meshStruct *) EG_alloc(tetgenInstance[iIndex].numVolumeMesh *sizeof(meshStruct));
        if (tetgenInstance[iIndex].volumeMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (body = 0; body < tetgenInstance[iIndex].numVolumeMesh; body++) {
            status = initiate_meshStruct(&tetgenInstance[iIndex].volumeMesh[body]);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Set reference mesh - One surface per body
            tetgenInstance[iIndex].volumeMesh[body].numReferenceMesh = 1;
            tetgenInstance[iIndex].volumeMesh[body].referenceMesh = (meshStruct *) EG_alloc(tetgenInstance[iIndex].volumeMesh[body].numReferenceMesh*
                                                                                            sizeof(meshStruct));
            if (tetgenInstance[iIndex].volumeMesh[body].referenceMesh == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            tetgenInstance[iIndex].volumeMesh[body].referenceMesh[0] = tetgenInstance[iIndex].surfaceMesh[body];
            printf("Tetgen MultiMesh TopoIndex = %d\n", tetgenInstance[iIndex].volumeMesh[0].referenceMesh[0].element[0].topoIndex);

        }

    } else {

        // Determine which body is the bounding body based on size
        bodyBoundingBox = 0;
        if (numBody > 1) {

            for (body = 0; body < numBody; body++) {

                // Get bounding box for the body
                status = EG_getBoundingBox(bodies[body], box);
                if (status != EGADS_SUCCESS) {
                    printf(" EG_getBoundingBox = %d\n\n", status);
                    return status;
                }

                // Just copy the box coordinates on the first go around
                if (body == 0) {

                    memcpy(boxMax, box, sizeof(box));

                    // Set body as the bounding box (ie. farfield)
                    bodyBoundingBox = body;

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
                    bodyBoundingBox = body;
                }
            }
        }

        // Swap the surface element orientation so normals point out of the computational domain
        for (body = 0; body < numBody; body++) {
            if (body != bodyBoundingBox) {
                // Swap two indices to reverse the normal vector of all elements on internal bodies
                // so they point out of the domain
                for (elem = 0; elem < tetgenInstance[iIndex].surfaceMesh[body].numElement; elem++) {

                    element = tetgenInstance[iIndex].surfaceMesh[body].element + elem;

                    // This should be valid for both Triangles and Quadrilaterals
                    i = element->connectivity[2];
                    element->connectivity[2] = element->connectivity[0];
                    element->connectivity[0] = i;
                }
            }
        }

        tetgenInstance[iIndex].numVolumeMesh = 1;
        tetgenInstance[iIndex].volumeMesh = (meshStruct *) EG_alloc(tetgenInstance[iIndex].numVolumeMesh*sizeof(meshStruct));
        if (tetgenInstance[iIndex].volumeMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        status = initiate_meshStruct(&tetgenInstance[iIndex].volumeMesh[0]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Combine mesh - temporary store the combined mesh in the volume mesh
        status = mesh_combineMeshStruct(tetgenInstance[iIndex].numSurfaceMesh,
                                        tetgenInstance[iIndex].surfaceMesh,
                                        &tetgenInstance[iIndex].volumeMesh[0]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Set reference meshes - All surfaces
        tetgenInstance[iIndex].volumeMesh[0].numReferenceMesh = tetgenInstance[iIndex].numSurfaceMesh;
        tetgenInstance[iIndex].volumeMesh[0].referenceMesh = (meshStruct *) EG_alloc(tetgenInstance[iIndex].volumeMesh[0].numReferenceMesh*
                                                                                     sizeof(meshStruct));
        if (tetgenInstance[iIndex].volumeMesh[0].referenceMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (body = 0; body < tetgenInstance[iIndex].numSurfaceMesh; body++) {
            tetgenInstance[iIndex].volumeMesh[0].referenceMesh[body] = tetgenInstance[iIndex].surfaceMesh[body];

        }

         // Report surface mesh
        printf("Number of surface nodes - %d\n",tetgenInstance[iIndex].volumeMesh[0].numNode);
        printf("Number of surface elements - %d\n",tetgenInstance[iIndex].volumeMesh[0].numElement);
    }

    // Setup meshing input structure - mesher specific input gets set below before entering interface
    //                                                      -1 because aim_getIndex is 1 bias
    tetgenInstance[iIndex].meshInput.preserveSurfMesh = aimInputs[aim_getIndex(aimInfo, "Preserve_Surf_Mesh", ANALYSISIN)-1].vals.integer;
    tetgenInstance[iIndex].meshInput.quiet            = aimInputs[aim_getIndex(aimInfo, "Mesh_Quiet_Flag",    ANALYSISIN)-1].vals.integer;
    tetgenInstance[iIndex].meshInput.outputASCIIFlag  = aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag",    ANALYSISIN)-1].vals.integer;

    tetgenInstance[iIndex].meshInput.outputFormat = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].vals.string);
    if (tetgenInstance[iIndex].meshInput.outputFormat == NULL) return EGADS_MALLOC;

    if (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].nullVal != IsNull) {

        tetgenInstance[iIndex].meshInput.outputFileName = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
        if (tetgenInstance[iIndex].meshInput.outputFileName == NULL) return EGADS_MALLOC;
    }

    tetgenInstance[iIndex].meshInput.outputDirectory = EG_strdup(analysisPath);
    if (tetgenInstance[iIndex].meshInput.outputDirectory == NULL) return EGADS_MALLOC;

    // Set tetgen specific mesh inputs
    //                                                                          -1 because aim_getIndex is 1 bias
    tetgenInstance[iIndex].meshInput.tetgenInput.meshQuality_rad_edge = aimInputs[aim_getIndex(aimInfo, "Quality_Rad_Edge"              , ANALYSISIN)-1].vals.real;
    tetgenInstance[iIndex].meshInput.tetgenInput.meshQuality_angle    = aimInputs[aim_getIndex(aimInfo, "Quality_Angle"                 , ANALYSISIN)-1].vals.real;
    tetgenInstance[iIndex].meshInput.tetgenInput.verbose              = aimInputs[aim_getIndex(aimInfo, "Mesh_Verbose_Flag"             , ANALYSISIN)-1].vals.integer;
    tetgenInstance[iIndex].meshInput.tetgenInput.ignoreSurfaceExtract = aimInputs[aim_getIndex(aimInfo, "Ignore_Surface_Mesh_Extraction", ANALYSISIN)-1].vals.integer;
    tetgenInstance[iIndex].meshInput.tetgenInput.meshTolerance        = aimInputs[aim_getIndex(aimInfo, "Mesh_Tolerance"                , ANALYSISIN)-1].vals.real;

    if (aimInputs[aim_getIndex(aimInfo, "Regions", ANALYSISIN) - 1].nullVal != IsNull) {
      populate_regions(&tetgenInstance[iIndex].meshInput.tetgenInput.regions,
                       aimInputs[aim_getIndex(aimInfo, "Regions", ANALYSISIN) - 1].length,
                       aimInputs[aim_getIndex(aimInfo, "Regions", ANALYSISIN) - 1].vals.tuple);
    }
    if (aimInputs[aim_getIndex(aimInfo, "Holes", ANALYSISIN) - 1].nullVal != IsNull) {
      populate_holes(&tetgenInstance[iIndex].meshInput.tetgenInput.holes,
                     aimInputs[aim_getIndex(aimInfo, "Holes", ANALYSISIN) - 1].length,
                     aimInputs[aim_getIndex(aimInfo, "Holes", ANALYSISIN) - 1].vals.tuple);
    }

    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].nullVal != IsNull) {

        tetgenInstance[iIndex].meshInput.tetgenInput.meshInputString = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].vals.string);
        if (tetgenInstance[iIndex].meshInput.tetgenInput.meshInputString == NULL) return EGADS_MALLOC;
    }

    status = populate_bndCondStruct_from_mapAttrToIndexStruct(&tetgenInstance[iIndex].attrMap, &tetgenInstance[iIndex].meshInput.bndConds);
    if (status != CAPS_SUCCESS) return status;

    // Call tetgen volume mesh interface
    for (body = 0; body < tetgenInstance[iIndex].numVolumeMesh; body++) {

        // Call tetgen volume mesh interface for each body
        if (tetgenInstance[iIndex].numVolumeMesh > 1) {
            printf("Getting volume mesh for body %d (of %d)\n", body+1, numBody);

            status = tetgen_VolumeMesh(tetgenInstance[iIndex].meshInput,
                                      &tetgenInstance[iIndex].volumeMesh[body].referenceMesh[0],
                                      &tetgenInstance[iIndex].volumeMesh[body]);
        } else {
            printf("Getting volume mesh\n");

            status = tetgen_VolumeMesh(tetgenInstance[iIndex].meshInput,
                                      &tetgenInstance[iIndex].volumeMesh[body],
                                      &tetgenInstance[iIndex].volumeMesh[body]);
        }

        if (status != CAPS_SUCCESS) {

            if (numBody > 1) printf("TetGen volume mesh failed on body - %d!!!!\n", body+1);
            else printf("TetGen volume mesh failed!!!!\n");
            return status;
        }
    }

    // If filename is not NULL write the mesh
    if (tetgenInstance[iIndex].meshInput.outputFileName != NULL) {

        for (body = 0; body < tetgenInstance[iIndex].numVolumeMesh; body++) {

            if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {
                sprintf(bodyNumber, "%d", body);
                filename = (char *) EG_alloc((strlen(tetgenInstance[iIndex].meshInput.outputFileName) +
                                              strlen(tetgenInstance[iIndex].meshInput.outputDirectory)+2+strlen("_Vol")+strlen(bodyNumber))*sizeof(char));
            } else {
                filename = (char *) EG_alloc((strlen(tetgenInstance[iIndex].meshInput.outputFileName) +
                                              strlen(tetgenInstance[iIndex].meshInput.outputDirectory)+2)*sizeof(char));

            }

            if (filename == NULL) return EGADS_MALLOC;

            strcpy(filename, tetgenInstance[iIndex].meshInput.outputDirectory);
#ifdef WIN32
            strcat(filename, "\\");
#else
            strcat(filename, "/");
#endif
            strcat(filename, tetgenInstance[iIndex].meshInput.outputFileName);

            if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {
                strcat(filename,"_Vol");
                strcat(filename,bodyNumber);
            }

            if (strcasecmp(tetgenInstance[iIndex].meshInput.outputFormat, "AFLR3") == 0) {

                status = mesh_writeAFLR3(filename,
                                         tetgenInstance[iIndex].meshInput.outputASCIIFlag,
                                         &tetgenInstance[iIndex].volumeMesh[body],
                                         1.0);

            } else if (strcasecmp(tetgenInstance[iIndex].meshInput.outputFormat, "VTK") == 0) {

                status = mesh_writeVTK(filename,
                                       tetgenInstance[iIndex].meshInput.outputASCIIFlag,
                                       &tetgenInstance[iIndex].volumeMesh[body],
                                       1.0);

            } else if (strcasecmp(tetgenInstance[iIndex].meshInput.outputFormat, "SU2") == 0) {

                status = mesh_writeSU2(filename,
                                       tetgenInstance[iIndex].meshInput.outputASCIIFlag,
                                       &tetgenInstance[iIndex].volumeMesh[body],
                                       tetgenInstance[iIndex].meshInput.bndConds.numBND,
                                       tetgenInstance[iIndex].meshInput.bndConds.bndID,
                                       1.0);

            } else if (strcasecmp(tetgenInstance[iIndex].meshInput.outputFormat, "Tecplot") == 0) {

                status = mesh_writeTecplot(filename,
                                           tetgenInstance[iIndex].meshInput.outputASCIIFlag,
                                           &tetgenInstance[iIndex].volumeMesh[body],
                                           1.0);

            } else if (strcasecmp(tetgenInstance[iIndex].meshInput.outputFormat, "Nastran") == 0) {

                status = mesh_writeNASTRAN(filename,
                                           tetgenInstance[iIndex].meshInput.outputASCIIFlag,
                                           &tetgenInstance[iIndex].volumeMesh[body],
                                           LargeField,
                                           1.0);

            } else {
                printf("Unrecognized mesh format, \"%s\", the volume mesh will not be written out\n", tetgenInstance[iIndex].meshInput.outputFormat);
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

        if (meshProp != NULL) {

            for (i = 0; i < numMeshProp; i++) {

                (void) destroy_meshSizingStruct(&meshProp[i]);
            }

            EG_free(meshProp);
        }

        return status;
}

extern "C" int
aimOutputs(/*@unused@*/ int iIndex, void *aimInfo,  int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsTetGen AIM Outputs
     * The following list outlines the TetGen AIM outputs available through the AIM interface.
     *
     * - <B>Done</B> = True if a volume mesh(es) was created, False if not.
     */

#ifdef DEBUG
    printf(" tetgenAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
#endif

    *aoname = EG_strdup("Done");
    form->type = Boolean;
    form->vals.integer = (int) false;

    return CAPS_SUCCESS;
}


extern "C" int
aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath, int index, capsValue *val,
        capsErrs **errors)
{
    int i; //Indexing

#ifdef DEBUG
    printf(" tetgenAIM/aimCalcOutput instance = %d  index = %d!\n", iIndex, index);
#endif

    *errors           = NULL;
    val->vals.integer = (int) false;

    // Check to see if a volume mesh was generated - maybe a file was written, maybe not
    for (i = 0; i < tetgenInstance[iIndex].numVolumeMesh; i++) {
        // Check to see if a volume mesh was generated
        if (tetgenInstance[iIndex].volumeMesh[i].numElement != 0 &&
            tetgenInstance[iIndex].volumeMesh[i].meshType == VolumeMesh) {

            val->vals.integer = (int) true;

        } else {

            val->vals.integer = (int) false;

            if (tetgenInstance[iIndex].numVolumeMesh > 1) {
                printf("No tetrahedral elements were generated for volume mesh %d\n", i+1);
            } else {
                printf("No tetrahedral elements were generate\n");
            }

            return CAPS_SUCCESS;
        }
    }

    return CAPS_SUCCESS;
}

extern "C" void
aimCleanup()
{
    int iIndex; //Indexing

    int status; // Function return status

#ifdef DEBUG
    printf(" tetgenAIM/aimCleanup!\n");

#endif

    // Clean up tetgenInstance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up tetgenInstance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS) printf("Status = %d, tetgenAIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
    }

    if (tetgenInstance != NULL) EG_free(tetgenInstance);
    tetgenInstance = NULL;
    numInstance = 0;
}

/************************************************************************/

/*
 * since this AIM does not support field variables or CAPS bounds, the
 * following functions are not required to be filled in except for aimDiscr
 * which just stores away our bodies and aimFreeDiscr that does some cleanup
 * when CAPS terminates
 */



extern "C" int
aimFreeDiscr(capsDiscr *discr)
{
#ifdef DEBUG
    printf(" tetgenAIM/aimFreeDiscr instance = %d!\n", discr->instance);
#endif

    return CAPS_SUCCESS;
}

extern "C" int
aimDiscr(char *transferName, capsDiscr *discr)
{
    int stat, inst;

    inst = discr->instance;

#ifdef DEBUG
    printf(" tetgenAIM/aimDiscr: transferName = %s, instance = %d!\n", transferName, inst);
#endif

    if ((inst < 0) || (inst >= numInstance)) return CAPS_BADINDEX;

    stat = aimFreeDiscr(discr);
    if (stat != CAPS_SUCCESS) return stat;

    return CAPS_SUCCESS;
}


extern "C" int
aimLocateElement(/*@unused@*/ capsDiscr *discr, /*@unused@*/ double *params,
        /*@unused@*/ double *param,    /*@unused@*/ int *eIndex,
        /*@unused@*/ double *bary)
{
#ifdef DEBUG
    printf(" tetgenAIM/aimLocateElement instance = %d!\n", discr->instance);
#endif

    return CAPS_SUCCESS;
}

extern "C" int
aimTransfer(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
        /*@unused@*/ int npts, /*@unused@*/ int rank,
        /*@unused@*/ double *data, /*@unused@*/ char **units)
{
#ifdef DEBUG
    printf(" tetgenAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n",
            name, discr->instance, npts, rank);
#endif

    return CAPS_SUCCESS;
}

extern "C" int
aimInterpolation(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
        /*@unused@*/ int eIndex, /*@unused@*/ double *bary,
        /*@unused@*/ int rank, /*@unused@*/ double *data,
        /*@unused@*/ double *result)
{
#ifdef DEBUG
    printf(" tetgenAIM/aimInterpolation  %s  instance = %d!\n",
            name, discr->instance);
#endif

    return CAPS_SUCCESS;
}


extern "C" int
aimInterpolateBar(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
        /*@unused@*/ int eIndex, /*@unused@*/ double *bary,
        /*@unused@*/ int rank, /*@unused@*/ double *r_bar,
        /*@unused@*/ double *d_bar)
{
#ifdef DEBUG
    printf(" tetgenAIM/aimInterpolateBar  %s  instance = %d!\n",
            name, discr->instance);
#endif

    return CAPS_SUCCESS;
}


extern "C" int
aimIntegration(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
        /*@unused@*/ int eIndex, /*@unused@*/ int rank,
        /*@unused@*/ /*@null@*/ double *data, /*@unused@*/ double *result)
{
#ifdef DEBUG
    printf(" tetgenAIM/aimIntegration  %s  instance = %d!\n",
            name, discr->instance);
#endif

    return CAPS_SUCCESS;
}


extern "C" int
aimIntegrateBar(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
        /*@unused@*/ int eIndex, /*@unused@*/ int rank,
        /*@unused@*/ double *r_bar, /*@unused@*/ double *d_bar)
{
#ifdef DEBUG
    printf(" tetgenAIM/aimIntegrateBar  %s  instance = %d!\n",
            name, discr->instance);
#endif

    return CAPS_SUCCESS;
}
