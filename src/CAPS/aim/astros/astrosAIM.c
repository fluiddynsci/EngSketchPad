/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             ASTROS AIM
 *
 *     Written by Dr. Ryan Durscher and Dr. Ed Alyanak AFRL/RQVC
 *
 */

/*! \mainpage Introduction
 *
 * \section overviewAstros Astros AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact (primarily
 * through input files) with the finite element structural solver ASTROS.
 *
 * Current issues include:
 *  - A thorough bug testing needs to be undertaken.
 *
 * An outline of the AIM's inputs, outputs and attributes are provided in \ref aimInputsAstros and
 * \ref aimOutputsAstros and \ref attributeAstros, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentAstros.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataAstros if
 * connecting this AIM to other AIMs in a parent-child like manner.
 *
 * Details of the AIM's automated data transfer capabilities are outlined in \ref dataTransferAstros
 *
 */


/*! \page attributeAstros Astros AIM attributes
 * The following list of attributes are required for the Astros AIM inside the geometry input.
 *
 * - <b> capsDiscipline</b> This attribute is a requirement if doing aeroelastic analysis within Nastran. capsDiscipline allows
 * the AIM to determine which bodies are meant for structural analysis and which are used for aerodynamics. Options
 * are:  Structure and Aerodynamic (case insensitive).
 *
 * - <b> capsGroup</b> This is a name assigned to any geometric body.  This body could be a solid, surface, face, wire, edge or node.
 * Recall that a string in ESP starts with a $.  For example, attribute <c>capsGroup $Wing</c>.
 *
 * - <b> capsLoad</b> This is a name assigned to any geometric body where a load is applied.  This attribute was separated from the <c>capsGroup</c>
 * attribute to allow the user to define a local area to apply a load on without adding multiple <c>capsGroup</c> attributes.
 * Recall that a string in ESP starts with a $.  For example, attribute <c>capsLoad $force</c>.
 *
 * - <b> capsConstraint</b> This is a name assigned to any geometric body where a constraint/boundary condition is applied.
 * This attribute was separated from the <c>capsGroup</c> attribute to allow the user to define a local area to apply a boundary condition
 * without adding multiple <c>capsGroup</c> attributes. Recall that a string in ESP starts with a $.  For example, attribute <c>capsConstraint $fixed</c>.
 *
 * - <b> capsIgnore</b> It is possible that there is a geometric body (or entity) that you do not want the Astros AIM to pay attention to when creating
 * a finite element model. The capsIgnore attribute allows a body (or entity) to be in the geometry and ignored by the AIM.  For example,
 * because of limitations in OpenCASCADE a situation where two edges are overlapping may occur; capsIgnore allows the user to only
 *  pay attention to one of the overlapping edges.
 *
 * - <b> capsConnect</b> This is a name assigned to any geometric body where the user wishes to create
 * "fictitious" connections such as springs, dampers, and/or rigid body connections to. The user must manually specify
 * the connection between two <c>capsConnect</c> entities using the "Connect" tuple (see \ref aimInputsAstros).
 * Recall that a string in ESP starts with a $.  For example, attribute <c>capsConnect $springStart</c>.
 *
 * - <b> capsConnectLink</b> Similar to <c>capsConnect</c>, this is a name assigned to any geometric body where
 * the user wishes to create "fictitious" connections to. A connection is automatically made if a <c>capsConnectLink</c>
 * matches a <c>capsConnect</c> group. Again, further specifics of the connection are input using the "Connect"
 * tuple (see \ref aimInputsAstros). Recall that a string in ESP starts with a $.
 * For example, attribute <c>capsConnectLink $springEnd</c>.
 *
 * - <b> capsBound </b> This is used to mark surfaces on the structural grid in which data transfer with an external
 * solver will take place. See \ref dataTransferAstros for additional details.
 *
 * Internal Aeroelastic Analysis
 *
 * - <b> capsBound </b> This is used to mark surfaces on the structural grid in which a spline will be created between
 * the structural and aero-loads.
 *
 * - <b> capsReferenceArea</b>  [Optional: Default 1.0] Reference area to use when doing aeroelastic analysis.
 * This attribute may exist on any aerodynamic cross-section.
 *
 * - <b> capsReferenceChord</b>  [Optional: Default 1.0] Reference chord to use when doing aeroelastic analysis.
 * This attribute may exist on any aerodynamic cross-section.
 *
 * - <b> capsReferenceSpan</b>  [Optional: Default 1.0] Reference span to use when doing aeroelastic analysis.
 * This attribute may exist on any aerodynamic cross-section.
 *
 */

#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"    // Meshing utilities
#include "miscUtils.h"    // Miscellaneous utilities
#include "vlmUtils.h"     // Vortex lattice method utilities
#include "vlmSpanSpace.h" // Auto spanwise distribution for vortex lattice
#include "feaUtils.h"     // FEA utilities
#include "nastranUtils.h" // Nastran utilities
#include "astrosUtils.h"  // Astros utilities

#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define NUMINPUT   21
#define NUMOUTPUT  9

#define MXCHAR  255

//#define DEBUG

typedef struct {

    // Project name
    const char *projectName; // Project name

    // Analysis file path/directory
    const char *analysisPath;

    feaProblemStruct feaProblem;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

    // Attribute to constraint index map
    mapAttrToIndexStruct constraintMap;

    // Attribute to load index map
    mapAttrToIndexStruct loadMap;

    // Attribute to transfer map
    mapAttrToIndexStruct transferMap;

    // Attribute to connect map
    mapAttrToIndexStruct connectMap;

    // Mesh holders
    int numMesh;
    meshStruct *feaMesh;

    // Number of geometric inputs
    int numGeomIn;

    // Pointer to CAPS geometric in values
    capsValue *geomInVal;

} aimStorage;

static aimStorage *astrosInstance = NULL;
static int         numInstance  = 0;


static int initiate_aimStorage(int iIndex) {

    int status;

    // Set initial values for astrosInstance
    astrosInstance[iIndex].projectName = NULL;

    // Analysis file path/directory
    astrosInstance[iIndex].analysisPath = NULL;

    /*
    // Check to make sure data transfer is ok
    astrosInstance[iIndex].dataTransferCheck = (int) true;
     */

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&astrosInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to constraint index map
    status = initiate_mapAttrToIndexStruct(&astrosInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to load index map
    status = initiate_mapAttrToIndexStruct(&astrosInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for transfer to index map
    status = initiate_mapAttrToIndexStruct(&astrosInstance[iIndex].transferMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for connect to index map
    status = initiate_mapAttrToIndexStruct(&astrosInstance[iIndex].connectMap);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_feaProblemStruct(&astrosInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS) return status;

    // Mesh holders
    astrosInstance[iIndex].numMesh = 0;
    astrosInstance[iIndex].feaMesh = NULL;

    // Number of geometric inputs
    astrosInstance[iIndex].numGeomIn = 0;

    // Pointer to CAPS geometric in values
    astrosInstance[iIndex].geomInVal = NULL;

    return CAPS_SUCCESS;
}

static int destroy_aimStorage(int iIndex) {

    int status;
    int i;
    // Attribute to index map
    status = destroy_mapAttrToIndexStruct(&astrosInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to constraint index map
    status = destroy_mapAttrToIndexStruct(&astrosInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to load index map
    status = destroy_mapAttrToIndexStruct(&astrosInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Transfer to index map
    status = destroy_mapAttrToIndexStruct(&astrosInstance[iIndex].transferMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Connect to index map
    status = destroy_mapAttrToIndexStruct(&astrosInstance[iIndex].connectMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Cleanup meshes
    if (astrosInstance[iIndex].feaMesh != NULL) {

        for (i = 0; i < astrosInstance[iIndex].numMesh; i++) {
            status = destroy_meshStruct(&astrosInstance[iIndex].feaMesh[i]);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_meshStruct!\n", status);
        }

        EG_free(astrosInstance[iIndex].feaMesh);
    }

    astrosInstance[iIndex].feaMesh = NULL;
    astrosInstance[iIndex].numMesh = 0;

    // Destroy FEA problem structure
    status = destroy_feaProblemStruct(&astrosInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS)  printf("Error: Status %d during destroy_feaProblemStruct!\n", status);

    // NULL projetName
    astrosInstance[iIndex].projectName = NULL;

    // NULL analysisPath
    astrosInstance[iIndex].analysisPath = NULL;

    // Number of geometric inputs
    astrosInstance[iIndex].numGeomIn = 0;

    // Pointer to CAPS geometric in values
    astrosInstance[iIndex].geomInVal = NULL;

    return CAPS_SUCCESS;
}

static int checkAndCreateMesh(int iIndex, void* aimInfo)
{
  // Function return flag
  int status;

  int  numBody;
  ego *bodies = NULL;
  const char   *intents;

  int i, needMesh = (int) true;

  // Meshing related variables
  double  tessParam[3];
  int edgePointMin = 2;
  int edgePointMax = 50;
  int quadMesh = (int) false;

  // analysis input values
  capsValue *Tess_Params = NULL;
  capsValue *Edge_Point_Min = NULL;
  capsValue *Edge_Point_Max = NULL;
  capsValue *Quad_Mesh = NULL;

  // Get AIM bodies
  status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
  if (status != CAPS_SUCCESS) printf("aim_getBodies status = %d!!\n", status);

  // Don't generate if any tess object is not null
  for (i = 0; i < numBody; i++) {
      needMesh = (int) (needMesh && (bodies[numBody+i] == NULL));
  }

  // the mesh has already been generated
  if (needMesh == (int)false) return CAPS_SUCCESS;


  // retrieve or create the mesh from fea_createMesh
  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN), ANALYSISIN, &Tess_Params);
  if (status != CAPS_SUCCESS) return status;

  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Edge_Point_Min", ANALYSISIN), ANALYSISIN, &Edge_Point_Min);
  if (status != CAPS_SUCCESS) return status;

  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Edge_Point_Max", ANALYSISIN), ANALYSISIN, &Edge_Point_Max);
  if (status != CAPS_SUCCESS) return status;

  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Quad_Mesh", ANALYSISIN), ANALYSISIN, &Quad_Mesh);
  if (status != CAPS_SUCCESS) return status;

  // Get FEA mesh if we don't already have one
  tessParam[0] = Tess_Params->vals.reals[0]; // Gets multiplied by bounding box size
  tessParam[1] = Tess_Params->vals.reals[1]; // Gets multiplied by bounding box size
  tessParam[2] = Tess_Params->vals.reals[2];

  // Max and min number of points
  if (Edge_Point_Min->nullVal != IsNull) {
      edgePointMin = Edge_Point_Min->vals.integer;
      if (edgePointMin < 2) {
        printf("**********************************************************\n");
        printf("Edge_Point_Min = %d must be greater or equal to 2\n", edgePointMin);
        printf("**********************************************************\n");
        return CAPS_BADVALUE;
      }
  }

  if (Edge_Point_Max->nullVal != IsNull) {
      edgePointMax = Edge_Point_Max->vals.integer;
      if (edgePointMax < 2) {
        printf("**********************************************************\n");
        printf("Edge_Point_Max = %d must be greater or equal to 2\n", edgePointMax);
        printf("**********************************************************\n");
        return CAPS_BADVALUE;
      }
  }

  if (edgePointMin >= 2 && edgePointMax >= 2 && edgePointMin > edgePointMax) {
    printf("**********************************************************\n");
    printf("Edge_Point_Max must be greater or equal Edge_Point_Min\n");
    printf("Edge_Point_Max = %d, Edge_Point_Min = %d\n",edgePointMax,edgePointMin);
    printf("**********************************************************\n");
    return CAPS_BADVALUE;
  }

  quadMesh     = Quad_Mesh->vals.integer;

  status = fea_createMesh(aimInfo,
                          tessParam,
                          edgePointMin,
                          edgePointMax,
                          quadMesh,
                          &astrosInstance[iIndex].attrMap,
                          &astrosInstance[iIndex].constraintMap,
                          &astrosInstance[iIndex].loadMap,
                          &astrosInstance[iIndex].transferMap,
                          &astrosInstance[iIndex].connectMap,
                          &astrosInstance[iIndex].numMesh,
                          &astrosInstance[iIndex].feaMesh,
                          &astrosInstance[iIndex].feaProblem );
  if (status != CAPS_SUCCESS) return status;

  return CAPS_SUCCESS;
}

static int createVLMMesh(int iIndex, void *aimInfo, capsValue *aimInputs) {

    int projectionMethod = (int) true;

    int status, status2; // Function return status

    int i, j, k, surfaceIndex = 0, sectionIndex, transferIndex; // Indexing
    int skip;

    int stringLength;
    char *analysisType = NULL;

    // Bodies
    const char *intents;
    int   numBody; // Number of Bodies
    ego  *bodies;

    // Aeroelastic information
    int numVLMSurface = 0;
    vlmSurfaceStruct *vlmSurface = NULL;
    int numSpanWise = 0;

    vlmSectionStruct *tempSection = NULL;

    feaAeroStruct *feaAeroTemp = NULL, *feaAeroTempCombine=NULL;

    // Vector variables
    double A[3], B[3], C[3], D[3], P[3], p[3], N[3], n[3], d_proj[3];

    double *a, *b, *c, *d;
    double apbArea, apcArea, cpdArea, bpdArea, Area;

    feaMeshDataStruct *feaData;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

#ifdef DEBUG
    printf(" astrosAIM/createVLMMesh instance = %d  nbody = %d!\n", iIndex, numBody);
#endif

    if ((numBody <= 0) || (bodies == NULL)) {
#ifdef DEBUG
        printf(" astrosAIM/createVLMMesh No Bodies!\n");
#endif

        return CAPS_SOURCEERR;
    }

    // Analysis type
    analysisType = aimInputs[aim_getIndex(aimInfo, "Analysis_Type", ANALYSISIN)-1].vals.string;

    // Get aerodynamic reference quantities
    status = fea_retrieveAeroRef(numBody, bodies, &astrosInstance[iIndex].feaProblem.feaAeroRef);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Cleanup Aero storage first
    if (astrosInstance[iIndex].feaProblem.feaAero != NULL) {

        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++) {
            status = destroy_feaAeroStruct(&astrosInstance[iIndex].feaProblem.feaAero[i]);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        EG_free(astrosInstance[iIndex].feaProblem.feaAero);
    }

    astrosInstance[iIndex].feaProblem.numAero = 0;

    // Get AVL surface information
    if (aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].nullVal != IsNull) {

        status = get_vlmSurface(aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].vals.tuple,
                                &astrosInstance[iIndex].attrMap,
                                0.0, // default Cspace
                                &numVLMSurface,
                                &vlmSurface);
        if (status != CAPS_SUCCESS) goto cleanup;

    } else {
        printf("An analysis type of Aeroelastic set but no VLM_Surface tuple specified\n");
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    printf("\nGetting FEA vortex lattice mesh\n");

    status = vlm_getSections(numBody,
                             bodies,
                             "Aerodynamic",
                             astrosInstance[iIndex].attrMap,
                             vlmPLANEYZ,
                             numVLMSurface,
                             &vlmSurface);
    if (status != CAPS_SUCCESS) goto cleanup;

    for (i = 0; i < numVLMSurface; i++) {

        // Compute auto spacing
        if (vlmSurface[i].NspanTotal > 0)
            numSpanWise = vlmSurface[i].NspanTotal;
        else if (vlmSurface[i].NspanSection > 0)
            numSpanWise = (vlmSurface[i].numSection-1)*vlmSurface[i].NspanSection;
        else {
            printf("Error: Only one of numSpanTotal and numSpanPerSection can be non-zero!\n");
            printf("       numSpanTotal      = %d\n", vlmSurface[i].NspanTotal);
            printf("       numSpanPerSection = %d\n", vlmSurface[i].NspanSection);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        status = vlm_equalSpaceSpanPanels(numSpanWise, vlmSurface[i].numSection, vlmSurface[i].vlmSection);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Split the surfaces that have more than 2 sections into a new surface
    for (i = 0; i < numVLMSurface; i++) {

        if (vlmSurface->numSection < 2) {
            printf("Error: Surface '%s' has less than two-sections!\n", vlmSurface[i].name);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        status = get_mapAttrToIndexIndex(&astrosInstance[iIndex].transferMap,
                                         vlmSurface[i].name,
                                         &transferIndex);
        if (status == CAPS_NOTFOUND) {
            printf("\tA corresponding capsBound name not found for \"%s\". Surface will be ignored!\n", vlmSurface[i].name);
            continue;
        } else if (status != CAPS_SUCCESS) goto cleanup;

        for (j = 0; j < vlmSurface[i].numSection-1; j++) {

            // Increment the number of Aero surfaces
            astrosInstance[iIndex].feaProblem.numAero += 1;

            surfaceIndex = astrosInstance[iIndex].feaProblem.numAero - 1;

            // Allocate
            if (astrosInstance[iIndex].feaProblem.numAero == 1) {

                feaAeroTemp = (feaAeroStruct *) EG_alloc(astrosInstance[iIndex].feaProblem.numAero*sizeof(feaAeroStruct));

            } else {

                feaAeroTemp = (feaAeroStruct *) EG_reall(astrosInstance[iIndex].feaProblem.feaAero,
                                                         astrosInstance[iIndex].feaProblem.numAero*sizeof(feaAeroStruct));
            }

            if (feaAeroTemp == NULL) {
                astrosInstance[iIndex].feaProblem.numAero -= 1;

                status = EGADS_MALLOC;
                goto cleanup;
            }

            astrosInstance[iIndex].feaProblem.feaAero = feaAeroTemp;

            // Initiate feaAeroStruct
            status = initiate_feaAeroStruct(&astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex]);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Get surface Name - copy from original surface
            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].name = EG_strdup(vlmSurface[i].name);
            if (astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].name == NULL) { status = EGADS_MALLOC; goto cleanup; }

            // Get surface ID - Multiple by 1000 !!
            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].surfaceID = 1000*astrosInstance[iIndex].feaProblem.numAero;

            // ADD something for coordinate systems

            // Sections aren't necessarily stored in order coming out of vlm_getSections, however sectionIndex is!
            sectionIndex = vlmSurface[i].vlmSection[j].sectionIndex;

            // Populate vmlSurface structure
            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.Cspace = vlmSurface[i].Cspace;
            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.Sspace = vlmSurface[i].Sspace;

            // use the section span count for the sub-surface
            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.NspanTotal = vlmSurface[i].vlmSection[sectionIndex].Nspan;
            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.Nchord     = vlmSurface[i].Nchord;

            // Copy section information
            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.numSection = 2;

            astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection = (vlmSectionStruct *) EG_alloc(2*sizeof(vlmSectionStruct));
            if (astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            for (k = 0; k < astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.numSection; k++) {

                // Add k to section indexing variable j to get j and j+1 during iterations

                // Sections aren't necessarily stored in order coming out of vlm_getSections, however sectionIndex is!
                sectionIndex = vlmSurface[i].vlmSection[j+k].sectionIndex;

                status = initiate_vlmSectionStruct(&astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection[k]);
                if (status != CAPS_SUCCESS) goto cleanup;

                // Copy the section data - This also copies the control data for the section
                status = copy_vlmSectionStruct( &vlmSurface[i].vlmSection[sectionIndex],
                                                &astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection[k]);
                if (status != CAPS_SUCCESS) goto cleanup;

                // Reset the sectionIndex that is keeping track of the section order.
                astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection[k].sectionIndex = k;
            }
        }
    }

    // Determine which grid points are to be used for each spline
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++) {

        // Debug
        //printf("\tDetermining grid points\n");

        // Get the transfer index for this surface - it has already been checked to make sure the name is in the
        // transfer index map
        status = get_mapAttrToIndexIndex(&astrosInstance[iIndex].transferMap,
                                         astrosInstance[iIndex].feaProblem.feaAero[i].name,
                                         &transferIndex);
        if (status != CAPS_SUCCESS ) goto cleanup;

        if (projectionMethod == (int) false) { // Look for attributes

            for (j = 0; j < astrosInstance[iIndex].feaProblem.feaMesh.numNode; j++) {

                if (astrosInstance[iIndex].feaProblem.feaMesh.node[j].analysisType == MeshStructure) {
                    feaData = (feaMeshDataStruct *) astrosInstance[iIndex].feaProblem.feaMesh.node[j].analysisData;
                } else {
                    continue;
                }

                if (feaData->transferIndex != transferIndex) continue;
                if (feaData->transferIndex == CAPSMAGIC) continue;


                astrosInstance[iIndex].feaProblem.feaAero[i].numGridID += 1;
                k = astrosInstance[iIndex].feaProblem.feaAero[i].numGridID;

                astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_reall(astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet,
                                                                                           k*sizeof(int));

                if (astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet[k-1] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].nodeID;
            }

        } else { // Projection method


            /*
             *   n = A X B Create a normal vector/ plane between A and B
             *
             *   d_proj = C - (C * n)*n/ ||n||^2 , projection of point d on plane created by AxB
             *
             *   p = D - (D * n)*n/ ||n||^2 , projection of point p on plane created by AxB
             *
             *                         (section 2)
             *                     LE(c)---------------->TE(d)
             *   Grid Point       -^                   ^ -|
             *           |^      -            -         - |
             *           | -     A      -   C          - d_proj
             *           |  D   -    -                 -
             *           |   - - -     (section 1     -
             *           p    LE(a)----------B------->TE(b)
             */

            // Vector between section 2 and 1
            a = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[0].xyzLE;
            b = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[0].xyzTE;
            c = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[1].xyzLE;
            d = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[1].xyzTE;


            // Debug
            //printf("a = %f %f %f\n", a[0], a[1], a[2]);
            //printf("b = %f %f %f\n", b[0], b[1], b[2]);
            //printf("c = %f %f %f\n", c[0], c[1], c[2]);
            //printf("d = %f %f %f\n", d[0], d[1], d[2]);

            // Vector between LE of section 1 and LE of section 2
            A[0] = c[0] - a[0];
            A[1] = c[1] - a[1];
            A[2] = c[2] - a[2];

            // Vector between LE and TE of section 1
            B[0] = b[0] - a[0];
            B[1] = b[1] - a[1];
            B[2] = b[2] - a[2];

            // Vector between LE of section 1 and TE of section 2
            C[0] = d[0] - a[0];
            C[1] = d[1] - a[1];
            C[2] = d[2] - a[2];

            // Normal vector between A and B
            cross_DoubleVal(A, B, N);

            // Normalize normal vector
            n[0] = N[0]/sqrt(dot_DoubleVal(N, N));
            n[1] = N[1]/sqrt(dot_DoubleVal(N, N));
            n[2] = N[2]/sqrt(dot_DoubleVal(N, N));

            //printf("n = %f %f %f\n", n[0], n[1], n[2]);

            // Projection of vector C on plane created by AxB
            d_proj[0] = C[0] - dot_DoubleVal(C, n)*n[0] + a[0];
            d_proj[1] = C[1] - dot_DoubleVal(C, n)*n[1] + a[1];
            d_proj[2] = C[2] - dot_DoubleVal(C, n)*n[2] + a[2];

            //printf("d_proj = %f %f %f\n", d_proj[0], d_proj[1], d_proj[2]);

            // Vector between LE of section 1 and TE of section 2 where the TE as been projected on A x B plane
            C[0] = d_proj[0] - a[0];
            C[1] = d_proj[1] - a[1];
            C[2] = d_proj[2] - a[2];

            // Area of the rectangle (first triangle)
            cross_DoubleVal(A, C, N);
            Area = 0.5*sqrt(N[0]*N[0] + N[1]*N[1] + N[2]*N[2]);

            // Area of the rectangle (second triangle)
            cross_DoubleVal(C, B, N);
            Area = 0.5*sqrt(N[0]*N[0] + N[1]*N[1] + N[2]*N[2])  + Area;

            // Debug
            //printf("\tArea = %f\n",Area);

            for (j = 0; j < astrosInstance[iIndex].feaProblem.feaMesh.numNode; j++) {

                if (astrosInstance[iIndex].feaProblem.feaMesh.node[j].analysisType == MeshStructure) {
                    feaData = (feaMeshDataStruct *) astrosInstance[iIndex].feaProblem.feaMesh.node[j].analysisData;
                } else {
                    continue;
                }

                if (feaData->transferIndex != transferIndex) continue;
                if (feaData->transferIndex == CAPSMAGIC) continue;

                D[0] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].xyz[0] - a[0];

                D[1] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].xyz[1] - a[1];

                D[2] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].xyz[2] - a[2];

                // Projection of vector D on plane created by AxB
                p[0] = D[0] - dot_DoubleVal(D, n)*n[0] + a[0];
                p[1] = D[1] - dot_DoubleVal(D, n)*n[1] + a[1];
                p[2] = D[2] - dot_DoubleVal(D, n)*n[2] + a[2];

                // First triangle
                A[0] = a[0] - p[0];
                A[1] = a[1] - p[1];
                A[2] = a[2] - p[2];

                B[0] = b[0] - p[0];
                B[1] = b[1] - p[1];
                B[2] = b[2] - p[2];
                cross_DoubleVal(A, B, P);
                apbArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Second triangle
                A[0] = a[0] - p[0];
                A[1] = a[1] - p[1];
                A[2] = a[2] - p[2];

                B[0] = c[0] - p[0];
                B[1] = c[1] - p[1];
                B[2] = c[2] - p[2];
                cross_DoubleVal(A, B, P);
                apcArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Third triangle
                A[0] = c[0] - p[0];
                A[1] = c[1] - p[1];
                A[2] = c[2] - p[2];

                B[0] = d_proj[0] - p[0];
                B[1] = d_proj[1] - p[1];
                B[2] = d_proj[2] - p[2];
                cross_DoubleVal(A, B, P);
                cpdArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Fourth triangle
                A[0] = b[0] - p[0];
                A[1] = b[1] - p[1];
                A[2] = b[2] - p[2];

                B[0] = d_proj[0] - p[0];
                B[1] = d_proj[1] - p[1];
                B[2] = d_proj[2] - p[2];
                cross_DoubleVal(A, B, P);
                bpdArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Debug
                //printf("Area of Triangle  = %f\n", (apbArea + apcArea + cpdArea + bpdArea));
                //if (fabs(apbArea + apcArea + cpdArea + bpdArea - Area) > 1E-5) {
                //  printf("Point index %d\n", j);
                //  printf("\tAreas don't match - %f vs %f!!\n", Area, (apbArea + apcArea + cpdArea + bpdArea));
                //  printf("\tPoint - %f %f %f\n", D[0] + a[0], D[1] + a[1], D[2] + a[2]);
                //  printf("\tPoint projection - %f %f %f\n", p[0], p[1], p[2]);
                //}

                if (fabs(apbArea + apcArea + cpdArea + bpdArea - Area) > 1E-5)  continue;

                astrosInstance[iIndex].feaProblem.feaAero[i].numGridID += 1;

                if (astrosInstance[iIndex].feaProblem.feaAero[i].numGridID == 1) {
                    astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_alloc(astrosInstance[iIndex].feaProblem.feaAero[i].numGridID*sizeof(int));
                } else {
                    astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_reall(astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet,
                                                                                              astrosInstance[iIndex].feaProblem.feaAero[i].numGridID*sizeof(int));
                }

                if (astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet[
                    astrosInstance[iIndex].feaProblem.feaAero[i].numGridID-1] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].nodeID;
            }
        }

        printf("\tSurface %d: Number of points found for aero-spline = %d\n", i+1, astrosInstance[iIndex].feaProblem.feaAero[i].numGridID );
    }

    // Need to combine all aero surfaces into one for static, opt and trim analysis
    if (strcasecmp(analysisType, "Aeroelastic") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrim") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrimOpt") == 0 ) {

        printf("\t(Re-)Combining all aerodynamic surfaces into a single surface with multiple sections!\n");

        feaAeroTempCombine = (feaAeroStruct *) EG_alloc(sizeof(feaAeroStruct));

        // Initiate feaAeroStruct
        status = initiate_feaAeroStruct(&feaAeroTempCombine[0]);
        if (status != CAPS_SUCCESS) goto cleanup;

        if (astrosInstance[iIndex].feaProblem.feaAero == NULL) {
            status = CAPS_NULLVALUE;
            goto cleanup;
        }
        stringLength = strlen(astrosInstance[iIndex].feaProblem.feaAero[0].name);

        feaAeroTempCombine[0].name = (char *) EG_alloc((stringLength+1)*sizeof(char));
        if (feaAeroTempCombine[0].name == NULL) {
            (void) destroy_feaAeroStruct(&feaAeroTempCombine[0]);
            EG_free(feaAeroTempCombine);
            status = EGADS_MALLOC;
            goto cleanup;
        }

        memcpy(feaAeroTempCombine[0].name,
               astrosInstance[iIndex].feaProblem.feaAero[0].name,
               stringLength*sizeof(char));
        feaAeroTempCombine[0].name[stringLength] = '\0';

        feaAeroTempCombine[0].surfaceID = 1000*1;

        // ADD something for coordinate systems

        // Populate vmlSurface structure
        sectionIndex = 0;
        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++) {
            if (i == 0) {
                feaAeroTempCombine[0].vlmSurface.Cspace = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.Cspace;
                feaAeroTempCombine[0].vlmSurface.Sspace = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.Sspace;
                feaAeroTempCombine[0].vlmSurface.Nchord = 0;
                feaAeroTempCombine[0].vlmSurface.NspanTotal = 0;
            }

            if (feaAeroTempCombine[0].vlmSurface.Nchord < astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.Nchord) {
                feaAeroTempCombine[0].vlmSurface.Nchord = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.Nchord;
            }

            feaAeroTempCombine[0].vlmSurface.NspanTotal += astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.NspanTotal;

            // Get grids
            feaAeroTempCombine[0].numGridID += astrosInstance[iIndex].feaProblem.feaAero[i].numGridID;
            feaAeroTempCombine[0].gridIDSet = (int *) EG_reall(feaAeroTempCombine[0].gridIDSet,
                                                               feaAeroTempCombine[0].numGridID*sizeof(int));
            if (feaAeroTempCombine[0].gridIDSet == NULL) {
                (void) destroy_feaAeroStruct(&feaAeroTempCombine[0]);
                EG_free(feaAeroTempCombine);
                status = EGADS_MALLOC;
                goto cleanup;
            }

            memcpy(&feaAeroTempCombine[0].gridIDSet[feaAeroTempCombine[0].numGridID -
                                                    astrosInstance[iIndex].feaProblem.feaAero[i].numGridID],
                   astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet,
                   astrosInstance[iIndex].feaProblem.feaAero[i].numGridID*sizeof(int));

            // Copy section information
            for (j = 0; j < astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.numSection; j++) {

                skip = (int) false;
                for (k = 0; k < feaAeroTempCombine[0].vlmSurface.numSection; k++) {

                    // Check geometry
                    status = EG_isEquivalent(feaAeroTempCombine[0].vlmSurface.vlmSection[k].ebody,
                                             astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[j].ebody);
                    if (status == EGADS_SUCCESS) {
                        skip = (int) true;
                        break;
                    }

                    // Check geometry
                    status = EG_isSame(feaAeroTempCombine[0].vlmSurface.vlmSection[k].ebody,
                                       astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[j].ebody);
                    if (status == EGADS_SUCCESS) {
                        skip = (int) true;
                        break;
                    }
                }

                if (skip == (int) true) continue;

                feaAeroTempCombine[0].vlmSurface.numSection += 1;

                tempSection = (vlmSectionStruct *) EG_reall(feaAeroTempCombine[0].vlmSurface.vlmSection,
                                                            feaAeroTempCombine[0].vlmSurface.numSection*sizeof(vlmSectionStruct));

                if (tempSection == NULL) {
                    feaAeroTempCombine[0].vlmSurface.numSection -= 1;
                    (void) destroy_feaAeroStruct(&feaAeroTempCombine[0]);
                    EG_free(feaAeroTempCombine);
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                feaAeroTempCombine[0].vlmSurface.vlmSection = tempSection;

                status = initiate_vlmSectionStruct(&feaAeroTempCombine[0].vlmSurface.vlmSection[sectionIndex]);
                if (status != CAPS_SUCCESS) {
                    feaAeroTempCombine[0].vlmSurface.numSection -= 1;
                    (void) destroy_feaAeroStruct(&feaAeroTempCombine[0]);
                    EG_free(feaAeroTempCombine);
                    goto cleanup;
                }

                // Copy the section data - This also copies the control data for the section
                status = copy_vlmSectionStruct( &astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[j],
                                                &feaAeroTempCombine[0].vlmSurface.vlmSection[sectionIndex]);
                if (status != CAPS_SUCCESS) {
                    (void) destroy_feaAeroStruct(&feaAeroTempCombine[0]);
                    EG_free(feaAeroTempCombine);
                    goto cleanup;
                }

                // Reset the sectionIndex that is keeping track of the section order.
                feaAeroTempCombine[0].vlmSurface.vlmSection[sectionIndex].sectionIndex = sectionIndex;

                sectionIndex += 1;
            }
        }

        // Order cross sections for the surface - just in case
        status = vlm_orderSections(feaAeroTempCombine[0].vlmSurface.numSection, feaAeroTempCombine[0].vlmSurface.vlmSection);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Free old feaProblem Aero
        if (astrosInstance[iIndex].feaProblem.feaAero != NULL) {

            for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++) {
                status = destroy_feaAeroStruct(&astrosInstance[iIndex].feaProblem.feaAero[i]);
                if (status != CAPS_SUCCESS) printf("Status %d during destroy_feaAeroStruct\n", status);
            }
        }
        
        if (astrosInstance[iIndex].feaProblem.feaAero != NULL) EG_free(astrosInstance[iIndex].feaProblem.feaAero);
        astrosInstance[iIndex].feaProblem.feaAero = NULL;
        astrosInstance[iIndex].feaProblem.numAero = 0;

        // Point to new data
        astrosInstance[iIndex].feaProblem.numAero = 1;
        astrosInstance[iIndex].feaProblem.feaAero = feaAeroTempCombine;
    }

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("\tPremature exit in createVLMMesh, status = %d\n", status);

        if (vlmSurface != NULL) {

            for (i = 0; i < numVLMSurface; i++) {

                status2 = destroy_vlmSurfaceStruct(&vlmSurface[i]);
                if (status2 != CAPS_SUCCESS) printf("\tPremature exit in destroy_vlmSurfaceStruct, status = %d\n", status2);
            }
        }

        if (vlmSurface != NULL) EG_free(vlmSurface);
        numVLMSurface = 0;

        return status;
}

/*
static int createVLMMesh(int iIndex, void *aimInfo, capsValue *aimInputs) {

    int status, status2; // Function return status

    int i, j, surfaceIndex = 0, sectionIndex, transferIndex; // Indexing

    int stringLength;

    // Bodies
    const char *intents;
    int   numBody; // Number of Bodies
    ego  *bodies;

    // Aeroelastic information
    int numVLMSurface = 0;
    vlmSurfaceStruct *vlmSurface = NULL;

    feaAeroStruct *feaAeroTemp = NULL;

    // Vector variables
    double A[3], B[3], C[3], D[3], P[3], p[3], N[3], n[3], d_proj[3];

    double *a, *b, *c, *d;
    double apbArea, apcArea, cpdArea, bpdArea, Area;

    feaMeshDataStruct *feaData;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    #ifdef DEBUG
        printf(" astrosAIM/createVLMMesh instance = %d  nbody = %d!\n", iIndex, numBody);
    #endif

    if ((numBody <= 0) || (bodies == NULL)) {
        #ifdef DEBUG
            printf(" astrosAIM/createVLMMesh No Bodies!\n");
        #endif

        return CAPS_SOURCEERR;
    }

    // Get aerodynamic reference quantities
    status = fea_retrieveAeroRef(numBody, bodies, &astrosInstance[iIndex].feaProblem.feaAeroRef);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Cleanup Aero storage first
    if (astrosInstance[iIndex].feaProblem.feaAero != NULL) {

        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++) {
            status = destroy_feaAeroStruct(&astrosInstance[iIndex].feaProblem.feaAero[i]);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        EG_free(astrosInstance[iIndex].feaProblem.feaAero);
    }

    astrosInstance[iIndex].feaProblem.numAero = 0;

    // Get AVL surface information
    if (aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].nullVal != IsNull) {

        status = get_vlmSurface(aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].vals.tuple,
                                &astrosInstance[iIndex].attrMap,
                                0.0, // default Cspace
                                &numVLMSurface,
                                &vlmSurface);
        if (status != CAPS_SUCCESS) goto cleanup;

    } else {
        printf("An analysis type of Aeroelastic set but no VLM_Surface tuple specified\n");
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    printf("\nGetting FEA vortex lattice mesh\n");

    status = vlm_getSections(numBody,
                             bodies,
                             "Aerodynamic",
                             astrosInstance[iIndex].attrMap,
                             Y,
                             numVLMSurface,
                             &vlmSurface);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Order cross sections for each surface
    for (i = 0; i < numVLMSurface; i++) {
        status = vlm_orderSections(vlmSurface[i].numSection, vlmSurface[i].vlmSection);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Copy the VLM surface into the problem
    for (i = 0; i < numVLMSurface; i++) {

        if (vlmSurface->numSection < 2) {
            printf("Error: Surface %s has less than two-sections!\n", vlmSurface[i].name);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        status = get_mapAttrToIndexIndex(&astrosInstance[iIndex].transferMap,
                                         vlmSurface[i].name,
                                         &transferIndex);
        if (status == CAPS_NOTFOUND) {
            printf("\tA corresponding capsBound name not found for \"%s\". Surface will be ignored!\n", vlmSurface[i].name);
            continue;
        } else if (status != CAPS_SUCCESS) goto cleanup;


        // Increment the number of Aero surfaces
        astrosInstance[iIndex].feaProblem.numAero += 1;

        surfaceIndex = astrosInstance[iIndex].feaProblem.numAero - 1;

        // Allocate
        if (astrosInstance[iIndex].feaProblem.numAero == 1) {

            feaAeroTemp = (feaAeroStruct *) EG_alloc(astrosInstance[iIndex].feaProblem.numAero*sizeof(feaAeroStruct));

        } else {

            feaAeroTemp = (feaAeroStruct *) EG_reall(astrosInstance[iIndex].feaProblem.feaAero,
                                                     astrosInstance[iIndex].feaProblem.numAero*sizeof(feaAeroStruct));
        }

        if (feaAeroTemp == NULL) {
            astrosInstance[iIndex].feaProblem.numAero -= 1;

            status = EGADS_MALLOC;
            goto cleanup;
        }

        astrosInstance[iIndex].feaProblem.feaAero = feaAeroTemp;

        // Initiate feaAeroStruct
        status = initiate_feaAeroStruct(&astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex]);
        if (status != CAPS_SUCCESS) goto cleanup;


        // Get surface Name - copy from orginal surface
        stringLength = strlen(vlmSurface[i].name);

        astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].name = (char *) EG_alloc((stringLength+1)*sizeof(char));
        if (astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].name == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        memcpy(astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].name,
               vlmSurface[i].name,
               stringLength*sizeof(char));
        astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].name[stringLength] = '\0';

        // Get surface ID - Multiple by 1000 !!
        astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].surfaceID = 1000*astrosInstance[iIndex].feaProblem.numAero;

        // ADD something for coordinate systems

        // Copy vmlSurface structure
        status = copy_vlmSurfaceStruct(&vlmSurface[i], &astrosInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Determine which grid points are to be used for each spline
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++) {

        // Loop through each section - sections should have been ordered during the copy
        for (sectionIndex = 0; sectionIndex < astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.numSection-1; sectionIndex++) {
            // Debug
            //printf("\tDetermining grid points\n");

            // Get the transfer index for this surface - it has already been checked to make sure the name is in the
            // transfer index map
            status = get_mapAttrToIndexIndex(&astrosInstance[iIndex].transferMap,
                                             astrosInstance[iIndex].feaProblem.feaAero[i].name,
                                             &transferIndex);
            if (status != CAPS_SUCCESS ) goto cleanup;


             *   n = A X B Create a normal vector/ plane between A and B
             *
             *   d_proj = C - (C * n)*n/ ||n||^2 , projection of point d on plane created by AxB
             *
             *   p = D - (D * n)*n/ ||n||^2 , projection of point p on plane created by AxB
             *
             *                            (section 2)
             *                     LE(c)---------------->TE(d)
             *   Grid Point       -^                   ^ -|
             *           |^      -            -         - |
             *           | -     A      -   C          - d_proj
             *           |  D   -    -                 -
             *           |   - - -     (section 1     -
             *           p    LE(a)----------B------->TE(b)


            // Vector between section 2 and 1
            a = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[sectionIndex].xyzLE;
            b = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[sectionIndex].xyzTE;
            c = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[sectionIndex+1].xyzLE;
            d = astrosInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[sectionIndex+1].xyzTE;


            // Debug
            //printf("a = %f %f %f\n", a[0], a[1], a[2]);
            //printf("b = %f %f %f\n", b[0], b[1], b[2]);
            //printf("c = %f %f %f\n", c[0], c[1], c[2]);
            //printf("d = %f %f %f\n", d[0], d[1], d[2]);

            // Vector between LE of section 1 and LE of section 2
            A[0] = c[0] - a[0];
            A[1] = c[1] - a[1];
            A[2] = c[2] - a[2];

            // Vector between LE and TE of section 1
            B[0] = b[0] - a[0];
            B[1] = b[1] - a[1];
            B[2] = b[2] - a[2];

            // Vector between LE of section 1 and TE of section 2
            C[0] = d[0] - a[0];
            C[1] = d[1] - a[1];
            C[2] = d[2] - a[2];

            // Normal vector between A and B
            cross_DoubleVal(A, B, N);

            // Normalize normal vector
            n[0] = N[0]/sqrt(dot_DoubleVal(N, N));
            n[1] = N[1]/sqrt(dot_DoubleVal(N, N));
            n[2] = N[2]/sqrt(dot_DoubleVal(N, N));

            //printf("n = %f %f %f\n", n[0], n[1], n[2]);

            // Projection of vector C on plane created by AxB
            d_proj[0] = C[0] - dot_DoubleVal(C, n)*n[0] + a[0];
            d_proj[1] = C[1] - dot_DoubleVal(C, n)*n[1] + a[1];
            d_proj[2] = C[2] - dot_DoubleVal(C, n)*n[2] + a[2];

            //printf("d_proj = %f %f %f\n", d_proj[0], d_proj[1], d_proj[2]);

            // Vector between LE of section 1 and TE of section 2 where the TE as been projected on A x B plane
            C[0] = d_proj[0] - a[0];
            C[1] = d_proj[1] - a[1];
            C[2] = d_proj[2] - a[2];

            // Area of the rectangle (first triangle)
            cross_DoubleVal(A, C, N);
            Area = 0.5*sqrt(N[0]*N[0] + N[1]*N[1] + N[2]*N[2]);

            // Area of the rectangle (second triangle)
            cross_DoubleVal(C, B, N);
            Area = 0.5*sqrt(N[0]*N[0] + N[1]*N[1] + N[2]*N[2])  + Area;

            // Debug
            //printf("\tArea = %f\n",Area);

            for (j = 0; j < astrosInstance[iIndex].feaProblem.feaMesh.numNode; j++) {

                if (astrosInstance[iIndex].feaProblem.feaMesh.node[j].analysisType == MeshStructure) {
                    feaData = (feaMeshDataStruct *) astrosInstance[iIndex].feaProblem.feaMesh.node[j].analysisData;
                } else {
                    continue;
                }

                if (feaData->transferIndex != transferIndex) continue;
                if (feaData->transferIndex == CAPSMAGIC) continue;

                D[0] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].xyz[0] - a[0];

                D[1] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].xyz[1] - a[1];

                D[2] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].xyz[2] - a[2];

                // Projection of vector D on plane created by AxB
                p[0] = D[0] - dot_DoubleVal(D, n)*n[0] + a[0];
                p[1] = D[1] - dot_DoubleVal(D, n)*n[1] + a[1];
                p[2] = D[2] - dot_DoubleVal(D, n)*n[2] + a[2];

                // First triangle
                A[0] = a[0] - p[0];
                A[1] = a[1] - p[1];
                A[2] = a[2] - p[2];

                B[0] = b[0] - p[0];
                B[1] = b[1] - p[1];
                B[2] = b[2] - p[2];
                cross_DoubleVal(A, B, P);
                apbArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Second triangle
                A[0] = a[0] - p[0];
                A[1] = a[1] - p[1];
                A[2] = a[2] - p[2];

                B[0] = c[0] - p[0];
                B[1] = c[1] - p[1];
                B[2] = c[2] - p[2];
                cross_DoubleVal(A, B, P);
                apcArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Third triangle
                A[0] = c[0] - p[0];
                A[1] = c[1] - p[1];
                A[2] = c[2] - p[2];

                B[0] = d_proj[0] - p[0];
                B[1] = d_proj[1] - p[1];
                B[2] = d_proj[2] - p[2];
                cross_DoubleVal(A, B, P);
                cpdArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Fourth triangle
                A[0] = b[0] - p[0];
                A[1] = b[1] - p[1];
                A[2] = b[2] - p[2];

                B[0] = d_proj[0] - p[0];
                B[1] = d_proj[1] - p[1];
                B[2] = d_proj[2] - p[2];
                cross_DoubleVal(A, B, P);
                bpdArea = 0.5*sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);

                // Debug
                //printf("Area of Triangle  = %f\n", (apbArea + apcArea + cpdArea + bpdArea));
                //if (fabs(apbArea + apcArea + cpdArea + bpdArea - Area) > 1E-5) {
                //    printf("Point index %d\n", j);
                //    printf("\tAreas don't match - %f vs %f!!\n", Area, (apbArea + apcArea + cpdArea + bpdArea));
                //    printf("\tPoint - %f %f %f\n", D[0] + a[0], D[1] + a[1], D[2] + a[2]);
                //    printf("\tPoint projection - %f %f %f\n", p[0], p[1], p[2]);
                //}

                if (fabs(apbArea + apcArea + cpdArea + bpdArea - Area) > 1E-5) continue;

                astrosInstance[iIndex].feaProblem.feaAero[i].numGridID += 1;

                if (astrosInstance[iIndex].feaProblem.feaAero[i].numGridID == 1) {
                    astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_alloc(astrosInstance[iIndex].feaProblem.feaAero[i].numGridID*sizeof(int));
                } else {
                    astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_reall(astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet,
                                                                                              astrosInstance[iIndex].feaProblem.feaAero[i].numGridID*sizeof(int));
                }

                if (astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                astrosInstance[iIndex].feaProblem.feaAero[i].gridIDSet[
                    astrosInstance[iIndex].feaProblem.feaAero[i].numGridID-1] = astrosInstance[iIndex].feaProblem.feaMesh.node[j].nodeID;
            }

        } // End section loop

        printf("\tSurface %d: Number of points found for aero-spline = %d\n", i+1,
                                                                              astrosInstance[iIndex].feaProblem.feaAero[i].numGridID );
    }

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("\tcreateVLMMesh status = %d\n", status);

        if (vlmSurface != NULL) {

            for (i = 0; i < numVLMSurface; i++) {

                status2 = destroy_vlmSurfaceStruct(&vlmSurface[i]);
                if (status2 != CAPS_SUCCESS) printf("\tdestroy_vlmSurfaceStruct status = %d\n", status2);
            }
        }

        if (vlmSurface != NULL) EG_free(vlmSurface);
        numVLMSurface = 0;

        return status;
}
*/


/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(int ngIn, capsValue *gIn, int *qeFlag, const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int  *ints, flag;
    char **strs;

    aimStorage *tmp;

    #ifdef DEBUG
        printf("\n astrosAIM/aimInitialize   ngIn = %d!\n", ngIn);
    #endif
    flag    = *qeFlag;
    *qeFlag = 0;

    /* specify the number of analysis input and out "parameters" */
    *nIn     = NUMINPUT;
    *nOut    = NUMOUTPUT;
    if (flag == 1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate */
    *nFields = 3;
    ints     = (int *) EG_alloc(*nFields*sizeof(int));
    if (ints == NULL) return EGADS_MALLOC;
    ints[0]  = 3;
    ints[1]  = 3;
    ints[2]  = 3;
    *ranks   = ints;

    strs     = (char **) EG_alloc(*nFields*sizeof(char *));
    if (strs == NULL) {
        EG_free(*ranks);
        *ranks = NULL;
        return EGADS_MALLOC;
    }

    strs[0]  = EG_strdup("Displacement");
    strs[1]  = EG_strdup("EigenVector");
    strs[2]  = EG_strdup("EigenVector_*");
    *fnames  = strs;

    // Allocate astrosInstance
    if (astrosInstance == NULL) {

        astrosInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));

        if (astrosInstance == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }

    } else {
        tmp = (aimStorage *) EG_reall(astrosInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }

        astrosInstance = tmp;
    }

    // Initialize instance storage
    (void) initiate_aimStorage(numInstance);

    // Number of geometric inputs
    astrosInstance[numInstance].numGeomIn = ngIn;

    // Point to CAPS geometric in values
    astrosInstance[numInstance].geomInVal = gIn;

    // Increment number of instances
    numInstance += 1;

    return (numInstance -1);
}

int aimInputs(int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{

    /*! \page aimInputsAstros AIM Inputs
     * The following list outlines the Astros inputs along with their default value available
     * through the AIM interface. Unless noted these values will be not be linked to
     * any parent AIMs with variables of the same name.
     */

    #ifdef DEBUG
        printf(" astrosAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
    #endif

    *ainame = NULL;

    // Astros Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("astros_CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsAstros
         * - <B> Proj_Name = "astros_CAPS"</B> <br>
         * This corresponds to the project name used for file naming.
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

        /*! \page aimInputsAstros
         * - <B> Tess_Params = [0.025, 0.001, 15.0]</B> <br>
         * Body tessellation parameters used when creating a boundary element model.
         * Tess_Params[0] and Tess_Params[1] get scaled by the bounding
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
        *ainame               = EG_strdup("Edge_Point_Min");
        defval->type          = Integer;
        defval->vals.integer  = 2;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsAstros
         * - <B> aimInputsAstros = 2</B> <br>
         * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 4) {
        *ainame               = EG_strdup("Edge_Point_Max");
        defval->type          = Integer;
        defval->vals.integer  = 50;
        defval->length        = 1;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsAstros
         * - <B> Edge_Point_Max = 50</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */


    } else if (index == 5) {
        *ainame               = EG_strdup("Quad_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = (int) false;

        /*! \page aimInputsAstros
         * - <B> Quad_Mesh = False</B> <br>
         * Create a quadratic mesh on four edge faces when creating the boundary element model.
         */

    } else if (index == 6) {
        *ainame              = EG_strdup("Property");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Property = NULL</B> <br>
         * Property tuple used to input property information for the model, see \ref feaProperty for additional details.
         */
    } else if (index == 7) {
        *ainame              = EG_strdup("Material");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Material = NULL</B> <br>
         * Material tuple used to input material information for the model, see \ref feaMaterial for additional details.
         */
    } else if (index == 8) {
        *ainame              = EG_strdup("Constraint");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Constraint = NULL</B> <br>
         * Constraint tuple used to input constraint information for the model, see \ref feaConstraint for additional details.
         */
    } else if (index == 9) {
        *ainame              = EG_strdup("Load");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Load = NULL</B> <br>
         * Load tuple used to input load information for the model, see \ref feaLoad for additional details.
         */
    } else if (index == 10) {
        *ainame              = EG_strdup("Analysis");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Analysis = NULL</B> <br>
         * Analysis tuple used to input analysis/case information for the model, see \ref feaAnalysis for additional details.
         */
    } else if (index == 11) {
        *ainame              = EG_strdup("Analysis_Type");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Modal");
        defval->lfixed       = Change;

        /*! \page aimInputsAstros
         * - <B> Analysis_Type = "Modal"</B> <br>
         * Type of analysis to generate files for, options include "Modal", "Static", "AeroelasticTrim", "AeroelasticTrimOpt", "AeroelasticFlutter", and "Optimization".
         * Note: "Aeroelastic" and "StaticOpt" are still supported and refer to "AeroelasticTrim" and "Optimization".
         */
    } else if (index == 12) {
        *ainame              = EG_strdup("File_Format");
        defval->type         = String;
        defval->vals.string  = EG_strdup("Small"); // Small, Large, Free
        defval->lfixed       = Change;

        /*! \page aimInputsAstros
         * - <B> File_Format = "Small"</B> <br>
         * Formatting type for the bulk file. Options: "Small", "Large", "Free".
         */

    } else if (index == 13) {
        *ainame              = EG_strdup("Mesh_File_Format");
        defval->type         = String;
        defval->vals.string  = EG_strdup("Free"); // Small, Large, Free
        defval->lfixed       = Change;

        /*! \page aimInputsAstros
         * - <B> Mesh_File_Format = "Free"</B> <br>
         * Formatting type for the mesh file. Options: "Small", "Large", "Free".
         */

    } else if (index == 14) {
        *ainame              = EG_strdup("Design_Variable");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Design_Variable = NULL</B> <br>
         * The design variable tuple used to input design variable information for the model optimization, see \ref feaDesignVariable for additional details.
         */

    } else if (index == 15) {
        *ainame              = EG_strdup("Design_Constraint");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Design_Constraint = NULL</B> <br>
         * The design constraint tuple used to input design constraint information for the model optimization, see \ref feaDesignConstraint for additional details.
         */

    } else if (index == 16) {
        *ainame              = EG_strdup("ObjectiveMinMax");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Max"); // Max, Min
        defval->lfixed       = Change;

        /*! \page aimInputsAstros
         * - <B> ObjectiveMinMax = "Max"</B> <br>
         * Maximize or minimize the design objective during an optimization. Option: "Max" or "Min".
         */

    } else if (index == 17) {
        *ainame              = EG_strdup("ObjectiveResponseType");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Weight"); // Weight
        defval->lfixed       = Change;

        /*! \page aimInputsAstros
         * - <B> ObjectiveResponseType = "Weight"</B> <br>
         * Object response type (see Astros manual).
         */
    } else if (index == 18) {
        *ainame              = EG_strdup("VLM_Surface");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsAstros
         * - <B>VLM_Surface = NULL </B> <br>
         * Vortex lattice method tuple input. See \ref vlmSurface for additional details.
         */
    } else if (index == 19) {
        *ainame              = EG_strdup("Support");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Support = NULL</B> <br>
         * Support tuple used to input support information for the model, see \ref feaSupport for additional details.
         */
    } else if (index == 20) {
        *ainame              = EG_strdup("Connect");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Connect = NULL</B> <br>
         * Connect tuple used to define connection to be made in the, see \ref feaConnection for additional details.
         */

    } else if (index == 21) {
        *ainame              = EG_strdup("Parameter");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsAstros
         * - <B> Parameter = NULL</B> <br>
         * Parameter tuple used to define user entries. This can be used to input things to ASTROS such as CONVERT or MFORM etc.
         * The input is in Tuple form ("DATACARD", "DATAVALUE"). All inputs are strings.  Example: ("CONVERT","MASS,  0.00254").
         * Note:  Inputs assume a "," delimited entry.  Notice the "," after MASS in the Example.
         */
    }

    // Link variable(s) to parent(s) if available
    /*if ((index != 1) && (*ainame != NULL) && index < 6 ) {
        status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);


        printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n",
                        status, index, defval->type, defval->link);

    }
    */
    return CAPS_SUCCESS;
}

int aimData(int iIndex, const char *name, enum capsvType *vtype, int *rank, int *nrow,
        int *ncol, void **data, char **units)
{

    /*! \page sharableDataAstros AIM Shareable Data
     * Currently the Astros AIM does not have any shareable data types or values. It will try, however, to inherit a "FEA_MESH"
     * or "Volume_Mesh" from any parent AIMs. Note that the inheritance of the mesh is not required.
     */

    #ifdef DEBUG
        printf(" astrosAIM/aimData instance = %d  name = %s!\n", iIndex, name);
    #endif
   return CAPS_NOTFOUND;
}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath,
                   capsValue *aimInputs, capsErrs **errs)
{

    int i, j, k; // Indexing

    int status; // Status return

    //int found; // Boolean operator

    int *tempIntegerArray = NULL; // Temporary array to store a list of integers

    // Analysis information
    char *analysisType = NULL;
    int optFlag; // 0 - ANALYSIS, 1 - OPTIMIZATION Set based on analysisType char input

    // Optimization Information
    char *objectiveResp = NULL;
    const char *geomInName;

    // File IO
    char *filename = NULL; // Output file name
    FILE *fp = NULL; // Output file pointer
    int addComma = (int) false; // Add comma between inputs

    // NULL out errors
    *errs = NULL;

#ifdef DEBUG
    // Bodies
    const char *intents;
    int   numBody; // Number of Bodies
    ego  *bodies;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    printf(" astrosAIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, numBody);
#endif

    // Store away the analysis path/directory
    astrosInstance[iIndex].analysisPath = analysisPath;

    // Get project name
    astrosInstance[iIndex].projectName = aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string;

    // Analysis type
    analysisType = aimInputs[aim_getIndex(aimInfo, "Analysis_Type", ANALYSISIN)-1].vals.string;


    // Get FEA mesh if we don't already have one
    if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

        status = checkAndCreateMesh(iIndex, aimInfo);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Get Aeroelastic mesh
        if( strcasecmp(analysisType, "Aeroelastic") == 0 ||
            strcasecmp(analysisType, "AeroelasticTrim") == 0 ||
            strcasecmp(analysisType, "AeroelasticTrimOpt") == 0 ||
            strcasecmp(analysisType, "AeroelasticFlutter") == 0) {

            status = createVLMMesh(iIndex, aimInfo, aimInputs);
            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }

    // Note: Setting order is important here.
    // 1. Materials should be set before properties.
    // 2. Coordinate system should be set before mesh and loads
    // 3. Mesh should be set before loads, constraints, and supports
    // 4. Constraints and loads should be set before analysis
    // 5. Optimization should be set after properties, but before analysis

    // Set material properties
    if (aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getMaterial(aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].vals.tuple,
                                 &astrosInstance[iIndex].feaProblem.numMaterial,
                                 &astrosInstance[iIndex].feaProblem.feaMaterial);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Material tuple is NULL - No materials set\n");

    // Set property properties
    if (aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getProperty(aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].vals.tuple,
                                 &astrosInstance[iIndex].attrMap,
                                 &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;

        // Assign element "subtypes" based on properties set
        status = fea_assignElementSubType(astrosInstance[iIndex].feaProblem.numProperty,
                                          astrosInstance[iIndex].feaProblem.feaProperty,
                                          &astrosInstance[iIndex].feaProblem.feaMesh);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Property tuple is NULL - No properties set\n");

    // Set constraint properties
    if (aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getConstraint(aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].length,
                                   aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].vals.tuple,
                                   &astrosInstance[iIndex].constraintMap,
                                   &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Constraint tuple is NULL - No constraints applied\n");

    // Set support properties
    if (aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getSupport(aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].vals.tuple,
                                &astrosInstance[iIndex].constraintMap,
                                &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Support tuple is NULL - No supports applied\n");

    // Set connection properties
    if (aimInputs[aim_getIndex(aimInfo, "Connect", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getConnection(aimInputs[aim_getIndex(aimInfo, "Connect", ANALYSISIN)-1].length,
                                   aimInputs[aim_getIndex(aimInfo, "Connect", ANALYSISIN)-1].vals.tuple,
                                   &astrosInstance[iIndex].connectMap,
                                   &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;


        // Unify all connectionID's for RBE2 cards sake to be used for MPC in case control
        for (i = 0; i < astrosInstance[iIndex].feaProblem.numConnect; i++) {

            if (astrosInstance[iIndex].feaProblem.feaConnect[i].connectionType == RigidBody) {

                astrosInstance[iIndex].feaProblem.feaConnect[i].connectionID = astrosInstance[iIndex].feaProblem.numConnect+1;
            }
        }

    } else printf("Connect tuple is NULL - Using defaults\n");

    // Set load properties
    if (aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getLoad(aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].length,
                             aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].vals.tuple,
                             &astrosInstance[iIndex].loadMap,
                             &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;

        // Loop through loads to see if any of them are supposed to be from an external source
        for (i = 0; i < astrosInstance[iIndex].feaProblem.numLoad; i++) {

            if (astrosInstance[iIndex].feaProblem.feaLoad[i].loadType == PressureExternal) {

                // Transfer external pressures from the AIM discrObj
                status = fea_transferExternalPressure(aimInfo,
                                                      &astrosInstance[iIndex].feaProblem.feaMesh,
                                                      &astrosInstance[iIndex].feaProblem.feaLoad[i]);
                if (status != CAPS_SUCCESS) return status;

            } // End PressureExternal if
        } // End load for loop
    } else printf("Load tuple is NULL - No loads applied\n");

    // Set design variables
    if (aimInputs[aim_getIndex(aimInfo, "Design_Variable", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getDesignVariable(aimInputs[aim_getIndex(aimInfo, "Design_Variable", ANALYSISIN)-1].length,
                                       aimInputs[aim_getIndex(aimInfo, "Design_Variable", ANALYSISIN)-1].vals.tuple,
                                       &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Design_Variable tuple is NULL - No design variables applied\n");

    // Set design constraints
    if (aimInputs[aim_getIndex(aimInfo, "Design_Constraint", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getDesignConstraint(aimInputs[aim_getIndex(aimInfo, "Design_Constraint", ANALYSISIN)-1].length,
                                         aimInputs[aim_getIndex(aimInfo, "Design_Constraint", ANALYSISIN)-1].vals.tuple,
                                         &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Design_Constraint tuple is NULL - No design constraints applied\n");

    // Set analysis settings
    if (aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getAnalysis(aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].vals.tuple,
                                 &astrosInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status; // It ok to not have an analysis tuple
    } else printf("Analysis tuple is NULL\n");

    // Set file format type
    if        (strcasecmp(aimInputs[aim_getIndex(aimInfo, "File_Format", ANALYSISIN)-1].vals.string, "Small") == 0) {
        astrosInstance[iIndex].feaProblem.feaFileFormat.fileType = SmallField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "File_Format", ANALYSISIN)-1].vals.string, "Large") == 0) {
        astrosInstance[iIndex].feaProblem.feaFileFormat.fileType = LargeField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "File_Format", ANALYSISIN)-1].vals.string, "Free") == 0)  {
        astrosInstance[iIndex].feaProblem.feaFileFormat.fileType = FreeField;
    } else {
        printf("Unrecognized \"File_Format\", valid choices are [Small, Large, or Free]. Reverting to default\n");
    }

    // Set grid file format type
    if        (strcasecmp(aimInputs[aim_getIndex(aimInfo, "Mesh_File_Format", ANALYSISIN)-1].vals.string, "Small") == 0) {
        astrosInstance[iIndex].feaProblem.feaFileFormat.gridFileType = SmallField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "Mesh_File_Format", ANALYSISIN)-1].vals.string, "Large") == 0) {
        astrosInstance[iIndex].feaProblem.feaFileFormat.gridFileType = LargeField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "Mesh_File_Format", ANALYSISIN)-1].vals.string, "Free") == 0)  {
        astrosInstance[iIndex].feaProblem.feaFileFormat.gridFileType = FreeField;
    } else {
        printf("Unrecognized \"Mesh_File_Format\", valid choices are [Small, Large, or Free]. Reverting to default\n");
    }

    // Write Astros Mesh
    filename = EG_alloc(MXCHAR +1);
    if (filename == NULL) return EGADS_MALLOC;
    strcpy(filename, analysisPath);
    #ifdef WIN32
        strcat(filename, "\\");
    #else
        strcat(filename, "/");
    #endif
    strcat(filename, astrosInstance[iIndex].projectName);

    status = mesh_writeAstros(filename,
                              1,
                              &astrosInstance[iIndex].feaProblem.feaMesh,
                              astrosInstance[iIndex].feaProblem.feaFileFormat.gridFileType,
                              astrosInstance[iIndex].feaProblem.numDesignVariable,
                              astrosInstance[iIndex].feaProblem.feaDesignVariable,
                              1.0);
    if (status != CAPS_SUCCESS) {
        EG_free(filename);
        return status;
    }

    // Write Nastran subElement types not supported by mesh_writeAstros
    strcat(filename, ".bdf");
    fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        EG_free(filename);
        return CAPS_IOERR;
    }
    EG_free(filename);

    printf("Writing subElement types (if any) - appending mesh file\n");
    status = astros_writeSubElementCard(fp,
                                         &astrosInstance[iIndex].feaProblem.feaMesh,
                                         astrosInstance[iIndex].feaProblem.numProperty,
                                         astrosInstance[iIndex].feaProblem.feaProperty,
                                         &astrosInstance[iIndex].feaProblem.feaFileFormat);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Connections
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numConnect; i++) {

        if (i == 0) {
            printf("Writing connection cards - appending mesh file\n");
        }

        status = astros_writeConnectionCard(fp,
                                            &astrosInstance[iIndex].feaProblem.feaConnect[i],
                                            &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }
    if (fp != NULL) fclose(fp);
    fp = NULL;


    // Write nastran input file
    filename = EG_alloc(MXCHAR +1);
    if (filename == NULL) return EGADS_MALLOC;
    strcpy(filename, analysisPath);
    #ifdef WIN32
        strcat(filename, "\\");
    #else
        strcat(filename, "/");
    #endif
    strcat(filename, astrosInstance[iIndex].projectName);
    strcat(filename, ".dat");


    printf("\nWriting Astros instruction file....\n");
    fp = fopen(filename, "w");
    EG_free(filename);
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        return CAPS_IOERR;
    }

    // define file format delimiter type
    /*
    if (astrosInstance[iIndex].feaProblem.feaFileFormat.fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }
    */

    //////////////// Executive control ////////////////
    fprintf(fp, "ASSIGN DATABASE CAPS PASS NEW\n");

    //////////////// Case control ////////////////
    fprintf(fp, "SOLUTION\n");
    fprintf(fp, "TITLE = %s\n", astrosInstance[iIndex].projectName);
    // Analysis type
    if     (strcasecmp(analysisType, "Modal")              == 0 ||
            strcasecmp(analysisType, "Static")             == 0 ||
            strcasecmp(analysisType, "AeroelasticTrim")    == 0 ||
            strcasecmp(analysisType, "AeroelasticFlutter") == 0 ||
            strcasecmp(analysisType, "Aeroelastic")        == 0) {

        fprintf(fp, "ANALYZE\n");
        optFlag = 0;
    }
    else if(strcasecmp(analysisType, "StaticOpt")          == 0 ||
            strcasecmp(analysisType, "Optimization")       == 0 ||
            strcasecmp(analysisType, "AeroelasticTrimOpt") == 0) {

        fprintf(fp, "OPTIMIZE STRATEGY=((FSD,10), (MP,20)), MAXITER=30, NRFAC=1.5,\n");
        fprintf(fp, "EPS= 1.00, MOVLIM=1.5\n");
        fprintf(fp, "PRINT DCON=ALL, GDES=ALL, GPWG=ALL\n");
        optFlag = 1;
    }

    else {
        printf("Unrecognized \"Analysis_Type\", %s, defaulting to \"Modal\" analysis\n", analysisType);
        analysisType = "Modal";
        optFlag = 0;
        fprintf(fp, "ANALYZE\n");
    }

    // Set up the case information
    if (astrosInstance[iIndex].feaProblem.numAnalysis != 0) {

        // Write subcase information if multiple analysis tuples were provide
        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAnalysis; i++) {

            // Write boundary constraints/supports/etc.
            fprintf(fp, "  BOUNDARY");

            addComma = (int) false;
            // Write support for sub-case
            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].numSupport != 0) {
                if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].numSupport > 1) {
                    printf("\tWARNING: More than 1 support is not supported at this time for sub-cases!\n");

                } else {
                    fprintf(fp, " SUPPORT = %d ", astrosInstance[iIndex].feaProblem.feaAnalysis[i].supportSetID[0]);
                    addComma = (int) true;
                }
            }

            // Write constraint for sub-case
            if (astrosInstance[iIndex].feaProblem.numConstraint != 0) {
                if (addComma == (int) true) fprintf(fp,",");

                fprintf(fp, " SPC = %d ", astrosInstance[iIndex].feaProblem.numConstraint+i+1);
                addComma = (int) true;
            }

            // Write MPC for sub-case - currently only supported when we have RBE2 elements - see above for unification
            for (j = 0; j < astrosInstance[iIndex].feaProblem.numConnect; j++) {

                if (astrosInstance[iIndex].feaProblem.feaConnect[j].connectionType == RigidBody) {

                    if (addComma == (int) true) fprintf(fp,",");

                    fprintf(fp, " MPC = %d ", astrosInstance[iIndex].feaProblem.feaConnect[j].connectionID);
                    addComma = (int) true;
                    break;
                }
            }

            // Issue some warnings regarding constraints if necessary
            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint == 0 &&
                astrosInstance[iIndex].feaProblem.numConstraint !=0) {

                printf("\tWarning: No constraints specified for case %s, assuming "
                        "all constraints are applied!!!!\n", astrosInstance[iIndex].feaProblem.feaAnalysis[i].name);

            } else if (astrosInstance[iIndex].feaProblem.numConstraint == 0) {

                printf("\tWarning: No constraints specified for case %s!!!!\n", astrosInstance[iIndex].feaProblem.feaAnalysis[i].name);
            }

            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Modal ||
                astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticFlutter ) {
                if (addComma == (int) true) fprintf(fp,",");
                fprintf(fp," METHOD = %d ", astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
                addComma = (int) true;
            }

            fprintf(fp, "\n");


            fprintf(fp, "    LABEL = %s\n", astrosInstance[iIndex].feaProblem.feaAnalysis[i].name);

            // Write discipline
            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Static) { // Static

                fprintf(fp, "    STATICS ");

                // Write loads for sub-case
                if (astrosInstance[iIndex].feaProblem.numLoad != 0) {
                    fprintf(fp, " (MECH = %d)", astrosInstance[iIndex].feaProblem.numLoad+i+1);
                }

                // Issue some warnings regarding loads if necessary
                if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].numLoad == 0 && astrosInstance[iIndex].feaProblem.numLoad !=0) {
                    printf("\tWarning: No loads specified for static case %s, assuming "
                           "all loads are applied!!!!\n", astrosInstance[iIndex].feaProblem.feaAnalysis[i].name);
                } else if (astrosInstance[iIndex].feaProblem.numLoad == 0) {
                    printf("\tWarning: No loads specified for static case %s!!!!\n", astrosInstance[iIndex].feaProblem.feaAnalysis[i].name);
                }


                if (optFlag == 0) {
                    fprintf(fp, "\n");
                    fprintf(fp, "    PRINT DISP=ALL, STRESS=ALL\n");
                } else {
                    fprintf(fp, ", CONST( STRESS = %d)\n", astrosInstance[iIndex].feaProblem.numDesignConstraint+i+1);
                    fprintf(fp, "    PRINT DISP(ITER=LAST)=ALL, STRESS(ITER=LAST)=ALL\n");
                }
            }

            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Modal) {// Modal
                fprintf(fp, "    MODES\n");
                fprintf(fp, "    PRINT (MODES=ALL) DISP=ALL, ROOT=ALL\n");
            }
            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticTrim) {// Trim
                fprintf(fp, "    SAERO SYMMETRIC (TRIM=%d)", astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);

                if (optFlag == 0) {
                	fprintf(fp, "\n");
                	fprintf(fp, "    PRINT DISP=ALL, GPWG=ALL, TRIM, TPRE=ALL, STRESS=ALL\n");
                } else {
                	fprintf(fp, ", CONST(STRESS = %d)\n", astrosInstance[iIndex].feaProblem.numDesignConstraint+i+1);
                	fprintf(fp, "    PRINT (ITER=LAST) DISP=ALL, GPWG=ALL, TRIM, TPRE=ALL, STRESS=ALL\n");
                }



            }
            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticFlutter) {// Flutter
                fprintf(fp, "    MODES\n");
                fprintf(fp, "    FLUTTER (FLCOND = %d)\n", astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
                fprintf(fp, "    PRINT (MODES=ALL) DISP=ALL, ROOT=ALL\n");
            }
        }

    } else { // If no sub-cases

        if (strcasecmp(analysisType, "Modal") == 0) {
            printf("Warning: No eigenvalue analysis information specified in \"Analysis\" tuple, through "
                    "AIM input \"Analysis_Type\" is set to \"Modal\"!!!!\n");
            return CAPS_NOTFOUND;
        }

        fprintf(fp, "  BOUNDARY");

        // Write support information
        addComma = (int) false;
        if (astrosInstance[iIndex].feaProblem.numSupport != 0) {
            if (astrosInstance[iIndex].feaProblem.numSupport > 1) {
                printf("\tWARNING: More than 1 support is not supported at this time for a given case!\n");
            } else {
                fprintf(fp, " SUPPORT = %d ", astrosInstance[iIndex].feaProblem.numSupport+1);
                addComma = (int) true;
            }
        }

        // Write constraint information
        if (astrosInstance[iIndex].feaProblem.numConstraint != 0) {
            if (addComma == (int) true) fprintf(fp, ",");
            fprintf(fp, " SPC = %d ", astrosInstance[iIndex].feaProblem.numConstraint+1);
        } else {
            printf("\tWarning: No constraints specified for job!!!!\n");
        }

        // Write MPC for sub-case - currently only supported when we have RBE2 elements - see above for unification
        for (j = 0; j < astrosInstance[iIndex].feaProblem.numConnect; j++) {

            if (astrosInstance[iIndex].feaProblem.feaConnect[j].connectionType == RigidBody) {

                if (addComma == (int) true) fprintf(fp,",");

                fprintf(fp, " MPC = %d ", astrosInstance[iIndex].feaProblem.feaConnect[j].connectionID);
                addComma = (int) true;
                break;
            }
        }

        fprintf(fp, "\n");

        // Write discipline
        if (strcasecmp(analysisType, "Static") == 0) { // Static loads

            fprintf(fp, "    STATICS");

            // Write loads for sub-case
            if (astrosInstance[iIndex].feaProblem.numLoad != 0) {
                fprintf(fp, " (MECH = %d)", astrosInstance[iIndex].feaProblem.numLoad+1);
            } else {
                printf("\tWarning: No loads specified for job!!!!\n");
            }

            fprintf(fp, "\n");

            fprintf(fp, "    PRINT DISP=ALL, STRESS=ALL\n");
        }
    }


    fprintf(fp, "END\n$\n"); // End Case control

    //////////////// Bulk data ////////////////
    fprintf(fp, "BEGIN BULK(SORT)\n");
    fprintf(fp, "$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|\n");

    //PRINT Parameter ENTRIES IN BULK DATA

    if (aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].nullVal == NotNull) {
        for (i = 0; i < aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].length; i++) {

            fprintf(fp, "%s, %s\n", aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].vals.tuple[i].name,
                                    aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].vals.tuple[i].value);
        }
    }

    // Turn off auto SPC
    //fprintf(fp, "%-8s %7s %7s\n", "PARAM", "AUTOSPC", "N");

    // Optimization Objective Response Response, SOL 200 only
    if (strcasecmp(analysisType, "StaticOpt") == 0 || strcasecmp(analysisType, "Optimization") == 0) {

        objectiveResp = aimInputs[aim_getIndex(aimInfo, "ObjectiveResponseType", ANALYSISIN)-1].vals.string;
        if     (strcasecmp(objectiveResp, "Weight") == 0) objectiveResp = "WEIGHT";
        else {
            printf("\tUnrecognized \"ObjectiveResponseType\", %s, defaulting to \"Weight\"\n", objectiveResp);
            objectiveResp = "WEIGHT";
        }

        /*
        fprintf(fp,"%-8s", "DRESP1");

        tempString = convert_integerToString(1, 7, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%7s", delimiter, objectiveResp);
        fprintf(fp, "%s%7s", delimiter, objectiveResp);


        fprintf(fp, "\n");
         */
    }

    // Write AERO Card
    if (strcasecmp(analysisType, "AeroelasticFlutter") == 0 ) {

        printf("\tWriting aero card\n");
        status = astros_writeAEROCard(fp,
                                      &astrosInstance[iIndex].feaProblem.feaAeroRef,
                                      &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) return status;
    }

    // Write AESTAT and AESURF cards
    if (strcasecmp(analysisType, "Aeroelastic") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrim") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrimOpt") == 0) {

        printf("\tWriting aeros card\n");
        status = astros_writeAEROSCard(fp,
                                       &astrosInstance[iIndex].feaProblem.feaAeroRef,
                                       &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) return status;

        /* No AESTAT Cards in ASTROS
        numAEStatSurf = 0;
        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAnalysis; i++) {

            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].analysisType != AeroelasticStatic) continue;

            if (i == 0) printf("\tWriting aestat cards\n");

            // Loop over rigid variables
            for (j = 0; j < astrosInstance[iIndex].feaProblem.feaAnalysis[i].numRigidVariable; j++) {

                found = (int) false;

                // Loop over previous rigid variables
                for (k = 0; k < i; k++) {
                    for (l = 0; l < astrosInstance[iIndex].feaProblem.feaAnalysis[k].numRigidVariable; l++) {

                        // If current rigid variable was is a previous written mark as found
                        if (strcmp(astrosInstance[iIndex].feaProblem.feaAnalysis[i].rigidVariable[j],
                                   astrosInstance[iIndex].feaProblem.feaAnalysis[k].rigidVariable[l]) == 0) {
                            found = (int) true;
                        }
                    }
                }

                // If already found continue
                if (found == (int) true) continue;

                // If not write out an aestat card
                numAEStatSurf += 1;

                fprintf(fp,"%-8s", "AESTAT");

                tempString = convert_integerToString(numAEStatSurf, 7, 1);
                fprintf(fp, "%s%s", delimiter, tempString);
                EG_free(tempString);

                fprintf(fp, "%s%7s\n", delimiter, astrosInstance[iIndex].feaProblem.feaAnalysis[i].rigidVariable[j]);
            }
        }
        */

        fprintf(fp,"\n");
    }

    // Analysis Cards - Eigenvalue and design objective included, as well as combined load, constraint, and design constraints
    if (astrosInstance[iIndex].feaProblem.numAnalysis != 0) {

        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAnalysis; i++) {

            if (i == 0) printf("\tWriting analysis cards\n");

            status = astros_writeAnalysisCard(fp,
                                              &astrosInstance[iIndex].feaProblem.feaAnalysis[i],
                                              &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) return status;

            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].numLoad != 0) {
                //TOOO: eliminate load add card?
                // Write combined load card
                printf("\tWriting load ADD cards\n");
                status = nastran_writeLoadADDCard(fp,
                                                  astrosInstance[iIndex].feaProblem.numLoad+i+1,
                                                  astrosInstance[iIndex].feaProblem.feaAnalysis[i].numLoad,
                                                  astrosInstance[iIndex].feaProblem.feaAnalysis[i].loadSetID,
                                                  astrosInstance[iIndex].feaProblem.feaLoad,
                                                  &astrosInstance[iIndex].feaProblem.feaFileFormat);
                if (status != CAPS_SUCCESS) return status;

            } else { // If no loads for an individual analysis are specified assume that all loads should be applied

                if (astrosInstance[iIndex].feaProblem.numLoad != 0) {

                    // Create a temporary list of load IDs
                    tempIntegerArray = (int *) EG_alloc(astrosInstance[iIndex].feaProblem.numLoad*sizeof(int));
                    if (tempIntegerArray == NULL) return EGADS_MALLOC;

                    for (j = 0; j < astrosInstance[iIndex].feaProblem.numLoad; j++) {
                        tempIntegerArray[j] = astrosInstance[iIndex].feaProblem.feaLoad[j].loadID;
                    }

                    //TOOO: eliminate load add card?
                    // Write combined load card
                    printf("\tWriting load ADD cards\n");
                    status = nastran_writeLoadADDCard(fp,
                                                      astrosInstance[iIndex].feaProblem.numLoad+i+1,
                                                      astrosInstance[iIndex].feaProblem.numLoad,
                                                      tempIntegerArray,
                                                      astrosInstance[iIndex].feaProblem.feaLoad,
                                                      &astrosInstance[iIndex].feaProblem.feaFileFormat);

                    if (status != CAPS_SUCCESS) return status;

                    // Free temporary load ID list
                    if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
                    tempIntegerArray = NULL;
                }

            }

            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint != 0) {

                // Write combined constraint card
                printf("\tWriting constraint cards--each subcase individually\n");
                fprintf(fp,"$\n$ Constraint(s)\n");

                for (j = 0; j < astrosInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint; j++) {
                    k = astrosInstance[iIndex].feaProblem.feaAnalysis[i].constraintSetID[j] - 1;

                    // one spc set per subcase, each different
                    status = astros_writeConstraintCard(fp,
                                                        astrosInstance[iIndex].feaProblem.numConstraint+i+1,
                                                        &astrosInstance[iIndex].feaProblem.feaConstraint[k],
                                                        &astrosInstance[iIndex].feaProblem.feaFileFormat);
                    if (status != CAPS_SUCCESS) goto cleanup;
                }

//                printf("\tWriting constraint ADD cards\n");
//                status = nastran_writeConstraintADDCard(fp,
//                                                        astrosInstance[iIndex].feaProblem.numConstraint+i+1,
//                                                        astrosInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint,
//                                                        astrosInstance[iIndex].feaProblem.feaAnalysis[i].constraintSetID,
//                                                        &astrosInstance[iIndex].feaProblem.feaFileFormat);
//                if (status != CAPS_SUCCESS) return status;

            } else { // If no constraints for an individual analysis are specified assume that all constraints should be applied

                if (astrosInstance[iIndex].feaProblem.numConstraint != 0) {

                    // Write combined constraint card
                    printf("\tWriting constraint cards--all constraints for each subcase\n");
                    fprintf(fp,"$\n$ Constraint(s)\n");

                    for (j = 0; j < astrosInstance[iIndex].feaProblem.numConstraint; j++) {

                        // one spc set per subcase, each the same
                        status = astros_writeConstraintCard(fp,
                                                            astrosInstance[iIndex].feaProblem.numConstraint+i+1,
                                                            &astrosInstance[iIndex].feaProblem.feaConstraint[j],
                                                            &astrosInstance[iIndex].feaProblem.feaFileFormat);
                        if (status != CAPS_SUCCESS) goto cleanup;
                    }
//
//                    printf("\tWriting combined constraint cards\n");
//
//                    // Create a temporary list of constraint IDs
//                    tempIntegerArray = (int *) EG_alloc(astrosInstance[iIndex].feaProblem.numConstraint*sizeof(int));
//                    if (tempIntegerArray == NULL) return EGADS_MALLOC;
//
//                    for (j = 0; j < astrosInstance[iIndex].feaProblem.numConstraint; j++) {
//                        tempIntegerArray[j] = astrosInstance[iIndex].feaProblem.feaConstraint[j].constraintID;
//                    }
//
//                    // Write combined constraint card
//                    status = nastran_writeConstraintADDCard(fp,
//                                                            astrosInstance[iIndex].feaProblem.numConstraint+i+1,
//                                                            astrosInstance[iIndex].feaProblem.numConstraint,
//                                                            tempIntegerArray,
//                                                            &astrosInstance[iIndex].feaProblem.feaFileFormat);
//                    if (status != CAPS_SUCCESS) goto cleanup;
//
//                    // Free temporary constraint ID list
//                    if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
//                    tempIntegerArray = NULL;
                }
            }

            if (astrosInstance[iIndex].feaProblem.feaAnalysis[i].numDesignConstraint != 0) {

                printf("\tWriting design constraint cards--no subcases\n");
                fprintf(fp,"$\n$ Design constraint(s)\n");
                for( j = 0; j < astrosInstance[iIndex].feaProblem.feaAnalysis[i].numDesignConstraint; j++) {
                    k = astrosInstance[iIndex].feaProblem.feaAnalysis[i].designConstraintSetID[j] - 1;

                    // one design constraint set per subcase analysis, each may be different
                    status = astros_writeDesignConstraintCard(fp,
                                                              astrosInstance[iIndex].feaProblem.numDesignConstraint+i+1,
                                                              &astrosInstance[iIndex].feaProblem.feaDesignConstraint[k],
                                                              astrosInstance[iIndex].feaProblem.numMaterial,
                                                              astrosInstance[iIndex].feaProblem.feaMaterial,
                                                              astrosInstance[iIndex].feaProblem.numProperty,
                                                              astrosInstance[iIndex].feaProblem.feaProperty,
                                                              &astrosInstance[iIndex].feaProblem.feaFileFormat);
                    if (status != CAPS_SUCCESS) goto cleanup;
                }

            } else { // If no design constraints for an individual analysis are specified assume that all design constraints should be applied

                if (astrosInstance[iIndex].feaProblem.numDesignConstraint != 0) {

                    printf("\tWriting design constraint cards\n");
                    fprintf(fp,"$\n$ Design constraint(s)\n");
                    for( j = 0; j < astrosInstance[iIndex].feaProblem.numDesignConstraint; j++) {

                        // one design constraint set per subcase analysis, all the same
                        status = astros_writeDesignConstraintCard(fp,
                                                                  astrosInstance[iIndex].feaProblem.numDesignConstraint+i+1,
                                                                  &astrosInstance[iIndex].feaProblem.feaDesignConstraint[j],
                                                                  astrosInstance[iIndex].feaProblem.numMaterial,
                                                                  astrosInstance[iIndex].feaProblem.feaMaterial,
                                                                  astrosInstance[iIndex].feaProblem.numProperty,
                                                                  astrosInstance[iIndex].feaProblem.feaProperty,
                                                                  &astrosInstance[iIndex].feaProblem.feaFileFormat);
                        if (status != CAPS_SUCCESS) goto cleanup;
                    }
                }

            }
        }

    } else { // If there aren't any analysis structures just write a single combined load , combined constraint,
             // and design constraint card

        // Combined loads
        if (astrosInstance[iIndex].feaProblem.numLoad != 0) {

            // Create a temporary list of load IDs
            tempIntegerArray = (int *) EG_alloc(astrosInstance[iIndex].feaProblem.numLoad*sizeof(int));
            if (tempIntegerArray == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            for (i = 0; i < astrosInstance[iIndex].feaProblem.numLoad; i++) {
                tempIntegerArray[i] = astrosInstance[iIndex].feaProblem.feaLoad[i].loadID;
            }

            // Write combined load card
            printf("\tWriting load ADD cards\n");
            status = nastran_writeLoadADDCard(fp,
                                              astrosInstance[iIndex].feaProblem.numLoad+1,
                                              astrosInstance[iIndex].feaProblem.numLoad,
                                              tempIntegerArray,
                                              astrosInstance[iIndex].feaProblem.feaLoad,
                                              &astrosInstance[iIndex].feaProblem.feaFileFormat);

            if (status != CAPS_SUCCESS) goto cleanup;

            // Free temporary load ID list
            if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
            tempIntegerArray = NULL;
        }

        // Combined constraints
        if (astrosInstance[iIndex].feaProblem.numConstraint != 0) {

            printf("\tWriting combined constraint cards\n");

            // Write combined constraint card
            printf("\tWriting constraint cards\n");
            fprintf(fp,"$\n$ Constraint(s)\n");

            for (i = 0; i < astrosInstance[iIndex].feaProblem.numConstraint; i++) {

                // one spc set, there are no subcases
                status = astros_writeConstraintCard(fp,
                                                    astrosInstance[iIndex].feaProblem.numConstraint+1,
                                                    &astrosInstance[iIndex].feaProblem.feaConstraint[i],
                                                    &astrosInstance[iIndex].feaProblem.feaFileFormat);
                if (status != CAPS_SUCCESS) goto cleanup;
            }
        }

        // Combined design constraints
        if (astrosInstance[iIndex].feaProblem.numDesignConstraint != 0) {

            printf("\tWriting design constraint cards\n");
            fprintf(fp,"$\n$ Design constraint(s)\n");
            for( i = 0; i < astrosInstance[iIndex].feaProblem.numDesignConstraint; i++) {

                // only one design constraint set--there are no subcase analyses
                status = astros_writeDesignConstraintCard(fp,
                                                          astrosInstance[iIndex].feaProblem.numDesignConstraint+1,
                                                          &astrosInstance[iIndex].feaProblem.feaDesignConstraint[i],
                                                          astrosInstance[iIndex].feaProblem.numMaterial,
                                                          astrosInstance[iIndex].feaProblem.feaMaterial,
                                                          astrosInstance[iIndex].feaProblem.numProperty,
                                                          astrosInstance[iIndex].feaProblem.feaProperty,
                                                          &astrosInstance[iIndex].feaProblem.feaFileFormat);
                if (status != CAPS_SUCCESS) goto cleanup;
            }
        }
    }

    // Loads
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numLoad; i++) {

        if (i == 0) {
            printf("\tWriting load cards\n");
            fprintf(fp,"$\n$ Load(s)\n");
        }

        status = astros_writeLoadCard(fp,
                                      &astrosInstance[iIndex].feaProblem.feaMesh,
                                      &astrosInstance[iIndex].feaProblem.feaLoad[i],
                                      &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Constraints
    // Move to subcase level because ASTROS does not support SPCADD card--DB 8 Mar 18
    /* for (i = 0; i < astrosInstance[iIndex].feaProblem.numConstraint; i++) {

        if (i == 0) {
            printf("\tWriting constraint cards\n");
            fprintf(fp,"$\n$ Constraint(s)\n");
        }

        status = nastran_writeConstraintCard(fp,
                                             &astrosInstance[iIndex].feaProblem.feaConstraint[i],
                                             &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    } */

    // Supports
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numSupport; i++) {

        if (i == 0) {
            printf("\tWriting support cards\n");
            fprintf(fp,"$\n$ Support(s)\n");
        }

        status = astros_writeSupportCard(fp,
                                         &astrosInstance[iIndex].feaProblem.feaSupport[i],
                                         &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }


    // Materials
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numMaterial; i++) {

        if (i == 0) {
            printf("\tWriting material cards\n");
            fprintf(fp,"$\n$ Material(s)\n");
        }

        status = nastran_writeMaterialCard(fp,
                                           &astrosInstance[iIndex].feaProblem.feaMaterial[i],
                                           &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Properties
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numProperty; i++) {

        if (i == 0) {
            printf("\tWriting property cards\n");
            fprintf(fp,"$\n$ Property(ies)\n");
        }

        status = astros_writePropertyCard(fp,
                                          &astrosInstance[iIndex].feaProblem.feaProperty[i],
                                          &astrosInstance[iIndex].feaProblem.feaFileFormat,
                                          astrosInstance[iIndex].feaProblem.numDesignVariable,
                                          astrosInstance[iIndex].feaProblem.feaDesignVariable);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Coordinate systems
    for (i = 0; i < astrosInstance[iIndex].feaProblem.numCoordSystem; i++) {

        if (i == 0) {
            printf("\tWriting coordinate system cards\n");
            fprintf(fp,"$\n$ Coordinate system(s)\n");
        }

        status = nastran_writeCoordinateSystemCard(fp, &astrosInstance[iIndex].feaProblem.feaCoordSystem[i], &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Optimization - design variables
    for( i = 0; i < astrosInstance[iIndex].feaProblem.numDesignVariable; i++) {

        if (i == 0) {
            printf("\tWriting design variables and analysis - design variable relation cards\n");
            fprintf(fp,"$\n$ Design variable(s)\n");
        }

        status = astros_writeDesignVariableCard(fp,
                                                &astrosInstance[iIndex].feaProblem.feaDesignVariable[i],
                                                astrosInstance[iIndex].feaProblem.numProperty,
                                                astrosInstance[iIndex].feaProblem.feaProperty,
                                                &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Optimization - design variables - geometry
    for( i = 0; i < astrosInstance[iIndex].feaProblem.numDesignVariable; i++) {

        // Geometric parameterization - only if needed
        for (j = 0; j < astrosInstance[iIndex].numGeomIn; j++) {

            status = aim_getName(aimInfo, j+1, GEOMETRYIN, &geomInName);
            if (status != CAPS_SUCCESS) goto cleanup;

            if (strcmp(astrosInstance[iIndex].feaProblem.feaDesignVariable[i].name, geomInName) == 0) break;
        }

        // If name isn't found in Geometry inputs skip write geometric design variables
        if (j >= astrosInstance[iIndex].numGeomIn) continue;

        if(aim_getGeomInType(aimInfo, j+1) == EGADS_OUTSIDE) {
            printf("Error: Geometric sensitivity not available for CFGPMTR = %s\n", geomInName);
            status = CAPS_NOSENSITVTY;
            goto cleanup;
        }

        printf(">>> Writing geometry parametrization\n");
        status = astros_writeGeomParametrization(fp,
                                                 aimInfo,
                                                 astrosInstance[iIndex].feaProblem.numDesignVariable,
                                                 astrosInstance[iIndex].feaProblem.feaDesignVariable,
                                                 astrosInstance[iIndex].numGeomIn,
                                                 astrosInstance[iIndex].geomInVal,
                                                 &astrosInstance[iIndex].feaProblem.feaMesh,
                                                 &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
        printf(">>> Done writing geometry parametrization\n");

        break; // Only need to call astros_writeGeomParametrization once!
    }

    // Optimization - design constraints
    // Move to subcase level because ASTROS does not support DCONADD card--DB 7 Mar 18
    /* for( i = 0; i < astrosInstance[iIndex].feaProblem.numDesignConstraint; i++) {

          if (i == 0) {
              printf("\tWriting design constraints and responses cards\n");
              fprintf(fp,"$\n$ Design constraint(s)\n");
          }

          status = astros_writeDesignConstraintCard(fp,
                                                    &astrosInstance[iIndex].feaProblem.feaDesignConstraint[i],
                                                    astrosInstance[iIndex].feaProblem.numMaterial,
                                                    astrosInstance[iIndex].feaProblem.feaMaterial,
                                                    astrosInstance[iIndex].feaProblem.numProperty,
                                                    astrosInstance[iIndex].feaProblem.feaProperty,
                                                    &astrosInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
      } */

    // Aeroelastic
    if (strcasecmp(analysisType, "Aeroelastic") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrim") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrimOpt") == 0 ) {

        printf("\tWriting aeroelastic cards\n");
        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++){

            status = astros_writeCAeroCard(fp,
                                           &astrosInstance[iIndex].feaProblem.feaAero[i],
                                           &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = astros_checkAirfoil(aimInfo,
                                         &astrosInstance[iIndex].feaProblem.feaAero[i]);
            if (status == CAPS_SOURCEERR) {

                j = (int) false;
                printf("\tBody topology used in aerodynamic surface %d, isn't suitable for airfoil shape, switching to panel", i+1);

            } else if (status != CAPS_SUCCESS) {

                goto cleanup;

            } else {

                j = (int) true;
            }

            status = astros_writeAirfoilCard(fp,
                                             j, // useAirfoilShape
                                             &astrosInstance[iIndex].feaProblem.feaAero[i],
                                             &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = astros_writeAeroData(aimInfo,
                                          fp,
                                          j, // useAirfoilShape
                                          &astrosInstance[iIndex].feaProblem.feaAero[i],
                                          &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = astros_writeAeroSplineCard(fp,
                                                &astrosInstance[iIndex].feaProblem.feaAero[i],
                                                &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = nastran_writeSet1Card(fp,
                                           &astrosInstance[iIndex].feaProblem.feaAero[i],
                                           &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }

    // Aeroelastic
    if (strcasecmp(analysisType, "AeroelasticFlutter") == 0) {

        printf("\tWriting unsteady aeroelastic cards\n");
        for (i = 0; i < astrosInstance[iIndex].feaProblem.numAero; i++){

            status = nastran_writeCAeroCard(fp,
                                            &astrosInstance[iIndex].feaProblem.feaAero[i],
                                            &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = astros_writeAeroSplineCard(fp,
                                                &astrosInstance[iIndex].feaProblem.feaAero[i],
                                                &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = nastran_writeSet1Card(fp,
                                           &astrosInstance[iIndex].feaProblem.feaAero[i],
                                           &astrosInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }

    // Include mesh file
    fprintf(fp,"$\nINCLUDE %s.bdf\n$\n", astrosInstance[iIndex].projectName);

    // End bulk data
    fprintf(fp,"ENDDATA\n");

    fclose(fp);
    fp = NULL;
/*
////////////////////////////////////////
    printf("\n\n\nTESTING OUT READER\n\n");

    // FO6 data variables
    int numGridPoint = 0;
    int numEigenVector = 0;
    double **dataMatrix = NULL;
    filename = (char *) EG_alloc((strlen(astrosInstance[iIndex].analysisPath) + strlen(astrosInstance[iIndex].projectName) +
                                      strlen(".out") + 2)*sizeof(char));

    sprintf(filename,"%s/%s%s", astrosInstance[iIndex].analysisPath,
                                astrosInstance[iIndex].projectName, ".out");

    // Open file
    fp = fopen(filename, "r");
    if (filename != NULL) EG_free(filename);
    filename = NULL;

    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);

        return CAPS_IOERR;
    }

    status = astros_readOUTDisplacement(fp,
                                         -9,
                                         &numGridPoint,
                                         &dataMatrix);

    for ( i = 0 ; i < numGridPoint ; i++) printf("dataMatrix  = %f %f %f %f %f %f %f %f\n",
                                                 dataMatrix[i][0],
                                                 dataMatrix[i][1],
                                                 dataMatrix[i][2],
                                                 dataMatrix[i][3],
                                                 dataMatrix[i][4],
                                                 dataMatrix[i][5],
                                                 dataMatrix[i][6],
                                                 dataMatrix[i][7]);
    fclose(fp);
///////////////////////////////////
*/

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\tPremature exit in astrosAIM preAnalysis, status = %d\n", status);

        if (fp != NULL) fclose(fp);

        return status;
}

// Check that astros ran without errors
int
aimPostAnalysis(/*@unused@*/ int iIndex, /*@unused@*/ void *aimStruc,
                const char *analysisPath, capsErrs **errs)
{
  int status = CAPS_SUCCESS;
  char currentPath[PATH_MAX]; // Current directory path

  char *filename = NULL; // File to open
  char extOUT[] = ".out";
  FILE *fp = NULL; // File pointer

  size_t linecap = 0;
  char *line = NULL; // Temporary line holder
  int withErrors = (int) false;
  int terminated = (int) false;

#ifdef DEBUG
  printf(" astrosAIM/aimPostAnalysis instance = %d!\n", iIndex);
#endif

  *errs = NULL;

  (void) getcwd(currentPath, PATH_MAX); // Get our current path

  // Set path to analysis directory
  if (chdir(analysisPath) != 0) {
      printf(" astrosAIM/aimPostAnalysis Cannot chdir to %s!\n", analysisPath);

      return CAPS_DIRERR;
  }

  filename = (char *) EG_alloc((strlen(astrosInstance[iIndex].projectName) + strlen(extOUT) +1)*sizeof(char));
  if (filename == NULL) return EGADS_MALLOC;

  sprintf(filename, "%s%s", astrosInstance[iIndex].projectName, extOUT);

  fp = fopen(filename, "r");

  EG_free(filename); // Free filename allocation

  chdir(currentPath);

  if (fp == NULL) {
      printf(" astrosAIM/aimPostAnalysis Cannot open Output file!\n");

      return CAPS_IOERR;
  }

  // Scan the file for the string
  while( !feof(fp) ) {

      // Get line from file
      status = getline(&line, &linecap, fp);
      if (status < 0) break;

      if (terminated == (int) false) terminated = (int) (strstr(line, "A S T R O S  T E R M I N A T E D") != NULL);
      if (withErrors == (int) false) withErrors = (int) (strstr(line, "W I T H  E R R O R S") != NULL);
  }
  fclose(fp);
  EG_free(line);
  status = CAPS_SUCCESS;

  if (terminated == (int) false) {
    printf("\n");
    printf("****************************************\n");
    printf("\n");
    printf("ERROR: Astros did not run to termination!\n");
    printf("\n");
    printf("****************************************\n");
    printf("\n");
    status = CAPS_EXECERR;
  }

  if (withErrors == (int) true) {
    printf("\n");
    printf("****************************************\n");
    printf("***                                  ***\n");
    printf("*** A S T R O S  T E R M I N A T E D ***\n");
    printf("***      W I T H  E R R O R S        ***\n");
    printf("***                                  ***\n");
    printf("****************************************\n");
    printf("\n");
    status = CAPS_EXECERR;
  }

  return status;
}

// Set Astros output variables
int aimOutputs(int iIndex, void *aimStruc,int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsAstros AIM Outputs
     * The following list outlines the Astros outputs available through the AIM interface.
     */

    #ifdef DEBUG
        printf(" astrosAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
    #endif

    /*! \page aimOutputsAstros AIM Outputs
     * - <B>EigenValue</B> = List of Eigen-Values (\f$ \lambda\f$) after a modal solve.
     * - <B>EigenRadian</B> = List of Eigen-Values in terms of radians (\f$ \omega = \sqrt{\lambda}\f$ ) after a modal solve.
     * - <B>EigenFrequency</B> = List of Eigen-Values in terms of frequencies (\f$ f = \frac{\omega}{2\pi}\f$) after a modal solve.
     * - <B>EigenGeneralMass</B> = List of generalized masses for the Eigen-Values.
     * - <B>EigenGeneralStiffness</B> = List of generalized stiffness for the Eigen-Values.
     * .
     */

    //printf("***** index=%d *****", index);

    if (index == 1) {
        *aoname = EG_strdup("EigenValue");

    } else if (index == 2) {
        *aoname = EG_strdup("EigenRadian");

    } else if (index == 3) {
        *aoname = EG_strdup("EigenFrequency");

    } else if (index == 4) {
        *aoname = EG_strdup("EigenGeneralMass");

    } else if (index == 5) {
        *aoname = EG_strdup("EigenGeneralStiffness");

    } else if (index == 6) {
        *aoname = EG_strdup("Tmax");

    } else if (index == 7) {
        *aoname = EG_strdup("T1max");

    } else if (index == 8) {
        *aoname = EG_strdup("T2max");

    } else if (index == 9) {
        *aoname = EG_strdup("T3max");
    }

    //printf(" %s\n", *aoname);

    if (index <= 5) {
        form->type          = Double;
        form->units         = NULL;
        form->lfixed        = Change;
        form->sfixed        = Change;
        form->vals.reals = NULL;
        form->vals.real = 0;
    } else {
        form->type = Double;
        form->dim  = Vector;
        form->length  = 1;
        form->nrow  = 1;
        form->ncol  = 1;
        form->units  = NULL;
        form->vals.reals = NULL;
        form->vals.real = 0;
    }

    return CAPS_SUCCESS;
}

// Calculate Astros output
int aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath,
                  int index, capsValue *val, capsErrs **errors)
{
    int status = CAPS_SUCCESS; // Function return status

    int i; //Indexing

    int numEigenVector;
    double **dataMatrix = NULL;

    char currentPath[PATH_MAX]; // Current directory path

    char *filename = NULL; // File to open
    char extOUT[] = ".out";
    FILE *fp = NULL; // File pointer

    (void) getcwd(currentPath, PATH_MAX); // Get our current path

    // Set path to analysis directory
    if (chdir(analysisPath) != 0) {
    #ifdef DEBUG
        printf(" astrosAIM/aimCalcOutput Cannot chdir to %s!\n", analysisPath);
    #endif

        return CAPS_DIRERR;
    }

    filename = (char *) EG_alloc((strlen(astrosInstance[iIndex].projectName) + strlen(extOUT) +1)*sizeof(char));
    if (filename == NULL) return EGADS_MALLOC;

    sprintf(filename, "%s%s", astrosInstance[iIndex].projectName, extOUT);

    fp = fopen(filename, "r");

    EG_free(filename); // Free filename allocation

    if (fp == NULL) {
        #ifdef DEBUG
            printf(" astrosAIM/aimCalcOutput Cannot open Output file!\n");
        #endif

        chdir(currentPath);

        return CAPS_IOERR;
    }

    if (index <= 5) {

        status = astros_readOUTEigenValue(fp, &numEigenVector, &dataMatrix);
        if (status == CAPS_SUCCESS) {

            val->nrow = numEigenVector;
            val->ncol = 1;
            val->length = val->nrow*val->ncol;
            if (val->length == 1) val->dim = Scalar;
            else val->dim = Vector;

            if (val->length == 1) {
                val->vals.real = dataMatrix[0][index-1];
            } else {

                val->vals.reals = (double *) EG_alloc(val->length*sizeof(double));
                if (val->vals.reals != NULL) {

                    for (i = 0; i < val->length; i++) {
                        val->vals.reals[i] = dataMatrix[i][index-1];
                    }

                } else status = EGADS_MALLOC;
            }
        }

        if (dataMatrix != NULL) {
            for (i = 0; i < numEigenVector; i++) {
                if (dataMatrix[i] != NULL) EG_free(dataMatrix[i]);
            }
            EG_free(dataMatrix);
        }

    } else if (index <= 9) {
        double T1max=0, T2max=0, T3max=0, Tmax=0, TT;
        int ipnt, numGridPoint;

        status = astros_readOUTDisplacement(fp, -1, &numGridPoint, &dataMatrix);
        if (status == CAPS_SUCCESS) {
            val->dim  = Scalar;
            val->nrow = 1;
            val->ncol = 1;
            val->length = val->nrow * val->ncol;

            for (ipnt = 0; ipnt < numGridPoint; ipnt++) {
                TT = sqrt(pow(dataMatrix[ipnt][2], 2)
                        + pow(dataMatrix[ipnt][3], 2)
                        + pow(dataMatrix[ipnt][4], 2));

                if (fabs(dataMatrix[ipnt][2]) > T1max) T1max = fabs(dataMatrix[ipnt][2]);
                if (fabs(dataMatrix[ipnt][3]) > T2max) T2max = fabs(dataMatrix[ipnt][3]);
                if (fabs(dataMatrix[ipnt][4]) > T3max) T3max = fabs(dataMatrix[ipnt][4]);
                if (TT                        > Tmax ) Tmax  = TT;
            }

            if        (index == 6) {
                val->vals.real = Tmax;
            } else if (index == 7) {
                val->vals.real = T1max;
            } else if (index == 8) {
                val->vals.real = T2max;
            } else {
                val->vals.real = T3max;
            }
        }

        if (dataMatrix != NULL) {
            for (i = 0; i < numGridPoint; i++) {
                if (dataMatrix[i] != NULL) EG_free(dataMatrix[i]);
            }
            EG_free(dataMatrix);
        }
    }

    if (fp != NULL) fclose(fp);

    // Restore the path we came in with
    chdir(currentPath);

    return status;
}

void aimCleanup()
{
    int iIndex; // Indexing

    int status; // Returning status

    #ifdef DEBUG
        printf(" astrosAIM/Cleanup!\n");
    #endif


    if (numInstance != 0) {

        // Clean up astrosInstance data
        for ( iIndex = 0; iIndex < numInstance; iIndex++) {

            printf(" Cleaning up astrosInstance - %d\n", iIndex);

            status = destroy_aimStorage(iIndex);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during clean up of instance %d\n", status, iIndex);

        }

    }
    numInstance = 0;

    if (astrosInstance != NULL) EG_free(astrosInstance);
    astrosInstance = NULL;

}

int aimFreeDiscr(capsDiscr *discr)
{
    int i; // Indexing

    #ifdef DEBUG
        printf(" astrosAIM/aimFreeDiscr instance = %d!\n", discr->instance);
    #endif


    // Free up this capsDiscr

    discr->nPoints = 0; // Points

    if (discr->mapping != NULL) EG_free(discr->mapping);
    discr->mapping = NULL;

    if (discr->types != NULL) { // Element types
        for (i = 0; i < discr->nTypes; i++) {
            if (discr->types[i].gst  != NULL) EG_free(discr->types[i].gst);
            if (discr->types[i].tris != NULL) EG_free(discr->types[i].tris);
        }

        EG_free(discr->types);
    }

    discr->nTypes  = 0;
    discr->types   = NULL;

    if (discr->elems != NULL) { // Element connectivity

        for (i = 0; i < discr->nElems; i++) {
              if (discr->elems[i].gIndices != NULL) EG_free(discr->elems[i].gIndices);
        }

        EG_free(discr->elems);
    }

    discr->nElems  = 0;
    discr->elems   = NULL;

    if (discr->ptrm != NULL) EG_free(discr->ptrm); // Extra information to store into the discr void pointer
    discr->ptrm    = NULL;


    discr->nVerts = 0;    // Not currently used
    discr->verts  = NULL; // Not currently used
    discr->celem = NULL; // Not currently used

    discr->nDtris = 0; // Not currently used
    discr->dtris  = NULL; // Not currently used


    return CAPS_SUCCESS;
}

int aimDiscr(char *tname, capsDiscr *discr) {

    int i, j, body, face, counter; // Indexing

    int status; // Function return status

    int iIndex; // Instance index

    int numBody;

    // EGADS objects
    ego tess, *bodies = NULL, *faces = NULL, tempBody;

    const char *intents, *string = NULL, *capsGroup = NULL; // capsGroups strings

    // EGADS function returns
    int plen, tlen, qlen;
    int atype, alen;
    const int    *ptype, *pindex, *tris, *nei, *ints;
    const double *xyz, *uv, *reals;

    // Body Tessellation
    int numFace = 0;
    int numFaceFound = 0;
    int numPoint = 0, numTri = 0, numQuad = 0, numGlobalPoint = 0;
    int *bodyFaceMap = NULL; // size=[2*numFaceFound], [2*numFaceFound + 0] = body, [2*numFaceFoun + 1] = face

    int *globalID = NULL, *localStitchedID = NULL, gID = 0;

    int *storage= NULL; // Extra information to store into the discr void pointer

    int numCAPSGroup = 0, attrIndex = 0, foundAttr = (int) false;
    int *capsGroupList = NULL;
    int dataTransferBodyIndex=-99;

    int numElem, stride, gsize, tindex;

    // Quading variables
    int quad = (int)false;
    int patch;
    int numPatch, n1, n2;
    const int *pvindex = NULL, *pbounds = NULL;

    iIndex = discr->instance;
    #ifdef DEBUG
        printf(" astrosAIM/aimDiscr: tname = %s, instance = %d!\n", tname, iIndex);
    #endif

    if ((iIndex < 0) || (iIndex >= numInstance)) return CAPS_BADINDEX;


    if (tname == NULL) return CAPS_NOTFOUND;

    /*if (astrosInstance[iIndex].dataTransferCheck == (int) false) {
        printf("The volume is not suitable for data transfer - possibly the volume mesher "
                "added unaccounted for points\n");
        return CAPS_BADVALUE;
    }*/

    // Currently this ONLY works if the capsTranfer lives on single body!
    status = aim_getBodies(discr->aInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
        printf(" astrosAIM/aimDiscr: %d aim_getBodies = %d!\n", iIndex, status);
        return status;
    }

    status = aimFreeDiscr(discr);
    if (status != CAPS_SUCCESS) return status;

    // Check and generate/retrieve the mesh
    status = checkAndCreateMesh(iIndex, discr->aInfo);
    if (status != CAPS_SUCCESS) goto cleanup;


    numFaceFound = 0;
    numPoint = numTri = numQuad = 0;
    // Find any faces with our boundary marker and get how many points and triangles there are
    for (body = 0; body < numBody; body++) {

        status = EG_getBodyTopos(bodies[body], NULL, FACE, &numFace, &faces);
        if (status != EGADS_SUCCESS) {
            printf("astrosAIM: getBodyTopos (Face) = %d for Body %d!\n", status, body);
            return status;
        }

        tess = bodies[body + numBody];
        if (tess == NULL) continue;

        quad = (int)false;
        status = EG_attributeRet(tess, ".tessType", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS)
          if (atype == ATTRSTRING)
            if (strcmp(string, "Quad") == 0)
              quad = (int)true;

        for (face = 0; face < numFace; face++) {

            // Retrieve the string following a capsBound tag
            status = retrieve_CAPSBoundAttr(faces[face], &string);
            if (status != CAPS_SUCCESS) continue;
            if (strcmp(string, tname) != 0) continue;

            status = retrieve_CAPSIgnoreAttr(faces[face], &string);
            if (status == CAPS_SUCCESS) {
              printf("astrosAIM: WARNING: capsIgnore found on bound %s\n", tname);
              continue;
            }

            #ifdef DEBUG
                printf(" astrosAIM/aimDiscr: Body %d/Face %d matches %s!\n", body, face+1, tname);
            #endif

            status = retrieve_CAPSGroupAttr(faces[face], &capsGroup);
            if (status != CAPS_SUCCESS) {
                printf("capsBound found on face %d, but no capGroup found!!!\n", face);
                continue;
            } else {

                status = get_mapAttrToIndexIndex(&astrosInstance[iIndex].attrMap, capsGroup, &attrIndex);
                if (status != CAPS_SUCCESS) {
                    printf("capsGroup %s NOT found in attrMap\n",capsGroup);
                    continue;
                } else {

                    // If first index create arrays and store index
                    if (numCAPSGroup == 0) {
                        numCAPSGroup += 1;
                        capsGroupList = (int *) EG_alloc(numCAPSGroup*sizeof(int));
                        if (capsGroupList == NULL) {
                            status =  EGADS_MALLOC;
                            goto cleanup;
                        }

                        capsGroupList[numCAPSGroup-1] = attrIndex;
                    } else { // If we already have an index(es) let make sure it is unique
                        foundAttr = (int) false;
                        for (i = 0; i < numCAPSGroup; i++) {
                            if (attrIndex == capsGroupList[i]) {
                                foundAttr = (int) true;
                                break;
                            }
                        }

                        if (foundAttr == (int) false) {
                            numCAPSGroup += 1;
                            capsGroupList = (int *) EG_reall(capsGroupList, numCAPSGroup*sizeof(int));
                            if (capsGroupList == NULL) {
                                status =  EGADS_MALLOC;
                                goto cleanup;
                            }

                            capsGroupList[numCAPSGroup-1] = attrIndex;
                        }
                    }
                }
            }

            numFaceFound += 1;
            dataTransferBodyIndex = body;
            bodyFaceMap = (int *) EG_reall(bodyFaceMap, 2*numFaceFound*sizeof(int));
            if (bodyFaceMap == NULL) { status = EGADS_MALLOC; goto cleanup; }

            // Get number of points and triangles
            bodyFaceMap[2*(numFaceFound-1) + 0] = body+1;
            bodyFaceMap[2*(numFaceFound-1) + 1] = face+1;

            // count Quads/triangles
            status = EG_getQuads(bodies[body+numBody], face+1, &qlen, &xyz, &uv, &ptype, &pindex, &numPatch);
            if (status == EGADS_SUCCESS && numPatch != 0) {

              // Sum number of points and quads
              numPoint  += qlen;

              for (patch = 1; patch <= numPatch; patch++) {
                status = EG_getPatch(bodies[body+numBody], face+1, patch, &n1, &n2, &pvindex, &pbounds);
                if (status != EGADS_SUCCESS) goto cleanup;
                numQuad += (n1-1)*(n2-1);
              }
            } else {
                // Get face tessellation
                status = EG_getTessFace(bodies[body+numBody], face+1, &plen, &xyz, &uv, &ptype, &pindex, &tlen, &tris, &nei);
                if (status != EGADS_SUCCESS) {
                    printf(" astrosAIM: EG_getTessFace %d = %d for Body %d!\n", face+1, status, body+1);
                    continue;
                }

                // Sum number of points and triangles
                numPoint += plen;
                if (quad == (int)true)
                    numQuad += tlen/2;
                else
                    numTri  += tlen;
            }
        }

        EG_free(faces); faces = NULL;

        if (dataTransferBodyIndex >=0) break; // Force that only one body can be used
    }

    if (numFaceFound == 0) {
        printf(" astrosAIM/aimDiscr: No Faces match %s!\n", tname);
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Debug
#ifdef DEBUG
    printf(" astrosAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
    printf(" astrosAIM/aimDiscr: nquad = %d, npts = %d!\n", numQuad, numPoint);
#endif

    if ( numPoint == 0 || (numTri == 0 && numQuad == 0) ) {
#ifdef DEBUG
      printf(" astrosAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
      printf(" astrosAIM/aimDiscr: nquad = %d, npts = %d!\n", numQuad, numPoint);
#endif
      status = CAPS_SOURCEERR;
      goto cleanup;
    }

#ifdef DEBUG
    printf(" astrosAIM/aimDiscr: Instance %d, Body Index for data transfer = %d\n", iIndex, dataTransferBodyIndex);
#endif

    // Specify our element type
    status = EGADS_MALLOC;
    discr->nTypes = 2;

    discr->types  = (capsEleType *) EG_alloc(discr->nTypes* sizeof(capsEleType));
    if (discr->types == NULL) goto cleanup;

    // Define triangle element topology
    discr->types[0].nref  = 3;
    discr->types[0].ndata = 0;            /* data at geom reference positions */
    discr->types[0].ntri  = 1;
    discr->types[0].nmat  = 0;            /* match points at geom ref positions */
    discr->types[0].tris  = NULL;
    discr->types[0].gst   = NULL;
    discr->types[0].dst   = NULL;
    discr->types[0].matst = NULL;

    discr->types[0].tris   = (int *) EG_alloc(3*sizeof(int));
    if (discr->types[0].tris == NULL) goto cleanup;
    discr->types[0].tris[0] = 1;
    discr->types[0].tris[1] = 2;
    discr->types[0].tris[2] = 3;

    discr->types[0].gst   = (double *) EG_alloc(6*sizeof(double));
    if (discr->types[0].gst == NULL) goto cleanup;
    discr->types[0].gst[0] = 0.0;
    discr->types[0].gst[1] = 0.0;
    discr->types[0].gst[2] = 1.0;
    discr->types[0].gst[3] = 0.0;
    discr->types[0].gst[4] = 0.0;
    discr->types[0].gst[5] = 1.0;

    // Define quad element type
    discr->types[1].nref  = 4;
    discr->types[1].ndata = 0;            /* data at geom reference positions */
    discr->types[1].ntri  = 2;
    discr->types[1].nmat  = 0;            /* match points at geom ref positions */
    discr->types[1].tris  = NULL;
    discr->types[1].gst   = NULL;
    discr->types[1].dst   = NULL;
    discr->types[1].matst = NULL;

    discr->types[1].tris   = (int *) EG_alloc(3*discr->types[1].ntri*sizeof(int));
    if (discr->types[1].tris == NULL) goto cleanup;
    discr->types[1].tris[0] = 1;
    discr->types[1].tris[1] = 2;
    discr->types[1].tris[2] = 3;

    discr->types[1].tris[3] = 1;
    discr->types[1].tris[4] = 3;
    discr->types[1].tris[5] = 4;

    discr->types[1].gst   = (double *) EG_alloc(2*discr->types[1].nref*sizeof(double));
    if (discr->types[1].gst == NULL) goto cleanup;
    discr->types[1].gst[0] = 0.0;
    discr->types[1].gst[1] = 0.0;
    discr->types[1].gst[2] = 1.0;
    discr->types[1].gst[3] = 0.0;
    discr->types[1].gst[4] = 1.0;
    discr->types[1].gst[5] = 1.0;
    discr->types[1].gst[6] = 0.0;
    discr->types[1].gst[7] = 1.0;

    // Get the tessellation and make up a simple linear continuous triangle discretization */

    discr->nElems = numTri + numQuad;

    discr->elems = (capsElement *) EG_alloc(discr->nElems*sizeof(capsElement));
    if (discr->elems == NULL) { status = EGADS_MALLOC; goto cleanup; }

    discr->mapping = (int *) EG_alloc(2*numPoint*sizeof(int)); // Will be resized
    if (discr->mapping == NULL) { status = EGADS_MALLOC; goto cleanup; }

    globalID = (int *) EG_alloc(numPoint*sizeof(int));
    if (globalID == NULL) { status = EGADS_MALLOC; goto cleanup; }

    numPoint = 0;
    numTri = 0;
    numQuad = 0;

    for (face = 0; face < numFaceFound; face++){

        tess = bodies[bodyFaceMap[2*face + 0]-1 + numBody];

        quad = (int)false;
        status = EG_attributeRet(tess, ".tessType", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS)
          if (atype == ATTRSTRING)
            if (strcmp(string, "Quad") == 0)
              quad = (int)true;

        if (localStitchedID == NULL) {
            status = EG_statusTessBody(tess, &tempBody, &i, &numGlobalPoint);
            if (status != EGADS_SUCCESS) goto cleanup;

            localStitchedID = (int *) EG_alloc(numGlobalPoint*sizeof(int));
            if (localStitchedID == NULL) { status = EGADS_MALLOC; goto cleanup; }

            for (i = 0; i < numGlobalPoint; i++) localStitchedID[i] = 0;
        }

        // Get face tessellation
        status = EG_getTessFace(tess, bodyFaceMap[2*face + 1], &plen, &xyz, &uv, &ptype, &pindex, &tlen, &tris, &nei);
        if (status != EGADS_SUCCESS) {
            printf(" astrosAIM: EG_getTessFace %d = %d for Body %d!\n", bodyFaceMap[2*face + 1], status, bodyFaceMap[2*face + 0]);
            continue;
        }

        for (i = 0; i < plen; i++ ) {

            status = EG_localToGlobal(tess, bodyFaceMap[2*face+1], i+1, &gID);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (localStitchedID[gID -1] != 0) continue;

            discr->mapping[2*numPoint  ] = bodyFaceMap[2*face + 0];
            discr->mapping[2*numPoint+1] = gID;

            localStitchedID[gID -1] = numPoint+1;

            globalID[numPoint] = gID;

            numPoint += 1;
        }

        // Attempt to retrieve quad information
        status = EG_getQuads(tess, bodyFaceMap[2*face + 1], &i, &xyz, &uv, &ptype, &pindex, &numPatch);
        if (status == EGADS_SUCCESS && numPatch != 0) {

            if (numPatch != 1) {
                status = CAPS_NOTIMPLEMENT;
                printf("astrosAIM/aimDiscr: EG_localToGlobal accidentally only works for a single quad patch! FIXME!\n");
                goto cleanup;
            }

            counter = 0;
            for (patch = 1; patch <= numPatch; patch++) {

                status = EG_getPatch(tess, bodyFaceMap[2*face + 1], patch, &n1, &n2, &pvindex, &pbounds);
                if (status != EGADS_SUCCESS) goto cleanup;

                for (j = 1; j < n2; j++) {
                    for (i = 1; i < n1; i++) {

                        discr->elems[numQuad+numTri].bIndex = bodyFaceMap[2*face + 0];
                        discr->elems[numQuad+numTri].tIndex = 2;
                        discr->elems[numQuad+numTri].eIndex = bodyFaceMap[2*face + 1];

                        discr->elems[numQuad+numTri].gIndices = (int *) EG_alloc(8*sizeof(int));
                        if (discr->elems[numQuad+numTri].gIndices == NULL) { status = EGADS_MALLOC; goto cleanup; }

                        discr->elems[numQuad+numTri].dIndices    = NULL;
                        //discr->elems[numQuad+numTri].eTris.tq[0] = (numQuad*2 + numTri) + 1;
                        //discr->elems[numQuad+numTri].eTris.tq[1] = (numQuad*2 + numTri) + 2;

                        discr->elems[numQuad+numTri].eTris.tq[0] = counter*2 + 1;
                        discr->elems[numQuad+numTri].eTris.tq[1] = counter*2 + 2;

                        status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], pvindex[(i-1)+n1*(j-1)], &gID);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        discr->elems[numQuad+numTri].gIndices[0] = localStitchedID[gID-1];
                        discr->elems[numQuad+numTri].gIndices[1] = pvindex[(i-1)+n1*(j-1)];

                        status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], pvindex[(i  )+n1*(j-1)], &gID);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        discr->elems[numQuad+numTri].gIndices[2] = localStitchedID[gID-1];
                        discr->elems[numQuad+numTri].gIndices[3] = pvindex[(i  )+n1*(j-1)];

                        status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], pvindex[(i  )+n1*(j  )], &gID);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        discr->elems[numQuad+numTri].gIndices[4] = localStitchedID[gID-1];
                        discr->elems[numQuad+numTri].gIndices[5] = pvindex[(i  )+n1*(j  )];

                        status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], pvindex[(i-1)+n1*(j  )], &gID);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        discr->elems[numQuad+numTri].gIndices[6] = localStitchedID[gID-1];
                        discr->elems[numQuad+numTri].gIndices[7] = pvindex[(i-1)+n1*(j  )];

//                        printf("Quad %d, GIndice = %d %d %d %d %d %d %d %d\n", numQuad+numTri,
//                                                                               discr->elems[numQuad+numTri].gIndices[0],
//                                                                               discr->elems[numQuad+numTri].gIndices[1],
//                                                                               discr->elems[numQuad+numTri].gIndices[2],
//                                                                               discr->elems[numQuad+numTri].gIndices[3],
//                                                                               discr->elems[numQuad+numTri].gIndices[4],
//                                                                               discr->elems[numQuad+numTri].gIndices[5],
//                                                                               discr->elems[numQuad+numTri].gIndices[6],
//                                                                               discr->elems[numQuad+numTri].gIndices[7]);

                        numQuad += 1;
                        counter += 1;
                    }
                }
            }

        } else {

            if (quad == (int)true) {
                numElem = tlen/2;
                stride = 6;
                gsize = 8;
                tindex = 2;
            } else {
                numElem = tlen;
                stride = 3;
                gsize = 6;
                tindex = 1;
            }

            // Get triangle/quad connectivity in global sense
            for (i = 0; i < numElem; i++) {

                discr->elems[numQuad+numTri].bIndex      = bodyFaceMap[2*face + 0];
                discr->elems[numQuad+numTri].tIndex      = tindex;
                discr->elems[numQuad+numTri].eIndex      = bodyFaceMap[2*face + 1];

                discr->elems[numQuad+numTri].gIndices    = (int *) EG_alloc(gsize*sizeof(int));
                if (discr->elems[numQuad+numTri].gIndices == NULL) { status = EGADS_MALLOC; goto cleanup; }

                discr->elems[numQuad+numTri].dIndices    = NULL;

                if (quad == (int)true) {
                    discr->elems[numQuad+numTri].eTris.tq[0] = i*2 + 1;
                    discr->elems[numQuad+numTri].eTris.tq[1] = i*2 + 2;
                } else {
                    discr->elems[numQuad+numTri].eTris.tq[0] = i + 1;
                }

                status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], tris[stride*i + 0], &gID);
                if (status != EGADS_SUCCESS) goto cleanup;

                discr->elems[numQuad+numTri].gIndices[0] = localStitchedID[gID-1];
                discr->elems[numQuad+numTri].gIndices[1] = tris[stride*i + 0];

                status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], tris[stride*i + 1], &gID);
                if (status != EGADS_SUCCESS) goto cleanup;

                discr->elems[numQuad+numTri].gIndices[2] = localStitchedID[gID-1];
                discr->elems[numQuad+numTri].gIndices[3] = tris[stride*i + 1];

                status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], tris[stride*i + 2], &gID);
                if (status != EGADS_SUCCESS) goto cleanup;

                discr->elems[numQuad+numTri].gIndices[4] = localStitchedID[gID-1];
                discr->elems[numQuad+numTri].gIndices[5] = tris[stride*i + 2];

                if (quad == (int)true) {
                    status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], tris[stride*i + 5], &gID);
                    if (status != EGADS_SUCCESS) goto cleanup;

                    discr->elems[numQuad+numTri].gIndices[6] = localStitchedID[gID-1];
                    discr->elems[numQuad+numTri].gIndices[7] = tris[stride*i + 5];
                }

                if (quad == (int)true) {
                    numQuad += 1;
                } else {
                    numTri += 1;
                }
            }
        }
    }

    discr->nPoints = numPoint;

#ifdef DEBUG
    printf(" astrosAIM/aimDiscr: ntris = %d, npts = %d!\n", discr->nElems, discr->nPoints);
#endif

    // Resize mapping to stitched together number of points
    discr->mapping = (int *) EG_reall(discr->mapping, 2*numPoint*sizeof(int));

    // Local to global node connectivity + numCAPSGroup + sizeof(capGrouplist)
    storage  = (int *) EG_alloc((numPoint + 1 + numCAPSGroup) *sizeof(int));
    if (storage == NULL) goto cleanup;
    discr->ptrm = storage;

    // Store the global node id
    for (i = 0; i < numPoint; i++) {

        storage[i] = globalID[i];

        //#ifdef DEBUG
        //    printf(" astrosAIM/aimDiscr: Instance = %d, Global Node ID %d\n", iIndex, storage[i]);
        //#endif
    }

    // Save way the attrMap capsGroup list
    storage[numPoint] = numCAPSGroup;
    for (i = 0; i < numCAPSGroup; i++) {
        storage[numPoint+1+i] = capsGroupList[i];
    }

    #ifdef DEBUG
        printf(" astrosAIM/aimDiscr: Instance = %d, Finished!!\n", iIndex);
    #endif

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\tPremature exit in astrosAIM aimDiscr, status = %d\n", status);

        EG_free(faces);

        EG_free(globalID);
        EG_free(localStitchedID);

        EG_free(capsGroupList);
        EG_free(bodyFaceMap);

        if (status != CAPS_SUCCESS) {
            aimFreeDiscr(discr);
        }

        return status;
}

static int invEvaluationQuad(double uvs[], // (in) uvs that support the geom (2*npts in length) */
                             double uv[],  // (in) the uv position to get st */
                             int    in[],  // (in) grid indices
                             double st[])  // (out) weighting
{

    int    i;
    double idet, delta, d, du[2], dv[2], duv[2], dst[2], uvx[2];

    delta  = 100.0;
    for (i = 0; i < 20; i++) {
        uvx[0] = (1.0-st[0])*((1.0-st[1])*uvs[2*in[0]  ]  +
                                   st[1] *uvs[2*in[3]  ]) +
                      st[0] *((1.0-st[1])*uvs[2*in[1]  ]  +
                                   st[1] *uvs[2*in[2]  ]);
        uvx[1] = (1.0-st[0])*((1.0-st[1])*uvs[2*in[0]+1]  +
                                   st[1] *uvs[2*in[3]+1]) +
                      st[0] *((1.0-st[1])*uvs[2*in[1]+1]  +
                                   st[1] *uvs[2*in[2]+1]);
        du[0]  = (1.0-st[1])*(uvs[2*in[1]  ] - uvs[2*in[0]  ]) +
                      st[1] *(uvs[2*in[2]  ] - uvs[2*in[3]  ]);
        du[1]  = (1.0-st[0])*(uvs[2*in[3]  ] - uvs[2*in[0]  ]) +
                      st[0] *(uvs[2*in[2]  ] - uvs[2*in[1]  ]);
        dv[0]  = (1.0-st[1])*(uvs[2*in[1]+1] - uvs[2*in[0]+1]) +
                      st[1] *(uvs[2*in[2]+1] - uvs[2*in[3]+1]);
        dv[1]  = (1.0-st[0])*(uvs[2*in[3]+1] - uvs[2*in[0]+1]) +
                      st[0] *(uvs[2*in[2]+1] - uvs[2*in[1]+1]);
        duv[0] = uv[0] - uvx[0];
        duv[1] = uv[1] - uvx[1];
        idet   = du[0]*dv[1] - du[1]*dv[0];
        if (idet == 0.0) break;
        dst[0] = (dv[1]*duv[0] - du[1]*duv[1])/idet;
        dst[1] = (du[0]*duv[1] - dv[0]*duv[0])/idet;
        d      = sqrt(dst[0]*dst[0] + dst[1]*dst[1]);
        if (d >= delta) break;
        delta  = d;
        st[0] += dst[0];
        st[1] += dst[1];
        if (delta < 1.e-8) break;
    }

    if (delta < 1.e-8) return CAPS_SUCCESS;

    return CAPS_NOTFOUND;
}

int aimLocateElement(capsDiscr *discr, double *params, double *param, int *eIndex,
                     double *bary)
{
    int status; // Function return status
    int    i, smallWeightIndex, in[4];
    double weight[3], weightTemp, smallWeight = -1.e300;

    int triangleIndex = 0;

    if (discr == NULL) return CAPS_NULLOBJ;

    smallWeightIndex = 0;
    for (i = 0; i < discr->nElems; i++) {

        if (discr->types[discr->elems[i].tIndex-1].nref == 3) { // Linear triangle

            in[0] = discr->elems[i].gIndices[0] - 1;
            in[1] = discr->elems[i].gIndices[2] - 1;
            in[2] = discr->elems[i].gIndices[4] - 1;
            status  = EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]], param, weight);

            if (status == EGADS_SUCCESS) {
                //printf("First Triangle Exact\n");
                *eIndex = i+1;
                bary[0] = weight[1];
                bary[1] = weight[2];
                return CAPS_SUCCESS;
            }

            weightTemp = weight[0];
            if (weight[1] < weightTemp) weightTemp = weight[1];
            if (weight[2] < weightTemp) weightTemp = weight[2];

            if (weightTemp > smallWeight) {

                smallWeightIndex = i+1;
                smallWeight = weightTemp;
            }

        } else if (discr->types[discr->elems[i].tIndex-1].nref == 4) {// Linear quad

            in[0] = discr->elems[i].gIndices[0] - 1;
            in[1] = discr->elems[i].gIndices[2] - 1;
            in[2] = discr->elems[i].gIndices[4] - 1;
            in[3] = discr->elems[i].gIndices[6] - 1;

            // First triangle
            status = EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]], param, weight);
            if (status == EGADS_SUCCESS) {

                weight[0] = weight[1];
                weight[1] = weight[2];
                (void) invEvaluationQuad(params, param, in, weight);

                *eIndex = i+1;
                bary[0] = weight[0];
                bary[1] = weight[1];
                return CAPS_SUCCESS;
            }

            weightTemp = weight[0];
            if (weight[1] < weightTemp) weightTemp = weight[1];
            if (weight[2] < weightTemp) weightTemp = weight[2];

            if (weightTemp > smallWeight) {

                smallWeightIndex = i+1;
                smallWeight = weightTemp;
                triangleIndex = 0;
            }

            // Second triangle
            status = EG_inTriExact(&params[2*in[0]], &params[2*in[2]], &params[2*in[3]], param, weight);
            if (status == EGADS_SUCCESS) {

                weight[0] = weight[1];
                weight[1] = weight[2];
                (void) invEvaluationQuad(params, param, in, weight);

                *eIndex = i+1;
                bary[0] = weight[0];
                bary[1] = weight[1];
                return CAPS_SUCCESS;
            }

            weightTemp = weight[0];
            if (weight[1] < weightTemp) weightTemp = weight[1];
            if (weight[2] < weightTemp) weightTemp = weight[2];

            if (weightTemp > smallWeight) {

                smallWeightIndex = i+1;
                smallWeight = weightTemp;
                triangleIndex = 1;
            }
        }
    }

    /* must extrapolate! */
    if (smallWeightIndex == 0) return CAPS_NOTFOUND;

    if (discr->types[discr->elems[smallWeightIndex-1].tIndex-1].nref == 4) {

        in[0] = discr->elems[smallWeightIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[smallWeightIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[smallWeightIndex-1].gIndices[4] - 1;
        in[3] = discr->elems[smallWeightIndex-1].gIndices[6] - 1;

        if (triangleIndex == 0) {

            (void) EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]], param, weight);

            weight[0] = weight[1];
            weight[1] = weight[2];

        } else {

            (void) EG_inTriExact(&params[2*in[0]], &params[2*in[2]], &params[2*in[3]], param, weight);

            weight[0] = weight[1];
            weight[1] = weight[2];
        }

        (void) invEvaluationQuad(params, param, in, weight);

        *eIndex = smallWeightIndex;
        bary[0] = weight[0];
        bary[1] = weight[1];

    } else {

        in[0] = discr->elems[smallWeightIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[smallWeightIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[smallWeightIndex-1].gIndices[4] - 1;

        (void) EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]], param, weight);

        *eIndex = smallWeightIndex;
        bary[0] = weight[1];
        bary[1] = weight[2];
    }

    /*
        printf(" aimLocateElement: extropolate to %d (%lf %lf %lf)  %lf\n", ismall, we[0], we[1], we[2], smallWeight);
     */
    return CAPS_SUCCESS;
}

int aimUsesDataSet(int inst, void *aimInfo, const char *bname,
                   const char *dname, enum capsdMethod dMethod)
{
  /*! \page aimUsesDataSetAstros data sets consumed by Astros
   *
   * This function checks if a data set name can be consumed by this aim.
   * The MYSTRAN aim can consume "Pressure" data sets for areolastic analysis.
   */

  if (strcasecmp(dname, "Pressure") == 0) {
      return CAPS_SUCCESS;
  }

  return CAPS_NOTNEEDED;
}

int aimTransfer(capsDiscr *discr, const char *dataName, int numPoint, int dataRank, double *dataVal, char **units)
{

    /*! \page dataTransferAstros Astros Data Transfer
     *
     * The Astros AIM has the ability to transfer displacements and eigenvectors from the AIM and pressure
     * distributions to the AIM using the conservative and interpolative data transfer schemes in CAPS.
     * Currently these transfers may only take place on triangular meshes.
     *
     * \section dataFromAstros Data transfer from Astros
     *
     * <ul>
     *  <li> <B>"Displacement"</B> </li> <br>
     *   Retrieves nodal displacements from the *.out file
     * </ul>
     *
     * <ul>
     *  <li> <B>"EigenVector_#"</B> </li> <br>
     *   Retrieves modal eigen-vectors from the *.out file, where "#" should be replaced by the
     *   corresponding mode number for the eigen-vector (e.g. EigenVector_3 would correspond to the third mode,
     *   while EigenVector_6 would be the sixth mode).
     * </ul>
     *
     * \section dataToAstros Data transfer to Astros
     * <ul>
     *  <li> <B>"Pressure"</B> </li> <br>
     *  Writes appropriate load cards using the provided pressure distribution.
     * </ul>
     *
     */

    int status; // Function return status
    int i, j, dataPoint; // Indexing


    // Current directory path
    char currentPath[PATH_MAX];

    char *extOUT = ".out";

    // FO6 data variables
    int numGridPoint = 0;
    int numEigenVector = 0;

    double **dataMatrix = NULL;

    // Specific EigenVector to use
    int eigenVectorIndex = 0;

    // Variables used in global node mapping
    int *nodeMap, *storage;
    int globalNodeID;


    // Filename stuff
    char *filename = NULL;
    FILE *fp; // File pointer

    #ifdef DEBUG
        printf(" astrosAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n", dataName, discr->instance, numPoint, dataRank);
    #endif

    //Get the appropriate parts of the tessellation to data
    storage = (int *) discr->ptrm;
    nodeMap = &storage[0]; // Global indexing on the body

    //capsGroupList = &storage[discr->nPoints]; // List of boundary ID (attrMap) in the transfer

    if (strcasecmp(dataName, "Displacement") != 0 &&
        strncmp(dataName, "EigenVector", 11) != 0) {

        printf("Unrecognized data transfer variable - %s\n", dataName);
        return CAPS_NOTFOUND;
    }

    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(astrosInstance[discr->instance].analysisPath) != 0) {
        #ifdef DEBUG
            printf(" astrosAIM/aimTransfer Cannot chdir to %s!\n", astrosInstance[discr->instance].analysisPath);
        #endif

        return CAPS_DIRERR;
    }

    filename = (char *) EG_alloc((strlen(astrosInstance[discr->instance].projectName) +
                                  strlen(extOUT) + 1)*sizeof(char));

    sprintf(filename,"%s%s",astrosInstance[discr->instance].projectName,extOUT);

    // Open file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        chdir(currentPath);

        if (filename != NULL) EG_free(filename);
        return CAPS_IOERR;
    }

    if (filename != NULL) EG_free(filename);
    filename = NULL;

    if (strcasecmp(dataName, "Displacement") == 0) {

        if (dataRank != 3) {

            printf("Invalid rank for dataName \"%s\" - excepted a rank of 3!!!\n", dataName);
            status = CAPS_BADRANK;

        } else {

            status = astros_readOUTDisplacement(fp,
                                                 -1,
                                                 &numGridPoint,
                                                 &dataMatrix);
            fclose(fp);
        }

    } else if (strncmp(dataName, "EigenVector", 11) == 0) {

        // Which EigenVector do we want ?
        for (i = 0; i < strlen(dataName); i++) {
            if (dataName[i] == '_' ) break;
        }

        if (i == strlen(dataName)) {
            eigenVectorIndex = 1;
        } else {

            status = sscanf(dataName, "EigenVector_%d", &eigenVectorIndex);
            if (status != 1) {
                printf("Unable to determine which EigenVector to use - Defaulting the first EigenVector!!!\n");
                eigenVectorIndex = 1;
            }
        }

        if (dataRank != 3) {

            printf("Invalid rank for dataName \"%s\" - excepted a rank of 3!!!\n", dataName);
            status = CAPS_BADRANK;

        } else {

            status = astros_readOUTEigenVector(fp,
                                               &numEigenVector,
                                               &numGridPoint,
                                               &dataMatrix);
        }

        fclose(fp);

    } else {

        status = CAPS_NOTFOUND;
    }

    // Restore the path we came in
    chdir(currentPath);
    if (status != CAPS_SUCCESS) return status;


    // Check EigenVector range
    if (strncmp(dataName, "EigenVector", 11) == 0)  {
        if (eigenVectorIndex > numEigenVector) {
            printf("Only %d EigenVectors found but index %d requested!\n", numEigenVector, eigenVectorIndex);
            status = CAPS_RANGEERR;
            goto cleanup;
        }

        if (eigenVectorIndex < 1) {
            printf("For EigenVector_X notation, X must be >= 1, currently X = %d\n", eigenVectorIndex);
            status = CAPS_RANGEERR;
            goto cleanup;
        }
    }

    for (i = 0; i < numPoint; i++) {

        globalNodeID = nodeMap[i];

        if (strcasecmp(dataName, "Displacement") == 0) {

            for (dataPoint = 0; dataPoint < numGridPoint; dataPoint++) {
                if ((int) dataMatrix[dataPoint][0] ==  globalNodeID) break;
            }

            if (dataPoint == numGridPoint) {
              printf("Unable to locate global ID = %d in the data matrix\n", globalNodeID);
              status = CAPS_NOTFOUND;
              goto cleanup;
            }

            dataVal[dataRank*i+0] = dataMatrix[dataPoint][2]; // T1
            dataVal[dataRank*i+1] = dataMatrix[dataPoint][3]; // T2
            dataVal[dataRank*i+2] = dataMatrix[dataPoint][4]; // T3

        } else if (strncmp(dataName, "EigenVector", 11) == 0) {

            for (dataPoint = 0; dataPoint < numGridPoint; dataPoint++) {
                if ((int) dataMatrix[eigenVectorIndex - 1][8*dataPoint + 0] ==  globalNodeID) break;
            }

            if (dataPoint == numGridPoint) {
              printf("Unable to locate global ID = %d in the data matrix\n", globalNodeID);
              status = CAPS_NOTFOUND;
              goto cleanup;
            }

            dataVal[dataRank*i+0] = dataMatrix[eigenVectorIndex- 1][8*dataPoint + 2]; // T1
            dataVal[dataRank*i+1] = dataMatrix[eigenVectorIndex- 1][8*dataPoint + 3]; // T2
            dataVal[dataRank*i+2] = dataMatrix[eigenVectorIndex- 1][8*dataPoint + 4]; // T3
            //dataVal[dataRank*i+3] = dataMatrix[eigenVectorIndex- 1][8*dataPoint + 5]; // R1 - Don't use rotations
            //dataVal[dataRank*i+4] = dataMatrix[eigenVectorIndex- 1][8*dataPoint + 6]; // R2
            //dataVal[dataRank*i+5] = dataMatrix[eigenVectorIndex- 1][8*dataPoint + 7]; // R3

        }
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\tPremature exit in astrosAIM aimTransfer, status = %d\n", status);
        // Free data matrix
        if (dataMatrix != NULL) {
            j = 0;
            if      (strcasecmp(dataName, "Displacement") == 0) j = numGridPoint;
            else if (strncmp(dataName, "EigenVector", 11) == 0) j = numEigenVector;

            for (i = 0; i < j; i++) {
                if (dataMatrix[i] != NULL) EG_free(dataMatrix[i]);
            }
            EG_free(dataMatrix);
        }

        return status;
}
int aimInterpolation(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                 double *bary, int rank, double *data, double *result)
{
    int    in[4], i;
    double we[3];
    /*
    #ifdef DEBUG
        printf(" astrosAIM/aimInterpolation  %s  instance = %d!\n",
         name, discr->instance);
    #endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" astrosAIM/Interpolation: eIndex = %d [1-%d]!\n",eIndex, discr->nElems);
    }

    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) { // Linear triangle
        we[0] = 1.0 - bary[0] - bary[1];
        we[1] = bary[0];
        we[2] = bary[1];
        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
        for (i = 0; i < rank; i++){
            result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] + data[rank*in[2]+i]*we[2];
        }
    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {// Linear quad
        we[0] = bary[0];
        we[1] = bary[1];

        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
        in[3] = discr->elems[eIndex-1].gIndices[6] - 1;
        for (i = 0; i < rank; i++) {

            result[i] = (1.0-we[0])*( (1.0-we[1])*data[rank*in[0]+i] + we[1] *data[rank*in[3]+i] ) +
                             we[0] *( (1.0-we[1])*data[rank*in[1]+i] + we[1] *data[rank*in[2]+i] );
        }

    } else {
        printf(" astrosAIM/Interpolation: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
        return CAPS_BADVALUE;
    }

    return CAPS_SUCCESS;
}


int aimInterpolateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                      double *bary, int rank, double *r_bar, double *d_bar)
{
    int    in[4], i;
    double we[3];

    /*
    #ifdef DEBUG
        printf(" astrosAIM/aimInterpolateBar  %s  instance = %d!\n", name, discr->instance);
    #endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" astrosAIM/InterpolateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) { // Linear triangle
        we[0] = 1.0 - bary[0] - bary[1];
        we[1] = bary[0];
        we[2] = bary[1];
        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

        for (i = 0; i < rank; i++) {
            /*  result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                            data[rank*in[2]+i]*we[2];  */
            d_bar[rank*in[0]+i] += we[0]*r_bar[i];
            d_bar[rank*in[1]+i] += we[1]*r_bar[i];
            d_bar[rank*in[2]+i] += we[2]*r_bar[i];
        }
    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {// Linear quad

        we[0] = bary[0];
        we[1] = bary[1];

        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
        in[3] = discr->elems[eIndex-1].gIndices[6] - 1;

        for (i = 0; i < rank; i++) {
            /*  result[i] = (1.0-we[0])*((1.0-we[1])*data[rank*in[0]+i]  +
                                              we[1] *data[rank*in[3]+i]) +
                                 we[0] *((1.0-we[1])*data[rank*in[1]+i]  +
                                              we[1] *data[rank*in[2]+i]);  */
            d_bar[rank*in[0]+i] += (1.0-we[0])*(1.0-we[1])*r_bar[i];
            d_bar[rank*in[1]+i] +=      we[0] *(1.0-we[1])*r_bar[i];
            d_bar[rank*in[2]+i] +=      we[0] *     we[1] *r_bar[i];
            d_bar[rank*in[3]+i] += (1.0-we[0])*     we[1] *r_bar[i];
        }

    } else {
        printf(" astrosAIM/InterpolateBar: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
        return CAPS_BADVALUE;
    }

    return CAPS_SUCCESS;
}


int aimIntegration(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
                   /*@null@*/ double *data, double *result)
{
    int        i, in[4], stat, ptype, pindex, nBody;
    double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area, area2;
    const char *intents;
    ego        *bodies;

    /*
    #ifdef DEBUG
        printf(" astrosAIM/aimIntegration  %s  instance = %d!\n", name, discr->instance);
    #endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" astrosAIM/aimIntegration: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" astrosAIM/aimIntegration: %d aim_getBodies = %d!\n", discr->instance, stat);
        return stat;
    }

    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) { // Linear triangle
        /* element indices */

        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

        stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                            discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        x1[0] = xyz2[0] - xyz1[0];
        x2[0] = xyz3[0] - xyz1[0];
        x1[1] = xyz2[1] - xyz1[1];
        x2[1] = xyz3[1] - xyz1[1];
        x1[2] = xyz2[2] - xyz1[2];
        x2[2] = xyz3[2] - xyz1[2];

        //CROSS(x3, x1, x2);
        cross_DoubleVal(x1, x2, x3);

        //area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */
        area  = sqrt(dot_DoubleVal(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

        if (data == NULL) {
            *result = 3.0*area;
            return CAPS_SUCCESS;
        }

        for (i = 0; i < rank; i++) {
            result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] + data[rank*in[2]+i])*area;
        }

    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {// Linear quad

        /* element indices */

        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
        in[3] = discr->elems[eIndex-1].gIndices[6] - 1;

        stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                            discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        x1[0] = xyz2[0] - xyz1[0];
        x2[0] = xyz3[0] - xyz1[0];
        x1[1] = xyz2[1] - xyz1[1];
        x2[1] = xyz3[1] - xyz1[1];
        x1[2] = xyz2[2] - xyz1[2];
        x2[2] = xyz3[2] - xyz1[2];

        //CROSS(x3, x1, x2);
        cross_DoubleVal(x1, x2, x3);

        //area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */
        area  = sqrt(dot_DoubleVal(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz2);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                            discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        x1[0] = xyz2[0] - xyz1[0];
        x2[0] = xyz3[0] - xyz1[0];
        x1[1] = xyz2[1] - xyz1[1];
        x2[1] = xyz3[1] - xyz1[1];
        x1[2] = xyz2[2] - xyz1[2];
        x2[2] = xyz3[2] - xyz1[2];

        cross_DoubleVal(x1, x2, x3);

        area2  = sqrt(dot_DoubleVal(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

        if (data == NULL) {
            *result = 3.0*area + 3.0*area2;
            return CAPS_SUCCESS;
        }

        for (i = 0; i < rank; i++) {
            result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] + data[rank*in[2]+i])*area +
                        (data[rank*in[0]+i] + data[rank*in[2]+i] + data[rank*in[3]+i])*area2;
        }

    } else {
        printf(" astrosAIM/aimIntegration: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
        return CAPS_BADVALUE;
    }

    return CAPS_SUCCESS;
}


int aimIntegrateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
                    double *r_bar, double *d_bar)
{
    int        i, in[4], stat, ptype, pindex, nBody;
    double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area, area2;
    const char *intents;
    ego        *bodies;

    /*
    #ifdef DEBUG
        printf(" astrosAIM/aimIntegrateBar  %s  instance = %d!\n", name, discr->instance);
    #endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" astrosAIM/aimIntegrateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" astrosAIM/aimIntegrateBar: %d aim_getBodies = %d!\n", discr->instance, stat);
        return stat;
    }

    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) { // Linear triangle
        /* element indices */

        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
        stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                            discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        x1[0] = xyz2[0] - xyz1[0];
        x2[0] = xyz3[0] - xyz1[0];
        x1[1] = xyz2[1] - xyz1[1];
        x2[1] = xyz3[1] - xyz1[1];
        x1[2] = xyz2[2] - xyz1[2];
        x2[2] = xyz3[2] - xyz1[2];

        //CROSS(x3, x1, x2);
        cross_DoubleVal(x1, x2, x3);

        //area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */
        area  = sqrt(dot_DoubleVal(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

        for (i = 0; i < rank; i++) {
            /*  result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                             data[rank*in[2]+i])*area;  */
            d_bar[rank*in[0]+i] += area*r_bar[i];
            d_bar[rank*in[1]+i] += area*r_bar[i];
            d_bar[rank*in[2]+i] += area*r_bar[i];
        }

    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {// Linear quad

        /* element indices */

        in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
        in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
        in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
        in[3] = discr->elems[eIndex-1].gIndices[6] - 1;

        stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                            discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        x1[0] = xyz2[0] - xyz1[0];
        x2[0] = xyz3[0] - xyz1[0];
        x1[1] = xyz2[1] - xyz1[1];
        x2[1] = xyz3[1] - xyz1[1];
        x1[2] = xyz2[2] - xyz1[2];
        x2[2] = xyz3[2] - xyz1[2];

        //CROSS(x3, x1, x2);
        cross_DoubleVal(x1, x2, x3);

        //area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */
        area  = sqrt(dot_DoubleVal(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz2);

        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                            discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" astrosAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        x1[0] = xyz2[0] - xyz1[0];
        x2[0] = xyz3[0] - xyz1[0];
        x1[1] = xyz2[1] - xyz1[1];
        x2[1] = xyz3[1] - xyz1[1];
        x1[2] = xyz2[2] - xyz1[2];
        x2[2] = xyz3[2] - xyz1[2];

        //CROSS(x3, x1, x2);
        cross_DoubleVal(x1, x2, x3);

        //area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */
        area2  = sqrt(dot_DoubleVal(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

        for (i = 0; i < rank; i++) {

            /*  result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] + data[rank*in[2]+i])*area +
                            (data[rank*in[0]+i] + data[rank*in[2]+i] + data[rank*in[3]+i])*area2;  */
            d_bar[rank*in[0]+i] += (area + area2)*r_bar[i];
            d_bar[rank*in[1]+i] +=  area         *r_bar[i];
            d_bar[rank*in[2]+i] += (area + area2)*r_bar[i];
            d_bar[rank*in[3]+i] +=         area2 *r_bar[i];
        }

    } else {
        printf(" astrosAIM/aimIntegrateBar: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
        return CAPS_BADVALUE;
    }

    return CAPS_SUCCESS;
}
