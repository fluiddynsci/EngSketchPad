#ifdef __cplusplus
extern "C" {
#endif

// Extract the FEPOINT Tecoplot data from a FUN3D Aero-Loads file (connectivity is ignored) - dataMatrix = [numVariable][numDataPoint]
int fun3d_readAeroLoad(char *filename, int *numVariable, char **variableName[], int *numDataPoint, double ***dataMatrix);

// Create a 3D mesh for FUN3D from a 2D mesh
int fun3d_2DMesh(meshStruct *surfaceMesh,
                 mapAttrToIndexStruct *attrMap,
                 meshStruct *volumeMesh,
                 int *extrusionBCIndex);

// Write FUN3D data transfer files
int fun3D_dataTransfer(void *aimInfo,
                       const char *analysisPath,
                       char *projectName,
                       meshStruct volumeMesh,
                       modalAeroelasticStruct *eigenVector);

// Write FUN3D fun3d.nml file
int fun3d_writeNML(void *aimInfo,
                   const char *analysisPath,
                   capsValue *aimInputs,
                   cfdBCsStruct bcProps);

// Write FUN3D movingbody.input file
int fun3d_writeMovingBody(const char *analysisPath,
                          cfdBCsStruct bcProps,
                          modalAeroelasticStruct *modalAeroelastic);

// Write FUN3D parametrization/sensitivity file
int fun3d_writeParameterization(void *aimInfo,
                                const char *analysisPath,
                                meshStruct *volumeMesh,
                                int numGeomIn,
                                capsValue *geomInVal);

// Write FUN3D rubber.data file
int fun3d_writeRubber(void *aimInfo,
                      capsValue *aimInputs,
                      const char *analysisPath,
                      meshStruct *volumeMesh,
                      int numGeomIn,
                      capsValue *geomInVal);

// Make FUN3D directory structure/tree:q
int fun3d_makeDirectory(const char *analysisPath);

#ifdef __cplusplus
}
#endif
