#include "meshTypes.h"  // Bring in mesh structurs
#include "capsTypes.h"  // Bring in CAPS types
#include "cfdTypes.h"   // Bring in cfd structures

int aflr2_Surface_Mesh(int Message_Flag, ego bodyIn,
                       meshInputStruct meshInput,
                       mapAttrToIndexStruct attrMap,
                       int numMeshProp,
                       meshSizingStruct *meshProp,
                       meshStruct *surfaceMesh);
