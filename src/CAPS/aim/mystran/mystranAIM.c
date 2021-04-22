/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             mystran AIM
 *
 *     Written by Dr. Ryan Durscher AFRL/RQVC
 *
 */


/*! \mainpage Introduction
 * \tableofcontents
 * \section overviewMYSTRAN MYSTRAN AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact (primarily
 * through input files) with the finite element structural solver MYSTRAN \cite MYSTRAN. MYSTRAN is an open source,
 * general purpose, linear finite element analysis computer program written by Dr. Bill Case. Available at,
 * http://www.mystran.com/ , MYSTRAN currently supports Linux and Windows operating systems.
 *
 *
 * An outline of the AIM's inputs, outputs and attributes are provided in \ref aimInputsMYSTRAN and
 * \ref aimOutputsMYSTRAN and \ref attributeMYSTRAN, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentMYSTRAN.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataMYSTRAN if
 * connecting this AIM to other AIMs in a parent-child like manner.
 *
 * Details of the AIM's automated data transfer capabilities are outlined in \ref dataTransferMYSTRAN
 *
 *\section mystranExamples Examples
 * An example problem using the MYSTRAN AIM may be found at \ref mystranExample.
 */


/*! \page attributeMYSTRAN MYSTRAN AIM attributes
 * The following list of attributes are required for the MYSTRAN AIM inside the geometry input.
 *
 * - <b> capsAIM</b> This attribute is a CAPS requirement to indicate the analysis the geometry
 * representation supports.
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
 * - <b> capsIgnore</b> It is possible that there is a geometric body (or entity) that you do not want the MYSTRAN AIM to pay attention to when creating
 * a finite element model. The capsIgnore attribute allows a body (or entity) to be in the geometry and ignored by the AIM.  For example,
 * because of limitations in OpenCASCADE a situation where two edges are overlapping may occur; capsIgnore allows the user to only
 *  pay attention to one of the overlapping edges.
 *
 * - <b> capsBound </b> This is used to mark surfaces on the structural grid in which data transfer with an external
 * solver will take place. See \ref dataTransferMYSTRAN for additional details.
 *
 *
 */

// No connections yet
/* - <b> capsConnect</b> This is a name assigned to any geometric body where the user wishes to create
 * "fictitious" connections such as springs, dampers, and/or rigid body connections to. The user must manually specify
 * the connection between two <c>capsConnect</c> entities using the "Connect" tuple (see \ref aimInputsMYSTRAN).
 * Recall that a string in ESP starts with a $.  For example, attribute <c>capsConnect $springStart</c>.
 *
 * - <b> capsConnectLink</b> Similar to <c>capsConnect</c>, this is a name assigned to any geometric body where
 * the user wishes to create "fictitious" connections to. A connection is automatically made if a <c>capsConnectLink</c>
 * matches a <c>capsConnect</c> group. Again further specifics of the connection are input using the "Connect"
 * tuple (see \ref aimInputsMYSTRAN). Recall that a string in ESP starts with a $.
 * For example, attribute <c>capsConnectLink $springEnd</c>.
 */

#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"
#include "miscUtils.h"
#include "feaUtils.h"
#include "nastranUtils.h"
#include "mystranUtils.h"

#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define NUMINPUT   12
#define NUMOUTPUT  4

//#define DEBUG

typedef struct {
    // Project name
    char *projectName;

    // Analysis file path/directory
    const char *analysisPath;

    feaProblemStruct feaProblem;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

    // Attribute to constraint index map
    mapAttrToIndexStruct constraintMap;

    // Attribute to load index map
    mapAttrToIndexStruct loadMap;

    /*
    // Check to make sure data transfer is ok
    int dataTransferCheck;
     */

    // Mesh holders
    int numMesh;
    meshStruct *feaMesh;

} aimStorage;

static aimStorage *mystranInstance = NULL;
static int         numInstance  = 0;


static int initiate_aimStorage(int iIndex) {

    int status;

    // Set initial values for mystranInstance
    mystranInstance[iIndex].projectName = NULL;

    // Analysis file path/directory
    mystranInstance[iIndex].analysisPath = NULL;

    /*
    // Check to make sure data transfer is ok
    mystranInstance[iIndex].dataTransferCheck = (int) true;
     */

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&mystranInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to constraint index map
    status = initiate_mapAttrToIndexStruct(&mystranInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to load index map
    status = initiate_mapAttrToIndexStruct(&mystranInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_feaProblemStruct(&mystranInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS) return status;

    // Mesh holders
    mystranInstance[iIndex].numMesh = 0;
    mystranInstance[iIndex].feaMesh = NULL;

    return CAPS_SUCCESS;
}

static int destroy_aimStorage(int iIndex) {

    int status;
    int i;
    // Attribute to index map
    status = destroy_mapAttrToIndexStruct(&mystranInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to constraint index map
    status = destroy_mapAttrToIndexStruct(&mystranInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to load index map
    status = destroy_mapAttrToIndexStruct(&mystranInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Cleanup meshes
    if (mystranInstance[iIndex].feaMesh != NULL) {

        for (i = 0; i < mystranInstance[iIndex].numMesh; i++) {
            status = destroy_meshStruct(&mystranInstance[iIndex].feaMesh[i]);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_meshStruct!\n", status);
        }

        EG_free(mystranInstance[iIndex].feaMesh);
    }

    mystranInstance[iIndex].feaMesh = NULL;
    mystranInstance[iIndex].numMesh = 0;

    // Destroy FEA problem structure
    status = destroy_feaProblemStruct(&mystranInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS)  printf("Error: Status %d during destroy_feaProblemStruct!\n", status);

    // NULL projetName
    mystranInstance[iIndex].projectName = NULL;

    // NULL analysisPath
    mystranInstance[iIndex].analysisPath = NULL;

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

  // Attribute to transfer map
  mapAttrToIndexStruct transferMap;

  // Attribute to connect map
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

  status = initiate_mapAttrToIndexStruct(&transferMap);
  if (status != CAPS_SUCCESS) return status;

  status = initiate_mapAttrToIndexStruct(&connectMap);
  if (status != CAPS_SUCCESS) return status;

  status = fea_createMesh(aimInfo,
                          tessParam,
                          edgePointMin,
                          edgePointMax,
                          quadMesh,
                          &mystranInstance[iIndex].attrMap,
                          &mystranInstance[iIndex].constraintMap,
                          &mystranInstance[iIndex].loadMap,
                          &transferMap,
                          &connectMap,
                          &mystranInstance[iIndex].numMesh,
                          &mystranInstance[iIndex].feaMesh,
                          &mystranInstance[iIndex].feaProblem );
  if (status != CAPS_SUCCESS) return status;

  status = destroy_mapAttrToIndexStruct(&transferMap);
  if (status != CAPS_SUCCESS) return status;

  status = destroy_mapAttrToIndexStruct(&connectMap);
  if (status != CAPS_SUCCESS) return status;

  return CAPS_SUCCESS;
}


/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(int ngIn, capsValue *gIn, int *qeFlag, const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int status; // Return flag
    int flag;
    int *ints;
    char **strs;

    aimStorage *tmp;

#ifdef DEBUG
    printf("\n mystranAIM/aimInitialize   ngIn = %d!\n", ngIn);
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

    strs = (char **) EG_alloc(*nFields*sizeof(char *));
    if (strs == NULL) {
        EG_free(*ranks);
        *ranks   = NULL;
        return EGADS_MALLOC;
    }

    strs[0]  = EG_strdup("Displacement");
    strs[1]  = EG_strdup("EigenVector");
    strs[2]  = EG_strdup("EigenVector_*");
    *fnames  = strs;

    // Allocate mystranInstance
    if (mystranInstance == NULL) {
        mystranInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (mystranInstance == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }
    } else {
        tmp = (aimStorage *) EG_reall(mystranInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }
        mystranInstance = tmp;
    }

    // Initialize instance storage
    status = initiate_aimStorage(numInstance);
    if (status != CAPS_SUCCESS) return status;

    // Increment number of instances
    numInstance += 1;

    return (numInstance -1);
}

int aimInputs(int iIndex, void *aimInfo, int index, char **ainame,
              capsValue *defval)
{
    /*! \page aimInputsMYSTRAN AIM Inputs
     * The following list outlines the MYSTRAN inputs along with their default value available
     * through the AIM interface. Unless noted these values will be not be linked to
     * any parent AIMs with variables of the same name.
     */


#ifdef DEBUG
    printf(" mystranAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
#endif

    *ainame = NULL;

    // MYSTRAN Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("mystran_CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsMYSTRAN
         * - <B> Proj_Name = "mystran_CAPS"</B> <br>
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

        /*! \page aimInputsMYSTRAN
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

        /*! \page aimInputsMYSTRAN
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

        /*! \page aimInputsMYSTRAN
         * - <B> Edge_Point_Max = 50</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 5) {
        *ainame               = EG_strdup("Quad_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = (int) false;

        /*! \page aimInputsMYSTRAN
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

        /*! \page aimInputsMYSTRAN
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

        /*! \page aimInputsMYSTRAN
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

        /*! \page aimInputsMYSTRAN
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

        /*! \page aimInputsMYSTRAN
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

        /*! \page aimInputsMYSTRAN
         * - <B> Analysis = NULL</B> <br>
         * Analysis tuple used to input analysis/case information for the model, see \ref feaAnalysis for additional details.
         */
    } else if (index == 11) {
        *ainame              = EG_strdup("Analysis_Type");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Modal");
        defval->lfixed       = Change;

        /*! \page aimInputsMYSTRAN
         * - <B> Analysis_Type = "Modal"</B> <br>
         * Type of analysis to generate files for, options include "Modal", "Static", and "Craig-Bampton".
         */
    } else if (index == 12) {
        *ainame              = EG_strdup("Support");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsMYSTRAN
         * - <B> Support = NULL</B> <br>
         * Support tuple used to input support information for the model, see \ref feaSupport for additional details.
         */
    }

    // Link variable(s) to parent(s) if available
    /*if ((index != 1) && (*ainame != NULL) && index < 6 ) {
		status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
  	 	printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n",
                        status, index, defval->type, defval->link);
	}*/

    return CAPS_SUCCESS;
}

int aimData(int iIndex, const char *name, enum capsvType *vtype, int *rank, int *nrow,
            int *ncol, void **data, char **units)
{

    /*! \page sharableDataMYSTRAN AIM Shareable Data
     * Currently the MYSTRAN AIM does not have any shareable data types or values. It will try, however, to inherit a "FEA_MESH"
     * or "Volume_Mesh" from any parent AIMs. Note that the inheritance of the mesh is not required.
     */

#ifdef DEBUG
    printf(" mystranAIM/aimData instance = %d  name = %s!\n", iIndex, name);
#endif
    return CAPS_NOTFOUND;
}


int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath,
                   capsValue *aimInputs, capsErrs **errs)
{

    int i, j, k; // Indexing

    int status; // Status return

    int found; // Boolean operator

    int *tempIntegerArray = NULL; // Temporary array to store a list of integers

    // Analyis information
    char *analysisType = NULL;

    // File IO
    char *filename = NULL; // Output file name
    FILE *fp = NULL; // Output file pointer

    // NULL out errors
    *errs = NULL;

    // Store away the analysis path/directory
    mystranInstance[iIndex].analysisPath = analysisPath;

    // Get project name
    mystranInstance[iIndex].projectName = aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string;

    // Check and generate/retrieve the mesh
    status = checkAndCreateMesh(iIndex, aimInfo);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Note: Setting order is important here.
    // 1. Materials should be set before properties.
    // 2. Coordinate system should be set before mesh and loads
    // 3. Mesh should be set before loads and constraints
    // 4. Constraints and loads should be set before analysis

    // Set material properties
    if (aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getMaterial(aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].vals.tuple,
                                 &mystranInstance[iIndex].feaProblem.numMaterial,
                                 &mystranInstance[iIndex].feaProblem.feaMaterial);
        if (status != CAPS_SUCCESS) goto cleanup;
    } else printf("\nLoad tuple is NULL - No materials set\n");

    // Set property properties
    if (aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getProperty(aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].vals.tuple,
                                 &mystranInstance[iIndex].attrMap,
                                 &mystranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) goto cleanup;
    } else printf("\nProperty tuple is NULL - No properties set\n");

    // Set constraint properties
    if (aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getConstraint(aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].length,
                                   aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].vals.tuple,
                                   &mystranInstance[iIndex].constraintMap,
                                   &mystranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) goto cleanup;
    } else printf("\nConstraint tuple is NULL - No constraints applied\n");

    // Set support properties
    if (aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getSupport(aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "Support", ANALYSISIN)-1].vals.tuple,
                                &mystranInstance[iIndex].constraintMap,
                                &mystranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Support tuple is NULL - No supports applied\n");

    // Set load properties
    if (aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getLoad(aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].length,
                             aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].vals.tuple,
                             &mystranInstance[iIndex].loadMap,
                             &mystranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Loop through loads to see if any of them are supposed to be from an external source
        for (i = 0; i < mystranInstance[iIndex].feaProblem.numLoad; i++) {

            if (mystranInstance[iIndex].feaProblem.feaLoad[i].loadType == PressureExternal) {

                // Transfer external pressures from the AIM discrObj
                status = fea_transferExternalPressure(aimInfo,
                                                      &mystranInstance[iIndex].feaProblem.feaMesh,
                                                      &mystranInstance[iIndex].feaProblem.feaLoad[i]);
                if (status != CAPS_SUCCESS) goto cleanup;
            }
        }
    } else printf("\nLoad tuple is NULL - No loads applied\n");

    // Set analysis settings
    if (aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getAnalysis(aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Analysis", ANALYSISIN)-1].vals.tuple,
                                 &mystranInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) goto cleanup; // It ok to not have an analysis tuple
    } else printf("\nAnalysis tuple is NULL\n");

    // Write Nastran Mesh
    filename = (char *) EG_alloc((PATH_MAX +1)*sizeof(char));
    if (filename == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }
    strcpy(filename, analysisPath);
#ifdef WIN32
    strcat(filename, "\\");
#else
    strcat(filename, "/");
#endif
    strcat(filename, mystranInstance[iIndex].projectName);

    status = mesh_writeNASTRAN(filename,
                               1,
                               &mystranInstance[iIndex].feaProblem.feaMesh,
                               mystranInstance[iIndex].feaProblem.feaFileFormat.gridFileType,
                               1.0);
    if (status != CAPS_SUCCESS) goto cleanup;

    if (filename != NULL) EG_free(filename);

    // Write mystran input file
    filename = (char *) EG_alloc((PATH_MAX +1)*sizeof(char));
    if (filename == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    strcpy(filename, analysisPath);
#ifdef WIN32
    strcat(filename, "\\");
#else
    strcat(filename, "/");
#endif
    strcat(filename, mystranInstance[iIndex].projectName);
    strcat(filename, ".dat");

    printf("\nWriting MYSTRAN instruction file....\n");
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        status = CAPS_IOERR;
        goto cleanup;
    }
    if (filename != NULL) EG_free(filename);
    filename = NULL;

    //////////////// Executive control ////////////////
    fprintf(fp, "ID CAPS generated Problem FOR MYSTRAN\n");

    // Analysis type
    analysisType = aimInputs[aim_getIndex(aimInfo, "Analysis_Type", ANALYSISIN)-1].vals.string;

    if     (strcasecmp(analysisType, "Modal")         == 0) fprintf(fp, "SOL 3\n");
    else if(strcasecmp(analysisType, "Static")        == 0) fprintf(fp, "SOL 1\n");
    else if(strcasecmp(analysisType, "Craig-Bampton") == 0) fprintf(fp, "SOL 31\n");
    else {
        printf("Unrecognized \"Analysis_Type\", %s, defaulting to \"Modal\" analysis\n", analysisType);
        analysisType = "Modal";
        fprintf(fp, "SOL 3\n");
    }

    if (strcasecmp(analysisType, "Modal") == 0) fprintf(fp, "OUTPUT4 EIGEN_VAL, EIGEN_VEC, GEN_MASS, , // -1/21 $\n"); // Binary output of eigenvalues and vectors

    fprintf(fp, "CEND\n\n");

    //////////////// Case control ////////////////

    // Write output request information
    fprintf(fp, "DISP = ALL\n"); // Output all displacements

    if(strcasecmp(analysisType, "Static") == 0) fprintf(fp, "STRE = ALL\n"); // Output all stress

    if(strcasecmp(analysisType, "Static") == 0) fprintf(fp, "STRA = ALL\n"); // Output all strain

    //fprintf(fp, "ELDATA(6,PRINT) = ALL\n");

    // Check thermal load - currently only a single thermal load is supported - also it only works for the global level - no subcase
    found = (int) false;
    for (i = 0; i < mystranInstance[iIndex].feaProblem.numLoad; i++) {

        if (mystranInstance[iIndex].feaProblem.feaLoad[i].loadType != Thermal) continue;

        if (found == (int) true) {
            printf("More than 1 Thermal load found - mystranAIM does NOT currently doesn't support multiple thermal loads!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        found = (int) true;

        fprintf(fp, "TEMPERATURE = %d\n", mystranInstance[iIndex].feaProblem.feaLoad[i].loadID);
    }

    // Write constraint information
    if (mystranInstance[iIndex].feaProblem.numConstraint != 0) {
        fprintf(fp, "SPC = %d\n", mystranInstance[iIndex].feaProblem.numConstraint+1);
    } else {
        printf("Warning: No constraints specified for job!!!!\n");
    }

    // Modal analysis - only
    // If modal - we are only going to use the first analysis structure we come across that has its type as modal
    if (strcasecmp(analysisType, "Modal") == 0) {

        // Look through analysis structures for a modal one
        found = (int) false;
        for (i = 0; i < mystranInstance[iIndex].feaProblem.numAnalysis; i++) {
            if (mystranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Modal) {
                found = (int) true;
                break;
            }
        }

        // Write out analysis ID if a modal analysis structure was found
        if (found == (int) true)  {
            fprintf(fp, "METHOD = %d\n", mystranInstance[iIndex].feaProblem.feaAnalysis[i].analysisID);
        } else {
            printf("Warning: No eigenvalue analysis information specified in \"Analysis\" tuple, though "
                    "AIM input \"Analysis_Type\" is set to \"Modal\"!!!!\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

    }
    // Static analysis - only
    if (strcasecmp(analysisType, "Static") == 0) {

        // If we have multiple analysis structures
        if (mystranInstance[iIndex].feaProblem.numAnalysis != 0) {

            // Write subcase information if multiple analysis tuples were provide
            for (i = 0; i < mystranInstance[iIndex].feaProblem.numAnalysis; i++) {

                if (mystranInstance[iIndex].feaProblem.feaAnalysis[i].analysisType == Static) {
                    fprintf(fp, "SUBCASE %d\n", i);
                    fprintf(fp, "\tLABEL %s\n", mystranInstance[iIndex].feaProblem.feaAnalysis[i].name);

                    // Write loads for sub-case
                    if (mystranInstance[iIndex].feaProblem.numLoad != 0) {
                        fprintf(fp, "\tLOAD = %d\n", mystranInstance[iIndex].feaProblem.numLoad+i+1);
                    }

                    // Issue some warnings regarding loads if necessary
                    if (mystranInstance[iIndex].feaProblem.feaAnalysis[i].numLoad == 0 && mystranInstance[iIndex].feaProblem.numLoad !=0) {
                        printf("Warning: No loads specified for static case %s, assuming "
                                "all loads are applied!!!!\n", mystranInstance[iIndex].feaProblem.feaAnalysis[i].name);
                    } else if (mystranInstance[iIndex].feaProblem.numLoad == 0) {
                        printf("Warning: No loads specified for static case %s!!!!\n", mystranInstance[iIndex].feaProblem.feaAnalysis[i].name);
                    }
                }
            }

        } else { // // If no sub-cases

            if (mystranInstance[iIndex].feaProblem.numLoad != 0) {
                fprintf(fp, "LOAD = %d\n", mystranInstance[iIndex].feaProblem.numLoad+1);
            } else {
                printf("Warning: No loads specified for static a job!!!!\n");
            }
        }
    }

    //////////////// Bulk data ////////////////
    fprintf(fp, "\nBEGIN BULK\n");

    // Turn off auto SPC
    //fprintf(fp, "%-8s %7s %7s\n", "PARAM", "AUTOSPC", "N");

    //fprintf(fp, "%-8s %7s %7d\n", "PARAM", "PRTOU4", 1);
    //fprintf(fp, "%-8s %7s %7d\n", "PARAM", "PRTDLR", 1);

    //fprintf(fp, "%-8s %7s %7d\n", "PARAM", "PRTDOF", 1);

    //fprintf(fp, "%-8s %7d %7d\n", "DEBUG", 183, 1);
    // Turn on FEMAP
    //fprintf(fp, "%-8s %7s %7d\n", "PARAM", "POST", -1);

    // Analysis Cards - Eigenvalue included, as well as combined load
    if (mystranInstance[iIndex].feaProblem.numAnalysis != 0) {

        for (i = 0; i < mystranInstance[iIndex].feaProblem.numAnalysis; i++) {

            status = nastran_writeAnalysisCard(fp, &mystranInstance[iIndex].feaProblem.feaAnalysis[i], &mystranInstance[iIndex].feaProblem.feaFileFormat);
            if (status != CAPS_SUCCESS) goto cleanup;

            if (mystranInstance[iIndex].feaProblem.feaAnalysis[i].numLoad != 0) {

                status = nastran_writeLoadADDCard(fp,
                                                  mystranInstance[iIndex].feaProblem.numLoad+i+1,
                                                  mystranInstance[iIndex].feaProblem.feaAnalysis[i].numLoad,
                                                  mystranInstance[iIndex].feaProblem.feaAnalysis[i].loadSetID,
                                                  mystranInstance[iIndex].feaProblem.feaLoad,
                                                  &mystranInstance[iIndex].feaProblem.feaFileFormat);
                if (status != CAPS_SUCCESS) goto cleanup;

            } else { // If no loads for an individual analysis are specified assume that all loads should be applied

                if (mystranInstance[iIndex].feaProblem.numLoad != 0) {

                    // Ignore thermal loads
                    k = 0;
                    for (j = 0; j < mystranInstance[iIndex].feaProblem.numLoad; j++) {
                        if (mystranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                        k += 1;
                    }

                    if (k != 0) {
                        // Create a temporary list of load IDs
                        tempIntegerArray = (int *) EG_alloc(k*sizeof(int));
                        if (tempIntegerArray == NULL) {
                            status = EGADS_MALLOC;
                            goto cleanup;
                        }

                        k = 0;
                        for (j = 0; j < mystranInstance[iIndex].feaProblem.numLoad; j++) {

                            if (mystranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                            tempIntegerArray[k] = mystranInstance[iIndex].feaProblem.feaLoad[j].loadID;

                            k += 1;
                        }

                        // Write combined load card
                        status = nastran_writeLoadADDCard(fp,
                                                          mystranInstance[iIndex].feaProblem.numLoad+i+1,
                                                          k,
                                                          tempIntegerArray,
                                                          mystranInstance[iIndex].feaProblem.feaLoad,
                                                          &mystranInstance[iIndex].feaProblem.feaFileFormat);
                        if (status != CAPS_SUCCESS) goto cleanup;

                        // Free temporary load ID list
                        if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
                        tempIntegerArray = NULL;
                    }
                }
            }
        }

    } else { // If there aren't any analysis structures just write a single combined load card

        // Combined loads
        if (mystranInstance[iIndex].feaProblem.numLoad != 0) {

            // Ignore thermal loads
            k = 0;
            for (j = 0; j < mystranInstance[iIndex].feaProblem.numLoad; j++) {
                if (mystranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                k += 1;
            }

            if (k != 0) {
                // Create a temporary list of load IDs
                tempIntegerArray = (int *) EG_alloc(k*sizeof(int));
                if (tempIntegerArray == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                k = 0;
                for (j = 0; j < mystranInstance[iIndex].feaProblem.numLoad; j++) {

                    if (mystranInstance[iIndex].feaProblem.feaLoad[j].loadType == Thermal) continue;
                    tempIntegerArray[k] = mystranInstance[iIndex].feaProblem.feaLoad[j].loadID;

                    k += 1;
                }

                // Write combined load card
                status = nastran_writeLoadADDCard(fp,
                                                  mystranInstance[iIndex].feaProblem.numLoad+1,
                                                  k,
                                                  tempIntegerArray,
                                                  mystranInstance[iIndex].feaProblem.feaLoad,
                                                  &mystranInstance[iIndex].feaProblem.feaFileFormat);

                if (status != CAPS_SUCCESS) goto cleanup;

                // Free temporary load ID list
                if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
                tempIntegerArray = NULL;
            }
        }
    }

    // Combined constraints
    if (mystranInstance[iIndex].feaProblem.numConstraint != 0) {

        // Create a temporary list of constraint IDs
        tempIntegerArray = (int *) EG_alloc(mystranInstance[iIndex].feaProblem.numConstraint*sizeof(int));
        if (tempIntegerArray == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (i = 0; i < mystranInstance[iIndex].feaProblem.numConstraint; i++) {
            tempIntegerArray[i] = mystranInstance[iIndex].feaProblem.feaConstraint[i].constraintID;
        }

        // Write combined constraint card
        status = nastran_writeConstraintADDCard(fp,
                                                mystranInstance[iIndex].feaProblem.numConstraint+1,
                                                mystranInstance[iIndex].feaProblem.numConstraint,
                                                tempIntegerArray,
                                                &mystranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Free temporary constraint ID list
        if (tempIntegerArray != NULL) EG_free(tempIntegerArray);
        tempIntegerArray = NULL;
    }

    // Loads
    for (i = 0; i < mystranInstance[iIndex].feaProblem.numLoad; i++) {
        status = nastran_writeLoadCard(fp, &mystranInstance[iIndex].feaProblem.feaLoad[i], &mystranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Constraints
    for (i = 0; i < mystranInstance[iIndex].feaProblem.numConstraint; i++) {

        status = nastran_writeConstraintCard(fp, &mystranInstance[iIndex].feaProblem.feaConstraint[i], &mystranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Supports
    for (i = 0; i < mystranInstance[iIndex].feaProblem.numSupport; i++) {

        status = nastran_writeSupportCard(fp, &mystranInstance[iIndex].feaProblem.feaSupport[i], &mystranInstance[iIndex].feaProblem.feaFileFormat, NULL);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Materials
    for (i = 0; i < mystranInstance[iIndex].feaProblem.numMaterial; i++) {
        status = nastran_writeMaterialCard(fp, &mystranInstance[iIndex].feaProblem.feaMaterial[i], &mystranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Properties
    for (i = 0; i < mystranInstance[iIndex].feaProblem.numProperty; i++) {
        status = nastran_writePropertyCard(fp, &mystranInstance[iIndex].feaProblem.feaProperty[i], &mystranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Coordinate systems
    for (i = 0; i < mystranInstance[iIndex].feaProblem.numCoordSystem; i++) {
        status = nastran_writeCoordinateSystemCard(fp, &mystranInstance[iIndex].feaProblem.feaCoordSystem[i], &mystranInstance[iIndex].feaProblem.feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Include mesh file
    fprintf(fp,"INCLUDE \'%s.bdf\'\n", mystranInstance[iIndex].projectName);

    // End bulk data
    fprintf(fp,"ENDDATA\n");

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Premature exit in mystranAIM preAnalysis status = %d\n", status);

        if (fp != NULL) fclose(fp);

        if (tempIntegerArray != NULL) EG_free(tempIntegerArray);

        if (filename != NULL) EG_free(filename);

        return status;
}


// Check that MYSTRAN ran without errors
int
aimPostAnalysis(/*@unused@*/ int iIndex, /*@unused@*/ void *aimStruc,
                const char *analysisPath, capsErrs **errs)
{
  int status = CAPS_SUCCESS;
  char currentPath[PATH_MAX]; // Current directory path

  char *filename = NULL; // File to open
  char extF06[] = ".F06";
  FILE *fp = NULL; // File pointer

#ifdef DEBUG
  printf(" astrosAIM/aimPostAnalysis instance = %d!\n", inst);
#endif

  *errs = NULL;

  (void) getcwd(currentPath, PATH_MAX); // Get our current path

  // Set path to analysis directory
  if (chdir(analysisPath) != 0) {
      printf(" mystranAIM/aimPostAnalysis: Cannot chdir to %s!\n", analysisPath);
      status = CAPS_DIRERR;
      goto cleanup;
  }

  // check that the mystran *.F06 file was created
  filename = (char *) EG_alloc((strlen(mystranInstance[iIndex].projectName) + strlen(extF06) +1)*sizeof(char));
  if (filename == NULL) return EGADS_MALLOC;

  sprintf(filename, "%s%s", mystranInstance[iIndex].projectName, extF06);

  fp = fopen(filename, "r");

  chdir(currentPath);

  if (fp == NULL) {
      printf(" mystranAIM/aimPostAnalysis: Cannot open Output file: %s!\n", filename);

      status = CAPS_IOERR;
      goto cleanup;
  }

cleanup:
    EG_free(filename); // Free filename allocation
    if (fp != NULL) fclose(fp);

    return status;
}

// Set MYSTRAN output variables
int aimOutputs(int iIndex, void *aimStruc, int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsMYSTRAN AIM Outputs
     * The following list outlines the MYSTRAN outputs available through the AIM interface.
     */

#ifdef DEBUG
    printf(" mystranAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
#endif

    /*! \page aimOutputsMYSTRAN AIM Outputs
     * - <B>EigenValue</B> = List of Eigen-Values (\f$ \lambda\f$) after a modal solve.
     * - <B>EigenRadian</B> = List of Eigen-Values in terms of radians (\f$ \omega = \sqrt{\lambda}\f$ ) after a modal solve.
     * - <B>EigenFrequency</B> = List of Eigen-Values in terms of frequencies (\f$ f = \frac{\omega}{2\pi}\f$) after a modal solve.
     * - <B>EigenGeneralMass</B> = List of generalized masses for the Eigen-Values.
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
    }

    form->type       = Double;
    form->units      = NULL;
    form->lfixed     = Change;
    form->sfixed     = Change;
    form->vals.reals = NULL;
    form->vals.real  = 0;

    /*else if (index == 3) {
     *aoname = EG_strdup("EigenVector");
    	form->type          = Double;
    	form->units         = NULL;
    	form->lfixed        = Change;
    	form->sfixed        = Change;
    }*/

    return CAPS_SUCCESS;
}

// Calculate MYSTRAN output
int aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath,
                  int index, capsValue *val, capsErrs **errors)
{
    int status = CAPS_SUCCESS; // Function return status

    int i; // Indexing

    char currentPath[PATH_MAX]; // Current directory path

    char *filename = NULL; // File to open
    char extOP1[] = ".OP1";
    char extF06[] = ".F06";
    FILE *fp = NULL; // File pointer

    const double pi = 4.0 * atan(1.0);

    (void) getcwd(currentPath, PATH_MAX); // Get our current path

    // Set path to analysis directory
    if (chdir(analysisPath) != 0) {
#ifdef DEBUG
        printf(" mystranAIM/aimCalcOutput Cannot chdir to %s!\n", analysisPath);
#endif

        return CAPS_DIRERR;
    }

    if (index == 1 || index == 2 || index == 3) {
        // Open mystran *.OP1 file - OUTPUT4
        filename = (char *) EG_alloc((strlen(mystranInstance[iIndex].projectName) + strlen(extOP1) +1)*sizeof(char));
        if (filename == NULL) return EGADS_MALLOC;

        sprintf(filename, "%s%s", mystranInstance[iIndex].projectName, extOP1);

        fp = fopen(filename, "rb");

        EG_free(filename); // Free filename allocation

        if (fp == NULL) {
#ifdef DEBUG
            printf(" mystranAIM/aimCalcOutput Cannot open Output file!\n");
#endif

            chdir(currentPath);

            return CAPS_IOERR;
        }

        status = mystran_readOutput4Data(fp, "EIGEN_VA", val);
        if (status == CAPS_SUCCESS) {
            if (index == 2) {
                for (i = 0; i < val->length; i++) {
                    val->vals.reals[i] = sqrt(val->vals.reals[i]);
                }

                //val->units = EG_strdup("rads/s");
            }

            if (index == 3) {
                for (i = 0; i < val->length; i++) {
                    val->vals.reals[i] = sqrt(val->vals.reals[i])/(2*pi);
                }

                //val->units = EG_strdup("Hz");
            }
        }

    } else if (index == 4) {
        // Open mystran *.OP1 file - OUTPUT4
        filename = (char *) EG_alloc((strlen(mystranInstance[iIndex].projectName) + strlen(extOP1) +1)*sizeof(char));
        if (filename == NULL) return EGADS_MALLOC;

        sprintf(filename, "%s%s", mystranInstance[iIndex].projectName, extOP1);

        fp = fopen(filename, "rb");

        EG_free(filename); // Free filename allocation

        if (fp == NULL) {
#ifdef DEBUG
            printf(" mystranAIM/aimCalcOutput Cannot open Output file!\n");
#endif

            chdir(currentPath);

            return CAPS_IOERR;
        }

        status = mystran_readOutput4Data(fp, "GEN_MASS", val);

    } else if (index == 5) {

        // Open mystran *.F06 file
        filename = (char *) EG_alloc((strlen(mystranInstance[iIndex].projectName) + strlen(extF06) +1)*sizeof(char));
        if (filename == NULL) return EGADS_MALLOC;

        sprintf(filename, "%s%s", mystranInstance[iIndex].projectName, extF06);

        fp = fopen(filename, "r");

        EG_free(filename); // Free filename allocation
        if (fp == NULL) {
#ifdef DEBUG
            printf(" mystranAIM/aimCalcOutput Cannot open Output file!\n");
#endif
            chdir(currentPath);

            return CAPS_IOERR;
        }

        /*
        double **dataMatrix = NULL;
        int numEigenVector = 0;
        int numGridPoint = 0;
        status = mystran_readF06Eigen(fp, &numEigenVector, &numGridPoint, &dataMatrix);
         */

    }

    // Restore the path we came in with
    chdir(currentPath);
    if (fp != NULL) fclose(fp);

    return status;
}

void aimCleanup()
{
    int iIndex; // Indexing

    int status; // Returning status

#ifdef DEBUG
    printf(" mystranAIM/aimCleanup!\n");
#endif

    if (numInstance != 0) {

      // Clean up mystranInstance data
      for ( iIndex = 0; iIndex < numInstance; iIndex++) {

          printf(" Cleaning up mystranInstance - %d\n", iIndex);

          status = destroy_aimStorage(iIndex);
          if (status != CAPS_SUCCESS) printf("Error: Status %d during clean up of instance %d\n", status, iIndex);
      }

  }
  numInstance = 0;

  if (mystranInstance != NULL) EG_free(mystranInstance);
  mystranInstance = NULL;

}

int aimFreeDiscr(capsDiscr *discr)
{
    int i; // Indexing

#ifdef DEBUG
    printf(" mystranAIM/aimFreeDiscr instance = %d!\n", discr->instance);
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
    printf(" mystranAIM/aimDiscr: tname = %s, instance = %d!\n", tname, iIndex);
#endif

    if ((iIndex < 0) || (iIndex >= numInstance)) return CAPS_BADINDEX;


    if (tname == NULL) return CAPS_NOTFOUND;

    /*if (mystranInstance[iIndex].dataTransferCheck == (int) false) {
	    printf("The volume is not suitable for data transfer - possibly the volume mesher "
	            "added unaccounted for points\n");
	    return CAPS_BADVALUE;
	}*/

    // Currently this ONLY works if the capsTranfer lives on single body!
    status = aim_getBodies(discr->aInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
        printf(" mystranAIM/aimDiscr: %d aim_getBodies = %d!\n", iIndex, status);
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
            printf(" mystranAIM: getBodyTopos (Face) = %d for Body %d!\n", status, body);
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
              printf("mystranAIM: WARNING: capsIgnore found on bound %s\n", tname);
              continue;
            }
            
#ifdef DEBUG
            printf(" mystranAIM/aimDiscr: Body %d/Face %d matches %s!\n", body, face+1, tname);
#endif

            status = retrieve_CAPSGroupAttr(faces[face], &capsGroup);
            if (status != CAPS_SUCCESS) {
                printf("capsBound found on face %d, but no capGroup found!!!\n", face);
                continue;
            } else {

                status = get_mapAttrToIndexIndex(&mystranInstance[iIndex].attrMap, capsGroup, &attrIndex);
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
                    printf(" mystranAIM: EG_getTessFace %d = %d for Body %d!\n", face+1, status, body+1);
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
        printf(" mystranAIM/aimDiscr: No Faces match %s!\n", tname);

        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Debug
#ifdef DEBUG
    printf(" mystranAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
    printf(" mystranAIM/aimDiscr: nquad = %d, npts = %d!\n", numQuad, numPoint);
#endif

    if ( numPoint == 0 || (numTri == 0 && numQuad == 0) ) {
#ifdef DEBUG
        printf(" mystranAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
        printf(" mystranAIM/aimDiscr: nquad = %d, npts = %d!\n", numQuad, numPoint);
#endif
        status = CAPS_SOURCEERR;
        goto cleanup;
    }

#ifdef DEBUG
    printf(" mystranAIM/aimDiscr: Instance %d, Body Index for data transfer = %d\n", iIndex, dataTransferBodyIndex);
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
            printf(" mystranAIM: EG_getTessFace %d = %d for Body %d!\n", bodyFaceMap[2*face + 1], status, bodyFaceMap[2*face + 0]);
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
                printf("mystranAIM/aimDiscr: EG_localToGlobal accidentally only works for a single quad patch! FIXME!\n");
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
    printf(" mystranAIM/aimDiscr: ntris = %d, npts = %d!\n", discr->nElems, discr->nPoints);
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
        //	printf(" mystranAIM/aimDiscr: Instance = %d, Global Node ID %d\n", iIndex, storage[i]);
        //#endif
    }

    // Save way the attrMap capsGroup list
    storage[numPoint] = numCAPSGroup;
    for (i = 0; i < numCAPSGroup; i++) {
        storage[numPoint+1+i] = capsGroupList[i];
    }

#ifdef DEBUG
    printf(" mystranAIM/aimDiscr: Instance = %d, Finished!!\n", iIndex);
#endif

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("\tPremature exit: function aimDiscr mystranAIM status = %d\n", status);

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
  /*! \page aimUsesDataSetMYSTRAN data sets consumed by MYSTRAN
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

    /*! \page dataTransferMYSTRAN MYSTRAN Data Transfer
     *
     * The MYSTRAN AIM has the ability to transfer displacements and eigenvectors from the AIM and pressure
     * distributions to the AIM using the conservative and interpolative data transfer schemes in CAPS.
     * Currently these transfers may only take place on triangular meshes.
     *
     * \section dataFromMYSTRAN Data transfer from MYSTRAN
     *
     * <ul>
     *  <li> <B>"Displacement"</B> </li> <br>
     *   Retrieves nodal displacements from the *.F06 file.
     * </ul>
     *
     * <ul>
     *  <li> <B>"EigenVector_#"</B> </li> <br>
     *   Retrieves modal eigen-vectors from the *.F06 file, where "#" should be replaced by the
     *   corresponding mode number for the eigen-vector (eg. EigenVector_3 would correspond to the third mode,
     *   while EigenVector_6 would be the sixth mode).
     * </ul>
     *
     * \section dataToMYSTRAN Data transfer to MYSTRAN
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

    char *extF06 = ".F06";

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
    printf(" mystranAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n", dataName, discr->instance, numPoint, dataRank);
#endif

    //Get the appropriate parts of the tessellation to data
    storage = (int *) discr->ptrm;
    nodeMap = &storage[0]; // Global indexing on the body

    //capsGroupList = &storage[discr->nPoints]; // List of boundary ID (attrMap) in the transfer

    if (strcasecmp(dataName, "Displacement")    != 0 &&
           strncmp(dataName, "EigenVector", 11) != 0) {

        printf("Unrecognized data transfer variable - %s\n", dataName);
        return CAPS_NOTFOUND;
    }

    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(mystranInstance[discr->instance].analysisPath) != 0) {
#ifdef DEBUG
        printf(" mystranAIM/aimTransfer Cannot chdir to %s!\n", mystranInstance[discr->instance].analysisPath);
#endif

        return CAPS_DIRERR;
    }

    filename = (char *) EG_alloc((strlen(mystranInstance[discr->instance].projectName) +
                                  strlen(extF06) + 1)*sizeof(char));

    sprintf(filename,"%s%s",mystranInstance[discr->instance].projectName,extF06);

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

            status = mystran_readF06Displacement(fp,
                                                 1,
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

            status = mystran_readF06EigenVector(fp,
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
  	  printf(" mystranAIM/aimInterpolation  %s  instance = %d!\n",
         name, discr->instance);
	#endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" mystranAIM/Interpolation: eIndex = %d [1-%d]!\n",eIndex, discr->nElems);
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

            result[i] = (1.0-we[0])*((1.0-we[1])*data[rank*in[0]+i] +
                             we[1] *data[rank*in[3]+i] ) +
                             we[0] *((1.0-we[1])*data[rank*in[1]+i] +
                             we[1] *data[rank*in[2]+i] );
        }

    } else {
        printf(" mystranAIM/Interpolation: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
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
  	  printf(" mystranAIM/aimInterpolateBar  %s  instance = %d!\n", name, discr->instance);
	#endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" mystranAIM/InterpolateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
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
        printf(" mystranAIM/InterpolateBar: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
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
  	  printf(" mystranAIM/aimIntegration  %s  instance = %d!\n", name, discr->instance);
	#endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" mystranAIM/aimIntegration: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" mystranAIM/aimIntegration: %d aim_getBodies = %d!\n", discr->instance, stat);
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
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);
        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[3], stat);
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
        printf(" mystranAIM/aimIntegration: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
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
  	  printf(" mystranAIM/aimIntegrateBar  %s  instance = %d!\n", name, discr->instance);
	#endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" mystranAIM/aimIntegrateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" mystranAIM/aimIntegrateBar: %d aim_getBodies = %d!\n", discr->instance, stat);
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
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
            return stat;
        }

        stat = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);

        if (stat != CAPS_SUCCESS) {
            printf(" mystranAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[3], stat);
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
        printf(" mystranAIM/aimIntegrateBar: eIndex = %d [1-%d], nref not recognized!\n",eIndex, discr->nElems);
        return CAPS_BADVALUE;
    }

    return CAPS_SUCCESS;
}
