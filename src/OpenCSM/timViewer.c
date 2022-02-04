/*
 ************************************************************************
 *                                                                      *
 * timViewer -- Tool Integration Module for simple viewer               *
 *                                                                      *
 *            Written by John Dannenhoffer@ Syracuse University         *
 *                                                                      *
 ************************************************************************
 */

/*
 * Copyright (C) 2013/2022  John F. Dannenhoffer, III (Syracuse University)
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
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "egads.h"
#include "common.h"
#include "OpenCSM.h"
#include "tim.h"
#include "wsserver.h"
#include "emp.h"

#define CINT    const int
#define CDOUBLE const double
#define CCHAR   const char

static void       addToSgMetaData(int *sgMetaDataUsed, int sgMetaDataSize, char sgMetaData[], char format[], ...);
static int        buildSceneGraph(esp_T *ESP);

/***********************************************************************/
/*                                                                     */
/* macros (including those that go along with common.h)                */
/*                                                                     */
/***********************************************************************/

static void *realloc_temp=NULL;            /* used by RALLOC macro */

#define  RED(COLOR)      (float)(COLOR / 0x10000        ) / (float)(255)
#define  GREEN(COLOR)    (float)(COLOR / 0x00100 % 0x100) / (float)(255)
#define  BLUE(COLOR)     (float)(COLOR           % 0x100) / (float)(255)

static int outLevel = 1;


/***********************************************************************/
/*                                                                     */
/*   timLoad - open a tim instance                                     */
/*                                                                     */
/***********************************************************************/

int
timLoad(/*@unused@*/esp_T *ESP,                     /* (in)  pointer to ESP structure */
        /*@unused@*/void  *data)                    /* (in)  component name */
{
    int    status;                      /* (out) return status */

    ROUTINE(timLoad(mitten));

    /* --------------------------------------------------------------- */

    /* hold the UI when executing */
    status = 1;

//cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   timMesg - get command, process, and return response               */
/*                                                                     */
/***********************************************************************/

int
timMesg(esp_T *ESP,                     /* (in)  pointer to ESP structure */
        char  command[])                /* (in)  command */
{
    int    status = EGADS_SUCCESS;      /* (out) return status */

    int    ibody, iface;
    modl_T *MODL;

    ROUTINE(timMesg(mitten));

    /* --------------------------------------------------------------- */

    /* "update|" */
    if        (strncmp(command, "update", 6) == 0) {

        buildSceneGraph(ESP);

    /* "red|" */
    } else if (strncmp(command, "red|", 4) == 0) {

        if (ESP == NULL) {
            printf("WARNING:: not running via serveESP\n");
            goto cleanup;
        } else {
            MODL = ESP->MODL;

            for (ibody = 1; ibody <= MODL->nbody; ibody++) {
                if (MODL->body[ibody].onstack != 1) continue;

                for (iface = 1; iface <= MODL->body[ibody].nface; iface++) {
                    MODL->body[ibody].face[iface].gratt.color = 0x00FF0000;
                }
            }
        }

        buildSceneGraph(ESP);

    /* "green|" */
    } else if (strncmp(command, "green|", 6) == 0) {

        if (ESP == NULL) {
            printf("WARNING:: not running via serveESP\n");
            goto cleanup;
        } else {
            MODL = ESP->MODL;

            for (ibody = 1; ibody <= MODL->nbody; ibody++) {
                if (MODL->body[ibody].onstack != 1) continue;

                for (iface = 1; iface <= MODL->body[ibody].nface; iface++) {
                    MODL->body[ibody].face[iface].gratt.color = 0x0000FF00;
                }
            }
        }

        buildSceneGraph(ESP);

    /* "blue|" */
    } else if (strncmp(command, "blue|", 5) == 0) {

        if (ESP == NULL) {
            printf("WARNING:: not running via serveESP\n");
            goto cleanup;
        } else {
            MODL = ESP->MODL;

            for (ibody = 1; ibody <= MODL->nbody; ibody++) {
                if (MODL->body[ibody].onstack != 1) continue;

                for (iface = 1; iface <= MODL->body[ibody].nface; iface++) {
                    MODL->body[ibody].face[iface].gratt.color = 0x000000FF;
                }
            }
        }

        buildSceneGraph(ESP);
    }

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   timSave - save tim data and close tim instance                    */
/*                                                                     */
/***********************************************************************/

int
timSave(/*@unused@*/esp_T *ESP)                     /* (in)  pointer to TI Mstructure */
{
    int    status = EGADS_SUCCESS;      /* (out) return status */

    ROUTINE(timSave(mitten));

    /* --------------------------------------------------------------- */

//cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   timQuit - close tim instance without saving                       */
/*                                                                     */
/***********************************************************************/

int
timQuit(/*@unused@*/esp_T *ESP,                     /* (in)  pointer to ESP structure */
        /*@unused@*/int   unload)                   /* (in)  flag to unload */
{
    int    status = EGADS_SUCCESS;      /* (out) return status */

    ROUTINE(timQuit(mitten));

    /* --------------------------------------------------------------- */

//cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   buildSceneGraph - make a scene graph for wv                       */
/*                                                                     */
/***********************************************************************/

static int
buildSceneGraph(esp_T  *ESP)
{
    int       status = SUCCESS;         /* return status */

    int       ibody, jbody, iface, iedge, inode, iattr, nattr, atype, alen;
    int       npnt, ipnt, ntri, itri, igprim, nseg, i, k;
    int       attrs, head[3], nitems, nnode, nedge, nface;
    int       oclass, mtype, nchild, *senses;
    int       *segs=NULL, *ivrts=NULL;
    int        npatch2, ipatch, n1, n2, i1, i2, *Tris=NULL;
    CINT      *ptype, *pindx, *tris, *tric, *pvindex, *pbounds;
    float     color[18], *pcolors=NULL, *plotdata=NULL, *segments=NULL, *tuft=NULL;
    double    bigbox[6], box[6], size, xyz_dum[6], axis[18];
    CDOUBLE   *xyz, *uv, *t;
    char      gpname[MAX_STRVAL_LEN], bname[MAX_NAME_LEN];
    ego       ebody, etess, eface, eedge, enode, eref, *echilds;
    ego       *enodes=NULL, *eedges=NULL, *efaces=NULL;

    int       nlist, itype;
    CINT      *tempIlist, *nquad=NULL;
    CDOUBLE   *tempRlist;
    CCHAR     *attrName, *tempClist;

    modl_T    *MODL;
    wvContext *cntxt;
    wvData    items[6];

/* global variables associated with scene graph meta-data */
#define MAX_METADATA_CHUNK 32000
    int       sgMetaDataSize = 0;
    int       sgMetaDataUsed = 0;
    char      *sgMetaData    = NULL;
    char      *sgFocusData   = NULL;

    ROUTINE(buildSceneGraph);

    /* --------------------------------------------------------------- */

    if (ESP == NULL) {
        printf("WARNING:: not running via serveESP\n");
        goto cleanup;
    } else {
        MODL  = ESP->MODL;
        cntxt = ESP->cntxt;
    }

    sgMetaDataSize = MAX_METADATA_CHUNK;

    MALLOC(sgMetaData,  char, sgMetaDataSize);
    MALLOC(sgFocusData, char, MAX_STRVAL_LEN);

    /* set the mutex.  if the mutex wss already set, wait until released */
    SPLINT_CHECK_FOR_NULL(ESP);
    EMP_LockSet(ESP->sgMutex);

    /* remove any graphic primitives that already exist */
    wv_removeAll(cntxt);

    if (MODL == NULL) {
        goto cleanup;
    }

    /* find the values needed to adjust the vertices */
    bigbox[0] = bigbox[1] = bigbox[2] = +HUGEQ;
    bigbox[3] = bigbox[4] = bigbox[5] = -HUGEQ;

    /* use Bodys on stack */
    for (ibody = 1; ibody <= MODL->nbody; ibody++) {
        if (MODL->body[ibody].onstack != 1) continue;

        ebody = MODL->body[ibody].ebody;

        status = EG_getBoundingBox(ebody, box);
        if (status != SUCCESS) {
            SPRINT2(0, "ERROR:: EG_getBoundingBox(%d) -> status=%d", ibody, status);
        }

        if (box[0] < bigbox[0]) bigbox[0] = box[0];
        if (box[1] < bigbox[1]) bigbox[1] = box[1];
        if (box[2] < bigbox[2]) bigbox[2] = box[2];
        if (box[3] > bigbox[3]) bigbox[3] = box[3];
        if (box[4] > bigbox[4]) bigbox[4] = box[4];
        if (box[5] > bigbox[5]) bigbox[5] = box[5];
    }

                                    size = bigbox[3] - bigbox[0];
    if (size < bigbox[4]-bigbox[1]) size = bigbox[4] - bigbox[1];
    if (size < bigbox[5]-bigbox[2]) size = bigbox[5] - bigbox[2];

    ESP->sgFocus[0] = (bigbox[0] + bigbox[3]) / 2;
    ESP->sgFocus[1] = (bigbox[1] + bigbox[4]) / 2;
    ESP->sgFocus[2] = (bigbox[2] + bigbox[5]) / 2;
    ESP->sgFocus[3] = size;

    /* generate the scene graph focus data */
    snprintf(sgFocusData, MAX_STRVAL_LEN-1, "sgFocus|[%20.12e,%20.12e,%20.12e,%20.12e]",
             ESP->sgFocus[0], ESP->sgFocus[1], ESP->sgFocus[2], ESP->sgFocus[3]);

    /* initialize the scene graph meta data */
    sgMetaData[ 0] = '\0';
    sgMetaDataUsed = 0;

    addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "sgData|{");

    /* loop through the Bodys */
    for (ibody = 1; ibody <= MODL->nbody; ibody++) {
        if (MODL->body[ibody].onstack != 1) continue;

        if (MODL->body[ibody].eebody == NULL) {
            ebody = MODL->body[ibody].ebody;

            EG_getBodyTopos(ebody, NULL, NODE, &nnode, &enodes);
            EG_getBodyTopos(ebody, NULL, EDGE, &nedge, &eedges);
            EG_getBodyTopos(ebody, NULL, FACE, &nface, &efaces);
        } else {
            ebody = MODL->body[ibody].eebody;

            EG_getBodyTopos(ebody, NULL, NODE,  &nnode, &enodes);
            EG_getBodyTopos(ebody, NULL, EEDGE, &nedge, &eedges);
            EG_getBodyTopos(ebody, NULL, EFACE, &nface, &efaces);
        }

        /* set up Body name */
        snprintf(bname, MAX_NAME_LEN-1, "Body %d", ibody);

        status = EG_attributeRet(ebody, "_name", &itype, &nlist,
                                 &tempIlist, &tempRlist, &tempClist);
        if (status == SUCCESS && itype == ATTRSTRING) {
            snprintf(bname, MAX_NAME_LEN-1, "%s", tempClist);
        }

        /* check for duplicate Body names */
        for (jbody = 1; jbody < ibody; jbody++) {
            if (MODL->body[jbody].onstack != 1) continue;

            status = EG_attributeRet(MODL->body[jbody].ebody, "_name", &itype, &nlist,
                                     &tempIlist, &tempRlist, &tempClist);
            if (status == SUCCESS && itype == ATTRSTRING) {
                if (strcmp(tempClist, bname) == 0) {
                    SPRINT2(0, "WARNING:: duplicate Body name (%s) found; being changed to \"Body %d\"", bname, ibody);
                    snprintf(bname, MAX_NAME_LEN-1, "Body %d", ibody);
                }
            }
        }

        /* Body info for NodeBody, SheetBody or SolidBody */
        snprintf(gpname, MAX_STRVAL_LEN, "%s", bname);
        status = EG_attributeNum(ebody, &nattr);
        if (status != SUCCESS) {
            SPRINT2(0, "ERROR:: EG_attributeNum(%d) -> status=%d", ibody, status);
        }

        /* add Node to meta data (if there is room) */
        if (nattr > 0) {
            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[", gpname);
        } else {
            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[\"body\",\"%d\"", gpname, ibody);
        }

        for (iattr = 1; iattr <= nattr; iattr++) {
            status = EG_attributeGet(ebody, iattr, &attrName, &itype, &nlist,
                                     &tempIlist, &tempRlist, &tempClist);
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: EG_attributeGet(%d,%d) -> status=%d", ibody, iattr, status);
            }

            if (itype == ATTRCSYS) continue;

            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\",\"", attrName);

            if        (itype == ATTRINT) {
                for (i = 0; i < nlist ; i++) {
                    addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %d", tempIlist[i]);
                }
            } else if (itype == ATTRREAL) {
                for (i = 0; i < nlist ; i++) {
                    addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %f", tempRlist[i]);
                }
            } else if (itype == ATTRSTRING) {
                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %s ", tempClist);
            }

            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\",");
        }
        sgMetaDataUsed--;
        addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "],");

        if (MODL->body[ibody].eetess == NULL) {
            etess = MODL->body[ibody].etess;
        } else {
            etess = MODL->body[ibody].eetess;
        }

        /* loop through the Faces within the current Body */
        for (iface = 1; iface <= nface; iface++) {
            nitems = 0;

            /* name and attributes */
            snprintf(gpname, MAX_STRVAL_LEN-1, "%s Face %d", bname, iface);
            attrs = WV_ON | WV_ORIENTATION;

            status = EG_getQuads(etess, iface,
                                 &npnt, &xyz, &uv, &ptype, &pindx, &npatch2);
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: EG_getQuads(%d,%d) -> status=%d", ibody, iface, status);
            }

            status = EG_attributeRet(etess, ".tessType",
                                     &atype, &alen, &tempIlist, &tempRlist, &tempClist);

            /* new-style Quads */
            if (status == SUCCESS && atype == ATTRSTRING && (
                    strcmp(tempClist, "Quad") == 0 || strcmp(tempClist, "Mixed") == 0)) {
                status = EG_attributeRet(etess, ".mixed",
                                         &atype, &alen, &nquad, &tempRlist, &tempClist);
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: EG_attributeRet(%d,%d) -> status=%d", ibody, iface, status);
                }

                status = EG_getTessFace(etess, iface,
                                        &npnt, &xyz, &uv, &ptype, &pindx,
                                        &ntri, &tris, &tric);
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: EG_getTessFace(%d,%d) -> status=%d", ibody, iface, status);
                }

                /* skip if no Triangles either */
                if (ntri <= 0) continue;

                /* vertices */
                status = wv_setData(WV_REAL64, npnt, (void*)xyz, WV_VERTICES, &(items[nitems]));
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
                }

                wv_adjustVerts(&(items[nitems]), ESP->sgFocus);
                nitems++;

                /* loop through the triangles and build up the segment table */
                nseg = 0;
                for (itri = 0; itri < ntri; itri++) {
                    for (k = 0; k < 3; k++) {
                        if (tric[3*itri+k] < itri+1) {
                            nseg++;
                        }
                    }
                }

                MALLOC(segs, int, 2*nseg);

                /* create segments between Triangles (bias-1) */
                SPLINT_CHECK_FOR_NULL(nquad);

                nseg = 0;
                for (itri = 0; itri < ntri-2*nquad[iface-1]; itri++) {
                    for (k = 0; k < 3; k++) {
                        if (tric[3*itri+k] < itri+1) {
                            segs[2*nseg  ] = tris[3*itri+(k+1)%3];
                            segs[2*nseg+1] = tris[3*itri+(k+2)%3];
                            nseg++;
                        }
                    }
                }

                /* create segments between Triangles (but not within the pair) (bias-1) */
                for (; itri < ntri; itri++) {
                    if (tric[3*itri  ] < itri+2) {
                        segs[2*nseg  ] = tris[3*itri+1];
                        segs[2*nseg+1] = tris[3*itri+2];
                        nseg++;
                    }
                    if (tric[3*itri+1] < itri+2) {
                        segs[2*nseg  ] = tris[3*itri+2];
                        segs[2*nseg+1] = tris[3*itri  ];
                        nseg++;
                    }
                    if (tric[3*itri+2] < itri+2) {
                        segs[2*nseg  ] = tris[3*itri  ];
                        segs[2*nseg+1] = tris[3*itri+1];
                        nseg++;
                    }
                    itri++;

                    if (tric[3*itri  ] < itri) {
                        segs[2*nseg  ] = tris[3*itri+1];
                        segs[2*nseg+1] = tris[3*itri+2];
                        nseg++;
                    }
                    if (tric[3*itri+1] < itri) {
                        segs[2*nseg  ] = tris[3*itri+2];
                        segs[2*nseg+1] = tris[3*itri  ];
                        nseg++;
                    }
                    if (tric[3*itri+2] < itri) {
                        segs[2*nseg  ] = tris[3*itri  ];
                        segs[2*nseg+1] = tris[3*itri+1];
                        nseg++;
                    }
                }

                /* triangles */
                status = wv_setData(WV_INT32, 3*ntri, (void*)tris, WV_INDICES, &(items[nitems]));
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
                }
                nitems++;

            /* patches in old-style Quads */
            } else if (npatch2 > 0) {
                /* vertices */
                status = wv_setData(WV_REAL64, npnt, (void*)xyz, WV_VERTICES, &(items[nitems]));
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
                }

                wv_adjustVerts(&(items[nitems]), ESP->sgFocus);
                nitems++;

                /* loop through the patches and build up the triangle and
                   segment tables (bias-1) */
                ntri = 0;
                nseg = 0;

                for (ipatch = 1; ipatch <= npatch2; ipatch++) {
                    status = EG_getPatch(etess, iface, ipatch, &n1, &n2, &pvindex, &pbounds);
                    if (status != SUCCESS) {
                        SPRINT3(0, "ERROR:: EG_getPatch(%d,%d) -> status=%d\n", ibody, iface, status);
                    }

                    ntri += 2 * (n1-1) * (n2-1);
                    nseg += n1 * (n2-1) + n2 * (n1-1);
                }

                MALLOC(Tris, int, 3*ntri);
                MALLOC(segs, int, 2*nseg);

                ntri = 0;
                nseg = 0;

                for (ipatch = 1; ipatch <= npatch2; ipatch++) {
                    status = EG_getPatch(etess, iface, ipatch, &n1, &n2, &pvindex, &pbounds);
                    if (status != SUCCESS) {
                        SPRINT3(0, "ERROR:: EG_getPatch(%d,%d) -> status=%d\n", ibody, iface, status);
                    }

                    for (i2 = 1; i2 < n2; i2++) {
                        for (i1 = 1; i1 < n1; i1++) {
                            Tris[3*ntri  ] = pvindex[(i1-1)+n1*(i2-1)];
                            Tris[3*ntri+1] = pvindex[(i1  )+n1*(i2-1)];
                            Tris[3*ntri+2] = pvindex[(i1  )+n1*(i2  )];
                            ntri++;

                            Tris[3*ntri  ] = pvindex[(i1  )+n1*(i2  )];
                            Tris[3*ntri+1] = pvindex[(i1-1)+n1*(i2  )];
                            Tris[3*ntri+2] = pvindex[(i1-1)+n1*(i2-1)];
                            ntri++;
                        }
                    }

                    for (i2 = 0; i2 < n2; i2++) {
                        for (i1 = 1; i1 < n1; i1++) {
                            segs[2*nseg  ] = pvindex[(i1-1)+n1*(i2)];
                            segs[2*nseg+1] = pvindex[(i1  )+n1*(i2)];
                            nseg++;
                        }
                    }

                    for (i1 = 0; i1 < n1; i1++) {
                        for (i2 = 1; i2 < n2; i2++) {
                            segs[2*nseg  ] = pvindex[(i1)+n1*(i2-1)];
                            segs[2*nseg+1] = pvindex[(i1)+n1*(i2  )];
                            nseg++;
                        }
                    }
                }

                /* two triangles for rendering each quadrilateral */
                status = wv_setData(WV_INT32, 3*ntri, (void*)Tris, WV_INDICES, &(items[nitems]));
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
                }
                nitems++;

                FREE(Tris);

            /* Triangles */
            } else {
                status = EG_getTessFace(etess, iface,
                                        &npnt, &xyz, &uv, &ptype, &pindx,
                                        &ntri, &tris, &tric);
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: EG_getTessFace(%d,%d) -> status=%d", ibody, iface, status);
                }

                /* skip if no Triangles either */
                if (ntri <= 0) continue;

                /* vertices */
                status = wv_setData(WV_REAL64, npnt, (void*)xyz, WV_VERTICES, &(items[nitems]));
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
                }

                wv_adjustVerts(&(items[nitems]), ESP->sgFocus);
                nitems++;

                /* loop through the triangles and build up the segment table (bias-1) */
                nseg = 0;
                for (itri = 0; itri < ntri; itri++) {
                    for (k = 0; k < 3; k++) {
                        if (tric[3*itri+k] < itri+1) {
                            nseg++;
                        }
                    }
                }

                MALLOC(segs, int, 2*nseg);

                nseg = 0;
                for (itri = 0; itri < ntri; itri++) {
                    for (k = 0; k < 3; k++) {
                        if (tric[3*itri+k] < itri+1) {
                            segs[2*nseg  ] = tris[3*itri+(k+1)%3];
                            segs[2*nseg+1] = tris[3*itri+(k+2)%3];
                            nseg++;
                        }
                    }
                }

                /* triangles */
                status = wv_setData(WV_INT32, 3*ntri, (void*)tris, WV_INDICES, &(items[nitems]));
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
                }
                nitems++;
            }

            /* constant triangle colors */
            if (MODL->body[ibody].eebody == NULL) {
                color[0] = RED(  MODL->body[ibody].face[iface].gratt.color);
                color[1] = GREEN(MODL->body[ibody].face[iface].gratt.color);
                color[2] = BLUE( MODL->body[ibody].face[iface].gratt.color);
            } else {
                color[0] = 0.75;
                color[1] = 0.75;
                color[2] = 1.00;
            }
            status = wv_setData(WV_REAL32, 1, (void*)color, WV_COLORS, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
            }
            nitems++;

            /* triangle backface color */
            if (MODL->body[ibody].eebody == NULL) {
                color[0] = RED(  MODL->body[ibody].face[iface].gratt.bcolor);
                color[1] = GREEN(MODL->body[ibody].face[iface].gratt.bcolor);
                color[2] = BLUE( MODL->body[ibody].face[iface].gratt.bcolor);
            } else {
                color[0] = 0.50;
                color[1] = 0.50;
                color[2] = 0.50;
            }
            status = wv_setData(WV_REAL32, 1, (void*)color, WV_BCOLOR, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
            }
            nitems++;

            /* segment indices */
            status = wv_setData(WV_INT32, 2*nseg, (void*)segs, WV_LINDICES, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
            }
            nitems++;

            FREE(segs);

            /* segment colors */
            if (MODL->body[ibody].eebody == NULL) {
                color[0] = RED(  MODL->body[ibody].face[iface].gratt.mcolor);
                color[1] = GREEN(MODL->body[ibody].face[iface].gratt.mcolor);
                color[2] = BLUE( MODL->body[ibody].face[iface].gratt.mcolor);
            } else {
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
            }
            status = wv_setData(WV_REAL32, 1, (void*)color, WV_LCOLOR, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iface, status);
            }
            nitems++;

            /* make graphic primitive */
            igprim = wv_addGPrim(cntxt, gpname, WV_TRIANGLE, attrs, nitems, items);
            if (igprim < 0) {
                SPRINT2(0, "ERROR:: wv_addGPrim(%s) -> igprim=%d", gpname, igprim);
            } else {
                SPLINT_CHECK_FOR_NULL(cntxt->gPrims);

                cntxt->gPrims[igprim].lWidth = 1.0;
            }

            /* get Attributes for the Face */
            SPLINT_CHECK_FOR_NULL(efaces);
            eface  = efaces[iface-1];
            status = EG_attributeNum(eface, &nattr);
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: EG_attributeNum(%d,%d) -> status=%d", ibody, iface, status);
            }

            /* add Face to meta data (if there is room) */
            if (nattr > 0) {
                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[",  gpname);
            } else {
                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[]", gpname);
            }

            for (iattr = 1; iattr <= nattr; iattr++) {
                status = EG_attributeGet(eface, iattr, &attrName, &itype, &nlist,
                                         &tempIlist, &tempRlist, &tempClist);
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: EG_attributeGet(%d,%d) -> status=%d", ibody, iface, status);
                }

                if (itype == ATTRCSYS) continue;

                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\",\"", attrName);

                if        (itype == ATTRINT) {
                    for (i = 0; i < nlist ; i++) {
                        addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %d", tempIlist[i]);
                    }
                } else if (itype == ATTRREAL) {
                    for (i = 0; i < nlist ; i++) {
                        addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %f", tempRlist[i]);
                    }
                } else if (itype == ATTRSTRING) {
                    addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %s ", tempClist);
                }

                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\",");
            }
            sgMetaDataUsed--;
            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "],");
        }

        /* loop through the Edges within the current Body */
        for (iedge = 1; iedge <= nedge; iedge++) {
            nitems = 0;

            /* skip edge if this is a NodeBody */
            if (MODL->body[ibody].botype == OCSM_NODE_BODY) continue;

            status = EG_getTessEdge(etess, iedge, &npnt, &xyz, &t);
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: EG_getTessEdge(%d,%d) -> status=%d", ibody, iedge, status);
            }

            /* name and attributes */
            snprintf(gpname, MAX_STRVAL_LEN-1, "%s Edge %d", bname, iedge);
            attrs = WV_ON;

            /* vertices */
            status = wv_setData(WV_REAL64, npnt, (void*)xyz, WV_VERTICES, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iedge, status);
            }

            wv_adjustVerts(&(items[nitems]), ESP->sgFocus);
            nitems++;

            /* segments (bias-1) */
            MALLOC(ivrts, int, 2*(npnt-1));

            for (ipnt = 0; ipnt < npnt-1; ipnt++) {
                ivrts[2*ipnt  ] = ipnt + 1;
                ivrts[2*ipnt+1] = ipnt + 2;
            }

            status = wv_setData(WV_INT32, 2*(npnt-1), (void*)ivrts, WV_INDICES, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iedge, status);
            }
            nitems++;

            FREE(ivrts);

            /* line colors */
            if (MODL->body[ibody].eebody == NULL) {
                color[0] = RED(  MODL->body[ibody].edge[iedge].gratt.color);
                color[1] = GREEN(MODL->body[ibody].edge[iedge].gratt.color);
                color[2] = BLUE( MODL->body[ibody].edge[iedge].gratt.color);
            } else {
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
            }
            status = wv_setData(WV_REAL32, 1, (void*)color, WV_COLORS, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iedge, status);
            }
            nitems++;

            /* points */
            MALLOC(ivrts, int, npnt);

            for (ipnt = 0; ipnt < npnt; ipnt++) {
                ivrts[ipnt] = ipnt + 1;
            }

            status = wv_setData(WV_INT32, npnt, (void*)ivrts, WV_PINDICES, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iedge, status);
            }
            nitems++;

            FREE(ivrts);

            /* point colors */
            if (MODL->body[ibody].eebody == NULL) {
                color[0] = RED(  MODL->body[ibody].edge[iedge].gratt.mcolor);
                color[1] = GREEN(MODL->body[ibody].edge[iedge].gratt.mcolor);
                color[2] = BLUE( MODL->body[ibody].edge[iedge].gratt.mcolor);
            } else {
                color[0] = 0;
                color[1] = 0;
                color[2 ]= 0;
            }
            status = wv_setData(WV_REAL32, 1, (void*)color, WV_PCOLOR, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iedge, status);
            }
            nitems++;

            /* make graphic primitive */
            igprim = wv_addGPrim(cntxt, gpname, WV_LINE, attrs, nitems, items);
            if (igprim < 0) {
                SPRINT2(0, "ERROR:: wv_addGPrim(%s) -> igprim=%d", gpname, igprim);
            } else {
                SPLINT_CHECK_FOR_NULL(cntxt->gPrims);

                /* make line width 2 (does not work for ANGLE) */
                cntxt->gPrims[igprim].lWidth = 2.0;

                /* make point size 5 */
                cntxt->gPrims[igprim].pSize  = 5.0;

                /* add arrow heads (requires that WV_ORIENTATION be set above) */
                head[0] = npnt - 1;
                status = wv_addArrowHeads(cntxt, igprim, 0.10/ESP->sgFocus[3], 1, head);
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: wv_addArrowHeads(%d,%d) -> status=%d", ibody, iedge, status);
                }
            }

            SPLINT_CHECK_FOR_NULL(eedges);
            eedge  = eedges[iedge-1];
            status = EG_attributeNum(eedge, &nattr);
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: EG_attributeNum(%d,%d) -> status=%d", ibody, iedge, status);
            }

            /* add Edge to meta data (if there is room) */
            if (nattr > 0) {
                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[",  gpname);
            } else {
                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[]", gpname);
            }

            for (iattr = 1; iattr <= nattr; iattr++) {
                status = EG_attributeGet(eedge, iattr, &attrName, &itype, &nlist,
                                         &tempIlist, &tempRlist, &tempClist);
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: EG_attributeGet(%d,%d) -> status=%d", ibody, iedge, status);
                }

                if (itype == ATTRCSYS) continue;

                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\",\"", attrName);

                if        (itype == ATTRINT) {
                    for (i = 0; i < nlist ; i++) {
                        addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %d", tempIlist[i]);
                    }
                } else if (itype == ATTRREAL) {
                    for (i = 0; i < nlist ; i++) {
                        addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %f", tempRlist[i]);
                    }
                } else if (itype == ATTRSTRING) {
                    addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %s ", tempClist);
                }

                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\",");
            }
            sgMetaDataUsed--;
            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "],");
        }

        /* loop through the Nodes within the current Body */
        for (inode = 1; inode <= nnode; inode++) {
            nitems = 0;

            /* name and attributes */
            snprintf(gpname, MAX_STRVAL_LEN-1, "%s Node %d", bname, inode);

            /* default for NodeBodys is to turn the Node on */
            if (MODL->body[ibody].botype == OCSM_NODE_BODY) {
                attrs = WV_ON;
            } else {
                attrs = 0;
            }

            SPLINT_CHECK_FOR_NULL(enodes);
            enode  = enodes[inode-1];

            /* two copies of vertex (to get around possible wv limitation) */
            status = EG_getTopology(enode, &eref, &oclass, &mtype,
                                    xyz_dum, &nchild, &echilds, &senses);
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: EG_getTopology(%d,%d) -> status=%d", ibody, inode, status);
            }

            xyz_dum[3] = xyz_dum[0];
            xyz_dum[4] = xyz_dum[1];
            xyz_dum[5] = xyz_dum[2];

            status = wv_setData(WV_REAL64, 2, (void*)xyz_dum, WV_VERTICES, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iedge, status);
            }

            wv_adjustVerts(&(items[nitems]), ESP->sgFocus);
            nitems++;

            /* node color (black) */
            color[0] = 0;   color[1] = 0;   color[2] = 0;

            if (MODL->body[ibody].eebody == NULL) {
                color[0] = RED(  MODL->body[ibody].node[inode].gratt.color);
                color[1] = GREEN(MODL->body[ibody].node[inode].gratt.color);
                color[2] = BLUE( MODL->body[ibody].node[inode].gratt.color);
            } else {
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
            }
            status = wv_setData(WV_REAL32, 1, (void*)color, WV_COLORS, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: wv_setData(%d,%d) -> status=%d", ibody, iedge, status);
            }
            nitems++;

            /* make graphic primitive */
            igprim = wv_addGPrim(cntxt, gpname, WV_POINT, attrs, nitems, items);
            if (igprim < 0) {
                SPRINT2(0, "ERROR:: wv_addGPrim(%s) -> igprim=%d", gpname, igprim);
            } else {
                SPLINT_CHECK_FOR_NULL(cntxt->gPrims);

                cntxt->gPrims[igprim].pSize = 6.0;
            }

            status = EG_attributeNum(enode, &nattr);
            if (status != SUCCESS) {
                SPRINT3(0, "ERROR:: EG_attributeNum(%d,%d) -> status=%d", ibody, iedge, status);
            }

            /* add Node to meta data (if there is room) */
            if (nattr > 0) {
                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[",  gpname);
            } else {
                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[]", gpname);
            }

            for (iattr = 1; iattr <= nattr; iattr++) {
                status = EG_attributeGet(enode, iattr, &attrName, &itype, &nlist,
                                         &tempIlist, &tempRlist, &tempClist);
                if (status != SUCCESS) {
                    SPRINT3(0, "ERROR:: EG_attributeGet(%d,%d) -> status=%d", ibody, iedge, status);
                }

                if (itype == ATTRCSYS) continue;

                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\",\"", attrName);

                if        (itype == ATTRINT) {
                    for (i = 0; i < nlist ; i++) {
                        addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %d", tempIlist[i]);
                    }
                } else if (itype == ATTRREAL) {
                    for (i = 0; i < nlist ; i++) {
                        addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %f", tempRlist[i]);
                    }
                } else if (itype == ATTRSTRING) {
                    addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, " %s ", tempClist);
                }

                addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\",");
            }
            sgMetaDataUsed--;
            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "],");
        }

        /* loop through the Csystems associated with the current Body */
        status = EG_attributeNum(ebody, &nattr);
        if (status != SUCCESS) {
            SPRINT2(0, "ERROR:: EG_attributeNum(%d) -> status=%d", ibody, status);
        }

        for (iattr = 1; iattr <= nattr; iattr++) {
            nitems = 0;

            status = EG_attributeGet(ebody, iattr, &attrName, &itype, &nlist,
                                     &tempIlist, &tempRlist, &tempClist);
            if (status != SUCCESS) {
                SPRINT1(0, "ERROR:: EG_attributeGet -> status=%d", status);
            }

            if (itype != ATTRCSYS) continue;

            /* name and attributes */
            snprintf(gpname, MAX_STRVAL_LEN-1, "%s Csys %s", bname, attrName);
            attrs = WV_ON | WV_SHADING | WV_ORIENTATION;

            /* vertices */
            axis[ 0] = tempRlist[nlist  ];
            axis[ 1] = tempRlist[nlist+1];
            axis[ 2] = tempRlist[nlist+2];
            axis[ 3] = tempRlist[nlist  ] + tempRlist[nlist+ 3];
            axis[ 4] = tempRlist[nlist+1] + tempRlist[nlist+ 4];
            axis[ 5] = tempRlist[nlist+2] + tempRlist[nlist+ 5];
            axis[ 6] = tempRlist[nlist  ];
            axis[ 7] = tempRlist[nlist+1];
            axis[ 8] = tempRlist[nlist+2];
            axis[ 9] = tempRlist[nlist  ] + tempRlist[nlist+ 6];
            axis[10] = tempRlist[nlist+1] + tempRlist[nlist+ 7];
            axis[11] = tempRlist[nlist+2] + tempRlist[nlist+ 8];
            axis[12] = tempRlist[nlist  ];
            axis[13] = tempRlist[nlist+1];
            axis[14] = tempRlist[nlist+2];
            axis[15] = tempRlist[nlist  ] + tempRlist[nlist+ 9];
            axis[16] = tempRlist[nlist+1] + tempRlist[nlist+10];
            axis[17] = tempRlist[nlist+2] + tempRlist[nlist+11];

            status = wv_setData(WV_REAL64, 6, (void*)axis, WV_VERTICES, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT1(0, "ERROR:: wv_setData(axis) -> status=%d", status);
            }

            wv_adjustVerts(&(items[nitems]), ESP->sgFocus);
            nitems++;

            /* line colors (multiple colors requires that WV_SHADING be set above) */
            color[ 0] = 1;   color[ 1] = 0;   color[ 2] = 0;
            color[ 3] = 1;   color[ 4] = 0;   color[ 5] = 0;
            color[ 6] = 0;   color[ 7] = 1;   color[ 8] = 0;
            color[ 9] = 0;   color[10] = 1;   color[11] = 0;
            color[12] = 0;   color[13] = 0;   color[14] = 1;
            color[15] = 0;   color[16] = 0;   color[17] = 1;
            status = wv_setData(WV_REAL32, 6, (void*)color, WV_COLORS, &(items[nitems]));
            if (status != SUCCESS) {
                SPRINT1(0, "ERROR:: wv_setData(color) -> status=%d", status);
            }
            nitems++;

            /* make graphic primitive */
            igprim = wv_addGPrim(cntxt, gpname, WV_LINE, attrs, nitems, items);
            if (igprim < 0) {
                SPRINT2(0, "ERROR:: wv_addGPrim(%s) -> igprim=%d", gpname, igprim);
            } else {
                SPLINT_CHECK_FOR_NULL(cntxt->gPrims);

                /* make line width 1 */
                cntxt->gPrims[igprim].lWidth = 1.0;

                /* add arrow heads (requires that WV_ORIENTATION be set above) */
                head[0] = 1;
                status = wv_addArrowHeads(cntxt, igprim, 0.10/ESP->sgFocus[3], 1, head);
                if (status != SUCCESS) {
                    SPRINT1(0, "ERROR:: wv_addArrowHeads -> status=%d", status);
                }
            }

            /* add Csystem to meta data (if there is room) */
            addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "\"%s\":[],", gpname);
        }
    }

    /* axes */
    nitems = 0;

    /* name and attributes */
    snprintf(gpname, MAX_STRVAL_LEN-1, "Axes");
    attrs = 0;

    /* vertices */
    axis[ 0] = MIN(2*bigbox[0]-bigbox[3], 0);   axis[ 1] = 0;   axis[ 2] = 0;
    axis[ 3] = MAX(2*bigbox[3]-bigbox[0], 0);   axis[ 4] = 0;   axis[ 5] = 0;

    axis[ 6] = 0;   axis[ 7] = MIN(2*bigbox[1]-bigbox[4], 0);   axis[ 8] = 0;
    axis[ 9] = 0;   axis[10] = MAX(2*bigbox[4]-bigbox[1], 0);   axis[11] = 0;

    axis[12] = 0;   axis[13] = 0;   axis[14] = MIN(2*bigbox[2]-bigbox[5], 0);
    axis[15] = 0;   axis[16] = 0;   axis[17] = MAX(2*bigbox[5]-bigbox[2], 0);
    status = wv_setData(WV_REAL64, 6, (void*)axis, WV_VERTICES, &(items[nitems]));
    if (status != SUCCESS) {
        SPRINT1(0, "ERROR:: wv_setData(axis) -> status=%d", status);
    }

    wv_adjustVerts(&(items[nitems]), ESP->sgFocus);
    nitems++;

    /* line color */
    color[0] = 0.7;   color[1] = 0.7;   color[2] = 0.7;
    status = wv_setData(WV_REAL32, 1, (void*)color, WV_COLORS, &(items[nitems]));
    if (status != SUCCESS) {
        SPRINT1(0, "ERROR:: wv_setData(color) -> status=%d", status);
    }
    nitems++;

    /* make graphic primitive */
    igprim = wv_addGPrim(cntxt, gpname, WV_LINE, attrs, nitems, items);
    if (igprim < 0) {
        SPRINT2(0, "ERROR:: wv_addGPrim(%s) -> igprim=%d", gpname, igprim);
    } else {
        SPLINT_CHECK_FOR_NULL(cntxt->gPrims);

        cntxt->gPrims[igprim].lWidth = 1.0;
    }

    /* finish the scene graph meta data */
    sgMetaDataUsed--;
    addToSgMetaData(&sgMetaDataUsed, sgMetaDataSize, sgMetaData, "}");

cleanup:

    /* release the mutex so that another thread can access the scene graph */
    if (ESP != NULL) {
        EMP_LockRelease(ESP->sgMutex);
    }

    if (enodes != NULL) EG_free(enodes);
    if (eedges != NULL) EG_free(eedges);
    if (efaces != NULL) EG_free(efaces);

    FREE(plotdata);
    FREE(pcolors );
    FREE(segments);
    FREE(segs    );
    FREE(ivrts   );
    FREE(tuft    );
    FREE(Tris    );

    FREE(sgFocusData);

    return status;
}


/***********************************************************************/
/*                                                                     */
/*   addToSgMetaData - add text to sgMetaData (with buffer length protection) */
/*                                                                     */
/***********************************************************************/

static void
addToSgMetaData(int   *sgMetaDataUsed,  /* (both)SG data used so far */
                int    sgMetaDataSize,  /* (in)  size of SG meta data */
                char   sgMetaData[],    /* (in)  SG meta data */
                char   format[],        /* (in)  format specifier */
                 ...)                   /* (in)  variable arguments */
{
    int      status=SUCCESS, newchars;

    va_list  args;

    ROUTINE(addToSgMetaData);

    /* --------------------------------------------------------------- */

    /* make sure sgMetaData is big enough */
    if ((*sgMetaDataUsed)+1024 >= sgMetaDataSize) {
        sgMetaDataSize += 1024 + MAX_METADATA_CHUNK;
        RALLOC(sgMetaData, char, sgMetaDataSize);
    }

    /* set up the va structure */
    va_start(args, format);

    newchars = vsnprintf(sgMetaData+(*sgMetaDataUsed), 1024, format, args);
    (*sgMetaDataUsed) += newchars;

    /* clean up the va structure */
    va_end(args);

cleanup:
    if (status != SUCCESS) {
        SPRINT0(0, "ERROR:: sgMetaData could not be increased");
    }

    return;
}
