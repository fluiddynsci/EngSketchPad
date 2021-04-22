/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             FUN3D AIM
 *
 *     Written by Dr. Ryan Durscher AFRL/RQVC
 *
 */


/*!\mainpage Introduction
 * \tableofcontents
 * \section overviewFUN3D FUN3D AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact (primarily
 * through input files) with NASA LaRC's unstructured flow solver FUN3D \cite FUN3D. FUN3D is a parallelized flow analysis
 * and design suite capable of addressing a wide variety of complex aerodynamic configurations by utilizing a
 * mixed-element, node-based, finite volume discretization.
 * The suite can simulate perfect gas (both incompressible and compressible), as well as multi-species
 * equilibrium and non-equilibrium flows. Turbulence effects may be represented through a wide variety
 * of models. Currently only a subset of FUN3D's input options have been exposed in the analysis interface
 * module (AIM), but features can easily be included as future needs arise.
 *
 * Current issues include:
 *  - A thorough bug testing needs to be undertaken.
 *  - Not all parameters/variables in fun3d.nml are currently available.
 *  .
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsFUN3D and \ref aimOutputsFUN3D, respectively.
 *
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataFUN3D if connecting this AIM to other AIMs in a
 * parent-child like manner.
 *
 * Details of the AIM's automated data transfer capabilities are outlined in \ref dataTransferFUN3D
 *
 *
 * \section fun3dNML Generating fun3d.nml
 * FUN3D's primarily input file is a master FORTRAN namelist, fun3d.nml. To generate a bare-bones fun3d.nml
 * file based on the variables set in \ref aimInputsFUN3D, nothing else besides the AIM needs to be provided. Since
 * this will create a new fun3d.nml file every time the AIM is executed it is essential to set
 * the Overwrite\_NML input variable to "True". This gives the user ample warning that their fun3d.nml (if it exists)
 * will be over written.
 *
 * Conversely, to read and append an existing namelist file the user needs Python installed so that the AIM may
 * be complied against the Python API library (and header file - Python.h). The AIM interacts with Python through
 * a Cython linked script that utilizes the "f90nml" Python module; note, having Cython installed is not
 * required. On systems with "pip" installed typing "pip install f90nml", will download and install the "f90nml" module.
 *
 * The Cython script will first try to read an existing fun3d.nml file in the specified
 * analysis directory; if the file does not exist one will be created. Only modified input variables that have been
 * specified as AIM inputs (currently supported variables are outlined in \ref aimInputsFUN3D) are
 * updated in the namelist file.
 *
 * \section fun3dExample Examples
 * An example problem using the FUN3D AIM (coupled with a meshing AIM - TetGen) may be found at
 * \ref fun3dTetgenExample.
 *
 */

//The accepted and expected geometric representation and analysis intentions are detailed in  \ref geomRepIntentFUN3D.

#ifdef HAVE_PYTHON

#include "Python.h" // Bring in Python API

#include "fun3dNamelist.h" // Bring in Cython generated header file
// for namelist generation wiht Python

#if PY_MAJOR_VERSION < 3
#define Initialize_fun3dNamelist initfun3dNamelist
#else
#define Initialize_fun3dNamelist PyInit_fun3dNamelist
#endif

#endif

#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"

#include "meshUtils.h"
#include "cfdUtils.h"
#include "miscUtils.h"
#include "fun3dUtils.h"

#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

/*
#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
 */

#define NUMINPUT   34
#define NUMOUTPUT  25

#define MXCHAR  255

//#define DEBUG

typedef struct {

    // Fun3d project name
    char *projectName;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

    // Check to make sure data transfer is ok
    int dataTransferCheck;

    // Analysis file path/directory
    const char *analysisPath;

    // Pointer to CAPS input value for scaling pressure during data transfer
    capsValue *pressureScaleFactor;

    // Pointer to CAPS input value for offset pressure during data transfer
    capsValue *pressureScaleOffset;

    // Number of geometric inputs
    int numGeomIn;

    // Pointer to CAPS geometric in values
    capsValue *geomInVal;

} aimStorage;

static aimStorage *fun3dInstance = NULL;
static int         numInstance  = 0;




// ********************** Exposed AIM Functions *****************************

int aimInitialize(int ngIn, capsValue *gIn,
                  int *qeFlag, const char *unitSys,
                  int *nIn, int *nOut,
                  int *nFields, char ***fnames, int **ranks)
{

    int status; // Function status return
    int flag;

    int *ints;
    char **strs;

    aimStorage *tmp;

#ifdef DEBUG
    printf("\n fun3dAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif
    flag     = *qeFlag;
    *qeFlag  = 0;

    // Specify the number of analysis input and out "parameters"
    *nIn     = NUMINPUT;
    *nOut    = NUMOUTPUT;
    if (flag == 1) return CAPS_SUCCESS;

    // Specify the field variables this analysis can generate
    *nFields = 4;
    ints = (int *) EG_alloc(*nFields*sizeof(int));
    if (ints == NULL) return EGADS_MALLOC;
    ints[0]  = 1;
    ints[1]  = 1;
    ints[2]  = 1;
    ints[3]  = 1;
    *ranks   = ints;

    strs = (char **) EG_alloc(*nFields*sizeof(char *));
    if (strs == NULL) {
        EG_free(*ranks);
        *ranks   = NULL;
        return EGADS_MALLOC;
    }

    strs[0]  = EG_strdup("Pressure");
    strs[1]  = EG_strdup("P");
    strs[2]  = EG_strdup("Cp");
    strs[3]  = EG_strdup("CoefficientOfPressure");
    *fnames  = strs;

    // Allocate fun3dInstance
    if (fun3dInstance == NULL) {

        fun3dInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (fun3dInstance == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }

    } else {
        tmp = (aimStorage *) EG_reall(fun3dInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) {
            EG_free(*fnames);
            EG_free(*ranks);
            *ranks   = NULL;
            *fnames  = NULL;
            return EGADS_MALLOC;
        }

        fun3dInstance = tmp;
    }

    // Set initial values for fun3dInstance
    fun3dInstance[numInstance].projectName = NULL;

    // Analysis file path/directory
    fun3dInstance[numInstance].analysisPath = NULL;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&fun3dInstance[numInstance].attrMap);
    if (status != CAPS_SUCCESS) {
        printf("Problem encountered during initiate_mapAttrToIndexStruct..continuing\n");
        EG_free(*fnames);
        EG_free(*ranks);
        *ranks   = NULL;
        *fnames  = NULL;
        return status;
    }

    // Check to make sure data transfer is ok
    fun3dInstance[numInstance].dataTransferCheck = (int) true;

    // Pointer to caps input value for scaling pressure during data transfer
    fun3dInstance[numInstance].pressureScaleFactor = NULL;

    // Pointer to caps input value for off setting pressure during data transfer
    fun3dInstance[numInstance].pressureScaleOffset = NULL;

    // Number of geometric inputs
    fun3dInstance[numInstance].numGeomIn = ngIn;

    // Point to CAPS geometric in values
    fun3dInstance[numInstance].geomInVal = gIn;

    // Increment number of instances
    numInstance += 1;

    return (numInstance -1);
}

int aimInputs(int iIndex, void *aimInfo, int index, char **ainame, capsValue *defval)
{
    /*! \page aimInputsFUN3D AIM Inputs
     * The following list outlines the FUN3D inputs along with their default values available
     * through the AIM interface. One will note most of the FUN3D parameters have a NULL value as their default.
     * This is done since a parameter in the FUN3D input deck (fun3d.nml) is only changed if the value has
     * been changed in CAPS (i.e. set to something other than NULL).
     */


#ifdef DEBUG
    printf(" fun3dAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
#endif

    *ainame = NULL;

    // FUN3D Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("fun3d_CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B> Proj_Name = "fun3d_CAPS"</B> <br>
         * This corresponds to the project\_rootname variable in the \&project namelist of fun3d.nml.
         */
    } else if (index == 2) {
        *ainame              = EG_strdup("Mach"); // Mach number
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B> Mach = NULL </B> <br>
         *  This corresponds to the mach\_number variable in the \&reference\_physical\_properties
         *  namelist of fun3d.nml.
         */
    } else if (index == 3) {
        *ainame              = EG_strdup("Re"); // Reynolds number
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B> Re = NULL </B> <br>
         *  This corresponds to the reynolds\_number variable in the \&reference\_physical\_properties
         *  namelist of fun3d.nml.
         */
    } else if (index == 4) {
        *ainame              = EG_strdup("Viscous"); // Viscous term
        defval->type         = String;
        defval->vals.string  = NULL;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B> Viscous = NULL </B> <br>
         *  This corresponds to the viscous\_terms variable in the \&governing\_equation namelist
         *  of fun3d.nml.
         */
    } else if (index == 5) {
        *ainame              = EG_strdup("Equation_Type"); // Equation type
        defval->type         = String;
        defval->vals.string  = NULL;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B> Equation_Type = NULL </B> <br>
         * This corresponds to the eqn\_type variable in the \&governing\_equation namelist of
         * fun3d.nml.
         */
    } else if (index == 6) {
        *ainame              = EG_strdup("Alpha");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;
        defval->units = EG_strdup("degree");

        /*! \page aimInputsFUN3D
         * - <B> Alpha = NULL </B> <br>
         *  This corresponds to the angle\_of\_attack variable in the \&reference\_physical\_properties
         *  namelist of fun3d.nml [degree].
         */
    } else if (index == 7) {
        *ainame              = EG_strdup("Beta");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;
        defval->units = EG_strdup("degree");

        /*! \page aimInputsFUN3D
         * - <B> Beta = NULL </B> <br>
         *  This corresponds to the angle\_of\_yaw variable in the \&reference\_physical\_properties
         *  namelist of fun3d.nml [degree].
         */
    } else if (index == 8) {
        *ainame              = EG_strdup("Overwrite_NML");
        defval->type         = Boolean;
        defval->vals.integer = false;
        defval->nullVal      = NotNull;

        /*! \page aimInputsFUN3D
         * - <B> Overwrite_NML = NULL</B> <br>
         *  - If Python is NOT linked with the FUN3D AIM at compile time or Use_Python_NML is set to False
         *  this flag gives the AIM permission to overwrite fun3d.nml if present.
         *  The namelist produced will solely consist of input variables present and set in the AIM.
         *
         *  - If Python IS linked with the FUN3D AIM at compile time and Use_Python_NML is set to True the
         *  namelist file will be overwritten, as opposed to being appended.
         *
         */
    } else if (index == 9) {
        *ainame              = EG_strdup("Mesh_Format");
        defval->type         = String;
        defval->vals.string  = EG_strdup("AFLR3");
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B>Mesh_Format = "AFLR3"</B> <br>
         *  Mesh output format. By default, an AFLR3 mesh will be used.
         */
    } else if (index == 10) {
        *ainame              = EG_strdup("Mesh_ASCII_Flag");
        defval->type         = Boolean;
        defval->vals.integer = true;

        /*! \page aimInputsFUN3D
         * - <B>Mesh_ASCII_Flag = True</B> <br>
         *  Output mesh in ASCII format, otherwise write a binary file if applicable.
         */
    } else if (index == 11) {
        *ainame              = EG_strdup("Num_Iter");
        defval->type         = Integer;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B>Num_Iter = NULL</B> <br>
         *  This corresponds to the steps variable in the \&code\_run\_control
         *  namelist of fun3d.nml.
         */
    } else if (index == 12) {
        *ainame               = EG_strdup("CFL_Schedule");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->length        = 2;
        defval->nrow          = 2;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->length*sizeof(double));
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;

        /*! \page aimInputsFUN3D
         * - <B>CFL_Schedule = NULL</B> <br>
         *  This corresponds to the schedule\_cfl variable in the \&nonlinear\_solver\_parameters
         *   namelist of fun3d.nml.
         */
    } else if (index == 13) {
        *ainame               = EG_strdup("CFL_Schedule_Iter");
        defval->type          = Integer;
        defval->dim           = Vector;
        defval->length        = 2;
        defval->nrow          = 2;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.integers = (int *) EG_alloc(defval->length*sizeof(int));
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;

        /*! \page aimInputsFUN3D
         * - <B>CFL_Schedule_Inter = NULL</B> <br>
         *  This corresponds to the schedule\_iteration variable in the \&nonlinear\_solver\_parameters
         *  namelist of fun3d.nml.
         */
    } else if (index == 14) {
        *ainame              = EG_strdup("Restart_Read");
        defval->type         = String;
        defval->vals.string  = NULL;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B>Restart_Read = NULL</B> <br>
         * This corresponds to the restart_read variable in the \&code_run_control namelist of fun3d.nml.
         */
    } else if (index == 15) {
        *ainame              = EG_strdup("Boundary_Condition");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsFUN3D
         * - <B>Boundary_Condition = NULL </B> <br>
         * See \ref cfdBoundaryConditions for additional details.
         */
    } else if (index == 16) {
        *ainame              = EG_strdup("Use_Python_NML");
        defval->type         = Boolean;
        defval->vals.integer = (int) false;

        /*! \page aimInputsFUN3D
         * - <B>Use_Python_NML = False </B> <br>
         * By default, even if Python has been linked to the FUN3D AIM it is not used unless the this value is set to True.
         */
    } else if (index == 17) {
        *ainame              = EG_strdup("Pressure_Scale_Factor");
        defval->type         = Double;
        defval->vals.real    = 1.0;
        defval->units      	 = NULL;

        fun3dInstance[iIndex].pressureScaleFactor = defval;

        /*! \page aimInputsFUN3D
         * - <B>Pressure_Scale_Factor = 1.0</B> <br>
         * Value to scale Cp data when transferring data. Data is scaled based on Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset.
         */
    } else if (index == 18) {
        *ainame              = EG_strdup("Pressure_Scale_Offset");
        defval->type         = Double;
        defval->vals.real    = 0.0;
        defval->units      	 = NULL;

        fun3dInstance[iIndex].pressureScaleOffset = defval;

        /*! \page aimInputsFUN3D
         * - <B>Pressure_Scale_Offset = 0.0</B> <br>
         * Value to offset Cp data when transferring data. Data is scaled based on Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset.
         */
    } else if (index == 19) {
        *ainame              = EG_strdup("NonInertial_Rotation_Rate");
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

        /*! \page aimInputsFUN3D
         * - <B>NonInertial_Rotation_Rate = NULL [0.0, 0.0, 0.0]</B> <br>
         * Array values correspond to the rotation\_rate\_x, rotation\_rate\_y, rotation\_rate\_z variables, respectively,
         * in the \&noninertial\_reference\_frame namelist of fun3d.nml.
         */
    } else if (index == 20) {
        *ainame              = EG_strdup("NonInertial_Rotation_Center");
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

        /*! \page aimInputsFUN3D
         * - <B>NonInertial_Rotation_Center = NULL, [0.0, 0.0, 0.0]</B> <br>
         * Array values correspond to the rotation\_center\_x, rotation\_center\_y, rotation\_center\_z variables, respectively,
         * in the \&noninertial\_reference\_frame namelist of fun3d.nml.
         */
    } else if (index == 21) {
        *ainame              = EG_strdup("Two_Dimensional");
        defval->type         = Boolean;
        defval->vals.integer = (int) false;

        /*! \page aimInputsFUN3D
         * - <B>Two_Dimensional = False</B> <br>
         * Run FUN3D in 2D mode. If set to True, the body must be a single "sheet" body in the x-z plane (a rudimentary node
         * swapping routine is attempted if not in the x-z plane). A 3D mesh will be written out,
         * where the body is extruded a length of 1 in the y-direction.
         */
    } else if (index == 22) {
        *ainame              = EG_strdup("Modal_Aeroelastic");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsFUN3D
         * - <B>Modal_Aeroelastic = NULL </B> <br>
         * See \ref cfdModalAeroelastic for additional details.
         */
    } else if (index == 23) {
        *ainame              = EG_strdup("Modal_Ref_Velocity");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Scalar;
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B>Modal_Ref_Velocity = NULL </B> <br>
         * The freestream velocity in structural dynamics equation units; used for scaling during modal
         * aeroelastic simulations. This corresponds to the uinf variable in the \&aeroelastic\_modal\_data
         *  namelist of movingbody.input.
         */
    }  else if (index == 24) {
        *ainame              = EG_strdup("Modal_Ref_Length");
        defval->type         = Double;
        //defval->units        = NULL;
        defval->dim          = Scalar;
        defval->lfixed       = Change;
        defval->vals.real    = 1.0;

        /*! \page aimInputsFUN3D
         * - <B>Modal_Ref_Length = 1.0 </B> <br>
         * The scaling factor between CFD and the structural dynamics equation units; used for scaling during modal
         * aeroelastic simulations. This corresponds to the grefl variable in the \&aeroelastic\_modal\_data
         *  namelist of movingbody.input.
         */
    }  else if (index == 25) {
        *ainame              = EG_strdup("Modal_Ref_Dynamic_Pressure");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B>Modal_Ref_Dynamic_Pressure = NULL </B> <br>
         * The freestream dynamic pressure in structural dynamics equation units; used for scaling during modal
         * aeroelastic simulations. This corresponds to the qinf variable in the \&aeroelastic\_modal\_data
         *  namelist of movingbody.input.
         */
    } else if (index == 26) {
        *ainame              = EG_strdup("Time_Accuracy");
        defval->type         = String;
        defval->vals.string  = NULL;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B>Time_Accuracy = NULL </B> <br>
         * Defines the temporal scheme to use. This corresponds to the time_accuracy variable
         *  in the \&nonlinear_solver_parameters namelist of fun3d.nml.
         */
    } else if (index == 27) {
        *ainame              = EG_strdup("Time_Step");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B>Time_Step = NULL </B> <br>
         * Non-dimensional time step during time accurate simulations. This corresponds to the time_step_nondim
         * variable in the \&nonlinear_solver_parameters namelist of fun3d.nml.
         */
    } else if (index == 28) {
        *ainame              = EG_strdup("Num_Subiter");
        defval->type         = Integer;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B>Num_Subiter = NULL </B> <br>
         * Number of subiterations used during a time step in a time accurate simulations. This
         * corresponds to the subiterations variable in the \&nonlinear_solver_parameters namelist of fun3d.nml.
         */
    } else if (index == 29) {
        *ainame              = EG_strdup("Temporal_Error");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;

        /*! \page aimInputsFUN3D
         * - <B>Temporal_Error = NULL </B> <br>
         * This sets the tolerance for which subiterations are stopped during time accurate simulations. This
         * corresponds to the temporal_err_floor variable in the \&nonlinear_solver_parameters namelist of fun3d.nml.
         */
    } else if (index == 30) {
        *ainame              = EG_strdup("Reference_Area");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;
        defval->vals.real    = 0.0;

        /*! \page aimInputsFUN3D
         * - <B>Reference_Area = NULL </B> <br>
         * This sets the reference area for used in force and moment calculations. This
         * corresponds to the area_reference variable in the \&force_moment_integ_properties namelist of fun3d.nml.
         * Alternatively, the geometry (body) attribute "capsReferenceArea" maybe used to specify this variable
         * (note: values set through the AIM input will supersede the attribution value).
         */
    } else if (index == 31) {
        *ainame              = EG_strdup("Moment_Length");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->length        = 2;
        defval->nrow          = 2;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals == NULL) {
            return EGADS_MALLOC;
        } else {
            defval->vals.reals[0] = 0.0;
            defval->vals.reals[1] = 0.0;
        }
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;

        /*! \page aimInputsFUN3D
         * - <B>Moment_Length = NULL, [0.0, 0.0]</B> <br>
         * Array values correspond to the x_moment_length and y_moment_length variables,
         * respectively, in the \&force_moment_integ_properties namelist of fun3d.nml.
         * Alternatively, the geometry (body) attributes "capsReferenceChord" and "capsReferenceSpan" may be
         * used to specify the x- and y- moment lengths, respectively (note: values set through
         * the AIM input will supersede the attribution values).
         */
    } else if (index == 32) {
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

        /*! \page aimInputsFUN3D
         * - <B>Moment_Center = NULL, [0.0, 0.0, 0.0]</B> <br>
         * Array values correspond to the x_moment_center, y_moment_center, and z_moment_center variables,
         * respectively, in the \&force_moment_integ_properties namelist of fun3d.nml.
         * Alternatively, the geometry (body) attributes "capsReferenceX", "capsReferenceY",
         * and "capsReferenceZ" may be used to specify the x-, y-, and z- moment centers, respectively
         * (note: values set through the AIM input will supersede the attribution values).
         */
    } else if (index == 33) {
        *ainame              = EG_strdup("FUN3D_Version");
        defval->type       = Double;
        defval->dim        = Vector;
        defval->length     = 1;
        defval->nrow       = 1;
        defval->ncol       = 1;
        defval->units      = NULL;
        defval->vals.real  = 13.1;
        defval->lfixed     = Fixed;

        /*! \page aimInputsFUN3D
         * - <B>FUN3D_Version = 13.1 </B> <br>
         * FUN3D version to generate specific configuration file for; currently only has influence over
         * rubber.data (sensitivity file).
         */
    } else if (index == 34) {
        *ainame              = EG_strdup("Sensitivity");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsFUN3D
         * - <B>Sensitivity = NULL </B> <br>
         * A work in progress - Currently setting this value to any string will retrieve the parameterization for all
         * geometric design parameters.
         */
    }

    // Link variable(s) to parent(s) if available
    /*if ((index != 1) && (*ainame != NULL) && (index !=15)) {
            status = aim_link(aimInfo, *ainame, ANALYSISIN, defval);
	    printf("Status = %d: Var Index = %d, Type = %d, link = %lX\n",
                   status, index, defval->type, defval->link);
	}*/

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

    /*! \page sharableDataFUN3D AIM Shareable Data
     * Currently the FUN3D AIM does not have any shareable data types or values. It will try, however, to inherit a
     * "Volume_Mesh" (for 3D simulations) or a "Surface_Mesh" (for 2D simulations) from any parent AIMs.
     * Note that the inheritance of the volume/surface mesh variable is required
     * if the FUN3D aim is to write a suitable grid and <sup>*</sup>.mapbc (boundary condition file for the AFLR3 mesh format) file.
     */

#ifdef DEBUG
    printf(" fun3dAIM/aimData instance = %d  name = %s!\n", iIndex, name);
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

    // AIM input bodies
    int  numBody;
    ego *bodies = NULL;
    int  bodySubType = 0, bodyOClass, bodyNumChild, *bodySense = NULL;
    ego  bodyRef, *bodyChild = NULL;
    double bodyData[4];

    // Boundary/surface properties
    cfdBCsStruct   bcProps;

    // Modal Aeroelastic properties
    modalAeroelasticStruct   modalAeroelastic;

    // Boundary conditions container - for writing .mapbc file
    bndCondStruct bndConds;

    // Volume Mesh obtained from meshing AIM
    meshStruct *volumeMesh = NULL;
    int numElementCheck; // Consistency checkers for volume-surface meshes

    // Discrete data transfer variables
    char **transferName = NULL;
    int numTransferName;

    // NULL out errors
    *errs = NULL;

    // Store away the analysis path/directory
    fun3dInstance[iIndex].analysisPath = analysisPath;

    // Initiate structures variables - will be destroyed during cleanup
    status = initiate_cfdBCsStruct(&bcProps);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_modalAeroelasticStruct(&modalAeroelastic);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_bndCondStruct(&bndConds);
    if (status != CAPS_SUCCESS) return status;


    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) printf("aim_getBodies status = %d!!\n", status);

#ifdef DEBUG
    printf(" fun3dAIM/aimPreAnalysis instance = %d  numBody = %d!\n", iIndex, numBody);
#endif

    if ((numBody <= 0) || (bodies == NULL)) {
#ifdef DEBUG
        printf(" fun3dAIM/aimPreAnalysis No Bodies!\n");
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
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }
        }

        if (aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].nullVal == IsNull) {

            status = EG_attributeRet(bodies[body], "capsReferenceChord", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS){

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].vals.reals[0] = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceChord should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }

            }

            status = EG_attributeRet(bodies[body], "capsReferenceSpan", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].vals.reals[1] = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Length", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceSpan should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
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
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }

            status = EG_attributeRet(bodies[body], "capsReferenceY", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[1] = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceY should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }

            status = EG_attributeRet(bodies[body], "capsReferenceZ", &atype, &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[2] = (double) reals[0];
                    aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceZ should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
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

    // Should we use python even if it was linked?
    usePython = aimInputs[aim_getIndex(aimInfo, "Use_Python_NML", ANALYSISIN)-1].vals.integer;
    if (usePython == (int) true && pythonLinked == (int) false) {

        printf("Use of Python library requested but not linked!\n");
        usePython = (int) false;

    } else if (usePython == (int) false && pythonLinked == (int) true) {

        printf("Python library was linked, but will not be used!\n");
    }

    // Get project name
    fun3dInstance[iIndex].projectName = aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string;

    // Get intent
/* Ryan -- please fix
    status = check_CAPSIntent(numBody, bodies, &currentIntent);
    if (status != CAPS_SUCCESS) goto cleanup;

    if (currentIntent != CAPSMAGIC) {
        if(currentIntent != CFD)  {
            printf("All bodies must have the same capsIntent of CFD!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }
    }  */

    if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

        // Get attribute to index mapping
        status = aim_getData(aimInfo, "Attribute_Map", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status == CAPS_SUCCESS) {

            printf("Found link for attrMap (Attribute_Map) from parent\n");

            //fun3dInstance[iIndex].attrMap = *(mapAttrToIndexStruct *) dataTransfer;

            status = copy_mapAttrToIndexStruct( (mapAttrToIndexStruct *) dataTransfer, &fun3dInstance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) goto cleanup;

        } else {


            if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
            else if (status == CAPS_SOURCEERR) printf("Multiple parents sharing Attribute_Map\n");
            else printf("Linking status during Attribute_Map = %d\n",status);

            printf("Didn't find a link to Attribute_Map from parent - getting it ourselves\n");


            if (aimInputs[aim_getIndex(aimInfo, "Two_Dimensional", ANALYSISIN)-1].vals.integer == (int) true ) {

                // Get capsGroup name and index mapping to make sure all faces and edges have a capsGroup value
                status = create_CAPSGroupAttrToIndexMap(numBody,
                                                        bodies,
                                                        2, // Only search down to the edge level of the EGADS body
                                                        &fun3dInstance[iIndex].attrMap);
                if (status != CAPS_SUCCESS) goto cleanup;

            } else {

                // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
                status = create_CAPSGroupAttrToIndexMap(numBody,
                                                        bodies,
                                                        1, // Only search down to the face level of the EGADS body
                                                        &fun3dInstance[iIndex].attrMap);
                if (status != CAPS_SUCCESS) goto cleanup;

            }
        }
    }

    // Get boundary conditions - Only if the boundary condition has been set
    if (aimInputs[aim_getIndex(aimInfo, "Boundary_Condition", ANALYSISIN)-1].nullVal ==  NotNull) {

        status = cfd_getBoundaryCondition( aimInputs[aim_getIndex(aimInfo, "Boundary_Condition", ANALYSISIN)-1].length,
                                           aimInputs[aim_getIndex(aimInfo, "Boundary_Condition", ANALYSISIN)-1].vals.tuple,
                                           &fun3dInstance[iIndex].attrMap,
                                           &bcProps);
        if (status != CAPS_SUCCESS) goto cleanup;

    } else {
        printf("Warning: No boundary conditions provided !!!!\n");
    }

    // Get modal aeroelastic information - only get modal aeroelastic inputs if they have be set
    if (aimInputs[aim_getIndex(aimInfo, "Modal_Aeroelastic", ANALYSISIN)-1].nullVal ==  NotNull) {

        status = cfd_getModalAeroelastic( aimInputs[aim_getIndex(aimInfo, "Modal_Aeroelastic", ANALYSISIN)-1].length,
                                          aimInputs[aim_getIndex(aimInfo, "Modal_Aeroelastic", ANALYSISIN)-1].vals.tuple,
                                          &modalAeroelastic);
        if (status != CAPS_SUCCESS) goto cleanup;


        modalAeroelastic.freestreamVelocity = aimInputs[aim_getIndex(aimInfo, "Modal_Ref_Velocity", ANALYSISIN)-1].vals.real;
        modalAeroelastic.freestreamDynamicPressure = aimInputs[aim_getIndex(aimInfo, "Modal_Ref_Dynamic_Pressure", ANALYSISIN)-1].vals.real;
        modalAeroelastic.lengthScaling = aimInputs[aim_getIndex(aimInfo, "Modal_Ref_Length", ANALYSISIN)-1].vals.real;
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


        // Add extruded plane boundary condition
        bcProps.numBCID += 1;
        bcProps.surfaceProps = (cfdSurfaceStruct *) EG_reall(bcProps.surfaceProps, bcProps.numBCID * sizeof(cfdSurfaceStruct));
        if (bcProps.surfaceProps == NULL) return EGADS_MALLOC;

        status = intiate_cfdSurfaceStruct(&bcProps.surfaceProps[bcProps.numBCID-1]);
        if (status != CAPS_SUCCESS) goto cleanup;

        bcProps.surfaceProps[bcProps.numBCID-1].surfaceType = Symmetry;
        bcProps.surfaceProps[bcProps.numBCID-1].symmetryPlane = 2;

        // Find largest index value for bcID and set it plus 1 to the new surfaceProp
        for (i = 0; i < bcProps.numBCID-1; i++) {

            if (bcProps.surfaceProps[i].bcID >= bcProps.surfaceProps[bcProps.numBCID-1].bcID) {

                bcProps.surfaceProps[bcProps.numBCID-1].bcID = bcProps.surfaceProps[i].bcID + 1;
            }
        }

        // Get Surface mesh
        status = aim_getData(aimInfo, "Surface_Mesh", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status != CAPS_SUCCESS){

            if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
            else printf("Linking status = %d\n",status);

            printf("Didn't find a link to a surface mesh (Surface_Mesh) from parent - a mesh will NOT be created.\n");

        } else {

            printf("Found link for surface mesh (Surface_Mesh) from parent\n");

            volumeMesh = (meshStruct *) EG_alloc(sizeof(meshStruct));
            if (volumeMesh == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            status = initiate_meshStruct(volumeMesh);
            if (status != CAPS_SUCCESS) return status;

            status = fun3d_2DMesh((meshStruct *) dataTransfer,
                                  &fun3dInstance[iIndex].attrMap,
                                  volumeMesh,
                                  &bcProps.surfaceProps[bcProps.numBCID-1].bcID);
            if (status != CAPS_SUCCESS) goto cleanup;

        }

        // Can't currently do data transfer in 2D-mode
        fun3dInstance[iIndex].dataTransferCheck = (int) false;

    } else {
        // Get Volume mesh
        status = aim_getData(aimInfo, "Volume_Mesh", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status != CAPS_SUCCESS){

            if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");
            //else printf("Linking status = %d\n",status);

            printf("Didn't find a link to a volume mesh (Volume_Mesh) from parent - a volume mesh will NOT be created.\n");

            // No volume currently means we can't do datatransfer
            fun3dInstance[iIndex].dataTransferCheck = (int) false;

        } else {

            printf("Found link for volume mesh (Volume_Mesh) from parent\n");
            volumeMesh = (meshStruct *) dataTransfer;
        }
    }

    if (status == CAPS_SUCCESS) {

        status = populate_bndCondStruct_from_bcPropsStruct(&bcProps, &bndConds);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Replace dummy values in bcVal with FUN3D specific values
        for (i = 0; i < bcProps.numBCID ; i++) {

            // {UnknownBoundary, Inviscid, Viscous, Farfield, Extrapolate, Freestream,
            //  BackPressure, Symmetry, SubsonicInflow, SubsonicOutflow,
            //  MassflowIn, MassflowOut, FixedInflow, FixedOutflow}

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
            else if (bcProps.surfaceProps[i].surfaceType == MachOutflow)     bndConds.bcVal[i] = 5052;
            else if (bcProps.surfaceProps[i].surfaceType == Symmetry) {

                if      (bcProps.surfaceProps[i].symmetryPlane == 1) bndConds.bcVal[i] = 6661;
                else if (bcProps.surfaceProps[i].symmetryPlane == 2) bndConds.bcVal[i] = 6662;
                else if (bcProps.surfaceProps[i].symmetryPlane == 3) bndConds.bcVal[i] = 6663;
                else {
                    printf("Unknown symmetryPlane for boundary %d - Defaulting to y-Symmetry\n", bcProps.surfaceProps[i].bcID);
                    bndConds.bcVal[i] = 6662;
                }
            }
        }

        filename = (char *) EG_alloc(MXCHAR +1);
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
        strcat(filename, fun3dInstance[iIndex].projectName);

        if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

            // Write AFLR3
            status = mesh_writeAFLR3(filename,
                                     aimInputs[aim_getIndex(aimInfo, "Mesh_ASCII_Flag",  ANALYSISIN)-1].vals.integer,
                                     volumeMesh,
                                     1.0);

            if (status != CAPS_SUCCESS) {
                goto cleanup;
            }
        }

        // Write *.mapbc file
        status = write_MAPBC(filename,
                             bndConds.numBND,
                             bndConds.bndID,
                             bndConds.bcVal);
        if (filename != NULL) EG_free(filename);
        filename = NULL;

        if (status != CAPS_SUCCESS) goto cleanup;

        // Lets check the volume mesh

        // Do we have an individual surface mesh for each body
        if (volumeMesh->numReferenceMesh != numBody) {
            printf("Number of inherited surface mesh in the volume mesh, %d, does not match the number "
                    "of bodies, %d - data transfer will NOT be possible.\n\n", volumeMesh->numReferenceMesh,numBody);
            fun3dInstance[iIndex].dataTransferCheck = (int) false;
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

        if (numElementCheck != (volumeMesh->meshQuickRef.numTriangle + volumeMesh->meshQuickRef.numQuadrilateral)) {

            fun3dInstance[iIndex].dataTransferCheck = (int) false;
            printf("Volume mesher added surface elements - data transfer will NOT be possible.\n");

        } else { // Data transfer appears to be ok
            fun3dInstance[iIndex].dataTransferCheck = (int) true;
        }
    }

    //////////////////////////////////////////////////////////
    // Open and write the fun3d.nml input file using Python //
    //////////////////////////////////////////////////////////
    if (usePython == (int) true && pythonLinked == (int) true) {
#ifdef HAVE_PYTHON
        {
            // Flag to see if Python was already initialized - i.e. the AIM was called from within Python
            int initPy = (int) false;

            // Python thread state variable for new interpreter if needed
            //PyThreadState *newThread = NULL;

            printf("\nUsing Python to write FUN3D namelist (fun3d.nml)\n");

            // Initialize python
            if (Py_IsInitialized() == 0) {
                printf("\tInitializing Python for FUN3D AIM\n\n");
                Py_Initialize();
                initPy = (int) true;
            } else {

                /*
                // If python is already initialized - create a new interpreter to run the namelist creator
                printf("\tInitializing new Python thread for FUN3D AIM\n\n");
                PyEval_InitThreads();
                newThread = Py_NewInterpreter();
                (void) PyThreadState_Swap(newThread);
                 */
                initPy = (int) false;
            }


            (void) Initialize_fun3dNamelist();

            status = fun3d_writeNMLPython(aimInfo, analysisPath, aimInputs, bcProps);
            if (status == -1) {
                printf("\tWarning: Python error occurred while writing namelist file\n");
            } else {
                printf("\tDone writing nml file with Python\n");
            }

            // Close down python
            if (initPy == (int) false) {
                printf("\n");
                /*
                printf("\tClosing new Python thread\n");
                Py_EndInterpreter(newThread);
                (void) PyThreadState_Swap(NULL); // This function call is probably not necessary;
                 */
            } else {
                printf("\tClosing Python\n");
                Py_Finalize(); // Do not finalize if the AIM was called from Python
            }
        }
#endif

    } else {

        if (aimInputs[aim_getIndex(aimInfo, "Overwrite_NML",ANALYSISIN)-1].vals.integer == (int) false) {

            printf("Since Python was not linked and/or being used, the \"Overwrite_NML\" input needs to be set to \"True\" to give");
            printf(" permission to create a new fun3d.nml. fun3d.nml will NOT be updated!!\n");

        } else {

            printf("Warning: The fun3d.nml file will be overwritten!\n");

            status = fun3d_writeNML(aimInfo, analysisPath, aimInputs, bcProps);
            if (status != CAPS_SUCCESS) goto cleanup;

        }
    }

    // If data transfer is ok ....
    if (fun3dInstance[iIndex].dataTransferCheck == (int) true) {

        //See if we have data transfer information
        status = aim_getBounds(aimInfo, &numTransferName, &transferName);
        if (status == CAPS_SUCCESS) {

            if (aimInputs[aim_getIndex(aimInfo, "Modal_Aeroelastic", ANALYSISIN)-1].nullVal ==  NotNull) {
                status = fun3D_dataTransfer(aimInfo,
                                            analysisPath,
                                            fun3dInstance[iIndex].projectName,
                                            *volumeMesh,
                                            &modalAeroelastic);
                if (status == CAPS_SUCCESS) {

                    status = fun3d_writeMovingBody(analysisPath, bcProps, &modalAeroelastic);
                }

            } else{
                status = fun3D_dataTransfer(aimInfo,
                                            analysisPath,
                                            fun3dInstance[iIndex].projectName,
                                            *volumeMesh,
                                            NULL);
            }

            if (status != CAPS_SUCCESS && status != CAPS_NOTFOUND) goto cleanup;
        }
    } // End if data transfer ok


    // Sensitivity
    if (aimInputs[aim_getIndex(aimInfo, "Sensitivity", ANALYSISIN)-1].nullVal ==  NotNull) {

        if (fun3dInstance[iIndex].dataTransferCheck == (int) true ) {

            status = fun3d_makeDirectory(analysisPath);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = fun3d_writeParameterization(aimInfo, analysisPath,
                                                 volumeMesh,
                                                 fun3dInstance[iIndex].numGeomIn,
                                                 fun3dInstance[iIndex].geomInVal);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = fun3d_writeRubber(aimInfo, aimInputs,
                                       analysisPath,
                                       volumeMesh,
                                       fun3dInstance[iIndex].numGeomIn,
                                       fun3dInstance[iIndex].geomInVal);
            if (status != CAPS_SUCCESS) goto cleanup;

        } else {
            printf("The volume is not suitable for sensitivity input generation - possibly the volume mesher "
                    "added unaccounted for points\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }
    }

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("Premature exit in fun3DAIM preAnalysis status = %d\n", status);

        if (transferName != NULL) EG_free(transferName);
        if (filename != NULL) EG_free(filename);

        (void) destroy_cfdBCsStruct(&bcProps);
        (void) destroy_modalAeroelasticStruct(&modalAeroelastic);

        (void) destroy_bndCondStruct(&bndConds);

        // Clean up the volume mesh that was created for 2D mode
        if (aimInputs[aim_getIndex(aimInfo, "Two_Dimensional", ANALYSISIN)-1].vals.integer == (int) true ) {

            // Destroy volume mesh since we created here instead of inheriting it
            (void) destroy_meshStruct(volumeMesh);
            EG_free(volumeMesh);

        }

        return status;
}


int aimOutputs(int iIndex, void *aimStruc, int index, char **aoname, capsValue *form)
{

    /*! \page aimOutputsFUN3D AIM Outputs
     * The following list outlines the FUN3D outputs available through the AIM interface. All variables currently
     * correspond to values for all boundaries (total) found in the *.forces file
     */

    int numOutVars = 8; // Grouped

#ifdef DEBUG
    printf(" fun3dAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
#endif

    // Total Forces - Pressure + Viscous
    if 		(index == 1) *aoname = EG_strdup("CLtot");
    else if (index == 2) *aoname = EG_strdup("CDtot");
    else if (index == 3) *aoname = EG_strdup("CMXtot");
    else if (index == 4) *aoname = EG_strdup("CMYtot");
    else if (index == 5) *aoname = EG_strdup("CMZtot");
    else if (index == 6) *aoname = EG_strdup("CXtot");
    else if (index == 7) *aoname = EG_strdup("CYtot");
    else if (index == 8) *aoname = EG_strdup("CZtot");

    /*! \page aimOutputsFUN3D
     * Net Forces - Pressure + Viscous:
     * - <B>CLtot</B> = The lift coefficient.
     * - <B>CDtot</B> = The drag coefficient.
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
    else if (index == 3 + numOutVars) *aoname = EG_strdup("CMXtot_p");
    else if (index == 4 + numOutVars) *aoname = EG_strdup("CMYtot_p");
    else if (index == 5 + numOutVars) *aoname = EG_strdup("CMZtot_p");
    else if (index == 6 + numOutVars) *aoname = EG_strdup("CXtot_p");
    else if (index == 7 + numOutVars) *aoname = EG_strdup("CYtot_p");
    else if (index == 8 + numOutVars) *aoname = EG_strdup("CZtot_p");

    /*! \page aimOutputsFUN3D
     * Pressure Forces:
     * - <B>CLtot_p</B> = The lift coefficient - pressure contribution only.
     * - <B>CDtot_p</B> = The drag coefficient - pressure contribution only.
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
    else if (index == 3 + 2*numOutVars) *aoname = EG_strdup("CMXtot_v");
    else if (index == 4 + 2*numOutVars) *aoname = EG_strdup("CMYtot_v");
    else if (index == 5 + 2*numOutVars) *aoname = EG_strdup("CMZtot_v");
    else if (index == 6 + 2*numOutVars) *aoname = EG_strdup("CXtot_v");
    else if (index == 7 + 2*numOutVars) *aoname = EG_strdup("CYtot_v");
    else if (index == 8 + 2*numOutVars) *aoname = EG_strdup("CZtot_v");

    /*! \page aimOutputsFUN3D
     * Viscous Forces:
     * - <B>CLtot_v</B> = The lift coefficient - viscous contribution only.
     * - <B>CDtot_v</B> = The drag coefficient - viscous contribution only.
     * - <B>CMXtot_v</B> = The moment coefficient about the x-axis - viscous contribution only.
     * - <B>CMYtot_v</B> = The moment coefficient about the y-axis - viscous contribution only.
     * - <B>CMZtot_v</B> = The moment coefficient about the z-axis - viscous contribution only.
     * - <B>CXtot_v</B> = The force coefficient about the x-axis - viscous contribution only.
     * - <B>CYtot_v</B> = The force coefficient about the y-axis - viscous contribution only.
     * - <B>CZtot_v</B> = The force coefficient about the z-axis - viscous contribution only.
     */
    else if (index == 25) {
        *aoname = EG_strdup("Forces");
        form->type         = Tuple;
        form->nullVal      = IsNull;
        //defval->units        = NULL;
        form->dim          = Vector;
        form->lfixed       = Change;
        form->vals.tuple   = NULL;

        /*! \page aimOutputsFUN3D
         * Force components:
         * - <B>Forces</B> = Returns a tuple array of JSON string dictionaries of forces and moments for each boundary (combined
         * forces also included). The structure for the Forces tuple = ("Boundary Name", "Value").
         * "Boundary Name" defines the boundary/component name (or "Total") and the "Value" is a JSON string dictionary. Entries
         * in the dictionary are the same as the other output variables without "tot" in the
         * name (e.g. CL, CD, CMX, CL_p, CMX_v, etc.).
         */

    } else {

        printf(" fun3dAIM/aimOutputs instance = %d  index = %d NOT Found!\n", iIndex, index);
        return CAPS_NOTFOUND;
    }

    if (index <= 3*numOutVars) {
        form->type   = Double;
        form->dim    = Vector;
        form->length = 1;
        form->nrow   = 1;
        form->ncol   = 1;
        form->units  = NULL;

        form->vals.reals = NULL;
        form->vals.real = 0;
    }

    return CAPS_SUCCESS;
}

static int fun3d_readForcesJSON(FILE *fp, mapAttrToIndexStruct *attrMap, capsValue *val) {

    int status; // Function return status

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder
    char *strValue;
    char *newLine;
    const char *keyWord = NULL;

    char lineVal[80];
    char json[9*80 + 5];
    char num1[15], num2[15], num3[15];

    char *title    = "FORCE SUMMARY FOR BOUNDARY";
    char *titleTot = "FORCE TOTALS FOR ALL BOUNDARIES";
    char name[80];
    int nameIndex;

    int i = 7, j = 30, k = 53;

    capsTuple *tempTuple=NULL;

    if (fp == NULL) {
        status = CAPS_NULLVALUE;
        goto cleanup;
    }

    if (attrMap == NULL) {
        status = CAPS_NULLVALUE;
        goto cleanup;
    }

    if (val == NULL) {
        status = CAPS_NULLVALUE;
        goto cleanup;
    }

    val->length = 0;
    val->nrow = 0;
    val->vals.tuple = NULL;

    status = CAPS_NOTFOUND;

    while (getline(&line, &linecap, fp) >= 0) {

        if (line == NULL) continue;

        strValue = strstr(line, title);

        if ( (strValue != NULL) || (strstr(line, titleTot) != NULL) ){

            if (strValue != NULL) {

                sprintf(name, "%s", &strValue[strlen(title)+1]);
                if ( (newLine = strchr(name, '\n')) != NULL) *newLine = '\0';

            } else {

                sprintf(name, "%s", "Total");
            }
            val->length += 1;
            val->nrow   += 1;

            tempTuple = EG_reall(val->vals.tuple, val->length*sizeof(capsTuple));
            if (tempTuple == NULL) {
                status = EGADS_MALLOC;
                val->length -= 1;
                val->nrow -= 1;
                goto cleanup;
            }

            val->vals.tuple = tempTuple;
            val->nullVal = NotNull;
            val->vals.tuple[val->length -1 ].name = NULL;
            val->vals.tuple[val->length -1 ].value = NULL;

            // Initiate JSON string
            sprintf(json, "{");

            if (strValue != NULL) {
                status = getline(&line, &linecap, fp); //Skip line - "Boundary type"
                if (status <=0) goto cleanup;
            }

            status = getline(&line, &linecap, fp); //Skip line - "----"
            if (status <=0) goto cleanup;

            status = getline(&line, &linecap, fp); //Skip line - "Pressure forces"
            if (status <=0) goto cleanup;

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,","CL_p", num1, "CD_p", num2);
            strcat(json, lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,","CMX_p", num1, "CMY_p", num2, "CMZ_p", num3);
            strcat(json, lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,","CX_p", num1, "CY_p", num2, "CZ_p", num3);
            strcat(json, lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp); //Skip line - "Viscous forces"
            if (status <=0) goto cleanup;

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,","CL_v", num1, "CD_v", num2);
            strcat(json,lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,","CMX_v", num1, "CMY_v", num2, "CMZ_v", num3);
            strcat(json,lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,","CX_v", num1, "CY_v", num2, "CZ_v", num3);
            strcat(json,lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp); //Skip line - "Total forces"
            if (status <=0) goto cleanup;

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,","CL", num1, "CD", num2);
            strcat(json,lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,","CMX", num1, "CMY", num2, "CMZ", num3);
            strcat(json,lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s","CX", num1, "CY", num2, "CZ", num3);
            strcat(json,lineVal); // Add to JSON string

            strcat(json,"}");

            //printf("JSON = %s\n", json);

            status = string_toInteger(name, &nameIndex);
            if (status != CAPS_SUCCESS) goto cleanup;

            status = get_mapAttrToIndexKeyword(attrMap, nameIndex, &keyWord);
            if (status == CAPS_SUCCESS) {
                val->vals.tuple[val->length-1].name  = EG_strdup(keyWord);
            } else {

                val->vals.tuple[val->length-1].name  = EG_strdup(name);
            }

            val->vals.tuple[val->length-1].value = EG_strdup(json);

            // Reset name index value in case we have totals next
            nameIndex = 0;
        }
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Premature exit in fun3DAIM fun3d_readForcesJSON status = %d\n", status);

        if (line != NULL) EG_free(line);

        return status;
}

static int fun3d_readForces(FILE *fp, int index, capsValue *val){

    int status; // Function return

    size_t linecap = 0;

    char *strKeyword = NULL, *bndSectionKeyword = NULL, *bndSubSectionKeyword = NULL; // Keyword strings

    char *strValue;    // Holder string for value of keyword
    char *line = NULL; // Temporary line holder

    int bndSectionFound = (int) false;
    int bndSubSectionFound = (int) false;

    int numOutVars = 8; // Grouped

    if (fp == NULL) {
        status = CAPS_NULLVALUE;
        goto cleanup;
    }

    if (val == NULL) {
        status = CAPS_NULLVALUE;
        goto cleanup;
    }

    // Set the "search" string(s)
    if (index <= numOutVars) {

        bndSectionKeyword  = (char *) " FORCE TOTALS FOR ALL BOUNDARIES\n";
        bndSubSectionKeyword = (char *) " Total forces\n";

    } else if (index > numOutVars && index <= 2*numOutVars) {

        bndSectionKeyword  = (char *) " FORCE TOTALS FOR ALL BOUNDARIES\n";
        bndSubSectionKeyword = (char *) " Pressure forces\n";

    } else if (index > 2*numOutVars && index <= 3*numOutVars) {

        bndSectionKeyword  = (char *) " FORCE TOTALS FOR ALL BOUNDARIES\n";
        bndSubSectionKeyword = (char *) " Viscous forces\n";
    } else {
        bndSectionKeyword  = (char *) "FORCE SUMMARY FOR BOUNDARY";
    }

    if      (index == 1 || index == 1 + numOutVars || index == 1 + 2*numOutVars) strKeyword = (char *) "Cl  =";
    else if (index == 2 || index == 2 + numOutVars || index == 2 + 2*numOutVars) strKeyword = (char *) "Cd  =";
    else if (index == 3 || index == 3 + numOutVars || index == 3 + 2*numOutVars) strKeyword = (char *) "Cmx =";
    else if (index == 4 || index == 4 + numOutVars || index == 4 + 2*numOutVars) strKeyword = (char *) "Cmy =";
    else if (index == 5 || index == 5 + numOutVars || index == 5 + 2*numOutVars) strKeyword = (char *) "Cmz =";
    else if (index == 6 || index == 6 + numOutVars || index == 6 + 2*numOutVars) strKeyword = (char *) "Cx  =";
    else if (index == 7 || index == 7 + numOutVars || index == 7 + 2*numOutVars) strKeyword = (char *) "Cy  =";
    else if (index == 8 || index == 8 + numOutVars || index == 8 + 2*numOutVars) strKeyword = (char *) "Cz  =";
    else {
        printf("Unrecognized output variable index - %d\n", index);
        return CAPS_BADINDEX;
    }

    // Scan the file for the string
    strValue = NULL;
    while (getline(&line, &linecap, fp) >= 0) {

        if (line == NULL) continue;

        if (strcmp(line,bndSectionKeyword) == 0 && bndSectionFound == (int) false) {

            //printf("FOUND section\n");
            bndSectionFound = (int) true;
            continue; // Skip to next line
        }

        if (strcmp(line, bndSubSectionKeyword) == 0 && bndSectionFound    == (int) true
                                                    && bndSubSectionFound == (int) false) {

            //printf("Found sub-sections\n");
            bndSubSectionFound = (int) true;
            continue; // Skipe to next line
        }

        if (bndSectionFound == (int) true && bndSubSectionFound == (int) true){

            strValue = strstr(line, strKeyword);

            if (strValue != NULL) {

                //printf("String value %s\n", strValue);

                // Found it -- get the value

                status = string_toDouble(&strValue[6], &val->vals.real);
                if (status != CAPS_SUCCESS) goto cleanup;

                break;
            }
        }
    }

    if (strValue == NULL) {
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Premature exit in fun3DAIM fun3d_readForces status = %d\n", status);

        if (line != NULL) EG_free(line);

        return status;
}


// Calculate FUN3D output
int aimCalcOutput(int iIndex, /*@unused@*/ void *aimInfo, const char *analysisPath,
        int index, capsValue *val, capsErrs **errors)
{
    int status;

    char currentPath[PATH_MAX]; // Current directory path

    char *filename = NULL; // File to open
    char fileExtension[] = ".forces";

    FILE *fp = NULL; // File pointer

    *errors        = NULL;
    val->vals.real = 0.0; // Set default value

    //printf("index = %d\n",index);

    (void) getcwd(currentPath, PATH_MAX); // Get our current path

    // Set path to analysis directory
    if (chdir(analysisPath) != 0) return CAPS_DIRERR;


    // Open fun3d *.force file
    filename = (char *) EG_alloc((strlen(fun3dInstance[iIndex].projectName) + strlen(fileExtension) +1)*sizeof(char));
    if (filename == NULL) {
        chdir(currentPath);

        status = EGADS_MALLOC;
        goto cleanup;
    }

    sprintf(filename, "%s%s", fun3dInstance[iIndex].projectName, fileExtension);

    fp = fopen(filename, "r");

    // Restore the path we came in with
    chdir(currentPath);

    if (fp == NULL) {

        printf("Could not open file: %s in %s\n", filename, analysisPath);
        status = CAPS_IOERR;
        goto cleanup;
    }

    if (index == 25) {
        status = fun3d_readForcesJSON(fp, &fun3dInstance[iIndex].attrMap, val);
        if (status != CAPS_SUCCESS) goto cleanup;

    } else {
        status = fun3d_readForces(fp, index, val);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Premature exit in fun3DAIM calcOutput status = %d\n", status);

        if (fp !=  NULL) fclose(fp);
        if (filename != NULL) EG_free(filename); // Free filename allocation

        return status;

}

void aimCleanup()
{
    int iIndex;

#ifdef DEBUG
    printf(" fun3dAIM/aimCleanup!\n");

#endif

    // Clean up fun3dInstance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up fun3dInstance - %d\n", iIndex);

        // Attribute to index map
        destroy_mapAttrToIndexStruct(&fun3dInstance[iIndex].attrMap);
        //status = destroy_mapAttrToIndexStruct(&fun3dInstance[iIndex].attrMap);
        //if (status != CAPS_SUCCESS) return status;

        // FUN3D project name
        fun3dInstance[iIndex].projectName = NULL;

        // Analysis file path/directory
        fun3dInstance[iIndex].analysisPath = NULL;

        // Pointer to caps input value for scaling pressure during data transfer
        fun3dInstance[iIndex].pressureScaleFactor = NULL;

        // Pointer to caps input value for offset pressure during data transfer
        fun3dInstance[iIndex].pressureScaleOffset = NULL;

        // Number of geometric inputs
        fun3dInstance[iIndex].numGeomIn = 0;

        // Pointer to CAPS geometric in values
        fun3dInstance[iIndex].geomInVal = NULL;
    }

    if (fun3dInstance != NULL) EG_free(fun3dInstance);
    fun3dInstance = NULL;
    numInstance = 0;
}


/************************************************************************/
// CAPS transferring functions

int aimFreeDiscr(capsDiscr *discr)
{
    int i;

#ifdef DEBUG
    printf(" fun3dAIM/aimFreeDiscr instance = %d!\n", discr->instance);
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
    printf(" fun3dAIM/aimDiscr: tname = %s, instance = %d!\n", tname, iIndex);
#endif

    if ((iIndex < 0) || (iIndex >= numInstance)) return CAPS_BADINDEX;


    if (tname == NULL) return CAPS_NOTFOUND;

    if (fun3dInstance[iIndex].dataTransferCheck == (int) false) {
        printf("The volume is not suitable for data transfer - possibly the volume mesher "
                "added unaccounted for points\n");
        return CAPS_BADVALUE;
    }

    // Currently this ONLY works if the capsTranfer lives on single body!
    status = aim_getBodies(discr->aInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimDiscr: %d aim_getBodies = %d!\n", iIndex, status);
        return status;
    }

    status = aimFreeDiscr(discr);
    if (status != CAPS_SUCCESS) return status;


    status = aim_getData(discr->aInfo, "Volume_Mesh", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
    if (status != CAPS_SUCCESS){
        if (status == CAPS_DIRTY) printf(" fun3dAIM: Parent AIMS are dirty\n");
        else printf(" fun3dAIM: Linking status = %d\n",status);

        printf(" fun3dAIM/aimDiscr: Didn't find a link to a volume mesh (Volume_Mesh) from parent - data transfer isn't possible.\n");

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

            printf(" fun3dAIM/aimDiscr: Found link for attrMap (Attribute_Map) from parent\n");

            status = copy_mapAttrToIndexStruct( (mapAttrToIndexStruct *) dataTransfer, &fun3dInstance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) goto cleanup;
        } else {

            if (status == CAPS_DIRTY) printf(" fun3dAIM/aimDiscr: Parent AIMS are dirty\n");
            else printf(" fun3dAIM/aimDiscr: Linking status during attrMap (Attribute_Map) = %d\n",status);

            printf(" fun3dAIM/aimDiscr: Didn't find a link to attrMap from parent - getting it ourselves\n");

            // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
            status = create_CAPSGroupAttrToIndexMap(numBody,
                                                    bodies,
                                                    1, // Only search down to the face level of the EGADS body
                                                    &fun3dInstance[iIndex].attrMap);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        // Lets check the volume mesh

        // Do we have an individual surface mesh for each body
        if (volumeMesh->numReferenceMesh != numBody) {
            printf("Number of inherited surface mesh in the volume mesh, %d, does not match the number "
                    "of bodies, %d - data transfer will NOT be possible.\n\n", volumeMesh->numReferenceMesh,numBody);
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

        if (numElementCheck != (volumeMesh->meshQuickRef.numTriangle +
                                volumeMesh->meshQuickRef.numQuadrilateral)) {
            printf("Volume mesher added surface elements - data transfer will NOT be possible.\n");
            status = CAPS_MISMATCH;
            goto cleanup;
        }

        // To this point is doesn't appear that the volume mesh has done anything bad to our surface mesh(es)

        // Lets store away our tessellation now
        for (i = 0; i < volumeMesh->numReferenceMesh; i++) {

            if (aim_newGeometry(discr->aInfo) == CAPS_SUCCESS ) {
#ifdef DEBUG
                printf(" fun3dAIM/aimPreAnalysis instance = %d Setting body tessellation number %d!\n", iIndex, i);
#endif

                status = aim_setTess(discr->aInfo, volumeMesh->referenceMesh[i].bodyTessMap.egadsTess);
                if (status != CAPS_SUCCESS) {
                    printf(" aim_setTess return = %d\n", status);
                    goto cleanup;
                }
            }
        }
    } // needMesh


    numFaceFound = 0;
    numPoint = numTri = 0;
    // Find any faces with our boundary marker and get how many points and triangles there are
    for (body = 0; body < numBody; body++) {

        status = EG_getBodyTopos(bodies[body], NULL, FACE, &numFace, &faces);
        if (status != EGADS_SUCCESS) {
            printf(" fun3dAIM: getBodyTopos (Face) = %d for Body %d!\n", status, body);
            return status;
        }

        for (face = 0; face < numFace; face++) {

            // Retrieve the string following a capsBound tag
            status = retrieve_CAPSBoundAttr(faces[face], &string);
            if (status != CAPS_SUCCESS) continue;

            if (strcmp(string, tname) != 0) continue;

#ifdef DEBUG
            printf(" fun3dAIM/aimDiscr: Body %d/Face %d matches %s!\n", body, face+1, tname);
#endif


            status = retrieve_CAPSGroupAttr(faces[face], &capsGroup);
            if (status != CAPS_SUCCESS) {
                printf("capsBound found on face %d, but no capGroup found!!!\n", face);
                continue;
            } else {

                status = get_mapAttrToIndexIndex(&fun3dInstance[iIndex].attrMap, capsGroup, &attrIndex);
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
                printf(" fun3dAIM: EG_getTessFace %d = %d for Body %d!\n", face+1, status, body+1);
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
        printf(" fun3dAIM/aimDiscr: No Faces match %s!\n", tname);

        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    if ( numPoint == 0 || numTri == 0) {
#ifdef DEBUG
        printf(" fun3dAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
#endif
        status = CAPS_SOURCEERR;
        goto cleanup;
    }

    if ((dataTransferBodyIndex - 1) > volumeMesh->numReferenceMesh ) {
        printf("Data transfer body index doesn't match number of reference meshes in volume mesh - data transfer isn't possible.\n");
        status = CAPS_MISMATCH;
        goto cleanup;
    }

#ifdef DEBUG
    printf(" fun3dAIM/aimDiscr: Instance %d, Body Index for data transfer = %d\n", iIndex, dataTransferBodyIndex);
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
            printf(" fun3dAIM: EG_getTessFace %d = %d for Body %d!\n", bodyFaceMap[2*face + 1], status, bodyFaceMap[2*face + 0]);
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
    printf(" fun3dAIM/aimDiscr: ntris = %d, npts = %d!\n", discr->nElems, discr->nPoints);
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
    printf(" fun3dAIM/aimDiscr: Instance = %d, nodeOffSet = %d, dataTransferBodyIndex = %d\n", iIndex, nodeOffSet, dataTransferBodyIndex);
#endif

    for (i = 0; i < numPoint; i++) {

        storage[i] = globalID[i] + nodeOffSet; //volumeMesh->referenceMesh[dataTransferBodyIndex-1].node[globalID[i]-1].nodeID + nodeOffSet;

        //#ifdef DEBUG
        //	printf(" fun3dAIM/aimDiscr: Instance = %d, Global Node ID %d\n", iIndex, storage[i]);
        //#endif
    }

    // Save way the attrMap capsGroup list
    storage[numPoint] = numCAPSGroup;
    for (i = 0; i < numCAPSGroup; i++) {
        storage[numPoint+1+i] = capsGroupList[i];
    }

#ifdef DEBUG
    printf(" fun3dAIM/aimDiscr: Instance = %d, Finished!!\n", iIndex);
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
  /*! \page aimUsesDataSetFUN3D data sets consumed by FUN3D
   *
   * This function checks if a data set name can be consumed by this aim.
   * The FUN3D aim can consume "Displacement" and EigenValue data sets for areolastic analysis.
   */

  int status = CAPS_NOTNEEDED;

  // Modal Aeroelastic properties
  int eigenIndex = 0;
  capsValue *Modal_Aeroelastic = NULL;
  modalAeroelasticStruct   modalAeroelastic;

  if (strcasecmp(dname, "Displacement") == 0) {
      return CAPS_SUCCESS;
  }

  aim_getValue(aimInfo, aim_getIndex(aimInfo, "Modal_Aeroelastic", ANALYSISIN), ANALYSISIN, &Modal_Aeroelastic);

  if (Modal_Aeroelastic->nullVal ==  NotNull) {

      status = initiate_modalAeroelasticStruct(&modalAeroelastic);
      if (status != CAPS_SUCCESS) return status;

      status = cfd_getModalAeroelastic( Modal_Aeroelastic->length,
                                        Modal_Aeroelastic->vals.tuple,
                                        &modalAeroelastic);
      if (status == CAPS_SUCCESS) {

          status = CAPS_NOTNEEDED;

          // check to see if dname matches one of the eigen value names
          for (eigenIndex = 0; eigenIndex < modalAeroelastic.numEigenValue; eigenIndex++) {
              if (strcasecmp(dname, modalAeroelastic.eigenValue[eigenIndex].name) == 0) {
                  status = CAPS_SUCCESS;
                  break;
              }
          }
      }

      destroy_modalAeroelasticStruct(&modalAeroelastic);
  }

  return status;
}

int aimTransfer(capsDiscr *discr, const char *dataName, int numPoint, int dataRank, double *dataVal, /*@unused@*/ char **units)
{
    /*! \page dataTransferFUN3D FUN3D Data Transfer
     *
     * The FUN3D AIM has the ability to transfer surface data (e.g. pressure distributions) to and from the AIM
     * using the conservative and interpolative data transfer schemes in CAPS. Currently these transfers may only
     * take place on triangular meshes.
     *
     * \section dataFromFUN3D Data transfer from FUN3D
     *
     * <ul>
     *  <li> <B>"Pressure", "P", "Cp", or "CoefficientOfPressure"</B> </li> <br>
     *  Loads the coefficient of pressure distribution from [project_name]_ddfdrive_bndry[#].dat file(s)
     *  (as generate from a FUN3D command line option of -\-write_aero_loads_to_file) into the data
     *  transfer scheme. This distribution may be scaled based on
     *  Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset, where "Pressure_Scale_Factor"
     *  and "Pressure_Scale_Offset" are AIM inputs (\ref aimInputsFUN3D).
     * </ul>
     *
     */ // Reset of this block comes from fun3dUtil.c

    int status, status2; // Function return status
    int i, j, dataPoint, capsGroupIndex; // Indexing

    // Current directory path
    char   currentPath[PATH_MAX];

    // Aero-Load data variables
    int numVariable;
    int numDataPoint;
    char **variableName = NULL;
    double **dataMatrix = NULL;
    double dataScaleFactor = 1.0;
    double dataScaleOffset = 0.0;
    //char *dataUnits = NULL;

    // Indexing in data variables
    int globalIDIndex = -99;
    int variableIndex = -99;

    // Variables used in global node mapping
    int *nodeMap, *storage;
    int globalNodeID;
    int found = (int) false;

    // Filename stuff
    int *capsGroupList;
    char *filename = NULL; //"pyCAPS_FUN3D_Tetgen_ddfdrive_bndry1.dat";

#ifdef DEBUG
    printf(" fun3dAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n", dataName, discr->instance, numPoint, dataRank);
#endif


    if (strcasecmp(dataName, "Pressure") != 0 &&
        strcasecmp(dataName, "P")        != 0 &&
        strcasecmp(dataName, "Cp")       != 0 &&
        strcasecmp(dataName, "CoefficientOfPressure") != 0) {

        printf("Unrecognized data transfer variable - %s\n", dataName);
        return CAPS_NOTFOUND;
    }

    //Get the appropriate parts of the tessellation to data
    storage = (int *) discr->ptrm;
    nodeMap = &storage[0]; // Global indexing on the body

    capsGroupList = &storage[discr->nPoints]; // List of boundary ID (attrMap) in the transfer

    // Zero out data
    for (i = 0; i < numPoint; i++) {
        for (j = 0; j < dataRank; j++ ) {
            dataVal[dataRank*i+j] = 0;
        }
    }

    for (capsGroupIndex = 0; capsGroupIndex < capsGroupList[0]; capsGroupIndex++) {

        (void) getcwd(currentPath, PATH_MAX);
        if (chdir(fun3dInstance[discr->instance].analysisPath) != 0) {
#ifdef DEBUG
            printf(" fun3dAIM/aimTransfer Cannot chdir to %s!\n", fun3dInstance[discr->instance].analysisPath);
#endif

            return CAPS_DIRERR;
        }

        filename = (char *) EG_alloc((strlen(fun3dInstance[discr->instance].projectName) +
                                      strlen("_ddfdrive_bndry.dat") + 7)*sizeof(char));

        sprintf(filename,"%s%s%d%s",fun3dInstance[discr->instance].projectName,
                                    "_ddfdrive_bndry",
                                    capsGroupList[capsGroupIndex+1],
                                    ".dat");

        status = fun3d_readAeroLoad(filename,
                                    &numVariable,
                                    &variableName,
                                    &numDataPoint,
                                    &dataMatrix);
        // Try body file
        if (status == CAPS_IOERR) {

            filename = (char *) EG_reall(filename, (strlen(fun3dInstance[discr->instance].projectName) +
                                                    strlen("_ddfdrive_body1.dat") + 5)*sizeof(char));

            sprintf(filename,"%s%s%s",fun3dInstance[discr->instance].projectName,
                                      "_ddfdrive_body1",
                                      ".dat");

            printf("Instead trying file : %s\n", filename);

            status = fun3d_readAeroLoad(filename,
                                        &numVariable,
                                        &variableName,
                                        &numDataPoint,
                                        &dataMatrix);
        }

        if (filename != NULL) EG_free(filename);
        filename = NULL;

        // Restore the path we came in with
        chdir(currentPath);
        if (status != CAPS_SUCCESS) return status;

        printf("Number of variables %d\n", numVariable);
        // Output some of the first row of the data
        //for (i = 0; i < numVariable; i++) printf("Variable %d - %.6f\n", i, dataMatrix[i][0]);

        // Loop through the variable list to find which one is the global node ID variable
        for (i = 0; i < numVariable; i++) {
            if (strcasecmp("id", variableName[i]) ==0 ) {
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
                strcasecmp(dataName, "P")        == 0 ||
                strcasecmp(dataName, "Cp")       == 0 ||
                strcasecmp(dataName, "CoefficientOfPressure") == 0) {

                if (dataRank != 1) {
                    printf("Data transfer rank should be 1 not %d\n", dataRank);
                    status = CAPS_BADRANK;
                    goto bail;
                }

                dataScaleFactor = fun3dInstance[discr->instance].pressureScaleFactor->vals.real;
                dataScaleOffset = fun3dInstance[discr->instance].pressureScaleOffset->vals.real;

                //dataUnits = fun3dInstance[discr->instance].pressureScaleFactor->units;

                if (strcasecmp("cp", variableName[i]) == 0) {
                    variableIndex = i;
                    break;
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
                if ((int) dataMatrix[globalIDIndex][dataPoint] ==  globalNodeID) {
                    found = (int) true;
                    break;
                }
            }

            if (found == (int) true) {
                for (j = 0; j < dataRank; j++) {

                    // Add something for units - aim_covert()

                    dataVal[dataRank*i+j] = dataMatrix[variableIndex][dataPoint]*dataScaleFactor + dataScaleOffset;
                    //printf("DataValue = %f\n",dataVal[dataRank*i+j]);
                    //dataVal[dataRank*i+j] = 99;

                }
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

    }

    return CAPS_SUCCESS;

    bail:
        if (status != CAPS_SUCCESS) printf("Premature exit in fun3DAIM transfer status = %d\n", status);

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
  	  printf(" fun3dAIM/aimInterpolation  %s  instance = %d!\n", name, discr->instance);
	#endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" fun3dAIM/Interpolation: eIndex = %d [1-%d]!\n",eIndex, discr->nElems);
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
  	  printf(" fun3dAIM/aimInterpolateBar  %s  instance = %d!\n", name, discr->instance);
	#endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" fun3dAIM/InterpolateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
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
  	  printf(" fun3dAIM/aimIntegration  %s  instance = %d!\n", name, discr->instance);
	#endif
     */
    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" fun3dAIM/aimIntegration: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegration: %d aim_getBodies = %d!\n", discr->instance, stat);
        return stat;
    }

    /* element indices */

    in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
    in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
    in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

    stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
            discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegration: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
  	  printf(" fun3dAIM/aimIntegrateBar  %s  instance = %d!\n", name, discr->instance);
	#endif
     */

    if ((eIndex <= 0) || (eIndex > discr->nElems)) {
        printf(" fun3dAIM/aimIntegrateBar: eIndex = %d [1-%d]!\n", eIndex, discr->nElems);
    }

    stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegrateBar: %d aim_getBodies = %d!\n", discr->instance, stat);
        return stat;
    }

    /* element indices */

    in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
    in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
    in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
    stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
            discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[0], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
            discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);

    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[1], stat);
        return stat;
    }

    stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
            discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);

    if (stat != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n", discr->instance, in[2], stat);
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
