/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             AFLR4 AIM
 *
 *      Modified from code written by Dr. Ryan Durscher AFRL/RQVC
 *
 */

/*!\mainpage Introduction
 *
 * \section overviewAFLR4 AFLR4 AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact with the
 * unstructured, surface grid generator AFLR4 \cite Marcum1995 \cite Marcum1998.
 *
 * The AFLR4 AIM provides the CAPS users with the ability to generate "unstructured, 3D surface grids" using an
 * "Advancing-Front/Local-Reconnection (AFLR) procedure." Only triangular elements may be generated, with planned future support of quadrilateral elements.
 *
 * An outline of the AIM's inputs, outputs and attributes are provided in \ref aimInputsAFLR4 and
 * \ref aimOutputsAFLR4 and \ref attributeAFLR4, respectively.
 * The complete AFLR documentation is available at the <a href="http://www.simcenter.msstate.edu/software/downloads/doc/system/index.html">SimCenter</a>.
 *
 * Details of the AIM's sharable data structures are outlined in \ref sharableDataAFLR4 if connecting this AIM to other AIMs in a
 * parent-child like manner.
 *
 * Example surface meshes:
 *  \image html wing2DAFRL4.png "AFLR4 meshing example - 2D Airfoil" width=500px
 *  \image latex wing2DAFRL4.png "AFLR4 meshing example - 2D Airfoil" width=5cm
 *
 *  \image html sphereAFRL4.png "AFLR4 meshing example - Sphere" width=500px
 *  \image latex sphereAFRL4.png "AFLR4 meshing example - Sphere" width=5cm
 *
 *  \image html wingAFRL4.png "AFLR4 meshing example - Wing" width=500px
 *  \image latex wingAFRL4.png "AFLR4 meshing example - Wing" width=5cm
 */


/*! \page attributeAFLR4 AIM Attributes
 * The following list of attributes are available to guide the mesh generation with the AFLR4 AIM.
 *
 * - <b> AFLR_GBC</b> [Optional FACE attribute: Default STD_UG3_GBC] This string <c>FACE</c> attribute informs AFLR4 what BC
 * treatment should be employed for each geometric <c>FACE</c>. The BC
 * defaults to the string STD_UG3_GBC if none is specified.
 *
 * Predefined AFLR Grid BC string values are:
 *
 * |AFLR_GBC String        | Description                                                         |
 * |:----------------------|:--------------------------------------------------------------------|
 * |FARFIELD_UG3_GBC       | farfield surface<br>same as a standard surface except w/AFLR4       |
 * |STD_UG3_GBC            | standard surface                                                    |
 * |-STD_UG3_GBC           | standard BL generating surface                                      |
 * |BL_INT_UG3_GBC         | symmetry or standard surface<br>that intersects BL region           |
 * |TRANSP_SRC_UG3_GBC     | embedded/transparent surface<br>converted to source nodes by AFLR   |
 * |TRANSP_BL_INT_UG3_GBC  | embedded/transparent surface<br>that intersects BL region           |
 * |TRANSP_UG3_GBC         | embedded/transparent surface                                        |
 * |-TRANSP_UG3_GBC        | embedded/transparent BL generating surface                          |
 * |TRANSP_INTRNL_UG3_GBC  | embedded/transparent surface<br>converted to internal faces by AFLR |
 * |FIXED_BL_INT_UG3_GBC   | fixed surface with BL region<br>that intersects BL region           |
 *
 * Within AFLR4 the grid BC determines how automatic spacing is applied. Their are
 * four basic Grid BC types that are each treated differently.<br>
 * <br>
 * 1. Faces/surfaces that are part of the farfield should be given a
 * FARFIELD_UG3_GBC Grid BC. Farfield faces/surfaces are given a uniform spacing
 * independent of other faces/surfaces with different Grid BCs.<br>
 * <br>
 * 2. Faces/surfaces that represent standard solid surfaces should be given either
 * a STD_UG3_GBC or -STD_UG3_GBC (BL generating) Grid BC. Standard surfaces are
 * given a curvature dependent spacing that may be modified by proximity checking.<br>
 * <br>
 * 3. Faces/surfaces that intersect a BL region should be given either a
 * BL_INT_UG3_GBC (standard boundary surface) or TRANSP_BL_INT_UG3_GBC (embedded/
 * transparent surface with volume mesh on both sides) Grid BC. A common example
 * for the BL_INT_UG3_GBC Grid BC is a symmetry plane. Faces/surfaces set as BL
 * intersecting surfaces are excluded from auto spacing calculations within AFLR4
 * and use edge spacing derived from their neighbors.<br>
 * <br>
 * 4. Surfaces set as transparent surfaces will have a volume mesh on both sides.
 * They can have free edges and can have non-manifold connections to standard
 * solid surfaces and/or BL intersecting surfaces. Vertices in the final surface
 * mesh are not duplicated at non-manifold connections. Transparent surfaces use
 * curvature driven surface spacing as used on standard solid surfaces. However,
 * at non-manifold connections with standard solid surfaces they inherit the
 * surface spacing set on the solid surface they are attached to. They are also
 * excluded from proximity checking. Typical examples of transparent surfaces
 * include wake sheets or multi-material interface surfaces.<br>
 * <br>
 * - <b> AFLR4_Cmp_ID</b> [Optional FACE attribute]<br>
 * EGADS attribute AFLR4_Cmp_ID represents the component identifier for a given
 * face/surface. Component IDs are used for proximity checking. Proximity is only
 * checked between different components. A component is one or more CAD surfaces
 * that represent a component of the full configuration that should be treated
 * individually. For example, a wing-body-strut-nacelle configuration could be
 * considered as four components with wing surfaces set to component 1, body
 * surfaces set to component 2, nacelle surfaces set to 3, and store surfaces set
 * to 4. If each component is a topologically closed surface/body then there is
 * no need to set components. If component IDS are not specified then component
 * identifiers are set for each body defined in the EGADS model or topologically
 * closed surfaces/bodies of the overall configuration. Proximity checking is
 * disabled if there is only one component/body defined. Note that proximity
 * checking only applies to standard surfaces. Component identifiers are set by
 * one of three methods, chosen in the following order.<br>
 * <br>
 * 1. If defined by EGADS attribute AFLR4_Cmp_ID then attribute sets component
 *    identifier.<br>
 * 2. Else, if multiple bodies are defined in the EGADS model then bodies index is
 *    used to set component identifier.<br>
 * 3. Else, component identifiers are set an index based on topologically closed
 *    surfaces/bodies of the overall configuration.<br>
 * <br>
 * - <b> AFLR4_Scale_Factor</b> [Optional FACE attribute: Default 1.0]<br>
 * EGADS attribute AFLR4_Scale_Factor represents the AFLR4 surface mesh spacing
 * scale factor for a given face/surface. Curvature dependent spacing can be
 * scaled on the face/surface by the value of the scale factor set with
 * AFLR4_Scale_Factor.<br>
 * <br>
 * - <b> AFLR4_Edge_Refinement_Weight</b> [Optional FACE attribute: Default 0.0, Range 0 to 1]<br>
 * EGADS attribute AFLR4_Edge_Refinement_Weight represents the edge mesh spacing
 * scale factor weight for a given face/surface. Edge mesh spacing can be scaled
 * on a given face/surface based on the discontinuity level between adjacent
 * faces/surfaces on both sides of the edge. The edge mesh spacing scale factor
 * weight set with AFLR4_Edge_Refinement_Weight is used as an interpolation
 * weight between the unmodified spacing and the modified spacing. A value of one
 * applies the maximum modification and a value of zero applies no change in edge
 * spacing. Note that no modification is done to edges that belong to farfield or
 * BL intersecting face/surface.
 *
 */

#ifdef WIN32
#define getcwd      _getcwd
#define strcasecmp  stricmp
#define strncasecmp _strnicmp
#define PATH_MAX    _MAX_PATH
typedef int         pid_t;        
#else
#include <unistd.h>
#include <limits.h>
#endif

#include <string.h>
#include <math.h>

#include "aimUtil.h"

#include "meshUtils.h"       // Collection of helper functions for meshing
#include "miscUtils.h"       // Collection of helper functions for miscellaneous analysis
#include "aflr4_Interface.h" // Bring in AFLR4 'interface' functions

extern int
EG_saveTess(ego tess, const char *name); // super secret experimental EGADS tessellation format

#include <ug/UG_LIB.h>
#include <aflr4/AFLR4_LIB.h> // Bring in AFLR4 API library

#define NUMINPUT  19      // Number of mesh inputs
#define NUMOUT    3       // Number of outputs

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

static aimStorage *aflr4Instance = NULL;
static int         numInstance  = 0;

static int destroy_aimStorage(int iIndex) {

    int i; // Indexing

    int status; // Function return status

    // Destroy meshInput
    status = destroy_meshInputStruct(&aflr4Instance[iIndex].meshInput);
    if (status != CAPS_SUCCESS) printf("Status = %d, aflr4AIM instance %d, meshInput cleanup!!!\n", status, iIndex);

    // Destroy surface mesh allocated arrays
    for (i = 0; i < aflr4Instance[iIndex].numSurface; i++) {

        status = destroy_meshStruct(&aflr4Instance[iIndex].surfaceMesh[i]);
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr4AIM instance %d, surfaceMesh cleanup!!!\n", status, iIndex);

    }
    aflr4Instance[iIndex].numSurface = 0;

    if (aflr4Instance[iIndex].surfaceMesh != NULL) EG_free(aflr4Instance[iIndex].surfaceMesh);
    aflr4Instance[iIndex].surfaceMesh = NULL;

    // Destroy attribute to index map
    status = destroy_mapAttrToIndexStruct(&aflr4Instance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Status = %d, aflr4AIM instance %d, attributeMap cleanup!!!\n", status, iIndex);

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


static int setAFLR4Attr(ego body,
                        mapAttrToIndexStruct *attrMap,
                        int numMeshProp,
                        meshSizingStruct *meshProp) {

    int status; // Function return status

    int faceIndex; // Indexing

    int numFace;
    ego *faces = NULL;

    const char *groupName = NULL;
    int attrIndex = 0;
    int propIndex = 0;
    const char* bcType = NULL;

    status = EG_getBodyTopos(body, NULL, FACE, &numFace, &faces);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Loop through the faces and copy capsGroup to PW:Name
    for (faceIndex = 0; faceIndex < numFace; faceIndex++) {

        status = retrieve_CAPSGroupAttr(faces[faceIndex], &groupName);
        if (status == EGADS_SUCCESS) {

            status = get_mapAttrToIndexIndex(attrMap, groupName, &attrIndex);
            if (status != CAPS_SUCCESS) goto cleanup;

            for (propIndex = 0; propIndex < numMeshProp; propIndex++) {

                // Check if the mesh property applies to the capsGroup of this face
                if (meshProp[propIndex].attrIndex != attrIndex) continue;

                // If scaleFactor specified in meshProp
                if (meshProp[propIndex].bcType != NULL) {

                    bcType = meshProp[propIndex].bcType;
                    if      (strncasecmp(meshProp[propIndex].bcType, "Farfield"  ,  8) == 0) bcType = "FARFIELD_UG3_GBC";
                    else if (strncasecmp(meshProp[propIndex].bcType, "Freestream", 10) == 0) bcType = "FARFIELD_UG3_GBC";
                    else if (strncasecmp(meshProp[propIndex].bcType, "Viscous"   ,  7) == 0) bcType = "-STD_UG3_GBC";
                    else if (strncasecmp(meshProp[propIndex].bcType, "Inviscid"  ,  8) == 0) bcType = "STD_UG3_GBC";
                    else if (strncasecmp(meshProp[propIndex].bcType, "Symmetry"  ,  8) == 0) bcType = "BL_INT_UG3_GBC";

                    // add the BC attribute
                    status = EG_attributeAdd(faces[faceIndex], "AFLR_GBC", ATTRSTRING, 0, NULL, NULL, bcType);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // If scaleFactor specified in meshProp
                if (meshProp[propIndex].scaleFactor > 0) {

                    // add the scale factor attribute
                    status = EG_attributeAdd(faces[faceIndex], "AFLR4_Scale_Factor", ATTRREAL, 1, NULL, &meshProp[propIndex].scaleFactor, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // If edgeWeight specified in meshProp
                if (meshProp[propIndex].edgeWeight >= 0) {

                    // add the edge scale factor weight attribute
                    status = EG_attributeAdd(faces[faceIndex], "AFLR4_Edge_Refinement_Weight", ATTRREAL, 1, NULL, &meshProp[propIndex].edgeWeight, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }
            }
        }
    } // Face loop

    status = CAPS_SUCCESS;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in setAFLR4Attr, status %d\n", status);

        EG_free(faces);

        return status;
}



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
        printf("\n aflr4AIM/aimInitialize   ngIn = %d!\n", ngIn);
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
    if (aflr4Instance == NULL) {
        aflr4Instance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (aflr4Instance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(aflr4Instance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        aflr4Instance = tmp;
    }

    // Set initial values for aflrInstance //

    // Container for surface meshes
    aflr4Instance[numInstance].surfaceMesh = NULL;
    aflr4Instance[numInstance].numSurface = 0;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&aflr4Instance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for mesh input
    status = initiate_meshInputStruct(&aflr4Instance[numInstance].meshInput);

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}

int aimInputs(/*@unused@*/ int iIndex, void *aimInfo, int index, char **ainame,
              capsValue *defval)
{

    int input = 0;
    /*! \page aimInputsAFLR4 AIM Inputs
     * The following list outlines the AFLR4 meshing options along with their default value available
     * through the AIM interface.
     *
     * Please consult the <a href="http://www.simcenter.msstate.edu/software/downloads/doc/aflr4/index.html">AFLR4 documentation</a> for default values not present here.
     */

    int status; // error code
    UG_Param_Struct *AFLR4_Param_Struct_Ptr = NULL; // AFRL4 input structure used to get default values

    int min_ncell, mer_all;
    CHAR_UG_MAX no_prox;

    double ff_cdfr, abs_min_scale, BL_thickness, Re_l, curv_factor,
           max_scale, min_scale, erw_all; //, ref_len;


    status = ug_malloc_param (&AFLR4_Param_Struct_Ptr);
    if ((status != CAPS_SUCCESS) || (AFLR4_Param_Struct_Ptr == NULL)) {
        printf("ug_malloc_param status = %d\n", status);
        goto cleanup;
    }

    status = ug_initialize_param (AFLR4_Param_Struct_Ptr);
    if (status != CAPS_SUCCESS) {
        printf("ug_initialize_param status = %d\n", status);
        goto cleanup;
    }

    status = aflr4_initialize_param (AFLR4_Param_Struct_Ptr);
    if (status != CAPS_SUCCESS) {
        printf("aflr4_initialize_param status = %d\n", status);
        goto cleanup;
    }

    #ifdef DEBUG
        printf(" aflr4AIM/aimInputs instance = %d  index = %d!\n", inst, index);
    #endif

    // Meshing Inputs
    if (index == ++input) {
        *ainame              = EG_strdup("Proj_Name"); // If NULL a volume grid won't be written by the AIM
        defval->type         = String;
        defval->nullVal      = IsNull;
        defval->vals.string  = NULL;
        //defval->vals.string  = EG_strdup("CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsAFLR4
         * - <B> Proj_Name = NULL</B> <br>
         * This corresponds to the output name of the mesh. If left NULL, the mesh is not written to a file.
         */

    } if (index == ++input) {
        *ainame               = EG_strdup("Mesh_Quiet_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = false;

        /*! \page aimInputsAFLR4
         * - <B> Mesh_Quiet_Flag = False</B> <br>
         * Complete suppression of mesh generator (not including errors)
         */

    } if (index == ++input) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->vals.string   = EG_strdup("AFLR3"); // TECPLOT, VTK, AFLR3, STL, FAST
        defval->lfixed        = Change;

        /*! \page aimInputsAFLR4
         * - <B> Mesh_Format = "AFLR3"</B> <br>
         * Mesh output format. Available format names include: "AFLR3", "VTK", "TECPLOT", "STL" (quadrilaterals will be
         * split into triangles), "FAST".
         */

    } if (index == ++input) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsAFLR4
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file if applicable.
         */

    } if (index == ++input) {
        *ainame               = EG_strdup("Mesh_Gen_Input_String");
        defval->type          = String;
        defval->nullVal       = IsNull;
        defval->vals.string   = NULL;

        /*! \page aimInputsAFLR4
         * - <B> Mesh_Gen_Input_String = NULL</B> <br>
         * Meshing program command line string (as if called in bash mode). Use this to specify more<br>
         * complicated options/use features of the mesher not currently exposed through other AIM input<br>
         * variables. Note that this is the exact string that will be provided to the volume mesher; no<br>
         * modifications will be made. If left NULL an input string will be created based on default values<br>
         * of the relevant AIM input variables.
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"ff_cdfr", &ff_cdfr, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'ff_cdfr'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame              = EG_strdup("ff_cdfr");
        defval->type         = Double;
        defval->dim          = Scalar;
        defval->vals.real    = ff_cdfr;

        /*! \page aimInputsAFLR4
         * - <B>ff_cdfr</B> <br>
         * Farfield growth rate for field point spacing.<br>
         * The farfield spacing is set to a uniform value dependent upon the maximum size <br>
         * of the domain, maximum size of inner bodies, max and min body spacing, and <br>
         * farfield growth rate. ; <br>
         * ff_spacing = (ff_cdfr-1)*L+(min_spacing+max_spacing)/2 ; <br>
         * where L is the approximate distance between inner bodies and farfield. <br>
         */

    } if (index == ++input) {

        status = ug_get_int_param ((char*)"min_ncell", &min_ncell, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'min_ncell'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame              = EG_strdup("min_ncell");
        defval->type         = Integer;
        defval->dim          = Scalar;
        defval->vals.integer  = min_ncell;

        /*! \page aimInputsAFLR4
         * - <B>min_ncell</B> <br>
         * Minimum number of cells between two components/bodies.<br>
         * Proximity of components/bodies<br>
         * to each other is estimated and surface<br>
         * spacing is locally reduced if needed. Local<br>
         * surface spacing is selectively reduced when<br>
         * components/bodies are close and their<br>
         * existing local surface spacing would generate<br>
         * less than the minimum number of cells<br>
         * specified by min_ncell. Proximity checking is<br>
         * automatically disabled if min_ncell=1 or if<br>
         * there is only one component/body defined.
         */

    } if (index == ++input) {

        status = ug_get_int_param ((char*)"mer_all", &mer_all, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'mer_all'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame              = EG_strdup("mer_all");
        defval->type         = Integer;
        defval->dim          = Scalar;
        defval->vals.integer = mer_all;

        /*! \page aimInputsAFLR4
         * - <B>mer_all</B> <br>
         * Global edge mesh spacing scale factor flag.<br>
         * Edge mesh spacing can be scaled on all surfaces based on discontinuity level<br>
         * between adjacent surfaces on both sides of the edge. For each surface the level<br>
         * of discontinuity (as defined by angerw1 and angerw2) determines the edge<br>
         * spacing scale factor for potentially reducing the edge spacing. See erw_ids and<br>
         * erw_list. This option is equivalent to setting erw_ids equal to the list of all<br>
         * surface IDS and the edge mesh spacing scale factor weight in erw_list equal to<br>
         * one. Note that no modification is done to edges that belong to surfaces with a<br>
         * grid BC of farfield (ff_ids) or BL intersecting (int_ids).
         */

    } if (index == ++input) {

        status = ug_get_char_param ((char*)"-no_prox", no_prox, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'no_prox'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame              = EG_strdup("no_prox");
        defval->type         = Boolean;
        defval->dim          = Scalar;
        defval->vals.integer = False;

        /*! \page aimInputsAFLR4
         * - <B>no_prox</B> <br>
         * Disable proximity check flag.<br>
         * If no_prox=False then proximity of components/bodies to each other is estimated <br>
         * and surface spacing is locally reduced if needed. <br>
         * If no_prox=True or if there is only one component/body defined then proximity <br>
         * checking is disabled. <br>
         *
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"abs_min_scale", &abs_min_scale, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'abs_min_scale'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame           = EG_strdup("abs_min_scale");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = abs_min_scale;

        /*! \page aimInputsAFLR4
         * - <B>abs_min_scale</B> <br>
         * Relative scale of absolute minimum spacing to<br>
         * reference length. The relative scale of<br>
         * absolute minimum spacing to reference length<br>
         * (ref_len) controls the absolute minimum<br>
         * spacing that can be set on any component/body<br>
         * surface by proximity checking (see min_ncell).<br>
         * Note that the value of abs_min_scale is limited to be less<br>
         * than or equal to min_scale.
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"BL_thickness", &BL_thickness, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'BL_thickness'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame           = EG_strdup("BL_thickness");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = BL_thickness;

        /*! \page aimInputsAFLR4
         * - <B>BL_thickness</B> <br>
         * Boundary layer thickness for proximity checking. <br>
         * Proximity of components/bodies to each other is estimated and surface spacing <br>
         * is locally reduced if needed. Note that if the Reynolds Number, Re_l, is set <br>
         * then the BL_thickness value is set to an estimate for turbulent flow. If the <br>
         * set or calculated value of BL_thickness>0 then the boundary layer thickness is <br>
         * included in the calculation for the required surface spacing during proximity <br>
         * checking.
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"Re_l", &Re_l, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'Re_l'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame           = EG_strdup("Re_l");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = Re_l;

        /*! \page aimInputsAFLR4
         * - <B>BL_thickness</B> <br>
         * Reynolds Number for estimating BL thickness.<br>
         * The Reynolds Number based on reference length, Re_l, (if set) along with <br>
         * reference length, ref_len, are used to estimate the BL thickness, BL_thickness, <br>
         * for turbulent flow. If Re_l>0 then this estimated value is used to set <br>
         * BL_thickness.
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"curv_factor", &curv_factor, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'curv_factor'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame           = EG_strdup("curv_factor");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = curv_factor;

        /*! \page aimInputsAFLR4
         * - <B>curv_factor</B> <br>
         * Curvature factor<br>
         * For surface curvature the spacing is derived from the curvature factor divided<br>
         * by the curvature.<br>
         * Curvature = 1 / Curvature_Radius<br>
         * Spacing = curv_factor / Curvature<br>
         * The resulting spacing between is limited by the minimum and maximum spacing set<br>
         * by min_scale and max_scale. Note that if curv_factor=0 then surface curvature<br>
         * adjustment is not used.
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"erw_all", &erw_all, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'erw_all'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame           = EG_strdup("erw_all");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = erw_all;

        /*! \page aimInputsAFLR4
         * - <B>erw_all</B> <br>
         * Global edge mesh spacing refinement weight.<br>
         * Edge mesh spacing can be scaled on all surfaces (if mer_all=1) based on<br>
         * discontinuity level between adjacent surfaces on both sides of the edge.<br>
         * For each surface the level of discontinuity (as defined by angerw1 and angerw2)<br>
         * determines the edge spacing scale factor for potentially reducing the edge<br>
         * spacing. The edge mesh spacing scale factor weight is then used as an<br>
         * interpolation weight between the unmodified spacing and the modified spacing.<br>
         * A value of one applies the maximum modification and a value of zero applies no<br>
         * change in edge spacing. If the global edge mesh spacing scale factor flag,<br>
         * mer_all, is set to 1 then that is equivalent to setting AFLR_Edge_Scale_Factor_Weight<br>
         * on all FACEs to the value erw_all. Note that no modification is<br>
         * done to edges that belong to surfaces with a grid BC of farfield (FARFIELD_UG3_GBC) or BL<br>
         * intersecting. Also, note that the global weight, erw_all, is not<br>
         * applicable if mer_all=0.
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"max_scale", &max_scale, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'max_scale'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame           = EG_strdup("max_scale");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = max_scale;

        /*! \page aimInputsAFLR4
         * - <B>max_scale</B> <br>
         * Relative scale of maximum spacing to<br>
         * reference length. The relative scale of<br>
         * maximum spacing to reference length (ref_len)<br>
         * controls the maximum spacing that can be set<br>
         * on any component/body surface.
         */

    } if (index == ++input) {

        status = ug_get_double_param ((char*)"min_scale", &min_scale, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'min_scale'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        *ainame           = EG_strdup("min_scale");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = min_scale;

        /*! \page aimInputsAFLR4
         * - <B>min_scale</B> <br>
         * Relative scale of minimum spacing to<br>
         * reference length. The relative scale of<br>
         * minimum spacing to reference length (ref_len)<br>
         * controls the minimum spacing that can be set<br>
         * on any component/body surface.
         */

    } if (index == ++input) {

      /* There is no reasonable default for the ref_len parameter,
       * the user must always set it via capsMeshLength andd Mesh_Length_Factor
       *
        status = ug_get_double_param ((char*)"ref_len", &ref_len, AFLR4_Param_Struct_Ptr);
        if (status == 1) status = CAPS_SUCCESS;
        if (status != CAPS_SUCCESS) {
            printf("Failed to retrieve default value for 'ref_len'\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }
     */

        *ainame           = EG_strdup("Mesh_Length_Factor");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->vals.real = 1;
        defval->nullVal   = NotNull;

        /*! \page aimInputsAFLR4
         * - <B>Mesh_Length_Factor = 1</B> <br>
         * Scaling factor to compute AFLR4 'ref_len' parameter via:<br>
         * <br>
         * ref_len = capsMeshLength * Mesh_Length_Factor<br>
         * <br>
         * where capsMeshLength is a numeric attribute that<br>
         * must be on at least one body and consistent if on multiple bodies.<br>
         * <br>
         * ref_len:<br>
         * Reference length for components/bodies in grid units. Reference length should<br>
         * be set to a physically relevant characteristic length for the configuration<br>
         * such as wing chord length or pipe diameter. If ref_len = 0 then it will be set<br>
         * to the bounding box for the largest component/body of interest.<br>
         * The parameters ref_len, max_scale, min_scale and abs_min_scale are all used to<br>
         * set spacing values on all component/body surfaces (those that are not on the<br>
         * farfield or symmetry plane-if any).<br>
         * <br>
         * max_spacing = max_scale * ref_len<br>
         * min_spacing = min_scale * ref_len<br>
         * abs_min_spacing = abs_min_scale * ref_len<br>
         */

    } if (index == ++input) {
        *ainame              = EG_strdup("Mesh_Sizing");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsAFLR4
         * - <B> Mesh_Sizing = NULL </B> <br>
         * See \ref meshSizingProp for additional details.
         */

    } if (index == ++input) {
        *ainame              = EG_strdup("EGADS_Quad");
        defval->type         = Boolean;
        defval->vals.integer  = false;

        /*! \page aimInputsAFLR4
         * - <B> EGADS_Quad = False </B> <br>
         * Apply EGADS quadding to the AFLR4 triangulation.
         */
    }

    if (input != NUMINPUT) {
        printf("DEVELOPER ERROR: NUMINPUTS %d != %d\n", NUMINPUT, input);
        return CAPS_BADINDEX;
    }

status = CAPS_SUCCESS;
cleanup:
    ug_free_param(AFLR4_Param_Struct_Ptr);

    if (status != CAPS_SUCCESS) {
        printf("An error occurred creating aimInputs\n");
    }
    return status;
}

int aimData(int iIndex, const char *name, enum capsvType *vtype,
            int *rank, int *nrow, int *ncol, void **data, char **units)
{
    /*! \page sharableDataAFLR4 AIM Shareable Data
     * The AFLR4 AIM has the following shareable data types/values with its children AIMs if they are so inclined.
     * - <B> Surface_Mesh</B> <br>
     * The returned surface mesh after AFLR4 execution is complete in meshStruct (see meshTypes.h) format.
     * - <B> Attribute_Map</B> <br>
     * An index mapping between capsGroups found on the geometry in mapAttrToIndexStruct (see miscTypes.h) format.
     */

    #ifdef DEBUG
        printf(" aflr4AIM/aimData instance = %d  name = %s!\n", inst, name);
    #endif

    // The returned surface mesh from aflr4

    if (strcasecmp(name, "Surface_Mesh") == 0){
        *vtype = Value;
        *rank = *ncol = 1;
        *nrow  = aflr4Instance[iIndex].numSurface;
        *data  = aflr4Instance[iIndex].surfaceMesh;
        *units = NULL;

        return CAPS_SUCCESS;
    }

    // Share the attribute map
    if (strcasecmp(name, "Attribute_Map") == 0){
         *vtype = Value;
         *rank  = *nrow = *ncol = 1;
         *data  = &aflr4Instance[iIndex].attrMap;
         *units = NULL;

         return CAPS_SUCCESS;
    }

    return CAPS_NOTFOUND;

}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath, capsValue *aimInputs, capsErrs **errs)
{
    int status; // Status return

    int i, bodyIndex; // Indexing

    // Body parameters
    const char *intents;
    int numBody = 0; // Number of bodies
    ego *bodies = NULL; // EGADS body objects

    // File output
    char *filename = NULL;
    char bodyNumber[11];

    // Mesh attribute parameters
    int numMeshProp = 0;
    meshSizingStruct *meshProp = NULL;

    // NULL out errors
    *errs = NULL;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    #ifdef DEBUG
        printf(" aflr4AIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, numBody);
    #endif

    if (numBody <= 0 || bodies == NULL) {
        #ifdef DEBUG
            printf(" aflr4AIM/aimPreAnalysis No Bodies!\n");
        #endif
        return CAPS_SOURCEERR;
    }

    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the same instance
    status = destroy_aimStorage(iIndex);
    if (status != CAPS_SUCCESS) {
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr4AIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
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
                                            2, // Edge level
                                            &aflr4Instance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Allocate surfaceMesh from number of bodies
    aflr4Instance[iIndex].numSurface = numBody;
    aflr4Instance[iIndex].surfaceMesh = (meshStruct *) EG_alloc(aflr4Instance[iIndex].numSurface*sizeof(meshStruct));
    if (aflr4Instance[iIndex].surfaceMesh == NULL) {
        aflr4Instance[iIndex].numSurface = 0;
        return EGADS_MALLOC;
    }

    // Initiate surface meshes
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++){
        status = initiate_meshStruct(&aflr4Instance[iIndex].surfaceMesh[bodyIndex]);
        if (status != CAPS_SUCCESS) return status;
    }

    // Setup meshing input structure -> -1 because aim_getIndex is 1 bias

    // Get Tessellation parameters -Tess_Params -> -1 because of aim_getIndex 1 bias
    aflr4Instance[iIndex].meshInput.paramTess[0] = 0;
    aflr4Instance[iIndex].meshInput.paramTess[1] = 0;
    aflr4Instance[iIndex].meshInput.paramTess[2] = 0;

    aflr4Instance[iIndex].meshInput.quiet            = aimInputs[aim_getIndex(aimInfo, "Mesh_Quiet_Flag",    ANALYSISIN)-1].vals.integer;
    aflr4Instance[iIndex].meshInput.outputASCIIFlag  = aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag",    ANALYSISIN)-1].vals.integer;

    // Mesh Format
    aflr4Instance[iIndex].meshInput.outputFormat = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].vals.string);
    if (aflr4Instance[iIndex].meshInput.outputFormat == NULL) return EGADS_MALLOC;

    // Project Name
    if (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].nullVal != IsNull) {

        aflr4Instance[iIndex].meshInput.outputFileName = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
        if (aflr4Instance[iIndex].meshInput.outputFileName == NULL) return EGADS_MALLOC;
    }

    // Output directory
    aflr4Instance[iIndex].meshInput.outputDirectory = EG_strdup(analysisPath);
    if (aflr4Instance[iIndex].meshInput.outputDirectory == NULL) return EGADS_MALLOC;

    // Set aflr4 specific mesh inputs  -1 because aim_getIndex is 1 bias
    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].nullVal != IsNull) {

        aflr4Instance[iIndex].meshInput.aflr4Input.meshInputString = EG_strdup(aimInputs[aim_getIndex(aimInfo, "Mesh_Gen_Input_String", ANALYSISIN)-1].vals.string);
        if (aflr4Instance[iIndex].meshInput.aflr4Input.meshInputString == NULL) return EGADS_MALLOC;
    }

    // Get mesh sizing parameters
    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].nullVal != IsNull) {

        status = mesh_getSizingProp(aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].length,
                                    aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].vals.tuple,
                                    &aflr4Instance[iIndex].attrMap,
                                    &numMeshProp,
                                    &meshProp);
        if (status != CAPS_SUCCESS) goto cleanup;

        // apply the sizing attributes
        for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {
            status = setAFLR4Attr(bodies[bodyIndex],
                                  &aflr4Instance[iIndex].attrMap,
                                  numMeshProp,
                                  meshProp);
            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }


    status = aflr4_Surface_Mesh(aflr4Instance[iIndex].meshInput.quiet,
                                numBody, bodies,
                                aimInfo, aimInputs,
                                aflr4Instance[iIndex].meshInput,
                                aflr4Instance[iIndex].attrMap,
                                aflr4Instance[iIndex].surfaceMesh);
    if (status != CAPS_SUCCESS) {
        printf("Problem during AFLR4 surface meshing\n");
        goto cleanup;
    }

    if (aflr4Instance[iIndex].meshInput.outputFileName != NULL) {

        for (bodyIndex = 0; bodyIndex < aflr4Instance[iIndex].numSurface; bodyIndex++) {

            if (aflr4Instance[iIndex].numSurface > 1) {
                sprintf(bodyNumber, "%d", bodyIndex);
                filename = (char *) EG_alloc((strlen(aflr4Instance[iIndex].meshInput.outputFileName)  +
                                              strlen(aflr4Instance[iIndex].meshInput.outputDirectory) +
                                              2 +
                                              strlen("_Surf_") +
                                              strlen(bodyNumber))*sizeof(char));
            } else {
                filename = (char *) EG_alloc((strlen(aflr4Instance[iIndex].meshInput.outputFileName) +
                                              strlen(aflr4Instance[iIndex].meshInput.outputDirectory)+2)*sizeof(char));

            }

            if (filename == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            strcpy(filename, aflr4Instance[iIndex].meshInput.outputDirectory);
            #ifdef WIN32
                strcat(filename, "\\");
            #else
                strcat(filename, "/");
            #endif
            strcat(filename, aflr4Instance[iIndex].meshInput.outputFileName);

            if (aflr4Instance[iIndex].numSurface > 1) {
                strcat(filename,"_Surf_");
                strcat(filename, bodyNumber);
            }

            if (strcasecmp(aflr4Instance[iIndex].meshInput.outputFormat, "AFLR3") == 0) {

                status = mesh_writeAFLR3(filename,
                                         aflr4Instance[iIndex].meshInput.outputASCIIFlag,
                                         &aflr4Instance[iIndex].surfaceMesh[bodyIndex],
                                         1.0);

            } else if (strcasecmp(aflr4Instance[iIndex].meshInput.outputFormat, "VTK") == 0) {

                status = mesh_writeVTK(filename,
                                       aflr4Instance[iIndex].meshInput.outputASCIIFlag,
                                       &aflr4Instance[iIndex].surfaceMesh[bodyIndex],
                                       1.0);

            } else if (strcasecmp(aflr4Instance[iIndex].meshInput.outputFormat, "Tecplot") == 0) {

                status = mesh_writeTecplot(filename,
                                           aflr4Instance[iIndex].meshInput.outputASCIIFlag,
                                           &aflr4Instance[iIndex].surfaceMesh[bodyIndex],
                                           1.0);

            } else if (strcasecmp(aflr4Instance[iIndex].meshInput.outputFormat, "STL") == 0) {

                status = mesh_writeSTL(filename,
                                       aflr4Instance[iIndex].meshInput.outputASCIIFlag,
                                       &aflr4Instance[iIndex].surfaceMesh[bodyIndex],
                                       1.0);

            } else if (strcasecmp(aflr4Instance[iIndex].meshInput.outputFormat, "FAST") == 0) {

                status = mesh_writeFAST(filename,
                                        aflr4Instance[iIndex].meshInput.outputASCIIFlag,
                                        &aflr4Instance[iIndex].surfaceMesh[bodyIndex],
                                        1.0);

            } else if (strcasecmp(aflr4Instance[iIndex].meshInput.outputFormat, "ETO") == 0) {

                // This format is not yet anything official. Do not document it!
                filename = (char *) EG_reall(filename,(strlen(filename) + 5) *sizeof(char));
                if (filename == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }
                strcat(filename,".eto");

                status = EG_saveTess(aflr4Instance[iIndex].surfaceMesh[bodyIndex].bodyTessMap.egadsTess, filename);

            } else {
                printf("Unrecognized mesh format, \"%s\", the volume mesh will not be written out\n", aflr4Instance[iIndex].meshInput.outputFormat);
            }

            if (filename != NULL) EG_free(filename);
            filename = NULL;

            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }


    status = CAPS_SUCCESS;
    goto cleanup;

cleanup:

    // Clean up meshProps
    if (meshProp != NULL) {
        for (i = 0; i < numMeshProp; i++) {
            (void) destroy_meshSizingStruct(&meshProp[i]);
        }
        EG_free(meshProp); meshProp = NULL;
    }

    if (status != CAPS_SUCCESS) printf("Error: aflr4AIM (instance = %d) status %d\n", iIndex, status);

    if (filename != NULL) EG_free(filename);
    return status;
}


int aimOutputs(/*@unused@*/ int iIndex, /*@unused@*/ void *aimStruc,
               /*@unused@*/ int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsAFLR4 AIM Outputs
     * The following list outlines the AFLR4 AIM outputs available through the AIM interface.
     */

#ifdef DEBUG
    printf(" aflr4AIM/aimOutputs instance = %d  index = %d!\n", inst, index);
#endif

    int output = 0;

    if (index == ++output) {
        *aoname = EG_strdup("Done");
        form->type = Boolean;
        form->vals.integer = (int) false;

        /*! \page aimOutputsAFLR4
         * - <B> Done </B> <br>
         * True if a surface mesh was created on all surfaces, False if not.
         */

    } if (index == ++output) {
        *aoname = EG_strdup("NumberOfElement");
        form->type = Integer;
        form->vals.integer = 0;

        /*! \page aimOutputsAFLR4
         * - <B> NumberOfElement </B> <br>
         * Number of elements in the surface mesh
         */

    } if (index == ++output) {
        *aoname = EG_strdup("NumberOfNode");
        form->type = Integer;
        form->vals.integer = 0;

        /*! \page aimOutputsAFLR4
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
int aimCalcOutput(int iIndex, void *aimInfo,
                  const char *apath, int index,
                  capsValue *val, capsErrs **errors)
{
    int status = CAPS_SUCCESS;
    int surf; // Indexing
    int numElement, numNodes, nElem;

    #ifdef DEBUG
        printf(" aflr4AIM/aimCalcOutput instance = %d  index = %d!\n", inst, index);
    #endif

    *errors = NULL;

    if (aim_getIndex(aimInfo, "Done", ANALYSISOUT) == index) {

        val->vals.integer = (int) false;

        // Check to see if surface meshes was generated
        for (surf = 0; surf < aflr4Instance[iIndex].numSurface; surf++ ) {

            if (aflr4Instance[iIndex].surfaceMesh[surf].numElement != 0) {

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
        for (surf = 0; surf < aflr4Instance[iIndex].numSurface; surf++ ) {

            status = mesh_retrieveNumMeshElements(aflr4Instance[iIndex].surfaceMesh[surf].numElement,
                                                  aflr4Instance[iIndex].surfaceMesh[surf].element,
                                                  Triangle,
                                                  &nElem);
            if (status != CAPS_SUCCESS) goto cleanup;
            numElement += nElem;

            status = mesh_retrieveNumMeshElements(aflr4Instance[iIndex].surfaceMesh[surf].numElement,
                                                  aflr4Instance[iIndex].surfaceMesh[surf].element,
                                                  Quadrilateral,
                                                  &nElem);
            if (status != CAPS_SUCCESS) goto cleanup;
            numElement += nElem;
        }

        val->vals.integer = numElement;

    } else if (aim_getIndex(aimInfo, "NumberOfNode", ANALYSISOUT) == index) {

        // Count the total number of surface vertices
        numNodes = 0;
        for (surf = 0; surf < aflr4Instance[iIndex].numSurface; surf++ ) {
            numNodes += aflr4Instance[iIndex].surfaceMesh[surf].numNode;
        }

        val->vals.integer = numNodes;
    } else {
        printf("DEVELOPER ERROR: Unknown output index %d\n", index);
        return CAPS_BADINDEX;
    }

cleanup:

    return status;
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
        printf(" aflr4AIM/aimCleanup!\n");
    #endif

    // Clean up aflr4Instance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up aflr4Instance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS) printf("Status = %d, aflr4AIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
    }

    if (aflr4Instance != NULL) EG_free(aflr4Instance);
    aflr4Instance = NULL;
    numInstance = 0;

}

int aimLocateElement(/*@unused@*/ capsDiscr *discr, /*@unused@*/ double *params,
                 /*@unused@*/ double *param,    /*@unused@*/ int *eIndex,
                 /*@unused@*/ double *bary)
{
#ifdef DEBUG
  printf(" aflr4AIM/aimLocateElement instance = %d!\n", discr->instance);
#endif

  return CAPS_SUCCESS;
}

int aimTransfer(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
            /*@unused@*/ int npts, /*@unused@*/ int rank,
            /*@unused@*/ double *data, /*@unused@*/ char **units)
{
#ifdef DEBUG
  printf(" aflr4AIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n",
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
  printf(" aflr4AIM/aimInterpolation  %s  instance = %d!\n",
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
  printf(" aflr4AIM/aimInterpolateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif

  return CAPS_SUCCESS;
}


int aimIntegration(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
               /*@unused@*/ int eIndex, /*@unused@*/ int rank,
               /*@unused@*/ /*@null@*/ double *data, /*@unused@*/ double *result)
{
#ifdef DEBUG
  printf(" aflr4AIM/aimIntegration  %s  instance = %d!\n",
         name, discr->instance);
#endif

  return CAPS_SUCCESS;
}


int aimIntegrateBar(/*@unused@*/ capsDiscr *discr, /*@unused@*/ const char *name,
                /*@unused@*/ int eIndex, /*@unused@*/ int rank,
                /*@unused@*/ double *r_bar, /*@unused@*/ double *d_bar)
{
#ifdef DEBUG
  printf(" aflr4AIM/aimIntegrateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif

  return CAPS_SUCCESS;
}
