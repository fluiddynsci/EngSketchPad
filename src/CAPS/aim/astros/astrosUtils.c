#include <string.h>
#include <math.h>
#include <ctype.h>

#include "aimUtil.h"

#include "feaTypes.h"    // Bring in FEA structures
#include "capsTypes.h"   // Bring in CAPS types

#include "miscUtils.h"   //Bring in misc. utility functions
#include "vlmUtils.h"    //Bring in vlm. utility functions
#include "meshUtils.h"   //Bring in mesh utility functions
#include "astrosUtils.h" // Bring in astros utility header

#ifdef WIN32
#define strcasecmp  stricmp
#endif


// Write a Astros connections card from a feaConnection structure
int astros_writeConnectionCard(FILE *fp, feaConnectionStruct *feaConnect, feaFileFormatStruct *feaFileFormat) {

    int  fieldWidth;
    char *tempString=NULL;
    char *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaConnect == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->gridFileType == FreeField) {
        delimiter = ",";
        fieldWidth = 7;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    // Mass
    if (feaConnect->connectionType == Mass) {

        fprintf(fp,"%-8s", "CMASS2");

        tempString = convert_integerToString(feaConnect->elementID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaConnect->mass, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->componentNumberStart, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[1], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->componentNumberEnd, fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    // Spring
    if (feaConnect->connectionType == Spring) {

        fprintf(fp,"%-8s", "CELAS2");

        tempString = convert_integerToString(feaConnect->elementID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaConnect->stiffnessConst, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->componentNumberStart, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[1], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->componentNumberEnd, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaConnect->dampingConst, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaConnect->stressCoeff, fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

    }

    // Damper
    if (feaConnect->connectionType == Damper) {

        printf("Astros doesn't appear to support CDAMP2 cards!\n");
        return CAPS_NOTFOUND;

        fprintf(fp,"%-8s", "CDAMP2");

        tempString = convert_integerToString(feaConnect->elementID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaConnect->dampingConst, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->componentNumberStart, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[1], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->componentNumberEnd, fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    // Rigid Body
    if (feaConnect->connectionType == RigidBody) {

        fprintf(fp,"%-8s", "RBE2");

        tempString = convert_integerToString(feaConnect->connectionID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->elementID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->dofDependent, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaConnect->connectivity[1], fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

    }

    return CAPS_SUCCESS;
}

// Write out PLYLIST Card.
int astros_writePlyListCard(FILE *fp, feaFileFormatStruct *feaFileFormat, int id, int numVal, int values[]) {

    char *delimiter;
    char *tempString=NULL;
    int fieldWidth;
    int i, fieldsRemaining;

    if (fp == NULL) return CAPS_IOERR;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    fprintf(fp,"%-8s", "PLYLIST");

    fieldsRemaining = 8;
    tempString = convert_integerToString(id, fieldWidth, 1);
    fprintf(fp, "%s%s", delimiter, tempString);
    EG_free(tempString);

    fieldsRemaining -= 1;

    for (i = 0; i < numVal; i++) {

        tempString = convert_integerToString(values[i], fieldWidth, 1);

        fprintf(fp, "%s%s", delimiter, tempString);
        fieldsRemaining-= 1;

        EG_free(tempString);

        if (fieldsRemaining == 0 && i < numVal) {

            if (feaFileFormat->fileType == FreeField) fprintf(fp, ",");

            fprintf(fp, "%-8s","+PL");
            fprintf(fp, "\n");
            fprintf(fp, "%-8s","+PL");

            fieldsRemaining = 8;
        }

    }
    fprintf(fp, "\n");
    return CAPS_SUCCESS;
}

// Write a Nastran Load card from a feaLoad structure
int astros_writeLoadCard(FILE *fp, meshStruct *mesh, feaLoadStruct *feaLoad, feaFileFormatStruct *feaFileFormat)
{
    int i, j; // Indexing
    int elemIDindex; // used in PLOAD element ID check logic
    int fieldWidth;
    char *tempString=NULL;
    char *delimiter;
    double avgPressure;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    // Concentrated force at a grid point
    if (feaLoad->loadType ==  GridForce) {

        for (i = 0; i < feaLoad->numGridID; i++) {

            fprintf(fp,"%-8s", "FORCE");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->gridIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->coordSystemID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->forceScaleFactor, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->directionVector[0], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->directionVector[1], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->directionVector[2], fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
        }
    }

    // Concentrated moment at a grid point
    if (feaLoad->loadType ==  GridMoment) {

        for (i = 0; i < feaLoad->numGridID; i++) {
            fprintf(fp,"%-8s", "MOMENT");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->gridIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->coordSystemID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->momentScaleFactor, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->directionVector[0], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->directionVector[1], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->directionVector[2], fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
        }
    }

    // Gravitational load
    if (feaLoad->loadType ==  Gravity) {

        fprintf(fp,"%-8s", "GRAV");

        tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaLoad->coordSystemID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaLoad->gravityAcceleration, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaLoad->directionVector[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaLoad->directionVector[1], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaLoad->directionVector[2], fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    // Pressure load
    if (feaLoad->loadType ==  Pressure) {

        for (i = 0; i < feaLoad->numElementID; i++) {

            fprintf(fp,"%-8s", "PLOAD");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->pressureForce, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            elemIDindex = -1;
            // Check to make sure element ID desired matches the id in the mesh input
            if (mesh->element[i].elementID == feaLoad->elementIDSet[i]) {
                elemIDindex = i;
            } else {
                for (j = 0; j < mesh->numElement; j++) {

                    if (mesh->element[j].elementID != feaLoad->elementIDSet[i]) continue;

                    elemIDindex = j;
                    break;

                }
            }

            if (elemIDindex < 0 ) {
                printf("Error: Element index wasn't found!\n");
                return CAPS_BADVALUE;
            }

            if (mesh->element[elemIDindex].elementType == Quadrilateral ||
                mesh->element[elemIDindex].elementType == Triangle) {

                for (j = 0; j <  mesh_numMeshConnectivity(mesh->element[elemIDindex].elementType); j++) {

                    tempString = convert_integerToString(mesh->element[elemIDindex].connectivity[j], fieldWidth, 1);
                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);
                }

            } else {
                printf("Unsupported element type for PLOAD in astrosAIM!\n");
                return CAPS_BADVALUE;
            }

            fprintf(fp,"\n");

            /*
                fprintf(fp,"%-8s", "PLOAD2");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->pressureForce, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->elementIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
             */
        }
    }

    // Pressure load at element Nodes
    if (feaLoad->loadType ==  PressureDistribute) {

        for (i = 0; i < feaLoad->numElementID; i++) {
            fprintf(fp,"%-8s", "PLOAD4");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->elementIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            for (j = 0; j < 4; j++) {
                tempString = convert_doubleToString(feaLoad->pressureDistributeForce[j], fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);
            }

            fprintf(fp,"\n");
        }
    }

    // Pressure load at element Nodes - different distribution per element
    if (feaLoad->loadType ==  PressureExternal) {

#ifdef ASTROS_11_DOES_NOT_HAVE_PLOAD4
        for (i = 0; i < feaLoad->numElementID; i++) {
            fprintf(fp,"%-8s", "PLOAD4");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->elementIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            for (j = 0; j < 4; j++) {
                tempString = convert_doubleToString(feaLoad->pressureMultiDistributeForce[4*i + j], fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);
            }

            fprintf(fp,"\n");
        }
#endif


        for (i = 0; i < feaLoad->numElementID; i++) {

            fprintf(fp,"%-8s", "PLOAD");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            elemIDindex = -1;
            // Check to make sure element ID desired matches the id in the mesh input
            if (mesh->element[i].elementID == feaLoad->elementIDSet[i]) {
                elemIDindex = i;
            } else {
                for (j = 0; j < mesh->numElement; j++) {

                    if (mesh->element[j].elementID != feaLoad->elementIDSet[i]) continue;

                    elemIDindex = j;
                    break;

                }
            }

            if (elemIDindex < 0 ) {
                printf("Error: Element index wasn't found!\n");
                return CAPS_BADVALUE;
            }

            avgPressure = 0;
            for (j = 0; j < mesh_numMeshConnectivity(mesh->element[elemIDindex].elementType); j++) {
                avgPressure += feaLoad->pressureMultiDistributeForce[4*i + j];
            }
            avgPressure /= mesh_numMeshConnectivity(mesh->element[elemIDindex].elementType);

            tempString = convert_doubleToString(avgPressure, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);


            if (mesh->element[elemIDindex].elementType == Quadrilateral ||
                mesh->element[elemIDindex].elementType == Triangle) {

                for (j = 0; j <  mesh_numMeshConnectivity(mesh->element[elemIDindex].elementType); j++) {

                    tempString = convert_integerToString(mesh->element[elemIDindex].connectivity[j], fieldWidth, 1);
                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);
                }

            } else {
                printf("Unsupported element type for PLOAD in astrosAIM!\n");
                return CAPS_BADVALUE;
            }


            fprintf(fp,"\n");

            /*
                fprintf(fp,"%-8s", "PLOAD2");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->pressureForce, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->elementIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
             */
        }

    }

    // Rotational velocity
    if (feaLoad->loadType ==  Rotational) {

        for (i = 0; i < feaLoad->numGridID; i++) {
            fprintf(fp,"%-8s", "RFORCE");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->gridIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->coordSystemID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaLoad->angularVelScaleFactor, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaLoad->directionVector[0], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaLoad->directionVector[1], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaLoad->directionVector[2], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp,", ,+RF\n");
                fprintf(fp,"+RF     ");
            } else {
                fprintf(fp," %7s%-7s\n", "", "+RF");
                fprintf(fp,"%-8s", "+RF");
            }

            tempString = convert_doubleToString( feaLoad->angularAccScaleFactor, fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
        }
    }
    // Thermal load at a grid point
    if (feaLoad->loadType ==  Thermal) {

        fprintf(fp,"%-8s", "TEMPD");

        tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaLoad->temperatureDefault, fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

        for (i = 0; i < feaLoad->numGridID; i++) {
            fprintf(fp,"%-8s", "TEMP");

            tempString = convert_integerToString(feaLoad->loadID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaLoad->gridIDSet[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaLoad->temperature, fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
        }
    }

    return CAPS_SUCCESS;
}

/*
 * Flow of ASTROS aero cards:
 *
 * for each vlm panel { (one per WING, CANARD, FIN--currently only WING) {
 *
 *     write vlm chord cuts AEFACT 0.0-100.0 pct
 *     write vlm span cuts AEFACT in PHYSICAL Y-COORDS, MUST *EXACTLY* ALIGN WITH AIRFOIL CARDS
 *     write CAERO6 card (one per WING, CANARD, FIN--currently only WING)
 *
 *     for each airfoil section in panel {
 *         write airfoil chord station AEFACT 0.0-100.0 pct (may share or be separate per airfoil)
 *         write airfoil upper surf AEFACT in pct chord (1.0 = 1% I believe)
 *         write airfoil lower surf AEFACT in pct chord
 *         write AIRFOIL card (ref chord/upper/lower AEFACTS and vlm CAERO6)
 *     }
 * }
 */

// Write a Astros AEROS card from a feaAeroRef structure
int astros_writeAEROSCard(FILE *fp, feaAeroRefStruct *feaAeroRef, feaFileFormatStruct *feaFileFormat) {

    char *delimiter = NULL;
    char *tempString;
    int fieldWidth;

    if (fp == NULL) return CAPS_IOERR;
    if (feaAeroRef == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    fprintf(fp,"%-8s", "AEROS");

    tempString = convert_integerToString( feaAeroRef->coordSystemID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // ACSID
    EG_free(tempString);

    tempString = convert_integerToString( feaAeroRef->rigidMotionCoordSystemID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // RCSID
    EG_free(tempString);

    tempString = convert_doubleToString( feaAeroRef->refChord, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // REFC
    EG_free(tempString);

    tempString = convert_doubleToString( feaAeroRef->refSpan, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // REFB
    EG_free(tempString);

    tempString = convert_doubleToString( feaAeroRef->refArea, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // REFS
    EG_free(tempString);

    fprintf(fp,"\n");

    return CAPS_SUCCESS;
}

// Write a Astros AERO card from a feaAeroRef structure
int astros_writeAEROCard(FILE *fp, feaAeroRefStruct *feaAeroRef, feaFileFormatStruct *feaFileFormat) {

    char *delimiter = NULL;
    char *tempString;
    int fieldWidth;

    if (fp == NULL) return CAPS_IOERR;
    if (feaAeroRef == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    fprintf(fp,"%-8s", "AERO");

    tempString = convert_integerToString( feaAeroRef->coordSystemID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // ACSID
    EG_free(tempString);

    tempString = convert_doubleToString( feaAeroRef->refChord, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // REFC
    EG_free(tempString);

    tempString = convert_doubleToString( 1.0, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // RHOREF, set to 1.0
    EG_free(tempString);

    fprintf(fp,"\n");

    return CAPS_SUCCESS;
}

// Write out FLFact Card.
int astros_writeFLFactCard(FILE *fp, feaFileFormatStruct *feaFileFormat,
                           int id, int numVal, double values[]) {
    char *delimiter;
    char *tempString=NULL;
    int fieldWidth;
    int i, fieldsRemaining;

    if (fp == NULL) return CAPS_IOERR;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    fprintf(fp,"%-8s", "FLFACT");

    fieldsRemaining = 8;
    tempString = convert_integerToString(id, fieldWidth, 1);
    fprintf(fp, "%s%s", delimiter, tempString);
    EG_free(tempString);

    fieldsRemaining -= 1;

    for (i = 0; i < numVal; i++) {

        tempString = convert_doubleToString(values[i], fieldWidth, 1);

        fprintf(fp, "%s%s", delimiter, tempString);
        fieldsRemaining-= 1;

        EG_free(tempString);

        if (fieldsRemaining == 0 && i < numVal) {

            if (feaFileFormat->fileType == FreeField) fprintf(fp, ",");

            fprintf(fp, "%-8s","+C");
            fprintf(fp, "\n");
            fprintf(fp, "%-8s","+C");

            fieldsRemaining = 8;
        }

    }

    fprintf(fp, "\n");
    return CAPS_SUCCESS;
}

// Write out AEFact Card.
int astros_writeAEFactCard(FILE *fp, feaFileFormatStruct *feaFileFormat,
                           int id, int numVal, double values[]) {
    char *delimiter;
    char *tempString=NULL;
    int fieldWidth;
    int i, fieldsRemaining;

    if (fp == NULL) return CAPS_IOERR;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    fprintf(fp,"%-8s", "AEFACT");

    fieldsRemaining = 8;
    tempString = convert_integerToString(id, fieldWidth, 1);
    fprintf(fp, "%s%s", delimiter, tempString);
    EG_free(tempString);

    fieldsRemaining -= 1;

    for (i = 0; i < numVal; i++) {

        tempString = convert_doubleToString(values[i], fieldWidth, 1);

        fprintf(fp, "%s%s", delimiter, tempString);
        fieldsRemaining-= 1;

        EG_free(tempString);

        if (fieldsRemaining == 0 && i < numVal) {

            if (feaFileFormat->fileType == FreeField) fprintf(fp, ",");

            fprintf(fp, "%-8s","+C");
            fprintf(fp, "\n");
            fprintf(fp, "%-8s","+C");

            fieldsRemaining = 8;
        }

    }

    fprintf(fp, "\n");
    return CAPS_SUCCESS;
}

// Check to make for the bodies' topology are acceptable for airfoil shapes
// Return: CAPS_SUCCESS if everything is ok
//                 CAPS_SOURCEERR if not acceptable
//                 CAPS_* if something else went wrong
int astros_checkAirfoil(void *aimInfo, feaAeroStruct *feaAero) {

    int status; // Function return status

    int i;

    if (aimInfo == NULL) return CAPS_NULLVALUE;
    if (feaAero == NULL) return CAPS_NULLVALUE;

    // Loop through sections in surface
    for (i = 0; i < feaAero->vlmSurface.numSection; i++) {
        if ( feaAero->vlmSurface.vlmSection[i].teClass != NODE ) {
            //printf("\tError in astros_checkAirfoil, body has %d nodes and %d edges!\n", numNode, numEdge);
            status = CAPS_SOURCEERR;
            goto cleanup;
        }
    }

    status = CAPS_SUCCESS;

cleanup:
    if (status != CAPS_SUCCESS && status != CAPS_SOURCEERR) printf("\tError: Premature exit in astros_checkAirfoil, status = %d\n", status);

    return status;
}

// Write out all the Aero cards necessary to define the VLM surface
int astros_writeAeroData(void *aimInfo,
                         FILE *fp,
                         int useAirfoilShape,
                         feaAeroStruct *feaAero,
                         feaFileFormatStruct *feaFileFormat){

    int status; // Function return status

    int i, j; // Indexing

    int numPoint = 30;
    double *xCoord=NULL, *yUpper=NULL, *yLower=NULL;

    if (aimInfo == NULL) return CAPS_NULLVALUE;
    if (fp == NULL) return CAPS_IOERR;
    if (feaAero == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    // Loop through sections in surface
    for (i = 0; i < feaAero->vlmSurface.numSection; i++) {

        if (useAirfoilShape == (int) true) { // Using the airfoil upper and lower surfaces or panels?

          status = vlm_getSectionCoordX(&feaAero->vlmSurface.vlmSection[i],
                                        1.0, // Cosine distribution
                                        (int) true, numPoint,
                                        &xCoord, &yUpper, &yLower);
          if (status != CAPS_SUCCESS) return status;

          for (j = 0; j < numPoint; j++) {
              xCoord[j] *= 100.0;
              yUpper[j] *= 100.0;
              yLower[j] *= 100.0;
          }

           fprintf(fp, "$ Upper surface - Airfoil %d (of %d) \n",i+1, feaAero->vlmSurface.numSection);
           status = astros_writeAEFactCard(fp, feaFileFormat, feaAero->surfaceID + 100*(feaAero->vlmSurface.vlmSection[i].sectionIndex+1) + 1, numPoint, yUpper);
           if (status != CAPS_SUCCESS) goto cleanup;

           fprintf(fp, "$ Lower surface - Airfoil %d (of %d) \n",i+1, feaAero->vlmSurface.numSection);
           status = astros_writeAEFactCard(fp, feaFileFormat, feaAero->surfaceID + 100*(feaAero->vlmSurface.vlmSection[i].sectionIndex+1) + 2, numPoint, yLower);
           if (status != CAPS_SUCCESS) goto cleanup;

        } else {

            xCoord = (double*)EG_alloc(numPoint*sizeof(double));
            if (xCoord == NULL) { status = EGADS_MALLOC; goto cleanup; }

            for (j = 0; j < numPoint; j++) {
                xCoord[j] = ( (double) j / ( (double) numPoint - 1.0 ) ) * 100.0;
            }
        }

        // Write chord range
        xCoord[0] = 0.0;
        xCoord[numPoint-1] = 100.0;

        fprintf(fp, "$ Chord - Airfoil %d (of %d) \n",i+1, feaAero->vlmSurface.numSection);
        status = astros_writeAEFactCard(fp, feaFileFormat, feaAero->surfaceID + 100*(feaAero->vlmSurface.vlmSection[i].sectionIndex+1), numPoint, xCoord);
        if (status != CAPS_SUCCESS) goto cleanup;

        EG_free(xCoord); xCoord = NULL;
        EG_free(yUpper); yUpper = NULL;
        EG_free(yLower); yLower = NULL;
    }

    status = CAPS_SUCCESS;

cleanup:

    if (status != CAPS_SUCCESS) printf("Error: Premature exit in astros_writeAero, status = %d\n", status);

    EG_free(xCoord);
    EG_free(yUpper);
    EG_free(yLower);

    return status;
}

// Write Astros CAERO6 cards from a feaAeroStruct
int astros_writeCAeroCard(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat) {

    int status; // Function return status

    int i, j; // Indexing

    char *delimiter;
    char *tempString=NULL;
    int fieldWidth;

    double xmin, xmax, result[6];

    int lengthTemp, offSet = 0;
    double *temp = NULL, pi, x;

    if (fp == NULL) return CAPS_IOERR;
    if (feaAero == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    //printf("WARNING CAER06 card may not be correct - need to confirm\n");

    fprintf(fp,"%-8s", "CAERO6");

    tempString = convert_integerToString(feaAero->surfaceID,fieldWidth, 1);
    fprintf(fp, "%s%s", delimiter, tempString);
    EG_free(tempString);

    fprintf(fp, "%s%7s", delimiter, "WING");
    fprintf(fp, "%s%7s", delimiter, " "); //csys

    tempString = convert_integerToString(1, fieldWidth, 1); // ?
    fprintf(fp, "%s%s", delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(feaAero->surfaceID + 10*feaAero->surfaceID + 1, fieldWidth, 1); // Chord AEFact ID - Coordinate with astros_writeAirfoilCard
    fprintf(fp, "%s%s", delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(feaAero->surfaceID + 10*feaAero->surfaceID + 2, fieldWidth, 1); //  Span AEFact ID - Coordinate with astros_writeAirfoilCard
    fprintf(fp, "%s%s", delimiter, tempString);
    EG_free(tempString);

    fprintf(fp, "\n");

    // Write Chord AEFact
    lengthTemp = feaAero->vlmSurface.Nchord+1; // One more point than boxes for spline

    temp = (double *) EG_alloc(lengthTemp*sizeof(double));
    if (temp == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    pi = 3.0*atan(sqrt(3));

    // Set bounds
    temp[0] = 0.0;
    temp[lengthTemp-1] = 100.0;

    for (j = 1; j < lengthTemp-1; j++) {

        x = j*100.0/(lengthTemp-1);

        // Cosine
        if (fabs(feaAero->vlmSurface.Cspace) == 1.0) {

            x = (x - 50.0)/50.0;

            temp[j] = 0.5 * (1.0 +  x + (1.0/pi)*sin(x*pi));

            // Equal spacing
        } else {
            temp[j] = (double) 0.0 + x;
        }
    }

    fprintf(fp, "$ Chord\n");
    status = astros_writeAEFactCard(fp, feaFileFormat, feaAero->surfaceID + 10*feaAero->surfaceID + 1, lengthTemp, temp);
    if (status != CAPS_SUCCESS) goto cleanup;

    if (temp != NULL) EG_free(temp);

    lengthTemp = feaAero->vlmSurface.NspanTotal + 1; //*(feaAero->vlmSurface.numSection-1) + 1;

    temp = (double *) EG_alloc(lengthTemp*sizeof(double));
    if (temp == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    // Write Span AEFact
    for (i = 0; i < feaAero->vlmSurface.numSection-1; i++) {

        result[0] = result[3] = feaAero->vlmSurface.vlmSection[i].xyzLE[0];
        result[1] = result[4] = feaAero->vlmSurface.vlmSection[i].xyzLE[1];
        result[2] = result[5] = feaAero->vlmSurface.vlmSection[i].xyzLE[2];

        if (feaAero->vlmSurface.vlmSection[i+1].xyzLE[0] < result[0]) result[0] = feaAero->vlmSurface.vlmSection[i+1].xyzLE[0];
        if (feaAero->vlmSurface.vlmSection[i+1].xyzLE[0] > result[3]) result[3] = feaAero->vlmSurface.vlmSection[i+1].xyzLE[0];
        if (feaAero->vlmSurface.vlmSection[i+1].xyzLE[1] < result[1]) result[1] = feaAero->vlmSurface.vlmSection[i+1].xyzLE[1];
        if (feaAero->vlmSurface.vlmSection[i+1].xyzLE[1] > result[4]) result[4] = feaAero->vlmSurface.vlmSection[i+1].xyzLE[1];
        if (feaAero->vlmSurface.vlmSection[i+1].xyzLE[2] < result[2]) result[2] = feaAero->vlmSurface.vlmSection[i+1].xyzLE[2];
        if (feaAero->vlmSurface.vlmSection[i+1].xyzLE[2] > result[5]) result[5] = feaAero->vlmSurface.vlmSection[i+1].xyzLE[2];

        xmin = result[3] - result[0];
        if (result[4] - result[1] > xmin) xmin = result[4] - result[1];
        if (result[5] - result[2] > xmin) xmin = result[5] - result[2];

        if ((result[4]-result[1])/xmin > 1.e-5) { // Y-ordering

            xmin = feaAero->vlmSurface.vlmSection[i].xyzLE[1];
            xmax = feaAero->vlmSurface.vlmSection[i+1].xyzLE[1];

        } else { // Z-ordering

            xmin = feaAero->vlmSurface.vlmSection[i].xyzLE[2];
            xmax = feaAero->vlmSurface.vlmSection[i+1].xyzLE[2];
        }

        for (j = 0; j < feaAero->vlmSurface.vlmSection[i].Nspan+1; j++) { // One more point than boxes for spline

            temp[j +offSet] = xmin + j*(xmax -xmin)/(feaAero->vlmSurface.vlmSection[i].Nspan);

        }

        // offset so the first point of the section overwrites the last point of the previous section
        offSet += feaAero->vlmSurface.vlmSection[i].Nspan;
    }

    fprintf(fp, "$ Span\n");
    status = astros_writeAEFactCard(fp, feaFileFormat, feaAero->surfaceID + 10*feaAero->surfaceID + 2, lengthTemp, temp);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = CAPS_SUCCESS;

cleanup:

    EG_free(temp);

    return status;

}

// Write out all the Airfoil cards for each each of a surface
int astros_writeAirfoilCard(FILE *fp,
                            int useAirfoilShape, // = true use the airfoils shape, = false panel
                            feaAeroStruct *feaAero,
                            feaFileFormatStruct *feaFileFormat){

    int i; // Indexing

    char *delimiter;
    char *tempString=NULL;
    int fieldWidth;

    if (fp == NULL) return CAPS_IOERR;
    if (feaAero == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    // This assumes the sections are in order
    for (i = 0; i < feaAero->vlmSurface.numSection; i++) {

        fprintf(fp, "$ Airfoil %d (of %d) \n",i+1, feaAero->vlmSurface.numSection);

        fprintf(fp,"%-8s", "AIRFOIL");

        tempString = convert_integerToString(feaAero->surfaceID, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%7s", delimiter, "WING");
        fprintf(fp, "%s%7s", delimiter, " "); //csys


        tempString = convert_integerToString(feaAero->surfaceID + 100*(feaAero->vlmSurface.vlmSection[i].sectionIndex+1), fieldWidth, 1); // Chord AEFact ID
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        if (useAirfoilShape == (int) true) {

            tempString = convert_integerToString(feaAero->surfaceID + 100*(feaAero->vlmSurface.vlmSection[i].sectionIndex+1) + 1, fieldWidth, 1); // Upper surface AEFact ID
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaAero->surfaceID + 100*(feaAero->vlmSurface.vlmSection[i].sectionIndex+1) + 2, fieldWidth, 1); // Lower surface AEFact ID
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

        } else {

            fprintf(fp, "%s%7s%s%7s", delimiter, " ", delimiter, " "); //Upper surface, lower surface
        }

        fprintf(fp, "%s%7s%s%7s", delimiter, " ", delimiter, " "); //camber, radius

        if (feaFileFormat->fileType == FreeField) fprintf(fp, ",");

        fprintf(fp, "%-8s","+C");
        fprintf(fp, "\n");
        fprintf(fp, "%-8s","+C");

        tempString = convert_doubleToString(feaAero->vlmSurface.vlmSection[i].xyzLE[0], fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaAero->vlmSurface.vlmSection[i].xyzLE[1], fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaAero->vlmSurface.vlmSection[i].xyzLE[2], fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);


        tempString = convert_doubleToString(feaAero->vlmSurface.vlmSection[i].chord, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "\n");
    }

    return CAPS_SUCCESS;
}

// Write Astros Spline1 cards from a feaAeroStruct
int astros_writeAeroSplineCard(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat) {

    int  fieldWidth;
    char *tempString = NULL, *idString = NULL, *delimiter = NULL;

    int numSpanWise;

    if (fp == NULL) return CAPS_IOERR;
    if (feaAero == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    idString = convert_integerToString( feaAero->surfaceID, fieldWidth, 1);

    fprintf(fp,"%-8s", "SPLINE1");
    fprintf(fp, "%s%s",delimiter, idString); // EID

    // BLANK SPACE FOR CP (CAERO CARD DEFINES SPLINE PLANE)
    if (feaFileFormat->fileType == FreeField) {
        fprintf(fp, ", ");
    } else if (feaFileFormat->fileType == LargeField) {
        fprintf(fp, " %15s", "");
    } else {
        fprintf(fp, " %7s", "");
    }

    fprintf(fp, "%s%s",delimiter, idString); // CAER0

    fprintf(fp, "%s%s",delimiter, idString); // Box 1
    EG_free(tempString);

    if (feaAero->vlmSurface.NspanTotal > 0)
        numSpanWise = feaAero->vlmSurface.NspanTotal;
    else if (feaAero->vlmSurface.NspanSection > 0)
        numSpanWise = (feaAero->vlmSurface.numSection-1)*feaAero->vlmSurface.NspanSection;
    else {
        printf("Error: Only one of numSpanTotal and numSpanPerSection can be non-zero!\n");
        printf("       numSpanTotal      = %d\n", feaAero->vlmSurface.NspanTotal);
        printf("       numSpanPerSection = %d\n", feaAero->vlmSurface.NspanSection);
        return CAPS_BADVALUE;
    }

    tempString = convert_integerToString( feaAero->surfaceID + numSpanWise*feaAero->vlmSurface.Nchord - 1, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // Box 2
    EG_free(tempString);


    fprintf(fp, "%s%s",delimiter, idString); // SetG

    fprintf(fp, "\n");

    if (idString != NULL) EG_free(idString);

    return CAPS_SUCCESS;
}

// Write Astros constraint card from a feaConstraint structure
int astros_writeConstraintCard(FILE *fp, int feaConstraintSetID, feaConstraintStruct *feaConstraint, feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing;
    char *tempString, *delimiter;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    if (feaConstraint->constraintType == Displacement) {

        for (i = 0; i < feaConstraint->numGridID; i++) {
            fprintf(fp,"%-8s", "SPC");

            tempString = convert_integerToString(feaConstraintSetID, 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaConstraint->gridIDSet[i], 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaConstraint->dofConstraint, 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaConstraint->gridDisplacement, 7, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
        }
    }

    if (feaConstraint->constraintType == ZeroDisplacement) {

        for (i = 0; i < feaConstraint->numGridID; i++) {

            fprintf(fp,"%-8s", "SPC1");

            tempString = convert_integerToString(feaConstraintSetID , 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaConstraint->dofConstraint, 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaConstraint->gridIDSet[i], 7, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);
        }
    }

    return CAPS_SUCCESS;
}


// Write Astros support card from a feaSupport structure
int astros_writeSupportCard(FILE *fp, feaSupportStruct *feaSupport, feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing;
    char *tempString, *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaSupport == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    for (i = 0; i < feaSupport->numGridID; i++) {

        fprintf(fp,"%-8s", "SUPORT");

        tempString = convert_integerToString(feaSupport->supportID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaSupport->gridIDSet[i], 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaSupport->dofSupport, 7, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    return CAPS_SUCCESS;
}

// Write a Astros Property card from a feaProperty structure w/ design parameters
int astros_writePropertyCard(FILE *fp, feaPropertyStruct *feaProperty, feaFileFormatStruct *feaFileFormat,
                             int numDesignVariable, feaDesignVariableStruct feaDesignVariable[]) {

    int i,j, designIndex; // Indexing
    int plyCount;
    int  fieldWidth;
    char *tempString=NULL;
    char *delimiter;

    int found = (int) false;

    if (fp == NULL) return CAPS_IOERR;
    if (feaProperty == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    //          1D Elements        //

    // Rod
    if (feaProperty->propertyType ==  Rod) {

        fprintf(fp,"%-8s", "PROD");

        tempString = convert_integerToString(feaProperty->propertyID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaProperty->materialID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->crossSecArea, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->torsionalConst, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->torsionalStressReCoeff, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->massPerLength, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Check for design minimum area
        found = (int) false;
        for (designIndex = 0; designIndex < numDesignVariable; designIndex++) {
            for (j = 0; j < feaDesignVariable[designIndex].numPropertyID; j++) {

                if (feaDesignVariable[designIndex].propertySetID[j] == feaProperty->propertyID) {
                    found = (int) true;

                    if (feaDesignVariable[designIndex].lowerBound == 0.0) {
                        tempString = convert_doubleToString( 0.0001, fieldWidth, 1);
                    } else {
                        tempString = convert_doubleToString( feaDesignVariable[designIndex].lowerBound, fieldWidth, 1);
                    }

                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);

                    break;
                }
            }

            if (found == (int) true) break;
        }

        fprintf(fp,"\n");
    }

    // Bar
    if (feaProperty->propertyType ==  Bar) {

        fprintf(fp,"%-8s", "PBAR");

        tempString = convert_integerToString(feaProperty->propertyID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaProperty->materialID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->crossSecArea, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->zAxisInertia, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->yAxisInertia, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->torsionalConst, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaProperty->massPerLength, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Check for design minimum area
        found = (int) false;
        for (designIndex = 0; designIndex < numDesignVariable; designIndex++) {
            for (j = 0; j < feaDesignVariable[designIndex].numPropertyID; j++) {

                if (feaDesignVariable[designIndex].propertySetID[j] == feaProperty->propertyID) {
                    found = (int) true;

                    if (feaDesignVariable[designIndex].lowerBound == 0.0) {
                        tempString = convert_doubleToString( 0.0001, fieldWidth, 1);
                    } else {
                        tempString = convert_doubleToString( feaDesignVariable[designIndex].lowerBound, fieldWidth, 1);
                    }

                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);

                    break;
                }
            }

            if (found == (int) true) break;
        }

        fprintf(fp,"\n");
    }

    //          2D Elements        //

    // Shell
    if (feaProperty->propertyType == Shell) {

        // Check for design minimum thickness
        found = (int) false;
        for (designIndex = 0; designIndex < numDesignVariable; designIndex++) {
            for (j = 0; j < feaDesignVariable[designIndex].numPropertyID; j++) {

                if (feaDesignVariable[designIndex].propertySetID[j] == feaProperty->propertyID) {
                    //found = (int) true; eja - let design variable upper/lower bounds handle thisMA
                    break;
                }
            }

            if (found == (int) true) break;
        }

        if (feaFileFormat->fileType == LargeField) {
            fprintf(fp,"%-8s", "PSHELL*");
            fieldWidth = 15;
        }
        else{
            fprintf(fp,"%-8s", "PSHELL");
        }

        // Property ID
        tempString = convert_integerToString(feaProperty->propertyID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Material ID
        tempString = convert_integerToString(feaProperty->materialID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Membrane thickness
        tempString = convert_doubleToString(feaProperty->membraneThickness, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        if (feaProperty->materialBendingID != 0) {
            tempString = convert_integerToString(feaProperty->materialBendingID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            if (feaFileFormat->fileType == LargeField) {
                fprintf(fp,"%-8s\n%-8s", "*P", "*P");
            }

            tempString = convert_doubleToString(feaProperty->bendingInertiaRatio, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

        } else {// Print a blank

            if (found == (int) true ||
                    feaProperty->materialShearID != 0 ||
                    feaProperty->massPerArea != 0) {

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ", , ");
                } else if (feaFileFormat->fileType == LargeField) {
                    fprintf(fp, " %15s%-8s\n%-8s %15s", "", "*P", "*P", "");
                } else {
                    fprintf(fp, " %7s %7s", "", "");
                }
            }

        }

        if (feaProperty->materialShearID != 0) {
            tempString = convert_integerToString(feaProperty->materialShearID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaProperty->shearMembraneRatio, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else {// Print a blank

            if (found == (int) true ||
                    feaProperty->massPerArea != 0) {

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ", , ");
                } else if (feaFileFormat->fileType == LargeField) {
                    fprintf(fp, " %15s", "");
                } else {
                    fprintf(fp, " %7s %7s", "", "");
                }
            }
        }

        if (feaProperty->massPerArea != 0){
            tempString = convert_doubleToString(feaProperty->massPerArea, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Print a blank

            if (found == (int) true) {
                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ", , ");
                } else if (feaFileFormat->fileType == LargeField) {
                    fprintf(fp, " %15s", "");
                } else {
                    fprintf(fp, " %7s %7s", "", "");
                }
            }
        }

        if (found == (int) true) {

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ",+C \n");
            } else if (feaFileFormat->fileType == LargeField) {
                fprintf(fp, "%-8s\n", "*P");
            } else {
                fprintf(fp, "+C%6s\n", "");
            }

            // Print a blank
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", , , , , , ,");
            } else if (feaFileFormat->fileType == LargeField) {
                fprintf(fp, "*P%-6s%-8s%-8s%-8s%-8s*P\n*P%-6s%-8s%-8s", "", "", "", "", "", "", "", "" );
            } else {
                fprintf(fp, "+C%6s %7s %7s %7s %7s %7s %7s", "", "", "", "", "", "", "");
            }

            if (feaDesignVariable[designIndex].lowerBound == 0.0) {
                tempString = convert_doubleToString( 0.0001, fieldWidth, 1);
            } else {
                tempString = convert_doubleToString( feaDesignVariable[designIndex].lowerBound, fieldWidth, 1);
            }

            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        }

        fprintf(fp, "\n");

        // Return field width to 7
        if (feaFileFormat->fileType == LargeField) {
            fieldWidth = 7;
        }
    }

    // Composite
    if (feaProperty->propertyType == Composite) {
        fprintf(fp,"%-8s", "PCOMP");

        // PID
        tempString = convert_integerToString(feaProperty->propertyID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        if (tempString != NULL) EG_free(tempString);

        // BLANK FIELD Z0
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else {
            fprintf(fp, " %7s", "");
        }

        // NSM
        if (feaProperty->massPerArea != 0){
            tempString = convert_doubleToString(feaProperty->massPerArea, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else {// Print a blank
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        // SHEAR BOND ALLOWABLE SB
        tempString = convert_doubleToString(feaProperty->compositeShearBondAllowable, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        if (tempString != NULL) EG_free(tempString);

        // FAILURE THEORY
        // HILL, HOFF, TSAI, STRESS, or STRAIN. ASTROS
        // HILL, HOFF, TSAI, STRN. NASTRAN
        if (feaProperty->compositeFailureTheory != NULL) {
//            if (strcmp("HILL",feaProperty->compositeFailureTheory) == 0 ||
//                strcmp("HOFF",feaProperty->compositeFailureTheory) == 0 ||
//                strcmp("TSAI",feaProperty->compositeFailureTheory) == 0 ||
//                strcmp("STRESS",feaProperty->compositeFailureTheory) == 0) {
//
//                fprintf(fp, "%s%7s",delimiter, feaProperty->compositeFailureTheory);
//            } else if (strcmp("STRN",feaProperty->compositeFailureTheory) == 0) {
//                fprintf(fp, "%s%7s",delimiter, "STRAIN");
//            }
            if (strcmp("STRN",feaProperty->compositeFailureTheory) == 0) {
                fprintf(fp, "%s%7s",delimiter, "STRAIN");
            } else {
                fprintf(fp, "%s%7s",delimiter, feaProperty->compositeFailureTheory);
            }
        }

        // Check for design minimum area
        found = (int) false;
        for (designIndex = 0; designIndex < numDesignVariable; designIndex++) {
            for (j = 0; j < feaDesignVariable[designIndex].numPropertyID; j++) {

                if (feaDesignVariable[designIndex].propertySetID[j] == feaProperty->propertyID) {
                    found = (int) true;

                    if (feaDesignVariable[designIndex].lowerBound == 0.0) {
                        tempString = convert_doubleToString( 0.0001, fieldWidth, 1);
                    } else {
                        tempString = convert_doubleToString( feaDesignVariable[designIndex].lowerBound, fieldWidth, 1);
                    }

                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);

                    break;
                }
            }

            if (found == (int) true) break;
        }

        if (found == (int) false) {
            // BLANK FIELD if no design
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        // BLANK FIELD
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else {
            fprintf(fp, " %7s", "");
        }

        // BLANK FIELD - LOPT
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else {
            fprintf(fp, " %7s", "");
        }

        // CONTINUATION LINE
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ",+C ");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        fprintf(fp, "\n");

        // LOOP OVER PLYS
        plyCount = 0;
        for (i = 0; i < feaProperty->numPly; i++) {

            if ( plyCount % 2 == 0) {
                // CONINUATION LINE
                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, "+C ");
                } else {
                    fprintf(fp, "+C%6s", "");
                }
            }

            // MID
            tempString = convert_integerToString(feaProperty->compositeMaterialID[i], 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            if (tempString != NULL) EG_free(tempString);

            // THICKNESS
            if (feaProperty->compositeThickness != NULL) {
                tempString = convert_doubleToString(feaProperty->compositeThickness[i], fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                if (tempString != NULL) EG_free(tempString);

            } else {
                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ", ");
                } else {
                    fprintf(fp, " %7s", "");
                }
            }

            // THETA
            if (feaProperty->compositeOrientation != NULL) {
                tempString = convert_doubleToString(feaProperty->compositeOrientation[i], fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                if (tempString != NULL) EG_free(tempString);
            } else {

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ", ");
                } else {
                    fprintf(fp, " %7s", "");
                }
            }

            // BLANK STRESS / STRAIN OUTPUT
            if (i < feaProperty->numPly-1) {
                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ", ");
                } else {
                    fprintf(fp, " %7s", "");
                }
            }

            // CONINUATION LINE
            if ( plyCount % 2 != 0 && i < feaProperty->numPly-1) {
                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ",+C ");
                } else {
                    fprintf(fp, "+C%6s", "");
                }

                fprintf(fp, "\n");
            }

            plyCount += 1;
        }

        // If a symmetric laminate as been specified loop over the plies in reverse order
        if (feaProperty->compositeSymmetricLaminate == (int) true) {

            // BLANK STRESS / STRAIN OUTPUT
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }

            if ( plyCount % 2 == 0) {
                // CONINUATION LINE
                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ",+C ");
                } else {
                    fprintf(fp, "+C%6s", "");
                }

                fprintf(fp, "\n");
            }

            if ( feaProperty->numPly % 2 == 0 ) {// Even
                j = feaProperty->numPly -1;
            } else { // Odd - don't repeat the last ply
                j = feaProperty->numPly -2;
            }

            // LOOP OVER PLYS - in reverse order
            for (i = j; i >= 0; i--) {

                if ( plyCount % 2 == 0) {
                    // CONINUATION LINE
                    if (feaFileFormat->fileType == FreeField) {
                        fprintf(fp, "+C ");
                    } else {
                        fprintf(fp, "+C%6s", "");
                    }
                }

                // MID
                tempString = convert_integerToString(feaProperty->compositeMaterialID[i], 7, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                if (tempString != NULL) EG_free(tempString);

                // THICKNESS
                if (feaProperty->compositeThickness != NULL) {
                    tempString = convert_doubleToString(feaProperty->compositeThickness[i], fieldWidth, 1);
                    fprintf(fp, "%s%s",delimiter, tempString);
                    if (tempString != NULL) EG_free(tempString);

                } else {
                    if (feaFileFormat->fileType == FreeField) {
                        fprintf(fp, ", ");
                    } else {
                        fprintf(fp, " %7s", "");
                    }
                }

                // THETA
                if (feaProperty->compositeOrientation != NULL) {
                    tempString = convert_doubleToString(feaProperty->compositeOrientation[i], fieldWidth, 1);
                    fprintf(fp, "%s%s",delimiter, tempString);
                    if (tempString != NULL) EG_free(tempString);
                } else {

                    if (feaFileFormat->fileType == FreeField) {
                        fprintf(fp, ", ");
                    } else {
                        fprintf(fp, " %7s", "");
                    }
                }

                // BLANK STRESS / STRAIN OUTPUT
                if (i != 0) {
                    if (feaFileFormat->fileType == FreeField) {
                        fprintf(fp, ", ");
                    } else {
                        fprintf(fp, " %7s", "");
                    }
                }

                // CONINUATION LINE
                if ( plyCount % 2 != 0  && i != 0) {

                    if (feaFileFormat->fileType == FreeField) {
                        fprintf(fp, ",+C ");
                    } else {
                        fprintf(fp, "+C%6s", "");
                    }

                    fprintf(fp, "\n");
                }

                plyCount += 1;
            }
        }

        fprintf(fp, "\n");
    }

    //          3D Elements        //

    // Solid
    if (feaProperty->propertyType == Solid) {

        fprintf(fp,"%-8s", "PIHEX");

        tempString = convert_integerToString(feaProperty->propertyID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaProperty->materialID, 7, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    return CAPS_SUCCESS;
}

// Write ASTROS element cards not supported by mesh_writeNastran in meshUtils.c
int astros_writeSubElementCard(FILE *fp, meshStruct *feaMesh, int numProperty, feaPropertyStruct *feaProperty, feaFileFormatStruct *feaFileFormat) {

    int i, j; // Indexing
    int  fieldWidth;
    char *tempString=NULL;
    char *delimiter;

    int found;
    feaMeshDataStruct *feaData;

    if (numProperty > 0 && feaProperty == NULL) return CAPS_NULLVALUE;
    if (feaMesh == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->gridFileType == FreeField) {
        delimiter = ",";
        fieldWidth = 7;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    if (feaMesh->meshType == VolumeMesh) return CAPS_SUCCESS;

    for (i = 0; i < feaMesh->numElement; i++) {

        if (feaMesh->element[i].analysisType != MeshStructure) continue;

        feaData = (feaMeshDataStruct *) feaMesh->element[i].analysisData;

        if (feaMesh->element[i].elementType == Node &&
            feaData->elementSubType == ConcentratedMassElement) {

            found = (int) false;
            for (j = 0; j < numProperty; j++) {
                if (feaData->propertyID == feaProperty[j].propertyID) {
                    found = (int) true;
                    break;
                }
            }

            if (found == (int) false) {
                printf("No property information found for element %d of type \"ConcentratedMass\"!", feaMesh->element[i].elementID);
                continue;
            }

            fprintf(fp, "%-8s%s%7d%s%7d%s%7d", "CONM2",
                                                delimiter, feaMesh->element[i].elementID,
                                                delimiter, feaMesh->element[i].connectivity[0],
                                                delimiter, feaData->coordID);

            tempString = convert_doubleToString( feaProperty[j].mass, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaProperty[j].massOffset[0], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaProperty[j].massOffset[1], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaProperty[j].massOffset[2], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // Blank space and continuation line
            if (feaFileFormat->gridFileType == FreeField) {
                fprintf(fp, ", %6s,+C\n+C%6s", "", "");
            } else {
                fprintf(fp, " %7s+C\n+C%6s", "", "");
            }

            // I11
            tempString = convert_doubleToString( feaProperty[j].massInertia[I11], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I21
            tempString = convert_doubleToString( feaProperty[j].massInertia[I21], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I22
            tempString = convert_doubleToString( feaProperty[j].massInertia[I22], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I31
            tempString = convert_doubleToString( feaProperty[j].massInertia[I31], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I32
            tempString = convert_doubleToString( feaProperty[j].massInertia[I32], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I33
            tempString = convert_doubleToString( feaProperty[j].massInertia[I33], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            fprintf(fp, "\n");
        }

        if (feaData->elementSubType == BarElement) {
            printf("Bar elements not supported yet - Sorry !\n");
        }

        if (feaData->elementSubType == BeamElement) {
            printf("Beam elements not supported yet - Sorry !\n");
        }
    }

    return CAPS_SUCCESS;
}

// Write a Astros Analysis card from a feaAnalysis structure
int astros_writeAnalysisCard(FILE *fp, feaAnalysisStruct *feaAnalysis, feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing
    double velocity, dv, vmin, vmax, velocityArray[21];

    int sidIndex= 0, lineCount = 0; // Counters

    int fieldWidth, symmetryFlag, trimType, symxy, symxz, status;

    char *tempString, *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaAnalysis == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth  = 7;
    }

    // Eigenvalue
    if (feaAnalysis->analysisType == Modal ||
        feaAnalysis->analysisType == AeroelasticFlutter) {

        fprintf(fp,"%-8s", "EIGR");

        tempString = convert_integerToString(feaAnalysis->analysisID, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        if (feaAnalysis->extractionMethod != NULL) {
            fprintf(fp, "%s%7s", delimiter, feaAnalysis->extractionMethod);
        } else {
            fprintf(fp, "%s%7s", delimiter, ""); // Print blank space
        }

        tempString = convert_doubleToString(feaAnalysis->frequencyRange[0], fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaAnalysis->frequencyRange[1], fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaAnalysis->numEstEigenvalue, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaAnalysis->numDesiredEigenvalue, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, "%s%7s%s%7s%s%-7s\n", delimiter, "", delimiter, "", delimiter, "+E1");
        } else {
            fprintf(fp, "%s%7s%s%7s%-7s\n", delimiter, "", delimiter, "", "+E1");
        }

        fprintf(fp, "%-8s", "+E1");

        if (feaAnalysis->eigenNormaliztion != NULL) {
            fprintf(fp, "%s%7s", delimiter, feaAnalysis->eigenNormaliztion);
        } else {
            fprintf(fp,"%s%7s", delimiter,  ""); // Print blank space
        }

        tempString = convert_integerToString(feaAnalysis->gridNormaliztion, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaAnalysis->componentNormaliztion, fieldWidth, 1);
        fprintf(fp, "%s%s\n", delimiter, tempString);
        EG_free(tempString);
    }

    if (feaAnalysis->analysisType == AeroelasticTrim) {

        // SYMMETRY
        // SYM (0)
        // ANTISYM (-1)
        // ASYM (1)
        if (feaAnalysis->aeroSymmetryXY == NULL ) {
            printf("\t*** Warning *** aeroSymmetryXY Input to AeroelasticTrim Analysis in astrosAIM not defined! Using ASYMMETRIC\n");
            trimType = 1;
        } else {
            if(strcmp("SYM",feaAnalysis->aeroSymmetryXY) == 0) {
                trimType = 0;
            } else if(strcmp("ANTISYM",feaAnalysis->aeroSymmetryXY) == 0) {
                trimType = -1;
            } else if(strcmp("ASYM",feaAnalysis->aeroSymmetryXY) == 0) {
                trimType = 1;
            } else if(strcmp("SYMMETRIC",feaAnalysis->aeroSymmetryXY) == 0) {
                trimType = 0;
            } else if(strcmp("ANTISYMMETRIC",feaAnalysis->aeroSymmetryXY) == 0) {
                trimType = -1;
            } else if(strcmp("ASYMMETRIC",feaAnalysis->aeroSymmetryXY) == 0) {
                trimType = 1;
            } else {
                printf("\t*** Warning *** aeroSymmetryXY Input %s to astrosAIM not understood! Using ASYMMETRIC\n", feaAnalysis->aeroSymmetryXY );
                trimType = 1;
            }
        }

        // ASTROS VARIABLES
        //SYM: 'NX','NZ','QACCEL','ALPHA','QRATE','THKCAM'
        //ANTISYM: 'NY','PACCEL','RACCEL','BETA','PRATE','RRATE'

        // NASTRAN VARIABLES
        //SYM: URDD1, URDD3, URDD5, ANGLEA, PITCH
        //ANTISYM: URDD2, URDD4, URDD6, SIDES, ROLL, YAW
        fprintf(fp,"%s\n","$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|");
        fprintf(fp,"%s\n","$TRIM   TRIMID  MACH    QDP     TRMTYP  EFFID   VO                      CONT");
        fprintf(fp,"%-8s", "TRIM");

        //TRIMID
        tempString = convert_integerToString(feaAnalysis->analysisID, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        //MACH
        if (feaAnalysis->machNumber != NULL && feaAnalysis->numMachNumber > 0) {
            tempString = convert_doubleToString(feaAnalysis->machNumber[0], fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);
        } else {
            fprintf(fp, "%s%7s", delimiter, " ");
        }

        //QDP
        tempString = convert_doubleToString(feaAnalysis->dynamicPressure, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        // BLANK SPACE TRMTYP
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else if (feaFileFormat->fileType == LargeField) {
            fprintf(fp, " %15s", "");
        } else {
            fprintf(fp, " %7s", "");
        }

        // BLANK SPACE EFFID
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else if (feaFileFormat->fileType == LargeField) {
            fprintf(fp, " %15s", "");
        } else {
            fprintf(fp, " %7s", "");
        }

        //V0
        if(feaAnalysis->density > 0.0) {
            velocity = sqrt(2*feaAnalysis->dynamicPressure/feaAnalysis->density);
            //V0
            tempString = convert_doubleToString(velocity, fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);
        } else {
            // BLANK SPACE V0
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else if (feaFileFormat->fileType == LargeField) {
                fprintf(fp, " %15s", "");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        // BLANK SPACE
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else if (feaFileFormat->fileType == LargeField) {
            fprintf(fp, " %15s", "");
        } else {
            fprintf(fp, " %7s", "");
        }

        // BLANK SPACE
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else if (feaFileFormat->fileType == LargeField) {
            fprintf(fp, " %15s", "");
        } else {
            fprintf(fp, " %7s", "");
        }

        //fprintf(fp, "%-8s","+T");
        //fprintf(fp, "\n");
        //fprintf(fp, "%-8s","+T");

        lineCount = 1;
        sidIndex = 0;
        for (i = 0; i < feaAnalysis->numRigidConstraint+1; i++) {

            //if (sidIndex % (8*lineCount) == 0) {
            if (sidIndex % 8 == 0) {

                //if (lineCount == 1) fprintf(fp,"%8s", " ");

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ",+T%-5d\n", lineCount-1); // Start of continuation
                    fprintf(fp, "+T%-5d,"  , lineCount-1);  // End of continuation
                } else {
                    fprintf(fp, "+T%-6d\n", lineCount-1);  // Start of continuation
                    fprintf(fp, "+T%-6d"  , lineCount-1);  // End of continuation
                }

                lineCount += 1;
                sidIndex = 0;
            }

            if (i == feaAnalysis->numRigidConstraint) {
                // ADD THKCAM for ASYM and SYM Cases
                if (trimType != -1) {
                    fprintf(fp,"%s%7s", delimiter,"THKCAM");
                    //EG_free(tempString);

                    tempString = convert_doubleToString(1.0, fieldWidth, 1);
                    fprintf(fp, "%s%s", delimiter, tempString);
                    EG_free(tempString);

                    sidIndex += 2;
                }

            } else {

                // ASTROS VARIABLES
                //SYM: 'NX','NZ','QACCEL','ALPHA','QRATE','THKCAM'
                //ANTISYM: 'NY','PACCEL','RACCEL','BETA','PRATE','RRATE'

                // NASTRAN VARIABLES
                //SYM: URDD1, URDD3, URDD5, ANGLEA, PITCH
                //ANTISYM: URDD2, URDD4, URDD6, SIDES, ROLL, YAW

                // SYMMETRY
                // SYM (0)
                // ANTISYM (-1)

                if(strcmp("URDD1",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "NX";
                    symmetryFlag = 0;
                } else if(strcmp("URDD2",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "NY";
                    symmetryFlag = -1;
                } else if(strcmp("URDD3",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "NZ";
                    symmetryFlag = 0;
                } else if(strcmp("URDD4",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "PACCEL";
                    symmetryFlag = -1;
                } else if(strcmp("URDD5",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "QACCEL";
                    symmetryFlag = 0;
                } else if(strcmp("URDD6",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "RACCEL";
                    symmetryFlag = -1;
                } else if(strcmp("ANGLEA",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "ALPHA";
                    symmetryFlag = 0;
                } else if(strcmp("PITCH",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "QRATE";
                    symmetryFlag = 0;
                } else if(strcmp("SIDES",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "BETA";
                    symmetryFlag = -1;
                } else if(strcmp("ROLL",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "PRATE";
                    symmetryFlag = -1;
                } else if(strcmp("YAW",feaAnalysis->rigidConstraint[i]) == 0) {
                    tempString = "RRATE";
                    symmetryFlag = -1;
                } else {
                    tempString = feaAnalysis->rigidConstraint[i];
                    printf("\t*** Warning *** rigidConstraint Input %s to astrosAIM not understood!\n", tempString);
                    symmetryFlag = 1;
                }

                if (trimType == 1 || trimType == symmetryFlag) {
                    fprintf(fp,"%s%7s", delimiter,tempString);
                    tempString = NULL;

                    tempString = convert_doubleToString(feaAnalysis->magRigidConstraint[i], fieldWidth, 1);
                    fprintf(fp, "%s%s", delimiter, tempString);
                    EG_free(tempString);

                    sidIndex += 2;
                }

                tempString = NULL;


            } // if i == feaAnalysis->numRigidConstraint
        }

        for (i = 0; i < feaAnalysis->numRigidVariable; i++) {

            //if (sidIndex % (8*lineCount) == 0) {
            if (sidIndex % 8 == 0) {

                //if (lineCount == 1) fprintf(fp,"%8s", " ");

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ",+T%-5d\n", lineCount-1); // Start of continuation
                    fprintf(fp, "+T%-5d,"  , lineCount-1);  // End of continuation
                } else {
                    fprintf(fp, "+T%-6d\n", lineCount-1);  // Start of continuation
                    fprintf(fp, "+T%-6d"  , lineCount-1);  // End of continuation
                }

                lineCount += 1;
                sidIndex = 0;
            }

            // ASTROS VARIABLES
            //SYM: 'NX','NZ','QACCEL','ALPHA','QRATE','THKCAM'
            //ANTISYM: 'NY','PACCEL','RACCEL','BETA','PRATE','RRATE'

            // NASTRAN VARIABLES
            //SYM: URDD1, URDD3, URDD5, ANGLEA, PITCH
            //ANTISYM: URDD2, URDD4, URDD6, SIDES, ROLL, YAW

            // SYMMETRY
            // SYM (0)
            // ANTISYM (-1)

            if(strcmp("URDD1",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "NX";
                symmetryFlag = 0;
            } else if(strcmp("URDD2",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "NY";
                symmetryFlag = -1;
            } else if(strcmp("URDD3",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "NZ";
                symmetryFlag = 0;
            } else if(strcmp("URDD4",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "PACCEL";
                symmetryFlag = -1;
            } else if(strcmp("URDD5",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "QACCEL";
                symmetryFlag = 0;
            } else if(strcmp("URDD6",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "RACCEL";
                symmetryFlag = -1;
            } else if(strcmp("ANGLEA",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "ALPHA";
                symmetryFlag = 0;
            } else if(strcmp("PITCH",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "QRATE";
                symmetryFlag = 0;
            } else if(strcmp("SIDES",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "BETA";
                symmetryFlag = -1;
            } else if(strcmp("ROLL",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "PRATE";
                symmetryFlag = -1;
            } else if(strcmp("YAW",feaAnalysis->rigidVariable[i]) == 0) {
                tempString = "RRATE";
                symmetryFlag = -1;
            } else {
                tempString = feaAnalysis->rigidVariable[i];
                printf("\t*** Warning *** rigidVariable Input %s to astrosAIM not understood!\n", tempString);
                symmetryFlag = 1;
            }

            if (trimType == 1 || trimType == symmetryFlag) {
                fprintf(fp,"%s%7s", delimiter,tempString);
                tempString = NULL;

                fprintf(fp, "%s%7s", delimiter, "FREE");

                sidIndex += 2;
            }
            tempString = NULL;
        }

        for (i = 0; i < feaAnalysis->numControlConstraint; i++) {

            if (sidIndex % 8 == 0) {

                //if (lineCount == 1) fprintf(fp,"%8s", " ");

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ",+T%-5d\n", lineCount-1); // Start of continuation
                    fprintf(fp, "+T%-5d,"  , lineCount-1);  // End of continuation
                } else {
                    fprintf(fp, "+T%-6d\n", lineCount-1);  // Start of continuation
                    fprintf(fp, "+T%-6d"  , lineCount-1);  // End of continuation
                }

                lineCount += 1;
                sidIndex = 0;
            }

            fprintf(fp,"%s%7s", delimiter,feaAnalysis->controlConstraint[i]);

            tempString = convert_doubleToString(feaAnalysis->magControlConstraint[i], fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            sidIndex += 2;
        }

        fprintf(fp, "\n");
    }

    if (feaAnalysis->analysisType == AeroelasticFlutter) {
        if (feaAnalysis->aeroSymmetryXY == NULL ) {
            printf("\t*** Warning *** aeroSymmetryXY Input to AeroelasticFlutter Analysis in astrosAIM not defined! Using ASYMMETRIC\n");
            symxy = 0;
        } else {
            if(strcmp("SYM",feaAnalysis->aeroSymmetryXY) == 0) {
                symxy = 0;
            } else if(strcmp("ANTISYM",feaAnalysis->aeroSymmetryXY) == 0) {
                symxy = -1;
            } else if(strcmp("ASYM",feaAnalysis->aeroSymmetryXY) == 0) {
                symxy = 1;
            } else if(strcmp("SYMMETRIC",feaAnalysis->aeroSymmetryXY) == 0) {
                symxy = 0;
            } else if(strcmp("ANTISYMMETRIC",feaAnalysis->aeroSymmetryXY) == 0) {
                symxy = -1;
            } else if(strcmp("ASYMMETRIC",feaAnalysis->aeroSymmetryXY) == 0) {
                symxy = 1;
            } else {
                printf("\t*** Warning *** aeroSymmetryXY Input %s to astrosAIM not understood! Using ASYMMETRIC\n",feaAnalysis->aeroSymmetryXY );
                symxy = 0;
            }
        }

        if (feaAnalysis->aeroSymmetryXZ == NULL ) {
            printf("\t*** Warning *** aeroSymmetryXY Input to AeroelasticFlutter Analysis in astrosAIM not defined! Using ASYMMETRIC\n");
            symxz = 0;
        } else {
            if(strcmp("SYM",feaAnalysis->aeroSymmetryXZ) == 0) {
                symxz = 0;
            } else if(strcmp("ANTISYM",feaAnalysis->aeroSymmetryXZ) == 0) {
                symxz = -1;
            } else if(strcmp("ASYM",feaAnalysis->aeroSymmetryXZ) == 0) {
                symxz = 1;
            } else if(strcmp("SYMMETRIC",feaAnalysis->aeroSymmetryXZ) == 0) {
                symxz = 0;
            } else if(strcmp("ANTISYMMETRIC",feaAnalysis->aeroSymmetryXZ) == 0) {
                symxz = -1;
            } else if(strcmp("ASYMMETRIC",feaAnalysis->aeroSymmetryXZ) == 0) {
                symxz = 1;
            } else {
                printf("\t*** Warning *** aeroSymmetryXZ Input %s to astrosAIM not understood! Using ASYMMETRIC\n",feaAnalysis->aeroSymmetryXZ );
                symxz = 0;
            }
        }

        // Write MKAERO1 INPUT
        fprintf(fp,"%-8s", "MKAERO1");

        //SYMXZ
        tempString = convert_integerToString(symxz, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        //SYMXY
        tempString = convert_integerToString(symxy, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        if (feaAnalysis->numMachNumber != 0) {
            if (feaAnalysis->numMachNumber > 6) {
                printf("\t*** Warning *** Mach number input for AeroelasticFlutter in astrosAIM must be less than six\n");
            }

            for (i = 0; i < 6; i++) {
                if (i < feaAnalysis->numMachNumber ){
                    tempString = convert_doubleToString(feaAnalysis->machNumber[i], fieldWidth, 1);
                    fprintf(fp, "%s%s", delimiter, tempString);
                    EG_free(tempString);
                } else {
                    fprintf(fp, "%s%-7s", delimiter, "");
                }
            }
        }

        if (feaAnalysis->numReducedFreq != 0) {
            if (feaAnalysis->numReducedFreq > 8) {
                printf("\t*** Warning *** Reduced freq. input for AeroelasticFlutter in astrosAIM must be less than eight\n");
            }

            fprintf(fp, "+MK\n");
            fprintf(fp, "%-8s","+MK");

            for (i = 0; i < feaAnalysis->numReducedFreq; i++) {
                tempString = convert_doubleToString(feaAnalysis->reducedFreq[i], fieldWidth, 1);
                fprintf(fp, "%s%s", delimiter, tempString);
                EG_free(tempString);
            }

            fprintf(fp, "\n");

        }

        fprintf(fp,"%s\n","$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|");
        fprintf(fp,"%s","$LUTTER SID     METHOD  DENS    MACH    VEL     MLIST   KLIST   EFFID   CONT\n");
        fprintf(fp,"%s\n","$CONT   SYMXZ   SYMXY   EPS     CURFIT");

        // Write MKAERO1 INPUT
        fprintf(fp,"%-8s", "FLUTTER");

        //SID
        tempString = convert_integerToString(feaAnalysis->analysisID, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%-7s", delimiter, "PK");

        //DENS
        tempString = convert_integerToString(10 * feaAnalysis->analysisID + 1, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        //Mach
        tempString = convert_doubleToString(feaAnalysis->machNumber[0], fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        //VEL
        tempString = convert_integerToString(10 * feaAnalysis->analysisID + 2, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%-7s", delimiter, ""); // MLIST
        fprintf(fp, "%s%-7s", delimiter, ""); // KLIST
        fprintf(fp, "%s%-7s", delimiter, ""); // EFFID

        fprintf(fp, "+FL\n");
        fprintf(fp, "%-8s","+FL");

        //SYMXZ
        tempString = convert_integerToString(symxz, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        //SYMXY
        tempString = convert_integerToString(symxy, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "\n$\n");

        fprintf(fp,"%-8s", "FLFACT");

        //DENS
        tempString = convert_integerToString(10 * feaAnalysis->analysisID + 1, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaAnalysis->density, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "\n");

        velocity = sqrt(2*feaAnalysis->dynamicPressure/feaAnalysis->density);
        vmin = velocity / 2.0;
        vmax = 2.0 * velocity;
        dv = (vmax - vmin) / 20.0;

        for (i = 0; i < 21; i++) {
            velocityArray[i] = vmin + (double) i * dv;
        }

        status = astros_writeFLFactCard(fp, feaFileFormat, 10 * feaAnalysis->analysisID + 2, 21, velocityArray);
        if (status != CAPS_SUCCESS) return status;

        fprintf(fp, "$\n");

    }

    return CAPS_SUCCESS;
}

// Write design variable/optimization information from a feaDesignVariable structure
int astros_writeDesignVariableCard(FILE *fp, feaDesignVariableStruct *feaDesignVariable, int numProperty, feaPropertyStruct feaProperty[],
                                   feaFileFormatStruct *feaFileFormat) {

    int  i; //Indexing
    int fieldWidth, len, composite, numPly = 0;
    long intVal = 0;
    int *layers=NULL;

    int status; // Function return status

    char *tempString=NULL;
    char *delimiter;
    char *copy;

    composite = (int) false;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
        fprintf(fp, "$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|\n");
    }

    if (feaDesignVariable->designVariableType != PropertyDesignVar) {
        printf("***\n*** ERROR *** For ASTROS Optimization designVariableType must be a property not a material\n***\n");
        return CAPS_BADVALUE;
    }


    if (feaDesignVariable->numDiscreteValue == 0) {
        // DESVARP BID, LINKID, VMIN, VMAX, VINIT, LAYERNUM, LAYRLST, LABEL
        fprintf(fp,"%-8s", "DESVARP");

        tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        if ( feaDesignVariable->initialValue != 0) {
            tempString = convert_doubleToString(feaDesignVariable->lowerBound/feaDesignVariable->initialValue, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaDesignVariable->upperBound/feaDesignVariable->initialValue, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaDesignVariable->initialValue/feaDesignVariable->initialValue, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

        } else {
            tempString = convert_doubleToString(0.0, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(1.0, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaDesignVariable->initialValue, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        }

        fprintf(fp, "%s%7s", delimiter, ""); // Print blank space LAYERNUM

        if (feaDesignVariable->propertySetType != NULL) {
            if (feaDesignVariable->propertySetType[0] == Composite) {
                tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);
            } else {
                fprintf(fp, "%s%7s", delimiter, ""); // Print blank space LAYRLST
            }
        } else {
            fprintf(fp, "%s%7s", delimiter, ""); // Print blank space LAYRLST
        }

        len = strlen(feaDesignVariable->name);
        if (len > 7) {
            printf("*** WARNING *** For ASTROS Optimization designVariable name \"%s\", must be 7 characters or less using default name VARi\n", feaDesignVariable->name);
            fprintf(fp, "%sVAR%d\n", delimiter, feaDesignVariable->designVariableID);
        } else {
            fprintf(fp, "%s%7s\n", delimiter,feaDesignVariable->name);
        }

    } else {
        printf("***\n*** ERROR *** For ASTROS Optimization designVariables can not be Discrete Values\n***\n");
        return CAPS_BADVALUE;
    }

    for (i = 0; i < feaDesignVariable->numPropertyID; i++) {
        // PLIST, LINKID, PTYPE, PID1, ...
        // PTYPE =  PROD, PSHEAR, PCOMP, PCOMP1, PCOMP2, PELAS, PSHELL, PMASS, PTRMEM, PQDMEM1, PBAR

        if (feaDesignVariable->propertySetType == NULL) {
            printf("*** WARNING *** For ASTROS Optimization designVariable name \"%s\", propertySetType not set. PLIST entries not written\n", feaDesignVariable->name);
            continue;
        }

        fprintf(fp,"%-8s", "PLIST");

        tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // UnknownProperty, Rod, Bar, Shear, Shell, Composite, Solid
        if (feaDesignVariable->propertySetType[i] == Rod) {
            fprintf(fp,"%s%7s",delimiter, "PROD");
        } else if (feaDesignVariable->propertySetType[i] == Bar) {
            fprintf(fp,"%s%7s",delimiter, "PBAR");
        } else if (feaDesignVariable->propertySetType[i] == Shell) {
            fprintf(fp,"%s%7s",delimiter, "PSHELL");
        } else if (feaDesignVariable->propertySetType[i] == Composite) {
            fprintf(fp,"%s%7s",delimiter, "PCOMP");
            composite = (int) true;
        } else if (feaDesignVariable->propertySetType[i] == Solid) {
            printf("***\n*** ERROR *** For ASTROS Optimization designVariables can not relate to PSOLID property types\n***\n");
            return CAPS_BADVALUE;
        }

        tempString = convert_integerToString(feaDesignVariable->propertySetID[i], fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    if (composite == (int) true) {
        // Check the field input
        if (feaDesignVariable->fieldName == NULL) {
            printf("***\n*** ERROR *** For ASTROS Optimization designVariables must have fieldName defined\n***\n");
            return CAPS_BADVALUE;
        }

        // Check if angle is input (i.e. not lamina thickness)
        if (strncmp(feaDesignVariable->fieldName, "THETA", 5) == 0) {
            printf("***\n*** ERROR *** For ASTROS Optimization designVariables, fieldName can not be an angle (i.e. THETAi)\n***\n");
            return CAPS_BADVALUE;
        }

        // Search all properties to determine the number of layers in the composite
        for (i = 0; i < numProperty; i++) {
            if (feaDesignVariable->propertySetID == NULL) {
                printf("*** WARNING *** For ASTROS Optimization designVariable name \"%s\", propertySetID not set.\n", feaDesignVariable->name);
                continue;
            }

            if (feaDesignVariable->propertySetID[0] == feaProperty[i].propertyID) {
                numPly = feaProperty[i].numPly;
                if (feaProperty->compositeSymmetricLaminate == (int) true) {
                    numPly = 2 * numPly;
                }
                break;
            }
        }

        if (strcmp(feaDesignVariable->fieldName,"TALL") == 0) {
            layers = (int *) EG_alloc((numPly)*sizeof(int));
            for (i = 0; i < numPly; i++) {
                layers[i] = i+1;
            }
            status = astros_writePlyListCard(fp, feaFileFormat, feaDesignVariable->designVariableID, numPly, layers);
            if (status != CAPS_SUCCESS) return status;
        }

        if (strncmp(feaDesignVariable->fieldName,"T", 1) == 0) {

            if (strncmp(feaDesignVariable->fieldName,"THETA", 5) == 0 ||
                    strncmp(feaDesignVariable->fieldName,"TALL", 4) == 0) {
                // do nothing
            } else {
                // Input is T1, T2, etc or T11 ... Need to print the integer part

                copy = feaDesignVariable->fieldName;

                while (*copy) { // While there are more characters to process...
                    if (isdigit(*copy)) { // Upon finding a digit, ...
                        intVal = strtol(copy, &copy, 10); // Read a number, ...
                        //fprintf(fp,"***** OUTPUT %d\n", (int) intVal); // and print it.
                    } else { // Otherwise, move on to the next character.
                        copy++;
                    }
                }

                len = strlen(feaDesignVariable->fieldName);
                fprintf(fp,"%-8s", "PLYLIST");

                tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_integerToString((int) intVal, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);


                if (feaProperty->compositeSymmetricLaminate == (int) true) {
                    // need to add sym laminate layer to the PLYLIST
                    // numPly - total including sym muliplyer
                    // intVal - selected play for 1/2 the stack
                    // otherside = numPly - intVal + 1

                    tempString = convert_integerToString(numPly + 1 - (int) intVal, fieldWidth, 1);
                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);
                }

                fprintf(fp,"\n");

                //if(copy != NULL) EG_free(copy);
            }
        }

    }

    if (feaDesignVariable->numIndependVariable > 0) {
        printf("*** WARNING *** For ASTROS Optimization design variable linking has not been implemented yet\n");
    }

    /*
    if (feaDesignVariable->numIndependVariable > 0) {
        c = 1;
                // DLINK, ID,  DDVID,  CO,  CMULT,  IDV1,  C1,  IDV2,  C2
                fprintf(fp,"%-8s", "DLINK");

                tempString = convert_integerToString(feaDesignVariable->designVariableID+10000, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_doubleToString(feaDesignVariable->variableWeight[0], fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_doubleToString(feaDesignVariable->variableWeight[1], fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);

                for (i = 0; i < feaDesignVariable->numIndependVariable; i++) {

                        tempString = convert_integerToString(feaDesignVariable->independVariableID[i], fieldWidth, 1);
                        fprintf(fp, "%s%s",delimiter, tempString);
                        EG_free(tempString);

                        tempString = convert_doubleToString(feaDesignVariable->independVariableWeight[i], fieldWidth, 1);
                        fprintf(fp, "%s%s",delimiter, tempString);
                        EG_free(tempString);

                        if (i > feaDesignVariable->numIndependVariable-1 && i == c) {
                                c = c + 4;
                                if (feaFileFormat->fileType == FreeField) {
                                        fprintf(fp, ",+C\n+, ");
                                } else {
                                        fprintf(fp, "+C%6s\n%8s", "", "");
                                }
                        } else if (i == feaDesignVariable->numIndependVariable-1) {
                                fprintf(fp,"\n");
                        }

                }
    }
     */

    if (layers != NULL) EG_free(layers);

    return CAPS_SUCCESS;
}

// Write design constraint/optimization information from a feaDesignConstraint structure
int astros_writeDesignConstraintCard(FILE *fp, int feaDesignConstraintSetID, feaDesignConstraintStruct *feaDesignConstraint,
                                     int numMaterial, feaMaterialStruct feaMaterial[],
                                     int numProperty, feaPropertyStruct feaProperty[],
                                     feaFileFormatStruct *feaFileFormat) {

    int  i, j; // Index
    int  iPID = 0, iMID = 0;
    long intVal = 0;

    int fieldWidth;

    char *tempString=NULL;
    char *delimiter, *copy;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
        //fprintf(fp, "$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|\n");
    }

    for (i = 0; i < feaDesignConstraint->numPropertyID; i++) {

        if (feaDesignConstraint->propertySetType[i] == Rod) {
            // DCONVMP, SID, ST, SC, SS, PTYPE, LAYRNUM, PID1, PID2, CONT
            // CONT, PID2, PID4, ETC ...
            /* SID Stress constraint set identification (Integer > 0).
             * ST Tensile stress limit (Real > 0.0 or blank)
             * Sc Compressive stress limit (Real, Default = ST)
             * ss Shear stress limit (Real > 0.0 or blank)
             * PTYPE Property type (Text)
             * LAYPNUM The layer number of a composite element (Integer > 0 or blank)
             * PIDi Property identification numbers (Integer > 0)
             */
            fprintf(fp,"%-8s", "DCONVMP");

            tempString = convert_integerToString(feaDesignConstraintSetID, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            // ST - Tensile Stress Limit
            tempString = convert_doubleToString(feaDesignConstraint->upperBound, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            // SC - Compressive Stress Limit (This is Von - Mises ...)
            fprintf(fp, "%s%7s", delimiter, "");

            // Shear Stress - This is a ROD element - tension / compression only
            fprintf(fp, "%s%7s", delimiter, "");

            fprintf(fp,"%s%7s", delimiter, "PROD");

            fprintf(fp, "%s%7s", delimiter, ""); // LAYERNUM - not composite

            tempString = convert_integerToString(feaDesignConstraint->propertySetID[i], fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);


        } else if (feaDesignConstraint->propertySetType[i] == Bar) {
            // Nothing set yet
        } else if (feaDesignConstraint->propertySetType[i] == Shell) {
            // DCONVMP, SID, ST, SC, SS, PTYPE, LAYRNUM, PID1, PID2, CONT
            // CONT, PID2, PID4, ETC ...
            fprintf(fp,"%-8s", "DCONVMP");

            tempString = convert_integerToString(feaDesignConstraintSetID, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            // ST - Tensile Stress Limit
            tempString = convert_doubleToString(feaDesignConstraint->upperBound, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            // SC - Compressive Stress Limit (This is Von - Mises ...)
            fprintf(fp, "%s%7s", delimiter, "");

            // Shear Stress set to 0.5 of upperBound
            tempString = convert_doubleToString(feaDesignConstraint->upperBound / 2.0, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            fprintf(fp,"%s%7s", delimiter, "PSHELL");

            fprintf(fp, "%s%7s", delimiter, ""); // LAYERNUM - not composite

            tempString = convert_integerToString(feaDesignConstraint->propertySetID[i], fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);


        } else if (feaDesignConstraint->propertySetType[i] == Composite) {
            /*
             * DCONTWP SID XT XC YT YC SS F12 PTYPE ICONT
             * CONT LAYRNUM PIDI ID2 PID3 -etc-
             * SID Stress constraint set identification (Integer > 0)
             * XT Tensile stress limit in the longitudinal direction (Real > 0.0)
             * XC Compressive stress limit in the longitudinal direction (Real, Default = XT)
             * YT Tensile stress limit in the transverse direction (Real > 0.0)
             * YC Compressive stress limit in the transverse direction (Real, Default = YT)
             * ss Shear stress limit for in-plane stress (Real > 0.0)
             * F12 Tsai-Wu interaction term (Real)
             * PTYPE Property type (Text)
             * LAYRNUM The layer number of a composite element (Integer > 0 or blank)
             * PIDi Property identification numbers (Integer > 0)
             */

            // Property ID for the constraint: feaDesignConstraint->propertySetID[i]
            // LIST of properties and their materials
            // numProperty, feaProperty[].propertyID
            //                          feaProperty[].materialID
            // LIST of materials
            // numMaterial, feaMaterial[].materialID
            //                          feaMaterial[].tensionAllow
            //                          feaMaterial[].compressAllow
            //                          feaMaterial[].tensionAllowLateral
            //                          feaMaterial[].compressAllowLateral
            //                          feaMaterial[].shearAllow

            for (j = 0; j < numProperty; j++) {
                if (feaDesignConstraint->propertySetID[i] == feaProperty[j].propertyID) {
                    iPID = j;
                    break;
                }
            }

            for (j = 0; j < numMaterial; j++) {
                if (feaProperty[iPID].materialID == feaMaterial[j].materialID) {
                    iMID = j;
                    break;
                }
            }

            fprintf(fp,"%-8s", "DCONTWP");

            tempString = convert_integerToString(feaDesignConstraintSetID, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaMaterial[iMID].tensionAllow, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaMaterial[iMID].compressAllow, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaMaterial[iMID].tensionAllowLateral, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaMaterial[iMID].compressAllowLateral, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaMaterial[iMID].shearAllow, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            // F12
            tempString = convert_doubleToString(0.0, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            fprintf(fp,"%s%7s", delimiter, "PCOMP");

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ",+DC\n+DC");
            } else {
                fprintf(fp, "+DC\n+DC%5s", "");
            }

            copy = feaDesignConstraint->fieldName;

            while (*copy) { // While there are more characters to process...
                if (isdigit(*copy)) { // Upon finding a digit, ...
                    intVal = strtol(copy, &copy, 10); // Read a number, ...
                    //fprintf(fp,"***** OUTPUT %d\n", (int) intVal); // and print it.
                } else { // Otherwise, move on to the next character.
                    copy++;
                }
            }

            tempString = convert_integerToString((int) intVal, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaDesignConstraint->propertySetID[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            fprintf(fp,"\n");

        } else if (feaDesignConstraint->propertySetType[i] == Solid) {
            // Nothing set yet
        }
    }

    return CAPS_SUCCESS;
}

// Read data from a Astros OUT file to determine the number of eignevalues
int astros_readOUTNumEigenValue(FILE *fp, int *numEigenVector) {

    int status; // Function return

    char *beginEigenLine = "                                   S U M M A R Y   O F   R E A L   E I G E N   A N A L Y S I S";

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder

    int tempInt[2];

    while (*numEigenVector == 0) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;

        // See how many Eigen-Values we have
        if (strncmp(beginEigenLine, line, strlen(beginEigenLine)) == 0) {

            // Skip ahead 2 lines
            status = getline(&line, &linecap, fp);
            if (status < 0) break;
            status = getline(&line, &linecap, fp);
            if (status < 0) break;

            // Grab summary line
            status = getline(&line,&linecap,fp );
            if (status < 0) break;

            sscanf(line, "%d EIGENVALUES AND %d EIGENVECTORS", &tempInt[0],
                    &tempInt[1]);
            *numEigenVector = tempInt[1];
        }
    }

    if (line != NULL) EG_free(line);

    // Rewind the file
    rewind(fp);

    if (*numEigenVector == 0) return CAPS_NOTFOUND;
    else return CAPS_SUCCESS;
}

// Read data from a Astros OUT file to determine the number of grid points
int astros_readOUTNumGridPoint(FILE *fp, int *numGridPoint) {

    int status; // Function return status

    int i, j;

    size_t linecap;

    char *line = NULL; // Temporary line holder

    char *beginEigenLine = "            EIGENVALUE       =";
    char *endEigenLine   = "            EIGENVALUE       =";

    int stop = (int) false;

    *numGridPoint = 0;

    // Loop through file line by line until we have determined how many grid points we have
    while (*numGridPoint == 0) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;

        // Look for start of Eigen-Vector 1
        if (strncmp(beginEigenLine, line, strlen(beginEigenLine)) == 0) {

            // Fast forward 3 lines
            for (i = 0; i < 3; i++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }

            // Loop through lines counting the number of grid points
            while (stop == (int) false) {

                status = getline(&line, &linecap, fp);
                if (status < 0)  break;

                // If we have a new page - skip ahead 8 lines and continue
                if (strncmp("1", line, 1) == 0) {

                    for (j = 0; j < 7; j++) {
                        status = getline(&line, &linecap, fp);
                        if (status < 0) break;

                        if (strncmp(endEigenLine, line, strlen(endEigenLine)) == 0) stop = (int) true;

                    }
                    continue;
                }

                if (strncmp(endEigenLine, line, strlen(endEigenLine)) == 0 ||
                        strlen(line) == 1) break;

                *numGridPoint +=1;
            }
        }
    }

    if (line != NULL) EG_free(line);

    // Rewind the file
    rewind(fp);

    if (*numGridPoint == 0) return CAPS_NOTFOUND;
    else return CAPS_SUCCESS;
}


// Read data from a Astros OUT file and load it into a dataMatrix[numEigenVector][numGridPoint*8]
// where variables are Grid Id, Coord Id, T1, T2, T3, R1, R2, R3
int astros_readOUTEigenVector(FILE *fp, int *numEigenVector, int *numGridPoint, double ***dataMatrix) {

    int status = CAPS_SUCCESS; // Function return

    int i, j, eigenValue = 0; // Indexing

    size_t linecap;

    char *line = NULL; // Temporary line holder

    char *beginEigenLine = "            EIGENVALUE       =";

    int tempInt;
    char tempString[2];

    int numVariable = 8; // Grid Id, Coord Id, T1, T2, T3, R1, R2, R3

    printf("Reading Astros OUT file - extracting Eigen-Vectors!\n");

    *numEigenVector = 0;
    *numGridPoint = 0;

    // See how many Eigen-Values we have
    status = astros_readOUTNumEigenValue(fp, numEigenVector);
    printf("\tNumber of Eigen-Vectors = %d\n", *numEigenVector);
    if (status != CAPS_SUCCESS) return status;

    status = astros_readOUTNumGridPoint(fp, numGridPoint);
    printf("\tNumber of Grid Points = %d for each Eigen-Vector\n", *numGridPoint);
    if (status != CAPS_SUCCESS) return status;

    // Allocate dataMatrix array
    if (*dataMatrix != NULL) EG_free(*dataMatrix);

    *dataMatrix = (double **) EG_alloc((*numEigenVector) *sizeof(double *));
    if (*dataMatrix == NULL) {
        status = EGADS_MALLOC; // If allocation failed ....
        goto cleanup;
    }

    for (i = 0; i < *numEigenVector; i++) {

        (*dataMatrix)[i] = (double *) EG_alloc((*numGridPoint)*numVariable*sizeof(double));

        if ((*dataMatrix)[i] == NULL) { // If allocation failed ....
            for (j = 0; j < i; j++) {

                if ((*dataMatrix)[j] != NULL ) EG_free((*dataMatrix)[j]);
            }

            if ((*dataMatrix) != NULL) EG_free((*dataMatrix));

            status = EGADS_MALLOC;
            goto cleanup;
        }
    }

    eigenValue = 0;
    // Loop through the file again and pull out data
    while (getline(&line, &linecap, fp) >= 0) {

        // Look for start of Eigen-Vector
        if (strncmp(beginEigenLine, line, strlen(beginEigenLine)) == 0) {

            printf("\tLoading Eigen-Vector = %d\n", eigenValue+1);

            // Fast forward 3 lines
            for (i = 0; i < 3; i++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }

            i = 0;
            while (i != *numGridPoint) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;

                // If we have a new page - skip ahead 7 lines and continue
                if (strncmp("1", line, 1) == 0) {

                    for (j = 0; j < 7; j++) {
                        status = getline(&line, &linecap, fp);
                        if (status < 0) break;
                    }
                    continue;
                }

                sscanf(line, "%d%s%lf%lf%lf%lf%lf%lf", &tempInt,
                                                       tempString,
                                                       &(*dataMatrix)[eigenValue][2+numVariable*i],
                                                       &(*dataMatrix)[eigenValue][3+numVariable*i],
                                                       &(*dataMatrix)[eigenValue][4+numVariable*i],
                                                       &(*dataMatrix)[eigenValue][5+numVariable*i],
                                                       &(*dataMatrix)[eigenValue][6+numVariable*i],
                                                       &(*dataMatrix)[eigenValue][7+numVariable*i]);

                (*dataMatrix)[eigenValue][0+numVariable*i] = (double) i+1;
                (*dataMatrix)[eigenValue][1+numVariable*i] = 0.0;

                /*
                printf("Data = %f %f %f %f %f %f %f %f\n", (*dataMatrix)[eigenValue][0+numVariable*i],
                                                           (*dataMatrix)[eigenValue][1+numVariable*i],
                                                           (*dataMatrix)[eigenValue][2+numVariable*i],
                                                           (*dataMatrix)[eigenValue][3+numVariable*i],
                                                           (*dataMatrix)[eigenValue][4+numVariable*i],
                                                           (*dataMatrix)[eigenValue][5+numVariable*i],
                                                           (*dataMatrix)[eigenValue][6+numVariable*i],
                                                           (*dataMatrix)[eigenValue][7+numVariable*i]);
                 */
                i = i+1;
            }

            eigenValue += 1;

            // Skip ahead 6 lines after reading an eigenvector
            for (j = 0; j < 6; j++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }
        }

        if (eigenValue == *numEigenVector) break;
    }

    if (eigenValue != *numEigenVector) {
        printf("\tOnly %d of %d Eigen-Vectors read!", eigenValue, *numEigenVector);
        status = CAPS_NOTFOUND;
    } else {
        status = CAPS_SUCCESS;
    }

    goto cleanup;

    cleanup:
        if (line != NULL) EG_free(line);

        return status;
}

// Read data from a Astros OUT file and load it into a dataMatrix[numEigenVector][5]
// where variables are eigenValue, eigenValue(radians), eigenValue(cycles), generalized mass, and generalized stiffness.                                                                   MASS              STIFFNESS
int astros_readOUTEigenValue(FILE *fp, int *numEigenVector, double ***dataMatrix) {

    int status; // Function return

    int i, j;// Indexing

    int tempInt, eigenValue =0;

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder

    char *beginEigenLine = "                              ORDER         (RAD/S)**2         (RAD/S)           (HZ)            MASS           STIFFNESS";
    //char *endEigenLine = "1";

    int numVariable = 5; // EigenValue, eigenValue(radians), eigenValue(cycles), generalized mass, and generalized stiffness.

    printf("Reading Astros OUT file - extracting Eigen-Values!\n");

    *numEigenVector = 0;

    // See how many Eigen-Values we have
    status = astros_readOUTNumEigenValue(fp, numEigenVector);
    printf("\tNumber of Eigen-Values = %d\n", *numEigenVector);
    if (status != CAPS_SUCCESS) return status;

    // Allocate dataMatrix array
    if (*dataMatrix != NULL) EG_free(*dataMatrix);

    *dataMatrix = (double **) EG_alloc(*numEigenVector *sizeof(double *));
    if (*dataMatrix == NULL) return EGADS_MALLOC; // If allocation failed ....

    for (i = 0; i < *numEigenVector; i++) {

        (*dataMatrix)[i] = (double *) EG_alloc(numVariable*sizeof(double));

        if ((*dataMatrix)[i] == NULL) { // If allocation failed ....
            for (j = 0; j < i; j++) {

                if ((*dataMatrix)[j] != NULL ) EG_free((*dataMatrix)[j]);
            }

            if ((*dataMatrix) != NULL) EG_free((*dataMatrix));

            return EGADS_MALLOC;
        }
    }

    // Loop through the file again and pull out data
    while (eigenValue != *numEigenVector) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;

        // Look for start of Eigen-Vector
        if (strncmp(beginEigenLine, line, strlen(beginEigenLine)) == 0) {

            // Fast forward 1 lines
            for (i = 0; i < 1; i++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }

            i = 0;
            while (eigenValue != *numEigenVector) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;

                // If we have a new page - skip ahead 8 lines and continue
                if (strncmp("1", line, 1) == 0) {

                    for (j = 0; j < 8; j++) {
                        status = getline(&line, &linecap, fp);
                        if (status < 0) break;
                    }
                    continue;
                }

                // Loop through the file and fill up the data matrix
                sscanf(line, "%d%d%lf%lf%lf%lf%lf", &eigenValue,
                        &tempInt,
                        &(*dataMatrix)[i][0],
                        &(*dataMatrix)[i][1],
                        &(*dataMatrix)[i][2],
                        &(*dataMatrix)[i][3],
                        &(*dataMatrix)[i][4]);
                printf("\tLoading Eigen-Value = %d\n", eigenValue);
                i += 1;
            }
        }
    }

    if (line != NULL) EG_free(line);

    return CAPS_SUCCESS;
}

// Read data from a Astros OUT file and load it into a dataMatrix[numGridPoint][8]
// where variables are Grid Id, Coord Id, T1, T2, T3, R1, R2, R3
int astros_readOUTDisplacement(FILE *fp, int subcaseId, int *numGridPoint, double ***dataMatrix) {

    int status; // Function return

    int i, j; // Indexing

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder

    char *outputSubcaseLine = "0                                                                                                            SUBCASE ";
    char *displacementLine ="                                             D I S P L A C E M E N T   V E C T O R";
    char *beginSubcaseLine=NULL;
    char *endSubcaseLine = "1";
    char tempString[2];

    int numVariable = 8; // Grid Id, coord Id T1, T2, T3, R1, R2, R3
    int intLength;
    int lineFastForward = 0;

    if (subcaseId != -1) {
        printf("Reading Astros FO6 file - extracting Displacements!\n");
    }

    *numGridPoint = 0;

    if (*dataMatrix != NULL) {
      printf("Developer error: dataMatrix should be NULL!\n");
      return CAPS_NULLVALUE;
    }


    if      (subcaseId >= 1000) intLength = 4;
    else if (subcaseId >= 100) intLength = 3;
    else if (subcaseId >= 10) intLength = 2;
    else intLength = 1;

    if (subcaseId == -1) {

        /* count number of grid points */
        rewind(fp);

        while (1) {
            status = getline(&line, &linecap, fp);
            if (status < 0) break;

            if (strstr(line, "D I S P L A C E M E N T   V E C T O R") != NULL) break;
        }

        (void) getline(&line, &linecap, fp);
        (void) getline(&line, &linecap, fp);

        while (1) {
            status = getline(&line, &linecap, fp);
            if (status < 0) break;

            if (strstr(line, "S T R E S S E S") != NULL) break;

            if (strstr(line, "   G   ") != NULL) {
                (*numGridPoint)++;
            }
        }

        /* allocate space for the grid points */
        *dataMatrix = (double **) EG_alloc((*numGridPoint)*sizeof(double *));
        if (*dataMatrix == NULL) { status = EGADS_MALLOC; goto cleanup; }

        for (i = 0; i < (*numGridPoint); i++) {
            (*dataMatrix)[i] = (double *) EG_alloc(8*sizeof(double));
            if ((*dataMatrix)[i] == NULL) { status = EGADS_MALLOC; goto cleanup; }
        }

        /* read the grid points */
        rewind(fp);

        while (1) {
            status = getline(&line, &linecap, fp);
            if (status < 0) break;

            if (strstr(line, "D I S P L A C E M E N T   V E C T O R") != NULL) break;
        }

        (void) getline(&line, &linecap, fp);
        (void) getline(&line, &linecap, fp);

        i = 0;
        while (i < *numGridPoint) {
            int igid;

            status = getline(&line, &linecap, fp);
            if (strstr(line, "   G   ") != NULL) {
                sscanf(line, "%d G %lf %lf %lf %lf %lf %lf",
                       &igid, &(*dataMatrix)[i][2], &(*dataMatrix)[i][3], &(*dataMatrix)[i][4],
                              &(*dataMatrix)[i][5], &(*dataMatrix)[i][6], &(*dataMatrix)[i][7]);
                (*dataMatrix)[i][0] = igid;
                (*dataMatrix)[i][1] = 0;
                i++;
            }
        }

        status = CAPS_SUCCESS;
        goto cleanup;
    }

    if (subcaseId > 0) {
        beginSubcaseLine = (char *) EG_alloc((strlen(outputSubcaseLine)+intLength+1)*sizeof(char));
        if (beginSubcaseLine == NULL) { status = EGADS_MALLOC; goto cleanup; }

        sprintf(beginSubcaseLine,"%s%d",outputSubcaseLine, subcaseId);

        lineFastForward = 4;

    } else {

        intLength = 0;
        beginSubcaseLine = (char *) EG_alloc((strlen(displacementLine)+1)*sizeof(char));
        if (beginSubcaseLine == NULL) { status = EGADS_MALLOC; goto cleanup; }
        sprintf(beginSubcaseLine,"%s",displacementLine);

        lineFastForward = 2;
    }
    printf("beginSubcaseLine=%s, lineFastForward=%d\n", beginSubcaseLine, lineFastForward);

    beginSubcaseLine[strlen(outputSubcaseLine)+intLength] = '\0';

    // Loop through file line by line until we have determined how many grid points we have
    while (*numGridPoint == 0) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;
        printf("line=%s\n", line);

        // Look for start of subcaseId
        if (strncmp(beginSubcaseLine, line, strlen(beginSubcaseLine)) == 0) {

            // Fast forward lines
            for (i = 0; i < lineFastForward; i++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }

            // Loop through lines counting the number of grid points
            while (getline(&line, &linecap, fp) >= 0) {
                if (strncmp(endSubcaseLine, line, strlen(endSubcaseLine)) == 0) break;
                *numGridPoint +=1;
            }
        }
    }

    printf("Number of Grid Points = %d\n", *numGridPoint);

    if (*numGridPoint == 0) {
        printf("Either data points  = 0 and/or subcase wasn't found\n");

        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Rewind the file
    rewind(fp);

    // Allocate dataMatrix array
    *dataMatrix = (double **) EG_alloc(*numGridPoint *sizeof(double *));
    if (*dataMatrix == NULL) { status = EGADS_MALLOC; goto cleanup; }

    for (i = 0; i < *numGridPoint; i++) {

        (*dataMatrix)[i] = (double *) EG_alloc(numVariable*sizeof(double));

        if ((*dataMatrix)[i] == NULL) { // If allocation failed ....
            for (j = 0; j < *numGridPoint; j++) {
                EG_free((*dataMatrix)[j]);
            }
            EG_free((*dataMatrix));

            status = EGADS_MALLOC;
            goto cleanup;
        }
    }

    // Loop through the file again and pull out data
    while (getline(&line, &linecap, fp) >= 0) {

        // Look for start of Displacement
        if (strncmp(beginSubcaseLine, line, strlen(beginSubcaseLine)) == 0) {

            printf("Loading displacements for Subcase = %d\n", subcaseId);

            // Fast forward lines
            for (i = 0; i < lineFastForward; i++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }

            // Loop through the file and fill up the data matrix
            for (i = 0; i < (*numGridPoint); i++) {
                for (j = 0; j < numVariable; j++) {

                    if (j == 0 || j % numVariable+1 == 0) {
                        fscanf(fp, "%lf", &(*dataMatrix)[i][j]);
                        fscanf(fp, "%s", tempString);
                        j = j + 1;
                        (*dataMatrix)[i][j] = 0.0;
                    } else {
                        fscanf(fp, "%lf", &(*dataMatrix)[i][j]);
                    }
                }
            }

            break;
        }
    }

    status = CAPS_SUCCESS;

cleanup:
    EG_free(beginSubcaseLine);
    EG_free(line);

    return status;
}

// Write out a DVGRID entry
static int astros_writeDVGRIDCard(FILE *fp, int dvID, meshNodeStruct node, double scaleCoeff, double designVec[3], feaFileFormatStruct *feaFileFormat)
{
    int fieldWidth;
    char *tempString=NULL;
    char *delimiter;

    if (fp == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = "";
        fieldWidth = 8;
    }

    fprintf(fp,"%-8s", "DVGRID");

    tempString = convert_integerToString(dvID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(node.nodeID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    fprintf(fp,"%-8s", " "); // CID blank field

    tempString = convert_doubleToString(scaleCoeff, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(designVec[0], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(designVec[1], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(designVec[2], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    fprintf(fp, "\n");

    return CAPS_SUCCESS;
}

static int astros_writeSNORMCard(FILE *fp, meshNodeStruct node, double snorm[3], int patchID, int cAxis, feaFileFormatStruct *feaFileFormat)
{
    int fieldWidth;
    char *tempString=NULL;
    char *delimiter;
    int coordID = 0;
    feaMeshDataStruct *feaData;

    if (fp == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;

    }

    if (node.analysisType == MeshStructure) {
        feaData = (feaMeshDataStruct *) node.analysisData;
        coordID = feaData->coordID;
    } else {
        printf("Incorrect analysis type for node %d", node.nodeID);
        return CAPS_BADVALUE;
    }

    fprintf(fp,"%-8s", "SNORM");

    tempString = convert_integerToString(node.nodeID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(coordID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(snorm[0], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(snorm[1], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(snorm[2], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(patchID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    // need control from pyCAPS script over cAxis value---blank (omit last field) for default
    //tempString = convert_integerToString(cAxis, fieldWidth, 1);
    //fprintf(fp, "%s%s",delimiter, tempString);
    //EG_free(tempString);

    fprintf(fp, "\n");

    return CAPS_SUCCESS;
}

static int astros_writeSNORMDTCard(FILE *fp, int dvID, meshNodeStruct node, double snormdt[3], int patchID, feaFileFormatStruct *feaFileFormat)
{
    int fieldWidth;
    char *tempString=NULL;
    char *delimiter;
    int coordID = 0;
    feaMeshDataStruct *feaData;
    double mag, vec[3];

    if (fp == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;

    }

    if (node.analysisType == MeshStructure) {
        feaData = (feaMeshDataStruct *) node.analysisData;
        coordID = feaData->coordID;
    } else {
        printf("Incorrect analysis type for node %d", node.nodeID);
        return CAPS_BADVALUE;
    }

    // Get vector length
    mag = dot_DoubleVal(snormdt, snormdt);
    mag = sqrt(mag);

    vec[0] = snormdt[0];
    vec[1] = snormdt[1];
    vec[2] = snormdt[2];

    if (mag != 0.0) {
        vec[0] /= mag;
        vec[1] /= mag;
        vec[2] /= mag;
    }

    fprintf(fp,"%-8s", "SNORMDT");

    tempString = convert_integerToString(dvID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(node.nodeID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(coordID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(mag, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(vec[0], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(vec[1], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(vec[2], fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_integerToString(patchID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    fprintf(fp, "\n");

    return CAPS_SUCCESS;
}

static int check_edgeInCoplanarFace(ego edge, double t, int currentFaceIndex, ego body, int *coplanarFlag){

    int status; // Function return status

    int faceIndex, loopIndex, edgeIndex; // Indexing

    int numBodyFace;
    ego *bodyFace = NULL;

    int numChildrenLoop, numChildrenEdge;
    ego *childrenLoop, *childrenEdge;

    ego geomRef;
    int oclass,  mtype, *senses;
    double data[18], params[2];

    double mag;
    double residual = 1E-6; // Tolerence residual for normal comparison
    double normal[3], normal2[3];

    *coplanarFlag = (int) false;

    status = EG_getBodyTopos(body, NULL, FACE, &numBodyFace, &bodyFace);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Get uv on face at edge - need to check sense setting currently 0;
    status = EG_getEdgeUV(bodyFace[currentFaceIndex], edge, 0, t, data);
    if (status != EGADS_SUCCESS) goto cleanup;

    params[0] = data[0];
    params[1] = data[2];

    // Get derivative along face
    status = EG_evaluate(bodyFace[currentFaceIndex], params, data);
    if (status != EGADS_SUCCESS) goto cleanup;

    // Get face normal
    (void) cross_DoubleVal(&data[3], &data[6], normal);

    mag = dot_DoubleVal(normal, normal);
    mag = sqrt(mag);

    normal[0] = fabs(normal[0])/mag;
    normal[1] = fabs(normal[1])/mag;
    normal[2] = fabs(normal[2])/mag;

    for (faceIndex = 0; faceIndex < numBodyFace; faceIndex++) {

        if (faceIndex == currentFaceIndex) continue;

        status = EG_getTopology(bodyFace[faceIndex],
                &geomRef, &oclass, &mtype, data,
                &numChildrenLoop, &childrenLoop, &senses);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (loopIndex = 0; loopIndex < numChildrenLoop; loopIndex++) {

            status = EG_getTopology(childrenLoop[loopIndex],
                    &geomRef, &oclass, &mtype, data,
                    &numChildrenEdge, &childrenEdge, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;

            for (edgeIndex = 0; edgeIndex < numChildrenEdge; edgeIndex++) {

                if (edge != childrenEdge[edgeIndex]) continue;

                // Get uv on face at edge - need to check sense setting currently 0;
                status = EG_getEdgeUV(bodyFace[faceIndex], edge, 0, t, data);
                if (status != EGADS_SUCCESS) goto cleanup;

                params[0] = data[0];
                params[1] = data[2];

                // Get derivative along face
                status = EG_evaluate(bodyFace[faceIndex], params, data);
                if (status != EGADS_SUCCESS) goto cleanup;

                // Get new face normal
                (void) cross_DoubleVal(&data[3], &data[6], normal2);

                mag = dot_DoubleVal(normal2, normal2);
                mag = sqrt(mag);

                normal2[0] = fabs(normal2[0])/mag;
                normal2[1] = fabs(normal2[1])/mag;
                normal2[2] = fabs(normal2[2])/mag;

                if (fabs(normal[0] - normal2[0]) <= residual &&
                        fabs(normal[1] - normal2[1]) <= residual &&
                        fabs(normal[2] - normal2[2]) <= residual) {

                    *coplanarFlag = true;
                    edgeIndex = numChildrenEdge;
                    loopIndex = numChildrenLoop;
                    faceIndex = numBodyFace;
                    break;
                }
            }
        }
    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in check_edgeInCoplanarFace, status %d\n", status);

        if (bodyFace != NULL) EG_free(bodyFace);

        return status;
}

#ifdef DEFINED_BUT_NOT_USED
static int check_nodeNormalHist(double normal[3], int *numNormal, double *normalHist[], int *normalExist) {

    int status; // Function return status

    int i; // Indexing

    double mag;
    double residual = 1E-6; // Tolerence residual for normal comparison

    double scaledNormal[3];

    *normalExist = (int) false;

    mag = dot_DoubleVal(normal, normal);
    mag = sqrt(mag);

    scaledNormal[0] = fabs(normal[0])/mag;
    scaledNormal[1] = fabs(normal[1])/mag;
    scaledNormal[2] = fabs(normal[2])/mag;

    // Does the normal already exist
    for (i = 0; i < *numNormal; i++) {

        // Have a miss-step between numNormal and normalHist
        if (normalHist == NULL) return CAPS_NULLVALUE;

        if (fabs(scaledNormal[0] - (*normalHist)[3*i + 0]) <= residual &&
                fabs(scaledNormal[1] - (*normalHist)[3*i + 1]) <= residual &&
                fabs(scaledNormal[2] - (*normalHist)[3*i + 2]) <= residual) {

            *normalExist = (int) true;
            break;
        }
    }

    if (*normalExist == (int) true) {
        status = CAPS_SUCCESS;
        goto cleanup;
    }

    *numNormal += 1;

    *normalHist = (double *) EG_reall(*normalHist, (*numNormal)*3*sizeof(double));
    if (*normalHist == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    //printf("Normal = %f %f %f\n", normal[0], normal[1], normal[2]);

    (*normalHist)[3*(*numNormal-1) + 0] = scaledNormal[0];
    (*normalHist)[3*(*numNormal-1) + 1] = scaledNormal[1];
    (*normalHist)[3*(*numNormal-1) + 2] = scaledNormal[2];

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in check_nodeNormalHist, status %d\n", status);

        return status;
}
#endif

// Get the configuration sensitivity at a given point and write out the SNORMDT card
static int astros_getConfigurationSens(FILE *fp,
                                       void *aimInfo,
                                       int numDesignVariable,
                                       feaDesignVariableStruct *feaDesignVariable,
                                       feaFileFormatStruct *feaFileFormat,
                                       int numGeomIn,
                                       capsValue *geomInVal,
                                       ego tess, int topoType, int topoIndex, int pointIndex,
                                       double *pointNorm, int patchID, meshNodeStruct node) {

    int status = 0; // Function return

    int i, j; // Indexing

    const char *geomInName;

    int numPoint;
    double *dxyz = NULL;

    double mag, snormDT[3];

    if (fp                == NULL)  return CAPS_NULLVALUE;
    if (aimInfo           == NULL ) return CAPS_NULLVALUE;
    if (feaDesignVariable == NULL ) return CAPS_NULLVALUE;
    if (geomInVal         == NULL ) return CAPS_NULLVALUE;
    if (feaFileFormat     == NULL ) return CAPS_NULLVALUE;
    if (pointNorm         == NULL) return CAPS_NULLVALUE;

    // Loop through design variables
    for (i = 0; i < numDesignVariable; i++) {

        for (j = 0; j < numGeomIn; j++) {

            status = aim_getName(aimInfo, j+1, GEOMETRYIN, &geomInName);
            if (status != CAPS_SUCCESS) goto cleanup;

            if (strcmp(feaDesignVariable[i].name, geomInName) == 0) break;
        }

        // If name is found in Geometry inputs skip design variables
        if (j >= numGeomIn) continue;

        if(aim_getGeomInType(aimInfo, j+1) == EGADS_OUTSIDE) {
            printf("Error: Geometric sensitivity not available for CFGPMTR = %s\n", geomInName);
            status = CAPS_NOSENSITVTY;
            goto cleanup;
        }

        //printf("Geometric sensitivity name = %s\n", feaDesignVariable[j].name);

        if (dxyz) EG_free(dxyz);
        dxyz = NULL;

        if (geomInVal[j].length > 1) {
            printf("Warning: Can NOT write SNORMDT cards for multidimensional design variables!\n");
            continue;
        }

        status = aim_setSensitivity(aimInfo, geomInName, 1, 1);
        if (status != CAPS_SUCCESS) goto cleanup;


        status = aim_getSensitivity(aimInfo, tess, topoType, topoIndex, &numPoint, &dxyz);
        if (status != CAPS_SUCCESS) goto cleanup;

//        printf("TopoType = %d, TopoIndex = %d\n", topoType, topoIndex);
//        printf("numPoint %d, pointIndex %d\n", numPoint, pointIndex);

        if (pointIndex > numPoint) {
            status = CAPS_BADINDEX;
            goto cleanup;
        }

        /*
                printf("Config Sense %f %f %f\n", dxyz[3*(pointIndex-1) + 0],
                                                                      dxyz[3*(pointIndex-1) + 1],
                                                                                  dxyz[3*(pointIndex-1) + 2]);
         */

        // Normalize in incoming normal vector - just in case
        mag = dot_DoubleVal(pointNorm, pointNorm);

        pointNorm[0] /= sqrt(mag);
        pointNorm[1] /= sqrt(mag);
        pointNorm[2] /= sqrt(mag);

        // Get the scalar projection of the configuration sensitivity on the normal
        mag = dot_DoubleVal(&dxyz[3*(pointIndex-1) + 0], pointNorm);

        // Get the vector project of the configuration on the normal
        snormDT[0] = mag*pointNorm[0];
        snormDT[1] = mag*pointNorm[1];
        snormDT[2] = mag*pointNorm[2];

        /*
                printf( "Norm, %f %f %f\n",pointNorm[0],
                                                                   pointNorm[1],
                                                                   pointNorm[2]);

                printf( "Snorm, %f %f %f\n",snormDT[0],
                                                                        snormDT[1],
                                                                        snormDT[2]);
         */

        printf(">>> Writing SNORMDT cards\n");
        status = astros_writeSNORMDTCard(fp, feaDesignVariable[i].designVariableID, node, snormDT, patchID, feaFileFormat);
        if (status != CAPS_SUCCESS) goto cleanup;

    } // Design variables

    cleanup:

        if (status != CAPS_SUCCESS) printf("Error: Premature exit in astros_getConfigurationSens, status %d\n", status);

        if (dxyz != NULL) EG_free(dxyz);

        return status;
}

// Write boundary normals for shape sensitivities - only valid for modifications made by Bob Canfield to Astros
static int astros_getBoundaryNormal(FILE *fp,
                                    void *aimInfo,
                                    int numDesignVariable,
                                    feaDesignVariableStruct *feaDesignVariable,
                                    int numGeomIn,
                                    capsValue *geomInVal,
                                    meshStruct *feaMesh, feaFileFormatStruct *feaFileFormat) {

    int status; // Function return status

    int i, j, k, faceIndex, loopIndex, edgeIndex; // Indexing

    int numMesh;
    meshStruct *mesh = NULL;
    int nodeOffSet = 0; //Keep track of global node indexing offset due to combining multiple meshes

    ego body;

    int numBodyNode, numBodyEdge, numBodyFace;
    ego *bodyNode = NULL, *bodyEdge = NULL, *bodyFace = NULL;

    int numChildrenLoop, numChildrenEdge, numChildrenNode;
    ego *childrenLoop, *childrenEdge, *childrenNode;

    int numPoint;
    int tessState, pointLocalIndex, nodeOrEdge, pointTopoIndex, edgeTopoIndex;

    int len;
    const double *xyz, *t;

    ego geomRef;
    int oclass, mtypeFace, mtype, *senses, *dummySenses;
    double data[18], params[2];

    double normEdge[3], normFace[3], normBoundary[3], mag;

    int coplanarFlag;

    //int numNormalHist; //, normalExistFlag;
    //double *normalHist = NULL;

    if (fp == NULL || feaMesh == NULL || feaFileFormat == NULL) {
        status = CAPS_NULLVALUE;
        goto cleanup;
    }

    // Are we dealing with a single mesh or a combined mesh
    if (feaMesh->bodyTessMap.egadsTess != NULL) {
        numMesh = 1;
        mesh = feaMesh;

    } else {

        numMesh = feaMesh->numReferenceMesh;
    }

    if (numMesh == 0) {
        printf("No bodies with tessellations found!\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    for (i = 0; i < numMesh; i++ ) {

        if (mesh == NULL) mesh = &feaMesh->referenceMesh[i];

        status = EG_statusTessBody(mesh->bodyTessMap.egadsTess, &body, &tessState, &numPoint);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (bodyNode != NULL) EG_free(bodyNode);
        if (bodyEdge != NULL) EG_free(bodyEdge);
        if (bodyFace != NULL) EG_free(bodyFace);

        status = EG_getBodyTopos(body, NULL, NODE, &numBodyNode, &bodyNode);
        if (status < EGADS_SUCCESS) goto cleanup;

        status = EG_getBodyTopos(body, NULL, EDGE, &numBodyEdge, &bodyEdge);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_getBodyTopos(body, NULL, FACE, &numBodyFace, &bodyFace);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (j = 0; j < numPoint; j++) {

            status = EG_getGlobal(mesh->bodyTessMap.egadsTess, j+1, &pointLocalIndex, &pointTopoIndex, NULL);
            if (status != EGADS_SUCCESS) goto cleanup;

            //printf("J = %d, PointLocalIndex %d, TopoIndex %d\n", j+1, pointLocalIndex, pointTopoIndex);

            if (pointLocalIndex < 0) continue; // Don't care about face nodes; only want edge and node nodes

            nodeOrEdge = pointLocalIndex;
            edgeTopoIndex = pointTopoIndex;

            // Has this point been included in the mesh?
            for (k = 0; k < mesh->numNode; k++) {
                if (mesh->node[k].nodeID != j+1) continue;
                break;
            }

            if (k >= mesh->numNode) continue; // Point isn't in the mesh - it has been removed

            // Set our node normal history settings
            /*
            numNormalHist = 0;
            if (normalHist != NULL) EG_free(normalHist);
            normalHist = NULL;
            */

            // Loop through the faces and find what edges
            for (faceIndex = 0; faceIndex < numBodyFace; faceIndex++) {

                status = EG_getTopology(bodyFace[faceIndex],
                                        &geomRef, &oclass, &mtypeFace, data,
                                        &numChildrenLoop, &childrenLoop, &senses);
                if (status != EGADS_SUCCESS) goto cleanup;

                for (loopIndex = 0; loopIndex < numChildrenLoop; loopIndex++) {

                    status = EG_getTopology(childrenLoop[loopIndex],
                                            &geomRef, &oclass, &mtype, data,
                                            &numChildrenEdge, &childrenEdge, &senses);
                    if (status != EGADS_SUCCESS) goto cleanup;

                    for (edgeIndex = 0; edgeIndex < numChildrenEdge; edgeIndex++) {

                        if (nodeOrEdge > 0)  { // Edge point

                            if (bodyEdge[edgeTopoIndex -1] != childrenEdge[edgeIndex]) continue;

                        } else { // Node

                            status = EG_getTopology(childrenEdge[edgeIndex],
                                                    &geomRef, &oclass, &mtype, data,
                                                    &numChildrenNode, &childrenNode, &dummySenses);
                            if (status != EGADS_SUCCESS) goto cleanup;

                            pointLocalIndex = -1;

                            status = EG_indexBodyTopo(body, childrenEdge[edgeIndex]);
                            if (status < EGADS_SUCCESS) goto cleanup;

                            edgeTopoIndex = status;

                            if (numChildrenNode == 1 || numChildrenNode == 2) {

                                status = EG_getTessEdge(mesh->bodyTessMap.egadsTess, edgeTopoIndex, &len, &xyz, &t);
                                if (status != EGADS_SUCCESS) goto cleanup;

                                if (bodyNode[pointTopoIndex -1] == childrenNode[0]) pointLocalIndex = 1;

                                if (numChildrenNode > 1) {
                                    if (bodyNode[pointTopoIndex -1] == childrenNode[1]) pointLocalIndex = len;
                                }

                            } else {
                                printf("Warning: Number of nodes = %d  for edge index %d\n", numChildrenNode, edgeIndex);
                                continue;
                            }

                            if (pointLocalIndex < 0) continue;
                        }

//                        printf("J = %d, egdeTopoIndex %d, pointLocalIndex %d\n", j+1, edgeTopoIndex, pointLocalIndex);

                        if (pointLocalIndex < 0) {
                            printf("Unable to determine pointLocalIndex\n");
                            status = CAPS_NOTFOUND;
                            goto cleanup;
                        }

                        // Get t - along edge
                        status = EG_getTessEdge(mesh->bodyTessMap.egadsTess, edgeTopoIndex, &len, &xyz, &t);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        params[0] = t[pointLocalIndex-1];
                        //status = EG_invEvaluate(bodyEdge[pointLocalIndex-1], mesh->node[k].xyz,
                        //      params, data);
                        //if (status != EGADS_SUCCESS) goto cleanup;

                        // Check to see if edge is part of a co-planar face
                        status = check_edgeInCoplanarFace(bodyEdge[edgeTopoIndex-1], params[0], faceIndex, body, &coplanarFlag);
                        if (status != CAPS_SUCCESS) goto cleanup;

                        if (coplanarFlag == (int) true) continue;

                        // Get derivative along edge
                        status = EG_evaluate(bodyEdge[edgeTopoIndex-1], params, data);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        normEdge[0] = data[3];
                        normEdge[1] = data[4];
                        normEdge[2] = data[5];

                        if (senses[edgeIndex]  < 0) {
                            normEdge[0] *= -1;
                            normEdge[1] *= -1;
                            normEdge[2] *= -1;
                        }

                        /*
                                                printf("Edge normal = %f %f %f, MTypeFace %d, %d %d\n", normEdge[0], normEdge[1], normEdge[2],
                                                                                                                                                                mtypeFace, SFORWARD, SREVERSE);
                         */

                        // Get uv on face at edge - need to check sense setting currently 0; in general the co-planar check
                        //  should catch this I think so it shouldn't be an issue
                        status = EG_getEdgeUV(bodyFace[faceIndex], bodyEdge[edgeTopoIndex-1], 0, params[0], data);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        params[0] = data[0];
                        params[1] = data[2];

                        // Get derivative along face
                        status = EG_evaluate(bodyFace[faceIndex], params, data);
                        if (status != EGADS_SUCCESS) goto cleanup;

                        // Get face normal
                        (void) cross_DoubleVal(&data[3], &data[6], normFace);

                        // Test face orientation
                        if (mtypeFace == SREVERSE) {
                            normFace[0] *= -1;
                            normFace[1] *= -1;
                            normFace[2] *= -1;
                        }

                        // Get normal boundary
                        (void) cross_DoubleVal(normEdge, normFace, normBoundary);

                        mag = dot_DoubleVal(normBoundary, normBoundary);
                        mag = sqrt(mag);

                        normBoundary[0] = normBoundary[0]/mag;
                        normBoundary[1] = normBoundary[1]/mag;
                        normBoundary[2] = normBoundary[2]/mag;

                        /* Want to repeat normals - for now
                        if (nodeOrEdge == 0) {
                            status = check_nodeNormalHist(normBoundary, &numNormalHist, &normalHist, &normalExistFlag);
                            if (status != CAPS_SUCCESS) goto cleanup;

                            if (normalExistFlag == (int) true) continue;
                        }
                         */

                        printf(">>> Writing SNORM card\n");
                        status = astros_writeSNORMCard(fp, feaMesh->node[k+nodeOffSet], normBoundary, edgeIndex+1, 1, feaFileFormat);
                        if (status != CAPS_SUCCESS) goto cleanup;

                        printf(">>> Getting SNORMDT information\n");
                        status = astros_getConfigurationSens(fp,
                                                             aimInfo,
                                                             numDesignVariable,
                                                             feaDesignVariable,
                                                             feaFileFormat,
                                                             numGeomIn,
                                                             geomInVal,
                                                             mesh->bodyTessMap.egadsTess, -1, edgeTopoIndex, pointLocalIndex,
                                                             normBoundary, faceIndex+1, feaMesh->node[k+nodeOffSet]);
                        printf(">>> Done with SNORMDT information\n");
                        if (status != CAPS_SUCCESS) goto cleanup;


                    } // Children edge loop

                } // Children loop loop

            } // Face loop

        } // Point loop

        nodeOffSet += mesh->numNode;

        mesh = NULL;

    } // Mesh loop

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in astros_getBoundaryNormal, status %d\n", status);

        if (bodyNode != NULL) EG_free(bodyNode);
        if (bodyEdge != NULL) EG_free(bodyEdge);
        if (bodyFace != NULL) EG_free(bodyFace);

        //if (normalHist != NULL) EG_free(normalHist);

        return status;

}

// Write geometric parametrization - only valid for modifications made by Bob Canfield to Astros
int astros_writeGeomParametrization(FILE *fp,
                                    void *aimInfo,
                                    int numDesignVariable,
                                    feaDesignVariableStruct *feaDesignVariable,
                                    int numGeomIn,
                                    capsValue *geomInVal,
                                    meshStruct *feaMesh,
                                    feaFileFormatStruct *feaFileFormat) {
    int status; // Function return status

    int i, j, k, m; // row, col; // Indexing

    int numMesh;
    meshStruct *mesh = NULL;

    int nodeOffSet = 0; //Keep track of global node indexing offset due to combining multiple meshes

    const char *geomInName;
    int numPoint;
    double *xyz = NULL;

    if (fp                == NULL)  return CAPS_NULLVALUE;
    if (aimInfo           == NULL ) return CAPS_NULLVALUE;
    if (feaDesignVariable == NULL ) return CAPS_NULLVALUE;
    if (geomInVal         == NULL ) return CAPS_NULLVALUE;
    if (feaMesh           == NULL ) return CAPS_NULLVALUE;
    if (feaFileFormat     == NULL ) return CAPS_NULLVALUE;

    // Are we dealing with a single mesh or a combined mesh
    if (feaMesh->bodyTessMap.egadsTess != NULL) {
        numMesh = 1;
        mesh = feaMesh;

    } else {
        //printf("Using reference meshes\n");
        numMesh = feaMesh->numReferenceMesh;
    }

    if (numMesh == 0) {
        printf("No bodies with tessellations found!\n");
        status = CAPS_BADVALUE;
        goto cleanup;
    }

    // Write sensitivity files for each  mesh
    for (i = 0; i < numMesh; i++ ) {
        printf(">>> Parametrization on mesh %d\n", i);
        if (mesh == NULL) mesh = &feaMesh->referenceMesh[i];

        for (j = 0; j < numDesignVariable; j++) {

            for (k = 0; k < numGeomIn; k++) {

                status = aim_getName(aimInfo, k+1, GEOMETRYIN, &geomInName);
                if (status != CAPS_SUCCESS) goto cleanup;

                if (strcmp(feaDesignVariable[j].name, geomInName) == 0) break;
            }

            // If name isn't found in Geometry inputs skip design variables
            if (k >= numGeomIn) continue;

            if(aim_getGeomInType(aimInfo, k+1) == EGADS_OUTSIDE) {
                printf("Error: Geometric sensitivity not available for CFGPMTR = %s\n", geomInName);
                status = CAPS_NOSENSITVTY;
                goto cleanup;
            }

            printf("Geometric sensitivity name = %s\n", geomInName);

            if (xyz != NULL) EG_free(xyz);
            xyz = NULL;

            if (geomInVal[k].length == 1) {
                printf(">>> Getting sensitivity\n");
                status = aim_sensitivity(aimInfo,
                        geomInName,
                        1, 1,
                        mesh->bodyTessMap.egadsTess,
                        &numPoint, &xyz);
                printf(">>> Back from getting sensitivity\n");
                if (status == CAPS_NOTFOUND) {

                    numPoint = mesh->numNode;
                    xyz = (double *) EG_reall(xyz, 3*numPoint*sizeof(double));
                    if (xyz == NULL) {
                        status = EGADS_MALLOC;
                        goto cleanup;
                    }

                    for (m = 0; m < 3*numPoint; m++) xyz[m] = 0.0;

                    printf("Warning: Sensitivity not found for %s, defaulting to 0.0s\n", geomInName);

                } else if (status != CAPS_SUCCESS) {

                    goto cleanup;

                }

                if (numPoint != mesh->numNode) {
                    printf("Error: the number of nodes returned by aim_senitivity does NOT match the surface mesh!\n");
                    status = CAPS_MISMATCH;
                    goto cleanup;
                }

                for (m = 0; m < mesh->numNode; m++) {

                    if (mesh->node[m].nodeID != m+1) {
                        printf("Error: Node Id %d is out of order (%d). No current fix!\n", mesh->node[m].nodeID, m+1);
                        status = CAPS_MISMATCH;
                        goto cleanup;
                    }
                    printf(">>> Write DVGRID cards\n");
                    status = astros_writeDVGRIDCard(fp,
                            feaDesignVariable[j].designVariableID,
                            feaMesh->node[m+nodeOffSet],
                            1.0,
                            &xyz[3*m + 0], feaFileFormat);
                    if (status != CAPS_SUCCESS) goto cleanup;
                }

            } else {

                printf("Warning: Can NOT write DVGRID cards for multidimensional design variables!\n");
                continue;

                /*
                                for (row = 0; row < geomInVal[k].nrow; row++) {
                                        for (col = 0; col < geomInVal[k].ncol; col++) {

                                                if (xyz != NULL) EG_free(xyz);
                                                xyz = NULL;

                                                status = aim_sensitivity(aimInfo,
                                                                                                 geomInName,
                                                                                                 row+1, col+1, // row, col
                                                                                                 mesh->bodyTessMap.egadsTess,
                                                                                                 &numPoint, &xyz);
                                                if (status == CAPS_NOTFOUND) {
                                                        numPoint = mesh->numNode;
                                                        xyz = (double *) EG_reall(xyz, 3*numPoint*sizeof(double));
                                                        if (xyz == NULL) {
                                                                status = EGADS_MALLOC;
                                                                goto cleanup;
                                                        }

                                                        for (m = 0; m < 3*numPoint; m++) xyz[m] = 0.0;

                                                        printf("Warning: Sensitivity not found %s, defaulting to 0.0s\n", geomInName);

                                                } else if (status != CAPS_SUCCESS) {

                                                        goto cleanup;

                                                }

                                                if (numPoint != mesh->numNode) {
                                                        printf("Error: the number of nodes returned by aim_senitivity does NOT match the surface mesh!\n");
                                                        status = CAPS_MISMATCH;
                                                        goto cleanup;
                                                }

                                                for (m = 0; m < mesh->numNode; m++) {

                                                        if (mesh->node[m].nodeID != m+1) {
                                                                printf("Error: Node Id %d is out of order (%d). No current fix!\n", mesh->node[m].nodeID, m+1);
                                                                status = CAPS_MISMATCH;
                                                                goto cleanup;
                                                        }

                                                        status = astros_writeDVGRIDCard(fp,
                                                                                                                        feaDesignVariable[j].designVariableID,
                                                                                                                        feaMesh->node[m+nodeOffSet],
                                                                                                                        1.0,
                                                                                                                        &xyz[3*m + 0], feaFileFormat);
                                                        if (status != CAPS_SUCCESS) goto cleanup;
                                                }
                                        } // Col
                                } // Row
                 */
            } // Size of geometry
        } // Design variables

        nodeOffSet += mesh->numNode;

        mesh = NULL;

    } // Mesh
    printf(">>> Done with DVGRID cards\n");

    // Write out SNorm card
    printf(">>> Getting SNORM data\n");
    status = astros_getBoundaryNormal(fp,
            aimInfo,
            numDesignVariable,
            feaDesignVariable,
            numGeomIn,
            geomInVal,
            feaMesh, feaFileFormat);
    printf(">>> Done with SNORM data\n");
    if (status != CAPS_SUCCESS) goto cleanup;

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in astros_writeGeomParametrization, status %d\n", status);

        if (xyz != NULL) EG_free(xyz);

        return status;

}
