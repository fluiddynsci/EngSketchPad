
#include "meshTypes.h"  // Bring in mesh structures
#include "capsTypes.h"  // Bring in CAPS types
#include "feaTypes.h"  // Bring in FEA structures


#ifdef __cplusplus
extern "C" {
#endif

// Retrieves a mesh via linkage or generates a mesh with fea_bodyToBEM
int fea_createMesh(void *aimInfo,
                   double paramTess[3],                 // (in)  Tessellation parameters
                   int    edgePointMin,                 // (in)  minimum points along any Edge
                   int    edgePointMax,                 // (in)  maximum points along any Edge
                   int    quadMesh,                     // (in)  only do tris-for faces
                   mapAttrToIndexStruct *attrMap,       // (in)  map from CAPSGroup names to indexes
                   mapAttrToIndexStruct *constraintMap, // (in)  map from CAPSConstraint names to indexes
                   mapAttrToIndexStruct *loadMap,       // (in)  map from CAPSLoad names to indexes
                   mapAttrToIndexStruct *transferMap,   // (in)  map from CAPSTransfer names to indexes
                   mapAttrToIndexStruct *connectMap,    // (in)  map from CAPSConnect names to indexes
                   int *numMesh,                        // (out) total number of FEA mesh structures
                   meshStruct **feaMesh,                // (out) FEA mesh structure
                   feaProblemStruct *feaProblem );      // (out) FEA problem structure

// Convert an EGADS body to a boundary element model, modified by Ryan Durscher (AFRL)
// from code written by John Dannenhoffer @ Syracuse University, patterned after code
// written by Bob Haimes  @ MIT
int fea_bodyToBEM(ego    ebody,                        // (in)  EGADS Body
                  double paramTess[3],                 // (in)  Tessellation parameters
                  int    edgePointMin,                 // (in)  minimum points along any Edge
                  int    edgePointMax,                 // (in)  maximum points along any Edge
                  int    quadMesh,                     // (in)  0 - only do tris-for faces, 1 - mixed quad/tri, 2 - regularized quads
                  mapAttrToIndexStruct *attrMap,       // (in)  map from CAPSGroup names to indexes
                  mapAttrToIndexStruct *coordSystemMap,// (in)  map from CoordSystem names to indexes
                  mapAttrToIndexStruct *constraintMap, // (in)  map from CAPSConstraint names to indexes
                  mapAttrToIndexStruct *loadMap,       // (in)  map from CAPSLoad names to indexes
                  mapAttrToIndexStruct *transferMap,   // (in)  map from CAPSTransfer names to indexes
                  mapAttrToIndexStruct *connectMap,    // (in)  map from CAPSConnect names to indexes
                  meshStruct *feaMesh);                // (out) FEA mesh structure

// Set the fea analysis meta data in a mesh
int fea_setAnalysisData( mapAttrToIndexStruct *attrMap,       // (in)  map from CAPSGroup names to indexes
                         mapAttrToIndexStruct *coordSystemMap,// (in)  map from CoordSystem names to indexes
                         mapAttrToIndexStruct *constraintMap, // (in)  map from CAPSConstraint names to indexes
                         mapAttrToIndexStruct *loadMap,       // (in)  map from CAPSLoad names to indexes
                         mapAttrToIndexStruct *transferMap,   // (in)  map from CAPSTransfer names to indexes
                         mapAttrToIndexStruct *connectMap,    // (in)  map from CAPSConnect names to indexes
                         meshStruct *feaMesh);                // (in/out) FEA mesh structure

// Set feaData for a given point index and topology index. Ego faces, edges, and nodes must be provided along with attribute maps
int fea_setFEADataPoint(ego *faces, ego *edges, ego *nodes,
                        mapAttrToIndexStruct *attrMap,
                        mapAttrToIndexStruct *coordSystemMap,
                        mapAttrToIndexStruct *constraintMap,
                        mapAttrToIndexStruct *loadMap,
                        mapAttrToIndexStruct *transferMap,
                        mapAttrToIndexStruct *connectMap,
                        int pointType, int pointTopoIndex,
                        feaMeshDataStruct *feaData);// Set the feaData structure

// Get the material properties from a capsTuple
int fea_getMaterial(int numMaterialTuple,
                    capsTuple materialTuple[],
                    int *numMaterial,
                    feaMaterialStruct *feaMaterial[]) ;

// Get the property properties from a capsTuple
int fea_getProperty(int numPropertyTuple,
                    capsTuple propertyTuple[],
                    mapAttrToIndexStruct *attrMap,
                    feaProblemStruct *feaProblem);

// Get the constraint properties from a capsTuple
int fea_getConstraint(int numConstraintTuple,
                      capsTuple constraintTuple[],
                      mapAttrToIndexStruct *attrMap,
                      feaProblemStruct *feaProblem);

// Get the support properties from a capsTuple
int fea_getSupport(int numSupportTuple,
                   capsTuple supportTuple[],
                   mapAttrToIndexStruct *attrMap,
                   feaProblemStruct *feaProblem);

// Get the Connections properties from a capsTuple and create connections based on the mesh
int fea_getConnection(int numConnectionTuple,
                      capsTuple connectionTuple[],
                      mapAttrToIndexStruct *attrMap,
                      feaProblemStruct *feaProblem);

// Get the analysis properties from a capsTuple
int fea_getAnalysis(int numAnalysisTuple,
                    capsTuple analysisTuple[],
                    feaProblemStruct *feaProblem);

// Get the load properties from a capsTuple
int fea_getLoad(int numLoadTuple,
                capsTuple loadTuple[],
                mapAttrToIndexStruct *attrMap,
                feaProblemStruct *feaProblem);

// Get the design variables from a capsTuple
int fea_getDesignVariable(int numDesignVariableTuple,
                          capsTuple designVariableTuple[],
                          feaProblemStruct *feaProblem);

// Get the design constraints from a capsTuple
int fea_getDesignConstraint(int numDesignConstraintTuple,
                            capsTuple designConstraintTuple[],
                            feaProblemStruct *feaProblem);

/// Get the coordinate system information from the bodies and an attribute map (of CoordSystem)
int fea_getCoordSystem(int numBody,
                       ego bodies[],
                       mapAttrToIndexStruct coordSystemMap,
                       int *numCys,
                       feaCoordSystemStruct *feaCoordSystem[]);

// Initiate (0 out all values and NULL all pointers) of feaProperty in the feaPropertyStruct structure format
int initiate_feaPropertyStruct(feaPropertyStruct *feaProperty);

// Destroy (0 out all values and NULL all pointers) of feaProperty in the feaPropertyStruct structure format
int destroy_feaPropertyStruct(feaPropertyStruct *feaProperty);

// Initiate (0 out all values and NULL all pointers) of feaMaterial in the feaMaterialStruct structure format
int initiate_feaMaterialStruct(feaMaterialStruct *feaMaterial);

// Destroy (0 out all values and free and NULL all pointers) of feaMaterial in the feaMaterialStruct structure format
int destroy_feaMaterialStruct(feaMaterialStruct *feaMaterial);

// Initiate (0 out all values and NULL all pointers) of feaConstraint in the feaConstraintStruct structure format
int initiate_feaConstraintStruct(feaConstraintStruct *feaConstraint);

// Destroy (0 out all values and free and NULL all pointers) of feaConstraint in the feaConstraintStruct structure format
int destroy_feaConstraintStruct(feaConstraintStruct *feaConstraint);

// Initiate (0 out all values and NULL all pointers) of feaSupport in the feaSupportStruct structure format
int initiate_feaSupportStruct(feaSupportStruct *feaSupport);

// Destroy (0 out all values and free and NULL all pointers) of feaSupport in the feaSupportStruct structure format
int destroy_feaSupportStruct(feaSupportStruct *feaSupport);

// Initiate (0 out all values and NULL all pointers) of feaLoad in the feaLoadStruct structure format
int initiate_feaLoadStruct(feaLoadStruct *feaLoad);

// Destroy (0 out all values and free and NULL all pointers) of feaLoad in the feaLoadStruct structure format
int destroy_feaLoadStruct(feaLoadStruct *feaLoad);

// Initiate (0 out all values and NULL all pointers) of feaAnalysis in the feaAnalysisStruct structure format
int initiate_feaAnalysisStruct(feaAnalysisStruct *feaAnalysis);

// Destroy (0 out all values and free and NULL all pointers) of feaAnalysis in the feaAnalysisStruct structure format
int destroy_feaAnalysisStruct(feaAnalysisStruct *feaAnalysis);

// Initiate (0 out all values and NULL all pointers) of feaProblem in the feaProblemStruct structure format
int initiate_feaProblemStruct(feaProblemStruct *feaProblem);

// Destroy (0 out all values and free and NULL all pointers) of feaProblem in the feaProblemStruct structure format
int destroy_feaProblemStruct(feaProblemStruct *feaProblem);

// Initiate (0 out all values and NULL all pointers) of feaFileFormat in the feaFileFormatStruct structure format
int initiate_feaFileFormatStruct(feaFileFormatStruct *feaFileFormat);

// Destroy (0 out all values and NULL all pointers) of feaFileFormat in the feaFileFormatStruct structure format
int destroy_feaFileFormatStruct(feaFileFormatStruct *feaFileFormat);

// Initiate (0 out all values and NULL all pointers) of feaDesignVariable in the feaDesignVariableStruct structure format
int initiate_feaDesignVariableStruct(feaDesignVariableStruct *feaDesignVariable);

// Destroy (0 out all values and NULL all pointers) of feaDesignVariable in the feaDesignVariableStruct structure format
int destroy_feaDesignVariableStruct(feaDesignVariableStruct *feaDesignVariable);

// Initiate (0 out all values and NULL all pointers) of feaDesignConstraint in the feaDesignConstraintStruct structure format
int initiate_feaDesignConstraintStruct(feaDesignConstraintStruct *feaDesignConstraint);

// Destroy (0 out all values and NULL all pointers) of feaDesignConstraint in the feaDesignConstraintStruct structure format
int destroy_feaDesignConstraintStruct(feaDesignConstraintStruct *feaDesignConstraint);

// Initiate (0 out all values and NULL all pointers) of feaCoordSystem in the feaCoordSystemStruct structure format
int initiate_feaCoordSystemStruct(feaCoordSystemStruct *feaCoordSystem);

// Destroy (0 out all values and NULL all pointers) of feaCoordSystem in the feaCoordSystemStruct structure format
int destroy_feaCoordSystemStruct(feaCoordSystemStruct *feaCoordSystem);

// Initiate (0 out all values and NULL all pointers) of feaAero in the feaAeroStruct structure format
int initiate_feaAeroStruct(feaAeroStruct *feaAero);

// Destroy (0 out all values and NULL all pointers) of feaAero in the feaAeroStruct structure format
int destroy_feaAeroStruct(feaAeroStruct *feaAero);

// Initiate (0 out all values and NULL all pointers) of feaAeroRef in the feaAeroRefStruct structure format
int initiate_feaAeroRefStruct(feaAeroRefStruct *feaAeroRef);

// Destroy (0 out all values and NULL all pointers) of feaAeroRef in the feaAeroRefStruct structure format
int destroy_feaAeroRefStruct(feaAeroRefStruct *feaAeroRef);

// Initiate (0 out all values and NULL all pointers) of feaConnect in the feaConnectionStruct structure format
int initiate_feaConnectionStruct(feaConnectionStruct *feaConnect);

// Destroy (0 out all values and NULL all pointers) of feaConnect in the feaConnectionStruct structure format
int Destroy_feaConnectionStruct(feaConnectionStruct *feaConnect);

// Transfer external pressure from the discrObj into the feaLoad structure
int fea_transferExternalPressure(void *aimInfo, meshStruct *feaMesh, feaLoadStruct *feaLoad);

// Retrieve aerodynamic reference quantities from bodies
int fea_retrieveAeroRef(int numBody, ego *bodies, feaAeroRefStruct *feaAeroRef);

// Assign element "subtypes" based on properties set
int fea_assignElementSubType(int numProperty, feaPropertyStruct *feaProperty, meshStruct *feaMesh);

#ifdef __cplusplus
}
#endif
