#include <string.h>
#include <math.h>

#include "feaTypes.h"  // Bring in FEA structures
#include "capsTypes.h" // Bring in CAPS types

#include "miscUtils.h" //Bring in misc. utility functions
#include "nastranUtils.h" // Bring in nastran utility header

#ifdef WIN32
#define strcasecmp  stricmp
#endif

// Write out FLFact Card.
int nastran_writeFLFactCard(FILE *fp, feaFileFormatStruct *feaFileFormat,
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

// Write a Nastran element cards not supported by mesh_writeNastran in meshUtils.c
int nastran_writeSubElementCard(FILE *fp, meshStruct *feaMesh, int numProperty, feaPropertyStruct *feaProperty, feaFileFormatStruct *feaFileFormat) {

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

            // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }

            /* No continuation lines in CONM2 Input
			// Continuation line
			if (feaFileFormat->fileType == FreeField) {
				fprintf(fp, ", +C");
			} else {
				fprintf(fp, "+C%6s", "");
			}
             */

            fprintf(fp, "\n");

            // Continuation line
            if (feaFileFormat->fileType == FreeField) {
                //fprintf(fp, "+C");
            } else {
                //fprintf(fp, "+C%6s", "");
                fprintf(fp, "%8s", "");
            }

            // I11
            tempString = convert_doubleToString( feaProperty[j].massInertia[0], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I21
            tempString = convert_doubleToString( feaProperty[j].massInertia[1], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I22
            tempString = convert_doubleToString( feaProperty[j].massInertia[2], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I31
            tempString = convert_doubleToString( feaProperty[j].massInertia[3], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I32
            tempString = convert_doubleToString( feaProperty[j].massInertia[4], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            // I33
            tempString = convert_doubleToString( feaProperty[j].massInertia[5], fieldWidth, 1);
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

// Write a Nastran connections card from a feaConnection structure
int nastran_writeConnectionCard(FILE *fp, feaConnectionStruct *feaConnect, feaFileFormatStruct *feaFileFormat) {

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

// Write a Nastran AERO card from a feaAeroRef structure
int nastran_writeAEROCard(FILE *fp, feaAeroRefStruct *feaAeroRef, feaFileFormatStruct *feaFileFormat) {

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

    // Blank space
    if (feaFileFormat->fileType == FreeField) {
        fprintf(fp, ", ");
    } else {
        fprintf(fp, " %7s", "");
    }

    tempString = convert_doubleToString( feaAeroRef->refChord, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // REFC
    EG_free(tempString);

    tempString = convert_doubleToString( 1.0, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // RHOREF
    EG_free(tempString);

    fprintf(fp,"\n");

    return CAPS_SUCCESS;
}

// Write a Nastran AEROS card from a feaAeroRef structure
int nastran_writeAEROSCard(FILE *fp, feaAeroRefStruct *feaAeroRef, feaFileFormatStruct *feaFileFormat) {

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

    tempString = convert_integerToString( feaAeroRef->symmetryXZ, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // SYMXZ
    EG_free(tempString);

    tempString = convert_integerToString( feaAeroRef->symmetryXY, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString); // SYMXY
    EG_free(tempString);

    fprintf(fp,"\n");

    return CAPS_SUCCESS;
}

// Write Nastran SET1 card from a feaAeroStruct
int nastran_writeSet1Card(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing

    char *delimiter = NULL;

    int sidIndex= 0, lineCount = 0; // Counters

    if (fp == NULL) return CAPS_IOERR;
    if (feaAero == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    if (feaAero->numGridID != 0) fprintf(fp,"%-8s%s%7d", "SET1", delimiter , feaAero->surfaceID);

    lineCount = 1;
    for (i = 0; i <feaAero->numGridID; i++) {
        sidIndex += 1;
        if (sidIndex % (8*lineCount) == 0) {

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ",+C%-5d\n", lineCount-1); // Start of continuation
                fprintf(fp, "+C%-5d,"  , lineCount-1);  // End of continuation
            } else {
                fprintf(fp, "+C%-6d\n", lineCount-1); // Start of continuation
                fprintf(fp, "+C%-6d"  , lineCount-1);  // End of continuation
            }

            lineCount += 1;
        }

        fprintf(fp,"%s%7d",delimiter, feaAero->gridIDSet[i]);
    }

    if (feaAero->numGridID != 0) fprintf(fp,"\n\n");

    return CAPS_SUCCESS;
}

// Write Nastran Spline1 cards from a feaAeroStruct
int nastran_writeAeroSplineCard(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat) {

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

    idString = convert_integerToString( feaAero->surfaceID, 7, 1);

    fprintf(fp,"%-8s", "SPLINE1");
    fprintf(fp, "%s%s",delimiter, idString); // EID
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

//    tempString = convert_integerToString( feaAero->surfaceID +
//                                         (feaAero->vlmSurface.Nspan-1)*(feaAero->vlmSurface.Nchord-1) - 1,
//                                         fieldWidth, 1);

    fprintf(fp, "%s%s",delimiter, tempString); // Box 2
    EG_free(tempString);


    fprintf(fp, "%s%s",delimiter, idString); // SetG

    fprintf(fp, "\n");

    if (idString != NULL) EG_free(idString);

    return CAPS_SUCCESS;
}

// Write Nastran CAERO1 cards from a feaAeroStruct
int nastran_writeCAeroCard(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat) {

    int i, j, section; // Indexing

    double chordLength = 0, chordVec[3];
    int  fieldWidth;
    char *tempString = NULL, *delimiter = NULL;

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

    //printf("NUMBER OF SECTIONS = %d\n", feaAero->vlmSurface.numSection);
    for (i = 0; i < feaAero->vlmSurface.numSection-1; i++) {

        tempString = convert_integerToString( feaAero->surfaceID, 7, 1);

        fprintf(fp,"\n");

        // Write necessary PAER0 card
        fprintf(fp,"%-8s", "PAERO1");
        fprintf(fp, "%s%s",delimiter, tempString);
        fprintf(fp, "\n");

        // Start of CAERO1 entry
        fprintf(fp,"%-8s", "CAERO1");

        fprintf(fp, "%s%s",delimiter, tempString); // Element ID
        fprintf(fp, "%s%s",delimiter, tempString); // PAERO  ID
        EG_free(tempString);

        tempString = convert_integerToString( feaAero->coordSystemID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // If Cspace and/or Sspace is something (to be defined) lets write a AEFact card instead with our own distributions
        if (feaAero->vlmSurface.Sspace == 0.0) {
            tempString = convert_integerToString( feaAero->vlmSurface.NspanTotal, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else {
            tempString = convert_integerToString( 0, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        }

        if (feaAero->vlmSurface.Cspace == 0.0) {
            tempString = convert_integerToString( feaAero->vlmSurface.Nchord, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else {
            tempString = convert_integerToString( 0, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        }

        if (feaAero->vlmSurface.Sspace != 0.0) {
            printf("Not implemented yet!\n");
        } else {
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
            //tempString = convert_integerToString( 0, fieldWidth, 1);
            //fprintf(fp, "%s%s",delimiter, tempString);
            //EG_free(tempString);
        }

        if (feaAero->vlmSurface.Cspace != 0.0) {
            printf("Not implemented yet!\n");
        } else {
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
            //tempString = convert_integerToString( 0, fieldWidth, 1);
            //fprintf(fp, "%s%s",delimiter, tempString);
            //EG_free(tempString);
        }

        // IGID - default to 1.... for now
        tempString = convert_integerToString( 1, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", +C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        fprintf(fp, "\n");

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, "+C");
        } else {
            fprintf(fp, "+C%6s", "");
        }


        for (j = i; j < (2+i); j++) {
            section = feaAero->vlmSurface.vlmSection[j].sectionIndex;

            tempString = convert_doubleToString( feaAero->vlmSurface.vlmSection[section].xyzLE[0], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaAero->vlmSurface.vlmSection[section].xyzLE[1], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString( feaAero->vlmSurface.vlmSection[section].xyzLE[2], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            chordVec[0] = feaAero->vlmSurface.vlmSection[section].xyzTE[0] - feaAero->vlmSurface.vlmSection[section].xyzLE[0];
            chordVec[1] = feaAero->vlmSurface.vlmSection[section].xyzTE[1] - feaAero->vlmSurface.vlmSection[section].xyzLE[1];
            chordVec[2] = feaAero->vlmSurface.vlmSection[section].xyzTE[2] - feaAero->vlmSurface.vlmSection[section].xyzLE[2];
            chordLength = sqrt(pow(chordVec[0], 2) +
                               pow(chordVec[1], 2) +
                               pow(chordVec[2], 2));

            tempString = convert_doubleToString( chordLength, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

        }

        fprintf(fp, "\n");
    }

    return CAPS_SUCCESS;
}

// Write Nastran coordinate system card from a feaCoordSystemStruct structure
int nastran_writeCoordinateSystemCard(FILE *fp, feaCoordSystemStruct *feaCoordSystem, feaFileFormatStruct *feaFileFormat) {

    int  fieldWidth;
    char *tempString, *delimiter;

    double pointA[3] = {0, 0, 0};
    double pointB[3] = {0, 0, 0};
    double pointC[3] = {0, 0, 0};

    if (fp == NULL) return CAPS_IOERR;
    if (feaCoordSystem == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    pointA[0] = feaCoordSystem->origin[0];
    pointA[1] = feaCoordSystem->origin[1];
    pointA[2] = feaCoordSystem->origin[2];

    pointB[0] = feaCoordSystem->normal3[0] + pointA[0];
    pointB[1] = feaCoordSystem->normal3[1] + pointA[1];
    pointB[2] = feaCoordSystem->normal3[2] + pointA[2];

    pointC[0] = feaCoordSystem->normal1[0] + pointB[0];
    pointC[1] = feaCoordSystem->normal1[1] + pointB[1];
    pointC[2] = feaCoordSystem->normal1[2] + pointB[2];

    // Rectangular
    if (feaCoordSystem->coordSystemType == RectangularCoordSystem) fprintf(fp,"%-8s", "CORD2R");

    // Spherical
    if (feaCoordSystem->coordSystemType == SphericalCoordSystem)  fprintf(fp,"%-8s", "CORD2S");

    // Cylindrical
    if (feaCoordSystem->coordSystemType == CylindricalCoordSystem) fprintf(fp,"%-8s", "CORD2C");

    if (feaCoordSystem->coordSystemType == RectangularCoordSystem ||
        feaCoordSystem->coordSystemType == SphericalCoordSystem   ||
        feaCoordSystem->coordSystemType == CylindricalCoordSystem) {

        tempString = convert_integerToString( feaCoordSystem->coordSystemID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString( feaCoordSystem->refCoordSystemID, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointA[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointA[1], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointA[2], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointB[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointB[1], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointB[2], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ",+C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        fprintf(fp, "\n");

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, "+C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        tempString = convert_doubleToString( pointC[0], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointC[1], fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( pointC[2], fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

    } else {

        printf("Unrecognized coordinate system type !!\n");
        return CAPS_BADVALUE;
    }

    return CAPS_SUCCESS;
}

// Write combined Nastran constraint card from a set of constraint IDs.
// 	The combined constraint ID is set through the constraintID variable.
int nastran_writeConstraintADDCard(FILE *fp, int constraintID, int numSetID, int constraintSetID[], feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing

    int sidIndex= 0, lineCount = 0; // Counters

    char *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    if (numSetID != 0) fprintf(fp,"%-8s%s%7d", "SPCADD",delimiter , constraintID);

    lineCount = 1;
    for (i = 0; i < numSetID; i++) {
        sidIndex += 1;
        if (sidIndex % (8*lineCount) == 0) {

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ",+C%-5d\n", lineCount-1); // Start of continuation
                fprintf(fp, "+C%-5d,"  , lineCount-1);  // End of continuation
            } else {
                fprintf(fp, "+C%-6d\n", lineCount-1); // Start of continuation
                fprintf(fp, "+C%-6d"  , lineCount-1);  // End of continuation
            }

            lineCount += 1;
        }

        fprintf(fp,"%s%7d",delimiter, constraintSetID[i]);
    }

    if (numSetID != 0) fprintf(fp,"\n$\n");

    return CAPS_SUCCESS;
}

// Write Nastran constraint card from a feaConstraint structure
int nastran_writeConstraintCard(FILE *fp, feaConstraintStruct *feaConstraint, feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing;
    char *tempString, *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaConstraint == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    if (feaConstraint->constraintType == Displacement) {

        for (i = 0; i < feaConstraint->numGridID; i++) {
            fprintf(fp,"%-8s", "SPC");

            tempString = convert_integerToString(feaConstraint->constraintID, 7, 1);
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

            tempString = convert_integerToString(feaConstraint->constraintID , 7, 1);
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

// Write Nastran support card from a feaSupport structure - withID = NULL or false SUPORT, if withID = true SUPORT1
int nastran_writeSupportCard(FILE *fp, feaSupportStruct *feaSupport, feaFileFormatStruct *feaFileFormat, int *withID) {

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

        if (withID != NULL && *withID == (int) true) {
            fprintf(fp,"%-8s", "SUPORT1");

            tempString = convert_integerToString(feaSupport->supportID, 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

        } else {
            fprintf(fp,"%-8s", "SUPORT");
        }

        tempString = convert_integerToString(feaSupport->gridIDSet[i], 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaSupport->dofSupport, 7, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    return CAPS_SUCCESS;
}

// Write a Nastran Material card from a feaMaterial structure
int nastran_writeMaterialCard(FILE *fp, feaMaterialStruct *feaMaterial, feaFileFormatStruct *feaFileFormat) {

    int  fieldWidth;
    char *tempString, *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaMaterial == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    // Isotropic
    if (feaMaterial->materialType == Isotropic) {

        /*
        fprintf(fp,"%-8s %s %s %s %s %s\n", "MAT1",
                                            convert_integerToString(feaMaterial->materialID  , 7, 1),
                                            convert_doubleToString( feaMaterial->youngModulus, 7, 1),
                                            convert_doubleToString( feaMaterial->shearModulus, 7, 1),
                                            convert_doubleToString( feaMaterial->poissonRatio, 7, 1),
                                            convert_doubleToString( feaMaterial->density     , 7, 1));
         */

        fprintf(fp,"%-8s", "MAT1");

        tempString = convert_integerToString(feaMaterial->materialID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->youngModulus, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->shearModulus, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->poissonRatio, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->density, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->thermalExpCoeff, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->temperatureRef, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->dampingCoeff, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        /*

        // Continuation line
		if (feaFileFormat->fileType == FreeField) {
			fprintf(fp, ",+C");
		} else {
			fprintf(fp, "+C%6s", "");
		}

		fprintf(fp, "\n");

		// Continuation line
		if (feaFileFormat->fileType == FreeField) {
			fprintf(fp, "+C");
		} else {
			fprintf(fp, "+C%6s", "");
		}

		tempString = convert_doubleToString( feaMaterial->tensionAllow, fieldWidth, 1);
		fprintf(fp, "%s%s",delimiter, tempString);
		EG_free(tempString);

		tempString = convert_doubleToString( feaMaterial->compressAllow, fieldWidth, 1);
		fprintf(fp, "%s%s",delimiter, tempString);
		EG_free(tempString);

		tempString = convert_doubleToString( feaMaterial->shearAllow, fieldWidth, 1);
		fprintf(fp, "%s%s",delimiter, tempString);
		EG_free(tempString);

         */

        fprintf(fp, "\n");
    }

    // Orthotropic
    if (feaMaterial->materialType == Orthotropic) {

        fprintf(fp,"%-8s", "MAT8");

        tempString = convert_integerToString(feaMaterial->materialID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->youngModulus, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->youngModulusLateral, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->poissonRatio, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->shearModulus, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        if (feaMaterial->shearModulusTrans1Z != 0) {
            tempString = convert_doubleToString( feaMaterial->shearModulusTrans1Z, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        if (feaMaterial->shearModulusTrans2Z != 0) {
            tempString = convert_doubleToString( feaMaterial->shearModulusTrans2Z, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        tempString = convert_doubleToString( feaMaterial->density, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", +C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        fprintf(fp, "\n");

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, "+C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        tempString = convert_doubleToString( feaMaterial->thermalExpCoeff, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->thermalExpCoeffLateral, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString( feaMaterial->temperatureRef, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        if (feaMaterial->tensionAllow != 0.0) {
            tempString = convert_doubleToString( feaMaterial->tensionAllow, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        if (feaMaterial->compressAllow != 0.0) {
            tempString = convert_doubleToString( feaMaterial->compressAllow, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        if (feaMaterial->tensionAllowLateral != 0.0) {
            tempString = convert_doubleToString( feaMaterial->tensionAllowLateral, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        if (feaMaterial->compressAllowLateral != 0.0) {
            tempString = convert_doubleToString( feaMaterial->compressAllowLateral, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        if (feaMaterial-> shearAllow != 0.0) {
            tempString = convert_doubleToString( feaMaterial->shearAllow, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else { // Blank space
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", +C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        fprintf(fp, "\n");

        // Continuation line
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, "+C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        tempString = convert_doubleToString( feaMaterial->dampingCoeff, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%7s",delimiter, "");

        if (feaMaterial->allowType == 0) {
            fprintf(fp, "\n");
        } else {
            fprintf(fp, "%s1.0\n",delimiter);
        }
    }

    return CAPS_SUCCESS;
}

// Write a Nastran Property card from a feaProperty structure
int nastran_writePropertyCard(FILE *fp, feaPropertyStruct *feaProperty, feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing

    int  fieldWidth;
    char *tempString=NULL;
    char *delimiter;

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
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

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
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

    }

    //          2D Elements        //

    // Shell
    if (feaProperty->propertyType == Shell) {

        fprintf(fp,"%-8s", "PSHELL");

        // Property ID
        tempString = convert_integerToString(feaProperty->propertyID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Material ID
        tempString = convert_integerToString(feaProperty->materialID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Membrane thickness
        tempString = convert_doubleToString(feaProperty->membraneThickness, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        if (feaProperty->materialBendingID != 0) {
            tempString = convert_integerToString(feaProperty->materialBendingID, 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaProperty->bendingInertiaRatio, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

        } else {// Print a blank
            if (feaFileFormat->fileType == FreeField) {
                if (feaProperty->materialShearID != 0 || feaProperty->massPerArea != 0) {
                    fprintf(fp, ", , ");
                }
            } else {
                fprintf(fp, " %7s %7s", "", "");
            }

            /*// Print a blank - ATTENTION - THIS LOGIC DIDN't LOOK RIGHT
			if (feaProperty->materialShearID != 0 | feaFileFormat->fileType == FreeField) {
				if (feaProperty->massPerArea != 0) {
					fprintf(fp, ", , ");
				}
			} else {
				fprintf(fp, " %7s  %7s", "", "");
			}*/
        }

        if (feaProperty->materialShearID != 0) {
            tempString = convert_integerToString(feaProperty->materialShearID, 7, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaProperty->shearMembraneRatio, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        } else {// Print a blank
            if (feaFileFormat->fileType == FreeField) {
                if (feaProperty->massPerArea != 0) {
                    fprintf(fp, ", , ");
                }
            } else {
                fprintf(fp, " %7s %7s", "", "");
            }
        }

        if (feaProperty->massPerArea != 0){
            tempString = convert_doubleToString(feaProperty->massPerArea, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        }

        fprintf(fp, "\n");
    }

    // Shear
    if (feaProperty->propertyType == Shear) {

        fprintf(fp,"%-8s", "PSHEAR");

        // Property ID
        tempString = convert_integerToString(feaProperty->propertyID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Material ID
        tempString = convert_integerToString(feaProperty->materialID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // Membrane thickness
        tempString = convert_doubleToString(feaProperty->membraneThickness, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        // NSM
        if (feaProperty->massPerArea != 0){
            tempString = convert_doubleToString(feaProperty->massPerArea, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);
        }

        fprintf(fp, "\n");
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
        if (feaProperty->compositeFailureTheory != NULL) {
            fprintf(fp, "%s%7s",delimiter, feaProperty->compositeFailureTheory);
        } else {
            fprintf(fp, "%s%7s",delimiter, "");
        }

        // BLANK FIELD TREF
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else {
            fprintf(fp, " %7s", "");
        }

        // BLANK FIELD DAMPING
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ", ");
        } else {
            fprintf(fp, " %7s", "");
        }

        // LAM
        if (feaProperty->compositeSymmetricLaminate == (int) true){
            fprintf(fp, "%s%7s",delimiter, "SYM");
        } else {// Print a blank
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ", ");
            } else {
                fprintf(fp, " %7s", "");
            }
        }

        // CONTINUATION LINE
        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp, ",+C");
        } else {
            fprintf(fp, "+C%6s", "");
        }

        fprintf(fp, "\n");

        // LOOP OVER PLYS
        for (i = 0; i < feaProperty->numPly; i++) {

            if ( (double)i/2.0 == (double) (i/2) ) {
                // CONINUATION LINE
                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, "+C");
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

            if ( (double)i/2.0 != (double) (i/2) ) {
                // CONINUATION LINE
                if (i < feaProperty->numPly-1) {
                    if (feaFileFormat->fileType == FreeField) {
                        fprintf(fp, ",+C");
                    } else {
                        fprintf(fp, "+C%6s", "");
                    }
                }
                fprintf(fp, "\n");
            }

            if (i == feaProperty->numPly-1){
                fprintf(fp, "\n");
            }

        }
    }

    //          3D Elements        //

    // Solid
    if (feaProperty->propertyType == Solid) {

        fprintf(fp,"%-8s", "PSOLID");

        tempString = convert_integerToString(feaProperty->propertyID, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_integerToString(feaProperty->materialID, 7, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);
    }

    return CAPS_SUCCESS;
}

// Write a combined Nastran load card from a set of load IDs. Uses the feaLoad structure to get the local scale factor
// 	for the load. The overall load scale factor is set to 1. The combined load ID is set through the loadID variable.
int nastran_writeLoadADDCard(FILE *fp, int loadID, int numSetID, int loadSetID[], feaLoadStruct feaLoad[], feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing

    int sidIndex= 0, lineCount = 0; // Counters

    double overallScale = 1.0;

    char *tempString, *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (numSetID > 0 && feaLoad == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    if (numSetID != 0) {
        fprintf(fp,"%-8s%s%7d", "LOAD", delimiter, loadID);

        tempString = convert_doubleToString(overallScale, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);
    }

    lineCount = 1;
    for (i = 0; i < numSetID; i++) {
        sidIndex += 2;
        if (sidIndex % (8*lineCount) == 0) {

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ",+L%-5d\n", lineCount-1); // Start of continuation
                fprintf(fp, "+L%-5d,"  , lineCount-1);  // End of continuation
            } else {
                fprintf(fp, "+L%-6d\n", lineCount-1);  // Start of continuation
                fprintf(fp, "+L%-6d"  , lineCount-1);  // End of continuation
            }

            lineCount += 1;
        }

        // ID's are 1 bias
        tempString = convert_doubleToString(feaLoad[loadSetID[i]-1].loadScaleFactor, 7, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        fprintf(fp,"%s%7d",delimiter, loadSetID[i]);
    }

    if (numSetID != 0) fprintf(fp,"\n$\n");

    return CAPS_SUCCESS;
}

// Write a Nastran Load card from a feaLoad structure
int nastran_writeLoadCard(FILE *fp, feaLoadStruct *feaLoad, feaFileFormatStruct *feaFileFormat)
{
    int i, j; // Indexing
    int fieldWidth;
    char *tempString=NULL;
    char *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaLoad == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

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

// Write a Nastran Analysis card from a feaAnalysis structure
int nastran_writeAnalysisCard(FILE *fp, feaAnalysisStruct *feaAnalysis, feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing
    int numVel = 23;
    double velocity, dv, vmin, vmax, velocityArray[23];

    int sidIndex= 0, lineCount = 0; // Counters

    int fieldWidth, status;

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
    if (feaAnalysis->analysisType == Modal || feaAnalysis->analysisType == AeroelasticFlutter) {

        if (feaAnalysis->extractionMethod != NULL &&
            strcasecmp(feaAnalysis->extractionMethod, "Lanczos") == 0) {

            fprintf(fp,"%-8s", "EIGRL");

            tempString = convert_integerToString(feaAnalysis->analysisID  , fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaAnalysis->frequencyRange[0], fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaAnalysis->frequencyRange[1], fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaAnalysis->numDesiredEigenvalue, fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            // fprintf(fp, "%s%7s%s%7s%s%-7s", delimiter, "", delimiter, "", delimiter, "");

            if (feaAnalysis->eigenNormaliztion != NULL) {

                fprintf(fp, "%s%7s%s%7s%s%-7s", delimiter, "", delimiter, "", delimiter, "");

                fprintf(fp, "%s%7s\n", delimiter, feaAnalysis->eigenNormaliztion);
                //fprintf(fp, "%s%7s", delimiter, feaAnalysis->eigenNormaliztion);
            } else {
                fprintf(fp,"\n");
                //fprintf(fp,"%s%7s", delimiter, ""); // Print blank space
            }

            /*
			fprintf(fp, "%s%-7s\n", delimiter, "+E1");

			fprintf(fp, "%-8s", "+E1");

			tempString = convert_integerToString(feaAnalysis->lanczosMode, 7, 1);
			fprintf(fp, "%s%s", delimiter, tempString);
			EG_free(tempString);

			if (feaAnalysis->lanczosType != NULL) {
				fprintf(fp, "%s%7s\n", delimiter, feaAnalysis->lanczosType);
			} else {
				fprintf(fp,"\n");
				//fprintf(fp, "%s%7s\n", delimiter, ""); // Print blank space
			}
             */

        } else {

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
    }

    if (feaAnalysis->analysisType == AeroelasticTrim) {

        fprintf(fp,"%-8s", "TRIM");

        tempString = convert_integerToString(feaAnalysis->analysisID, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        if (feaAnalysis->machNumber != NULL && feaAnalysis->numMachNumber > 0) {
            tempString = convert_doubleToString(feaAnalysis->machNumber[0], fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);
        } else {
            fprintf(fp, "%s%7s", delimiter, " ");
        }

        tempString = convert_doubleToString(feaAnalysis->dynamicPressure, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        lineCount = 1;
        sidIndex = 4;
        for (i = 0; i < feaAnalysis->numRigidConstraint; i++) {


            if (sidIndex % (8*lineCount) == 0) {

                if (lineCount == 1) fprintf(fp,"%8s", " ");

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ",+L%-5d\n", lineCount-1); // Start of continuation
                    fprintf(fp, "+L%-5d,"  , lineCount-1);  // End of continuation
                } else {
                    fprintf(fp, "+L%-6d\n", lineCount-1);  // Start of continuation
                    fprintf(fp, "+L%-6d"  , lineCount-1);  // End of continuation
                }

                lineCount += 1;
            }

            fprintf(fp,"%s%7s", delimiter,feaAnalysis->rigidConstraint[i]);

            tempString = convert_doubleToString(feaAnalysis->magRigidConstraint[i], fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            sidIndex += 2;
        }

        for (i = 0; i < feaAnalysis->numControlConstraint; i++) {

            if (sidIndex % (8*lineCount) == 0) {

                if (lineCount == 1) fprintf(fp,"%8s", " ");

                if (feaFileFormat->fileType == FreeField) {
                    fprintf(fp, ",+L%-5d\n", lineCount-1); // Start of continuation
                    fprintf(fp, "+L%-5d,"  , lineCount-1);  // End of continuation
                } else {
                    fprintf(fp, "+L%-6d\n", lineCount-1);  // Start of continuation
                    fprintf(fp, "+L%-6d"  , lineCount-1);  // End of continuation
                }

                lineCount += 1;
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
        fprintf(fp,"%s\n","$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|");
        // MKAERO1, FLUTTER, FLFACT for density, mach and velocity
        // Write MKAERO1 INPUT
        fprintf(fp,"%-8s", "MKAERO1");

        if (feaAnalysis->numMachNumber != 0) {
            if (feaAnalysis->numMachNumber > 8) {
                printf("\t*** Warning *** Mach number input for AeroelasticFlutter in nastranAIM must be less than eight\n");
            }

            for (i = 0; i < 8; i++) {
                if (i < feaAnalysis->numMachNumber) {
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
                printf("\t*** Warning *** Reduced freq. input for AeroelasticFlutter in nastranAIM must be less than eight\n");
            }

            fprintf(fp, "+MK\n");
            fprintf(fp, "%-8s","+MK");

            for (i = 0; i < feaAnalysis->numReducedFreq; i++) {
                if (i == 8) break;
                tempString = convert_doubleToString(feaAnalysis->reducedFreq[i], fieldWidth, 1);
                fprintf(fp, "%s%s", delimiter, tempString);
                EG_free(tempString);
            }

            //fprintf(fp, "\n");

        }

        fprintf(fp, "\n");

        //fprintf(fp,"%s","$LUTTER SID     METHOD  DENS    MACH    RFREQ     IMETH   NVAL/OMAX   EPS   CONT\n");


        // Write MKAERO1 INPUT
        fprintf(fp,"%-8s", "FLUTTER");

        //SID
        tempString = convert_integerToString(100 + feaAnalysis->analysisID, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%-7s", delimiter, "PK");
        //fprintf(fp, "%s%-7s", delimiter, "KE");

        //DENS
        tempString = convert_integerToString(10 * feaAnalysis->analysisID + 1, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        //MACH
        tempString = convert_integerToString(10 * feaAnalysis->analysisID + 2, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        //VELOCITY
        tempString = convert_integerToString(10 * feaAnalysis->analysisID + 3, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "%s%-7s", delimiter, "L"); // IMETH

        tempString = convert_integerToString(feaAnalysis->numDesiredEigenvalue, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        fprintf(fp, "\n");

        fprintf(fp,"$ DENSITY\n");
        fprintf(fp,"%-8s", "FLFACT");
        //DENS
        tempString = convert_integerToString(10 * feaAnalysis->analysisID + 1, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaAnalysis->density, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);
        fprintf(fp, "\n");

        fprintf(fp,"$ MACH\n");
        status = nastran_writeFLFactCard(fp, feaFileFormat, 10 * feaAnalysis->analysisID + 2, feaAnalysis->numMachNumber, feaAnalysis->machNumber);
        if (status != CAPS_SUCCESS) return status;

        fprintf(fp,"$ DYANMIC PRESSURE=%f\n", feaAnalysis->dynamicPressure);

        velocity = sqrt(2*feaAnalysis->dynamicPressure/feaAnalysis->density);
        vmin = velocity / 2.0;
        vmax = 2 * velocity;
        dv = (vmax - vmin) / (double) (numVel-3);

        for (i = 0; i < numVel-2; i++) {
            velocityArray[i+1] = vmin + (double) i * dv;
        }

        velocityArray[0] = velocity/10;
        velocityArray[numVel-1] = velocity*10;

        fprintf(fp,"$ VELOCITY\n");
        status = nastran_writeFLFactCard(fp, feaFileFormat, 10 * feaAnalysis->analysisID + 3, numVel, velocityArray);
        if (status != CAPS_SUCCESS) return status;

        //fprintf(fp,"$ REDUCED FREQ\n");
        //status = nastran_writeFLFactCard(fp, feaFileFormat, 10 * feaAnalysis->analysisID + 3,feaAnalysis->numReducedFreq, feaAnalysis->reducedFreq);
        //if (status != CAPS_SUCCESS) return status;

    }

    return CAPS_SUCCESS;
}

// Write a combined Nastran design constraint card from a set of constraint IDs.
//  The combined design constraint ID is set through the constraint ID variable.
int nastran_writeDesignConstraintADDCard(FILE *fp, int constraintID, int numSetID, int designConstraintSetID[], feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing

    int sidIndex= 0, lineCount = 0; // Counters

    char *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (numSetID > 0 && designConstraintSetID == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
    } else {
        delimiter = " ";
    }

    if (numSetID != 0) {
        fprintf(fp,"%-8s%s%7d", "DCONADD", delimiter, constraintID);
    }

    lineCount = 1;
    for (i = 0; i < numSetID; i++) {
        sidIndex += 1;
        if (sidIndex % (8*lineCount) == 0) {

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ",+L%-5d\n", lineCount-1); // Start of continuation
                fprintf(fp, "+L%-5d,"  , lineCount-1);  // End of continuation
            } else {
                fprintf(fp, "+L%-6d\n", lineCount-1);  // Start of continuation
                fprintf(fp, "+L%-6d"  , lineCount-1);  // End of continuation
            }

            lineCount += 1;
        }

        fprintf(fp,"%s%7d",delimiter, designConstraintSetID[i]);
    }

    if (numSetID != 0) fprintf(fp,"\n\n");

    return CAPS_SUCCESS;
}

// Write design constraint/optimization information from a feaDesignConstraint structure
int nastran_writeDesignConstraintCard(FILE *fp, feaDesignConstraintStruct *feaDesignConstraint, feaFileFormatStruct *feaFileFormat) {

    int  i, j, found; // Index

    int fieldWidth;

    int elementStressLocation[4];

    char *tempString=NULL;
    char *delimiter, *valstr;

    if (fp == NULL) return CAPS_IOERR;
    if (feaDesignConstraint == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

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

            // DCONSTR, DCID, RID, LALLOW, UALLOW
            fprintf(fp,"%-8s", "DCONSTR");

            tempString = convert_integerToString(feaDesignConstraint->designConstraintID, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaDesignConstraint->lowerBound, fieldWidth, 1);
            fprintf(fp,"%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(feaDesignConstraint->upperBound, fieldWidth, 1);
            fprintf(fp,"%s%s\n",delimiter, tempString);
            EG_free(tempString);

            //$...STRUCTURAL RESPONSE IDENTIFICATION
            //$DRESP1 ID      LABEL   RTYPE   PTYPE   REGION  ATTA    ATTB    ATT1    +
            //$+      ATT2    ...

            // ------------- STRESS RESPONSE ---------------------------------------------------------------------
            if (strcmp(feaDesignConstraint->responseType,"STRESS") == 0) {
                fprintf(fp,"%-8s", "DRESP1");

                tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000, fieldWidth, 1);
                fprintf(fp,"%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000, fieldWidth-2, 0);
                fprintf(fp, "%sR%s", delimiter,  tempString);
                EG_free(tempString);

                fprintf(fp,"%s%7s",delimiter,feaDesignConstraint->responseType);

                fprintf(fp,"%s%7s",delimiter,"PROD");

                fprintf(fp,"%s%7s", delimiter,""); // REGION

                tempString = convert_integerToString(2, fieldWidth, 1); // Axial Stress
                // 2 - Axial Stress
                // 4 - Torsional Stress
                fprintf(fp, "%s%s",delimiter, tempString); // ATTA
                EG_free(tempString);

                fprintf(fp,"%s%7s", delimiter,""); // ATTB

                tempString = convert_integerToString(feaDesignConstraint->propertySetID[i], fieldWidth, 1);
                fprintf(fp, "%s%s\n",delimiter, tempString); // ATT1 (PROD PID)
                EG_free(tempString);
            }
        } else if (feaDesignConstraint->propertySetType[i] == Bar) {
            // Nothing set yet
        } else if (feaDesignConstraint->propertySetType[i] == Shell) {

            // ------------- STRESS RESPONSE ---------------------------------------------------------------------
            if (strcmp(feaDesignConstraint->responseType,"STRESS") == 0) {
                elementStressLocation[0] = 7; // Major principal at Z1
                elementStressLocation[1] = 9; // Von Mises (or max shear) at Z1
                elementStressLocation[2] = 16; // Minor principal at Z2
                elementStressLocation[3] = 17; // von Mises (or max shear) at Z2

                for (j = 0; j < 4; j++) {

                    // DCONSTR, DCID, RID, LALLOW, UALLOW
                    fprintf(fp,"%-8s", "DCONSTR");

                    tempString = convert_integerToString(feaDesignConstraint->designConstraintID, fieldWidth, 1);
                    fprintf(fp,"%s%s",delimiter, tempString);
                    EG_free(tempString);

                    tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000+j*1000, fieldWidth, 1);
                    fprintf(fp,"%s%s",delimiter, tempString);
                    EG_free(tempString);

                    tempString = convert_doubleToString(feaDesignConstraint->lowerBound, fieldWidth, 1);
                    fprintf(fp,"%s%s",delimiter, tempString);
                    EG_free(tempString);

                    tempString = convert_doubleToString(feaDesignConstraint->upperBound, fieldWidth, 1);
                    fprintf(fp,"%s%s\n",delimiter, tempString);
                    EG_free(tempString);


                    // DRESP1, ID, LABEL, RTYPE, PTYPE, REGION, ATTA, ATTB, ATT1
                    //         ATT2, ...

                    fprintf(fp,"%-8s", "DRESP1");

                    tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000+j*1000, fieldWidth, 1);
                    fprintf(fp,"%s%s",delimiter, tempString);
                    EG_free(tempString);

                    tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000+j*1000, fieldWidth-2, 0);
                    //fprintf(fp, "%s%s%.*s", delimiter,  tempString,1,feaDesignConstraint->name);
                    fprintf(fp, "%sR%s", delimiter,  tempString);
                    EG_free(tempString);

                    fprintf(fp,"%s%7s",delimiter,feaDesignConstraint->responseType);

                    fprintf(fp,"%s%7s",delimiter,"PSHELL");

                    fprintf(fp,"%s%7s", delimiter,"");

                    //printf("Printing DRESP1 isn't very robust yet.....\n");
                    tempString = convert_integerToString(elementStressLocation[j], fieldWidth, 1);
                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);

                    fprintf(fp,"%s%7s", delimiter,"");

                    tempString = convert_integerToString(feaDesignConstraint->propertySetID[i], fieldWidth, 1);
                    fprintf(fp, "%s%s\n",delimiter, tempString);
                    EG_free(tempString);
                }
            }

        } else if (feaDesignConstraint->propertySetType[i] == Composite) {

            // ------------- CFAILURE RESPONSE ---------------------------------------------------------------------
            if (strcmp(feaDesignConstraint->responseType,"CFAILURE") == 0) {
                // DCONSTR, DCID, RID, LALLOW, UALLOW
                fprintf(fp,"%-8s", "DCONSTR");

                tempString = convert_integerToString(feaDesignConstraint->designConstraintID, fieldWidth, 1);
                fprintf(fp,"%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000, fieldWidth, 1);
                fprintf(fp,"%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_doubleToString(feaDesignConstraint->lowerBound, fieldWidth, 1);
                fprintf(fp,"%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_doubleToString(feaDesignConstraint->upperBound, fieldWidth, 1);
                fprintf(fp,"%s%s\n",delimiter, tempString);
                EG_free(tempString);

                // DRESP1, ID, LABEL, RTYPE, PTYPE, REGION, ATTA, ATTB, ATT1
                //         ATT2, ...
                fprintf(fp,"%-8s", "DRESP1");

                tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000, fieldWidth, 1);
                fprintf(fp,"%s%s",delimiter, tempString);
                EG_free(tempString);

                tempString = convert_integerToString(feaDesignConstraint->designConstraintID+10000, fieldWidth-2, 0);
                //fprintf(fp, "%s%s%.*s", delimiter,  tempString,1,feaDesignConstraint->name);
                fprintf(fp, "%sL%s", delimiter,  tempString);
                EG_free(tempString);

                fprintf(fp,"%s%7s",delimiter,feaDesignConstraint->responseType);

                fprintf(fp,"%s%7s",delimiter,"PCOMP");

                fprintf(fp,"%s%7s", delimiter,"");

                // ATTA - Failure Criterion Item Code (5)
                tempString = convert_integerToString(5, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);

                // ATTB - Lamina Number
                if (feaDesignConstraint->fieldPosition == 0) {
                    found = 0;
                    // OPTIONS ARE "Ti", "THETAi", "LAMINAi" all = Integer i
                    valstr = NULL;
                    // skan for T, THETA or LAMINA

                    valstr = strstr(feaDesignConstraint->fieldName, "THETA");
                    if (valstr != NULL) {
                        fprintf(fp,"%s%7s", delimiter,&feaDesignConstraint->fieldName[5]);
                        found = 1;
                    }

                    if (found == 0) {
                        valstr = strstr(feaDesignConstraint->fieldName, "LAMINA");
                        if (valstr != NULL) {
                            fprintf(fp,"%s%7s", delimiter,&feaDesignConstraint->fieldName[6]);
                            found = 1;
                        }
                    }

                    if (found == 0) {
                        valstr = strstr(feaDesignConstraint->fieldName, "T");
                        if (valstr != NULL) {
                            fprintf(fp,"%s%7s", delimiter,&feaDesignConstraint->fieldName[1]);
                            found = 1;
                        }
                    }

                    if (found == 0) {
                        printf("  WARNING: Cound not determine what Lamina to apply constraint too, using default = 1\n");
                        printf("  String Entered: %s\n", feaDesignConstraint->fieldName);
                        tempString = convert_integerToString(1, fieldWidth, 1);
                        fprintf(fp, "%s%s",delimiter, tempString);
                        EG_free(tempString);
                    }
                } else {
                    tempString = convert_integerToString(feaDesignConstraint->fieldPosition, fieldWidth, 1);
                    fprintf(fp, "%s%s",delimiter, tempString);
                    EG_free(tempString);
                }

                // ATT1 - Property ID
                tempString = convert_integerToString(feaDesignConstraint->propertySetID[i], fieldWidth, 1);
                fprintf(fp, "%s%s\n",delimiter, tempString);
                EG_free(tempString);
            }


        } else if (feaDesignConstraint->propertySetType[i] == Solid) {
            // Nothing set yet
        }
    }

    return CAPS_SUCCESS;
}

// Write design variable/optimization information from a feaDesignVariable structure
int nastran_writeDesignVariableCard(FILE *fp, feaDesignVariableStruct *feaDesignVariable, feaFileFormatStruct *feaFileFormat) {

    int  i, c, fieldWidth;

    int status; // Function return status

    char *tempString=NULL;
    char *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (feaDesignVariable == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
        fprintf(fp, "$---1---|---2---|---3---|---4---|---5---|---6---|---7---|---8---|---9---|---10--|\n");
    }


    // DESVAR, ID, LABEL, XINIT, XLB, XUB, DELXV
    fprintf(fp,"%-8s", "DESVAR");

    tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    fprintf(fp, "%s%7s", delimiter,feaDesignVariable->name);

    tempString = convert_doubleToString(feaDesignVariable->initialValue, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    /*
    tempString = convert_doubleToString(feaDesignVariable->lowerBound, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);

    tempString = convert_doubleToString(feaDesignVariable->upperBound, fieldWidth, 1);
    fprintf(fp, "%s%s",delimiter, tempString);
    EG_free(tempString);
     */

    if (feaDesignVariable->numDiscreteValue == 0) {

        tempString = convert_doubleToString(feaDesignVariable->lowerBound, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaDesignVariable->upperBound, fieldWidth, 1);
        fprintf(fp, "%s%s",delimiter, tempString);
        EG_free(tempString);

        tempString = convert_doubleToString(feaDesignVariable->maxDelta, fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

    } else {

        if (feaFileFormat->fileType == FreeField) {
            fprintf(fp,", , ");
        } else {
            fprintf(fp," %7s %7s", "", "");
        }

        // ID to DDVAL entry
        tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
        fprintf(fp, "%s%s\n",delimiter, tempString);
        EG_free(tempString);

        // Write DDVAL card
        status = nastran_writeDDVALCard(fp,
                                        feaDesignVariable->designVariableID,
                                        feaDesignVariable->numDiscreteValue,
                                        feaDesignVariable->discreteValue,
                                        feaFileFormat);
        if (status != CAPS_SUCCESS) return status;

    }


    if (feaDesignVariable->designVariableType == MaterialDesignVar) {

        for (i = 0; i < feaDesignVariable->numMaterialID; i++) {
            fprintf(fp,"%-8s", "DVMREL1");

            tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
            fprintf(fp, "%s%s", delimiter, tempString);
            EG_free(tempString);

            // UnknownMaterial, Isotropic, Anisothotropic, Orthotropic, Anisotropic
            if (feaDesignVariable->materialSetType[i] == Isotropic) {
                fprintf(fp,"%s%7s",delimiter, "MAT1");
            } else if (feaDesignVariable->materialSetType[i] == Anisothotropic) {
                fprintf(fp,"%s%7s",delimiter, "MAT2");
            } else if (feaDesignVariable->materialSetType[i] == Orthotropic) {
                fprintf(fp,"%s%7s",delimiter, "MAT8");
            } else if (feaDesignVariable->materialSetType[i] == Anisotropic) {
                fprintf(fp,"%s%7s",delimiter, "MAT9");
            }

            tempString = convert_integerToString(feaDesignVariable->materialSetID[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            if (feaDesignVariable->fieldPosition == 0) {
                fprintf(fp,"%s%7s",delimiter, feaDesignVariable->fieldName);
            } else {
                tempString = convert_integerToString(feaDesignVariable->fieldPosition, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);
            }

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp,", , , , ");
            } else {
                fprintf(fp," %7s %7s %7s %7s", "", "", "", "");
            }

            tempString = convert_integerToString(feaDesignVariable->designVariableID, 5, 0);
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, "%s+DV%s\n+DV%s",delimiter, tempString, tempString);
            } else {
                fprintf(fp, "+DV%s\n+DV%s", tempString, tempString);
            }
            EG_free(tempString);

            tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(1.0, fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);

        }
    }

    if (feaDesignVariable->designVariableType == PropertyDesignVar) {

        // DVPREL1, ID, TYPE, PID, FID, PMIN, PMAX, CO
        //          DVID1, COEF1, DVID2, COEF2 ...
        for (i = 0; i < feaDesignVariable->numPropertyID; i++) {

            fprintf(fp,"%-8s", "DVPREL1");

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
            } else if (feaDesignVariable->propertySetType[i] == Solid) {
                fprintf(fp,"%s%7s",delimiter, "PSOLID");
            }

            tempString = convert_integerToString(feaDesignVariable->propertySetID[i], fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            if (feaDesignVariable->fieldPosition == 0) {
                fprintf(fp,"%s%7s",delimiter, feaDesignVariable->fieldName);
            } else {
                tempString = convert_integerToString(feaDesignVariable->fieldPosition, fieldWidth, 1);
                fprintf(fp, "%s%s",delimiter, tempString);
                EG_free(tempString);
            }

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp,", , , , ");
            } else {
                fprintf(fp," %7s %7s %7s %7s", "", "", "", "");
            }

            tempString = convert_integerToString(feaDesignVariable->designVariableID, 5, 0);
            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, "%s+DV%s\n+DV%s",delimiter, tempString, tempString);
            } else {
                fprintf(fp, "+DV%s\n+DV%s", tempString, tempString);
            }
            EG_free(tempString);

            tempString = convert_integerToString(feaDesignVariable->designVariableID, fieldWidth, 1);
            fprintf(fp, "%s%s",delimiter, tempString);
            EG_free(tempString);

            tempString = convert_doubleToString(1.0, fieldWidth, 1);
            fprintf(fp, "%s%s\n",delimiter, tempString);
            EG_free(tempString);

        }
    }

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

    return CAPS_SUCCESS;
}

// Write a Nastran DDVAL card from a set of ddvalSet. The id is set through the ddvalID variable.
int nastran_writeDDVALCard(FILE *fp, int ddvalID, int numDDVALSet, double ddvalSet[], feaFileFormatStruct *feaFileFormat) {

    int i; // Indexing
    int fieldWidth;
    int sidIndex= 0, lineCount = 0; // Counters
    char *tempString = NULL, *delimiter;

    if (fp == NULL) return CAPS_IOERR;
    if (numDDVALSet > 0 && ddvalSet == NULL) return CAPS_NULLVALUE;
    if (feaFileFormat == NULL) return CAPS_NULLVALUE;

    if (feaFileFormat->fileType == FreeField) {
        delimiter = ",";
        fieldWidth = 8;
    } else {
        delimiter = " ";
        fieldWidth = 7;
    }

    if (numDDVALSet != 0) {
        fprintf(fp,"%-8s", "DDVAL");

        tempString = convert_integerToString(ddvalID, fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);
    }

    lineCount = 1;
    for (i = 0; i < numDDVALSet; i++) {
        sidIndex += 2;
        if (sidIndex % (8*lineCount) == 0) {

            if (feaFileFormat->fileType == FreeField) {
                fprintf(fp, ",+L%-5d\n", lineCount-1); // Start of continuation
                fprintf(fp, "+L%-5d,"  , lineCount-1);  // End of continuation
            } else {
                fprintf(fp, "+L%-6d\n", lineCount-1);  // Start of continuation
                fprintf(fp, "+L%-6d"  , lineCount-1);  // End of continuation
            }

            lineCount += 1;
        }

        tempString = convert_doubleToString(ddvalSet[i], fieldWidth, 1);
        fprintf(fp, "%s%s", delimiter, tempString);
        EG_free(tempString);
    }

    if (numDDVALSet != 0) fprintf(fp,"\n");

    return CAPS_SUCCESS;
}

// Read data from a Nastran F06 file to determine the number of eignevalues
int nastran_readF06NumEigenValue(FILE *fp, int *numEigenVector) {

    int status; // Function return

    char *beginEigenLine = "                                              R E A L   E I G E N V A L U E S";
    char *endEigenLine = "1";
    int keepCollecting = (int) true;

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder

    int tempInt[2];
    double tempDouble[5];

    *numEigenVector = 0;

    if (fp == NULL) return CAPS_IOERR;

    while (*numEigenVector == 0) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;

        // See how many Eigen-Values we have
        if (strncmp(beginEigenLine, line,strlen(beginEigenLine)) == 0) {

            // Skip ahead 2 lines
            status = getline(&line, &linecap, fp);
            if (status < 0) break;
            status = getline(&line, &linecap, fp);
            if (status < 0) break;

            while (keepCollecting == (int) true) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;

                if (strncmp(endEigenLine, line, strlen(endEigenLine)) == 0) {
                    keepCollecting = (int) false;
                    break;
                }

                sscanf(line, "%d%d%lf%lf%lf%lf%lf", &tempInt[0],
                                                    &tempInt[1],
                                                    &tempDouble[0],
                                                    &tempDouble[1],
                                                    &tempDouble[2],
                                                    &tempDouble[3],
                                                    &tempDouble[4]);

                if (tempDouble[3] < 1E-15 && tempDouble[4] < 1E-15){
                    keepCollecting = (int) false;
                    break;
                }

                *numEigenVector += 1;
            }
        }
    }

    if (line != NULL) EG_free(line);

    // Rewind the file
    rewind(fp);

    if (*numEigenVector == 0) return CAPS_NOTFOUND;
    else return CAPS_SUCCESS;
}

// Read data from a Nastran F06 file and load it into a dataMatrix[numEigenVector][numGridPoint*8]
// where variables are Grid Id, Coord Id, T1, T2, T3, R1, R2, R3
int nastran_readF06EigenVector(FILE *fp, int *numEigenVector, int *numGridPoint, double ***dataMatrix) {

    int status = 0; // Function return

    int i, j, eigenValue = 0; // Indexing

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder

    char *beginEigenLine = "      EIGENVALUE =";
    char *endEigenLine = "1";
    char tempString[30];

    int numVariable = 8; // Grid Id, Coord Id, T1, T2, T3, R1, R2, R3

    printf("Reading Nastran FO6 file - extracting Eigen-Vectors!\n");

    *numEigenVector = 0;
    *numGridPoint = 0;

    if (fp == NULL) return CAPS_IOERR;

    // See how many Eigen-Values we have
    //status = nastran_readF06NumEigenValue(fp, numEigenVector);
    *numEigenVector = 10;
    printf("\tNumber of Eigen-Vectors = %d\n", *numEigenVector);
    if (status != CAPS_SUCCESS) return status;

    // Loop through file line by line until we have determined how many Eigen-Values and grid points we have
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
            while (getline(&line, &linecap, fp) >= 0) {

                if (strncmp(endEigenLine, line, strlen(endEigenLine)) == 0) break;

                *numGridPoint +=1;
            }
        }
    }

    if (line != NULL) EG_free(line);
    line = NULL;

    printf("\tNumber of Grid Points = %d for each Eigen-Vector\n", *numGridPoint);
    if (*numGridPoint == 0) return CAPS_NOTFOUND;

    // Rewind the file
    rewind(fp);

    // Allocate dataMatrix array
    if (*dataMatrix != NULL) EG_free(*dataMatrix);

    *dataMatrix = (double **) EG_alloc(*numEigenVector *sizeof(double *));
    if (*dataMatrix == NULL) return EGADS_MALLOC; // If allocation failed ....

    for (i = 0; i < *numEigenVector; i++) {

        (*dataMatrix)[i] = (double *) EG_alloc((*numGridPoint)*numVariable*sizeof(double));

        if ((*dataMatrix)[i] == NULL) { // If allocation failed ....
            for (j = 0; j < i; j++) {

                if ((*dataMatrix)[j] != NULL ) EG_free((*dataMatrix)[j]);
            }

            if ((*dataMatrix) != NULL) EG_free((*dataMatrix));

            return EGADS_MALLOC;
        }
    }

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

            if (line != NULL) EG_free(line);
            line = NULL;

            // Loop through the file and fill up the data matrix
            for (i = 0; i < *numGridPoint; i++) {
                for (j = 0; j < numVariable; j++) {

                    if (j == 0) {
                        fscanf(fp, "%lf", &(*dataMatrix)[eigenValue][j+numVariable*i]);
                        //printf("Data (i,j) = (%d, %d) = %f\n", i,j, (*dataMatrix)[eigenValue][j+numVariable*i]);
                        fscanf(fp, "%s", tempString);
                        //printf("String = %s\n", tempString);
                        j = j + 1;
                        (*dataMatrix)[eigenValue][j+numVariable*i] = 0.0;
                    } else fscanf(fp, "%lf", &(*dataMatrix)[eigenValue][j+numVariable*i]);

                    //printf("Data (i,j) = (%d, %d) = %f\n", i,j, (*dataMatrix)[eigenValue][j+numVariable*i]);
                }
            }

            eigenValue += 1;
        }

        if (eigenValue == *numEigenVector) break;
    }

    if (line != NULL) EG_free(line);

    return CAPS_SUCCESS;
}

// Read data from a Nastran F06 file and load it into a dataMatrix[numEigenVector][5]
// where variables are eigenValue, eigenValue(radians), eigenValue(cycles), generalized mass, and generalized stiffness.                                                                   MASS              STIFFNESS
int nastran_readF06EigenValue(FILE *fp, int *numEigenVector, double ***dataMatrix) {

    int status; // Function return

    int i, j;// Indexing

    int tempInt, eigenValue =0;

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder

    char *beginEigenLine = "                                              R E A L   E I G E N V A L U E S";
    //char *endEigenLine = "1";

    int numVariable = 5; // EigenValue, eigenValue(radians), eigenValue(cycles), generalized mass, and generalized stiffness.

    printf("Reading Nastran FO6 file - extracting Eigen-Values!\n");

    *numEigenVector = 0;

    if (fp == NULL) return CAPS_IOERR;

    // See how many Eigen-Values we have
    status = nastran_readF06NumEigenValue(fp, numEigenVector);
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

            // Fast forward 2 lines
            for (i = 0; i < 2; i++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }

            for (i = 0; i < *numEigenVector; i++) {
                fscanf(fp, "%d", &eigenValue);
                printf("\tLoading Eigen-Value = %d\n", eigenValue);

                fscanf(fp, "%d", &tempInt);

                // Loop through the file and fill up the data matrix
                for (j = 0; j < numVariable; j++) {

                    fscanf(fp, "%lf", &(*dataMatrix)[i][j]);

                }
            }
        }
    }

    if (line != NULL) EG_free(line);

    return CAPS_SUCCESS;
}

// Read data from a Nastran F06 file and load it into a dataMatrix[numGridPoint][8]
// where variables are Grid Id, Coord Id, T1, T2, T3, R1, R2, R3
int nastran_readF06Displacement(FILE *fp, int subcaseId, int *numGridPoint, double ***dataMatrix) {

    int status; // Function return

    int i, j; // Indexing

    size_t linecap = 0;

    char *line = NULL; // Temporary line holder

    char *outputSubcaseLine = "0                                                                                                            SUBCASE ";
    char *displacementLine ="                                             D I S P L A C E M E N T   V E C T O R";
    char *beginSubcaseLine=NULL;
    char *endSubcaseLine = "1";
    char tempString[30];

    int numVariable = 8; // Grid Id, coord Id T1, T2, T3, R1, R2, R3
    int intLength;
    int lineFastForward = 0;

    printf("Reading Nastran FO6 file - extracting Displacements!\n");

    *numGridPoint = 0;

    if (fp == NULL) return CAPS_IOERR;

    // Rewind the file
    rewind(fp);

    if      (subcaseId >= 1000) intLength = 4;
    else if (subcaseId >= 100) intLength = 3;
    else if (subcaseId >= 10) intLength = 2;
    else intLength = 1;

    if (subcaseId > 0) {
        beginSubcaseLine = (char *) EG_alloc((strlen(outputSubcaseLine)+intLength+1)*sizeof(char));
        if (beginSubcaseLine == NULL) return EGADS_MALLOC;

        sprintf(beginSubcaseLine,"%s%d",outputSubcaseLine, subcaseId);

        beginSubcaseLine[strlen(outputSubcaseLine)+intLength] = '\0';

        lineFastForward = 4;

    } else {

        intLength = 0;
        beginSubcaseLine = (char *) EG_alloc((strlen(displacementLine)+1)*sizeof(char));
        if (beginSubcaseLine == NULL) return EGADS_MALLOC;
        sprintf(beginSubcaseLine,"%s",displacementLine);

        //beginSubcaseLine[strlen(displacementLine)] = '\0';

        lineFastForward = 2;
    }

    // Loop through file line by line until we have determined how many grid points we have
    while (*numGridPoint == 0) {

        // Get line from file
        status = getline(&line, &linecap, fp);
        if (status < 0) break;

        // Look for start of subcaseId
        if (strncmp(beginSubcaseLine, line, strlen(beginSubcaseLine)) == 0) {

            // Fast forward lines
            for (i = 0; i < lineFastForward; i++) {
                status = getline(&line, &linecap, fp);
                if (status < 0) break;
            }

            // Loop through lines counting the number of grid points
            while (getline(&line, &linecap, fp) >= 0) {

                if (strncmp(endSubcaseLine, line,strlen(endSubcaseLine)) == 0)break;
                *numGridPoint +=1;
            }
        }
    }
    if (line != NULL) EG_free(line);
    line = NULL;
    linecap = 0;

    printf("Number of Grid Points = %d\n", *numGridPoint);

    if (*numGridPoint == 0) {
        printf("Either data points  = 0 and/or subcase wasn't found\n");

        if (beginSubcaseLine != NULL) EG_free(beginSubcaseLine);
        return CAPS_NOTFOUND;
    }

    // Rewind the file
    rewind(fp);

    // Allocate dataMatrix array
    //if (*dataMatrix != NULL) EG_free(*dataMatrix);

    *dataMatrix = (double **) EG_alloc(*numGridPoint *sizeof(double *));
    if (*dataMatrix == NULL) {
        if (beginSubcaseLine != NULL) EG_free(beginSubcaseLine);
        return EGADS_MALLOC; // If allocation failed ....
    }

    for (i = 0; i < *numGridPoint; i++) {

        (*dataMatrix)[i] = (double *) EG_alloc(numVariable*sizeof(double));

        if ( (*dataMatrix)[i] == NULL) { // If allocation failed ....
            for (j = 0; j < i; j++) {

                if ((*dataMatrix)[j] != NULL ) EG_free((*dataMatrix)[j]);
            }

            if ((*dataMatrix) != NULL) EG_free((*dataMatrix));

            if (beginSubcaseLine != NULL) EG_free(beginSubcaseLine);

            return EGADS_MALLOC;
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
                //printf("Line - %s\n", line);
                if (status < 0) {
                    printf("Unable to fast forward through file- status %d\n", status);
                    break;
                }
            }


            // Loop through the file and fill up the data matrix
            for (i = 0; i < (*numGridPoint); i++) {
                for (j = 0; j < numVariable; j++) {

                    if (j == 0){// || j % numVariable+1 == 0) {
                        fscanf(fp, "%lf", &(*dataMatrix)[i][j]);
                        fscanf(fp, "%s", tempString);
                        j = j + 1;
                        (*dataMatrix)[i][j] = 0.0;
                    } else fscanf(fp, "%lf", &(*dataMatrix)[i][j]);
                }
            }

            break;
        }
    }

    if (beginSubcaseLine != NULL) EG_free(beginSubcaseLine);
    if (line != NULL) EG_free(line);

    printf("Done reading displacements for Subcase = %d\n", subcaseId);
    return CAPS_SUCCESS;
}
