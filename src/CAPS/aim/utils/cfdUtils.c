// CFD analysis related utility functions - Written by Dr. Ryan Durscher AFRL/RQVC

#include <string.h>
#include "capsTypes.h"  // Bring in CAPS types
#include "cfdTypes.h"   // Bring in cfd structures
#include "cfdUtils.h"
#include "miscUtils.h"  // Bring in miscellaneous utilities

#ifdef WIN32
#define strcasecmp  stricmp
#endif

// Fill bcProps in a cfdBCStruct format with boundary condition information from incoming BC Tuple
int cfd_getBoundaryCondition(int numTuple,
        capsTuple bcTuple[],
        mapAttrToIndexStruct *attrMap,
        cfdBCsStruct *bcProps) {

    /*! \page cfdBoundaryConditions CFD Boundary Conditions
     * Structure for the boundary condition tuple  = ("CAPS Group Name", "Value").
     * "CAPS Group Name" defines the capsGroup on which the boundary condition should be applied.
     *	The "Value" can either be a JSON String dictionary (see Section \ref jsonStringCFDBoundary) or a single string keyword string
     *	(see Section \ref keyStringCFDBoundary)
     */

    int status; //Function return

    int i, j; // Indexing

    int found = (int) false;

    int bcIndex;

    char *keyValue = NULL;
    char *keyWord = NULL;

    // Destroy our bcProps structures coming in if aren't 0 and NULL already
    status = destroy_cfdBCsStruct(bcProps);
    if (status != CAPS_SUCCESS) return status;

    printf("Getting CFD boundary conditions\n");

    //bcProps->numBCID = attrMap->numAttribute;
    bcProps->numBCID = numTuple;

    if (bcProps->numBCID > 0) {
        bcProps->surfaceProps = (cfdSurfaceStruct *) EG_alloc(bcProps->numBCID * sizeof(cfdSurfaceStruct));
        if (bcProps->surfaceProps == NULL) return EGADS_MALLOC;

    } else {
        printf("\tWarning: Number of Boundary Conditions is 0\n");
        return CAPS_NOTFOUND;
    }

    for (i = 0; i < bcProps->numBCID; i++) {
        status = intiate_cfdSurfaceStruct(&bcProps->surfaceProps[i]);
        if (status != CAPS_SUCCESS) return status;
    }

    //printf("Number of tuple pairs = %d\n", numTuple);

    for (i = 0; i < numTuple; i++) {

        printf("\tBoundary condition name - %s\n", bcTuple[i].name);

        status = get_mapAttrToIndexIndex(attrMap, (const char *) bcTuple[i].name, &bcIndex);
        if (status == CAPS_NOTFOUND) {
            printf("\tBC name \"%s\" not found in attrMap\n", bcTuple[i].name);
            return status;
        }

        // Replaced bcIndex with i
        // bcIndex is 1 bias
        bcProps->surfaceProps[i].bcID = bcIndex;

        // bcIndex is 1 bias coming from attribute mapper
        //bcIndex = bcIndex -1;

        // Copy boundary condition name
        if (bcProps->surfaceProps[i].name != NULL) EG_free(bcProps->surfaceProps[i].name);

        bcProps->surfaceProps[i].name = (char *) EG_alloc((strlen(bcTuple[i].name) + 1)*sizeof(char));
        if (bcProps->surfaceProps[i].name == NULL) return EGADS_MALLOC;

        memcpy(bcProps->surfaceProps[i].name, bcTuple[i].name, strlen(bcTuple[i].name)*sizeof(char));
        bcProps->surfaceProps[i].name[strlen(bcTuple[i].name)] = '\0';

        // Do we have a json string?
        if (strncmp(bcTuple[i].value, "{", 1) == 0) {

            //printf("\t\tJSON String - %s\n",bcTuple[i].value);

            /*! \page cfdBoundaryConditions
             * \section jsonStringCFDBoundary JSON String Dictionary
             *
             * If "Value" is a JSON string dictionary
             * \if (FUN3D || SU2)
             *  (eg. "Value" = {"bcType": "Viscous", "wallTemperature": 1.1})
             * \endif
             *  the following keywords ( = default values) may be used:
             *
             * <ul>
             * <li> <B>bcType = "Inviscid"</B> </li> <br>
             *      Boundary condition type. Options:
             *
             * \if (FUN3D)
             *  - Inviscid
             *  - Viscous
             *  - Farfield
             *  - Extrapolate
             *  - Freestream
             *  - BackPressure
             *  - Symmetry
             *  - SubsonicInflow
             *  - SubsonicOutflow
             *  - MassflowIn
             *  - MassflowOut
             *  - MachOutflow
             *
             * \elseif (SU2)
             *  - Inviscid
             *  - Viscous
             *  - Farfield
             *  - Freestream
             *  - BackPressure
             *  - Symmetry
             *  - SubsonicInflow
             *  - SubsonicOutflow

             * \else
             *  - Inviscid
             *  - Viscous
             *  - Farfield
             *  - Extrapolate
             *  - Freestream
             *  - BackPressure
             *  - Symmetry
             *  - SubsonicInflow
             *  - SubsonicOutflow
             *  - MassflowIn
             *  - MassflowOut
             *  - MachOutflow
             *  - FixedInflow
             *  - FixedOutflow
             *
             * \endif
             * </ul>
             */

            // Get property Type
            keyWord = "bcType";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                //{UnknownBoundary, Inviscid, Viscous, Farfield, Extrapolate, Freestream,
                // BackPressure, Symmetry, SubsonicInflow, SubsonicOutflow,
                // MassflowIn, MassflowOut, FixedInflow, FixedOutflow, MachOutflow}
                if      (strcasecmp(keyValue, "\"Inviscid\"")        == 0) bcProps->surfaceProps[i].surfaceType = Inviscid;
                else if (strcasecmp(keyValue, "\"Viscous\"")         == 0) bcProps->surfaceProps[i].surfaceType = Viscous;
                else if (strcasecmp(keyValue, "\"Farfield\"")        == 0) bcProps->surfaceProps[i].surfaceType = Farfield;
                else if (strcasecmp(keyValue, "\"Extrapolate\"")     == 0) bcProps->surfaceProps[i].surfaceType = Extrapolate;
                else if (strcasecmp(keyValue, "\"Freestream\"")      == 0) bcProps->surfaceProps[i].surfaceType = Freestream;
                else if (strcasecmp(keyValue, "\"BackPressure\"")    == 0) bcProps->surfaceProps[i].surfaceType = BackPressure;
                else if (strcasecmp(keyValue, "\"Symmetry\"")        == 0) bcProps->surfaceProps[i].surfaceType = Symmetry;
                else if (strcasecmp(keyValue, "\"SubsonicInflow\"")  == 0) bcProps->surfaceProps[i].surfaceType = SubsonicInflow;
                else if (strcasecmp(keyValue, "\"SubsonicOutflow\"") == 0) bcProps->surfaceProps[i].surfaceType = SubsonicOutflow;
                else if (strcasecmp(keyValue, "\"MassflowIn\"")      == 0) bcProps->surfaceProps[i].surfaceType = MassflowIn;
                else if (strcasecmp(keyValue, "\"MassflowOut\"")     == 0) bcProps->surfaceProps[i].surfaceType = MassflowOut;
                else if (strcasecmp(keyValue, "\"MachOutflow\"")     == 0) bcProps->surfaceProps[i].surfaceType = MachOutflow;
                else if (strcasecmp(keyValue, "\"FixedInflow\"")     == 0) bcProps->surfaceProps[i].surfaceType = FixedInflow;
                else if (strcasecmp(keyValue, "\"FixedOutflow\"")    == 0) bcProps->surfaceProps[i].surfaceType = FixedOutflow;
                else {

                    printf("\tUnrecognized \"%s\" specified (%s) for Boundary_Condition tuple %s, current options (not all options "
                            "are valid for this analysis tool - see AIM documentation) are "
                            "\" Inviscid, Viscous, Farfield, Extrapolate, Freestream, BackPressure, Symmetry, "
                            "SubsonicInflow, SubsonicOutflow, MassflowIn, MassflowOut, MachOutflow, "
                            "FixedInflow, FixedOutflow"
                            "\"\n",
                            keyWord, keyValue, bcTuple[i].name);

                    if (keyValue != NULL) EG_free(keyValue);

                    return CAPS_NOTFOUND;
                }

            } else {

                printf("\tNo \"%s\" specified for tuple %s in json string , defaulting to Inviscid\n", keyWord,
                        bcTuple[i].name);
                bcProps->surfaceProps[i].surfaceType = Inviscid;
            }

            if (keyValue != NULL) {
                EG_free(keyValue);
                keyValue = NULL;
            }

            // Wall specific properties

            /*! \page cfdBoundaryConditions
             *  \subsection cfdBoundaryWallProp Wall Properties
             *
             * \if FUN3D
             *  <ul>
             *	<li> <B>wallTemperature = 0.0</B> </li> <br>
             *  The ratio of wall temperature to reference temperature for inviscid and viscous surfaces. Adiabatic wall = -1
             *  </ul>
             * \elseif SU2
             *
             *  <ul>
             *  <li> <B>wallTemperature = 0.0</B> </li> <br>
             *  Dimensional wall temperature for inviscid and viscous surfaces
             *  </ul>
             *
             * \else
             * <ul>
             * <li>  <B>wallTemperature = 0.0</B> </li> <br>
             *  Temperature on inviscid and viscous surfaces.
             * </ul>
             * \endif
             */
            keyWord = "wallTemperature";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                bcProps->surfaceProps[i].wallTemperatureFlag = (int) true;

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].wallTemperature);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             * \if FUN3D
             * \elseif SU2
             *
             * <ul>
             * <li>  <B>wallHeatFlux = 0.0</B> </li> <br>
             *  Heat flux on viscous surfaces.
             * </ul>
             *
             * \else
             * <ul>
             * <li>  <B>wallHeatFlux = 0.0</B> </li> <br>
             *  Heat flux on inviscid and viscous surfaces.
             * </ul>
             * \endif
             */

            keyWord = "wallHeatFlux";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                bcProps->surfaceProps[i].wallTemperatureFlag = (int) true;
                bcProps->surfaceProps[i].wallTemperature = -10;

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].wallHeatFlux);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            // Stagnation quantities

            /*! \page cfdBoundaryConditions
             * \subsection cfdBoundaryStagProp Stagnation Properties
             *
             *
             * \if FUN3D
             * <ul>
             *  <li>  <B>totalPressure = 0.0</B> </li> <br>
             *  Ratio of total pressure to reference pressure on a boundary surface.
             *
             * </ul>
             *
             * \elseif SU2
             * <ul>
             * <li>  <B>totalPressure = 0.0</B> </li> <br>
             *  Dimensional total pressure on a boundary surface.
             * </ul>
             *
             * \else
             * <ul>
             * <li>  <B>totalPressure = 0.0</B> </li> <br>
             *  Total pressure on a boundary surface.
             * </ul>
             * \endif
             *
             */
            keyWord = "totalPressure";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].totalPressure);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             * \if FUN3D
             * <ul>
             * <li>  <B>totalTemperature = 0.0</B> </li> <br>
             *  Ratio of total temperature to reference temperature on a boundary surface.
             * </ul>
             *
             * \elseif SU2
             * <ul>
             * <li>  <B>totalTemperature = 0.0</B> </li> <br>
             *  Dimensional total temperature on a boundary surface.
             * </ul>
             *
             * \else
             * <ul>
             * <li>  <B>totalTemperature = 0.0</B> </li> <br>
             *  Total temperature on a boundary surface.
             * </ul>
             * \endif
             *
             */
            keyWord = "totalTemperature";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].totalTemperature);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             * \if FUN3D
             * \elseif SU2
             *
             * \else
             * <ul>
             *  <li> <B>totalDensity = 0.0</B> </li> <br>
             *  Total density of boundary.
             * </ul>
             * \endif
             *
             */
            keyWord = "totalDensity";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].totalDensity);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            // Static quantities

            /*! \page cfdBoundaryConditions
             * \subsection cfdBoundaryStaticProp Static Properties
             *
             * \if FUN3D
             *  <ul>
             *  <li> <B>staticPressure = 0.0</B> </li> <br>
             *  Ratio of static pressure to reference pressure on a boundary surface.
             *  </ul>
             * \elseif SU2
             *  <ul>
             *  <li> <B>staticPressure = 0.0</B> </li> <br>
             *  Dimensional static pressure on a boundary surface.
             *  </ul>
             * \else
             * <ul>
             * <li> <B>staticPressure = 0.0</B> </li> <br>
             *  Static pressure of boundary.
             * </ul>
             * \endif
             */
            keyWord = "staticPressure";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].staticPressure);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             *
             * \if FUN3D
             * \elseif SU2
             *
             * \else
             *  <ul>
             *  <li> <B>staticTemperature = 0.0</B> </li> <br>
             *  Static temperature of boundary.
             *  </ul>
             * \endif
             */
            keyWord = "staticTemperature";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].staticTemperature);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             *
             * \if FUN3D
             * \elseif SU2
             *
             * \else
             *  <ul>
             *  <li>  <B>staticDensity = 0.0</B> </li> <br>
             *  Static density of boundary.
             *  </ul>
             * \endif
             */
            keyWord = "staticDensity";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].staticDensity);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }


            // Velocity components
            /*! \page cfdBoundaryConditions
             * \subsection cfdBoundaryVelocity Velocity Components
             *
             * \if FUN3D
             * \elseif SU2
             *
             * \else
             * <ul>
             *  <li>  <B>uVelocity = 0.0</B> </li> <br>
             *  X-velocity component on boundary.
             * </ul>
             * \endif
             */
            keyWord = "uVelocity";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].uVelocity);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             * \if FUN3D
             * \elseif SU2
             *
             * \else
             *  <ul>
             *  <li>  <B>vVelocity = 0.0</B> </li> <br>
             *  Y-velocity component on boundary.
             *  </ul>
             * \endif
             *
             */
            keyWord = "vVelocity";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].vVelocity);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             * \if FUN3D
             * \elseif SU2
             *
             * \else
             *  <ul>
             *  <li>  <B>wVelocity = 0.0</B> </li> <br>
             *  Z-velocity component on boundary.
             * </ul>
             * \endif
             */
            keyWord = "wVelocity";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].wVelocity);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \page cfdBoundaryConditions
             *
             * \if FUN3D
             * <ul>
             *  <li> <B>machNumber = 0.0</B> </li> <br>
             *  Mach number on boundary.
             * </ul>
             * \elseif SU2
             *
             * \else
             *  <ul>
             *  <li> <B>machNumber = 0.0</B> </li> <br>
             *  Mach number on boundary.
             * </ul>
             * \endif
             */
            keyWord = "machNumber";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].machNumber);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            // Massflow
            /*! \page cfdBoundaryConditions
             * \subsection cfdBoundaryMassflow Massflow Properties
             *
             * \if FUN3D
             *  <ul>
             *	<li> <B>massflow = 0.0</B> </li> <br>
             *  Massflow through the boundary in units of grid units squared.
             *  </ul>
             * \elseif SU2
             *
             * \else
             *  <ul>
             *  <li> <B>massflow = 0.0</B> </li> <br>
             *  Massflow through the boundary.
             *  </ul>
             * \endif
             */
            keyWord = "massflow";
            status = search_jsonDictionary( bcTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &bcProps->surfaceProps[i].massflow);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

        } else {
            //printf("\t\tkeyString value - %s\n", bcTuple[i].value);

            /*! \page cfdBoundaryConditions
             * \section keyStringCFDBoundary Single Value String
             *
             * If "Value" is a single string the following options maybe used:
             *
             * \if (SU2)
             * - "Inviscid" (default)
             * - "Viscous"
             * - "Farfield"
             * - "Freestream"
             * - "SymmetryX"
             * - "SymmetryY"
             * - "SymmetryZ"
             * \else
             * - "Inviscid" (default)
             * - "Viscous"
             * - "Farfield"
             * - "Extrapolate"
             * - "Freestream"
             * - "SymmetryX"
             * - "SymmetryY"
             * - "SymmetryZ"
             * \endif
             * */
            //{UnknownBoundary, Inviscid, Viscous, Farfield, Extrapolate, Freestream,
            // BackPressure, Symmetry, SubsonicInflow, SubsonicOutflow,
            // MassflowIn, MassflowOut, FixedInflow, FixedOutflow, MachOutflow}
            if      (strcasecmp(bcTuple[i].value, "Inviscid" ) == 0) bcProps->surfaceProps[i].surfaceType = Inviscid;
            else if (strcasecmp(bcTuple[i].value, "Viscous"  ) == 0) bcProps->surfaceProps[i].surfaceType = Viscous;
            else if (strcasecmp(bcTuple[i].value, "Farfield" ) == 0) bcProps->surfaceProps[i].surfaceType = Farfield;
            else if (strcasecmp(bcTuple[i].value, "Extrapolate" ) == 0) bcProps->surfaceProps[i].surfaceType = Extrapolate;
            else if (strcasecmp(bcTuple[i].value, "Freestream" ) == 0)  bcProps->surfaceProps[i].surfaceType = Freestream;
            else if (strcasecmp(bcTuple[i].value, "SymmetryX") == 0) {
                bcProps->surfaceProps[i].surfaceType = Symmetry;
                bcProps->surfaceProps[i].symmetryPlane = 1;
            }
            else if (strcasecmp(bcTuple[i].value, "SymmetryY") == 0) {
                bcProps->surfaceProps[i].surfaceType = Symmetry;
                bcProps->surfaceProps[i].symmetryPlane = 2;
            }
            else if (strcasecmp(bcTuple[i].value, "SymmetryZ") == 0) {
                bcProps->surfaceProps[i].surfaceType = Symmetry;
                bcProps->surfaceProps[i].symmetryPlane = 3;
            }
            else {
                printf("\tUnrecognized bcType (%s) in tuple %s defaulting to an"
                        " inviscid boundary (index = %d)!\n", bcTuple[i].value,
                                                              bcProps->surfaceProps[i].name,
                                                              bcProps->surfaceProps[i].bcID);
                bcProps->surfaceProps[i].surfaceType = Inviscid;
            }
        }
    }

    for (i = 0; i < attrMap->numAttribute; i++) {
        for (j = 0; j < bcProps->numBCID; j++) {

            found = (int) false;
            if (attrMap->attributeIndex[i] == bcProps->surfaceProps[j].bcID) {
                found = (int) true;
                break;
            }
        }

        if (found == (int) false) {
            printf("\tWarning: No boundary condition specified for capsGroup %s!\n", attrMap->attributeName[i]);
        }
    }

    printf("\tDone getting CFD boundary conditions\n");

    return CAPS_SUCCESS;
}

// Initiate (0 out all values and NULL all pointers) of surfaceProps in the cfdSurfaceStruct structure format
int intiate_cfdSurfaceStruct(cfdSurfaceStruct *surfaceProps) {

    if (surfaceProps == NULL) return CAPS_NULLVALUE;

    surfaceProps->surfaceType = UnknownBoundary;

    surfaceProps->name = NULL;

    surfaceProps->bcID = 0; // ID of boundary

    // Wall specific properties
    surfaceProps->wallTemperatureFlag = (int) false; // Wall temperature flag
    surfaceProps->wallTemperature = 0.0;     // Wall temperature value -1 = adiabatic ; >0 = isothermal
    surfaceProps->wallHeatFlux = 0.0;	     // Wall heat flux. to use Temperature flag should be true and wallTemperature < 0

    // Symmetry plane
    surfaceProps->symmetryPlane = 0; // Symmetry flag / plane

    // Stagnation quantities
    surfaceProps->totalPressure = 0;    // Total pressure
    surfaceProps->totalTemperature = 0; // Total temperature
    surfaceProps->totalDensity = 0;     // Total density

    // Static quantities
    surfaceProps->staticPressure = 0;   // Static pressure
    surfaceProps->staticTemperature = 0;// Static temperature
    surfaceProps->staticDensity = 0;    // Static temperature

    // Velocity components
    surfaceProps->uVelocity = 0;  // x-component of velocity
    surfaceProps->vVelocity = 0;  // y-component of velocity
    surfaceProps->wVelocity = 0;  // z-compoentn of velocity
    surfaceProps->machNumber = 0; // Mach number

    // Massflow
    surfaceProps->massflow = 0; // Mass flow through a boundary

    return CAPS_SUCCESS;
}

// Destroy (0 out all values and NULL all pointers) of surfaceProps in the cfdSurfaceStruct structure format
int destroy_cfdSurfaceStruct(cfdSurfaceStruct *surfaceProps) {

    if (surfaceProps == NULL) return CAPS_NULLVALUE;

    surfaceProps->surfaceType = UnknownBoundary;

    if (surfaceProps->name != NULL) EG_free(surfaceProps->name);
    surfaceProps->name = NULL;

    surfaceProps->bcID = 0;             // ID of boundary

    // Wall specific properties
    surfaceProps->wallTemperatureFlag = 0; // Wall temperature flag
    surfaceProps->wallTemperature = 0;     // Wall temperature value -1 = adiabatic ; >0 = isothermal

    // Symmetry plane
    surfaceProps->symmetryPlane = 0; // Symmetry flag / plane

    // Stagnation quantities
    surfaceProps->totalPressure = 0;    // Total pressure
    surfaceProps->totalTemperature = 0; // Total temperature
    surfaceProps->totalDensity = 0;     // Total density

    // Static quantities
    surfaceProps->staticPressure = 0;   // Static pressure
    surfaceProps->staticTemperature = 0;// Static temperature
    surfaceProps->staticDensity = 0;    // Static temperature

    // Velocity components
    surfaceProps->uVelocity = 0;  // x-component of velocity
    surfaceProps->vVelocity = 0;  // y-component of velocity
    surfaceProps->wVelocity = 0;  // z-compoentn of velocity
    surfaceProps->machNumber = 0; // Mach number

    // Massflow
    surfaceProps->massflow = 0; // Mass flow through a boundary

    return CAPS_SUCCESS;
}

// Initiate (0 out all values and NULL all pointers) of bcProps in the cfdBCsStruct structure format
int initiate_cfdBCsStruct(cfdBCsStruct *bcProps) {

    if (bcProps == NULL) return CAPS_NULLVALUE;

    bcProps->name = NULL;

    bcProps->numBCID = 0;

    bcProps->surfaceProps = NULL;

    return CAPS_SUCCESS;
}

// Destroy (0 out all values and NULL all pointers) of bcProps in the cfdBCsStruct structure format
int destroy_cfdBCsStruct(cfdBCsStruct *bcProps) {

    int status; // Function return
    int i; // Indexing

    if (bcProps == NULL) return CAPS_NULLVALUE;

    if (bcProps->name != NULL) EG_free(bcProps->name);
    bcProps->name = NULL;

    for  (i = 0; i < bcProps->numBCID; i++) {
        status = destroy_cfdSurfaceStruct(&bcProps->surfaceProps[i]);
        if (status != CAPS_SUCCESS) printf("Error in destroy_cfdBCsStruct, status = %d\n", status);
    }

    bcProps->numBCID = 0;

    if (bcProps->surfaceProps != NULL) EG_free(bcProps->surfaceProps);
    bcProps->surfaceProps = NULL;

    return CAPS_SUCCESS;
}

// Initiate (0 out all values and NULL all pointers) of eigenValue in the eigenValueStruct structure format
int initiate_eigenValueStruct(eigenValueStruct *eigenValue) {

    if (eigenValue == NULL) return CAPS_NULLVALUE;

    eigenValue->name = NULL;

    eigenValue->modeNumber = 0;

    eigenValue->frequency = 0.0;
    eigenValue->damping = 0.0;

    eigenValue->generalMass = 0.0;
    eigenValue->generalDisplacement = 0.0;
    eigenValue->generalVelocity = 0.0;
    eigenValue->generalForce = 0.0;

    return CAPS_SUCCESS;
}

// Destroy (0 out all values and NULL all pointers) of eigenValue in the eigenValueStruct structure format
int destroy_eigenValueStruct(eigenValueStruct *eigenValue) {

    if (eigenValue == NULL) return CAPS_NULLVALUE;

    if (eigenValue->name != NULL) EG_free(eigenValue->name);
    eigenValue->name = NULL;

    eigenValue->modeNumber = 0;

    eigenValue->frequency = 0.0;
    eigenValue->damping = 0.0;

    eigenValue->generalMass = 0.0;
    eigenValue->generalDisplacement = 0.0;
    eigenValue->generalVelocity = 0.0;
    eigenValue->generalForce = 0.0;

    return CAPS_SUCCESS;
}

// Initiate (0 out all values and NULL all pointers) of modalAeroelastic in the modalAeroelasticStruct structure format
int initiate_modalAeroelasticStruct(modalAeroelasticStruct *modalAeroelastic) {

    if (modalAeroelastic == NULL) return CAPS_NULLVALUE;

    modalAeroelastic->surfaceID = 0;

    modalAeroelastic->numEigenValue = 0;
    modalAeroelastic->eigenValue = NULL;

    modalAeroelastic->freestreamVelocity = 0.0;
    modalAeroelastic->freestreamDynamicPressure = 0.0;
    modalAeroelastic->lengthScaling = 1.0;

    return CAPS_SUCCESS;
}


// Destroy (0 out all values and NULL all pointers) of modalAeroelastic in the modalAeroelasticStruct structure format
int destroy_modalAeroelasticStruct(modalAeroelasticStruct *modalAeroelastic) {

    int status; // Function return status;
    int i; // Indexing

    if (modalAeroelastic == NULL) return CAPS_NULLVALUE;

    modalAeroelastic->surfaceID = 0;

    if ( modalAeroelastic->eigenValue != NULL) {

        for (i = 0; i < modalAeroelastic->numEigenValue; i++) {

            status = destroy_eigenValueStruct(&modalAeroelastic->eigenValue[i]);

            if (status != CAPS_SUCCESS) printf("Error in destroy_modalAeroelasticStruct, status = %d\n", status);
        }

        if (modalAeroelastic->eigenValue != NULL) EG_free(modalAeroelastic->eigenValue);
    }

    modalAeroelastic->numEigenValue = 0;
    modalAeroelastic->eigenValue = NULL;

    modalAeroelastic->freestreamVelocity = 0.0;
    modalAeroelastic->freestreamDynamicPressure = 0.0;
    modalAeroelastic->lengthScaling = 1.0;

    return CAPS_SUCCESS;
}

// Fill modalAeroelastic in a modalAeroelasticStruct format with modal aeroelastic data from Modal Tuple
int cfd_getModalAeroelastic(int numTuple,
                            capsTuple modalTuple[],
                            modalAeroelasticStruct *modalAeroelastic) {

    /*! \if (FUN3D)
     * \page cfdModalAeroelastic Modal Aeroelastic Inputs
     * Structure for the modal aeroelastic tuple  = ("EigenVector_#", "Value").
     * The tuple name "EigenVector_#" defines the eigen-vector in which the supplied information corresponds to, where "#" should be
     * replaced by the corresponding mode number for the eigen-vector (eg. EigenVector_3 would correspond to the third mode, while
     * EigenVector_6 would be the sixth mode). This notation is the same as found in \ref dataTransferFUN3D. The "Value" must
     * be a JSON String dictionary (see Section \ref jsonStringModalAeroelastic).
     * \endif
     */

    int status; //Function return

    int i; // Indexing

    char *keyValue = NULL;
    char *keyWord = NULL;

    int stringLength = 0;

    // Destroy our modalAeroelasticStruct structures coming in if it isn't 0 and NULL already
    status = destroy_modalAeroelasticStruct(modalAeroelastic);
    if (status != CAPS_SUCCESS) return status;

    printf("Getting CFD modal aeroelastic information\n");

    modalAeroelastic->numEigenValue = numTuple;

    if (modalAeroelastic->numEigenValue > 0) {

        modalAeroelastic->eigenValue = (eigenValueStruct *) EG_alloc(modalAeroelastic->numEigenValue * sizeof(eigenValueStruct));
        if (modalAeroelastic->eigenValue == NULL) return EGADS_MALLOC;

    } else {

        printf("Warning: Number of modal aeroelastic tuples is 0\n");
        return CAPS_NOTFOUND;
    }

    for (i = 0; i < modalAeroelastic->numEigenValue; i++) {
        status = initiate_eigenValueStruct(&modalAeroelastic->eigenValue[i]);
        if (status != CAPS_SUCCESS) return status;
    }

    //printf("Number of tuple pairs = %d\n", numTuple);

    for (i = 0; i < modalAeroelastic->numEigenValue; i++) {

        printf("\tEigen-Vector name - %s\n", modalTuple[i].name);

        stringLength = strlen(modalTuple[i].name);

        modalAeroelastic->eigenValue[i].name = (char *) EG_alloc((stringLength + 1)*sizeof(char));
        if (modalAeroelastic->eigenValue[i].name == NULL) return EGADS_MALLOC;

        memcpy(modalAeroelastic->eigenValue[i].name,
               modalTuple[i].name,
               stringLength*sizeof(char));

        modalAeroelastic->eigenValue[i].name[stringLength] = '\0';

        status = sscanf(modalTuple[i].name, "EigenVector_%d", &modalAeroelastic->eigenValue[i].modeNumber);
        if (status != 1) {
            printf("Unable to determine which EigenVector mode number - Defaulting the mode 1!!!\n");
            modalAeroelastic->eigenValue[i].modeNumber = 1;
        }

        // Do we have a json string?
        if (strncmp(modalTuple[i].value, "{", 1) == 0) {
            //printf("JSON String - %s\n",bcTuple[i].value);

            /*! \if (FUN3D)
             *  \page cfdModalAeroelastic
             * 	\section jsonStringModalAeroelastic JSON String Dictionary
             *
             * If "Value" is a JSON string dictionary (eg. "Value" = {"generalMass": 1.0, "frequency": 10.7})
             *  the following keywords ( = default values) may be used:
             *  \endif
             */


            /*! \if (FUN3D)
             *  \page cfdModalAeroelastic
             *
             *  <ul>
             *	<li> <B>frequency = 0.0</B> </li> <br>
             *  This is the frequency of specified mode, in rad/sec.
             *  </ul>
             * \endif
             */
            keyWord = "frequency";
            status = search_jsonDictionary( modalTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &modalAeroelastic->eigenValue[i].frequency);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \if (FUN3D)
             *  \page cfdModalAeroelastic
             *
             *  <ul>
             *	<li> <B>damping = 0.0</B> </li> <br>
             *  The critical damping ratio of the mode.
             *  </ul>
             * \endif
             */
            keyWord = "damping";
            status = search_jsonDictionary( modalTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &modalAeroelastic->eigenValue[i].damping);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \if (FUN3D)
             *  \page cfdModalAeroelastic
             *
             *  <ul>
             *	<li> <B>generalMass = 0.0</B> </li> <br>
             *  The generalized mass of the mode.
             *  </ul>
             * \endif
             */
            keyWord = "generalMass";
            status = search_jsonDictionary( modalTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &modalAeroelastic->eigenValue[i].generalMass);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \if (FUN3D)
             *  \page cfdModalAeroelastic
             *
             *  <ul>
             *	<li> <B>generalDisplacement = 0.0</B> </li> <br>
             *  The generalized displacement used at the starting time step to perturb the mode and excite a dynamic response.
             *  </ul>
             * \endif
             */
            keyWord = "generalDisplacement";
            status = search_jsonDictionary( modalTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &modalAeroelastic->eigenValue[i].generalDisplacement);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \if (FUN3D)
             *  \page cfdModalAeroelastic
             *
             *  <ul>
             *	<li> <B>generalVelocity = 0.0</B> </li> <br>
             *  The generalized velocity used at the starting time step to perturb the mode and excite a dynamic response.
             *  </ul>
             * \endif
             */
            keyWord = "generalVelocity";
            status = search_jsonDictionary( modalTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &modalAeroelastic->eigenValue[i].generalVelocity);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

            /*! \if (FUN3D)
             *  \page cfdModalAeroelastic
             *
             *  <ul>
             *	<li> <B>generalForce = 0.0</B> </li> <br>
             *  The generalized force used at the starting time step to perturb the mode and excite a dynamic response.
             *  </ul>
             * \endif
             */
            keyWord = "generalForce";
            status = search_jsonDictionary( modalTuple[i].value, keyWord, &keyValue);
            if (status == CAPS_SUCCESS) {

                status = string_toDouble(keyValue, &modalAeroelastic->eigenValue[i].generalForce);
                if (keyValue != NULL) {
                    EG_free(keyValue);
                    keyValue = NULL;
                }
                if (status != CAPS_SUCCESS) return status;
            }

        } else {
            printf("\tA JSON string was NOT provided for tuple %s\n!!!", modalTuple[i].name);
            return CAPS_NOTFOUND;

        }

    }

    printf("Done getting CFD boundary conditions\n");

    return CAPS_SUCCESS;
}

