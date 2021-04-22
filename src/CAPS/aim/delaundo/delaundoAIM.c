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
 * \section overviewDelaundo Delaundo AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact with the 2D Delaunay
 * mesh generator Delaundo, developed by J.-D. Müller. Details and download information for Delaundo may be found at
 * http://www.ae.metu.edu.tr/tuncer/ae546/prj/delaundo/
 *
 * Along with isotropic triangular mesh generation, Delaundo has limited anisotropic mesh generating capabilities.
 * From the Delaundo website - "delaundo has also a rudimentary capability to create grids with stretched
 * layers for viscous calculations that works well for moderate stretching factors of up to 100.
 * Due to the simple implementation the stretched layers must completely wrap around a simply connected body
 * such as an airfoil with a wake. It cannot do bump-like cases, where non-stretched boundaries are
 * attached to stretched ones."
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsDelaundo and \ref aimOutputsDelaundo, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentDelaundo.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataDelaundo if connecting this AIM to other AIMs in a
 * parent-child like manner.
 */



#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"       // Collection of helper functions for meshing
#include "cfdUtils.h"        // Collection of helper functions for cfd analysis
#include "miscUtils.h"       // Collection of helper functions for miscellaneous analysis

#ifdef WIN32
#define getcwd     _getcwd
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif


#define NUMINPUT  19 // Number of mesh inputs
#define NUMOUT    1  // Number of outputs

#define MXCHAR  255

typedef struct {

    // Container for surface mesh
    meshStruct *surfaceMesh;
    int numSurface;

    // Container for mesh input
    meshInputStruct meshInput;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

    // Number of boundary elements
    int numBoundaryEdge;

    int numEdge;
    int *edgeAttrMap; // size = [numEdge]
    int faceAttr;

    // Swapping node flags
    int swapZX;
    int swapZY;

} aimStorage;

static aimStorage *delaundoInstance = NULL;
static int         numInstance  = 0;


static int destroy_aimStorage(int iIndex) {

    int status; // Function return status

    int i; // Indexing

    // Destroy meshInput
    status = destroy_meshInputStruct(&delaundoInstance[iIndex].meshInput);
    if (status != CAPS_SUCCESS) printf("Status = %d, delaundoAIM instance %d, meshInput cleanup!!!\n", status, iIndex);

    // Destroy surface mesh allocated arrays
    if (delaundoInstance[iIndex].surfaceMesh != NULL) {

        for (i = 0; i < delaundoInstance[iIndex].numSurface; i++) {
            status = destroy_meshStruct(&delaundoInstance[iIndex].surfaceMesh[i]);
            if (status != CAPS_SUCCESS) printf("Status = %d, delaundoAIM instance %d, surfaceMesh cleanup!!!\n", status, iIndex);
        }
        if (delaundoInstance[iIndex].surfaceMesh != NULL)  EG_free(delaundoInstance[iIndex].surfaceMesh);
    }

    delaundoInstance[iIndex].numSurface = 0;
    delaundoInstance[iIndex].surfaceMesh = NULL;

    // Destroy attribute to index map
    status = destroy_mapAttrToIndexStruct(&delaundoInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Status = %d, delaundoAIM instance %d, attributeMap cleanup!!!\n", status, iIndex);

    delaundoInstance[iIndex].numEdge = 0;

    if (delaundoInstance[iIndex].edgeAttrMap != NULL) EG_free(delaundoInstance[iIndex].edgeAttrMap);
    delaundoInstance[iIndex].edgeAttrMap = NULL; // size = [numEdge]

    delaundoInstance[iIndex].faceAttr = 0;

    delaundoInstance[iIndex].swapZX = (int) false;
    delaundoInstance[iIndex].swapZY = (int) false;

    delaundoInstance[iIndex].numBoundaryEdge = 0;

    return CAPS_SUCCESS;
}

static int write_ctrFile(void *aimInfo, const char *analysisPath, capsValue *aimInputs) {

    int status; //Function return status

    FILE *fp = NULL;
    char *filename = NULL;
    char *fileExt = ".ctr";
    int stringLength;

    stringLength = strlen(analysisPath) +
                    1 +
                    strlen(aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string) +
                    strlen(fileExt);

    filename = (char *) EG_alloc((stringLength+1)*sizeof(char));
    if (filename == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    sprintf(filename, "%s/%s%s", analysisPath,
                                 aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string,
                                 fileExt);
    filename[stringLength] = '\0';

    printf("Writing delaundo control file - %s\n", filename);

    // Lets read our mesh back in
    fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Unable to open file - %s\n", filename);
        status = CAPS_IOERR;
        goto cleanup;
    }

    fprintf(fp, "VERBOSe:\n   3\n");
    fprintf(fp, "ALLPARameters:\n   F\n");
    fprintf(fp, "INFILE:\n   %s.pts\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);
    fprintf(fp, "INFORMatted:\n   T\n");
    fprintf(fp, "NODEUSe:\n   F\n");
    fprintf(fp, "NODECOnstr.:\n   T\n");
    fprintf(fp, "ANTICOnnect.:\n   F\n");

    fprintf(fp, "SPCRATio:\n   %f\n", aimInputs[aim_getIndex(aimInfo, "Spatial_Ratio", ANALYSISIN)-1].vals.real);

    fprintf(fp, "DTOLERance:\n   %f\n", aimInputs[aim_getIndex(aimInfo, "D_Tolerance", ANALYSISIN)-1].vals.real);

    fprintf(fp, "QTOLERance:\n   %f\n", aimInputs[aim_getIndex(aimInfo, "Q_Tolerance", ANALYSISIN)-1].vals.real);

    if (aimInputs[aim_getIndex(aimInfo, "Delta_Thickness", ANALYSISIN)-1].vals.real > 0) {
        fprintf(fp, "STRETChing:\n   T\n");
    } else {
        fprintf(fp, "STRETChing:\n   F\n");
    }

    fprintf(fp, "BTOLERance:\n   %f\n", aimInputs[aim_getIndex(aimInfo, "B_Tolerance", ANALYSISIN)-1].vals.real);

    fprintf(fp, "DELTAStar:\n    %f\n", aimInputs[aim_getIndex(aimInfo, "Delta_Thickness", ANALYSISIN)-1].vals.real);

    fprintf(fp, "MAXASPect ratio:\n   %f\n", aimInputs[aim_getIndex(aimInfo, "Max_Aspect", ANALYSISIN)-1].vals.real);

    fprintf(fp, "MVISROw:\n   %d\n", aimInputs[aim_getIndex(aimInfo, "Num_Anisotropic", ANALYSISIN)-1].vals.integer);

    fprintf(fp, "ASKROW:\n   F\n");

    fprintf(fp, "ISMOOTh:\n   %d\n",aimInputs[aim_getIndex(aimInfo, "Transition_Scheme", ANALYSISIN)-1].vals.integer);

    fprintf(fp, "MISOROw:\n   %d\n", aimInputs[aim_getIndex(aimInfo, "Num_Isotropic", ANALYSISIN)-1].vals.integer);

    if (aimInputs[aim_getIndex(aimInfo, "Flat_Swap", ANALYSISIN)-1].vals.integer == (int) true) {
        fprintf(fp, "FLATSWap:\n   T\n");
    } else {
        fprintf(fp, "FLATSWap:\n   F\n");
    }

    fprintf(fp, "ANGMAX:\n    %f\n", aimInputs[aim_getIndex(aimInfo, "Max_Angle", ANALYSISIN)-1].vals.real);
    fprintf(fp, "MCYCSWap:\n   %d\n", aimInputs[aim_getIndex(aimInfo, "Num_Swap", ANALYSISIN)-1].vals.integer);

    fprintf(fp, "OUTFILe:\n   ./%s.mesh\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);

    fprintf(fp, "OUTTYPe:\n   t\n");
    fprintf(fp, "OUTFORmat:\n   d\n");
    fprintf(fp, "DOLOGFile:\n   T\n");
    fprintf(fp, "LOGFILe:\n   ./%s.log\n", aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("write_ctrFile status = %d\n", status);

        if (fp != NULL) fclose(fp);
        if (filename != NULL) EG_free(filename);

        return status;
}

/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(/*@unused@*/ int ngIn, /*@unused@*/ capsValue *gIn,
                  int *qeFlag, /*@null@*/ const char *unitSys, int *nIn, int *nOut,
                  int *nFields, char ***fnames, int **ranks)
{
    int status; // Function return status
    int flag;   // query/execute flag

    aimStorage *tmp;

#ifdef DEBUG
    printf("\n delaundoAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif
    flag    = *qeFlag;
    *qeFlag = 0;

    /* specify the number of analysis input and out "parameters" */
    *nIn     = NUMINPUT;
    *nOut    = NUMOUT;
    if (flag == 1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate */
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Allocate aflrInstance
    if (delaundoInstance == NULL) {
        delaundoInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (delaundoInstance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(delaundoInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        delaundoInstance = tmp;
    }

    // Set initial values for delaundoInstance //

    // Container for surface meshes
    delaundoInstance[numInstance].surfaceMesh = NULL;
    delaundoInstance[numInstance].numSurface = 0;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&delaundoInstance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for mesh input
    status = initiate_meshInputStruct(&delaundoInstance[numInstance].meshInput);
    if (status != CAPS_SUCCESS) return status;

    delaundoInstance[numInstance].numEdge = 0;
    delaundoInstance[numInstance].edgeAttrMap = NULL; // size = [numEdge]

    delaundoInstance[numInstance].faceAttr = 0;


    delaundoInstance[numInstance].swapZX = (int) false;
    delaundoInstance[numInstance].swapZY = (int) false;

    delaundoInstance[numInstance].numBoundaryEdge = 0;

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}

int aimInputs(int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{

    /*! \page aimInputsDelaundo AIM Inputs
     * The following list outlines the Delaundo meshing options along with their default value available
     * through the AIM interface. Please note that not all of Delaundo's inputs are currently exposed.
     */


#ifdef DEBUG
    printf(" delaundoAIM/aimInputs instance = %d  index = %d!\n", inst, index);
#endif

    // Meshing Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name");
        defval->type         = String;
        defval->vals.string  = NULL;
        defval->vals.string  = EG_strdup("delaundoCAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsDelaundo
         * - <B> Proj_Name = delaundoCAPS</B> <br>
         * This corresponds to the output name of the mesh.
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

        /*! \page aimInputsDelaundo
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
    } else if (index == 3) {
        *ainame               = EG_strdup("Mesh_Format");
        defval->type          = String;
        defval->nullVal       = IsNull;
        defval->vals.string   = EG_strdup("AFLR3"); // VTK, AFLR3
        defval->lfixed        = Change;

        /*! \page aimInputsDelaundo
         * - <B> Mesh_Format = NULL</B> <br>
         * Mesh output format. If left NULL, the mesh is not written in the new file format.
         * Available format names include: "AFLR3", "VTK", "TECPLOT", "STL".
         */
    } else if (index == 4) {
        *ainame               = EG_strdup("Mesh_ASCII_Flag");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsDelaundo
         * - <B> Mesh_ASCII_Flag = True</B> <br>
         * Output mesh in ASCII format, otherwise write a binary file if applicable.
         */

    } else if (index == 5) {
        *ainame               = EG_strdup("Edge_Point_Min");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsDelaundo
         * - <B> Edge_Point_Min = NULL</B> <br>
         * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 6) {
        *ainame               = EG_strdup("Edge_Point_Max");
        defval->type          = Integer;
        defval->vals.integer  = 0;
        defval->length        = 1;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = IsNull;

        /*! \page aimInputsDelaundo
         * - <B> Edge_Point_Max = NULL</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 7) {
        *ainame              = EG_strdup("Mesh_Sizing");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsDelaundo
         * - <B>Mesh_Sizing = NULL </B> <br>
         * See \ref meshSizingProp for additional details.
         */
    } else if (index == 8) {
        *ainame               = EG_strdup("Spatial_Ratio");
        defval->type          = Double;
        defval->vals.real     = 1;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Spatial_Ratio = 1.0</B> <br>
         * This corresponds to SPCRAT in the Delaundo manual - Ratio between the spacing gradients at the points of highest and lowest spacing.
         * Values higher than one will cause Delaundo to interpolate with a power law to extend the
         * regions of fine spacing further into the domain.
         */
    } else if (index == 9) {
        *ainame               = EG_strdup("D_Tolerance");
        defval->type          = Double;
        defval->vals.real     = 0.65;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> D_Tolerance = 0.65</B> <br>
         * This corresponds to DTOLER in the Delaundo manual - Specifies the fraction of the background mesh size
         * that is being used as a minimum distance between nodes.
         */
    } else if (index == 10) {
        *ainame               = EG_strdup("Q_Tolerance");
        defval->type          = Double;
        defval->vals.real     = 0.65;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Q_Tolerance = 0.65</B> <br>
         * This corresponds to QTOLER in the Delaundo manual - specifies the minimum fraction of the maximum side
         * length that the smaller sides must have in order to make the triangle acceptable.
         */
    } else if (index == 11) {
        *ainame               = EG_strdup("B_Tolerance");
        defval->type          = Double;
        defval->vals.real     = 2.0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> B_Tolerance = 2.0</B> <br>
         * This corresponds to BTOLER in the Delaundo manual - specifies the minimum fraction of the background mesh
         * size that is being used as a minimum distance between nodes in the background grid.
         */
    } else if (index == 12) {
        *ainame               = EG_strdup("Delta_Thickness");
        defval->type          = Double;
        defval->vals.real     = 0.0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Delta_Thickness  = 0.0</B> <br>
         * This corresponds to DELTAS in the Delaundo manual - specifies the thickness of the stretched
         * layer in the scale of the other points. No stretched region will be created if the value is less than or
         * equal to 0.0 .
         */
    } else if (index == 13) {
        *ainame               = EG_strdup("Max_Aspect");
        defval->type          = Double;
        defval->vals.real     = 20.0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Max_Aspect  = 20.0</B> <br>
         * This corresponds to MAXASP in the Delaundo manual - specifies the maximum aspect ratio in the stretched layer.
         */
    } else if (index == 14) {
        *ainame               = EG_strdup("Num_Anisotropic");
        defval->type          = Integer;
        defval->vals.integer  = 30000;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Num_Anisotropic  = 30,000</B> <br>
         * This corresponds to MVISRO in the Delaundo manual - specifies how many stretched, viscous rows are to be built.
         */
    } else if (index == 15) {
        *ainame               = EG_strdup("Num_Isotropic");
        defval->type          = Integer;
        defval->vals.integer  = 30000;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Num_Isotropic  = 30,000</B> <br>
         * This corresponds to MISORO in the Delaundo manual - specifies how many isotropic rows are to be built.
         */
    } else if (index == 16) {
        *ainame               = EG_strdup("Transition_Scheme");
        defval->type          = Integer;
        defval->vals.integer  = 2;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Transition_Scheme  = 2</B> <br>
         * This corresponds to ISMOOT in the Delaundo manual - specifies how many stretched rows of cells will be opened
         * for isotropic re-triangulation once the stretched process has terminated. 0 does not allow any re-triangulation,
         * 1 allows re-triangulation of the outermost cells, and 2 allows re-triangulation of the neighbors
         *  of the outermost cells as well.
         */
    } else if (index == 17) {
        *ainame               = EG_strdup("Flat_Swap");
        defval->type          = Boolean;
        defval->vals.integer  = true;

        /*! \page aimInputsDelaundo
         * - <B> Flat_Swap = True</B> <br>
         * This corresponds to FLATSW in the Delaundo manual - if True this will make DELAUNDO swap diagonals in the
         *  final mesh in order to minimize the maximum angles.
         */

    } else if (index == 18) {
        *ainame               = EG_strdup("Max_Angle");
        defval->type          = Double;
        defval->vals.real     = 120.0;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Max_Angle  = 120.0</B> <br>
         * This corresponds to ANGMAX in the Delaundo manual - specifies the maximum tolerable cell angle before FLATSW
         * kicks in.
         */
    } else if (index == 19) {
        *ainame               = EG_strdup("Num_Swap");
        defval->type          = Integer;
        defval->vals.integer  = 10;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        /*! \page aimInputsDelaundo
         * - <B> Num_Swap  = 10</B> <br>
         * This corresponds to MCYCSW in the Delaundo manual - specifies how many swapping cycles will be executed.
         */
    }


    /*if (index != 1){
        // Link variable(s) to parent(s) if available
        status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
        //printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n", status, index, defval->type, defval->link);
    }*/

    return CAPS_SUCCESS;
}

int aimData(int iIndex, const char *name, enum capsvType *vtype,
            int *rank, int *nrow, int *ncol, void **data, char **units)
{
    /*! \page sharableDataDelaundo AIM Shareable Data
     * The delaundo AIM has the following shareable data types/values with its children AIMs if they are so inclined.
     * - <B> Surface_Mesh</B> <br>
     * The returned surface mesh after delaundo execution is complete in meshStruct (see meshTypes.h) format.
     * - <B> Attribute_Map</B> <br>
     * An index mapping between capsGroups found on the geometry in mapAttrToIndexStruct (see miscTypes.h) format.
     */

#ifdef DEBUG
    printf(" delaundoAIM/aimData instance = %d  name = %s!\n", inst, name);
#endif

    // The returned surface mesh from delaundo

    if (strcasecmp(name, "Surface_Mesh") == 0){
        *vtype = Value;
        *rank = *nrow = 1;
        *ncol  = delaundoInstance[iIndex].numSurface;
        *data  = delaundoInstance[iIndex].surfaceMesh;
        *units = NULL;

        return CAPS_SUCCESS;
    }

    // Share the attribute map
    if (strcasecmp(name, "Attribute_Map") == 0){
        *vtype = Value;
        *rank  = *nrow = *ncol = 1;
        *data  = &delaundoInstance[iIndex].attrMap;
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

    // Tessellation
    double size, box[6], params[3];
    ego egadsTess;

    // Global settings
    int minEdgePoint = -1, maxEdgePoint = -1;

    // Mesh attribute parameters
    int numMeshProp = 0;
    meshSizingStruct *meshProp = NULL;

    // FILEIO
    FILE *fp = NULL;
    char fileExt[] = ".pts";
    char *filename = NULL;

    // 2D mesh checks
    int xMeshConstant = (int) true, yMeshConstant = (int) true, zMeshConstant= (int) true;

    // EGADS meshing variables
    ego *faces = NULL, *loops = NULL, *edges = NULL, geom;
    int oclass, mtype;
    double uvbox[4], refLen = -1;
    int *edgeSense = NULL, *loopSense = NULL;

    int loopIndex, edgeIndex, edgeBodyIndex; // Indexing
    int numFace, numLoop, numEdge; // Totals

    int numPoints;
    const double *points = NULL, *uv = NULL, *ts = NULL;

    int stringLength = 0;

    // Mesh attribute parameters
    const char *groupName = NULL;
    int attrIndex = 0;

    // NULL out errors
    *errs = NULL;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

#ifdef DEBUG
    printf(" delaundoAIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, delaundoInstance[iIndex].numSurface);
#endif

    if ((numBody <= 0) || (bodies == NULL)) {
#ifdef DEBUG
        printf(" delaundoAIM/aimPreAnalysis No Bodies!\n");
#endif
        return CAPS_SOURCEERR;
    }

    if (numBody != 1) {
        printf("Delaundo is a 2D mesh generator only one body may be supplied!\n");
        return CAPS_BADVALUE;
    }

    // Cleanup previous aimStorage for the instance in case this is the second time through preAnalysis for the same instance
    status = destroy_aimStorage(iIndex);
    if (status != CAPS_SUCCESS) {
        printf("Status = %d, delaundo instance %d, aimStorage cleanup!!!\n", status, iIndex);
        return status;
    }

    // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
    status = create_CAPSGroupAttrToIndexMap(numBody,
                                            bodies,
                                            2,
                                            &delaundoInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Allocate surfaceMesh from number of bodies
    delaundoInstance[iIndex].numSurface = numBody;
    delaundoInstance[iIndex].surfaceMesh = (meshStruct *) EG_alloc(delaundoInstance[iIndex].numSurface*sizeof(meshStruct));

    if (delaundoInstance[iIndex].surfaceMesh == NULL) {
        delaundoInstance[iIndex].numSurface = 0;
        return EGADS_MALLOC;
    }

    // Initiate surface meshes
    for (bodyIndex = 0; bodyIndex < delaundoInstance[iIndex].numSurface; bodyIndex++){
        status = initiate_meshStruct(&delaundoInstance[iIndex].surfaceMesh[bodyIndex]);
        if (status != CAPS_SUCCESS) return status;
    }

    // Setup meshing input structure -> -1 because aim_getIndex is 1 bias

    // Get Tessellation parameters -Tess_Params -> -1 because of aim_getIndex 1 bias
    delaundoInstance[iIndex].meshInput.paramTess[0] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[0]; // Gets multiplied by bounding box size
    delaundoInstance[iIndex].meshInput.paramTess[1] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[1]; // Gets multiplied by bounding box size
    delaundoInstance[iIndex].meshInput.paramTess[2] = aimInputs[aim_getIndex(aimInfo, "Tess_Params", ANALYSISIN)-1].vals.reals[2];

    if (aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].nullVal != IsNull) {
        stringLength = strlen(aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].vals.string);

        delaundoInstance[iIndex].meshInput.outputFormat = (char *) EG_alloc((stringLength+1) *sizeof(char));
        if (delaundoInstance[iIndex].meshInput.outputFormat == NULL) return EGADS_MALLOC;

        strncpy(delaundoInstance[iIndex].meshInput.outputFormat,
                aimInputs[aim_getIndex(aimInfo, "Mesh_Format", ANALYSISIN)-1].vals.string,
                stringLength);

        delaundoInstance[iIndex].meshInput.outputFormat[stringLength] = '\0';
    }

    delaundoInstance[iIndex].meshInput.outputASCIIFlag  = aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag",    ANALYSISIN)-1].vals.integer;

    if (aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].nullVal != IsNull) {

        stringLength = strlen(aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string);

        delaundoInstance[iIndex].meshInput.outputFileName = (char *) EG_alloc((stringLength+1) *sizeof(char));
        if (delaundoInstance[iIndex].meshInput.outputFileName == NULL) return EGADS_MALLOC;

        strncpy(delaundoInstance[iIndex].meshInput.outputFileName,
                aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string,
                stringLength);

        delaundoInstance[iIndex].meshInput.outputFileName[stringLength] = '\0';
    }

    stringLength = strlen(analysisPath);

    delaundoInstance[iIndex].meshInput.outputDirectory = (char *) EG_alloc((stringLength+1) * sizeof(char));
    if (delaundoInstance[iIndex].meshInput.outputDirectory == NULL) return EGADS_MALLOC;

    strncpy(delaundoInstance[iIndex].meshInput.outputDirectory,
            analysisPath,
            stringLength);

    delaundoInstance[iIndex].meshInput.outputDirectory[stringLength] = '\0';


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
                                    &delaundoInstance[iIndex].attrMap,
                                    &numMeshProp,
                                    &meshProp);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Modify the EGADS body tessellation based on given inputs
    status =  mesh_modifyBodyTess(numMeshProp,
                                  meshProp,
                                  minEdgePoint,
                                  maxEdgePoint,
                                  (int)false, // quadMesh
                                  &refLen,
                                  delaundoInstance[iIndex].meshInput.paramTess,
                                  delaundoInstance[iIndex].attrMap,
                                  numBody,
                                  bodies);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Write control file
    status = write_ctrFile(aimInfo, analysisPath, aimInputs);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Run delaundo for each body
    for (bodyIndex = 0 ; bodyIndex < numBody; bodyIndex++) {

        printf("Getting edge discretization for body %d\n", bodyIndex+1);

        // Get bounding box for scaling
        status = EG_getBoundingBox(bodies[bodyIndex], box);
        if (status != EGADS_SUCCESS) goto cleanup;

        size = sqrt((box[0]-box[3])*(box[0]-box[3]) +
                    (box[1]-box[4])*(box[1]-box[4]) +
                    (box[2]-box[5])*(box[2]-box[5]));

        // Negating the first parameter triggers EGADS to only put vertexes on edges
        params[0] = -delaundoInstance[iIndex].meshInput.paramTess[0]*size;
        params[1] =  delaundoInstance[iIndex].meshInput.paramTess[1]*size;
        params[2] =  delaundoInstance[iIndex].meshInput.paramTess[2];

        status = EG_makeTessBody(bodies[bodyIndex], params, &egadsTess);
        if (status != EGADS_SUCCESS) {
            printf("\tProblem during edge discretization of body %d\n", bodyIndex+1);
            goto cleanup;
        }

        stringLength = strlen(analysisPath) +
                       strlen(delaundoInstance[iIndex].meshInput.outputFileName) +
                       strlen(fileExt) + 1;

        filename = (char *) EG_alloc((stringLength +1) *sizeof(char));
        if (filename == NULL) {
            status =  EGADS_MALLOC;
            goto cleanup;
        }

        sprintf(filename, "%s/%s%s", analysisPath,delaundoInstance[iIndex].meshInput.outputFileName, fileExt);
        filename[stringLength] = '\0';

        fp = fopen(filename,"w");
        if (fp == NULL) {
            printf("\tUnable to open file - %s\n", filename);
            status = CAPS_IOERR;
            goto cleanup;
        }

        printf("\tWriting out *.pts file\n");

        // Get faces
        status = EG_getBodyTopos(bodies[bodyIndex], NULL, FACE, &numFace, &faces);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (numFace != 1) {
            printf("\tBody should only have 1 face!!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        status = retrieve_CAPSGroupAttr(faces[0], &groupName);
        if (status == EGADS_SUCCESS) {
            status = get_mapAttrToIndexIndex(&delaundoInstance[iIndex].attrMap, groupName, &attrIndex);
            if (status == CAPS_SUCCESS) {
                delaundoInstance[iIndex].faceAttr = attrIndex;


            } else {
                printf("\tNo capsGroup \"%s\" not found in attribute map\n", groupName);
                goto cleanup;
            }
        } else if (status != EGADS_NOTFOUND) {
            printf("\tWarning: No capsGroup found on face %d, this may be a issue for some analyses\n", numFace);
        } else goto cleanup;

        // Get body edges
        status = EG_getBodyTopos(bodies[bodyIndex], NULL, EDGE, &delaundoInstance[iIndex].numEdge, &edges);
        if (status != EGADS_SUCCESS) {
            EG_free(edges);
            goto cleanup;
        }

        // See what plane our face is on
        for (edgeIndex = 0; edgeIndex < delaundoInstance[iIndex].numEdge; edgeIndex++) {

            edgeBodyIndex = EG_indexBodyTopo(bodies[bodyIndex], edges[edgeIndex]);
            if (edgeBodyIndex < EGADS_SUCCESS) {
                EG_free(edges);
                status = -edgeBodyIndex;
                goto cleanup;
            }

            status = EG_getTessEdge(egadsTess, edgeBodyIndex, &numPoints, &points, &ts);
            if (status != EGADS_SUCCESS) {
                EG_free(edges);
                status =  -status;
                goto cleanup;
            }

            // Constant x?
            for (i = 0; i < numPoints; i++) {
                if (fabs(points[3*i + 0] - points[3*0 + 0]) > 1E-7) {
                    xMeshConstant = (int) false;
                    break;
                }
            }

            // Constant y?
            for (i = 0; i < numPoints; i++) {
                if (fabs(points[3*i + 1] - points[3*0 + 1] ) > 1E-7) {
                    yMeshConstant = (int) false;
                    break;
                }
            }

            // Constant z?
            for (i = 0; i < numPoints; i++) {
                if (fabs(points[3*i + 2]-points[3*0 + 2]) > 1E-7) {
                    zMeshConstant = (int) false;
                    break;
                }
            }
        }
        EG_free(edges); edges = NULL;

        if (zMeshConstant != (int) true) {
            printf("\tDelaundo expects 2D meshes be in the x-y plane... attempting to rotate mesh through node swapping!\n");

            if (xMeshConstant == (int) true && yMeshConstant == (int) false) {

                printf("\tSwapping z and x coordinates!\n");
                delaundoInstance[iIndex].swapZX = (int) true;

            } else if(xMeshConstant == (int) false && yMeshConstant == (int) true) {

                printf("\tSwapping z and y coordinates!\n");
                delaundoInstance[iIndex].swapZY = (int) true;

            } else {

                printf("\tUnable to rotate mesh!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }


        delaundoInstance[iIndex].edgeAttrMap = (int *) EG_alloc(delaundoInstance[iIndex].numEdge*sizeof(int));
        if (delaundoInstance[iIndex].edgeAttrMap == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        status = EG_getTopology(faces[0], &geom, &oclass, &mtype, uvbox, &numLoop, &loops, &loopSense);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (faces != NULL) EG_free(faces);
        faces = NULL;

        // Go around the loop and collect edges
        for (loopIndex = 0; loopIndex < numLoop; loopIndex++) {

            //printf("Loop %d\n", loopIndex);

            status = EG_getTopology(loops[loopIndex], &geom, &oclass, &mtype,
                    NULL, &numEdge, &edges, &edgeSense);
            if (status != EGADS_SUCCESS) {
                printf("\tEG_getTopology status = %d\n",status);
                goto cleanup;
            }

            for (edgeIndex = 0; edgeIndex < numEdge; edgeIndex++) {

                //printf("edgeIndex = %d\n", edgeIndex);
                edgeBodyIndex = EG_indexBodyTopo(bodies[bodyIndex], edges[edgeIndex]);
                if (edgeBodyIndex < EGADS_SUCCESS) {
                    status = CAPS_BADINDEX;
                    goto cleanup;
                }

                status = retrieve_CAPSGroupAttr(edges[edgeIndex], &groupName);
                if (status == EGADS_SUCCESS) {
                    status = get_mapAttrToIndexIndex(&delaundoInstance[iIndex].attrMap, groupName, &attrIndex);
                    if (status == CAPS_SUCCESS) {
                        delaundoInstance[iIndex].edgeAttrMap[edgeBodyIndex-1]  = attrIndex;
                    } else {
                        printf("\tNo capsGroup \"%s\" not found in attribute map\n", groupName);
                        goto cleanup;
                    }
                } else if (status == EGADS_NOTFOUND) {
                    printf("\tError: No capsGroup found on edge %d\n", edgeBodyIndex);
                    goto cleanup;
                } else goto cleanup;

                //printf("EdgeBodyIndex = %d %d\n", edgeBodyIndex, edgeSense[edgeIndex]);
                status = EG_getTessEdge(egadsTess, edgeBodyIndex, &numPoints, &points, &uv);
                if (status != EGADS_SUCCESS) {
                    printf("\tEG_getTessEdge status = %d\n",status);
                    goto cleanup;
                }

                fprintf(fp,"NEWBND\n");

                fprintf(fp,"NAMEBN\n %d\n", edgeBodyIndex);

                fprintf(fp,"NRBNDE\n %d\n", numPoints);

                delaundoInstance[iIndex].numBoundaryEdge = (numPoints -1) + delaundoInstance[iIndex].numBoundaryEdge;

                // Edge connected at the start
                if (edgeIndex == 0){
                    edgeBodyIndex = EG_indexBodyTopo(bodies[bodyIndex], edges[numEdge-1]);
                    if (edgeBodyIndex < EGADS_SUCCESS) {
                        status = CAPS_BADINDEX;
                        goto cleanup;
                    }
                } else {
                    edgeBodyIndex = EG_indexBodyTopo(bodies[bodyIndex], edges[edgeIndex-1]);
                    if (edgeBodyIndex < EGADS_SUCCESS) {
                        status = CAPS_BADINDEX;
                        goto cleanup;
                    }
                }

                fprintf(fp,"NFRSBN\n %d\n", edgeBodyIndex);

                // Edge connected at the end
                if (edgeIndex == numEdge-1) {
                    edgeBodyIndex = EG_indexBodyTopo(bodies[bodyIndex], edges[0]);
                    if (edgeBodyIndex < EGADS_SUCCESS) {
                        status = CAPS_BADINDEX;
                        goto cleanup;
                    }
                } else {
                    edgeBodyIndex = EG_indexBodyTopo(bodies[bodyIndex], edges[edgeIndex+1]);
                    if (edgeBodyIndex < EGADS_SUCCESS) {
                        status = CAPS_BADINDEX;
                        goto cleanup;
                    }
                }

                fprintf(fp,"NLSTBN\n %d\n", edgeBodyIndex);

                if (loopSense[loopIndex] == 1) fprintf(fp,"ITYPBN\n %d\n", 2); // Outer
                else fprintf(fp,"ITYPBN\n %d\n", 1); // Inner

                fprintf(fp,"BNDEXY\n");

                if (edgeSense[edgeIndex] > 0) {
                    for (i = 0; i < numPoints; i++) {

                        if (delaundoInstance[iIndex].swapZX == (int) true) {
                            // x = z
                            fprintf(fp, " %f %f\n", points[3*i + 2],
                                    points[3*i + 1]);

                        } else if (delaundoInstance[iIndex].swapZY == (int) true) {
                            // y = z
                            fprintf(fp, " %f %f\n", points[3*i + 0],
                                    points[3*i + 2]);
                        } else {
                            fprintf(fp, " %f %f\n", points[3*i + 0],
                                    points[3*i + 1]);
                        }
                    }

                } else {

                    for (i = numPoints-1; i >= 0; i--) {

                        if (delaundoInstance[iIndex].swapZX == (int) true) {
                            // x = z
                            fprintf(fp, " %f %f\n", points[3*i + 2],
                                    points[3*i + 1]);

                        } else if (delaundoInstance[iIndex].swapZY == (int) true) {
                            // y = z
                            fprintf(fp, " %f %f\n", points[3*i + 0],
                                    points[3*i + 2]);
                        } else {
                            fprintf(fp, " %f %f\n", points[3*i + 0],
                                    points[3*i + 1]);
                        }
                    }
                }
            }
        }

        fprintf(fp, "ENDDAT\n");

        if (fp != NULL) fclose(fp);
        fp = NULL;
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("aimPreanalysis status = %d\n", status);

        if (fp != NULL) fclose(fp);

        if (filename != NULL) EG_free(filename);

        if (faces != NULL) EG_free(faces);

        if (meshProp != NULL) {

            for (i = 0; i < numMeshProp; i++) {

                (void) destroy_meshSizingStruct(&meshProp[i]);
            }

            EG_free(meshProp);
        }

        return status;
}

int aimOutputs( int iIndex, void *aimStruc, int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsDelaundo AIM Outputs
     * Delaundo only has one available output, "Mesh", which triggers the AIM to read the generated mesh file back into CAPS.
     * Once read the mesh may be shared with other AIMs and/or written out in a specified mesh format.
     */

#ifdef DEBUG
    printf(" delaundoAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
#endif

    if( index == 1) {

        *aoname = EG_strdup("Mesh");
    }

    form->type = Boolean;
    form->vals.integer = (int) false;

    return CAPS_SUCCESS;
}

// See if a surface mesh was generated for each body
int aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath, int index, capsValue *val, capsErrs **errors)
{
    int status; // Function status return

    int i, j, surf; // Indexing

    int numEdge = 0, numEdgePoints = 0, edgeIndex; // Boundary edge counter index
    int numTriFace = 0;
    int elementIndex = 0;

    // Current directory path
    char currentPath[PATH_MAX];

    char *filename = NULL;
    char fileExt[] = ".mesh";
    char bodyNumber[11];

    int stringLength = 0;

    int tempInteger1, tempInteger2, tempInteger3, tempInteger4, tempInteger5;
    double tempDouble1, tempDouble2, tempDouble3, tempDouble4;

    double xyz[3] = {0.0, 0.0, 0.0};
    // FILE - IO
    FILE *fp = NULL;

    size_t linecap = 0;

    char *line = NULL;

#ifdef DEBUG
    printf(" delaundoAIM/aimCalcOutput instance = %d  index = %d!\n", inst, index);
#endif

    *errors           = NULL;
    val->vals.integer = (int) false;

    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(analysisPath) != 0) {
#ifdef DEBUG
        printf(" Cannot chdir to %s!\n", analysisPath);
#endif

        return CAPS_DIRERR;
    }

    stringLength  = strlen(delaundoInstance[iIndex].meshInput.outputFileName) + strlen(fileExt);

    filename = (char *) EG_alloc( (stringLength + 1)*sizeof(char));
    if (filename == NULL) return EGADS_MALLOC;

    sprintf(filename, "%s%s", delaundoInstance[iIndex].meshInput.outputFileName,fileExt);
    filename[stringLength] = '\0';

    printf("Reading delaundo mesh file - %s\n", filename);

    // Lets read our mesh back in
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("\tUnable to open file - %s\n", filename);
        chdir(currentPath);
        EG_free(filename);
        return CAPS_IOERR;
    }

    chdir(currentPath);
    if (filename != NULL) EG_free(filename);
    filename = NULL;

    // Check to see if surface meshes was generated
    for (surf = 0; surf < delaundoInstance[iIndex].numSurface; surf++ ) {

        // Cleanup surface mesh
        status = destroy_meshStruct(&delaundoInstance[iIndex].surfaceMesh[surf]);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) goto cleanup;

        fscanf(fp, "%d %d %d", &numTriFace,
                &tempInteger1, &tempInteger2);

        if (numTriFace == 0) {
            val->vals.integer = (int) false;
            printf("\tNo surface Tris were generated for surface - %d\n", surf);
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        delaundoInstance[iIndex].surfaceMesh[surf].meshType = Surface2DMesh;
        delaundoInstance[iIndex].surfaceMesh[surf].meshQuickRef.numTriangle = numTriFace;
        delaundoInstance[iIndex].surfaceMesh[surf].meshQuickRef.startIndexTriangle = 0;
        delaundoInstance[iIndex].surfaceMesh[surf].meshQuickRef.useStartIndex = (int) true;

        // Triangle elements
        delaundoInstance[iIndex].surfaceMesh[surf].numElement = numTriFace;
        delaundoInstance[iIndex].surfaceMesh[surf].element = (meshElementStruct *) EG_alloc(numTriFace*sizeof(meshElementStruct));

        // Initiate triangle elements
        for (i = 0; i < numTriFace; i++) {
            status = initiate_meshElementStruct(&delaundoInstance[iIndex].surfaceMesh[surf].element[i], UnknownMeshAnalysis);
            if (status != CAPS_SUCCESS) goto cleanup;

            delaundoInstance[iIndex].surfaceMesh[surf].element[i].elementType = Triangle;

            status = mesh_allocMeshElementConnectivity(&delaundoInstance[iIndex].surfaceMesh[surf].element[i]);
            if (status != CAPS_SUCCESS) goto cleanup;

        }

        for (i = 0; i < numTriFace; i++ ) {
            fscanf(fp, "%d %d %d %d %d %d %d %d", &tempInteger1,
                                                  &delaundoInstance[iIndex].surfaceMesh[surf].element[i].connectivity[0],
                                                  &delaundoInstance[iIndex].surfaceMesh[surf].element[i].connectivity[1],
                                                  &delaundoInstance[iIndex].surfaceMesh[surf].element[i].connectivity[2],
                                                  &tempInteger2,
                                                  &tempInteger3,
                                                  &tempInteger4,
                                                  &tempInteger5);

            delaundoInstance[iIndex].surfaceMesh[surf].element[i].elementID = i+1;
            delaundoInstance[iIndex].surfaceMesh[surf].element[i].markerID = delaundoInstance[iIndex].faceAttr;
        }

        // Nodes
        fscanf(fp, "%d", &delaundoInstance[iIndex].surfaceMesh[surf].numNode);

        delaundoInstance[iIndex].surfaceMesh[surf].node = (meshNodeStruct *) EG_alloc(delaundoInstance[iIndex].surfaceMesh[surf].numNode
                                                                                      *sizeof(meshNodeStruct));

        // Initiate nodes
        for (i = 0; i < delaundoInstance[iIndex].surfaceMesh[surf].numNode; i++) {
            status = initiate_meshNodeStruct(&delaundoInstance[iIndex].surfaceMesh[surf].node[i], UnknownMeshAnalysis);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        //printf("Number of Nodes = %d\n",delaundoInstance[iIndex].surfaceMesh[surf].numNode);

        status = getline(&line, &linecap, fp);
        if (status < 0) goto cleanup;

        status = getline(&line, &linecap, fp);
        if (status < 0) goto cleanup;

        for (i = 0; i < delaundoInstance[iIndex].surfaceMesh[surf].numNode; i++) {

            fscanf(fp, "%lf %lf %lf %lf %lf %lf %d", &xyz[0],
                                                     &xyz[1],
                                                     &tempDouble1, &tempDouble2, &tempDouble3, &tempDouble4,
                                                     &tempInteger1);

            if (delaundoInstance[iIndex].swapZX ==  (int) true) {
                // z = x, x = 0.0
                if (i == 0) printf("\tSwapping x and z coordinates!\n");
                xyz[2] = xyz[0];
                xyz[0] = 0.0;
            }

            if (delaundoInstance[iIndex].swapZY == (int) true) {
                // z = y, y = 0.0
                if (i == 0) printf("\tSwapping y and z coordinates!\n");
                xyz[2] = xyz[1];
                xyz[1] = 0.0;
            }

            delaundoInstance[iIndex].surfaceMesh[surf].node[i].xyz[0] = xyz[0];
            delaundoInstance[iIndex].surfaceMesh[surf].node[i].xyz[1] = xyz[1];
            delaundoInstance[iIndex].surfaceMesh[surf].node[i].xyz[2] = xyz[2];

            delaundoInstance[iIndex].swapZX = (int) false;
        }

        status = getline(&line, &linecap, fp);
        if (status < 0) goto cleanup;

        status = getline(&line, &linecap, fp);
        if (status < 0) goto cleanup;

        // Get edges
        sscanf(line, "%d", &numEdge);

        //printf("Number of Edges = %d\n", numEdge);
        for (i = 0; i < numEdge; i++) {

            status = getline(&line, &linecap, fp);
            if (status < 0) goto cleanup;
            sscanf(line, "%d %d", &numEdgePoints, &edgeIndex);

            //printf("Number of edgePoints = %d\n", numEdgePoints);
            for (j = 0; j < numEdgePoints; j++) {

                delaundoInstance[iIndex].surfaceMesh[surf].numElement += 1;
                delaundoInstance[iIndex].surfaceMesh[surf].element = (meshElementStruct *) EG_reall(delaundoInstance[iIndex].surfaceMesh[surf].element,
                                                                                                    delaundoInstance[iIndex].surfaceMesh[surf].numElement
                                                                                                    *sizeof(meshElementStruct));
                elementIndex = delaundoInstance[iIndex].surfaceMesh[surf].numElement-1;

                status = initiate_meshElementStruct(&delaundoInstance[iIndex].surfaceMesh[surf].element[elementIndex],
                        UnknownMeshAnalysis);
                if (status != CAPS_SUCCESS) goto cleanup;

                delaundoInstance[iIndex].surfaceMesh[surf].element[elementIndex].elementType = Line;

                status = mesh_allocMeshElementConnectivity(&delaundoInstance[iIndex].surfaceMesh[surf].element[elementIndex]);
                if (status != CAPS_SUCCESS) goto cleanup;

                fscanf(fp, "%d %d %d %d", &delaundoInstance[iIndex].surfaceMesh[surf].element[elementIndex].connectivity[0],
                                          &delaundoInstance[iIndex].surfaceMesh[surf].element[elementIndex].connectivity[1],
                                          &tempInteger1,
                                          &tempInteger2);

                delaundoInstance[iIndex].surfaceMesh[surf].element[elementIndex].elementID = elementIndex+1;
                delaundoInstance[iIndex].surfaceMesh[surf].element[elementIndex].markerID = delaundoInstance[iIndex].edgeAttrMap[edgeIndex-1];
                delaundoInstance[iIndex].surfaceMesh[surf].meshQuickRef.numLine +=1;
                delaundoInstance[iIndex].surfaceMesh[surf].meshQuickRef.startIndexLine = delaundoInstance[iIndex].surfaceMesh[surf].meshQuickRef.numTriangle;
            }

            status = getline(&line, &linecap, fp); // Get blank line character
            if (status < 0) goto cleanup;
        }

    }

    if (delaundoInstance[iIndex].meshInput.outputFormat != NULL &&
        delaundoInstance[iIndex].meshInput.outputFileName != NULL) {

        if (filename != NULL) EG_free(filename);
        filename = NULL;

        for (surf = 0; surf < delaundoInstance[iIndex].numSurface; surf++) {

            if (delaundoInstance[iIndex].numSurface > 1) {
                sprintf(bodyNumber, "%d", surf);
                filename = (char *) EG_alloc((strlen(delaundoInstance[iIndex].meshInput.outputFileName)  +
                                              strlen(delaundoInstance[iIndex].meshInput.outputDirectory) +
                                              2 +
                                              strlen("_Surf_") +
                                              strlen(bodyNumber))*sizeof(char));
            } else {
                filename = (char *) EG_alloc((strlen(delaundoInstance[iIndex].meshInput.outputFileName) +
                                              strlen(delaundoInstance[iIndex].meshInput.outputDirectory)+2)*sizeof(char));

            }

            if (filename == NULL) return EGADS_MALLOC;

            strcpy(filename, delaundoInstance[iIndex].meshInput.outputDirectory);
#ifdef WIN32
            strcat(filename, "\\");
#else
            strcat(filename, "/");
#endif
            strcat(filename, delaundoInstance[iIndex].meshInput.outputFileName);

            if (delaundoInstance[iIndex].numSurface > 1) {
                strcat(filename,"_Surf_");
                strcat(filename, bodyNumber);
            }


            if (strcasecmp(delaundoInstance[iIndex].meshInput.outputFormat, "AFLR3") == 0) {

                status = mesh_writeAFLR3(filename,
                                         delaundoInstance[iIndex].meshInput.outputASCIIFlag,
                                         &delaundoInstance[iIndex].surfaceMesh[surf],
                                         1.0);

            } else if (strcasecmp(delaundoInstance[iIndex].meshInput.outputFormat, "VTK") == 0) {

                status = mesh_writeVTK(filename,
                                       delaundoInstance[iIndex].meshInput.outputASCIIFlag,
                                       &delaundoInstance[iIndex].surfaceMesh[surf],
                                       1.0);

            } else if (strcasecmp(delaundoInstance[iIndex].meshInput.outputFormat, "Tecplot") == 0) {

                status = mesh_writeTecplot(filename,
                                           delaundoInstance[iIndex].meshInput.outputASCIIFlag,
                                           &delaundoInstance[iIndex].surfaceMesh[surf],
                                           1.0);

            } else if (strcasecmp(delaundoInstance[iIndex].meshInput.outputFormat, "STL") == 0) {

                status = mesh_writeSTL(filename,
                                       delaundoInstance[iIndex].meshInput.outputASCIIFlag,
                                       &delaundoInstance[iIndex].surfaceMesh[surf],
                                       1.0);

            } else {
                printf("Unrecognized mesh format, \"%s\", the mesh will not be written out\n", delaundoInstance[iIndex].meshInput.outputFormat);
                status = CAPS_NOTFOUND;
            }

            if (filename != NULL) EG_free(filename);
            filename = NULL;

            if (status != CAPS_SUCCESS) goto cleanup;
        }
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (line != NULL) EG_free(line);
        if (filename != NULL) EG_free(filename);

        return status;
}


void aimCleanup()
{
    int iIndex; // Indexing

    int status; // Returning status

#ifdef DEBUG
    printf(" delaundoAIM/aimFreeDiscr w/ NULL!\n");
#endif


    // Clean up delaundoInstance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up delaundoInstance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS) printf("Status = %d, delaundoAIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
    }

    if (delaundoInstance != NULL) EG_free(delaundoInstance);
    delaundoInstance = NULL;
    numInstance = 0;

}
