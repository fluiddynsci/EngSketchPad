// Write FEPOINT Tecplot data (compatible with FUN3D) (connectivity is optional) - dataMatrix = [numVariable][numDataPoint],
// connectMatrix (optional) = [4*numConnect], the formating of the data may be specified through
// dataFormat = [numVariable] (use capsTypes Integer and Double)- If NULL default is a double
int tecplot_writeFEPOINT(char *filename,
                         char *message,
                         char *zoneTitle,
                         int numVariable,
                         char *variableName[],
                         int numDataPoint,
                         double **dataMatrix,
                         int *dataFormat,
                         int numConnect,
                         int *connectMatrix,
                         double *solutionTime);
