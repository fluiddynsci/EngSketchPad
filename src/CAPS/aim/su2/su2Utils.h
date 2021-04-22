#ifdef __cplusplus
extern "C" {
#endif

// Extract the FEPOINT Tecoplot data from a FUN3D Aero-Loads file (connectivity is ignored) - dataMatrix = [numVariable][numDataPoint]
int su2_readAeroLoad(char *filename, int *numVariable, char **variableName[], int *numDataPoint, double ***dataMatrix);

// Write SU2 surface motion file (connectivity is optional) - dataMatrix = [numVariable][numDataPoint], connectMatrix (optional) = [4*numConnect]
//  the formating of the data may be specified through dataFormat = [numVariable] (use capsTypes Integer and Double)- If NULL default to double
int su2_writeSurfaceMotion(char *filename,
                           int numVariable,
                           int numDataPoint,
                           double **dataMatrix,
                           int *dataFormat,
                           int numConnect,
                           int *connectMatrix);


// Write SU2 data transfer files
int su2_dataTransfer(void *aimInfo,
                     const char *analysisPath,
                     char *projectName,
                     meshStruct volumeMesh);

// Writes out surface indexes for a marker list
int su2_marker(void *aimInfo, const char* iname, capsValue *aimInputs, FILE *fp, cfdBCsStruct bcProps);

// Write SU2 configuration file for version Cardinal (4.0)
int su2_writeCongfig_Cardinal(void *aimInfo, const char *analysisPath, capsValue *aimInputs, cfdBCsStruct bcProps);

// Write SU2 configuration file for version Raven (5.0)
int su2_writeCongfig_Raven(void *aimInfo, const char *analysisPath, capsValue *aimInputs, cfdBCsStruct bcProps);

// Write SU2 configuration file for version Falcon (6.1)
int su2_writeCongfig_Falcon(void *aimInfo, const char *analysisPath, capsValue *aimInputs, cfdBCsStruct bcProps, int withMotion);

#ifdef __cplusplus
}
#endif
