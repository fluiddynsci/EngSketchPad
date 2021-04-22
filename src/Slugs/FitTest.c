 /*
 ************************************************************************
 *                                                                      *
 * FotTest.c -- test Fitter on a single Face                            *
 *                                                                      *
 *              Written by John Dannenhoffer @ Syracuse University      *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#ifdef GRAFIC
    #include "grafic.h"
#endif

#ifdef WIN32
    #define snprintf   _snprintf
#endif

#define STRNCPY(A, B, LEN) strncpy(A, B, LEN); A[LEN-1] = '\0';

#include "egads.h"
#include "common.h"
#include "Fitter.h"
#include "Tessellate.h"
#include "RedBlackTree.h"

/*
 ***********************************************************************
 *                                                                     *
 * macros (including those that go along with common.h)                *
 *                                                                     *
 ***********************************************************************
 */
#ifdef DEBUG
   #define DOPEN {if (dbg_fp == NULL) dbg_fp = fopen("FitTest.dbg", "w");}
   static  FILE *dbg_fp=NULL;
#endif

/*
 ***********************************************************************
 *                                                                     *
 * structures                                                          *
 *                                                                     *
 ***********************************************************************
 */

/*
 ***********************************************************************
 *                                                                     *
 * global variables                                                    *
 *                                                                     *
 ***********************************************************************
 */
/* global variables associated with tessellation, ... */
static int               outLevel = 1;       /* default output level */

ego    context;

/*
 ***********************************************************************
 *                                                                     *
 * declarations for routines defined below                             *
 *                                                                     *
 ***********************************************************************
 */

static int makeNode(double xyz[], ego *enode);
static int makeEdge(int ncp, double cp[], ego enodeB, ego enodeE, ego *eedge);
static int makeFace(int ncp, double cp[], ego eedgeS, ego eedgeN, ego eedgeW, ego eedgeE, ego *eface);

/*
 ***********************************************************************
 *                                                                     *
 *   Main program                                                      *
 *                                                                     *
 ***********************************************************************
 */
int
main(int       argc,                /* (in)  number of arguments */
     char      *argv[])             /* (in)  array of arguments */
{

    int       status, i, j, idum;
    char      *casename=NULL, *filename=NULL, temp[257];

    int       nS, nN, nW, nE, nI;
    double    *xyzS=NULL, *cpS=NULL, *tS=NULL;
    double    *xyzN=NULL, *cpN=NULL, *tN=NULL;
    double    *xyzW=NULL, *cpW=NULL, *tW=NULL;
    double    *xyzE=NULL, *cpE=NULL, *tE=NULL;
    double    *xyzI=NULL, *cpI=NULL, *uvI=NULL;

    int       ncp, nmin, numiter, bitflag;
    double    smooth, normf, maxf, dotmin;
    ego       enodeSW, enodeSE, enodeNW, enodeNE;
    ego       eedgeS, eedgeN, eedgeW, eedgeE;
    ego       eface, eshell, ebody, emodel;

    FILE      *fp;

#ifdef GRAFIC
    int      io_kbd=5, io_scr=6;
    char     pltitl[255];
#endif

    ROUTINE(MAIN);

    /* --------------------------------------------------------------- */

    MALLOC(casename, char, 257);
    MALLOC(filename, char, 257);

    /* get the flags and casename(s) from the command line */
    if (argc < 3) {
        SPRINT0(0, "Proper usage: FitTest casename ncp [outLevel]");
        exit(0);
    } else {
        STRNCPY(casename, argv[1], 256);
        sscanf(argv[2], "%d", &ncp);
    }

    if (argc == 4) {
        sscanf(argv[3], "%d", &outLevel);
    }

    /* welcome banner */
    SPRINT0(1, "**********************************************************");
    SPRINT0(1, "*                                                        *");
    SPRINT0(1, "*                   Program FitTest                      *");
    SPRINT0(1, "*                                                        *");
    SPRINT0(1, "*           written by John Dannenhoffer, 2020           *");
    SPRINT0(1, "*                                                        *");
    SPRINT0(1, "**********************************************************");

#ifdef GRAFIC
    /* initialize the grafics */
    sprintf(pltitl, "Program FitTest");
    grinit_(&io_kbd, &io_scr, pltitl, strlen(pltitl));
#endif

    /* set up an EGADS context */
    status = EG_open(&context);
    CHECK_STATUS(EG_open);

    status = EG_setOutLevel(context, outLevel);
    CHECK_STATUS(EG_setOutLevel);

    /* read the input file */
    STRNCPY(filename, casename, 257);
    strcat(filename, ".dat");

    fp = fopen(filename, "r");
    if (fp == NULL) {
        SPRINT1(0, "File \"%s\" does not exist", filename);
        exit(0);
    }

    /* read south boundary */
    fscanf(fp, "%d %d %s", &nS, &idum, temp);
    SPRINT1(2, "nS=%5d", nS);

    MALLOC(xyzS, double, 3*nS);

    for (i = 0; i < nS; i++) {
        fscanf(fp, "%lf %lf %lf",
               &(xyzS[3*i]), &(xyzS[3*i+1]), &(xyzS[3*i+2]));
    }

    /* read north boundary */
    fscanf(fp, "%d %d %s", &nN, &idum, temp);
    SPRINT1(2, "nN=%5d", nN);

    MALLOC(xyzN, double, 3*nN);

    for (i = 0; i < nN; i++) {
        fscanf(fp, "%lf %lf %lf",
               &(xyzN[3*i]), &(xyzN[3*i+1]), &(xyzN[3*i+2]));
    }

    /* read west boundary */
    fscanf(fp, "%d %d %s", &nW, &idum, temp);
    SPRINT1(2, "nW =%5d", nW);

    MALLOC(xyzW, double, 3*nW);

    for (i = 0; i < nW; i++) {
        fscanf(fp, "%lf %lf %lf",
               &(xyzW[3*i]), &(xyzW[3*i+1]), &(xyzW[3*i+2]));
    }

    /* read east boundary */
    fscanf(fp, "%d %d %s", &nE, &idum, temp);
    SPRINT1(2, "nE =%5d", nE);

    MALLOC(xyzE, double, 3*nE);

    for (i = 0; i < nE; i++) {
        fscanf(fp, "%lf %lf %lf",
               &(xyzE[3*i]), &(xyzE[3*i+1]), &(xyzE[3*i+2]));
    }

    /* read interior points */
    fscanf(fp, "%d %d %s", &nI, &idum, temp);

    MALLOC(xyzI, double, 3*nI);

    for (i = 0; i < nI; i++) {
        fscanf(fp, "%lf %lf %lf",
               &(xyzI[3*i]), &(xyzI[3*i+1]), &(xyzI[3*i+2]));
    }

    fclose(fp);
    fp = NULL;

    /* create Nodes at beginning and end of south and north */
    status = makeNode(&(xyzS[     0]), &enodeSW);
    SPRINT1(2, "makeNode(SW) -> status=%d", status);
    CHECK_STATUS(makeNode);

    status = makeNode(&(xyzS[3*nS-3]), &enodeSE);
    SPRINT1(2, "makeNode(SE) -> status=%d", status);
    CHECK_STATUS(makeNode);

    status = makeNode(&(xyzN[     0]), &enodeNW);
    SPRINT1(2, "makeNode(NW) -> status=%d", status);
    CHECK_STATUS(makeNode);

    status = makeNode(&(xyzN[3*nN-3]), &enodeNE);
    SPRINT1(2, "makeNode(NE) -> status=%d", status);
    CHECK_STATUS(makeNode);

    /* set up memory for the 2d fit */
    ncp = 7;
    MALLOC(cpI, double, 3*ncp*ncp);
    MALLOC(uvI, double, 2*nI     );


    /* fit the south boundary */
    MALLOC(cpS, double, 3*nS);
    MALLOC(tS,  double,   nS);

    cpS[      0] = xyzS[     0];
    cpS[      1] = xyzS[     1];
    cpS[      2] = xyzS[     2];
    cpS[3*ncp-3] = xyzS[3*nS-3];
    cpS[3*ncp-2] = xyzS[3*nS-2];
    cpS[3*ncp-1] = xyzS[3*nS-1];

    bitflag = 0;
    smooth  = 1;
    if (outLevel < 2) {
        status = fit1dCloud(nS, bitflag, xyzS, ncp, cpS, smooth, tS,
                            &normf, &maxf, &dotmin, &nmin, &numiter, NULL);
    } else {
        status = fit1dCloud(nS, bitflag, xyzS, ncp, cpS, smooth, tS,
                            &normf, &maxf, &dotmin, &nmin, &numiter, stdout);
    }
    SPRINT7(1, "fit1dCloud(south, npnt=%4d, ncp=%4d) -> status=%4d,  numiter=%4d,  normf=%12.4e,  dotmin=%.4f,  nmin=%d",
           nS, ncp, status, numiter, normf, dotmin, nmin);
    CHECK_STATUS(fit1dCloud);

    for (j = 0; j < ncp; j++) {
        i = 0;
        cpI[3*(i*ncp+j)  ] = cpS[3*j  ];
        cpI[3*(i*ncp+j)+1] = cpS[3*j+1];
        cpI[3*(i*ncp+j)+2] = cpS[3*j+2];
    }

#ifdef GRAFIC
    status = plotCurve(nS, xyzS, tS, ncp, cpS, normf, dotmin, nmin);
    SPRINT1(2, "plotCurve(south) -> status=%d", status);
    CHECK_STATUS(plotCurve);
#endif

    status = makeEdge(ncp, cpS, enodeSW, enodeSE, &eedgeS);
    SPRINT1(2, "makeEdge(S) -> status=%d", status);
    CHECK_STATUS(makeEdge);

    /* fit the north boundary */
    MALLOC(cpN, double, 3*nN);
    MALLOC(tN,  double,   nN);

    cpN[      0] = xyzN[     0];
    cpN[      1] = xyzN[     1];
    cpN[      2] = xyzN[     2];
    cpN[3*ncp-3] = xyzN[3*nN-3];
    cpN[3*ncp-2] = xyzN[3*nN-2];
    cpN[3*ncp-1] = xyzN[3*nN-1];

    if (outLevel < 2) {
        status = fit1dCloud(nN, bitflag, xyzN, ncp, cpN, smooth, tN,
                            &normf, &maxf, &dotmin, &nmin, &numiter, NULL);
    } else {
        status = fit1dCloud(nN, bitflag, xyzN, ncp, cpN, smooth, tN,
                            &normf, &maxf, &dotmin, &nmin, &numiter, stdout);
    }
    SPRINT7(1, "fit1dCloud(north, npnt=%4d, ncp=%4d) -> status=%4d,  numiter=%4d,  normf=%12.4e,  dotmin=%.4f,  nmin=%d",
           nN, ncp, status, numiter, normf, dotmin, nmin);
    CHECK_STATUS(fit1dCloud);

    for (j = 0; j < ncp; j++) {
        i = ncp - 1;
        cpI[3*(i*ncp+j)  ] = cpN[3*j  ];
        cpI[3*(i*ncp+j)+1] = cpN[3*j+1];
        cpI[3*(i*ncp+j)+2] = cpN[3*j+2];
    }

#ifdef GRAFIC
    status = plotCurve(nN, xyzN, tN, ncp, cpN, normf, dotmin, nmin);
    SPRINT1(2, "plotCurve(north) -> status=%d", status);
    CHECK_STATUS(plotCurve);
#endif

    status = makeEdge(ncp, cpN, enodeNW, enodeNE, &eedgeN);
    SPRINT1(2, "makeEdge(N) -> status=%d", status);
    CHECK_STATUS(makeEdge);

    /* fit the west boundary */
    MALLOC(cpW, double, 3*nW);
    MALLOC(tW,  double,   nW);

    cpW[      0] = xyzW[     0];
    cpW[      1] = xyzW[     1];
    cpW[      2] = xyzW[     2];
    cpW[3*ncp-3] = xyzW[3*nW-3];
    cpW[3*ncp-2] = xyzW[3*nW-2];
    cpW[3*ncp-1] = xyzW[3*nW-1];

    if (outLevel < 2) {
        status = fit1dCloud(nW, bitflag, xyzW, ncp, cpW, smooth, tW,
                            &normf, &maxf, &dotmin, &nmin, &numiter, NULL);
    } else {
        status = fit1dCloud(nW, bitflag, xyzW, ncp, cpW, smooth, tW,
                            &normf, &maxf, &dotmin, &nmin, &numiter, stdout);
    }
    SPRINT7(1, "fit1dCloud(west,  npnt=%4d, ncp=%4d) -> status=%4d,  numiter=%4d,  normf=%12.4e,  dotmin=%.4f,  nmin=%d",
           nW, ncp, status, numiter, normf, dotmin, nmin);
    CHECK_STATUS(fit1dCloud);

    for (i = 0; i < ncp; i++) {
        j = 0;
        cpI[3*(i*ncp+j)  ] = cpW[3*i  ];
        cpI[3*(i*ncp+j)+1] = cpW[3*i+1];
        cpI[3*(i*ncp+j)+2] = cpW[3*i+2];
    }

#ifdef GRAFIC
    status = plotCurve(nW, xyzW, tW, ncp, cpW, normf, dotmin, nmin);
    SPRINT1(2, "plotCurve(west) -> status=%d", status);
    CHECK_STATUS(plotCurve);
#endif

    status = makeEdge(ncp, cpW, enodeSW, enodeNW, &eedgeW);
    SPRINT1(2, "makeEdge(W) -> status=%d", status);
    CHECK_STATUS(makeEdge);

    /* fit the east boundary */
    MALLOC(cpE, double, 3*nE);
    MALLOC(tE,  double,   nE);

    cpE[      0] = xyzE[     0];
    cpE[      1] = xyzE[     1];
    cpE[      2] = xyzE[     2];
    cpE[3*ncp-3] = xyzE[3*nE-3];
    cpE[3*ncp-2] = xyzE[3*nE-2];
    cpE[3*ncp-1] = xyzE[3*nE-1];

    if (outLevel < 2) {
        status = fit1dCloud(nE, bitflag, xyzE, ncp, cpE, smooth, tE,
                            &normf, &maxf, &dotmin, &nmin, &numiter, NULL);
    } else {
        status = fit1dCloud(nE, bitflag, xyzE, ncp, cpE, smooth, tE,
                            &normf, &maxf, &dotmin, &nmin, &numiter, stdout);
    }
    SPRINT7(1, "fit1dCloud(east,  npnt=%4d, ncp=%4d) -> status=%4d,  numiter=%4d,  normf=%12.4e,  dotmin=%.4f,  nmin=%d",
           nE, ncp, status, numiter, normf, dotmin, nmin);
    CHECK_STATUS(fit1dCloud);

    for (i = 0; i < ncp; i++) {
        j = ncp - 1;
        cpI[3*(i*ncp+j)  ] = cpE[3*i  ];
        cpI[3*(i*ncp+j)+1] = cpE[3*i+1];
        cpI[3*(i*ncp+j)+2] = cpE[3*i+2];
    }

#ifdef GRAFIC
    status = plotCurve(nE, xyzE, tE, ncp, cpE, normf, dotmin, nmin);
    SPRINT1(2, "plotCurve(east) -> status=%d", status);
    CHECK_STATUS(plotCurve);
#endif

    status = makeEdge(ncp, cpE, enodeSE, enodeNE, &eedgeE);
    SPRINT1(2, "makeEdge(E) -> status=%d", status);
    CHECK_STATUS(makeEdge);

    /* fit the 2d surface */
    if (outLevel < 2) {
        status = fit2dCloud(nI, bitflag, xyzI, ncp, ncp, cpI, smooth, uvI,
                            &normf, &maxf, &nmin, &numiter, NULL);
    } else {
        status = fit2dCloud(nI, bitflag, xyzI, ncp, ncp, cpI, smooth, uvI,
                            &normf, &maxf, &nmin, &numiter, stdout);
    }
    SPRINT6(1, "fit2dCloud(       npnt=%4d, ncp=%4d) -> status=%4d,  numiter=%4d,  normf=%12.4e,                  nmin=%d",
           nI, ncp, status, numiter, normf, nmin);
    CHECK_STATUS(fit2dCloud);

#ifdef GRAFIC
    status = plotSurface(nI, xyzI, uvI, ncp, cpI, normf, nmin);
    SPRINT1(2, "plotSurface -> status=%d", status);
    CHECK_STATUS(plotSurface);
#endif

    /* make the Face */
    status = makeFace(ncp, cpI, eedgeS, eedgeN, eedgeW, eedgeE, &eface);
    SPRINT1(2, "makeFace -> status=%d", status);
    CHECK_STATUS(makeFace);

    /* make a Sheel, SheetBody, and Model */
    status = EG_makeTopology(context, NULL, SHELL, OPEN,
                             NULL, 1, &eface, NULL, &eshell);
    SPRINT1(2, "makeShell -> status=%d", status);
    CHECK_STATUS(EG_makeTopology);

    status = EG_makeTopology(context, NULL, BODY, SHEETBODY,
                             NULL, 1, &eshell, NULL, &ebody);
    SPRINT1(2, "makeBody -> status=%d", status);
    CHECK_STATUS(EG_makeTopology);

    status = EG_makeTopology(context, NULL, MODEL, 0,
                             NULL, 1, &ebody, NULL, &emodel);
    SPRINT1(2, "makeModel -> status=%d", status);
    CHECK_STATUS(EG_makeTopology);

    /* write out an egads file */
    STRNCPY(filename, casename, 256);
    strcat(filename, ".egads");

    status = remove(filename);
    if (status == 0) {
        SPRINT1(0, "WARNING:: file \"%s\" is being overwritten", filename);
    } else {
        SPRINT1(1, "File \"%s\" is being written", filename);

    }

    status = EG_saveModel(emodel, filename);
    CHECK_STATUS(EG_saveModel);

    status = EG_deleteObject(emodel);
    CHECK_STATUS(EG_deleteObject);

    /* clean up */
    status = EG_close(context);
    CHECK_STATUS(EG_close);

    SPRINT0(1, "==> FitTest completed successfully");

cleanup:
    FREE(casename);
    FREE(filename);

    FREE(tS);
    FREE(tN);
    FREE(tW);
    FREE(tE);
    FREE(uvI);

    FREE(cpS);
    FREE(cpN);
    FREE(cpW);
    FREE(cpE);
    FREE(cpI);

    FREE(xyzS);
    FREE(xyzN);
    FREE(xyzW);
    FREE(xyzE);
    FREE(xyzI);

    return status;
}


/*
 ***********************************************************************
 *                                                                     *
 *   makeNode - make an Egads Node                                     *
 *                                                                     *
 ***********************************************************************
 */
static int
makeNode(double xyz[],
         ego    *enode)
{
    int    status = SUCCESS;

    ROUTINE(makeNode);

    status = EG_makeTopology(context, NULL, NODE, 0, xyz, 0, NULL, NULL, enode);
    CHECK_STATUS(EG_makeTopology);

cleanup:
    return status;
}


/*
 ***********************************************************************
 *                                                                     *
 *   makeEdge - make an Egads Edge                                     *
 *                                                                     *
 ***********************************************************************
 */
static int
makeEdge(int    ncp,
         double cp[],
         ego    enodeB,
         ego    enodeE,
         ego    *eedge)
{
    int    status = SUCCESS;

    int    header[4], ndata, periodic, j;
    double *cpdata=NULL, tdata[4];
    ego    ecurv, enodes[2];

    ROUTINE(makeEdge);

    header[0] = 0;
    header[1] = 3;
    header[2] = ncp;
    header[3] = ncp + 4;

    ndata = (ncp+4) + 3 * ncp;
    MALLOC(cpdata, double, ndata);

    ndata = 0;

    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    for (j = 1; j < ncp-3; j++) {
        cpdata[ndata++] = j;
    }
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;

    for (j = 0; j < ncp; j++) {
        cpdata[ndata++] = cp[3*j  ];
        cpdata[ndata++] = cp[3*j+1];
        cpdata[ndata++] = cp[3*j+2];
    }

    status = EG_makeGeometry(context, CURVE, BSPLINE, NULL, header, cpdata, &ecurv);
    CHECK_STATUS(EG_makeGeometry);

    FREE(cpdata);

    status = EG_getRange(ecurv, tdata, &periodic);
    CHECK_STATUS(EG_getRange);

    enodes[0] = enodeB;
    enodes[1] = enodeE;
    status = EG_makeTopology(context, ecurv, EDGE, TWONODE,
                             tdata, 2, enodes, NULL, eedge);
    CHECK_STATUS(EG_makeTopology);

cleanup:
    FREE(cpdata);

    return status;
}


/*
 ***********************************************************************
 *                                                                     *
 *   makeFace - make an Egads Face                                     *
 *                                                                     *
 ***********************************************************************
 */
static int
makeFace(int    ncp,
         double cp[],
         ego    eedgeS,
         ego    eedgeN,
         ego    eedgeW,
         ego    eedgeE,
         ego    *eface)
{
    int    status = SUCCESS;

    int    header[7], ndata, periodic, j, senses[4];
    double *cpdata=NULL, tdata[4];
    ego    esurf, eedges[8], eloop;

    ROUTINE(makeFace);

    header[0] = 0;
    header[1] = 3;
    header[2] = ncp;
    header[3] = ncp + 4;
    header[4] = 3;
    header[5] = ncp;
    header[6] = ncp + 4;

    ndata = 2 * (ncp+4) + 3 * ncp * ncp;
    MALLOC(cpdata, double, ndata);

    ndata = 0;

    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    for (j = 1; j < ncp-3; j++) {
        cpdata[ndata++] = j;
    }
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;

    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    cpdata[ndata++] = 0;
    for (j = 1; j < ncp-3; j++) {
        cpdata[ndata++] = j;
    }
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;
    cpdata[ndata++] = ncp - 3;

    for (j = 0; j < ncp*ncp; j++) {
        cpdata[ndata++] = cp[3*j  ];
        cpdata[ndata++] = cp[3*j+1];
        cpdata[ndata++] = cp[3*j+2];
    }

    status = EG_makeGeometry(context, SURFACE, BSPLINE, NULL, header, cpdata, &esurf);
    SPRINT1(2, "makeSurf -> status=%d", status);
    CHECK_STATUS(EG_makeGeometry);

    status = EG_getRange(esurf, tdata, &periodic);
    CHECK_STATUS(EG_getRange);

    /* south Edge and Pcurve */
    eedges[0] = eedgeS;
    senses[0] = SFORWARD;

    cpdata[0] = tdata[0];
    cpdata[1] = tdata[2];
    cpdata[2] = tdata[1] - tdata[0];
    cpdata[3] = 0;
    status = EG_makeGeometry(context, PCURVE, LINE, esurf, NULL, cpdata, &(eedges[4]));
    SPRINT1(2, "makePcurve -> status=%d", status);
    CHECK_STATUS(EG_makeGeometry);

    /* east Edge and Pcurve */
    eedges[1] = eedgeE;
    senses[1] = SFORWARD;

    cpdata[0] = tdata[1];
    cpdata[1] = tdata[2];
    cpdata[2] = 0;
    cpdata[3] = tdata[3] - tdata[2];
    status = EG_makeGeometry(context, PCURVE, LINE, esurf, NULL, cpdata, &(eedges[5]));
    SPRINT1(2, "makePcurve -> status=%d", status);
    CHECK_STATUS(EG_makeGeometry);

    /* north Edge and Pcurve */
    eedges[2] = eedgeN;
    senses[2] = SREVERSE;

    cpdata[0] = tdata[0];
    cpdata[1] = tdata[3];
    cpdata[2] = tdata[1] - tdata[0];
    cpdata[3] = 0;
    status = EG_makeGeometry(context, PCURVE, LINE, esurf, NULL, cpdata, &(eedges[6]));
    SPRINT1(2, "makePcurve -> status=%d", status);
    CHECK_STATUS(EG_makeGeometry);

    /* west Edge and Pcurve */
    eedges[3] = eedgeW;
    senses[3] = SREVERSE;

    cpdata[0] = tdata[0];
    cpdata[1] = tdata[2];
    cpdata[2] = 0;
    cpdata[3] = tdata[3] - tdata[2];
    status = EG_makeGeometry(context, PCURVE, LINE, esurf, NULL, cpdata, &(eedges[7]));
    SPRINT1(2, "makePcurve -> status=%d", status);
    CHECK_STATUS(EG_makeGeometry);

    /* make Loop and Face */
    status = EG_makeTopology(context, esurf, LOOP, CLOSED, NULL, 4, eedges, senses, &eloop);
    SPRINT1(2, "makeLoop -> status=%d", status);
    CHECK_STATUS(EG_makeLoop);

    senses[0] = SFORWARD;
    status = EG_makeTopology(context, esurf, FACE, SFORWARD, NULL,
                             1, &eloop, senses, eface);
    SPRINT1(2, "makeFace -> status=%d", status);
    CHECK_STATUS(EG_makeTopology);

cleanup:
    FREE(cpdata);

    return status;
}
