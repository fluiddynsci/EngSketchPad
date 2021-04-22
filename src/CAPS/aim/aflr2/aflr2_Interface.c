// AFLR2 interface functions - Modified from functions provided with
//	AFLR2_LIB source (aflr2_cad_geom_setup.c) written by David L. Marcum

//  Modified by Dr. Ryan Durscher AFRL/RQVC

#include "aflr2c/AFLR2_LIB.h" // Bring in AFLR2 API library
#include "ug_gq/UG_GQ_LIB_INC.h"

#include "egads.h"     // Bring in EGADS

#include "meshUtils.h" // Collection of helper functions for meshing
#include "cfdUtils.h"  // Collection of helper functions for cfd analysis
#include "miscUtils.h"

#ifdef WIN32
#define strtok_r   strtok_s
#endif

//TODO: xplt_Case_Name is needed for OSX at the moment to resolve a link error. Remove it as soon as you can...
char xplt_Case_Name[133];

//#define AFLR2_BOUNDARY_LAYER

static INT_ egads_eval_bedge (mapAttrToIndexStruct *attrMap,
                              INT_ idef, ego tess, int *ix, int *iy, INT_ *nbedge, INT_2D ** inibe,
                              INT_1D **Bnd_Edge_ID_Flag, DOUBLE_2D ** x)
{

  /*
   * Generate a boundary edge grid for a given surface (idef) with a CAD geometry
   * definition. The boundary edge connectivity and coordinate arrays do
   * not need to be allocated prior to calling this routine. They should be set to
   * NULL if they are not allocated. In either case they will be reallocated for
   * the generated boundary edge grid size.
   *
   * Modifications are required for alternative CAD systems.
   *
   * Input Parameters
   *
   *  idef  : identification label for surface definition
   *  inibe : boundary edge node connectivity pointer
   *          must at least be set to NULL it will be reallocated
   *  u     : u,v-coordinates pointer
   *          must at least be set to NULL it will be reallocated
   *  x     : x,y,z-coordinates pointer
   *          must at least be set to NULL it will be reallocated
   *
   * Output Parameters
   *
   *  nbedge : number of boundary edges in boundary edge grid
   *  inibe  : boundary edge node connectivity for each boundary edge of
   *              boundary edge grid
   *  x      : x,y,z-coordinates of boundary edge grid
   *
   * Return Value
   *
   *  0     : no errors
   *  > 0   : there were errors
   *
   */

    INT_         ierr;
    int          cnt, i, iloop, iedge, index, last, mtype, oclass,
                 n, nloop, nedge, *sen, *senses, status;
    int          attrIndex;

    double       range[2], uvbox[4];

    const double *xyzs = NULL, *ts = NULL;

    ego          geom, *loops = NULL, *edges = NULL, *nodes = NULL;

    int tessStatus, numPoints, numEdge, numFace;
    ego body, *bodyFaces = NULL, *bodyEdges = NULL;
    const char *groupName = NULL;

    // 2D mesh checks
    int xMeshConstant = (int) true, yMeshConstant = (int) true, zMeshConstant= (int) true;

    // Get body from tessellation
    status = EG_statusTessBody(tess, &body, &tessStatus, &numPoints);
    if (status != EGADS_SUCCESS) {
        status =  -status;
        goto cleanup;
    }

    // Get edges
    status = EG_getBodyTopos(body, NULL, EDGE, &numEdge, &bodyEdges);
    if (status != EGADS_SUCCESS) {
        status =  -status;
        goto cleanup;
    }

    // Get faces
    status = EG_getBodyTopos(body, NULL, FACE, &numFace, &bodyFaces);
    if (status != EGADS_SUCCESS) {
        status =  -status;
        goto cleanup;
    }

    status = EG_getTopology(bodyFaces[idef-1], &geom, &oclass, &mtype, uvbox,
                            &nloop, &loops, &senses);
    if (status != EGADS_SUCCESS) {
        status =  -status;
        goto cleanup;
    }

    for (cnt = iloop = 0; iloop < nloop; iloop++) {
        status = EG_getTopology(loops[iloop], &geom, &oclass, &mtype,
                                NULL, &nedge, &edges, &senses);
        if (status != EGADS_SUCCESS) {
            status =  -status;
            goto cleanup;
        }

        for (iedge = 0; iedge < nedge; iedge++) {
            status = EG_getTopology(edges[iedge], &geom, &oclass,
                                    &mtype, range, &n, &nodes, &sen);
            if (status != EGADS_SUCCESS) {
                status =  -status;
                goto cleanup;
            }

            if (mtype == DEGENERATE) continue;

            index = EG_indexBodyTopo(body, edges[iedge]);
            if (index < EGADS_SUCCESS) {
                status = -index;
                goto cleanup;
            }

            status = EG_getTessEdge(tess, index, &n, &xyzs, &ts);
            if (status != EGADS_SUCCESS) {
                status =  -status;
                goto cleanup;
            }

            // Constant x?
            for (i = 0; i < n; i++) {
                if (fabs(xyzs[3*i + 0] - xyzs[3*0 + 0]) > 1E-7) {
                    xMeshConstant = (int) false;
                    break;
                }
            }

            // Constant y?
            for (i = 0; i < n; i++) {
                if (fabs(xyzs[3*i + 1] - xyzs[3*0 + 1] ) > 1E-7) {
                    yMeshConstant = (int) false;
                    break;
                }
            }

            // Constant z?
            for (i = 0; i < n; i++) {
                if (fabs(xyzs[3*i + 2]-xyzs[3*0 + 2]) > 1E-7) {
                    zMeshConstant = (int) false;
                    break;
                }
            }

            cnt += n-1;
        }
    }

    *ix = 0;
    *iy = 1;

    if (zMeshConstant != (int) true) {
        printf("\tAFLR expects 2D meshes be in the x-y plane... attempting to rotate mesh through node swapping!\n");

        if (xMeshConstant == (int) true && yMeshConstant == (int) false) {

            printf("\tSwapping z and x coordinates!\n");
            *ix = 2;
            *iy = 1;

        } else if(xMeshConstant == (int) false && yMeshConstant == (int) true) {

            printf("\tSwapping z and y coordinates!\n");
            *ix = 0;
            *iy = 2;

        } else {

            printf("\tUnable to rotate mesh!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }
    }

    ierr = 0;

    *nbedge = cnt;

    *inibe            = (INT_2D *)    ug_malloc (&ierr, ((*nbedge)+1) * sizeof (INT_2D));
    *Bnd_Edge_ID_Flag = (INT_1D *)    ug_malloc (&ierr, ((*nbedge)+1) * sizeof (INT_1D));
    *x                = (DOUBLE_2D *) ug_malloc (&ierr, ((*nbedge)+1) * sizeof (DOUBLE_2D));

    if (ierr) {
        ug_error_message ("*** ERROR 104111 : unable to allocate required memory ***");
        return 104111;
    }

    for (cnt = iloop = 0; iloop < nloop; iloop++) {
        status = EG_getTopology(loops[iloop], &geom, &oclass, &mtype,
                                NULL, &nedge, &edges, &senses);
        if (status != EGADS_SUCCESS) {
            status =  -status;
            goto cleanup;
        }


        last = cnt;
        for (iedge = 0; iedge < nedge; iedge++) {
            status = EG_getTopology(edges[iedge], &geom, &oclass,
                                    &mtype, range, &n, &nodes, &sen);
            if (status != EGADS_SUCCESS) {
                status =  -status;
                goto cleanup;
            }

            if (mtype == DEGENERATE) continue;

            index = EG_indexBodyTopo(body, edges[iedge]);
            if (index < EGADS_SUCCESS) {
                status = -index;
                goto cleanup;
            }

            status = retrieve_CAPSGroupAttr(bodyEdges[index-1], &groupName);
            if (status == EGADS_SUCCESS) {
                status = get_mapAttrToIndexIndex(attrMap, groupName, &attrIndex);
                if (status != CAPS_SUCCESS) {
                    printf("\tNo capsGroup \"%s\" not found in attribute map\n", groupName);
                    goto cleanup;
                }
            } else if (status == EGADS_NOTFOUND) {
                printf("\tError: No capsGroup found on edge %d\n", index);
                printf("Available attributes are:\n");
                print_AllAttr( bodyEdges[index-1] );
                goto cleanup;
            } else goto cleanup;


            status = EG_getTessEdge(tess, index, &n, &xyzs, &ts);
            if (status != EGADS_SUCCESS) {
                status =  -status;
                goto cleanup;
            }

            for (i = 0; i < n-1; i++, cnt++) {

                (*Bnd_Edge_ID_Flag)[cnt+1] = attrIndex;

                if (senses[iedge] == 1) {
                    (*x)[cnt+1][0] = xyzs[3*i+*ix];
                    (*x)[cnt+1][1] = xyzs[3*i+*iy];
                } else {
                    (*x)[cnt+1][0] = xyzs[3*(n-i-1)+*ix];
                    (*x)[cnt+1][1] = xyzs[3*(n-i-1)+*iy];
                }

                (*inibe)[cnt+1][0] = cnt+1;
                (*inibe)[cnt+1][1] = cnt+2;
            }
        }
        (*inibe)[cnt][1] = last+1;
    }

    status = EGADS_SUCCESS;

    cleanup:

        //if (ts != NULL) EG_free(ts);
        ts = NULL;
        //if (xyzs != NULL) EG_free(xyzs);
        xyzs = NULL;
        // if (nodes != NULL) EG_free(nodes);
        nodes = NULL;
        // if (edges != NULL) EG_free(edges);
        edges = NULL;
        // if (loops != NULL) EG_free(loops);
        loops = NULL;

        EG_free(bodyEdges);
        EG_free(bodyFaces);

        //printf("egads_eval_bedge- cleanup complete\n");
        return status;
}


int aflr2_Surface_Mesh(int Message_Flag, ego bodyIn,
                       meshInputStruct meshInput,
                       mapAttrToIndexStruct attrMap,
                       int numMeshProp,
                       meshSizingStruct *meshProp,
                       meshStruct *surfaceMesh)
{

    int status; // Function return status

    int i, elementIndex; // Indexing
    int faceAttr = 0;
    int numFace;

    double box[6], size, params[3]; // Bounding box variables

    ego tess = NULL, *bodyFaces = NULL;

#ifdef AFLR2_BOUNDARY_LAYER
    // Boundary Layer meshing related variables
    int propIndex, attrIndex = 0; // Indexing
    double capsMeshLength = 0;
    int createBL = (int) false; // Indicate if boundary layer meshes should be generated
#endif

    // AFRL2 arrays

    INT_1D *Bnd_Edge_Grid_BC_Flag = NULL;
    INT_1D *Bnd_Edge_ID_Flag = NULL;
    INT_1D *Bnd_Edge_Error_Flag  =  NULL;
    INT_2D *Bnd_Edge_Connectivity = NULL;
    INT_3D *Tria_Connectivity = NULL;
    INT_4D *Quad_Connectivity = NULL;

    DOUBLE_2D *Coordinates = NULL;
    DOUBLE_1D *Initial_Normal_Spacing = NULL;

    INT_1D *BG_Bnd_Edge_Grid_BC_Flag = NULL;
    INT_1D *BG_Bnd_Edge_ID_Flag = NULL;
    INT_2D *BG_Bnd_Edge_Connectivity = NULL;
    INT_3D *BG_Tria_Neighbors = NULL;
    INT_3D *BG_Tria_Connectivity = NULL;

    DOUBLE_1D *BG_Spacing = NULL;
    DOUBLE_2D *BG_Coordinates = NULL;
    DOUBLE_3D *BG_Metric = NULL;

    DOUBLE_1D *Source_Spacing = NULL;
    DOUBLE_2D *Source_Coordinates = NULL;
    DOUBLE_3D *Source_Metric = NULL;

    INT_ Number_of_Bnd_Edges = 0;
    INT_ Number_of_Nodes = 0;
    INT_ Number_of_Quads = 0;
    INT_ Number_of_Trias = 0;

    INT_ Number_of_BG_Bnd_Edges = 0;
    INT_ Number_of_BG_Nodes = 0;
    INT_ Number_of_BG_Trias = 0;

    INT_ Number_of_Source_Nodes = 0;

    int ix = 0, iy = 1;

     // Commandline inputs
    int  prog_argc   = 1;    // Number of arguments
    char **prog_argv = NULL; // String arrays
    char *meshInputString = NULL;
    char *rest = NULL, *token = NULL;

    // Get bounding box for scaling
    status = EG_getBoundingBox(bodyIn, box);
    if (status != EGADS_SUCCESS) goto cleanup;

    size = sqrt((box[0]-box[3])*(box[0]-box[3]) +
                (box[1]-box[4])*(box[1]-box[4]) +
                (box[2]-box[5])*(box[2]-box[5]));

    // Negating the first parameter triggers EGADS to only put vertexes on edges
    params[0] = -meshInput.paramTess[0]*size;
    params[1] =  meshInput.paramTess[1]*size;
    params[2] =  meshInput.paramTess[2];

    // Get Tessellation  - save to global variable
    status = EG_makeTessBody(bodyIn, params, &tess);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Get bodyFaces
    status = EG_getBodyTopos(bodyIn, NULL, FACE, &numFace, &bodyFaces);
    if (status != EGADS_SUCCESS) goto cleanup;

    if (numFace != 1) {
        printf("\tBody should only have 1 face!!\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // extract the boundary tessellation from the egads body
    status = egads_eval_bedge (&attrMap, 1, tess, &ix, &iy, &Number_of_Bnd_Edges, &Bnd_Edge_Connectivity, &Bnd_Edge_ID_Flag, &Coordinates);
    if (status != CAPS_SUCCESS) goto cleanup; // Add error code

    Number_of_Nodes = Number_of_Bnd_Edges;

//#define DUMP_TECPLOT_DEBUG_FILE
#ifdef DUMP_TECPLOT_DEBUG_FILE
    {
        FILE *fp = fopen("aflr2_debug.dat", "w");
        fprintf(fp, "VARIABLES = X, Y, ID\n");

        fprintf(fp, "ZONE N=%d, E=%d, F=FEPOINT, ET=LINESEG\n", Number_of_Nodes, Number_of_Bnd_Edges);
        for (int i = 0; i < Number_of_Nodes; i++)
            fprintf(fp, "%22.15e %22.15e %d\n", Coordinates[i+1][0], Coordinates[i+1][1], Bnd_Edge_ID_Flag[i+1]);
        for (int i = 0; i < Number_of_Bnd_Edges; i++)
            fprintf(fp, "%d %d\n", Bnd_Edge_Connectivity[i+1][0], Bnd_Edge_Connectivity[i+1][1]);

        fclose(fp); fp = NULL;
    }
#endif


#ifdef AFLR2_BOUNDARY_LAYER
    // Loop through meshing properties and see if boundaryLayerThickness and boundaryLayerSpacing have been specified
    for (propIndex = 0; propIndex < numMeshProp && createBL == (int) false; propIndex++) {

      // If no boundary layer specified in meshProp continue
      if (meshProp[propIndex].boundaryLayerSpacing == 0) continue;

      // Loop through Elements and see if marker match
      for (i = 1; i <= Number_of_Bnd_Edges && createBL == (int) false; i++) {

        //If they don't match continue
        if (Bnd_Edge_ID_Flag[i] != meshProp[propIndex].attrIndex) continue;

        // Set face "bl" flag
        createBL = (int) true;
      }
    }

    // Get the capsMeshLenght if boundary layer meshing has been requested
    if (createBL == (int) true) {
        status = check_CAPSMeshLength(1, &bodyIn, &capsMeshLength);

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

    // Allocate Initial_Normal_Spacing
    Initial_Normal_Spacing = (DOUBLE_1D *) ug_malloc (&status, (Number_of_Nodes+1) * sizeof (DOUBLE_1D));
    if (Initial_Normal_Spacing == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    // Set default to none
    for (i = 1; i <= Number_of_Nodes; i++) {
        Initial_Normal_Spacing[i] = 0.0;
    }

    // Loop through meshing properties and see if boundaryLayerThickness and boundaryLayerSpacing have been specified
    for (propIndex = 0; propIndex < numMeshProp; propIndex++) {

        // If no boundary layer specified in meshProp continue
        if (meshProp[propIndex].boundaryLayerSpacing == 0) continue;

        // Loop through Elements and see if marker match
        for (i = 1; i <= Number_of_Bnd_Edges; i++) {

            //If they don't match continue
            if (Bnd_Edge_ID_Flag[i] != meshProp[propIndex].attrIndex) continue;

            // Set boundary layer spacing
            Initial_Normal_Spacing[Bnd_Edge_Connectivity[i][0]] = meshProp[propIndex].boundaryLayerSpacing*capsMeshLength;
            Initial_Normal_Spacing[Bnd_Edge_Connectivity[i][1]] = meshProp[propIndex].boundaryLayerSpacing*capsMeshLength;
        }
    }
#endif //AFLR2_BOUNDARY_LAYER


    // Initialize and setup AFLR2 input parameter structure

    ug_set_prog_param_n_dim (2);

    ug_set_prog_param_function1 (ug_initialize_aflr_param);
    ug_set_prog_param_function1 (ug_gq_initialize_param); // optional
    ug_set_prog_param_function2 (aflr2_initialize_param);
    ug_set_prog_param_function2 (ice2_initialize_param);

    status = ug_add_new_arg (&prog_argv, (char*)"allocate_and_initialize_argv");
    if (status != CAPS_SUCCESS) goto cleanup;

    // Parse input string
    if (meshInput.aflr4Input.meshInputString != NULL) {

        rest = meshInputString = EG_strdup(meshInput.aflr4Input.meshInputString);
        while ((token = strtok_r(rest, " ", &rest))) {
            status = ug_add_flag_arg (token, &prog_argc, &prog_argv);
            if (status != CAPS_SUCCESS) {
                printf("Error: Failed to parse input string: %s\n", token);
                printf("Complete input string: %s\n", meshInputString);
                goto cleanup;
            }
        }

        /* printf("Number of args = %d\n",prog_argc);
        for (i = 0; i <prog_argc ; i++) printf("Arg %d = %s\n", i, prog_argv[i]);
         */
    } else {
        // Set other command options
      //if (createBL == (int) true) {
      //    status = ug_add_flag_arg ((char*)"bl", &prog_argc, &prog_argv);
      //}
      if (status != 0) goto cleanup;
    }

    // set the last argument to 1 to print what arguments have been set
    status = ug_check_prog_param (prog_argv, prog_argc, Message_Flag);
    if (status != CAPS_SUCCESS) goto cleanup; // Add error code

    status = aflr2_grid_generator (prog_argc, prog_argv,
                                   Message_Flag,
                                   &Number_of_Bnd_Edges, &Number_of_Nodes,
                                   &Number_of_Quads, &Number_of_Trias,
                                   &Number_of_BG_Bnd_Edges,
                                   &Number_of_BG_Nodes, &Number_of_BG_Trias,
                                   &Number_of_Source_Nodes,
                                   &Bnd_Edge_Connectivity,
                                   &Bnd_Edge_Error_Flag,
                                   &Bnd_Edge_Grid_BC_Flag,
                                   &Bnd_Edge_ID_Flag,
                                   &Quad_Connectivity, &Tria_Connectivity,
                                   &BG_Bnd_Edge_Connectivity,
                                   &BG_Bnd_Edge_Grid_BC_Flag,
                                   &BG_Bnd_Edge_ID_Flag,
                                   &BG_Tria_Neighbors,
                                   &BG_Tria_Connectivity,
                                   &Coordinates,
                                   Initial_Normal_Spacing,
                                   &BG_Coordinates,
                                   &BG_Spacing,
                                   &BG_Metric,
                                   &Source_Coordinates,
                                   &Source_Spacing,
                                   &Source_Metric);
    if (status != CAPS_SUCCESS) goto cleanup; // Add error code

    surfaceMesh->meshType = Surface2DMesh;
    surfaceMesh->meshQuickRef.numTriangle      = Number_of_Trias;
    surfaceMesh->meshQuickRef.numQuadrilateral = Number_of_Quads;
    surfaceMesh->meshQuickRef.numLine          = Number_of_Bnd_Edges;

    surfaceMesh->meshQuickRef.startIndexTriangle      = 0;
    surfaceMesh->meshQuickRef.startIndexQuadrilateral = Number_of_Trias;
    surfaceMesh->meshQuickRef.startIndexLine          = Number_of_Trias + Number_of_Quads;

    surfaceMesh->meshQuickRef.useStartIndex = (int) true;

    // Nodes
    surfaceMesh->numNode = Number_of_Nodes;

    surfaceMesh->node = (meshNodeStruct *) EG_alloc(surfaceMesh->numNode*sizeof(meshNodeStruct));

    // Initiate nodes
    for (i = 0; i < surfaceMesh->numNode; i++) {
        status = initiate_meshNodeStruct(&surfaceMesh->node[i], UnknownMeshAnalysis);
        if (status != CAPS_SUCCESS) goto cleanup;

        surfaceMesh->node[i].xyz[ix] = Coordinates[i+1][0];
        surfaceMesh->node[i].xyz[iy] = Coordinates[i+1][1];
    }

    // Allocate and initialize all elements
    surfaceMesh->numElement = Number_of_Trias + Number_of_Quads + Number_of_Bnd_Edges;
    surfaceMesh->element = (meshElementStruct *) EG_alloc(surfaceMesh->numElement*sizeof(meshElementStruct));

    // Initiate all elements
    for (i = 0; i < surfaceMesh->numElement; i++) {
        status = initiate_meshElementStruct(surfaceMesh->element + i, UnknownMeshAnalysis);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Initiate triangle elements
    for (i = 0; i < Number_of_Trias; i++) {
        elementIndex = i;

        surfaceMesh->element[elementIndex].elementType = Triangle;

        status = mesh_allocMeshElementConnectivity(&surfaceMesh->element[elementIndex]);
        if (status != CAPS_SUCCESS) goto cleanup;

        surfaceMesh->element[elementIndex].connectivity[0] = Tria_Connectivity[i+1][0];
        surfaceMesh->element[elementIndex].connectivity[1] = Tria_Connectivity[i+1][1];
        surfaceMesh->element[elementIndex].connectivity[2] = Tria_Connectivity[i+1][2];

        surfaceMesh->element[elementIndex].elementID = elementIndex+1;
        surfaceMesh->element[elementIndex].markerID = faceAttr;
    }

    // Initiate quad elements
    for (i = 0; i < Number_of_Quads; i++) {

        elementIndex = Number_of_Trias + i;

        surfaceMesh->element[elementIndex].elementType = Quadrilateral;

        status = mesh_allocMeshElementConnectivity(&surfaceMesh->element[elementIndex]);
        if (status != CAPS_SUCCESS) goto cleanup;

        surfaceMesh->element[elementIndex].connectivity[0] = Quad_Connectivity[i+1][0];
        surfaceMesh->element[elementIndex].connectivity[1] = Quad_Connectivity[i+1][1];
        surfaceMesh->element[elementIndex].connectivity[2] = Quad_Connectivity[i+1][2];
        surfaceMesh->element[elementIndex].connectivity[3] = Quad_Connectivity[i+1][3];

        surfaceMesh->element[elementIndex].elementID = elementIndex+1;
        surfaceMesh->element[elementIndex].markerID = faceAttr;
    }

    for (i = 0; i < Number_of_Bnd_Edges; i++) {

        elementIndex = Number_of_Trias + Number_of_Quads + i;

        surfaceMesh->element[elementIndex].elementType = Line;

        status = mesh_allocMeshElementConnectivity(&surfaceMesh->element[elementIndex]);
        if (status != CAPS_SUCCESS) goto cleanup;

        surfaceMesh->element[elementIndex].connectivity[0] = Bnd_Edge_Connectivity[i+1][0];
        surfaceMesh->element[elementIndex].connectivity[1] = Bnd_Edge_Connectivity[i+1][1];

        surfaceMesh->element[elementIndex].elementID = elementIndex+1;
        surfaceMesh->element[elementIndex].markerID = Bnd_Edge_ID_Flag[i+1];
    }

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        (void) EG_deleteObject(tess);

        EG_free(meshInputString);
        EG_free(bodyFaces);

        //Free the arguments
        ug_free_argv (prog_argv);

        ug_free(Bnd_Edge_Grid_BC_Flag);
        ug_free(Bnd_Edge_ID_Flag);
        ug_free(Bnd_Edge_Error_Flag);
        ug_free(Bnd_Edge_Connectivity);
        ug_free(Tria_Connectivity);
        ug_free(Quad_Connectivity);

        ug_free(Coordinates);
        ug_free(Initial_Normal_Spacing);

        ug_free(BG_Bnd_Edge_Grid_BC_Flag);
        ug_free(BG_Bnd_Edge_ID_Flag);
        ug_free(BG_Bnd_Edge_Connectivity);
        ug_free(BG_Tria_Neighbors);
        ug_free(BG_Tria_Connectivity);

        ug_free(BG_Spacing);
        ug_free(BG_Coordinates);
        ug_free(BG_Metric);

        ug_free(Source_Spacing);
        ug_free(Source_Coordinates);
        ug_free(Source_Metric);

        return status;
}
