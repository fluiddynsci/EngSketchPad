/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             TetGen AIM
 *
 *      Written by Dr. Ryan Durscher AFRL/RQVC
 *
 *     This software has been cleared for public release on 05 Nov 2020, case number 88ABW-2020-3462.
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
 *
 *  \section clearanceTetgen Clearance Statement
 *  This software has been cleared for public release on 05 Nov 2020, case number 88ABW-2020-3462.
 */

#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"       // Collection of helper functions for meshing
#include "miscUtils.h"
#include "deprecateUtils.h"

#include "tetgen_Interface.hpp" // Bring in TetGen 'interface' function

#ifdef WIN32
#define snprintf   _snprintf
#define strcasecmp stricmp
#endif

//#define DEBUG


enum aimInputs
{
  Proj_Name = 1,                 /* index is 1-based */
  Tess_Params,
  Preserve_Surf_Mesh,
  Mesh_Verbose_Flag,
  Mesh_Quiet_Flag,
  Quality_Rad_Edge,
  Quality_Angle,
  Mesh_Format,
  Mesh_ASCII_Flag,
  Mesh_Gen_Input_String,
  Ignore_Surface_Mesh_Extraction,
  Mesh_Tolerance,
  Multiple_Mesh,
  Regions,
  Holes,
  Surface_Mesh,
  NUMINPUT = Surface_Mesh        /* Total number of inputs */
};

enum aimOutputs
{
  Done = 1,                    /* index is 1-based */
  Volume_Mesh,
  NUMOUT = Volume_Mesh         /* Total number of outputs */
};


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



static int destroy_aimStorage(aimStorage *tetgenInstance)
{

    int i; // Indexing

    int status; // Function return status

    // Destroy meshInput
    status = destroy_meshInputStruct(&tetgenInstance->meshInput);
    if (status != CAPS_SUCCESS)
      printf("Status = %d, tetgenAIM  meshInput cleanup!!!\n", status);

    // Destroy surfaceMesh allocated arrays
    if (tetgenInstance->surfaceMeshInherited == (int) false) {
        // Did the parent aim already destroy the surfaces?
        if (tetgenInstance->surfaceMesh != NULL) {
            for (i = 0; i < tetgenInstance->numSurfaceMesh; i++) {
                status = destroy_meshStruct(&tetgenInstance->surfaceMesh[i]);
                if (status != CAPS_SUCCESS)
                  printf("Error during destroy_meshStruct, status = %d\n",
                         status);
            }
            EG_free(tetgenInstance->surfaceMesh);
        }
    }

    tetgenInstance->numSurfaceMesh = 0;
    tetgenInstance->surfaceMeshInherited = (int) false;
    tetgenInstance->surfaceMesh = NULL;

    // Destroy volumeMesh allocated arrays
    if (tetgenInstance->volumeMesh != NULL) {
        for (i = 0; i < tetgenInstance->numVolumeMesh; i++) {
            status = destroy_meshStruct(&tetgenInstance->volumeMesh[i]);
            if (status != CAPS_SUCCESS)
              printf("Error during destroy_meshStruct, status = %d\n", status);
        }

        EG_free(tetgenInstance->volumeMesh);
    }
    tetgenInstance->numVolumeMesh = 0;
    tetgenInstance->volumeMesh = NULL;

    // Destroy attribute to index map
    status = destroy_mapAttrToIndexStruct(&tetgenInstance->attrMap);
    if (status != CAPS_SUCCESS)
      printf("Status = %d, tetgenAIM  attrMap cleanup!!!\n", status);

    return CAPS_SUCCESS;
}



/* ********************** Exposed AIM Functions ***************************** */

extern "C" int
aimInitialize(int inst, /*@unused@*/ const char *unitSys, void *aimInfo,
              /*@unused@*/ void **instStore, /*@unused@*/ int *major,
              /*@unused@*/ int *minor, int *nIn, int *nOut,
              int *nFields, char ***fnames, int **franks, int **fInOut)
{
    int status = CAPS_SUCCESS; // Function status return

    aimStorage *tetgenInstance = NULL;

#ifdef DEBUG
    printf("\n tetgenAIM/aimInitialize   instance = %d!\n", inst);
#endif

    /* specify the number of analysis input and out "parameters" */
    *nIn     = NUMINPUT;
    *nOut    = NUMOUT;
    if (inst == -1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate and consume */
    *nFields = 0;
    *fnames  = NULL;
    *franks  = NULL;
    *fInOut  = NULL;

    // Allocate tetgenInstance
    AIM_ALLOC(tetgenInstance, 1, aimStorage, aimInfo, status);
    *instStore = tetgenInstance;

    // Set initial values for tetgenInstance //

    // Container for volume mesh
    tetgenInstance->numVolumeMesh = 0;
    tetgenInstance->volumeMesh = NULL;

    // Container for surface meshes
    tetgenInstance->surfaceMeshInherited = (int) false;

    tetgenInstance->numSurfaceMesh = 0;
    tetgenInstance->surfaceMesh = NULL;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&tetgenInstance->attrMap);
    AIM_STATUS(aimInfo, status);

    // Container for mesh input
    status = initiate_meshInputStruct(&tetgenInstance->meshInput);
    AIM_STATUS(aimInfo, status);

cleanup:
    if (status != CAPS_SUCCESS) AIM_FREE(*instStore);
    return CAPS_SUCCESS;
}


extern "C" int
aimInputs(/*@unused@*/ void *instStore, /*@unused@*/ void *aimInfo, int index,
          char **ainame, capsValue *defval)
{
    /*! \page aimInputsTetGen AIM Inputs
     * The following list outlines the TetGen meshing options along with their default value available
     * through the AIM interface.
     */
    int status = CAPS_SUCCESS;

#ifdef DEBUG
    printf(" tetgenAIM/aimInputs  index = %d!\n", index);
#endif

    // Meshing Inputs
    if (index == Proj_Name) {
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

    } else if (index == Tess_Params) {
        *ainame               = EG_strdup("Tess_Params");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->lfixed        = Fixed;
        defval->vals.reals    = (double *) EG_alloc(defval->nrow*sizeof(double));
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
    } else if (index == Preserve_Surf_Mesh) {
        *ainame               = EG_strdup("Preserve_Surf_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsTetGen
         * - <B> Preserve_Surf_Mesh = True</B> <br>
         * Tells TetGen to preserve the surface mesh provided (i.e. do not add Steiner points on the surface). Discrete data transfer
         * will <b>NOT</b> be possible if Steiner points are added.
         */
    } else if (index == Mesh_Verbose_Flag) {
        *ainame               = EG_strdup("Mesh_Verbose_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = false;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Verbose_Flag = False</B> <br>
         * Verbose output from TetGen.
         */
    } else if (index == Mesh_Quiet_Flag) {
        *ainame               = EG_strdup("Mesh_Quiet_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = false;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Quiet_Flag = False</B> <br>
         * Complete suppression of all TetGen output messages (not including errors).
         */
    } else if (index == Quality_Rad_Edge) {
        *ainame               = EG_strdup("Quality_Rad_Edge"); // TetGen specific parameters
        defval->type          = Double;
        defval->vals.real     = 1.5;

        /*! \page aimInputsTetGen
         * - <B> Quality_Rad_Edge = 1.5</B> <br>
         * TetGen maximum radius-edge ratio.
         */
    } else if (index == Quality_Angle) {
        *ainame               = EG_strdup("Quality_Angle"); // TetGen specific parameters
        defval->type          = Double;
        defval->vals.real     = 0.0;

        /*! \page aimInputsTetGen
         * - <B> Quality_Angle = 0.0</B> <br>
         * TetGen minimum dihedral angle (in degrees).
         */
    } else if (index == Mesh_Format) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->vals.string   = EG_strdup("AFLR3"); // TECPLOT, VTK, AFLR3, SU2, NASTRAN
        defval->lfixed        = Change;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Format = "AFLR3"</B> <br>
         * Mesh output format. Available format names  include: "AFLR3", "TECPLOT", "SU2", "VTK", and "NASTRAN".
         */
    } else if (index == Mesh_ASCII_Flag) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsTetGen
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file if applicable.
         */
    } else if (index == Mesh_Gen_Input_String) {
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
    } else if (index == Ignore_Surface_Mesh_Extraction) { // Deprecated
        *ainame               = EG_strdup("Ignore_Surface_Mesh_Extraction");
        defval->type          = Boolean;
        defval->vals.integer  = true;

//        /*! \page aimInputsTetGen
//         * - <B> Ignore_Surface_Mesh_Extraction = True</B> <br>
//         * If TetGen doesn't preserve the surface mesh provided (i.e. Steiner points are added) a simple search algorithm may be
//         * used to reconstruct a separate (not dependent on the volume mesh node numbering) representation of the surface mesh. In
//         * general, this has little use and can add a significant computational penalty. The default value of "True" is recommended.
//         */
    } else if (index == Mesh_Tolerance) {
        *ainame               = EG_strdup("Mesh_Tolerance"); // TetGen specific parameters
        defval->type          = Double;
        defval->vals.real     = 1E-16;

        /*! \page aimInputsTetGen
         * - <B> Mesh_Tolerance = 1E-16</B> <br>
         * Sets the tolerance for coplanar test in TetGen.
         */
    } else if (index == Multiple_Mesh) {
        *ainame               = EG_strdup("Multiple_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = (int) false;

        /*! \page aimInputsTetGen
         * - <B> Multiple_Mesh = False</B> <br>
         * If set to True a volume will be generated for each body. When set to False (default value) only a single volume
         * mesh will be created.
         */
//    } else if (index == 14) {
//        *ainame               = EG_strdup("Edge_Point_Min");
//        defval->type          = Integer;
//        defval->vals.integer  = 0;
//        defval->lfixed        = Fixed;
//        defval->nrow          = 1;
//        defval->ncol          = 1;
//        defval->nullVal       = IsNull;
//
//        /*! \page aimInputsTetGen
//         * - <B> Edge_Point_Min = NULL</B> <br>
//         * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
//         */
//
//    } else if (index == 15) {
//        *ainame               = EG_strdup("Edge_Point_Max");
//        defval->type          = Integer;
//        defval->vals.integer  = 0;
//        defval->lfixed        = Fixed;
//        defval->nrow          = 1;
//        defval->ncol          = 1;
//        defval->nullVal       = IsNull;
//
//        /*! \page aimInputsTetGen
//         * - <B> Edge_Point_Max = NULL</B> <br>
//         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
//         */
//
//    } else if (index == 16) {
//        *ainame              = EG_strdup("Mesh_Sizing");
//        defval->type         = Tuple;
//        defval->nullVal      = IsNull;
//        //defval->units        = NULL;
//        defval->dim          = Vector;
//        defval->lfixed       = Change;
//        defval->vals.tuple   = NULL;
//
//        /*! \page aimInputsTetGen
//         * - <B>Mesh_Sizing = NULL </B> <br>
//         * See \ref meshSizingProp for additional details.
//         */
    } else if (index == Regions) {
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
         *  tetgen.input.Regions = {
         *    'A': { 'id': 10, 'seed': [0, 0,  1] },
         *    'B': { 'id': 20, 'seed': [0, 0, -1] }
         *  }
         *  \endcode
         *
         * Automatic hole detection will be disabled if one or both of the
         * `Regions` and `Holes` inputs is not NULL.
         */
    } else if (index == Holes) {
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
       *  tetgen.input.Holes = {
       *    'A': { 'seed': [ 1, 0, 0] },
       *    'B': { 'seed': [-1, 0, 0] }
       *  }
       *  \endcode
       *
       * Automatic hole detection will be disabled if one or both of the
       * `Regions` and `Holes` inputs is not NULL.
       */

    } else if (index == Surface_Mesh) {
        *ainame             = AIM_NAME(Surface_Mesh);
        defval->type        = Pointer;
        defval->dim         = Vector;
        defval->lfixed      = Change;
        defval->sfixed      = Change;
        defval->vals.AIMptr = NULL;
        defval->nullVal     = IsNull;
        AIM_STRDUP(defval->units, "meshStruct", aimInfo, status);

        /*! \page aimInputsTetGen
         * - <B>Surface_Mesh = NULL</B> <br>
         * A Surface_Mesh link.
         */
    }

    AIM_NOTNULL(*ainame, aimInfo, status);

cleanup:
    if (status != CAPS_SUCCESS) AIM_FREE(*ainame);
    return status;
}


extern "C" int
aimPreAnalysis(void *instStore, void *aimInfo, capsValue *aimInputs)
{
    int i, elem, body; // Indexing

    int status; // Return status
  
    aimStorage *tetgenInstance;

    // Incoming bodies
    const char *intents;
    ego *bodies = NULL;
    int numBody = 0;

    // Meshing related variables
    //double  tessParams[3]; //, refLen = -1.0;
    meshElementStruct *element;

    // Global settings
    //int minEdgePoint = -1, maxEdgePoint = -1;

    // Mesh attribute parameters
    //int numMeshProp = 0;
    //meshSizingStruct *meshProp = NULL;

    double  box[6], boxMax[6] = {0,0,0,0,0,0};

    int     bodyBoundingBox = 0;

    // File ouput
    char *filename;
    char bodyNumber[11];


    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

#ifdef DEBUG
    printf(" tetgenAIM/aimPreAnalysis  numBody = %d!\n", numBody);
#endif

    if (numBody <= 0 || bodies == NULL) {
#ifdef DEBUG
        printf(" tetgenAIM/aimPreAnalysis No Bodies!\n");
#endif
        return CAPS_SOURCEERR;
    }
    tetgenInstance = (aimStorage *) instStore;

    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the same instance
    status = destroy_aimStorage(tetgenInstance);
    if (status != CAPS_SUCCESS) {
        printf("Status = %d, tetgenAIM  aimStorage cleanup!!!\n", status);
        return status;
    }

    // Get capsGroup name and index mapping
    status = create_CAPSGroupAttrToIndexMap(numBody,
                                            bodies,
                                            2, // Only search down to the edge level of the EGADS body
                                            &tetgenInstance->attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Get surface mesh
    if (aimInputs[Surface_Mesh-1].nullVal == IsNull) {
        AIM_ANALYSISIN_ERROR(aimInfo, Surface_Mesh, "'Surface_Mesh' input must be linked to an output 'Surface_Mesh'");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // Get mesh
    tetgenInstance->numSurfaceMesh = aimInputs[Surface_Mesh-1].length;
    tetgenInstance->surfaceMesh    = (meshStruct *)aimInputs[Surface_Mesh-1].vals.AIMptr;
    tetgenInstance->surfaceMeshInherited = (int) true;

    if (tetgenInstance->numSurfaceMesh != numBody) {
        AIM_ANALYSISIN_ERROR(aimInfo, Surface_Mesh, "Number of linked surface meshes (%d) does not match the number of bodies (%d)\n",
                             tetgenInstance->numSurfaceMesh, numBody);
        return CAPS_SOURCEERR;
    }

    // Create/setup volume meshes
    if (aimInputs[Multiple_Mesh-1].vals.integer == (int) true) {

        tetgenInstance->numVolumeMesh = numBody;
        tetgenInstance->volumeMesh = (meshStruct *)
                     EG_alloc(tetgenInstance->numVolumeMesh*sizeof(meshStruct));
        if (tetgenInstance->volumeMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (body = 0; body < tetgenInstance->numVolumeMesh; body++) {
            status = initiate_meshStruct(&tetgenInstance->volumeMesh[body]);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Set reference mesh - One surface per body
            tetgenInstance->volumeMesh[body].numReferenceMesh = 1;
            tetgenInstance->volumeMesh[body].referenceMesh = (meshStruct *)
                  EG_alloc(tetgenInstance->volumeMesh[body].numReferenceMesh*
                           sizeof(meshStruct));
            if (tetgenInstance->volumeMesh[body].referenceMesh == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            tetgenInstance->volumeMesh[body].referenceMesh[0] =
                tetgenInstance->surfaceMesh[body];
            printf("Tetgen MultiMesh TopoIndex = %d\n",
                   tetgenInstance->volumeMesh[0].referenceMesh[0].element[0].topoIndex);

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
                for (elem = 0; elem < tetgenInstance->surfaceMesh[body].numElement; elem++) {

                    element = tetgenInstance->surfaceMesh[body].element + elem;

                    // This should be valid for both Triangles and Quadrilaterals
                    i = element->connectivity[2];
                    element->connectivity[2] = element->connectivity[0];
                    element->connectivity[0] = i;
                }
            }
        }

        tetgenInstance->numVolumeMesh = 1;
        tetgenInstance->volumeMesh = (meshStruct *)
                     EG_alloc(tetgenInstance->numVolumeMesh*sizeof(meshStruct));
        if (tetgenInstance->volumeMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        status = initiate_meshStruct(&tetgenInstance->volumeMesh[0]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Combine mesh - temporary store the combined mesh in the volume mesh
        status = mesh_combineMeshStruct(tetgenInstance->numSurfaceMesh,
                                        tetgenInstance->surfaceMesh,
                                        &tetgenInstance->volumeMesh[0]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Set reference meshes - All surfaces
        tetgenInstance->volumeMesh[0].numReferenceMesh =
            tetgenInstance->numSurfaceMesh;
        tetgenInstance->volumeMesh[0].referenceMesh = (meshStruct *)
            EG_alloc(tetgenInstance->volumeMesh[0].numReferenceMesh*
                     sizeof(meshStruct));
        if (tetgenInstance->volumeMesh[0].referenceMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (body = 0; body < tetgenInstance->numSurfaceMesh; body++) {
            tetgenInstance->volumeMesh[0].referenceMesh[body] =
                tetgenInstance->surfaceMesh[body];
        }

         // Report surface mesh
        printf("Number of surface nodes - %d\n",
               tetgenInstance->volumeMesh[0].numNode);
        printf("Number of surface elements - %d\n",
               tetgenInstance->volumeMesh[0].numElement);
    }

    // Setup meshing input structure - mesher specific input gets set below before entering interface
    tetgenInstance->meshInput.preserveSurfMesh = aimInputs[Preserve_Surf_Mesh-1].vals.integer;
    tetgenInstance->meshInput.quiet            = aimInputs[Mesh_Quiet_Flag-1].vals.integer;
    tetgenInstance->meshInput.outputASCIIFlag  = aimInputs[Mesh_ASCII_Flag-1].vals.integer;

    tetgenInstance->meshInput.outputFormat = EG_strdup(aimInputs[Mesh_Format-1].vals.string);
    if (tetgenInstance->meshInput.outputFormat == NULL) return EGADS_MALLOC;

    if (aimInputs[Proj_Name-1].nullVal != IsNull) {

        tetgenInstance->meshInput.outputFileName = EG_strdup(aimInputs[Proj_Name-1].vals.string);
        if (tetgenInstance->meshInput.outputFileName == NULL) return EGADS_MALLOC;
    }

    // Set tetgen specific mesh inputs
    tetgenInstance->meshInput.tetgenInput.meshQuality_rad_edge =
       aimInputs[Quality_Rad_Edge-1].vals.real;
    tetgenInstance->meshInput.tetgenInput.meshQuality_angle    =
       aimInputs[Quality_Angle-1].vals.real;
    tetgenInstance->meshInput.tetgenInput.verbose              =
       aimInputs[Mesh_Verbose_Flag-1].vals.integer;
    tetgenInstance->meshInput.tetgenInput.ignoreSurfaceExtract =
       aimInputs[Ignore_Surface_Mesh_Extraction-1].vals.integer;
    tetgenInstance->meshInput.tetgenInput.meshTolerance        =
       aimInputs[Mesh_Tolerance-1].vals.real;

    if (aimInputs[Regions-1].nullVal != IsNull) {
      populate_regions(&tetgenInstance->meshInput.tetgenInput.regions,
                       aimInputs[Regions-1].length,
                       aimInputs[Regions-1].vals.tuple);
    }
    if (aimInputs[Holes-1].nullVal != IsNull) {
      populate_holes(&tetgenInstance->meshInput.tetgenInput.holes,
                     aimInputs[Holes-1].length,
                     aimInputs[Holes-1].vals.tuple);
    }

    if (aimInputs[Mesh_Gen_Input_String-1].nullVal != IsNull) {

        tetgenInstance->meshInput.tetgenInput.meshInputString =
            EG_strdup(aimInputs[Mesh_Gen_Input_String-1].vals.string);
        if (tetgenInstance->meshInput.tetgenInput.meshInputString == NULL)
            return EGADS_MALLOC;
    }

    status = populate_bndCondStruct_from_mapAttrToIndexStruct(&tetgenInstance->attrMap,
                                                              &tetgenInstance->meshInput.bndConds);
    if (status != CAPS_SUCCESS) return status;

    // Call tetgen volume mesh interface
    for (body = 0; body < tetgenInstance->numVolumeMesh; body++) {

        // Call tetgen volume mesh interface for each body
        if (tetgenInstance->numVolumeMesh > 1) {
            printf("Getting volume mesh for body %d (of %d)\n", body+1, numBody);

            status = tetgen_VolumeMesh(aimInfo,
                                       tetgenInstance->meshInput,
                                      &tetgenInstance->volumeMesh[body].referenceMesh[0],
                                      &tetgenInstance->volumeMesh[body]);
        } else {
            printf("Getting volume mesh\n");

            status = tetgen_VolumeMesh(aimInfo,
                                       tetgenInstance->meshInput,
                                      &tetgenInstance->volumeMesh[body],
                                      &tetgenInstance->volumeMesh[body]);
        }

        if (status != CAPS_SUCCESS) {

            if (numBody > 1) printf("TetGen volume mesh failed on body - %d!!!!\n",
                                    body+1);
            else printf("TetGen volume mesh failed!!!!\n");
            return status;
        }
    }

    // If filename is not NULL write the mesh
    if (tetgenInstance->meshInput.outputFileName != NULL) {

        for (body = 0; body < tetgenInstance->numVolumeMesh; body++) {

            if (aimInputs[Multiple_Mesh-1].vals.integer == (int) true) {
                sprintf(bodyNumber, "%d", body);
                filename = (char *) EG_alloc((strlen(tetgenInstance->meshInput.outputFileName) +
                                              2+strlen("_Vol")+strlen(bodyNumber))*sizeof(char));
            } else {
                filename = (char *) EG_alloc((strlen(tetgenInstance->meshInput.outputFileName) +2)*sizeof(char));
            }

            if (filename == NULL) return EGADS_MALLOC;

            strcpy(filename, tetgenInstance->meshInput.outputFileName);

            if (aimInputs[Multiple_Mesh-1].vals.integer == (int) true) {
                strcat(filename,"_Vol");
                strcat(filename,bodyNumber);
            }

            if (strcasecmp(tetgenInstance->meshInput.outputFormat, "AFLR3") == 0) {

                status = mesh_writeAFLR3(aimInfo,
                                         filename,
                                         tetgenInstance->meshInput.outputASCIIFlag,
                                         &tetgenInstance->volumeMesh[body],
                                         1.0);

            } else if (strcasecmp(tetgenInstance->meshInput.outputFormat, "VTK") == 0) {

                status = mesh_writeVTK(aimInfo,
                                       filename,
                                       tetgenInstance->meshInput.outputASCIIFlag,
                                       &tetgenInstance->volumeMesh[body],
                                       1.0);

            } else if (strcasecmp(tetgenInstance->meshInput.outputFormat, "SU2") == 0) {

                status = mesh_writeSU2(aimInfo,
                                       filename,
                                       tetgenInstance->meshInput.outputASCIIFlag,
                                       &tetgenInstance->volumeMesh[body],
                                       tetgenInstance->meshInput.bndConds.numBND,
                                       tetgenInstance->meshInput.bndConds.bndID,
                                       1.0);

            } else if (strcasecmp(tetgenInstance->meshInput.outputFormat, "Tecplot") == 0) {

                status = mesh_writeTecplot(aimInfo,
                                           filename,
                                           tetgenInstance->meshInput.outputASCIIFlag,
                                           &tetgenInstance->volumeMesh[body],
                                           1.0);

            } else if (strcasecmp(tetgenInstance->meshInput.outputFormat, "Nastran") == 0) {

                status = mesh_writeNASTRAN(aimInfo,
                                           filename,
                                           tetgenInstance->meshInput.outputASCIIFlag,
                                           &tetgenInstance->volumeMesh[body],
                                           LargeField,
                                           1.0);

            } else {
                printf("Unrecognized mesh format, \"%s\", the volume mesh will not be written out\n",
                       tetgenInstance->meshInput.outputFormat);
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
        if (status != CAPS_SUCCESS)
          printf("Error: tetgenAIM status %d\n", status);

//        if (meshProp != NULL) {
//
//            for (i = 0; i < numMeshProp; i++) {
//
//                (void) destroy_meshSizingStruct(&meshProp[i]);
//            }
//
//            EG_free(meshProp);
//        }

        return status;
}


/* the execution code from above should be moved here */
extern "C" int
aimExecute(/*@unused@*/ void *instStore, /*@unused@*/ void *aimStruc, int *state)
{
  *state = 0;
  return CAPS_SUCCESS;
}


/* no longer optional and needed for restart */
extern "C" int
aimPostAnalysis(/*@unused@*/ void *instStore, /*@unused@*/ void *aimStruc,
                /*@unused@*/ int restart, /*@unused@*/ capsValue *inputs)
{
  return CAPS_SUCCESS;
}


extern "C" int
aimOutputs(/*@unused@*/ void *instStore, /*@unused@*/ void *aimInfo,  int index,
           char **aoname, capsValue *form)
{
    /*! \page aimOutputsTetGen AIM Outputs
     * The following list outlines the TetGen AIM outputs available through the AIM interface.
     *
     * - <B>Done</B> = True if a volume mesh(es) was created, False if not.
     */

    int status = CAPS_SUCCESS;

#ifdef DEBUG
    printf(" tetgenAIM/aimOutputs  index = %d!\n", index);
#endif

    if (index == Done) {

        *aoname = EG_strdup("Done");
        form->type = Boolean;
        form->vals.integer = (int) false;

    } else if (index == Volume_Mesh) {

        *aoname           = AIM_NAME(Volume_Mesh);
        form->type        = Pointer;
        form->dim         = Vector;
        form->lfixed      = Change;
        form->sfixed      = Change;
        form->vals.AIMptr = NULL;
        form->nullVal     = IsNull;
        AIM_STRDUP(form->units, "meshStruct", aimInfo, status);

        /*! \page aimOutputsTetGen
         * - <B> Volume_Mesh </B> <br>
         * The volume mesh for a link
         */

    } else {
        status = CAPS_BADINDEX;
        AIM_STATUS(aimInfo, status, "Unknown output index %d!", index);
    }

    AIM_NOTNULL(*aoname, aimInfo, status);

cleanup:
    if (status != CAPS_SUCCESS) AIM_FREE(*aoname);
    return status;
}


extern "C" int
aimCalcOutput(void *instStore, void *aimInfo, int index, capsValue *val)
{
    int i, status = CAPS_SUCCESS;
    aimStorage *tetgenInstance;

#ifdef DEBUG
    printf(" tetgenAIM/aimCalcOutput  index = %d!\n", index);
#endif
    tetgenInstance = (aimStorage *) instStore;

    if (Done == index) {

        val->vals.integer = (int) false;

        // Check to see if a volume mesh was generated - maybe a file was written, maybe not
        for (i = 0; i < tetgenInstance->numVolumeMesh; i++) {
            // Check to see if a volume mesh was generated
            if (tetgenInstance->volumeMesh[i].numElement != 0 &&
                tetgenInstance->volumeMesh[i].meshType == VolumeMesh) {

                val->vals.integer = (int) true;

            } else {

                val->vals.integer = (int) false;

                if (tetgenInstance->numVolumeMesh > 1) {
                    printf("No tetrahedral elements were generated for volume mesh %d\n", i+1);
                } else {
                    printf("No tetrahedral elements were generate\n");
                }

                return CAPS_SUCCESS;
            }
        }

    } else if (Volume_Mesh == index) {

        // Return the volume meshes
        val->nrow        = tetgenInstance->numVolumeMesh;
        val->vals.AIMptr = tetgenInstance->volumeMesh;

    } else {

        status = CAPS_BADINDEX;
        AIM_STATUS(aimInfo, status, "Unknown output index %d!", index);

    }

cleanup:

    return status;
}

extern "C" void
aimCleanup(void *instStore)
{
    int status; // Function return status
    aimStorage *tetgenInstance;

#ifdef DEBUG
    printf(" tetgenAIM/aimCleanup!\n");
#endif
    tetgenInstance = (aimStorage *) instStore;

    status = destroy_aimStorage(tetgenInstance);
    if (status != CAPS_SUCCESS)
        printf("Status = %d, tetgenAIM  aimStorage cleanup!!!\n", status);

    EG_free(tetgenInstance);
}
