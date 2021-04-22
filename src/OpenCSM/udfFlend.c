/*
 ************************************************************************
 *                                                                      *
 * udfFlend -- create a flended Body                                    *
 *                                                                      *
 *            Written by John Dannenhoffer@ Syracuse University         *
 *                                                                      *
 ************************************************************************
 */

/*
 * Copyright (C) 2013/2020  John F. Dannenhoffer, III (Syracuse University)
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

#define NUMUDPINPUTBODYS -2
#define NUMUDPARGS        4

/* set up the necessary structures (uses NUMUDPARGS) */
#include "udpUtilities.h"

/* shorthands for accessing argument values and velocities */
#define FRACA( IUDP)     ((double *) (udps[IUDP].arg[0].val))[0]
#define FRACB( IUDP)     ((double *) (udps[IUDP].arg[1].val))[0]
#define TOLER( IUDP)     ((double *) (udps[IUDP].arg[2].val))[0]
#define PLOT(  IUDP)     ((int    *) (udps[IUDP].arg[3].val))[0]

static char  *argNames[NUMUDPARGS] = {"fraca",  "fracb",  "toler",  "plot",  };
static int    argTypes[NUMUDPARGS] = {ATTRREAL, ATTRREAL, ATTRREAL, ATTRINT, };
static int    argIdefs[NUMUDPARGS] = {0,        0,        0,        0,       };
static double argDdefs[NUMUDPARGS] = {0.20,     0.20,     1.0e-6,   0.,      };

/* get utility routines: udpErrorStr, udpInitialize, udpReset, udpSet,
                         udpGet, udpVel, udpClean, udpMesh */
#include "udpUtilities.c"

#include "OpenCSM.h"

static int reorderLoop(int nloop, ego eloops[]);
static int flipLoop(ego *eloop);

extern int EG_spline2dAppx(      ego    context,
                                 int    endc,
                /*@null@*/ const double *uknot,
                /*@null@*/ const double *vknot,
                /*@null@*/ const int    *vdata,
                /*@null@*/ const double *wesT,    // tangency at imin
                /*@null@*/ const double *easT,    // tangency at imax
                /*@null@*/ const double *south,   // data for nose treatment
                /*@null@*/       double *snor,    // tangency at jmin
                /*@null@*/ const double *north,   // data for tail treatment
                /*@null@*/       double *nnor,    // tangency at jmax
                                 int    imax,
                                 int    jmax,
                           const double *xyz,
                                 double tol,
                                 ego    *esurf);

#define   HUGEQ   1.0e+20


/*
 ************************************************************************
 *                                                                      *
 *   udpExecute - execute the primitive                                 *
 *                                                                      *
 ************************************************************************
 */

int
udpExecute(ego  emodel,                 /* (in)  input model */
           ego  *ebody,                 /* (out) Body pointer */
           int  *nMesh,                 /* (out) number of associated meshes */
           char *string[])              /* (out) error message */
{
    int     status = EGADS_SUCCESS;     /* (in)  return status */

    ego     context, *ebodys;

    int     nloop = 4;        // number of Loops
    int     npnt  = 33;       // number of points on each Edge

    int      nedgeA, iedgeA, nfaceA, ifaceA, nlistA, *sensesA;
    int      nedgeB, iedgeB, nfaceB, ifaceB, nlistB, *sensesB;
    int      iloop, ipnt, nfacelist, brchattr[2], bodyattr[2];
    int      oclass, mtype, nchild, *senses, ntemp, atype, alen, count, itemp, iedge, jedge, periodic;
    int      nnode, nedge2, iedge2;
    CINT     *tempIlist;
    double   data[18], value, dot, trangeA[2], trangeB[2], fraci, fracj, tt;
    double   uvA[2], uvB[2], tangA[18], tangB[18], vold[3], vnew[3], *point=NULL;
    double   stepT, stepU, stepV, dxyzbeg[3], dxyzend[3];
    double   *spln=NULL, *west=NULL, *east=NULL, toler=1.0e-8;
    CDOUBLE  *tempRlist;
    char     temp[1];
    CCHAR    *tempClist;
    ego      *eedgesA, *efacesA=NULL, *elistA=NULL;
    ego      *eedgesB, *efacesB=NULL, *elistB=NULL;
    ego      eref, *etemps, *eloops=NULL, *echilds;
    ego      *eedgelist=NULL, *efacelist=NULL;
    ego      esurf, enode, eedge, *enodes, *eedges2;
    void     *modl;

#ifdef DEBUG
    printf("udpExecute(emodel=%llx)\n", (long long)emodel);
    printf("fraca(0) = %f\n", FRACA(0));
    printf("fracb(0) = %f\n", FRACB(0));
    printf("toler(0) = %f\n", TOLER(0));
    printf("plot( 0) = %d\n", PLOT( 0));
#endif

    /* default return values */
    *ebody  = NULL;
    *nMesh  = 0;
    *string = NULL;

    /* check/process arguments */
    if (udps[0].arg[0].size > 1) {
        printf(" udpExecute: fraca should be a scalar\n");
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (FRACA(0) <= 0) {
        printf(" udpExecute: fraca = %f <= 0\n", FRACA(0));
        status  =  EGADS_RANGERR;
        goto cleanup;

    } else if (udps[0].arg[1].size > 1) {
        printf(" udpExecute: fracb should be a scalar\n");
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (FRACB(0) <= 0) {
        printf(" udpExecute: fracb = %f <= 0\n", FRACB(0));
        status  =  EGADS_RANGERR;
        goto cleanup;

    } else if (udps[0].arg[2].size > 1) {
        printf(" udpExecute: toler should be a scalar\n");
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (TOLER(0) < 0) {
        printf(" udpExecute: toler = %f <= 0\n", TOLER(0));
        status  =  EGADS_RANGERR;
        goto cleanup;

    } else if (udps[0].arg[3].size > 1) {
        printf(" udpExecute: plot should be a scalar\n");
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (nloop != 4) {
        printf(" udpExecute: nloop = %d != 4\n", nloop);
        status  =  EGADS_RANGERR;
        goto cleanup;
    }

    /* check that Model was input that contains two Bodys */
    status = EG_getTopology(emodel, &eref, &oclass, &mtype,
                            data, &nchild, &ebodys, &senses);
    if (status != EGADS_SUCCESS) goto cleanup;

    printf("emodel contains %d Bodys\n", nchild);

    if (oclass != MODEL) {
        printf(" udpExecute: expecting a Model\n");
        status = EGADS_NOTMODEL;
        goto cleanup;
    } else if (nchild != 1 && nchild != 2) {
        printf(" udpExecute: expecting Model to contain one or two Bodys (not %d)\n", nchild);
        status = EGADS_NOTBODY;
        goto cleanup;
    }

    if (nchild == 1) {
        printf("FLEND only works with 2 Bodys at this time\n");
        status = -999;
        goto cleanup;
    }

#ifdef DEBUG
    printf("emodel\n");
    ocsmPrintEgo(emodel);
#endif

    /* cache copy of arguments for future use */
    status = cacheUdp();
    if (status < 0) {
        printf(" udpExecute: problem caching arguments\n");
        goto cleanup;
    }

#ifdef DEBUG
    printf("fraca(%d) = %f\n", numUdp, FRACA(numUdp));
    printf("fracb(%d) = %f\n", numUdp, FRACB(numUdp));
    printf("toler(%d) = %f\n", numUdp, TOLER(numUdp));
    printf("plot( %d) = %d\n", numUdp, PLOT( numUdp));
#endif

    status = EG_getContext(emodel, &context);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* return the modfied Model that contains the two input Bodys */
    *ebody = emodel;

#ifdef DEBUG
    printf("*ebody\n");
    (void) ocsmPrintEgo(*ebody);
#endif

    /* get pointer to model */
    status = EG_getUserPointer(context, (void**)(&(modl)));
    if (status != EGADS_SUCCESS) {
        printf(" udpExecute: bad return from getUserPointer\n");
        goto cleanup;
    }

    /* make a list of the exposed Edges in BodyA (which are adjacent to one Face
       with the _flend=remove attribute) */
    status = EG_getBodyTopos(ebodys[0], NULL, EDGE, &nedgeA, &eedgesA);
    if (status != EGADS_SUCCESS) goto cleanup;

    nlistA = 0;
    elistA = (ego *) EG_alloc(nedgeA*sizeof(ego));
    if (elistA == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    for (iedgeA = 0; iedgeA < nedgeA; iedgeA++) {
        status = EG_getBodyTopos(ebodys[0], eedgesA[iedgeA], FACE, &ntemp, &etemps);
        if (status != EGADS_SUCCESS) goto cleanup;

        count = 0;
        for (itemp = 0; itemp < ntemp; itemp++) {
            status = EG_attributeRet(etemps[itemp], "_flend", &atype, &alen,
                                     &tempIlist, &tempRlist, &tempClist);
            if (status == EGADS_SUCCESS) {
                if (atype == ATTRSTRING && strcmp(tempClist, "remove") == 0) {
                    count++;
                }
            }
        }

        if (count == 1) {
            elistA[nlistA++] = eedgesA[iedgeA];
        }

        EG_free(etemps);
    }

    EG_free(eedgesA);

    /* make a list of the exposed Edges in BodyB (which are adjacent to one Face
       with the _flend=remove attribute) */
    status = EG_getBodyTopos(ebodys[1], NULL, EDGE, &nedgeB, &eedgesB);
    if (status != EGADS_SUCCESS) goto cleanup;

    nlistB = 0;
    elistB = (ego *) EG_alloc(nedgeB*sizeof(ego));
    if (elistB == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    for (iedgeB = 0; iedgeB < nedgeB; iedgeB++) {
        status = EG_getBodyTopos(ebodys[1], eedgesB[iedgeB], FACE, &ntemp, &etemps);
        if (status != EGADS_SUCCESS) goto cleanup;

        count = 0;
        for (itemp = 0; itemp < ntemp; itemp++) {
            status = EG_attributeRet(etemps[itemp], "_flend", &atype, &alen,
                                     &tempIlist, &tempRlist, &tempClist);
            if (status == EGADS_SUCCESS) {
                if (atype == ATTRSTRING && strcmp(tempClist, "remove") == 0) {
                    count++;
                }
            }
        }

        if (count == 1) {
            elistB[nlistB++] = eedgesB[iedgeB];
        }

        EG_free(etemps);
    }

    EG_free(eedgesB);

    /* make sure both Bodys have the same number of exposed Edges */
    if (nlistA == 0 || nlistA != nlistB) {
        printf(" udpExecute: nlistA=%d and nlistB=%d\n", nlistA, nlistB);
        printf("ebodys[0]\n");
        ocsmPrintEgo(ebodys[0]);
        printf("ebodys[1]\n");
        ocsmPrintEgo(ebodys[1]);
        status = -281;
        goto cleanup;
    }

    eloops = (ego *) EG_alloc(nloop*sizeof(ego));
    if (eloops == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    /* make a Loop from the exposed Edges in BodyA */
    status = EG_makeLoop(nlistA, elistA, NULL, 0, &eloops[0]);
    if (status < EGADS_SUCCESS) {
        goto cleanup;
    } else if (status > 0) {
        printf(" udpExecute: could not make one Loop from exposed Edges in BodyA -> status=%d\n", status);
        status = -282;
        goto cleanup;
    }

    /* make a loop from the exposed Edges in BodyB */
    status = EG_makeLoop(nlistB, elistB, NULL, 0, &eloops[nloop-1]);
    if (status < EGADS_SUCCESS) {
        goto cleanup;
    } else if (status > 0) {
        printf(" udpExecute: could not make one Loop from exposed Edges in BodyB -> status=%d\n", status);
        status = -283;
        goto cleanup;
    }

    /* reorient eloops[nloop-1] to minimize twist from eloops[0] */
    status = reorderLoop(nloop, eloops);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get Edges for bounding loops */
    status = EG_getTopology(eloops[0], &eref, &oclass, &mtype, data, &nedgeA, &eedgesA, &sensesA);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_getTopology(eloops[nloop-1], &eref, &oclass, &mtype, data, &nedgeB, &eedgesB, &sensesB);
    if (status != EGADS_SUCCESS) goto cleanup;

    efacesA = (ego *) EG_alloc(nedgeA*sizeof(ego));
    efacesB = (ego *) EG_alloc(nedgeB*sizeof(ego));
    if (efacesA== NULL || efacesB == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    /* get the Faces in BodyA adjacent to the Edges in eloops[0] */
    for (iedge = 0; iedge < nedgeA; iedge++) {
        efacesA[iedge] = NULL;

        status = EG_getBodyTopos(ebodys[0], eedgesA[iedge], FACE, &ntemp, &etemps);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (itemp = 0; itemp < ntemp; itemp++) {
            status = EG_attributeRet(etemps[itemp], "_flend", &atype, &alen,
                                     &tempIlist, &tempRlist, &tempClist);
            if (status != EGADS_SUCCESS) {
                efacesA[iedge] = etemps[itemp];
                break;
            }
        }

        EG_free(etemps);

        if (efacesA[iedge] == NULL) {
            status = -284;
            goto cleanup;
        }
    }

    /* get the Faces in BodyB adjacent to the Edges in eloops[nloop-1] */
    for (iedge = 0; iedge < nedgeB; iedge++) {
        efacesB[iedge] = NULL;

        status = EG_getBodyTopos(ebodys[1], eedgesB[iedge], FACE, &ntemp, &etemps);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (itemp = 0; itemp < ntemp; itemp++) {
            status = EG_attributeRet(etemps[itemp], "_flend", &atype, &alen,
                                     &tempIlist, &tempRlist, &tempClist);
            if (status != EGADS_SUCCESS) {
                efacesB[iedge] = etemps[itemp];
                break;
            }
        }

        EG_free(etemps);

        if (efacesB[iedge] == NULL) {
            status = -285;
            goto cleanup;
        }
    }

    /* get an array to hold the points that will be used to make the Curves */
    point = (double *) EG_alloc(3*npnt*nloop*nedgeA*sizeof(double));
    if (point == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

#define XPNT(IEDGE,ILOOP,IPNT) point[3*(IPNT+npnt*(ILOOP+nloop*IEDGE))  ]
#define YPNT(IEDGE,ILOOP,IPNT) point[3*(IPNT+npnt*(ILOOP+nloop*IEDGE))+1]
#define ZPNT(IEDGE,ILOOP,IPNT) point[3*(IPNT+npnt*(ILOOP+nloop*IEDGE))+2]

    /* get the (initial) Points in the Loops */
    for (iedge = 0; iedge < nedgeA; iedge++) {
        status = EG_getRange(eedgesA[iedge], trangeA, &periodic);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_getRange(eedgesB[iedge], trangeB, &periodic);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (ipnt = 0; ipnt < npnt; ipnt++) {
            fraci = (double)(ipnt) / (double)(npnt-1);

            /* iloop = 0 */
            if (sensesA[iedge] == SFORWARD) {
                tt = (1-fraci) * trangeA[0] + fraci * trangeA[1];
            } else {
                tt = (1-fraci) * trangeA[1] + fraci * trangeA[0];
            }
            status = EG_evaluate(eedgesA[iedge], &tt, data);
            if (status != EGADS_SUCCESS) goto cleanup;

            XPNT(iedge,0,ipnt) = data[0];
            YPNT(iedge,0,ipnt) = data[1];
            ZPNT(iedge,0,ipnt) = data[2];

            /* iloop = nloop-1 */
            if (sensesB[iedge] == SFORWARD) {
                tt = (1-fraci) * trangeB[0] + fraci * trangeB[1];
            } else {
                tt = (1-fraci) * trangeB[1] + fraci * trangeB[0];
            }
            status = EG_evaluate(eedgesB[iedge], &tt, data);
            if (status != EGADS_SUCCESS) goto cleanup;

            XPNT(iedge,nloop-1,ipnt) = data[0];
            YPNT(iedge,nloop-1,ipnt) = data[1];
            ZPNT(iedge,nloop-1,ipnt) = data[2];

            /* interior Loops (1 -> nloop-2) */
            for (iloop = 1; iloop < nloop-1; iloop++) {
                if (iloop == 1) {
                    fracj =     FRACA(0);
                } else if (iloop == nloop-2) {
                    fracj = 1 - FRACB(0);
                } else {
                    fracj = (double)(iloop) / (double)(nloop-1);
                }

                XPNT(iedge,iloop,ipnt) = (1-fracj) * XPNT(iedge,0,ipnt) + fracj * XPNT(iedge,nloop-1,ipnt);
                YPNT(iedge,iloop,ipnt) = (1-fracj) * YPNT(iedge,0,ipnt) + fracj * YPNT(iedge,nloop-1,ipnt);
                ZPNT(iedge,iloop,ipnt) = (1-fracj) * ZPNT(iedge,0,ipnt) + fracj * ZPNT(iedge,nloop-1,ipnt);
            }
        }
    }

    /* update the Points that are associated with the Nodes */
    for (iedge = 0; iedge < nedgeA; iedge++) {
        jedge = (iedge + nedgeA - 1) % nedgeA;

        /* BodyA */
        status = EG_getRange(eedgesA[iedge], trangeA, &periodic);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (sensesA[iedge] == SFORWARD) {
            tt = trangeA[0];
        } else {
            tt = trangeA[1];
        }

        status = EG_getEdgeUV(efacesA[iedge], eedgesA[iedge], 0, tt, uvA);
        if (status != EGADS_SUCCESS) goto cleanup;

        /* BodyB */
        status = EG_getRange(eedgesB[iedge], trangeB, &periodic);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (sensesB[iedge] == SFORWARD) {
            tt = trangeB[0];
        } else {
            tt = trangeB[1];
        }

        status = EG_getEdgeUV(efacesB[iedge], eedgesB[iedge], 0, tt, uvB);
        if (status != EGADS_SUCCESS) goto cleanup;

        /* process loop adjacent to BodyA */
        iloop = 1;
        ipnt  = 0;

        /* if the two bounding Faces for iloop=0 are the same,
           simply project the point to the shared efaceA */
        if (efacesA[iedge] == efacesA[jedge]) {
            status = EG_evaluate(efacesA[iedge], uvA, tangA);
            if (status != EGADS_SUCCESS) goto cleanup;

            /* only use component of vector that is along the Face */
            vold[0] = XPNT(iedge,iloop,ipnt) - XPNT(iedge,0,ipnt);
            vold[1] = YPNT(iedge,iloop,ipnt) - YPNT(iedge,0,ipnt);
            vold[2] = ZPNT(iedge,iloop,ipnt) - ZPNT(iedge,0,ipnt);

            stepU = (vold[ 0] * tangA[3] + vold[ 1] * tangA[4] + vold[ 2] * tangA[5])
                  / (tangA[3] * tangA[3] + tangA[4] * tangA[4] + tangA[5] * tangA[5]);
            stepV = (vold[ 0] * tangA[6] + vold[ 1] * tangA[7] + vold[ 2] * tangA[8])
                  / (tangA[6] * tangA[6] + tangA[7] * tangA[7] + tangA[8] * tangA[8]);

            vnew[0] = stepU * tangA[3] + stepV * tangA[6];
            vnew[1] = stepU * tangA[4] + stepV * tangA[7];
            vnew[2] = stepU * tangA[5] + stepV * tangA[8];

            XPNT(iedge,iloop,ipnt) = XPNT(iedge,0,ipnt) + vnew[0];
            YPNT(iedge,iloop,ipnt) = YPNT(iedge,0,ipnt) + vnew[1];
            ZPNT(iedge,iloop,ipnt) = ZPNT(iedge,0,ipnt) + vnew[2];

        /* otherwise find the Edge that sets direction for C1 continuity */
        } else {
            status = EG_getTopology(eedgesA[iedge], &eref, &oclass, &mtype, data, &nnode, &enodes, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (sensesA[iedge] == SFORWARD) {
                enode = enodes[0];
            } else {
                enode = enodes[1];
            }

            status = EG_getBodyTopos(ebodys[0], enode, EDGE, &nedge2, &eedges2);
            if (status != EGADS_SUCCESS) goto cleanup;

            eedge = NULL;
            for (iedge2 = 0; iedge2 < nedge2; iedge2++) {
                if (eedges2[iedge2] != eedgesA[iedge] &&
                    eedges2[iedge2] != eedgesA[jedge]   ) {
                    eedge = eedges2[iedge2];
                }
            }

            EG_free(eedges2);

            status = EG_getTopology(eedge, &eref, &oclass, &mtype, data, &nnode, &enodes, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;

            if        (enodes[0] == enode) {
                status = EG_evaluate(eedge, &data[0], tangA);
                if (status != EGADS_SUCCESS) goto cleanup;
            } else if (enodes[1] == enode) {
                status = EG_evaluate(eedge, &data[1], tangA);
                if (status != EGADS_SUCCESS) goto cleanup;
            } else {
                printf("did not match Nodes\n");
                exit(0);
            }

            /* only use component of vector that is parallel to Edge */
            vold[0] = XPNT(iedge,iloop,ipnt) - XPNT(iedge,0,ipnt);
            vold[1] = YPNT(iedge,iloop,ipnt) - YPNT(iedge,0,ipnt);
            vold[2] = ZPNT(iedge,iloop,ipnt) - ZPNT(iedge,0,ipnt);

            stepT = (vold[ 0] * tangA[3] + vold[ 1] * tangA[4] + vold[ 2] * tangA[5])
                  / (tangA[3] * tangA[3] + tangA[4] * tangA[4] + tangA[5] * tangA[5]);

            vnew[0] = stepT * tangA[3];
            vnew[1] = stepT * tangA[4];
            vnew[2] = stepT * tangA[5];

            XPNT(iedge,iloop,ipnt) = XPNT(iedge,0,ipnt) + vnew[0];
            YPNT(iedge,iloop,ipnt) = YPNT(iedge,0,ipnt) + vnew[1];
            ZPNT(iedge,iloop,ipnt) = ZPNT(iedge,0,ipnt) + vnew[2];
        }

        /* process loop adjacent to BodyB */
        iloop = nloop - 2;
        ipnt  = 0;

        /* if the two bounding Faces for iloop=nloop-1 are the same,
           simply project the point to the shared efaceB */
        if (efacesB[iedge] == efacesB[jedge]) {
            status = EG_evaluate(efacesB[iedge], uvB, tangB);
            if (status != EGADS_SUCCESS) goto cleanup;

            /* only use component of vector that is along the Face */
            vold[0] = XPNT(iedge,iloop,ipnt) - XPNT(iedge,nloop-1,ipnt);
            vold[1] = YPNT(iedge,iloop,ipnt) - YPNT(iedge,nloop-1,ipnt);
            vold[2] = ZPNT(iedge,iloop,ipnt) - ZPNT(iedge,nloop-1,ipnt);

            stepU = (vold[ 0] * tangB[3] + vold[ 1] * tangB[4] + vold[ 2] * tangB[5])
                  / (tangB[3] * tangB[3] + tangB[4] * tangB[4] + tangB[5] * tangB[5]);
            stepV = (vold[ 0] * tangB[6] + vold[ 1] * tangB[7] + vold[ 2] * tangB[8])
                  / (tangB[6] * tangB[6] + tangB[7] * tangB[7] + tangB[8] * tangB[8]);

            vnew[0] = stepU * tangB[3] + stepV * tangB[6];
            vnew[1] = stepU * tangB[4] + stepV * tangB[7];
            vnew[2] = stepU * tangB[5] + stepV * tangB[8];

            XPNT(iedge,iloop,ipnt) = XPNT(iedge,nloop-1,ipnt) + vnew[0];
            YPNT(iedge,iloop,ipnt) = YPNT(iedge,nloop-1,ipnt) + vnew[1];
            ZPNT(iedge,iloop,ipnt) = ZPNT(iedge,nloop-1,ipnt) + vnew[2];

        /* otherwise find the Edge that sets direction for C1 continuity */
        } else {
            status = EG_getTopology(eedgesB[iedge], &eref, &oclass, &mtype, data, &nnode, &enodes, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (sensesB[iedge] == SFORWARD) {
                enode = enodes[0];
            } else {
                enode = enodes[1];
            }

            status = EG_getBodyTopos(ebodys[1], enode, EDGE, &nedge2, &eedges2);
            if (status != EGADS_SUCCESS) goto cleanup;

            eedge = NULL;
            for (iedge2 = 0; iedge2 < nedge2; iedge2++) {
                if (eedges2[iedge2] != eedgesB[iedge] &&
                    eedges2[iedge2] != eedgesB[jedge]   ) {
                    eedge = eedges2[iedge2];
                }
            }

            EG_free(eedges2);

            status = EG_getTopology(eedge, &eref, &oclass, &mtype, data, &nnode, &enodes, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;

            if        (enodes[0] == enode) {
                status = EG_evaluate(eedge, &data[0], tangB);
                if (status != EGADS_SUCCESS) goto cleanup;
            } else if (enodes[1] == enode) {
                status = EG_evaluate(eedge, &data[1], tangB);
                if (status != EGADS_SUCCESS) goto cleanup;
            } else {
                printf("did not match Nodes\n");
                exit(0);
            }

            /* only use component of vector that is parallel to Edge */
            vold[0] = XPNT(iedge,iloop,ipnt) - XPNT(iedge,nloop-1,ipnt);
            vold[1] = YPNT(iedge,iloop,ipnt) - YPNT(iedge,nloop-1,ipnt);
            vold[2] = ZPNT(iedge,iloop,ipnt) - ZPNT(iedge,nloop-1,ipnt);

            stepT = (vold[ 0] * tangB[3] + vold[ 1] * tangB[4] + vold[ 2] * tangB[5])
                  / (tangB[3] * tangB[3] + tangB[4] * tangB[4] + tangB[5] * tangB[5]);

            vnew[0] = stepT * tangB[3];
            vnew[1] = stepT * tangB[4];
            vnew[2] = stepT * tangB[5];

            XPNT(iedge,iloop,ipnt) = XPNT(iedge,nloop-1,ipnt) + vnew[0];
            YPNT(iedge,iloop,ipnt) = YPNT(iedge,nloop-1,ipnt) + vnew[1];
            ZPNT(iedge,iloop,ipnt) = ZPNT(iedge,nloop-1,ipnt) + vnew[2];
        }
    }

    /* match the points at the end of each region to match the
       beginning of the next region */
    for (iedge = 0; iedge < nedgeA; iedge++) {
        jedge = (iedge + nedgeA - 1) % nedgeA;

        for (iloop = 1; iloop < nloop-1; iloop++) {
            XPNT(jedge,iloop,npnt-1) = XPNT(iedge,iloop,0);
            YPNT(jedge,iloop,npnt-1) = YPNT(iedge,iloop,0);
            ZPNT(jedge,iloop,npnt-1) = ZPNT(iedge,iloop,0);
        }
    }

    /* modify the interior Points based upon the displacements
       at the ends (computed above) */
    for (iedge = 0; iedge < nedgeA; iedge++) {

        iloop = 1;

        ipnt  = 0;
        dxyzbeg[0] = XPNT(iedge,iloop,ipnt) - ((1-FRACA(0)) * XPNT(iedge,0,ipnt) + FRACA(0) * XPNT(iedge,nloop-1,ipnt));
        dxyzbeg[1] = YPNT(iedge,iloop,ipnt) - ((1-FRACA(0)) * YPNT(iedge,0,ipnt) + FRACA(0) * YPNT(iedge,nloop-1,ipnt));
        dxyzbeg[2] = ZPNT(iedge,iloop,ipnt) - ((1-FRACA(0)) * ZPNT(iedge,0,ipnt) + FRACA(0) * ZPNT(iedge,nloop-1,ipnt));

        ipnt = npnt - 1;
        dxyzend[0] = XPNT(iedge,iloop,ipnt) - ((1-FRACA(0)) * XPNT(iedge,0,ipnt) + FRACA(0) * XPNT(iedge,nloop-1,ipnt));
        dxyzend[1] = YPNT(iedge,iloop,ipnt) - ((1-FRACA(0)) * YPNT(iedge,0,ipnt) + FRACA(0) * YPNT(iedge,nloop-1,ipnt));
        dxyzend[2] = ZPNT(iedge,iloop,ipnt) - ((1-FRACA(0)) * ZPNT(iedge,0,ipnt) + FRACA(0) * ZPNT(iedge,nloop-1,ipnt));

        for (ipnt = 1; ipnt < npnt-1; ipnt++) {
            fraci = (double)(ipnt) / (double)(npnt-1);

            XPNT(iedge,iloop,ipnt) += (1-fraci) * dxyzbeg[0] + fraci * dxyzend[0];
            YPNT(iedge,iloop,ipnt) += (1-fraci) * dxyzbeg[1] + fraci * dxyzend[1];
            ZPNT(iedge,iloop,ipnt) += (1-fraci) * dxyzbeg[2] + fraci * dxyzend[2];
        }


        iloop = nloop - 2;

        ipnt = 0;
        dxyzbeg[0] = XPNT(iedge,iloop,ipnt) - ((1-FRACB(0)) * XPNT(iedge,nloop-1,ipnt) + FRACB(0) * XPNT(iedge,0,ipnt));
        dxyzbeg[1] = YPNT(iedge,iloop,ipnt) - ((1-FRACB(0)) * YPNT(iedge,nloop-1,ipnt) + FRACB(0) * YPNT(iedge,0,ipnt));
        dxyzbeg[2] = ZPNT(iedge,iloop,ipnt) - ((1-FRACB(0)) * ZPNT(iedge,nloop-1,ipnt) + FRACB(0) * ZPNT(iedge,0,ipnt));

        ipnt = npnt - 1;
        dxyzend[0] = XPNT(iedge,iloop,ipnt) - ((1-FRACB(0)) * XPNT(iedge,nloop-1,ipnt) + FRACB(0) * XPNT(iedge,0,ipnt));
        dxyzend[1] = YPNT(iedge,iloop,ipnt) - ((1-FRACB(0)) * YPNT(iedge,nloop-1,ipnt) + FRACB(0) * YPNT(iedge,0,ipnt));
        dxyzend[2] = ZPNT(iedge,iloop,ipnt) - ((1-FRACB(0)) * ZPNT(iedge,nloop-1,ipnt) + FRACB(0) * ZPNT(iedge,0,ipnt));

        for (ipnt = 1; ipnt < npnt-1; ipnt++) {
            fraci = (double)(ipnt) / (double)(npnt-1);

            XPNT(iedge,iloop,ipnt) += (1-fraci) * dxyzbeg[0] + fraci * dxyzend[0];
            YPNT(iedge,iloop,ipnt) += (1-fraci) * dxyzbeg[1] + fraci * dxyzend[1];
            ZPNT(iedge,iloop,ipnt) += (1-fraci) * dxyzbeg[2] + fraci * dxyzend[2];
        }
    }

    /* generate the points for the interior loops */
    for (iedge = 0; iedge < nedgeA; iedge++) {
        status = EG_getRange(eedgesA[iedge], trangeA, &periodic);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_getRange(eedgesB[iedge], trangeB, &periodic);
        if (status != EGADS_SUCCESS) goto cleanup;

        /* create the Curve using npnt points */
        for (ipnt = 1; ipnt < npnt-1; ipnt++) {
            fracj = (double)(ipnt) / (double)(npnt-1);

            /* modify the points associated with iloop=1 so that they lie
               in the plane of the adjoining surfaceA */
            iloop = 1;

            if (sensesA[iedge] == SFORWARD) {
                tt = (1-fracj) * trangeA[0] + fracj * trangeA[1];
            } else {
                tt = (1-fracj) * trangeA[1] + fracj * trangeA[0];
            }

            status = EG_getEdgeUV(efacesA[iedge], eedgesA[iedge], 0, tt, uvA);
            if (status != EGADS_SUCCESS) goto cleanup;

            status = EG_evaluate(efacesA[iedge], uvA, tangA);
            if (status != EGADS_SUCCESS) goto cleanup;

            /* only use component of vector that is along the surface */
            vold[0] = XPNT(iedge,iloop,ipnt) - XPNT(iedge,0,ipnt);
            vold[1] = YPNT(iedge,iloop,ipnt) - YPNT(iedge,0,ipnt);
            vold[2] = ZPNT(iedge,iloop,ipnt) - ZPNT(iedge,0,ipnt);

            stepU = (vold[ 0] * tangA[3] + vold[ 1] * tangA[4] + vold[ 2] * tangA[5])
                  / (tangA[3] * tangA[3] + tangA[4] * tangA[4] + tangA[5] * tangA[5]);
            stepV = (vold[ 0] * tangA[6] + vold[ 1] * tangA[7] + vold[ 2] * tangA[8])
                  / (tangA[6] * tangA[6] + tangA[7] * tangA[7] + tangA[8] * tangA[8]);

//$$$            printf("tangA %9.5f %9.5f %9.5f  %9.5f %9.5f %9.5f  %9.5f  %9.5f %9.5f\n",
//$$$                   tangA[3], tangA[4], tangA[5], tangA[6], tangA[7], tangA[8], 
//$$$                   tangA[3]*tangA[6]+tangA[4]*tangA[7]+tangA[5]*tangA[8], stepU, stepV);

            vnew[0] = stepU * tangA[3] + stepV * tangA[6];
            vnew[1] = stepU * tangA[4] + stepV * tangA[7];
            vnew[2] = stepU * tangA[5] + stepV * tangA[8];

//$$$            printf("A: iedge=%d, iloop=%d, ipnt=%2d, vold=%10.5f %10.5f %10.5f (%10.5f), vnew=%10.5f %10.5f %10.5f (%10.5f)\n",
//$$$                   iedge, iloop, ipnt, 
//$$$                   vold[0], vold[1], vold[2], sqrt(vold[0]*vold[0]+vold[1]*vold[1]+vold[2]*vold[2]),
//$$$                   vnew[0], vnew[1], vnew[2], sqrt(vnew[0]*vnew[0]+vnew[1]*vnew[1]+vnew[2]*vnew[2]));

            XPNT(iedge,iloop,ipnt) = XPNT(iedge,0,ipnt) + vnew[0];
            YPNT(iedge,iloop,ipnt) = YPNT(iedge,0,ipnt) + vnew[1];
            ZPNT(iedge,iloop,ipnt) = ZPNT(iedge,0,ipnt) + vnew[2];

            /* modify the points associated with iloop=nloop-2 so that they lie
               in the plane of the adjoining surfaceB */
            iloop = nloop - 2;

            if (sensesB[iedge] == SFORWARD) {
                tt = (1-fracj) * trangeB[0] + fracj * trangeB[1];
            } else {
                tt = (1-fracj) * trangeB[1] + fracj * trangeB[0];
            }

            status = EG_getEdgeUV(efacesB[iedge], eedgesB[iedge], 0, tt, uvB);
            if (status != EGADS_SUCCESS) goto cleanup;

            status = EG_evaluate(efacesB[iedge], uvB, tangB);
            if (status != EGADS_SUCCESS) goto cleanup;

            /* only use component of vector that is along the surface */
            vold[0] = XPNT(iedge,iloop,ipnt) - XPNT(iedge,nloop-1,ipnt);
            vold[1] = YPNT(iedge,iloop,ipnt) - YPNT(iedge,nloop-1,ipnt);
            vold[2] = ZPNT(iedge,iloop,ipnt) - ZPNT(iedge,nloop-1,ipnt);

            stepU = (vold[ 0] * tangB[3] + vold[ 1] * tangB[4] + vold[ 2] * tangB[5])
                  / (tangB[3] * tangB[3] + tangB[4] * tangB[4] + tangB[5] * tangB[5]);
            stepV = (vold[ 0] * tangB[6] + vold[ 1] * tangB[7] + vold[ 2] * tangB[8])
                  / (tangB[6] * tangB[6] + tangB[7] * tangB[7] + tangB[8] * tangB[8]);

//$$$            printf("tangB %9.5f %9.5f %9.5f  %9.5f %9.5f %9.5f  %9.5f  %9.5f %9.5f\n",
//$$$                   tangB[3], tangB[4], tangB[5], tangB[6], tangB[7], tangB[8], 
//$$$                   tangB[3]*tangB[6]+tangB[4]*tangB[7]+tangB[5]*tangB[8], stepU, stepV);

            vnew[0] = stepU * tangB[3] + stepV * tangB[6];
            vnew[1] = stepU * tangB[4] + stepV * tangB[7];
            vnew[2] = stepU * tangB[5] + stepV * tangB[8];

//$$$            printf("B: iedge=%d, iloop=%d, ipnt=%2d, vold=%10.5f %10.5f %10.5f (%10.5f), vnew=%10.5f %10.5f %10.5f (%10.5f)\n",
//$$$                   iedge, iloop, ipnt, 
//$$$                   vold[0], vold[1], vold[2], sqrt(vold[0]*vold[0]+vold[1]*vold[1]+vold[2]*vold[2]),
//$$$                   vnew[0], vnew[1], vnew[2], sqrt(vnew[0]*vnew[0]+vnew[1]*vnew[1]+vnew[2]*vnew[2]));

            XPNT(iedge,iloop,ipnt) = XPNT(iedge,nloop-1,ipnt) + vnew[0];
            YPNT(iedge,iloop,ipnt) = YPNT(iedge,nloop-1,ipnt) + vnew[1];
            ZPNT(iedge,iloop,ipnt) = ZPNT(iedge,nloop-1,ipnt) + vnew[2];
        }
    }

    /* plot the loops */
    if (PLOT(0) != 0) {
        FILE *fp;

        fp = fopen("flend.plot", "w");
        if (fp != NULL) {
            for (iedge = 0; iedge < nedgeA; iedge++) {
                for (iloop = 0; iloop < nloop; iloop++) {
                    fprintf(fp, "%5d %5d line_edge_%d_loop_%d\n", npnt, 1, iedge, iloop);
                    for (ipnt = 0; ipnt < npnt; ipnt++) {
                        fprintf(fp, " %9.5f %9.5f %9.5f\n", XPNT(iedge,iloop,ipnt), YPNT(iedge,iloop,ipnt), ZPNT(iedge,iloop,ipnt));
                    }
                    fprintf(fp, "%5d %5d pnts_edge_%d_loop_%d\n", npnt, 0, iedge, iloop);
                    for (ipnt = 0; ipnt < npnt; ipnt++) {
                        fprintf(fp, " %9.5f %9.5f %9.5f\n", XPNT(iedge,iloop,ipnt), YPNT(iedge,iloop,ipnt), ZPNT(iedge,iloop,ipnt));
                    }
                }
            }
        }
        fclose(fp);
    }

    /* get list of Faces in BodyA and BodyB */
    EG_free(efacesA);
    EG_free(efacesB);

    status = EG_getBodyTopos(ebodys[0], NULL, FACE, &nfaceA, &efacesA);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_getBodyTopos(ebodys[1], NULL, FACE, &nfaceB, &efacesB);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get a list to hold all Faces */
    efacelist = (ego *) EG_alloc((nedgeA+nfaceA+nfaceB)*sizeof(ego));
    if (efacelist == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    status = ocsmEvalExpr(modl, "@nbody", &value, &dot, temp);
    if (status != EGADS_SUCCESS) goto cleanup;

    brchattr[0] = -1;                   // fixed in buildPrimitive because _markFaces_ is not set
    bodyattr[0] = value + 1.01;

    spln = (double *) EG_alloc(6*npnt*sizeof(double));
    west = (double *) EG_alloc(3*npnt*sizeof(double));
    east = (double *) EG_alloc(3*npnt*sizeof(double));
    if (spln == NULL || west == NULL || east == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    /* make a Face associated with each Edge */
    for (iedge = 0; iedge < nedgeA; iedge++) {
        for (ipnt = 0; ipnt < npnt; ipnt++) {
            spln[6*ipnt  ] = XPNT(iedge,0,ipnt);
            spln[6*ipnt+1] = YPNT(iedge,0,ipnt);
            spln[6*ipnt+2] = ZPNT(iedge,0,ipnt);

            spln[6*ipnt+3] = XPNT(iedge,3,ipnt);
            spln[6*ipnt+4] = YPNT(iedge,3,ipnt);
            spln[6*ipnt+5] = ZPNT(iedge,3,ipnt);

            west[3*ipnt  ] = 10 * (XPNT(iedge,1,ipnt) - XPNT(iedge,0,ipnt));
            west[3*ipnt+1] = 10 * (YPNT(iedge,1,ipnt) - YPNT(iedge,0,ipnt));
            west[3*ipnt+2] = 10 * (ZPNT(iedge,1,ipnt) - ZPNT(iedge,0,ipnt));

            east[3*ipnt  ] = 10 * (XPNT(iedge,2,ipnt) - XPNT(iedge,3,ipnt));
            east[3*ipnt+1] = 10 * (YPNT(iedge,2,ipnt) - YPNT(iedge,3,ipnt));
            east[3*ipnt+2] = 10 * (ZPNT(iedge,2,ipnt) - ZPNT(iedge,3,ipnt));
        }

        status = EG_spline2dAppx(context, 0, NULL, NULL, NULL, west, east,
                                 NULL, NULL, NULL, NULL, 2, npnt, spln, toler, &esurf);
        if (status != EGADS_SUCCESS) goto cleanup;

        data[0] = 0;
        data[1] = 1;
        data[2] = 0;
        data[3] = 1;
        status = EG_makeFace(esurf, SFORWARD, data, &efacelist[iedge]);
        if (status != EGADS_SUCCESS) goto cleanup;

        /* set _brch and _body Attributes on new Face */
        brchattr[1] = iedge + 1;
        bodyattr[1] = iedge + 1;

        status = EG_attributeAdd(efacelist[iedge], "_brch", ATTRINT, 2,
                                 brchattr, NULL, NULL);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_attributeAdd(efacelist[iedge], "_body", ATTRINT, 2,
                                 bodyattr, NULL, NULL);
        if (status != EGADS_SUCCESS) goto cleanup;
    }

    /* add to efacelist the unmarked Faces in BodyA and BodyB */
    nfacelist = nedgeA;

    for (ifaceA = 0; ifaceA < nfaceA; ifaceA++) {
        status = EG_attributeRet(efacesA[ifaceA], "_flend", &atype, &alen,
                                 &tempIlist, &tempRlist, &tempClist);
        if (status != EGADS_SUCCESS || atype != ATTRSTRING || strcmp(tempClist, "remove") != 0) {
            efacelist[nfacelist++] = efacesA[ifaceA];
        }
    }

    for (ifaceB = 0; ifaceB < nfaceB; ifaceB++) {
        status = EG_attributeRet(efacesB[ifaceB], "_flend", &atype, &alen,
                                 &tempIlist, &tempRlist, &tempClist);
        if (status != EGADS_SUCCESS || atype != ATTRSTRING || strcmp(tempClist, "remove") != 0) {
            efacelist[nfacelist++] = efacesB[ifaceB];
        }
    }

    EG_free(efacesA);
    EG_free(efacesB);

    /* sew the Faces into a single (output) Body */
    status = EG_sewFaces(nfacelist, efacelist, TOLER(0), 0, &emodel);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_getTopology(emodel, &eref, &oclass, &mtype,
                            data, &nchild, &echilds, &senses);
    if (status != EGADS_SUCCESS) goto cleanup;

    if (nchild != 1) {
        printf(" udpExecute: expecting emodel to have only one child, but has %d\n", nchild);
        status = -287;
        goto cleanup;
    }

    status = EG_copyObject(echilds[0], NULL, ebody);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_getTopology(*ebody, &eref, &oclass, &mtype,
                            data, &nchild, &echilds, &senses);
    if (status != EGADS_SUCCESS) goto cleanup;

    if (mtype != SOLIDBODY) {
        printf(" udpExecute: expecting SolidBody\n");
        status = -288;
        goto cleanup;
    }

    status = EG_deleteObject(emodel);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* return output value(s) */

    /* remember this model (Body) */
    udps[numUdp].ebody = *ebody;
    goto cleanup;

#ifdef DEBUG
    printf("udpExecute -> *ebody=%llx\n", (long long)(*ebody));
#endif

cleanup:
    if (elistA    != NULL) EG_free(elistA   );
    if (elistB    != NULL) EG_free(elistB   );
    if (eloops    != NULL) EG_free(eloops   );
    if (eedgelist != NULL) EG_free(eedgelist);
    if (efacelist != NULL) EG_free(efacelist);
    if (point     != NULL) EG_free(point    );
    if (spln      != NULL) EG_free(spln     );
    if (west      != NULL) EG_free(west     );
    if (east      != NULL) EG_free(east     );

    if (status != EGADS_SUCCESS) {
        *string = udpErrorStr(status);
    }

    return status;
}

#undef XPNT
#undef YPNT
#undef ZPNT


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
 *   reorderLoop - reorder eloops[nloop-1] to give minimum twist from eloops[0] *
 *                                                                      *
 ************************************************************************
 */

static int
reorderLoop(int    nloop,               /* (in)  Loop to be reordered */
            ego    eloops[])            /* (in)  two Loops */
                                        /* (out) eloops[nloop-1] is modified */
{
    int     status = EGADS_SUCCESS;     /* (out) return status */

    int      iloop, jloop, iedge, jedge, itest, ishift;
    int      oclassi, oclassj, oclassk, oclassg, mtypei, mtypej, mtypek, mtypeg, nedgei, nedgej, nedgek;
    int      *sensesi, *sensesj, *sensesk, nnode, *sensesnew=NULL;
    double   uvlimitsi[4], uvlimitsj[4], uvlimitsk[4], data[18], *xyzi=NULL, *xyzj=NULL;
    double   areai[3], areaj[3], dotprod, ltest, lshift;
    ego      erefi, erefj, erefk, erefg, *eedgesi, *eedgesj, *eedgesk;
    ego      *enodes, *elist, *eedgesnew=NULL, etemp, context;

    ROUTINE(reorderLoop);

    /* --------------------------------------------------------------- */

    iloop = 0;
    jloop = nloop -1;

    /* get info on iloop */
    status = EG_getTopology(eloops[iloop], &erefi,
                            &oclassi, &mtypei, uvlimitsi, &nedgei, &eedgesi, &sensesi);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get info on jloop */
    status = EG_getTopology(eloops[jloop], &erefj,
                            &oclassj, &mtypej, uvlimitsj, &nedgej, &eedgesj, &sensesj);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* set up the coordinates at the Nodes and midpoints in the two Loops */
    xyzi = (double *) EG_alloc(6*nedgei*sizeof(double));
    xyzj = (double *) EG_alloc(6*nedgei*sizeof(double));
    if (xyzi == NULL || xyzj == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    for (iedge = 0; iedge < nedgei; iedge++) {
        status = EG_getTopology(eedgesi[iedge], &erefk,
                                &oclassk, &mtypeg, uvlimitsk, &nnode, &enodes, &sensesk);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (sensesi[iedge] > 0) {
            status = EG_getTopology(enodes[0], &erefk,
                                    &oclassk, &mtypek, &(xyzi[6*iedge]), &nnode, &elist, &sensesk);
            if (status != EGADS_SUCCESS) goto cleanup;
        } else {
            status = EG_getTopology(enodes[1], &erefk,
                                    &oclassk, &mtypek, &(xyzi[6*iedge]), &nnode, &elist, &sensesk);
            if (status != EGADS_SUCCESS) goto cleanup;
        }

        if (mtypeg != DEGENERATE) {
            uvlimitsk[2] = (uvlimitsk[0] + uvlimitsk[1]) / 2;

            status = EG_evaluate(eedgesi[iedge], &uvlimitsk[2], data);
            if (status != EGADS_SUCCESS) goto cleanup;

            xyzi[6*iedge+3] = data[0];
            xyzi[6*iedge+4] = data[1];
            xyzi[6*iedge+5] = data[2];
        } else {
            xyzi[6*iedge+3] = xyzi[3*iedge  ];
            xyzi[6*iedge+4] = xyzi[3*iedge+1];
            xyzi[6*iedge+5] = xyzi[3*iedge+2];
        }
    }

    for (iedge = 0; iedge < nedgej; iedge++) {
        status = EG_getTopology(eedgesj[iedge], &erefk,
                                &oclassk, &mtypeg, uvlimitsk, &nnode, &enodes, &sensesk);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (sensesj[iedge] > 0) {
            status = EG_getTopology(enodes[0], &erefk,
                                    &oclassk, &mtypek, &(xyzj[6*iedge]), &nnode, &elist, &sensesk);
            if (status != EGADS_SUCCESS) goto cleanup;
        } else {
            status = EG_getTopology(enodes[1], &erefk,
                                    &oclassk, &mtypek, &(xyzj[6*iedge]), &nnode, &elist, &sensesk);
            if (status != EGADS_SUCCESS) goto cleanup;
        }

        if (mtypeg != DEGENERATE) {
            uvlimitsk[2] = (uvlimitsk[0] + uvlimitsk[1]) / 2;

            status = EG_evaluate(eedgesj[iedge], &uvlimitsk[2], data);
            if (status != EGADS_SUCCESS) goto cleanup;

            xyzj[6*iedge+3] = data[0];
            xyzj[6*iedge+4] = data[1];
            xyzj[6*iedge+5] = data[2];
        } else {
            xyzj[6*iedge+3] = xyzj[3*iedge  ];
            xyzj[6*iedge+4] = xyzj[3*iedge+1];
            xyzj[6*iedge+5] = xyzj[3*iedge+2];
        }
    }

    /* find the area of iloop and jloop */
    areai[0] = areai[1] = areai[2] = 0;
    areaj[0] = areaj[1] = areaj[2] = 0;

    for (iedge = 1; iedge < 2*nedgei-1; iedge++) {
        areai[0] += (xyzi[1] - xyzi[6*nedgei-2]) * (xyzi[3*iedge+2] - xyzi[6*nedgei-1])
                  - (xyzi[2] - xyzi[6*nedgei-1]) * (xyzi[3*iedge+1] - xyzi[6*nedgei-2]);
        areai[1] += (xyzi[2] - xyzi[6*nedgei-1]) * (xyzi[3*iedge  ] - xyzi[6*nedgei-3])
                  - (xyzi[0] - xyzi[6*nedgei-3]) * (xyzi[3*iedge+2] - xyzi[6*nedgei-1]);
        areai[2] += (xyzi[0] - xyzi[6*nedgei-3]) * (xyzi[3*iedge+1] - xyzi[6*nedgei-2])
                  - (xyzi[1] - xyzi[6*nedgei-2]) * (xyzi[3*iedge  ] - xyzi[6*nedgei-3]);

        areaj[0] += (xyzj[1] - xyzj[6*nedgej-2]) * (xyzj[3*iedge+2] - xyzj[6*nedgej-1])
                  - (xyzj[2] - xyzj[6*nedgej-1]) * (xyzj[3*iedge+1] - xyzj[6*nedgej-2]);
        areaj[1] += (xyzj[2] - xyzj[6*nedgej-1]) * (xyzj[3*iedge  ] - xyzj[6*nedgej-3])
                  - (xyzj[0] - xyzj[6*nedgej-3]) * (xyzj[3*iedge+2] - xyzj[6*nedgej-1]);
        areaj[2] += (xyzj[0] - xyzj[6*nedgej-3]) * (xyzj[3*iedge+1] - xyzj[6*nedgej-2])
                  - (xyzj[1] - xyzj[6*nedgej-2]) * (xyzj[3*iedge  ] - xyzj[6*nedgej-3]);
    }

    /* if the dot products of the areas is negative, flip the direction of jloop */
    dotprod = areai[0] * areaj[0] + areai[1] * areaj[1] + areai[2] * areaj[2];
    if (dotprod < 0) {
        status = flipLoop(&(eloops[jloop]));
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_getTopology(eloops[jloop], &erefj,
                                &oclassj, &mtypej, uvlimitsj, &nedgej, &eedgesj, &sensesj);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (oclassj == FACE) {
            etemp  = eedgesj[0];
            status = EG_getTopology(etemp, &erefj,
                                    &oclassj, &mtypej, uvlimitsj, &nedgej, &eedgesj, &sensesj);
            if (status != EGADS_SUCCESS) goto cleanup;
        }

        for (iedge = 0; iedge < nedgej; iedge++) {
            status = EG_getTopology(eedgesj[iedge], &erefk,
                                    &oclassk, &mtypek, uvlimitsk, &nnode, &enodes, &sensesk);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (sensesj[iedge] > 0) {
                status = EG_getTopology(enodes[0], &erefk,
                                        &oclassk, &mtypek, &(xyzj[6*iedge]), &nnode, &elist, &sensesk);
                if (status != EGADS_SUCCESS) goto cleanup;
            } else {
                status = EG_getTopology(enodes[1], &erefk,
                                        &oclassk, &mtypek, &(xyzj[6*iedge]), &nnode, &elist, &sensesk);
                if (status != EGADS_SUCCESS) goto cleanup;
            }
        }
    }

    /* find the shift of jloop that minimizes the distance between the Nodes of iloop
       and the Nodes of jloop */
    ishift = 0;
    lshift = HUGEQ;

    /* compute the sum of the distances for each candidate shift */
    for (itest = 0; itest < 2*nedgei; itest++) {

        ltest = 0;
        for (iedge = 0; iedge < 2*nedgei; iedge++) {
            jedge = (iedge + itest) % (2*nedgei);

            ltest += sqrt((xyzi[3*iedge  ]-xyzj[3*jedge  ])*(xyzi[3*iedge  ]-xyzj[3*jedge  ])
                         +(xyzi[3*iedge+1]-xyzj[3*jedge+1])*(xyzi[3*iedge+1]-xyzj[3*jedge+1])
                         +(xyzi[3*iedge+2]-xyzj[3*jedge+2])*(xyzi[3*iedge+2]-xyzj[3*jedge+2]));
        }
        if (ltest < lshift) {
            ishift = itest / 2;
            lshift = ltest;
        }
    }

    /* create the new rotated Loop */
    if (ishift > 0) {
        eedgesnew = (ego *) EG_alloc(2*nedgej*sizeof(ego));
        sensesnew = (int *) EG_alloc(2*nedgej*sizeof(int));
        if (eedgesnew == NULL || sensesnew == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        /* shift the Loop by ishift */
        for (iedge = 0; iedge < nedgej; iedge++) {
            jedge = (iedge + ishift) % nedgej;
            eedgesnew[iedge] = eedgesj[jedge];
            sensesnew[iedge] = sensesj[jedge];
        }

        status = EG_getTopology(eloops[jloop], &erefk,
                                &oclassk, &mtypek, uvlimitsk, &nedgek, &eedgesk, &sensesk);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (oclassk == FACE) {
            status = EG_getGeometry(erefk, &oclassg, &mtypeg, &erefg, NULL, NULL);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (mtypeg != PLANE) {
                for (iedge = 0; iedge < nedgej; iedge++) {
                    jedge = (iedge + ishift) % nedgej;
                    eedgesnew[iedge+nedgej] = eedgesj[jedge+nedgej];
                    sensesnew[iedge+nedgej] = sensesj[jedge+nedgej];
                }
            }
        }

        /* make new Loop */
        status = EG_getContext(eloops[0], &context);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_makeTopology(context, erefk, LOOP, CLOSED,
                                 NULL, nedgej, eedgesnew, sensesnew, &eloops[jloop]);
        if (status != EGADS_SUCCESS) goto cleanup;

        EG_free(sensesnew);
        EG_free(eedgesnew);

        sensesnew = NULL;
        eedgesnew = NULL;
    }

cleanup:
    if (xyzi != NULL) EG_free(xyzi);
    if (xyzj != NULL) EG_free(xyzj);

    return status;
}


/*
 ************************************************************************
 *                                                                      *
 *   flipLoop - flip Loop using original Edges                          *
 *                                                                      *
 ************************************************************************
 */

static int
flipLoop(ego    *eloop)                 /* (both) Loop to be flipped */
{
    int    status = SUCCESS;            /* (out)  return status */

    int    oclass, mtype, nedge, *oldSenses, *newSenses=NULL, i, j;
    double data[4];
    ego    context, eref, *oldEedges, *newEedges=NULL;

    ROUTINE(flipLoop);

    /* --------------------------------------------------------------- */

    status = EG_getContext(*eloop, &context);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_getTopology(*eloop, &eref, &oclass, &mtype,
                            data, &nedge, &oldEedges, &oldSenses);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* if eref!=NULL, we might need to take care of Pcurves too */
    if (oclass != LOOP || mtype != CLOSED || eref != NULL) {
        status = -289;
        goto cleanup;
    }

    newEedges = (ego *) EG_alloc(nedge*sizeof(ego));
    newSenses = (int *) EG_alloc(nedge*sizeof(int));
    if (newEedges == NULL || newSenses == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    i = 0;
    j = nedge - 1;

    while (i <= j) {
        newEedges[i] =  oldEedges[j];
        newSenses[i] = -oldSenses[j];

        newEedges[j] =  oldEedges[i];
        newSenses[j] = -oldSenses[i];

        i++;
        j--;
    }

    status = EG_makeTopology(context, eref, LOOP, CLOSED, data,
                             nedge, newEedges, newSenses, eloop);
    if (status != EGADS_SUCCESS) goto cleanup;

cleanup:
    if (newEedges != NULL) EG_free(newEedges);
    if (newSenses != NULL) EG_free(newSenses);

    return status;
}
