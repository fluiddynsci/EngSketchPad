#include "feaTypes.h"  // Bring in FEA structures
#include "vlmTypes.h"  // Bring in VLM structures

#ifdef __cplusplus
extern "C" {
#endif

// Write out FLFact Card.
int nastran_writeFLFactCard(FILE *fp, feaFileFormatStruct *feaFileFormat, int id, int numVal, double values[]);

// Write a Nastran element cards not supported by mesh_writeNastran in meshUtils.c
int nastran_writeSubElementCard(FILE *fp, meshStruct *feaMesh, int numProperty, feaPropertyStruct *feaProperty, feaFileFormatStruct *feaFileFormat);

// Write a Nastran connections card from a feaConnection structure
int nastran_writeConnectionCard(FILE *fp, feaConnectionStruct *feaConnect, feaFileFormatStruct *feaFileFormat);

// Write a Nastran AERO card from a feaAeroRef structure
int nastran_writeAEROCard(FILE *fp, feaAeroRefStruct *feaAeroRef, feaFileFormatStruct *feaFileFormat);

// Write a Nastran AEROS card from a feaAeroRef structure
int nastran_writeAEROSCard(FILE *fp, feaAeroRefStruct *feaAeroRef, feaFileFormatStruct *feaFileFormat);

// Write Nastran SET1 card from a feaAeroStruct
int nastran_writeSet1Card(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat);

// Write Nastran Spline1 cards from a feaAeroStruct
int nastran_writeAeroSplineCard(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat);

// Write Nastran CAERO1 cards from a feaAeroStruct
int nastran_writeCAeroCard(FILE *fp, feaAeroStruct *feaAero, feaFileFormatStruct *feaFileFormat);

// Write Nastran coordinate system card from a feaCoordSystemStruct structure
int nastran_writeCoordinateSystemCard(FILE *fp, feaCoordSystemStruct *feaCoordSystem, feaFileFormatStruct *feaFileFormat);

// Write combined Nastran constraint card from a set of constraint IDs.
// 	The combined constraint ID is set through the constraintID variable.
int nastran_writeConstraintADDCard(FILE *fp, int constraintID, int numSetID, int constraintSetID[], feaFileFormatStruct *feaFileFormat);

// Write Nastran constraint card from a feaConstraint structure
int nastran_writeConstraintCard(FILE *fp, feaConstraintStruct *feaConstraint, feaFileFormatStruct *feaFileFormat);

// Write Nastran support card from a feaSupport structure - withID = NULL or false SUPORT, if withID = true SUPORT1
int nastran_writeSupportCard(FILE *fp, feaSupportStruct *feaSupport, feaFileFormatStruct *feaFileFormat, int *withID);

// Write a Nastran Material card from a feaMaterial structure
int nastran_writeMaterialCard(FILE *fp, feaMaterialStruct *feaMaterial, feaFileFormatStruct *feaFileFormat);

// Write a Nastran Property card from a feaProperty structure
int nastran_writePropertyCard(FILE *fp, feaPropertyStruct *feaProperty, feaFileFormatStruct *feaFileFormat);

// Write a combined Nastran load card from a set of load IDs. Uses the feaLoad structure to get the local scale factor
// 	for the load. The overall load scale factor is set to 1. The combined load ID is set through the loadID variable.
int nastran_writeLoadADDCard(FILE *fp, int loadID, int numSetID, int loadSetID[], feaLoadStruct feaLoad[], feaFileFormatStruct *feaFileFormat);

// Write Nastran load card from a feaLoad structure
int nastran_writeLoadCard(FILE *fp, feaLoadStruct *feaLoad, feaFileFormatStruct *feaFileFormat);

// Write Nastran analysis card from a feaAnalysis structure
int nastran_writeAnalysisCard(FILE *fp, feaAnalysisStruct *feaAnalysis, feaFileFormatStruct *feaFileFormat);

// Write a combined Nastran design constraint card from a set of constraint IDs.
//  The combined design constraint ID is set through the constraint ID variable.
int nastran_writeDesignConstraintADDCard(FILE *fp, int constraintID, int numSetID, int designConstraintSetID[], feaFileFormatStruct *feaFileFormat);

// Write design constraint/optimization information from a feaDesignConstraint structure
int nastran_writeDesignConstraintCard(FILE *fp, feaDesignConstraintStruct *feaDesignConstraint, feaFileFormatStruct *feaFileFormat);

// Write design variable/optimization information from a feaDesignVariable structure
int nastran_writeDesignVariableCard(FILE *fp, feaDesignVariableStruct *feaDesignVariable, feaFileFormatStruct *feaFileFormat);

// Write a Nastran DDVAL card from a set of ddvalSet. The id is set through the ddvalID variable.
int nastran_writeDDVALCard(FILE *fp, int ddvalID, int numDDVALSet, double ddvalSet[], feaFileFormatStruct *feaFileFormat);

// Read data from a Nastran F06 file to determine the number of eignevalues
int nastran_readF06NumEigenValue(FILE *fp, int *numEigenVector);

// Read data from a Nastran F06 file and load it into a dataMatrix[numEigenVector][numGridPoint*8]
// where variables are Grid Id, Coord Id, T1, T2, T3, R1, R2, R3
int nastran_readF06EigenVector(FILE *fp, int *numEigenVector, int *numGridPoint, double ***dataMatrix);

// Read data from a Nastran F06 file and load it into a dataMatrix[numEigenVector][5]
// where variables are eigenValue, eigenValue(radians), eigenValue(cycles), generalized mass, and generalized stiffness.                                                                   MASS              STIFFNESS
int nastran_readF06EigenValue(FILE *fp, int *numEigenVector, double ***dataMatrix);

// Read data from a Nastran F06 file and load it into a dataMatrix[numGridPoint][8]
// where variables are Grid Id, Coord Id, T1, T2, T3, R1, R2, R3
int nastran_readF06Displacement(FILE *fp, int subcaseId, int *numGridPoint, double ***dataMatrix);

#ifdef __cplusplus
}
#endif
