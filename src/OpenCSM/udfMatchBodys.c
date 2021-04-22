/*
 ************************************************************************
 *                                                                      *
 * udfMatchBodys -- finds matching Nodes, Edges, and Faces              *
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

#define NUMUDPINPUTBODYS 2
#define NUMUDPARGS       4

/* set up the necessary structures (uses NUMUDPARGS) */
#include "udpUtilities.h"

/* shorthands for accessing argument values and velocities */
#define TOLER( IUDP)     ((double *) (udps[IUDP].arg[0].val))[0]
#define NNODES(IUDP)     ((int    *) (udps[IUDP].arg[1].val))[0]
#define NEDGES(IUDP)     ((int    *) (udps[IUDP].arg[2].val))[0]
#define NFACES(IUDP)     ((int    *) (udps[IUDP].arg[3].val))[0]

static char  *argNames[NUMUDPARGS] = {"toler",  "nnodes", "nedges", "nfaces", };
static int    argTypes[NUMUDPARGS] = {ATTRREAL, -ATTRINT, -ATTRINT, -ATTRINT, };
static int    argIdefs[NUMUDPARGS] = {0,        0,        0,        0,        };
static double argDdefs[NUMUDPARGS] = {1e-6,     0.,       0.,       0.,       };

/* get utility routines: udpErrorStr, udpInitialize, udpReset, udpSet,
                         udpGet, udpVel, udpClean, udpMesh */
#include "udpUtilities.c"

#include "OpenCSM.h"

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
    int     status = EGADS_SUCCESS;

    ego     context, *ebodys;

    int    oclass, mtype, inode1, inode2, nnode1, nnode2, nchild, *senses;
    int    nmatch, i, *matches=NULL, *list1=NULL, *list2=NULL;
    double data[18], xyz1[4], xyz2[4];
    ego    *enode1, *enode2, eref, *echilds;

#ifdef DEBUG
    printf("udpExecute(emodel=%llx)\n", (long long)emodel);
    printf("toler(0) = %f\n", TOLER(0));
#endif

    /* default return values */
    *ebody  = NULL;
    *nMesh  = 0;
    *string = NULL;

    /* check/process arguments */
    if (udps[0].arg[0].size > 1) {
        printf(" udpExecute: toler should be a scalar\n");
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (TOLER(0) < 0) {
        printf(" udpExecute: toler = %f < 0\n", TOLER(0));
        status  =  EGADS_RANGERR;
        goto cleanup;
    }

    /* check that Model was input that contains two Bodys */
    status = EG_getTopology(emodel, &eref, &oclass, &mtype,
                            data, &nchild, &ebodys, &senses);
    if (status < EGADS_SUCCESS) goto cleanup;

    if (oclass != MODEL) {
        printf(" udpExecute: expecting a Model\n");
        status = EGADS_NOTMODEL;
        goto cleanup;
    } else if (nchild != 2) {
        printf(" udpExecute: expecting Model to contain one Body (not %d)\n", nchild);
        status = EGADS_NOTBODY;
        goto cleanup;
    }

#ifdef DEBUG
    printf("emodel\n");
    ocsmPrintEgo(emodel);
#endif

    /* default output value(s) */
    NNODES(0) = 0;
    NEDGES(0) = 0;
    NFACES(0) = 0;

    /* cache copy of arguments for future use */
    status = cacheUdp();
    if (status < 0) {
        printf(" udpExecute: problem caching arguments\n");
        goto cleanup;
    }

#ifdef DEBUG
    printf("toler(%d) = %f\n", numUdp, TOLER(numUdp));
#endif

    status = EG_getContext(emodel, &context);
    if (status < EGADS_SUCCESS) goto cleanup;

    /* get a list of the Nodes in each Body */
    status = EG_getBodyTopos(ebodys[0], NULL, NODE, &nnode1, &enode1);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_getBodyTopos(ebodys[1], NULL, NODE, &nnode2, &enode2);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make an array to hold the possible Node matches */
    list1 = (int *) malloc(nnode1*sizeof(int));
    list2 = (int *) malloc(nnode2*sizeof(int));
    if (list1 == NULL || list2 == NULL) {
        status = EGADS_MALLOC;
        goto cleanup;
    }

    /* find Node matches */
    nmatch = 0;

    for (inode1 = 0; inode1 < nnode1; inode1++) {
        status = EG_getTopology(enode1[inode1], &eref, &oclass, &mtype,
                                xyz1, &nchild, &echilds, &senses);
        if (status != EGADS_SUCCESS) goto cleanup;

        for (inode2 = 0; inode2 < nnode2; inode2++) {
            status = EG_getTopology(enode2[inode2], &eref, &oclass, &mtype,
                                    xyz2, &nchild, &echilds, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;

            if (fabs(xyz1[0]-xyz2[0]) < TOLER(0) &&
                fabs(xyz1[1]-xyz2[1]) < TOLER(0) &&
                fabs(xyz1[2]-xyz2[2]) < TOLER(0)   ) {
                list1[nmatch] = inode1 + 1;
                list2[nmatch] = inode2 + 1;
                nmatch++;
            }
        }
    }

    EG_free(enode1);
    EG_free(enode2);

    /* add Attributes to the two Bodys that identify the matches */
    if (nmatch > 0) {
        NNODES(0) = nmatch;

        status = EG_attributeAdd(ebodys[0], "_nodeMatches_", ATTRINT,
                                 nmatch, list1, NULL, NULL);
        if (status != EGADS_SUCCESS) goto cleanup;
    
        status = EG_attributeAdd(ebodys[1], "_nodeMatches_", ATTRINT,
                                 nmatch, list2, NULL, NULL);
        if (status != EGADS_SUCCESS) goto cleanup;
    }
    
    if (list1 != NULL) {free(list1);  list1 = NULL;}
    if (list2 != NULL) {free(list2);  list2 = NULL;}

    /* find the Edge matches */
    if (nmatch > 0) {
        status = EG_matchBodyEdges(ebodys[0], ebodys[1], TOLER(0), &nmatch, &matches);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (nmatch > 0) {
            list1 = (int *) malloc(nmatch*sizeof(int));
            list2 = (int *) malloc(nmatch*sizeof(int));
            if (list1 == NULL || list2 == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            for (i = 0; i < nmatch; i++) {
                list1[i] = matches[2*i  ];
                list2[i] = matches[2*i+1];
            }

            NEDGES(0) = nmatch;

            status = EG_attributeAdd(ebodys[0], "_edgeMatches_", ATTRINT,
                                     nmatch, list1, NULL, NULL);
            if (status != EGADS_SUCCESS) goto cleanup;
    
            status = EG_attributeAdd(ebodys[1], "_edgeMatches_", ATTRINT,
                                     nmatch, list2, NULL, NULL);
            if (status != EGADS_SUCCESS) goto cleanup;

            EG_free(matches);
            if (list1 != NULL) {free(list1);  list1 = NULL;}
            if (list2 != NULL) {free(list2);  list2 = NULL;}
        }
    }

    /* find the Face matches */
    if (nmatch > 0) {
        status = EG_matchBodyFaces(ebodys[0], ebodys[1], TOLER(0), &nmatch, &matches);
        if (status != EGADS_SUCCESS) goto cleanup;

        if (nmatch > 0) {
            list1 = (int *) malloc(nmatch*sizeof(int));
            list2 = (int *) malloc(nmatch*sizeof(int));
            if (list1 == NULL || list2 == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            for (i = 0; i < nmatch; i++) {
                list1[i] = matches[2*i  ];
                list2[i] = matches[2*i+1];
            }

            NFACES(0) = nmatch;

            status = EG_attributeAdd(ebodys[0], "_faceMatches_", ATTRINT,
                                     nmatch, list1, NULL, NULL);
            if (status != EGADS_SUCCESS) goto cleanup;
    
            status = EG_attributeAdd(ebodys[1], "_faceMatches_", ATTRINT,
                                     nmatch, list2, NULL, NULL);
            if (status != EGADS_SUCCESS) goto cleanup;

            EG_free(matches);
            if (list1 != NULL) {free(list1);  list1 = NULL;}
            if (list2 != NULL) {free(list2);  list2 = NULL;}
        }
    }

    /* return the modfied Model that contains the two input Bodys */
    *ebody = emodel;

#ifdef DEBUG
    printf("*ebody\n");
    (void) ocsmPrintEgo(*ebody);
#endif

    /* return output value(s) --- done above */

    /* remember this model (Body) */
    udps[numUdp].ebody = *ebody;
    goto cleanup;

#ifdef DEBUG
    printf("udpExecute -> *ebody=%llx\n", (long long)(*ebody));
#endif

cleanup:
    if (list1 != NULL) free(list1);
    if (list2 != NULL) free(list2);

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
