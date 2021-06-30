/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             FUN3D AIM
 *
 *     Written by Dr. Ryan Durscher AFRL/RQVC
 *
 *     This software has been cleared for public release on 05 Nov 2020, case number 88ABW-2020-3462.
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
 * \section clearanceFUN3D Clearance Statement
 *  This software has been cleared for public release on 05 Nov 2020, case number 88ABW-2020-3462.
 */

#ifdef HAVE_PYTHON

#include "Python.h" // Bring in Python API

#include "fun3dNamelist.h" // Bring in Cython generated header file
// for namelist generation wiht Python

#ifndef CYTHON_PEP489_MULTI_PHASE_INIT
#define CYTHON_PEP489_MULTI_PHASE_INIT (PY_VERSION_HEX >= 0x03050000)
#endif

#if CYTHON_PEP489_MULTI_PHASE_INIT
static int fun3dNamelist_Initialized = (int)false;
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
#define snprintf   _snprintf
#define strcasecmp stricmp
#endif

/*
#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
 */

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

    // Pointer to CAPS input value for scaling pressure during data transfer
    capsValue *pressureScaleFactor;

    // Pointer to CAPS input value for offset pressure during data transfer
    capsValue *pressureScaleOffset;

    // Design information
    cfdDesignStruct design;

} aimStorage;


// ********************** Exposed AIM Functions *****************************

int aimInitialize(int inst, /*@null@*/ /*@unused@*/ const char *unitSys, void *aimInfo,
                  void **instStore, /*@unused@*/ int *major,
                  /*@unused@*/ int *minor, int *nIn, int *nOut,
                  int *nFields, char ***fnames, int **franks, int **fInOut)
{

    int  status = CAPS_SUCCESS; // Function status return

    int  *ints=NULL, i;
    char **strs=NULL;

    aimStorage *fun3dInstance=NULL;

#ifdef DEBUG
    printf("\n fun3dAIM/aimInitialize  inst = %d!\n", inst);
#endif

    // Specify the number of analysis input and out "parameters"
    *nIn     = NUMINPUT;
    *nOut    = NUMOUTPUT;
    if (inst == -1) return CAPS_SUCCESS;

    /* specify the field variables this analysis can generate and consume */
    *nFields = 7;

    /* specify the name of each field variable */
    AIM_ALLOC(strs, *nFields, char *, aimInfo, status);
    strs[0]  = EG_strdup("Pressure");
    strs[1]  = EG_strdup("P");
    strs[2]  = EG_strdup("Cp");
    strs[3]  = EG_strdup("CoefficientOfPressure");
    strs[4]  = EG_strdup("Displacement");
    strs[5]  = EG_strdup("EigenVector");
    strs[6]  = EG_strdup("EigenVector_#");
    for (i = 0; i < *nFields; i++)
      if (strs[i] == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
      }
    *fnames  = strs;

    /* specify the dimension of each field variable */
    AIM_ALLOC(ints, *nFields, int, aimInfo, status);

    ints[0]  = 1;
    ints[1]  = 1;
    ints[2]  = 1;
    ints[3]  = 1;
    ints[4]  = 3;
    ints[5]  = 3;
    ints[6]  = 3;
    *franks   = ints;
    ints = NULL;

    /* specify if a field is an input field or output field */
    AIM_ALLOC(ints, *nFields, int, aimInfo, status);

    ints[0]  = FieldOut;
    ints[1]  = FieldOut;
    ints[2]  = FieldOut;
    ints[3]  = FieldOut;
    ints[4]  = FieldIn;
    ints[5]  = FieldIn;
    ints[6]  = FieldIn;
    *fInOut  = ints;
    ints = NULL;


    // Allocate fun3dInstance
    AIM_ALLOC(fun3dInstance, 1, aimStorage, aimInfo, status);
    *instStore = fun3dInstance;

    // Set initial values for fun3dInstance
    fun3dInstance->projectName = NULL;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&fun3dInstance->attrMap);
    AIM_STATUS(aimInfo, status);

    // Check to make sure data transfer is ok
    fun3dInstance->dataTransferCheck = (int) true;

    // Pointer to caps input value for scaling pressure during data transfer
    fun3dInstance->pressureScaleFactor = NULL;

    // Pointer to caps input value for off setting pressure during data transfer
    fun3dInstance->pressureScaleOffset = NULL;

    // Design information
    status = initiate_cfdDesignStruct(&fun3dInstance->design);
    AIM_STATUS(aimInfo, status);

cleanup:
    if (status != CAPS_SUCCESS) {
        /* release all possibly allocated memory on error */
        if (*fnames != NULL)
          for (i = 0; i < *nFields; i++) AIM_FREE((*fnames)[i]);
        AIM_FREE(*franks);
        AIM_FREE(*fInOut);
        AIM_FREE(*fnames);
        AIM_FREE(*instStore);
        *nFields = 0;
    }

    return status;
}


int aimInputs(void *instStore, /*@unused@*/ void *aimInfo, int index,
              char **ainame, capsValue *defval)
{
    /*! \page aimInputsFUN3D AIM Inputs
     * The following list outlines the FUN3D inputs along with their default values available
     * through the AIM interface. One will note most of the FUN3D parameters have a NULL value as their default.
     * This is done since a parameter in the FUN3D input deck (fun3d.nml) is only changed if the value has
     * been changed in CAPS (i.e. set to something other than NULL).
     */

    int status = CAPS_SUCCESS;
    aimStorage *fun3dInstance;

#ifdef DEBUG
    printf(" fun3dAIM/aimInputs index = %d!\n", index);
#endif
    fun3dInstance = (aimStorage *) instStore;
    if (fun3dInstance == NULL) return CAPS_NULLVALUE;

    *ainame = NULL;
  
    // FUN3D Inputs
    if (index == Proj_Name) {
        *ainame              = EG_strdup("Proj_Name");
        defval->type         = String;
        defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("fun3d_CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B> Proj_Name = "fun3d_CAPS"</B> <br>
         * This corresponds to the project\_rootname variable in the \&project namelist of fun3d.nml.
         */
    } else if (index == Mach) {
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
    } else if (index == Re) {
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
    } else if (index == Viscoux) {
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
    } else if (index == Equation_Type) {
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
    } else if (index == Alpha) {
        *ainame              = EG_strdup("Alpha");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;
        //defval->units = EG_strdup("degree");

        /*! \page aimInputsFUN3D
         * - <B> Alpha = NULL </B> <br>
         *  This corresponds to the angle\_of\_attack variable in the \&reference\_physical\_properties
         *  namelist of fun3d.nml [degree].
         */
    } else if (index == Beta) {
        *ainame              = EG_strdup("Beta");
        defval->type         = Double;
        defval->nullVal      = IsNull;
        defval->units        = NULL;
        defval->lfixed       = Change;
        defval->dim          = Scalar;
        //defval->units = EG_strdup("degree");

        /*! \page aimInputsFUN3D
         * - <B> Beta = NULL </B> <br>
         *  This corresponds to the angle\_of\_yaw variable in the \&reference\_physical\_properties
         *  namelist of fun3d.nml [degree].
         */
    } else if (index == Overwrite_NML) {
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
    } else if (index == Mesh_Format) {
        *ainame              = EG_strdup("Mesh_Format");
        defval->type         = String;
        defval->vals.string  = EG_strdup("AFLR3");
        defval->lfixed       = Change;

        /*! \page aimInputsFUN3D
         * - <B>Mesh_Format = "AFLR3"</B> <br>
         *  Mesh output format. By default, an AFLR3 mesh will be used.
         */
    } else if (index == Mesh_ASCII_Flag) {
        *ainame              = EG_strdup("Mesh_ASCII_Flag");
        defval->type         = Boolean;
        defval->vals.integer = true;

        /*! \page aimInputsFUN3D
         * - <B>Mesh_ASCII_Flag = True</B> <br>
         *  Output mesh in ASCII format, otherwise write a binary file if applicable.
         */
    } else if (index == Num_Iter) {
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
    } else if (index == CFL_Schedule) {
        *ainame               = EG_strdup("CFL_Schedule");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->nrow          = 2;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;

        AIM_ALLOC(defval->vals.reals, defval->nrow, double, aimInfo, status);
        defval->vals.reals[0] = defval->vals.reals[1] = 0.0;

        /*! \page aimInputsFUN3D
         * - <B>CFL_Schedule = NULL</B> <br>
         *  This corresponds to the schedule\_cfl variable in the \&nonlinear\_solver\_parameters
         *   namelist of fun3d.nml.
         */
    } else if (index == CFL_Schedule_Iter) {
        *ainame               = EG_strdup("CFL_Schedule_Iter");
        defval->type          = Integer;
        defval->dim           = Vector;
        defval->nrow          = 2;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;

        AIM_ALLOC(defval->vals.integers, defval->nrow, int, aimInfo, status);
        defval->vals.integers[0] = defval->vals.integers[1] = 0;

        /*! \page aimInputsFUN3D
         * - <B>CFL_Schedule_Inter = NULL</B> <br>
         *  This corresponds to the schedule\_iteration variable in the \&nonlinear\_solver\_parameters
         *  namelist of fun3d.nml.
         */
    } else if (index == Restart_Read) {
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
    } else if (index == Boundary_Condition) {
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
    } else if (index == Use_Python_NML) {
        *ainame              = EG_strdup("Use_Python_NML");
        defval->type         = Boolean;
        defval->vals.integer = (int) false;

        /*! \page aimInputsFUN3D
         * - <B>Use_Python_NML = False </B> <br>
         * By default, even if Python has been linked to the FUN3D AIM it is not used unless the this value is set to True.
         */
    } else if (index == Pressure_Scale_Factor) {
        *ainame              = EG_strdup("Pressure_Scale_Factor");
        defval->type         = Double;
        defval->vals.real    = 1.0;
        defval->units           = NULL;

        fun3dInstance->pressureScaleFactor = defval;

        /*! \page aimInputsFUN3D
         * - <B>Pressure_Scale_Factor = 1.0</B> <br>
         * Value to scale Cp data when transferring data. Data is scaled based on Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset.
         */
    } else if (index == Pressure_Scale_Offset) {
        *ainame              = EG_strdup("Pressure_Scale_Offset");
        defval->type         = Double;
        defval->vals.real    = 0.0;
        defval->units           = NULL;

        fun3dInstance->pressureScaleOffset = defval;

        /*! \page aimInputsFUN3D
         * - <B>Pressure_Scale_Offset = 0.0</B> <br>
         * Value to offset Cp data when transferring data. Data is scaled based on Pressure = Pressure_Scale_Factor*Cp + Pressure_Scale_Offset.
         */
    } else if (index == NonInertial_Rotation_Rate) {
        *ainame              = EG_strdup("NonInertial_Rotation_Rate");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->nrow*sizeof(double));
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
    } else if (index == NonInertial_Rotation_Center) {
        *ainame              = EG_strdup("NonInertial_Rotation_Center");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->nrow*sizeof(double));
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
    } else if (index == Two_Dimensional) {
        *ainame              = EG_strdup("Two_Dimensional");
        defval->type         = Boolean;
        defval->vals.integer = (int) false;

        /*! \page aimInputsFUN3D
         * - <B>Two_Dimensional = False</B> <br>
         * Run FUN3D in 2D mode. If set to True, the body must be a single "sheet" body in the x-z plane (a rudimentary node
         * swapping routine is attempted if not in the x-z plane). A 3D mesh will be written out,
         * where the body is extruded a length of 1 in the y-direction.
         */
    } else if (index == Modal_Aeroelastic) {
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
    } else if (index == Modal_Ref_Velocity) {
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
    }  else if (index == Modal_Ref_Length) {
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
    }  else if (index == Modal_Ref_Dynamic_Pressure) {
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
    } else if (index == Time_Accuracy) {
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
    } else if (index == Time_Step) {
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
    } else if (index == Num_Subiter) {
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
    } else if (index == Temporal_Error) {
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
    } else if (index == Reference_Area) {
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
    } else if (index == Moment_Length) {
        *ainame              = EG_strdup("Moment_Length");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->nrow          = 2;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->nrow*sizeof(double));
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
    } else if (index == Moment_Center) {
        *ainame              = EG_strdup("Moment_Center");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->nrow*sizeof(double));
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
    } else if (index == FUN3D_Version) {
        *ainame              = EG_strdup("FUN3D_Version");
        defval->type       = Double;
        defval->dim        = Scalar;
        defval->units      = NULL;
        defval->vals.real  = 13.1;
        defval->lfixed     = Fixed;

        /*! \page aimInputsFUN3D
         * - <B>FUN3D_Version = 13.1 </B> <br>
         * FUN3D version to generate specific configuration file for; currently only has influence over
         * rubber.data (sensitivity file) and aeroelastic modal data namelist in moving_body.input .
         */
    } else if (index == Design_Variable) {
        *ainame              = EG_strdup("Design_Variable");
         defval->type         = Tuple;
         defval->nullVal      = IsNull;
         //defval->units        = NULL;
         defval->lfixed       = Change;
         defval->vals.tuple   = NULL;
         defval->dim          = Vector;

         /*! \page aimInputsFUN3D
          * - <B> Design_Variable = NULL</B> <br>
          * The design variable tuple is used to input design variable information for optimization, see \ref cfdDesignVariable for additional details.
          */
    } else if (index == Design_Objective) {
        *ainame              = EG_strdup("Design_Objective");
         defval->type         = Tuple;
         defval->nullVal      = IsNull;
         //defval->units        = NULL;
         defval->lfixed       = Change;
         defval->vals.tuple   = NULL;
         defval->dim          = Vector;

         /*! \page aimInputsFUN3D
          * - <B> Design_Objective = NULL</B> <br>
          * The design objective tuple is used to input objective information for optimization, see \ref cfdDesignObjective for additional details.
          */
    } else if (index == Mesh) {
        *ainame             = AIM_NAME(Mesh);
        defval->type        = Pointer;
        defval->nrow        = 1;
        defval->lfixed      = Fixed;
        defval->vals.AIMptr = NULL;
        defval->nullVal     = IsNull;
        AIM_STRDUP(defval->units, "meshStruct", aimInfo, status);

        /*! \page aimInputsFUN3D
         * - <B>Mesh = NULL</B> <br>
         * A Surface_Mesh or Volume_Mesh link for 2D and 3D calculations respectively.
         */

    } else {
        status = CAPS_BADINDEX;
        AIM_STATUS(aimInfo, status, "Unknown input index %d!", index);
    }

    AIM_NOTNULL(*ainame, aimInfo, status);

cleanup:
    if (status != CAPS_SUCCESS) AIM_FREE(*ainame);
    return status;
}


int aimPreAnalysis(void *instStore, void *aimInfo, capsValue *aimInputs)
{

    // Function return flag
    int status;

    // Python linking
    int pythonLinked = (int) false;
    int usePython = (int) false;

    // Indexing
    int i, body;

    int attrLevel = 0;
    int       nGeomIn;
    capsValue *geomInVal = NULL;

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

    // FUN3D Version
    double fun3dVersion;

    // Optimization/Design
    int optimization = (int) false;

    // Boundary/surface properties
    cfdBoundaryConditionStruct   bcProps;

    // Modal Aeroelastic properties
    cfdModalAeroelasticStruct   modalAeroelastic;

    // Boundary conditions container - for writing .mapbc file
    bndCondStruct bndConds;

    // Volume Mesh obtained from meshing AIM
    meshStruct *meshlink = NULL;
    meshStruct *surfaceMesh = NULL;
    meshStruct *volumeMesh = NULL;
    int numElementCheck; // Consistency checkers for volume-surface meshes

    // Discrete data transfer variables
    char **transferName = NULL;
    int numTransferName;

#ifdef HAVE_PYTHON
    PyObject* mobj = NULL;
#if CYTHON_PEP489_MULTI_PHASE_INIT
    PyModuleDef *mdef = NULL;
    PyObject *modname = NULL;
#endif
#endif

    aimStorage *fun3dInstance;

    fun3dInstance = (aimStorage *) instStore;
    if ((fun3dInstance == NULL) || (aimInputs == NULL)) return CAPS_NULLVALUE;

    // Initiate structures variables - will be destroyed during cleanup
    status = initiate_cfdBoundaryConditionStruct(&bcProps);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_cfdModalAeroelasticStruct(&modalAeroelastic);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_bndCondStruct(&bndConds);
    if (status != CAPS_SUCCESS) return status;

    nGeomIn = aim_getIndex(aimInfo, NULL, GEOMETRYIN);
    if (nGeomIn > 0) {
        status = aim_getValue(aimInfo, 1, GEOMETRYIN, &geomInVal);
        if (status != CAPS_SUCCESS) {
            printf("Error: Cannot get Geometry In Value Structures\n");
            return status;
        }
    }
  
    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    AIM_STATUS(aimInfo, status);

#ifdef DEBUG
    printf(" fun3dAIM/aimPreAnalysis numBody = %d!\n", numBody);
#endif

    if ((numBody <= 0) || (bodies == NULL)) {
#ifdef DEBUG
        printf(" fun3dAIM/aimPreAnalysis No Bodies!\n");
#endif
        return CAPS_SOURCEERR;
    }

    // Get Version number
    fun3dVersion = aimInputs[FUN3D_Version-1].vals.real;

    // Get reference quantities from the bodies
    for (body=0; body < numBody; body++) {

        if (aimInputs[Reference_Area-1].nullVal == IsNull) {

            status = EG_attributeRet(bodies[body], "capsReferenceArea", &atype,
                                     &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[Reference_Area-1].vals.real = (double) reals[0];
                    aimInputs[Reference_Area-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceArea should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }
        }

        if (aimInputs[Moment_Length-1].nullVal == IsNull) {

            status = EG_attributeRet(bodies[body], "capsReferenceChord", &atype,
                                     &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS){

                if (atype == ATTRREAL) {
                    aimInputs[Moment_Length-1].vals.reals[0] = (double) reals[0];
                    aimInputs[Moment_Length-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceChord should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }

            }

            status = EG_attributeRet(bodies[body], "capsReferenceSpan", &atype,
                                     &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[Moment_Length-1].vals.reals[1] = (double) reals[0];
                    aimInputs[Moment_Length-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceSpan should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }
        }

        if (aimInputs[Moment_Center-1].nullVal == IsNull) {

            status = EG_attributeRet(bodies[body], "capsReferenceX", &atype,
                                     &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[Moment_Center-1].vals.reals[0] = (double) reals[0];
                    aimInputs[Moment_Center-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceX should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }

            status = EG_attributeRet(bodies[body], "capsReferenceY", &atype,
                                     &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[Moment_Center-1].vals.reals[1] = (double) reals[0];
                    aimInputs[Moment_Center-1].nullVal = NotNull;
                } else {
                    printf("capsReferenceY should be followed by a single real value!\n");
                    status = CAPS_BADVALUE;
                    goto cleanup;
                }
            }

            status = EG_attributeRet(bodies[body], "capsReferenceZ", &atype,
                                     &alen, &ints, &reals, &string);
            if (status == EGADS_SUCCESS) {

                if (atype == ATTRREAL) {
                    aimInputs[Moment_Center-1].vals.reals[2] = (double) reals[0];
                    aimInputs[Moment_Center-1].nullVal = NotNull;
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
    usePython = aimInputs[Use_Python_NML-1].vals.integer;
    if (usePython == (int) true && pythonLinked == (int) false) {

        printf("Use of Python library requested but not linked!\n");
        usePython = (int) false;

    } else if (usePython == (int) false && pythonLinked == (int) true) {

        printf("Python library was linked, but will not be used!\n");
    }

    // Get project name
    fun3dInstance->projectName = aimInputs[Proj_Name-1].vals.string;

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

    // Get attribute to index mapping
    if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {
        if (aimInputs[Two_Dimensional-1].vals.integer == (int) true) {
          attrLevel = 2; // Only search down to the edge level of the EGADS body
        } else {
          attrLevel = 1; // Only search down to the face level of the EGADS body
        }

        // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
        status = create_CAPSGroupAttrToIndexMap(numBody,
                                                bodies,
                                                attrLevel,
                                                &fun3dInstance->attrMap);
        AIM_STATUS(aimInfo, status);
    }

    // Get boundary conditions - Only if the boundary condition has been set
    if (aimInputs[Boundary_Condition-1].nullVal ==  NotNull) {

        status = cfd_getBoundaryCondition( aimInfo,
                                           aimInputs[Boundary_Condition-1].length,
                                           aimInputs[Boundary_Condition-1].vals.tuple,
                                           &fun3dInstance->attrMap,
                                           &bcProps);
        AIM_STATUS(aimInfo, status);

    } else {
        AIM_ANALYSISIN_ERROR(aimInfo, Boundary_Condition, "No boundary conditions provided!");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // Get modal aeroelastic information - only get modal aeroelastic inputs if they have be set
    if (aimInputs[Modal_Aeroelastic-1].nullVal ==  NotNull) {

        status = cfd_getModalAeroelastic( aimInputs[Modal_Aeroelastic-1].length,
                                          aimInputs[Modal_Aeroelastic-1].vals.tuple,
                                         &modalAeroelastic);
        AIM_STATUS(aimInfo, status);


        modalAeroelastic.freestreamVelocity = aimInputs[Modal_Ref_Velocity-1].vals.real;
        modalAeroelastic.freestreamDynamicPressure = aimInputs[Modal_Ref_Dynamic_Pressure-1].vals.real;
        modalAeroelastic.lengthScaling = aimInputs[Modal_Ref_Length-1].vals.real;
    }

    // Get design variables
    if (aimInputs[Design_Variable-1].nullVal == NotNull) {
/*@-nullpass@*/
        status = cfd_getDesignVariable(aimInputs[Design_Variable-1].length,
                                       aimInputs[Design_Variable-1].vals.tuple,
                                       aimInfo,
                                       NUMINPUT, aimInputs,
                                       nGeomIn,  geomInVal,
                                       &fun3dInstance->design.numDesignVariable,
                                       &fun3dInstance->design.designVariable);
/*@+nullpass@*/
        AIM_STATUS(aimInfo, status);

        optimization = (int) true;
    }

    // Get design objectives
    if (aimInputs[Design_Objective-1].nullVal == NotNull) {

        if (optimization == (int) false) {
            printf("\"Design_Objective\" has been set, but no values have been provided for \"Design_Variable\"!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }
        status = cfd_getDesignObjective(aimInputs[Design_Objective-1].length,
                                        aimInputs[Design_Objective-1].vals.tuple,
                                        &fun3dInstance->design.numDesignObjective,
                                        &fun3dInstance->design.designObjective);
        AIM_STATUS(aimInfo, status);

    } else {

        if (optimization == (int) true) { // Create a default objective

            printf("Creation of a default objective functions is not supported yet, user must provide an input for \"Design_Objective\"!\n");
            status = CAPS_NOTIMPLEMENT;
            goto cleanup;
        }
    }

    if (aimInputs[Mesh-1].nullVal == IsNull) {
        AIM_ANALYSISIN_ERROR(aimInfo, Mesh, "'Mesh' input must be linked to an output 'Surface_Mesh' or 'Volume_Mesh'");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // Get mesh
    meshlink = (meshStruct *)aimInputs[Mesh-1].vals.AIMptr;
    AIM_NOTNULL(meshlink, aimInfo, status);

    // Are we running in two-mode
    if (aimInputs[Two_Dimensional-1].vals.integer == (int) true) {

        if (numBody > 1) {
            AIM_ERROR(aimInfo, "Only 1 body may be provided when running in two mode!! numBody = %d", numBody);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        for (body = 0; body < numBody; body++) {
            // What type of BODY do we have?
            status = EG_getTopology(bodies[body], &bodyRef, &bodyOClass,
                                    &bodySubType, bodyData, &bodyNumChild,
                                    &bodyChild, &bodySense);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (bodySubType != FACEBODY && bodySubType != SHEETBODY) {
                printf("Body type must be either FACEBODY (%d) or a SHEETBODY (%d) when running in two mode!\n",
                       FACEBODY, SHEETBODY);
                status = CAPS_BADTYPE;
                goto cleanup;
            }
        }

        // Add extruded plane boundary condition
        bcProps.numSurfaceProp += 1;
        bcProps.surfaceProp = (cfdSurfaceStruct *) EG_reall(bcProps.surfaceProp,
                             bcProps.numSurfaceProp * sizeof(cfdSurfaceStruct));
        if (bcProps.surfaceProp == NULL) return EGADS_MALLOC;

        status = initiate_cfdSurfaceStruct(&bcProps.surfaceProp[bcProps.numSurfaceProp-1]);
        AIM_STATUS(aimInfo, status);

        bcProps.surfaceProp[bcProps.numSurfaceProp-1].surfaceType = Symmetry;
        bcProps.surfaceProp[bcProps.numSurfaceProp-1].symmetryPlane = 2;

        // Find largest index value for bcID and set it plus 1 to the new surfaceProp
        for (i = 0; i < bcProps.numSurfaceProp-1; i++) {

            if (bcProps.surfaceProp[i].bcID >= bcProps.surfaceProp[bcProps.numSurfaceProp-1].bcID) {
                bcProps.surfaceProp[bcProps.numSurfaceProp-1].bcID = bcProps.surfaceProp[i].bcID + 1;
            }
        }

        // Extrude Surface mesh
        surfaceMesh = meshlink;

        volumeMesh = (meshStruct *) EG_alloc(sizeof(meshStruct));
        if (volumeMesh == NULL) {
          status = EGADS_MALLOC;
          goto cleanup;
        }

        status = initiate_meshStruct(volumeMesh);
        AIM_STATUS(aimInfo, status);

        status = fun3d_2DMesh(surfaceMesh,
                              &fun3dInstance->attrMap,
                              volumeMesh,
                              &bcProps.surfaceProp[bcProps.numSurfaceProp-1].bcID);
        AIM_STATUS(aimInfo, status);

        // Can't currently do data transfer in 2D-mode
        fun3dInstance->dataTransferCheck = (int) false;

    } else {
        // Get Volume mesh
        volumeMesh = (meshStruct *) meshlink;
    }

    if (status == CAPS_SUCCESS) {

        status = populate_bndCondStruct_from_bcPropsStruct(&bcProps, &bndConds);
        AIM_STATUS(aimInfo, status);

        // Replace dummy values in bcVal with FUN3D specific values
        for (i = 0; i < bcProps.numSurfaceProp ; i++) {

            // {UnknownBoundary, Inviscid, Viscous, Farfield, Extrapolate, Freestream,
            //  BackPressure, Symmetry, SubsonicInflow, SubsonicOutflow,
            //  MassflowIn, MassflowOut, FixedInflow, FixedOutflow}


            if      (bcProps.surfaceProp[i].surfaceType == Inviscid       ) bndConds.bcVal[i] = 3000;
            else if (bcProps.surfaceProp[i].surfaceType == Viscous        ) bndConds.bcVal[i] = 4000;
            else if (bcProps.surfaceProp[i].surfaceType == Farfield       ) bndConds.bcVal[i] = 5000;
            else if (bcProps.surfaceProp[i].surfaceType == Extrapolate    ) bndConds.bcVal[i] = 5026;
            else if (bcProps.surfaceProp[i].surfaceType == Freestream     ) bndConds.bcVal[i] = 5050;
            else if (bcProps.surfaceProp[i].surfaceType == BackPressure   ) bndConds.bcVal[i] = 5051;
            else if (bcProps.surfaceProp[i].surfaceType == SubsonicInflow ) bndConds.bcVal[i] = 7011;
            else if (bcProps.surfaceProp[i].surfaceType == SubsonicOutflow) bndConds.bcVal[i] = 7012;
            else if (bcProps.surfaceProp[i].surfaceType == MassflowIn     ) bndConds.bcVal[i] = 7036;
            else if (bcProps.surfaceProp[i].surfaceType == MassflowOut    ) bndConds.bcVal[i] = 7031;
            else if (bcProps.surfaceProp[i].surfaceType == FixedInflow    ) bndConds.bcVal[i] = 7100;
            else if (bcProps.surfaceProp[i].surfaceType == FixedOutflow   ) bndConds.bcVal[i] = 7105;
            else if (bcProps.surfaceProp[i].surfaceType == MachOutflow    ) bndConds.bcVal[i] = 5052;
            else if (bcProps.surfaceProp[i].surfaceType == Symmetry       ) {

                if      (bcProps.surfaceProp[i].symmetryPlane == 1) bndConds.bcVal[i] = 6661;
                else if (bcProps.surfaceProp[i].symmetryPlane == 2) bndConds.bcVal[i] = 6662;
                else if (bcProps.surfaceProp[i].symmetryPlane == 3) bndConds.bcVal[i] = 6663;
                else {
                    printf("Unknown symmetryPlane for boundary %d - Defaulting to y-Symmetry\n", bcProps.surfaceProp[i].bcID);
                    bndConds.bcVal[i] = 6662;
                }
            }
        }

        filename = (char *) EG_alloc(MXCHAR +1);
        if (filename == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }
        strcpy(filename, fun3dInstance->projectName);

        if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

            // Write AFLR3
/*@-nullpass@*/
            status = mesh_writeAFLR3(aimInfo, filename,
                                     aimInputs[Mesh_ASCII_Flag-1].vals.integer,
                                     volumeMesh,
                                     1.0);
/*@+nullpass@*/
            if (status != CAPS_SUCCESS) {
                goto cleanup;
            }
        }

        // Write *.mapbc file
        status = write_MAPBC(aimInfo, filename,
                             bndConds.numBND,
                             bndConds.bndID,
                             bndConds.bcVal);
        if (filename != NULL) EG_free(filename);
        filename = NULL;

        AIM_STATUS(aimInfo, status);

        // Lets check the volume mesh
        if (volumeMesh == NULL) {
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        // Do we have an individual surface mesh for each body
        if (volumeMesh->numReferenceMesh != numBody &&
            aimInputs[Two_Dimensional-1].vals.integer == (int) false) {
            printf("Number of linked surface mesh in the volume mesh, %d, does not match the number "
                    "of bodies, %d - data transfer will NOT be possible.",
                   volumeMesh->numReferenceMesh, numBody);
            fun3dInstance->dataTransferCheck = (int) false;
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
            AIM_STATUS(aimInfo, status);

            status = mesh_retrieveNumMeshElements(volumeMesh->numElement,
                                                  volumeMesh->element,
                                                  Quadrilateral,
                                                  &volumeMesh->meshQuickRef.numQuadrilateral);
            AIM_STATUS(aimInfo, status);

        }

        if (numElementCheck != (volumeMesh->meshQuickRef.numTriangle + volumeMesh->meshQuickRef.numQuadrilateral)) {

            fun3dInstance->dataTransferCheck = (int) false;
            printf("Volume mesher added surface elements - data transfer will NOT be possible.\n");

        } else { // Data transfer appears to be ok
            fun3dInstance->dataTransferCheck = (int) true;
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
                Py_InitializeEx(0);
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

            PyGILState_STATE gstate;
            gstate = PyGILState_Ensure();

             // Taken from "main" by running cython with --embed
            #if PY_MAJOR_VERSION < 3
                initfun3dNamelist();
            #elif CYTHON_PEP489_MULTI_PHASE_INIT
                   if (fun3dNamelist_Initialized == (int)false || initPy == (int)true) {
                    fun3dNamelist_Initialized = (int)true;
                       mobj = PyInit_fun3dNamelist();
                       if (!PyModule_Check(mobj)) {
                        mdef = (PyModuleDef *) mobj;
                        modname = PyUnicode_FromString("fun3dNamelist");
                        mobj = NULL;
                        if (modname) {
                            mobj = PyModule_NewObject(modname);
                            Py_DECREF(modname);
                            if (mobj) PyModule_ExecDef(mobj, mdef);
                          }
                    }
                }
            #else
                mobj = PyInit_fun3dNamelist();
            #endif
            
            if (PyErr_Occurred()) {
                PyErr_Print();
                #if PY_MAJOR_VERSION < 3
                if (Py_FlushLine()) PyErr_Clear();
                #endif
                /* Release the thread. No Python API allowed beyond this point. */
                PyGILState_Release(gstate);
                status = CAPS_BADVALUE;
                goto cleanup;
            }
            
            Py_XDECREF(mobj);

            status = fun3d_writeNMLPython(aimInfo, aimInputs, bcProps);
            if (status == -1) {
                printf("\tError: Python error occurred while writing namelist file\n");
            } else {
                printf("\tDone writing nml file with Python\n");
            }

            if (PyErr_Occurred()) {
                PyErr_Print();
                #if PY_MAJOR_VERSION < 3
                if (Py_FlushLine()) PyErr_Clear();
                #endif
                /* Release the thread. No Python API allowed beyond this point. */
                PyGILState_Release(gstate);
                status = CAPS_BADVALUE;
                goto cleanup;
            }

            /* Release the thread. No Python API allowed beyond this point. */
            PyGILState_Release(gstate);

            // Close down python
            if (initPy == (int) false) {
                /*
                printf("\n");
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

        if (aimInputs[Overwrite_NML-1].vals.integer == (int) false) {

            printf("Since Python was not linked and/or being used, the \"Overwrite_NML\" input needs to be set to \"True\" to give");
            printf(" permission to create a new fun3d.nml. fun3d.nml will NOT be updated!!\n");

        } else {

            printf("Warning: The fun3d.nml file will be overwritten!\n");

            status = fun3d_writeNML(aimInfo, aimInputs, bcProps);
            AIM_STATUS(aimInfo, status);

        }
    }

    // If data transfer is ok ....
    if (fun3dInstance->dataTransferCheck == (int) true) {

        //See if we have data transfer information
        status = aim_getBounds(aimInfo, &numTransferName, &transferName);
        if (status == CAPS_SUCCESS) {

            if (aimInputs[Modal_Aeroelastic-1].nullVal ==  NotNull) {
                status = fun3d_dataTransfer(aimInfo,
                                            fun3dInstance->projectName,
                                            bcProps,
                                            *volumeMesh,
                                            &modalAeroelastic);
                if (status == CAPS_SUCCESS) {
                    status = fun3d_writeMovingBody(aimInfo, fun3dVersion, bcProps, &modalAeroelastic);
                }

            } else{
                status = fun3d_dataTransfer(aimInfo,
                                            fun3dInstance->projectName,
                                            bcProps,
                                            *volumeMesh,
                                            NULL);
            }
            if (status != CAPS_SUCCESS && status != CAPS_NOTFOUND) goto cleanup;
        }
    } // End if data transfer ok


    // Optimization - variable must be set at a minimum
    if (optimization == (int) true) {

        if (fun3dInstance->dataTransferCheck == (int) true ) {

            status = fun3d_makeDirectory(aimInfo);
            AIM_STATUS(aimInfo, status);

            status = fun3d_writeParameterization(fun3dInstance->design.numDesignVariable,
                                                 fun3dInstance->design.designVariable,
                                                 aimInfo,
                                                 volumeMesh,
                                                 nGeomIn, geomInVal);
            AIM_STATUS(aimInfo, status);

            status = fun3d_writeRubber(aimInfo,
                                       fun3dInstance->design,
                                       aimInputs[FUN3D_Version-1].vals.real,
                                       volumeMesh);
            AIM_STATUS(aimInfo, status);

        } else {
            AIM_ERROR(aimInfo, "The volume is not suitable for sensitivity input generation - possibly the volume mesher "
                               "added unaccounted for points\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }
    }

    status = CAPS_SUCCESS;

cleanup:

    if (transferName != NULL) EG_free(transferName);
    if (filename != NULL) EG_free(filename);

    (void) destroy_cfdBoundaryConditionStruct(&bcProps);
    (void) destroy_cfdModalAeroelasticStruct(&modalAeroelastic);

    (void) destroy_bndCondStruct(&bndConds);

    // Clean up the volume mesh that was created for 2D mode
    if (aimInputs[Two_Dimensional-1].vals.integer == (int) true) {

        // Destroy volume mesh since we created here instead of inheriting it
        if (volumeMesh != NULL) {
            (void) destroy_meshStruct(volumeMesh);
            EG_free(volumeMesh);
        }

    }

    return status;
}


/* no longer optional and needed for restart */
int aimPostAnalysis(/*@unused@*/ void *instStore, /*@unused@*/ void *aimStruc,
                    /*@unused@*/ int restart, /*@unused@*/ capsValue *inputs)
{
  return CAPS_SUCCESS;
}


int aimOutputs(/*@unused@*/ void *instStore, /*@unused@*/ void *aimStruc,
               int index, char **aoname, capsValue *form)
{

    /*! \page aimOutputsFUN3D AIM Outputs
     * The following list outlines the FUN3D outputs available through the AIM interface. All variables currently
     * correspond to values for all boundaries (total) found in the *.forces file
     */

    int numOutVars = 8; // Grouped

#ifdef DEBUG
    printf(" fun3dAIM/aimOutputs index = %d!\n", index);
#endif

    // Total Forces - Pressure + Viscous
    if      (index == 1) *aoname = EG_strdup("CLtot");
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
        printf(" fun3dAIM/aimOutputs index = %d NOT Found!\n", index);
        return CAPS_NOTFOUND;
    }

    if (index <= 3*numOutVars) {
        form->type    = Double;
        form->dim     = Vector;
        form->nrow    = 1;
        form->ncol    = 1;
        form->units   = NULL;
        form->nullVal = IsNull;

        form->vals.reals = NULL;
        form->vals.real = 0;
    }

    return CAPS_SUCCESS;
}


static int fun3d_readForcesJSON(FILE *fp, mapAttrToIndexStruct *attrMap,
                                capsValue *val)
{
    int status = CAPS_SUCCESS; // Function return status

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

    val->nrow = 0;
    val->vals.tuple = NULL;

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
            val->nrow   += 1;

            tempTuple = EG_reall(val->vals.tuple, val->nrow*sizeof(capsTuple));
            if (tempTuple == NULL) {
                status = EGADS_MALLOC;
                val->nrow -= 1;
                goto cleanup;
            }

            val->vals.tuple = tempTuple;
            val->nullVal = NotNull;
            val->vals.tuple[val->nrow -1 ].name = NULL;
            val->vals.tuple[val->nrow -1 ].value = NULL;

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

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,",
                    "CMX_p", num1, "CMY_p", num2, "CMZ_p", num3);
            strcat(json, lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,",
                    "CX_p", num1, "CY_p", num2, "CZ_p", num3);
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

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,",
                    "CMX_v", num1, "CMY_v", num2, "CMZ_v", num3);
            strcat(json,lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,",
                    "CX_v", num1, "CY_v", num2, "CZ_v", num3);
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

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s,",
                    "CMX", num1, "CMY", num2, "CMZ", num3);
            strcat(json,lineVal); // Add to JSON string

            status = getline(&line, &linecap, fp);
            if (status <=0) goto cleanup;

            strncpy(num1, &line[i], 14); num1[14] = '\0';
            strncpy(num2, &line[j], 14); num2[14] = '\0';
            strncpy(num3, &line[k], 14); num3[14] = '\0';

            sprintf(lineVal, "\"%s\":%s,\"%s\":%s,\"%s\":%s",
                    "CX", num1, "CY", num2, "CZ", num3);
            strcat(json,lineVal); // Add to JSON string

            strcat(json,"}");

//            printf("JSON = %s\n", json);

            if (strcmp(name, "Total") != 0) {
                status = string_toInteger(name, &nameIndex);
                if (status != CAPS_SUCCESS) goto cleanup;
            } else {
                nameIndex = -1;
            }

            status = get_mapAttrToIndexKeyword(attrMap, nameIndex, &keyWord);
            if (status == CAPS_SUCCESS) {
                val->vals.tuple[val->nrow-1].name  = EG_strdup(keyWord);
            } else {

                val->vals.tuple[val->nrow-1].name  = EG_strdup(name);
            }

            val->vals.tuple[val->nrow-1].value = EG_strdup(json);

            // Reset name index value in case we have totals next
            nameIndex = 0;
        }
    }

    status = CAPS_SUCCESS;

cleanup:
    if (status != CAPS_SUCCESS)
      printf("Premature exit in fun3dAIM fun3d_readForcesJSON status = %d\n",
             status);

    if (line != NULL) EG_free(line);

    return status;
}


static int fun3d_readForces(FILE *fp, int index, capsValue *val)
{

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

        if (strcmp(line, bndSectionKeyword) == 0 && bndSectionFound == (int) false) {

            //printf("FOUND section\n");
            bndSectionFound = (int) true;
            continue; // Skip to next line
        }

        if (bndSubSectionKeyword != NULL)
            if (strcmp(line, bndSubSectionKeyword) == 0 &&
                bndSectionFound == (int) true && bndSubSectionFound == (int) false) {

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
                val->nullVal = NotNull;

                break;
            }
        }
    }

    if (strValue == NULL) {
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    status = CAPS_SUCCESS;

cleanup:
    if (status != CAPS_SUCCESS)
        printf("Premature exit in fun3dAIM fun3d_readForces status = %d\n",
               status);

    if (line != NULL) EG_free(line);

    return status;
}


// Calculate FUN3D output
int aimCalcOutput(void *instStore, /*@unused@*/ void *aimInfo, int index,
                  capsValue *val)
{
    int status;

    char *filename = NULL; // File to open
    char fileExtension[] = ".forces";

    FILE *fp = NULL; // File pointer
  
    aimStorage *fun3dInstance;

    fun3dInstance = (aimStorage *) instStore;

    val->vals.real = 0.0; // Set default value

    // Open fun3d *.force file
    filename = (char *) EG_alloc((strlen(fun3dInstance->projectName) +
                                  strlen(fileExtension) +1)*sizeof(char));
    if (filename == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    sprintf(filename, "%s%s", fun3dInstance->projectName, fileExtension);

    fp = aim_fopen(aimInfo, filename, "r");

    if (fp == NULL) {

        AIM_ERROR(aimInfo, "Could not open file: %s\n", filename);
        status = CAPS_IOERR;
        goto cleanup;
    }

    if (index == 25) {
        status = fun3d_readForcesJSON(fp, &fun3dInstance->attrMap, val);
        AIM_STATUS(aimInfo, status);

    } else {
        status = fun3d_readForces(fp, index, val);
        AIM_STATUS(aimInfo, status);
    }

    status = CAPS_SUCCESS;

cleanup:
    if (status != CAPS_SUCCESS)
        printf("Premature exit in fun3dAIM calcOutput status = %d\n", status);

    if (fp != NULL) fclose(fp);
    if (filename != NULL) EG_free(filename); // Free filename allocation

    return status;
}


void aimCleanup(void *instStore)
{
    aimStorage *fun3dInstance;

#ifdef DEBUG
    printf(" fun3dAIM/aimCleanup!\n");

#endif
    fun3dInstance = (aimStorage *) instStore;
  
    // Clean up fun3dInstance data

    // Attribute to index map
    (void) destroy_mapAttrToIndexStruct(&fun3dInstance->attrMap);
    //status = destroy_mapAttrToIndexStruct(&fun3dInstance->attrMap);
    //if (status != CAPS_SUCCESS) return status;

    // FUN3D project name
    fun3dInstance->projectName = NULL;

    // Pointer to caps input value for scaling pressure during data transfer
    fun3dInstance->pressureScaleFactor = NULL;

    // Pointer to caps input value for offset pressure during data transfer
    fun3dInstance->pressureScaleOffset = NULL;

    // Design information
    (void) destroy_cfdDesignStruct(&fun3dInstance->design);

    EG_free(fun3dInstance);
}


/************************************************************************/
// CAPS transferring functions

void aimFreeDiscrPtr(void *ptrm)
{
#ifdef DEBUG
    printf(" fun3dAIM/aimFreeDiscr!\n");
#endif

    /* free up this capsDiscr user pointer */
    AIM_FREE(ptrm);
}


int aimDiscr(char *tname, capsDiscr *discr)
{

    int i; // Indexing

    int status; // Function return status

    int numBody;

    // EGADS objects
    ego *bodies = NULL, *tess = NULL;

    const char   *intents;
    capsValue *meshVal;

#ifdef OLD_DISCR_IMPLEMENTATION_TO_REMOVE
    int body, face; // Indexing

    // EGADS objects
    ego tess, *faces = NULL, tempBody;

    const char   *string, *capsGroup; // capsGroups strings

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
#endif

    // Volume Mesh obtained from meshing AIM
    meshStruct *volumeMesh;
    int numElementCheck = 0;

    aimStorage *fun3dInstance;

    fun3dInstance = (aimStorage *) discr->instStore;

#ifdef DEBUG
    printf(" fun3dAIM/aimDiscr: tname = %s!\n", tname);
#endif

    if (tname == NULL) return CAPS_NOTFOUND;

    if (fun3dInstance->dataTransferCheck == (int) false) {
        printf("The volume is not suitable for data transfer - possibly the volume mesher "
                "added unaccounted for points\n");
        return CAPS_BADVALUE;
    }

    // Currently this ONLY works if the capsTranfer lives on single body!
    status = aim_getBodies(discr->aInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
        printf(" fun3dAIM/aimDiscr: aim_getBodies = %d!\n", status);
        return status;
    }
    if (bodies == NULL) {
         AIM_ERROR(discr->aInfo, " fun3dAIM/aimDiscr: NULL Bodies!\n");
        return CAPS_NULLOBJ;
    }


    // Get the mesh Value
    status = aim_getValue(discr->aInfo, Mesh, ANALYSISIN, &meshVal);
    AIM_STATUS(discr->aInfo, status);

    if (meshVal->nullVal == IsNull) {
        AIM_ANALYSISIN_ERROR(discr->aInfo, Mesh, "'Mesh' input must be linked to an output 'Surface_Mesh' or 'Volume_Mesh'");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // Get mesh
    volumeMesh = (meshStruct *)meshVal->vals.AIMptr;
    AIM_NOTNULL(volumeMesh, discr->aInfo, status);

    if (volumeMesh->referenceMesh == NULL) {
        AIM_ERROR(discr->aInfo, "No reference meshes in volume mesh - data transfer isn't possible.\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    if (aim_newGeometry(discr->aInfo) == CAPS_SUCCESS) {
        // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
        status = create_CAPSGroupAttrToIndexMap(numBody,
                                                bodies,
                                                1, // Only search down to the face level of the EGADS body
                                                &fun3dInstance->attrMap);
        AIM_STATUS(discr->aInfo, status);
    }

    // Lets check the volume mesh

    // Do we have an individual surface mesh for each body
    if (volumeMesh->numReferenceMesh != numBody) {
        AIM_ERROR(  discr->aInfo, "Number of surface mesh in the linked volume mesh (%d) does not match the number");
        AIM_ADDLINE(discr->aInfo,"of bodies (%d) - data transfer is NOT possible.", volumeMesh->numReferenceMesh,numBody);
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
        AIM_STATUS(discr->aInfo, status);

        status = mesh_retrieveNumMeshElements(volumeMesh->numElement,
                                              volumeMesh->element,
                                              Quadrilateral,
                                              &volumeMesh->meshQuickRef.numQuadrilateral);
        AIM_STATUS(discr->aInfo, status);
    }

    if (numElementCheck != (volumeMesh->meshQuickRef.numTriangle +
                            volumeMesh->meshQuickRef.numQuadrilateral)) {
        AIM_ERROR(discr->aInfo, "Volume mesher added surface elements - data transfer will NOT be possible.\n");
        status = CAPS_MISMATCH;
        goto cleanup;
    }

    // To this point is doesn't appear that the volume mesh has done anything bad to our surface mesh(es)

    // Lets store away our tessellation now
    AIM_ALLOC(tess, volumeMesh->numReferenceMesh, ego, discr->aInfo, status);
    for (i = 0; i < volumeMesh->numReferenceMesh; i++) {
        tess[i] = volumeMesh->referenceMesh[i].bodyTessMap.egadsTess;
    }

    status = mesh_fillDiscr(tname, &fun3dInstance->attrMap, volumeMesh->numReferenceMesh, tess, discr);
    AIM_STATUS(discr->aInfo, status);

#ifdef OLD_DISCR_IMPLEMENTATION_TO_REMOVE
    numFaceFound = 0;
    numPoint = numTri = 0;
    // Find any faces with our boundary marker and get how many points and triangles there are
    for (body = 0; body < numBody; body++) {

        status = EG_getBodyTopos(bodies[body], NULL, FACE, &numFace, &faces);
        if (status != EGADS_SUCCESS) {
            printf(" fun3dAIM: getBodyTopos (Face) = %d for Body %d!\n", status, body);
            return status;
        }
        if (faces == NULL) {
            printf(" fun3dAIM: Faces = NULL Body %d!\n", body);
            return CAPS_NULLOBJ;
        }

        for (face = 0; face < numFace; face++) {

            // Retrieve the string following a capsBound tag
            status = retrieve_CAPSBoundAttr(faces[face], &string);
            if (status != CAPS_SUCCESS) continue;

            if (strcmp(string, tname) != 0) continue;

#ifdef DEBUG
            printf(" fun3dAIM/aimDiscr: Body %d/Face %d matches %s!\n",
                   body, face+1, tname);
#endif


            status = retrieve_CAPSGroupAttr(faces[face], &capsGroup);
            if (status != CAPS_SUCCESS) {
                printf("capsBound found on face %d, but no capGroup found!!!\n", face);
                continue;
            } else {

                status = get_mapAttrToIndexIndex(&fun3dInstance->attrMap,
                                                 capsGroup, &attrIndex);
                if (status != CAPS_SUCCESS) {
                    printf("capsGroup %s NOT found in attrMap\n", capsGroup);
                    continue;
                } else {

                    // If first index create arrays and store index
                    if (capsGroupList == NULL) {
                        numCAPSGroup  = 1;
                        capsGroupList = (int *) EG_alloc(numCAPSGroup*sizeof(int));
                        if (capsGroupList == NULL) {
                            status = EGADS_MALLOC;
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
                            capsGroupList = (int *) EG_reall(capsGroupList,
                                                             numCAPSGroup*sizeof(int));
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
            status = EG_getTessFace(bodies[body+numBody], face+1, &plen, &xyz,
                                    &uv, &ptype, &pindex, &tlen, &tris, &nei);
            if (status != EGADS_SUCCESS) {
                printf(" fun3dAIM: EG_getTessFace %d = %d for Body %d!\n",
                       face+1, status, body+1);
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

        if (dataTransferBodyIndex >= 0) break; // Force that only one body can be used
    }

    if ((numFaceFound == 0) || (bodyFaceMap == NULL)) {
        printf(" fun3dAIM/aimDiscr: No Faces match %s!\n", tname);
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    if (numPoint == 0 || numTri == 0) {
#ifdef DEBUG
        printf(" fun3dAIM/aimDiscr: ntris = %d, npts = %d!\n", numTri, numPoint);
#endif
        status = CAPS_SOURCEERR;
        goto cleanup;
    }

    if ((dataTransferBodyIndex - 1) > volumeMesh->numReferenceMesh) {
        printf("Data transfer body index doesn't match number of reference meshes in volume mesh - data transfer isn't possible.\n");
        status = CAPS_MISMATCH;
        goto cleanup;
    }

#ifdef DEBUG
    printf(" fun3dAIM/aimDiscr: Body Index for data transfer = %d\n",
           dataTransferBodyIndex);
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
    numTri   = 0;
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
        status = EG_getTessFace(tess, bodyFaceMap[2*face + 1], &plen, &xyz, &uv,
                                &ptype, &pindex, &tlen, &tris, &nei);
        if (status != EGADS_SUCCESS) {
            printf(" fun3dAIM: EG_getTessFace %d = %d for Body %d!\n",
                   bodyFaceMap[2*face + 1], status, bodyFaceMap[2*face + 0]);
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

            status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1],
                                      tris[3*i + 0], &gID);
            if (status != EGADS_SUCCESS) goto cleanup;

            discr->elems[numTri].gIndices[0] = localStitchedID[gID-1];
            discr->elems[numTri].gIndices[1] = tris[3*i + 0];

            status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1],
                                      tris[3*i + 1], &gID);
            if (status != EGADS_SUCCESS) goto cleanup;

            discr->elems[numTri].gIndices[2] = localStitchedID[gID-1];
            discr->elems[numTri].gIndices[3] = tris[3*i + 1];

            status = EG_localToGlobal(tess, bodyFaceMap[2*face + 1],
                                      tris[3*i + 2], &gID);
            if (status != EGADS_SUCCESS) goto cleanup;

            discr->elems[numTri].gIndices[4] = localStitchedID[gID-1];
            discr->elems[numTri].gIndices[5] = tris[3*i + 2];

            numTri += 1;
        }
    }

    discr->nPoints = numPoint;

#ifdef DEBUG
    printf(" fun3dAIM/aimDiscr: ntris = %d, npts = %d!\n",
           discr->nElems, discr->nPoints);
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
    printf(" fun3dAIM/aimDiscr: nodeOffSet = %d, dataTransferBodyIndex = %d\n",
           nodeOffSet, dataTransferBodyIndex);
#endif

    for (i = 0; i < numPoint; i++) {
        storage[i] = globalID[i] + nodeOffSet;
//volumeMesh->referenceMesh[dataTransferBodyIndex-1].node[globalID[i]-1].nodeID + nodeOffSet;
//#ifdef DEBUG
//      printf(" fun3dAIM/aimDiscr: Global Node ID %d\n", storage[i]);
//#endif
    }

    // Save way the attrMap capsGroup list
    if (capsGroupList != NULL) {
        storage[numPoint] = numCAPSGroup;
        for (i = 0; i < numCAPSGroup; i++) {
            storage[numPoint+1+i] = capsGroupList[i];
        }
    }
#endif
#ifdef DEBUG
    printf(" fun3dAIM/aimDiscr: Finished!!\n");
#endif

    status = CAPS_SUCCESS;

cleanup:
#ifdef OLD_DISCR_IMPLEMENTATION_TO_REMOVE
    if (faces != NULL) EG_free(faces);

    if (globalID  != NULL) EG_free(globalID);
    if (localStitchedID != NULL) EG_free(localStitchedID);

    if (capsGroupList != NULL) EG_free(capsGroupList);
    if (bodyFaceMap != NULL) EG_free(bodyFaceMap);

    if (status != CAPS_SUCCESS) {
        aimFreeDiscr(discr);
    }
#endif
    AIM_FREE(tess);
    return status;
}


int
aimLocateElement(capsDiscr *discr, double *params, double *param,
                 int *bIndex, int *eIndex, double *bary)
{
    return aim_locateElement(discr, params, param, bIndex, eIndex, bary);
}


int aimTransfer(capsDiscr *discr, const char *dataName, int numPoint,
                int dataRank, double *dataVal, /*@unused@*/ char **units)
{
    /*! \page dataTransferFUN3D AIM Data Transfer
     *
     * The FUN3D AIM has the ability to transfer surface data (e.g. pressure distributions) to and from the AIM
     * using the conservative and interpolative data transfer schemes in CAPS. Currently these transfers may only
     * take place on triangular meshes.
     *
     * \section dataFromFUN3D Data transfer from FUN3D (FieldOut)
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
     */ // Rest of this block comes from fun3dUtil.c

    int status, status2; // Function return status
    int i, j, dataPoint, capsGroupIndex, bIndex; // Indexing
    aimStorage *fun3dInstance;

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
    int *storage;
    int globalNodeID;
    int found = (int) false;

    // Filename stuff
    int *capsGroupList;
    char *filename = NULL; //"pyCAPS_FUN3D_Tetgen_ddfdrive_bndry1.dat";

#ifdef DEBUG
    printf(" fun3dAIM/aimTransfer name = %s  npts = %d/%d!\n",
           dataName, numPoint, dataRank);
#endif

    fun3dInstance = (aimStorage *) discr->instStore;

    if (strcasecmp(dataName, "Pressure") != 0 &&
        strcasecmp(dataName, "P")        != 0 &&
        strcasecmp(dataName, "Cp")       != 0 &&
        strcasecmp(dataName, "CoefficientOfPressure") != 0) {

        printf("Unrecognized data transfer variable - %s\n", dataName);
        return CAPS_NOTFOUND;
    }

    //Get the appropriate parts of the tessellation to data
    storage = (int *) discr->ptrm;
    capsGroupList = &storage[0]; // List of boundary ID (attrMap) in the transfer

    // Zero out data
    for (i = 0; i < numPoint; i++) {
        for (j = 0; j < dataRank; j++ ) {
            dataVal[dataRank*i+j] = 0;
        }
    }

    for (capsGroupIndex = 0; capsGroupIndex < capsGroupList[0]; capsGroupIndex++) {

        filename = (char *) EG_alloc((strlen(fun3dInstance->projectName) +
                                      strlen("_ddfdrive_bndry.dat")+7)*sizeof(char));
        if (filename == NULL) return EGADS_MALLOC;

        sprintf(filename,"%s%s%d%s",fun3dInstance->projectName,
                                    "_ddfdrive_bndry",
                                    capsGroupList[capsGroupIndex+1],
                                    ".dat");

        status = fun3d_readAeroLoad(discr->aInfo, filename,
                                    &numVariable,
                                    &variableName,
                                    &numDataPoint,
                                    &dataMatrix);
        // Try body file
        if (status == CAPS_IOERR) {

            filename = (char *) EG_reall(filename,
                                         (strlen(fun3dInstance->projectName) +
                                          strlen("_ddfdrive_body1.dat")+5)*sizeof(char));

            sprintf(filename,"%s%s%s",fun3dInstance->projectName,
                                      "_ddfdrive_body1",
                                      ".dat");

            printf("Instead trying file : %s\n", filename);

            status = fun3d_readAeroLoad(discr->aInfo, filename,
                                        &numVariable,
                                        &variableName,
                                        &numDataPoint,
                                        &dataMatrix);
        }

        if (filename != NULL) EG_free(filename);
        filename = NULL;

        if (status != CAPS_SUCCESS) return status;

        printf("Number of variables %d\n", numVariable);
        // Output some of the first row of the data
        //for (i = 0; i < numVariable; i++) printf("Variable %d - %.6f\n", i, dataMatrix[i][0]);

        // Loop through the variable list to find which one is the global node ID variable
        for (i = 0; i < numVariable; i++) {
            AIM_NOTNULL(variableName, discr->aInfo, status);
            if (strcasecmp("id", variableName[i]) == 0) {
                globalIDIndex = i;
                break;
            }
        }

        if (globalIDIndex == -99) {
            AIM_ERROR(discr->aInfo, "Global node number variable not found in data file\n");
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        // Loop through the variable list to see if we can find the transfer data name
        for (i = 0; i < numVariable; i++) {
            AIM_NOTNULL(variableName, discr->aInfo, status);

            if (strcasecmp(dataName, "Pressure") == 0 ||
                strcasecmp(dataName, "P")        == 0 ||
                strcasecmp(dataName, "Cp")       == 0 ||
                strcasecmp(dataName, "CoefficientOfPressure") == 0) {

                if (dataRank != 1) {
                    printf("Data transfer rank should be 1 not %d\n", dataRank);
                    status = CAPS_BADRANK;
                    goto cleanup;
                }

                dataScaleFactor = fun3dInstance->pressureScaleFactor->vals.real;
                dataScaleOffset = fun3dInstance->pressureScaleOffset->vals.real;

                //dataUnits = fun3dInstance->pressureScaleFactor->units;
                if (strcasecmp("cp", variableName[i]) == 0) {
                    variableIndex = i;
                    break;
                }
            }
        }

        if (variableIndex == -99) {
            printf("Variable %s not found in data file\n", dataName);
            status = CAPS_NOTFOUND;
            goto cleanup;
        }
        if (dataMatrix == NULL) {
            printf("Variable %s daata mtrix is NULL!\n", dataName);
            status = CAPS_NULLVALUE;
            goto cleanup;
        }

        for (i = 0; i < numPoint; i++) {

            bIndex       = discr->tessGlobal[2*i  ];
            globalNodeID = discr->tessGlobal[2*i+1] +
                           discr->bodys[bIndex-1].globalOffset;

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

                    dataVal[dataRank*i+j] = dataMatrix[variableIndex][dataPoint]*dataScaleFactor +
                                            dataScaleOffset;
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

cleanup:
    if (status != CAPS_SUCCESS)
        printf("Premature exit in fun3dAIM transfer status = %d\n", status);

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


int
aimInterpolation(capsDiscr *discr, /*@unused@*/ const char *name, int bIndex,
                 int eIndex, double *bary, int rank,
                 double *data, double *result)
{
#ifdef DEBUG
    printf(" fun3dAIM/aimInterpolation  %s!\n", name);
#endif
    return  aim_interpolation(discr, name, bIndex, eIndex,
                              bary, rank, data, result);
}


int
aimInterpolateBar(capsDiscr *discr, /*@unused@*/ const char *name, int bIndex,
                  int eIndex, double *bary, int rank,
                  double *r_bar, double *d_bar)
{
#ifdef DEBUG
    printf(" fun3dAIM/aimInterpolateBar  %s!\n", name);
#endif
    return  aim_interpolateBar(discr, name, bIndex, eIndex,
                               bary, rank, r_bar, d_bar);
}


int
aimIntegration(capsDiscr *discr, /*@unused@*/ const char *name, int bIndex,
               int eIndex, int rank, double *data, double *result)
{
#ifdef DEBUG
    printf(" fun3dAIM/aimIntegration  %s!\n", name);
#endif
    return aim_integration(discr, name, bIndex, eIndex, rank,
                           data, result);
}


int
aimIntegrateBar(capsDiscr *discr, /*@unused@*/ const char *name, int bIndex,
                int eIndex, int rank, double *r_bar, double *d_bar)
{
#ifdef DEBUG
    printf(" fun3dAIM/aimIntegrateBar  %s!\n", name);
#endif
    return aim_integrateBar(discr, name, bIndex, eIndex, rank,
                            r_bar, d_bar);
}
