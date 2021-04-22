/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             masstran AIM
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */


/*! \mainpage Introduction
 * \tableofcontents
 *
 * \section overviewMasstran Masstran AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to compute
 * mass properties using attributions for finite element structural solvers.
 *
 * An outline of the AIM's inputs, outputs and attributes are provided in \ref aimInputsMasstran and
 * \ref aimOutputsMasstran and \ref attributeMasstran, respectively.
 *
 * The mass properties are computed via the formulas:
 *
 *  \f{eqnarray*}{
 *  m &=& \sum_i m_i \\
 *  x_{cg} &=& \frac{1}{m}\sum_i m_i x_i \\
 *  y_{cg} &=& \frac{1}{m}\sum_i m_i y_i \\
 *  z_{cg} &=& \frac{1}{m}\sum_i m_i z_i \\
 * \big(I_{xx}\big)_{cg} &=& \sum_i m_i \big(y_i^2 + z_i^2\big) - m \big(y_{cg}^2 + z_{cg}^2\big) \\
 * \big(I_{yy}\big)_{cg} &=& \sum_i m_i \big(x_i^2 + z_i^2\big) - m \big(x_{cg}^2 + z_{cg}^2\big) \\
 * \big(I_{zz}\big)_{cg} &=& \sum_i m_i \big(x_i^2 + y_i^2\big) - m \big(x_{cg}^2 + y_{cg}^2\big) \\
 * \big(I_{xy}\big)_{cg} &=& \sum_i m_i \big(x_i     y_i  \big) - m \big(x_{cg}     y_{cg}  \big) \\
 * \big(I_{xz}\big)_{cg} &=& \sum_i m_i \big(x_i     z_i  \big) - m \big(x_{cg}     z_{cg}  \big) \\
 * \big(I_{yz}\big)_{cg} &=& \sum_i m_i \big(y_i     z_i  \big) - m \big(y_{cg}     z_{cg}  \big),
 * \f}
 *
 * where i represents an element index in the mesh, and the mass \f$m_i\f$ is computed from the density, thickness, and area of the element.
 *
 * The moment of inertias are accessible individually, in vector form as
 *
 * \f[
 *  \vec{I} = \begin{bmatrix} I_{xx} & I_{yy} & I_{zz} & I_{xy} & I_{xz} & I_{yz} \end{bmatrix},
 * \f]
 *
 * as lower/upper triangular form
 *
 * \f[
 *  \vec{I}_{lower} = \begin{bmatrix} I_{xx} & -I_{xy} & I_{yy} & -I_{xz} & -I_{yz} & I_{zz} \end{bmatrix},
 * \f]
 *
 * \f[
 *  \vec{I}_{upper} = \begin{bmatrix} I_{xx} & -I_{xy} & -I_{xz} & I_{yy} & -I_{yz} & I_{zz} \end{bmatrix},
 * \f]
 *
 * or in full tensor form as
 *
 * \f[
 * \bar{\bar{I}} =
 *  \begin{bmatrix}
 *    I_{xx} & -I_{xy} & -I_{xz} \\
 *   -I_{xy} &  I_{yy} & -I_{yz} \\
 *   -I_{xz} & -I_{yz} &  I_{zz}
 *  \end{bmatrix}.
 * \f]
 *
 *\section masstranExamples Examples
 * An example problem using the Masstran AIM may be found at \ref masstranExample.
 */


/*! \page attributeMasstran Masstran AIM attributes
 * The following list of attributes are required for the MYSTRAN AIM inside the geometry input.
 *
 * - <b> capsAIM</b> This attribute is a CAPS requirement to indicate the analysis the geometry
 * representation supports.
 *
 * - <b> capsGroup</b> This is a name assigned to any geometric body.  This body could be a solid, surface, face, wire, edge or node.
 * Recall that a string in ESP starts with a $.  For example, attribute <c>capsGroup $Wing</c>.
 *
 * - <b> capsIgnore</b> It is possible that there is a geometric body (or entity) that you do not want the Masstran AIM to pay attention to when creating
 * a finite element model. The capsIgnore attribute allows a body (or entity) to be in the geometry and ignored by the AIM.  For example,
 * because of limitations in OpenCASCADE a situation where two edges are overlapping may occur; capsIgnore allows the user to only
 *  pay attention to one of the overlapping edges.
 *
 */

#include <string.h>
#include <math.h>
#include "aimUtil.h"

#include "meshUtils.h"
#include "miscUtils.h"
#include "feaUtils.h"

//#define DEBUG

#define NUMINPUT   6
#define NUMOUTPUT  14

#define NINT(A)   (((A) < 0)   ? (int)(A-0.5) : (int)(A+0.5))
#define MIN(A,B)  (((A) < (B)) ? (A) : (B))
#define MAX(A,B)  (((A) < (B)) ? (B) : (A))

#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])

typedef struct {

  double area;
  double mass;
  double Ixx, Iyy, Izz;
  double Ixy, Ixz, Iyz;
  double Cx, Cy, Cz;
  double CGx, CGy, CGz;

} massProperties;

/* AIM "local" per instance storage
   needed data should be added here & cleaned up in aimCleanup */
typedef struct {

  // Project name
  char *projectName; // Project name

  // Analysis file path/directory
  const char *analysisPath;

  feaProblemStruct feaProblem;

  // Attribute to index map
  mapAttrToIndexStruct attrMap;

  // Mesh holders
  int numMesh;
  meshStruct *feaMesh;

  // mass properties
  massProperties massProp;

} aimStorage;


/* AIM instance counter & storage */
static int        nInstance = 0;
static aimStorage *masstranInstance = NULL;


static int initiate_aimStorage(int iIndex) {

    int status;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&masstranInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_feaProblemStruct(&masstranInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS) return status;

    // Mesh holders
    masstranInstance[iIndex].numMesh = 0;
    masstranInstance[iIndex].feaMesh = NULL;

    return CAPS_SUCCESS;
}

static int destroy_aimStorage(int iIndex) {

    int status;
    int i;
    // Attribute to index map
    status = destroy_mapAttrToIndexStruct(&masstranInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Cleanup meshes
    if (masstranInstance[iIndex].feaMesh != NULL) {

        for (i = 0; i < masstranInstance[iIndex].numMesh; i++) {
            status = destroy_meshStruct(&masstranInstance[iIndex].feaMesh[i]);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_meshStruct!\n", status);
        }

        EG_free(masstranInstance[iIndex].feaMesh);
    }

    masstranInstance[iIndex].feaMesh = NULL;
    masstranInstance[iIndex].numMesh = 0;

    // Destroy FEA problem structure
    status = destroy_feaProblemStruct(&masstranInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS)  printf("Error: Status %d during destroy_feaProblemStruct!\n", status);

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

  // Dummy attribute to maps
  mapAttrToIndexStruct constraintMap;
  mapAttrToIndexStruct loadMap;
  mapAttrToIndexStruct transferMap;
  mapAttrToIndexStruct connectMap;

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

  status = initiate_mapAttrToIndexStruct(&constraintMap);
  if (status != CAPS_SUCCESS) return status;

  status = initiate_mapAttrToIndexStruct(&loadMap);
  if (status != CAPS_SUCCESS) return status;

  status = initiate_mapAttrToIndexStruct(&transferMap);
  if (status != CAPS_SUCCESS) return status;

  status = initiate_mapAttrToIndexStruct(&connectMap);
  if (status != CAPS_SUCCESS) return status;

  status = fea_createMesh(aimInfo,
                          tessParam,
                          edgePointMin,
                          edgePointMax,
                          quadMesh,
                          &masstranInstance[iIndex].attrMap,
                          &constraintMap,
                          &loadMap,
                          &transferMap,
                          &connectMap,
                          &masstranInstance[iIndex].numMesh,
                          &masstranInstance[iIndex].feaMesh,
                          &masstranInstance[iIndex].feaProblem );
  if (status != CAPS_SUCCESS) return status;

  status = destroy_mapAttrToIndexStruct(&constraintMap);
  if (status != CAPS_SUCCESS) return status;

  status = destroy_mapAttrToIndexStruct(&loadMap);
  if (status != CAPS_SUCCESS) return status;

  status = destroy_mapAttrToIndexStruct(&transferMap);
  if (status != CAPS_SUCCESS) return status;

  status = destroy_mapAttrToIndexStruct(&connectMap);
  if (status != CAPS_SUCCESS) return status;

  return CAPS_SUCCESS;
}

/****************** exposed AIM entry points -- Analysis **********************/

/* aimInitialize: Initialization Information for the AIM */
int
aimInitialize(int ngIn, /*@null@*/ capsValue *gIn, int *qeFlag,
              /*@unused@*/ const char *unitSys, int *nIn, int *nOut,
              int *nFields, char ***fnames, int **ranks)
{
  int        ret, flag, status, i;
  char       **strs = NULL;

#ifdef DEBUG
  printf("\n masstranAIM/aimInitialize  ngIn = %d  qeFlag = %d!\n",
         ngIn, *qeFlag);
#endif

  /* on Input: 1 indicates a query and not an analysis instance */
  flag = *qeFlag;

  /* on Output: 1 specifies that the AIM executes the analysis
   *              (i.e. no external executable is called)
   *            0 relies on external execution */
  *qeFlag = 1;

  /* specify the number of analysis inputs  defined in aimInputs
   *     and the number of analysis outputs defined in aimOutputs */
  *nIn    = NUMINPUT;
  *nOut   = NUMOUTPUT;

  /* return if "query" only */
  if (flag == 1) return CAPS_SUCCESS;

  /* specify the field variables this analysis can generate */
  *nFields = 0;

  /* specify the dimension of each field variable */
  *ranks   = NULL;

  /* specify the name of each field variable */
  *fnames  = NULL;

  /* create our "local" storage for anything that needs to be persistent
     remember there can be multiple instances of the AIM */
  masstranInstance = (aimStorage *) EG_reall(masstranInstance, (nInstance+1)*sizeof(aimStorage));

  if (masstranInstance == NULL) {
    status = EGADS_MALLOC;
    goto cleanup;
  }

  /* return the index for the instance generated & store our local data */
  ret = nInstance;
  initiate_aimStorage(nInstance);
  nInstance++; /* increment the instance count */
  return ret;

cleanup:
  /* release all possibly allocated memory on error */
  printf("masstranAIM/aimInitialize: failed to allocate memory\n");
  *nFields = 0;

  EG_free(*ranks);
  *ranks = NULL;
  if (*fnames != NULL)
    for (i = 0; i < *nFields; i++) EG_free(strs[i]);
  EG_free(*fnames);
  *fnames = NULL;

  return status;
}


/* aimInputs: Input Information for the AIM */
int
aimInputs(int iIndex, /*@unused@*/ void *aimInfo, int index, char **ainame,
          capsValue *defval)
{

  /*! \page aimInputsMasstran AIM Inputs
   * The following list outlines the Masstran inputs along with their default value available
   * through the AIM interface.
   */

#ifdef DEBUG
  printf(" nastranAIM/aimInputs instance = %d  index = %d!\n", inst, index);
#endif
  if ((iIndex < 0) || (iIndex >= nInstance)) return CAPS_BADINDEX;

  *ainame = NULL;

  // Masstran Inputs
  if (index == 1) {
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

    /*! \page aimInputsMasstran
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

  } else if (index == 2) {
    *ainame               = EG_strdup("Edge_Point_Min");
    defval->type          = Integer;
    defval->vals.integer  = 2;
    defval->lfixed        = Fixed;
    defval->nrow          = 1;
    defval->ncol          = 1;
    defval->nullVal       = NotNull;

    /*! \page aimInputsMasstran
     * - <B> Edge_Point_Min = 2</B> <br>
     * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
     */

  } else if (index == 3) {
    *ainame               = EG_strdup("Edge_Point_Max");
    defval->type          = Integer;
    defval->vals.integer  = 50;
    defval->length        = 1;
    defval->lfixed        = Fixed;
    defval->nrow          = 1;
    defval->ncol          = 1;
    defval->nullVal       = NotNull;

    /*! \page aimInputsMasstran
     * - <B> Edge_Point_Max = 50</B> <br>
     * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
     */

  } else if (index == 4) {
    *ainame               = EG_strdup("Quad_Mesh");
    defval->type          = Boolean;
    defval->vals.integer  = (int) false;

    /*! \page aimInputsMasstran
     * - <B> Quad_Mesh = False</B> <br>
     * Create a quadratic mesh on four edge faces when creating the boundary element model.
     */

  } else if (index == 5) {
    *ainame              = EG_strdup("Property");
    defval->type         = Tuple;
    defval->nullVal      = IsNull;
    //defval->units        = NULL;
    defval->lfixed       = Change;
    defval->vals.tuple   = NULL;
    defval->dim          = Vector;

    /*! \page aimInputsMasstran
     * - <B> Property = NULL</B> <br>
     * Property tuple used to input property information for the model, see \ref feaProperty for additional details.
     */
  } else if (index == 6) {
    *ainame              = EG_strdup("Material");
    defval->type         = Tuple;
    defval->nullVal      = IsNull;
    //defval->units        = NULL;
    defval->lfixed       = Change;
    defval->vals.tuple   = NULL;
    defval->dim          = Vector;

    /*! \page aimInputsMasstran
     * - <B> Material = NULL</B> <br>
     * Material tuple used to input material information for the model, see \ref feaMaterial for additional details.
     */

  } else {
    printf(" nastranAIM/aimInputs: unknown input index = %d for instance = %d!\n",
           index, iIndex);
    return CAPS_BADINDEX;
  }

#if NUMINPUT != 6
#error "NUMINPUT is inconsistent with the list of inputs"
#endif

  return CAPS_SUCCESS;
}

/* aimPreAnalysis: Parse Inputs, Generate Input File(s) & Possibly Tessellate */
int
aimPreAnalysis(int iIndex, void *aimInfo, /*@unused@*/ const char *apath,
               capsValue *aimInputs, capsErrs **errs)
{
  int i, j; // Indexing

  int status; // Status return

  int found;

  double area   = 0;
  double cgxmom = 0;
  double cgymom = 0;
  double cgzmom = 0;
  double cxmom  = 0;
  double cymom  = 0;
  double czmom  = 0;
  double mass   = 0;
  double Ixx    = 0;
  double Ixy    = 0;
  double Izz    = 0;
  double Ixz    = 0;
  double Iyy    = 0;
  double Iyz    = 0;
  double Cx, Cy, Cz, CGx, CGy, CGz;
  double xelem[4], yelem[4], zelem[4], elemArea, thick, density, elemWeight;
  double xcent, ycent, zcent;
  double dx1[3], dx2[3], n[3];

  int elem[4];
  int *n2a = NULL;

  feaMeshDataStruct *feaData = NULL;
  int propertyID, materialID;
  //meshElementSubTypeEnum elementSubType;
  feaPropertyStruct *feaProperty;
  feaMaterialStruct *feaMaterial;

  meshStruct *nasMesh;
  massProperties *massProp;

  // NULL out errors
  *errs = NULL;

  if ((iIndex < 0) || (iIndex >= nInstance)) return CAPS_BADINDEX;

  // Get FEA mesh if we don't already have one
  if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

      status = checkAndCreateMesh(iIndex, aimInfo);
      if (status != CAPS_SUCCESS) goto cleanup;

  }

  // Note: Setting order is important here.
  // 1. Materials should be set before properties.
  // 2. Coordinate system should be set before mesh and loads
  // 3. Mesh should be set before loads, constraints, supports, and connections

  // Set material properties
  if (aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].nullVal == NotNull) {
      status = fea_getMaterial(aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].length,
                               aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].vals.tuple,
                               &masstranInstance[iIndex].feaProblem.numMaterial,
                               &masstranInstance[iIndex].feaProblem.feaMaterial);
      if (status != CAPS_SUCCESS) return status;
  } else printf("Material tuple is NULL - No materials set\n");

  // Set property properties
  if (aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].nullVal == NotNull) {
      status = fea_getProperty(aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].length,
                               aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].vals.tuple,
                               &masstranInstance[iIndex].attrMap,
                               &masstranInstance[iIndex].feaProblem);
      if (status != CAPS_SUCCESS) return status;


      // Assign element "subtypes" based on properties set
      status = fea_assignElementSubType(masstranInstance[iIndex].feaProblem.numProperty,
                                        masstranInstance[iIndex].feaProblem.feaProperty,
                                        &masstranInstance[iIndex].feaProblem.feaMesh);
      if (status != CAPS_SUCCESS) return status;

  } else printf("Property tuple is NULL - No properties set\n");

  nasMesh = &masstranInstance[iIndex].feaProblem.feaMesh;
  massProp = &masstranInstance[iIndex].massProp;

  // maps from nodeID to mesh->node index
  mesh_nodeID2Array(nasMesh, &n2a);

  for (i = 0; i < nasMesh->numElement; i++) {

    // Get the property and material
    if (nasMesh->element[i].analysisType == MeshStructure) {
        feaData = (feaMeshDataStruct *) nasMesh->element[i].analysisData;
        propertyID = feaData->propertyID;
        //coordID = feaData->coordID;
        //elementSubType = feaData->elementSubType;
    } else {
        feaData = NULL;
        propertyID = nasMesh->element[i].markerID;
        //coordID = 0;
        //elementSubType = UnknownMeshSubElement;
    }
    elemWeight = 0;

    found = (int) false;
    for (j = 0; j < masstranInstance[iIndex].feaProblem.numProperty; j++) {
        if (propertyID == masstranInstance[iIndex].feaProblem.feaProperty[j].propertyID) {
            found = (int) true;
            break;
        }
    }

    if (found == (int) false) {
        printf("No property information found for element %d!\n", nasMesh->element[i].elementID);
        continue;
    }

    feaProperty = masstranInstance[iIndex].feaProblem.feaProperty + j;

    if (nasMesh->element[i].elementType == Node ) {

      if (feaData != NULL && feaData->elementSubType == ConcentratedMassElement) {

        elem[0] = n2a[nasMesh->element[i].connectivity[0]];

        xcent = nasMesh->node[elem[0]].xyz[0] + feaProperty->massOffset[0];
        ycent = nasMesh->node[elem[0]].xyz[1] + feaProperty->massOffset[1];
        zcent = nasMesh->node[elem[0]].xyz[2] + feaProperty->massOffset[2];

        elemArea = 0;

        elemWeight = feaProperty->mass;

        // add the inertia at the point
        Ixx    += feaProperty->massInertia[I11];
        Iyy    += feaProperty->massInertia[I22];
        Izz    += feaProperty->massInertia[I33];
        Ixy    -= feaProperty->massInertia[I21];
        Ixz    -= feaProperty->massInertia[I31];
        Iyz    -= feaProperty->massInertia[I32];
      } else {
        continue; // nothing to do if the node is not a concentrated mass
      }

    } else if ( nasMesh->element[i].elementType == Triangle) {

      elem[0] = n2a[nasMesh->element[i].connectivity[0]];
      elem[1] = n2a[nasMesh->element[i].connectivity[1]];
      elem[2] = n2a[nasMesh->element[i].connectivity[2]];

      xelem[0] = nasMesh->node[elem[0]].xyz[0];
      yelem[0] = nasMesh->node[elem[0]].xyz[1];
      zelem[0] = nasMesh->node[elem[0]].xyz[2];
      xelem[1] = nasMesh->node[elem[1]].xyz[0];
      yelem[1] = nasMesh->node[elem[1]].xyz[1];
      zelem[1] = nasMesh->node[elem[1]].xyz[2];
      xelem[2] = nasMesh->node[elem[2]].xyz[0];
      yelem[2] = nasMesh->node[elem[2]].xyz[1];
      zelem[2] = nasMesh->node[elem[2]].xyz[2];

      xcent  = (xelem[0] + xelem[1] + xelem[2]) / 3;
      ycent  = (yelem[0] + yelem[1] + yelem[2]) / 3;
      zcent  = (zelem[0] + zelem[1] + zelem[2]) / 3;

      dx1[0] = xelem[1] - xelem[0];
      dx1[1] = yelem[1] - yelem[0];
      dx1[2] = zelem[1] - zelem[0];
      dx2[0] = xelem[2] - xelem[0];
      dx2[1] = yelem[2] - yelem[0];
      dx2[2] = zelem[2] - zelem[0];

      CROSS(n, dx1, dx2);

      elemArea  = sqrt(DOT(n, n))/2.0;      /* 1/2 for area */

    } else if ( nasMesh->element[i].elementType == Quadrilateral) {

      elem[0] = n2a[nasMesh->element[i].connectivity[0]];
      elem[1] = n2a[nasMesh->element[i].connectivity[1]];
      elem[2] = n2a[nasMesh->element[i].connectivity[2]];
      elem[3] = n2a[nasMesh->element[i].connectivity[3]];

      xelem[0] = nasMesh->node[elem[0]].xyz[0];
      yelem[0] = nasMesh->node[elem[0]].xyz[1];
      zelem[0] = nasMesh->node[elem[0]].xyz[2];
      xelem[1] = nasMesh->node[elem[1]].xyz[0];
      yelem[1] = nasMesh->node[elem[1]].xyz[1];
      zelem[1] = nasMesh->node[elem[1]].xyz[2];
      xelem[2] = nasMesh->node[elem[2]].xyz[0];
      yelem[2] = nasMesh->node[elem[2]].xyz[1];
      zelem[2] = nasMesh->node[elem[2]].xyz[2];
      xelem[3] = nasMesh->node[elem[3]].xyz[0];
      yelem[3] = nasMesh->node[elem[3]].xyz[1];
      zelem[3] = nasMesh->node[elem[3]].xyz[2];

      xcent  = (xelem[0] + xelem[1] + xelem[2] + xelem[3]) / 4;
      ycent  = (yelem[0] + yelem[1] + yelem[2] + yelem[3]) / 4;
      zcent  = (zelem[0] + zelem[1] + zelem[2] + zelem[3]) / 4;

      dx1[0] = xelem[2] - xelem[0];
      dx1[1] = yelem[2] - yelem[0];
      dx1[2] = zelem[2] - zelem[0];
      dx2[0] = xelem[3] - xelem[1];
      dx2[1] = yelem[3] - yelem[1];
      dx2[2] = zelem[3] - zelem[1];

      CROSS(n, dx1, dx2);

      elemArea  = sqrt(DOT(n, n))/2.0;      /* 1/2 for area */

    } else {
      printf("Error: Unknown element type %d\n", nasMesh->element[i].elementType);
      return CAPS_BADVALUE;
    }

    if (feaData == NULL || feaData->elementSubType != ConcentratedMassElement) {

      materialID = feaProperty->materialID;

      for (j = 0; j < masstranInstance[iIndex].feaProblem.numMaterial; j++) {
        if (materialID == masstranInstance[iIndex].feaProblem.feaMaterial[j].materialID) {
          break;
        }
      }

      if (found == (int) false) {
        printf("No material information found for element %d!\n", nasMesh->element[i].elementID);
        continue;
      }

      feaMaterial = masstranInstance[iIndex].feaProblem.feaMaterial + j;

      density = 1;
      if (feaMaterial->density > 0)
        density = feaMaterial->density;

      thick = 1;
      if (feaProperty->membraneThickness > 0) {
        thick   = feaProperty->membraneThickness;
      }

      //TODO: Should massPerArea override density?
      if (feaProperty->massPerArea > 0) {
        density = feaProperty->massPerArea;
        thick = 1;
      }

      elemWeight = elemArea * density * thick;
    }

    area   += elemArea;
    mass   += elemWeight;

    cxmom  += xcent * elemArea;
    cymom  += ycent * elemArea;
    czmom  += zcent * elemArea;

    cgxmom += xcent * elemWeight;
    cgymom += ycent * elemWeight;
    cgzmom += zcent * elemWeight;

    Ixx    += (ycent * ycent + zcent * zcent) * elemWeight;
    Iyy    += (xcent * xcent + zcent * zcent) * elemWeight;
    Izz    += (xcent * xcent + ycent * ycent) * elemWeight;
    Ixy    += (xcent         * ycent        ) * elemWeight;
    Ixz    += (xcent         * zcent        ) * elemWeight;
    Iyz    += (ycent         * zcent        ) * elemWeight;
  }

  /* compute statistics for whole Body */
  Cx  = cxmom / area;
  Cy  = cymom / area;
  Cz  = czmom / area;

  CGx = cgxmom / mass;
  CGy = cgymom / mass;
  CGz = cgzmom / mass;

  Ixx -= mass * (CGy * CGy + CGz * CGz);
  Iyy -= mass * (CGx * CGx + CGz * CGz);
  Izz -= mass * (CGx * CGx + CGy * CGy);
  Ixy -= mass *  CGx * CGy;
  Ixz -= mass *  CGx * CGz;
  Iyz -= mass *  CGy * CGz;

  // store the mass properties
  massProp->area = area;
  massProp->mass = mass;

  massProp->Cx = Cx;
  massProp->Cy = Cy;
  massProp->Cz = Cz;

  massProp->CGx = CGx;
  massProp->CGy = CGy;
  massProp->CGz = CGz;

  massProp->Ixx = Ixx;
  massProp->Iyy = Iyy;
  massProp->Izz = Izz;
  massProp->Ixy = Ixy;
  massProp->Ixz = Ixz;
  massProp->Iyz = Iyz;

  status = CAPS_SUCCESS;

  cleanup:
      EG_free(n2a);

      if (status != CAPS_SUCCESS) printf("Error: Status %d during masstranAIM preAnalysis\n", status);

      return status;
}

/* aimOutputs: Output Information for the AIM */
int
aimOutputs(int iIndex, /*@unused@*/ void *aimInfo, /*@unused@*/ int index,
           char **aoname, capsValue *form)
{
#ifdef DEBUG
  printf(" masstranAIM/aimOutputs instance = %d  index  = %d!\n", iIndex, index);
#endif
  if ((iIndex < 0) || (iIndex >= nInstance)) return CAPS_BADINDEX;

  /*! \page aimOutputsMasstran AIM Outputs
   * The following list outlines the Masstran outputs available through the AIM interface.
   */
  /*! \page aimOutputsMasstran AIM Outputs
   * - <B>Area</B> = Total area of the mesh.
   * - <B>Mass</B> = Total mass of the model.
   * - <B>Centroid</B> = Centroid of the model.
   * - <B>CG</B> = Center of gravity of the model.
   * - <B>Ixx</B> = Moment of inertia
   * - <B>Iyy</B> = Moment of inertia
   * - <B>Izz</B> = Moment of inertia
   * - <B>Ixy</B> = Moment of inertia
   * - <B>Izy</B> = Moment of inertia
   * - <B>Iyz</B> = Moment of inertia
   * - <B>I_Vector</B> = Moment of inertia vector
   *  \f[
   *    \vec{I} = \begin{bmatrix} I_{xx} & I_{yy} & I_{zz} & I_{xy} & I_{xz} & I_{yz} \end{bmatrix}
   *  \f]
   * - <B>I_Lower</B> = Moment of inertia lower triangular tensor
   * \f[
   *  \vec{I}_{lower} = \begin{bmatrix} I_{xx} & -I_{xy} & I_{yy} & -I_{xz} & -I_{yz} & I_{zz} \end{bmatrix},
   * \f]
   * - <B>I_Upper</B> = Moment of inertia upper triangular tensor
   * \f[
   *  \vec{I}_{upper} = \begin{bmatrix} I_{xx} & -I_{xy} & -I_{xz} & I_{yy} & -I_{yz} & I_{zz} \end{bmatrix},
   * \f]
   * - <B>I_Tensor</B> = Moment of inertia tensor
   * \f[
   * \bar{\bar{I}} =
   *  \begin{bmatrix}
   *    I_{xx} & -I_{xy} & -I_{xz} \\
   *   -I_{xy} &  I_{yy} & -I_{yz} \\
   *   -I_{xz} & -I_{yz} &  I_{zz}
   *  \end{bmatrix}
   * \f]
   */

  if (index == 1) {
      *aoname = EG_strdup("Area");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 2) {
      *aoname = EG_strdup("Mass");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 3) {
      *aoname = EG_strdup("Centroid");
      form->length = 3;
      form->dim    = Vector;

  } else if (index == 4) {
      *aoname = EG_strdup("CG");
      form->length = 3;
      form->dim    = Vector;

  } else if (index == 5) {
      *aoname = EG_strdup("Ixx");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 6) {
      *aoname = EG_strdup("Iyy");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 7) {
      *aoname = EG_strdup("Izz");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 8) {
      *aoname = EG_strdup("Ixy");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 9) {
      *aoname = EG_strdup("Ixz");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 10) {
      *aoname = EG_strdup("Iyz");
      form->length = 1;
      form->dim    = Scalar;

  } else if (index == 11) {
      *aoname = EG_strdup("I_Vector");
      form->length = 6;
      form->dim    = Vector;

  } else if (index == 12) {
      *aoname = EG_strdup("I_Lower");
      form->length = 6;
      form->dim    = Vector;

  } else if (index == 13) {
      *aoname = EG_strdup("I_Upper");
      form->length = 6;
      form->dim    = Vector;

  } else if (index == 14) {
      *aoname = EG_strdup("I_Tensor");
      form->length = 9;
      form->dim    = Array2D;

  }

#if NUMOUTPUT != 14
#error "NUMOUTPUT is inconsistent with the list of inputs"
#endif

  if (*aoname == NULL) return EGADS_MALLOC;

  form->type       = Double;
  form->units      = NULL;
  form->nrow       = form->length;
  form->lfixed     = Change;
  form->sfixed     = Change;
  form->vals.reals = NULL;
  form->vals.real  = 0;

  if (index == 14) {
      form->nrow     = 3;
      form->ncol     = 3;
  }

  return CAPS_SUCCESS;
}

/* aimCalcOutput: Calculate/Retrieve Output Information */
int
aimCalcOutput(int iIndex, /*@unused@*/ void *aimInfo, /*@unused@*/ const char *ap,
              /*@unused@*/ int index, capsValue *val, capsErrs **errors)
{
  double Ixx = 0;
  double Ixy = 0;
  double Izz = 0;
  double Ixz = 0;
  double Iyy = 0;
  double Iyz = 0;
  double *I;

#ifdef DEBUG
  int        status;
  const char *name;

  status = aim_getName(aimInfo, index, ANALYSISOUT, &name);
  printf(" masstranAIM/aimCalcOutput instance = %d  index = %d %s %d!\n",
         iIndex, index, name, status);
#endif

  *errors = NULL;
  if ((iIndex < 0) || (iIndex >= nInstance)) return CAPS_BADINDEX;

  if (val->length > 1) {
    val->vals.reals = (double*)EG_alloc(val->length*sizeof(double));
    if (val->vals.reals == NULL) return EGADS_MALLOC;
  }

  if (index == 1) {
    val->vals.real = masstranInstance[iIndex].massProp.area;

  } else if (index == 2) {
    val->vals.real = masstranInstance[iIndex].massProp.mass;

  } else if (index == 3) {
    val->vals.reals[0] = masstranInstance[iIndex].massProp.Cx;
    val->vals.reals[1] = masstranInstance[iIndex].massProp.Cy;
    val->vals.reals[2] = masstranInstance[iIndex].massProp.Cz;

  } else if (index == 4) {
    val->vals.reals[0] = masstranInstance[iIndex].massProp.CGx;
    val->vals.reals[1] = masstranInstance[iIndex].massProp.CGy;
    val->vals.reals[2] = masstranInstance[iIndex].massProp.CGz;

  } else if (index == 5) {
    val->vals.real = masstranInstance[iIndex].massProp.Ixx;

  } else if (index == 6) {
    val->vals.real = masstranInstance[iIndex].massProp.Iyy;

  } else if (index == 7) {
    val->vals.real = masstranInstance[iIndex].massProp.Izz;

  } else if (index == 8) {
    val->vals.real = masstranInstance[iIndex].massProp.Ixy;

  } else if (index == 9) {
    val->vals.real = masstranInstance[iIndex].massProp.Ixz;

  } else if (index == 10) {
    val->vals.real = masstranInstance[iIndex].massProp.Iyz;

  } else if (index == 11) {
    val->vals.reals[0] = masstranInstance[iIndex].massProp.Ixx;
    val->vals.reals[1] = masstranInstance[iIndex].massProp.Iyy;
    val->vals.reals[2] = masstranInstance[iIndex].massProp.Izz;
    val->vals.reals[3] = masstranInstance[iIndex].massProp.Ixy;
    val->vals.reals[4] = masstranInstance[iIndex].massProp.Ixz;
    val->vals.reals[5] = masstranInstance[iIndex].massProp.Iyz;

  } else if (index == 12) {
    val->vals.reals[0] =  masstranInstance[iIndex].massProp.Ixx;
    val->vals.reals[1] = -masstranInstance[iIndex].massProp.Ixy;
    val->vals.reals[2] =  masstranInstance[iIndex].massProp.Iyy;
    val->vals.reals[3] = -masstranInstance[iIndex].massProp.Ixz;
    val->vals.reals[4] = -masstranInstance[iIndex].massProp.Iyz;
    val->vals.reals[5] =  masstranInstance[iIndex].massProp.Izz;

  } else if (index == 13) {
    val->vals.reals[0] =  masstranInstance[iIndex].massProp.Ixx;
    val->vals.reals[1] = -masstranInstance[iIndex].massProp.Ixy;
    val->vals.reals[2] = -masstranInstance[iIndex].massProp.Ixz;
    val->vals.reals[3] =  masstranInstance[iIndex].massProp.Iyy;
    val->vals.reals[4] = -masstranInstance[iIndex].massProp.Iyz;
    val->vals.reals[5] =  masstranInstance[iIndex].massProp.Izz;

  } else if (index == 14) {

    Ixx = masstranInstance[iIndex].massProp.Ixx;
    Iyy = masstranInstance[iIndex].massProp.Iyy;
    Iyz = masstranInstance[iIndex].massProp.Iyz;
    Ixy = masstranInstance[iIndex].massProp.Ixy;
    Ixz = masstranInstance[iIndex].massProp.Ixz;
    Izz = masstranInstance[iIndex].massProp.Izz;
    I = val->vals.reals;

    // populate the tensor
    I[0] =  Ixx; I[1] = -Ixy; I[2] = -Ixz;
    I[3] = -Ixy; I[4] =  Iyy; I[5] = -Iyz;
    I[6] = -Ixz; I[7] = -Iyz; I[8] =  Izz;
  }


  return CAPS_SUCCESS;
}


/* aimCleanup: Free up the AIMs storage */
void aimCleanup()
{
  int iIndex;
#ifdef DEBUG
  printf(" masstranAIM/aimCleanup!\n");
#endif

  if (nInstance != 0) {
    /* clean up all allocated data
     */
    for (iIndex = 0; iIndex < nInstance; iIndex++)
      destroy_aimStorage(iIndex);

    EG_free(masstranInstance);
  }
  nInstance = 0;
  masstranInstance  = NULL;
}
