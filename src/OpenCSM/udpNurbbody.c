 /*
 ************************************************************************
 *                                                                      *
 * udpNurbbody -- udp file to generate a Body from NURBS                *
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

/* this can act as a UDF and write a nurbs.txt file from a Body */
#define NUMUDPINPUTBODYS 0
#define NUMUDPARGS 1
#include "udpUtilities.h"

/* shorthands for accessing argument values and velocities */
#define FILENAME(      IUDP)   ((char   *) (udps[IUDP].arg[ 0].val))

/* data about possible arguments */
static char*  argNames[NUMUDPARGS] = {"filename", };
static int    argTypes[NUMUDPARGS] = {ATTRSTRING, };
static int    argIdefs[NUMUDPARGS] = {0,          };
static double argDdefs[NUMUDPARGS] = {0.,         };

/* get utility routines: udpErrorStr, udpInitialize, udpReset, udpSet,
                         udpGet, udpVel, udpClean, udpMesh */
#include "udpUtilities.c"
#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>

#ifdef WIN32
   #define SLASH '\\'
#else
   #include <unistd.h>
   #include <libgen.h>
   #define SLASH '/'
#endif

/***********************************************************************/
/*                                                                     */
/* declarations                                                        */
/*                                                                     */
/***********************************************************************/

#define           HUGEQ           99999999.0
#define           PIo2            1.5707963267948965579989817
#define           EPS06           1.0e-06
#define           EPS12           1.0e-12
#define           MIN(A,B)        (((A) < (B)) ? (A) : (B))
#define           MAX(A,B)        (((A) < (B)) ? (B) : (A))
#define           SQR(A)          ((A) * (A))

#ifdef WIN32
// Returns filename portion of the given path
// Returns empty string if path is directory
const char *basename(const char *path)
{
  const char *filename = strrchr(path, '\\');
  if (filename == NULL)
    filename = path;
  else
    filename++;
  return filename;
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
udpExecute(ego  context_in,             /* (in)  Egads context (or model) */
           ego  *ebody,                 /* (out) Body pointer */
           int  *nMesh,                 /* (out) number of associated meshes */
           char *string[])              /* (out) error message */
{
    int     status = EGADS_SUCCESS;

    int     oclass, mtype, nchild, *senses;
    int     nface, iface, i, n, header[7];
    int     j, nuknots, nvknots, ncpu, ncpv, *tempI=NULL;
    double  data[18], temp, *rdata=NULL, *tempR=NULL;
    FILE    *fp, *fp_output;
    ego     eref, esurf, *ebodys, *efaces, *echilds, context, emodel, emodel2;

#ifdef DEBUG
    printf("udpExecute(context_in=%llx)\n", (long long)context_in);
    printf("filename(0) = %s\n", FILENAME(0));
#endif

    /* default return values */
    *ebody  = NULL;
    *nMesh  = 0;
    *string = NULL;

    /* check arguments */

    /* cache copy of arguments for future use */
    status = cacheUdp();
    if (status < 0) {
        printf(" udpExecute: problem caching arguments\n");
        goto cleanup;
    }

#ifdef DEBUG
    printf("filename(%d) = %s\n", numUdp, FILENAME(numUdp));
#endif

    /* special mode to write nurbs.txt if running as UDF */
    if (NUMUDPINPUTBODYS > 0) {

        /* store the context into emodel in case it is run as a UDF */
        emodel = context_in;

        /* check that Model was input that contains one Body */
        status = EG_getTopology(emodel, &eref, &oclass, &mtype,
                                data, &nchild, &ebodys, &senses);
        if (status < EGADS_SUCCESS) goto cleanup;

        if (oclass != MODEL) {
            printf(" udpExecute: expecting a Model\n");
            status = EGADS_NOTMODEL;
            goto cleanup;
        } else if (nchild != 1) {
            printf(" udpExecute: expecting Model to contain one Body (not %d)\n", nchild);
            status = EGADS_NOTBODY;
            goto cleanup;
        }

        status = EG_getContext(emodel, &context);
        if (status < EGADS_SUCCESS) goto cleanup;

        status = EG_getBodyTopos(ebodys[0], NULL, FACE, &nface, &efaces);
        if (status < EGADS_SUCCESS) goto cleanup;

        fp_output = fopen("nurbs.txt", "w");
        if (fp_output == NULL) {
            printf(" udpExecute: could not open file\n");
            status = EGADS_NOTFOUND;
            goto cleanup;
        }

        for (iface = 0; iface < nface; iface++) {
            status = EG_getTopology(efaces[iface], &esurf, &oclass, &mtype,
                                    data, &nchild, &echilds, &senses);
            if (status < EGADS_SUCCESS) goto cleanup;

            status = EG_getGeometry(esurf, &oclass, &mtype, &eref, &tempI, &tempR);
            if (status < EGADS_SUCCESS) goto cleanup;

            if (oclass != SURFACE || mtype != BSPLINE) {
                printf(" udpExecute: not a bspline surface\n");
                status = EGADS_NOTGEOM;
                goto cleanup;
            }

            fprintf(fp_output, "%5d %5d %5d %5d %5d %5d %5d\n",
                    tempI[0], tempI[1], tempI[2], tempI[3], tempI[4], tempI[5], tempI[6]);

            ncpu    = tempI[2];
            nuknots = tempI[3];
            ncpv    = tempI[5];
            nvknots = tempI[6];

            j = 0;
            for (i = 0; i < nuknots; i++) {
                fprintf(fp_output, "%20.13e\n", tempR[j++]);
            }
            for (i = 0; i < nvknots; i++) {
                fprintf(fp_output, "%20.13e\n", tempR[j++]);
            }
            for (i = 0; i < ncpu*ncpv; i++) {
                fprintf(fp_output, "%20.13e %20.13e %20.13e\n", tempR[j], tempR[j+1], tempR[j+2]);
                j += 3;
            }
            if (tempI[0]%2 == 1) {
                for (i = 0; i < ncpu*ncpv; i++) {
                    fprintf(fp_output, "%20.13e\n", tempR[j++]);
                }
            }

            EG_free(tempI);
            EG_free(tempR);
        }

        fclose(fp_output);

        EG_free(efaces);

        /* make a copy of the Body (so that it does not get removed
           when OpenCSM deletes emodel) */
        status = EG_copyObject(ebodys[0], NULL, ebody);
        if (status < EGADS_SUCCESS) goto cleanup;

    /* normal processing */
    } else {

        context = context_in;

        /* open the filename and find the number of Faces (surfaces) */
        fp = fopen(FILENAME(numUdp), "r");
        if (fp == NULL) {
            status = EGADS_NOTFOUND;
            goto cleanup;
        }

        nface = 0;
        while (!feof(fp)) {

            /* read the header */
            for (i = 0; i < 7; i++) {
                fscanf(fp, "%d", &header[i]);
            }
            if (feof(fp)) break;

            /* increment the number of Faces */
            nface++;

            /* read the (double) data */
            /*  nknotu      nknotv           weight         ncpu        ncpv */
            n = header[3] + header[6] + (3 + header[0]%2) * header[2] * header[5];

            for (i = 0; i < n; i++) {
                fscanf(fp, "%lf", &temp);
            }
        }
#ifdef DEBUG
        printf("nface=%d\n", nface);
#endif

        /* allocate array for the Faces */
        assert(nface>0);

        efaces = (ego*) malloc(nface*sizeof(ego));
        if (efaces == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        /* go back and read each surface (Face) */
        rewind(fp);

        for (iface = 0; iface < nface; iface++) {

            /* read the header */
            for (i = 0; i < 7; i++) {
                fscanf(fp, "%d", &header[i]);
            }

            n = header[3] + header[6] + (3 + header[0]%2) * header[2] * header[5];

            /* read the (double) data */
            rdata = (double*) malloc(n*sizeof(double));
            if (rdata == NULL) {
                status = EGADS_MALLOC;
                goto cleanup;
            }

            for (i = 0; i < n; i++) {
                fscanf(fp, "%lf", &rdata[i]);
            }

            /* find the minimum and maximum knot values */
            data[0] = rdata[                    0];
            data[1] = rdata[header[3]          -1];
            data[2] = rdata[header[3]            ];
            data[3] = rdata[header[3]+header[6]-1];

            /* make the BSPLINE Surface */
            status = EG_makeGeometry(context, SURFACE, BSPLINE, NULL,
                                     header, rdata, &esurf);
            if (status < EGADS_SUCCESS) goto cleanup;

            free(rdata);

            /* make the Face */
            status = EG_makeFace(esurf, SFORWARD, data, &efaces[iface]);
            if (status < EGADS_SUCCESS) goto cleanup;
        }

        fclose(fp);

        /* sew the Faces into a new Model */
        status = EG_sewFaces(nface, efaces, 0, 0, &emodel2);
        if (status < EGADS_SUCCESS) goto cleanup;

        free(efaces);

        /* get the Body out of the new Model */
        status = EG_getTopology(emodel2, &eref, &oclass, &mtype, data,
                                &nchild, &echilds, &senses);
        if (status < EGADS_SUCCESS) goto cleanup;

        /* make a copy of the Body so that it does not get destroyed
           when the Model is deleted */
        status = EG_copyObject(echilds[0], NULL, ebody);
        if (status < EGADS_SUCCESS) goto cleanup;

        status = EG_deleteObject(emodel2);
        if (status < EGADS_SUCCESS) goto cleanup;
    }

    /* return the copied Body */
    udps[numUdp].ebody = *ebody;

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
    int    iudp, judp;

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

    /* this routine is not written yet */
    return EGADS_NOLOAD;
}
