/* Common data transfer functions to be used within an AIM */

#include "math.h"

#include "aimUtil.h"

#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


/* create a nodal triangle element type */
int aim_nodalTriangleType(capsEleType *eletype)
{
  int status;

  if (eletype == NULL) return CAPS_NULLOBJ;

  status = EGADS_MALLOC;

  eletype->nref  = 3;
  eletype->ndata = 0;            /* data at geom reference positions */
  eletype->ntri  = 1;
  eletype->nmat  = 0;            /* match points at geom ref positions */
  eletype->gst   = NULL;
  eletype->dst   = NULL;
  eletype->matst = NULL;
  eletype->tris  = NULL;

  /*  t
      ^
      |
      3
      | \
      |   \
      |     \
      1-------2---> s
  */

  eletype->tris    = (int *) EG_alloc(3*eletype->ntri*sizeof(int));
  if (eletype->tris == NULL) goto cleanup;
  eletype->tris[0] = 1;
  eletype->tris[1] = 2;
  eletype->tris[2] = 3;

  eletype->gst    = (double *) EG_alloc(2*eletype->nref*sizeof(double));
  if (eletype->gst == NULL) goto cleanup;
  eletype->gst[0] = 0.0;    /* s = 0, t = 0 */
  eletype->gst[1] = 0.0;
  eletype->gst[2] = 1.0;    /* s = 1, t = 0 */
  eletype->gst[3] = 0.0;
  eletype->gst[4] = 0.0;    /* s = 0, t = 1 */
  eletype->gst[5] = 1.0;

  return CAPS_SUCCESS;

cleanup:
  printf(" aimTransferUtil/aim_NodalTriangleType: malloc error!\n");

  EG_free(eletype->gst);
  eletype->gst = NULL;
  EG_free(eletype->dst);
  eletype->dst = NULL;
  EG_free(eletype->matst);
  eletype->matst = NULL;
  EG_free(eletype->tris);
  eletype->tris = NULL;

  return status;
}


/* create a nodal quadrilateral element type */
int aim_nodalQuadType(capsEleType *eletype)
{
  int status;

  if (eletype == NULL) return CAPS_NULLOBJ;

  status = EGADS_MALLOC;

  eletype->nref  = 4;
  eletype->ndata = 0;            /* data at geom reference positions */
  eletype->ntri  = 2;
  eletype->nmat  = 0;            /* match points at geom ref positions */
  eletype->gst   = NULL;
  eletype->dst   = NULL;
  eletype->matst = NULL;
  eletype->tris  = NULL;

  /*  t
      ^
      |
      4-------3
      |     / |
      |   /   |
      | /     |
      1-------2---> s
  */

  eletype->tris    = (int *) EG_alloc(3*eletype->ntri*sizeof(int));
  if (eletype->tris == NULL) goto cleanup;
  eletype->tris[0] = 1;
  eletype->tris[1] = 2;
  eletype->tris[2] = 3;

  eletype->tris[3] = 3;
  eletype->tris[4] = 4;
  eletype->tris[5] = 1;

  eletype->gst    = (double *) EG_alloc(2*eletype->nref*sizeof(double));
  if (eletype->gst == NULL) goto cleanup;
  eletype->gst[0] = 0.0;    /* s = 0, t = 0 */
  eletype->gst[1] = 0.0;
  eletype->gst[2] = 1.0;    /* s = 1, t = 0 */
  eletype->gst[3] = 0.0;
  eletype->gst[4] = 1.0;    /* s = 1, t = 1 */
  eletype->gst[5] = 1.0;
  eletype->gst[6] = 0.0;    /* s = 0, t = 1 */
  eletype->gst[7] = 1.0;

  return CAPS_SUCCESS;

cleanup:
  printf(" aimTransferUtil/aim_nodalQuadType: malloc error!\n");

  EG_free(eletype->gst);
  eletype->gst = NULL;
  EG_free(eletype->dst);
  eletype->dst = NULL;
  EG_free(eletype->matst);
  eletype->matst = NULL;
  EG_free(eletype->tris);
  eletype->tris = NULL;

  return status;
}


/* Create element type for cell centered triangle */
int aim_cellTriangleType(capsEleType *eletype)
{
  int status;

  if (eletype == NULL) return CAPS_NULLOBJ;

  status = EGADS_MALLOC;

  eletype->nref  = 3;
  eletype->ndata = 1;            /* data at 1 reference positions */
  eletype->ntri  = 1;
  eletype->nmat  = 1;            /* match points at 1 ref positions */
  eletype->gst   = NULL;
  eletype->dst   = NULL;
  eletype->matst = NULL;
  eletype->tris  = NULL;

  /*  t
      ^
      |
      3
      | \
      |   \
      |     \
      1-------2---> s
  */

  eletype->tris    = (int *) EG_alloc(3*eletype->ntri*sizeof(int));
  if (eletype->tris == NULL) goto cleanup;
  eletype->tris[0] = 1;
  eletype->tris[1] = 2;
  eletype->tris[2] = 3;

  eletype->gst    = (double *) EG_alloc(2*eletype->nref*sizeof(double));
  if (eletype->gst == NULL) goto cleanup;
  eletype->gst[0] = 0.0;    /* s = 0, t = 0 */
  eletype->gst[1] = 0.0;
  eletype->gst[2] = 1.0;    /* s = 1, t = 0 */
  eletype->gst[3] = 0.0;
  eletype->gst[4] = 0.0;    /* s = 0, t = 1 */
  eletype->gst[5] = 1.0;

  /* reference location of the data */
  eletype->dst    = (double *) EG_alloc(2*eletype->ndata*sizeof(double));
  if (eletype->dst == NULL) goto cleanup;
  eletype->dst[0] = 1./3.;
  eletype->dst[1] = 1./3.;

  eletype->matst    = (double *) EG_alloc(2*eletype->ndata*sizeof(double));
  if (eletype->matst == NULL) goto cleanup;
  eletype->matst[0] = 1./3.;
  eletype->matst[1] = 1./3.;

  return CAPS_SUCCESS;

cleanup:
  printf(" aimTransferUtil/aim_CellTriangleType: malloc error!\n");

  EG_free(eletype->gst);
  eletype->gst = NULL;
  EG_free(eletype->dst);
  eletype->dst = NULL;
  EG_free(eletype->matst);
  eletype->matst = NULL;
  EG_free(eletype->tris);
  eletype->tris = NULL;

  return status;
}


/* Create element type for cell centered quadrilateral */
int aim_cellQuadType(capsEleType *eletype)
{
  int status;

  if (eletype == NULL) return CAPS_NULLOBJ;

  status = EGADS_MALLOC;

  eletype->nref  = 4;
  eletype->ndata = 1;            /* data at 1 reference positions */
  eletype->ntri  = 2;
  eletype->nmat  = 1;            /* match points at 1 ref positions */
  eletype->gst   = NULL;
  eletype->dst   = NULL;
  eletype->matst = NULL;
  eletype->tris  = NULL;

  /*  t
      ^
      |
      4-------3
      |     / |
      |   /   |
      | /     |
      1-------2---> s
  */

  eletype->tris    = (int *) EG_alloc(3*eletype->ntri*sizeof(int));
  if (eletype->tris == NULL) goto cleanup;
  eletype->tris[0] = 1;
  eletype->tris[1] = 2;
  eletype->tris[2] = 3;

  eletype->tris[3] = 3;
  eletype->tris[4] = 4;
  eletype->tris[5] = 1;

  eletype->gst    = (double *) EG_alloc(2*eletype->nref*sizeof(double));
  if (eletype->gst == NULL) goto cleanup;
  eletype->gst[0] = 0.0;    /* s = 0, t = 0 */
  eletype->gst[1] = 0.0;
  eletype->gst[2] = 1.0;    /* s = 1, t = 0 */
  eletype->gst[3] = 0.0;
  eletype->gst[4] = 1.0;    /* s = 1, t = 1 */
  eletype->gst[5] = 1.0;
  eletype->gst[6] = 0.0;    /* s = 0, t = 1 */
  eletype->gst[7] = 1.0;

  /* reference location of the data */
  eletype->dst    = (double *) EG_alloc(2*eletype->ndata*sizeof(double));
  if (eletype->dst == NULL) goto cleanup;
  eletype->dst[0] = 1./2.;
  eletype->dst[1] = 1./2.;

  eletype->matst    = (double *) EG_alloc(2*eletype->ndata*sizeof(double));
  if (eletype->matst == NULL) goto cleanup;
  eletype->matst[0] = 1./2.;
  eletype->matst[1] = 1./2.;

  return CAPS_SUCCESS;

cleanup:
  printf(" aimTransferUtil/aim_CellTriangleType: malloc error!\n");

  EG_free(eletype->gst);
  eletype->gst = NULL;
  EG_free(eletype->dst);
  eletype->dst = NULL;
  EG_free(eletype->matst);
  eletype->matst = NULL;
  EG_free(eletype->tris);
  eletype->tris = NULL;

  return status;
}


/* free element type  */
void aim_freeEleType(capsEleType *eletype)
{
  eletype->nref  = 0;
  eletype->ndata = 0;
  eletype->ntri  = 0;
  eletype->nmat  = 0;

  EG_free(eletype->gst);
  eletype->gst = NULL;
  EG_free(eletype->dst);
  eletype->dst = NULL;
  EG_free(eletype->matst);
  eletype->matst = NULL;
  EG_free(eletype->tris);
  eletype->tris = NULL;
}


/* free a discretization object except for the aim ptrm */
int aim_FreeDiscr(capsDiscr *discr)
{
  int i;

  /* free up this capsDiscr */
  if (discr->mapping != NULL) EG_free(discr->mapping);
  if (discr->verts   != NULL) EG_free(discr->verts);
  if (discr->celem   != NULL) EG_free(discr->celem);
  if (discr->types   != NULL) {
    for (i = 0; i < discr->nTypes; i++) {
      aim_freeEleType(discr->types + i);
    }
    EG_free(discr->types);
  }
  if (discr->elems   != NULL) EG_free(discr->elems);
  if (discr->dtris   != NULL) EG_free(discr->dtris);
  if (discr->ptrm    != NULL) EG_free(discr->ptrm);

  discr->nPoints  = 0;
  discr->mapping  = NULL;
  discr->nVerts   = 0;
  discr->verts    = NULL;
  discr->celem    = NULL;
  discr->nTypes   = 0;
  discr->types    = NULL;
  discr->nElems   = 0;
  discr->elems    = NULL;
  discr->nDtris   = 0;
  discr->dtris    = NULL;

  /* aim must free discr->ptrm and set it to null */
  if (discr->ptrm != NULL) {
    printf("aimTransferUtil/aim_FreeDiscr: discr->ptrm is not NULL. aim must free discr->ptrm!\n");
    return CAPS_NULLVALUE;
  }

  return CAPS_SUCCESS;
}


/* Use newton's method to solve for the quadrilaterly reference coordiantes */
static int invEvaluationQuad(const double uvs[], /* (in) uvs that support the geom
                                                    (2*npts in length) */
                             const double uv[],  /* (in) the uv position to get st */
                             const int    in[],  /* (in) grid indices */
                                   double st[])  /* (out) weighting */
{

  int    i;
  double idet, delta, d, du[2], dv[2], duv[2], dst[2], uvx[2];

  delta  = 100.0;
  for (i = 0; i < 20; i++) {
    uvx[0] = (1.0-st[0])*((1.0-st[1])*uvs[2*in[0]  ]  +
                               st[1] *uvs[2*in[3]  ]) +
                  st[0] *((1.0-st[1])*uvs[2*in[1]  ]  +
                               st[1] *uvs[2*in[2]  ]);
    uvx[1] = (1.0-st[0])*((1.0-st[1])*uvs[2*in[0]+1]  +
                               st[1] *uvs[2*in[3]+1]) +
                  st[0] *((1.0-st[1])*uvs[2*in[1]+1]  +
                               st[1] *uvs[2*in[2]+1]);
    du[0]  = (1.0-st[1])*(uvs[2*in[1]  ] - uvs[2*in[0]  ]) +
                  st[1] *(uvs[2*in[2]  ] - uvs[2*in[3]  ]);
    du[1]  = (1.0-st[0])*(uvs[2*in[3]  ] - uvs[2*in[0]  ]) +
                  st[0] *(uvs[2*in[2]  ] - uvs[2*in[1]  ]);
    dv[0]  = (1.0-st[1])*(uvs[2*in[1]+1] - uvs[2*in[0]+1]) +
                  st[1] *(uvs[2*in[2]+1] - uvs[2*in[3]+1]);
    dv[1]  = (1.0-st[0])*(uvs[2*in[3]+1] - uvs[2*in[0]+1]) +
                  st[0] *(uvs[2*in[2]+1] - uvs[2*in[1]+1]);
    duv[0] = uv[0] - uvx[0];
    duv[1] = uv[1] - uvx[1];
    idet   = du[0]*dv[1] - du[1]*dv[0];
    if (idet == 0.0) break;
    dst[0] = (dv[1]*duv[0] - du[1]*duv[1])/idet;
    dst[1] = (du[0]*duv[1] - dv[0]*duv[0])/idet;
    d      = sqrt(dst[0]*dst[0] + dst[1]*dst[1]);
    if (d >= delta) break;
    delta  = d;
    st[0] += dst[0];
    st[1] += dst[1];
    if (delta < 1.e-8) break;
  }

  if (delta < 1.e-8) return CAPS_SUCCESS;

  return CAPS_NOTFOUND;
}


/* locate an element within the trianglution of an element */
int aim_locateElement(capsDiscr *discr, double *params, double *param,
                      int *eIndex, double *bary)
{
  int         i, j, k, in[4], itri[3], status, itsmall, ismall;
  double      we[3], w, smallw = -1.e300;
  capsEleType *eletype;

  if (discr == NULL) return CAPS_NULLOBJ;

  for (itsmall = ismall = i = 0; i < discr->nElems; i++) {
    eletype = discr->types + discr->elems[i].tIndex-1;
    for (j = 0; j < eletype->ntri; j++) {

      itri[0] = eletype->tris[3*j+0]-1;
      itri[1] = eletype->tris[3*j+1]-1;
      itri[2] = eletype->tris[3*j+2]-1;
      in[0]   = discr->elems[i].gIndices[2*itri[0]] - 1;
      in[1]   = discr->elems[i].gIndices[2*itri[1]] - 1;
      in[2]   = discr->elems[i].gIndices[2*itri[2]] - 1;
      status  = EG_inTriExact(&params[2*in[0]], &params[2*in[1]],
                              &params[2*in[2]], param, we);

      if (status == EGADS_SUCCESS) {
        *eIndex = i+1;
        /* interpolate reference coordinates to bary */
        for (k = 0; k < 2; k++)
          bary[k] = eletype->gst[2*itri[0]+k]*we[0] +
                    eletype->gst[2*itri[1]+k]*we[1] +
                    eletype->gst[2*itri[2]+k]*we[2];

        /* Linear quad */
        if (discr->types[discr->elems[i].tIndex-1].nref == 4) {
          in[0] = discr->elems[i].gIndices[0] - 1;
          in[1] = discr->elems[i].gIndices[2] - 1;
          in[2] = discr->elems[i].gIndices[4] - 1;
          in[3] = discr->elems[i].gIndices[6] - 1;
          invEvaluationQuad(params, param, in, bary);
        }

        return CAPS_SUCCESS;
      }

                     w = we[0];
      if (we[1] < w) w = we[1];
      if (we[2] < w) w = we[2];
      if (w > smallw) {
        ismall = i+1;
        itsmall = j;
        smallw = w;
      }
    }
  }

  /* must extrapolate! */
  if (ismall == 0) return CAPS_NOTFOUND;

  eletype = discr->types + discr->elems[ismall-1].tIndex-1;

  itri[0] = eletype->tris[3*itsmall+0]-1;
  itri[1] = eletype->tris[3*itsmall+1]-1;
  itri[2] = eletype->tris[3*itsmall+2]-1;
  in[0]   = discr->elems[ismall-1].gIndices[2*itri[0]] - 1;
  in[1]   = discr->elems[ismall-1].gIndices[2*itri[1]] - 1;
  in[2]   = discr->elems[ismall-1].gIndices[2*itri[2]] - 1;
  EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]], param, we);

  *eIndex = ismall;
  /* interpolate reference coordinates to bary */
  for (k = 0; k < 2; k++)
    bary[k] = eletype->gst[2*itri[0]+k]*we[0] +
              eletype->gst[2*itri[1]+k]*we[1] +
              eletype->gst[2*itri[2]+k]*we[2];

  return CAPS_SUCCESS;
}


/* Interpolation for a linear triangular element */
static int interpolation_LinearTriangle(capsDiscr *discr, int eIndex,
                                        double *bary, int rank, double *data,
                                        double *result)
{
  int    in[3], i;
  double we[3];

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if ((discr->types[discr->elems[eIndex-1].tIndex-1].nref  != 3) ||
      (discr->types[discr->elems[eIndex-1].tIndex-1].ndata != 0))
    return CAPS_BADTYPE;

  /* Linear triangle */
  we[0] = 1.0 - bary[0] - bary[1];
  we[1] = bary[0];
  we[2] = bary[1];
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  for (i = 0; i < rank; i++)
    result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2];

  return CAPS_SUCCESS;
}


/* Interpolation for a linear quadrilateral element */
static int interpolation_LinearQuad(capsDiscr *discr, int eIndex, double *bary,
                                    int rank, double *data, double *result)
{
  int    in[4], i;
  double we[2];

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if ((discr->types[discr->elems[eIndex-1].tIndex-1].nref  != 4) ||
      (discr->types[discr->elems[eIndex-1].tIndex-1].ndata != 0))
    return CAPS_BADTYPE;

  we[0] = bary[0];
  we[1] = bary[1];

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  in[3] = discr->elems[eIndex-1].gIndices[6] - 1;
  for (i = 0; i < rank; i++)
    result[i] = (1.0-we[0])*( (1.0-we[1])*data[rank*in[0]+i] +
                                   we[1] *data[rank*in[3]+i] ) +
                     we[0] *( (1.0-we[1])*data[rank*in[1]+i] +
                                   we[1] *data[rank*in[2]+i] );

  return CAPS_SUCCESS;
}


/* Interpolation for cell centered data */
static int interpolation_CellCenter(capsDiscr *discr, int eIndex, int rank,
                                    double *data, double *result)
{
  int ind, i;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata != 1)
    return CAPS_BADTYPE;

  ind = discr->elems[eIndex-1].dIndices[0] - 1;
  for (i = 0; i < rank; i++)
    result[i] = data[rank*ind+i];

  return CAPS_SUCCESS;
}


/* Interpolation selector */
int aim_interpolation(capsDiscr *discr, const char *name, int eIndex,
                      double *bary, int rank, double *data, double *result)
{
  int status;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) {
    printf(" aimTransferUtil/aim_interpolation: name = %s, eIndex = %d [1-%d]!\n",
           name, eIndex, discr->nElems);
    status = CAPS_BADINDEX;
    goto cleanup;
  }

  if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 0 ) {
    /* data at the reference locations */
    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) {
      /* Linear triangle */
      status = interpolation_LinearTriangle(discr, eIndex, bary, rank, data,
                                            result);
    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {
      /* Linear quadrilateral */
      status = interpolation_LinearQuad(discr, eIndex, bary, rank, data, result);
    } else {
      printf(" aimTransferUtil/aim_interpolation: name = %s, eIndex = %d [1-%d], nref not recognized!\n",
             name, eIndex, discr->nElems);
      status = CAPS_BADVALUE;
      goto cleanup;
    }
  } else if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 1 ) {
    /* data at cell center */
    status = interpolation_CellCenter(discr, eIndex, rank, data, result);
  } else {
    printf(" aimTransferUtil/aim_interpolation: name = %s, tIndex = %d, ndata = %d. Only supports ndata = 0 or ndata = 1!\n",
           name, discr->elems[eIndex-1].tIndex,
           discr->types[discr->elems[eIndex-1].tIndex-1].ndata);
    status = CAPS_BADTYPE;
    goto cleanup;
  }

cleanup:
  if (status != CAPS_SUCCESS)
    printf("Premature exit in transferUtils interpolation name = %s, status = %d\n",
           name, status);

  return status;
}


/* Interpolation Bar for a linear triangular element */
static int interpolateBar_LinearTriangle(capsDiscr *discr, int eIndex,
                                         double *bary, int rank,
                                         double *r_bar, double *d_bar)
{
  int    in[4], i;
  double we[3];

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref != 3)
    return CAPS_BADTYPE;

  we[0] = 1.0 - bary[0] - bary[1];
  we[1] = bary[0];
  we[2] = bary[1];
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

  for (i = 0; i < rank; i++) {
/*  result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2]; */
    d_bar[rank*in[0]+i] += we[0]*r_bar[i];
    d_bar[rank*in[1]+i] += we[1]*r_bar[i];
    d_bar[rank*in[2]+i] += we[2]*r_bar[i];
  }

  return CAPS_SUCCESS;
}


/* Interpolation Bar for a linear quadrilateral element */
static int interpolateBar_LinearQuad(capsDiscr *discr, int eIndex, double *bary,
                                     int rank, double *r_bar, double *d_bar)
{
  int    in[4], i;
  double we[3];

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref != 4)
    return CAPS_BADTYPE;

  we[0] = bary[0];
  we[1] = bary[1];

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  in[3] = discr->elems[eIndex-1].gIndices[6] - 1;

  for (i = 0; i < rank; i++) {
/*  result[i] = (1.0-we[0])*((1.0-we[1])*data[rank*in[0]+i]  +
                                  we[1] *data[rank*in[3]+i]) +
                     we[0] *((1.0-we[1])*data[rank*in[1]+i]  +
                                  we[1] *data[rank*in[2]+i]);  */
    d_bar[rank*in[0]+i] += (1.0-we[0])*(1.0-we[1])*r_bar[i];
    d_bar[rank*in[1]+i] +=      we[0] *(1.0-we[1])*r_bar[i];
    d_bar[rank*in[2]+i] +=      we[0] *     we[1] *r_bar[i];
    d_bar[rank*in[3]+i] += (1.0-we[0])*     we[1] *r_bar[i];
  }

  return CAPS_SUCCESS;
}


/* Interpolation Bar for cell centered data */
static int interpolateBar_CellCenter(capsDiscr *discr, int eIndex, int rank,
                                     double *r_bar, double *d_bar)
{
  int    ind, i;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata != 1)
    return CAPS_BADTYPE;

  ind = discr->elems[eIndex-1].dIndices[0] - 1;
  for (i = 0; i < rank; i++) {
/*  result[i] = data[rank*ind+i];  */
    d_bar[rank*ind+i] += r_bar[i];
  }

  return CAPS_SUCCESS;
}


/* Interpolate bar selector */
int aim_interpolateBar(capsDiscr *discr, const char *name, int eIndex,
                       double *bary, int rank, double *r_bar, double *d_bar)
{
  int status;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) {

    printf(" aimTransferUtil/aim_interpolateBar: name = %s, eIndex = %d [1-%d]!\n",
           name, eIndex, discr->nElems);
    status = CAPS_BADINDEX;
    goto cleanup;
  }

  if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 0 ) {
    /* data at the reference locations */
    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) {
      /* Linear triangle */
      status = interpolateBar_LinearTriangle(discr, eIndex, bary, rank,
                                             r_bar, d_bar);
    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {
      /* Linear quad */
      status = interpolateBar_LinearQuad(discr, eIndex, bary, rank, r_bar,
                                         d_bar);
    } else {
      printf(" aimTransferUtil/aim_interpolateBar: name = %s, eIndex = %d [1-%d], nref not recognized!\n",
             name ,eIndex, discr->nElems);
      status = CAPS_BADVALUE;
      goto cleanup;
    }
  } else if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 1 ) {
    /* data at cell center */
    status = interpolateBar_CellCenter(discr, eIndex, rank, r_bar, d_bar);
  } else {
    printf(" aimTransferUtil/aim_interpolateBar: name = %s, tIndex = %d, ndata = %d. Only supports ndata = 0 or ndata = 1!\n",
           name, discr->elems[eIndex-1].tIndex,
           discr->types[discr->elems[eIndex-1].tIndex-1].ndata);
    status = CAPS_BADTYPE;
    goto cleanup;
  }

cleanup:
  if (status != CAPS_SUCCESS)
    printf("Premature exit in transferUtils interpolation name = %s, status = %d\n",
           name, status);
  return status;
}


/* Integration for a linear triangular element */
static int integration_LinearTriangle(capsDiscr *discr, int eIndex, int rank,
                                      double *data, double *result)
{
  int        i, in[4], ptype, pindex, nBody, status;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref != 3)
    return CAPS_BADTYPE;

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) return status;

  /* Element indices */
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

  status = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[0], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[1], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x3, x1, x2);

  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  if (data == NULL) {
    *result = 3.0*area;
    return CAPS_SUCCESS;
  }

  for (i = 0; i < rank; i++) {
    result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area;
  }

  return CAPS_SUCCESS;
}


/* Integration for a linear quadrilateral element */
static int integration_LinearQuad(capsDiscr *discr, int eIndex, int rank,
                                  double *data, double *result)
{
  int        i, in[4], ptype, pindex, nBody, status;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area, area2;
  const char *intents;
  ego        *bodies;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref != 4)
    return CAPS_BADTYPE;

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) return status;

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  in[3] = discr->elems[eIndex-1].gIndices[6] - 1;

  status = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[0], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[1], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x3, x1, x2);

  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                        discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[3], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x1, x2, x3);

  area2  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  if (data == NULL) {
    *result = 3.0*area + 3.0*area2;
    return CAPS_SUCCESS;
  }

  for (i = 0; i < rank; i++) {
    result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area +
                (data[rank*in[0]+i] + data[rank*in[2]+i] +
                 data[rank*in[3]+i])*area2;
  }

  return CAPS_SUCCESS;
}


/* Integration for an element with cell centered data */
static int integration_CellCenter(capsDiscr *discr, int eIndex, int rank,
                                  double *data, double *result)
{
  int        i, in[4], ind, ptype, pindex, nBody, status;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area, area2;
  const char *intents;
  ego        *bodies;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) return status;

  /* Triangle or first half of the quad */
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

  status = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[0], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[1], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x3, x1, x2);

  area  = sqrt(DOT(x3, x3))/2.0;      /* 1/2 for area */

  ind = discr->elems[eIndex-1].dIndices[0] - 1;

  if (data == NULL) {
    *result = area;
  } else {
    for (i = 0; i < rank; i++) {
      result[i] = data[rank*ind+i]*area;
    }
  }

  /* second half of the quad */
  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {

    in[3]  = discr->elems[eIndex-1].gIndices[6] - 1;
    status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                          discr->mapping[2*in[2]+1], &ptype, &pindex, xyz2);
    if (status != CAPS_SUCCESS) {
      printf(" EG_getGlobal %d = %d!\n", in[2], status);
      return status;
    }

    status = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                          discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);
    if (status != CAPS_SUCCESS) {
      printf(" EG_getGlobal %d = %d!\n", in[3], status);
      return status;
    }

    x1[0] = xyz2[0] - xyz1[0];
    x2[0] = xyz3[0] - xyz1[0];
    x1[1] = xyz2[1] - xyz1[1];
    x2[1] = xyz3[1] - xyz1[1];
    x1[2] = xyz2[2] - xyz1[2];
    x2[2] = xyz3[2] - xyz1[2];

    CROSS(x1, x2, x3);

    area2  = sqrt(DOT(x3, x3))/2.0;      /* 1/2 for area */

    if (data == NULL) {
      *result += area2;
    } else {
      for (i = 0; i < rank; i++) {
        result[i] += data[rank*ind+i]*area2;
      }
    }
  }

  return CAPS_SUCCESS;
}


/* Integration selector */
int aim_integration(capsDiscr *discr, const char *name, int eIndex, int rank,
                    double *data, double *result)
{
  int status;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) {
    printf(" aimTransferUtil/aim_integration: name = %s, eIndex = %d [1-%d]!\n",
           name, eIndex, discr->nElems);
    status = CAPS_BADINDEX;
    goto cleanup;
  }

  if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 0 ) {
    /* data at the reference locations */
    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) {
      /* Linear triangle */
      status = integration_LinearTriangle(discr, eIndex, rank, data, result);
    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {
      /* Linear quad */
      status = integration_LinearQuad(discr, eIndex, rank, data, result);
    } else {
      printf(" aimTransferUtil/aim_integration: name = %s, eIndex = %d [1-%d], nref not recognized!\n",
             name, eIndex, discr->nElems);
      status = CAPS_BADVALUE;
      goto cleanup;
    }
  } else if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 1 ) {
    /* data at cell center */
    status = integration_CellCenter(discr, eIndex, rank, data, result);
  } else {
    printf(" aimTransferUtil/aim_integration: name = %s, tIndex = %d, ndata = %d. Only supports ndata = 0 or ndata = 1!\n",
           name, discr->elems[eIndex-1].tIndex,
           discr->types[discr->elems[eIndex-1].tIndex-1].ndata);
    status = CAPS_BADTYPE;
    goto cleanup;
  }

cleanup:
  if (status != CAPS_SUCCESS)
    printf("Premature exit in transferUtils interpolation name = %s, status = %d\n",
           name, status);
  return status;
}


/* Integration bar for a linear triangular element */
static int integrateBar_LinearTriangle(capsDiscr *discr, int eIndex, int rank,
                                       double *r_bar, double *d_bar)
{
  int        i, in[4], ptype, pindex, nBody, status;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref != 3)
    return CAPS_BADTYPE;

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) return status;

  /* Element indices */
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

  status = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[0], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[1], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x3, x1, x2);

  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  for (i = 0; i < rank; i++) {
/*  result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area;  */
    d_bar[rank*in[0]+i] += area*r_bar[i];
    d_bar[rank*in[1]+i] += area*r_bar[i];
    d_bar[rank*in[2]+i] += area*r_bar[i];
  }

  return CAPS_SUCCESS;
}


/* Integration bar for a linear quadrilateral element */
static int integrateBar_LinearQuad(capsDiscr *discr, int eIndex, int rank,
                                   double *r_bar, double *d_bar)
{
  int        i, in[4], ptype, pindex, nBody, status;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area, area2;
  const char *intents;
  ego        *bodies;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref != 4)
    return CAPS_BADTYPE;

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) return status;

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  in[3] = discr->elems[eIndex-1].gIndices[6] - 1;

  status = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[0], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[1], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x3, x1, x2);

  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                        discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[3], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x3, x1, x2);

  area2  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  for (i = 0; i < rank; i++) {

/*  result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area +
                (data[rank*in[0]+i] + data[rank*in[2]+i] +
                 data[rank*in[3]+i])*area2;  */
    d_bar[rank*in[0]+i] += (area + area2)*r_bar[i];
    d_bar[rank*in[1]+i] +=  area         *r_bar[i];
    d_bar[rank*in[2]+i] += (area + area2)*r_bar[i];
    d_bar[rank*in[3]+i] +=         area2 *r_bar[i];
  }

  return CAPS_SUCCESS;
}


/* Integration bar for an element with cell centered data */
static int integrateBar_CellCenter(capsDiscr *discr, int eIndex, int rank,
                                   double *r_bar, double *d_bar)
{
  int        i, in[4], ind, ptype, pindex, nBody, status;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area, area2;
  const char *intents;
  ego        *bodies;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) return CAPS_BADINDEX;

  status = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) return status;

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

  status = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                        discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[0], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                        discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[1], status);
    return status;
  }

  status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                        discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (status != CAPS_SUCCESS) {
    printf(" EG_getGlobal %d = %d!\n", in[2], status);
    return status;
  }

  x1[0] = xyz2[0] - xyz1[0];
  x2[0] = xyz3[0] - xyz1[0];
  x1[1] = xyz2[1] - xyz1[1];
  x2[1] = xyz3[1] - xyz1[1];
  x1[2] = xyz2[2] - xyz1[2];
  x2[2] = xyz3[2] - xyz1[2];

  CROSS(x3, x1, x2);

  area  = sqrt(DOT(x3, x3))/2.0;      /* 1/2 for area */

  ind = discr->elems[eIndex-1].dIndices[0] - 1;

  for (i = 0; i < rank; i++) {
/*  result[i] = data[rank*ind+i]*area;  */
    d_bar[rank*ind+i] += area*r_bar[i];
  }

  if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {

    in[3]  = discr->elems[eIndex-1].gIndices[6] - 1;
    status = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                          discr->mapping[2*in[2]+1], &ptype, &pindex, xyz2);
    if (status != CAPS_SUCCESS) {
      printf(" EG_getGlobal %d = %d!\n", in[2], status);
      return status;
    }

    status = EG_getGlobal(bodies[discr->mapping[2*in[3]]+nBody-1],
                          discr->mapping[2*in[3]+1], &ptype, &pindex, xyz3);
    if (status != CAPS_SUCCESS) {
      printf(" EG_getGlobal %d = %d!\n", in[3], status);
      return status;
    }

    x1[0] = xyz2[0] - xyz1[0];
    x2[0] = xyz3[0] - xyz1[0];
    x1[1] = xyz2[1] - xyz1[1];
    x2[1] = xyz3[1] - xyz1[1];
    x1[2] = xyz2[2] - xyz1[2];
    x2[2] = xyz3[2] - xyz1[2];

    CROSS(x3, x1, x2);

    area2  = sqrt(DOT(x3, x3))/2.0;      /* 1/2 for area */

    for (i = 0; i < rank; i++) {
/*    result[i] += data[rank*ind+i]*area2;  */
      d_bar[rank*ind+i] += area2*r_bar[i];
    }
  }

  return CAPS_SUCCESS;
}


/* Integrate bar selector */
int aim_integrateBar(capsDiscr *discr, const char *name, int eIndex, int rank,
                     double *r_bar, double *d_bar)
{
  int status;

  if ((eIndex <= 0) || (eIndex > discr->nElems)) {
    printf(" aimTransferUtil/aim_integrateBar: name = %s, eIndex = %d [1-%d]!\n",
           name, eIndex, discr->nElems);
    status = CAPS_BADINDEX;
    goto cleanup;
  }

  if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 0 ) {
    /* data at the reference locations */
    if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 3) {
      /* Linear triangle */
      status = integrateBar_LinearTriangle(discr, eIndex, rank, r_bar, d_bar);
    } else if (discr->types[discr->elems[eIndex-1].tIndex-1].nref == 4) {
      /* Linear quad */
      status = integrateBar_LinearQuad(discr, eIndex, rank, r_bar, d_bar);
    } else {
      printf(" aimTransferUtil/aim_integrateBar: name = %s, eIndex = %d [1-%d], nref not recognized!\n",
             name, eIndex, discr->nElems);
      status = CAPS_BADVALUE;
      goto cleanup;
    }
  } else if (discr->types[discr->elems[eIndex-1].tIndex-1].ndata == 1 ) {
    /* data at cell center */
    status = integrateBar_CellCenter(discr, eIndex, rank, r_bar, d_bar);
  } else {
    printf(" aimTransferUtil/aim_integrateBar: name = %s, tIndex = %d, ndata = %d. Only supports ndata = 0 or ndata = 1!\n",
           name, discr->elems[eIndex-1].tIndex,
           discr->types[discr->elems[eIndex-1].tIndex-1].ndata);
    status = CAPS_BADTYPE;
    goto cleanup;
  }

cleanup:
  if (status != CAPS_SUCCESS)
    printf("Premature exit in transferUtils interpolation name = %s, status = %d\n",
           name, status);
  return status;
}
