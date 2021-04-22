#include "meshTypes.h"  // Bring in mesh structurs
#include "capsTypes.h"  // Bring in CAPS types
#include "cfdTypes.h"   // Bring in cfd structures

// AFLR4 interface function - Modified from functions provided with
//	AFLR4_LIB source (aflr4_cad_geom_setup.c) written by David L. Marcum

int aflr4_Surface_Mesh(int quiet,
                       int numBody, ego *bodies,
                       void *aimInfo, capsValue *aimInputs,
                       meshInputStruct meshInput,
                       mapAttrToIndexStruct attrMap,
                       meshStruct *surfaceMeshes);
