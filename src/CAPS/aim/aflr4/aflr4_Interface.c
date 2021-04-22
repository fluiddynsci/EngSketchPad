// AFLR4 interface functions - Modified from functions provided with
// AFLR4_LIB source written by David L. Marcum

//  Modified by Dr. Ryan Durscher AFRL/RQVC
//  Modified by Dr. Marshall Galbraith MIT

#ifdef WIN32
#define strcasecmp stricmp
#define strtok_r   strtok_s
typedef int         pid_t;   
#endif

#include <aflr4/AFLR4_LIB.h> // Bring in AFLR4 API library
#include <egads_aflr4/EGADS_AFLR4_LIB_INC.h>

#include "meshUtils.h" // Collection of helper functions for meshing
#include "miscUtils.h"
#include "aimUtil.h"


int aflr4_Surface_Mesh(int quiet,
                       int numBody, ego *bodies,
                       void *aimInfo, capsValue *aimInputs,
                       meshInputStruct meshInput,
                       mapAttrToIndexStruct attrMap,
                       meshStruct *surfaceMeshes)
{
    int status; // Function return status

    int numFace = 0; // Number of faces on a body

    int faceIndex = 0, bodyIndex = 0;

    int numNodeTotal = 0, numElemTotal = 0;

    const char *pstring = NULL;
    const int *pints = NULL;
    const double *preals = NULL;
    int atype, n;

    double capsMeshLength = 0, meshLenFac = 0;

    ego *faces = NULL, *copy_bodies=NULL, context, tess, model=NULL;

    int min_ncell, mer_all, no_prox;
    int EGADS_Quad = (int)false;

    double ff_cdfr, abs_min_scale, BL_thickness, Re_l, curv_factor,
           max_scale, min_scale, ref_len, erw_all;

     // Commandline inputs
    int  prog_argc   = 1;    // Number of arguments
    char **prog_argv = NULL; // String arrays
    char *meshInputString = NULL;

    // return values from egads_aflr4_tess
    ego *tessBodies = NULL;

    // Do some additional anity checks on the attributes
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

        status = EG_getBodyTopos (bodies[bodyIndex], NULL, FACE, &numFace, &faces);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (faceIndex = 0; faceIndex < numFace ; faceIndex++) {

            // AFLR Grid BC values should be set for each CAD surface
            // check to make sure the values are not bad

            // check to see if AFLR_GBC is already set
            status = EG_attributeRet(faces[faceIndex], "AFLR_GBC", &atype, &n, &pints, &preals, &pstring);

            if (status == EGADS_SUCCESS) {
                if (atype == ATTRSTRING) {
                    if ( !( (strcasecmp (pstring, "STD_UG3_GBC") == 0)            ||
                            (strcasecmp (pstring, "-STD_UG3_GBC") == 0)           ||
                            (strcasecmp (pstring, "FARFIELD_UG3_GBC") == 0)       ||
                            (strcasecmp (pstring, "BL_INT_UG3_GBC") == 0)         ||
                            (strcasecmp (pstring, "TRANSP_SRC_UG3_GBC") == 0)     ||
                            (strcasecmp (pstring, "TRANSP_BL_INT_UG3_GBC") == 0)  ||
                            (strcasecmp (pstring, "TRANSP_UG3_GBC") == 0)         ||
                            (strcasecmp (pstring, "-TRANSP_UG3_GBC") == 0)        ||
                            (strcasecmp (pstring, "TRANSP_INTRNL_UG3_GBC") == 0)  ||
                            (strcasecmp (pstring, "-TRANSP_INTRNL_UG3_GBC") == 0) ) ) {
                        printf("**********************************************************\n");
                        printf("Invalid AFLR_GBC on face %d of body %d: \"%s\"\n", faceIndex+1, bodyIndex+1, pstring);
                        printf("Valid string values are:\n");
                        printf("  FARFIELD_UG3_GBC       : farfield surface\n");
                        printf("  STD_UG3_GBC            : standard surface\n");
                        printf("  -STD_UG3_GBC           : standard surface\n");
                        printf("                           BL generating surface\n");
                        printf("  BL_INT_UG3_GBC         : symmetry or standard surface that intersects BL\n");
                        printf("  TRANSP_SRC_UG3_GBC     : embedded/transparent surface\n");
                        printf("                           converted to source nodes\n");
                        printf("  TRANSP_BL_INT_UG3_GBC  : embedded/transparent surface that intersects BL\n");
                        printf("  TRANSP_UG3_GBC         : embedded/transparent surface\n");
                        printf("  -TRANSP_UG3_GBC        : embedded/transparent surface\n");
                        printf("                           BL generating surface\n");
                        printf("  TRANSP_INTRNL_UG3_GBC  : embedded/transparent surface\n");
                        printf("                           converted to an internal surface\n");
                        printf("                           coordinates are retained but connectivity is not\n");
                        printf("  -TRANSP_INTRNL_UG3_GBC : embedded/transparent surface\n");
                        printf("                           converted to an internal surface\n");
                        printf("                           coordinates are retained but connectivity is not\n");
                        printf("                           BL generating surface\n");
                        printf("**********************************************************\n");
                        status = CAPS_BADVALUE;
                        goto cleanup;
                    }
                } else {
                    printf("**********************************************************\n");
                    printf("AFLR_GBC on face %d of body %d has %d entries ", faceIndex+1, bodyIndex+1, n);
                    if (atype == ATTRREAL)        printf("of reals\n");
                    else if (atype == ATTRINT)    printf("of integers\n");
                    printf("Should only contain a string!\n");
                    printf("**********************************************************\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }

            // check for deprecation
            status = EG_attributeRet(faces[faceIndex], "AFLR_Cmp_ID", &atype, &n, &pints, &preals, &pstring);
            if (status == EGADS_SUCCESS) {
              printf("**********************************************************\n");
              printf("Error: AFLR_Cmp_ID on face %d of body %d is deprecated\n", faceIndex+1, bodyIndex+1);
              printf("   use AFLR4_Cmp_ID instead!\n");
              printf("**********************************************************\n");
              status = CAPS_BADVALUE;
              goto cleanup;
            }

            // check to see if AFLR4_Cmp_ID is set correctly
            status = EG_attributeRet(faces[faceIndex], "AFLR4_Cmp_ID", &atype, &n, &pints, &preals, &pstring);

            if (status == EGADS_SUCCESS && (!(atype == ATTRREAL || atype == ATTRINT) || n != 1)) {
                //make sure it is only a single real
                printf("**********************************************************\n");
                printf("AFLR4_Cmp_ID on face %d of body %d has %d entries ", faceIndex+1, bodyIndex+1, n);
                if (atype == ATTRREAL)        printf("of reals\n");
                else if (atype == ATTRINT)    printf("of integers\n");
                else if (atype == ATTRSTRING) printf("of a string\n");
                printf("Should only contain a single integer or real!\n");
                printf("**********************************************************\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }

            // check for deprecation
            status = EG_attributeRet(faces[faceIndex], "AFLR_Scale_Factor", &atype, &n, &pints, &preals, &pstring);
            if (status == EGADS_SUCCESS) {
              printf("**********************************************************\n");
              printf("Error: AFLR_Scale_Factor on face %d of body %d is deprecated\n", faceIndex+1, bodyIndex+1);
              printf("   use AFLR4_Scale_Factor instead!\n");
              printf("**********************************************************\n");
              status = CAPS_BADVALUE;
              goto cleanup;
            }

            // check to see if AFLR4_Scale_Factor is already set
            status = EG_attributeRet(faces[faceIndex], "AFLR4_Scale_Factor", &atype, &n, &pints, &preals, &pstring);

            if (status == EGADS_SUCCESS && !(atype == ATTRREAL && n == 1)) {
                //make sure it is only a single real
                printf("**********************************************************\n");
                printf("AFLR4_Scale_Factor on face %d of body %d has %d entries ", faceIndex+1, bodyIndex+1, n);
                if (atype == ATTRREAL)
                    printf("of reals\n");
                else if (atype == ATTRINT)
                    printf("of integers\n");
                else if (atype == ATTRSTRING)
                    printf("of a string\n");
                printf("Should only contain a single real!\n");
                printf("**********************************************************\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }

            // edge mesh spacing scale factor weight for each CAD definition
            // no need to set if scale factor weight is 0.0 (default)

            // check for deprecation
            status = EG_attributeRet(faces[faceIndex], "AFLR_Edge_Scale_Factor_Weight", &atype, &n, &pints, &preals, &pstring);
            if (status == EGADS_SUCCESS) {
              printf("**********************************************************\n");
              printf("Error: AFLR_Edge_Scale_Factor_Weight on face %d of body %d is deprecated\n", faceIndex+1, bodyIndex+1);
              printf("   use AFLR4_Edge_Refinement_Weight instead!\n");
              printf("**********************************************************\n");
              status = CAPS_BADVALUE;
              goto cleanup;
            }

            status = EG_attributeRet(faces[faceIndex], "AFLR4_Edge_Scale_Factor_Weight", &atype, &n, &pints, &preals, &pstring);
            if (status == EGADS_SUCCESS) {
              printf("**********************************************************\n");
              printf("Error: AFLR4_Edge_Scale_Factor_Weight on face %d of body %d is deprecated\n", faceIndex+1, bodyIndex+1);
              printf("   use AFLR4_Edge_Refinement_Weight instead!\n");
              printf("**********************************************************\n");
              status = CAPS_BADVALUE;
              goto cleanup;
            }

            // check to see if AFLR4_Edge_Refinement_Weight is already set
            status = EG_attributeRet(faces[faceIndex], "AFLR4_Edge_Refinement_Weight", &atype, &n, &pints, &preals, &pstring);

            if (status == EGADS_SUCCESS && !(atype == ATTRREAL && n == 1) ) {
                //make sure it is only a single real
                printf("**********************************************************\n");
                printf("AFLR4_Edge_Refinement_Weight on face %d of body %d has %d entries ", faceIndex+1, bodyIndex+1, n);
                if (atype == ATTRREAL)
                    printf("of reals\n");
                else if (atype == ATTRINT)
                    printf("of integers\n");
                else if (atype == ATTRSTRING)
                    printf("of a string\n");
                printf("Should only contain a single real!\n");
                printf("**********************************************************\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        EG_free(faces); faces = NULL;
    }

    // get AFLR parameters from user inputs

    ff_cdfr       = aimInputs[aim_getIndex(aimInfo, "ff_cdfr"           , ANALYSISIN)-1].vals.real;
    min_ncell     = aimInputs[aim_getIndex(aimInfo, "min_ncell"         , ANALYSISIN)-1].vals.integer;
    mer_all       = aimInputs[aim_getIndex(aimInfo, "mer_all"           , ANALYSISIN)-1].vals.integer;
    no_prox       = aimInputs[aim_getIndex(aimInfo, "no_prox"           , ANALYSISIN)-1].vals.integer;

    BL_thickness  = aimInputs[aim_getIndex(aimInfo, "BL_thickness"      , ANALYSISIN)-1].vals.real;
    Re_l          = aimInputs[aim_getIndex(aimInfo, "Re_l"              , ANALYSISIN)-1].vals.real;
    curv_factor   = aimInputs[aim_getIndex(aimInfo, "curv_factor"       , ANALYSISIN)-1].vals.real;
    abs_min_scale = aimInputs[aim_getIndex(aimInfo, "abs_min_scale"     , ANALYSISIN)-1].vals.real;
    max_scale     = aimInputs[aim_getIndex(aimInfo, "max_scale"         , ANALYSISIN)-1].vals.real;
    min_scale     = aimInputs[aim_getIndex(aimInfo, "min_scale"         , ANALYSISIN)-1].vals.real;
    erw_all       = aimInputs[aim_getIndex(aimInfo, "erw_all"           , ANALYSISIN)-1].vals.real;
    meshLenFac    = aimInputs[aim_getIndex(aimInfo, "Mesh_Length_Factor", ANALYSISIN)-1].vals.real;

    EGADS_Quad = aimInputs[aim_getIndex(aimInfo, "EGADS_Quad", ANALYSISIN)-1].vals.integer;

    status = check_CAPSMeshLength(numBody, bodies, &capsMeshLength);

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

    if (meshLenFac <= 0) {
      printf("**********************************************************\n");
      printf("Mesh_Length_Factor is: %f\n", meshLenFac);
      printf("Mesh_Length_Factor must be a positive number.\n");
      printf("**********************************************************\n");
      status = CAPS_BADVALUE;
      goto cleanup;
    }

    // compute the reference length used by AFLR4
    ref_len = meshLenFac*capsMeshLength;

    /*
    printf("ff_cdfr               = %d\n", ff_cdfr       );
    printf("min_ncell             = %d\n", min_ncell     );
    printf("mer_all               = %d\n", mer_all       );
    printf("no_prox               = %d\n", no_prox       );

    printf("BL_thickness          = %f\n", BL_thickness  );
    printf("Re_l                  = %f\n", Re_l          );
    printf("curv_factor           = %f\n", curv_factor   );
    printf("abs_min_scale         = %f\n", abs_min_scale );
    printf("max_scale             = %f\n", max_scale     );
    printf("min_scale             = %f\n", min_scale     );
    printf("ref_len               = %f\n", ref_len       );
    printf("erw_all               = %f\n", erw_all       );
    */

    // Allocate argument vector.

    status = ug_add_new_arg (&prog_argv, "allocate_and_initialize_argv");
    if (status != CAPS_SUCCESS) goto cleanup;

    // set AFLR4 input parameters

    status = ug_add_flag_arg (  "ff_cdfr"  , &prog_argc, &prog_argv);
    status = ug_add_double_arg ( ff_cdfr   , &prog_argc, &prog_argv);
    status = ug_add_flag_arg (  "min_ncell", &prog_argc, &prog_argv);
    status = ug_add_int_arg (    min_ncell , &prog_argc, &prog_argv);
    status = ug_add_flag_arg (  "mer_all" , &prog_argc, &prog_argv);
    status = ug_add_int_arg (    mer_all  , &prog_argc, &prog_argv);
    if (no_prox == True)
      status = ug_add_flag_arg ("-no_prox"    , &prog_argc, &prog_argv);

    status = ug_add_flag_arg ( "BL_thickness" , &prog_argc, &prog_argv);
    status = ug_add_double_arg (BL_thickness  , &prog_argc, &prog_argv);
    status = ug_add_flag_arg ( "Re_l"         , &prog_argc, &prog_argv);
    status = ug_add_double_arg (Re_l          , &prog_argc, &prog_argv);
    status = ug_add_flag_arg ( "curv_factor"  , &prog_argc, &prog_argv);
    status = ug_add_double_arg (curv_factor   , &prog_argc, &prog_argv);
    status = ug_add_flag_arg ( "abs_min_scale", &prog_argc, &prog_argv);
    status = ug_add_double_arg (abs_min_scale , &prog_argc, &prog_argv);
    status = ug_add_flag_arg ( "max_scale"    , &prog_argc, &prog_argv);
    status = ug_add_double_arg (max_scale     , &prog_argc, &prog_argv);
    status = ug_add_flag_arg ( "min_scale"    , &prog_argc, &prog_argv);
    status = ug_add_double_arg (min_scale     , &prog_argc, &prog_argv);
    status = ug_add_flag_arg ( "ref_len"      , &prog_argc, &prog_argv);
    status = ug_add_double_arg (ref_len       , &prog_argc, &prog_argv);
    status = ug_add_flag_arg ( "erw_all"      , &prog_argc, &prog_argv);
    status = ug_add_double_arg (erw_all       , &prog_argc, &prog_argv);

    // Add meshInputString arguments (if any) to argument vector.
    // Note that since this comes after setting aimInputs the
    // meshInputString could override the value set by aimInputs.
    // Move this to before setting aimInputs arguments to reverse
    // that behavior.

    if (meshInput.aflr4Input.meshInputString != NULL) {
        meshInputString = EG_strdup(meshInput.aflr4Input.meshInputString);

        status = ug_add_list_arg (meshInputString, &prog_argc, &prog_argv);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Set AFLR4 case name.
    // Used for any requested output files.

    if (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].nullVal != IsNull)
        ug_set_case_name (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
    else
        ug_set_case_name ("AFLR4");

    // Set quiet message flag.

    if (quiet == (int)true )
        status = ug_add_flag_arg ("mmsg=0", &prog_argc, &prog_argv);

    // Output AFLR4 heading.

    else
        aflr4_message (990, 0, 1200);

    // Register AFLR4-EGADS routines for CAD related setup & cleanup,
    // cad evaluation, cad bounds and generating boundary edge grids.

    aflr4_register_cad_geom_setup (egads_cad_geom_setup);
    aflr4_register_cad_geom_data_cleanup (egads_cad_geom_data_cleanup);
    aflr4_register_auto_cad_geom_setup (egads_auto_cad_geom_setup);

    dgeom_register_cad_eval_curv_at_uv (egads_eval_curv_at_uv);
    dgeom_register_cad_eval_xyz_at_uv (egads_eval_xyz_at_uv);
    dgeom_register_cad_eval_uv_bounds (egads_eval_uv_bounds);
    dgeom_register_cad_eval_bedge (surfgen_cad_eval_bedge);

    egen_auto_register_cad_eval_xyz_at_u (egads_eval_xyz_at_u);
    egen_auto_register_cad_eval_edge_uv (egads_eval_edge_uv);
    egen_auto_register_cad_eval_arclen (egads_eval_arclen);

    // Allocate AFLR4-EGADS data structure, initialize, and link body data.

    copy_bodies = (ego*)EG_alloc(numBody*sizeof(ego));
    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {
      status = EG_copyObject(bodies[bodyIndex], NULL, &copy_bodies[bodyIndex]);
      if (status != CAPS_SUCCESS) goto cleanup;
    }
    status = EG_getContext(bodies[0], &context);
    if (status != CAPS_SUCCESS) goto cleanup;
    status = EG_makeTopology(context, NULL, MODEL, 0, NULL, numBody, copy_bodies, NULL, &model);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = egads_set_ext_cad_data (model);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Complete all tasks required for AFLR4 surface grid generation.

    status = aflr4_setup_and_grid_gen (prog_argc, prog_argv);
    if (status != 0) {
      printf("**********************************************************\n");
      printf("AFLR4 mesh generation failed...\n");
      printf("An EGADS file with all AFLR4 parameters\n");
      printf("has been written to 'aflr4_debug.egads'\n");
      printf("**********************************************************\n");
      remove("aflr4_debug.egads");
      EG_saveModel(model, "aflr4_debug.egads");
      goto cleanup;
    }

//#define DUMP_TECPLOT_DEBUG_FILE
#ifdef DUMP_TECPLOT_DEBUG_FILE
    {
      int surf = 0;
      int numSurface = 0;
      int numTriFace = 0;
      int numNodes = 0;
      int numQuadFace = 0;
      int glueId; // Id of glued composite surface

      // AFRL4 output arrays
      INT_1D *bcFlag = NULL;
      INT_1D *blFlag = NULL;
      INT_3D *triCon = NULL;
      INT_4D *quadCon = NULL;
      DOUBLE_2D *uv = NULL;
      DOUBLE_3D *xyz = NULL;

      // Get output id index (glue-only composite)
      status = aflr4_get_idef_output (&glueId);

      FILE *fp = fopen("aflr4_debug.tec", "w");
      fprintf(fp, "VARIABLES = X, Y, Z, u, v\n");

      numSurface = dgeom_def_get_ndef(); // Get number of surfaces meshed

      for (surf = 0; surf < numSurface ; surf++) {

        if (surf+1 == glueId) continue;

        status = aflr4_get_def (surf+1,
                                0, // check normals
                                0, // If there are quads, don't get them.
                                &numTriFace,
                                &numNodes,
                                &numQuadFace,
                                &blFlag,
                                &bcFlag,
                                &triCon,
                                &quadCon,
                                &uv,
                                &xyz);
        if (status != CAPS_SUCCESS) goto cleanup;

        fprintf(fp, "ZONE T=\"def %d\" N=%d, E=%d, F=FEPOINT, ET=Triangle\n", surf+1, numNodes, numTriFace);
        for (int i = 0; i < numNodes; i++)
          fprintf(fp, "%22.15e %22.15e %22.15e %22.15e %22.15e\n", xyz[i+1][0], xyz[i+1][1], xyz[i+1][2], uv[i+1][0], uv[i+1][1]);
        for (int i = 0; i < numTriFace; i++)
          fprintf(fp, "%d %d %d\n", triCon[i+1][0], triCon[i+1][1], triCon[i+1][2]);


        ug_free (bcFlag); bcFlag = NULL;
        ug_free (blFlag); blFlag = NULL;
        ug_free (triCon); triCon = NULL;
        ug_free (quadCon); quadCon = NULL;
        ug_free (uv); uv = NULL;
        ug_free (xyz); xyz = NULL;

      }
      fclose(fp); fp = NULL;

    }
#endif

    status = egads_aflr4_get_tess (1, numBody, bodies, &tessBodies);
    if (status != EGADS_SUCCESS) goto cleanup;

    for (bodyIndex = 0; bodyIndex < numBody; bodyIndex++) {

        /* apply EGADS quading if requested */
        if (EGADS_Quad == (int)true) {
            if (quiet == (int)false )
                printf("Creating EGADS quads for Body %d\n", bodyIndex+1);
            tess = tessBodies[bodyIndex];
            status = EG_quadTess(tess, &tessBodies[bodyIndex]);
            if (status < EGADS_SUCCESS) {
                printf(" EG_quadTess = %d  -- reverting...\n", status);
                tessBodies[bodyIndex] = tess;
            }
        }

        // save off the tessellation object
        surfaceMeshes[bodyIndex].bodyTessMap.egadsTess = tessBodies[bodyIndex];

        status = EG_getBodyTopos(bodies[bodyIndex], NULL, FACE, &numFace, &faces);
        EG_free(faces); faces = NULL;
        if (status != EGADS_SUCCESS) goto cleanup;

        surfaceMeshes[bodyIndex].bodyTessMap.numTessFace = 0; // Number of faces in the tessellation
        //surfaceMeshes[bodyIndex].bodyTessMap.tessFaceQuadMap = tessFaceQuadMap[bodyIndex]; // Save off the quad map
        //tessFaceQuadMap[bodyIndex] = NULL;

        status = mesh_surfaceMeshEGADSTess(&attrMap,
                                           &surfaceMeshes[bodyIndex]);
        if (status != CAPS_SUCCESS) goto cleanup;

        status = aim_setTess(aimInfo, surfaceMeshes[bodyIndex].bodyTessMap.egadsTess);
        if (status != CAPS_SUCCESS) {
             printf(" aim_setTess return = %d\n", status);
             goto cleanup;
         }

        if (quiet == (int)false ) {
            printf("Body = %d\n", bodyIndex+1);
            printf("Number of nodes = %d\n", surfaceMeshes[bodyIndex].numNode);
            printf("Number of elements = %d\n", surfaceMeshes[bodyIndex].numElement);
            if (surfaceMeshes[bodyIndex].meshQuickRef.useStartIndex == (int) true ||
                surfaceMeshes[bodyIndex].meshQuickRef.useListIndex == (int) true) {
                printf("Number of tris = %d\n", surfaceMeshes[bodyIndex].meshQuickRef.numTriangle);
                printf("Number of quad = %d\n", surfaceMeshes[bodyIndex].meshQuickRef.numQuadrilateral);
            }
        }

        numNodeTotal += surfaceMeshes[bodyIndex].numNode;
        numElemTotal += surfaceMeshes[bodyIndex].numElement;
    }
    if (quiet == (int)false ) {
        printf("----------------------------\n");
        printf("Total number of nodes = %d\n", numNodeTotal);
        printf("Total number of elements = %d\n", numElemTotal);
    }
    status = CAPS_SUCCESS;

cleanup:
    // Free all aflr4 data
    aflr4_free_all (0);

    // Free DGEOM data from AFLR4 - note this deletes the data used by aflr4_get_def
    dgeom_def_free_all ();

    // Free program arguements
    ug_free_argv(prog_argv); prog_argv = NULL;

    EG_free(faces); faces = NULL;

    EG_free(meshInputString); meshInputString = NULL;

    EG_free(copy_bodies);

    // free memory from egads_aflr4_tess
    EG_free(tessBodies); tessBodies = NULL;

    return status;
}
