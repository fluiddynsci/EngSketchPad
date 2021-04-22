// Replace "Pointwise" and "pointwise" the "Name" and "name" of AIM respectfully!

/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Pointwise AIM
 *
 *      Modified from code written by Author, Affiliation
 *
 */


/*!\mainpage Introduction
 *
 * \section overviewPointwise Pointwise AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact with the
 * general grid generator <a href="https://www.pointwise.com/">Pointwise</a>.
 *
 * The Pointwise AIM provides the CAPS users with the ability to generate volume meshes mostly suitable for CFD analysis.
 * This includes both inviscid analysis and viscous analysis with boundary layers using the pointwise T-Rex algorithm.
 *
 * An outline of the AIM's inputs, outputs and attributes are provided in \ref aimInputsPointwise and
 * \ref aimOutputsPointwise and \ref attributePointwise, respectively.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataPointwise if connecting this AIM to other AIMs in a
 * parent-child like manner.
 *
 * Files output:
 *  - caps.egads - Pointwise egads file generated
 *  - capsUserDefaults.glf - Glyph script with parameters set with \ref aimInputsPointwise
 *
 * Pointwise should be executed on Linux/macOS with the command line:
 *
 * \code{.sh}
 * pointwise -b $CAPS_GLYPH/GeomToMesh.glf caps.egads capsUserDefaults.glf
 * \endcode
 *
 * and on Windows with:
 *
 * \code{.sh}
 * %PW_HOME%\win64\bin\tclsh.exe %CAPS_GLYPH%\GeomToMesh.glf caps.egads capsUserDefaults.glf
 * \endcode
 */

 /*! \page attributePointwise AIM Attributes
 * The following list of attributes are available to guide the mesh generation with the Pointwise AIM.
 *
 * | Key                      | Value                                                                  | Geometry Location  |  Description |
 * | :------------------------| :--------------------------------------------------------------------: | :----------------: | :----------- |
 * | PW:NodeSpacing           | > 0.0                                                                  | Node               | Specified connector endpoint spacing for a node.                                                                                    |
 * |                          |                                                                        |                    |                                                                                                                                     |
 * | PW:ConnectorMaxEdge      | > 0.0                                                                  | Edge               | Maximum Edge Length in connector.                                                                                                   |
 * | PW:ConnectorEndSpacing   | > 0.0                                                                  | Edge               | Specified connector endpoint spacing.                                                                                               |
 * | PW:ConnectorDimension    | > 0                                                                    | Edge               | Specify connector dimension.                                                                                                        |
 * | PW:ConnectorAverageDS    | > 0.0                                                                  | Edge               | Specified average delta spacing for connector dimension.                                                                            |
 * | PW:ConnectorMaxAngle     | [ 0, 180 )                                                             | Edge               | Connector Maximum Angle. (0.0 = NOT APPLIED)                                                                                        |
 * | PW:ConnectorMaxDeviation | [ 0, infinity )                                                        | Edge               | Connector Maximum Deviation. (0.0 = NOT APPLIED)                                                                                    |
 * | PW:ConnectorAdaptSource  | $true or $false                                                        | Edge               | Set connector up for adaptation as a source                                                                                         |
 * |                          |                                                                        |                    |                                                                                                                                     |
 * | PW:Name                  | Set by pointwiseAIM to the value of capsGroup                          | Face               | Boundary name for domain or collection of domains.                                                                                  |
 * | PW:QuiltName             | Quilting is not supported with CAPS, but input files are generated     | Face               | Name to give one or more quilts that are assembled into a single quilt. <br>No angle test is performed.                             |
 * | PW:Baffle                | $Baffle or $Intersect                                                  | Face               | Either a true baffle surface or a surface intersected by a baffle.                                                                  |
 * | PW:DomainAlgorithm       | $Delaunay, $AdvancingFront, $AdvancingFrontOrtho                       | Face               | Surface meshing algorithm.                                                                                                          |
 * | PW:DomainIsoType         | $Triangle, $TriangleQuad                                               | Face               | Surface cell type. Global default is Triangle.                                                                                      |
 * | PW:DomainMinEdge         | > 0.0                                                                  | Face               | Cell Minimum Equilateral Edge Length in domain. (0.0 = USE BOUNDARY)                                                                |
 * | PW:DomainMaxEdge         | > 0.0                                                                  | Face               | Cell Maximum Equilateral Edge Length in domain. (0.0 = USE BOUNDARY)                                                                |
 * | PW:DomainMaxAngle        | [ 0, 180 )                                                             | Face               | Cell Maximum Angle in domain (0.0 = NOT APPLIED)                                                                                    |
 * | PW:DomainMaxDeviation    | [ 0, infinity )                                                        | Face               | Cell Maximum Deviation in domain (0.0 = NOT APPLIED)                                                                                |
 * | PW:DomainSwapCells       | $true or $false                                                        | Face               | Swap cells with no interior points.                                                                                                 |
 * | PW:DomainQuadMaxAngle    | ( 90, 180 )                                                            | Face               | Quad Maximum Included Angle in domain.                                                                                              |
 * | PW:DomainQuadMaxWarp     | ( 0, 90 )                                                              | Face               | Cell Maximum Warp Angle in domain.                                                                                                  |
 * | PW:DomainDecay           | [ 0, 1 ]                                                               | Face               | Boundary decay applied on domain.                                                                                                   |
 * | PW:DomainMaxLayers       | [ 0, infinity )                                                        | Face               | Maximum T-Rex layers in domain.                                                                                                     |
 * | PW:DomainFullLayers      | [ 0, infinity )                                                        | Face               | Number of full T-Rex layers in domain. (0 allows multi-normals)                                                                     |
 * | PW:DomainTRexGrowthRate  | [ 1, infinity )                                                        | Face               | T-Rex growth rate in domain.                                                                                                        |
 * | PW:DomainTRexType        | $Triangle, $TriangleQuad                                               | Face               | Cell types in T-Rex layers in domain.                                                                                               |
 * | PW:DomainTRexIsoHeight   | > 0.0                                                                  | Face               | Isotropic height for T-Rex cells in domain. Default is 1.0.                                                                         |
 * | PW:PeriodicTranslate     | "tx; ty; tz"                                                           | Face               | Periodic domain with given translation vector.                                                                                      |
 * | PW:PeriodicRotate        | "px; py; pz; nx; ny; nz; angle"                                        | Face               | Periodic domain with given point, normal and rotation angle.                                                                        |
 * | PW:PeriodicTarget        | $true or $false                                                        | Face               | Target domain of a translate or rotate periodic domain. <br>This domain will be deleted before the creation of the periodic domain. |
 * | PW:DomainAdaptSource     | $true or $false                                                        | Face               | Set domain up for adaptation as a source                                                                                            |
 * | PW:DomainAdaptTarget     | $true or $false                                                        | Face               | Set domain up for adaptation as a target                                                                                            |
 * | PW:DomainShapeConstraint | $DataBase or $Free                                                     | Face               | Set the domain shape constraint                                                                                                     |
 * | PW:WallSpacing           | $Wall or > 0.0                                                         | Face               | Viscous normal spacing for T-Rex extrusion. $Wall uses domParams(WallSpacing)                                                       |
 */

 /* These are set by capsUserDefaults.glf rather than as attributes the model
 * |                          |                                                                        |                    |                                                                                                                                     |
 * | PW:TRexIsoHeight         | > 0.0                                                                  | Model              | Isotropic height for volume T-Rex cells. Default is 1.0.                                                                            |
 * | PW:TRexCollisionBuffer   | > 0.0                                                                  | Model              | T-Rex collision buffer. Default is 0.5.                                                                                             |
 * | PW:TRexMaxSkewAngle      | [ 0, 180 ]                                                             | Model              | T-Rex maximum skew angle. Default 180 (Off).                                                                                        |
 * | PW:TRexGrowthRate        | [ 1, infinity )                                                        | Model              | T-Rex growth rate.                                                                                                                  |
 * | PW:TRexType              | $TetPyramid, $TetPyramidPrismHex, <br>or $AllAndConvertWallDoms        | Model              | T-Rex cell type.                                                                                                                    |
 * | PW:BoundaryDecay         | [ 0, 1 ]                                                               | Model              | Volumetric boundary decay. Default is 0.5.                                                                                          |
 * | PW:EdgeMaxGrowthRate     | [ 1, infinity )                                                        | Model              | Volumetric edge maximum growth rate. Default is 1.8.                                                                                |
 * | PW:MinEdge               | $Boundary or > 0.0                                                     | Model              | Tetrahedral Minimum Equilateral Edge Length in block.                                                                               |
 * | PW:MaxEdge               | $Boundary or > 0.0                                                     | Model              | Tetrahedral Maximum Equilateral Edge Length in block.                                                                               |
 */

// Header functions
#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"       // Collection of helper functions for meshing
#include "miscUtils.h"       // Collection of helper functions for miscellaneous analysis

#include "egads.h"

#include "hashElement.h"

// Windows aliasing
#ifdef WIN32
#define getcwd     _getcwd
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define PTOL               1.e-5
#define PI                 3.1415926535897931159979635
#define CROSS(a,b,c)       a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                           a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                           a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         ((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2])


#define NUMINPUT  40  // number of mesh aimInputs
#define NUMOUT    1   // number of outputs


// this easter egg function returns result which is value of the p-curve and it's derivatives w.r.t 't' so
// used to detect periodicity in parameter space
  extern int EG_getEdgeUVeval( const ego face, const ego topo, int sense,
                               double t, double *result );

typedef struct {
  double **rvec;
  ego    *surfaces;
  ego    body;
  ego    *faces;
  ego    *edges;
  ego    *nodes;
  int    nfaces;
  int    nedges;
  int    nnodes;

  int    *nodes_isp; // surface mesh index

  int    *edges_npts;
  double **edges_xyz;
  double **edges_t;
  int    **edges_isp; // surface mesh index

  int    *faces_npts;
  double **faces_xyz;
  double **faces_uv;
  int    *faces_ntri;
  int    *faces_nquad;
  int    **faces_tris;
} bodyData;

typedef struct {
  int ind;         // global index into UGRID file
  int egadsID;     // egadsID encoding type, body, and type-index
  double param[2]; // parametric coordinates of the vertex
} gmaVertex;

//#define DEBUG
//#define CHECKGRID // check that parametric evaluation gives the grid xyz

static int initiate_bodyData(int numBody, bodyData *bodydata) {

  int i;

  for (i = 0; i < numBody; i++) {
      bodydata[i].rvec = NULL;
      bodydata[i].surfaces = NULL;
      bodydata[i].faces = NULL;
      bodydata[i].edges = NULL;
      bodydata[i].nodes = NULL;
      bodydata[i].nfaces = 0;
      bodydata[i].nedges = 0;
      bodydata[i].nnodes = 0;
      bodydata[i].nodes_isp = NULL;
      bodydata[i].edges_npts = NULL;
      bodydata[i].edges_xyz = NULL;
      bodydata[i].edges_isp = NULL;
      bodydata[i].edges_t = NULL;
      bodydata[i].faces_npts = NULL;
      bodydata[i].faces_xyz = NULL;
      bodydata[i].faces_uv = NULL;
      bodydata[i].faces_ntri = NULL;
      bodydata[i].faces_nquad = NULL;
      bodydata[i].faces_tris = NULL;
  }

  return CAPS_SUCCESS;
}

static int destroy_bodyData(int numBody, bodyData *bodydata) {

  int i, j;

  if (bodydata == NULL) return CAPS_SUCCESS;

  for (i = 0; i < numBody; i++) {
    for (j = 0; j < bodydata[i].nfaces; j++) {
      if (bodydata[i].surfaces != NULL)
        if (bodydata[i].surfaces[j+bodydata[i].nfaces] != NULL)
          EG_deleteObject(bodydata[i].surfaces[j+bodydata[i].nfaces]);
      if (bodydata[i].rvec != NULL)
        EG_free(bodydata[i].rvec[j]);
    }
    EG_free(bodydata[i].nodes);
    EG_free(bodydata[i].edges);
    EG_free(bodydata[i].faces);
    EG_free(bodydata[i].surfaces);
    EG_free(bodydata[i].rvec);

    EG_free(bodydata[i].nodes_isp);
    EG_free(bodydata[i].edges_npts);
    EG_free(bodydata[i].faces_npts);
    EG_free(bodydata[i].faces_ntri);
    EG_free(bodydata[i].faces_nquad);

    if (bodydata[i].edges_xyz != NULL) {
      for (j = 0; j < bodydata[i].nedges; j++)
        EG_free(bodydata[i].edges_xyz[j]);
      EG_free(bodydata[i].edges_xyz);
    }

    if (bodydata[i].edges_t != NULL) {
      for (j = 0; j < bodydata[i].nedges; j++)
        EG_free(bodydata[i].edges_t[j]);
      EG_free(bodydata[i].edges_t);
    }

    if (bodydata[i].edges_isp != NULL) {
      for (j = 0; j < bodydata[i].nedges; j++)
        EG_free(bodydata[i].edges_isp[j]);
      EG_free(bodydata[i].edges_isp);
    }

    if (bodydata[i].faces_xyz != NULL) {
      for (j = 0; j < bodydata[i].nfaces; j++)
        EG_free(bodydata[i].faces_xyz[j]);
      EG_free(bodydata[i].faces_xyz);
    }

    if (bodydata[i].faces_uv != NULL) {
      for (j = 0; j < bodydata[i].nfaces; j++)
        EG_free(bodydata[i].faces_uv[j]);
      EG_free(bodydata[i].faces_uv);
    }

    if (bodydata[i].faces_tris != NULL) {
      for (j = 0; j < bodydata[i].nfaces; j++)
        EG_free(bodydata[i].faces_tris[j]);
      EG_free(bodydata[i].faces_tris);
    }
  }

  return CAPS_SUCCESS;
}

typedef enum {
  MODELID,  // 0
  SHELLID,  // 1
  FACEID,   // 2
  LOOPID,   // 3
  EDGEID,   // 4
  COEDGEID, // 5
  NODEID    // 6
} egadsIDEnum;


void decodeEgadsID( int id, int *type, int *bodyID, int *index ) {

    //#define PACK(t, m, i)       ((t)<<28 | (m)<<20 | (i))
    //#define UNPACK(v, t, m, i)  t = v>>28; m = (v>>20)&255; i = v&0xFFFFF;

  *type   =  id >> 28;
  *bodyID = (id >> 20) & 255;
  *index  =  id        & 0xFFFFF;

  // change to 0-baed indexing
  *bodyID -= 1;
  *index  -= 1;
}


// Additional storage values for the instance of the AIM
typedef struct {

  // Container for volume mesh
  int numVolumeMesh;
  meshStruct *volumeMesh;

  // Container for surface mesh
  int numSurfaceMesh;
  meshStruct *surfaceMesh;

  // Attribute to index map
  mapAttrToIndexStruct attrMap;

} aimStorage;

static aimStorage *pointwiseInstance = NULL;
static int         numInstance  = 0;


static int initiate_aimStorage(int iIndex) {

  int status = CAPS_SUCCESS;

  pointwiseInstance[iIndex].numVolumeMesh = 0;
  pointwiseInstance[iIndex].volumeMesh = NULL;

  pointwiseInstance[iIndex].numSurfaceMesh = 0;
  pointwiseInstance[iIndex].surfaceMesh = NULL;

  // Destroy attribute to index map
  status = initiate_mapAttrToIndexStruct(&pointwiseInstance[iIndex].attrMap);
  if (status != CAPS_SUCCESS) return status;

  return CAPS_SUCCESS;
}

static int destroy_aimStorage(int iIndex) {


  int i; // Indexing

  int status; // Function return status

  // Destroy volume mesh allocated arrays
  for (i = 0; i < pointwiseInstance[iIndex].numVolumeMesh; i++) {
      status = destroy_meshStruct(&pointwiseInstance[iIndex].volumeMesh[i]);
      if (status != CAPS_SUCCESS) printf("Status = %d, pointwiseAIM instance %d, volumeMesh cleanup!!!\n", status, iIndex);
  }
  pointwiseInstance[iIndex].numVolumeMesh = 0;

  EG_free(pointwiseInstance[iIndex].volumeMesh);
  pointwiseInstance[iIndex].volumeMesh = NULL;

  // Destroy surface mesh allocated arrays
  for (i = 0; i < pointwiseInstance[iIndex].numSurfaceMesh; i++) {
      status = destroy_meshStruct(&pointwiseInstance[iIndex].surfaceMesh[i]);
      if (status != CAPS_SUCCESS) printf("Status = %d, pointwiseAIM instance %d, surfaceMesh cleanup!!!\n", status, iIndex);
  }
  pointwiseInstance[iIndex].numSurfaceMesh = 0;

  EG_free(pointwiseInstance[iIndex].surfaceMesh);
  pointwiseInstance[iIndex].surfaceMesh = NULL;

  // Destroy attribute to index map
  status = destroy_mapAttrToIndexStruct(&pointwiseInstance[iIndex].attrMap);
  if (status != CAPS_SUCCESS) printf("Status = %d, pointwiseAIM instance %d, attributeMap cleanup!!!\n", status, iIndex);

    return CAPS_SUCCESS;
}


static int getUGRID(FILE *fp, meshStruct *volumeMesh)
{
  int    status = CAPS_SUCCESS;

  int    numNode, numTriangle, numQuadrilateral;
  int    numTetrahedral, numPyramid, numPrism, numHexahedral;
  int    i, elementIndex, numPoint;
  int    defaultVolID = 1; // Defailt volume ID
  double *coords = NULL;

  /* we get a binary UGRID file from Pointwise */
  status = fread(&numNode,          sizeof(int), 1, fp); if (status != 1) { status = CAPS_IOERR; goto cleanup; }
  status = fread(&numTriangle,      sizeof(int), 1, fp); if (status != 1) { status = CAPS_IOERR; goto cleanup; }
  status = fread(&numQuadrilateral, sizeof(int), 1, fp); if (status != 1) { status = CAPS_IOERR; goto cleanup; }
  status = fread(&numTetrahedral,   sizeof(int), 1, fp); if (status != 1) { status = CAPS_IOERR; goto cleanup; }
  status = fread(&numPyramid,       sizeof(int), 1, fp); if (status != 1) { status = CAPS_IOERR; goto cleanup; }
  status = fread(&numPrism,         sizeof(int), 1, fp); if (status != 1) { status = CAPS_IOERR; goto cleanup; }
  status = fread(&numHexahedral,    sizeof(int), 1, fp); if (status != 1) { status = CAPS_IOERR; goto cleanup; }

  /*
  printf("\n Header from UGRID file: %d  %d %d  %d %d %d %d\n", numNode,
         numTriangle, numQuadrilateral, numTetrahedral, numPyramid, numPrism,
         numHexahedral);
   */

  coords = (double *) EG_alloc(3*numNode*sizeof(double));
  if (coords == NULL) return EGADS_MALLOC;

  /* read all of the vertices */
  status = fread(coords, sizeof(double), 3*numNode, fp);
  if (status != 3*numNode) { status = CAPS_IOERR; goto cleanup; }

  // TODO: Should this be something else?
  volumeMesh->analysisType = UnknownMeshAnalysis;

  // Set that this is a volume mesh
  volumeMesh->meshType = VolumeMesh;

  // Numbers
  volumeMesh->numNode = numNode;
  volumeMesh->numElement = numTriangle      +
                           numQuadrilateral +
                           numTetrahedral   +
                           numPyramid       +
                           numPrism         +
                           numHexahedral;

  volumeMesh->meshQuickRef.useStartIndex = (int) true;

  volumeMesh->meshQuickRef.numTriangle      = numTriangle;
  volumeMesh->meshQuickRef.numQuadrilateral = numQuadrilateral;

  volumeMesh->meshQuickRef.numTetrahedral = numTetrahedral;
  volumeMesh->meshQuickRef.numPyramid     = numPyramid;
  volumeMesh->meshQuickRef.numPrism       = numPrism;
  volumeMesh->meshQuickRef.numHexahedral  = numHexahedral;

  printf("Volume mesh:\n");
  printf("\tNumber of nodes          = %d\n", numNode);
  printf("\tNumber of elements       = %d\n", volumeMesh->numElement);
  printf("\tNumber of triangles      = %d\n", numTriangle);
  printf("\tNumber of quadrilatarals = %d\n", numQuadrilateral);
  printf("\tNumber of tetrahedrals   = %d\n", numTetrahedral);
  printf("\tNumber of pyramids       = %d\n", numPyramid);
  printf("\tNumber of prisms         = %d\n", numPrism);
  printf("\tNumber of hexahedrals    = %d\n", numHexahedral);

  // Nodes - allocate
  volumeMesh->node = (meshNodeStruct *) EG_alloc(volumeMesh->numNode*sizeof(meshNodeStruct));
  if (volumeMesh->node == NULL) return EGADS_MALLOC;

  // Initialize
  for (i = 0; i < volumeMesh->numNode; i++) {
      status = initiate_meshNodeStruct(&volumeMesh->node[i], volumeMesh->analysisType);
      if (status != CAPS_SUCCESS) return status;
  }

  // Nodes - set
  for (i = 0; i < volumeMesh->numNode; i++) {

    // Copy node data
    volumeMesh->node[i].nodeID = i+1;

    volumeMesh->node[i].xyz[0] = coords[3*i+0];
    volumeMesh->node[i].xyz[1] = coords[3*i+1];
    volumeMesh->node[i].xyz[2] = coords[3*i+2];
  }
  EG_free(coords); coords = NULL;


  // Elements - allocate
  volumeMesh->element = (meshElementStruct *) EG_alloc(volumeMesh->numElement*sizeof(meshElementStruct));
  if (volumeMesh->element == NULL) {
      EG_free(volumeMesh->node); volumeMesh->node = NULL;
      return EGADS_MALLOC;
  }

  // Initialize
  for (i = 0; i < volumeMesh->numElement; i++ ) {
      status = initiate_meshElementStruct(&volumeMesh->element[i], volumeMesh->analysisType);
      if (status != CAPS_SUCCESS) return status;
  }

  // Start of element index
  elementIndex = 0;

  // Elements -Set triangles
  if (numTriangle > 0) volumeMesh->meshQuickRef.startIndexTriangle = elementIndex;

  numPoint = mesh_numMeshConnectivity(Triangle);
  for (i = 0; i < numTriangle; i++) {

      volumeMesh->element[elementIndex].elementType = Triangle;
      volumeMesh->element[elementIndex].elementID   = elementIndex+1;

      status = mesh_allocMeshElementConnectivity(&volumeMesh->element[elementIndex]);
      if (status != CAPS_SUCCESS) return status;

      // read the element connectivity
      status = fread(volumeMesh->element[elementIndex].connectivity, sizeof(int), numPoint, fp);
      if (status != numPoint) { status = CAPS_IOERR; goto cleanup; }

      elementIndex += 1;
  }

  // Elements -Set quadrilateral
  if (numQuadrilateral > 0) volumeMesh->meshQuickRef.startIndexQuadrilateral = elementIndex;

  numPoint = mesh_numMeshConnectivity(Quadrilateral);
  for (i = 0; i < numQuadrilateral; i++) {

      volumeMesh->element[elementIndex].elementType = Quadrilateral;
      volumeMesh->element[elementIndex].elementID   = elementIndex+1;

      status = mesh_allocMeshElementConnectivity(&volumeMesh->element[elementIndex]);
      if (status != CAPS_SUCCESS) return status;

      // read the element connectivity
      status = fread(volumeMesh->element[elementIndex].connectivity, sizeof(int), numPoint, fp);
      if (status != numPoint) { status = CAPS_IOERR; goto cleanup; }

      elementIndex += 1;
  }

  // skip face ID section of the file
  // they do not map to the elements on faces
  status = fseek(fp, (numTriangle + numQuadrilateral)*sizeof(int), SEEK_CUR);
  if (status != 0) { status = CAPS_IOERR; goto cleanup; }

  // Elements -Set Tetrahedral
  if (numTetrahedral > 0) volumeMesh->meshQuickRef.startIndexTetrahedral = elementIndex;

  numPoint = mesh_numMeshConnectivity(Tetrahedral);
  for (i = 0; i < numTetrahedral; i++) {

      volumeMesh->element[elementIndex].elementType = Tetrahedral;
      volumeMesh->element[elementIndex].elementID   = elementIndex+1;
      volumeMesh->element[elementIndex].markerID    = defaultVolID;

      status = mesh_allocMeshElementConnectivity(&volumeMesh->element[elementIndex]);
      if (status != CAPS_SUCCESS) return status;

      // read the element connectivity
      status = fread(volumeMesh->element[elementIndex].connectivity, sizeof(int), numPoint, fp);
      if (status != numPoint) { status = CAPS_IOERR; goto cleanup; }

      elementIndex += 1;
  }

  // Elements -Set Pyramid
  if (numPyramid > 0) volumeMesh->meshQuickRef.startIndexPyramid = elementIndex;

  numPoint = mesh_numMeshConnectivity(Pyramid);
  for (i = 0; i < numPyramid; i++) {

      volumeMesh->element[elementIndex].elementType = Pyramid;
      volumeMesh->element[elementIndex].elementID   = elementIndex+1;
      volumeMesh->element[elementIndex].markerID    = defaultVolID;

      status = mesh_allocMeshElementConnectivity(&volumeMesh->element[elementIndex]);
      if (status != CAPS_SUCCESS) return status;

      // read the element connectivity
      status = fread(volumeMesh->element[elementIndex].connectivity, sizeof(int), numPoint, fp);
      if (status != numPoint) { status = CAPS_IOERR; goto cleanup; }

      elementIndex += 1;
  }

  // Elements -Set Prism
  if (numPrism > 0) volumeMesh->meshQuickRef.startIndexPrism = elementIndex;

  numPoint = mesh_numMeshConnectivity(Prism);
  for (i = 0; i < numPrism; i++) {

      volumeMesh->element[elementIndex].elementType = Prism;
      volumeMesh->element[elementIndex].elementID   = elementIndex+1;
      volumeMesh->element[elementIndex].markerID    = defaultVolID;

      status = mesh_allocMeshElementConnectivity(&volumeMesh->element[elementIndex]);
      if (status != CAPS_SUCCESS) return status;

      // read the element connectivity
      status = fread(volumeMesh->element[elementIndex].connectivity, sizeof(int), numPoint, fp);
      if (status != numPoint) { status = CAPS_IOERR; goto cleanup; }

      elementIndex += 1;
  }

  // Elements -Set Hexa
  if (numHexahedral > 0) volumeMesh->meshQuickRef.startIndexHexahedral = elementIndex;

  numPoint = mesh_numMeshConnectivity(Hexahedral);
  for (i = 0; i < numHexahedral; i++) {

      volumeMesh->element[elementIndex].elementType = Hexahedral;
      volumeMesh->element[elementIndex].elementID   = elementIndex+1;
      volumeMesh->element[elementIndex].markerID    = defaultVolID;

      status = mesh_allocMeshElementConnectivity(&volumeMesh->element[elementIndex]);
      if (status != CAPS_SUCCESS) return status;

      // read the element connectivity
      status = fread(volumeMesh->element[elementIndex].connectivity, sizeof(int), numPoint, fp);
      if (status != numPoint) { status = CAPS_IOERR; goto cleanup; }

      elementIndex += 1;
  }

  status = CAPS_SUCCESS;

  cleanup:
      if (status != CAPS_SUCCESS) printf("Premature exit in getUGRID status = %d\n", status);

      EG_free(coords); coords = NULL;

      return status;
}

static int writeMesh(int iIndex, void* aimInfo) {

  int status = CAPS_SUCCESS;

  int surfIndex, volIndex;

  // analysis input values
  capsValue *Proj_Name = NULL;
  capsValue *Mesh_Format = NULL;
  capsValue *Mesh_ASCII_Flag = NULL;

  char *filename = NULL, surfNumber[11];
  const char *outputFileName = NULL;
  const char *outputFormat = NULL;
  int outputASCIIFlag;  // 0 = Binary output, anything else for ASCII

  bndCondStruct bndConds; // Structure of boundary conditions

  initiate_bndCondStruct(&bndConds);

  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN), ANALYSISIN, &Proj_Name);
  if (status != CAPS_SUCCESS) return status;

  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN), ANALYSISIN, &Mesh_Format);
  if (status != CAPS_SUCCESS) return status;

  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Mesh_ASCII_Flag", ANALYSISIN), ANALYSISIN, &Mesh_ASCII_Flag);
  if (status != CAPS_SUCCESS) return status;

  // Project Name
  if (Proj_Name->nullVal == IsNull) {
    printf("No project name (\"Proj_Name\") provided - A volume mesh will not be written out\n");
    return CAPS_SUCCESS;
  }

  outputFileName = Proj_Name->vals.string;
  outputFormat = Mesh_Format->vals.string;
  outputASCIIFlag = Mesh_ASCII_Flag->vals.integer;

  if (strcasecmp(outputFormat, "SU2") != 0) {
    for (surfIndex = 0; surfIndex < pointwiseInstance[iIndex].numSurfaceMesh; surfIndex++) {

      sprintf(surfNumber, "%d", surfIndex+1);
      filename = (char *) EG_alloc((strlen(outputFileName) +strlen("_Surf_") + strlen(surfNumber)+1)*sizeof(char));
      if (filename == NULL) { status = EGADS_MALLOC; goto cleanup; }

      sprintf(filename, "%s_Surf_%s", outputFileName, surfNumber);

      if (strcasecmp(outputFormat, "AFLR3") == 0) {

        status = mesh_writeAFLR3(filename,
                                 outputASCIIFlag,
                                 &pointwiseInstance[iIndex].surfaceMesh[surfIndex],
                                 1.0);

      } else if (strcasecmp(outputFormat, "VTK") == 0) {

        status = mesh_writeVTK(filename,
                               outputASCIIFlag,
                               &pointwiseInstance[iIndex].surfaceMesh[surfIndex],
                               1.0);

      } else if (strcasecmp(outputFormat, "Tecplot") == 0) {

        status = mesh_writeTecplot(filename,
                                   outputASCIIFlag,
                                   &pointwiseInstance[iIndex].surfaceMesh[surfIndex],
                                   1.0);
      } else {
        printf("Unrecognized mesh format, \"%s\", the surface mesh will not be written out\n", outputFormat);
      }

      EG_free(filename); filename = NULL;

      if (status != CAPS_SUCCESS) goto cleanup;
    }
  }

  for (volIndex = 0; volIndex < pointwiseInstance[iIndex].numVolumeMesh; volIndex++) {

//      if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {
//          sprintf(bodyIndexNumber, "%d", bodyIndex);
//          filename = (char *) EG_alloc((strlen(outputFileName) +
//                                        strlen(analysisPath)+2+strlen("_Vol")+strlen(bodyIndexNumber))*sizeof(char));
//      } else {
          filename = EG_strdup(outputFileName);

//      }

      if (filename == NULL) {
          status = EGADS_MALLOC;
          goto cleanup;
      }

      //if (aimInputs[aim_getIndex(aimInfo, "Multiple_Mesh", ANALYSISIN)-1].vals.integer == (int) true) {
      //    strcat(filename,"_Vol");
      //    strcat(filename,bodyIndexNumber);
      //}

      if (strcasecmp(outputFormat, "AFLR3") == 0) {

          status = mesh_writeAFLR3(filename,
                                   outputASCIIFlag,
                                   &pointwiseInstance[iIndex].volumeMesh[volIndex],
                                   1.0);

      } else if (strcasecmp(outputFormat, "VTK") == 0) {

          status = mesh_writeVTK(filename,
                                 outputASCIIFlag,
                                 &pointwiseInstance[iIndex].volumeMesh[volIndex],
                                 1.0);

      } else if (strcasecmp(outputFormat, "SU2") == 0) {

          // construct boundary condition information for SU2
          status = populate_bndCondStruct_from_mapAttrToIndexStruct(&pointwiseInstance[iIndex].attrMap, &bndConds);
          if (status != CAPS_SUCCESS) goto cleanup;

          status = mesh_writeSU2(filename,
                                 outputASCIIFlag,
                                 &pointwiseInstance[iIndex].volumeMesh[volIndex],
                                 bndConds.numBND,
                                 bndConds.bndID,
                                 1.0);

      } else if (strcasecmp(outputFormat, "Tecplot") == 0) {

          status = mesh_writeTecplot(filename,
                                     outputASCIIFlag,
                                     &pointwiseInstance[iIndex].volumeMesh[volIndex],
                                     1.0);

      } else if (strcasecmp(outputFormat, "Nastran") == 0) {

          status = mesh_writeNASTRAN(filename,
                                     outputASCIIFlag,
                                     &pointwiseInstance[iIndex].volumeMesh[volIndex],
                                     LargeField,
                                     1.0);
      } else {
          printf("Unrecognized mesh format, \"%s\", the volume mesh will not be written out\n", outputFormat);
      }

      EG_free(filename); filename = NULL;
      if (status != CAPS_SUCCESS) goto cleanup;
  }

  cleanup:
      if (status != CAPS_SUCCESS) printf("Premature exit in writeMesh status = %d\n", status);

      destroy_bndCondStruct(&bndConds);

      EG_free(filename);
      return status;
}


static int writeGlobalGlyph(void *aimInfo, capsValue *aimInputs) {

    int status;

    FILE *fp = NULL;
    char filename[] = "capsUserDefaults.glf";

    // connector controls
    int         conInitDim           = aimInputs[aim_getIndex(aimInfo, "Connector_Initial_Dim"     , ANALYSISIN)-1].vals.integer;
    int         conMaxDim            = aimInputs[aim_getIndex(aimInfo, "Connector_Max_Dim"         , ANALYSISIN)-1].vals.integer;
    int         conMinDim            = aimInputs[aim_getIndex(aimInfo, "Connector_Min_Dim"         , ANALYSISIN)-1].vals.integer;
    double      conTurnAngle         = aimInputs[aim_getIndex(aimInfo, "Connector_Turn_Angle"      , ANALYSISIN)-1].vals.real;
    double      conDeviation         = aimInputs[aim_getIndex(aimInfo, "Connector_Deviation"       , ANALYSISIN)-1].vals.real;
    double      conSplitAngle        = aimInputs[aim_getIndex(aimInfo, "Connector_Split_Angle"     , ANALYSISIN)-1].vals.real;
    double      conProxGrowthRate    = aimInputs[aim_getIndex(aimInfo, "Connector_Prox_Growth_Rate", ANALYSISIN)-1].vals.real;
    int         conAdaptSources      = aimInputs[aim_getIndex(aimInfo, "Connector_Adapt_Sources"   , ANALYSISIN)-1].vals.integer;
    int         conSourceSpacing     = aimInputs[aim_getIndex(aimInfo, "Connector_Source_Spacing"  , ANALYSISIN)-1].vals.integer;
    double      conTurnAngleHard     = aimInputs[aim_getIndex(aimInfo, "Connector_Turn_Angle_Hard" , ANALYSISIN)-1].vals.real;

    // domain controls
    const char *domAlgorithm         = aimInputs[aim_getIndex(aimInfo, "Domain_Algorithm"          , ANALYSISIN)-1].vals.string;
    int         domFullLayers        = aimInputs[aim_getIndex(aimInfo, "Domain_Full_Layers"        , ANALYSISIN)-1].vals.integer;
    int         domMaxLayers         = aimInputs[aim_getIndex(aimInfo, "Domain_Max_Layers"         , ANALYSISIN)-1].vals.integer;
    double      domGrowthRate        = aimInputs[aim_getIndex(aimInfo, "Domain_Growth_Rate"        , ANALYSISIN)-1].vals.real;
    const char *domIsoType           = aimInputs[aim_getIndex(aimInfo, "Domain_Iso_Type"           , ANALYSISIN)-1].vals.string;
    const char *domTRexType          = aimInputs[aim_getIndex(aimInfo, "Domain_TRex_Type"          , ANALYSISIN)-1].vals.string;
    double      domTRexARLimit       = aimInputs[aim_getIndex(aimInfo, "Domain_TRex_ARLimit"       , ANALYSISIN)-1].vals.real;
    double      domDecay             = aimInputs[aim_getIndex(aimInfo, "Domain_Decay"              , ANALYSISIN)-1].vals.real;
    double      domMinEdge           = aimInputs[aim_getIndex(aimInfo, "Domain_Min_Edge"           , ANALYSISIN)-1].vals.real;
    double      domMaxEdge           = aimInputs[aim_getIndex(aimInfo, "Domain_Max_Edge"           , ANALYSISIN)-1].vals.real;
    int         domAdapt             = aimInputs[aim_getIndex(aimInfo, "Domain_Adapt"              , ANALYSISIN)-1].vals.integer;

    // block controls
    const char *blkAlgorithm         = aimInputs[aim_getIndex(aimInfo, "Block_Algorithm"           , ANALYSISIN)-1].vals.string;
    int         blkVoxelLayers       = aimInputs[aim_getIndex(aimInfo, "Block_Voxel_Layers"        , ANALYSISIN)-1].vals.integer;
    double      blkboundaryDecay     = aimInputs[aim_getIndex(aimInfo, "Block_Boundary_Decay"      , ANALYSISIN)-1].vals.real;
    double      blkcollisionBuffer   = aimInputs[aim_getIndex(aimInfo, "Block_Collision_Buffer"    , ANALYSISIN)-1].vals.real;
    double      blkmaxSkewAngle      = aimInputs[aim_getIndex(aimInfo, "Block_Max_Skew_Angle"      , ANALYSISIN)-1].vals.real;
    double      blkedgeMaxGrowthRate = aimInputs[aim_getIndex(aimInfo, "Block_Edge_Max_Growth_Rate", ANALYSISIN)-1].vals.real;
    int         blkfullLayers        = aimInputs[aim_getIndex(aimInfo, "Block_Full_Layers"         , ANALYSISIN)-1].vals.integer;
    int         blkmaxLayers         = aimInputs[aim_getIndex(aimInfo, "Block_Max_Layers"          , ANALYSISIN)-1].vals.integer;
    double      blkgrowthRate        = aimInputs[aim_getIndex(aimInfo, "Block_Growth_Rate"         , ANALYSISIN)-1].vals.real;
    const char *blkTRexType          = aimInputs[aim_getIndex(aimInfo, "Block_TRexType"            , ANALYSISIN)-1].vals.string;

    // general controls
    double genSourceBoxLengthScale   = aimInputs[aim_getIndex(aimInfo, "Gen_Source_Box_Length_Scale", ANALYSISIN)-1].vals.real;
    double *genSourceBoxDirection    = aimInputs[aim_getIndex(aimInfo, "Gen_Source_Box_Direction"   , ANALYSISIN)-1].vals.reals;
    double genSourceBoxAngle         = aimInputs[aim_getIndex(aimInfo, "Gen_Source_Box_Angle"       , ANALYSISIN)-1].vals.real;
    double genSourceGrowthFactor     = aimInputs[aim_getIndex(aimInfo, "Gen_Source_Growth_Factor"   , ANALYSISIN)-1].vals.real;

/*  These parameters are for high-order mesh generation. This should be hooked up in the future.
    // Elevate On Export controls
    double      eoecostThreshold     = aimInputs[aim_getIndex(aimInfo, "Elevate _Cost_Threshold"   , ANALYSISIN)-1].vals.real;
    double      eoemaxIncAngle       = aimInputs[aim_getIndex(aimInfo, "Elevate _Max_Include_Angle", ANALYSISIN)-1].vals.real;
    double      eoerelax             = aimInputs[aim_getIndex(aimInfo, "Elevate _Relax"            , ANALYSISIN)-1].vals.real;
    int         eoesmoothingPasses   = aimInputs[aim_getIndex(aimInfo, "Elevate _Smoothing_Passes" , ANALYSISIN)-1].vals.integer;
    double      eoeWCNWeight         = aimInputs[aim_getIndex(aimInfo, "Elevate _WCN_Weight"       , ANALYSISIN)-1].vals.real;
    const char *eoeWCNMode           = aimInputs[aim_getIndex(aimInfo, "Elevate _WCN_Mode"         , ANALYSISIN)-1].vals.string;
*/
    // Assumed we are currently already in the correct directory
    fp = fopen(filename, "w");
    if (fp == NULL) {
        status = CAPS_IOERR;
        goto cleanup;
    }

    fprintf(fp, "# Connector level\n");
    fprintf(fp, "set conParams(InitDim)                          %d; # Initial connector dimension\n", conInitDim);
    fprintf(fp, "set conParams(MaxDim)                           %d; # Maximum connector dimension\n", conMaxDim);
    fprintf(fp, "set conParams(MinDim)                           %d; # Minimum connector dimension\n", conMinDim);
    fprintf(fp, "set conParams(TurnAngle)                       %lf; # Maximum turning angle on connectors for dimensioning (0 - not used)\n", conTurnAngle);
    fprintf(fp, "set conParams(Deviation)                       %lf; # Maximum deviation on connectors for dimensioning (0 - not used)\n", conDeviation);
    fprintf(fp, "set conParams(SplitAngle)                      %lf; # Turning angle on connectors to split (0 - not used)\n", conSplitAngle);
    fprintf(fp, "set conParams(JoinCons)                          0; # Perform joining operation on 2 connectors at one endpoint\n"); // This must be 0 in order to put the surface mesh back on an egads tessellation
    fprintf(fp, "set conParams(ProxGrowthRate)                  %lf; # Connector proximity growth rate\n", conProxGrowthRate);
    fprintf(fp, "set conParams(AdaptSources)                     %d; # Compute sources using connectors (0 - not used) V18.2+ (experimental)\n", conAdaptSources);
    fprintf(fp, "set conParams(SourceSpacing)                    %d; # Use source cloud for adaptive pass on connectors V18.2+\n", conSourceSpacing);
    fprintf(fp, "set conParams(TurnAngleHard)                   %lf; # Hard edge turning angle limit for domain T-Rex (0.0 - not used)\n", conTurnAngleHard);
    fprintf(fp, "\n");
    fprintf(fp, "# Domain level\n");
    fprintf(fp, "set domParams(Algorithm)                    \"%s\"; # Isotropic (Delaunay, AdvancingFront or AdvancingFrontOrtho)\n", domAlgorithm);
    fprintf(fp, "set domParams(FullLayers)                       %d; # Domain full layers (0 for multi-normals, >= 1 for single normal)\n", domFullLayers);
    fprintf(fp, "set domParams(MaxLayers)                        %d; # Domain maximum layers\n", domMaxLayers);
    fprintf(fp, "set domParams(GrowthRate)                      %lf; # Domain growth rate for 2D T-Rex extrusion\n", domGrowthRate);
    fprintf(fp, "set domParams(IsoType)                      \"%s\"; # Domain iso cell type (Triangle or TriangleQuad)\n", domIsoType);
    fprintf(fp, "set domParams(TRexType)                     \"%s\"; # Domain T-Rex cell type (Triangle or TriangleQuad)\n", domTRexType);
    fprintf(fp, "set domParams(TRexARLimit)                     %lf; # Domain T-Rex maximum aspect ratio limit (0 - not used)\n", domTRexARLimit);
    fprintf(fp, "set domParams(Decay)                           %lf; # Domain boundary decay\n", domDecay);
    fprintf(fp, "set domParams(MinEdge)                         %lf; # Domain minimum edge length\n", domMinEdge);
    fprintf(fp, "set domParams(MaxEdge)                         %lf; # Domain maximum edge length\n", domMaxEdge);
    fprintf(fp, "set domParams(Adapt)                            %d; # Set up all domains for adaptation (0 - not used) V18.2+ (experimental)\n", domAdapt);
    fprintf(fp, "\n");
    fprintf(fp, "# Block level\n");
    fprintf(fp, "set blkParams(Algorithm)                    \"%s\"; # Isotropic (Delaunay, Voxel) (V18.3+)\n", blkAlgorithm);
    fprintf(fp, "set blkParams(VoxelLayers)                      %d; # Number of Voxel transition layers if Algorithm set to Voxel (V18.3+)\n", blkVoxelLayers);
    fprintf(fp, "set blkParams(boundaryDecay)                   %lf; # Volumetric boundary decay\n", blkboundaryDecay);
    fprintf(fp, "set blkParams(collisionBuffer)                 %lf; # Collision buffer for colliding T-Rex fronts\n", blkcollisionBuffer);
    fprintf(fp, "set blkParams(maxSkewAngle)                    %lf; # Maximum skew angle for T-Rex extrusion\n", blkmaxSkewAngle);
    fprintf(fp, "set blkParams(edgeMaxGrowthRate)               %lf; # Volumetric edge ratio\n", blkedgeMaxGrowthRate);
    fprintf(fp, "set blkParams(fullLayers)                       %d; # Full layers (0 for multi-normals, >= 1 for single normal)\n", blkfullLayers);
    fprintf(fp, "set blkParams(maxLayers)                        %d; # Maximum layers\n", blkmaxLayers);
    fprintf(fp, "set blkParams(growthRate)                      %lf; # Growth rate for volume T-Rex extrusion\n", blkgrowthRate);
    fprintf(fp, "set blkParams(TRexType)                     \"%s\"; # T-Rex cell type (TetPyramid, TetPyramidPrismHex, AllAndConvertWallDoms)\n", blkTRexType);
    fprintf(fp, "set blkParams(volInitialize)                     1; # Initialize block after setup\n");
    fprintf(fp, "\n");
    fprintf(fp, "# General\n");
    fprintf(fp, "set genParams(SkipMeshing)                       1; # Skip meshing of domains during interim processing (V18.3+)\n");
    fprintf(fp, "set genParams(CAESolver)                 \"UGRID\"; # Selected CAE Solver (Currently support CGNS, Gmsh and UGRID)\n");
    fprintf(fp, "set genParams(outerBoxScale)                     0; # Enclose geometry in box with specified scale (0 - no box)\n");
    fprintf(fp, "set genParams(sourceBoxLengthScale)            %lf; # Length scale of enclosed viscous walls in source box (0 - no box)\n", genSourceBoxLengthScale);
    fprintf(fp, "set genParams(sourceBoxDirection)  { %lf %lf %lf }; # Principal direction vector (i.e. normalized freestream vector)\n", genSourceBoxDirection[0], genSourceBoxDirection[1], genSourceBoxDirection[2]);
    fprintf(fp, "set genParams(sourceBoxAngle)                  %lf; # Angle for widening source box in the assigned direction\n", genSourceBoxAngle);
    fprintf(fp, "set genParams(sourceGrowthFactor)              %lf; # Growth rate for spacing value along box\n", genSourceGrowthFactor);
    fprintf(fp, "set genParams(ModelSize)                         0; # Set model size before CAD import (0 - get from file)\n");
    fprintf(fp, "set genParams(writeGMA)                   \"true\"; # Write out geometry-mesh associativity file (true or false)\n");
    fprintf(fp, "set genParams(assembleTolMult)                 1.0; # Multiplier on model assembly tolerance for allowed MinEdge\n");
    fprintf(fp, "\n");
/*  These parameters are for high-order mesh generation. This should be hooked up in the future.
    fprintf(fp, "# Elevate On Export V18.2+\n");
    fprintf(fp, "set eoeParams(degree)                           Q1; # Polynomial degree (Q1, Q2, Q3 or Q4) NOTE: ONLY APPLIES TO CGNS AND GMSH\n");
    fprintf(fp, "set eoeParams(costThreshold)                   %lf; # Cost convergence threshold\n", eoecostThreshold);
    fprintf(fp, "set eoeParams(maxIncAngle)                     %lf; # Maximum included angle tolerance\n", eoemaxIncAngle);
    fprintf(fp, "set eoeParams(relax)                           %lf; # Iteration relaxation factor\n", eoerelax);
    fprintf(fp, "set eoeParams(smoothingPasses)                  %d; # Number of smoothing passes\n", eoesmoothingPasses);
    fprintf(fp, "set eoeParams(WCNWeight)                       %lf; # WCN cost component weighting factor\n", eoeWCNWeight);
    fprintf(fp, "set eoeParams(WCNMode)                      \"%s\"; # WCN weight factor method (UseValue or Calculate)\n", eoeWCNMode);
    fprintf(fp, "set eoeParams(writeVTU)                  \"false\"; # Write out ParaView VTU files (true or false)\n");
*/

    status = CAPS_SUCCESS;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in writeGlobalGlyph, status %d\n", status);

        if (fp != NULL) fclose(fp);

        return status;
}

#if 0
static int set_autoQuiltAttr(ego *body) {

    int status; // Function return status

    int i, j; // Indexing

    int numBodyFace, numChildrenLoop;
    ego *bodyFace = NULL, *childrenLoop=NULL;

    ego geomRef, geomRef2;
    int oclass, mtypeFace, *senses;
    double data[18];

    char pwQuiltAttr[] = "PW:QuiltName";

    const char *face1Attr = NULL, *face2Attr=NULL;

    char quiltNamePrefix[] = "capsAutoQuilt_";
    char quiltName[30];
    int quiltIndex = 1;

    status = EG_getBodyTopos(*body, NULL, FACE, &numBodyFace, &bodyFace);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Loop through the faces and find what edges
    for (i = 0; i < numBodyFace; i++) {

        status = EG_getTopology(bodyFace[i], &geomRef, &oclass, &mtypeFace, data,
                                &numChildrenLoop, &childrenLoop, &senses);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = retrieve_stringAttr(bodyFace[i], pwQuiltAttr, &face1Attr);
        if (status == EGADS_NOTFOUND) {
            face1Attr = NULL;
        } else if (status != CAPS_SUCCESS) {
            goto cleanup;
        }

        for (j = 0; j < numBodyFace; j++) {

            // If faces match
            if (i == j) continue;

            status = EG_getTopology(bodyFace[j], &geomRef2, &oclass, &mtypeFace, data,
                                    &numChildrenLoop, &childrenLoop, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;

            status = EG_isSame(geomRef, geomRef2);
            if (status < CAPS_SUCCESS ) goto cleanup;

            if (status == EGADS_OUTSIDE) continue;

            // I have the same piece of geometry - should be continuous - let's quilt
            status = retrieve_stringAttr(bodyFace[j], pwQuiltAttr, &face2Attr);
            if (status == EGADS_NOTFOUND) {
                face2Attr = NULL;
            } else if (status != CAPS_SUCCESS) {
                goto cleanup;
            }

            // If neither face has an attribute
            if (face1Attr == NULL && face2Attr == NULL) {

                sprintf(quiltName, "%s%d", quiltNamePrefix, quiltIndex);

                // Face 1
                status = EG_attributeAdd(bodyFace[i], pwQuiltAttr, ATTRSTRING, 0, NULL, NULL, quiltName);
                if (status != EGADS_SUCCESS) goto cleanup;

                // Face 2
                status = EG_attributeAdd(bodyFace[j], pwQuiltAttr, ATTRSTRING, 0, NULL, NULL, quiltName);
                if (status != EGADS_SUCCESS) goto cleanup;

                quiltIndex += 1;

            } else if (face1Attr == NULL) {

                status = EG_attributeAdd(bodyFace[i], pwQuiltAttr, ATTRSTRING, 0, NULL, NULL, face2Attr);
                if (status != EGADS_SUCCESS) goto cleanup;

            } else if (face2Attr == NULL) {

                status = EG_attributeAdd(bodyFace[i], pwQuiltAttr, ATTRSTRING, 0, NULL, NULL, face1Attr);
                if (status != EGADS_SUCCESS) goto cleanup;
            } else {
                // Both Face 1 and Face 2 already have attributes
                continue;
            }
        } // Second face loop
    } // Face loop

    status = CAPS_SUCCESS;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in set_autoQuiltAttr, status %d\n", status);

        EG_free(bodyFace);

        return status;
}
#endif


static int setPWAttr(ego body,
                     mapAttrToIndexStruct *attrMap,
                     int numMeshProp,
                     meshSizingStruct *meshProp,
                     double capsMeshLength,
                     int *quilting) {

    int status; // Function return status

    int nodeIndex, edgeIndex, faceIndex; // Indexing

    int numNode;
    ego *nodes = NULL;

    int numEdge;
    ego *edges = NULL;

    int numFace;
    ego *faces = NULL;

    const char *groupName = NULL;
    int attrIndex = 0;
    int propIndex = 0;
    double real = 0;

    int atype, alen; // EGADS return variables
    const int    *ints;
    const double *reals;
    const char   *string;

    status = EG_getBodyTopos(body, NULL, NODE, &numNode, &nodes);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Loop through the nodes and set PW:NodeSpacing attribute
    for (nodeIndex = 0; nodeIndex < numNode; nodeIndex++) {

        status = retrieve_CAPSGroupAttr(nodes[nodeIndex], &groupName);
        if (status != EGADS_SUCCESS) continue;

        status = get_mapAttrToIndexIndex(attrMap, groupName, &attrIndex);
        if (status != CAPS_SUCCESS) {
          printf("Error: Unable to retrieve index from capsGroup %s\n", groupName);
          goto cleanup;
        }

        for (propIndex = 0; propIndex < numMeshProp; propIndex++) {

            // Check if the mesh property applies to the capsGroup of this edge
            if (meshProp[propIndex].attrIndex != attrIndex) continue;

            // Is the attribute set?
            if (meshProp[propIndex].nodeSpacing > 0) {

                real = capsMeshLength*meshProp[propIndex].nodeSpacing;

                // add the attribute
                status = EG_attributeAdd(nodes[nodeIndex], "PW:NodeSpacing", ATTRREAL, 1, NULL, &real, NULL);
                if (status != EGADS_SUCCESS) goto cleanup;
            }
        }
    }

    status = EG_getBodyTopos(body, NULL, EDGE, &numEdge, &edges);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Loop through the edges and set PW:Connector* attributes
    for (edgeIndex = 0; edgeIndex < numEdge; edgeIndex++) {

        status = retrieve_CAPSGroupAttr(edges[edgeIndex], &groupName);
        if (status != EGADS_SUCCESS) continue;

        status = get_mapAttrToIndexIndex(attrMap, groupName, &attrIndex);
        if (status != CAPS_SUCCESS) {
          printf("Error: Unable to retrieve index from capsGroup %s\n", groupName);
          goto cleanup;
        }

        for (propIndex = 0; propIndex < numMeshProp; propIndex++) {

            // Check if the mesh property applies to the capsGroup of this edge
            if (meshProp[propIndex].attrIndex != attrIndex) continue;

            // Is the attribute set?
            if (meshProp[propIndex].maxSpacing > 0) {

                real = capsMeshLength*meshProp[propIndex].maxSpacing;

                // add the attribute
                status = EG_attributeAdd(edges[edgeIndex], "PW:ConnectorMaxEdge", ATTRREAL, 1, NULL, &real, NULL);
                if (status != EGADS_SUCCESS) goto cleanup;
            }

            // Is the attribute set?
            if (meshProp[propIndex].nodeSpacing > 0) {

                real = capsMeshLength*meshProp[propIndex].nodeSpacing;

                // add the attribute
                status = EG_attributeAdd(edges[edgeIndex], "PW:ConnectorEndSpacing", ATTRREAL, 1, NULL, &real, NULL);
                if (status != EGADS_SUCCESS) goto cleanup;
            }

            // Is the attribute set?
            if (meshProp[propIndex].numEdgePoints > 0) {

                // add the attribute
                status = EG_attributeAdd(edges[edgeIndex], "PW:ConnectorDimension", ATTRINT, 1, &meshProp[propIndex].numEdgePoints, NULL, NULL);
                if (status != EGADS_SUCCESS) goto cleanup;
            }

            // Is the attribute set?
            if (meshProp[propIndex].avgSpacing > 0) {

                real = capsMeshLength*meshProp[propIndex].avgSpacing;

                // add the attribute
                status = EG_attributeAdd(edges[edgeIndex], "PW:ConnectorAverageDS", ATTRREAL, 1, NULL, &real, NULL);
                if (status != EGADS_SUCCESS) goto cleanup;
            }

            // Is the attribute set?
            if (meshProp[propIndex].maxAngle > 0) {

                real = meshProp[propIndex].maxAngle;

                // add the attribute
                status = EG_attributeAdd(edges[edgeIndex], "PW:ConnectorMaxAngle", ATTRREAL, 1, NULL, &real, NULL);
                if (status != EGADS_SUCCESS) goto cleanup;
            }

            // Is the attribute set?
            if (meshProp[propIndex].maxDeviation > 0) {

                real = capsMeshLength*meshProp[propIndex].maxDeviation;

                // add the attribute
                status = EG_attributeAdd(edges[edgeIndex], "PW:ConnectorMaxDeviation", ATTRREAL, 1, NULL, &real, NULL);
                if (status != EGADS_SUCCESS) goto cleanup;
            }
        }
    }

    status = EG_getBodyTopos(body, NULL, FACE, &numFace, &faces);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Loop through the faces and copy capsGroup to PW:Name and set PW:Domain* attributes
    for (faceIndex = 0; faceIndex < numFace; faceIndex++) {

        status = retrieve_CAPSGroupAttr(faces[faceIndex], &groupName);
        if (status == EGADS_SUCCESS) {

            status = EG_attributeAdd(faces[faceIndex], "PW:Name", ATTRSTRING, 0, NULL, NULL, groupName);
            if (status != EGADS_SUCCESS) goto cleanup;

            status = get_mapAttrToIndexIndex(attrMap, groupName, &attrIndex);
            if (status != CAPS_SUCCESS) {
              printf("Error: Unable to retrieve index from capsGroup %s\n", groupName);
              goto cleanup;
            }

            for (propIndex = 0; propIndex < numMeshProp; propIndex++) {

                // Check if the mesh property applies to the capsGroup of this face
                if (meshProp[propIndex].attrIndex != attrIndex) continue;

                // Is the attribute set?
                if (meshProp[propIndex].minSpacing > 0) {

                    real = capsMeshLength*meshProp[propIndex].minSpacing;

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainMinEdge", ATTRREAL, 1, NULL, &real, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].maxSpacing > 0) {

                    real = capsMeshLength*meshProp[propIndex].maxSpacing;

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainMaxEdge", ATTRREAL, 1, NULL, &real, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].maxAngle > 0) {

                    real = meshProp[propIndex].maxAngle;

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainMaxAngle", ATTRREAL, 1, NULL, &real, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].maxDeviation > 0) {

                    real = capsMeshLength*meshProp[propIndex].maxDeviation;

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainMaxDeviation", ATTRREAL, 1, NULL, &real, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].boundaryDecay > 0) {

                    real = meshProp[propIndex].boundaryDecay;

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainDecay", ATTRREAL, 1, NULL, &real, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].boundaryLayerMaxLayers > 0) {

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainMaxLayers", ATTRINT, 1, &meshProp[propIndex].boundaryLayerMaxLayers, NULL, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].boundaryLayerFullLayers > 0) {

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainFullLayers", ATTRINT, 1, &meshProp[propIndex].boundaryLayerFullLayers, NULL, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].boundaryLayerGrowthRate > 0) {

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:DomainTRexGrowthRate", ATTRREAL, 1, NULL, &meshProp[propIndex].boundaryLayerGrowthRate, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }

                // Is the attribute set?
                if (meshProp[propIndex].boundaryLayerSpacing > 0) {

                  real = capsMeshLength*meshProp[propIndex].boundaryLayerSpacing;

                    // add the attribute
                    status = EG_attributeAdd(faces[faceIndex], "PW:WallSpacing", ATTRREAL, 1, NULL, &real, NULL);
                    if (status != EGADS_SUCCESS) goto cleanup;
                }
            }
        } else {
            printf("Error: No capsGroup attribute found on Face %d\n", faceIndex+1);
            printf("Available attributes are:\n");
            print_AllAttr( faces[faceIndex] );
            goto cleanup;
        }

        // Check for quilting on faces
        status = EG_attributeRet(faces[faceIndex], "PW:QuiltName", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS) *quilting = (int)true;

    } // Face loop

    status = CAPS_SUCCESS;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in setPWAttr, status %d\n", status);

        EG_free(nodes);
        EG_free(edges);
        EG_free(faces);

        return status;
}

// corrects uv values from pointwise gma files
static void correctUV(ego face, ego geom, ego newSurf, double *rvec, double *uv)
{
  int    status, iper;
  double limits[4], results[18], x, y, z, d, di, *x1, *x2, norm[3];

  status = EG_getRange(face, limits, &iper);
  if (status != EGADS_SUCCESS) {
    printf(" getRange = %d\n", status);
    return;
  }
  status = EG_evaluate(newSurf, uv, results);
  if (status != EGADS_SUCCESS) {
    printf(" Error: Surface eval status = %d!\n", status);
    return;
  }

  if ((geom->mtype == CYLINDRICAL) || (geom->mtype == CONICAL)) {
    x = (results[0]-rvec[0])*rvec[3] + (results[1]-rvec[1])*rvec[4] +
        (results[2]-rvec[2])*rvec[5];
    y = (results[0]-rvec[0])*rvec[6] + (results[1]-rvec[1])*rvec[7] +
        (results[2]-rvec[2])*rvec[8];
    d = atan2(y, x);
    while (d < limits[0]) d += 2*PI;
    while (d > limits[1]) d -= 2*PI;
    uv[0] = d;
  } else if (geom->mtype == SPHERICAL) {
    d  =  rvec[9];
    x1 = &rvec[3];
    x2 = &rvec[6];
    CROSS(norm, x1, x2);
    di = sqrt(norm[0]*norm[0] + norm[1]*norm[1] + norm[2]*norm[2]);
    if (d < 0.0) {
      di = -di;
      d  = -d;
    }
    if (di != 0.0) {
      norm[0] /= di;
      norm[1] /= di;
      norm[2] /= di;
    }
    x = (results[0]-rvec[0])*rvec[3] + (results[1]-rvec[1])*rvec[4] +
        (results[2]-rvec[2])*rvec[5];
    y = (results[0]-rvec[0])*rvec[6] + (results[1]-rvec[1])*rvec[7] +
        (results[2]-rvec[2])*rvec[8];
    z = (results[0]-rvec[0])*norm[0] + (results[1]-rvec[1])*norm[1] +
        (results[2]-rvec[2])*norm[2];
    uv[1] = asin(z/d);
    while (uv[1] < limits[2]) uv[1] += PI;
    while (uv[1] > limits[3]) uv[1] -= PI;
    uv[0] = atan2(y, x);
    while (uv[0] < limits[0]) uv[0] += 2*PI;
    while (uv[0] > limits[1]) uv[0] -= 2*PI;
  } else {
    x = (results[0]-rvec[0])*rvec[3] + (results[1]-rvec[1])*rvec[4] +
        (results[2]-rvec[2])*rvec[5];
    y = (results[0]-rvec[0])*rvec[6] + (results[1]-rvec[1])*rvec[7] +
        (results[2]-rvec[2])*rvec[8];
    z = (results[0]-rvec[0])*rvec[9] + (results[1]-rvec[1])*rvec[10] +
        (results[2]-rvec[2])*rvec[11];
    uv[1] = asin(z/rvec[13]);
    while (uv[1] < limits[2]) uv[1] += 2*PI;
    while (uv[1] > limits[3]) uv[1] -= 2*PI;
    uv[0] = atan2(y, x);
    while (uv[0] < limits[0]) uv[0] += 2*PI;
    while (uv[0] > limits[1]) uv[0] -= 2*PI;
  }
}


// count faces that must occur twice in the tessellation of a face
// these are on the bounds of a periodic uv-space
static int getFaceEdgeCount(ego body, ego face, int *nedge, ego **edges, int **edgeCount) {

  int status = CAPS_SUCCESS;

  int *senses, *count = NULL;
  int oclass, mtype, iedge, nnode;
  double trange[4], uv[2];
  ego *nodes, ref;

  *edgeCount = NULL;

  status = EG_getBodyTopos(body, face, EDGE, nedge, edges);
  if (status != EGADS_SUCCESS) {
      printf("EG_getBodyTopos EDGE = %d\n", status);
      goto cleanup;
  }

  count = (int*)EG_alloc((*nedge)*sizeof(int));
  if (count == NULL) { status = EGADS_MALLOC; goto cleanup; }

  for (iedge = 0; iedge < *nedge; iedge++) {
    count[iedge] = 1;

    status = EG_getTopology((*edges)[iedge], &ref, &oclass, &mtype, trange, &nnode, &nodes, &senses);
    if (status != EGADS_SUCCESS) goto cleanup;

    // try to get edge UV at t-mid with sense of 0
    status = EG_getEdgeUV(face, (*edges)[iedge], 0, (trange[0]+trange[1])/2, uv);
    if (status == EGADS_TOPOERR) {
      /* sense in Face twice! */
      count[iedge] = 2;
    }
  }

  *edgeCount = count;

  status = CAPS_SUCCESS;
  cleanup:
    if (status != CAPS_SUCCESS) printf("Error: Premature exit in getFaceEdgeCount status = %d\n", status);

    if (status != CAPS_SUCCESS) EG_free(count);

    return status;
}


// gets a face coordinates and UV values from FACE/EDGE/NODE points
static int getFacePoints(bodyData *bodydata, int ibody, int iface, meshStruct *volumeMesh,
                         int nSurfPts, gmaVertex *surfacedata,
                         int *face_pnt, int *face_ind, double *uv) {

  int status = CAPS_SUCCESS;
  int it, ib, in, iper, edgeIndex;
  int ifp, isp, ivp; // face point, surface point, volume point
  int oclass, mtype, npts, *senses, *tmp, offset;
  int nloop, iloop, nedge, iedge, nnode, i;
  double limits[4], uvbox[4], trange[4], t, *exyz; //, result[18];
  ego face, geom, *loops, *edges, *nodes, ref;

  face = bodydata->faces[iface];
  status = EG_getTopology(face, &ref, &oclass, &mtype, uvbox, &nloop, &loops, &senses);
  if (status != EGADS_SUCCESS) goto cleanup;

  geom = bodydata->surfaces[iface];
  if (geom->mtype == BSPLINE) {
    status = EG_getRange(geom, limits, &iper);
    if (status != EGADS_SUCCESS) goto cleanup;
  } else {
    for (i = 0; i < 4; i++)
      limits[i] = uvbox[i];
  }

  // populate UV values interior to the face
  for (isp = 0; isp < nSurfPts; isp++) {
    if (face_pnt[isp] == 1) {

      decodeEgadsID(surfacedata[isp].egadsID, &it, &ib, &in);
      if (it != FACEID) continue;

      if (ibody != ib) {
        printf(" Error: Inconsistent body index!\n" );
        goto cleanup;
      }

      ivp = surfacedata[isp].ind-1;
      ifp = face_ind[isp];

      uv[0] = limits[0] + surfacedata[isp].param[0]*(limits[1]-limits[0]);
      uv[1] = limits[2] + surfacedata[isp].param[1]*(limits[3]-limits[2]);
      if ((geom->mtype == CYLINDRICAL) || (geom->mtype == CONICAL) ||
          (geom->mtype == SPHERICAL)   || (geom->mtype == TOROIDAL))
        correctUV(face, geom, bodydata->surfaces[iface+bodydata->nfaces],
                  bodydata->rvec[iface], uv);

      bodydata->faces_xyz[iface][3*ifp  ] = volumeMesh->node[ivp].xyz[0];
      bodydata->faces_xyz[iface][3*ifp+1] = volumeMesh->node[ivp].xyz[1];
      bodydata->faces_xyz[iface][3*ifp+2] = volumeMesh->node[ivp].xyz[2];

      bodydata->faces_uv[iface][2*ifp  ] = uv[0];
      bodydata->faces_uv[iface][2*ifp+1] = uv[1];
    }
  }

  for (iloop = 0; iloop < nloop; iloop++) {
    status = EG_getTopology(loops[iloop], &ref, &oclass, &mtype, limits, &nedge, &edges, &senses);
    if (status != EGADS_SUCCESS) goto cleanup;

    for (iedge = 0; iedge < nedge; iedge++) {

      // skip degenerate edges
      status = EG_getTopology(edges[iedge], &ref, &oclass, &mtype, trange, &nnode, &nodes, &tmp);
      if (status != EGADS_SUCCESS) goto cleanup;
      if (mtype == DEGENERATE) continue;

      // try to get edge UV at t-mid with sense of 0
      offset = 0;
      status = EG_getEdgeUV(face, edges[iedge], 0, (trange[0]+trange[1])/2, uv);
      if (status == EGADS_TOPOERR) {
        /* sense in Face twice! */
        offset = nSurfPts;
      }

      edgeIndex = EG_indexBodyTopo(bodydata->body, edges[iedge]);

      exyz = bodydata->edges_xyz[edgeIndex-1];

      npts = bodydata->edges_npts[edgeIndex-1];
      for (i = 0; i < npts; i++) {

        t   = bodydata->edges_t[edgeIndex-1][i];
        isp = bodydata->edges_isp[edgeIndex-1][i] + offset;

        ifp = face_ind[isp];

        status = EG_getEdgeUV(face, edges[iedge], senses[iedge], t, uv);
        if (status != EGADS_SUCCESS) goto cleanup;

        bodydata->faces_xyz[iface][3*ifp  ] = exyz[3*i  ];
        bodydata->faces_xyz[iface][3*ifp+1] = exyz[3*i+1];
        bodydata->faces_xyz[iface][3*ifp+2] = exyz[3*i+2];

        bodydata->faces_uv[iface][2*ifp  ] = uv[0];
        bodydata->faces_uv[iface][2*ifp+1] = uv[1];
      }
    }
  }

  status = CAPS_SUCCESS;
  cleanup:
    if (status != CAPS_SUCCESS) printf("Error: Premature exit in getFaceUV status = %d\n", status);

    return status;
}

static void swapd(double *xp, double *yp) {

    double temp = *xp;
    *xp = *yp;
    *yp = temp;
}

static void swapi(int *xp, int *yp) {

    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// A function to implement bubble sort
static void bubbleSort(int n, double t[], double xyz[], int isp[]) {

  int i, j;
  for (i = 0; i < n-1; i++)
    // Last i elements are already in place
    for (j = 0; j < n-i-1; j++)
      if (t[j] > t[j+1]) {
        swapd(&t[j]      , &t[j+1]        );
        swapd(&xyz[3*j+0], &xyz[3*(j+1)+0]);
        swapd(&xyz[3*j+1], &xyz[3*(j+1)+1]);
        swapd(&xyz[3*j+2], &xyz[3*(j+1)+2]);
        swapi(&isp[j]    , &isp[j+1]      );
      }
}

/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(int ngIn, capsValue *gIn, int *qeFlag, const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int flag;  // query/execute flag
    aimStorage *tmp;

#ifdef DEBUG
    printf("\n pointwiseAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif

    flag    = *qeFlag;

    // Does the AIM execute itself (i.e. no external executable is called)
    *qeFlag = 0; // 1 = AIM executes itself, 0 otherwise

    // specify the number of analysis input and out "parameters"
    *nIn     = NUMINPUT;
    *nOut    = NUMOUT;
    if (flag == 1) return CAPS_SUCCESS;

    // Specify the field variables this analysis can generate
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Allocate pointwiseInstance
    tmp = (aimStorage *) EG_reall(pointwiseInstance, (numInstance+1)*sizeof(aimStorage));
    if (tmp == NULL) return EGADS_MALLOC;
    pointwiseInstance = tmp;

    // Set initial values for pointwiseInstance
    initiate_aimStorage(numInstance);

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}

// Available AIM inputs
int aimInputs(int iIndex, void *aimInfo, int index, char **ainame,
        capsValue *defval)
{

    int input = 0;
    /*! \page aimInputsPointwise AIM Inputs
     * The following list outlines the Pointwise options along with their default value available
     * through the AIM interface.
     */

#ifdef DEBUG
    printf(" pointwiseAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
#endif

    // Inputs
    if (index == ++input) {
        *ainame              = EG_strdup("Proj_Name"); // If NULL a volume grid won't be written by the AIM
        defval->type         = String;
        defval->nullVal      = IsNull;
        defval->vals.string  = NULL;
        defval->lfixed       = Change;

        /*! \page aimInputsPointwise
         * - <B> Proj_Name = NULL</B> <br>
         * This corresponds to the output name of the mesh. If left NULL, the mesh is not written to a file.
         */
    } if (index == ++input) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->vals.string   = EG_strdup("VTK"); // TECPLOT, VTK, AFLR3, STL, AF, FAST, NASTRAN
        defval->lfixed        = Change;
        defval->nullVal       = IsNull;

        /*! \page aimInputsPointwise
         * - <B> Mesh_Format = NULL</B> <br>
         * Mesh output format. Available format names include: "AFLR3", "VTK", "TECPLOT", SU2, "Nastran".
         * This file format is written from CAPS, and is not the CAE solver in Pointwise.
         */
    } if (index == ++input) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsPointwise
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file, if applicable.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Mesh_Sizing");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsPointwise
         * - <B>Mesh_Sizing = NULL </B> <br>
         * These parameters are implemented by overriding PW: attributes.
         * See \ref meshSizingProp for additional details.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Mesh_Length_Factor");
        defval->type         = Double;
        defval->dim          = Scalar;
        defval->vals.real    = 1;
        defval->nullVal      = NotNull;

        /*! \page aimInputsPointwise
         * - <B> Mesh_Length_Factor = 1</B> <br>
         * Scaling factor to compute a meshing Reference_Length via:<br>
         * <br>
         * Reference_Length = capsMeshLength*Mesh_Length_Factor<br>
         * <br>
         * Reference_Length scales all input parameters with units of length
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Initial_Dim");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 11;

        /*! \page aimInputsPointwise
         * - <B> Connector_Initial_Dim = 11</B> <br>
         * Initial connector dimension.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Max_Dim");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 1024;

        /*! \page aimInputsPointwise
         * - <B> Connector_Max_Dim = 1024</B> <br>
         * Maximum connector dimension.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Min_Dim");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 4;

        /*! \page aimInputsPointwise
         * - <B> Connector_Min_Dim = 4</B> <br>
         * Minimum connector dimension.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Turn_Angle");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = EG_strdup("degree");
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsPointwise
         * - <B> Connector_Turn_Angle = 0.0</B> <br>
         * Maximum turning angle on connectors for dimensioning (0 - not used).
         * Influences connector resolution in high curvature regions. Suggested values from 5 to 20 degrees.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Deviation");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsPointwise
         * - <B> Connector_Deviation = 0.0</B> <br>
         * Maximum deviation on connectors for dimensioning (0 - not used).
         * This is the maximum distance between the center of a segment on the connector to the CAD surface.
         * Influences connector resolution in high curvature regions.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Split_Angle");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsPointwise
         * - <B> Connector_Split_Angle = 0.0</B> <br>
         * Turning angle on connectors to split (0 - not used).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Prox_Growth_Rate");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 1.3;

        /*! \page aimInputsPointwise
         * - <B> Connector_Prox_Growth_Rate = 1.3</B> <br>
         *  Connector proximity growth rate.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Adapt_Sources");
        defval->type         = Boolean;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 0;

        /*! \page aimInputsPointwise
         * - <B> Connector_Adapt_Sources = False</B> <br>
         *  Compute sources using connectors.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Source_Spacing");
        defval->type         = Boolean;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 0;

        /*! \page aimInputsPointwise
         * - <B> Connector_Source_Spacing = False</B> <br>
         *  Use source cloud for adaptive pass on connectors V18.2+.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Connector_Turn_Angle_Hard");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 70;

        /*! \page aimInputsPointwise
         * - <B> Connector_Turn_Angle_Hard = 70</B> <br>
         * Hard edge turning angle limit for domain T-Rex (0.0 - not used).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Algorithm");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.string  = EG_strdup("Delaunay");

        /*! \page aimInputsPointwise
         * - <B> Domain_Algorithm = "Delaunay"</B> <br>
         * Isotropic (Delaunay, AdvancingFront or AdvancingFrontOrtho).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Full_Layers");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 0;

        /*! \page aimInputsPointwise
         * - <B> Domain_Full_Layers = 0</B> <br>
         * Domain full layers (0 for multi-normals, >= 1 for single normal).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Max_Layers");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 0;

        /*! \page aimInputsPointwise
         * - <B> Domain_Max_Layers = 0</B> <br>
         * Domain maximum layers.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Growth_Rate");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 1.3;

        /*! \page aimInputsPointwise
         * - <B> Domain_Growth_Rate = 1.3</B> <br>
         * Domain growth rate for 2D T-Rex extrusion.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Iso_Type");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.string  = EG_strdup("Triangle");

        /*! \page aimInputsPointwise
         * - <B> Domain_Iso_Type = "Triangle"</B> <br>
         * Domain iso cell type (Triangle or TriangleQuad).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_TRex_Type");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.string  = EG_strdup("Triangle");

        /*! \page aimInputsPointwise
         * - <B> Domain_TRex_Type = "Triangle"</B> <br>
         * Domain T-Rex cell type (Triangle or TriangleQuad).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_TRex_ARLimit");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 200.0;

        /*! \page aimInputsPointwise
         * - <B> Domain_TRex_ARLimit = 200.0</B> <br>
         * Domain T-Rex maximum aspect ratio limit (0 - not used).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Decay");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.5;

        /*! \page aimInputsPointwise
         * - <B> Domain_Decay = 0.5</B> <br>
         * Domain boundary decay.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Min_Edge");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsPointwise
         * - <B> Domain_Min_Edge = 0.0</B> <br>
         * Domain minimum edge length.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Max_Edge");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsPointwise
         * - <B> Domain_Max_Edge = 0.0</B> <br>
         * Domain minimum edge length.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Domain_Adapt");
        defval->type         = Boolean;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 0;

        /*! \page aimInputsPointwise
         * - <B> Domain_Adapt = False</B> <br>
         * Set up all domains for adaptation.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Algorithm");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.string  = EG_strdup("Delaunay");

        /*! \page aimInputsPointwise
         * - <B> Domain_Algorithm = "Delaunay"</B> <br>
         * Isotropic (Delaunay, Voxel) (V18.3+).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Voxel_Layers");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 3;

        /*! \page aimInputsPointwise
         * - <B> Block_Voxel_Layers = 3</B> <br>
         * Number of Voxel transition layers if Algorithm set to Voxel (V18.3+).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Boundary_Decay");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.5;

        /*! \page aimInputsPointwise
         * - <B> Block_Boundary_Decay = 0.5</B> <br>
         * Volumetric boundary decay.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Collision_Buffer");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.5;

        /*! \page aimInputsPointwise
         * - <B> Block_Collision_Buffer = 0.5</B> <br>
         * Collision buffer for colliding T-Rex fronts.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Max_Skew_Angle");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 180.0;

        /*! \page aimInputsPointwise
         * - <B> Block_Max_Skew_Angle = 180.0</B> <br>
         * Maximum skew angle for T-Rex extrusion.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Edge_Max_Growth_Rate");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 1.8;

        /*! \page aimInputsPointwise
         * - <B> Block_Edge_Max_Growth_Rate = 1.8</B> <br>
         * Volumetric edge ratio.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Full_Layers");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 0;

        /*! \page aimInputsPointwise
         * - <B> Block_Full_Layers = 0</B> <br>
         * Full layers (0 for multi-normals, >= 1 for single normal).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Max_Layers");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 0;

        /*! \page aimInputsPointwise
         * - <B> Block_Max_Layers = 0</B> <br>
         * Maximum layers.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_Growth_Rate");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 1.3;

        /*! \page aimInputsPointwise
         * - <B> Block_Growth_Rate = 1.3</B> <br>
         * Growth rate for volume T-Rex extrusion.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Block_TRexType");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.string  = EG_strdup("TetPyramid");

        /*! \page aimInputsPointwise
         * - <B> Block_TRexType = "TetPyramid"</B> <br>
         * T-Rex cell type (TetPyramid, TetPyramidPrismHex, AllAndConvertWallDoms).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Gen_Source_Box_Length_Scale");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsPointwise
         * - <B> Gen_Source_Box_Length_Scale = 0.0</B> <br>
         * Length scale of enclosed viscous walls in source box (0 - no box).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Gen_Source_Box_Direction");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Vector;
        defval->length        = 3;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->vals.reals    = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals != NULL) {
            defval->vals.reals[0] = 1.0;
            defval->vals.reals[1] = 0.0;
            defval->vals.reals[2] = 0.0;
        } else return EGADS_MALLOC;

        /*! \page aimInputsPointwise
         * - <B> Gen_Source_Box_Direction = [1.0, 0.0, 0.0]</B> <br>
         * Principal direction vector (i.e. normalized freestream vector).
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Gen_Source_Box_Angle");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsPointwise
         * - <B> Gen_Source_Box_Angle = 0.0</B> <br>
         * Angle for widening source box in the assigned direction.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Gen_Source_Growth_Factor");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 10.0;

        /*! \page aimInputsPointwise
         * - <B> Gen_Source_Growth_Factor = 10.0</B> <br>
         * Growth rate for spacing value along box.
         */
#ifdef IMPLEMENTED_HIGH_ORDER_MESH_READ
    } if (index == ++input) {
        *ainame              = EG_strdup("Elevate_Cost_Threshold");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.8;

        /*! \page aimInputsPointwise
         * - <B> Elevate_Cost_Threshold = 0.8</B> <br>
         * Cost convergence threshold.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Elevate_Max_Include_Angle");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 175.0;

        /*! \page aimInputsPointwise
         * - <B> Elevate_Max_Include_Angle = 175.0</B> <br>
         * Maximum included angle tolerance.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Elevate_Relax");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.05;

        /*! \page aimInputsPointwise
         * - <B> Elevate_Relax = 0.05</B> <br>
         * Iteration relaxation factor.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Elevate_Smoothing_Passes");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 1000;

        /*! \page aimInputsPointwise
         * - <B> Elevate_Smoothing_Passes = 1000</B> <br>
         * Number of smoothing passes.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Elevate_WCN_Weight");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.5;

        /*! \page aimInputsPointwise
         * - <B> Elevate_WCN_Weight = 0.5</B> <br>
         * WCN cost component weighting factor.
         */
    } if (index == ++input) {
        *ainame              = EG_strdup("Elevate_WCN_Mode");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.string  = EG_strdup("Calculate");

        /*! \page aimInputsPointwise
         * - <B> Elevate_WCN_Mode = "Calculate"</B> <br>
         * WCN weight factor method (UseValue or Calculate).
         */
#endif
    }

    if (input != NUMINPUT) {
        printf("DEVELOPER ERROR: NUMINPUTS %d != %d\n", NUMINPUT, input);
        return CAPS_BADINDEX;
    }

    return CAPS_SUCCESS;
}

// Shareable data for the AIM - typically optional
int aimData(int iIndex, const char *name, enum capsvType *vtype,
        int *rank, int *nrow, int *ncol, void **data, char **units)
{

  /*! \page sharableDataPointwise AIM Shareable Data
   * The Pointwise AIM has the following shareable data types/values with its children AIMs if they are so inclined.
   * - <B> Surface_Mesh</B> <br>
   * The returned surface mesh in meshStruct (see meshTypes.h) format.
   * - <B> Volume_Mesh</B> <br>
   * The returned volume mesh after AFLR3 execution is complete in meshStruct (see meshTypes.h) format.
   * - <B> Attribute_Map</B> <br>
   * An index mapping between capsGroups found on the geometry in mapAttrToIndexStruct (see miscTypes.h) format.
   */

  #ifdef DEBUG
      printf(" pointwiseAIM/aimData instance = %d  name = %s!\n", inst, name);
  #endif

  // The returned surface mesh from AFLR3
  if (strcasecmp(name, "Surface_Mesh") == 0){
      *vtype = Value;
      *rank  = *ncol = 1;
      *nrow = pointwiseInstance[iIndex].numSurfaceMesh;
      *data  = &pointwiseInstance[iIndex].surfaceMesh;
      *units = NULL;

      return CAPS_SUCCESS;
  }

  // The returned Volume mesh from AFLR3
  if (strcasecmp(name, "Volume_Mesh") == 0){
      *vtype = Value;
      *rank  = *ncol = 1;
      *nrow = pointwiseInstance[iIndex].numVolumeMesh;
      if (pointwiseInstance[iIndex].numVolumeMesh == 1) {
          *data  = &pointwiseInstance[iIndex].volumeMesh[0];
      } else {
          *data  = pointwiseInstance[iIndex].volumeMesh;
      }

      *units = NULL;

      return CAPS_SUCCESS;
  }

  // Share the attribute map
  if (strcasecmp(name, "Attribute_Map") == 0){
      *vtype = Value;
      *rank  = *nrow = *ncol = 1;
      *data  = &pointwiseInstance[iIndex].attrMap;
      *units = NULL;

      return CAPS_SUCCESS;
  }

    return CAPS_NOTFOUND;

}

// AIM preAnalysis function
int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath, capsValue *aimInputs, capsErrs **errs)
{
    int status; // Status return
    int i, bodyIndex; // Index

    char currentPath[PATH_MAX]; // Current directory path

    char egadsFileName[] = "caps.egads";

    // Mesh attribute parameters
    int numMeshProp = 0;
    meshSizingStruct *meshProp = NULL;

    double capsMeshLength = 0;

    const char *intents;
    int numBody = 0; // Number of bodies
    ego *bodies = NULL; // EGADS body objects
    ego *bodyCopy = NULL;

    ego context, model = NULL;

    int quilting = (int)false;

    // NULL out errors
    *errs = NULL;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    #ifdef DEBUG
        printf(" pointwiseAIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, numBody);
    #endif

    if (numBody <= 0 || bodies == NULL) {
        #ifdef DEBUG
            printf(" pointwiseAIM/aimPreAnalysis No Bodies!\n");
        #endif

        return CAPS_SOURCEERR;
    }


    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) return CAPS_DIRERR;


    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the
    // same instance
    status = destroy_aimStorage(iIndex);
    if (status != CAPS_SUCCESS) {
        printf("Status = %d, pointwiseAIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
        goto cleanup;
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
                                            2, // Only search down to the face level of the EGADS bodyIndex
                                            &pointwiseInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Get mesh sizing parameters
    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].nullVal != IsNull) {

        status = mesh_getSizingProp(aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].length,
                                    aimInputs[aim_getIndex(aimInfo, "Mesh_Sizing", ANALYSISIN)-1].vals.tuple,
                                    &pointwiseInstance[iIndex].attrMap,
                                    &numMeshProp,
                                    &meshProp);
        if (status != CAPS_SUCCESS) return status;
    }

    // Get the capsMeshLenght if boundary layer meshing has been requested
    status = check_CAPSMeshLength(numBody, bodies, &capsMeshLength);

    if (capsMeshLength <= 0 || status != CAPS_SUCCESS) {
        printf("**********************************************************\n");
        if (status != CAPS_SUCCESS)
          printf("capsMeshLength is not set on any body.\n");
        else
          printf("capsMeshLength: %f\n", capsMeshLength);
        printf("\n");
        printf("The capsMeshLength attribute must present on at least one body.\n"
               "\n"
               "capsMeshLength should be a a positive value representative\n"
               "of a characteristic length of the geometry,\n"
               "e.g. the MAC of a wing or diameter of a fuselage.\n");
        printf("**********************************************************\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // Scale the reference length
    capsMeshLength *= aimInputs[aim_getIndex(aimInfo, "Mesh_Length_Factor", ANALYSISIN)-1].vals.real;

    bodyCopy = (ego *) EG_alloc(numBody*sizeof(ego));
    if (bodyCopy == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    for (i = 0; i < numBody; i++) bodyCopy[i] = NULL;

    // Get context
    status = EG_getContext(bodies[0], &context);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Make a copy of the bodies
    for (i = 0; i < numBody; i++) {
        status = EG_copyObject(bodies[i], NULL, &bodyCopy[i]);
        if (status != EGADS_SUCCESS) goto cleanup;
        status = setPWAttr(bodyCopy[i], &pointwiseInstance[iIndex].attrMap, numMeshProp, meshProp, capsMeshLength, &quilting);
        if (status != EGADS_SUCCESS) goto cleanup;
    }

    // Auto quilt faces
    /*
    if (aimInputs[aim_getIndex(aimInfo, "Auto_Quilt_Flag", ANALYSISIN)-1].vals.integer == (int) true) {
        printf("Automatically quilting faces...\n");
        for (i = 0; i < numBody; i++) {
            status = set_autoQuiltAttr(&bodyCopy[i]);
            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }
    */

    // Create a model from the copied bodies
    status = EG_makeTopology(context, NULL, MODEL, 0, NULL, numBody, bodyCopy, NULL, &model);
    if (status != EGADS_SUCCESS) goto cleanup;

    printf("Writing global Glyph inputs...\n");
    status = writeGlobalGlyph(aimInfo, aimInputs);
    if (status != CAPS_SUCCESS) goto cleanup;

    printf("Writing egads file....\n");
    remove(egadsFileName);
    status = EG_saveModel(model,egadsFileName);
    if (status != EGADS_SUCCESS) {
        printf(" EG_saveModel status = %d\n", status);
        goto cleanup;
    }

    /*
    status = NMB_write(model, nmbFileName, asciiOut, verbose, modelSize);
    if (status != EGADS_SUCCESS) {
        printf(" NMB_write status = %d\n", status);
        goto cleanup;
    }
    */

    status = CAPS_SUCCESS;

    if (quilting == (int)true) {
        printf("Error: Quilting is enabled with 'PW:QuiltName' attribute on faces.\n");
        printf("       Pointwise input files were generated, but CAPS cannot process the resulting grid.\n");
       status = CAPS_MISMATCH;
    }

    cleanup:

        if (status != CAPS_SUCCESS) printf("Error: pointwiseAIM (instance = %d) status %d\n", iIndex, status);

        chdir(currentPath);

        // Clean up meshProps
        if (meshProp != NULL) {
            for (i = 0; i < numMeshProp; i++) {
                destroy_meshSizingStruct(&meshProp[i]);
            }
            EG_free(meshProp); meshProp = NULL;
        }

        // delete the model
        if (model != NULL) {
          EG_deleteObject(model);
        } else {
          if (bodyCopy != NULL) {
            for (i = 0;  i < numBody; i++) {
              if (bodyCopy[i] != NULL)  {
                (void) EG_deleteObject(bodyCopy[i]);
              }
            }
          }
        }
        EG_free(bodyCopy);

        return status;
}


// Read back in the resulting grid
int
aimPostAnalysis(/*@unused@*/ int iIndex, /*@unused@*/ void *aimInfo,
                const char *analysisPath, capsErrs **errs)
{
    int status = CAPS_SUCCESS;

    char currentPath[PATH_MAX]; // Current directory path

    int        i, j, k, n, id, ib, it, in, ib2, it2, in2, ib3, it3, in3;
    int        ifp, isp, ivp, npts, nSurfPts, nVolPts, iper, iline = 0, cID = 0;
    int        oclass, mtype, numBody = 0, *senses = NULL, *ivec = NULL, *face_pnt = NULL, *face_ind = NULL, *surf_ind = NULL, *edgeCount = NULL;
    int        numFaces = 0, iface, numEdges = 0, iedge, numNodes = 0, inode, nDegen = 0, periodic, duplicate;
    int        ntri = 0, nquad = 0, elem[4], *face_tris, nedge, edge_npts, edgeIndex, nNodeEdge, elemIndex;
    char       gmafilename[]   = "caps.GeomToMesh.gma";
    char       ugridfilename[] = "caps.GeomToMesh.ugrid";
    const char *intents;
    const char *groupName = NULL;
    double     limits[4], trange[4], result[18], uv[2], uv_orig[2], uv_dup[2], t, du_orig, dv_orig, du_dup, dv_dup;
    double     *face_uv = NULL, *face_xyz = NULL, coord[3];
#ifdef CHECKGRID
    double     d;
#endif
    double     *rvec, v1[3], v2[3], faceNormal[3], triNormal[3], ndot;
    ego        geom, geom2, obj, ref, *faces = NULL, *edges = NULL, *nodes = NULL, *bodies = NULL, *objs = NULL, tess, prev, next, *nodeEdges = NULL;
    bodyData   *bodydata = NULL;
    gmaVertex  *surfacedata = NULL;
    meshStruct *surfaceMeshes = NULL, *volumeMesh;
    hashElemTable table;
    FILE       *fp = NULL;

    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) return CAPS_DIRERR;

    initiate_hashTable(&table);

    fp = fopen(ugridfilename, "rb");
    if (fp == NULL) {
      printf("*********************************************************\n");
      printf("\n Error: Pointwise did not generate %s!\n\n", ugridfilename);
      printf("*********************************************************\n");
      status = CAPS_IOERR;
      goto cleanup;
    }

    pointwiseInstance[iIndex].numVolumeMesh = 1;
    pointwiseInstance[iIndex].volumeMesh = (meshStruct *) EG_alloc(pointwiseInstance[iIndex].numVolumeMesh*sizeof(meshStruct));
    if (pointwiseInstance[iIndex].volumeMesh == NULL) { status = EGADS_MALLOC; goto cleanup; }

    status = initiate_meshStruct(pointwiseInstance[iIndex].volumeMesh);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = getUGRID(fp, pointwiseInstance[iIndex].volumeMesh);
    fclose(fp); fp = NULL;
    if (status != EGADS_SUCCESS) {
      printf("\n Error: getUGRID = %d!\n\n", status);
      goto cleanup;
    }
    nVolPts = pointwiseInstance[iIndex].volumeMesh->numNode;
    volumeMesh = pointwiseInstance[iIndex].volumeMesh;

    // map from the total volume index the indexing of the surface points in the GMA file
    surf_ind = (int *) EG_alloc(nVolPts*sizeof(int));
    if (surf_ind == NULL ) { status = EGADS_MALLOC; goto cleanup; }
    for (i = 0; i < nVolPts; i++) surf_ind[i] = -1;

    // construct the hash table into the surface elements to mark ID's
    allocate_hashTable(nVolPts, volumeMesh->meshQuickRef.numTriangle +
                                volumeMesh->meshQuickRef.numQuadrilateral, &table);
    for (i = 0; i < volumeMesh->meshQuickRef.numTriangle; i++) {
      elemIndex = i+volumeMesh->meshQuickRef.startIndexTriangle;
      status = hash_addElement(3, volumeMesh->element[elemIndex].connectivity,
                               i, &table);
      if (status != CAPS_SUCCESS) goto cleanup;
    }
    for (i = 0; i < volumeMesh->meshQuickRef.numQuadrilateral; i++) {
      elemIndex = i+volumeMesh->meshQuickRef.startIndexQuadrilateral;
      status = hash_addElement(4, volumeMesh->element[elemIndex].connectivity,
                               i+volumeMesh->meshQuickRef.numTriangle, &table);
      if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != EGADS_SUCCESS) goto cleanup;

    if (numBody == 0) {
        printf(" Error: numBody = 0!\n");
        status = CAPS_BADOBJECT;
        goto cleanup;
    }
    bodydata = (bodyData *) EG_alloc(numBody*sizeof(bodyData));
    if (bodydata == NULL) {
        printf(" Error: EG_alloc on Body storage!\n");
        status = EGADS_MALLOC;
        goto cleanup;
    }
    initiate_bodyData(numBody, bodydata);

    /* get all of the EGADS Objects */
    for (i = 0; i < numBody; i++) {
      bodydata[i].body = bodies[i];

      status = EG_getBodyTopos(bodies[i], NULL, NODE, &bodydata[i].nnodes, &bodydata[i].nodes);
      if (status != EGADS_SUCCESS) {
          printf(" Body %d: EG_getBodyTopos NODE = %d\n", i+1, status);
          goto cleanup;
      }
      status = EG_getBodyTopos(bodies[i], NULL, EDGE, &bodydata[i].nedges, &bodydata[i].edges);
      if (status != EGADS_SUCCESS) {
          printf(" Body %d: EG_getBodyTopos EDGE = %d\n", i+1, status);
          goto cleanup;
      }
      numEdges += bodydata[i].nedges; // Accumulate the total number of edges

      status = EG_getBodyTopos(bodies[i], NULL, FACE, &bodydata[i].nfaces, &bodydata[i].faces);
      if (status != EGADS_SUCCESS) {
          printf(" Body %d: EG_getBodyTopos FACE = %d\n", i+1, status);
          goto cleanup;
      }
      numFaces += bodydata[i].nfaces; // Accumulate the total number of faces

      bodydata[i].surfaces = (ego *) EG_alloc(2*bodydata[i].nfaces*sizeof(ego));
      if (bodydata[i].surfaces == NULL) continue;
      for (j = 0; j < 2*bodydata[i].nfaces; j++) bodydata[i].surfaces[j] = NULL;

      bodydata[i].rvec = (double **) EG_alloc(bodydata[i].nfaces*sizeof(double *));
      if (bodydata[i].rvec == NULL) continue;
      for (j = 0; j < bodydata[i].nfaces; j++) bodydata[i].rvec[j] = NULL;

      bodydata[i].nodes_isp = (int *) EG_alloc(bodydata[i].nnodes*sizeof(int));
      if (bodydata[i].nodes_isp == NULL) continue;
      for (j = 0; j < bodydata[i].nnodes; j++) bodydata[i].nodes_isp[j] = 0;

      bodydata[i].edges_npts = (int *) EG_alloc(bodydata[i].nedges*sizeof(int));
      if (bodydata[i].edges_npts == NULL) continue;
      for (j = 0; j < bodydata[i].nedges; j++) bodydata[i].edges_npts[j] = 0;

      bodydata[i].edges_xyz = (double **) EG_alloc(bodydata[i].nedges*sizeof(double *));
      if (bodydata[i].edges_xyz == NULL) continue;
      for (j = 0; j < bodydata[i].nedges; j++) bodydata[i].edges_xyz[j] = NULL;

      bodydata[i].edges_t = (double **) EG_alloc(bodydata[i].nedges*sizeof(double *));
      if (bodydata[i].edges_t == NULL) continue;
      for (j = 0; j < bodydata[i].nedges; j++) bodydata[i].edges_t[j] = NULL;

      bodydata[i].edges_isp = (int **) EG_alloc(bodydata[i].nedges*sizeof(int *));
      if (bodydata[i].edges_isp == NULL) continue;
      for (j = 0; j < bodydata[i].nedges; j++) bodydata[i].edges_isp[j] = NULL;

      bodydata[i].faces_npts = (int *) EG_alloc(bodydata[i].nfaces*sizeof(int));
      if (bodydata[i].faces_npts == NULL) continue;
      for (j = 0; j < bodydata[i].nfaces; j++) bodydata[i].faces_npts[j] = 0;

      bodydata[i].faces_xyz = (double **) EG_alloc(bodydata[i].nfaces*sizeof(double *));
      if (bodydata[i].faces_xyz == NULL) continue;
      for (j = 0; j < bodydata[i].nfaces; j++) bodydata[i].faces_xyz[j] = NULL;

      bodydata[i].faces_uv = (double **) EG_alloc(bodydata[i].nfaces*sizeof(double *));
      if (bodydata[i].faces_uv == NULL) continue;
      for (j = 0; j < bodydata[i].nfaces; j++) bodydata[i].faces_uv[j] = NULL;

      bodydata[i].faces_ntri = (int *) EG_alloc(bodydata[i].nfaces*sizeof(int));
      if (bodydata[i].faces_ntri == NULL) continue;
      for (j = 0; j < bodydata[i].nfaces; j++) bodydata[i].faces_ntri[j] = 0;

      bodydata[i].faces_nquad = (int *) EG_alloc(bodydata[i].nfaces*sizeof(int));
      if (bodydata[i].faces_nquad == NULL) continue;
      for (j = 0; j < bodydata[i].nfaces; j++) bodydata[i].faces_nquad[j] = 0;

      bodydata[i].faces_tris = (int **) EG_alloc(bodydata[i].nfaces*sizeof(int *));
      if (bodydata[i].faces_tris == NULL) continue;
      for (j = 0; j < bodydata[i].nfaces; j++) bodydata[i].faces_tris[j] = NULL;

      for (j = 0; j < bodydata[i].nfaces; j++) {
        status = EG_getTopology(bodydata[i].faces[j], &bodydata[i].surfaces[j],
                                &oclass, &mtype, limits, &n, &objs, &senses);
        if (status != EGADS_SUCCESS) goto cleanup;

        geom = bodydata[i].surfaces[j];
        if (geom->mtype == TRIMMED) {
          status = EG_getGeometry(geom, &oclass, &mtype, &ref, &ivec, &rvec);
          if (status != EGADS_SUCCESS) {
              printf(" Error: Face %d getGeometry status = %d!\n", j+1, status);
              continue;
          }
          bodydata[i].surfaces[j] = ref;
          EG_free(ivec);
          EG_free(rvec);
          geom = ref;
        }

        if ((geom->mtype != CYLINDRICAL) && (geom->mtype != CONICAL) &&
            (geom->mtype != SPHERICAL)   && (geom->mtype != TOROIDAL)) continue;

        status = EG_getGeometry(geom, &oclass, &mtype, &ref, &ivec, &bodydata[i].rvec[j]);
        if (status != EGADS_SUCCESS) {
            printf(" Error: Surface %d getGeometry status = %d!\n", j+1, status);
            continue;
        }

        status = EG_convertToBSpline(bodydata[i].faces[j], &bodydata[i].surfaces[j+bodydata[i].nfaces]);
        if (status != EGADS_SUCCESS) {
            printf(" Error: Face %d Convert status = %d!\n", j+1, status);
            continue;
        }
      }
    }


    /* open and parse the gma file to count surface/edge tessellations points */
    fp = fopen(gmafilename, "r");
    if (fp == NULL) {
        printf(" Error: Cannot open file: %s!\n", gmafilename);
        goto cleanup;
    }
    status = fscanf(fp, "%d", &nSurfPts); iline++;
    if (status != 1) {
        printf(" Error: Cannot get NPTS!\n");
        goto cleanup;
    }
    surfacedata = (gmaVertex *) EG_alloc(nSurfPts*sizeof(gmaVertex));
    if (surfacedata == NULL) {
        printf(" Error: EG_alloc on vertex storage!\n");
        status = EGADS_MALLOC;
        goto cleanup;
    }

    //printf(" npts = %d\n", npts);
    for (j = 0; j < nSurfPts; j++) {
        status = fscanf(fp, "%d %d %lf %lf\n", &surfacedata[j].ind, &surfacedata[j].egadsID, &surfacedata[j].param[0], &surfacedata[j].param[1]); iline++;
        if (status != 4) {
            printf(" Error: read line %d return = %d\n", iline, status);
            goto cleanup;
        }
        decodeEgadsID(surfacedata[j].egadsID, &it, &ib, &in);

        // save the mapping from the volume indexing to the surface indexing
        surf_ind[surfacedata[j].ind-1] = j;

    /*  printf(" %d: %d  %d -- %d %d %d\n", i, status, id, it, ib, in);  */
        if ((ib < 0) || (ib >= numBody)) {
            printf(" Error: line %d Bad body index = %d [1-%d]!\n", iline, ib+1, numBody);
        }
        if (it == NODEID) {
            /* Nodes */
            if ((in < 0) || (in >= bodydata[ib].nnodes)) {
                printf(" Error: line %d Bad Node index = %d [1-%d]!\n", iline, in+1, bodydata[ib].nnodes);
                goto cleanup;
            }
        } else if (it == EDGEID) {
            /* Edges */
            if ((in < 0) || (in >= bodydata[ib].nedges)) {
                printf(" Error: line %d Bad Edge index = %d [1-%d]!\n", iline, in+1, bodydata[ib].nedges);
                goto cleanup;
            }
        } else if (it == FACEID) {
            /* Faces */
            if ((in < 0) || (in >= bodydata[ib].nfaces)) {
                printf(" Error: line %d Bad Face index = %d [1-%d]!\n", iline, in+1, bodydata[ib].nfaces);
                goto cleanup;
            }
        } else {
            printf(" Error: line %d Bad type = %d!\n", iline, it);
            goto cleanup;
        }
    }

    // Count the number of degenerate edges in all bodies
    nDegen = 0;
    for (ib = 0; ib < numBody; ib++) {
      for ( iedge = 0; iedge < bodydata[ib].nedges; iedge++ ) {
        status = EG_getTopology( bodydata[ib].edges[iedge], &ref, &oclass, &mtype, limits, &n, &objs, &senses);
        if (status != EGADS_SUCCESS) goto cleanup;
        if ( mtype == DEGENERATE ) nDegen++;
      }
    }

    // read in the edge tessellation connectivity
    for (iedge = 0; iedge < numEdges-nDegen; iedge++) {

        status = fscanf(fp, "%d %d\n", &id, &npts); iline++;
        if (status != 2) {
            printf(" Error: read line %d return = %d\n", iline, status);
            goto cleanup;
        }
        decodeEgadsID(id, &it, &ib, &in);
        if (it == EDGEID) {
            /* Edges */
            if ((in < 0) || (in >= bodydata[ib].nedges)) {
                printf(" Error: line %d Bad Edge index = %d [1-%d]!\n", iline, in+1, bodydata[ib].nedges);
                goto cleanup;
            }

            bodydata[ib].edges_npts[in] = npts;
            bodydata[ib].edges_xyz[in]  = (double *) EG_alloc(3*nSurfPts*sizeof(double));
            bodydata[ib].edges_t[in]    = (double *) EG_alloc(  nSurfPts*sizeof(double));
            bodydata[ib].edges_isp[in]  = (int    *) EG_alloc(  nSurfPts*sizeof(int   ));

            if (bodydata[ib].edges_xyz[in] == NULL ||
                bodydata[ib].edges_t[in]   == NULL ||
                bodydata[ib].edges_isp[in] == NULL) { status = EGADS_MALLOC; goto cleanup; }

            obj    = bodydata[ib].edges[in];
            status = EG_getTopology(obj, &geom, &oclass, &mtype, trange, &numNodes, &nodes, &senses);
            if (status != EGADS_SUCCESS) {
                printf(" Error: line %d Bad Edge status = %d!\n", iline, status);
                goto cleanup;
            }
            if (geom->mtype == BSPLINE) {
                status = EG_getRange(geom, limits, &iper);
                if (status != EGADS_SUCCESS) {
                    printf(" Error: line %d EG_getRange C = %d!\n", iline, status);
                    goto cleanup;
                }
            } else {
                limits[0] = trange[0];
                limits[1] = trange[1];
            }

            // populate the edge tessellations
            for (j = 0; j < npts; j++) {
              status = fscanf(fp, "%d\n", &ivp); iline++;
              if ((i < 1) || (i > nVolPts)) {
                  printf(" Error: line %d Bad Vertex index = %d [1-%d]!\n", iline, ivp, nVolPts);
                  goto cleanup;
              }

              ivp -= 1; // change to 0-based indexing
              isp  = surf_ind[ivp]; // map the volume index to the surface

              if (ivp != surfacedata[isp].ind-1) {
                printf(" Error: line %d Inconsistent edge indexing!\n", iline);
                goto cleanup;
              }

              // get the interpolated t-value of the edge
              t = limits[0] + surfacedata[isp].param[0]*(limits[1]-limits[0]);

              decodeEgadsID(surfacedata[isp].egadsID, &it2, &ib2, &in2);
              if (it2 == NODEID) {
                status = EG_getTopology(bodydata[ib2].nodes[in2], &geom2, &oclass, &mtype, coord, &n, &objs, &senses);
                if (status != EGADS_SUCCESS) {
                    printf(" Error: line %d Bad Node status = %d!\n", iline, status);
                    goto cleanup;
                }
#ifdef CHECKGRID
                d = dist_DoubleVal(coord, volumeMesh->node[ivp].xyz);
                if (d > PTOL) printf(" line %d: %d %d Node deviation = %le\n", iline, ib2+1, in2+1, d);
#endif
                if ((ib != ib2)) {
                    printf(" Error: line %d Inconsistent Edge Vertex index!\n", iline);
                    status = EGADS_TOPOERR;
                    goto cleanup;
                }
                // save the surface index of the node
                bodydata[ib].nodes_isp[in2] = isp;
                // get the t based on the node match a the limits
                for (inode = 0; inode < numNodes; inode++) {
                  t = trange[inode];
                  if (nodes[inode] == bodydata[ib].nodes[in2]) break;
                }
                // special treatment for a one-node
                if (mtype == ONENODE) t = (j == 0) ? trange[0] : trange[1];

                if (inode == numNodes) {
                  printf(" Error: line %d Could not find edge node!\n", iline);
                  status = EGADS_TOPOERR;
                  goto cleanup;
                }
                volumeMesh->node[ivp].xyz[0] = coord[0];
                volumeMesh->node[ivp].xyz[1] = coord[1];
                volumeMesh->node[ivp].xyz[2] = coord[2];
#ifdef CHECKGRID
                status = EG_evaluate(obj, &t, result);
                if (status != EGADS_SUCCESS) {
                    printf(" Error: line %d Bad Edge status = %d!\n", iline, status);
                    goto cleanup;
                }
                d = dist_DoubleVal(result, volumeMesh->node[ivp].xyz);
                if (d > PTOL) printf(" line %3d: %d Node %d Edge-t %d, %d deviation = %le  %lf  %d\n",
                                     iline, ib+1, in2+1, in+1, j+1, d, t, geom->mtype);
#endif
              } else if (it2 == EDGEID) {
#ifdef CHECKGRID
                status = EG_evaluate(obj, &t, result);
                if (status != EGADS_SUCCESS) {
                    printf(" Error: line %d Bad Edge status = %d!\n", iline, status);
                    goto cleanup;
                }
                d = dist_DoubleVal(result, volumeMesh->node[ivp].xyz);
                if (d > PTOL) printf(" line %3d: %d %d Edge deviation = %le  %lf  %d\n",
                                     iline, ib+1, in+1, d, t, geom->mtype);
                if (d > PTOL) {
                  status = EG_invEvaluate(obj, volumeMesh->node[ivp].xyz, &t, coord);
                  if (status != EGADS_SUCCESS) goto cleanup;
                  d = dist_DoubleVal(coord, volumeMesh->node[ivp].xyz);
                  printf("           %d  %le  %lf [%lf %lf]\n", status, d, t, limits[0], limits[1]);
                }
#endif
                if ((ib != ib2) || (in != in2)) {
                    printf(" Error: line %d Inconsistent Edge Vertex index!\n", iline);
                    status = EGADS_TOPOERR;
                    goto cleanup;
                }
              }
              bodydata[ib].edges_xyz[in][3*j  ] = volumeMesh->node[ivp].xyz[0];
              bodydata[ib].edges_xyz[in][3*j+1] = volumeMesh->node[ivp].xyz[1];
              bodydata[ib].edges_xyz[in][3*j+2] = volumeMesh->node[ivp].xyz[2];

              bodydata[ib].edges_t[in][j] = t;

              bodydata[ib].edges_isp[in][j] = isp;
            }
            bubbleSort(bodydata[ib].edges_npts[in], bodydata[ib].edges_t[in], bodydata[ib].edges_xyz[in], bodydata[ib].edges_isp[in]);

        } else {
            printf(" Error: line %d Type = %d is not an EDGEID!\n", iline, it);
            printf("        Found %d edges when expecting %d\n", iedge, numEdges-nDegen);
            status = CAPS_MISMATCH;
            goto cleanup;
        }
    }

    // logical flag to tag which points are on a face
    face_pnt = (int *) EG_alloc(nSurfPts*sizeof(int));
    if (face_pnt == NULL ) { status = EGADS_MALLOC; goto cleanup; }

    // map from the total index to a face local index in a face tessellation
    // the size is doubled to account for edge that require points to be duplicated
    face_ind = (int *) EG_alloc(2*nSurfPts*sizeof(int));
    if (face_ind == NULL ) { status = EGADS_MALLOC; goto cleanup; }

    // read in the face tessellation connectivity
    for (iface = 0; iface < numFaces; iface++) {
        status = fscanf(fp, "%d %d %d\n", &id, &ntri, &nquad); iline++;
        if (status != 3) {
            printf(" Error: read line %d return = %d\n", iline, status);
            goto cleanup;
        }
        decodeEgadsID(id, &it, &ib, &in);

        if (it == FACEID) {
          /* Faces */
          if ((in < 0) || (in >= bodydata[ib].nfaces)) {
            printf(" Error: line %d Bad Face index = %d [1-%d]!\n", iline, in+1, bodydata[ib].nfaces);
            goto cleanup;
          }

          obj  = bodydata[ib].faces[in];
          status = EG_getTopology(obj, &geom, &oclass, &mtype, limits, &n, &objs, &senses);
          if (status != EGADS_SUCCESS) {
              printf(" Error: line %d Bad Face status = %d!\n", iline, status);
              goto cleanup;
          }
          status = EG_getRange(obj, limits, &periodic);
          if (status != EGADS_SUCCESS) {
              printf(" Error: line %d Bad EG_getRange status = %d!\n", iline, status);
              goto cleanup;
          }

          // Look for component/boundary ID for attribute mapper based on capsGroup
          status = retrieve_CAPSGroupAttr(obj, &groupName);
          if (status != CAPS_SUCCESS) {
              printf("Error: No capsGroup attribute found on Face %d, unable to assign a boundary index value\n", in+1);
              printf("Available attributes are:\n");
              print_AllAttr( obj );
              goto cleanup;
          }

          status = get_mapAttrToIndexIndex(&pointwiseInstance[iIndex].attrMap, groupName, &cID);
          if (status != CAPS_SUCCESS) {
              printf("Error: Unable to retrieve boundary index from capsGroup %s\n", groupName);
              goto cleanup;
          }

          // check how many times an edge occurs in the loop
          getFaceEdgeCount(bodydata[ib].body, obj, &nedge, &edges, &edgeCount);

          // reset the face index flags
          for (isp = 0; isp < nSurfPts; isp++) {
            face_pnt[isp] = 0;
            face_ind[isp] = -1;
          }
          for (isp = nSurfPts; isp < 2*nSurfPts; isp++) {
            face_ind[isp] = -1;
          }

          bodydata[ib].faces_ntri[in] = ntri;
          bodydata[ib].faces_tris[in] = (int *) EG_alloc(3*ntri*sizeof(int));
          if (bodydata[ib].faces_tris[in] == NULL ) { status = EGADS_MALLOC; goto cleanup; }

          bodydata[ib].faces_nquad[in] = nquad;
          if (nquad != 0) {
            printf(" Error: line %d Quads are currently not supported!\n", iline);
            goto cleanup;
          }

          for (i = 0; i < ntri; i++) {
            status = fscanf(fp, "%d %d %d\n", &elem[0], &elem[1], &elem[2]); iline++;
            if (status != 3) {
              printf(" Error: read line %d return = %d\n", iline, status);
              goto cleanup;
            }
            for (j = 0; j < 3; j++) {
              if ((elem[j] < 1) || (elem[j] > nVolPts)) {
                printf(" Error: line %d Bad Vertex index = %d [1-%d]!\n", iline, elem[j], nVolPts);
                goto cleanup;
              }
            }

            // find the element index from the table and set the face marker
            status = hash_getIndex(3, elem, &table, &elemIndex);
            if (status != CAPS_SUCCESS) goto cleanup;
            volumeMesh->element[elemIndex+volumeMesh->meshQuickRef.startIndexTriangle].markerID = cID;
            volumeMesh->element[elemIndex+volumeMesh->meshQuickRef.startIndexTriangle].topoIndex = in+1;

            // map the index to the surface
            elem[0] = surf_ind[elem[0]-1];
            elem[1] = surf_ind[elem[1]-1];
            elem[2] = surf_ind[elem[2]-1];

            // these triangle now map into surfacedata
            bodydata[ib].faces_tris[in][3*i  ] = elem[0];
            bodydata[ib].faces_tris[in][3*i+1] = elem[1];
            bodydata[ib].faces_tris[in][3*i+2] = elem[2];

            // mark the points that are part of this face
            face_pnt[elem[0]] = 1;
            face_pnt[elem[1]] = 1;
            face_pnt[elem[2]] = 1;
          }

          // count the number of face points and create the map to face local indexing
          npts = 0;
          for (isp = 0; isp < nSurfPts; isp++) {
            if (face_pnt[isp] == 1) {
              face_ind[isp] = npts;
              npts++;
            }
          }

          // add duplicated points
          for (iedge = 0; iedge < nedge; iedge++) {
            if (edgeCount[iedge] == 2) {
              edgeIndex = EG_indexBodyTopo(bodies[ib], edges[iedge]);
              edge_npts = bodydata[ib].edges_npts[edgeIndex-1];

              for (i = 0; i < edge_npts; i++) {
                // offset the surface index by the total number of surface points
                isp = bodydata->edges_isp[edgeIndex-1][i] + nSurfPts;

                face_ind[isp] = npts;
                npts++;
              }
            }
          }

          // allocate the vertex memory
          bodydata[ib].faces_npts[in] = npts;
          bodydata[ib].faces_xyz[in]  = (double *) EG_alloc(3*npts*sizeof(double));
          bodydata[ib].faces_uv[in]   = (double *) EG_alloc(2*npts*sizeof(double));

          if (bodydata[ib].faces_xyz[in] == NULL ||
              bodydata[ib].faces_uv[in]  == NULL) { status = EGADS_MALLOC; goto cleanup; }

          // get the face UV values from the triangles and edges
          getFacePoints(bodydata+ib, ib, in, volumeMesh, nSurfPts, surfacedata, face_pnt, face_ind, uv);

#ifdef CHECKGRID
          // check the face vertexes
          for (isp = 0; isp < nSurfPts; isp++) {
            if (face_pnt[isp] == 1) {

              ivp = surfacedata[isp].ind-1;
              ifp = face_ind[isp];

              uv[0] = bodydata[ib].faces_uv[in][2*ifp  ];
              uv[1] = bodydata[ib].faces_uv[in][2*ifp+1];

              status = EG_evaluate(obj, uv, result);
              if (status != EGADS_SUCCESS) {
                printf(" Error: line %d Bad Face stat = %d!\n", iline, status);
                goto cleanup;
              }
              d = dist_DoubleVal(result, volumeMesh->node[ivp].xyz);
              if (d > PTOL) printf(" line %d: %d %d Face deviation = %le  %d  %d\n",
                                   iline, ib+1, in+1, d, geom->mtype, surfacedata[isp].egadsID);
            }
          }
#endif

          for (i = 0; i < ntri; i++) {
            // get the connectivity
            elem[0] = bodydata[ib].faces_tris[in][3*i  ];
            elem[1] = bodydata[ib].faces_tris[in][3*i+1];
            elem[2] = bodydata[ib].faces_tris[in][3*i+2];


            // update element connectivity based on periodicity
            for (j = 0; j < 3; j++) {
              decodeEgadsID(surfacedata[elem[j]].egadsID, &it2, &ib2, &in2);

              duplicate = (int) false;

              if (it2 == NODEID) {
                status = EG_getBodyTopos(bodies[ib], bodydata[ib].nodes[in2], EDGE, &nNodeEdge, &nodeEdges);
                if (status != EGADS_SUCCESS) goto cleanup;

                for (iedge = 0; iedge < nedge && duplicate == (int)false; iedge++) {
                  for (k = 0; k < nNodeEdge; k++)
                    if (nodeEdges[k] == edges[iedge] && edgeCount[iedge] == 2) {
                      duplicate = (int)true;
                      break;
                    }
                }
                EG_free(nodeEdges); nodeEdges = NULL;
              }

              if (it2 == EDGEID) {
                for (iedge = 0; iedge < nedge; iedge++) {
                  edgeIndex = EG_indexBodyTopo(bodies[ib], edges[iedge]);

                  if (edgeIndex == in2 && edgeCount[iedge] == 2) {
                    duplicate = (int)true;
                    break;
                  }
                }
              }

              if ( duplicate == (int)true ){

                status = EG_getRange(bodydata[ib].edges[in2], trange, &iper);
                if (status != EGADS_SUCCESS) goto cleanup;

                // get the original uv
                ifp = face_ind[elem[j]];

                uv_orig[0] = bodydata[ib].faces_uv[in][2*ifp  ];
                uv_orig[1] = bodydata[ib].faces_uv[in][2*ifp+1];

                // and the duplicated uv
                ifp = face_ind[elem[j]+nSurfPts];

                uv_dup[0] = bodydata[ib].faces_uv[in][2*ifp  ];
                uv_dup[1] = bodydata[ib].faces_uv[in][2*ifp+1];

                // look for a vertex not on the edge
                for (k = 0; k < 3; k++) {
                  if (k == j) continue;

                  decodeEgadsID(surfacedata[elem[k]].egadsID, &it3, &ib3, &in3);

                  // the vertex cannot be on the same edge
                  if (it3 == EDGEID && in2 == in3) continue;

                  ifp = face_ind[elem[k]];

                  uv[0] = bodydata[ib].faces_uv[in][2*ifp  ];
                  uv[1] = bodydata[ib].faces_uv[in][2*ifp+1];

                  du_orig = fabs( uv[0] - uv_orig[0] );
                  dv_orig = fabs( uv[1] - uv_orig[1] );

                  du_dup = fabs( uv[0] - uv_dup[0] );
                  dv_dup = fabs( uv[1] - uv_dup[1] );

                  if (periodic == 1) { // periodicity in u

                    if (du_dup < du_orig) {
                      elem[j] += nSurfPts;
                      break;
                    }

                  } else if (periodic == 2) { // periodicity in v

                    if (dv_dup < dv_orig) {
                      elem[j] += nSurfPts;
                      break;
                    }

                  } else if (periodic == 3) { // periodicity in both u and v

                    status = EG_getEdgeUVeval(faces[i], objs[j], 1, (trange[0]+trange[1])/2, result);
                    if (status != EGADS_SUCCESS) {
                      printf(" EGADS Internal: EG_getEdgeUVeval = %d\n", status);
                      continue;
                    }

                    // du/dt != 0 means variation in u, and constant v
                    if (result[3] != 0.0) {
                      if (du_dup < du_orig) {
                        elem[j] += nSurfPts;
                        break;
                      }
                    } else { // otherwase the variation is in v
                      if (dv_dup < dv_orig) {
                        elem[j] += nSurfPts;
                        break;
                      }
                    }
                  }
                }
              }
            }



            // map the connectivity to face indexing
            elem[0] = face_ind[elem[0]];
            elem[1] = face_ind[elem[1]];
            elem[2] = face_ind[elem[2]];

            // put it back 1-based
            bodydata[ib].faces_tris[in][3*i  ] = elem[0]+1;
            bodydata[ib].faces_tris[in][3*i+1] = elem[1]+1;
            bodydata[ib].faces_tris[in][3*i+2] = elem[2]+1;
          }

          EG_free(edges); edges = NULL;
          EG_free(edgeCount); edgeCount = NULL;
        } else {
          printf(" Error: line %d Type = %d is not a FACEID!\n", iline, it);
          printf("        Found %d faces when expecting %d\n", iface, numFaces);
          goto cleanup;
        }
    }
    // done reading the file
    fclose(fp); fp = NULL;


    // Allocate surfaceMesh from number of bodies
    pointwiseInstance[iIndex].numSurfaceMesh = numBody;
    pointwiseInstance[iIndex].surfaceMesh = (meshStruct *) EG_alloc(pointwiseInstance[iIndex].numSurfaceMesh*sizeof(meshStruct));
    if (pointwiseInstance[iIndex].surfaceMesh == NULL) {
        pointwiseInstance[iIndex].numSurfaceMesh = 0;
        status = EGADS_MALLOC;
        goto cleanup;
    }

    // Initiate surface meshes
    for (ib = 0; ib < numBody; ib++){
        status = initiate_meshStruct(&pointwiseInstance[iIndex].surfaceMesh[ib]);
        if (status != CAPS_SUCCESS) goto cleanup;
    }
    surfaceMeshes = pointwiseInstance[iIndex].surfaceMesh;

    // populate the tess objects
    for (ib = 0; ib < numBody; ib++) {

        // Build up the body tessellation object
        status = EG_initTessBody(bodies[ib], &tess);
        if (status != EGADS_SUCCESS) goto cleanup;

        for ( iedge = 0; iedge < bodydata[ib].nedges; iedge++ )
        {
          // Check if the edge is degenerate
          status = EG_getTopology( bodydata[ib].edges[iedge], &ref, &oclass, &mtype, limits, &n, &objs, &senses);
          if (status != EGADS_SUCCESS) goto cleanup;
          if ( mtype == DEGENERATE ) continue;

          // set the edge tessellation on the tess object
          status = EG_setTessEdge( tess, iedge+1, bodydata[ib].edges_npts[iedge],
                                   bodydata[ib].edges_xyz[iedge], bodydata[ib].edges_t[iedge] );
          if (status != EGADS_SUCCESS) goto cleanup;
       }

        for (iface = 0; iface < bodydata[ib].nfaces; iface++) {

            ntri  = bodydata[ib].faces_ntri[iface];
            nquad = bodydata[ib].faces_ntri[iface];

            face_tris = bodydata[ib].faces_tris[iface];
            face_uv   = bodydata[ib].faces_uv[iface];
            face_xyz  = bodydata[ib].faces_xyz[iface];

            // check the normals of the elements match the geometry normals
            // only need to check one element per face to decide for all
            elem[0] = face_tris[0]-1;
            elem[1] = face_tris[1]-1;
            elem[2] = face_tris[2]-1;

            // get the uv centroid
            uv[0] = (face_uv[2*elem[0]+0] + face_uv[2*elem[1]+0] + face_uv[2*elem[2]+0])/3.;
            uv[1] = (face_uv[2*elem[0]+1] + face_uv[2*elem[1]+1] + face_uv[2*elem[2]+1])/3.;

            // get the normal of the face
            status = EG_evaluate(bodydata[ib].faces[iface], uv, result);
            if (status != EGADS_SUCCESS) goto cleanup;

            // use cross dX/du x dX/dv to get geometey normal
            v1[0] = result[3]; v1[1] = result[4]; v1[2] = result[5];
            v2[0] = result[6]; v2[1] = result[7]; v2[2] = result[8];
            CROSS(faceNormal, v1, v2);

            // get mtype=SFORWARD or mtype=SREVERSE for the face to get topology normal
            status = EG_getInfo(bodydata[ib].faces[iface], &oclass, &mtype, &ref, &prev, &next);
            if (status != EGADS_SUCCESS) goto cleanup;
            faceNormal[0] *= mtype;
            faceNormal[1] *= mtype;
            faceNormal[2] *= mtype;

            // get the normal of the mesh triangle
            v1[0] = face_xyz[3*elem[1]+0] - face_xyz[3*elem[0]+0];
            v1[1] = face_xyz[3*elem[1]+1] - face_xyz[3*elem[0]+1];
            v1[2] = face_xyz[3*elem[1]+2] - face_xyz[3*elem[0]+2];

            v2[0] = face_xyz[3*elem[2]+0] - face_xyz[3*elem[0]+0];
            v2[1] = face_xyz[3*elem[2]+1] - face_xyz[3*elem[0]+1];
            v2[2] = face_xyz[3*elem[2]+2] - face_xyz[3*elem[0]+2];

            CROSS(triNormal, v1, v2);

            // get the dot product between the triangle and face normals
            ndot = DOT(faceNormal,triNormal);

            // if the normals are opposite, swap all triangles
            if (ndot < 0) {
              for (i = 0; i < ntri; i++) {
                elem[0] = face_tris[3*i+0];
                elem[1] = face_tris[3*i+1];
                elem[2] = face_tris[3*i+2];

                // swap two vertices to reverse the normal
                face_tris[3*i + 0] = elem[2];
                face_tris[3*i + 1] = elem[1];
                face_tris[3*i + 2] = elem[0];
              }
            }

            status = EG_setTessFace(tess,
                                    iface+1,
                                    bodydata[ib].faces_npts[iface],
                                    face_xyz,
                                    face_uv,
                                    ntri,
                                    face_tris);

            if (status != CAPS_SUCCESS) goto cleanup;
        }

        // finalize the tessellation
        status = EG_statusTessBody(tess, &bodies[ib], &i, &n);
        if (status != EGADS_SUCCESS) {
            printf("\nTessellation object was not built correctly!!!\n");
            goto cleanup;
        }

        // construct the surface mesh object
        surfaceMeshes[ib].bodyTessMap.egadsTess = tess;

        surfaceMeshes[ib].bodyTessMap.numTessFace = bodydata[ib].nfaces; // Number of faces in the tessellation
        surfaceMeshes[ib].bodyTessMap.tessFaceQuadMap = NULL; // Save off the quad map

        status = mesh_surfaceMeshEGADSTess(&pointwiseInstance[iIndex].attrMap,
                                           &surfaceMeshes[ib]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // save the tessellation with caps
        status = aim_setTess(aimInfo, surfaceMeshes[ib].bodyTessMap.egadsTess);
        if (status != CAPS_SUCCESS) {
             printf(" aim_setTess return = %d\n", status);
             goto cleanup;
         }

        printf("Body = %d\n", ib+1);
        printf("\tNumber of nodes    = %d\n", surfaceMeshes[ib].numNode);
        printf("\tNumber of elements = %d\n", surfaceMeshes[ib].numElement);
        if (surfaceMeshes[ib].meshQuickRef.useStartIndex == (int) true ||
            surfaceMeshes[ib].meshQuickRef.useListIndex == (int) true) {
            printf("\tNumber of tris = %d\n", surfaceMeshes[ib].meshQuickRef.numTriangle);
            printf("\tNumber of quad = %d\n", surfaceMeshes[ib].meshQuickRef.numQuadrilateral);
        }

    }

    // write out the mesh if requested
    status = writeMesh(iIndex, aimInfo);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = CAPS_SUCCESS;

    cleanup:
        if (status != CAPS_SUCCESS) {
          printf("Error: Premature exit in pointwiseAIM aimPostAnalysis, status %d\n", status);
          printf("\n");
          printf("       Please make sure you are using Pointwise V18.2 or newer.\n");
          printf("*********************************************************\n");
        }

        destroy_bodyData(numBody, bodydata);
        destroy_hashTable(&table);
        EG_free(surfacedata);
        EG_free(bodydata);
        EG_free(faces);
        EG_free(edges);
        EG_free(face_pnt);
        EG_free(face_ind);
        EG_free(surf_ind);
        if (fp != NULL) fclose(fp);

        chdir(currentPath);

        return status;
}


// Available AIM outputs
int aimOutputs(int iIndex, void *aimInfo,
               int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsPointwise AIM Outputs
     * The following list outlines the Pointwise AIM outputs available through the AIM interface.
     *
     * - <B>Done</B> = True if a volume mesh(es) was created, False if not.
     */

#ifdef DEBUG
    printf(" pointwiseAIM/aimOutputs instance = %d  index = %d!\n", inst, index);
#endif

    if (index == 1) {
        *aoname = EG_strdup("OutputVariable");
        form->type = Boolean;
        form->vals.integer = (int) false;
    }

    return CAPS_SUCCESS;
}

// Get value for a given output variable
int aimCalcOutput(int iIndex, void *aimInfo,
                  const char *analysisPath, int index,
                  capsValue *val, capsErrs **errors)
{
    int i; // Indexing

#ifdef DEBUG
    printf(" pointwiseAIM/aimCalcOutput instance = %d  index = %d!\n", iIndex, index);
#endif

    // Fill in to populate output variable = index
    *errors           = NULL;
    val->vals.integer = (int) false;

    // Check to see if a volume mesh was generated - maybe a file was written, maybe not
    for (i = 0; i < pointwiseInstance[iIndex].numVolumeMesh; i++) {
        // Check to see if a volume mesh was generated
        if (pointwiseInstance[iIndex].volumeMesh[i].numElement != 0 &&
            pointwiseInstance[iIndex].volumeMesh[i].meshType == VolumeMesh) {

            val->vals.integer = (int) true;

        } else {
            val->vals.integer = (int) false;

            if (pointwiseInstance[iIndex].numVolumeMesh > 1) {
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
    int iIndex; // Indexing

    int status; // Returning status

#ifdef DEBUG
    printf(" pointwiseAIM/aimCleanup!\n");
#endif

    // Clean up pointwiseInstance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up pointwiseInstance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS) printf("Status = %d, pointwiseAIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
    }

    if (pointwiseInstance != NULL) EG_free(pointwiseInstance);
    pointwiseInstance = NULL;
    numInstance = 0;

}
