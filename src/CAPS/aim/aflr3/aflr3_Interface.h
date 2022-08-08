// AFLR3 interface functions - Modified from functions provided with
//    AFLR3_LIB source (aflr3.c) written by David L. Marcum


enum aimInputs
{
  Proj_Name = 1,               /* index is 1-based */
  Mesh_Quiet_Flag,
  Mesh_Format,
  Mesh_ASCII_Flag,
  Mesh_Gen_Input_String,
  Multiple_Mesh,
  Mesh_Sizing,
  BL_Initial_Spacing,
  BL_Thickness,
  BL_Max_Layers,
  BL_Max_Layer_Diff,
  Surface_Mesh,
  NUMINPUT = Surface_Mesh     /* Total number of inputs */
};

#define AFLR3TESSFILE "aflr3_%d.eto"

extern
int aflr3_Volume_Mesh (void *aimInfo,
                       capsValue *aimInputs,
                       int ibodyOffset,
                       meshInputStruct meshInput,
                       const char *fileName,
                       int boundingBoxIndex,
                       int createBL,
                       double globalBLSpacing,
                       double globalBLThickness,
                       double capsMeshLength,
                       const mapAttrToIndexStruct *groupMap,
                       const mapAttrToIndexStruct *meshMap,
                       int numMeshProp,
                       meshSizingStruct *meshProp,
                       meshStruct *volumeMesh);
