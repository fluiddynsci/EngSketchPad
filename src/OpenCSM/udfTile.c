/*
 ************************************************************************
 *                                                                      *
 * udfTile -- create a tiled configuration via IRIT                     *
 *                                                                      *
 *            Written by John Dannenhoffer @ Syracuse University        *
 *                                                                      *
 ************************************************************************
 */

/*
 * Copyright (C) 2011/2020  John F. Dannenhoffer, III (Syracuse University)
 *
 * This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *     MA  02110-1301  USA
 */

#include <assert.h>

#define NUMUDPARGS      10
#define NUMUDPINPUTBODYS 1
#include "udpUtilities.h"

/* shorthands for accessing argument values and velocities */
#define FILENAME( IUDP)  ((char   *) (udps[IUDP].arg[0].val))
#define TABLENAME(IUDP)  ((char   *) (udps[IUDP].arg[1].val))
#define NUTILE(   IUDP)  ((int    *) (udps[IUDP].arg[2].val))[0]
#define NVTILE(   IUDP)  ((int    *) (udps[IUDP].arg[3].val))[0]
#define NWTILE(   IUDP)  ((int    *) (udps[IUDP].arg[4].val))[0]
#define WRITEITD( IUDP)  ((int    *) (udps[IUDP].arg[5].val))[0]
#define BODYNUM(  IUDP)  ((int    *) (udps[IUDP].arg[6].val))[0]
#define OUTLEVEL( IUDP)  ((int    *) (udps[IUDP].arg[7].val))[0]
#define DUMPEGADS(IUDP)  ((int    *) (udps[IUDP].arg[8].val))[0]
#define NUMBODYS( IUDP)  ((int    *) (udps[IUDP].arg[9].val))[0]

/* data about possible arguments */
static char  *argNames[NUMUDPARGS] = {"filename", "tablename", "nutile",   "nvtile",    "nwtile",
                                      "writeitd", "bodynum",   "outlevel", "dumpegads", "numbodys", };
static int    argTypes[NUMUDPARGS] = {ATTRSTRING, ATTRSTRING,  ATTRINT,    ATTRINT,     ATTRINT,
                                      ATTRINT,    ATTRINT,     ATTRINT,    ATTRINT,     -ATTRINT,   };
static int    argIdefs[NUMUDPARGS] = {0,          0,           1,          1,           1,
                                      0,          1,           0,          0,           1,          };
static double argDdefs[NUMUDPARGS] = {0.,         0.,          1.,         1.,          1.,
                                      0.,         1.,          0.,         0.,          1.,         };

/* get utility routines: udpErrorStr, udpInitialize, udpReset, udpSet,
                         udpGet, udpVel, udpClean, udpMesh */
#include "udpUtilities.c"

#include "OpenCSM.h"
#define  EPS06   1.0e-6
#define  HUGEQ   99999999.0

/* IRIT includes */
#include "inc_irit/irit_sm.h"
#include "inc_irit/iritprsr.h"
#include "inc_irit/allocate.h"
#include "inc_irit/attribut.h"
#include "inc_irit/cagd_lib.h"
#include "inc_irit/geom_lib.h"
#include "inc_irit/user_lib.h"

/* table for trilinear interpolations */
typedef struct {
    int      nu;              /* number of entries in u direction */
    int      nv;              /* number of entries in v direction */
    int      nw;              /* number of entries in w direction */
    int      rank;            /* number of dependent variables */
    char     **name;          /* name associated with each rank */
    char     **scale;         /* scale direction associated with each rank */
    double   *u;              /* vector of nu u-values */
    double   *v;              /* vector of nv v-values */
    double   *w;              /* vector of nw w-values */
    double   *dv;             /* vector of dependent variables */
} table_T;

/* user-specific data in callback functions */
typedef struct UserLocalDataStruct {
    ego            context;        /* EGADS contect */
    ego            duct;           /* duct Body */
    ego            tile;           /* tile Body */
    void           *modl;          /* tile OpenCSM model */
    char           filename[256];  /* name of CSM file containing tile */
    int            count;          /* number of tiles processed so far */
    int            nutile;         /* number of tiles in U direction */
    int            nvtile;         /* number of tiles in V direction */
    int            nwtile;         /* number of tiles in W direction */
    int            iutile;         /* U index of knot interval being generated */
    int            ivtile;         /* V index of knot interval being generated */
    int            iwtile;         /* W index of knot interval being generated */
    MvarMVStruct   *DefMap;        /* multivariate to be tiled */
    double         tspec;          /* value of "thick" in parent (or -1) */
    table_T        table;          /* interpolation table */
    int            nface;          /* number of Faces */
    ego            *eface;         /* list   of Facee */
    int            *colors;        /* colors: 0=none, 1=red, 2=green, 3=blue, 4=yellow, 5=magenta, 6=cyan */
} UserLocalDataStruct;

/* prototype for function defined below */
static /*@null@*/ IPObjectStruct *PreProcessTile(IPObjectStruct *Tile, UserMicroPreProcessTileCBStruct *CBData);
static /*@null@*/ IPObjectStruct *PostProcessTile(IPObjectStruct *Tile, UserMicroPostProcessTileCBStruct *CBData);
static int makeBSplineList(UserMicroPreProcessTileCBStruct *CBData, int *nBSpl, ego **BSpl);
static int makeIRITsrf(ego eobj, CagdSrfStruct **surf);
//$$$static void FlattenHierarchy2LinearList(const IPObjectStruct *MS, IPObjectStruct *LinMS);
static void trilinear(table_T *table, double u, double v, double w, double dv[]);

static void *realloc_temp=NULL;              /* used by RALLOC macro */


/*
 ************************************************************************
 *                                                                      *
 *   udpExecute - execute the primitive                                 *
 *                                                                      *
 ************************************************************************
 */

int
udpExecute(ego  emodel,                 /* (in)  Model containing Body */
           ego  *ebody,                 /* (out) Body pointer */
           int  *nMesh,                 /* (out) number of associated meshes */
           char *string[])              /* (out) error message */
{
    int     status = EGADS_SUCCESS;

    int     oclass, mtype, nchild, *senses, nface, iface, nloop, nedge, iedge;
    int     periodic, i, j, k, m, ijkm, ipmtr;
    double  data[18], trange[2], tt, umin, umax, vmin, vmax, dot;
    char    *ErrStr, temp[128];
    ego     context, eref, esurf, *ebodys, *efaces=NULL, *eloops, *eedges, epcurve;
    ego     *echilds, newModel;
    void    *modl=NULL, *save_modl=NULL;
    modl_T  *MODL;
    FILE    *fp;

    IPObjectStruct              *MS;
    CagdSrfStruct               *surf1, *surf2, *surf3, *surf4;
    MvarMVStruct                *DeformMV;
    TrivTVStruct                *TVMap;
    UserMicroParamStruct        MSParam;
    UserLocalDataStruct         CBfunData;

    ROUTINE(udpExecute);

    /* --------------------------------------------------------------- */

#ifdef DEBUG
    printf("udpExecute(emodel=%llx)\n", (long long)emodel);
    printf("filename( 0) = %s\n", FILENAME( 0));       // name of .csm file containing tile
    printf("tablename(0) = %s\n", TABLENAME(0));       // name of file containing interpolation data
    printf("nutile(   0) = %d\n", NUTILE(   0));       // number of tiles in U direction
    printf("nvtile(   0) = %d\n", NVTILE(   0));       // number of tiles in V direction
    printf("nwtile(   0) = %d\n", NWTILE(   0));       // number of files in W direction
    printf("writeitd( 0) = %d\n", WRITEITD( 0));       // =1 to write surf1.itd ... surf4.itd or enclosure
    printf("bodynum(  0) = %d\n", BODYNUM(  0));       // number of Body to return 1-numBodys
    printf("outlevel( 0) = %d\n", OUTLEVEL( 0));       // output level
    printf("dumpegads(0) = %d\n", DUMPEGADS(0));       // =1 to dump udfTile.egads after sewing
#endif

    /* default return values */
    *ebody  = NULL;
    *nMesh  = 0;
    *string = NULL;

    /* initialize the IRIT tiling callback structure */
    CBfunData.duct        = NULL;
    CBfunData.tile        = NULL;
    CBfunData.filename[0] = '\0';
    CBfunData.nutile      = 0;
    CBfunData.nvtile      = 0;
    CBfunData.nwtile      = 0;
    CBfunData.iutile      = 0;
    CBfunData.ivtile      = 0;
    CBfunData.iwtile      = 0;
    CBfunData.DefMap      = NULL;
    CBfunData.tspec       = -1;
    CBfunData.eface       = NULL;

    CBfunData.table.nu    = 0;
    CBfunData.table.nv    = 0;
    CBfunData.table.nw    = 0;
    CBfunData.table.rank  = 0;
    CBfunData.table.name  = NULL;
    CBfunData.table.scale = NULL;
    CBfunData.table.u     = NULL;
    CBfunData.table.v     = NULL;
    CBfunData.table.w     = NULL;
    CBfunData.table.dv    = NULL;

    /* check that Model was input that contains one Body */
    status = EG_getTopology(emodel, &eref, &oclass, &mtype,
                            data, &nchild, &ebodys, &senses);
    CHECK_STATUS(EG_getTopology);

    if (oclass != MODEL) {
        printf(" udpExecute: expecting a Model\n");
        status = EGADS_NOTMODEL;
        goto cleanup;
    } else if (nchild != 1) {
        printf(" udpExecute: expecting Model to contain one Bodys (not %d)\n", nchild);
        status = EGADS_NOTBODY;
        goto cleanup;
    }

    status = EG_getContext(emodel, &context);
    CHECK_STATUS(EG_getContext);

    /* remember pointer to duct model */
    status = EG_getUserPointer(context, (void**)(&(save_modl)));
    if (status != EGADS_SUCCESS) {
        printf(" udpExecute: bad return from getUserPointer\n");
        goto cleanup;
    }

    /* check arguments */
    if        (strlen(FILENAME(0)) == 0) {
        printf(" udpExecute: filename must not be blank\n");
        status = EGADS_NOTFOUND;
        goto cleanup;
    } else if (udps[0].arg[2].size != 1) {
        printf(" udpExecute: nutile shold be a scalar\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (NUTILE(0) < 1) {
        printf(" udpExecute: nutile should be a positive integer\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (udps[0].arg[3].size != 1) {
        printf(" udpExecute: nvtile shold be a scalar\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (NVTILE(0) < 1) {
        printf(" udpExecute: nvtile should be a positive integer\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (udps[0].arg[4].size != 1) {
        printf(" udpExecute: nwtile should be a scalar\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (NWTILE(0) < 1) {
        printf(" udpExecute: nwtile should be a positive integer\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (udps[0].arg[6].size != 1) {
        printf(" udpexecute: bodynum should be a scalar\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (BODYNUM(0) < 1) {
        printf(" udpExecute: bodynum should be a positive integer\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (udps[0].arg[7].size != 1) {
        printf(" udpExecute: outlevel should be a scalar\n");
        status = EGADS_RANGERR;
        goto cleanup;
    } else if (OUTLEVEL(0) != 0 && OUTLEVEL(0) != 1 && OUTLEVEL(0) != 2) {
        printf(" udpExecute: outlevel should be 0, 1 or 2\n");
        status = EGADS_RANGERR;
        goto cleanup;
    }

    /* cache copy of arguments for future use */
    status = cacheUdp();
    if (status < 0) {
        printf(" udpExecute: problem caching arguments\n");
        status = -999;
        goto cleanup;
    }

#ifdef DEBUG
    printf("filename( %d) = %s\n", numUdp, FILENAME( numUdp));
    printf("tablename(%d) = %s\n", numUdp, TABLENAME(numUdp));
    printf("nutile(   %d) = %d\n", numUdp, NUTILE(   numUdp));
    printf("nvtile(   %d) = %d\n", numUdp, NVTILE(   numUdp));
    printf("nwtile(   %d) = %d\n", numUdp, NWTILE(   numUdp));
    printf("writeitd( %d) = %d\n", numUdp, WRITEITD( numUdp));
    printf("bodynum(  %d) = %d\n", numUdp, BODYNUM(  numUdp));
    printf("outlevel( %d) = %d\n", numUdp, OUTLEVEL( numUdp));
    printf("dumpegads(%d) = %d\n", numUdp, DUMPEGADS(numUdp));
#endif

    /* make sure that the first Body (the duct) contains 6 Faces */
    status = EG_getBodyTopos(ebodys[0], NULL, FACE, &nface, &efaces);
    CHECK_STATUS(EG_getBodyTopos);

    if (nface != 6) {
        EG_free(efaces);

        printf(" udpExecute: first Body (duct) does not contain 6 Faces\n");
        status = EGADS_TOPOERR;
        goto cleanup;
    }

    /* make sure that the first 4 Faces all have exactly 4 Edges and
       are not trimmed (except along isoU or isoV lines) */
    printf("Checking Body to be tiled...\n");
    for (iface = 0; iface < 4; iface++) {
        status = EG_getTopology(efaces[iface], &esurf, &oclass, &mtype,
                                data, &nloop, &eloops, &senses);
        CHECK_STATUS(EG_getTopology);

        if (nloop != 1) {
            printf(" udpExecute: Face %d has more than one Loop\n", iface+1);
            status = EGADS_TOPOERR;
            goto cleanup;
        }

        status = EG_getTopology(eloops[0], &eref, &oclass, &mtype,
                                data, &nedge, &eedges, &senses);
        CHECK_STATUS(EG_getTopology);

        if (nedge != 4) {
            printf(" udpExecute: Face %d is not bounded by 4 Edges\n", iface+1);
            status = EGADS_TOPOERR;
            goto cleanup;
        }

        for (iedge = 0; iedge < 4; iedge++) {
            umin = +HUGEQ;
            umax = -HUGEQ;
            vmin = +HUGEQ;
            vmax = -HUGEQ;

            status = EG_getRange(eedges[iedge], trange, &periodic);
            CHECK_STATUS(EG_getRange);

            status = EG_otherCurve(esurf, eedges[iedge], 0, &epcurve);
            CHECK_STATUS(EG_otherCurve);

            for (i = 0; i < 51; i++) {
                tt = trange[0] + (trange[1] - trange[0]) * i / 50.0;

                status = EG_evaluate(epcurve, &tt, data);
                CHECK_STATUS(EG_evaluate);

                if (data[0] < umin) umin = data[0];
                if (data[0] > umax) umax = data[0];
                if (data[1] < vmin) vmin = data[1];
                if (data[1] > vmax) vmax = data[1];
            }

            if (fabs(umax-umin) > EPS06 && fabs(vmax-vmin) > EPS06) {
                printf(" udpExecute: Face %d has Edge %d that is not an isocline\n", iface+1, iedge+1);
                status = EGADS_GEOMERR;
                goto cleanup;
            }
        }
    }

    /* set up the IRIT trivatiate for the duct (use Surfaces since in
     general not planar) */
    printf("Setting up IRIT duct...\n");
    status = EG_convertToBSpline(efaces[0], &esurf);
    CHECK_STATUS(EG_convertToBSpline);

    status = makeIRITsrf(esurf, &surf1);
    CHECK_STATUS(makeIRITsrf);
    if (WRITEITD(numUdp)) {
        BspSrfWriteToFile(surf1, "surf1.itd", 0, "surf1", &ErrStr);
        if (ErrStr != NULL) printf(" udpExecute: surf1 ErrStr: %s\n", ErrStr);
    }

    status = EG_convertToBSpline(efaces[1], &esurf);
    CHECK_STATUS(EG_convertToBSpline);

    status = makeIRITsrf(esurf, &surf2);
    CHECK_STATUS(makeIRITsrf);
    if (WRITEITD(numUdp)) {
        BspSrfWriteToFile(surf2, "surf2.itd", 0, "surf2", &ErrStr);
        if (ErrStr != NULL) printf(" udpExecute: surf2 ErrStr: %s\n", ErrStr);
    }

    status = EG_convertToBSpline(efaces[2], &esurf);
    CHECK_STATUS(EG_convertToBSpline);

    status = makeIRITsrf(esurf, &surf3);
    CHECK_STATUS(makeIRITsrf);
    if (WRITEITD(numUdp)) {
        BspSrfWriteToFile(surf3, "surf3.itd", 0, "surf3", &ErrStr);
        if (ErrStr != NULL) printf(" udpExecute: surf3 ErrStr: %s\n", ErrStr);
    }

    status = EG_convertToBSpline(efaces[3], &esurf);
    CHECK_STATUS(EG_convertToBSpline);

    status = makeIRITsrf(esurf, &surf4);
    CHECK_STATUS(makeIRITsrf);
    if (WRITEITD(numUdp)) {
        BspSrfWriteToFile(surf4, "surf4.itd", 0, "surf4", &ErrStr);
        if (ErrStr != NULL) printf(" udpExecute: surf4 ErrStr: %s\n", ErrStr);
    }

    TVMap = MvarTrivarBoolSum3(surf1, surf2, surf3, surf4, NULL, NULL);
    DeformMV = MvarCnvrtTVToMV(TVMap);
    if (WRITEITD(numUdp)) {
        status = TrivBspTVWriteToFile(TVMap, "duct.itd", 0, "duct", &ErrStr);
        if (ErrStr != NULL) printf(" udpExecute: duct ErrStr: %s  (status=%d)\n", ErrStr, status);
    }

    TrivTVFree(TVMap);

    CagdSrfFree(surf1);
    CagdSrfFree(surf2);
    CagdSrfFree(surf3);
    CagdSrfFree(surf4);

    EG_free(efaces);

    /* initialize the IRIT tiling callback structure */
    CBfunData.context = context;
    CBfunData.duct   = ebodys[0];
    CBfunData.tile   = NULL;
    CBfunData.modl   = NULL;
    strncpy(CBfunData.filename, FILENAME(numUdp), 255);
    CBfunData.count  = 0;
    CBfunData.nutile = DeformMV->Lengths[0] - DeformMV->Orders[0] + 1;
    CBfunData.nvtile = DeformMV->Lengths[1] - DeformMV->Orders[1] + 1;
    CBfunData.nwtile = DeformMV->Lengths[2] - DeformMV->Orders[2] + 1;
    CBfunData.iutile = 0;
    CBfunData.ivtile = 0;
    CBfunData.iwtile = 0;
    CBfunData.DefMap = DeformMV;
    CBfunData.tspec  = -1;
    CBfunData.nface  = 0;
    CBfunData.eface  = NULL;
    CBfunData.colors = NULL;

    /* read in thicknesses from GPkit */
    if (strlen(TABLENAME(numUdp)) > 0) {
        fp = fopen(TABLENAME(numUdp), "r");
        if (fp == NULL) {
            printf(" udpExecute: could not open %s\n", TABLENAME(numUdp));
            status = EGADS_NOTFOUND;
            goto cleanup;
        }

        fscanf(fp, "%d %d %d %d", &(CBfunData.table.nu),
                                  &(CBfunData.table.nv),
                                  &(CBfunData.table.nw),
                                  &(CBfunData.table.rank));

        MALLOC(CBfunData.table.u,     double, CBfunData.table.nu);
        MALLOC(CBfunData.table.v,     double, CBfunData.table.nv);
        MALLOC(CBfunData.table.w,     double, CBfunData.table.nw);
        MALLOC(CBfunData.table.dv,    double, CBfunData.table.nu*CBfunData.table.nv*CBfunData.table.nw*CBfunData.table.rank);
        MALLOC(CBfunData.table.name,  char*,  CBfunData.table.rank);
        MALLOC(CBfunData.table.scale, char*,  CBfunData.table.rank);

        for (i = 0; i < CBfunData.table.nu; i++) {
            fscanf(fp, "%lf", &(CBfunData.table.u[i]));
        }
        for (j = 0; j < CBfunData.table.nv; j++) {
            fscanf(fp, "%lf", &(CBfunData.table.v[j]));
        }
        for (k = 0; k < CBfunData.table.nw; k++) {
            fscanf(fp, "%lf", &(CBfunData.table.w[k]));
        }

        for (m = 0; m < CBfunData.table.rank; m++) {
            fscanf(fp, "%s", temp);
            CBfunData.table.name[m] = NULL;
            MALLOC(CBfunData.table.name[m], char, strlen(temp)+1);
            strcpy(CBfunData.table.name[m], temp);

            fscanf(fp, "%s", temp);
            CBfunData.table.scale[m] = NULL;
            MALLOC(CBfunData.table.scale[m], char, strlen(temp)+1);
            strcpy(CBfunData.table.scale[m], temp);
        }

        ijkm = 0;
        for (k = 0; k < CBfunData.table.nw; k++) {
            for (j = 0; j < CBfunData.table.nv; j++) {
                for (i = 0; i < CBfunData.table.nu; i++) {
                    for (m = 0; m < CBfunData.table.rank; m++) {
                        fscanf(fp, "%lf", &(CBfunData.table.dv[ijkm]));
                        ijkm++;
                    }
                }
            }
        }

        fclose(fp);

    } else {
        /* set the thickness into the callback structure */
        status = EG_getUserPointer(context, (void**)(&(modl)));
        if (status != EGADS_SUCCESS) {
            printf(" udpExecute: problem in getUserPointer\n");
            goto cleanup;
        }

        MODL = (modl_T *) modl;

        for(ipmtr = 1; ipmtr <= MODL->npmtr; ipmtr++) {
            if (strcmp(MODL->pmtr[ipmtr].name, "thick") == 0) {
                status = ocsmGetValu(modl, ipmtr, 1, 1, &(CBfunData.tspec), &dot);
                if (status != EGADS_SUCCESS) {
                    printf(" udpExecute: problem in ocsmGetValu\n");
                goto cleanup;
                }
                break;
            }
        }
    }

    /* call IRIT to tile the duct (initialize with dummy tile that will not be used) */
    printf("Calling IRIT...\n");
    IRIT_ZAP_MEM(&MSParam, sizeof(UserMicroParamStruct));

    MSParam.TilingType                       = USER_MICRO_TILE_REGULAR;
    MSParam.DeformMV                         = DeformMV;
    MSParam.U.RegularParam.Tile              = NULL;
    MSParam.U.RegularParam.TilingStepMode    = TRUE;
    MSParam.U.RegularParam.TilingSteps[0]    = NULL;
    MSParam.U.RegularParam.TilingSteps[1]    = NULL;
    MSParam.U.RegularParam.TilingSteps[2]    = NULL;

    MALLOC(MSParam.U.RegularParam.TilingSteps[0], double, 2);
    MALLOC(MSParam.U.RegularParam.TilingSteps[1], double, 2);
    MALLOC(MSParam.U.RegularParam.TilingSteps[2], double, 2);

    MSParam.U.RegularParam.TilingSteps[0][0] = 1;
    MSParam.U.RegularParam.TilingSteps[0][1] = NUTILE(numUdp);
    MSParam.U.RegularParam.TilingSteps[1][0] = 1;
    MSParam.U.RegularParam.TilingSteps[1][1] = NVTILE(numUdp);
    MSParam.U.RegularParam.TilingSteps[2][0] = 1;
    MSParam.U.RegularParam.TilingSteps[2][1] = NWTILE(numUdp);
    MSParam.U.RegularParam.PreProcessCBFunc  = PreProcessTile;
    MSParam.U.RegularParam.PostProcessCBFunc = PostProcessTile;
    MSParam.U.RegularParam.CBFuncData        = &CBfunData;

    MS = UserMicroStructComposition(&MSParam);

    FREE(MSParam.U.RegularParam.TilingSteps[0]);
    FREE(MSParam.U.RegularParam.TilingSteps[1]);
    FREE(MSParam.U.RegularParam.TilingSteps[2]);

    MvarMVFree(DeformMV);

    /* write out the IRIT geometry */
    if (WRITEITD(numUdp)) {
        IPExportObjectToFile("tiled.itd", MS, IP_ANY_FILE);
    }

    IPFreeObject(MS);

    /* restore user data to original modl */
    status = EG_setUserPointer(context, save_modl);
    if (status != EGADS_SUCCESS) {
        printf(" udpExecute: problem reseting user pointer\n");
        goto cleanup;
    }

    /* sew the Faces together into a Model */
    printf("Sewing %d Faces into a Model...\n", CBfunData.nface);
    status = EG_sewFaces(CBfunData.nface, CBfunData.eface, 0.0, 0, &newModel);
    CHECK_STATUS(EG_sewFaces);

    if (nchild != 1) {
        printf(" udpExecute: sewing produced %d Bodys\n", nchild);
    }

    for (i = 0; i < CBfunData.nface; i++) {
        status = EG_getTopology(CBfunData.eface[i], &esurf, &oclass, &mtype,
                                data, &nchild, &echilds, &senses);
        if (status != EGADS_SUCCESS) {
            printf(" udpExecute: problem in getTopology\n");
        } else {
            EG_deleteObject(CBfunData.eface[i]);
            EG_deleteObject(esurf);
        }
    }

    /* optionally dump an EGADS file with the results of EG_sewFaces */
    if (DUMPEGADS(numUdp) > 0) {
        status = remove("udfTile.egads");
        if (status == 0) {
            printf("WARNING:: file \"udfTile.egads\" is being overwritten");
        }

        printf("    writing \"udfTile.egads\"\n");
        status = EG_saveModel(newModel, "udfTile.egads");
        CHECK_STATUS(EG_saveModel);
    }

    /* extract the Body from the newModel */
    status = EG_getTopology(newModel, &eref, &oclass, &mtype, data,
                            &nchild, &echilds, &senses);
    CHECK_STATUS(EG_getTopology);

    if (nchild < 1) {
        printf(" udpExecute: sewing produced %d Bodys\n", nchild);
        status = EGADS_TOPOERR;
        goto cleanup;
    } else if (BODYNUM(numUdp) > nchild) {
        printf(" udpExecute: bodynum=%d but only %d Bodys produced\n", BODYNUM(numUdp), nchild);
        status = EGADS_RANGERR;
        goto cleanup;
    }

    status = EG_copyObject (echilds[BODYNUM(numUdp)-1], NULL, ebody);
    CHECK_STATUS(EG_copyObject);

    status = EG_deleteObject(newModel);
    CHECK_STATUS(EG_deleteObject);

    /* print info about Edges that are not manifold */
    status = EG_getBodyTopos(*ebody, NULL, EDGE, &nedge, &eedges);
    CHECK_STATUS(EG_getBodyTopos);

    for (iedge = 0; iedge < nedge; iedge++) {
        status = EG_getBodyTopos(*ebody, eedges[iedge], FACE, &nface, NULL);
        CHECK_STATUS(EG_getBodyTopos);

        if (nface != 2) {
            printf("Edge %5d has %2d incident Faces\n", iedge+1, nface);
        }
    }
    EG_free(eedges);

    /* add a special Atribute to the Body to tell OpenCSM to mark the
       Faces with the current Branch */
    status = EG_attributeAdd(*ebody, "__markFaces__", ATTRSTRING,
                             0, NULL, NULL, "udfTile");
    CHECK_STATUS(EG_attributeAdd);

    /* set the output value(s) */
    NUMBODYS(0) = nchild;

    /* remember this model (body) */
    udps[numUdp].ebody = *ebody;

cleanup:
    for (m = 0; m < CBfunData.table.rank; m++) {
        FREE(CBfunData.table.name[ m]);
        FREE(CBfunData.table.scale[m]);
    }

    FREE(CBfunData.eface);

    FREE(CBfunData.table.u    );
    FREE(CBfunData.table.v    );
    FREE(CBfunData.table.w    );
    FREE(CBfunData.table.dv   );
    FREE(CBfunData.table.name );
    FREE(CBfunData.table.scale);

    if (status != EGADS_SUCCESS) {
        *string = udpErrorStr(status);
    }

    return status;
}


/*
 ************************************************************************
 *                                                                      *
 *   udpSensitivity - return sensitivity derivatives for the "real" argument *
 *                                                                      *
 ************************************************************************
 */

int
udpSensitivity(ego    ebody,            /* (in)  Body pointer */
   /*@unused@*/int    npnt,             /* (in)  number of points */
   /*@unused@*/int    entType,          /* (in)  OCSM entity type */
   /*@unused@*/int    entIndex,         /* (in)  OCSM entity index (bias-1) */
   /*@unused@*/double uvs[],            /* (in)  parametric coordinates for evaluation */
   /*@unused@*/double vels[])           /* (out) velocities */
{
    int iudp, judp;

    ROUTINE(udpSensitivity);

    /* --------------------------------------------------------------- */

    /* check that ebody matches one of the ebodys */
    iudp = 0;
    for (judp = 1; judp <= numUdp; judp++) {
        if (ebody == udps[judp].ebody) {
            iudp = judp;
            break;
        }
    }
    if (iudp <= 0) {
        return EGADS_NOTMODEL;
    }

    /* this routine is not written yet */
    return EGADS_NOLOAD;
}


/*
 ************************************************************************
 *                                                                      *
 *   PreProcessTile - callback function during tiling process           *
 *                                                                      *
 ************************************************************************
 */

static /*@null@*/ IPObjectStruct*
PreProcessTile(IPObjectStruct                  *Tile,
               UserMicroPreProcessTileCBStruct *CBData)
{
    int     status = EGADS_SUCCESS;

    int     nBSpline, j, m, buildTo, builtTo, nbody, nface, iface, jface, locn, ipmtr;
    int     old_outLevel, oclass, mtype, *senses;
    int     nloop, iedge, nedgei, jedge, nedgej, matches;
    double  bbox[6], bbox2[6], myUVWMin[3], myUVWMax[3], tspec, *dv=NULL;
    double  X000[3], X100[3], X010[3], X110[3], X001[3], X101[3], X011[3], X111[3];
    double  L00x, L01x, L10x, L11x, L0x0, L0x1, L1x0, L1x1, Lx00, Lx01, Lx10, Lx11;
    double  data[4], vec[3], toler;
    char    pmtrname[129], filename[80];
    ego     *BSplines, *efaces, *eloops, eref, *eedgesi, *eedgesj, emodel;
    modl_T  *MODL;

    CagdRType      R[MVAR_MAX_PT_SIZE];
    IPObjectStruct *IPListObj;
    CagdSrfStruct  *srf, *Srfs=NULL, *Last=NULL;

    UserLocalDataStruct *CBfunData = (UserLocalDataStruct *) CBData->CBFuncData;

    ROUTINE(PreProcessTile);

    /* --------------------------------------------------------------- */

    /* convert to global UVW */
    myUVWMin[0] = CBData->TileIdxsMinOrig[0] + CBData->TileIdxsMin[0] * (CBData->TileIdxsMaxOrig[0] - CBData->TileIdxsMinOrig[0]);
    myUVWMax[0] = CBData->TileIdxsMinOrig[0] + CBData->TileIdxsMax[0] * (CBData->TileIdxsMaxOrig[0] - CBData->TileIdxsMinOrig[0]);
    myUVWMin[1] = CBData->TileIdxsMinOrig[1] + CBData->TileIdxsMin[1] * (CBData->TileIdxsMaxOrig[1] - CBData->TileIdxsMinOrig[1]);
    myUVWMax[1] = CBData->TileIdxsMinOrig[1] + CBData->TileIdxsMax[1] * (CBData->TileIdxsMaxOrig[1] - CBData->TileIdxsMinOrig[1]);
    myUVWMin[2] = CBData->TileIdxsMinOrig[2] + CBData->TileIdxsMin[2] * (CBData->TileIdxsMaxOrig[2] - CBData->TileIdxsMinOrig[2]);
    myUVWMax[2] = CBData->TileIdxsMinOrig[2] + CBData->TileIdxsMax[2] * (CBData->TileIdxsMaxOrig[2] - CBData->TileIdxsMinOrig[2]);

#ifdef DEBUG
    printf("\niutile=%3d  ivtile=%3d  iwtile=%3d\n", CBfunData->iutile, CBfunData->ivtile, CBfunData->iwtile);
    printf("TileIdxsMin     = %10.4f %10.4f %10.4f\n", CBData->TileIdxsMin[0], CBData->TileIdxsMin[1], CBData->TileIdxsMin[2]);
    printf("TileIdxsMax     = %10.4f %10.4f %10.4f\n", CBData->TileIdxsMax[0], CBData->TileIdxsMax[1], CBData->TileIdxsMax[2]);
    printf("TileIdxsMinOrig = %10.4f %10.4f %10.4f\n", CBData->TileIdxsMinOrig[0], CBData->TileIdxsMinOrig[1], CBData->TileIdxsMinOrig[2]);
    printf("TileIdxsMaxOrig = %10.4f %10.4f %10.4f\n", CBData->TileIdxsMaxOrig[0], CBData->TileIdxsMaxOrig[1], CBData->TileIdxsMaxOrig[2]);
    printf("myUVWMin        = %10.4f %10.4f %10.4f\n", myUVWMin[0], myUVWMin[1], myUVWMin[2]);
    printf("myUVWMax        = %10.4f %10.4f %10.4f\n", myUVWMax[0], myUVWMax[1], myUVWMax[2]);
#endif

    /* find XYZ coorindates at corners */
    vec[0] = myUVWMin[0]; vec[1] = myUVWMin[1]; vec[2] = myUVWMin[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X000[0] = R[1]; X000[1] = R[2]; X000[2]= R[3];

    vec[0] = myUVWMax[0]; vec[1] = myUVWMin[1]; vec[2] = myUVWMin[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X001[0] = R[1]; X001[1] = R[2]; X001[2]= R[3];

    vec[0] = myUVWMin[0]; vec[1] = myUVWMax[1]; vec[2] = myUVWMin[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X010[0] = R[1]; X010[1] = R[2]; X010[2]= R[3];

    vec[0] = myUVWMax[0]; vec[1] = myUVWMax[1]; vec[2] = myUVWMin[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X011[0] = R[1]; X011[1] = R[2]; X011[2]= R[3];

    vec[0] = myUVWMin[0]; vec[1] = myUVWMin[1]; vec[2] = myUVWMax[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X100[0] = R[1]; X100[1] = R[2]; X100[2]= R[3];

    vec[0] = myUVWMax[0]; vec[1] = myUVWMin[1]; vec[2] = myUVWMax[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X101[0] = R[1]; X101[1] = R[2]; X101[2]= R[3];

    vec[0] = myUVWMin[0]; vec[1] = myUVWMax[1]; vec[2] = myUVWMax[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X110[0] = R[1]; X110[1] = R[2]; X110[2]= R[3];

    vec[0] = myUVWMax[0]; vec[1] = myUVWMax[1]; vec[2] = myUVWMax[2];
    MvarMVEvalToData(CBfunData->DefMap, vec, R);
    X111[0] = R[1]; X111[1] = R[2]; X111[2]= R[3];
  
//$$$    printf("X000= %10.4f %10.4f %10.4f\n", X000[0], X000[1], X000[2]);
//$$$    printf("X001= %10.4f %10.4f %10.4f\n", X001[0], X001[1], X001[2]);
//$$$    printf("X010= %10.4f %10.4f %10.4f\n", X010[0], X010[1], X010[2]);
//$$$    printf("X011= %10.4f %10.4f %10.4f\n", X011[0], X011[1], X011[2]);
//$$$    printf("X100= %10.4f %10.4f %10.4f\n", X100[0], X100[1], X100[2]);
//$$$    printf("X101= %10.4f %10.4f %10.4f\n", X101[0], X101[1], X101[2]);
//$$$    printf("X110= %10.4f %10.4f %10.4f\n", X110[0], X110[1], X110[2]);
//$$$    printf("X111= %10.4f %10.4f %10.4f\n", X111[0], X111[1], X111[2]);

    /* find lengths of 12 Edges */
    L00x = sqrt( (X000[0]-X001[0]) * (X000[0]-X001[0])
                +(X000[1]-X001[1]) * (X000[1]-X001[1])
                +(X000[2]-X001[2]) * (X000[2]-X001[2]));
    L01x = sqrt( (X010[0]-X011[0]) * (X010[0]-X011[0])
                +(X010[1]-X011[1]) * (X010[1]-X011[1])
                +(X010[2]-X011[2]) * (X010[2]-X011[2]));
    L10x = sqrt( (X100[0]-X101[0]) * (X100[0]-X101[0])
                +(X100[1]-X101[1]) * (X100[1]-X101[1])
                +(X100[2]-X101[2]) * (X100[2]-X101[2]));
    L11x = sqrt( (X110[0]-X111[0]) * (X110[0]-X111[0])
                +(X110[1]-X111[1]) * (X110[1]-X111[1])
                +(X110[2]-X111[2]) * (X110[2]-X111[2]));

    L0x0 = sqrt( (X000[0]-X010[0]) * (X000[0]-X010[0])
                +(X000[1]-X010[1]) * (X000[1]-X010[1])
                +(X000[2]-X010[2]) * (X000[2]-X010[2]));
    L0x1 = sqrt( (X001[0]-X011[0]) * (X001[0]-X011[0])
                +(X001[1]-X011[1]) * (X001[1]-X011[1])
                +(X001[2]-X011[2]) * (X001[2]-X011[2]));
    L1x0 = sqrt( (X100[0]-X110[0]) * (X100[0]-X110[0])
                +(X100[1]-X110[1]) * (X100[1]-X110[1])
                +(X100[2]-X110[2]) * (X100[2]-X110[2]));
    L1x1 = sqrt( (X101[0]-X111[0]) * (X101[0]-X111[0])
                +(X101[1]-X111[1]) * (X101[1]-X111[1])
                +(X101[2]-X111[2]) * (X101[2]-X111[2]));

    Lx00 = sqrt( (X000[0]-X100[0]) * (X000[0]-X100[0])
                +(X000[1]-X100[1]) * (X000[1]-X100[1])
                +(X000[2]-X100[2]) * (X000[2]-X100[2]));
    Lx01 = sqrt( (X001[0]-X101[0]) * (X001[0]-X101[0])
                +(X001[1]-X101[1]) * (X001[1]-X101[1])
                +(X001[2]-X101[2]) * (X001[2]-X101[2]));
    Lx10 = sqrt( (X010[0]-X110[0]) * (X010[0]-X110[0])
                +(X010[1]-X110[1]) * (X010[1]-X110[1])
                +(X010[2]-X110[2]) * (X010[2]-X110[2]));
    Lx11 = sqrt( (X011[0]-X111[0]) * (X011[0]-X111[0])
                +(X011[1]-X111[1]) * (X011[1]-X111[1])
                +(X011[2]-X111[2]) * (X011[2]-X111[2]));

    printf("... working on tile   u:%3d (%8.3f:%8.3f)   v:%3d (%8.3f:%8.3f)   w:%3d (%8.3f:%8.3f)\n",
           CBfunData->iutile, CBData->TileIdxsMin[0], CBData->TileIdxsMax[0],
           CBfunData->ivtile, CBData->TileIdxsMin[1], CBData->TileIdxsMax[1],
           CBfunData->iwtile, CBData->TileIdxsMin[2], CBData->TileIdxsMax[2]);

    /* set outLevel to 0 */
    old_outLevel = ocsmSetOutLevel(OUTLEVEL(numUdp));

    /* load the .csm file */
    status = ocsmLoad(CBfunData->filename, &(CBfunData->modl));
    if (status < 0) {
        printf(" udpExecute: problem during ocsmLoad\n");
        goto cleanup;
    }

    MODL = (modl_T *)(CBfunData->modl);

    /* make the new MODL use the same context as the caller */
    EG_deleteObject(MODL->context);
    status = EG_getContext(CBfunData->duct, &(MODL->context));
    CHECK_STATUS(EG_getContext);

    /* propagate thick to the corners of the tile */
    if (CBfunData->table.rank > 0) {
        MALLOC(dv, double, CBfunData->table.rank);
    }

    if (CBfunData->tspec >= 0) {
        tspec = CBfunData->tspec;

        for (ipmtr = 1; ipmtr <= MODL->npmtr; ipmtr++) {
            if (MODL->pmtr[ipmtr].type != OCSM_EXTERNAL) continue;

            if        (strcmp(MODL->pmtr[ipmtr].name, "thick:000") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L0x0;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            } else if (strcmp(MODL->pmtr[ipmtr].name, "thick:100") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L1x0;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            } else if (strcmp(MODL->pmtr[ipmtr].name, "thick:010") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L0x0;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            } else if (strcmp(MODL->pmtr[ipmtr].name, "thick:110") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L1x0;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            } else if (strcmp(MODL->pmtr[ipmtr].name, "thick:001") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L0x1;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            } else if (strcmp(MODL->pmtr[ipmtr].name, "thick:101") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L1x1;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            } else if (strcmp(MODL->pmtr[ipmtr].name, "thick:011") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L0x1;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            } else if (strcmp(MODL->pmtr[ipmtr].name, "thick:111") == 0) {
                MODL->pmtr[ipmtr].value[0] = tspec / L1x1;
                MODL->pmtr[ipmtr].dot[  0] = 0;
            }
        }
    } else if (CBfunData->table.rank > 0) {
        for (m = 0; m < CBfunData->table.rank; m++) {
            for (ipmtr = 1; ipmtr <= MODL->npmtr; ipmtr++) {
                if (MODL->pmtr[ipmtr].type != OCSM_EXTERNAL) continue;

                snprintf(pmtrname, 128, "%s:000", CBfunData->table.name[m]);
                if        (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMin[0], myUVWMin[1], myUVWMin[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L00x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L0x0;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx00;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }

                snprintf(pmtrname, 128, "%s:001", CBfunData->table.name[m]);
                if (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMax[0], myUVWMin[1], myUVWMin[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L00x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L0x1;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx01;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }

                snprintf(pmtrname, 128, "%s:010", CBfunData->table.name[m]);
                if (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMin[0], myUVWMax[1], myUVWMin[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L01x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L0x0;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx10;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }

                snprintf(pmtrname, 128, "%s:011", CBfunData->table.name[m]);
                if (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMax[0], myUVWMax[1], myUVWMin[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L01x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L0x1;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx11;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }

                snprintf(pmtrname, 128, "%s:100", CBfunData->table.name[m]);
                if (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMin[0], myUVWMin[1], myUVWMax[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L10x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L1x0;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx00;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }

                snprintf(pmtrname, 128, "%s:101", CBfunData->table.name[m]);
                if (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMax[0], myUVWMin[1], myUVWMax[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L10x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L1x1;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx01;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }

                snprintf(pmtrname, 128, "%s:110", CBfunData->table.name[m]);
                if (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMin[0], myUVWMax[1], myUVWMax[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L11x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L1x0;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx10;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }

                snprintf(pmtrname, 128, "%s:111", CBfunData->table.name[m]);
                if (strcmp(MODL->pmtr[ipmtr].name, pmtrname) == 0) {
                    trilinear(&(CBfunData->table), myUVWMax[0], myUVWMax[1], myUVWMax[2], dv);

                    if        (strcmp(CBfunData->table.scale[m], "u") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L11x;
                    } else if (strcmp(CBfunData->table.scale[m], "v") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / L1x1;
                    } else if (strcmp(CBfunData->table.scale[m], "w") == 0) {
                        MODL->pmtr[ipmtr].value[0] = dv[m] / Lx11;
                    } else {
                        MODL->pmtr[ipmtr].value[0] = dv[m];
                    }
                    MODL->pmtr[ipmtr].dot[0] = 0;
                }
            }
        }
    }

    FREE(dv);

    /* build the MODL (without cleaning up unattached egos - since we need them) */
    buildTo = 0;
    nbody   = 0;
    ((modl_T *)(CBfunData->modl))->cleanup = 0;
    status = ocsmBuild(CBfunData->modl, buildTo, &builtTo, &nbody, NULL);
    ((modl_T *)(CBfunData->modl))->cleanup = 1;
    if (status < 0) {
        printf(" udpExecute: problem during ocsmBuild\n");
        goto cleanup;
    }

    /* restore the outLevel */
    (void) ocsmSetOutLevel(old_outLevel);

    CBfunData->tile = MODL->body[MODL->nbody].ebody;

    /* make sure that the bounding box of the second Body (the tile)
       fits within the unit cube */
    status = EG_getBoundingBox(CBfunData->tile, bbox);
    CHECK_STATUS(EG_getBoundingBox);

    status = EG_tolerance(CBfunData->tile, &toler);
    CHECK_STATUS(EG_getTolerance);

    if (bbox[0] < -toler || bbox[3] > 1+toler ||
        bbox[1] < -toler || bbox[4] > 1+toler ||
        bbox[2] < -toler || bbox[5] > 1+toler   ) {
        printf(" udpExecute: second Body (tile) is not in a unit cube\n");
        printf("             xmin=%14.7e  xmax=%14.7e\n", bbox[0], bbox[3]);
        printf("             ymin=%14.7e  ymax=%14.7e\n", bbox[1], bbox[4]);
        printf("             zmin=%14.7e  zmax=%14.7e\n", bbox[2], bbox[5]);
        goto cleanup;
    }

    /* optionally generate an egads file named udfTile_xxx.egads that
       contains this tile */
    if (DUMPEGADS(numUdp) > 0) {
        sprintf(filename, "udfTile_%03d.egads", CBfunData->count);

        status = EG_makeTopology(MODL->context, NULL, MODEL, 0, NULL,
                                 1, &(CBfunData->tile), NULL, &emodel);
        CHECK_STATUS(EG_makeTopology);

        printf("    writing \"%s\"\n", filename);
        status = EG_saveModel(emodel, filename);
        CHECK_STATUS(EG_saveModel);

        status = EG_deleteObject(emodel);
        CHECK_STATUS(EG_deleteObject);

        (CBfunData->count)++;
    }

    /* set an attribute on each Face depending on its position
       relative to the unit cube */
    status = EG_getBodyTopos(CBfunData->tile, NULL, FACE, &nface, &efaces);
    CHECK_STATUS(EG_getBodyTopos);

    for (iface = 0; iface < nface; iface++) {
        status = EG_getBoundingBox(efaces[iface], bbox);
        CHECK_STATUS(EG_getBoundingBox);

        if        (bbox[3] <=   toler) {
            locn = -1;
        } else if (bbox[0] >= 1-toler) {
            locn = +1;
        } else if (bbox[4] <=   toler) {
            locn = -2;
        } else if (bbox[1] >= 1-toler) {
            locn = +2;
        } else if (bbox[5] <=   toler) {
            locn = -3;
        } else if (bbox[2] >= 1-toler) {
            locn = +3;
        } else {
            locn =  0;
        }

        /* exclude Faces within another Face */
        for (jface = 0; jface < nface; jface++) {
            if (iface == jface) continue;

            status = EG_getBoundingBox(efaces[jface], bbox2);
            CHECK_STATUS(EG_getBoundingBox);

            if ((bbox[3]-bbox[0] < toler && bbox2[3]-bbox2[0] < toler) ||
                (bbox[4]-bbox[1] < toler && bbox2[4]-bbox2[1] < toler) ||
                (bbox[5]-bbox[2] < toler && bbox2[5]-bbox2[2] < toler)   ) {
                if (bbox[0] >= bbox2[0]-toler && bbox[3] <= bbox2[3]+toler &&
                    bbox[1] >= bbox2[1]-toler && bbox[4] <= bbox2[4]+toler &&
                    bbox[2] >= bbox2[2]-toler && bbox[5] <= bbox2[5]+toler   ) {

                    /* make sure iface and jface share at least one Edge */
                    status = EG_getTopology(efaces[iface], &eref, &oclass, &mtype, data,
                                            &nloop, &eloops, &senses);
                    CHECK_STATUS(EG_getTopology);
                    if (nloop > 1) continue;
                    status = EG_getTopology(eloops[0], &eref, &oclass, &mtype, data,
                                            &nedgei, &eedgesi, &senses);
                    CHECK_STATUS(EG_getTopology);

                    status = EG_getTopology(efaces[jface], &eref, &oclass, &mtype, data,
                                            &nloop, &eloops, &senses);
                    CHECK_STATUS(EG_getTopology);
                    if (nloop > 1) continue;
                    status = EG_getTopology(eloops[0], &eref, &oclass, &mtype, data,
                                            &nedgej, &eedgesj, &senses);
                    CHECK_STATUS(EG_getTopology);

                    matches = 0;
                    for (iedge = 0; iedge < nedgei; iedge++) {
                        for (jedge = 0; jedge < nedgej; jedge++) {
                            if (eedgesi[iedge] == eedgesj[jedge]) {
                                matches++;
                            }
                        }
                    }

                    if (matches != 0) {
#ifdef DEBUG
                        printf("excluding iface=%3d (matches=%2d)\n", iface+1, matches);
#endif
                        locn = 99;
                        break;
                    }
                }
            }
        }

        status = EG_attributeAdd(efaces[iface], "__locn__",
                                 ATTRINT, 1, &locn,  NULL, NULL);
        CHECK_STATUS(EG_attributeAdd);
    }

    EG_free(efaces);

    /* make list of all non-bounding Bsplines */
    status = makeBSplineList(CBData, &nBSpline, &BSplines);
    CHECK_STATUS(makeBSplineList);

    if (CBData->TileIdxsMax[0] == 1 && CBData->TileIdxsMax[1] == 1 && CBData->TileIdxsMax[2] == 1) {
        CBfunData->iutile++;
        if (CBfunData->iutile == CBfunData->nutile) {
            CBfunData->iutile = 0;
            CBfunData->ivtile++;
            if (CBfunData->ivtile == CBfunData->nvtile) {
                CBfunData->ivtile = 0;
                CBfunData->iwtile++;
                if (CBfunData->iwtile == CBfunData->nwtile) {
                    CBfunData->iutile = 0;
                    CBfunData->ivtile = 0;
                    CBfunData->iwtile = 0;
                }
            }
        }
    }

    if (Tile != NULL) {
        IPFreeObject(Tile);                            /* Free the old (dummy) tile */
    }

    /* concatenate the IRIT surfaces */
    for (j = 0; j < nBSpline; j++) {
        status = makeIRITsrf(BSplines[j], &srf);
        EG_deleteObject(BSplines[j]);

        if (status < EGADS_SUCCESS) continue;
        if (Srfs == NULL) {
            Srfs = srf;
        } else {
            Last->Pnext = srf;
        }
        Last = srf;
    }
    FREE(BSplines);

    /* make the tile */
    IPListObj = IPLnkListToListObject(Srfs, IP_OBJ_SURFACE);
    Tile      = GMTransformObject(IPListObj, CBData->Mat);

    IPFreeObject(IPListObj);

cleanup:
    return Tile;
}


/*
 ************************************************************************
 *                                                                      *
 *   PostProcessTile - callback function during tiling process          *
 *                                                                      *
 ************************************************************************
 */

static /*@null@*/ IPObjectStruct*
PostProcessTile(IPObjectStruct                  *Tile,
               UserMicroPostProcessTileCBStruct *CBData)
{
    int     status = EGADS_SUCCESS;

    int     periodic, i, j, k, l, Idx, nsurf=0, header[7], ldata2;
    int     nfaceOrig, iface, oclass, mtype, nloop, *senses;
    int     atype, alen;
    CINT    *tempIlist;
    double  UVbox[4], data[4], *data2=NULL;
    CDOUBLE *tempRlist;
    CCHAR   *tempClist;
    ego     *efacesOrig=NULL, eref, geom, eface, *eloops;

    IPObjectStruct              *srfObj;
    CagdSrfStruct               *srf;

    UserLocalDataStruct *CBfunData = (UserLocalDataStruct *) CBData->CBFuncData;

    ROUTINE(PostProcessTile);

    /* --------------------------------------------------------------- */

    /* get the Faces associated with the original Tile */
    status = EG_getBodyTopos(CBfunData->tile, NULL, FACE,
                             &nfaceOrig, &efacesOrig);
    CHECK_STATUS(EG_getBodyTopos);

    /* make/extend the array that will hold the Faces made in each of
       the Tiles */
    nsurf = IPListObjectLength(Tile);
    assert(nsurf > 0);

    if (CBfunData->nface == 0) {
        MALLOC(CBfunData->eface, ego,                  nsurf);
    } else {
        RALLOC(CBfunData->eface, ego, CBfunData->nface+nsurf);
    }

    /* loop over IRIT surfaces and (re)populate EGADS */
    i = -1;
    for (iface = 0; iface < nfaceOrig; iface++) {
        status = EG_attributeRet(efacesOrig[iface], "__skip__", &atype, &alen,
                        &tempIlist, &tempRlist, &tempClist);
        CHECK_STATUS(EG_attributeRet);

        if (tempIlist[0] == 1) continue;
        i++;
        srfObj = IPListObjectGet(Tile, i);

        if (srfObj == NULL) {
            printf(" udpExecute: NULL object %d %d\n", i, nsurf);
            continue;
        } else if (!IP_IS_SRF_OBJ(srfObj)) {
            printf(" udpExecute: object %d %d not a srf object\n", i, nsurf);
            continue;
        }

        /* the 4 can be changed to 0, 3, or 4 to control precision of result */
        UserCnvrtObjApproxLowOrderBzr(srfObj, 4);

        srf = srfObj->U.Srfs;
        if (srf == NULL) {
            printf(" udpExecute: NULL surface %d %d\n", i, nsurf);
            continue;
        }

        if (CAGD_IS_BEZIER_SRF(srf)) {
            CagdSrfStruct *newSrf = CagdCnvrtBzr2BspSrf(srf);
            CagdSrfFree(srf);
            srf = newSrf;
        }
        if (!CAGD_IS_BSPLINE_SRF(srf)) {
            printf(" udpExecute: surface %d (of %d) is not BSpline\n", i, nsurf);
            continue;
        }

        /* set up header */
        header[0] = 0;
        if (CAGD_IS_RATIONAL_PT(srf->PType) != 0) header[0] |= 2;
        if (srf->UPeriodic                  != 0) header[0] |= 4;
        if (srf->VPeriodic                  != 0) header[0] |= 8;
        header[1] = srf->UOrder - 1;
        header[2] = srf->ULength;
        header[3] = srf->ULength + srf->UOrder + (srf->UPeriodic ? srf->UOrder - 1 : 0);
        header[4] = srf->VOrder - 1;
        header[5] = srf->VLength;
        header[6] = srf->VLength + srf->VOrder + (srf->VPeriodic ? srf->VOrder - 1 : 0);

        /* set up data2 */
        ldata2 = header[3] + header[6] + 3*header[2]*header[5];
        if (CAGD_IS_RATIONAL_PT(srf->PType) != 0) ldata2 += header[2]*header[5];

        MALLOC(data2, double, ldata2);

        for (k = j = 0; j < header[3]; j++, k++) {
            data2[k] = srf->UKnotVector[j];
        }
        for (    j = 0; j < header[6]; j++, k++) {
            data2[k] = srf->VKnotVector[j];
        }
        for (j = 0; j < header[5]; j++){
            for (l = 0; l < header[2]; l++, k+=3) {
                Idx       = CAGD_MESH_UV(srf, l, j);
                data2[k  ] = srf->Points[1][Idx];   /* X coefs */
                data2[k+1] = srf->Points[2][Idx];   /* Y coefs */
                data2[k+2] = srf->Points[3][Idx];   /* Z coefs */
            }
        }
        if (CAGD_IS_RATIONAL_PT(srf->PType) != 0) {
            for (j = 0; j < header[5]; j++) {
                for (l = 0; l < header[2]; l++, k++) {
                    Idx     = CAGD_MESH_UV(srf, l, j);
                    data2[k] = srf->Points[0][Idx];   /* Weights */
                }
            }
        }

        CagdSrfFree(srf);

        /* make the surface */
        status = EG_makeGeometry(CBfunData->context, SURFACE, BSPLINE, NULL, header, data2, &geom);
        CHECK_STATUS(EG_makeGeometry);

        FREE(data2);

        status = EG_getRange(geom, UVbox, &periodic);
        CHECK_STATUS(EG_getRange);

        /* if the corresponding Face in the Tile is untrimmed, make the Face the simple way */
        status = EG_getTopology(efacesOrig[i], &eref, &oclass, &mtype, data,
                                &nloop, &eloops, &senses);
        CHECK_STATUS(EG_getTopology);
//$$$        printf("eref=%lx, oclass=%d, mtype=%d, nloop=%d\n", (long)eref, oclass, mtype, nloop);
//$$$
//$$$        status = EG_getTolerance(efacesOrig[i], &toler);
//$$$        CHECK_STATUS(EG_getTolerance);
//$$$        printf("toler=%e\n", toler);

        if (nloop == 1) {
            status = EG_makeFace(geom, SFORWARD, UVbox, &eface);
            CHECK_STATUS(EG_makeFace);

        /* otherwise, create the necessary Loops from the original Face's pcurves */
        } else {
            printf("we should not be here\n");
            exit(0);
//$$$            int iloop, nedge, iedge;
//$$$            ego eref2, *eedges, *eloops2=NULL, *eedges2=NULL;
//$$$
//$$$            printf("Face %d is trimmed, nloop=%d (%d)\n", i, nloop, eref->mtype);
//$$$
//$$$            MALLOC(eloops2, ego, nloop);
//$$$
//$$$            for (iloop = 0; iloop < nloop; iloop++) {
//$$$                printf("iloop=%d\n", iloop);
//$$$                status = EG_getTopology(eloops[iloop], &eref2, &oclass, &mtype, data,
//$$$                                        &nedge, &eedges, &senses);
//$$$                printf("EG_getTopology -> status=%d, oclass=%d, mtype=%d, nedge=%d\n", status, oclass, mtype, nedge);
//$$$                CHECK_STATUS(EG_getTopology);
//$$$
//$$$                MALLOC(eedges2, ego, 2*nedge);
//$$$
//$$$                ego enodes[2];
//$$$                double xyz[3];
//$$$
//$$$                for (iedge = 0; iedge < nedge; iedge++) {
//$$$                    printf("iedge=%d\n", iedge);
//$$$                    double trange[4];
//$$$                    status = EG_getRange(eedges[iedge], trange, &periodic);
//$$$                    CHECK_STATUS(EG_getRange);
//$$$
//$$$                    if (eref->mtype == PLANE) {
//$$$                        printf("we should not be here\n");
//$$$                        exit(0);
//$$$                    } else if (eedges[nedge+iedge]->mtype == LINE) {
//$$$                        status = EG_convertToBSplineRange(eedges[nedge+iedge], trange, &eedges2[nedge+iedge]);
//$$$                        ocsmPrintEgo(eedges2[nedge+iedge]);
//$$$                        CHECK_STATUS(EG_convertToBsplineRange);
//$$$                    } else {
//$$$                        eedges2[nedge+iedge] = eedges[nedge+iedge];
//$$$                        ocsmPrintEgo(eedges2[nedge+iedge]);
//$$$                    }
//$$$                    ocsmPrintEgo(geom);
//$$$                    status = EG_otherCurve(geom, eedges2[nedge+iedge], toler, &ecurve);
//$$$                    printf("EG_otherCurveA -> status=%d\n", status);
//$$$                    CHECK_STATUS(EG_otherCurve);
//$$$
//$$$                    if (iedge == 0) {
//$$$                        EG_evaluate();
//$$$                    }
//$$$
//$$$                    status = EG_makeTopology(CBfunData->context, ecurve, EDGE, TWONODE,
//$$$                                             trange, 2, enodes, NULL, &eedges2[iedge]);
//$$$
//$$$                }
//$$$
//$$$                status = EG_makeTopology(CBfunData->context, geom, LOOP, CLOSED, NULL,
//$$$                                         nedge, eedges2, senses, &eloops2[iloop]);
//$$$                printf("EG_makeTopology(LOOP) -> status=%d\n", status);
//$$$                CHECK_STATUS(EG_makeTopology);
//$$$
//$$$                FREE(eedges2);
//$$$            }
//$$$
//$$$            status = EG_makeTopology(CBfunData->context, geom, FACE, SFORWARD, NULL,
//$$$                                     nloop, eloops2, NULL, &eface);
//$$$            printf("EG_makeTopology(FACE) -> status=%d\n", status);
//$$$            CHECK_STATUS(EG_makeTopology);
//$$$
//$$$            FREE(eloops);
        }

        /* color the Face */
        if        (CBfunData->colors[i] == 1) {
            status = EG_attributeAdd(eface, "_color", ATTRSTRING, 1, NULL, NULL, "red");
            CHECK_STATUS(EG_attributeAdd);
        } else if (CBfunData->colors[i] == 2) {
            status = EG_attributeAdd(eface, "_color", ATTRSTRING, 1, NULL, NULL, "green");
            CHECK_STATUS(EG_attributeAdd);
        } else if (CBfunData->colors[i] == 3) {
            status = EG_attributeAdd(eface, "_color", ATTRSTRING, 1, NULL, NULL, "blue");
            CHECK_STATUS(EG_attributeAdd);
        } else if (CBfunData->colors[i] == 4) {
            status = EG_attributeAdd(eface, "_color", ATTRSTRING, 1, NULL, NULL, "yellow");
            CHECK_STATUS(EG_attributeAdd);
        } else if (CBfunData->colors[i] == 5) {
            status = EG_attributeAdd(eface, "_color", ATTRSTRING, 1, NULL, NULL, "magenta");
            CHECK_STATUS(EG_attributeAdd);
        } else if (CBfunData->colors[i] == 6) {
            status = EG_attributeAdd(eface, "_color", ATTRSTRING, 1, NULL, NULL, "cyan");
            CHECK_STATUS(EG_attributeAdd);
        }

        CBfunData->eface[CBfunData->nface] = eface;
        CBfunData->nface++;
    }

cleanup:
    /* get rid of ocsm modl */
    if (CBfunData->modl != NULL) {
        (void) ocsmFree(CBfunData->modl);
    }

    if (efacesOrig != NULL) EG_free(efacesOrig);

    FREE(CBfunData->colors);
    FREE(data2);

    return NULL;
}


/*
 ************************************************************************
 *                                                                      *
 *   makeBSplineList - fill CBData with BSplines                        *
 *                                                                      *
 ************************************************************************
 */

static int
makeBSplineList(UserMicroPreProcessTileCBStruct *CBData,
                int                             *nBSpl,
                ego                             **BSpl)
{
    int status = EGADS_SUCCESS;

    int          nface, i, j, atype, alen, *include=NULL, skip;
    CINT         *tempIlist;
    CDOUBLE      *tempRlist;
    CCHAR        *tempClist;
    ego          *efaces;

    UserLocalDataStruct *CBfunData = (UserLocalDataStruct *) CBData->CBFuncData;

    ROUTINE(makeBSplineList);

    /* --------------------------------------------------------------- */

    /* default returns */
    *nBSpl = 0;
    *BSpl  = NULL;

    status = EG_getBodyTopos(CBfunData->tile, NULL, FACE, &nface, &efaces);
    CHECK_STATUS(EG_getBodyTopos);

    MALLOC(include,           int, nface);
    MALLOC(CBfunData->colors, int, nface);

    for (i = 0; i < nface; i++) {
        status = EG_attributeRet(efaces[i], "__locn__", &atype, &alen,
                                 &tempIlist, &tempRlist, &tempClist);
        CHECK_STATUS(EG_attributeRet);

        /* include the surfaces where the Face was marked with __locn__ == 0 */
        if (tempIlist[0] == 0) {
            include[(*nBSpl)++] = i;
            skip = 0;

        /* include the surfaces where the Face was marked with a __locn__ != 0
           but at the end of the domain */
        } else if (tempIlist[0] == -1 && CBData->TileIdxsMin[0] == 0 && CBfunData->iutile == 0                  ) {
            include[(*nBSpl)++] = i;
            skip = 0;

        } else if (tempIlist[0] == +1 && CBData->TileIdxsMax[0] == 1 && CBfunData->iutile == CBfunData->nutile-1) {
            include[(*nBSpl)++] = i;
            skip = 0;

        } else if (tempIlist[0] == -2 && CBData->TileIdxsMin[1] == 0 && CBfunData->ivtile == 0                  ) {
            include[(*nBSpl)++] = i;
            skip = 0;

        } else if (tempIlist[0] == +2 && CBData->TileIdxsMax[1] == 1 && CBfunData->ivtile == CBfunData->nvtile-1) {
            include[(*nBSpl)++] = i;
            skip = 0;

        } else if (tempIlist[0] == -3 && CBData->TileIdxsMin[2] == 0 && CBfunData->iwtile == 0                  ) {
            include[(*nBSpl)++] = i;
            skip = 0;

        } else if (tempIlist[0] == +3 && CBData->TileIdxsMax[2] == 1 && CBfunData->iwtile == CBfunData->nwtile-1) {
            include[(*nBSpl)++] = i;
            skip = 0;

        } else {
            skip = 1;
        }

        EG_attributeAdd(efaces[i], "__skip__", ATTRINT, 1, &skip, NULL, NULL);
    }

    if (*nBSpl == 0) {
        printf(" makeBSplineList: no Faces selected\n");
        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    *BSpl = NULL;
    MALLOC(*BSpl, ego, *nBSpl);

    for (j = 0; j < *nBSpl; j++) {
        i      = include[j];
        status = EG_copyObject(efaces[i], NULL, &(*BSpl)[j]);
        CHECK_STATUS(EG_copyObject);

        status = EG_attributeRet(efaces[i], "_color", &atype, &alen,
                                 &tempIlist, &tempRlist, &tempClist);
        if (status == EGADS_SUCCESS) {
            if        (strcmp(tempClist, "red") == 0) {
                CBfunData->colors[j] = 1;
            } else if (strcmp(tempClist, "green") == 0) {
                CBfunData->colors[j] = 2;
            } else if (strcmp(tempClist, "blue") == 0) {
                CBfunData->colors[j] = 3;
            } else if (strcmp(tempClist, "yellow") == 0) {
                CBfunData->colors[j] = 4;
            } else if (strcmp(tempClist, "magenta") == 0) {
                CBfunData->colors[j] = 5;
            } else if (strcmp(tempClist, "cyan") == 0) {
                CBfunData->colors[j] = 6;
            }
        } else {
            CBfunData->colors[j] = 0;
            status = EGADS_SUCCESS;
        }
    }

cleanup:
    FREE(include);
    EG_free(efaces);

    return status;
}


/*
 ************************************************************************
 *                                                                      *
 *   makeIRITsrf -- convert EGADS BSPINE into IRIT Surface              *
 *                                                                      *
 ************************************************************************
 */

static int
makeIRITsrf(ego           eobj,         /* (in)  Egads surface of Face */
            CagdSrfStruct **Srf)        /* (in)  IRIT  surface (or NULL) */
{
    int     status = EGADS_SUCCESS;

    int     i, j, k, indx, oclass, mtype, *ivec=NULL, nloop, nedge, nnode, nchild, *senses, *senses2;
    double  *rvec=NULL, data[4], xyzsw[4], xyzse[4], xyznw[4], xyzne[4];
    ego     topRef, eprev, enext, eref, esurf, *eloops, *eedges, *enodes, *echilds;

    ROUTINE(makeIRITsrf);

    /* --------------------------------------------------------------- */

    /* default return */
    *Srf = NULL;

    /* get type of eobj */
    status = EG_getInfo(eobj, &oclass, &mtype, &topRef, &eprev, &enext);
    CHECK_STATUS(EG_getInfo);

    /* eobj is a Surface */
    if (oclass == SURFACE) {
        esurf = eobj;

        /* make sure esurf is a BSpline (and not a NURB and not periodic) */
        status = EG_getGeometry(esurf, &oclass, &mtype, &eref, &ivec, &rvec);
        CHECK_STATUS(EG_getGeometry);

        if (mtype != BSPLINE) {
            printf(" makeIRITsrf: esurf NOT a BSpline = %d\n", mtype);
            status = EGADS_GEOMERR;
            goto cleanup;
        } else if (ivec[0] != 0) {
            printf(" makeIRITsrf: BSpline flags = %d\n", ivec[0]);
            status = EGADS_GEOMERR;
            goto cleanup;
        }

        /* start a new IRIT BSpline surface */
        *Srf = BspSrfNew(ivec[2], ivec[5], ivec[1]+1, ivec[4]+1, CAGD_PT_E3_TYPE);

        /* Populate the control points in Srf */
        k = ivec[3] + ivec[6];
        for (j = 0; j < ivec[5]; j++) {
            for (i = 0; i < ivec[2]; i++) {
                indx = CAGD_MESH_UV(*Srf, i, j);
                (*Srf)->Points[1][indx] = rvec[k++];   /* X coefs */
                (*Srf)->Points[2][indx] = rvec[k++];   /* Y coefs */
                (*Srf)->Points[3][indx] = rvec[k++];   /* Z coefs */
            }
        }

        /* Copy the knot vectors into Srf */
        for (i = 0; i < ivec[3]; i++) {
            (*Srf)->UKnotVector[i] = rvec[i];
        }
        for (i = 0; i < ivec[6]; i++) {
            (*Srf)->VKnotVector[i] = rvec[i+ivec[3]];
        }

    /* eobj is a Face */
    } else if (oclass == FACE) {
        status = EG_getTopology(eobj, &eref, &oclass, &mtype, data,
                                &nloop, &eloops, &senses);
        CHECK_STATUS(EG_getTopology);

//$$$        if (nloop != 1) {
//$$$            printf(" makeIritsrf: Face has %d Loops\n", nloop);
//$$$            status = EGADS_GEOMERR;
//$$$            goto cleanup;
//$$$        }

        status = EG_getTopology(eloops[0], &eref, &oclass, &mtype, data,
                                &nedge, &eedges, &senses);
        CHECK_STATUS(EG_getTopology);

        /* get the 4 corners as the beginning/end of each Edge */
        if (nedge == 4) {
            status = EG_getTopology(eedges[0], &eref, &oclass, &mtype, data,
                                     &nnode, &enodes, &senses2);
            CHECK_STATUS(EG_getTopology);
            if (senses[0] == SFORWARD) {
                status = EG_getTopology(enodes[0], &eref, &oclass, &mtype, xyzsw,
                                        &nchild, &echilds, &senses2);
            } else {
                status = EG_getTopology(enodes[1], &eref, &oclass, &mtype, xyzsw,
                                        &nchild, &echilds, &senses2);
            }
            CHECK_STATUS(EG_getTopology);

            status = EG_getTopology(eedges[1], &eref, &oclass, &mtype, data,
                                     &nnode, &enodes, &senses2);
            CHECK_STATUS(EG_getTopology);
            if (senses[1] == SFORWARD) {
                status = EG_getTopology(enodes[0], &eref, &oclass, &mtype, xyzse,
                                        &nchild, &echilds, &senses2);
            } else {
                status = EG_getTopology(enodes[1], &eref, &oclass, &mtype, xyzse,
                                        &nchild, &echilds, &senses2);
            }
            CHECK_STATUS(EG_getTopology);

            status = EG_getTopology(eedges[2], &eref, &oclass, &mtype, data,
                                     &nnode, &enodes, &senses2);
            CHECK_STATUS(EG_getTopology);
            if (senses[2] == SFORWARD) {
                status = EG_getTopology(enodes[0], &eref, &oclass, &mtype, xyzne,
                                        &nchild, &echilds, &senses2);
            } else {
                status = EG_getTopology(enodes[1], &eref, &oclass, &mtype, xyzne,
                                        &nchild, &echilds, &senses2);
            }
            CHECK_STATUS(EG_getTopology);

            status = EG_getTopology(eedges[3], &eref, &oclass, &mtype, data,
                                     &nnode, &enodes, &senses2);
            CHECK_STATUS(EG_getTopology);
            if (senses[3] == SFORWARD) {
                status = EG_getTopology(enodes[0], &eref, &oclass, &mtype, xyznw,
                                        &nchild, &echilds, &senses2);
            } else {
                status = EG_getTopology(enodes[1], &eref, &oclass, &mtype, xyznw,
                                        &nchild, &echilds, &senses2);
            }
            CHECK_STATUS(EG_getTopology);

            /* make an untrimmed Bspline Surface */
            *Srf = BspSrfNew(2, 2, 2, 2, CAGD_PT_E3_TYPE);

            /* knot vectors */
            (*Srf)->UKnotVector[0] = 0;
            (*Srf)->UKnotVector[1] = 0;
            (*Srf)->UKnotVector[2] = 1;
            (*Srf)->UKnotVector[3] = 1;

            (*Srf)->VKnotVector[0] = 0;
            (*Srf)->VKnotVector[1] = 0;
            (*Srf)->VKnotVector[2] = 1;
            (*Srf)->VKnotVector[3] = 1;

            /* control points */
            indx = CAGD_MESH_UV(*Srf, 0, 0);
            (*Srf)->Points[1][indx] = xyzsw[0];
            (*Srf)->Points[2][indx] = xyzsw[1];
            (*Srf)->Points[3][indx] = xyzsw[2];

            indx = CAGD_MESH_UV(*Srf, 1, 0);
            (*Srf)->Points[1][indx] = xyzse[0];
            (*Srf)->Points[2][indx] = xyzse[1];
            (*Srf)->Points[3][indx] = xyzse[2];

            indx = CAGD_MESH_UV(*Srf, 0, 1);
            (*Srf)->Points[1][indx] = xyznw[0];
            (*Srf)->Points[2][indx] = xyznw[1];
            (*Srf)->Points[3][indx] = xyznw[2];

            indx = CAGD_MESH_UV(*Srf, 1, 1);
            (*Srf)->Points[1][indx] = xyzne[0];
            (*Srf)->Points[2][indx] = xyzne[1];
            (*Srf)->Points[3][indx] = xyzne[2];

        /* if Face does not have 4 Nodes but is a planar
           Face (hopefully with only a notch), so that we can
           convert Face to BSpline and then use it directly */
        } else {
            status = EG_convertToBSpline(eobj, &esurf);
            CHECK_STATUS(EG_convertToBSpline);

            status = EG_getGeometry(esurf, &oclass, &mtype, &eref, &ivec, &rvec);
            CHECK_STATUS(EG_getGeometry);

            /* start a new IRIT BSpline surface */
            *Srf = BspSrfNew(ivec[2], ivec[5], ivec[1]+1, ivec[4]+1, CAGD_PT_E3_TYPE);

            /* Populate the control points in Srf */
            k = ivec[3] + ivec[6];
            for (j = 0; j < ivec[5]; j++) {
                for (i = 0; i < ivec[2]; i++) {
                    indx = CAGD_MESH_UV(*Srf, i, j);
                    (*Srf)->Points[1][indx] = rvec[k++];   /* X coefs */
                    (*Srf)->Points[2][indx] = rvec[k++];   /* Y coefs */
                    (*Srf)->Points[3][indx] = rvec[k++];   /* Z coefs */
                }
            }

            /* Copy the knot vectors into Srf */
            for (i = 0; i < ivec[3]; i++) {
                (*Srf)->UKnotVector[i] = rvec[i];
            }
            for (i = 0; i < ivec[6]; i++) {
                (*Srf)->VKnotVector[i] = rvec[i+ivec[3]];
            }
        }

    /* eobj is an unknown type */
    } else {
        printf(" makeIRITsrf: eobj is neither Surface nor Face (oclass=%d)\n", oclass);
        status = EGADS_GEOMERR;
        goto cleanup;
    }

cleanup:
    if (ivec != NULL) EG_free(ivec);
    if (rvec != NULL) EG_free(rvec);

    return status;
}


/*
 ************************************************************************
 *                                                                      *
 *   FlattenHierarchy2LinearList - convert a hierarchy of geometries    *
 *                                 into a linear list of geometries     *
 *                                                                      *
 ************************************************************************
 */

//$$$static void
//$$$FlattenHierarchy2LinearList(const IPObjectStruct *MS,
//$$$                            IPObjectStruct       *LinMS)
//$$${
//$$$    int i;
//$$$
//$$$    switch (MS->ObjType) {
//$$$    case IP_OBJ_LIST_OBJ:
//$$$        for (i = 0; i < IPListObjectLength(MS); i++) {
//$$$            FlattenHierarchy2LinearList(IPListObjectGet(MS, i), LinMS);
//$$$        }
//$$$        break;
//$$$    default:
//$$$        IPListObjectAppend(LinMS, IPCopyObject(NULL, MS, TRUE));
//$$$        break;
//$$$    }
//$$$}


/*
 ************************************************************************
 *                                                                      *
 *   trilinear - trilinear interpolation (extrapolation) into table     *
 *                                                                      *
 ************************************************************************
 */

static void
trilinear(table_T  *table,
          double   u,
          double   v,
          double   w,
          double   dv[])
{
    int      ileft, irite, imid, jleft, jrite, jmid, kleft, krite, kmid, m;
    double   fracu, fracv, fracw;

    /* binary search in u (i) direction */
    ileft = 0;
    irite = table->nu - 1;

    while (irite > ileft+1) {
        imid = (ileft + irite) / 2;
        if (table->u[imid] <= u) {
            ileft = imid;
        } else {
            irite = imid;
        }
    }

    fracu = (u - table->u[ileft]) / (table->u[irite] - table->u[ileft]);

    /* binary search in v (j) direction */
    jleft = 0;
    jrite = table->nv - 1;

    while (jrite > jleft+1) {
        jmid = (jleft + jrite) / 2;
        if (table->v[jmid] <= v) {
            jleft = jmid;
        } else {
            jrite = jmid;
        }
    }

    fracv = (v - table->v[jleft]) / (table->v[jrite] - table->v[jleft]);

    /* binary search in w (k) direction */
    kleft = 0;
    krite = table->nw - 1;

    while (krite > kleft+1) {
        kmid = (kleft + krite) / 2;
        if (table->w[kmid] <= w) {
            kleft = kmid;
        } else {
            krite = kmid;
        }
    }

    fracw = (w - table->w[kleft]) / (table->w[krite] - table->w[kleft]);

    assert(ileft >= 0);
    assert(irite < table->nu);
    assert(jleft >= 0);
    assert(jrite < table->nv);
    assert(kleft >= 0);
    assert(krite < table->nw);

#define IJK(M,I,J,K)            (M)+table->rank*((I)+table->nu*((J)+table->nv*(K)))
#define DV(M,I,J,K)   table->dv[(M)+table->rank*((I)+table->nu*((J)+table->nv*(K)))]

    /* perform trilinear interpolation */
    for (m = 0; m < table->rank; m++) {
        assert(IJK(m,ileft,jleft,kleft) >= 0);
        assert(IJK(m,irite,jrite,krite) < table->nu*table->nv*table->nw*table->rank);
        dv[m]
            = (1-fracu) * (1-fracv) * (1-fracw) * DV(m,ileft,jleft,kleft)
            + (  fracu) * (1-fracv) * (1-fracw) * DV(m,irite,jleft,kleft)
            + (1-fracu) * (  fracv) * (1-fracw) * DV(m,ileft,jrite,kleft)
            + (  fracu) * (  fracv) * (1-fracw) * DV(m,irite,jrite,kleft)
            + (1-fracu) * (1-fracv) * (  fracw) * DV(m,ileft,jleft,krite)
            + (  fracu) * (1-fracv) * (  fracw) * DV(m,irite,jleft,krite)
            + (1-fracu) * (  fracv) * (  fracw) * DV(m,ileft,jrite,krite)
            + (  fracu) * (  fracv) * (  fracw) * DV(m,irite,jrite,krite);
    }

#undef DV
}
