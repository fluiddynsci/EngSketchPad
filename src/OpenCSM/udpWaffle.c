/*
 ************************************************************************
 *                                                                      *
 * udpWaffle -- udp file to generate 2D waffle                          *
 *                                                                      *
 *            Written by John Dannenhoffer @ Syracuse University        *
 *            Patterned after code written by Bob Haimes  @ MIT         *
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

#define NUMUDPARGS 4
#include "udpUtilities.h"

/* shorthands for accessing argument values and velocities */
#define DEPTH(   IUDP)    ((double *) (udps[IUDP].arg[0].val))[0]
#define SEGMENTS(IUDP,I)  ((double *) (udps[IUDP].arg[1].val))[I]
#define FILENAME(IUDP)    ((char   *) (udps[IUDP].arg[2].val))
#define PROGRESS(IUDP)    ((int    *) (udps[IUDP].arg[3].val))[0]

/* data about possible arguments */
static char  *argNames[NUMUDPARGS] = {"depth",  "segments", "filename", "progress", };
static int    argTypes[NUMUDPARGS] = {ATTRREAL, ATTRREAL,   ATTRSTRING, ATTRINT,    };
static int    argIdefs[NUMUDPARGS] = {0,        0,          0,          0,          };
static double argDdefs[NUMUDPARGS] = {1.,       0.,         0.,         0.,         };

/* get utility routines: udpErrorStr, udpInitialize, udpReset, udpSet,
                         udpGet, udpVel, udpClean, udpMesh */
#include "udpUtilities.c"

#ifdef GRAFIC
    #include "grafic.h"
    #define DEBUG
#endif

/* include OpenCSM to make use of its evaluator */
#include "OpenCSM.h"

typedef struct {
    int    type;                        /* =0 for cpoint, =1 for point */
    double x;                           /* X-coordinate */
    double y;                           /* Y-coordinate */
    char   name[80];                    /* name */
} pnt_T;

typedef struct {
    int    type;                        /* =0 for cline, =1 for line */
    int    ibeg;                        /* Point at beginning */
    int    iend;                        /* Point at end */
    int    num;                         /* number */
    int    idx;                         /* index */
    char   name[80];                    /* name */
    int    nattr;                       /* number of Attributes */
    char   **aname;                     /* array  of Attribute names */
    char   **avalu;                     /* array  of Attribute values */
} seg_T;

/* prototype for function defined below */
static int processSegments(         int *npnt, pnt_T *pnt_p[], int *nseg, seg_T *seg_p[]);
static int processFile(ego context, int *npnt, pnt_T *pnt_p[], int *nseg, seg_T *seg_p[]);
static int getToken(char *text, int nskip, char sep, int maxtok, char *token);

#ifdef GRAFIC
static int     plotWaffle(int npnt, pnt_T pnt[], int nseg, seg_T seg[]);
#endif

#define           MAX(A,B)        (((A) < (B)) ? (B) : (A))


/*
 ************************************************************************
 *                                                                      *
 *   udpExecute - execute the primitive                                 *
 *                                                                      *
 ************************************************************************
 */

int
udpExecute(ego  context,                /* (in)  EGADS context */
           ego  *ebody,                 /* (out) Body pointer */
           int  *nMesh,                 /* (out) number of associated meshes */
           char *string[])              /* (out) error message */
{
    int     status = EGADS_SUCCESS;

    int     ipnt, npnt, inode, nnode, iseg, jseg, nseg, jedge, jface, senses[4];
    int     ibeg, iend, jbeg, jend, seginfo[2], jpnt, mpnt, mseg, iter, nchange, iattr;
    double  xyz[20], data[6], xyz_out[20], D, s, t, xx, yy, frac, dist;
    pnt_T   *pnt=NULL;
    seg_T   *seg=NULL;
    void    *temp;
    ego     *enodes=NULL, *eedges=NULL, *efaces=NULL, ecurve, echild[4], eloop, eshell;

    double  EPS06 = 1.0e-6;

#ifdef DEBUG
    printf("udpExecute(context=%llx)\n", (long long)context);
    printf("depth(   0) = %f\n", DEPTH(     0));
    printf("segments(0) = %f",   SEGMENTS(0,0));
    for (iseg = 1; iseg < udps[0].arg[1].size; iseg++) {
        printf(" %f", SEGMENTS(0,iseg));
    }
    printf("\n");
    printf("filename(0) = %s\n", FILENAME(  0));
    printf("progress(0) = %d\n", PROGRESS(  0));
#endif

    /* default return values */
    *ebody  = NULL;
    *nMesh  = 0;
    *string = NULL;

    /* check arguments */
    if (udps[0].arg[1].size == 1 && STRLEN(FILENAME(0)) == 0) {
        printf(" udpExecute: must specify segments or filename\n");
        status = EGADS_RANGERR;
        goto cleanup;

    } else if (udps[0].arg[1].size > 1 && STRLEN(FILENAME(0)) > 0) {
        printf(" udpExecute: must specify segments or filename\n");
        status = EGADS_RANGERR;
        goto cleanup;

    } else if (udps[0].arg[0].size > 1) {
        printf(" udpExecute: depth should be a scalar\n");
        status = EGADS_RANGERR;
        goto cleanup;

    } else if (DEPTH(0) <= 0) {
        printf(" udpExecute: depth = %f <= 0\n", DEPTH(0));
        status = EGADS_RANGERR;
        goto cleanup;

    } else if (STRLEN(FILENAME(0)) == 0 && udps[0].arg[1].size%4 != 0) {
        printf(" udpExecute: segments must be divisible by 4\n");
        status = EGADS_RANGERR;
        goto cleanup;
    }

    /* cache copy of arguments for future use */
    status = cacheUdp();
    if (status < 0) {
        printf(" udpExecute: problem caching arguments\n");
        goto cleanup;
    }

#ifdef DEBUG
    printf("depth(   %d) = %f\n", numUdp, DEPTH(   numUdp));
    printf("segments(%d) = %f",   numUdp, SEGMENTS(numUdp,0));
    for (iseg = 1; iseg < udps[0].arg[1].size; iseg++) {
        printf(" %f", SEGMENTS(numUdp,iseg));
    }
    printf("\n");
    printf("filename(%d) = %s\n", numUdp, FILENAME(numUdp));
    printf("progress(%d) = %d\n", numUdp, PROGRESS(numUdp));
#endif

    /* if filename is given, process the file */
    if (STRLEN(FILENAME(numUdp)) > 0) {
        status = processFile(context, &npnt, &pnt, &nseg, &seg);
        if (status != EGADS_SUCCESS) goto cleanup;

    /* otherwise, process the Segments */
    } else {
        status = processSegments(&npnt, &pnt, &nseg, &seg);
        if (status != EGADS_SUCCESS) goto cleanup;
    }

    mpnt = npnt;
    mseg = nseg;

    /* check for intersections of Lines only */
    for (jseg = 0; jseg < nseg; jseg++) {
        if (seg[jseg].type == 0) continue;

        for (iseg = jseg+1; iseg < nseg; iseg++) {
            if (seg[iseg].type == 0) continue;

            ibeg = seg[iseg].ibeg;
            iend = seg[iseg].iend;
            jbeg = seg[jseg].ibeg;
            jend = seg[jseg].iend;

            D = (pnt[iend].x - pnt[ibeg].x) * (pnt[jbeg].y - pnt[jend].y) - (pnt[jbeg].x - pnt[jend].x) * (pnt[iend].y - pnt[ibeg].y);
            if (fabs(D) > EPS06) {
                s = ((pnt[jbeg].x - pnt[ibeg].x) * (pnt[jbeg].y - pnt[jend].y) - (pnt[jbeg].x - pnt[jend].x) * (pnt[jbeg].y - pnt[ibeg].y)) / D;
                t = ((pnt[iend].x - pnt[ibeg].x) * (pnt[jbeg].y - pnt[ibeg].y) - (pnt[jbeg].x - pnt[ibeg].x) * (pnt[iend].y - pnt[ibeg].y)) / D;

                if (s > -EPS06 && s < 1+EPS06 &&
                    t > -EPS06 && t < 1+EPS06   ) {
                    xx = (1 - s) * pnt[ibeg].x + s * pnt[iend].x;
                    yy = (1 - s) * pnt[ibeg].y + s * pnt[iend].y;

                    ipnt = -1;
                    for (jpnt = 0; jpnt < npnt; jpnt++) {
                        if (fabs(xx-pnt[jpnt].x) < EPS06 &&
                            fabs(yy-pnt[jpnt].y) < EPS06   ) {
                            ipnt = jpnt;
                            break;
                        }
                    }

                    if (ipnt < 0) {
                        if (npnt+1 >= mpnt) {
                            mpnt += 10;

                            temp = EG_reall(pnt, mpnt*sizeof(pnt_T));
                            if (temp != NULL) {
                                pnt = (pnt_T *) temp;
                            } else {
                                status = EGADS_MALLOC;
                                goto cleanup;
                            }
                        }

                        ipnt = npnt;

                        pnt[npnt].type = 1;
                        pnt[npnt].x    = xx;
                        pnt[npnt].y    = yy;
                        npnt++;
                    }

                    if ((ibeg != ipnt && iend != ipnt) ||
                        (jbeg != ipnt && jend != ipnt)   ) {
                        if (nseg+2 >= mseg) {
                            mseg += 10;

                            temp = EG_reall(seg, mseg*sizeof(seg_T));
                            if (temp != NULL) {
                                seg = (seg_T *) temp;
                            } else {
                                status = EGADS_MALLOC;
                                goto cleanup;
                            }
                        }
                    }

                    if (ibeg != ipnt && iend != ipnt) {
                        seg[nseg].type    = seg[iseg].type;
                        seg[nseg].ibeg    = ipnt;
                        seg[nseg].iend    = iend;
                        seg[nseg].num     = seg[iseg].num;
                        seg[nseg].idx     = 0;
                        seg[nseg].name[0] = '\0';

                        seg[nseg].nattr = seg[iseg].nattr;
                        if (seg[nseg].nattr == 0) {
                            seg[nseg].aname = NULL;
                            seg[nseg].avalu = NULL;
                        } else {
                            seg[nseg].aname = (char **) EG_alloc(seg[nseg].nattr*sizeof(char *));
                            seg[nseg].avalu = (char **) EG_alloc(seg[nseg].nattr*sizeof(char *));
                            if (seg[nseg].aname == NULL || seg[nseg].avalu == NULL) {
                                status = EGADS_MALLOC;
                                goto cleanup;
                            }
                        }

                        for (iattr = 0; iattr < seg[nseg].nattr; iattr++) {
                            seg[nseg].aname[iattr] = (char *) EG_alloc(80*sizeof(char));
                            seg[nseg].avalu[iattr] = (char *) EG_alloc(80*sizeof(char));
                            if (seg[nseg].aname[iattr] == NULL || seg[nseg].avalu[iattr] == NULL) {
                                status = EGADS_MALLOC;
                                goto cleanup;
                            }

                            strncpy(seg[nseg].aname[iattr], seg[iseg].aname[iattr], 80);
                            strncpy(seg[nseg].avalu[iattr], seg[iseg].avalu[iattr], 80);
                        }

                        nseg++;

                        seg[iseg].iend = ipnt;
                    }

                    if (jbeg != ipnt && jend != ipnt) {
                        seg[nseg].type    = seg[jseg].type;
                        seg[nseg].ibeg    = ipnt;
                        seg[nseg].iend    = jend;
                        seg[nseg].num     = seg[jseg].num;
                        seg[nseg].idx     = 0;
                        seg[nseg].name[0] = '\0';

                        seg[nseg].nattr = seg[jseg].nattr;
                        if (seg[nseg].nattr == 0) {
                            seg[nseg].aname = NULL;
                            seg[nseg].avalu = NULL;
                        } else {
                            seg[nseg].aname = (char **) EG_alloc(seg[nseg].nattr*sizeof(char *));
                            seg[nseg].avalu = (char **) EG_alloc(seg[nseg].nattr*sizeof(char *));
                            if (seg[nseg].aname == NULL || seg[nseg].avalu == NULL) {
                                status = EGADS_MALLOC;
                                goto cleanup;
                            }
                        }

                        for (iattr = 0; iattr < seg[nseg].nattr; iattr++) {
                            seg[nseg].aname[iattr] = (char *) EG_alloc(80*sizeof(char));
                            seg[nseg].avalu[iattr] = (char *) EG_alloc(80*sizeof(char));
                            if (seg[nseg].aname[iattr] == NULL || seg[nseg].avalu[iattr] == NULL) {
                                status = EGADS_MALLOC;
                                goto cleanup;
                            }

                            strncpy(seg[nseg].aname[iattr], seg[jseg].aname[iattr], 80);
                            strncpy(seg[nseg].avalu[iattr], seg[jseg].avalu[iattr], 80);
                        }

                        nseg++;

                        seg[jseg].iend = ipnt;
                    }
                }
            }
        }
    }

    /* break Lines at Points */
    for (iseg = 0; iseg < nseg; iseg++) {
        if (seg[iseg].type == 0) continue;
        
        ibeg = seg[iseg].ibeg;
        iend = seg[iseg].iend;

        for (ipnt = 0; ipnt < npnt; ipnt++) {
            if (pnt[ipnt].type == 0) continue;
            
            /* distance from Point to line */
            frac   = ( (pnt[ipnt].x - pnt[ibeg].x) * (pnt[iend].x - pnt[ibeg].x)
                     + (pnt[ipnt].y - pnt[ibeg].y) * (pnt[iend].y - pnt[ibeg].y))
                   / ( (pnt[iend].x - pnt[ibeg].x) * (pnt[iend].x - pnt[ibeg].x)
                     + (pnt[iend].y - pnt[ibeg].y) * (pnt[iend].y - pnt[ibeg].y));

            if (frac < EPS06 || frac > 1-EPS06) continue;

            xx = (1-frac) * pnt[ibeg].x + frac * pnt[iend].x;
            yy = (1-frac) * pnt[ibeg].y + frac * pnt[iend].y;

            dist = sqrt( (xx - pnt[ipnt].x) * (xx - pnt[ipnt].x)
                       + (yy - pnt[ipnt].y) * (yy - pnt[ipnt].y));
            

            if (dist < EPS06) {

                /* make room for new Segment */
                if (nseg+1 >= mseg) {
                    mseg += 10;

                    temp = EG_reall(seg, mseg*sizeof(seg_T));
                    if (temp != NULL) {
                        seg = (seg_T *) temp;
                    } else {
                        status = EGADS_MALLOC;
                        goto cleanup;
                    }
                }

                /* make second half */
                seg[nseg].type    = seg[iseg].type;
                seg[nseg].ibeg    = ipnt;
                seg[nseg].iend    = seg[iseg].iend;
                seg[nseg].num     = seg[iseg].num;
                seg[nseg].idx     = 0;
                seg[nseg].name[0] = '\0';

                seg[nseg].nattr = seg[iseg].nattr;
                if (seg[nseg].nattr == 0) {
                    seg[nseg].aname = NULL;
                    seg[nseg].avalu = NULL;
                } else {
                    seg[nseg].aname = (char **) EG_alloc(seg[nseg].nattr*sizeof(char *));
                    seg[nseg].avalu = (char **) EG_alloc(seg[nseg].nattr*sizeof(char *));
                    if (seg[nseg].aname == NULL || seg[nseg].avalu == NULL) {
                        status = EGADS_MALLOC;
                        goto cleanup;
                    }
                }

                for (iattr = 0; iattr < seg[nseg].nattr; iattr++) {
                    seg[nseg].aname[iattr] = (char *) EG_alloc(80*sizeof(char));
                    seg[nseg].avalu[iattr] = (char *) EG_alloc(80*sizeof(char));
                    if (seg[nseg].aname[iattr] == NULL || seg[nseg].avalu[iattr] == NULL) {
                                status = EGADS_MALLOC;
                                goto cleanup;
                    }

                    strncpy(seg[nseg].aname[iattr], seg[iseg].aname[iattr], 80);
                    strncpy(seg[nseg].avalu[iattr], seg[iseg].avalu[iattr], 80);
                }

                nseg++;

                seg[iseg].iend = ipnt;

                /* revise first half */
                seg[iseg].iend = ipnt;
            }
        }
    }
    

    /* remove the "cline" Segments */
    for (iseg = 0; iseg < nseg; iseg++) {
        if (seg[iseg].type == 0) {
            if (seg[iseg].aname != NULL) EG_free(seg[iseg].aname);
            if (seg[iseg].avalu != NULL) EG_free(seg[iseg].avalu);

            for (jseg = iseg; jseg < nseg-1; jseg++) {
                seg[jseg].type  = seg[jseg+1].type;
                seg[jseg].ibeg  = seg[jseg+1].ibeg;
                seg[jseg].iend  = seg[jseg+1].iend;
                seg[jseg].num   = seg[jseg+1].num;
                seg[jseg].idx   = seg[jseg+1].idx;
                seg[jseg].nattr = seg[jseg+1].nattr;
                seg[jseg].aname = seg[jseg+1].aname;
                seg[jseg].avalu = seg[jseg+1].avalu;

                strcpy(seg[jseg].name, seg[jseg+1].name);
            }

            iseg--;
            nseg--;
        }
    }

    /* check for degenerate Segments */
    for (jseg = 0; jseg < nseg; jseg++) {
        if (seg[jseg].ibeg == seg[jseg].iend) {
            printf(" udpExecute: Segment %d is degenerate\n", iseg);
            status = EGADS_DEGEN;
            goto cleanup;
        }
    }

    /* assign the Segment indices */
    for (iter = 0; iter < nseg; iter++) {
        nchange = 0;

        for (iseg = 0; iseg < nseg; iseg++) {
            if (seg[iseg].idx > 0) continue;

            for (jseg = 0; jseg < nseg; jseg++) {
                if (jseg == iseg || seg[jseg].idx <= 0) continue;

                if (seg[iseg].num  == seg[jseg].num  &&
                    seg[iseg].ibeg == seg[jseg].iend   ) {
                    seg[iseg].idx = seg[jseg].idx + 1;
                    nchange++;
                    break;
                }
            }
        }

        if (nchange == 0) break;
    }

    /* show Points and Segments after intersections */
    if (PROGRESS(numUdp) != 0) {
        printf("after intersections\n");
        for (ipnt = 0; ipnt < npnt; ipnt++) {
            printf("        Pnt %3d: %-20s %1d %10.5f %10.5f\n", ipnt,
                   pnt[ipnt].name, pnt[ipnt].type, pnt[ipnt].x, pnt[ipnt].y);
        }
        for (iseg = 0; iseg < nseg; iseg++) {
            printf("        Seg %3d: %-20s %1d %5d %5d\n", iseg,
                   seg[iseg].name, seg[iseg].type, seg[iseg].ibeg, seg[iseg].iend);
        }
    }

    /* allocate space for the egos */
    enodes = (ego *) EG_alloc((2*npnt       )*sizeof(ego));
    eedges = (ego *) EG_alloc((  npnt+2*nseg)*sizeof(ego));
    efaces = (ego *) EG_alloc((         nseg)*sizeof(ego));

    if (enodes == NULL || eedges == NULL || efaces == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    /* set up enodes at Z=0 and at Z=Depth */
    nnode = 0;

    for (ipnt = 0; ipnt < npnt; ipnt++) {
        xyz[0] = pnt[ipnt].x;
        xyz[1] = pnt[ipnt].y;
        xyz[2] = 0;

        status = EG_makeTopology(context, NULL, NODE, 0,
                                 xyz, 0, NULL, NULL, &(enodes[nnode]));
        if (status != EGADS_SUCCESS) goto cleanup;

        nnode++;
    }

    for (ipnt = 0; ipnt < npnt; ipnt++) {
        xyz[0] = pnt[ipnt].x;
        xyz[1] = pnt[ipnt].y;
        xyz[2] = DEPTH(0);

        status = EG_makeTopology(context, NULL, NODE, 0, xyz, 0,
                                 NULL, NULL, &(enodes[nnode]));
        if (status != EGADS_SUCCESS) goto cleanup;

        nnode++;
    }

    /* set up the edges on the z=0 plane */
    jedge = 0;

    for (iseg = 0; iseg < nseg; iseg++) {
        xyz[0] = pnt[seg[iseg].ibeg].x;
        xyz[1] = pnt[seg[iseg].ibeg].y;
        xyz[2] = 0;
        xyz[3] = pnt[seg[iseg].iend].x - pnt[seg[iseg].ibeg].x;
        xyz[4] = pnt[seg[iseg].iend].y - pnt[seg[iseg].ibeg].y;
        xyz[5] = 0;

        status = EG_makeGeometry(context, CURVE, LINE, NULL,
                                 NULL, xyz, &ecurve);
        if (status != EGADS_SUCCESS) goto cleanup;

        echild[0] = enodes[seg[iseg].ibeg];
        echild[1] = enodes[seg[iseg].iend];

        status = EG_invEvaluate(ecurve, xyz, &(data[0]), xyz_out);
        if (status != EGADS_SUCCESS) goto cleanup;

        xyz[0] = pnt[seg[iseg].iend].x;
        xyz[1] = pnt[seg[iseg].iend].y;
        xyz[2] = 0;

        status = EG_invEvaluate(ecurve, xyz, &(data[1]), xyz_out);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_makeTopology(context, ecurve, EDGE, TWONODE,
                                 data, 2, echild, NULL, &(eedges[jedge]));
        if (status != EGADS_SUCCESS) goto cleanup;

        jedge++;
    }

    /* set up the edges on the z=Depth plane */
    for (iseg = 0; iseg < nseg; iseg++) {
        xyz[0] = pnt[seg[iseg].ibeg].x;
        xyz[1] = pnt[seg[iseg].ibeg].y;
        xyz[2] = DEPTH(0);
        xyz[3] = pnt[seg[iseg].iend].x - pnt[seg[iseg].ibeg].x;
        xyz[4] = pnt[seg[iseg].iend].y - pnt[seg[iseg].ibeg].y;
        xyz[5] = 0;

        status = EG_makeGeometry(context, CURVE, LINE, NULL,
                                 NULL, xyz, &ecurve);
        if (status != EGADS_SUCCESS) goto cleanup;

        echild[0] = enodes[seg[iseg].ibeg+nnode/2];
        echild[1] = enodes[seg[iseg].iend+nnode/2];

        status = EG_invEvaluate(ecurve, xyz, &(data[0]), xyz_out);
        if (status != EGADS_SUCCESS) goto cleanup;

        xyz[0] = pnt[seg[iseg].iend].x;
        xyz[1] = pnt[seg[iseg].iend].y;
        xyz[2] = DEPTH(0);

        status = EG_invEvaluate(ecurve, xyz, &(data[1]), xyz_out);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_makeTopology(context, ecurve, EDGE, TWONODE,
                                 data, 2, echild, NULL, &(eedges[jedge]));
        if (status != EGADS_SUCCESS) goto cleanup;

        jedge++;
    }

    /* set up the edges between z=0 and Depth */
    for (inode = 0; inode < nnode/2; inode++) {
        xyz[0] = pnt[inode].x;
        xyz[1] = pnt[inode].y;
        xyz[2] = 0;
        xyz[3] = 0;
        xyz[4] = 0;
        xyz[5] = 1;

        status = EG_makeGeometry(context, CURVE, LINE, NULL,
                                 NULL, xyz, &ecurve);
        if (status != EGADS_SUCCESS) goto cleanup;

        echild[0] = enodes[inode        ];
        echild[1] = enodes[inode+nnode/2];

        status = EG_invEvaluate(ecurve, xyz, &(data[0]), xyz_out);
        if (status != EGADS_SUCCESS) goto cleanup;

        xyz[0] = pnt[inode].x;
        xyz[1] = pnt[inode].y;
        xyz[2] = DEPTH(0);

        status = EG_invEvaluate(ecurve, xyz, &(data[1]), xyz_out);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_makeTopology(context, ecurve, EDGE, TWONODE,
                                 data, 2, echild, NULL, &(eedges[jedge]));
        if (status != EGADS_SUCCESS) goto cleanup;

        jedge++;
    }

    /* set up the Faces */
    jface = 0;

    for (iseg = 0; iseg < nseg; iseg++) {
        echild[0] = eedges[iseg];
        echild[1] = eedges[2*nseg+seg[iseg].iend];
        echild[2] = eedges[  nseg+    iseg      ];
        echild[3] = eedges[2*nseg+seg[iseg].ibeg];

        senses[0] = SFORWARD;
        senses[1] = SFORWARD;
        senses[2] = SREVERSE;
        senses[3] = SREVERSE;

        status = EG_makeTopology(context, NULL, LOOP, CLOSED,
                                 NULL, 4, echild, senses, &eloop);
        if (status != EGADS_SUCCESS) goto cleanup;

        status = EG_makeFace(eloop, SFORWARD, NULL, &(efaces[jface]));
        if (status != EGADS_SUCCESS) goto cleanup;

        jseg = iseg + 1;
        status = EG_attributeAdd(efaces[jface], "segment", ATTRINT,
                                 1, &jseg, NULL, NULL);
        if (status != EGADS_SUCCESS) goto cleanup;

        seginfo[0] = seg[iseg].num;
        seginfo[1] = seg[iseg].idx;
        status = EG_attributeAdd(efaces[jface], "waffleseg", ATTRINT,
                                 2, seginfo, NULL, NULL);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (iattr = 0; iattr < seg[iseg].nattr; iattr++) {
            status = EG_attributeAdd(efaces[jface], seg[iseg].aname[iattr], ATTRSTRING,
                                     1, NULL, NULL, seg[iseg].avalu[iattr]);
            if (status != EGADS_SUCCESS) goto cleanup;
        }

        jface++;
    }

    /* make the sheet body */
    status = EG_makeTopology(context, NULL, SHELL, OPEN,
                             NULL, jface, efaces, NULL, &eshell);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_makeTopology(context, NULL, BODY, SHEETBODY,
                             NULL, 1, &eshell, NULL, ebody);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* set the output value(s) */

    /* remember this model (body) */
    udps[numUdp].ebody = *ebody;

cleanup:
    if (efaces != NULL) EG_free(efaces);
    if (eedges != NULL) EG_free(eedges);
    if (enodes != NULL) EG_free(enodes);

    if (pnt != NULL) EG_free(pnt);

    if (seg != NULL) {
        for (iseg = 0; iseg < nseg; iseg++) {
            if (seg[iseg].aname != NULL) {
                for (iattr = 0; iattr < seg[iseg].nattr; iattr++) {
                    if (seg[iseg].aname[iattr] != NULL) EG_free(seg[iseg].aname[iattr]);
                }
                EG_free(seg[iseg].aname);
            }
            if (seg[iseg].avalu != NULL) {
                for (iattr = 0; iattr < seg[iseg].nattr; iattr++) {
                    if (seg[iseg].avalu[iattr] != NULL) EG_free(seg[iseg].avalu[iattr]);
                }
                EG_free(seg[iseg].avalu);
            }
        }
        EG_free(seg);
    }

    if (status < 0) {
        *string = udpErrorStr(status);
    }

    return status;
}


/*
 ************************************************************************
 *                                                                      *
 *   processSegments - segments was specified                           *
 *                                                                      *
 ************************************************************************
 */

static int
processSegments(int    *npnt,           /* (out) number of Points */
                pnt_T  *pnt_p[],        /* (out) array  of Points */
                int    *nseg,           /* (both)number of Segments */
                seg_T  *seg_P[])        /* (out) array  of Segments */
{
    int     status = EGADS_SUCCESS;

    int     mpnt, jpnt, mseg, iseg;
    pnt_T   *pnt=NULL;
    seg_T   *seg=NULL;

    double  EPS06 = 1.0e-6;

    /* get number of Segments from size of SEGMENTS() */
    *npnt = 0;
    *nseg = udps[0].arg[1].size / 4;

    /* default (empty) Point and Segment tables */
    mpnt = *nseg * 2;   /* perhaps too big */
    mseg = *nseg;

    pnt = (pnt_T *) EG_alloc(mpnt*sizeof(pnt_T));
    seg = (seg_T *) EG_alloc(mseg*sizeof(seg_T ));

    if (pnt == NULL || seg == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    /* create Segments and unique Points */
    for (iseg = 0; iseg < *nseg; iseg++) {

        /* check if beginning of Segment matches any Points so far */
        seg[iseg].ibeg = -1;
        for (jpnt = 0; jpnt < *npnt; jpnt++) {
            if (fabs(SEGMENTS(0,4*iseg  )-pnt[jpnt].x) < EPS06 &&
                fabs(SEGMENTS(0,4*iseg+1)-pnt[jpnt].y) < EPS06   ) {
                seg[iseg].ibeg = jpnt;
                break;
            }
        }

        if (seg[iseg].ibeg < 0) {
            seg[iseg].ibeg = *npnt;

            pnt[*npnt].type = 1;
            pnt[*npnt].x    = SEGMENTS(0,4*iseg  );
            pnt[*npnt].y    = SEGMENTS(0,4*iseg+1);
            (*npnt)++;
        }

        /* check if end of Segment matches any Points so far */
        seg[iseg].iend = -1;
        for (jpnt = 0; jpnt < *npnt; jpnt++) {
            if (fabs(SEGMENTS(0,4*iseg+2)-pnt[jpnt].x) < EPS06 &&
                fabs(SEGMENTS(0,4*iseg+3)-pnt[jpnt].y) < EPS06   ) {
                seg[iseg].iend = jpnt;
                break;
            }
        }

        if (seg[iseg].iend < 0) {
            seg[iseg].iend = *npnt;

            pnt[*npnt].type = 1;
            pnt[*npnt].x    = SEGMENTS(0,4*iseg+2);
            pnt[*npnt].y    = SEGMENTS(0,4*iseg+3);
            (*npnt)++;
        }

        seg[iseg].type  = 1;
        seg[iseg].num   = iseg + 1;
        seg[iseg].idx   = 1;
        seg[iseg].nattr = 0;
        seg[iseg].aname = NULL;
        seg[iseg].avalu = NULL;
    }

cleanup:
    /* return pointers to freeable memory */
    *pnt_p = pnt;
    *seg_P = seg;

    return status;
}


/*
 ************************************************************************
 *                                                                      *
 *   processFile - filename was specified                               *
 *                                                                      *
 ************************************************************************
 */

/*
    notes:  keywords can either be lowercase or UPPERCASE (not mixedCase)
            keywords are shown here in UPPERCASE to distinguish them from variables
            segments are only created for LINEs
            segments are broken at intersection of other LINEs and at POINTs

    POINT  pointname AT  xloc           yloc            creates point at <xloc,yloc>
                     AT  x@pointname+dx y@pointname+dy  creates point <dx,dy> from named point
                     AT  xloc           y@pointname     creates point at same y as named point and at given xloc
                     AT  x@pointname    yloc            creates point at same x as named point and at given yloc
                     ON  linename FRAC  fractDist       creates point on line at given fractional distance
                     ON  linename XLOC  xloc            creates point on line at given xloc
                     ON  linename YLOC  yloc            creates point on line at given yloc
                     ON  linename PERP  pointname       creates point on line that is closest to point
                     ON  linename XSECT linename        creates point at intersection of two lines
                     OFF linename dist  pointname       creates point dist to the left of linename at pointname
    CPOINT -------------- same as POINT --------------- creates construction point ...
    LINE   linename  pointname pointname [attrName1=attrValue1 [...]]
                                                        creates line between points with given attributes
    CLINE  -------------- same as LINE ---------------- creates construction line ...
    PATBEG varname ncopy                                loops ncopy times with varname=1,...,ncopy
    PATEND
*/

static int
processFile(ego    context,             /* (in)  EGADS context */
            int    *npnt,               /* (out) number of Points */
            pnt_T  *pnt_p[],            /* (out) array  of Points */
            int    *nseg,               /* (both)number of Segments */
            seg_T  *seg_p[])            /* (out) array  of Segments */
{
    int    status = EGADS_SUCCESS;      /* (out) default return */

    int    nbrch, npmtr, npmtr_save, ipmtr, nbody, type, nrow, ncol, itoken, ichar;
    int    mpnt=0, ipnt, jpnt, mseg=0, iseg, jseg, ibeg, iend, jbeg, jend, itype;
    int    pat_pmtr[ ]={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int    pat_value[]={ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    int    pat_end[  ]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    long   pat_seek[ ]={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int    iskip, npat=0, outLevel;
    double xloc, yloc, xvalue, yvalue, value, dot, frac, D, s, dist, dx, dy, alen;
    pnt_T  *pnt=NULL;
    seg_T  *seg=NULL;
    char   templine[256], token[256], pname1[256], pname2[256], lname1[256], lname2[256];
    char   name[MAX_NAME_LEN], str[256];
    void   *modl, *temp;
    FILE   *fp=NULL;

    double  EPS06 = 1.0e-6;

    ROUTINE(processFile);

    /* --------------------------------------------------------------- */

    /* initially there are no Points or Segments */
    *npnt = 0;
    *nseg = 0;

    /* get pointer to model */
    status = EG_getUserPointer(context, (void**)(&(modl)));
    if (status != EGADS_SUCCESS) {
        printf(" udpExecute: bad return from getUserPointer\n");
        goto cleanup;
    }

    /* get the outLevel from OpenCSM */
    outLevel = ocsmSetOutLevel(       0);
    (void)     ocsmSetOutLevel(outLevel);

    /* make sure that there are no Parameters whose names start with
       x@ or y@ */
    status = ocsmInfo(modl, &nbrch, &npmtr_save, &nbody);
    if (status != EGADS_SUCCESS) goto cleanup;

    for (ipmtr = 1; ipmtr <= npmtr_save; ipmtr++) {
        status = ocsmGetPmtr(modl, ipmtr, &type, &nrow, &ncol, name);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (strncmp(name, "x@", 2) == 0 ||
            strncmp(name, "y@", 2) == 0   ) {
            printf(" udpExecute: cannot start with Parameter named \"%s\"\n", name);
            status = EGADS_NODATA;
            goto cleanup;
        }
    }

    /* open the file */
    fp = fopen(FILENAME(numUdp), "r");
    if (fp == NULL) {
        printf(" udpExecute: could not open \"%s\"\n", FILENAME(numUdp));
        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    /* default (empty) Point and Segment tables */
    mpnt = 10;
    mseg = 10;

    pnt = (pnt_T  *) EG_alloc(mpnt*sizeof(pnt_T ));
    seg = (seg_T  *) EG_alloc(mseg*sizeof(seg_T ));

    if (pnt == NULL || seg == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    /* by default, we are not skipping */
    iskip = 0;

    /* read the file */
    while (!feof(fp)) {
 
        /* read the next line */
        (void) fgets(templine, 255, fp);

        if (feof(fp)) break;

        if (outLevel >= 1) printf("    processing: %s", templine);

        if (templine[0] == '#') continue;

        /* overwite the \n and \r at the end */
        if (STRLEN(templine) > 0 && templine[STRLEN(templine)-1] == '\n') {
            templine[STRLEN(templine)-1] = '\0';
        }
        if (STRLEN(templine) > 0 && templine[STRLEN(templine)-1] == '\r') {
            templine[STRLEN(templine)-1] = '\0';
        }

        /* get and process the first token */
        status = getToken(templine, 0, ' ', 255, token);
        if (status < EGADS_SUCCESS) goto cleanup;

        /* skip blank line */
        if (strcmp(token, "") == 0) {
            continue;

        /* skip comment */
        } else if (strcmp(token, "#") == 0) {

        /* POINT and CPOINT */
        } else if (strcmp(token,  "point") == 0 ||
                   strcmp(token,  "POINT") == 0 ||
                   strcmp(token, "cpoint") == 0 ||
                   strcmp(token, "CPOINT") == 0   ) {

            if (iskip > 0) continue;

            if (strcmp(token, "point") == 0 ||
                strcmp(token, "POINT") == 0   ) {
                itype = 1;
            } else {
                itype = 0;
            }

            /* pname1 */
            status = getToken(templine, 1, ' ', 255, pname1);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = getToken(templine, 2, ' ', 255, token);
            if (status < EGADS_SUCCESS) goto cleanup;

            /* POINT pname1 AT xloc yloc */
            if        (strcmp(token, "at") == 0 ||
                       strcmp(token, "AT") == 0   ) {

                /* get xloc */
                status = getToken(templine, 3, ' ', 255, token);
                if (status < EGADS_SUCCESS) goto cleanup;

                status = ocsmEvalExpr(modl, token, &xloc, &dot, str);
                if (status < EGADS_SUCCESS) goto cleanup;

                if (STRLEN(str) > 0) {
                    printf(" udpExecute: xvalue must be a number\n");
                    status = EGADS_NODATA;
                    goto cleanup;
                }

                /* get yloc */
                status = getToken(templine, 4, ' ', 255, token);
                if (status < EGADS_SUCCESS) goto cleanup;

                status = ocsmEvalExpr(modl, token, &yloc, &dot, str);
                if (status < EGADS_SUCCESS) goto cleanup;

                if (STRLEN(str) > 0) {
                    printf(" udpExecute: yvalue must be a number\n");
                    status = EGADS_NODATA;
                    goto cleanup;
                }

                /* compute (xvalue,yvalue) */
                xvalue = xloc;
                yvalue = yloc;

            /* POINT pname1 ON lname1 ... */
            } else if (strcmp(token, "on") == 0 ||
                       strcmp(token, "ON") == 0   ) {

                /* find lname1 */
                status = getToken(templine, 3, ' ', 255, lname1);
                if (status < EGADS_SUCCESS) goto cleanup;

                iseg = -1;
                for (jseg = 0; jseg < *nseg; jseg++) {
                    if (strcmp(lname1, seg[jseg].name) == 0) {
                        iseg = jseg;
                    }
                }
                if (iseg < 0) {
                    printf(" udpExecute: line \"%s\" could not be found\n", lname1);
                    status = EGADS_NOTFOUND;
                    goto cleanup;
                }

                ibeg = seg[iseg].ibeg;
                iend = seg[iseg].iend;

                /* determine the sub-operator */
                status = getToken(templine, 4, ' ', 255, token);
                if (status < EGADS_SUCCESS) goto cleanup;

                /* POINT pname1 ON lname1 FRAC fracDist */
                if        (strcmp(token, "frac") == 0 ||
                           strcmp(token, "FRAC") == 0   ) {

                    /* get fracDist */
                    status = getToken(templine, 5, ' ', 255, token);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    status = ocsmEvalExpr(modl, token, &frac, &dot, str);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    /* compute (xvalue,yvalue) */
                    xvalue = (1-frac) * pnt[ibeg].x + frac * pnt[iend].x;
                    yvalue = (1-frac) * pnt[ibeg].y + frac * pnt[iend].y;

                /* POINT pname1 ON lname1 XLOC xloc */
                } else if (strcmp(token, "xloc") == 0 ||
                           strcmp(token, "XLOC") == 0   ) {

                    if(fabs(pnt[ibeg].x-pnt[iend].x) < EPS06) {
                        printf(" udpExecute: cannot specify XLOC on a constant X line\n");
                        status = EGADS_RANGERR;
                        goto cleanup;
                    }

                    /* get xloc */
                    status = getToken(templine, 5, ' ', 255, token);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    status = ocsmEvalExpr(modl, token, &xloc, &dot, str);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    /* compute (xvalue,yvalue) */
                    frac   = (xloc - pnt[ibeg].x) / (pnt[iend].x - pnt[ibeg].x);
                    xvalue = xloc;
                    yvalue = (1-frac) * pnt[ibeg].y + frac * pnt[iend].y;

                /* POINT pname1 ON lname1 YLOC yloc */
                } else if (strcmp(token, "yloc") == 0 ||
                           strcmp(token, "YLOC") == 0   ) {

                    if(fabs(pnt[ibeg].y-pnt[iend].y) < EPS06) {
                        printf(" udpExecute: cannot specify YLOC on a constant Y line\n");
                        status = EGADS_RANGERR;
                        goto cleanup;
                    }

                    /* get yloc */
                    status = getToken(templine, 5, ' ', 255, token);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    status = ocsmEvalExpr(modl, token, &yloc, &dot, str);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    /* compute (xvalue,yvalue) */
                    frac   = (yloc - pnt[ibeg].y) / (pnt[iend].y - pnt[ibeg].y);
                    xvalue = (1-frac) * pnt[ibeg].x + frac * pnt[iend].x;
                    yvalue = yloc;

                /* POINT pname1 ON lname1 PERP pname2 */
                } else if (strcmp(token, "perp") == 0 ||
                           strcmp(token, "PERP") == 0   ) {

                    /* get pname2 */
                    status = getToken(templine, 5, ' ', 255, pname2);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    ipnt = -1;
                    for (jpnt = 0; jpnt < *npnt; jpnt++) {
                        if (strcmp(pname2, pnt[jpnt].name) == 0) {
                            ipnt = jpnt;
                        }
                    }
                    if (ipnt < 0) {
                        printf(" udpExecute: point \"%s\" could not be found\n", pname2);
                        status = EGADS_NOTFOUND;
                        goto cleanup;
                    }

                    /* compute (xvalue,yvalue) */
                    frac   = ( (pnt[ipnt].x - pnt[ibeg].x) * (pnt[iend].x - pnt[ibeg].x)
                             + (pnt[ipnt].y - pnt[ibeg].y) * (pnt[iend].y - pnt[ibeg].y))
                           / ( (pnt[iend].x - pnt[ibeg].x) * (pnt[iend].x - pnt[ibeg].x)
                             + (pnt[iend].y - pnt[ibeg].y) * (pnt[iend].y - pnt[ibeg].y));
                    xvalue = (1-frac) * pnt[ibeg].x + frac * pnt[iend].x;
                    yvalue = (1-frac) * pnt[ibeg].y + frac * pnt[iend].y;

                /* POINT pname1 ON lname1 XSECT lname2 */
                } else if (strcmp(token, "xsect") == 0 ||
                           strcmp(token, "XSECT") == 0   ) {

                    /* find lname2 */
                    status = getToken(templine, 5, ' ', 255, lname2);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    iseg = -1;
                    for (jseg = 0; jseg < *nseg; jseg++) {
                        if (strcmp(lname2, seg[jseg].name) == 0) {
                            iseg = jseg;
                        }
                    }
                    if (iseg < 0) {
                        printf(" udpExecute: line \"%s\" could not be found\n", lname2);
                        status = EGADS_NOTFOUND;
                        goto cleanup;
                    }

                    jbeg = seg[iseg].ibeg;
                    jend = seg[iseg].iend;

                    /* compute (xvalue,yvalue) */
                    D = (pnt[iend].x - pnt[ibeg].x) * (pnt[jbeg].y - pnt[jend].y) - (pnt[jbeg].x - pnt[jend].x) * (pnt[iend].y - pnt[ibeg].y);
                    if (fabs(D) > EPS06) {
                        s = ((pnt[jbeg].x - pnt[ibeg].x) * (pnt[jbeg].y - pnt[jend].y) - (pnt[jbeg].x - pnt[jend].x) * (pnt[jbeg].y - pnt[ibeg].y)) / D;
//                        t = ((pnt[iend].x - pnt[ibeg].x) * (pnt[jbeg].y - pnt[ibeg].y) - (pnt[jbeg].x - pnt[ibeg].x) * (pnt[iend].y - pnt[ibeg].y)) / D;

                        xvalue = (1 - s) * pnt[ibeg].x + s * pnt[iend].x;
                        yvalue = (1 - s) * pnt[ibeg].y + s * pnt[iend].y;
                    } else {
                        printf(" udpExecute: segments do not intersect\n");
                        status = EGADS_NOTFOUND;
                        goto cleanup;
                    }

                } else {
                    printf(" udpExecute: fifth token should be FRAC, PERP, XLOC, YLOC, SAMEX, SAMEY, or XSECT\n");
                    status = EGADS_RANGERR;
                    goto cleanup;
                }

            /* POINT pname1 OFF lname1 dist pname2 */
            } else if (strcmp(token, "off") == 0 ||
                       strcmp(token, "OFF") == 0   ) {

                /* find lname1 */
                status = getToken(templine, 3, ' ', 255, lname1);
                if (status < EGADS_SUCCESS) goto cleanup;

                iseg = -1;
                for (jseg = 0; jseg < *nseg; jseg++) {
                    if (strcmp(lname1, seg[jseg].name) == 0) {
                        iseg = jseg;
                    }
                }
                if (iseg < 0) {
                    printf(" udpExecute: line \"%s\" could not be found\n", lname1);
                    status = EGADS_NOTFOUND;
                    goto cleanup;
                }

                ibeg = seg[iseg].ibeg;
                iend = seg[iseg].iend;

                /* get dist */
                status = getToken(templine, 4, ' ', 255, token);
                if (status < EGADS_SUCCESS) goto cleanup;

                status = ocsmEvalExpr(modl, token, &dist, &dot, str);
                if (status < EGADS_SUCCESS) goto cleanup;

                /* get pname2 */
                status = getToken(templine, 5, ' ', 255, pname2);
                if (status < EGADS_SUCCESS) goto cleanup;

                ipnt = -1;
                for (jpnt = 0; jpnt < *npnt; jpnt++) {
                    if (strcmp(pname2, pnt[jpnt].name) == 0) {
                        ipnt = jpnt;
                    }
                }
                if (ipnt < 0) {
                    printf(" udpExecute: point \"%s\" could not be found\n", pname2);
                    status = EGADS_NOTFOUND;
                    goto cleanup;
                }

                /* compute (xvalue,yvalue) */
                dx   = pnt[iend].x - pnt[ibeg].x;
                dy   = pnt[iend].y - pnt[ibeg].y;
                alen = sqrt(dx * dx + dy * dy);

                xvalue = pnt[ipnt].x - dist * dy / alen;
                yvalue = pnt[ipnt].y + dist * dx / alen;

            } else {
                printf(" udpExecute: third token should be AT, ON, or OFF\n");
                status = EGADS_RANGERR;
                goto cleanup;
            }

            /* make room for (possible) new Point */
            if (*npnt+1 >= mpnt) {
                mpnt += 10;

                temp = EG_reall(pnt, mpnt*sizeof(pnt_T));
                if (temp != NULL) {
                    pnt = (pnt_T *) temp;
                } else {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }
            }

            /* if another Point has same name, remove the name */
            for (jpnt = 0; jpnt < *npnt; jpnt++) {
                if (strcmp(pnt[jpnt].name, pname1) == 0) {
                    strcpy(pnt[jpnt].name, "");
                    break;
                }
            }

            /* see if Point already exists at these coordintes  */
            ipnt = -1;
            for (jpnt = 0; jpnt < *npnt; jpnt++) {
                if (fabs(xvalue-pnt[jpnt].x) < EPS06 &&
                    fabs(yvalue-pnt[jpnt].y) < EPS06   ) {
                    ipnt = jpnt;
                    break;
                }
            }

            /* if Point already exists, just update its name */
            if (ipnt >= 0) {
                strcpy(pnt[ipnt].name, pname1);

                /* otherwise, create the new Point */
            } else {
                strcpy(pnt[*npnt].name, pname1);

                pnt[*npnt].type = itype;
                pnt[*npnt].x    = xvalue;
                pnt[*npnt].y    = yvalue;
                (*npnt)++;

            }

            /* store coordinates in a local variable */
            (void) strcpy(str, "x@");
            (void) strcat(str, pname1);

            status = ocsmFindPmtr(modl, str, OCSM_INTERNAL, 1, 1, &ipmtr);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = ocsmSetValuD(modl, ipmtr, 1, 1, xvalue);
            if (status < EGADS_SUCCESS) goto cleanup;

            (void) strcpy(str, "y@");
            (void) strcat(str, pname1);

            status = ocsmFindPmtr(modl, str, OCSM_INTERNAL, 1, 1, &ipmtr);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = ocsmSetValuD(modl, ipmtr, 1, 1, yvalue);
            if (status < EGADS_SUCCESS) goto cleanup;

        } else if (strcmp(token,  "line") == 0 ||
                   strcmp(token,  "LINE") == 0 ||
                   strcmp(token, "cline") == 0 ||
                   strcmp(token, "CLINE") == 0 ||
                   strcmp(token,  "path") == 0 ||       // for backward compatability
                   strcmp(token,  "PATH") == 0   ) {    // for backward compatability

            if (iskip > 0) continue;

            status = getToken(templine, 1, ' ', 255, lname1);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = getToken(templine, 2, ' ', 255, pname1);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = getToken(templine, 3, ' ', 255, pname2);
            if (status < EGADS_SUCCESS) goto cleanup;

            /* make room in the Segment table */
            if (*nseg+1 >= mseg) {
                mseg += 10;

                temp = EG_reall(seg, mseg*sizeof(seg_T));
                if (temp != NULL) {
                    seg = (seg_T *) temp;
                } else {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }
            }

            /* set the Segment type (0  for cline, 1 for line) */
            if (strcmp(token, "line") == 0 ||
                strcmp(token, "LINE") == 0   ) {
                seg[*nseg].type = 1;
            } else {
                seg[*nseg].type = 0;
            }

            /* use last Point named pname1 (which takes care of redefinitions) */
            seg[*nseg].ibeg = -1;
            for (ipnt = *npnt-1; ipnt >= 0; ipnt--) {
                if (strcmp(pname1, pnt[ipnt].name) == 0) {
                    seg[*nseg].ibeg = ipnt;
                    break;
                }
            }
            if (seg[*nseg].ibeg < 0) {
                printf(" udpExecute: \"%s\" not found\n", pname1);
                status = EGADS_NODATA;
                goto cleanup;
            }

            /* use last Point named pname2 (which takes care of redefinitions) */
            seg[*nseg].iend = -1;
            for (ipnt = *npnt-1; ipnt >= 0; ipnt--) {
                if (strcmp(pname2, pnt[ipnt].name) == 0) {
                    seg[*nseg].iend = ipnt;
                    break;
                }
            }
            if (seg[*nseg].iend < 0) {
                printf(" udpExecute: \"%s\" not found\n", pname2);
                status = EGADS_NODATA;
                goto cleanup;
            }

            /* create Segment */
            seg[*nseg].num     = *nseg + 1;
            seg[*nseg].idx     = 1;
            seg[*nseg].name[0] = '\0';
            seg[*nseg].nattr   = 0;
            seg[*nseg].aname   = NULL;
            seg[*nseg].avalu   = NULL;

            strncpy(seg[*nseg].name, lname1, 80);

            /* process Attribute pairs */
            for (itoken = 4; itoken < 100; itoken++) {
                status = getToken(templine, itoken, ' ', 255, token);
                if (status < EGADS_SUCCESS) goto cleanup;

                if (STRLEN(token) == 0) break;

                if (strstr(token, "=") == NULL) {
                    printf(" udpExecute: attribute pair must contain = sign\n");
                    status = EGADS_RANGERR;
                    goto cleanup;
                } else {
                    if (seg[*nseg].nattr == 0) {
                        seg[*nseg].aname = (char **) EG_alloc(sizeof(char*));
                        seg[*nseg].avalu = (char **) EG_alloc(sizeof(char*));

                        if (seg[*nseg].aname == NULL || seg[*nseg].avalu == NULL) {
                            status = EGADS_MALLOC;
                            goto cleanup;
                        }
                    } else {
                        temp = (char **) EG_reall(seg[*nseg].aname, (seg[*nseg].nattr+1)*sizeof(char*));
                        if (temp != NULL) {
                            seg[*nseg].aname = (char **) temp;
                        } else {
                            status = EGADS_MALLOC;
                            goto cleanup;
                        }

                        temp = (char **) EG_reall(seg[*nseg].avalu, (seg[*nseg].nattr+1)*sizeof(char*));
                        if (temp != NULL) {
                            seg[*nseg].avalu = (char **) temp;
                        } else {
                            status = EGADS_MALLOC;
                            goto cleanup;
                        }
                    }

                    seg[*nseg].aname[seg[*nseg].nattr] = EG_alloc(80*sizeof(char));
                    seg[*nseg].avalu[seg[*nseg].nattr] = EG_alloc(80*sizeof(char));

                    if (seg[*nseg].aname[seg[*nseg].nattr] == NULL ||
                        seg[*nseg].avalu[seg[*nseg].nattr] == NULL   ) {
                        status = EGADS_MALLOC;
                        goto cleanup;
                    }

                    for (ichar = 0; ichar < STRLEN(token); ichar++) {
                        if (token[ichar] == '=') {
                            token[ichar] = '\0';
                            break;
                        }
                    }

                    strncpy(seg[*nseg].aname[seg[*nseg].nattr], token, 80);

                    token[ichar] = '$';
                    status = ocsmEvalExpr(modl, &(token[ichar]), &value, &dot, str);
                    if (status < EGADS_SUCCESS) goto cleanup;

                    if (STRLEN(str) == 0) {
                        printf(" udpExecute: attribute value must be a string\n");
                        status = EGADS_NODATA;
                        goto cleanup;
                    }

                    strncpy(seg[*nseg].avalu[seg[*nseg].nattr], str, 80);

                    (seg[*nseg].nattr)++;
                }
            }

            (*nseg)++;

        } else if (strcmp(token, "patbeg") == 0 ||
                   strcmp(token, "PATBEG") == 0   ) {

            if (npat < 9) {
                npat++;
            } else {
                printf(" udpExecute: PATBEGs nested too deeply\n");
                status = EGADS_RANGERR;
                goto cleanup;
            }

            if (iskip > 0) {
                iskip++;
                continue;
            }

            /* remember where we in the file are so that we can get back here */
            pat_seek[npat] = ftell(fp);

            /* get the number of replicates */
            status = getToken(templine, 2, ' ', 255, token);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = ocsmEvalExpr(modl, token, &value, &dot, str);
            if (status < EGADS_SUCCESS) goto cleanup;

            pat_end[npat] = NINT(value);

            if (pat_end[npat] <= 0) {
                iskip++;
                continue;
            } else {
                pat_value[npat] = 1;
            }

            /* set up Parameter to hold pattern index */
            status = getToken(templine, 1, ' ', 255, token);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = ocsmFindPmtr(modl, token, OCSM_INTERNAL, 1, 1, &pat_pmtr[npat]);
            if (status < EGADS_SUCCESS) goto cleanup;

            if (pat_pmtr[npat] <= npmtr_save) {
                printf(" udpExecute: cannot use \"%s\" as pattern variable since it was previously defined\n", token);
                status = EGADS_NONAME;
                goto cleanup;
            }
            
            status = ocsmSetValuD(modl, pat_pmtr[npat], 1, 1, (double)pat_value[npat]);
            if (status < EGADS_SUCCESS) goto cleanup;

        } else if (strcmp(token, "patend") == 0 ||
                   strcmp(token, "PATEND") == 0   ) {

            if (pat_end[npat] < 0) {
                printf(" udpExecute: PATEND without PATBEG\n");
                status = EGADS_RANGERR;
                goto cleanup;
            }

            if (iskip > 0) {
                iskip--;
                continue;
            }

            /* for another replicate, increment the value and place file pointer
               just after the PATBEG statement */
            if (pat_value[npat] < pat_end[npat]) {
                (pat_value[npat])++;

                status = ocsmSetValuD(modl, pat_pmtr[npat], 1, 1, (double)pat_value[npat]);
                if (status < EGADS_SUCCESS) goto cleanup;

                if (pat_seek[npat] != 0) {
                    fseek(fp, pat_seek[npat], SEEK_SET);
                }

                /* otherwise, reset the pattern variables */
            } else {

                pat_pmtr[npat] = -1;
                pat_end[ npat] = -1;

                npat--;
            }

        } else {
            printf(" udpExecute: input should start with POINT, LINE, CLINE, PATBEG, or PATEND\n");
            status = EGADS_RANGERR;
            goto cleanup;
        }

        /* show Points and Segments after processing this statement */
        if (PROGRESS(numUdp) != 0) {
            for (ipnt = 0; ipnt < *npnt; ipnt++) {
                printf("        Pnt %3d: %-20s %1d %10.5f %10.5f\n", ipnt,
                       pnt[ipnt].name, pnt[ipnt].type, pnt[ipnt].x, pnt[ipnt].y);
            }
            for (iseg = 0; iseg < *nseg; iseg++) {
                printf("        Seg %3d: %-20s %1d %5d %5d\n", iseg,
                       seg[iseg].name, seg[iseg].type, seg[iseg].ibeg, seg[iseg].iend);
            }
        }

#ifdef GRAFIC
        status = plotWaffle(*npnt, pnt, *nseg, seg);
        if (status != EGADS_SUCCESS) goto cleanup;
#endif
    }

    /* delete any Parameters that were added */
    status = ocsmInfo(modl, &nbrch, &npmtr, &nbody);
    if (status < EGADS_SUCCESS) goto cleanup;

    for (ipmtr = npmtr; ipmtr > npmtr_save; ipmtr--) {
        status = ocsmDelPmtr(modl, ipmtr);
        if (status < EGADS_SUCCESS) goto cleanup;
    }

cleanup:

    /* return pointers to freeable memory */
    *pnt_p = pnt;
    *seg_p = seg;

    /* close the file */
    if (fp != NULL) fclose(fp);

    return status;
}


/*
 ************************************************************************
 *                                                                      *
 *   getToken - get a token from a string                               *
 *                                                                      *
 ************************************************************************
 */

static int
getToken(char   *text,                  /* (in)  full text */
         int    nskip,                  /* (in)  tokens to skip */
         char   sep,                    /* (in)  seperator character */
         int    maxtok,                 /* (in)  size of token */
         char   *token)                 /* (out) token */
{
    int     lentok, ibeg, i, j, count, iskip;

    /* --------------------------------------------------------------- */

    token[0] = '\0';
    lentok   = 0;

    /* convert tabs to spaces */
    for (i = 0; i < STRLEN(text); i++) {
        if (text[i] == '\t') {
            text[i] = ' ';
        }
    }

    /* skip past white spaces at beginning of text */
    ibeg = 0;
    while (ibeg < STRLEN(text)) {
        if (text[ibeg] != ' ' && text[ibeg] != '\t' && text[ibeg] != '\r') break;
        ibeg++;
    }

    /* count the number of separators */
    count = 0;
    for (i = ibeg; i < STRLEN(text); i++) {
        if (text[i] == sep) {
            count++;
            for (j = i+1; j < STRLEN(text); j++) {
                if (text[j] != sep) break;
                i++;
            }
        }
    }

    if (count < nskip) return 0;

    /* skip over nskip tokens */
    i = ibeg;
    for (iskip = 0; iskip < nskip; iskip++) {
        while (text[i] != sep) {
            i++;
        }
        while (text[i] == sep) {
            i++;
        }
    }

    /* extract the token we are looking for */
    while (text[i] != sep && i < STRLEN(text)) {
        token[lentok++] = text[i++];
        token[lentok  ] = '\0';

        if (lentok >= maxtok-1) {
            printf("ERROR:: token exceeds maxtok=%d\n", maxtok);
            break;
        }
    }

    return STRLEN(token);
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
 *   plotWaffle - plot Points and Segments                              *
 *                                                                      *
 ************************************************************************
 */

#ifdef GRAFIC
static int
plotWaffle(int    npnt,                 /* (in)  number of Points */
           pnt_T  pnt[],                /* (in)  array  of Points */
           int    nseg,                 /* (in)  number of Segments */
           seg_T  seg[])                /* (in)  array  of Segments */
{
    int    status = EGADS_SUCCESS;

    int    io_kbd=5, io_scr=6, nline=0, nplot=0;
    int    indgr=1+2+4+16+64, ipnt, iseg;
    int    *ilin=NULL, *isym=NULL, *nper=NULL;
    float  *xplot=NULL, *yplot=NULL;

    /* --------------------------------------------------------------- */

    if (npnt == 0 && nseg == 0) return status;

    xplot = (float *) EG_alloc((2*nseg+npnt)*sizeof(float));
    yplot = (float *) EG_alloc((2*nseg+npnt)*sizeof(float));
    ilin  = (int   *) EG_alloc((  nseg+npnt)*sizeof(int  ));
    isym  = (int   *) EG_alloc((  nseg+npnt)*sizeof(int  ));
    nper  = (int   *) EG_alloc((  nseg+npnt)*sizeof(int  ));

    if (xplot == NULL || yplot == NULL ||
        ilin  == NULL || isym  == NULL || nper == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    nplot = 0;
    nline = 0;

    /* build plot arrays for Points */
    for (ipnt = 0; ipnt < npnt; ipnt++) {
        xplot[nplot] = pnt[ipnt].x;
        yplot[nplot] = pnt[ipnt].y;
        nplot++;

        if (pnt[ipnt].type == 0) {
            ilin[nline] = 0;
            isym[nline] = GR_X;
            nper[nline] = 1;
        } else {
            ilin[nline] = 0;
            isym[nline] = GR_CIRCLE;
            nper[nline] = 1;
        }
        nline++;
    }

    /* build plot arrays for Segments */
    for (iseg = 0; iseg < nseg; iseg++) {
        xplot[nplot] = pnt[seg[iseg].ibeg].x;
        yplot[nplot] = pnt[seg[iseg].ibeg].y;
        nplot++;

        xplot[nplot] = pnt[seg[iseg].iend].x;
        yplot[nplot] = pnt[seg[iseg].iend].y;
        nplot++;

        if (seg[iseg].type == 0) {
            ilin[nline] = GR_DOTTED;
            isym[nline] = 0;
            nper[nline] = 2;
        } else {
            ilin[nline] = GR_SOLID;
            isym[nline] = 0;
            nper[nline] = 2;
        }
        nline++;
    }

    /* generate plot */
    grinit_(&io_kbd, &io_scr, "udpWaffle", strlen("udpWaffle"));

    grline_(ilin, isym, &nline,                "~x~y~ ",
            &indgr, xplot, yplot, nper, strlen("~x~y~ "));

cleanup:
    if (xplot != NULL) EG_free(xplot);
    if (yplot != NULL) EG_free(yplot);
    if (ilin  != NULL) EG_free(ilin );
    if (isym  != NULL) EG_free(isym );
    if (nper  != NULL) EG_free(nper );

    return status;
}
#endif
