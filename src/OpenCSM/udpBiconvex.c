/*
 ************************************************************************
 *                                                                      *
 * udpBiconvex -- biconvex airfoil between (0,0) and (1,0)              *
 *                                                                      *
 *            Written by John Dannenhoffer@ Syracuse University         *
 *                                                                      *
 ************************************************************************
 */

#define UDP    1
#define EPS06  1.0e-6

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

#define NUMUDPARGS 2
#include "udpUtilities.h"

/* shorthands for accessing argument values and velocities */
#ifdef UDP
    #define THICK(     IUDP)  ((double *) (udps[IUDP].arg[0].val))[0]
    #define THICK_DOT( IUDP)  ((double *) (udps[IUDP].arg[0].dot))[0]
    #define CAMBER(    IUDP)  ((double *) (udps[IUDP].arg[1].val))[0]
    #define CAMBER_DOT(IUDP)  ((double *) (udps[IUDP].arg[1].dot))[0]

    /* data about possible arguments */
    static char  *argNames[NUMUDPARGS] = {"thick",     "camber",    };
    static int    argTypes[NUMUDPARGS] = {ATTRREALSEN, ATTRREALSEN, };
    static int    argIdefs[NUMUDPARGS] = {0,           0,           };
    static double argDdefs[NUMUDPARGS] = {0.,          0.,          };

    /* get utility routines: udpErrorStr, udpInitialize, udpReset, udpSet,
                             udpGet, udpVel, udpClean, udpMesh */
    #include "udpUtilities.c"
#else
    #define THICK(     IUDP)  0.12
    #define THICK_DOT( IUDP)  0.00
    #define CAMBER(    IUDP)  0.04
    #define CAMBER_DOT(IUDP)  0.00

/*@null@*/ char *
udpErrorStr(int stat)                   /* (in)  status number */
{
    char *string;                       /* (out) error message */

    string = EG_alloc(25*sizeof(char));
    if (string == NULL) {
        return string;
    }
    snprintf(string, 25, "EGADS status = %d", stat);

    return string;
}

int
main(int       argc,                    /* (in)  number of arguments */
     char      *argv[])                 /* (in)  array of arguments */
{
    int  status, nMesh;
    char *string;
    ego  context, ebody, emodel;

    /* dummy call to prevent compiler warnings */
    ocsmPrintEgo(NULL);

    /* define a context */
    status = EG_open(&context);
    printf("EG_open -> status=%d\n", status);
    if (status < 0) exit(1);

    /* call the execute routine */
    status = udpExecute(context, &ebody, &nMesh, &string);
    printf("udpExecute -> status=%d\n", status);
    if (status < 0) exit(1);

    EG_free(string);

    /* make and dump the model */
    status = EG_makeTopology(context, NULL, MODEL, 0, NULL,
                             1, &ebody, NULL, &emodel);
    printf("EG_makeTopology -> status=%d\n", status);
    if (status < 0) exit(1);

    status = EG_saveModel(emodel, "biconvex.egads");
    printf("EG_saveModel -> status=%d\n", status);
    if (status < 0) exit(1);

    /* cleanup */
    status = EG_deleteObject(emodel);
    printf("EG_close -> status=%d\n", status);

    status = EG_close(context);
    printf("EG_close -> status=%d\n", status);
}
#endif


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
    int     sense[2];
    double  L, Hup, Rup, Hlo, Rlo, node1[3], node2[3], circ[10], trange[2], data[18];
    ego     enodes[3], ecurves[2], eedges[2], eloop, eface;

#ifdef DEBUG
    printf("udpExecute(context=%llx)\n", (long long)context);
    printf("thick(0)       = %f\n", THICK(     0));
    printf("thick_dot(0)   = %f\n", THICK_DOT (0));
    printf("camber(0)      = %f\n", CAMBER(    0));
    printf("camber_dot(0)  = %f\n", CAMBER_DOT(0));
#endif

    /* default return values */
    *ebody  = NULL;
    *nMesh  = 0;
    *string = NULL;

#ifdef UDP
    /* check arguments */
    if (udps[0].arg[0].size > 1) {
        printf(" udpExecute: thick should be a scalar\n");
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (THICK(0) <= 0) {
        printf(" udpExecute: thick=%f < 0\n", THICK(0));
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (udps[0].arg[1].size > 1) {
        printf(" udpExecute: camber should be a scalar\n");
        status  = EGADS_RANGERR;
        goto cleanup;

    } else if (fabs(CAMBER(0)) > THICK(0)/2) {
        printf(" udpExecute: fabs(camber)=%f > thick/2=%f\n", fabs(CAMBER(0)), THICK(0)/2);
        status  = EGADS_RANGERR;
        goto cleanup;

    }

    /* cache copy of arguments for future use */
    status = cacheUdp();
    if (status < 0) {
        printf(" udpExecute: problem caching arguments\n");
        goto cleanup;
    }
#endif

#ifdef DEBUG
    printf("thick[%d]      = %f\n", numUdp, THICK(     numUdp));
    printf("thick_dot[%d]  = %f\n", numUdp, THICK_DOT( numUdp));
    printf("camber[%d]     = %f\n", numUdp, CAMBER(    numUdp));
    printf("camber_dot[%d] = %f\n", numUdp, CAMBER_DOT(numUdp));
#endif

    /* compute the geometry */
    L   = 0.50;
    Hup = THICK(0) / 2 + CAMBER(0);
    Hlo = THICK(0) / 2 - CAMBER(0);
    Rup = (L*L + Hup*Hup) / (2*Hup);
    Rlo = (L*L + Hlo*Hlo) / (2*Hlo);

    /* create the upper surface circle */
    if (fabs(Hup) > EPS06) {
        circ[0] = L;
        circ[1] = Hup - Rup;
        circ[2] = 0;
        circ[3] = 1;
        circ[4] = 0;
        circ[5] = 0;
        circ[6] = 0;
        circ[7] = 1;
        circ[8] = 0;
        circ[9] = Rup;

        status = EG_makeGeometry(context, CURVE, CIRCLE, NULL, NULL, circ, &(ecurves[0]));
        if (status != EGADS_SUCCESS) goto cleanup;
    } else {
        circ[0] =  1;
        circ[1] =  0;
        circ[2] =  0;
        circ[3] = -1;
        circ[4] =  0;
        circ[5] =  0;

        status = EG_makeGeometry(context, CURVE, LINE, NULL, NULL, circ, &(ecurves[0]));
        if (status != EGADS_SUCCESS) goto cleanup;
    }

    /* create the lower surface circle */
    if (fabs(Hlo) > EPS06) {
        circ[0] = L;
        circ[1] = Rlo - Hlo;
        circ[2] = 0;
        circ[3] = 1;
        circ[4] = 0;
        circ[5] = 0;
        circ[6] = 0;
        circ[7] = 1;
        circ[8] = 0;
        circ[9] = Rlo;

        status = EG_makeGeometry(context, CURVE, CIRCLE, NULL, NULL, circ, &(ecurves[1]));
        if (status != EGADS_SUCCESS) goto cleanup;
    } else {
        circ[0] =  0;
        circ[1] =  0;
        circ[2] =  0;
        circ[3] =  1;
        circ[4] =  0;
        circ[5] =  0;

        status = EG_makeGeometry(context, CURVE, LINE, NULL, NULL, circ, &(ecurves[1]));
        if (status != EGADS_SUCCESS) goto cleanup;
    }

    /* make Nodes */
    node1[0] = 0;
    node1[1] = 0;
    node1[2] = 0;

    status = EG_makeTopology(context, NULL, NODE, 0, node1, 0, NULL, NULL, &(enodes[0]));
    if (status != EGADS_SUCCESS) goto cleanup;

    node2[0] = 1;
    node2[1] = 0;
    node2[2] = 0;

    status = EG_makeTopology(context, NULL, NODE, 0, node2, 0, NULL, NULL, &(enodes[1]));
    if (status != EGADS_SUCCESS) goto cleanup;

    enodes[2] = enodes[0];

    /* get the parameter range for upper surface */
    status = EG_invEvaluate(ecurves[0], node2, &(trange[0]), data);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_invEvaluate(ecurves[0], node1, &(trange[1]), data);
    if (status != EGADS_SUCCESS) goto cleanup;

    if (trange[1] < trange[0]) trange[1] += TWOPI;

    /* make Edge for upper surface */
    status = EG_makeTopology(context, ecurves[0], EDGE, TWONODE, trange, 2, &(enodes[1]), NULL, &(eedges[0]));
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the parameter range for lower surface */
    status = EG_invEvaluate(ecurves[1], node1, &(trange[0]), data);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_invEvaluate(ecurves[1], node2, &(trange[1]), data);
    if (status != EGADS_SUCCESS) goto cleanup;

    if (trange[1] < trange[0]) trange[1] += TWOPI;

    /* make Edge for lower surface */
    status = EG_makeTopology(context, ecurves[1], EDGE, TWONODE, trange, 2, &(enodes[0]), NULL, &(eedges[1]));
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make Loop from these Edges */
    sense[0] = SFORWARD;
    sense[1] = SFORWARD;

    status = EG_makeTopology(context, NULL, LOOP, CLOSED, NULL, 2, eedges, sense, &eloop);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make Face from the loop */
    status = EG_makeFace(eloop, SFORWARD, NULL, &eface);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* create the FaceBody (which will be returned) */
    status = EG_makeTopology(context, NULL, BODY, FACEBODY, NULL, 1, &eface, NULL, ebody);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* no output value(s) */

    /* remember this model (Body) */
#ifdef UDP
    udps[numUdp].ebody = *ebody;
#endif

#ifdef DEBUG
    printf("udpExecute -> *ebody=%llx\n", (long long)(*ebody));
#endif

cleanup:
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
    int    status = EGADS_SUCCESS;

    int    iudp, judp, nnode, nedge, nface, ipnt, oclass, mtype, nchild, *senses;
    int    iupper, ilower;
    double Hup, Hupdot, Hlo, Hlodot, L, Rup, Rupdot, Rlo, Rlodot;
    double Tup, Tupdot, Tlo, Tlodot, xcup, xcupdot, xclo, xclodot, ycup, ycupdot, yclo, yclodot;
    double data[18], velup[3], vello[3], yup, ylo, frac;
    ego    *enodes, *eedges, *efaces, eent, eref, *echilds;

#ifdef DEBUG
    printf("udpSensitivity(ebody=%llx, npnt=%d, entType=%d, entIndex=%d, uvs=%f %f)\n",
           (long long)ebody, npnt, entType, entIndex, uvs[0], uvs[1]);
#endif

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

    /* find the ego entity */
    if (entType == OCSM_NODE) {
        status = EG_getBodyTopos(ebody, NULL, NODE, &nnode, &enodes);
        if (status != EGADS_SUCCESS) goto cleanup;

        eent = enodes[entIndex-1];

        EG_free(enodes);
    } else if (entType == OCSM_EDGE) {
        status = EG_getBodyTopos(ebody, NULL, EDGE, &nedge, &eedges);
        if (status != EGADS_SUCCESS) goto cleanup;

        eent = eedges[entIndex-1];

        EG_free(eedges);
    } else if (entType == OCSM_FACE) {
        status = EG_getBodyTopos(ebody, NULL, FACE, &nface, &efaces);
        if (status != EGADS_SUCCESS) goto cleanup;

        eent = efaces[entIndex-1];

        EG_free(efaces);
    } else {
        printf("udpSensitivity: bad entType=%d\n", entType);
        status = EGADS_ATTRERR;
        goto cleanup;
    }

    /* compute properties of upper surface */
    Hup    = THICK(    iudp)/2 + CAMBER(    iudp);
    Hupdot = THICK_DOT(iudp)/2 + CAMBER_DOT(iudp);
    if (fabs(Hup) > EPS06) {
        L       = 0.50;
        Rup     = (Hup*Hup + L*L) / (2*Hup);
        Rupdot  = (Hup*Hup - L*L) / (2*Hup*Hup) * Hupdot;
        xcup    = L;
        xcupdot = 0;
        ycup    = Hup    - Rup;
        ycupdot = Hupdot - Rupdot;
        iupper  = 1;
    } else {
        iupper  = 0;
    }

    /* compute properties of lower surface */
    Hlo    = THICK(    iudp)/2 - CAMBER(    iudp);
    Hlodot = THICK_DOT(iudp)/2 - CAMBER_DOT(iudp);
    if (fabs(Hlo) > EPS06) {
        L       = 0.50;
        Rlo     = (Hlo*Hlo + L*L) / (2*Hlo);
        Rlodot  = (Hlo*Hlo - L*L) / (2*Hlo*Hlo) * Hlodot;
        xclo    = L;
        xclodot = 0;
        yclo    = Rlo    - Hlo;
        yclodot = Rlodot - Hlodot;
        ilower  = 1;
    } else {
        ilower  = 0;
    }

    /* loop through the points */
    for (ipnt = 0; ipnt < npnt; ipnt++) {

        /* find the physical coordinates */
        if        (entType == OCSM_NODE) {
            status = EG_getTopology(eent, &eref, &oclass, &mtype,
                                    data, &nchild, &echilds, &senses);
            if (status != EGADS_SUCCESS) goto cleanup;
        } else if (entType == OCSM_EDGE) {
            status = EG_evaluate(eent, &(uvs[ipnt]), data);
            if (status != EGADS_SUCCESS) goto cleanup;
        } else if (entType == OCSM_FACE) {
            status = EG_evaluate(eent, &(uvs[2*ipnt]), data);
            if (status != EGADS_SUCCESS) goto cleanup;
        }

        /* compute upper and lower airfoil points and sensitivities */
        if (iupper == 1) {
            yup      = ycup + sqrt(pow(Rup,2) - pow(data[0]-xcup,2));
            Tup      = atan2(yup-ycup, data[0]-xcup);
            Tupdot   = -ycupdot * (data[0]-xcup) / (pow(data[0]-xcup,2) + pow(yup-ycup,2));

            velup[0] = xcupdot + Rupdot * cos(Tup) - Tupdot * Rup * sin(Tup);
            velup[1] = ycupdot + Rupdot * sin(Tup) + Tupdot * Rup * cos(Tup);
        } else {
            yup      = 0;
            velup[0] = 0;
            velup[1] = 0;
        }

        if (ilower == 1) {
            ylo      = yclo - sqrt(pow(Rlo,2) - pow(data[0]-xclo,2));
            Tlo      = atan2(ylo-yclo, data[0]-xclo);
            Tlodot   = -yclodot * (data[0]-xclo) / (pow(data[0]-xclo,2) + pow(ylo-yclo,2));

            vello[0] = xclodot + Rlodot * cos(Tlo) - Tlodot * Rlo * sin(Tlo);
            vello[1] = yclodot + Rlodot * sin(Tlo) + Tlodot * Rlo * cos(Tlo);
        } else {
            ylo      = 0;
            vello[0] = 0;
            vello[1] = 0;
        }

        /* take the weighted average */
        if (fabs(yup) > fabs(ylo)) {
            frac = (data[1] - ylo) / (yup - ylo);
        } else {
            frac = 0.5;
        }

        vels[3*ipnt  ] = (1-frac) * vello[0] + frac * velup[0];
        vels[3*ipnt+1] = (1-frac) * vello[1] + frac * velup[1];
        vels[3*ipnt+2] = 0;
    }

cleanup:
    return status;
}
