/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             HSM AIM
 *
 *      Code written by Ryan Durscher  [AFRL/RQVC] and Bob Haimes [MIT]
 *
 */


/*!\mainpage Introduction
 *
 * \section overviewHSM HSM AIM Overview
 * A module in the Computational Aircraft Prototype Syntheses (CAPS) has been developed to interact with the
 * Hybrid Shell Model (HSM) code developed Mark Drela [MIT Department of Aeronautics & Astronautics].
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsHSM and \ref aimOutputsHSM, respectively.
 *
 *
 * Details of the AIM's shareable data structures are outlined in \ref sharableDataHSM if connecting this AIM to other AIMs in a
 * parent-child like manner.
 *
 */

//The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentHSM.
 // The HSM AIM provides the CAPS users with the ability to generate .......

// Header functions
#include <string.h>
#include <math.h>
#include "capsTypes.h"
#include "aimUtil.h"
#include "caps.h"


#include "meshUtils.h"    // Meshing utilities
#include "feaUtils.h"     // FEA utilities
#include "miscUtils.h"    // Miscellaneous utilities

#include "hsmUtils.h" // HSM utilities

#define HSM_RCM

#ifdef HSM_RCM
#include "rcm/genrcmi.hpp"
#endif

// Windows aliasing is ALL CAPS
#ifndef WIN32
#define HSMSOL hsmsol_
#define HSMDEP hsmdep_
#define HSMOUT hsmout_
#endif


/* FORTRAN logicals */
static int ffalse = 0, ftrue = 1;

extern void HSMSOL(int *lvinit, int *lprint,
                   int *lrcurv, int *ldrill,
                   int *itmax, double *rref,
                   double *rlim, double *rtol, double *rdel,
                   double *alim, double *atol, double *adel,
                   double *parg,
                   double *damem, double *rtolm,
                   int *nnode, double *pars, double *vars,
                   int *nvarg, double *varg,
                   int *nelem, int *kelem,
                   int *nbcedge, int *kbcedge, double *pare,
                   int *nbcnode, int *kbcnode, double *parp, int *lbcnode,
                   int *njoint, int *kjoint,
                   int *kdim, int *ldim, int *nedim, int *nddim, int *nmdim,
                   double *bf, double *bf_dj, double *bm, double *bm_dj,
                   int *ibr1, int *ibr2, int *ibr3, int *ibd1, int *ibd2, int *ibd3,
                   double *resc, double *resc_vars,
                   double *resp, double *resp_vars, double *resp_dvp,
                   int *kdvp, int *ndvp,
                   double *ares,
                   int *ifrst, int *ilast, int *mfrst,
                   /*@null@*/ double *amat, /*@null@*/ int *ipp, double *dvars);
extern void HSMDEP(int *leinit, int *lprint,
                   int *lrcurv, int *ldrill,
                   int *itmax,
                   double *elim, double *etol, double *edel,
                   int *nnode, double *par, double *var, double *dep,
                   int *nelem, int *kelem,
                   int *kdim, int *ldim, int *nedim, int *nddim, int *nmdim,
                   double *acn,
                   double *resn, double *rese, double *rese_de,
                   double *rest, double *rest_t,
                   int *kdt, int *ndt,
                   int *ifrstt, int *ilastt, int *mfrstt,
                   double *amatt,
                   double *resv, double *resv_v,
                   int *kdv, int *ndv,
                   int *ifrstv, int *ilastv, int *mfrstv,
                   double *amatv);
extern void HSMOUT(int *nelem, int *kelem, double *vars, double *deps, double *pars, double *parg,
                   int *kdim, int *ldim, int *nedim, int *nddim, int *nmdim,
                   int *idim, int *jdim, int *ni, int *nj, int *kij);

//#define DEBUG

static void
ortmat(double  e1, double  e2, double g12, double v12,
       double c16, double c26,
       double g13, double g23,
       double tsh, double zrf, double srfac,
       double  *a, double  *b, double  *d, double *s)
{

/* -------------------------------------------------------------------------
       Sets stiffness matrices A,B,D,S for an orthotropic shell,
       augmented with shear/extension coupling

       Inputs:
         e1    modulus in 1 direction
         e2    modulus in 2 direction
         g12   shear modulus
         v12   Poisson's ratio

         c16   12-shear / 1-extension coupling modulus
         c26   12-shear / 2-extension coupling modulus

         g13   1-direction transverse-shear modulus
         g23   2-direction transverse-shear modulus

         tsh   shell thickness
         zrf   reference surface location parameter -1 .. +1
         srfac transverse-shear strain energy reduction factor
                ( = 5/6 for isotropic shell)

      Outputs:
         a(.)   stiffness  tensor components  A11, A22, A12, A16, A26, A66
         b(.)   stiffness  tensor components  B11, B22, B12, B16, B26, B66
         d(.)   stiffness  tensor components  D11, D22, D12, D16, D26, D66
         s(.)   compliance tensor components  A55, A44
    ------------------------------------------------------------------------- */
  int    i;
  double den, tfac1, tfac2, tfac3, sfac, econ[6], scon[2];

  /* ---- in-plane stiffnesses  */
  den     = 1.0 - v12*v12 * e2/e1;
  econ[0] = e1/den;                /* c11 */
  econ[1] = e2/den;                /* c22 */
  econ[2] = e2/den * v12;          /* c12 */
  econ[3] = c16;                   /* c16 */
  econ[4] = c26;                   /* c26 */
  econ[5] = 2.0*g12;               /* c66 */

  /* ---- transverse shear stiffnesses */
  scon[0] = g13;                   /* c55 */
  scon[1] = g23;                   /* c44 */

  /*---- elements of in-plane stiffness matrices A,B,D for homogeneous shell */
  tfac1 =  tsh;
  tfac2 = -tsh*tsh     * zrf / 2.0;
  tfac3 =  tsh*tsh*tsh * (1.0 + 3.0*zrf*zrf) / 12.0;
  for (i = 0; i < 6; i++) {
    a[i] = econ[i] * tfac1;
    b[i] = econ[i] * tfac2;
    d[i] = econ[i] * tfac3;
  }

  sfac = tsh*srfac;
  s[0] = scon[0] * sfac;
  s[1] = scon[1] * sfac;
}


// Additional storage values for the instance of the AIM
typedef struct {

    // Project name
    char *projectName; // Project names

    // Analysis file path/directory
    const char *analysisPath;

    feaProblemStruct feaProblem;

    // Attribute to index map
    mapAttrToIndexStruct attrMap;

    // Attribute to constraint index map
    mapAttrToIndexStruct constraintMap;

    // Attribute to load index map
    mapAttrToIndexStruct loadMap;

    // Attribute to transfer map
    mapAttrToIndexStruct transferMap;

    // Pointer to caps input value for tessellation parameters
    capsValue *tessParam;

    // Pointer to caps input value for Edge_Point- Min
    capsValue *edgePointMin;

    // Pointer to caps input value for Edge_Point- Max
    capsValue *edgePointMax;

    // Pointer to caps input value for No_Quad_Faces
    capsValue *quadMesh;

    // Mesh holders
    int numMesh;
    meshStruct *feaMesh;

} aimStorage;

static aimStorage *hsmInstance = NULL;
static int         numInstance  = 0;

static int initiate_aimStorage(int iIndex) {

    int status;

    // Set initial values for hsmInstance
    hsmInstance[iIndex].projectName = NULL;

    // Analysis file path/directory
    hsmInstance[iIndex].analysisPath = NULL;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&hsmInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to constraint index map
    status = initiate_mapAttrToIndexStruct(&hsmInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to load index map
    status = initiate_mapAttrToIndexStruct(&hsmInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for transfer to index map
    status = initiate_mapAttrToIndexStruct(&hsmInstance[iIndex].transferMap);
    if (status != CAPS_SUCCESS) return status;

    status = initiate_feaProblemStruct(&hsmInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS) return status;

    // Pointer to caps input value for tessellation parameters
    hsmInstance[iIndex].tessParam = NULL;

    // Pointer to caps input value for Edge_Point- Min
    hsmInstance[iIndex].edgePointMin = NULL;

    // Pointer to caps input value for Edge_Point- Max
    hsmInstance[iIndex].edgePointMax = NULL;

    // Pointer to caps input value for No_Quad_Faces
    hsmInstance[iIndex].quadMesh = NULL;

    // Mesh holders
    hsmInstance[iIndex].numMesh = 0;
    hsmInstance[iIndex].feaMesh = NULL;

    return CAPS_SUCCESS;
}

static int destroy_aimStorage(int iIndex) {

    int status;
    int i;
    // Attribute to index map
    status = destroy_mapAttrToIndexStruct(&hsmInstance[iIndex].attrMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to constraint index map
    status = destroy_mapAttrToIndexStruct(&hsmInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Attribute to load index map
    status = destroy_mapAttrToIndexStruct(&hsmInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Transfer to index map
    status = destroy_mapAttrToIndexStruct(&hsmInstance[iIndex].transferMap);
    if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_mapAttrToIndexStruct!\n", status);

    // Cleanup meshes
    if (hsmInstance[iIndex].feaMesh != NULL) {

        for (i = 0; i < hsmInstance[iIndex].numMesh; i++) {
            status = destroy_meshStruct(&hsmInstance[iIndex].feaMesh[i]);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_meshStruct!\n", status);
        }

        EG_free(hsmInstance[iIndex].feaMesh);
    }

    hsmInstance[iIndex].feaMesh = NULL;
    hsmInstance[iIndex].numMesh = 0;

    // Destroy FEA problem structure
    status = destroy_feaProblemStruct(&hsmInstance[iIndex].feaProblem);
    if (status != CAPS_SUCCESS)  printf("Error: Status %d during destroy_feaProblemStruct!\n", status);

    // NULL projetName
    hsmInstance[iIndex].projectName = NULL;

    // NULL analysisPath
    hsmInstance[iIndex].analysisPath = NULL;

    // NULL pointer to caps input value for tessellation parameters
    hsmInstance[iIndex].tessParam = NULL;

    // NULL pointer to caps input value for Edge_Point- Min
    hsmInstance[iIndex].edgePointMin = NULL;

    // NULL pointer to caps input value for Edge_Point- Max
    hsmInstance[iIndex].edgePointMax = NULL;

    // NULL pointer to caps input value for No_Quad_Faces
    hsmInstance[iIndex].quadMesh = NULL;

    return CAPS_SUCCESS;
}

static int createMesh(int iIndex, void *aimInfo) {

    int status; // Function return status

        int i, body; // Indexing

    // Bodies
        const char *intents;
     int   numBody; // Number of Bodies
    ego  *bodies;

    // Meshing related variables
    double  tessParam[3];
    int edgePointMin = 3;
    int edgePointMax = 50;
    int quadMesh = (int) false;

    // Coordinate system
    mapAttrToIndexStruct coordSystemMap;

    // AIM Data transfer
    int  nrow, ncol, rank;
    void *dataTransfer;
    enum capsvType vtype;
    char *units;

    meshStruct *tempMesh;
    int *feaMeshList = NULL; // List to seperate structural meshes for aero

    // Inherited fea/volume mesh related variables
    int numFEAMesh = 0;
    int feaMeshInherited = (int) false;

    // Destroy feaMeshes
    if (hsmInstance[iIndex].feaMesh != NULL) {

        for (i = 0; i < hsmInstance[iIndex].numMesh; i++) {
            status = destroy_meshStruct(&hsmInstance[iIndex].feaMesh[i]);
            if (status != CAPS_SUCCESS) printf("Error: Status %d during destroy_meshStruct!\n", status);
        }

        EG_free(hsmInstance[iIndex].feaMesh);
    }

    hsmInstance[iIndex].feaMesh = NULL;
    hsmInstance[iIndex].numMesh = 0;

    // Get AIM bodies
    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) return status;

    #ifdef DEBUG
           printf(" nastranAIM/createMesh instance = %d  nbody = %d!\n", iIndex, numBody);
    #endif

    if ((numBody <= 0) || (bodies == NULL)) {
        printf(" nastranAIM/createMesh No Bodies!\n");

        return CAPS_SOURCEERR;
    }

    // Alloc feaMesh list
    feaMeshList = (int *) EG_alloc(numBody*sizeof(int));
    if (feaMeshList == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    // Set all indexes to true
    for (i = 0; i < numBody; i++ ) feaMeshList[i] = (int) true;


    // Get CoordSystem attribute to index mapping
    status = initiate_mapAttrToIndexStruct(&coordSystemMap);
    if (status != CAPS_SUCCESS) return status;

    status = create_CoordSystemAttrToIndexMap(numBody,
                                              bodies,
                                              3, //>2 - search the body, faces, edges, and all the nodes
                                              &coordSystemMap);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = fea_getCoordSystem(numBody,
                                bodies,
                                coordSystemMap,
                                &hsmInstance[iIndex].feaProblem.numCoordSystem,
                                &hsmInstance[iIndex].feaProblem.feaCoordSystem);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get capsConstraint name and index mapping
    status = create_CAPSConstraintAttrToIndexMap(numBody,
                                                 bodies,
                                                 3, //>2 - search the body, faces, edges, and all the nodes
                                                 &hsmInstance[iIndex].constraintMap);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get capsLoad name and index mapping
    status = create_CAPSLoadAttrToIndexMap(numBody,
                                           bodies,
                                           3, //>2 - search the body, faces, edges, and all the nodes
                                           &hsmInstance[iIndex].loadMap);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get transfer to index mapping
    status = create_CAPSBoundAttrToIndexMap(numBody,
                                            bodies,
                                            3, //>2 - search the body, faces, edges, and all the nodes
                                            &hsmInstance[iIndex].transferMap);
    if (status != CAPS_SUCCESS) goto cleanup;

    /*// Get connect to index mapping
    status = create_CAPSConnectAttrToIndexMap(numBody,
                                               bodies,
                                               3, //>2 - search the body, faces, edges, and all the nodes
                                               &hsmInstance[iIndex].connectMap);
    if (status != CAPS_SUCCESS) goto cleanup;*/

    // Get attribute to index mapping
    status = aim_getData(aimInfo, "Attribute_Map", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
    if (status == CAPS_SUCCESS) {

        printf("Found link for attrMap (Attribute_Map) from parent\n");

        status = copy_mapAttrToIndexStruct( (mapAttrToIndexStruct *) dataTransfer, &hsmInstance[iIndex].attrMap);
        if (status != CAPS_SUCCESS) goto cleanup;

    } else {

        if (status == CAPS_DIRTY) printf("Parent AIMS are dirty\n");

        printf("Didn't find a link to attrMap from parent - getting it ourselves\n");

        // Get capsGroup name and index mapping to make sure all faces have a capsGroup value
        status = create_CAPSGroupAttrToIndexMap(numBody,
                                                bodies,
                                                3, //>2 - search the body, faces, edges, and all the nodes
                                                &hsmInstance[iIndex].attrMap);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // See if a FEA mesh is available from parent
    status = aim_getData(aimInfo, "FEA_MESH", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
    if (status == CAPS_SUCCESS) {

        printf("Found link for a FEA mesh (FEA_Mesh) from parent\n");

        numFEAMesh = 1;
        if      (nrow == 1 && ncol != 1) numFEAMesh = ncol;
        else if (nrow != 1 && ncol == 1) numFEAMesh = nrow;
        else {

            printf("Can not accept 2D matrix of fea meshes\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        if (numFEAMesh != numBody) {
            printf("Number of inherited fea meshes does not match the number of bodies\n");
            return CAPS_SOURCEERR;
        }

        if (numFEAMesh > 0) printf("Combining multiple FEA meshes!\n");
         status = mesh_combineMeshStruct(numFEAMesh,
                                        (meshStruct *) dataTransfer,
                                        &hsmInstance[iIndex].feaProblem.feaMesh);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Set reference meshes
        hsmInstance[iIndex].feaProblem.feaMesh.numReferenceMesh = numFEAMesh;
        hsmInstance[iIndex].feaProblem.feaMesh.referenceMesh = (meshStruct *) EG_alloc(numFEAMesh*sizeof(meshStruct));
        if (hsmInstance[iIndex].feaProblem.feaMesh.referenceMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (body = 0; body < numFEAMesh; body++) {
            hsmInstance[iIndex].feaProblem.feaMesh.referenceMesh[body] = ((meshStruct *) dataTransfer)[body];
        }

        feaMeshInherited = (int) true;

    } else if (status == CAPS_DIRTY) {

        printf("Link for a FEA mesh (FEA_Mesh) from parent found, but Parent AIMS are dirty\n");
        goto cleanup;

    } else { // Check to see if a general unstructured volume mesh is available

        printf("Didn't find a FEA mesh (FEA_Mesh) from parent - checking for volume mesh\n");

        status = aim_getData(aimInfo, "Volume_MESH", &vtype, &rank, &nrow, &ncol, &dataTransfer, &units);
        if (status == CAPS_DIRTY) {

            printf("Link for a volume mesh (Volume_Mesh) from parent found, but Parent AIMS are dirty\n");
            goto cleanup;

        } else if (status == CAPS_SUCCESS) {

            printf("Found link for a  volume mesh (Volume_Mesh) from parent\n");

            numFEAMesh = 1;
            if (nrow != 1 && ncol != 1) {

                printf("Can not accept multiple volume meshes\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }

            if (numFEAMesh != numBody) {
                printf("Number of inherited volume meshes does not match the number of bodies - assuming volume mesh is already combined\n");
            }

            hsmInstance[iIndex].feaProblem.feaMesh = *(meshStruct *) dataTransfer;

            for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numReferenceMesh; i++) {

                status = aim_setTess(aimInfo,
                                     hsmInstance[iIndex].feaProblem.feaMesh.referenceMesh[i].bodyTessMap.egadsTess);
                if (status != CAPS_SUCCESS) goto cleanup;
            }

            feaMeshInherited = (int) true;
        } else {
            printf("Didn't find a volume mesh (Volume_Mesh) from parent\n");

            feaMeshInherited = (int) false;
        }
    }


    // If we didn't inherit a FEA mesh we need to get one ourselves
    if (feaMeshInherited == (int) false) {

        tessParam[0] = hsmInstance[iIndex].tessParam->vals.reals[0]; // Gets multiplied by bounding box size
        tessParam[1] = hsmInstance[iIndex].tessParam->vals.reals[1]; // Gets multiplied by bounding box size
        tessParam[2] = hsmInstance[iIndex].tessParam->vals.reals[2];

        edgePointMin = hsmInstance[iIndex].edgePointMin->vals.integer;
        edgePointMax = hsmInstance[iIndex].edgePointMax->vals.integer;
        quadMesh     = hsmInstance[iIndex].quadMesh->vals.integer;

        if (edgePointMin < 2) {
            printf("The minimum number of allowable edge points is 2 not %d\n", edgePointMin);
            edgePointMin = 2;
        }

        if (edgePointMax < edgePointMin) {
            printf("The maximum number of edge points must be greater than the current minimum (%d)\n", edgePointMin);
            edgePointMax = edgePointMin+1;
        }

        for (body = 0; body < numBody; body++) {
            if (feaMeshList[body] != (int) true) continue;

            hsmInstance[iIndex].numMesh += 1;
            if (hsmInstance[iIndex].numMesh == 1) {
                tempMesh = (meshStruct *) EG_alloc(sizeof(meshStruct));
            } else {
                tempMesh= (meshStruct *) EG_reall(hsmInstance[iIndex].feaMesh,
                                                  hsmInstance[iIndex].numMesh*sizeof(meshStruct));
            }
            if (tempMesh == NULL) {
                status = EGADS_MALLOC;
                hsmInstance[iIndex].numMesh -= 1;
                goto cleanup;
            }

            hsmInstance[iIndex].feaMesh = tempMesh;

            status = initiate_meshStruct(&hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1]);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Get FEA Problem from EGADs body
            status = hsm_bodyToBEM(bodies[body], // (in)  EGADS Body
                                   tessParam,    // (in)  Tessellation parameters
                                   edgePointMin, // (in)  minimum points along any Edge
                                   edgePointMax, // (in)  maximum points along any Edge
                                   quadMesh,
                                   &hsmInstance[iIndex].attrMap,
                                   &coordSystemMap,
                                   &hsmInstance[iIndex].constraintMap,
                                   &hsmInstance[iIndex].loadMap,
                                   &hsmInstance[iIndex].transferMap,
                                   NULL, //&hsmInstance[iIndex].connectMap,
                                   &hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1]);

            if (status != CAPS_SUCCESS) goto cleanup;

            printf("\tNumber of nodal coordinates = %d\n", hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1].numNode);
            printf("\tNumber of elements = %d\n", hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1].numElement);
            //printf("\tElemental Nodes = %d\n", hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1].meshQuickRef.numNode);
            //printf("\tElemental Rods  = %d\n", hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1].meshQuickRef.numLine);
            printf("\tElemental Tria3 = %d\n", hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1].meshQuickRef.numTriangle);
            printf("\tElemental Quad4 = %d\n", hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1].meshQuickRef.numQuadrilateral);

            status = aim_setTess(aimInfo, hsmInstance[iIndex].feaMesh[hsmInstance[iIndex].numMesh-1].bodyTessMap.egadsTess);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        if (hsmInstance[iIndex].numMesh > 1) printf("Combining multiple FEA meshes!\n");
        // Combine fea meshes into a single mesh for the problem
        status = mesh_combineMeshStruct(hsmInstance[iIndex].numMesh,
                                        hsmInstance[iIndex].feaMesh,
                                        &hsmInstance[iIndex].feaProblem.feaMesh);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Set reference meshes
        hsmInstance[iIndex].feaProblem.feaMesh.numReferenceMesh = hsmInstance[iIndex].numMesh;
        hsmInstance[iIndex].feaProblem.feaMesh.referenceMesh = (meshStruct *) EG_alloc(hsmInstance[iIndex].numMesh*sizeof(meshStruct));
        if (hsmInstance[iIndex].feaProblem.feaMesh.referenceMesh == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (body = 0; body < hsmInstance[iIndex].numMesh; body++) {
            hsmInstance[iIndex].feaProblem.feaMesh.referenceMesh[body] = hsmInstance[iIndex].feaMesh[body];
        }
    }
    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in createMesh status = %d\n", status);

        if (feaMeshList != NULL) EG_free(feaMeshList);

        (void ) destroy_mapAttrToIndexStruct(&coordSystemMap);

        return status;
}




// ********************** Exposed AIM Functions *****************************

int aimInitialize(int ngIn, capsValue *gIn, int *qeFlag, const char *unitSys,
                  int *nIn, int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int status; // Function return

    int flag;  // query/execute flag

    aimStorage *tmp;

    #ifdef DEBUG
            printf("\n hsmAIM/aimInitialize   ngIn = %d!\n", ngIn);
    #endif

    flag    = *qeFlag;

    // Does the AIM execute itself (i.e. no external executable is called)
    *qeFlag = 1; // 1 = AIM executes itself, 0 otherwise

    // specify the number of analysis input and out "parameters"
    *nIn     = 9;
    *nOut    = 0;
    if (flag == 1) return CAPS_SUCCESS;

    // Specify the field variables this analysis can generate
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Allocate hsmInstance
    if (hsmInstance == NULL) {
        hsmInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (hsmInstance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(hsmInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        hsmInstance = tmp;
    }

    // Set initial values for hsmInstance

    status = initiate_aimStorage(numInstance);
    if (status != CAPS_SUCCESS) return status;

    // Increment number of instances
    numInstance += 1;

    return (numInstance-1);
}

// Available AIM inputs
int aimInputs(int iIndex, void *aimInfo, int index, char **ainame,
              capsValue *defval)
{

    /*! \page aimInputsHSM AIM Inputs
     * The following list outlines the HSM options along with their default value available
     * through the AIM interface.
     */

    #ifdef DEBUG
        printf(" hsmAIM/aimInputs instance = %d  index = %d!\n", iIndex, index);
    #endif

    // Inputs
    if (index == 1) {
        *ainame              = EG_strdup("Proj_Name");
         defval->type         = String;
         defval->nullVal      = NotNull;
        defval->vals.string  = EG_strdup("hsm_CAPS");
        defval->lfixed       = Change;

        /*! \page aimInputsHSM
         * - <B> Proj_Name = "hsm_CAPS"</B> <br>
         * This corresponds to the project name used for file naming.
         */

    } else if (index == 2) {
        *ainame               = EG_strdup("Tess_Params");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->length        = 3;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->lfixed        = Fixed;
        defval->vals.reals    = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals != NULL) {
            defval->vals.reals[0] = 0.025;
            defval->vals.reals[1] = 0.001;
            defval->vals.reals[2] = 15.00;
        } else return EGADS_MALLOC;

        hsmInstance[iIndex].tessParam = defval;

        /*! \page aimInputsHSM
         * - <B> Tess_Params = [0.025, 0.001, 15.0]</B> <br>
         * Body tessellation parameters used when creating a boundary element model.
         * Tess_Params[0] and Tess_Params[1] get scaled by the bounding
         * box of the body. (From the EGADS manual) A set of 3 parameters that drive the EDGE discretization
         * and the FACE triangulation. The first is the maximum length of an EDGE segment or triangle side
         * (in physical space). A zero is flag that allows for any length. The second is a curvature-based
         * value that looks locally at the deviation between the centroid of the discrete object and the
         * underlying geometry. Any deviation larger than the input value will cause the tessellation to
         * be enhanced in those regions. The third is the maximum interior dihedral angle (in degrees)
         * between triangle facets (or Edge segment tangents for a WIREBODY tessellation), note that a
         * zero ignores this phase
         */

    } else if (index == 3) {
        *ainame               = EG_strdup("Edge_Point_Min");
        defval->type          = Integer;
        defval->vals.integer  = 2;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        hsmInstance[iIndex].edgePointMin = defval;

        /*! \page aimInputsHSM
         * - <B> Edge_Point_Min = 2</B> <br>
         * Minimum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 4) {
        *ainame               = EG_strdup("Edge_Point_Max");
        defval->type          = Integer;
        defval->vals.integer  = 50;
        defval->length        = 1;
        defval->lfixed        = Fixed;
        defval->nrow          = 1;
        defval->ncol          = 1;
        defval->nullVal       = NotNull;

        hsmInstance[iIndex].edgePointMax = defval;

        /*! \page aimInputsHSM
         * - <B> Edge_Point_Max = 50</B> <br>
         * Maximum number of points on an edge including end points to use when creating a surface mesh (min 2).
         */

    } else if (index == 5) {
        *ainame               = EG_strdup("Quad_Mesh");
        defval->type          = Boolean;
        defval->vals.integer  = (int) false;

        hsmInstance[iIndex].quadMesh = defval;

        /*! \page aimInputsHSM
         * - <B> Quad_Mesh = False</B> <br>
         * Create a quadratic mesh on four edge faces when creating the boundary element model.
         */

    } else if (index == 6) {
        *ainame              = EG_strdup("Property");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsHSM
         * - <B> Property = NULL</B> <br>
         * Property tuple used to input property information for the model, see \ref feaProperty for additional details.
         */
    } else if (index == 7) {
        *ainame              = EG_strdup("Material");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsHSM
         * - <B> Material = NULL</B> <br>
         * Material tuple used to input material information for the model, see \ref feaMaterial for additional details.
         */
    } else if (index == 8) {
        *ainame              = EG_strdup("Constraint");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsHSM
         * - <B> Constraint = NULL</B> <br>
         * Constraint tuple used to input constraint information for the model, see \ref feaConstraint for additional details.
         */
    } else if (index == 9) {
        *ainame              = EG_strdup("Load");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;
        defval->dim          = Vector;

        /*! \page aimInputsHSM
         * - <B> Load = NULL</B> <br>
         * Load tuple used to input load information for the model, see \ref feaLoad for additional details.
         */
    }

    return CAPS_SUCCESS;
}

// Shareable data for the AIM - typically optional
int aimData(int iIndex, const char *name, enum capsvType *vtype,
            int *rank, int *nrow, int *ncol, void **data, char **units)
{
    /*! \page sharableDataHSM AIM Shareable Data
     * The HSM AIM has the following shareable data types/values with its children AIMs if they are so inclined.
     * - None
     */

    #ifdef DEBUG
        printf(" hsmAIM/aimData instance = %d  name = %s!\n", inst, name);
    #endif

    return CAPS_NOTFOUND;

}

// AIM preAnalysis function
int aimPreAnalysis(int iIndex, void *aimInfo,  const char *analysisPath, capsValue *aimInputs, capsErrs **errs)
{
    int status; // Status return

    int i, j, k, propertyIndex, materialIndex; // Indexing

    char *filename = NULL; // Output file

    //int numConnect; // Number of connections for element of a given type

    // RCM variables
    //int numOpenSeg;
    int *elementConnect=NULL, *permutation=NULL, *permutationInv=NULL, *openSeg=NULL;

    // HSM input/output parameters
    int kdim,ldim,nedim,nddim,nmdim;
    int lrcurv, ldrill;

    int itmax;
    double rref, elim, etol, edel;
    double rlim, rtol, rdel;
    double alim, atol, adel;
    double damem, rtolm;

    // global variables
    int nvarg = 0;
    double varg[1];

    // HSM memory
    int numBCEdge = 0, numBCNode = 0, numJoint = 0, maxDim = 0;
    int hsmNumElement = 0;
    int type, topoIndex;
    int maxAdjacency = 0;
    int *xadj = NULL, *adj = NULL, *kjoint = NULL;

    hsmMemoryStruct hsmMemory;
    hsmTempMemoryStruct hsmTempMemory;

    // Temp storage
    feaMeshDataStruct *feaData; // Not freeable
    meshElementStruct *element; // Not freeable
    meshNodeStruct *node; // Not freeable

    // Setting variables
    double membraneThickness, shearMembraneRatio, refLocation, massPerArea;
    double youngModulus; // E - Young's Modulus [Longitudinal if distinction is made]
    double youngModulusLateral; // Young's Modulus in the lateral direction
    double shearModulus; // G - Shear Modulus
    double poissonRatio; // Poisson's Ratio
    double shearModulusTrans1Z; // Transverse shear modulus in the 1-Z plane
    double shearModulusTrans2Z; // Transverse shear modulus in the 2-Z plane

    double normalize, norm[3];

    int found; // Checker

    // NULL out errors
    *errs = NULL;

    // Newton convergence tolerances
    rtol = 1.0e-11;  /* relative displacements, |d|/dref */
    atol = 1.0e-11;  /* angles (unit vector changes) */

    /* reference length for displacement limiting, convergence checks
         (should be comparable to size of the geometry) */
    rref = 1.0;

    // max Newton changes (dimensionless)
    rlim = 0.5;
    alim = 0.5;

    // d,psi change threshold to trigger membrane-only sub-iterations
    damem = 0.025;

    // displacement convergence tolerance for membrane-only sub-iterations
    rtolm = 1.0e-4;

    //ddel    last max position Newton delta  (fraction of dref)
    //adel    last max normal angle Newton delta

    // Store away the analysis path/directory
    hsmInstance[iIndex].analysisPath = analysisPath;

    // Get project name
    hsmInstance[iIndex].projectName = aimInputs[aim_getIndex(aimInfo, "Proj_Name", ANALYSISIN)-1].vals.string;

    status = initiate_hsmMemoryStruct(&hsmMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = initiate_hsmTempMemoryStruct(&hsmTempMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get FEA mesh if we don't already have one
    if (aim_newGeometry(aimInfo) == CAPS_SUCCESS) {

        status = createMesh(iIndex, aimInfo);
        if (status != CAPS_SUCCESS) goto cleanup;

        // Write Nastran Mesh
        filename = (char *) EG_alloc((strlen(analysisPath) +1 + strlen(hsmInstance[iIndex].projectName)+1)*sizeof(char));
        if (filename == NULL) return EGADS_MALLOC;

        strcpy(filename, analysisPath);
        #ifdef WIN32
            strcat(filename, "\\");
        #else
            strcat(filename, "/");
        #endif

        strcat(filename, hsmInstance[iIndex].projectName);

        status = mesh_writeNASTRAN(filename,
                                   1,
                                   &hsmInstance[iIndex].feaProblem.feaMesh,
                                   SmallField,
                                   1.0);
        if (status != CAPS_SUCCESS) {
            EG_free(filename);
            return status;
        }
    }

    // Note: Setting order is important here.
    // 1. Materials should be set before properties.
    // 2. Coordinate system should be set before mesh and loads
    // 3. Mesh should be set before loads, constraints

    // Set material properties
    if (aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getMaterial(aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Material", ANALYSISIN)-1].vals.tuple,
                                 &hsmInstance[iIndex].feaProblem.numMaterial,
                                 &hsmInstance[iIndex].feaProblem.feaMaterial);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Load tuple is NULL - No materials set\n");

    // Set property properties
    if (aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getProperty(aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].length,
                                 aimInputs[aim_getIndex(aimInfo, "Property", ANALYSISIN)-1].vals.tuple,
                                 &hsmInstance[iIndex].attrMap,
                                 &hsmInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;


        // Assign element "subtypes" based on properties set
/*        status = fea_assignElementSubType(hsmInstance[iIndex].feaProblem.numProperty,
                                          hsmInstance[iIndex].feaProblem.feaProperty,
                                          &hsmInstance[iIndex].feaProblem.feaMesh);
        if (status != CAPS_SUCCESS) return status;*/

    } else printf("Property tuple is NULL - No properties set\n");

    // Set constraint properties
    if (aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getConstraint(aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].length,
                                   aimInputs[aim_getIndex(aimInfo, "Constraint", ANALYSISIN)-1].vals.tuple,
                                   &hsmInstance[iIndex].constraintMap,
                                   &hsmInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;
    } else printf("Constraint tuple is NULL - No constraints applied\n");


    // Set load properties
    if (aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].nullVal == NotNull) {
        status = fea_getLoad(aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].length,
                             aimInputs[aim_getIndex(aimInfo, "Load", ANALYSISIN)-1].vals.tuple,
                             &hsmInstance[iIndex].loadMap,
                             &hsmInstance[iIndex].feaProblem);
        if (status != CAPS_SUCCESS) return status;

        // Loop through loads to see if any of them are supposed to be from an external source
        for (i = 0; i < hsmInstance[iIndex].feaProblem.numLoad; i++) {

            if (hsmInstance[iIndex].feaProblem.feaLoad[i].loadType == PressureExternal) {

                // Transfer external pressures from the AIM discrObj
                status = fea_transferExternalPressure(aimInfo,
                                                      &hsmInstance[iIndex].feaProblem.feaMesh,
                                                      &hsmInstance[iIndex].feaProblem.feaLoad[i]);
                if (status != CAPS_SUCCESS) return status;

            } // End PressureExternal if
        } // End load for loop
    } else printf("Load tuple is NULL - No loads applied\n");


    // Count the number joints
    for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numNode; i++) {

        node = &hsmInstance[iIndex].feaProblem.feaMesh.node[i];

        if (node->geomData == NULL) {
            printf("No geometry data set for node %d\n", node->nodeID);
            status = CAPS_NULLVALUE;
            goto cleanup;
        }

        type = node->geomData->type;
        topoIndex = node->geomData->topoIndex;

        if ( type < 0) continue;

        for (j = 0; j < i; j++) {

            if (hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData == NULL) {
                printf("No geometry data set for node %d\n", hsmInstance[iIndex].feaProblem.feaMesh.node[j].nodeID);
                status = CAPS_NULLVALUE;
                goto cleanup;
            }

            if (hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData->type < 0) continue;

            if (type == hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData->type  &&
                topoIndex == hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData->topoIndex) {

                numJoint += 1;
                break; // one point can only connect to one other point
            }
        }
    }

    kjoint = (int *) EG_alloc(2*numJoint*sizeof(int));

    // Fill in the joints
    numJoint = 0;
    for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numNode; i++) {

        node = &hsmInstance[iIndex].feaProblem.feaMesh.node[i];

        if (node->geomData == NULL) {
            printf("No geometry data set for node %d\n",node->nodeID);
            status = CAPS_NULLVALUE;
            goto cleanup;
        }

        type = node->geomData->type;
        topoIndex = node->geomData->topoIndex;

        if ( type < 0) continue;

        // only searching lower nodes prevents creating a complete cyclic joint
        for (j = 0; j < i; j++) {

            if (hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData == NULL) {
                printf("No geometry data set for node %d\n", hsmInstance[iIndex].feaProblem.feaMesh.node[j].nodeID);
                status = CAPS_NULLVALUE;
                goto cleanup;
            }

            if (hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData->type < 0) continue;

            if (type == hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData->type  &&
                topoIndex == hsmInstance[iIndex].feaProblem.feaMesh.node[j].geomData->topoIndex) {

                kjoint[2*numJoint + 0] = i;
                kjoint[2*numJoint + 1] = j;
//printf("%d (%d, %d)\n", numJoint, i, j);
                numJoint += 1;
                break; // one point can only connect to one other point
            }
        }
    }

    status = hsm_Adjacency(&hsmInstance[iIndex].feaProblem.feaMesh, numJoint, kjoint, &maxAdjacency, &xadj, &adj);

    printf("Max Adjacency set to = %d\n", maxAdjacency);

    // Count the number of BC nodes - for Constraints
    for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numNode; i++) {

        feaData = (feaMeshDataStruct *) hsmInstance[iIndex].feaProblem.feaMesh.node[i].analysisData;
        if (feaData == NULL) {
            status = CAPS_NULLVALUE;
            goto cleanup;
        }

        if (feaData->constraintIndex != CAPSMAGIC && feaData->constraintIndex != 0)
          numBCNode += 1;
    }


    // Count the number of BC Edges - for Loads
    for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numElement; i++) {

        element = &hsmInstance[iIndex].feaProblem.feaMesh.element[i];

        if (element->elementType != Line) continue;

        feaData = (feaMeshDataStruct *) element->analysisData;
        if (feaData == NULL) {
            status = CAPS_NULLVALUE;
            goto cleanup;
        }

        if (feaData->loadIndex != CAPSMAGIC && feaData->loadIndex != 0)
            numBCEdge +=1;
    }


    // Max dimension
    maxDim = numBCEdge;
    if (maxDim < numBCNode) maxDim = numBCNode;
    if (maxDim < numJoint)  maxDim = numJoint;
    if (maxDim == 0) maxDim = 1;

    //maxDim += 1;

    printf("MaxDim = %d, numBCEdge = %d, numBCNode = %d, numJoint = %d\n", maxDim, numBCEdge, numBCNode, numJoint);

    hsmNumElement = hsmInstance[iIndex].feaProblem.feaMesh.meshQuickRef.numTriangle
                  + hsmInstance[iIndex].feaProblem.feaMesh.meshQuickRef.numQuadrilateral;

#ifdef HSM_RCM
    permutation = (int *) EG_alloc(hsmInstance[iIndex].feaProblem.feaMesh.numNode*sizeof(int));

    genrcmi(hsmInstance[iIndex].feaProblem.feaMesh.numNode,
            xadj[hsmInstance[iIndex].feaProblem.feaMesh.numNode]-1,
            xadj, adj,
            permutation);

    EG_free(xadj); xadj = NULL;
    EG_free(adj);  adj = NULL;

    // Convert to 1-based indexing
    //for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numNode; i++)
    //  permutation[i]++;

#else
    EG_free(xadj); xadj = NULL;
    EG_free(adj);  adj = NULL;

    permutation    = (int *) EG_alloc(hsmInstance[iIndex].feaProblem.feaMesh.numNode*sizeof(int));

    // Set to identity
    for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numNode; i++)
      permutation[i] = i+1;
#endif

    // Allocate of HSM memory
    status = allocate_hsmMemoryStruct(hsmInstance[iIndex].feaProblem.feaMesh.numNode,
                                      hsmNumElement,
                                      maxDim, //, numBCEdge, numBCNode, numJoint);
                                      &hsmMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = allocate_hsmTempMemoryStruct(hsmInstance[iIndex].feaProblem.feaMesh.numNode,
                                          maxAdjacency,
                                          maxDim,
                                          &hsmTempMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    /////  Populate HSM inputs /////

    // Connectivity
    for (j = i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numElement; i++) {

        element = &hsmInstance[iIndex].feaProblem.feaMesh.element[i];

        if (element->elementType == Line) continue;

        if (element->elementType == Triangle) {

            hsmMemory.kelem[4*j+0] = permutation[element->connectivity[0]-1];
            hsmMemory.kelem[4*j+1] = permutation[element->connectivity[1]-1];
            hsmMemory.kelem[4*j+2] = permutation[element->connectivity[2]-1];
            hsmMemory.kelem[4*j+3] = 0;

        } else if (element->elementType == Quadrilateral) {
            hsmMemory.kelem[4*j+0] = permutation[element->connectivity[0]-1];
            hsmMemory.kelem[4*j+1] = permutation[element->connectivity[1]-1];
            hsmMemory.kelem[4*j+2] = permutation[element->connectivity[2]-1];
            hsmMemory.kelem[4*j+3] = permutation[element->connectivity[3]-1];

        } else {
            printf("Invalid element type!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }
        j++;
    }

    // set the joints with the permutation
    for (j = 0; j < numJoint; j++) {
        hsmMemory.kjoint[2*j + 0] = permutation[kjoint[2*j+0]];
        hsmMemory.kjoint[2*j + 1] = permutation[kjoint[2*j+1]];
    }
    EG_free(kjoint); kjoint = NULL;

#if 1
    // Node parameters
    for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numNode; i++) {

        node = &hsmInstance[iIndex].feaProblem.feaMesh.node[i];
        feaData = (feaMeshDataStruct *) node->analysisData;

        if (feaData == NULL) {
            printf("No FEA data set for node %d\n", node->nodeID);
            status = CAPS_NULLVALUE;
            goto cleanup;
        }

        if (node->geomData == NULL) {
            printf("Geometry data not set for node %d\n", node->nodeID);
            status = CAPS_NULLVALUE;
            goto cleanup;
        }

        k = permutation[i] - 1;

        hsmMemory.pars[k*LVTOT + lvr0x] = node->xyz[0]; // x
        hsmMemory.pars[k*LVTOT + lvr0y] = node->xyz[1]; // y
        hsmMemory.pars[k*LVTOT + lvr0z] = node->xyz[2]; // z

        normalize = dot_DoubleVal(node->geomData->firstDerivative, node->geomData->firstDerivative);
        normalize = sqrt(normalize);

        if (normalize == 0.0) {
            printf(" Node %d e0_1 is degenerate!\n", node->nodeID);
            normalize = 1.0;
        }

        hsmMemory.pars[k*LVTOT + lve01x] = node->geomData->firstDerivative[0]/normalize; // e_1 - x
        hsmMemory.pars[k*LVTOT + lve01y] = node->geomData->firstDerivative[1]/normalize; // e_1 - y
        hsmMemory.pars[k*LVTOT + lve01z] = node->geomData->firstDerivative[2]/normalize; // e_1 - z

        normalize = dot_DoubleVal(&node->geomData->firstDerivative[3], &node->geomData->firstDerivative[3]);
        normalize = sqrt(normalize);

        if (normalize == 0.0) {
            printf(" Node %d e0_2 is degenerate!\n", node->nodeID);
            normalize = 1.0;
        }

        hsmMemory.pars[k*LVTOT + lve02x] = node->geomData->firstDerivative[3]/normalize; // e_2 - x
        hsmMemory.pars[k*LVTOT + lve02y] = node->geomData->firstDerivative[4]/normalize; // e_2 - y
        hsmMemory.pars[k*LVTOT + lve02z] = node->geomData->firstDerivative[5]/normalize; // e_2 - z

        cross_DoubleVal(node->geomData->firstDerivative, &node->geomData->firstDerivative[3], norm);

        normalize = dot_DoubleVal(norm, norm);
        normalize = sqrt(normalize);

        hsmMemory.pars[k*LVTOT + lvn0x] = norm[0]/normalize; // normal - x
        hsmMemory.pars[k*LVTOT + lvn0y] = norm[1]/normalize; // normal - y
        hsmMemory.pars[k*LVTOT + lvn0z] = norm[2]/normalize; // normal - z

        found = (int) false;
        for (propertyIndex = 0; propertyIndex < hsmInstance[iIndex].feaProblem.numProperty; propertyIndex++) {

            if (feaData->propertyID != hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].propertyID) continue;

            found = (int) true;
            break;
        }

        if (found == false) {
            printf("Property ID, %d, for node %d NOT found!\n", feaData->propertyID , node->nodeID);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        membraneThickness  = hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].membraneThickness;
        shearMembraneRatio = hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].shearMembraneRatio;
        refLocation = 0.0;
        massPerArea = hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].massPerArea;

       found = (int) false;
        for (materialIndex = 0; materialIndex < hsmInstance[iIndex].feaProblem.numProperty; materialIndex++) {

            if (hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].materialID !=
                hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialID) {
                continue;
            }

            found = (int) true;
            break;
        }

        if (found == false) {
            printf("Material ID for node %d NOT found!\n", node->nodeID);
            status = CAPS_NOTFOUND;
            goto cleanup;
        }

        youngModulus = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].youngModulus;
        shearModulus = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulus;
        poissonRatio = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].poissonRatio;

        if (hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialType == Isotropic) {
            youngModulusLateral = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].youngModulus;
            shearModulusTrans1Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulus;
            shearModulusTrans2Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulus;

        } else if (hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialType == Orthotropic) {
            youngModulusLateral = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].youngModulusLateral;
            shearModulusTrans1Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulusTrans1Z;
            shearModulusTrans2Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulusTrans2Z;
        } else {
            printf("Unsupported material type!\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        // Checks
        if (youngModulus <= 0.0) {
            printf("Error: Young's modulus for material, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].name);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        if (membraneThickness <= 0.0) {
            printf("Error: Membrane thickness for property, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].name);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        if (shearMembraneRatio <= 0.0) {
            printf("Error: Shear membrane ratio for property, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].name);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        if (hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialType == Orthotropic) {

            if (shearModulusTrans1Z <= 0.0) {
                printf("Error: Shear modulus trans. 1Z for material, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].name);
                status = CAPS_BADVALUE;
                goto cleanup;
            }

            if (shearModulusTrans2Z <= 0.0) {
                printf("Error: Shear modulus trans. 2Z for material, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].name);
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        // Set material properties
        ortmat(youngModulus, youngModulusLateral,
               shearModulus, poissonRatio,
               0.0, 0.0,
               shearModulusTrans1Z, shearModulusTrans2Z,
               membraneThickness,
               refLocation,
               shearMembraneRatio,
               &hsmMemory.pars[k*LVTOT+lva11],
               &hsmMemory.pars[k*LVTOT+lvb11],
               &hsmMemory.pars[k*LVTOT+lvd11],
               &hsmMemory.pars[k*LVTOT+lva55]);

        // mass/area, can be zero if g = 0 and a = 0
        hsmMemory.pars[k*LVTOT+lvmu] = membraneThickness*massPerArea;

        // Shell thickness (for post-processing only)
        hsmMemory.pars[k*LVTOT+lvtsh] = membraneThickness;

        // Ref.-surface location within shell (for post-processing only)
        hsmMemory.pars[k*LVTOT+lvzrf] = refLocation;
    }

#else

    // Node parameters
    for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numElement; i++) {

          element = &hsmInstance[iIndex].feaProblem.feaMesh.element[i];
          feaData = (feaMeshDataStruct *) element->analysisData;

          if (feaData == NULL) {
              printf("No FEA data set for element %d\n", element->elementID);
              status = CAPS_NULLVALUE;
              goto cleanup;
          }

          for (j = 0; j < mesh_numMeshConnectivity(element->elementType); j++) {

              node = &hsmInstance[iIndex].feaProblem.feaMesh.node[element->connectivity[j]-1];

              if (node->geomData == NULL) {
                  printf("Geometry data not set for node %d\n", node->nodeID);
                  status = CAPS_NULLVALUE;
                  goto cleanup;
              }

              k = permutation[element->connectivity[j]-1] -1;

              hsmMemory.pars[k*LVTOT + lvr0x] = node->xyz[0]; // x
              hsmMemory.pars[k*LVTOT + lvr0y] = node->xyz[1]; // y
              hsmMemory.pars[k*LVTOT + lvr0z] = node->xyz[2]; // z

              normalize = dot_DoubleVal(node->geomData->firstDerivative, node->geomData->firstDerivative);
              normalize = sqrt(normalize);

              if (normalize == 0.0) {
                  printf(" Node %d e0_1 is degenerate!\n", node->nodeID);
                  normalize = 1.0;
              }

              hsmMemory.pars[k*LVTOT + lve01x] = node->geomData->firstDerivative[0]/normalize; // e_1 - x
              hsmMemory.pars[k*LVTOT + lve01y] = node->geomData->firstDerivative[1]/normalize; // e_1 - y
              hsmMemory.pars[k*LVTOT + lve01z] = node->geomData->firstDerivative[2]/normalize; // e_1 - z

              normalize = dot_DoubleVal(&node->geomData->firstDerivative[3], &node->geomData->firstDerivative[3]);
              normalize = sqrt(normalize);

              if (normalize == 0.0) {
                  printf(" Node %d e0_2 is degenerate!\n", node->nodeID);
                  normalize = 1.0;
              }

              hsmMemory.pars[k*LVTOT + lve02x] = node->geomData->firstDerivative[3]/normalize; // e_2 - x
              hsmMemory.pars[k*LVTOT + lve02y] = node->geomData->firstDerivative[4]/normalize; // e_2 - y
              hsmMemory.pars[k*LVTOT + lve02z] = node->geomData->firstDerivative[5]/normalize; // e_2 - z

              cross_DoubleVal(node->geomData->firstDerivative, &node->geomData->firstDerivative[3], norm);

              normalize = dot_DoubleVal(norm, norm);
              normalize = sqrt(normalize);

              hsmMemory.pars[k*LVTOT + lvn0x] = norm[0]/normalize; // normal - x
              hsmMemory.pars[k*LVTOT + lvn0y] = norm[1]/normalize; // normal - y
              hsmMemory.pars[k*LVTOT + lvn0z] = norm[2]/normalize; // normal - z

              found = (int) false;
              for (propertyIndex = 0; propertyIndex < hsmInstance[iIndex].feaProblem.numProperty; propertyIndex++) {

                  if (feaData->propertyID != hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].propertyID) continue;

                  found = (int) true;
                  break;
              }

              if (found == false) {
                  printf("Property ID, %d, for element %d NOT found!\n", feaData->propertyID , element->elementID);
                  status = CAPS_BADVALUE;
                  goto cleanup;
              }

              membraneThickness  = hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].membraneThickness;
              shearMembraneRatio = hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].shearMembraneRatio;
              refLocation = 0.0;
              massPerArea = hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].massPerArea;

             found = (int) false;
              for (materialIndex = 0; materialIndex < hsmInstance[iIndex].feaProblem.numProperty; materialIndex++) {

                  if (hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].materialID !=
                      hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialID) {
                      continue;
                  }

                  found = (int) true;
                  break;
              }

              if (found == false) {
                  printf("Material ID for element %d NOT found!\n", element->elementID);
                  status = CAPS_NOTFOUND;
                  goto cleanup;
              }

              youngModulus = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].youngModulus;
              shearModulus = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulus;
              poissonRatio = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].poissonRatio;

              if (hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialType == Isotropic) {
                  youngModulusLateral = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].youngModulus;
                  shearModulusTrans1Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulus;
                  shearModulusTrans2Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulus;

              } else if (hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialType == Orthotropic) {
                  youngModulusLateral = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].youngModulusLateral;
                  shearModulusTrans1Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulusTrans1Z;
                  shearModulusTrans2Z = hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].shearModulusTrans2Z;
              } else {
                  printf("Unsupported material type!\n");
                  status = CAPS_BADVALUE;
                  goto cleanup;
              }

              // Checks
              if (youngModulus <= 0.0) {
                  printf("Error: Young's modulus for material, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].name);
                  status = CAPS_BADVALUE;
                  goto cleanup;
              }

              if (membraneThickness <= 0.0) {
                  printf("Error: Membrane thickness for property, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].name);
                  status = CAPS_BADVALUE;
                  goto cleanup;
              }

              if (shearMembraneRatio <= 0.0) {
                  printf("Error: Shear membrane ratio for property, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaProperty[propertyIndex].name);
                  status = CAPS_BADVALUE;
                  goto cleanup;
              }

              if (hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].materialType == Orthotropic) {

                  if (shearModulusTrans1Z <= 0.0) {
                      printf("Error: Shear modulus trans. 1Z for material, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].name);
                      status = CAPS_BADVALUE;
                      goto cleanup;
                  }

                  if (shearModulusTrans2Z <= 0.0) {
                      printf("Error: Shear modulus trans. 2Z for material, %s, is <= 0.0!\n", hsmInstance[iIndex].feaProblem.feaMaterial[materialIndex].name);
                      status = CAPS_BADVALUE;
                      goto cleanup;
                  }
              }

              // Set material properties
              ortmat(youngModulus, youngModulusLateral,
                     shearModulus, poissonRatio,
                     0.0, 0.0,
                     shearModulusTrans1Z, shearModulusTrans2Z,
                     membraneThickness,
                     refLocation,
                     shearMembraneRatio,
                     &hsmMemory.pars[k*LVTOT+lva11],
                     &hsmMemory.pars[k*LVTOT+lvb11],
                     &hsmMemory.pars[k*LVTOT+lvd11],
                     &hsmMemory.pars[k*LVTOT+lva55]);

              // mass/area, can be zero if g = 0 and a = 0
              hsmMemory.pars[k*LVTOT+lvmu] = membraneThickness*massPerArea;

              // Shell thickness (for post-processing only)
              hsmMemory.pars[k*LVTOT+lvtsh] = membraneThickness;

              // Ref.-surface location within shell (for post-processing only)
              hsmMemory.pars[k*LVTOT+lvzrf] = refLocation;

              // Set initial guess

              // XYZ
              hsmMemory.vars[k*IVTOT + ivrx] = hsmMemory.pars[k*LVTOT + lvr0x];
              hsmMemory.vars[k*IVTOT + ivry] = hsmMemory.pars[k*LVTOT + lvr0y];
              hsmMemory.vars[k*IVTOT + ivrz] = hsmMemory.pars[k*LVTOT + lvr0z];

              // Unit material-normal vector the geometry
              hsmMemory.vars[k*IVTOT + ivdx] = hsmMemory.pars[k*LVTOT + lvn0x];
              hsmMemory.vars[k*IVTOT + ivdy] = hsmMemory.pars[k*LVTOT + lvn0y];
              hsmMemory.vars[k*IVTOT + ivdz] = hsmMemory.pars[k*LVTOT + lvn0z];

              // drilling rotation DOF
              hsmMemory.vars[k*IVTOT + ivps] = 0;
          }
    }
#endif

    status = hsm_setGlobalParameter(hsmInstance[iIndex].feaProblem, &hsmMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = hsm_setSurfaceParameter(hsmInstance[iIndex].feaProblem, permutation, &hsmMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = hsm_setEdgeBCParameter(hsmInstance[iIndex].feaProblem, permutation, &hsmMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = hsm_setNodeBCParameter(hsmInstance[iIndex].feaProblem, permutation, &hsmMemory);
    if (status != CAPS_SUCCESS) goto cleanup;

    printf("NumBCNode = %d\n", hsmMemory.numBCNode);
    //for (i = 0; i < hsmInstance[iIndex].feaProblem.feaMesh.numNode*LVTOT;i++) printf("i = %d, %f\n", i, hsmMemory.par[i]);

    //for (i = 0; i < LPTOT*maxDim; i++) printf("i = %d, %f\n", i, hsmMemory.parp[i]);
    //for (i = 0; i < maxDim; i++) printf("i = %d, %d\n", i, hsmMemory.kbcnode[i]);
    //for (i = 0; i < maxDim; i++) printf("i = %d, %d\n", i, hsmMemory.lbcnode[i]);
    //for (i = 0; i < LETOT*maxDim; i++) printf("i = %d, %f\n", i, hsmMemory.parb[i]);

    //for (i = 0; i < 2*maxDim; i++) printf("i = %d, %d\n", i, hsmMemory.kjoint[i]);

    // Get the size of the matrix
    itmax = -2; // Return nmdim
    nmdim =  1;
    kdim  = hsmInstance[iIndex].feaProblem.feaMesh.numNode;
    ldim  = maxDim;
    nedim = hsmNumElement;
    nddim = maxAdjacency;

    lrcurv = ftrue;
    ldrill = ftrue;

    //printf("%d %d %d %d %d\n",  kdim, ldim, nedim, nddim, nmdim);
    printf("->HSMSOL\n");

    HSMSOL(&ffalse, &ftrue,
           &lrcurv, &ldrill,
           &itmax, &rref,
           &rlim, &rtol, &rdel,
           &alim, &atol, &adel,
           &damem, &rtolm,
           hsmMemory.parg,
           &hsmInstance[iIndex].feaProblem.feaMesh.numNode, hsmMemory.pars, hsmMemory.vars,
           &nvarg, varg,
           &hsmNumElement, hsmMemory.kelem,
           &numBCEdge, hsmMemory.kbcedge, hsmMemory.pare,
           &hsmMemory.numBCNode, hsmMemory.kbcnode, hsmMemory.parp, hsmMemory.lbcnode,
           &numJoint, hsmMemory.kjoint,
           &kdim, &ldim, &nedim, &nddim, &nmdim,
           hsmTempMemory.bf, hsmTempMemory.bf_dj, hsmTempMemory.bm, hsmTempMemory.bm_dj,
           &hsmTempMemory.ibx[0], &hsmTempMemory.ibx[ldim], &hsmTempMemory.ibx[2*ldim], &hsmTempMemory.ibx[3*ldim], &hsmTempMemory.ibx[4*ldim], &hsmTempMemory.ibx[5*ldim],
           hsmTempMemory.resc, hsmTempMemory.resc_vars,
           hsmTempMemory.resp, hsmTempMemory.resp_vars, hsmTempMemory.resp_dvp,
           hsmTempMemory.kdvp, hsmTempMemory.ndvp,
           hsmTempMemory.ares,
           &hsmTempMemory.frst[0], &hsmTempMemory.frst[kdim], &hsmTempMemory.frst[2*kdim],
           hsmTempMemory.amat, hsmTempMemory.ipp, hsmTempMemory.dvars);

//#define WRITE_MATRIX_MARKET
#ifdef WRITE_MATRIX_MARKET
    FILE *file = fopen("B.mtx", "w");

    //Write the banner
    fprintf(file, "%%%%MatrixMarket matrix coordinate real general\n");
    fprintf(file, "%d %d %d\n", kdim, kdim, nmdim);

    //Write out the matrix data
    //Add one to the column index as the file format is 1-based
    for ( int row = 0; row < kdim; row++ )
    {
      for ( int k = 0; k < hsmTempMemory.ndvp[row]; k++ )
        fprintf(file, "%d %d 1\n", row+1, hsmTempMemory.kdvp[nddim*row+k]);
    }

    fclose(file); file = NULL;
#endif

    // Allocate the larger matrix storage after probing for the size
    printf(" Matrix Non-zero Entries = %d\n", nmdim);

    hsmTempMemory.amat  = (double *) EG_alloc(IRTOT*IRTOT*nmdim*sizeof(double));
    if (hsmTempMemory.amat  == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    hsmTempMemory.amatt = (double *) EG_alloc(3*3*nmdim*sizeof(double));
    if (hsmTempMemory.amatt == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    hsmTempMemory.amatv = (double *) EG_alloc(2*2*nmdim*sizeof(double));
    if (hsmTempMemory.amatv == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    hsmTempMemory.ipp   = (int *)    EG_alloc(IRTOT*nmdim*sizeof(int));
    if (hsmTempMemory.ipp   == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    i = 100;  // max allowed number of Newton iterations
    HSMSOL(&ffalse, &ftrue,
           &lrcurv, &ldrill,
           &i, &rref,
           &rlim, &rtol, &rdel,
           &alim, &atol, &adel,
           &damem, &rtolm,
           hsmMemory.parg,
           &hsmInstance[iIndex].feaProblem.feaMesh.numNode, hsmMemory.pars, hsmMemory.vars,
           &nvarg, varg,
           &hsmNumElement, hsmMemory.kelem,
           &numBCEdge, hsmMemory.kbcedge, hsmMemory.pare,
           &hsmMemory.numBCNode, hsmMemory.kbcnode, hsmMemory.parp, hsmMemory.lbcnode,
           &numJoint, hsmMemory.kjoint,
           &kdim, &ldim, &nedim, &nddim, &nmdim,
           hsmTempMemory.bf, hsmTempMemory.bf_dj, hsmTempMemory.bm, hsmTempMemory.bm_dj,
           &hsmTempMemory.ibx[0], &hsmTempMemory.ibx[ldim], &hsmTempMemory.ibx[2*ldim], &hsmTempMemory.ibx[3*ldim], &hsmTempMemory.ibx[4*ldim], &hsmTempMemory.ibx[5*ldim],
           hsmTempMemory.resc, hsmTempMemory.resc_vars,
           hsmTempMemory.resp, hsmTempMemory.resp_vars, hsmTempMemory.resp_dvp,
           hsmTempMemory.kdvp, hsmTempMemory.ndvp,
           hsmTempMemory.ares,
           &hsmTempMemory.frst[0], &hsmTempMemory.frst[kdim], &hsmTempMemory.frst[2*kdim],
           hsmTempMemory.amat, hsmTempMemory.ipp, hsmTempMemory.dvars);

/*
    status = hsm_writeTecplot(hsmInstance[iIndex].analysisPath,
                              hsmInstance[iIndex].projectName,
                              hsmInstance[iIndex].feaProblem.feaMesh,
                              &hsmMemory, permutation);
    if (status != CAPS_SUCCESS) goto cleanup;
*/

    if (i >= 0) {
        i = 20;
        elim   = 1.0;
        etol   = atol;

        HSMDEP(&ffalse, &ftrue,
               &lrcurv, &ldrill,
               &i,
               &elim, &etol, &edel,
               &hsmInstance[iIndex].feaProblem.feaMesh.numNode, hsmMemory.pars, hsmMemory.vars, hsmMemory.deps,
               &hsmNumElement, hsmMemory.kelem,
               &kdim, &ldim, &nedim, &nddim, &nmdim,
               &hsmTempMemory.res[0],
               &hsmTempMemory.res[kdim], &hsmTempMemory.res[4*kdim], &hsmTempMemory.res[5*kdim],
               hsmTempMemory.rest, hsmTempMemory.rest_t,
               &hsmTempMemory.idt[kdim], &hsmTempMemory.idt[0],
               &hsmTempMemory.frstt[0], &hsmTempMemory.frstt[kdim], &hsmTempMemory.frstt[2*kdim],
               hsmTempMemory.amatt,
               hsmTempMemory.resv, hsmTempMemory.resv_v,
               hsmTempMemory.kdv, hsmTempMemory.ndv,
               &hsmTempMemory.frstv[0], &hsmTempMemory.frstv[kdim], &hsmTempMemory.frstv[2*kdim],
               hsmTempMemory.amatv);

        /*
        HSMOUT(&hsmInstance[iIndex].feaProblem.feaMesh.numElement,
                hsmMemory.kelem,
                hsmMemory.var, hsmMemory.dep, hsmMemory.par, hsmMemory.parg,
                &hsmInstance[iIndex].feaProblem.feaMesh.numNode, &ldim, &hsmInstance[iIndex].feaProblem.feaMesh.numElement, &nddim, &nmdim);
         */

        status = hsm_writeTecplot(hsmInstance[iIndex].analysisPath,
                                  hsmInstance[iIndex].projectName,
                                  hsmInstance[iIndex].feaProblem.feaMesh,
                                  &hsmMemory, permutation);
        if (status != CAPS_SUCCESS) goto cleanup;

    }

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:

        if (status != CAPS_SUCCESS) printf("Error: hsmAIM (instance = %d) status %d\n", iIndex, status);

        //EG_free(nodeValence);    nodeValence    = NULL;

        EG_free(elementConnect); elementConnect = NULL;
        EG_free(permutation);    permutation    = NULL;
        EG_free(permutationInv); permutationInv = NULL;

        EG_free(openSeg);        openSeg        = NULL;

        EG_free(xadj);           xadj = NULL;
        EG_free(adj);            adj = NULL;
        EG_free(kjoint);         kjoint = NULL;

        (void ) destroy_hsmMemoryStruct(&hsmMemory);
        (void ) destroy_hsmTempMemoryStruct(&hsmTempMemory);

        if (filename != NULL) EG_free(filename);

        return status;
}

// Available AIM outputs
int aimOutputs(int iIndex, void *aimInfo,
               int index, char **aoname, capsValue *form)
{
    /*! \page aimOutputsHSM AIM Outputs
     * The following list outlines the HSM AIM outputs available through the AIM interface.
     *
     * - <B>None </B>
     */

    #ifdef DEBUG
        printf(" hsmAIM/aimOutputs instance = %d  index = %d!\n", inst, index);
    #endif

    if (index == 1) {
        *aoname = EG_strdup("OutputVariable");
        form->type = Boolean;
        form->vals.integer = (int) false;
    }

    return CAPS_SUCCESS;
}

// Get value for a given output variable
int aimCalcOutput(int iIndex, void *aimInfo,
                  const char *analysisPath, int index,
                  capsValue *val, capsErrs **errors)
{

    #ifdef DEBUG
        printf(" hsmAIM/aimCalcOutput instance = %d  index = %d!\n", iIndex, index);
    #endif

    *errors = NULL;

    // Fill in to populate output variable = index

    if (index == 1){

        val->vals.integer = (int) false;
    }

    return CAPS_SUCCESS;
}


void aimCleanup()
{
    int iIndex; // Indexing

    int status; // Returning status

    #ifdef DEBUG
        printf(" hsmAIM/aimCleanup!\n");
    #endif

    // Clean up hsmInstance data
    for ( iIndex = 0; iIndex < numInstance; iIndex++) {

        printf(" Cleaning up hsmInstance - %d\n", iIndex);

        status = destroy_aimStorage(iIndex);
        if (status != CAPS_SUCCESS) printf("Status = %d, hsmAIM instance %d, aimStorage cleanup!!!\n", status, iIndex);
    }

    if (hsmInstance != NULL) EG_free(hsmInstance);
    hsmInstance = NULL;
    numInstance = 0;

}
