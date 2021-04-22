#include "capsTypes.h" // Bring in CAPS types
#include "cfdTypes.h"  // Bring in CFD structures
#include "miscTypes.h"  // Bring in miscellaneous types

#ifdef __cplusplus
extern "C" {
#endif

// Fill bcProps in a cfdBCStruct format with boundary condition information from incoming BC Tuple
int cfd_getBoundaryCondition(int numTuple,
                             capsTuple bcTuple[],
                             mapAttrToIndexStruct *attrMap,
                             cfdBCsStruct *bcProps);

// Initiate (0 out all values and NULL all pointers) of surfaceProps in the cfdSurfaceStruct structure format
int intiate_cfdSurfaceStruct(cfdSurfaceStruct *surfaceProps);

// Destroy (0 out all values and NULL all pointers) of bcProps in the cfdBCsStruct structure format
int destroy_cfdSurfaceStruct(cfdSurfaceStruct *surfaceProps);

// Initiate (0 out all values and NULL all pointers) of bcProps in the cfdBCsStruct structure format
int initiate_cfdBCsStruct(cfdBCsStruct *bcProps);

// Destroy (0 out all values and NULL all pointers) of surfaceProps in the cfdSurfaceStruct structure format
int destroy_cfdBCsStruct(cfdBCsStruct *bcProps);

// Initiate (0 out all values and NULL all pointers) of eigenValue in the eigenValueStruct structure format
int initiate_eigenValueStruct(eigenValueStruct *eigenValue);

// Destroy (0 out all values and NULL all pointers) of eigenValue in the eigenValueStruct structure format
int destroy_eigenValueStruct(eigenValueStruct *eigenValue);

// Initiate (0 out all values and NULL all pointers) of modalAeroelastic in the modalAeroelasticStruct structure format
int initiate_modalAeroelasticStruct(modalAeroelasticStruct *modalAeroelastic);

// Destroy (0 out all values and NULL all pointers) of modalAeroelastic in the modalAeroelasticStruct structure format
int destroy_modalAeroelasticStruct(modalAeroelasticStruct *modalAeroelastic);

// Fill modalAeroelastic in a modalAeroelasticStruct format with modal aeroelastic data from Modal Tuple
int cfd_getModalAeroelastic(int numTuple,
                            capsTuple modalTuple[],
                            modalAeroelasticStruct *modalAeroelastic);

#ifdef __cplusplus
}
#endif
