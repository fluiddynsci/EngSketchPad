/*
 ************************************************************************
 *                                                                      *
 * udfOffset -- make an offset of a planar WireBody or SheetBody        *
 *                                                                      *
 *            Written by John Dannenhoffer @ Syracuse University        *
 *            Patterned after code written by Bob Haimes  @ MIT         *
 *                                                                      *
 ************************************************************************
 */

/*
 * Copyright (C) 2011/2022  John F. Dannenhoffer, III (Syracuse University)
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

#define NUMUDPARGS 1
#define NUMUDPINPUTBODYS 1
#include "udpUtilities.h"

/* shorthands for accessing argument values and velocities */
#define DIST(IUDP)  ((double *) (udps[IUDP].arg[0].val))[0]

/* data about possible arguments */
static char  *argNames[NUMUDPARGS] = {"dist",   };
static int    argTypes[NUMUDPARGS] = {ATTRREAL, };
static int    argIdefs[NUMUDPARGS] = {0,        };
static double argDdefs[NUMUDPARGS] = {0.,       };

/* get utility routines: udpErrorStr, udpInitialize, udpReset, udpSet,
                         udpGet, udpVel, udpClean, udpMesh */
#include "udpUtilities.c"

#include "OpenCSM.h"
#include <assert.h>

#define EPS12     1.0e-12
#define EPS30     1.0e-30

#define MIN(A,B)  (((A) < (B)) ? (A) : (B))
#define MAX(A,B)  (((A) < (B)) ? (B) : (A))

#ifdef GRAFIC
   #include "grafic.h"
#endif

/* prototypes */
static int intersectLines(int npnt1, double xyz1[], int npnt2, double xyz2[], int iplane, double *t1, double *t2);


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

    int     oclass, mtype, nchild, *senses, iplane, nloop, nedge, medge, iedge, jedge, ipnt, nchange, isave;
    int     periodic, sizes[2], ntemp1, ntemp2, *ibeg=NULL, *iend=NULL, *active=NULL, *newSenses=NULL;
    double  data[18], bbox[6], xplane=0, yplane=0, zplane=0, tt, trange[4], alen, fact, frac;
    double  **xyz=NULL, *tbeg=NULL, *tend=NULL;
    char    *message=NULL;
    ego     context, eref, *ebodys, *echilds, *eedges=NULL, *eloops=NULL;
    ego     *newCurves=NULL, *newNodes=NULL, *newEdges=NULL;
    ego     newLoop, newFace, newShell, topRef, eprev, enext;

    int     npnt_min=5, npnt_max=201;

#ifdef GRAFIC
    float   *xplot=NULL, *yplot=NULL;
    int     io_kbd=5, io_scr=6, indgr=1+2+4+16+64;
    int     nline, nplot, *ilin=NULL, *isym=NULL, *nper=NULL;
    char    pltitl[80];
#endif

    ROUTINE(udpExecute);

    /* --------------------------------------------------------------- */

#ifdef DEBUG
    printf("udpExecute(emodel=%llx)\n", (long long)emodel);
    printf("dist(0) = %f\n", DIST(0));
#endif

    /* default return values */
    *ebody  = NULL;
    *nMesh  = 0;
    *string = NULL;

    MALLOC(message, char, 1024);
    message[0] = '\0';

    /* check inputs */
    if (udps[0].arg[0].size != 1) {
        snprintf(message, 1024, "dist must be a scalar");
        status = EGADS_RANGERR;
        goto cleanup;
    }

    /* check that Model was input that contains one Body */
    status = EG_getTopology(emodel, &eref, &oclass, &mtype,
                            data, &nchild, &ebodys, &senses);
    CHECK_STATUS(EG_getTopology);

    if (oclass != MODEL) {
        snprintf(message, 1024, "expecting a Model\n");
        status = EGADS_NOTMODEL;
        goto cleanup;
    } else if (nchild != 1) {
        snprintf(message, 1024, "expecting Model to contain one Body (not %d)\n", nchild);
        status = EGADS_NOTBODY;
        goto cleanup;
    }

    /* make sure input is WireBody or SheetBody */
    status = EG_getTopology(ebodys[0], &eref, &oclass, &mtype,
                            data, &nchild, &echilds, &senses);
    CHECK_STATUS(EG_getTopology);

    if (mtype != WIREBODY && mtype != FACEBODY && mtype != SHEETBODY) {
        snprintf(message, 1024, "input must be WireBody or SheetBody\n");
        status = EGADS_NOTBODY;
        goto cleanup;
    }

    /* make sure body only contains one loop */
    status = EG_getBodyTopos(ebodys[0], NULL, LOOP, &nloop, &eloops);
    CHECK_STATUS(EG_getBodyTopos);

    if (nloop != 1) {
        snprintf(message, 1024, "input Body has %d Loops, but was expecting one\n", nloop);
        status = EGADS_NOTBODY;
        goto cleanup;
    }

    /* if a WireBody, make sure it is not non-manifold */
    if (mtype == WIREBODY) {
        status = EG_getBodyTopos(ebodys[0], NULL, EDGE, &ntemp1, NULL);
        CHECK_STATUS(EG_getBodyTopos);

        SPLINT_CHECK_FOR_NULL(eloops);

        status = EG_getTopology(eloops[0], &eref, &oclass, &mtype,
                                data, &ntemp2, &echilds, &senses);
        CHECK_STATUS(EG_getTopology);

        if (ntemp1 != ntemp2) {
            snprintf(message, 1024, "Input WireBody must be manifold\n");
            status = EGADS_NOTBODY;
            goto cleanup;
        }
    }

    /* make sure input is aligned with one of the coordinate planes */
    status = EG_getBoundingBox(ebodys[0], bbox);
    CHECK_STATUS(EG_getBoundingBox);

    if        (fabs(bbox[5]-bbox[2]) < EPS06) {
        iplane = 3;
        zplane = (bbox[5] + bbox[2]) / 2;
    } else if (fabs(bbox[4]-bbox[1]) < EPS06) {
        iplane = 2;
        yplane = (bbox[4] + bbox[1]) / 2;
    } else if (fabs(bbox[3]-bbox[0]) < EPS06) {
        iplane = 1;
        xplane = (bbox[3] + bbox[0]) / 2;
    } else {
        snprintf(message, 1024, "input Body must be aligned with a coordinate plane: dx=%f dy=%f dz=%f\n",
               bbox[3]-bbox[0], bbox[4]-bbox[1], bbox[5]-bbox[2]);
        status = EGADS_RANGERR;
        goto cleanup;
    }

    status = EG_getContext(emodel, &context);
    CHECK_STATUS(EG_getContext);

    /* cache copy of arguments for future use */
    status = cacheUdp(NULL);
    CHECK_STATUS(cacheUdp);

#ifdef DEBUG
    printf("dist(%d) = %f\n", numUdp, DIST(numUdp));
#endif

    /* get the Edges associated with the input Body */
    SPLINT_CHECK_FOR_NULL(eloops);

    status = EG_getTopology(eloops[0], &eref, &oclass, &mtype,
                            data, &nedge, &eedges, &senses);
    CHECK_STATUS(EG_getTopology);

#ifdef DEBUG
    printf("Body has %d Edges\n", nedge);
#endif

    assert(nedge > 0);
    SPLINT_CHECK_FOR_NULL(eedges);

    /* allocate space for new Curves, Nodes, and Edges */
    MALLOC(ibeg,      int,     nedge  );
    MALLOC(iend,      int,     nedge  );
    MALLOC(tbeg,      double,  nedge  );
    MALLOC(tend,      double,  nedge  );
    MALLOC(active,    int,     nedge  );
    MALLOC(xyz,       double*, nedge  );
    MALLOC(newCurves, ego,     nedge  );
    MALLOC(newNodes,  ego,     nedge+1);
    MALLOC(newEdges,  ego,     nedge  );
    MALLOC(newSenses, int,     nedge  );

    for (iedge = 0; iedge < nedge; iedge++) {
        active[iedge] = 1;
        xyz[   iedge] = NULL;
    }

#ifdef GRAFIC
    MALLOC(ilin,  int,   4*nedge         );
    MALLOC(isym,  int,   4*nedge         );
    MALLOC(nper,  int,   4*nedge         );
    MALLOC(xplot, float, 4*nedge*npnt_max);
    MALLOC(yplot, float, 4*nedge*npnt_max);

    nplot = 0;
    nline = 0;
#endif

    /* make offset points for each of the Edges */
    for (iedge = 0; iedge < nedge; iedge++) {

        /* determine the number of offset points */
        status = EG_getRange(eedges[iedge], trange, &periodic);
        CHECK_STATUS(EG_getRange);

        status = EG_arcLength(eedges[iedge], trange[0], trange[1], &alen);
        CHECK_STATUS(EG_arclength);

        ibeg[iedge] = 0;
        iend[iedge] = 10 * alen / fabs(DIST(numUdp));
        if (iend[iedge] < npnt_min) iend[iedge] = npnt_min;
        if (iend[iedge] > npnt_max) iend[iedge] = npnt_max;

#ifdef DEBUG
        printf("ibeg[%d]=%d,  iend[%d]=%d\n", iedge, ibeg[iedge], iedge, iend[iedge]);
#endif

        /* generate the points */
        MALLOC(xyz[iedge], double, 3*iend[iedge]+3);

        for (ipnt = ibeg[iedge]; ipnt <= iend[iedge]; ipnt++) {
            if (senses[iedge] == SFORWARD) {
                tt = trange[0] + (double)(ipnt-ibeg[iedge])/(double)(iend[iedge]-ibeg[iedge]) * (trange[1]-trange[0]);
            } else {
                tt = trange[1] + (double)(ipnt-ibeg[iedge])/(double)(iend[iedge]-ibeg[iedge]) * (trange[0]-trange[1]);
            }

            status = EG_evaluate(eedges[iedge], &tt, data);
            CHECK_STATUS(EG_evaluate);

#ifdef GRAFIC
            /* original curve */
            if (iplane == 3) {
                xplot[nplot] = data[0];
                yplot[nplot] = data[1];
                nplot++;
            } else if (iplane == 2) {
                xplot[nplot] = data[2];
                yplot[nplot] = data[0];
                nplot++;
            } else {
                xplot[nplot] = data[1];
                yplot[nplot] = data[2];
                nplot++;
            }
#endif

            /* apply the offset */
            if (senses[iedge] == SFORWARD) {
                fact = +DIST(numUdp) / sqrt(data[3]*data[3] + data[4]*data[4] + data[5]*data[5]);
            } else {
                fact = -DIST(numUdp) / sqrt(data[3]*data[3] + data[4]*data[4] + data[5]*data[5]);
            }

            if (iplane == 3) {
                xyz[iedge][3*ipnt  ] = data[0] + fact * data[4];
                xyz[iedge][3*ipnt+1] = data[1] - fact * data[3];
                xyz[iedge][3*ipnt+2] = zplane;
            } else if (iplane == 2) {
                xyz[iedge][3*ipnt  ] = data[0] - fact * data[5];
                xyz[iedge][3*ipnt+1] = yplane;
                xyz[iedge][3*ipnt+2] = data[2] + fact * data[3];
            } else {
                xyz[iedge][3*ipnt  ] = xplane;
                xyz[iedge][3*ipnt+1] = data[1] + fact * data[5];
                xyz[iedge][3*ipnt+2] = data[2] - fact * data[4];
            }
        }

#ifdef GRAFIC
        ilin[nline] = +GR_SOLID;
        isym[nline] = -GR_X;
        nper[nline] = iend[iedge] - ibeg[iedge] + 1;
        nline++;
#endif
    }

#ifdef GRAFIC
    for (iedge = 0; iedge < nedge; iedge++) {
        if (active[iedge] == 0) continue;

        /* offset curve */
        for (ipnt = ibeg[iedge]; ipnt <= iend[iedge]; ipnt++) {
            if (iplane == 3) {
                xplot[nplot] = xyz[iedge][3*ipnt  ];
                yplot[nplot] = xyz[iedge][3*ipnt+1];
                nplot++;
            } else if (iplane == 2) {
                xplot[nplot] = xyz[iedge][3*ipnt+2];
                yplot[nplot] = xyz[iedge][3*ipnt  ];
                nplot++;
            } else {
                xplot[nplot] = xyz[iedge][3*ipnt+1];
                yplot[nplot] = xyz[iedge][3*ipnt+2];
                nplot++;
            }
        }

        ilin[nline] = +GR_DOTTED;
        isym[nline] = +GR_CIRCLE;
        nper[nline] = iend[iedge] - ibeg[iedge] + 1;
        nline++;
    }
#endif

#ifdef DEBUG
    printf("before end adjustment\n");
    for (iedge = 0; iedge < nedge; iedge++) {
        printf("iedge=%d\n", iedge);
        for (ipnt = ibeg[iedge]; ipnt <= iend[iedge]; ipnt++) {
            printf("%3d %12.6f %12.6f %12.6f\n", ipnt, xyz[iedge][3*ipnt], xyz[iedge][3*ipnt+1], xyz[iedge][3*ipnt+2]);
        }
    }
#endif
#ifdef GRAFIC
    if (iplane == 3) {
        strcpy(pltitl, "~x~y~Original curves and offsets");
    } else if (iplane == 2) {
        strcpy(pltitl, "~z~x~Original curves and offsets");
    } else {
        strcpy(pltitl, "~y~z~Original curves and offsets");
    }

    grinit_(&io_kbd, &io_scr, "udfOffset", strlen("udfOffset"));
    grline_(ilin, isym, &nline, pltitl, &indgr, xplot, yplot, nper, strlen(pltitl));
#endif

    /* intersect each offset curve with its neighbors in order to
       find tbeg and tend of the intersections */
    status = EG_getInfo(eloops[0], &oclass, &mtype, &topRef, &eprev, &enext);
    CHECK_STATUS(EG_getInfo);

    medge = nedge;
    nchange = nedge - 1;
    while (nchange > 0) {
        nchange = 0;

        /* use whole range of each active Edge */
        for (iedge = 0; iedge < nedge; iedge++) {
            if (active[iedge] == 1) {
                tbeg[iedge] = 0;
                tend[iedge] = iend[iedge];
            }
        }

        /* find intersection for each pair of adjacent (active) Edges */
        for (iedge = 0; iedge < nedge; iedge++) {
            if (mtype == OPEN && iedge == medge-1) break;

            if (active[iedge] == 0) continue;     // ignore iedge if already inactive

            for (jedge = iedge+1; jedge != iedge; jedge++) {
                if (jedge >= nedge) jedge -= nedge;

                if (active[jedge] == 0) continue; // ignore jedge if already inactive

                status = intersectLines(iend[iedge]+1, xyz[iedge], iend[jedge]+1, xyz[jedge], iplane, &(tend[iedge]), &(tbeg[jedge]));
                if (status == OCSM_UDP_ERROR1) {
                    snprintf(message, 1024, "Edges %d and %d are parallel but not colinear\n", iedge+1, jedge+1);
                    goto cleanup;
                }
                CHECK_STATUS(intersectLines);

                break;
            }
        }

        /* inactivate any Edge in which t is not increasing */
        for (iedge = 0; iedge < nedge; iedge++) {
            if (active[iedge] == 1 && tend[iedge] <= tbeg[iedge]) {
                active[iedge] = 0;
                if (iedge == medge-1) medge--;
                nchange++;
            }
#ifdef DEBUG
            printf("    tbeg[%2d]=%12.4e, tend[%2d]=%12.4e,  active=%d\n", iedge, tbeg[iedge], iedge, tend[iedge], active[iedge]);
#endif
        }
    }

    /* reset ibeg and iend for each active Edge and adjust the first and last points
       so that the ends line up */
    for (iedge = 0; iedge < nedge; iedge++) {
        if (active[iedge] == 0) continue;

        if (nedge == 1) break;

        ibeg[iedge] = MAX(0, (int)tbeg[iedge]);
        frac        = tbeg[iedge] - ibeg[iedge];

        xyz[iedge][3*ibeg[iedge]  ] = (1-frac) * xyz[iedge][3*ibeg[iedge]  ] + frac * xyz[iedge][3*ibeg[iedge]+3];
        xyz[iedge][3*ibeg[iedge]+1] = (1-frac) * xyz[iedge][3*ibeg[iedge]+1] + frac * xyz[iedge][3*ibeg[iedge]+4];
        xyz[iedge][3*ibeg[iedge]+2] = (1-frac) * xyz[iedge][3*ibeg[iedge]+2] + frac * xyz[iedge][3*ibeg[iedge]+5];

        if (fabs(xyz[iedge][3*ibeg[iedge]  ]-xyz[iedge][3*ibeg[iedge]+3]) < EPS06 &&
            fabs(xyz[iedge][3*ibeg[iedge]+1]-xyz[iedge][3*ibeg[iedge]+4]) < EPS06 &&
            fabs(xyz[iedge][3*ibeg[iedge]+2]-xyz[iedge][3*ibeg[iedge]+5]) < EPS06   ) {
            ibeg[iedge]++;
        }

        iend[iedge] = MIN(iend[iedge], 1+(int)(tend[iedge]-EPS12));
        frac        = iend[iedge] - tend[iedge];

        xyz[iedge][3*iend[iedge]  ] = (1-frac) * xyz[iedge][3*iend[iedge]  ] + frac * xyz[iedge][3*iend[iedge]-3];
        xyz[iedge][3*iend[iedge]+1] = (1-frac) * xyz[iedge][3*iend[iedge]+1] + frac * xyz[iedge][3*iend[iedge]-2];
        xyz[iedge][3*iend[iedge]+2] = (1-frac) * xyz[iedge][3*iend[iedge]+2] + frac * xyz[iedge][3*iend[iedge]-1];

        if (fabs(xyz[iedge][3*iend[iedge]  ]-xyz[iedge][3*iend[iedge]-3]) < EPS06 &&
            fabs(xyz[iedge][3*iend[iedge]+1]-xyz[iedge][3*iend[iedge]-2]) < EPS06 &&
            fabs(xyz[iedge][3*iend[iedge]+2]-xyz[iedge][3*iend[iedge]-1]) < EPS06   ) {
            iend[iedge]--;
        }
    }

#ifdef DEBUG
    printf("after end adjustment\n");
    for (iedge = 0; iedge < nedge; iedge++) {
        printf("iedge=%d\n", iedge);
        for (ipnt = ibeg[iedge]; ipnt <= iend[iedge]; ipnt++) {
            printf("%3d %12.6f %12.6f %12.6f\n", ipnt, xyz[iedge][3*ipnt], xyz[iedge][3*ipnt+1], xyz[iedge][3*ipnt+2]);
        }
    }
#endif
    /* make the needed Nodes */
    isave = 0;
    jedge = 0;
    for (iedge = 0; iedge < nedge; iedge++) {
        if (active[iedge] == 0) continue;

#ifdef DEBUG
        printf("Node[%d] at (%10.5f %10.5f %10.5f\n", jedge, xyz[iedge][3*ibeg[iedge]], xyz[iedge][3*ibeg[iedge]+1], xyz[iedge][3*ibeg[iedge]+2]);
#endif

        status = EG_makeTopology(context, NULL, NODE, 0,
                                 &(xyz[iedge][3*ibeg[iedge]]), 0, NULL, NULL, &(newNodes[jedge]));
        CHECK_STATUS(EG_makeTopology);

        isave = iedge;

        jedge++;
    }

    if (mtype == CLOSED) {
        newNodes[jedge] = newNodes[0];
    } else {
#ifdef DEBUG
        printf("Node[%d] at (%10.5f %10.5f %10.5f\n", jedge, xyz[isave][3*ibeg[isave]], xyz[isave][3*ibeg[isave]+1], xyz[isave][3*ibeg[isave]+2]);
#endif

        status = EG_makeTopology(context, NULL, NODE, 0,
                                 &(xyz[isave][3*iend[isave]]), 0, NULL, NULL, &(newNodes[jedge]));
        CHECK_STATUS(EG_makeTopology);
    }

    /* make the Curves and Edges */
    jedge = 0;
    for (iedge = 0; iedge < nedge; iedge++) {
        if (active[iedge] == 0) continue;

        sizes[0] = iend[iedge] - ibeg[iedge] + 1;
        sizes[1] = 0;
        status = EG_approximate(context, 0, EPS06, sizes, &(xyz[iedge][3*ibeg[iedge]]), &newCurves[jedge]);
        CHECK_STATUS(EG_approximate);

        status = EG_getRange(newCurves[jedge], trange, &periodic);
        CHECK_STATUS(EG_getRange);

        newSenses[jedge] = SFORWARD;
        status = EG_makeTopology(context, newCurves[jedge], EDGE, TWONODE,
                                 trange, 2, &(newNodes[jedge]), &(newSenses[jedge]), &newEdges[jedge]);
        CHECK_STATUS(EG_makeTopology);

        jedge++;
    }

    /* make a Loop from the Edges */
    status = EG_makeTopology(context, NULL, LOOP, mtype,
                             NULL, jedge, newEdges, newSenses, &newLoop);
    CHECK_STATUS(EG_makeToplogy);

    /* get the mtype of the input (original) Body */
    status = EG_getInfo(ebodys[0], &oclass, &mtype, &topRef, &eprev, &enext);
    CHECK_STATUS(EG_getInfo);

    /* if input was a WireBody, we can make the WireBody directly from the Loop */
    if (mtype == WIREBODY) {
        status = EG_makeTopology(context, NULL, BODY, WIREBODY,
                                 NULL, 1, &newLoop, NULL, ebody);
        CHECK_STATUS(EG_makeTopology);

    /* otherwise, make the Face, Sheet, Shell, and SheetBody */
    } else {
        status = EG_makeFace(newLoop, newSenses[0], NULL, &newFace);
        CHECK_STATUS(EG_makeFace);

        status = EG_makeTopology(context, NULL, SHELL, OPEN,
                                 NULL, 1, &newFace, NULL, &newShell);
        CHECK_STATUS(EG_makeTopology);

        status = EG_makeTopology(context, NULL, BODY, SHEETBODY,
                                 NULL, 1, &newShell, NULL, ebody);
        CHECK_STATUS(EG_makeTopology);

        SPLINT_CHECK_FOR_NULL(*ebody);

        status = EG_attributeAdd(*ebody, "__markFaces__", ATTRSTRING, 1, NULL, NULL, "true");
        CHECK_STATUS(EG_attributeAdd);
    }

    /* set the output value */

    /* return the new Body */
    udps[numUdp].ebody = *ebody;

cleanup:
    if (eloops != NULL) EG_free(eloops);

    if (xyz != NULL) {
        for (iedge = 0; iedge < nedge; iedge++) {
            FREE(xyz[iedge]);
        }
    }

    FREE(ibeg     );
    FREE(iend     );
    FREE(tbeg     );
    FREE(tend     );
    FREE(active   );
    FREE(xyz      );
    FREE(newCurves);
    FREE(newNodes );
    FREE(newEdges );
    FREE(newSenses);

#ifdef GRAFIC
    FREE(ilin );
    FREE(isym );
    FREE(nper );
    FREE(xplot);
    FREE(yplot);
#endif

    if (strlen(message) > 0) {
        *string = message;
    } else if (status != EGADS_SUCCESS) {
        FREE(message);
        *string = udpErrorStr(status);
    } else {
        FREE(message);
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
 *   interesctLines - find intersection of two discrete lines           *
 *                                                                      *
 ************************************************************************
 */

static int
intersectLines(int    npnt1,            /* (in)  number of points in line 1 */
               double xyz1[],           /* (in)  xyz array       for line 1 */
               int    npnt2,            /* (in)  number of points in line 2 */
               double xyz2[],           /* (in)  xyz array       for line 2 */
               int    iplane,           /* (in)  1=yz, 2=zx, 3=xy */
               double *t1,              /* (out) parametric coordinate for line 1 */
               double *t2)              /* (out) parametric coordinate for line 2 */
{
    int status = EGADS_SUCCESS;         /* (out) return status */

    int    ipnt1, ipnt2, redo;
    double xA, yA, xB, yB, xC, yC, xD, yD, det, s1, s2;

    ROUTINE(intersect);

    /* --------------------------------------------------------------- */

    /* default returns */
    *t1 = -HUGEQ;
    *t2 = +HUGEQ;

    ipnt1 = npnt1 - 2;                  // start at end of line 1
    ipnt2 = 0;                          // start at beg of line 2

    redo = 1;
    while (redo != 0) {
        redo = 0;

        /* get coordinates */
        if (iplane == 1) {
            xA = xyz1[3*ipnt1+1];
            yA = xyz1[3*ipnt1+2];
            xB = xyz1[3*ipnt1+4];
            yB = xyz1[3*ipnt1+5];

            xC = xyz2[3*ipnt2+1];
            yC = xyz2[3*ipnt2+2];
            xD = xyz2[3*ipnt2+4];
            yD = xyz2[3*ipnt2+5];
        } else if (iplane == 2) {
            xA = xyz1[3*ipnt1+2];
            yA = xyz1[3*ipnt1  ];
            xB = xyz1[3*ipnt1+5];
            yB = xyz1[3*ipnt1+3];

            xC = xyz2[3*ipnt2+2];
            yC = xyz2[3*ipnt2  ];
            xD = xyz2[3*ipnt2+5];
            yD = xyz2[3*ipnt2+3];
        } else {
            xA = xyz1[3*ipnt1  ];
            yA = xyz1[3*ipnt1+1];
            xB = xyz1[3*ipnt1+3];
            yB = xyz1[3*ipnt1+4];

            xC = xyz2[3*ipnt2  ];
            yC = xyz2[3*ipnt2+1];
            xD = xyz2[3*ipnt2+3];
            yD = xyz2[3*ipnt2+4];
        }

        /* find the intersection */
        det = (xB-xA) * (yC-yD) - (xC-xD) * (yB-yA);
        if (fabs(det) < EPS30) {
            if (fabs(xB-xC) < EPS06 && fabs(yB-yC) < EPS06) {
                *t1 = npnt1 - 1;
                *t2 = 0;
            } else {
                status = OCSM_UDP_ERROR1;
            }

            goto cleanup;
        } else {
            s1 = ((xC-xA) * (yC-yD) - (xC-xD) * (yC-yA)) / det;
            s2 = ((xB-xA) * (yC-yA) - (xC-xA) * (yB-yA)) / det;
        }

        /* if not in this segment in line 1, decrease ipnt1 if possible and redo */
        if (s1 < 0) {
            if (ipnt1 > 1) {
                ipnt1--;
                redo ++;
            } else {
                *t2 = ipnt2 + s2;
                goto cleanup;
            }
        }

        /* if not in this segment in line 2, increase ipnt2 if possible and redo */
        if (s2 > 1) {
            if (ipnt2 < npnt2-2) {
                ipnt2++;
                redo ++;
            } else {
                *t1 = ipnt1 + s1;
                goto cleanup;
            }
        }
    }

    *t1 = ipnt1 + s1;
    *t2 = ipnt2 + s2;

cleanup:
    return status;
}
