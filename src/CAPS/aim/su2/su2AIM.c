/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             SU2 AIM
 *
 *     Modified from code written by Dr. Ryan Durscher AFRL/RQVC
 *
 */


/*!\mainpage Introduction
 *
 * \section overviewSU2 SU2 AIM Overview
 * This module can be used to interface with the open-source CFD code, SU2 \cite Palacios2013 \cite Palacios2014
 *  with geometry in the CAPS system. For SU2 capabilities and related documentation, please refer to
 * http://su2.stanford.edu/. SU2 expects a volume mesh file and a corresponding
 * configuration file to perform the analysis.
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsSU2 and \ref aimOutputsSU2, respectively.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentSU2.
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataSU2 if connecting this AIM to other AIMs
 * in a parent-child like manner.
 *
 * Details of the AIM's automated data transfer capabilities are outlined in \ref dataTransferSU2
 *
 * \subsection meshSU2 Automatic generation of SU2 Mesh file
 * The volume mesh file from SU2 AIM is written in native SU2
 * format ("filename.su2"). The description of the native SU2 mesh can be
 * found SU2 website. For the automatic generation of mesh file, SU2 AIM
 * depends on Mesh AIMs, for example, TetGen or AFLR4/3 AIM.
 *
 * \subsection configSU2 Automatic generation of SU2 Configuration file
 * The configuration file ("filename.cfg") from SU2 AIM is automatically
 * created by using the flow features and boundary conditions that were set in
 * the driver program as a user input. For the rest of the configuration
 * variables, default set of values are provided for a general execution.
 * If desired, a user has freedom to manually (a) change these variables based
 * on  personal preference, or (b) override the configuration file with unique
 * configuration variables.
 *
 * \section su2examples SU2 Examples
 *  Here is an example that illustrated use of SU2 AIM \ref su2Example. Note
 *  this AIM uses TetGen AIM for volume mesh generation.
 *
 */

#ifdef HAVE_PYTHON
#include "Python.h" // Bring in Python API
#endif

#include <string.h>
#include <ctype.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"
#include "cfdUtils.h"
#include "miscUtils.h"
#include "su2Utils.h"


#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define NUMINPUT   31
#define NUMOUTPUT  3*9

#define MXCHAR  255

//#define DEBUG

typedef struct {

    // SU2 project name
    char *projectName;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

    // Check to make sure data transfer is ok
    int dataTransferCheck;

    // Analysis file path/directory
    const char *analysisPath;

    // Point to caps input value for version of Su2
    capsValue *su2Version;

    // Pointer to caps input value for scaling pressure during data transfer
    capsValue *pressureScaleFactor;

    // Pointer to caps input value for offset pressure during data transfer
    capsValue *pressureScaleOffset;

} aimStorage;

static aimStorage *su2Instance = NULL;
static int         numInstance  = 0;



//#include "su2DataExchange.c"

/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(/*@unused@*/ int ngIn, /*@unused@*/ /*@null@*/ capsValue *gIn,
                  int *qeFlag, /*@null@*/ const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int *ints;
    int flag;
    int status; // Function return

    char **strs;

    aimStorage *tmp;

    #ifdef DEBUG
        printf("\n su2AIM/aimInitialize   ngIn = %d!\n", ngIn);
    #endif
    flag     = *qeFlag;
    *qeFlag  = 0;

    /* specify the number of analysis input and out "parameters" */
    *nIn     = NUMINPUT;
    *nOut    = NUMOUTPUT;
    if (flag == 1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate */
    *nFields = 4;
    ints     = (int *) EG_alloc(*nFields*sizeof(int));
    if (ints == NULL) return EGADS_MALLOC;

    ints[0]  = 1;
    ints[1]  = 1;
    ints[2]  = 1;
    ints[3]  = 1;
    *ranks   = ints;

    strs     = (char **) EG_alloc(*nFields*sizeof(char *));
    if (strs == NULL) {
        EG_free(*ranks);
        *ranks   = NULL;
        return EGADS_MALLOC;
    }

    strs[0]  = EG_strdup("Pressure");
    strs[1]  = EG_strdup("P");
    strs[2]  = EG_strdup("Cp");
    strs[3]  = EG_strdup("CoefficientofPressure");
    *fnames  = strs;

    // Allocate su2Instance
    if (su2Instance == NULL) {
        su2Instance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (su2Instance == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }
    } else {
        tmp = (aimStorage *) EG_reall(su2Instance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }
        su2Instance = tmp;
    }

    // Set initial values for su2Instance
    su2Instance[numInstance].projectName = NULL;

    // Analysis file path/directory
    su2Instance[numInstance].analysisPath = NULL;

    // Version
    su2Instance[numInstance].su2Version = NULL;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&su2Instance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Check to make sure data transfer is ok
    su2Instance[numInstance].dataTransferCheck = (int) true;

    // Pointer to caps input value for scaling pressure during data transfer
    su2Instance[numInstance].pressureScaleFactor = NULL;

    // Pointer to caps input value for off setting pressure during data transfer
    su2Instance[numInstance].pressureScaleOffset = NULL;

    // Increment number of instances
    numInstance += 1;

    return (numInstance -1);
}

int aimInputs(int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{
    #ifdef DEBUG
        printf(" su2AIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
    #endif

    *ainame = NULL;

    // SU2 Inputs
    /*! \page aimInputsSU2 AIM Inputs
     * For the description of the configuration variables, associated values,
     * and available options refer to the template configuration file that is
     * distributed with SU2.
     * Note: The configuration file is dependent on the version of SU2 used.
     * This configuration file that will be auto generated is compatible with
     * SU2 4.1.1. (Cardinal), 5.0.0 (Raven), 6.2.0 (Falcon - Default)
     */

    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("su2_CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsSU2
         * - <B> Proj_Name = "su2_CAPS"</B> <br>
         * This corresponds to the project "root" name.
         */

    } else if (index == 2) {
        *ainame              = EG_strdup("Mach"); // Mach number
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.8;

        /*! \page aimInputsSU2
         * - <B> Mach = 0.8</B> <br>
         * Mach number; this corresponds to the MACH_NUMBER keyword in the configuration file.
         */

    } else if (index == 3) {
        *ainame              = EG_strdup("Re"); // Reynolds number
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 6.5E6;

        /*! \page aimInputsSU2
         * - <B> Re = 6.5E6</B> <br>
         * Reynolds number; this corresponds to the REYNOLDS_NUMBER keyword in the configuration file.
         */

    } else if (index == 4) {
        *ainame              = EG_strdup("Physical_Problem");
        defval->type         = String;
        defval->vals.string  = NULL;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;
        defval->vals.string  = EG_strdup("Euler");

        /*! \page aimInputsSU2
         * - <B> Physical_Problem = "Euler"</B> <br>
         * Physical problem type; this corresponds to the PHYSICAL_PROBLEM keyword in the configuration file.
         * Options: Euler, Navier_Stokes, Wave_Equation, ... see SU2 template for additional options.
         */

    } else if (index == 5) {
        *ainame              = EG_strdup("Equation_Type"); // Equation type
        defval->type         = String;
        defval->vals.string  = NULL;
        defval->nullVal      = NotNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.string  = EG_strdup("Compressible");

        /*! \page aimInputsSU2
         * - <B> Equation_Type = "Compressible"</B> <br>
         * Equation regime type; this corresponds to the REGIME_TYPE keyword in the configuration file.
         * Options: Compressible or Incompressible.
         */
    } else if (index == 6) {
        *ainame              = EG_strdup("Alpha");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;
        defval->units        = EG_strdup("degree");

        /*! \page aimInputsSU2
         * - <B> Alpha = 0.0</B> <br>
         * Angle of attack [degree]; this corresponds to the AoA keyword in the configuration file.
         */
    } else if (index == 7) {
        *ainame              = EG_strdup("Beta");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;
        defval->units        = EG_strdup("degree");

        /*! \page aimInputsSU2
         * - <B> Beta = 0.0</B> <br>
         * Side slip angle [degree]; this corresponds to the SIDESLIP_ANGLE keyword in the configuration file.
         */

    } else if (index == 8) {
        *ainame              = EG_strdup("Overwrite_CFG");
        defval->type         = Boolean;
        defval->vals.integer = (int) true;
        defval->nullVal      = NotNull;

        /*! \page aimInputsSU2
         * - <B> Overwrite_CFG = True</B> <br>
         * Provides permission to overwrite configuration file. If set to False a new configuration file won't be generated.
         */

    } else if (index == 9) {
        *ainame              = EG_strdup("Num_Iter");
        defval->type         = Integer;
        defval->nullVal      = NotNull;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.integer = 9999;

        /*! \page aimInputsSU2
         * - <B> Num_Iter = 9999</B> <br>
         * Number of total iterations; this corresponds to the EXT_ITER keyword in the configuration file.
         */

    } else if (index == 10) {
        *ainame              = EG_strdup("CFL_Number");
        defval->type         = Double;
        defval->nullVal      = NotNull;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;
        defval->vals.real    = 10.0;

        /*! \page aimInputsSU2
         * - <B> CFL_Number = 10.0</B> <br>
         *  Courant–Friedrichs–Lewy number; this corresponds to the CFL_NUMBER keyword in the configuration file.
         */

    } else if (index == 11) {
        *ainame              = EG_strdup("Boundary_Condition");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsSU2
         * - <B>Boundary_Condition = NULL </B> <br>
         * See \ref cfdBoundaryConditions for additional details.
         */
    } else if (index == 12) {
        *ainame              = EG_strdup("MultiGrid_Level");
        defval->type         = Integer;
        defval->vals.integer = 2;
        defval->units        = NULL;
        defval->dim          = Scalar;

        /*! \page aimInputsSU2
         * - <B> MultiGrid_Level = 2</B> <br>
         *  Number of multi-grid levels; this corresponds to the MGLEVEL keyword in the configuration file.
         */

    } else if (index == 13) {
        *ainame              = EG_strdup("Residual_Reduction");
        defval->type         = Integer;
        defval->vals.integer = 6;
        defval->units        = NULL;
        defval->dim          = Scalar;

        /*! \page aimInputsSU2
         * - <B> Residual_Reduction = 6</B> <br>
         *  Residual reduction (order of magnitude with respect to the initial value);
         *  this corresponds to the RESIDUAL_REDUCTION keyword in the configuration file.
         */

    } else if (index == 14) {
        *ainame              = EG_strdup("Unit_System");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("SI");
        defval->lfixed       = Change;

        /*! \page aimInputsSU2
         * - <B> Unit_System = "SI"</B> <br>
         * Measurement unit system; this corresponds to the SYSTEM_MEASUREMENTS keyword in the configuration file. See SU2
         * template for additional details.
         */

    } else if (index == 15) {
        *ainame              = EG_strdup("Reference_Dimensionalization");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Dimensional");
        defval->lfixed       = Change;

        /*! \page aimInputsSU2
         * - <B> Reference_Dimensionalization = NULL</B> <br>
         * Reference dimensionalization; this corresponds to the REF_DIMENSIONALIZATION keyword in the configuration file. See SU2
         * template for additional details.
         */

    } else if (index == 16) {
        *ainame              = EG_strdup("Freestream_Pressure");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;

        /*! \page aimInputsSU2
         * - <B> Freestream_Pressure = NULL</B> <br>
         * Freestream reference pressure; this corresponds to the FREESTREAM_PRESSURE keyword in the configuration file. See SU2
         * template for additional details.
         */

    } else if (index == 17) {
        *ainame              = EG_strdup("Freestream_Temperature");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;

        /*! \page aimInputsSU2
         * - <B> Freestream_Temperature = NULL</B> <br>
         * Freestream reference temperature; this corresponds to the FREESTREAM_TEMPERATURE keyword in the configuration file. See SU2
         * template for additional details.
         */
    } else if (index == 18) {
        *ainame              = EG_strdup("Freestream_Density");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;

        /*! \page aimInputsSU2
         * - <B> Freestream_Density = NULL</B> <br>
         * Freestream reference density; this corresponds to the FREESTREAM_DENSITY keyword in the configuration file. See SU2
         * template for additional details.
         */
    } else if (index == 19) {
        *ainame              = EG_strdup("Freestream_Velocity");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;

        /*! \page aimInputsSU2
         * - <B> Freestream_Velocity = NULL</B> <br>
         * Freestream reference velocity; this corresponds to the FREESTREAM_VELOCITY keyword in the configuration file. See SU2
         * template for additional details.
         */
    } else if (index == 20) {
        *ainame              = EG_strdup("Freestream_Viscosity");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Fixed;
        defval->dim          = Scalar;

        /*! \page aimInputsSU2
         * - <B> Freestream_Viscosity = NULL</B> <br>
         * Freestream reference viscosity; this corresponds to the FREESTREAM_VISCOSITY keyword in the configuration file. See SU2
         * template for additional details.
         */
    } else if (index == 21) {
        *ainame              = EG_strdup("Moment_Center");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->length        = 3;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals == NULL) {
            return EGADS_MALLOC;
        } else {
            defval->vals.reals[0] = 0.0;
            defval->vals.reals[1] = 0.0;
            defval->vals.reals[2] = 0.0;
        }
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;

        /*! \page aimInputsSU2
         * - <B>Moment_Center = NULL, [0.0, 0.0, 0.0]</B> <br>
         * Array values correspond to the x_moment_center, y_moment_center, and z_moment_center variables; which correspond
         * to the REF_ORIGIN_MOMENT_X, REF_ORIGIN_MOMENT_Y, and REF_ORIGIN_MOMENT_Z variables respectively in the SU2
         * configuration script.
         * Alternatively, the geometry (body) attributes "capsReferenceX", "capsReferenceY",
         * and "capsReferenceZ" may be used to specify the x-, y-, and z- moment centers, respectively
         * (note: values set through the AIM input will supersede the attribution values).
         */
    } else if (index == 22) {
        *ainame              = EG_strdup("Moment_Length");
        defval->type          = Double;
        defval->dim           = Scalar;
        defval->length        = 1;
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;
        defval->vals.real     = 1.0;

        /*! \page aimInputsSU2
         * - <B>Moment_Length = NULL, 1.0</B> <br>
         * Reference length for pitching, rolling, and yawing non-dimensional; which correspond
         * to the REF_LENGTH_MOMENT. Alternatively, the geometry (body) attribute "capsReferenceSpan" may be
         * used to specify the x-, y-, and z- moment lengths, respectively (note: values set through
         * the AIM input will supersede the attribution values).
         */
    } else if (index == 23) {
        *ainame              = EG_strdup("Reference_Area");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;
        defval->vals.real    = 1.0;

        /*! \page aimInputsSU2
         * - <B>Reference_Area = NULL </B> <br>
         * This sets the reference area for used in force and moment calculations;
         * this corresponds to the REF_AREA keyword in the configuration file.
         * Alternatively, the geometry (body) attribute "capsReferenceArea" maybe used to specify this variable
         * (note: values set through the AIM input will supersede the attribution value).
         */
    } else if (index == 24) {
        *ainame              = EG_strdup("Pressure_Scale_Factor");
        defval->type         = Double;
        defval->vals.real    = 1.0;
        defval->units        = NULL;

        su2Instance[iIndex].pressureScaleFactor = defval;

        /*! \page aimInputsSU2
         * - <B>Pressure_Scale_Factor = 1.0</B> <br>
         * Value to scale Cp or Pressure data when transferring data. Data is scaled based on Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset.
         */
    } else if (index == 25) {
        *ainame              = EG_strdup("Pressure_Scale_Offset");
        defval->type         = Double;
        defval->vals.real    = 0.0;
        defval->units        = NULL;

        su2Instance[iIndex].pressureScaleOffset = defval;

        /*! \page aimInputsSU2
         * - <B>Pressure_Scale_Offset = 0.0</B> <br>
         * Value to offset Cp or Pressure data when transferring data.
         * Data is scaled based on Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset.
         */

    } else if (index == 26) {
        *ainame              = EG_strdup("Output_Format");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Paraview");
        defval->lfixed       = Change;

        /*! \page aimInputsSU2
         * - <B> Output_Format = "Paraview"</B> <br>
         * Output file format; this corresponds to the OUTPUT_FORMAT keyword in the configuration file. See SU2
         * template for additional details.
         */

    }  else if (index == 27) {
        *ainame              = EG_strdup("Two_Dimensional");
        defval->type         = Boolean;
        defval->type         = Boolean;
        defval->vals.integer = (int) false;

        /*! \page aimInputsSU2
         * - <B>Two_Dimensional = False</B> <br>
         * Run SU2 in 2D mode.
         */

    } else if (index == 28) {
        *ainame              = EG_strdup("Convective_Flux");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Roe");
        defval->lfixed       = Change;

        /*! \page aimInputsSU2
         * - <B> Convective_Flux = "Roe"</B> <br>
         * Numerical method for convective (inviscid) flux construction; this corresponds to the CONV_NUM_METHOD_FLOW keyword in the configuration file. See SU2
         * template for additional details.
         */


    } else if (index == 29) {
        *ainame              = EG_strdup("SU2_Version");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("Falcon");
        defval->lfixed       = Change;

        /*! \page aimInputsSU2
         * - <B>SU2_Version = "Falcon"</B> <br>
         * SU2 version to generate specific configuration file. Options: "Cardinal(4.0)", "Raven(5.0)" or "Falcon(6.2)".
         */

        su2Instance[iIndex].su2Version = defval;

    } else if (index == 30) {
      *ainame             = EG_strdup("Surface_Monitor");
      defval->type        = String;
      defval->vals.string = NULL;
      defval->nullVal     = IsNull;

      /*! \page aimInputsSU2
       * - <B>Surface_Monitor = NULL</B> <br>
       * Array of surface names where the non-dimensional coefficients are evaluated
       */
    } else if (index == 31) {
      *ainame             = EG_strdup("Surface_Deform");
      defval->type        = String;
      defval->vals.string = NULL;
      defval->nullVal     = IsNull;

      /*! \page aimInputsSU2
       * - <B>Surface_Deform = NULL</B> <br>
       * Array of surface names that should be deformed. Defaults to all invisid and viscous surfaces.
       */
    }


#if NUMINPUT != 31
#error "NUMINPUTS is inconsistent with the list of inputs"
#endif

// Link variable(s) to parent(s) if available
//	if ((index != 1) && (*ainame != NULL) && (index !=15)) {
//           status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
/*	    printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n",
                   status, index, defval->type, defval->link);  */
//	}

    return CAPS_SUCCESS;
}

int aimData(/*@unused@*/ int iIndex,
            /*@unused@*/ const char *name,
            /*@unused@*/ enum capsvType *vtype,
            /*@unused@*/ int *rank,
            /*@unused@*/ int *nrow,
            /*@unused@*/ int *ncol,
            /*@unused@*/ void **data,
            /*@unused@*/ char **units)
{

    /*! \page sharableDataSU2 AIM Shareable Data
     * Currently the SU2 AIM does not have any shareable data types or values. It will try, however, to inherit a
     * "Volume_Mesh" (for 3D simulations) or a "Surface_Mesh" (for 2D simulations) from any parent AIMs.
     * Note that the inheritance of the volume/surface mesh variable is required
     * if the SU2 aim is to write a suitable grid file.
     */

    #ifdef DEBUG
        printf(" su2AIM/aimData instance = %d  name = %s!\n", iIndex, name);
    #endif
   return CAPS_NOTFOUND;
}

int aimPreAnalysis(int iIndex, void *aimInfo, const char *analysisPath,
                   capsValue *aimInputs, capsErrs **errs)
{
    // Function return flag
    int status;

    // Python linking
    int pythonLinked = (int) false;
    int usePython = (int) false;

    // Indexing
    int i, body;

    // Data transfer
    int  nrow, ncol, rank;
    void *dataTransfer = NULL;
    enum capsvType vtype;
    char *units = NULL;

    // EGADS return values
    int          atype, alen;
    const int    *ints;
    const char   *string, *intents;
    const double *reals;

    // Output filename
    char *filename = NULL;

    // File pointer if we are going to internally write the namelist
    // FILE *fnml;

    // AIM input bodies
    int  numBody;
    ego *bodies = NULL;
    int  bodySubType = 0, bodyOClass, bodyNumChild, *bodySense;
    ego  bodyRef, *bodyChild;
    double bodyData[4];

    // Boundary/surface properties
    cfdBCsStruct bcProps;

    // Boundary conditions container - for writing .mapbc file
    bndCondStruct bndConds;

    // Volume Mesh obtained from meshing AIM
    meshStruct *volumeMesh = NULL;
    int numElementCheck; // Consistency checkers for volume-surface meshes

    // Discrete data transfer variables
    char **transferName = NULL;
    int numTransferName;

    int xMeshConstant = (int) true, yMeshConstant = (int) true, zMeshConstant= (int) true; // 2D mesh checks
    double tempCoord;
    int withMotion = (int) false;

    // NULL out errors
    *errs = NULL;

    // Store away the analysis path/directory
    su2Instance[iIndex].analysisPath = analysisPath;

    status = initiate_bndCondStruct(&bndConds);
    if (status != CAPS_SUCCESS) return status;

    // Get boundary conditions
    status = initiate_cfdBCsStruct(&bcProps);
    if (status != CAPS_SUCCESS) return status;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) printf("aim_getBodies status = %d!!\n", status);

    #ifdef DEBUG
        printf(" su2AIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, numBody);
    #endif

    if ((numBody <= 0) || (bodies == NULL)) {
        #ifdef DEBUG
            printf(" su2AIM/aimPreAnalysis No Bodies!\n");
        #endif
        return CAPS_SOURCEERR;
    }

    // Get reference quantities from the bodies
    for (body=0; body < numBody; body++) {

        if (aimInputs[aim_getIndex(aimInfo, "Reference_Area", ANALYSISIN)-1].nullVal == IsNull) {
            status = EG_attributeRet(bodies[body], "capsReferenceArea", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Reference_Area", ANALYSISIN)-1].vals.real = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Reference_Area", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceArea should be followed by a single real value!\n");
                }
            }
        }

        if (aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].nullVal == IsNull) {
            status = EG_attributeRet(bodies[body], "capsReferenceSpan", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS){

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].vals.real = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceSpan should be followed by a single real value!\n");
                }
            }
        }

        if (aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].nullVal == IsNull) {

            status = EG_attributeRet(bodies[body], "capsReferenceX", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[0] = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceX should be followed by a single real value!\n");
                }
            }

            status = EG_attributeRet(bodies[body], "capsReferenceY", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[1] = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceY should be followed by a single real value!\n");
                }
            }

            status = EG_attributeRet(bodies[body], "capsReferenceZ", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[2] = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceZ should be followed by a single real value!\n");
                }
            }
        }

    }

    // Check to see if python was linked
    #ifdef HAVE_PYTHON
        pythonLinked = (int) true;
    #else
        pythonLinked = (int) false;
    #endif

    // Should we use python even if it was linked? - Not implemented
    /*usePython = aimInputs[aim_getIndex(aimInfo, "Use_Python_NML", ANALYSISIN)-1].vals.integer;
    if (usePython == (int) true && pythonLinked == (int) false) {

        printf("Use of Python library requested but not linked!\n");
        usePython = (int) false;

    } else if (usePython == (int) false && pythonLinked == (int) true) {

        printf("Python library was linked, but will not be used!\n");
    }
    */

    // Get project name
    su2Instance[iIndex].projectName = aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string;

    // Check intent
/* Ryan -- please fix
    status = check_CAPSIntent(numBody, bodies, &currentIntent);
    if (status != CAPS_SUCCESS) goto cleanup;

    if(currentIntent != CFD) {
        printf("All bodies must have the same capsIntent of CFD!\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    } */

    // Get attribute to index mapping
    if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {
        status = aim_getData(aimInfo, "Attribute_Map", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status == CAPS_SUCCESS) {

            printf("Found link for attrMap (Attribute_Map) from parent\n");

            status = copy_mapAttrToIndexStruct( (mapAttrToIndexStruct *) dataTransfer, &su2Instance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) return status;
        } else {

            if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
            else printf("Linking status during attrMap (Attribute_Map) = %d\n",status);

            printf("Didn't find a link to attrMap from parent - getting it ourselves\n");

            // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
            status = create_CAPSGroupAttrToIndexMap(numBody,
                                                    bodies,
                                                    1, // Only search down to the face level of the EGADS body
                                                    &su2Instance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) return status;
        }
    }

    // Get boundary conditions - if the boundary condition has been set
    if (aimInputs[aim_getIndex(aimInfo, "Boundary_Condition", ANALYSISIN)-1].nullVal ==  NotNull) {

        status = cfd_getBoundaryCondition( aimInputs[aim_getIndex(aimInfo, "Boundary_Condition", ANALYSISIN)-1].length,
                                           aimInputs[aim_getIndex(aimInfo, "Boundary_Condition", ANALYSISIN)-1].vals.tuple,
                                           &su2Instance[iIndex].attrMap,
                                           &bcProps);
        if (status != CAPS_SUCCESS) goto cleanup;

    } else {
        printf("Warning: No boundary conditions provided !!!!\n");
    }

    // Are we running in two-mode
    if (aimInputs[aim_getIndex(aimInfo, "Two_Dimensional", ANALYSISIN)-1].vals.integer == (int) true ) {

        if (numBody > 1) {
            printf("Only 1 body may be provided when running in two mode!!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        for (body = 0; body < numBody; body++) {
            // What type of BODY do we have?
            status = EG_getTopology(bodies[body], &bodyRef, &bodyOClass, &bodySubType, bodyData, &bodyNumChild, &bodyChild, &bodySense);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (bodySubType != FACEBODY && bodySubType != SHEETBODY) {
                printf("Body type must be either FACEBODY (%d) or a SHEETBODY (%d) when running in two mode!\n", FACEBODY, SHEETBODY);
                status = CAPS_BADTYPE;
                goto cleanup;
            }
        }


        // Get Surface mesh
        status = aim_getData(aimInfo, "Surface_Mesh", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status != CAPS_SUCCESS){

            if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
            else printf("Linking status = %d\n",status);

            printf("Didn't find a link to a surface mesh (Surface_Mesh) from parent - a mesh will NOT be created.\n");

        } else {

            printf("Found link for a surface mesh (Surface_Mesh) from parent\n");

            volumeMesh = (meshStruct *) dataTransfer;

            // Constant x?
            for (i = 0; i < volumeMesh->numNode; i++) {
                if ((volumeMesh->node[i].xyz[0] - volumeMesh->node[0].xyz[0]) > 1E-7) {
                    xMeshConstant = (int) false;
                    break;
                }
            }

            // Constant y?
            for (i = 0; i < volumeMesh->numNode; i++) {
                if ((volumeMesh->node[i].xyz[1] - volumeMesh->node[0].xyz[1] ) > 1E-7) {
                    yMeshConstant = (int) false;
                    break;
                }
            }

            // Constant z?
            for (i = 0; i < volumeMesh->numNode; i++) {
                if ((volumeMesh->node[i].xyz[2]-volumeMesh->node[0].xyz[2]) > 1E-7) {
                    zMeshConstant = (int) false;
                    break;
                }
            }

            if (zMeshConstant != (int) true) {
                printf("SU2 expects 2D meshes be in the x-y plane... attempting to rotate mesh!\n");

                if (xMeshConstant == (int) true && yMeshConstant == (int) false) {
                    printf("Swapping z and x coordinates!\n");
                    for (i = 0; i < volumeMesh->numNode; i++) {
                        tempCoord = volumeMesh->node[i].xyz[0];
                        volumeMesh->node[i].xyz[0] = volumeMesh->node[i].xyz[2];
                        volumeMesh->node[i].xyz[2] = tempCoord;
                    }

                } else if(xMeshConstant == (int) false && yMeshConstant == (int) true) {

                    printf("Swapping z and y coordinates!\n");
                    for (i = 0; i < volumeMesh->numNode; i++) {
                        tempCoord = volumeMesh->node[i].xyz[1];
                        volumeMesh->node[i].xyz[1] = volumeMesh->node[i].xyz[2];
                        volumeMesh->node[i].xyz[2] = tempCoord;
                    }

                } else {
                    printf("Unable to rotate mesh!\n");
                    status = CAPS_NOTFOUND;
                }
            }
        }

        // add boundary elements if they are missing
        if (volumeMesh->meshQuickRef.numLine == 0) {
            status = mesh_addTess2Dbc(volumeMesh, &su2Instance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        // Can't currently do data transfer in 2D-mode
        su2Instance[iIndex].dataTransferCheck = (int) false;
    } else {
        // Get Volume mesh
        status = aim_getData(aimInfo, "Volume_Mesh", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status != CAPS_SUCCESS){

            if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
            else printf("Linking status = %d\n",status);

            printf("Didn't find a link to a volume mesh (Volume_Mesh) from parent - a volume mesh will NOT be created.\n");

            // No volume currently means we can't do datatransfer
            su2Instance[iIndex].dataTransferCheck = (int) false;
        } else {

            printf("Found link for volume mesh (Volume_Mesh) from parent\n");
            volumeMesh = (meshStruct *) dataTransfer;
        }
    }

    if (status == CAPS_SUCCESS) {

        status = populate_bndCondStruct_from_bcPropsStruct(&bcProps, &bndConds);
        if (status != CAPS_SUCCESS) return status;

        // Replace dummy values in bcVal with SU2 specific values (more to add here)
        printf("Writing boundary flags\n");
        for (i = 0; i < bcProps.numBCID ; i++) {
            printf(" - bcProps.surfaceProps[%d].surfaceType = %u\n", i, bcProps.surfaceProps[i].surfaceType);

            // {UnknownBoundary, Inviscid, Viscous, Farfield, Freestream,
            //  BackPressure, Symmetry, SubsonicInflow, SubsonicOutflow}

            if      (bcProps.surfaceProps[i].surfaceType == Inviscid) bndConds.bcVal[i] = 3000;
            else if (bcProps.surfaceProps[i].surfaceType == Viscous)  bndConds.bcVal[i] = 4000;
            else if (bcProps.surfaceProps[i].surfaceType == Farfield) bndConds.bcVal[i] = 5000;
            else if (bcProps.surfaceProps[i].surfaceType == Extrapolate) bndConds.bcVal[i] = 5026;
            else if (bcProps.surfaceProps[i].surfaceType == Freestream)  bndConds.bcVal[i] = 5050;
            else if (bcProps.surfaceProps[i].surfaceType == BackPressure)    bndConds.bcVal[i] = 5051;
            else if (bcProps.surfaceProps[i].surfaceType == SubsonicInflow)  bndConds.bcVal[i] = 7011;
            else if (bcProps.surfaceProps[i].surfaceType == SubsonicOutflow) bndConds.bcVal[i] = 7012;
            else if (bcProps.surfaceProps[i].surfaceType == MassflowIn)      bndConds.bcVal[i] = 7036;
            else if (bcProps.surfaceProps[i].surfaceType == MassflowOut)     bndConds.bcVal[i] = 7031;
            else if (bcProps.surfaceProps[i].surfaceType == FixedInflow)     bndConds.bcVal[i] = 7100;
            else if (bcProps.surfaceProps[i].surfaceType == FixedOutflow)    bndConds.bcVal[i] = 7105;
            else if (bcProps.surfaceProps[i].surfaceType == Symmetry) {

                if      (bcProps.surfaceProps[i].symmetryPlane == 1) bndConds.bcVal[i] = 6021;
                else if (bcProps.surfaceProps[i].symmetryPlane == 2) bndConds.bcVal[i] = 6022;
                else if (bcProps.surfaceProps[i].symmetryPlane == 3) bndConds.bcVal[i] = 6023;
            }
        }
        printf("Done boundary flags\n");

        // Write SU2 Mesh
        filename = (char *) EG_alloc(MXCHAR +1);
        if (filename == NULL) return EGADS_MALLOC;
        strcpy(filename, analysisPath);
        #ifdef WIN32
            strcat(filename, "\\");
        #else
            strcat(filename, "/");
        #endif
        strcat(filename, su2Instance[iIndex].projectName);

        if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

            status = mesh_writeSU2( filename,
                                    (int) false,
                                    volumeMesh, bndConds.numBND, bndConds.bndID,
                                    1.0);
            if (status != CAPS_SUCCESS) {
                goto cleanup;
            }
        }

        if (filename != NULL) EG_free(filename);
        filename = NULL;

        status = destroy_bndCondStruct(&bndConds);
        if (status != CAPS_SUCCESS) return status;

        // Lets check the volume mesh

        // Do we have an individual surface mesh for each body
        if (volumeMesh->numReferenceMesh != numBody) {
            printf("Number of inherited surface mesh in the volume mesh, %d, does not match the number "
                   "of bodies, %d - data transfer will NOT be possible.\n\n", volumeMesh->numReferenceMesh,numBody);

            su2Instance[iIndex].dataTransferCheck = (int) false;
        }

        // Check to make sure the volume mesher didn't add any unaccounted for points/faces
        numElementCheck = 0;
        for (i = 0; i < volumeMesh->numReferenceMesh; i++) {
            numElementCheck += volumeMesh->referenceMesh[i].numElement;
        }

        if (volumeMesh->meshQuickRef.useStartIndex == (int) false &&
            volumeMesh->meshQuickRef.useListIndex  == (int) false) {

            status = mesh_retrieveNumMeshElements(volumeMesh->numElement,
                                                  volumeMesh->element,
                                                  Triangle,
                                                  &volumeMesh->meshQuickRef.numTriangle);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = mesh_retrieveNumMeshElements(volumeMesh->numElement,
                                                  volumeMesh->element,
                                                  Quadrilateral,
                                                  &volumeMesh->meshQuickRef.numQuadrilateral);
            if (status != CAPS_SUCCESS) goto cleanup;

        }

        if (numElementCheck != volumeMesh->meshQuickRef.numTriangle +
                               volumeMesh->meshQuickRef.numQuadrilateral) {

            su2Instance[iIndex].dataTransferCheck = (int) false;
            printf("Volume mesher added surface elements - data transfer will NOT be possible.\n");

        } else { // Data transfer appears to be ok
            su2Instance[iIndex].dataTransferCheck = (int) true;
        }
    }

    // If data transfer is ok ....
    withMotion = (int) false;
    if (su2Instance[iIndex].dataTransferCheck == (int) true) {

        //See if we have data transfer information
        status = aim_getBounds(aimInfo, &numTransferName, &transferName);

        if (status == CAPS_SUCCESS && numTransferName > 0) {

            status = su2_dataTransfer(aimInfo,
                                      analysisPath,
                                      su2Instance[iIndex].projectName,
                                      *volumeMesh);

            if (status != CAPS_SUCCESS && status != CAPS_NOTFOUND) goto cleanup;

            withMotion = (int) true;
        }
    } // End if data transfer ok

    if (usePython == (int) true && pythonLinked == (int) true) {
        ///////////////////////////////////////////////////////////////////////////////////
        // Place holder for writing SU2 CFG file using python. Currently NOT implemented //
        ///////////////////////////////////////////////////////////////////////////////////
    } else {

        if (aimInputs[aim_getIndex(aimInfo, "Overwrite_CFG",ANALYSISIN)-1].vals.integer == (int) false) {

            printf("Since Python was not linked and/or being used, the \"Overwrite_CFG\" input needs to be set to \"true\" to give");
            printf(" permission to create a new SU2 cfg. SU2 CFG will NOT be updated!!\n");

        } else {

            printf("Warning: The su2 cfg file will be overwritten!\n");

            if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "SU2_Version",ANALYSISIN)-1].vals.string, "Cardinal") == 0) {

                status = su2_writeCongfig_Cardinal(aimInfo, analysisPath, aimInputs, bcProps);

            } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "SU2_Version",ANALYSISIN)-1].vals.string, "Raven") == 0) {

                status = su2_writeCongfig_Raven(aimInfo, analysisPath, aimInputs, bcProps);

            } else if (strcasecmp(aimInputs[aim_getIndex(aimInfo, "SU2_Version",ANALYSISIN)-1].vals.string, "Falcon") == 0) {

                status = su2_writeCongfig_Falcon(aimInfo, analysisPath, aimInputs, bcProps, withMotion);

            } else {

                printf("Unrecognized 'SU2_Version' = %s! Valid choices are Cardinal, Raven, and Falcon.\n", aimInputs[aim_getIndex(aimInfo, "SU2_Version",ANALYSISIN)-1].vals.string);
                status = CAPS_BADVALUE;
            }

            if (status != CAPS_SUCCESS){
                goto cleanup;
            }

        }
    }

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("Premature exit in su2AIM preAnalysis status = %d\n", status);

        if (transferName != NULL) EG_free(transferName);
        if (filename != NULL) EG_free(filename);

        (void) destroy_cfdBCsStruct(&bcProps);
        // (void) destroy_modalAeroelasticStruct(&modalAeroelastic);

        (void) destroy_bndCondStruct(&bndConds);

        return status;
}


int aimOutputs(int iIndex, void *aimStruc, int index, char **aoname, capsValue *form)
{
	// SU2 Outputs
    /*! \page aimOutputsSU2 AIM Outputs
     * After successful completion, SU2 writes results in various files.
     * The data from these files can be directly viewed, visualized, and or
     * used for further postprocessing.
     *
     * One of the files is ("forces_breakdown.dat") which summarizes
     * convergence including flow properties, numerical parameters, and
     * resulting force and moment values. As an AIM output, this file is
     * parsed for force and moment coefficients, and printed as closing
     * remarks.
     */


    int numOutVars = 9; // Grouped

    #ifdef DEBUG
        printf(" su2AIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
    #endif

    // Total Forces - Pressure + Viscous
    if      (index == 1) *aoname = EG_strdup("CLtot");
    else if (index == 2) *aoname = EG_strdup("CDtot");
    else if (index == 3) *aoname = EG_strdup("CSFtot");
    else if (index == 4) *aoname = EG_strdup("CMXtot");
    else if (index == 5) *aoname = EG_strdup("CMYtot");
    else if (index == 6) *aoname = EG_strdup("CMZtot");
    else if (index == 7) *aoname = EG_strdup("CXtot");
    else if (index == 8) *aoname = EG_strdup("CYtot");
    else if (index == 9) *aoname = EG_strdup("CZtot");

    /*! \page aimOutputsSU2
     * Net Forces - Pressure + Viscous:
     * - <B>CLtot</B> = The lift coefficient.
     * - <B>CDtot</B> = The drag coefficient.
     * - <B>CSFtot</B> = The skin friction coefficient.
     * - <B>CMXtot</B> = The moment coefficient about the x-axis.
     * - <B>CMYtot</B> = The moment coefficient about the y-axis.
     * - <B>CMZtot</B> = The moment coefficient about the z-axis.
     * - <B>CXtot</B> = The force coefficient about the x-axis.
     * - <B>CYtot</B> = The force coefficient about the y-axis.
     * - <B>CZtot</B> = The force coefficient about the z-axis.
     * .
     *
     */

    // Pressure Forces
    else if (index == 1 + numOutVars) *aoname = EG_strdup("CLtot_p");
    else if (index == 2 + numOutVars) *aoname = EG_strdup("CDtot_p");
    else if (index == 3 + numOutVars) *aoname = EG_strdup("CSFtot_p");
    else if (index == 4 + numOutVars) *aoname = EG_strdup("CMXtot_p");
    else if (index == 5 + numOutVars) *aoname = EG_strdup("CMYtot_p");
    else if (index == 6 + numOutVars) *aoname = EG_strdup("CMZtot_p");
    else if (index == 7 + numOutVars) *aoname = EG_strdup("CXtot_p");
    else if (index == 8 + numOutVars) *aoname = EG_strdup("CYtot_p");
    else if (index == 9 + numOutVars) *aoname = EG_strdup("CZtot_p");

    /*! \page aimOutputsSU2
     * Pressure Forces:
     * - <B>CLtot_p</B> = The lift coefficient - pressure contribution only.
     * - <B>CDtot_p</B> = The drag coefficient - pressure contribution only.
     * - <B>CSFtot_p</B> = The skin friction coefficient - pressure contribution only.
     * - <B>CMXtot_p</B> = The moment coefficient about the x-axis - pressure contribution only.
     * - <B>CMYtot_p</B> = The moment coefficient about the y-axis - pressure contribution only.
     * - <B>CMZtot_p</B> = The moment coefficient about the z-axis - pressure contribution only.
     * - <B>CXtot_p</B> = The force coefficient about the x-axis - pressure contribution only.
     * - <B>CYtot_p</B> = The force coefficient about the y-axis - pressure contribution only.
     * - <B>CZtot_p</B> = The force coefficient about the z-axis - pressure contribution only.
     * .
     *
     */

    // Viscous Forces
    else if (index == 1 + 2*numOutVars) *aoname = EG_strdup("CLtot_v");
    else if (index == 2 + 2*numOutVars) *aoname = EG_strdup("CDtot_v");
    else if (index == 3 + 2*numOutVars) *aoname = EG_strdup("CSFtot_v");
    else if (index == 4 + 2*numOutVars) *aoname = EG_strdup("CMXtot_v");
    else if (index == 5 + 2*numOutVars) *aoname = EG_strdup("CMYtot_v");
    else if (index == 6 + 2*numOutVars) *aoname = EG_strdup("CMZtot_v");
    else if (index == 7 + 2*numOutVars) *aoname = EG_strdup("CXtot_v");
    else if (index == 8 + 2*numOutVars) *aoname = EG_strdup("CYtot_v");
    else if (index == 9 + 2*numOutVars) *aoname = EG_strdup("CZtot_v");

    /*! \page aimOutputsSU2
     * Viscous Forces:
     * - <B>CLtot_p</B> = The lift coefficient - viscous contribution only.
     * - <B>CDtot_p</B> = The drag coefficient - viscous contribution only.
     * - <B>CSFtot_p</B> = The skin friction coefficient - viscous contribution only.
     * - <B>CMXtot_p</B> = The moment coefficient about the x-axis - viscous contribution only.
     * - <B>CMYtot_p</B> = The moment coefficient about the y-axis - viscous contribution only.
     * - <B>CMZtot_p</B> = The moment coefficient about the z-axis - viscous contribution only.
     * - <B>CXtot_p</B> = The force coefficient about the x-axis - viscous contribution only.
     * - <B>CYtot_p</B> = The force coefficient about the y-axis - viscous contribution only.
     * - <B>CZtot_p</B> = The force coefficient about the z-axis - viscous contribution only.
     * .
     *
     */

    //if (index <= 3*numOutVars) {
    form->type   = Double;
    form->dim    = Vector;
    form->length = 1;
    form->nrow   = 1;
    form->ncol   = 1;
    form->units  = NULL;
    //}

    return CAPS_SUCCESS;
}

// Calculate SU2 output
int aimCalcOutput(int iIndex, void *aimInfo, const char *analysisPath,
                  int index, capsValue *val, capsErrs **errors)
{
    int status;

    char currentPath[PATH_MAX]; // Current directory path
    char *strKeyword = NULL; //, *bndSectionKeyword = NULL; // Keyword strings

    char *strValue;    // Holder string for value of keyword

    char *filename = NULL; // File to open
    char fileExtension[] = ".dat";
    char fileSuffix[] = "forces_breakdown_";

    size_t linecap = 0;
    char *line = NULL; // Temporary line holder

    FILE *fp; // File pointer

    int numOutVars = 9; // Grouped

    capsValue *Surface_Monitor = NULL;

    #ifdef DEBUG
        printf(" su2AIM/aimCalcOutput instance = %d  index = %d!\n", iIndex, index);
    #endif

    *errors        = NULL;
    val->vals.real = 0.0; // Set default value

    //printf("index = %d\n",index);

    // Set the "search" string(s)
//    if (index <= numOutVars) {
//        bndSectionKeyword  = (char *) "Forces";
//    }

    if      (index == 1 || index == 1 + numOutVars || index == 1 + 2*numOutVars) strKeyword = (char *) "CL:";
    else if (index == 2 || index == 2 + numOutVars || index == 2 + 2*numOutVars) strKeyword = (char *) "CD:";
    else if (index == 3 || index == 3 + numOutVars || index == 3 + 2*numOutVars) strKeyword = (char *) "CSF:";
    else if (index == 4 || index == 4 + numOutVars || index == 4 + 2*numOutVars) strKeyword = (char *) "CMx:";
    else if (index == 5 || index == 5 + numOutVars || index == 5 + 2*numOutVars) strKeyword = (char *) "CMy:";
    else if (index == 6 || index == 6 + numOutVars || index == 6 + 2*numOutVars) strKeyword = (char *) "CMz:";
    else if (index == 7 || index == 7 + numOutVars || index == 7 + 2*numOutVars) strKeyword = (char *) "CFx:";
    else if (index == 8 || index == 8 + numOutVars || index == 8 + 2*numOutVars) strKeyword = (char *) "CFy:";
    else if (index == 9 || index == 9 + numOutVars || index == 9 + 2*numOutVars) strKeyword = (char *) "CFz:";
    else {
        printf("Unrecognized output variable index - %d\n", index);
        return CAPS_BADINDEX;
    }

    status = aim_getValue(aimInfo, aim_getIndex(aimInfo, "Surface_Monitor", ANALYSISIN), ANALYSISIN, &Surface_Monitor);
    if (status != CAPS_SUCCESS) return status;

    if (Surface_Monitor->nullVal == IsNull) {
        printf("***************************************************************************\n");
        printf("\n");
        printf("ERROR: Forces are not available because 'Surface_Monitor' is not specified.\n");
        printf("\n");
        printf("***************************************************************************\n");
        return CAPS_BADVALUE;
    }

    (void) getcwd(currentPath, PATH_MAX); // Get our current path

    // Set path to analysis directory
    if (chdir(analysisPath) != 0) {
        #ifdef DEBUG
            printf(" su2AIM/aimCalcOutput Cannot chdir to %s!\n", analysisPath);
        #endif

        return CAPS_DIRERR;
    }

    // Open SU2 force file
    filename = (char *) EG_alloc((strlen(su2Instance[iIndex].projectName) + strlen(fileSuffix) + strlen(fileExtension) +1)*sizeof(char));
    if (filename == NULL) return EGADS_MALLOC;

    sprintf(filename, "%s%s%s", fileSuffix, su2Instance[iIndex].projectName, fileExtension);

    fp = fopen(filename, "r");
    EG_free(filename); filename = NULL; // Free filename allocation

    if (fp == NULL) {
        #ifdef DEBUG
            printf(" su2AIM/aimCalcOutput Cannot open Output file!\n");
        #endif

        chdir(currentPath);

        return CAPS_IOERR;
    } else {
        #ifdef DEBUG
            printf(" Found force file: %s%s%s", fileSuffix, su2Instance[iIndex].projectName, fileExtension);
        #endif
    }

    // Scan the file for the string
    strValue = NULL;
    while( !feof(fp) ) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;

        strValue = strstr(line, strKeyword); // Totals
        if( strValue != NULL) {

            if ( index <= numOutVars ) { // Totals
              strValue += strlen(strKeyword);
            } else if ( index <= 2*numOutVars ) { // Pressure
                strValue = strstr(line, "Pressure (");
                strValue = strstr(strValue, "):");
                strValue += 2;
            } else if ( index <= 3*numOutVars ) { // Friction
                strValue = strstr(line, "Friction (");
                strValue = strstr(strValue, "):");
                strValue += 2;
            }

            sscanf(strValue, "%lf", &val->vals.real);
            break;
        }
    }

    EG_free(line);
    fclose(fp);

    // Restore the path we came in with
    chdir(currentPath);

    #ifdef DEBUG
        printf(" Done\n Closing force file.\n");
    #endif

    if (strValue == NULL) {
        //#ifdef DEBUG
            printf(" su2AIM/aimCalcOutput Cannot find %s in Output file!\n", strKeyword);
        //#endif

        return CAPS_NOTFOUND;
    }

    return CAPS_SUCCESS;
}

void aimCleanup()
{
    int iIndex;

    #ifdef DEBUG
        printf(" su2AIM/aimCleanup!\n");
    #endif

    // Clean up su2Instance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up su2Instance - %d\n", iIndex);

        // Attribute to index map
        //status = destroy_mapAttrToIndexStruct(&su2Instance[iIndex].attrMap);
        //if (status != CAPS_SUCCESS) return status;
        destroy_mapAttrToIndexStruct(&su2Instance[iIndex].attrMap);

        // SU2 project name
        su2Instance[iIndex].projectName = NULL;

        // Analysis file path/directory
        su2Instance[iIndex].analysisPath = NULL;

        // Pointer to caps input value for scaling pressure during data transfer
        su2Instance[iIndex].pressureScaleFactor = NULL;

        // Pointer to caps input value for offset pressure during data transfer
        su2Instance[iIndex].pressureScaleOffset = NULL;

    }

    if (su2Instance != NULL) EG_free(su2Instance);
    su2Instance = NULL;
    numInstance = 0;

}

/************************************************************************/
// CAPS transferring functions
int aimFreeDiscr(capsDiscr *discr)
{
    int i;

    #ifdef DEBUG
        printf(" su2AIM/aimFreeDiscr instance = %d!\n", discr->instance);
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

    int i, body, face; // Indexing

    int status; // Function return status

    int iIndex; // Instance index

    int numBody;
    int needMesh = (int) true;

    // EGADS objects
    ego tess, *bodies = NULL, *faces = NULL, tempBody;

    const char   *intents, *string, *capsGroup; // capsGroups strings

    // EGADS function returns
    int plen, tlen;
    const int    *ptype, *pindex, *tris, *nei;
    const double *xyz, *uv;

    // Body Tessellation
    int numFace = 0;
    int numFaceFound = 0;
    int numPoint = 0, numTri = 0, numGlobalPoint = 0;
    int *bodyFaceMap = NULL; // size=[2*numFaceFound], [2*numFaceFound + 0] = body, [2*numFaceFoun + 1] = face

    int *globalID = NULL, *localStitchedID = NULL, gID = 0;

    int nodeOffSet = 0;

    int *storage= NULL; // Extra information to store into the discr void pointer

    int numCAPSGroup = 0, attrIndex = 0, foundAttr = (int) false;
    int *capsGroupList = NULL;
    int dataTransferBodyIndex=-99;

    // Data transfer
    int  nrow, ncol, rank;
    void *dataTransfer = NULL;
    enum capsvType vtype;
    char *units;

    // Volume Mesh obtained from meshing AIM
    meshStruct *volumeMesh;
    int numElementCheck = 0;

    iIndex = discr->instance;
    #ifdef DEBUG
        printf(" su2AIM/aimDiscr: tname = %s, instance = %d!\n", tname, iIndex);
    #endif

    if ((iIndex < 0) || (iIndex >= numInstance)) return CAPS_BADINDEX;


    if (tname == NULL) return CAPS_NOTFOUND;

/*
    if (su2Instance[iIndex].dataTransferCheck == (int) false) {
        printf(" su2AIM/aimDiscr: The volume is not suitable for data transfer - possibly the volume mesher "
                "added unaccounted for points\n");
        return CAPS_BADVALUE;
    }
*/

    status = aimFreeDiscr(discr);
    if (status != CAPS_SUCCESS) return status;

    // Currently this ONLY works if the capsTranfer lives on single body!
    status = aim_getBodies(discr->aInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
        printf(" su2AIM/aimDiscr: %d aim_getBodies = %d!\n", iIndex, status);
        return status;
    }

    status = aim_getData(discr->aInfo, "Volume_Mesh", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
    if (status != CAPS_SUCCESS){
        if (status == CAPS_DIRTY) printf(" su2AIM: Parent AIMS are dirty\n");
        else printf(" su2AIM: Linking status = %d\n",status);

        printf(" su2AIM/aimDiscr: Didn't find a link to a volume mesh (Volume_Mesh) from parent - data transfer isn't possible.\n");

        goto cleanup;
    }

    volumeMesh = (meshStruct *) dataTransfer;
    if ( volumeMesh->referenceMesh == NULL) {
        printf(" ERROR: No reference meshes in volume mesh - data transfer isn't possible.\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // check if any the meshes have been set
    for (i = 0; i < numBody; i++) {
        needMesh = (int) (needMesh && (bodies[numBody+i] == NULL));
    }

    if (needMesh == (int)true) {

        // Get attribute to index mapping
        status = aim_getData(discr->aInfo, "Attribute_Map", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status == CAPS_SUCCESS) {

            printf(" su2AIM/aimDiscr: Found link for attrMap (Attribute_Map) from parent\n");

            status = copy_mapAttrToIndexStruct( (mapAttrToIndexStruct *) dataTransfer, &su2Instance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) goto cleanup;
        } else {

            if (status == CAPS_DIRTY) printf(" su2AIM/aimDiscr: Parent AIMS are dirty\n");
            else printf(" su2AIM/aimDiscr: Linking status during attrMap (Attribute_Map) = %d\n",status);

            printf(" su2AIM/aimDiscr: Didn't find a link to attrMap from parent - getting it ourselves\n");

            // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
            status = create_CAPSGroupAttrToIndexMap(numBody,
                                                    bodies,
                                                    1, // Only search down to the face level of the EGADS body
                                                    &su2Instance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        // Do we have an individual surface mesh for each body
        if (volumeMesh->numReferenceMesh != numBody) {
            printf(" ERROR: Number of inherited surface mesh in the volume mesh, %d, does not match the number "
                   "of bodies, %d - data transfer is NOT be possible.\n\n", volumeMesh->numReferenceMesh,numBody);
            status = CAPS_MISMATCH;
            goto cleanup;
        }

        // Check to make sure the volume mesher didn't add any unaccounted for points/faces
        numElementCheck = 0;
        for (i = 0; i < volumeMesh->numReferenceMesh; i++) {
            numElementCheck += volumeMesh->referenceMesh[i].numElement;
        }

        if (volumeMesh->meshQuickRef.useStartIndex == (int) false &&
            volumeMesh->meshQuickRef.useListIndex  == (int) false) {

            status = mesh_retrieveNumMeshElements(volumeMesh->numElement,
                                                  volumeMesh->element,
                                                  Triangle,
                                                  &volumeMesh->meshQuickRef.numTriangle);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = mesh_retrieveNumMeshElements(volumeMesh->numElement,
                                                  volumeMesh->element,
                                                  Quadrilateral,
                                                  &volumeMesh->meshQuickRef.numQuadrilateral);
            if (status != CAPS_SUCCESS) goto cleanup;

        }

        if (numElementCheck != volumeMesh->meshQuickRef.numTriangle +
                               volumeMesh->meshQuickRef.numQuadrilateral) {
            printf(" ERROR: Volume mesher added surface elements - data transfer will NOT be possible.\n");
            status = CAPS_MISMATCH;
            goto cleanup;
        }

        // To this point is doesn't appear that the volume mesh has done anything bad to our surface mesh(es)

        // Store away the tessellation now
        for (i = 0; i < volumeMesh->numReferenceMesh; i++) {

            #ifdef DEBUG
                printf(" su2AIM/aimDiscr instance = %d Setting body tessellation number %d!\n", iIndex, i);
            #endif

            status = aim_setTess(discr->aInfo, volumeMesh->referenceMesh[i].bodyTessMap.egadsTess);
            if (status != CAPS_SUCCESS) {
                printf(" ERROR: aim_setTess return = %d\n", status);
                goto cleanup;
            }
        }
    } // needMesh

    numFaceFound = 0;
    numPoint = numTri = 0;
    // Find any faces with our boundary marker and get how many points and triangles there are
    for (body = 0; body < numBody; body++) {

        status = EG_getBodyTopos(bodies[body], NULL, FACE, &numFace, &faces);
        if (status != EGADS_SUCCESS) {
            printf(" su2AIM: getBodyTopos (Face) = %d for Body %d!\n", status, body);
            return status;
        }

        for (face = 0; face < numFace; face++) {

            // Retrieve the string following a capsBound tag
            status = retrieve_CAPSBoundAttr(faces[face], &string);
            if (status != CAPS_SUCCESS) continue;

            if (strcmp(string, tname) != 0) continue;

            #ifdef DEBUG
                printf(" su2AIM/aimDiscr: Body %d/Face %d matches %s!\n", body, face+1, tname);
            #endif


            status = retrieve_CAPSGroupAttr(faces[face], &capsGroup);
            if (status != CAPS_SUCCESS) {
                printf("capsBound found on face %d, but no capGroup found!!!\n", face);
                continue;
            } else {

                status = get_mapAttrToIndexIndex(&su2Instance[iIndex].attrMap, capsGroup, &attrIndex);
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

            // Get face tessellation
            status = EG_getTessFace(bodies[body+numBody], face+1, &plen, &xyz, &uv, &ptype, &pindex, &tlen, &tris, &nei);
            if (status != EGADS_SUCCESS) {
                printf(" su2AIM: EG_getTessFace %d = %d for Body %d!\n", face+1, status, body+1);
                continue;
            }

            numFaceFound += 1;
            dataTransferBodyIndex = body;
            if (numFaceFound == 1) {
                bodyFaceMap = (int *) EG_alloc(2*numFaceFound*sizeof(int));
            } else {
                bodyFaceMap = (int *) EG_reall(bodyFaceMap, 2*numFaceFound*sizeof(int));
            }

            if (bodyFaceMap == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            // Get number of points and triangles
            bodyFaceMap[2*(numFaceFound-1) + 0] = body+1;
            bodyFaceMap[2*(numFaceFound-1) + 1] = face+1;

            // Sum number of points and triangles
            numPoint  += plen;
            numTri  += tlen;
        }

        if (faces != NULL) EG_free(faces);
        faces = NULL;


        if (dataTransferBodyIndex >=0) break; // Force that only one body can be used
    }

    if (numFaceFound == 0) {
        printf(" su2AIM/aimDiscr: No Faces match %s!\n", tname);

        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    if ((dataTransferBodyIndex - 1) > volumeMesh->numReferenceMesh ) {
        printf(" su2AIM/aimDiscr: Data transfer body index doesn't match number of reference meshes in volume mesh - data transfer isn't possible.\n");
        status = CAPS_MISMATCH;
        goto cleanup;
    }

    if ( numPoint == 0 || numTri == 0) {
        #ifdef DEBUG
            printf(" su2AIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
        #endif
        status = CAPS_SOURCEERR;
        goto cleanup;
    }

    #ifdef DEBUG
        printf(" su2AIM/aimDiscr: Instance %d, Body Index for data transfer = %d\n", iIndex, dataTransferBodyIndex);
    #endif


    // Specify our single element type
    status = EGADS_MALLOC;
    discr->nTypes = 1;

    discr->types  = (capsEleType *) EG_alloc(sizeof(capsEleType));
    if (discr->types == NULL) goto cleanup;
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

    // Get the tessellation and make up a simple linear continuous triangle discretization */

    discr->nElems = numTri;

    discr->elems = (capsElement *) EG_alloc(discr->nElems*sizeof(capsElement));
    if (discr->elems == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    discr->mapping = (int *) EG_alloc(2*numPoint*sizeof(int)); // Will be resized
    if (discr->mapping == NULL) goto cleanup;

    globalID = (int *) EG_alloc(numPoint*sizeof(int));
    if (globalID == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    numPoint = 0;
    numTri = 0;
    for (face = 0; face < numFaceFound; face++){

        tess = bodies[bodyFaceMap[2*face + 0]-1 + numBody];

        if (localStitchedID == NULL) {
            status = EG_statusTessBody(tess, &tempBody, &i, &numGlobalPoint);
            if (status != CAPS_SUCCESS) goto cleanup;

            localStitchedID = (int *) EG_alloc(numGlobalPoint*sizeof(int));
            if (localStitchedID == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            for (i = 0; i < numGlobalPoint; i++) localStitchedID[i] = 0;
        }

        // Get face tessellation
        status = EG_getTessFace(tess, bodyFaceMap[2*face + 1], &plen, &xyz, &uv, &ptype, &pindex, &tlen, &tris, &nei);
        if (status != EGADS_SUCCESS) {
            printf(" su2AIM: EG_getTessFace %d = %d for Body %d!\n", bodyFaceMap[2*face + 1], status, bodyFaceMap[2*face + 0]);
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

        // Get triangle connectivity in global sense
        for (i = 0; i < tlen; i++) {

            discr->elems[numTri].bIndex      = bodyFaceMap[2*face + 0];
            discr->elems[numTri].tIndex      = 1;
            discr->elems[numTri].eIndex      = bodyFaceMap[2*face + 1];

            discr->elems[numTri].gIndices    = (int *) EG_alloc(6*sizeof(int));
            if (discr->elems[numTri].gIndices == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            discr->elems[numTri].dIndices    = NULL;
            discr->elems[numTri].eTris.tq[0] = i+1;

            status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], tris[3*i + 0], &gID);
            if (status != EGADS_SUCCESS) goto cleanup;

            discr->elems[numTri].gIndices[0] = localStitchedID[gID-1];
            discr->elems[numTri].gIndices[1] = tris[3*i + 0];

            status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], tris[3*i + 1], &gID);
            if (status != EGADS_SUCCESS) goto cleanup;

            discr->elems[numTri].gIndices[2] = localStitchedID[gID-1];
            discr->elems[numTri].gIndices[3] = tris[3*i + 1];

            status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1], tris[3*i + 2], &gID);
            if (status != EGADS_SUCCESS) goto cleanup;

            discr->elems[numTri].gIndices[4] = localStitchedID[gID-1];
            discr->elems[numTri].gIndices[5] = tris[3*i + 2];

            numTri += 1;
        }
    }

    discr->nPoints = numPoint;

    #ifdef DEBUG
        printf(" su2AIM/aimDiscr: ntris = %d, npts = %d!\n", discr->nElems, discr->nPoints);
    #endif

    // Resize mapping to switched together number of points
    discr->mapping = (int *) EG_reall(discr->mapping, 2*numPoint*sizeof(int));

    // Local to global node connectivity + numCAPSGroup + sizeof(capGrouplist)
    storage  = (int *) EG_alloc((numPoint + 1 + numCAPSGroup) *sizeof(int));
    if (storage == NULL) goto cleanup;
    discr->ptrm = storage;

    // Store the local-to-Global (volume) id
    nodeOffSet = 0;
    for (i = 0; i < dataTransferBodyIndex; i++) {
        nodeOffSet += volumeMesh->referenceMesh[i].numNode;
    }

    #ifdef DEBUG
        printf(" su2AIM/aimDiscr: Instance = %d, nodeOffSet = %d, dataTransferBodyIndex = %d\n", iIndex, nodeOffSet, dataTransferBodyIndex);
    #endif

    for (i = 0; i < numPoint; i++) {

        storage[i] = globalID[i] + nodeOffSet; //volumeMesh->referenceMesh[dataTransferBodyIndex-1].node[globalID[i]-1].nodeID + nodeOffSet;

        //#ifdef DEBUG
        //	printf(" su2AIM/aimDiscr: Instance = %d, Global Node ID %d\n", iIndex, storage[i]);
        //#endif
    }

    // Save way the attrMap capsGroup list
    storage[numPoint] = numCAPSGroup;
    for (i = 0; i < numCAPSGroup; i++) {
        storage[numPoint+1+i] = capsGroupList[i];
    }

    #ifdef DEBUG
        printf(" su2AIM/aimDiscr: Instance = %d, Finished!!\n", iIndex);
    #endif

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (faces != NULL) EG_free(faces);

        if (globalID  != NULL) EG_free(globalID);
        if (localStitchedID != NULL) EG_free(localStitchedID);

        if (capsGroupList != NULL) EG_free(capsGroupList);
        if (bodyFaceMap != NULL) EG_free(bodyFaceMap);

        if (status != CAPS_SUCCESS) {
            aimFreeDiscr(discr);
        }

        return status;
}

int aimLocateElement(capsDiscr *discr, double *params, double *param, int *eIndex,
                 	 double *bary)
{
    int    i, in[3], stat, ismall;
    double we[3], w, smallw = -1.e300;

    if (discr == NULL) return CAPS_NULLOBJ;

    for (ismall = i = 0; i < discr->nElems; i++) {
        in[0] = discr->elems[i].gIndices[0] - 1;
        in[1] = discr->elems[i].gIndices[2] - 1;
        in[2] = discr->elems[i].gIndices[4] - 1;
        stat  = EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]], param, we);

        if (stat == EGADS_SUCCESS) {
            *eIndex = i+1;
            bary[0] = we[1];
            bary[1] = we[2];
            return CAPS_SUCCESS;
        }

        w = we[0];
        if (we[1] < w) w = we[1];
        if (we[2] < w) w = we[2];
        if (w > smallw) {
            ismall = i+1;
            smallw = w;
        }
    }

    /* must extrapolate! */
    if (ismall == 0) return CAPS_NOTFOUND;
    in[0] = discr->elems[ismall-1].gIndices[0] - 1;
    in[1] = discr->elems[ismall-1].gIndices[2] - 1;
    in[2] = discr->elems[ismall-1].gIndices[4] - 1;
    EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]], param, we);

    *eIndex = ismall;
    bary[0] = we[1];
    bary[1] = we[2];

    /*
      printf(" aimLocateElement: extropolate to %d (%lf %lf %lf)  %lf\n", ismall, we[0], we[1], we[2], smallw);
     */
    return CAPS_SUCCESS;
}

int aimUsesDataSet(int inst, void *aimInfo, const char *bname,
                   const char *dname, enum capsdMethod dMethod)
{
  /*! \page aimUsesDataSetSU2  data sets consumed by SU2
   *
   * This function checks if a data set name can be consumed by this aim.
   * The SU2 aim can consume "Displacement" data sets for areolastic analysis.
   */

  if (strcasecmp(dname, "Displacement") == 0) {
      return CAPS_SUCCESS;
  }

  return CAPS_NOTNEEDED;
}

int aimTransfer(capsDiscr *discr, const char *dataName, int numPoint, int dataRank, double *dataVal, /*@unused@*/ char **units)
{
    /*! \page dataTransferSU2 SU2 Data Transfer
     *
     * The SU2 AIM has the ability to transfer surface data (e.g. pressure distributions) to and from the AIM
     * using the conservative and interpolative data transfer schemes in CAPS. Currently these transfers may only
     * take place on triangular meshes.
     *
     * \section dataFromSU2 Data transfer from SU2
     *
     * <ul>
     *  <li> <B>"Cp", or "CoefficientofPressure"</B> </li> <br>
     *  Loads the coefficient of pressure distribution from surface_flow_[project_name].cvs file.
     *  This distribution may be scaled based on
     *  Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset, where "Pressure_Scale_Factor"
     *  and "Pressure_Scale_Offset" are AIM inputs (\ref aimInputsSU2)
     *
     * <li> <B>"Pressure" or "P" </B> </li> <br>
     *  Loads the pressure distribution from surface_flow_[project_name].cvs file.
     *  This distribution may be scaled based on
     *  Pressure = Pressure_Scale_Factor*Pressure + Pressure_Scale_Offset, where "Pressure_Scale_Factor"
     *  and "Pressure_Scale_Offset" are AIM inputs (\ref aimInputsSU2)
     * </ul>
     *
     */ // Reset of this block comes from su2Util.c

    int status, status2; // Function return status
    int i, j, dataPoint; // Indexing

    // Current directory path
    char   currentPath[PATH_MAX];

    // Aero-Load data variables
    int numVariable;
    int numDataPoint;
    char **variableName = NULL;
    double **dataMatrix = NULL;
    double dataScaleFactor = 1.0;
    double dataScaleOffset = 0.0;
    //char *dataUnits;

    // Indexing in data variables
    int globalIDIndex = -99;
    int variableIndex = -99;

    // Variables used in global node mapping
    int *nodeMap, *storage;
    int globalNodeID;
    int found = (int) false;

    // Filename stuff
    char *filename = NULL; //"pyCAPS_SU2_Tetgen_ddfdrive_bndry1.dat";

    #ifdef DEBUG
        printf(" su2AIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n", dataName, discr->instance, numPoint, dataRank);
    #endif


    if (strcasecmp(dataName, "Pressure") != 0 &&
        strcasecmp(dataName, "P")        != 0 &&
        strcasecmp(dataName, "Cp")       != 0 &&
        strcasecmp(dataName, "CoefficientofPressure") != 0) {

        printf("Unrecognized data transfer variable - %s\n", dataName);
        return CAPS_NOTFOUND;
    }

    //Get the appropriate parts of the tessellation to data
    storage = (int *) discr->ptrm;
    nodeMap = &storage[0]; // Global indexing on the body

    // Zero out data
    for (i = 0; i < numPoint; i++) {
        for (j = 0; j < dataRank; j++ ) {
            dataVal[dataRank*i+j] = 0;
        }
    }

    (void) getcwd(currentPath, PATH_MAX);
    if (chdir(su2Instance[discr->instance].analysisPath) != 0) {
        #ifdef DEBUG
            printf(" su2AIM/aimTransfer Cannot chdir to %s!\n", su2Instance[discr->instance].analysisPath);
        #endif

        return CAPS_DIRERR;
    }


    filename = (char *) EG_alloc((strlen(su2Instance[discr->instance].projectName) +
                                  strlen("surface_flow_.csv") + 1)*sizeof(char));

    sprintf(filename,"%s%s%s", "surface_flow_",
                               su2Instance[discr->instance].projectName,
                               ".csv");

    status = su2_readAeroLoad(filename,
                              &numVariable,
                              &variableName,
                              &numDataPoint,
                              &dataMatrix);

    if (filename != NULL) EG_free(filename);
    filename = NULL;

    // Restore the path we came in with
    chdir(currentPath);
    if (status != CAPS_SUCCESS) return status;

    //printf("Number of variables %d\n", numVariable);
    // Output some of the first row of the data
    //for (i = 0; i < numVariable; i++) printf("Variable %d - %.6f\n", i, dataMatrix[i][0]);

    // Loop through the variable list to find which one is the global node ID variable
    for (i = 0; i < numVariable; i++) {
        if (strcasecmp("Global_Index", variableName[i]) ==0 ) {
            globalIDIndex = i;
            break;
        }
    }

    if (globalIDIndex == -99) {
        printf("Global node number variable not found in data file\n");
        status = CAPS_NOTFOUND;
        goto bail;
    }

    // Loop through the variable list to see if we can find the transfer data name
    for (i = 0; i < numVariable; i++) {

        if (strcasecmp(dataName, "Pressure") == 0 ||
            strcasecmp(dataName, "P")        == 0) {

            if (dataRank != 1) {
                printf("Data transfer rank should be 1 not %d\n", dataRank);
                status = CAPS_BADRANK;
                goto bail;
            }

            dataScaleFactor = su2Instance[discr->instance].pressureScaleFactor->vals.real;
            dataScaleOffset = su2Instance[discr->instance].pressureScaleOffset->vals.real;

            //dataUnits = su2Instance[discr->instance].pressureScaleFactor->units;
            if (strcasecmp("Pressure", variableName[i]) == 0) {
                variableIndex = i;
                break;
            }
        }

        if (strcasecmp(dataName, "Cp")       == 0 ||
            strcasecmp(dataName, "CoefficientofPressure") == 0) {

            if (dataRank != 1) {
                printf("Data transfer rank should be 1 not %d\n", dataRank);
                status = CAPS_BADRANK;
                goto bail;
            }

            dataScaleFactor = su2Instance[discr->instance].pressureScaleFactor->vals.real;
            dataScaleOffset = su2Instance[discr->instance].pressureScaleOffset->vals.real;

            //dataUnits = su2Instance[discr->instance].pressureScaleFactor->units;

            if (strcasecmp(su2Instance[discr->instance].su2Version->vals.string, "Cardinal") == 0) {
                if (strcasecmp("C<sub>p</sub>", variableName[i]) == 0) {
                    variableIndex = i;
                    break;
                }
            } else {

                if (strcasecmp("Pressure_Coefficient", variableName[i]) == 0) {
                    variableIndex = i;
                    break;
                }
            }
        }
    }

    if (variableIndex == -99) {
        printf("Variable %s not found in data file\n", dataName);
        status = CAPS_NOTFOUND;
        goto bail;
    }

    for (i = 0; i < numPoint; i++) {

        globalNodeID = nodeMap[i];

        found = (int) false;
        for (dataPoint = 0; dataPoint < numDataPoint; dataPoint++) {
            if ((int) dataMatrix[globalIDIndex][dataPoint] +1 ==  globalNodeID) { // SU2 meshes are 0-index
                found = (int) true;
                break;
            }
        }

        if (found == (int) true) {
            for (j = 0; j < dataRank; j++) {

                // Add something for units - aim_covert()

                dataVal[dataRank*i+j] = dataMatrix[variableIndex][dataPoint]*dataScaleFactor + dataScaleOffset;
                //dataVal[dataRank*i+j] = 99;

            }
        } else {
            printf("Error: Unable to find node %d!\n", globalNodeID);
            status = CAPS_BADVALUE;
            goto bail;
        }
    }


    // Free data matrix
    if (dataMatrix != NULL) {
        for (i = 0; i < numVariable; i++) {
            if (dataMatrix[i] != NULL) EG_free(dataMatrix[i]);
        }

        EG_free(dataMatrix);
    }

    // Free variable list
    status = string_freeArray(numVariable, &variableName);
    if (status != CAPS_SUCCESS) return status;

    return CAPS_SUCCESS;

    bail:
        // Free data matrix
        if (dataMatrix != NULL) {
            for (i = 0; i < numVariable; i++) {
                if (dataMatrix[i] != NULL) EG_free(dataMatrix[i]);
            }

            EG_free(dataMatrix);
        }

        // Free variable list
        status2 = string_freeArray(numVariable, &variableName);
        if (status2 != CAPS_SUCCESS) return status2;

        return status;
}

int aimInterpolation(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                 double *bary, int rank, double *data, double *result)
{
    int    in[3], i;
    double we[3];
    /*
    #ifdef DEBUG
        printf(" su2AIM/aimInterpolation  %s  instance = %d!\n",
         name, discr->instance);
    #endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" su2AIM/Interpolation: eIndex = %d [1-%d]!\n",eIndex, discr->nElems);
    }

    we[0] = 1.0 - bary[0] - bary[1];
    we[1] = bary[0];
    we[2] = bary[1];
    in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
    in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
    in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
    for (i = 0; i < rank; i++){
        result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] + data[rank*in[2]+i]*we[2];
    }

    return CAPS_SUCCESS;
}


int aimInterpolateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                      double *bary, int rank, double *r_bar, double *d_bar)
{
    int    in[3], i;
    double we[3];

    /*
	#ifdef DEBUG
  	  printf(" su2AIM/aimInterpolateBar  %s  instance = %d!\n", name, discr->instance);
	#endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" su2AIM/InterpolateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

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

    return CAPS_SUCCESS;
}


int aimIntegration(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
                   /*@null@*/ double *data, double *result)
{
    int        i, in[3], stat, ptype, pindex, nBody;
    double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
    const char *intents;
    ego        *bodies;

    /*
	#ifdef DEBUG
  	  printf(" su2AIM/aimIntegration  %s  instance = %d!\n", name, discr->instance);
	#endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" su2AIM/aimIntegration: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegration: %d aim_getBodies = %d!\n", discr->instance, stat);
        return stat;
    }

    /* element indices */

    in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
    in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
    in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

    stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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

    return CAPS_SUCCESS;
}


int aimIntegrateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
                    double *r_bar, double *d_bar)
{
    int        i, in[3], stat, ptype, pindex, nBody;
    double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
    const char *intents;
    ego        *bodies;

    /*
	#ifdef DEBUG
  	  printf(" su2AIM/aimIntegrateBar  %s  instance = %d!\n", name, discr->instance);
	#endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" su2AIM/aimIntegrateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegrateBar: %d aim_getBodies = %d!\n", discr->instance, stat);
        return stat;
    }

    /* element indices */

    in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
    in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
    in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
    stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

    if (stat != CAPS_SUCCESS) {
        printf(" su2AIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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

    return CAPS_SUCCESS;
}
