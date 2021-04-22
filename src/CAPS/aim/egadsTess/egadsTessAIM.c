/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             EGADS Tessellation AIM
 *
 */

/*!\mainpage Introduction
 *
 * \section overviewEgadsTess EGADS Tessellation AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact .....
 *
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsEgadsTess and \ref aimOutputsEgadsTess, respectively.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataEgadsTess if connecting this AIM to other AIMs in a
 * parent-child like manner.
 *
 */



#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"       // Collection of helper functions for meshing
#include "cfdUtils.h"        // Collection of helper functions for cfd analysis
#include "feaUtils.h"        // Collection of helper functions for fea analysis
#include "miscUtils.h"       // Collection of helper functions for miscellaneous analysis

#ifdef WIN32
#define getcwd      _getcwd
#define strcasecmp  stricmp
#define strncasecmp _strnicmp
#define PATH_MAX    _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif


#define NUMINPUT 11 // Number of mesh inputs
#define NUMOUT    3 // Number of outputs

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

static aimStorage *egadsInstance = NULL;
static int         numInstance  = 0;

static int destroy_aimStorage(int iIndex) {

    int i; // Indexing

    int status; // Function return status

    // Destroy meshInput
    status = destroy_meshInputStruct(&egadsInstance[iIndex].meshInput);
    if (status != CAPS_SUCCESS)
      printf("Status = %d, egadsTessAIM instance %d, meshInput cleanup!!!\n",
             status, iIndex);

    // Destroy surface mesh allocated arrays
    for (i = 0; i < egadsInstance[iIndex].numSurface; i++) {
        status = destroy_meshStruct(&egadsInstance[iIndex].surfaceMesh[i]);
        if (status != CAPS_SUCCESS)
          printf("Status = %d, egadsTessAIM instance %d, surfaceMesh cleanup!!!\n",
                 status, iIndex);
    }
    egadsInstance[iIndex].numSurface = 0;

    if (egadsInstance[iIndex].surfaceMesh != NULL)
      EG_free(egadsInstance[iIndex].surfaceMesh);
    egadsInstance[iIndex].surfaceMesh = NULL;

    // Destroy attribute to index map
    status = destroy_mapAttrToIndexStruct(&egadsInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS)
      printf("Status = %d, egadsTessAIM instance %d, attributeMap cleanup!!!\n",
             status, iIndex);
    return CAPS_SUCCESS;
}



/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(/*@unused@*/ int ngIn, /*@unused@*/ capsValue *gIn,
                  int *qeFlag, /*@unused@*/ /*@null@*/ const char *unitSys,
                  int *nIn, int *nOut, int *nFields,
                  char ***fnames, int **ranks)
{
    int status; // Function return status
    int flag;   // query/execute flag

    aimStorage *tmp;

#ifdef DEBUG
    printf("\n egadsTessAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif

    flag     = *qeFlag;

    // AIM executes itself
    *qeFlag  = 1;

    // Specify the number of analysis input and out "parameters"
    *nIn     = NUMINPUT;
    *nOut    = NUMOUT;
    if (flag == 1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate */
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Allocate aflrInstance
    if (egadsInstance == NULL) {
        egadsInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (egadsInstance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(egadsInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        egadsInstance = tmp;
    }

    // Set initial values for aflrInstance //

    // Container for surface meshes
    egadsInstance[numInstance].surfaceMesh = NULL;
    egadsInstance[numInstance].numSurface = 0;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&egadsInstance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for mesh input
    status = initiate_meshInputStruct(&egadsInstance[numInstance].meshInput);

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}

int aimInputs(/*@unused@*/ int iIndex, /*@unused@*/ void *aimInfo, int index,
              char **ainame, capsValue *defval)
{

    /*! \page aimInputsEgadsTess AIM Inputs
     * The following list outlines the EGADS Tessellation meshing options along with their default value available
     * through the AIM interface.
     */


#ifdef DEBUG
    printf(" egadsTessAIM/aimInputs instance = %d  index = %d!\n", inst, index);
#endif

    // Meshing Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name"); // If NULL a volume grid won't be written by the AIM
        defval->type         = String;
        defval->nullVal      = IsNull;
        defval->vals.string  = NULL;
        //defval->vals.string  = EG_strdup("CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsEgadsTess
         * - <B> Proj_Name = NULL</B> <br>
         * This corresponds to the output name of the mesh. If left NULL, the mesh is not written to a file.
         */

    } else if (index == 2) {
        *ainame              = EG_strdup("Mesh_Length_Factor");
        defval->type         = Double;
        defval->dim          = Scalar;
        defval->vals.real    = 1;
        defval->nullVal      = NotNull;

        /*! \page aimInputsEgadsTess
         * - <B> Mesh_Length_Factor = 1</B> <br>
         * Scaling factor to compute a meshing Reference_Length via:<br>
         * <br>
         * Reference_Length = capsMeshLength*Mesh_Length_Factor<br>
         * <br>
         * Reference_Length scales Tess_Params[0] and Tess_Params[1] in both aimInputs and Mesh_Sizing
         */

    } else if (index == 3) {
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
            defval->vals.reals[0] = 0.10;
            defval->vals.reals[1] = 0.01;
            defval->vals.reals[2] = 15.0;
        } else return EGADS_MALLOC;

        /*! \page aimInputsEgadsTess
         * - <B> Tess_Params = [0.1, 0.01, 15.0]</B> <br>
         * Body tessellation parameters. Tess_Params[0] and Tess_Params[1] get scaled by Reference_Length if
         * it is set, otherwise by the bounding box of the largest body. (From the EGADS manual)
         * A set of 3 parameters that drive the EDGE discretization
         * and the FACE triangulation. The first is the maximum length of an EDGE segment or triangle side
         * (in physical space). A zero is flag that allows for any length. The second is a curvature-based
         * value that looks locally at the deviation between the centroid of the discrete object and the
         * underlying geometry. Any deviation larger than the input value will cause the tessellation to
         * be enhanced in those regions. The third is the maximum interior dihedral angle (in degrees)
         * between triangle facets (or Edge segment tangents for a WIREBODY tessellation), note that a
         * zero ignores this phase
         */
    } else if (index == 4) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->vals.string   = EG_strdup("AFLR3"); // TECPLOT, VTK, AFLR3, STL, AF, FAST, NASTRAN
        defval->lfixed        = Change;

        /*! \page aimInputsEgadsTess
         * - <B> Mesh_Format = "AFLR3"</B> <br>
         * Mesh output format. Available format names include: "AFLR3", "VTK", "TECPLOT", "STL" (quadrilaterals will be
         * split into triangles), "Airfoil", "FAST", "Nastran".
         *
         *   - "Airfoil" corresponds to the following file format in which the nodal coordinates of the body's edges
         *  are written. Bodies should be face bodies, planar, and have no holes. A *.af suffix is used for the file:
         * <br>"Character Name"
         * <br>x[0] y[0] x y coordinates
         * <br>x[1] y[1]
         * <br>...  ...
         */
    } else if (index == 5) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsEgadsTess
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file if applicable.
         */

    } else if (index == 6) {
        *ainame               = EG_strdup("Edge_Point_Min");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsEgadsTess
         * - <B> Edge_Point_Min = NULL</B> <br>
         * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 7) {
        *ainame               = EG_strdup("Edge_Point_Max");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->length        = 1;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsEgadsTess
         * - <B> Edge_Point_Max = NULL</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 8) {
        *ainame              = EG_strdup("Mesh_Sizing");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsEgadsTess
         * - <B>Mesh_Sizing = NULL </B> <br>
         * See \ref meshSizingProp for additional details.
         */

    } else if (index == 9) {
        *ainame              = EG_strdup("Mesh_Elements");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Tri");
        defval->lfixed       = Change;

        /*! \page aimInputsEgadsTess
         * - <B>Mesh_Elements = "Tri" </B> <br>
         * Element topology in the resulting mesh:
         *  - "Tri"   - All triangle elements
         *  - "Quad"  - All quadrilateral elements
         *  - "Mixed" - Quad elements for four-sided faces with TFI, triangle elements otherwise (deprecated)
         */

    } else if (index == 10) {
        *ainame               = EG_strdup("Multiple_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = (int) true;

        /*! \page aimInputsEgadsTess
         * - <B> Multiple_Mesh = True</B> <br>
         * If set to True (default) a surface mesh will be generated and output (given "Proj_Name" is set) for each body.
         * When set to False only a single surface
         * mesh will be created. Note, this only affects the mesh when writing to a
         * file (no effect on sharable data \ref sharableDataEgadsTess) .
         */

    } else if (index == 11) {
        *ainame               = EG_strdup("TFI_Templates");
        defval->type          = Boolean;
        defval->vals.integer  = (int) true;

        /*! \page aimInputsEgadsTess
         * - <B> TFI_Templates = True</B> <br>
         * Use Transfinite Interpolation and Templates to generate
         * structured triangulations on FACEs with 3 or 4 "sides" with similar opposing vertex counts.
         */
    }
    /*if (index != 1){
        // Link variable(s) to parent(s) if available
        status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
        //printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n", status, index, defval->type, defval->link);
    }*/

#if NUMINPUT != 11
#error "NUMINPUT is inconsistent with the list of inputs"
#endif

    return CAPS_SUCCESS;
}

int aimData(int iIndex, const char *name, enum capsvType *vtype,
            int *rank, int *nrow, int *ncol, void **data, char **units)
{
    /*! \page sharableDataEgadsTess AIM Shareable Data
     * The egadsTess AIM has the following shareable data types/values with its children AIMs if they are so inclined.
     * - <B> Surface_Mesh</B> <br>
     * The returned surface mesh after egadsTess execution is complete in meshStruct (see meshTypes.h) format.
     * - <B> Attribute_Map</B> <br>
     * An index mapping between capsGroups found on the geometry in mapAttrToIndexStruct (see miscTypes.h) format.
     */

#ifdef DEBUG
    printf(" egadsTessAIM/aimData instance = %d  name = %s!\n", inst, name);
#endif

    // The returned surface mesh from egadsTess

/*@-unrecog@*/
    if (strcasecmp(name, "Surface_Mesh") == 0){
        *vtype = Value;
        *rank = *nrow = 1;
        *ncol  = egadsInstance[iIndex].numSurface;
        *data  = egadsInstance[iIndex].surfaceMesh;
        *units = NULL;

        return CAPS_SUCCESS;
    }

    // Share the attribute map
    if (strcasecmp(name, "Attribute_Map") == 0){
        *vtype = Value;
        *rank  = *nrow = *ncol = 1;
        *data  = &egadsInstance[iIndex].attrMap;
        *units = NULL;

        return CAPS_SUCCESS;
    }
/*@+unrecog@*/

    return CAPS_NOTFOUND;

}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath,
                   capsValue *aimInputs, capsErrs **errs)
{

    int status; // Status return

    int i, bodyIndex; // Indexing

    int numNodeTotal = 0, numElemTotal = 0;

    // Body parameters
    const char *intents;
    int numBody = 0; // Number of bodies
    ego *bodies = NULL; // EGADS body objects

    // Global settings
    int minEdgePoint = -1, maxEdgePoint = -1, quadMesh = 0;
    double refLen = 0, meshLenFac = 0, capsMeshLength = 0;

    // Mesh attribute parameters
    int numMeshProp = 0;
    meshSizingStruct *meshProp = NULL;
    const char *Mesh_Elements = NULL;

    // Combined mesh
    meshStruct combineMesh;

    // File output
    char *filename = NULL;
    char bodyNumber[11];

    // NULL out errors
    *errs = NULL;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_meshStruct(&combineMesh);
    if (status != CAPS_SUCCESS) return status;

#ifdef DEBUG
    printf(" egadsTessAIM/aimPreAnalysis instance = %d  numBody = %d!\n",
           iIndex, numBody);
#endif

    if ((numBody <= 0) || (bodies == NULL)) {
#ifdef DEBUG
        printf(" egadsTessAIM/aimPreAnalysis No Bodies!\n");
#endif
        return CAPS_SOURCEERR;
    }
  
    if (aimInputs == NULL) {
#ifdef DEBUG
        printf(" egadsTessAIM/aimPreAnalysis No aimInputs!\n");
#endif
        return CAPS_SOURCEERR;
    }

    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the same instance
    status = destroy_aimStorage(iIndex);
    if (status != CAPS_SUCCESS) {
        if (status != CAPS_SUCCESS)
          printf("Status = %d, egadsTessAIM instance %d, aimStorage cleanup!!!\n",
                 status, iIndex);
        return status;
    }

    // Remove any previous tessellation object
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {
        if (bodies[bodyIndex+numBody] != NULL) {
            EG_deleteObject(bodies[bodyIndex+numBody]);
            bodies[bodyIndex+numBody] = NULL;
        }
    }

    // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
    status = create_CAPSGroupAttrToIndexMap(numBody,
                                            bodies,
                                            2,
                                            &egadsInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Allocate surfaceMesh from number of bodies
    egadsInstance[iIndex].numSurface = numBody;
    egadsInstance[iIndex].surfaceMesh = (meshStruct *) EG_alloc(egadsInstance[iIndex].numSurface*sizeof(meshStruct));
    if (egadsInstance[iIndex].surfaceMesh == NULL) {
        egadsInstance[iIndex].numSurface = 0;
        return EGADS_MALLOC;
    }

    // Initiate surface meshes
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++){
        status = initiate_meshStruct(&egadsInstance[iIndex].surfaceMesh[bodyIndex]);
        if (status != CAPS_SUCCESS) return status;
    }

    // Setup meshing input structure -> -1 because aim_getIndex is 1 bias

    // Project Name
    if (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].nullVal != IsNull) {
        egadsInstance[iIndex].meshInput.outputFileName =
                                    EG_strdup(aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
        if (egadsInstance[iIndex].meshInput.outputFileName == NULL) { status = EGADS_MALLOC; goto cleanup; }
    }

    // Mesh Format
    egadsInstance[iIndex].meshInput.outputFormat =
                                    EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].vals.string);
    if (egadsInstance[iIndex].meshInput.outputFormat == NULL) { status = EGADS_MALLOC; goto cleanup; }

    // Output directory
    egadsInstance[iIndex].meshInput.outputDirectory = EG_strdup(analysisPath);
    if (egadsInstance[iIndex].meshInput.outputDirectory == NULL) { status = EGADS_MALLOC; goto cleanup; }

    // ASCII flag
    egadsInstance[iIndex].meshInput.outputASCIIFlag  = aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag", ANALYSISIN)-1].vals.integer;

    // Reference length for meshing
    meshLenFac = aimInputs[aim_getIndex(aimInfo, "Mesh_Length_Factor", ANALYSISIN)-1].vals.real;

    status = check_CAPSMeshLength(numBody, bodies, &capsMeshLength);

    // TODO: Should capsMeshLength be optional?
    if      (status == CAPS_NOTFOUND) capsMeshLength = -1;
    else if (status != CAPS_SUCCESS) goto cleanup;

    /*
    if (capsMeshLength <= 0 || status != CAPS_SUCCESS) {
      printf("**********************************************************\n");
      if (status != CAPS_SUCCESS)
        printf("capsMeshLength is not set on any body.\n");
      else
        printf("capsMeshLength: %f\n", capsMeshLength);
      printf("\n");
      printf("The capsMeshLength attribute must\n"
             "present on at least one body.\n"
             "\n"
             "capsMeshLength should be a a positive value representative\n"
             "of a characteristic length of the geometry,\n"
             "e.g. the MAC of a wing or diameter of a fuselage.\n");
      printf("**********************************************************\n");
      status = CAPS_BADVALUE;
      goto cleanup;
    }
    */

    if (meshLenFac <= 0) {
      printf("**********************************************************\n");
      printf("Mesh_Length_Factor is: %f\n", meshLenFac);
      printf("Mesh_Length_Factor must be a positive number.\n");
      printf("**********************************************************\n");
      status = CAPS_BADVALUE;
      goto cleanup;
    }

    refLen = meshLenFac*capsMeshLength;

    // Get Tessellation parameters -Tess_Params -> -1 because of aim_getIndex 1 bias
    egadsInstance[iIndex].meshInput.paramTess[0] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[0]; // Gets multiplied by bounding box size
    egadsInstance[iIndex].meshInput.paramTess[1] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[1]; // Gets multiplied by bounding box size
    egadsInstance[iIndex].meshInput.paramTess[2] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[2];

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
                                    &egadsInstance[iIndex].attrMap,
                                    &numMeshProp,
                                    &meshProp);
        if (status != CAPS_SUCCESS) return status;
    }

    // Get mesh element types
    Mesh_Elements = aimInputs[aim_getIndex(aimInfo, "Mesh_Elements", ANALYSISIN)-1].vals.string;

/*@-unrecog@*/
         if ( strncasecmp(Mesh_Elements,"Tri",3)   == 0 ) { quadMesh = 0; }
    else if ( strncasecmp(Mesh_Elements,"Quad",4)  == 0 ) { quadMesh = 1; }
    else if ( strncasecmp(Mesh_Elements,"Mixed",3) == 0 ) { quadMesh = 2; }
    else {
        printf("Error: Unknown Mesh_Elements = \"%s\"\n", Mesh_Elements);
        printf("       Shoule be one of \"Tri\", \"Quad\", or \"Mixed\"\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }
/*@+unrecog@*/

    if (aimInputs[aim_getIndex(aimInfo, "TFI_Templates", ANALYSISIN)-1].vals.integer == (int)false) {
        for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++){
            // Disable TFI and templates
            status = EG_attributeAdd(bodies[bodyIndex], ".qParams", ATTRSTRING,
                                     0, NULL, NULL, "off");
            if (status != EGADS_SUCCESS) goto cleanup;
        }
    }

    // Modify the EGADS body tessellation based on given inputs
/*@-nullpass@*/
    status =  mesh_modifyBodyTess(numMeshProp,
                                  meshProp,
                                  minEdgePoint,
                                  maxEdgePoint,
                                  quadMesh,
                                  &refLen,
                                  egadsInstance[iIndex].meshInput.paramTess,
                                  egadsInstance[iIndex].attrMap,
                                  numBody,
                                  bodies);
/*@+nullpass@*/
    if (status != CAPS_SUCCESS) goto cleanup;

    // Clean up meshProps
    if (meshProp != NULL) {

        for (i = 0; i < numMeshProp; i++) {

            (void) destroy_meshSizingStruct(&meshProp[i]);
        }

        EG_free(meshProp);
        meshProp = NULL;
    }

    // Run egadsTess for each body
    for (bodyIndex = 0 ; bodyIndex < numBody; bodyIndex++) {

        printf("Getting surface mesh for body %d (of %d)\n", bodyIndex+1, numBody);

        status = mesh_surfaceMeshEGADSBody(bodies[bodyIndex],
                                           refLen,
                                           egadsInstance[iIndex].meshInput.paramTess,
                                           quadMesh,
                                           &egadsInstance[iIndex].attrMap,
                                           &egadsInstance[iIndex].surfaceMesh[bodyIndex]);
        if (status != CAPS_SUCCESS) {
            printf("Problem during surface meshing of body %d\n", bodyIndex+1);
            goto cleanup;
        }

        status = aim_setTess(aimInfo, egadsInstance[iIndex].surfaceMesh[bodyIndex].bodyTessMap.egadsTess);
        if (status != CAPS_SUCCESS) {
            printf(" aim_setTess return = %d\n", status);
            goto cleanup;
        }

        printf("Number of nodes = %d\n", egadsInstance[iIndex].surfaceMesh[bodyIndex].numNode);
        printf("Number of elements = %d\n", egadsInstance[iIndex].surfaceMesh[bodyIndex].numElement);

        if (egadsInstance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.useStartIndex == (int) true ||
            egadsInstance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.useListIndex  == (int) true) {

            printf("Number of tris = %d\n", egadsInstance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.numTriangle);
            printf("Number of quad = %d\n", egadsInstance[iIndex].surfaceMesh[bodyIndex].meshQuickRef.numQuadrilateral);
        }

        numNodeTotal += egadsInstance[iIndex].surfaceMesh[bodyIndex].numNode;
        numElemTotal += egadsInstance[iIndex].surfaceMesh[bodyIndex].numElement;
    }
    //if (quiet == (int)false ) {
    printf("----------------------------\n");
    printf("Total number of nodes = %d\n", numNodeTotal);
    printf("Total number of elements = %d\n", numElemTotal);
    //}


    if (egadsInstance[iIndex].meshInput.outputFileName != NULL) {

        // We need to combine the mesh
        if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) false) {

            status = mesh_combineMeshStruct(egadsInstance[iIndex].numSurface,
                                            egadsInstance[iIndex].surfaceMesh,
                                            &combineMesh);

            if (status != CAPS_SUCCESS) goto cleanup;

            filename = (char *) EG_alloc((strlen(egadsInstance[iIndex].meshInput.outputFileName) +
                                          strlen(egadsInstance[iIndex].meshInput.outputDirectory)+2)*sizeof(char));

            if (filename == NULL) goto cleanup;

            strcpy(filename, egadsInstance[iIndex].meshInput.outputDirectory);

            #ifdef WIN32
                strcat(filename, "\\");
            #else
                strcat(filename, "/");
            #endif

            strcat(filename, egadsInstance[iIndex].meshInput.outputFileName);

            if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "AFLR3") == 0) {

                status = mesh_writeAFLR3(filename,
                                         egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                         &combineMesh,
                                         1.0);

            } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "VTK") == 0) {

                status = mesh_writeVTK(filename,
                                       egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                       &combineMesh,
                                       1.0);

            } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "Tecplot") == 0) {

                status = mesh_writeTecplot(filename,
                                           egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                           &combineMesh,
                                           1.0);

            } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "STL") == 0) {

                status = mesh_writeSTL(filename,
                                       egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                       &combineMesh,
                                       1.0);

            } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "Airfoil") == 0) {

                status = mesh_writeAirfoil(filename,
                                           egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                           &combineMesh,
                                           1.0);

            } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "FAST") == 0) {

                status = mesh_writeFAST(filename,
                                        egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                        &combineMesh,
                                        1.0);

            } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "Nastran") == 0) {

                status = mesh_writeNASTRAN(filename,
                                           egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                           &combineMesh,
                                           FreeField,
                                           1.0);
            } else {
                printf("Unrecognized mesh format, \"%s\", the mesh will not be written out\n", egadsInstance[iIndex].meshInput.outputFormat);
            }

            if (filename != NULL) EG_free(filename);
            filename = NULL;

            if (status != CAPS_SUCCESS) goto cleanup;

        } else {

            for (bodyIndex = 0; bodyIndex < egadsInstance[iIndex].numSurface; bodyIndex++) {

                if (egadsInstance[iIndex].numSurface > 1) {
/*@-bufferoverflowhigh@*/
                    sprintf(bodyNumber, "%d", bodyIndex);
/*@+bufferoverflowhigh@*/
                    filename = (char *) EG_alloc((strlen(egadsInstance[iIndex].meshInput.outputFileName)  +
                                                  strlen(egadsInstance[iIndex].meshInput.outputDirectory) +
                                                  2 +
                                                  strlen("_Surf_") +
                                                  strlen(bodyNumber))*sizeof(char));
                } else {
                    filename = (char *) EG_alloc((strlen(egadsInstance[iIndex].meshInput.outputFileName) +
                                                  strlen(egadsInstance[iIndex].meshInput.outputDirectory)+2)*sizeof(char));

                }

                if (filename == NULL) goto cleanup;

                strcpy(filename, egadsInstance[iIndex].meshInput.outputDirectory);

                #ifdef WIN32
                    strcat(filename, "\\");
                #else
                    strcat(filename, "/");
                #endif

                strcat(filename, egadsInstance[iIndex].meshInput.outputFileName);

                if (egadsInstance[iIndex].numSurface > 1) {
                    strcat(filename,"_Surf_");
                    strcat(filename, bodyNumber);
                }

                if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "AFLR3") == 0) {

                    status = mesh_writeAFLR3(filename,
                                             egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                             &egadsInstance[iIndex].surfaceMesh[bodyIndex],
                                             1.0);

                } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "VTK") == 0) {

                    status = mesh_writeVTK(filename,
                                            egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                            &egadsInstance[iIndex].surfaceMesh[bodyIndex],
                                            1.0);

                } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "Tecplot") == 0) {

                    status = mesh_writeTecplot(filename,
                                               egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                               &egadsInstance[iIndex].surfaceMesh[bodyIndex],
                                               1.0);

                } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "STL") == 0) {

                    status = mesh_writeSTL(filename,
                                           egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                           &egadsInstance[iIndex].surfaceMesh[bodyIndex],
                                           1.0);

                } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "Airfoil") == 0) {

                    status = mesh_writeAirfoil(filename,
                                               egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                               &egadsInstance[iIndex].surfaceMesh[bodyIndex],
                                               1.0);

                } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "FAST") == 0) {

                    status = mesh_writeFAST(filename,
                                            egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                            &egadsInstance[iIndex].surfaceMesh[bodyIndex],
                                            1.0);

                } else if (strcasecmp(egadsInstance[iIndex].meshInput.outputFormat, "Nastran") == 0) {

                    status = mesh_writeNASTRAN(filename,
                                               egadsInstance[iIndex].meshInput.outputASCIIFlag,
                                               &egadsInstance[iIndex].surfaceMesh[bodyIndex],
                                               FreeField,
                                               1.0);
                } else {
                    printf("Unrecognized mesh format, \"%s\", the mesh will not be written out\n", egadsInstance[iIndex].meshInput.outputFormat);
                }

                if (filename != NULL) EG_free(filename);
                filename = NULL;

                if (status != CAPS_SUCCESS) goto cleanup;
            }
        }
    }

    status = CAPS_SUCCESS;
    goto cleanup;


    cleanup:

        if (status != CAPS_SUCCESS)
          printf("Error: egadsTessAIM (instance = %d) status %d\n",
                 iIndex, status);

        if (meshProp != NULL) {

            for (i = 0; i < numMeshProp; i++) {

                (void) destroy_meshSizingStruct(&meshProp[i]);
            }
        }

        (void) destroy_meshStruct(&combineMesh);

        if (filename != NULL) EG_free(filename);
        return status;
}


int aimOutputs(/*@unused@*/ int iIndex, /*@unused@*/ void *aimStruc,
                /*@unused@*/ int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsEgadsTess AIM Outputs
     * The following list outlines the EGADS Tessellation AIM outputs available through the AIM interface.
     */

#ifdef DEBUG
    printf(" egadsTessAIM/aimOutputs instance = %d  index = %d!\n", inst, index);
#endif

    int output = 0;

    if (index == ++output) {
        *aoname = EG_strdup("Done");
        form->type = Boolean;
        form->vals.integer = (int) false;

        /*! \page aimOutputsEgadsTess
         * - <B> Done </B> <br>
         * True if a surface mesh was created on all surfaces, False if not.
         */

    } if (index == ++output) {
        *aoname = EG_strdup("NumberOfElement");
        form->type = Integer;
        form->vals.integer = 0;

        /*! \page aimOutputsEgadsTess
         * - <B> NumberOfElement </B> <br>
         * Number of elements in the surface mesh
         */

    } if (index == ++output) {
        *aoname = EG_strdup("NumberOfNode");
        form->type = Integer;
        form->vals.integer = 0;

        /*! \page aimOutputsEgadsTess
         * - <B> NumberOfNode </B> <br>
         * Number of vertices in the surface mesh
         */
    }


    if (output != NUMOUT) {
        printf("DEVELOPER ERROR: NUMOUT %d != %d\n", NUMOUT, output);
        return CAPS_BADINDEX;
    }

    return CAPS_SUCCESS;
}

// See if a surface mesh was generated for each body
int aimCalcOutput(int iIndex, /*@unused@*/ void *aimInfo,
                  /*@unused@*/ const char *apath, /*@unused@*/ int index,
                  capsValue *val, capsErrs **errors)
{
    int status = CAPS_SUCCESS;
    int surf; // Indexing
    int numElement, numNodes, nElem;

#ifdef DEBUG
    printf(" egadsTessAIM/aimCalcOutput instance = %d  index = %d!\n",
           inst, index);
#endif

    *errors = NULL;

    if (aim_getIndex(aimInfo, "Done", ANALYSISOUT) == index) {

        val->vals.integer = (int) false;

        // Check to see if surface meshes was generated
        for (surf = 0; surf < egadsInstance[iIndex].numSurface; surf++ ) {

            if (egadsInstance[iIndex].surfaceMesh[surf].numElement != 0) {

                val->vals.integer = (int) true;

            } else {

                val->vals.integer = (int) false;
                printf("No surface Tris and/or Quads were generated for surface - %d\n", surf);
                return CAPS_SUCCESS;
            }
        }

    } else if (aim_getIndex(aimInfo, "NumberOfElement", ANALYSISOUT) == index) {

        // Count the total number of surface elements
        numElement = 0;
        for (surf = 0; surf < egadsInstance[iIndex].numSurface; surf++ ) {

            status = mesh_retrieveNumMeshElements(egadsInstance[iIndex].surfaceMesh[surf].numElement,
                                                  egadsInstance[iIndex].surfaceMesh[surf].element,
                                                  Triangle,
                                                  &nElem);
            if (status != CAPS_SUCCESS) goto cleanup;
            numElement += nElem;

            status = mesh_retrieveNumMeshElements(egadsInstance[iIndex].surfaceMesh[surf].numElement,
                                                  egadsInstance[iIndex].surfaceMesh[surf].element,
                                                  Quadrilateral,
                                                  &nElem);
            if (status != CAPS_SUCCESS) goto cleanup;
            numElement += nElem;
        }

        val->vals.integer = numElement;

    } else if (aim_getIndex(aimInfo, "NumberOfNode", ANALYSISOUT) == index) {

        // Count the total number of surface vertices
        numNodes = 0;
        for (surf = 0; surf < egadsInstance[iIndex].numSurface; surf++ ) {
            numNodes += egadsInstance[iIndex].surfaceMesh[surf].numNode;
        }

        val->vals.integer = numNodes;
    } else {
        printf("DEVELOPER ERROR: Unknown output index %d\n", index);
        return CAPS_BADINDEX;
    }

cleanup:

    return CAPS_SUCCESS;
}

void aimCleanup()
{
    int iIndex; // Indexing

    int status; // Returning status

#ifdef DEBUG
    printf(" egadsTessAIM/aimClenup!\n");
#endif

    // Clean up egadsInstance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up egadsInstance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS)
          printf("Status = %d, egadsTessAIM instance %d, aimStorage cleanup!!!\n",
                 status, iIndex);
    }

    if (egadsInstance != NULL) EG_free(egadsInstance);
    egadsInstance = NULL;
    numInstance = 0;

}
