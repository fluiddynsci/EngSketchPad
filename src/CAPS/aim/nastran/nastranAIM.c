/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             FUN3D AIM
 *
 *     Written by Dr. Ryan Durscher and Dr. Ed Alyanak AFRL/RQVC
 *
 */

/*! \mainpage Introduction
 *
 * \section overviewNastran Nastran AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact (primarily
 * through input files) with the finite element structural solver Nastran \cite Nastran.
 *
 * Current issues include:
 *  - A thorough bug testing needs to be undertaken.
 *
 * An outline of the AIM's inputs, outputs and attributes are provided in \ref aimInputsNastran and
 * \ref aimOutputsNastran and \ref attributeNastran, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentNastran.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataNastran if
 * connecting this AIM to other AIMs in a parent-child like manner.
 *
 * Details of the AIM's automated data transfer capabilities are outlined in \ref dataTransferNastran
 *
 * \section nastranExamples Examples
 * Example problems using the Nastran AIM may be found at \ref examplesNastran .
 *  - \ref nastranSingleLoadEx
 *  - \ref nastranMultipleLoadEx
 *  - \ref nastranModalEx
 *  - \ref nastranOptEx
 *  - \ref nastranCompositeEx
 *  - \ref nastranCompOptimizationEx
 *
 */

/*! \page attributeNastran Nastran AIM attributes
 * The following list of attributes are required for the Nastran AIM inside the geometry input.
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
 * - <b> capsIgnore</b> It is possible that there is a geometric body (or entity) that you do not want the Nastran AIM to pay attention to when creating
 * a finite element model. The capsIgnore attribute allows a body (or entity) to be in the geometry and ignored by the AIM.  For example,
 * because of limitations in OpenCASCADE a situation where two edges are overlapping may occur; capsIgnore allows the user to only
 *  pay attention to one of the overlapping edges.
 *
 * - <b> capsConnect</b> This is a name assigned to any geometric body where the user wishes to create
 * "fictitious" connections such as springs, dampers, and/or rigid body connections to. The user must manually specify
 * the connection between two <c>capsConnect</c> entities using the "Connect" tuple (see \ref aimInputsNastran).
 * Recall that a string in ESP starts with a $.  For example, attribute <c>capsConnect $springStart</c>.
 *
 * - <b> capsConnectLink</b> Similar to <c>capsConnect</c>, this is a name assigned to any geometric body where
 * the user wishes to create "fictitious" connections to. A connection is automatically made if a <c>capsConnectLink</c>
 * matches a <c>capsConnect</c> group. Again further specifics of the connection are input using the "Connect"
 * tuple (see \ref aimInputsNastran). Recall that a string in ESP starts with a $.
 * For example, attribute <c>capsConnectLink $springEnd</c>.
 *
 * - <b> capsBound </b> This is used to mark surfaces on the structural grid in which data transfer with an external
 * solver will take place. See \ref dataTransferNastran for additional details.
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
#include "vlmSpanSpace.h"
#include "feaUtils.h"     // FEA utilities
#include "nastranUtils.h" // Nastran utilities

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
#define NUMOUTPUT  5

#define MXCHAR  255

//#define DEBUG

typedef struct {

    // Project name
    char *projectName; // Project name

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

} aimStorage;

static aimStorage *nastranInstance = NULL;
static int         numInstance  = 0;


static int initiate_aimStorage(int iIndex) {

    int status;

    // Set initial values for nastranInstance
    nastranInstance[iIndex].projectName = NULL;

    // Analysis file path/directory
    nastranInstance[iIndex].analysisPath = NULL;

    /*
    // Check to make sure data transfer is ok
    nastranInstance[iIndex].dataTransferCheck = (int) true;
     */

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&nastranInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to constraint index map
    status = initiate_mapAttrToIndexStruct(&nastranInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to load index map
    status = initiate_mapAttrToIndexStruct(&nastranInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for transfer to index map
    status = initiate_mapAttrToIndexStruct(&nastranInstance[iIndex].transferMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for connect to index map
    status = initiate_mapAttrToIndexStruct(&nastranInstance[iIndex].connectMap);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_feaProblemStruct(&nastranInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS) return status;

    // Mesh holders
    nastranInstance[iIndex].numMesh = 0;
    nastranInstance[iIndex].feaMesh = NULL;

    return CAPS_SUCCESS;
}

static int destroy_aimStorage(int iIndex) {

    int status;
    int i;
    // Attribute to index map
    status = destroy_mapAttrToIndexStruct(&nastranInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to constraint index map
    status = destroy_mapAttrToIndexStruct(&nastranInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to load index map
    status = destroy_mapAttrToIndexStruct(&nastranInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Transfer to index map
    status = destroy_mapAttrToIndexStruct(&nastranInstance[iIndex].transferMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Connect to index map
    status = destroy_mapAttrToIndexStruct(&nastranInstance[iIndex].connectMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Cleanup meshes
    if (nastranInstance[iIndex].feaMesh != NULL) {

        for (i = 0; i < nastranInstance[iIndex].numMesh; i++) {
            status = destroy_meshStruct(&nastranInstance[iIndex].feaMesh[i]);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_meshStruct!\n", status);
        }

        EG_free(nastranInstance[iIndex].feaMesh);
    }

    nastranInstance[iIndex].feaMesh = NULL;
    nastranInstance[iIndex].numMesh = 0;

    // Destroy FEA problem structure
    status = destroy_feaProblemStruct(&nastranInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS)  printf("Error: Status %d during destroy_feaProblemStruct!\n", status);

    // NULL projetName
    nastranInstance[iIndex].projectName = NULL;

    // NULL analysisPath
    nastranInstance[iIndex].analysisPath = NULL;

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
                          &nastranInstance[iIndex].attrMap,
                          &nastranInstance[iIndex].constraintMap,
                          &nastranInstance[iIndex].loadMap,
                          &nastranInstance[iIndex].transferMap,
                          &nastranInstance[iIndex].connectMap,
                          &nastranInstance[iIndex].numMesh,
                          &nastranInstance[iIndex].feaMesh,
                          &nastranInstance[iIndex].feaProblem );
  if (status != CAPS_SUCCESS) return status;

  return CAPS_SUCCESS;
}

static int createVLMMesh(int iIndex, void *aimInfo, capsValue *aimInputs) {

    int projectionMethod = (int) true;

    int status, status2; // Function return status

    int i, j, k, surfaceIndex = 0, sectionIndex, transferIndex; // Indexing

    // Bodies
    const char *intents;
    int   numBody; // Number of Bodies
    ego  *bodies;

    // Aeroelastic information
    int numVLMSurface = 0;
    vlmSurfaceStruct *vlmSurface = NULL;
    int numSpanWise;

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
    printf(" nastranAIM/createVLMMesh instance = %d  nbody = %d!\n", iIndex, numBody);
#endif

    if ((numBody <= 0) || (bodies == NULL)) {
#ifdef DEBUG
        printf(" nastranAIM/createVLMMesh No Bodies!\n");
#endif

        return CAPS_SOURCEERR;
    }

    // Get aerodynamic reference quantities
    status = fea_retrieveAeroRef(numBody, bodies, &nastranInstance[iIndex].feaProblem.feaAeroRef);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Cleanup Aero storage first
    if (nastranInstance[iIndex].feaProblem.feaAero != NULL) {

        for (i = 0; i < nastranInstance[iIndex].feaProblem.numAero; i++) {
            status = destroy_feaAeroStruct(&nastranInstance[iIndex].feaProblem.feaAero[i]);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        EG_free(nastranInstance[iIndex].feaProblem.feaAero);
    }

    nastranInstance[iIndex].feaProblem.numAero = 0;

    // Get AVL surface information
    if (aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].nullVal != IsNull) {

        status = get_vlmSurface(aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "VLM_Surface", ANALYSISIN)-1].vals.tuple,
                                &nastranInstance[iIndex].attrMap,
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
                             nastranInstance[iIndex].attrMap,
                             vlmGENERIC,
                             numVLMSurface,
                             &vlmSurface);
    if (status != CAPS_SUCCESS) goto cleanup;

    for (i = 0; i < numVLMSurface; i++) {

        // Compute equal spacing
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
            printf("\tSurface '%s' has less than two-sections!\n", vlmSurface[i].name);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        status = get_mapAttrToIndexIndex(&nastranInstance[iIndex].transferMap,
                                         vlmSurface[i].name,
                                         &transferIndex);
        if (status == CAPS_NOTFOUND) {
            printf("\tA corresponding capsBound name not found for \"%s\". Surface will be ignored!\n", vlmSurface[i].name);
            continue;
        } else if (status != CAPS_SUCCESS) goto cleanup;

        for (j = 0; j < vlmSurface[i].numSection-1; j++) {

            // Increment the number of Aero surfaces
            nastranInstance[iIndex].feaProblem.numAero += 1;

            surfaceIndex = nastranInstance[iIndex].feaProblem.numAero - 1;

            // Allocate
            if (nastranInstance[iIndex].feaProblem.numAero == 1) {

                feaAeroTemp = (feaAeroStruct *) EG_alloc(nastranInstance[iIndex].feaProblem.numAero*sizeof(feaAeroStruct));

            } else {

                feaAeroTemp = (feaAeroStruct *) EG_reall(nastranInstance[iIndex].feaProblem.feaAero,
                                                         nastranInstance[iIndex].feaProblem.numAero*sizeof(feaAeroStruct));
            }

            if (feaAeroTemp == NULL) {
                nastranInstance[iIndex].feaProblem.numAero -= 1;

                status = EGADS_MALLOC;
                goto cleanup;
            }

            nastranInstance[iIndex].feaProblem.feaAero = feaAeroTemp;

            // Initiate feaAeroStruct
            status = initiate_feaAeroStruct(&nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex]);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Get surface Name - copy from original surface
            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].name = EG_strdup(vlmSurface[i].name);
            if (nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].name == NULL) { status = EGADS_MALLOC; goto cleanup; }

            // Get surface ID - Multiple by 1000 !!
            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].surfaceID = 1000*nastranInstance[iIndex].feaProblem.numAero;

            // ADD something for coordinate systems

            // Sections aren't necessarily stored in order coming out of vlm_getSections, however sectionIndex is!
            sectionIndex = vlmSurface[i].vlmSection[j].sectionIndex;

            // Populate vmlSurface structure
            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.Cspace = vlmSurface[i].Cspace;
            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.Sspace = vlmSurface[i].Sspace;

            // use the section span count for the sub-surface
            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.NspanTotal = vlmSurface[i].vlmSection[sectionIndex].Nspan;
            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.Nchord     = vlmSurface[i].Nchord;

            // Copy section information
            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.numSection = 2;

            nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection = (vlmSectionStruct *) EG_alloc(2*sizeof(vlmSectionStruct));
            if (nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            for (k = 0; k < nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.numSection; k++) {

                // Add k to section indexing variable j to get j and j+1 during iterations

                // Sections aren't necessarily stored in order coming out of vlm_getSections, however sectionIndex is!
                sectionIndex = vlmSurface[i].vlmSection[j+k].sectionIndex;

                status = initiate_vlmSectionStruct(&nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection[k]);
                if (status != CAPS_SUCCESS) goto cleanup;

                // Copy the section data - This also copies the control data for the section
                status = copy_vlmSectionStruct( &vlmSurface[i].vlmSection[sectionIndex],
                                                &nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection[k]);
                if (status != CAPS_SUCCESS) goto cleanup;

                // Reset the sectionIndex that is keeping track of the section order.
                nastranInstance[iIndex].feaProblem.feaAero[surfaceIndex].vlmSurface.vlmSection[k].sectionIndex = k;
            }
        }
    }

    // Determine which grid points are to be used for each spline
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numAero; i++) {

        // Debug
        //printf("\tDetermining grid points\n");

        // Get the transfer index for this surface - it has already been checked to make sure the name is in the
        // transfer index map
        status = get_mapAttrToIndexIndex(&nastranInstance[iIndex].transferMap,
                                         nastranInstance[iIndex].feaProblem.feaAero[i].name,
                                         &transferIndex);
        if (status != CAPS_SUCCESS ) goto cleanup;

        if (projectionMethod == (int) false) { // Look for attributes

            for (j = 0; j < nastranInstance[iIndex].feaProblem.feaMesh.numNode; j++) {

                if (nastranInstance[iIndex].feaProblem.feaMesh.node[j].analysisType == MeshStructure) {
                    feaData = (feaMeshDataStruct *) nastranInstance[iIndex].feaProblem.feaMesh.node[j].analysisData;
                } else {
                    continue;
                }

                if (feaData->transferIndex != transferIndex) continue;
                if (feaData->transferIndex == CAPSMAGIC) continue;


                nastranInstance[iIndex].feaProblem.feaAero[i].numGridID += 1;
                k = nastranInstance[iIndex].feaProblem.feaAero[i].numGridID;

                nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_reall(nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet,
                                                                                           k*sizeof(int));

                if (nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet[k-1] = nastranInstance[iIndex].feaProblem.feaMesh.node[j].nodeID;
            }

        } else { // Projection method


            /*
             *   n = A X B Create a normal vector/ plane between A and B
             *
             *   d_proj = C - (C * n)*n/ ||n||^2 , projection of point d on plane created by AxB
             *
             *   p = D - (D * n)*n/ ||n||^2 , projection of point p on plane created by AxB
             *
             * 				           (section 2)
             *                     LE(c)---------------->TE(d)
             *   Grid Point       -^                   ^ -|
             *           |^      -            -         - |
             *           | -     A      -   C          - d_proj
             *           |  D   -    -                 -
             *           |   - - -     (section 1     -
             *           p    LE(a)----------B------->TE(b)
             */

            // Vector between section 2 and 1
            a = nastranInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[0].xyzLE;
            b = nastranInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[0].xyzTE;
            c = nastranInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[1].xyzLE;
            d = nastranInstance[iIndex].feaProblem.feaAero[i].vlmSurface.vlmSection[1].xyzTE;


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

            for (j = 0; j < nastranInstance[iIndex].feaProblem.feaMesh.numNode; j++) {

                if (nastranInstance[iIndex].feaProblem.feaMesh.node[j].analysisType == MeshStructure) {
                    feaData = (feaMeshDataStruct *) nastranInstance[iIndex].feaProblem.feaMesh.node[j].analysisData;
                } else {
                    continue;
                }

                if (feaData->transferIndex != transferIndex) continue;
                if (feaData->transferIndex == CAPSMAGIC) continue;

                D[0] = nastranInstance[iIndex].feaProblem.feaMesh.node[j].xyz[0] - a[0];

                D[1] = nastranInstance[iIndex].feaProblem.feaMesh.node[j].xyz[1] - a[1];

                D[2] = nastranInstance[iIndex].feaProblem.feaMesh.node[j].xyz[2] - a[2];

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
                //	printf("Point index %d\n", j);
                //	printf("\tAreas don't match - %f vs %f!!\n", Area, (apbArea + apcArea + cpdArea + bpdArea));
                //	printf("\tPoint - %f %f %f\n", D[0] + a[0], D[1] + a[1], D[2] + a[2]);
                //	printf("\tPoint projection - %f %f %f\n", p[0], p[1], p[2]);
                //}

                if (fabs(apbArea + apcArea + cpdArea + bpdArea - Area) > 1E-5) 	continue;

                nastranInstance[iIndex].feaProblem.feaAero[i].numGridID += 1;

                if (nastranInstance[iIndex].feaProblem.feaAero[i].numGridID == 1) {
                    nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_alloc(nastranInstance[iIndex].feaProblem.feaAero[i].numGridID*sizeof(int));
                } else {
                    nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet = (int *) EG_reall(nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet,
                                                                                               nastranInstance[iIndex].feaProblem.feaAero[i].numGridID*sizeof(int));
                }

                if (nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                nastranInstance[iIndex].feaProblem.feaAero[i].gridIDSet[
                    nastranInstance[iIndex].feaProblem.feaAero[i].numGridID-1] = nastranInstance[iIndex].feaProblem.feaMesh.node[j].nodeID;
            }
        }

        printf("\tSurface %d: Number of points found for aero-spline = %d\n", i+1, nastranInstance[iIndex].feaProblem.feaAero[i].numGridID );
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


/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(int ngIn, capsValue *gIn, int *qeFlag, const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int flag;
    int *ints;
    char **strs;

    aimStorage *tmp;

    #ifdef DEBUG
        printf("\n nastranAIM/aimInitialize   ngIn = %d!\n", ngIn);
    #endif
    flag     = *qeFlag;
    *qeFlag  = 0;

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
        *ranks   = NULL;
        return EGADS_MALLOC;
    }

    strs[0]  = EG_strdup("Displacement");
    strs[1]  = EG_strdup("EigenVector");
    strs[2]  = EG_strdup("EigenVector_*");
    *fnames  = strs;

    // Allocate nastranInstance
    if (nastranInstance == NULL) {
        nastranInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (nastranInstance == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }
    } else {
        tmp = (aimStorage *) EG_reall(nastranInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }
        nastranInstance = tmp;
    }

    // Initialize instance storage
    (void) initiate_aimStorage(numInstance);

    // Increment number of instances
    numInstance += 1;

    return (numInstance -1);
}

int aimInputs(int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{

    /*! \page aimInputsNastran AIM Inputs
     * The following list outlines the Nastran inputs along with their default value available
     * through the AIM interface. Unless noted these values will be not be linked to
     * any parent AIMs with variables of the same name.
     */

    #ifdef DEBUG
        printf(" nastranAIM/aimInputs instance = %d  index = %d!\n", inst, index);
    #endif

    *ainame = NULL;

    // Nastran Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("nastran_CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsNastran
         * - <B> Proj_Name = "nastran_CAPS"</B> <br>
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

        /*! \page aimInputsNastran
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
         * zero ignores this phase
         */

    } else if (index == 3) {
        *ainame               = EG_strdup("Edge_Point_Min");
        defval->type          = Integer;
        defval->vals.integer  = 2;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsNastran
         * - <B> Edge_Point_Min = 2</B> <br>
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

        /*! \page aimInputsNastran
         * - <B> Edge_Point_Max = 50</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 5) {
        *ainame               = EG_strdup("Quad_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = (int) false;

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
         * - <B> Analysis = NULL</B> <br>
         * Analysis tuple used to input analysis/case information for the model, see \ref feaAnalysis for additional details.
         */
    } else if (index == 11) {
        *ainame              = EG_strdup("Analysis_Type");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Modal");
        defval->lfixed       = Change;

        /*! \page aimInputsNastran
         * - <B> Analysis_Type = "Modal"</B> <br>
         * Type of analysis to generate files for, options include "Modal", "Static", "AeroelasticTrim", "AeroelasticFlutter", and "Optimization".
         * Note: "Aeroelastic" and "StaticOpt" are still supported and refer to "AeroelasticTrim" and "Optimization".
         */
    } else if (index == 12) {
	    *ainame              = EG_strdup("File_Format");
        defval->type         = String;
        defval->vals.string  = EG_strdup("Small"); // Small, Large, Free
        defval->lfixed       = Change;

        /*! \page aimInputsNastran
         * - <B> File_Format = "Small"</B> <br>
         * Formatting type for the bulk file. Options: "Small", "Large", "Free".
         */

    } else if (index == 13) {
	    *ainame              = EG_strdup("Mesh_File_Format");
        defval->type         = String;
        defval->vals.string  = EG_strdup("Free"); // Small, Large, Free
        defval->lfixed       = Change;

        /*! \page aimInputsNastran
         * - <B> Mesh_File_Format = "Small"</B> <br>
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

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
         * - <B> Design_Constraint = NULL</B> <br>
         * The design constraint tuple used to input design constraint information for the model optimization, see \ref feaDesignConstraint for additional details.
         */

    } else if (index == 16) {
        *ainame              = EG_strdup("ObjectiveMinMax");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Max"); // Max, Min
        defval->lfixed       = Change;

        /*! \page aimInputsNastran
         * - <B> ObjectiveMinMax = "Max"</B> <br>
         * Maximize or minimize the design objective during an optimization. Option: "Max" or "Min".
         */

    } else if (index == 17) {
        *ainame              = EG_strdup("ObjectiveResponseType");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Weight"); // Weight
        defval->lfixed       = Change;

        /*! \page aimInputsNastran
         * - <B> ObjectiveResponseType = "Weight"</B> <br>
         * Object response type (see Nastran manual).
         */
    } else if (index == 18) {
        *ainame              = EG_strdup("VLM_Surface");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
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

        /*! \page aimInputsNastran
         * - <B> Parameter = NULL</B> <br>
         * Parameter tuple used to define PARAM entries. Note, entries are output exactly as inputed, that is, if the PARAM entry
         * requires an integer entry the user must input an integer!
         */
    } else {
      printf(" nastranAIM/aimInputs: unknown input index = %d for instance = %d!\n",
             index, iIndex);
      return CAPS_BADINDEX;
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

    /*! \page sharableDataNastran AIM Shareable Data
     * - <B> FEA_Problem</B> <br>
     * The FEA problem data in feaProblemStruct (see feaTypes.h) format.
     */

    #ifdef DEBUG
        printf(" nastranAIM/aimData instance = %d  name = %s!\n", iIndex, name);
    #endif

    if (strcasecmp(name, "FEA_Problem") == 0){
        *vtype = Value;
        *rank = *nrow = *ncol = 1;
        *data  = &nastranInstance[iIndex].feaProblem;
        *units = NULL;

        return CAPS_SUCCESS;
    }

   return CAPS_NOTFOUND;
}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath,
                   capsValue *aimInputs, capsErrs **errs)
{

    int i, j, k, l; // Indexing

    int status; // Status return

    int found; // Boolean operator

    int *tempIntegerArray = NULL; // Temporary array to store a list of integers

    // Analysis information
    char *analysisType = NULL;

    // Optimization Information
    char *objectiveMinMax = NULL, *objectiveResp = NULL;

    // Aeroelastic Information
    int numAEStatSurf = 0;
    //char **aeStatSurf = NULL;

    // File format information
    char *tempString = NULL, *delimiter = NULL;

    // File IO
    char *filename = NULL; // Output file name
    FILE *fp = NULL; // Output file pointer

    // NULL out errors
    *errs = NULL;

    // Store away the analysis path/directory
    nastranInstance[iIndex].analysisPath = analysisPath;

    // Get project name
    nastranInstance[iIndex].projectName = aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string;

    // Analysis type
    analysisType = aimInputs[aim_getIndex(aimInfo, "Analysis_Type", ANALYSISIN)-1].vals.string;


    // Get FEA mesh if we don't already have one
    if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

        status = checkAndCreateMesh(iIndex, aimInfo);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Get Aeroelastic mesh
        if ((strcasecmp(analysisType, "Aeroelastic") == 0) ||
            (strcasecmp(analysisType, "AeroelasticTrim") == 0) ||
            (strcasecmp(analysisType, "AeroelasticFlutter") == 0)) {

            status = createVLMMesh(iIndex, aimInfo, aimInputs);
            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }

    // Note: Setting order is important here.
    // 1. Materials should be set before properties.
    // 2. Coordinate system should be set before mesh and loads
    // 3. Mesh should be set before loads, constraints, supports, and connections
    // 4. Constraints and loads should be set before analysis
    // 5. Optimization should be set after properties, but before analysis

    // Set material properties
    if (aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getMaterial(aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].vals.tuple,
                                 &nastranInstance[iIndex].feaProblem.numMaterial,
                                 &nastranInstance[iIndex].feaProblem.feaMaterial);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Material tuple is NULL - No materials set\n");

    // Set property properties
    if (aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getProperty(aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].vals.tuple,
                                 &nastranInstance[iIndex].attrMap,
                                 &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;


        // Assign element "subtypes" based on properties set
        status = fea_assignElementSubType(nastranInstance[iIndex].feaProblem.numProperty,
                                          nastranInstance[iIndex].feaProblem.feaProperty,
                                          &nastranInstance[iIndex].feaProblem.feaMesh);
        if (status != CAPS_SUCCESS) return status;

    } else printf("Property tuple is NULL - No properties set\n");

    // Set constraint properties
    if (aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getConstraint(aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].length,
                                   aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].vals.tuple,
                                   &nastranInstance[iIndex].constraintMap,
                                   &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Constraint tuple is NULL - No constraints applied\n");

    // Set support properties
    if (aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getSupport(aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].vals.tuple,
                                &nastranInstance[iIndex].constraintMap,
                                &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Support tuple is NULL - No supports applied\n");

    // Set connection properties
    if (aimInputs[aim_getIndex(aimInfo, "Connect", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getConnection(aimInputs[aim_getIndex(aimInfo, "Connect", ANALYSISIN)-1].length,
                                   aimInputs[aim_getIndex(aimInfo, "Connect", ANALYSISIN)-1].vals.tuple,
                                   &nastranInstance[iIndex].connectMap,
                                   &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Connect tuple is NULL - Using defaults\n");

    // Set load properties
    if (aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getLoad(aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].length,
                             aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].vals.tuple,
                             &nastranInstance[iIndex].loadMap,
                             &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;

        // Loop through loads to see if any of them are supposed to be from an external source
        for (i = 0; i < nastranInstance[iIndex].feaProblem.numLoad; i++) {

            if (nastranInstance[iIndex].feaProblem.feaLoad[i].loadType == PressureExternal) {

                // Transfer external pressures from the AIM discrObj
                status = fea_transferExternalPressure(aimInfo,
                                                      &nastranInstance[iIndex].feaProblem.feaMesh,
                                                      &nastranInstance[iIndex].feaProblem.feaLoad[i]);
                if (status != CAPS_SUCCESS) return status;

            } // End PressureExternal if
        } // End load for loop
    } else printf("Load tuple is NULL - No loads applied\n");

    // Set design variables
    if (aimInputs[aim_getIndex(aimInfo, "Design_Variable", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getDesignVariable(aimInputs[aim_getIndex(aimInfo, "Design_Variable", ANALYSISIN)-1].length,
                                       aimInputs[aim_getIndex(aimInfo, "Design_Variable", ANALYSISIN)-1].vals.tuple,
                                       &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Design_Variable tuple is NULL - No design variables applied\n");

    // Set design constraints
    if (aimInputs[aim_getIndex(aimInfo, "Design_Constraint", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getDesignConstraint(aimInputs[aim_getIndex(aimInfo, "Design_Constraint", ANALYSISIN)-1].length,
                                         aimInputs[aim_getIndex(aimInfo, "Design_Constraint", ANALYSISIN)-1].vals.tuple,
                                         &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Design_Constraint tuple is NULL - No design constraints applied\n");

    // Set analysis settings
    if (aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getAnalysis(aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].vals.tuple,
                                 &nastranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status; // It ok to not have an analysis tuple
    } else printf("Analysis tuple is NULL\n");

    // Set file format type
    if        (strcasecmp(aimInputs[aim_getIndex(aimInfo, "File_Format", ANALYSISIN)-1].vals.string, "Small") == 0) {
    	nastranInstance[iIndex].feaProblem.feaFileFormat.fileType = SmallField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "File_Format", ANALYSISIN)-1].vals.string, "Large") == 0) {
    	nastranInstance[iIndex].feaProblem.feaFileFormat.fileType = LargeField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "File_Format", ANALYSISIN)-1].vals.string, "Free") == 0)  {
    	nastranInstance[iIndex].feaProblem.feaFileFormat.fileType = FreeField;
    } else {
    	printf("Unrecognized \"File_Format\", valid choices are [Small, Large, or Free]. Reverting to default\n");
    }

    // Set grid file format type
    if        (strcasecmp(aimInputs[aim_getIndex(aimInfo, "Mesh_File_Format", ANALYSISIN)-1].vals.string, "Small") == 0) {
        nastranInstance[iIndex].feaProblem.feaFileFormat.gridFileType = SmallField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "Mesh_File_Format", ANALYSISIN)-1].vals.string, "Large") == 0) {
        nastranInstance[iIndex].feaProblem.feaFileFormat.gridFileType = LargeField;
    } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "Mesh_File_Format", ANALYSISIN)-1].vals.string, "Free") == 0)  {
        nastranInstance[iIndex].feaProblem.feaFileFormat.gridFileType = FreeField;
    } else {
        printf("Unrecognized \"Mesh_File_Format\", valid choices are [Small, Large, or Free]. Reverting to default\n");
    }

    // Write Nastran Mesh
    filename = EG_alloc(MXCHAR +1);
    if (filename == NULL) return EGADS_MALLOC;
    strcpy(filename, analysisPath);
    #ifdef WIN32
        strcat(filename, "\\");
    #else
        strcat(filename, "/");
    #endif
    strcat(filename, nastranInstance[iIndex].projectName);

    status = mesh_writeNASTRAN(filename,
                               1,
                               &nastranInstance[iIndex].feaProblem.feaMesh,
                               nastranInstance[iIndex].feaProblem.feaFileFormat.gridFileType,
                               1.0);
    if (status != CAPS_SUCCESS) {
        EG_free(filename);
        return status;
    }

    // Write Nastran subElement types not supported by mesh_writeNASTRAN
    strcat(filename, ".bdf");
    fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        EG_free(filename);
        return CAPS_IOERR;
    }
    EG_free(filename);

    printf("Writing subElement types (if any) - appending mesh file\n");
    status = nastran_writeSubElementCard(fp,
                                         &nastranInstance[iIndex].feaProblem.feaMesh,
                                         nastranInstance[iIndex].feaProblem.numProperty,
                                         nastranInstance[iIndex].feaProblem.feaProperty,
                                         &nastranInstance[iIndex].feaProblem.feaFileFormat);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Connections
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numConnect; i++) {

        if (i == 0) {
            printf("Writing connection cards - appending mesh file\n");
        }

        status = nastran_writeConnectionCard(fp,
                                             &nastranInstance[iIndex].feaProblem.feaConnect[i],
                                             &nastranInstance[iIndex].feaProblem.feaFileFormat);
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
    strcat(filename, nastranInstance[iIndex].projectName);
    strcat(filename, ".dat");


    printf("\nWriting nastran instruction file....\n");
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        EG_free(filename);
        return CAPS_IOERR;
    }
    EG_free(filename);

    // define file format delimiter type
    if (nastranInstance[iIndex].feaProblem.feaFileFormat.fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    //////////////// Executive control ////////////////
    fprintf(fp, "ID CAPS generated Problem FOR Nastran\n");

    // Analysis type
    if     (strcasecmp(analysisType, "Modal")         		== 0) fprintf(fp, "SOL 3\n");
    else if(strcasecmp(analysisType, "Static")        		== 0) fprintf(fp, "SOL 1\n");
    else if(strcasecmp(analysisType, "Craig-Bampton") 		== 0) fprintf(fp, "SOL 31\n");
    else if(strcasecmp(analysisType, "StaticOpt")     		== 0) fprintf(fp, "SOL 200\n");
    else if(strcasecmp(analysisType, "Optimization")  		== 0) fprintf(fp, "SOL 200\n");
    else if(strcasecmp(analysisType, "Aeroelastic")   		== 0) fprintf(fp, "SOL 144\n");
    else if(strcasecmp(analysisType, "AeroelasticTrim") 	== 0) fprintf(fp, "SOL 144\n");
    else if(strcasecmp(analysisType, "AeroelasticFlutter") 	== 0) fprintf(fp, "SOL 145\n");
    else {
        printf("Unrecognized \"Analysis_Type\", %s, defaulting to \"Modal\" analysis\n", analysisType);
        analysisType = "Modal";
        fprintf(fp, "SOL 3\n");
    }

    fprintf(fp, "CEND\n\n");
    fprintf(fp, "LINE=10000\n");


    //////////////// Case control ////////////////

    // Write output request information
    fprintf(fp, "DISP (PRINT,PUNCH) = ALL\n"); // Output all displacements

    fprintf(fp, "STRE (PRINT,PUNCH) = ALL\n"); // Output all stress

    fprintf(fp, "STRA (PRINT,PUNCH) = ALL\n"); // Output all strain

    //fprintf(fp, "PARAM,");

    //PRINT PARAM ENTRIES IN CASE CONTROL - most entries are printed in bulkd data below
    /*
    if (aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].nullVal == NotNull) {
        for (i = 0; i < aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].length; i++) {

            fprintf(fp, " %s, %s,", aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].vals.tuple[i].name,
                                    aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].vals.tuple[i].value);
        }
    }
    */

    // Printed in Bulk Data below
    //fprintf(fp, "PARAM, %s\n", "POST, -1\n"); // Output OP2 file

    // Check thermal load - currently only a single thermal load is supported - also it only works for the global level - no subcase
    found = (int) false;
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numLoad; i++) {

        if (nastranInstance[iIndex].feaProblem.feaLoad[i].loadType != Thermal) continue;

        if (found == (int) true) {
            printf("More than 1 Thermal load found - nastranAIM does NOT currently doesn't support multiple thermal loads!\n");
        }

        found = (int) true;

        fprintf(fp, "TEMPERATURE = %d\n", nastranInstance[iIndex].feaProblem.feaLoad[i].loadID);
    }

    // Design objective information, SOL 200 only
    if ((strcasecmp(analysisType, "StaticOpt") == 0) || (strcasecmp(analysisType, "Optimization") == 0)) {

        objectiveMinMax = aimInputs[aim_getIndex(aimInfo, "ObjectiveMinMax", ANALYSISIN)-1].vals.string;
        if     (strcasecmp(objectiveMinMax, "Min") == 0) fprintf(fp, "DESOBJ(MIN) = 1\n");
        else if(strcasecmp(objectiveMinMax, "Max") == 0) fprintf(fp, "DESOBJ(MAX) = 1\n");
        else {
            printf("Unrecognized \"ObjectiveMinMax\", %s, defaulting to \"Min\"\n", objectiveMinMax);
            objectiveMinMax = "Min";
            fprintf(fp, "DESOBJ(MIN) = 1\n");
        }

    }

    // Modal analysis - only
    // If modal - we are only going to use the first analysis structure we come across that has its type as modal
    if (strcasecmp(analysisType, "Modal") == 0) {

        // Look through analysis structures for a modal one
        found = (int) false;
        for (i = 0; i < nastranInstance[iIndex].feaProblem.numAnalysis; i++) {
            if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Modal) {
                found = (int) true;
                break;
            }
        }

        // Write out analysis ID if a modal analysis structure was found
        if (found == (int) true)  {
            fprintf(fp, "METHOD = %d\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
        } else {
            printf("Warning: No eigenvalue analysis information specified in \"Analysis\" tuple, through "
            		"AIM input \"Analysis_Type\" is set to \"Modal\"!!!!\n");
            return CAPS_NOTFOUND;
        }

        // Write support for sub-case
        if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numSupport != 0) {
            if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numSupport > 1) {
                printf("\tWARNING: More than 1 support is not supported at this time for sub-cases!\n");

            } else {
                fprintf(fp, "SUPORT1 = %d\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].supportSetID[0]);
            }
        }

        // Write constraint for sub-case
        if (nastranInstance[iIndex].feaProblem.numConstraint != 0) {
            fprintf(fp, "SPC = %d\n", nastranInstance[iIndex].feaProblem.numConstraint+i+1);
        }

        // Issue some warnings regarding constraints if necessary
        if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint == 0 && nastranInstance[iIndex].feaProblem.numConstraint !=0) {
            printf("\tWarning: No constraints specified for modal case %s, assuming "
                    "all constraints are applied!!!!\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].name);
        } else if (nastranInstance[iIndex].feaProblem.numConstraint == 0) {
            printf("\tWarning: No constraints specified for modal case %s!!!!\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].name);
        }

    }


    if (strcasecmp(analysisType, "Static") == 0 ||
        strcasecmp(analysisType, "StaticOpt") == 0 ||
        strcasecmp(analysisType, "Optimization") == 0 ||
        strcasecmp(analysisType, "AeroelasticFlutter") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrim") == 0 ||
        strcasecmp(analysisType, "Aeroelastic") ==0) {

        // If we have multiple analysis structures
        if (nastranInstance[iIndex].feaProblem.numAnalysis != 0) {

            // Write subcase information if multiple analysis tuples were provide
            for (i = 0; i < nastranInstance[iIndex].feaProblem.numAnalysis; i++) {

                if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Static ||
                    nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Optimization ||
                    nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticTrim ||
                    nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticFlutter) {

                    fprintf(fp, "SUBCASE %d\n", i);
                    fprintf(fp, "\tLABEL %s\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].name);

                    if (strcasecmp(analysisType, "StaticOpt") == 0 || // StaticOpt is = Optimization
                            strcasecmp(analysisType, "Optimization") == 0) {
                        if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Static) {
                            fprintf(fp,"\tANALYSIS = STATICS\n");
                        } else if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Modal) {
                            fprintf(fp,"\tANALYSIS = MODES\n");
                        } else if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticTrim) {
                            fprintf(fp,"\tANALYSIS = SAERO\n");
                        } else if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticFlutter) {
                            fprintf(fp,"\tANALYSIS = FLUTTER\n");
                        } else if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Optimization) {
                            printf("\t *** WARNING :: INPUT TO ANALYSIS CASE INPUT analysisType should NOT be Optimization or StaticOpt - Defaulting to Static\n");
                            fprintf(fp,"\tANALYSIS = STATICS\n");
                        }
                    }

                    if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticTrim) {
                        fprintf(fp,"\tTRIM = %d\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
                    }

                    if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticFlutter) {
                        fprintf(fp,"\tMETHOD = %d\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
                        fprintf(fp,"\tFMETHOD = %d\n", 100+nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
                    }

                    if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticTrim ||
                        nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == AeroelasticFlutter) {

                        if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].aeroSymmetryXY != NULL) {

                            if(strcmp("SYM",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXY) == 0) {
                                fprintf(fp,"\tAESYMXY = %s\n","SYMMETRIC");
                            } else if(strcmp("ANTISYM",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXY) == 0) {
                                fprintf(fp,"\tAESYMXY = %s\n","ANTISYMMETRIC");
                            } else if(strcmp("ASYM",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXY) == 0) {
                                fprintf(fp,"\tAESYMXY = %s\n","ASYMMETRIC");
                            } else if(strcmp("SYMMETRIC",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXY) == 0) {
                                fprintf(fp,"\tAESYMXY = %s\n","SYMMETRIC");
                            } else if(strcmp("ANTISYMMETRIC",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXY) == 0) {
                                fprintf(fp,"\tAESYMXY = %s\n","ANTISYMMETRIC");
                            } else if(strcmp("ASYMMETRIC",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXY) == 0) {
                                fprintf(fp,"\tAESYMXY = %s\n","ASYMMETRIC");
                            } else {
                                printf("\t*** Warning *** aeroSymmetryXY Input %s to nastranAIM not understood!\n",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXY );
                            }

                        }

                        if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].aeroSymmetryXZ != NULL) {

                            if(strcmp("SYM",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXZ) == 0) {
                                fprintf(fp,"\tAESYMXZ = %s\n","SYMMETRIC");
                            } else if(strcmp("ANTISYM",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXZ) == 0) {
                                fprintf(fp,"\tAESYMXZ = %s\n","ANTISYMMETRIC");
                            } else if(strcmp("ASYM",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXZ) == 0) {
                                fprintf(fp,"\tAESYMXZ = %s\n","ASYMMETRIC");
                            } else if(strcmp("SYMMETRIC",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXZ) == 0) {
                                fprintf(fp,"\tAESYMXZ = %s\n","SYMMETRIC");
                            } else if(strcmp("ANTISYMMETRIC",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXZ) == 0) {
                                fprintf(fp,"\tAESYMXZ = %s\n","ANTISYMMETRIC");
                            } else if(strcmp("ASYMMETRIC",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXZ) == 0) {
                                fprintf(fp,"\tAESYMXZ = %s\n","ASYMMETRIC");
                            } else {
                                printf("\t*** Warning *** aeroSymmetryXZ Input %s to nastranAIM not understood!\n",nastranInstance[iIndex].feaProblem.feaAnalysis->aeroSymmetryXZ );
                            }

                        }
                    }


                    // Write support for sub-case
                    if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numSupport != 0) {
                        if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numSupport > 1) {
                            printf("\tWARNING: More than 1 support is not supported at this time for sub-cases!\n");

                        } else {
                            fprintf(fp, "\tSUPORT1 = %d\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].supportSetID[0]);
                        }
                    }

                    // Write constraint for sub-case
                    if (nastranInstance[iIndex].feaProblem.numConstraint != 0) {
                        fprintf(fp, "\tSPC = %d\n", nastranInstance[iIndex].feaProblem.numConstraint+i+1);
                    }

                    // Issue some warnings regarding constraints if necessary
                    if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint == 0 && nastranInstance[iIndex].feaProblem.numConstraint !=0) {
                        printf("\tWarning: No constraints specified for static case %s, assuming "
                                "all constraints are applied!!!!\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].name);
                    } else if (nastranInstance[iIndex].feaProblem.numConstraint == 0) {
                        printf("\tWarning: No constraints specified for static case %s!!!!\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].name);
                    }

                    // Write loads for sub-case
                    if (nastranInstance[iIndex].feaProblem.numLoad != 0) {
                        fprintf(fp, "\tLOAD = %d\n", nastranInstance[iIndex].feaProblem.numLoad+i+1);
                    }

                    // Issue some warnings regarding loads if necessary
                    if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numLoad == 0 && nastranInstance[iIndex].feaProblem.numLoad !=0) {
                        printf("\tWarning: No loads specified for static case %s, assuming "
                                "all loads are applied!!!!\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].name);
                    } else if (nastranInstance[iIndex].feaProblem.numLoad == 0) {
                        printf("\tWarning: No loads specified for static case %s!!!!\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].name);
                    }
                }

                if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Optimization) {
                    // Write objective function
                    //fprintf(fp, "\tDESOBJ(%s) = %d\n", nastranInstance[iIndex].feaProblem.feaAnalysis[i].objectiveMinMax,
                    //                                   nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
                    // Write optimization constraints
                    if (nastranInstance[iIndex].feaProblem.numDesignConstraint != 0) {
                        fprintf(fp, "\tDESSUB = %d\n", nastranInstance[iIndex].feaProblem.numDesignConstraint+i+1);
                    }
                }
            }

        } else { // If no sub-cases

            if (nastranInstance[iIndex].feaProblem.numSupport != 0) {
                if (nastranInstance[iIndex].feaProblem.numSupport > 1) {
                    printf("\tWARNING: More than 1 support is not supported at this time for a given case!\n");
                } else {
                    fprintf(fp, "SUPORT1 = %d ", nastranInstance[iIndex].feaProblem.numSupport+1);
                }
            }

            // Write constraint information
            if (nastranInstance[iIndex].feaProblem.numConstraint != 0) {
                fprintf(fp, "SPC = %d\n", nastranInstance[iIndex].feaProblem.numConstraint+1);
            } else {
                printf("\tWarning: No constraints specified for job!!!!\n");
            }

            // Write load card
            if (nastranInstance[iIndex].feaProblem.numLoad != 0) {
                fprintf(fp, "LOAD = %d\n", nastranInstance[iIndex].feaProblem.numLoad+1);
            } else {
                printf("\tWarning: No loads specified for job!!!!\n");
            }

            // What about an objective function if no analysis tuple? Do we need to add a new capsValue?

            // Write design constraints
            if (nastranInstance[iIndex].feaProblem.numDesignConstraint != 0) {
                fprintf(fp, "\tDESSUB = %d\n", nastranInstance[iIndex].feaProblem.numDesignConstraint+1);
            }
        }
    }

    //////////////// Bulk data ////////////////
    fprintf(fp, "\nBEGIN BULK\n");
    fprintf(fp, "$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|\n");
    //PRINT PARAM ENTRIES IN BULK DATA

    if (aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].nullVal == NotNull) {
        for (i = 0; i < aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].length; i++) {

            fprintf(fp, "PARAM, %s, %s\n", aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].vals.tuple[i].name,
                                           aimInputs[aim_getIndex(aimInfo, "Parameter", ANALYSISIN)-1].vals.tuple[i].value);
        }
    }

	fprintf(fp, "PARAM, %s\n", "POST, -1\n"); // Output OP2 file

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

        fprintf(fp,"%-8s", "DRESP1");

        tempString = convert_integerToString(1, 7, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%7s", delimiter, objectiveResp);
        fprintf(fp, "%s%7s", delimiter, objectiveResp);


        fprintf(fp, "\n");
    }

    // Write AEROS, AESTAT and AESURF cards
    if (strcasecmp(analysisType, "AeroelasticFlutter") == 0) {

        printf("\tWriting aero card\n");
        status = nastran_writeAEROCard(fp,
                                       &nastranInstance[iIndex].feaProblem.feaAeroRef,
                                       &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) return status;
    }

    // Write AEROS, AESTAT and AESURF cards
    if (strcasecmp(analysisType, "Aeroelastic") == 0 || strcasecmp(analysisType, "AeroelasticTrim") == 0) {

        printf("\tWriting aeros card\n");
        status = nastran_writeAEROSCard(fp,
                                        &nastranInstance[iIndex].feaProblem.feaAeroRef,
                                        &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) return status;

        numAEStatSurf = 0;
        for (i = 0; i < nastranInstance[iIndex].feaProblem.numAnalysis; i++) {

            if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType != AeroelasticTrim) continue;

            if (i == 0) printf("\tWriting aestat cards\n");

            // Loop over rigid variables
            for (j = 0; j < nastranInstance[iIndex].feaProblem.feaAnalysis[i].numRigidVariable; j++) {

                found = (int) false;

                // Loop over previous rigid variables
                for (k = 0; k < i; k++) {
                    for (l = 0; l < nastranInstance[iIndex].feaProblem.feaAnalysis[k].numRigidVariable; l++) {

                        // If current rigid variable was previous written - mark as found
                        if (strcmp(nastranInstance[iIndex].feaProblem.feaAnalysis[i].rigidVariable[j],
                                   nastranInstance[iIndex].feaProblem.feaAnalysis[k].rigidVariable[l]) == 0) {
                            found = (int) true;
                            break;
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

                fprintf(fp, "%s%7s\n", delimiter, nastranInstance[iIndex].feaProblem.feaAnalysis[i].rigidVariable[j]);
            }

            // Loop over rigid Constraints
            for (j = 0; j < nastranInstance[iIndex].feaProblem.feaAnalysis[i].numRigidConstraint; j++) {

                found = (int) false;

                // Loop over previous rigid constraints
                for (k = 0; k < i; k++) {
                    for (l = 0; l < nastranInstance[iIndex].feaProblem.feaAnalysis[k].numRigidConstraint; l++) {

                        // If current rigid constraint was previous written - mark as found
                        if (strcmp(nastranInstance[iIndex].feaProblem.feaAnalysis[i].rigidConstraint[j],
                                   nastranInstance[iIndex].feaProblem.feaAnalysis[k].rigidConstraint[l]) == 0) {
                            found = (int) true;
                            break;
                        }
                    }
                }

                // If already found continue
                if (found == (int) true) continue;

                // Make sure constraint isn't already in rigid variables too!
                for (k = 0; k < i; k++) {
                    for (l = 0; l < nastranInstance[iIndex].feaProblem.feaAnalysis[k].numRigidVariable; l++) {

                        // If current rigid constraint was previous written - mark as found
                        if (strcmp(nastranInstance[iIndex].feaProblem.feaAnalysis[i].rigidConstraint[j],
                                   nastranInstance[iIndex].feaProblem.feaAnalysis[k].rigidVariable[l]) == 0) {
                            found = (int) true;
                            break;
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

                fprintf(fp, "%s%7s\n", delimiter, nastranInstance[iIndex].feaProblem.feaAnalysis[i].rigidConstraint[j]);
            }
        }

        fprintf(fp,"\n");
    }

    // Analysis Cards - Eigenvalue and design objective included, as well as combined load, constraint, and design constraints
    if (nastranInstance[iIndex].feaProblem.numAnalysis != 0) {

        for (i = 0; i < nastranInstance[iIndex].feaProblem.numAnalysis; i++) {

            if (i == 0) printf("\tWriting analysis cards\n");

            status = nastran_writeAnalysisCard(fp,
                                               &nastranInstance[iIndex].feaProblem.feaAnalysis[i],
                                               &nastranInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) return status;

            if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numLoad != 0) {
                // Write combined load card
                printf("\tWriting load ADD cards\n");
                status = nastran_writeLoadADDCard(fp,
                                                  nastranInstance[iIndex].feaProblem.numLoad+i+1,
                                                  nastranInstance[iIndex].feaProblem.feaAnalysis[i].numLoad,
                                                  nastranInstance[iIndex].feaProblem.feaAnalysis[i].loadSetID,
                                                  nastranInstance[iIndex].feaProblem.feaLoad,
                                                  &nastranInstance[iIndex].feaProblem.feaFileFormat);
                if (status != CAPS_SUCCESS) return status;

            } else { // If no loads for an individual analysis are specified assume that all loads should be applied

                if (nastranInstance[iIndex].feaProblem.numLoad != 0) {

                    // Ignore thermal loads
                    k = 0;
                    for (j = 0; j < nastranInstance[iIndex].feaProblem.numLoad; j++) {
                        if (nastranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                        k += 1;
                    }

                    if (k != 0) {
                        // Create a temporary list of load IDs
                        tempIntegerArray = (int *) EG_alloc(k*sizeof(int));
                        if (tempIntegerArray == NULL) return EGADS_MALLOC;

                        k = 0;
                        for (j = 0; j < nastranInstance[iIndex].feaProblem.numLoad; j++) {

                            if (nastranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                            tempIntegerArray[k] = nastranInstance[iIndex].feaProblem.feaLoad[j].loadID;
                            k += 1;
                        }


                        // Write combined load card
                        printf("\tWriting load ADD cards\n");
                        status = nastran_writeLoadADDCard(fp,
                                                          nastranInstance[iIndex].feaProblem.numLoad+i+1,
                                                          k,
                                                          tempIntegerArray,
                                                          nastranInstance[iIndex].feaProblem.feaLoad,
                                                          &nastranInstance[iIndex].feaProblem.feaFileFormat);

                        // Free temporary load ID list
                        if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
                        tempIntegerArray = NULL;

                        if (status != CAPS_SUCCESS) return status;
                    }

                }
            }

            if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint != 0) {
                // Write combined constraint card
                printf("\tWriting constraint ADD cards\n");
                status = nastran_writeConstraintADDCard(fp,
                                                        nastranInstance[iIndex].feaProblem.numConstraint+i+1,
                                                        nastranInstance[iIndex].feaProblem.feaAnalysis[i].numConstraint,
                                                        nastranInstance[iIndex].feaProblem.feaAnalysis[i].constraintSetID,
                                                        &nastranInstance[iIndex].feaProblem.feaFileFormat);
                if (status != CAPS_SUCCESS) return status;

            } else { // If no constraints for an individual analysis are specified assume that all constraints should be applied

                if (nastranInstance[iIndex].feaProblem.numConstraint != 0) {

                    printf("\tWriting combined constraint cards\n");

                    // Create a temporary list of constraint IDs
                    tempIntegerArray = (int *) EG_alloc(nastranInstance[iIndex].feaProblem.numConstraint*sizeof(int));
                    if (tempIntegerArray == NULL) return EGADS_MALLOC;

                    for (j = 0; j < nastranInstance[iIndex].feaProblem.numConstraint; j++) {
                        tempIntegerArray[j] = nastranInstance[iIndex].feaProblem.feaConstraint[j].constraintID;
                    }

                    // Write combined constraint card
                    status = nastran_writeConstraintADDCard(fp,
                                                            nastranInstance[iIndex].feaProblem.numConstraint+i+1,
                                                            nastranInstance[iIndex].feaProblem.numConstraint,
                                                            tempIntegerArray,
                                                            &nastranInstance[iIndex].feaProblem.feaFileFormat);
                    if (status != CAPS_SUCCESS) goto cleanup;

                    // Free temporary constraint ID list
                    if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
                    tempIntegerArray = NULL;
                }
            }

            if (nastranInstance[iIndex].feaProblem.feaAnalysis[i].numDesignConstraint != 0) {

                // Write combined design constraint card
                printf("\tWriting design constraint ADD cards\n");
                status = nastran_writeDesignConstraintADDCard(fp,
                                                              nastranInstance[iIndex].feaProblem.numDesignConstraint+i+1,
                                                              nastranInstance[iIndex].feaProblem.feaAnalysis[i].numDesignConstraint,
                                                              nastranInstance[iIndex].feaProblem.feaAnalysis[i].designConstraintSetID,
                                                              &nastranInstance[iIndex].feaProblem.feaFileFormat);
                if (status != CAPS_SUCCESS) return status;

            } else { // If no design constraints for an individual analysis are specified assume that all design constraints should be applied

                if (nastranInstance[iIndex].feaProblem.numDesignConstraint != 0) {

                    // Create a temporary list of design constraint IDs
                    tempIntegerArray = (int *) EG_alloc(nastranInstance[iIndex].feaProblem.numDesignConstraint*sizeof(int));
                    if (tempIntegerArray == NULL) return EGADS_MALLOC;

                    for (j = 0; j < nastranInstance[iIndex].feaProblem.numDesignConstraint; j++) {
                        tempIntegerArray[j] = nastranInstance[iIndex].feaProblem.feaDesignConstraint[j].designConstraintID;
                    }

                    // Write combined design constraint card
                    printf("\tWriting design constraint ADD cards\n");
                    status = nastran_writeDesignConstraintADDCard(fp,
                                                                  nastranInstance[iIndex].feaProblem.numDesignConstraint+i+1,
                                                                  nastranInstance[iIndex].feaProblem.numDesignConstraint,
                                                                  tempIntegerArray,
                                                                  &nastranInstance[iIndex].feaProblem.feaFileFormat);
                    if (status != CAPS_SUCCESS) return status;

                    // Free temporary design constraint ID list
                    if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
                    tempIntegerArray = NULL;
                }

            }
        }

    } else { // If there aren't any analysis structures just write a single combined load , combined constraint,
             // and design constraint card

        // Combined loads
        if (nastranInstance[iIndex].feaProblem.numLoad != 0) {

            // Ignore thermal loads
            k = 0;
            for (j = 0; j < nastranInstance[iIndex].feaProblem.numLoad; j++) {
                if (nastranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                k += 1;
            }

            if (k != 0) {
                // Create a temporary list of load IDs
                tempIntegerArray = (int *) EG_alloc(k*sizeof(int));
                if (tempIntegerArray == NULL) return EGADS_MALLOC;

                k = 0;
                for (j = 0; j < nastranInstance[iIndex].feaProblem.numLoad; j++) {

                    if (nastranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                    tempIntegerArray[k] = nastranInstance[iIndex].feaProblem.feaLoad[j].loadID;
                    k += 1;
                }

                // Write combined load card
                printf("\tWriting load ADD cards\n");
                status = nastran_writeLoadADDCard(fp,
                                                  nastranInstance[iIndex].feaProblem.numLoad+1,
                                                  k,
                                                  tempIntegerArray,
                                                  nastranInstance[iIndex].feaProblem.feaLoad,
                                                  &nastranInstance[iIndex].feaProblem.feaFileFormat);

                if (status != CAPS_SUCCESS) goto cleanup;

                // Free temporary load ID list
                if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
                tempIntegerArray = NULL;
            }
        }

        // Combined constraints
        if (nastranInstance[iIndex].feaProblem.numConstraint != 0) {

            printf("\tWriting combined constraint cards\n");

            // Create a temporary list of constraint IDs
            tempIntegerArray = (int *) EG_alloc(nastranInstance[iIndex].feaProblem.numConstraint*sizeof(int));
            if (tempIntegerArray == NULL) return EGADS_MALLOC;

            for (i = 0; i < nastranInstance[iIndex].feaProblem.numConstraint; i++) {
                tempIntegerArray[i] = nastranInstance[iIndex].feaProblem.feaConstraint[i].constraintID;
            }

            // Write combined constraint card
            status = nastran_writeConstraintADDCard(fp,
                                                    nastranInstance[iIndex].feaProblem.numConstraint+1,
                                                    nastranInstance[iIndex].feaProblem.numConstraint,
                                                    tempIntegerArray,
                                                    &nastranInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Free temporary constraint ID list
            if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
            tempIntegerArray = NULL;
        }

        // Combined design constraints
        if (nastranInstance[iIndex].feaProblem.numDesignConstraint != 0) {

            printf("\tWriting design constraint cards\n");
            // Create a temporary list of design constraint IDs
            tempIntegerArray = (int *) EG_alloc(nastranInstance[iIndex].feaProblem.numDesignConstraint*sizeof(int));
            if (tempIntegerArray == NULL) return EGADS_MALLOC;

            for (i = 0; i < nastranInstance[iIndex].feaProblem.numDesignConstraint; i++) {
                tempIntegerArray[i] = nastranInstance[iIndex].feaProblem.feaDesignConstraint[i].designConstraintID;
            }

            // Write combined design constraint card
            status = nastran_writeDesignConstraintADDCard(fp,
                                                          nastranInstance[iIndex].feaProblem.numDesignConstraint+1,
                                                          nastranInstance[iIndex].feaProblem.numDesignConstraint,
                                                          tempIntegerArray,
                                                          &nastranInstance[iIndex].feaProblem.feaFileFormat);

            if (status != CAPS_SUCCESS) goto cleanup;

            // Free temporary design constraint ID list
            if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
            tempIntegerArray = NULL;
        }


    }

    // Loads
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numLoad; i++) {

        if (i == 0) printf("\tWriting load cards\n");

        status = nastran_writeLoadCard(fp,
                                       &nastranInstance[iIndex].feaProblem.feaLoad[i],
                                       &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Constraints
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numConstraint; i++) {

        if (i == 0) printf("\tWriting constraint cards\n");

        status = nastran_writeConstraintCard(fp,
                                             &nastranInstance[iIndex].feaProblem.feaConstraint[i],
                                             &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Supports
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numSupport; i++) {

        if (i == 0) printf("\tWriting support cards\n");
        j = (int) true;
        status = nastran_writeSupportCard(fp,
                                          &nastranInstance[iIndex].feaProblem.feaSupport[i],
                                          &nastranInstance[iIndex].feaProblem.feaFileFormat,
                                          &j);
        if (status != CAPS_SUCCESS) goto cleanup;
    }


    // Materials
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numMaterial; i++) {

        if (i == 0) printf("\tWriting material cards\n");

        status = nastran_writeMaterialCard(fp,
                                           &nastranInstance[iIndex].feaProblem.feaMaterial[i],
                                           &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Properties
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numProperty; i++) {

        if (i == 0) printf("\tWriting property cards\n");

        status = nastran_writePropertyCard(fp,
                                           &nastranInstance[iIndex].feaProblem.feaProperty[i],
                                           &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Coordinate systems
    for (i = 0; i < nastranInstance[iIndex].feaProblem.numCoordSystem; i++) {

        if (i == 0) printf("\tWriting coordinate system cards\n");

        status = nastran_writeCoordinateSystemCard(fp, &nastranInstance[iIndex].feaProblem.feaCoordSystem[i], &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Optimization - design variables
    for( i = 0; i < nastranInstance[iIndex].feaProblem.numDesignVariable; i++) {

        if (i == 0) printf("\tWriting design variables and analysis - design variable relation cards\n");

        status = nastran_writeDesignVariableCard(fp,
                                                 &nastranInstance[iIndex].feaProblem.feaDesignVariable[i],
                                                 &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Optimization - design constraints
    for( i = 0; i < nastranInstance[iIndex].feaProblem.numDesignConstraint; i++) {

        if (i == 0) printf("\tWriting design constraints and responses cards\n");

        status = nastran_writeDesignConstraintCard(fp,
                                                   &nastranInstance[iIndex].feaProblem.feaDesignConstraint[i],
                                                   &nastranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Aeroelastic
    if (strcasecmp(analysisType, "Aeroelastic") == 0 ||
        strcasecmp(analysisType, "AeroelasticTrim") == 0 ||
        strcasecmp(analysisType, "AeroelasticFlutter") == 0) {

        printf("\tWriting aeroelastic cards\n");
        for (i = 0; i < nastranInstance[iIndex].feaProblem.numAero; i++){
            status = nastran_writeCAeroCard(fp,
                                            &nastranInstance[iIndex].feaProblem.feaAero[i],
                                            &nastranInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = nastran_writeAeroSplineCard(fp,
                                                 &nastranInstance[iIndex].feaProblem.feaAero[i],
                                                 &nastranInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = nastran_writeSet1Card(fp,
                                           &nastranInstance[iIndex].feaProblem.feaAero[i],
                                           &nastranInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }

    // Include mesh file
    fprintf(fp,"\nINCLUDE \'%s.bdf\'\n\n", nastranInstance[iIndex].projectName);

    // End bulk data
    fprintf(fp,"ENDDATA\n");

    fclose(fp);
    fp = NULL;
/*
////////////////////////////////////////
    printf("\n\n\nTESTING F06 READER\n\n");

    // FO6 data variables
    int numGridPoint = 0;
    int numEigenVector = 0;
    double **dataMatrix = NULL;
    filename = (char *) EG_alloc((strlen(nastranInstance[iIndex].analysisPath) + strlen(nastranInstance[iIndex].projectName) +
                                      strlen(".f06") + 2)*sizeof(char));

    sprintf(filename,"%s/%s%s", nastranInstance[iIndex].analysisPath,
                                nastranInstance[iIndex].projectName, ".f06");

    // Open file
    fp = fopen(filename, "r");
    if (filename != NULL) EG_free(filename);
    filename = NULL;

    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);

        return CAPS_IOERR;
    }

    status = nastran_readF06Displacement(fp,
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
        if (status != CAPS_SUCCESS) printf("Error: Status %d during nastranAIM preAnalysis\n", status);

        if (tempIntegerArray != NULL) EG_free(tempIntegerArray);

        if (fp != NULL) fclose(fp);

        return status;
}

// Set Nastran output variables
int aimOutputs(int iIndex, void *aimStruc, int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsNastran AIM Outputs
     * The following list outlines the Nastran outputs available through the AIM interface.
     */

    #ifdef DEBUG
        printf(" nastranAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
    #endif

    /*! \page aimOutputsNastran AIM Outputs
     * - <B>EigenValue</B> = List of Eigen-Values (\f$ \lambda\f$) after a modal solve.
     * - <B>EigenRadian</B> = List of Eigen-Values in terms of radians (\f$ \omega = \sqrt{\lambda}\f$ ) after a modal solve.
     * - <B>EigenFrequency</B> = List of Eigen-Values in terms of frequencies (\f$ f = \frac{\omega}{2\pi}\f$) after a modal solve.
     * - <B>EigenGeneralMass</B> = List of generalized masses for the Eigen-Values.
     * - <B>EigenGeneralStiffness</B> = List of generalized stiffness for the Eigen-Values.
     * .
     */

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
    }

    form->type       = Double;
    form->units      = NULL;
    form->lfixed     = Change;
    form->sfixed     = Change;
    form->vals.reals = NULL;
    form->vals.real  = 0;

    return CAPS_SUCCESS;
}

// Calculate Nastran output
int aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath,
              	  int index, capsValue *val, capsErrs **errors)
{
    int status = CAPS_SUCCESS; // Function return status

    int i; //Indexing

    int numEigenVector;
    double **dataMatrix = NULL;

    char currentPath[PATH_MAX]; // Current directory path

    char *filename = NULL; // File to open
    char extF06[] = ".f06";
    FILE *fp = NULL; // File pointer

    (void) getcwd(currentPath, PATH_MAX); // Get our current path

    // Set path to analysis directory
    if (chdir(analysisPath) != 0) {
    #ifdef DEBUG
        printf(" nastranAIM/aimCalcOutput Cannot chdir to %s!\n", analysisPath);
    #endif

        return CAPS_DIRERR;
    }

    if (index <= 5) {
        filename = (char *) EG_alloc((strlen(nastranInstance[iIndex].projectName) + strlen(extF06) +1)*sizeof(char));
        if (filename == NULL) return EGADS_MALLOC;

        sprintf(filename, "%s%s", nastranInstance[iIndex].projectName, extF06);

        fp = fopen(filename, "r");

        EG_free(filename); // Free filename allocation

        if (fp == NULL) {
            #ifdef DEBUG
                printf(" nastranAIM/aimCalcOutput Cannot open Output file!\n");
            #endif

            chdir(currentPath);

            return CAPS_IOERR;
        }

        status = nastran_readF06EigenValue(fp, &numEigenVector, &dataMatrix);
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
    }

    // Restore the path we came in with
    chdir(currentPath);
    if (fp != NULL) fclose(fp);

    if (dataMatrix != NULL) {
        for (i = 0; i < numEigenVector; i++) {
            if (dataMatrix[i] != NULL) EG_free(dataMatrix[i]);
        }
        EG_free(dataMatrix);
    }

    return status;
}


void aimCleanup()
{
    int iIndex; // Indexing

    int status; // Returning status

    #ifdef DEBUG
        printf(" nastranAIM/aimCleanup!\n");
    #endif

    if (numInstance != 0) {

        // Clean up nastranInstance data
        for ( iIndex = 0; iIndex < numInstance; iIndex++) {

            printf(" Cleaning up nastranInstance - %d\n", iIndex);

            status = destroy_aimStorage(iIndex);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during clean up of instance %d\n", status, iIndex);
        }

    }
    numInstance = 0;

    if (nastranInstance != NULL) EG_free(nastranInstance);
    nastranInstance = NULL;

}


int aimFreeDiscr(capsDiscr *discr)
{
    int i; // Indexing

    #ifdef DEBUG
        printf(" nastranAIM/aimFreeDiscr instance = %d!\n", discr->instance);
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
        printf(" nastranAIM/aimDiscr: tname = %s, instance = %d!\n", tname, iIndex);
    #endif

    if ((iIndex < 0) || (iIndex >= numInstance)) return CAPS_BADINDEX;


    if (tname == NULL) return CAPS_NOTFOUND;

    /*if (nastranInstance[iIndex].dataTransferCheck == (int) false) {
        printf("The volume is not suitable for data transfer - possibly the volume mesher "
                "added unaccounted for points\n");
        return CAPS_BADVALUE;
    }*/

    // Currently this ONLY works if the capsTranfer lives on single body!
    status = aim_getBodies(discr->aInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
        printf(" nastranAIM/aimDiscr: %d aim_getBodies = %d!\n", iIndex, status);
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
            printf("nastranAIM: getBodyTopos (Face) = %d for Body %d!\n", status, body);
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
              printf("nastranAIM: WARNING: capsIgnore found on bound %s\n", tname);
              continue;
            }

            #ifdef DEBUG
                printf(" nastranAIM/aimDiscr: Body %d/Face %d matches %s!\n", body, face+1, tname);
            #endif

            status = retrieve_CAPSGroupAttr(faces[face], &capsGroup);
            if (status != CAPS_SUCCESS) {
                printf("capsBound found on face %d, but no capGroup found!!!\n", face);
                continue;
            } else {

                status = get_mapAttrToIndexIndex(&nastranInstance[iIndex].attrMap, capsGroup, &attrIndex);
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
                    printf(" nastranAIM: EG_getTessFace %d = %d for Body %d!\n", face+1, status, body+1);
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
        printf(" nastranAIM/aimDiscr: No Faces match %s!\n", tname);
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Debug
#ifdef DEBUG
    printf(" nastranAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
    printf(" nastranAIM/aimDiscr: nquad = %d, npts = %d!\n", numQuad, numPoint);
#endif

    if ( numPoint == 0 || (numTri == 0 && numQuad == 0) ) {
        #ifdef DEBUG
            printf(" nastranAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
            printf(" nastranAIM/aimDiscr: nquad = %d, npts = %d!\n", numQuad, numPoint);
        #endif
        status = CAPS_SOURCEERR;
        goto cleanup;
    }

    #ifdef DEBUG
        printf(" nastranAIM/aimDiscr: Instance %d, Body Index for data transfer = %d\n", iIndex, dataTransferBodyIndex);
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
            printf(" nastranAIM: EG_getTessFace %d = %d for Body %d!\n", bodyFaceMap[2*face + 1], status, bodyFaceMap[2*face + 0]);
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
                printf("nastranAIM/aimDiscr: EG_localToGlobal accidentally only works for a single quad patch! FIXME!\n");
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
        printf(" nastranAIM/aimDiscr: ntris = %d, npts = %d!\n", discr->nElems, discr->nPoints);
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
        //    printf(" nastranAIM/aimDiscr: Instance = %d, Global Node ID %d\n", iIndex, storage[i]);
        //#endif
    }

    // Save way the attrMap capsGroup list
    storage[numPoint] = numCAPSGroup;
    for (i = 0; i < numCAPSGroup; i++) {
        storage[numPoint+1+i] = capsGroupList[i];
    }

    #ifdef DEBUG
        printf(" nastranAIM/aimDiscr: Instance = %d, Finished!!\n", iIndex);
    #endif

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\tPremature exit: function aimDiscr nastranAIM status = %d", status);

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
  /*! \page aimUsesDataSetNastran data sets consumed by Nastran
   *
   * This function checks if a data set name can be consumed by this aim.
   * The Nastran aim can consume "Pressure" data sets for aeroelastic analysis.
   */

  if (strcasecmp(dname, "Pressure") == 0) {
      return CAPS_SUCCESS;
  }

  return CAPS_NOTNEEDED;
}

int aimTransfer(capsDiscr *discr, const char *dataName, int numPoint, int dataRank, double *dataVal, char **units)
{

    /*! \page dataTransferNastran Nastran Data Transfer
     *
     * The Nastran AIM has the ability to transfer displacements and eigenvectors from the AIM and pressure
     * distributions to the AIM using the conservative and interpolative data transfer schemes in CAPS.
     * Currently these transfers may only take place on triangular meshes.
     *
     * \section dataFromNastran Data transfer from Nastran
     *
     * <ul>
     *  <li> <B>"Displacement"</B> </li> <br>
     *   Retrieves nodal displacements from the *.f06 file.
     * </ul>
     *
     * <ul>
     *  <li> <B>"EigenVector_#"</B> </li> <br>
     *   Retrieves modal eigen-vectors from the *.f06 file, where "#" should be replaced by the
     *   corresponding mode number for the eigen-vector (eg. EigenVector_3 would correspond to the third mode,
     *   while EigenVector_6 would be the sixth mode).
     * </ul>
     *
     * \section dataToNastran Data transfer to Nastran
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

    char *extF06 = ".f06";

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
        printf(" nastranAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n", dataName, discr->instance, numPoint, dataRank);
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
    if (chdir(nastranInstance[discr->instance].analysisPath) != 0) {
        #ifdef DEBUG
            printf(" nastranAIM/aimTransfer Cannot chdir to %s!\n", nastranInstance[discr->instance].analysisPath);
        #endif

        return CAPS_DIRERR;
    }

    filename = (char *) EG_alloc((strlen(nastranInstance[discr->instance].projectName) +
                                  strlen(extF06) + 1)*sizeof(char));

    sprintf(filename,"%s%s",nastranInstance[discr->instance].projectName,extF06);

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

            status = nastran_readF06Displacement(fp,
                                                 -1,
                                                 &numGridPoint,
                                                 &dataMatrix);
            fclose(fp);
            fp = NULL;
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

            status = nastran_readF06EigenVector(fp,
                                                &numEigenVector,
                                                &numGridPoint,
                                                &dataMatrix);
        }

        fclose(fp);
        fp = NULL;

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
        printf(" nastranAIM/aimInterpolation  %s  instance = %d!\n",
         name, discr->instance);
    #endif
    */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" nastranAIM/Interpolation: eIndex = %d [1-%d]!\n",eIndex, discr->nElems);
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
        printf(" nastranAIM/Interpolation: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
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
        printf(" nastranAIM/aimInterpolateBar  %s  instance = %d!\n", name, discr->instance);
    #endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" nastranAIM/InterpolateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
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
        printf(" nastranAIM/InterpolateBar: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
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
        printf(" nastranAIM/aimIntegration  %s  instance = %d!\n", name, discr->instance);
    #endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" nastranAIM/aimIntegration: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" nastranAIM/aimIntegration: %d aim_getBodies = %d!\n", discr->instance, stat);
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
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                            discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
        printf(" nastranAIM/aimIntegration: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
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
        printf(" nastranAIM/aimIntegrateBar  %s  instance = %d!\n", name, discr->instance);
    #endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" nastranAIM/aimIntegrateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" nastranAIM/aimIntegrateBar: %d aim_getBodies = %d!\n", discr->instance, stat);
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
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                            discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" nastranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
        printf(" nastranAIM/aimIntegrateBar: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
        return CAPS_BADVALUE;
    }

    return CAPS_SUCCESS;
}
