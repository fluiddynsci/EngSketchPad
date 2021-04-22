#include <math.h>
#include "egads.h"
#include "egads_dot.h"

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define __func__  __FUNCTION__
#endif

#define TWOPI 6.2831853071795862319959269
#define PI    (TWOPI/2.0)
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define DOT(a,b)          (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
#define CROSS(a,b,c)       a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                           a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                           a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define CROSS_DOT(a_dot,b,b_dot,c,c_dot) a_dot[0] = (b_dot[1]*c[2]) + (b[1]*c_dot[2]) - (b_dot[2]*c[1]) - (b[2]*c_dot[1]);\
                                         a_dot[1] = (b_dot[2]*c[0]) + (b[2]*c_dot[0]) - (b_dot[0]*c[2]) - (b[0]*c_dot[2]);\
                                         a_dot[2] = (b_dot[0]*c[1]) + (b[0]*c_dot[1]) - (b_dot[1]*c[0]) - (b[1]*c_dot[0])

/*****************************************************************************/
/*                                                                           */
/*  pingBodies                                                               */
/*                                                                           */
/*****************************************************************************/

int
pingBodies(ego tess1, ego tess2, double dtime, int iparam, const char *shape, double ftol, double etol, double ntol)
{
  int    status = EGADS_SUCCESS;
  int    n, d, np1, np2, nt1, nt2, nerr=0;
  int    nface, nedge, nnode, iface, iedge, inode, oclass, mtype;
  double p1_dot[18], p1[18], p2[18], fd_dot[3];
  const int    *pt1, *pi1, *pt2, *pi2, *ts1, *tc1, *ts2, *tc2;
  const double *t1, *t2, *x1, *x2, *uv1, *uv2;
  ego    ebody1, ebody2;
  ego    *efaces1=NULL, *efaces2=NULL, *eedges1=NULL, *eedges2=NULL, *enodes1=NULL, *enodes2=NULL;
  ego    top, prev, next;

  status = EG_statusTessBody( tess1, &ebody1, &np1, &np2 );
  if (status != EGADS_SUCCESS) goto cleanup;

  status = EG_statusTessBody( tess2, &ebody2, &np1, &np2 );
  if (status != EGADS_SUCCESS) goto cleanup;

  /* get the Faces from the Body 1 */
  status = EG_getBodyTopos(ebody1, NULL, FACE, &nface, &efaces1);
  if (status != EGADS_SUCCESS) goto cleanup;

  /* get the Edges from the Body 1 */
  status = EG_getBodyTopos(ebody1, NULL, EDGE, &nedge, &eedges1);
  if (status != EGADS_SUCCESS) goto cleanup;

  /* get the Nodes from the Body 1 */
  status = EG_getBodyTopos(ebody1, NULL, NODE, &nnode, &enodes1);
  if (status != EGADS_SUCCESS) goto cleanup;


  /* get the Faces from the Body 2 */
  status = EG_getBodyTopos(ebody2, NULL, FACE, &nface, &efaces2);
  if (status != EGADS_SUCCESS) goto cleanup;

  /* get the Edges from the Body 2 */
  status = EG_getBodyTopos(ebody2, NULL, EDGE, &nedge, &eedges2);
  if (status != EGADS_SUCCESS) goto cleanup;

  /* get the Nodes from the Body 2 */
  status = EG_getBodyTopos(ebody2, NULL, NODE, &nnode, &enodes2);
  if (status != EGADS_SUCCESS) goto cleanup;


  for (iface = 0; iface < nface; iface++) {

    /* extract the face tessellation */
    status = EG_getTessFace(tess1, iface+1, &np1, &x1, &uv1, &pt1, &pi1,
                                            &nt1, &ts1, &tc1);
    if (status != EGADS_SUCCESS) goto cleanup;

    status = EG_getTessFace(tess2, iface+1, &np2, &x2, &uv2, &pt2, &pi2,
                                            &nt2, &ts2, &tc2);
    if (status != EGADS_SUCCESS) goto cleanup;

    for (n = 0; n < np1; n++) {

      /* evaluate original edge and velocities*/
      status = EG_evaluate_dot(efaces1[iface], &uv1[2*n], NULL, p1, p1_dot);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* evaluate perturbed edge */
      status = EG_evaluate(efaces2[iface], &uv2[2*n], p2);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* compute the configuration velocity based on finite difference */
      fd_dot[0] = (p2[0] - p1[0])/dtime - p2[3]*(uv2[2*n] - uv1[2*n])/dtime - p2[6]*(uv2[2*n+1] - uv1[2*n+1])/dtime;
      fd_dot[1] = (p2[1] - p1[1])/dtime - p2[4]*(uv2[2*n] - uv1[2*n])/dtime - p2[7]*(uv2[2*n+1] - uv1[2*n+1])/dtime;
      fd_dot[2] = (p2[2] - p1[2])/dtime - p2[5]*(uv2[2*n] - uv1[2*n])/dtime - p2[8]*(uv2[2*n+1] - uv1[2*n+1])/dtime;

      for (d = 0; d < 3; d++) {
        if (fabs(p1_dot[d] - fd_dot[d]) > ftol) {
          printf("%s Face %d iparam=%d, p1[%d]=%+le fabs(%+le - %+le) = %+le > %e\n",
                 shape, iface+1, iparam, d, p1[d], p1_dot[d], fd_dot[d], fabs(p1_dot[d] - fd_dot[d]), ftol);
          nerr++;
        }
      }

      //printf("p1_dot = (%+f, %+f, %+f)\n", p1_dot[0], p1_dot[1], p1_dot[2]);
      //printf("fd_dot = (%+f, %+f, %+f)\n", fd_dot[0], fd_dot[1], fd_dot[2]);
      //printf("\n");
    }
  }

  for (iedge = 0; iedge < nedge; iedge++) {

    status = EG_getInfo(eedges1[iedge], &oclass, &mtype, &top, &prev, &next);
    if (status != EGADS_SUCCESS) goto cleanup;
    if (mtype == DEGENERATE) continue;

    /* extract the tessellation from the original edge */
    status = EG_getTessEdge(tess1, iedge+1, &np1, &x1, &t1);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the tessellation from the perturbed edge */
    status = EG_getTessEdge(tess2, iedge+1, &np2, &x2, &t2);
    if (status != EGADS_SUCCESS) goto cleanup;

    for (n = 0; n < np1; n++) {

      /* evaluate original edge and velocities*/
      status = EG_evaluate_dot(eedges1[iedge], &t1[n], NULL, p1, p1_dot);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* evaluate perturbed edge */
      status = EG_evaluate(eedges2[iedge], &t2[n], p2);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* compute the configuration velocity based on finite difference */
      fd_dot[0] = (p2[0] - p1[0])/dtime - p2[3]*(t2[n] - t1[n])/dtime;
      fd_dot[1] = (p2[1] - p1[1])/dtime - p2[4]*(t2[n] - t1[n])/dtime;
      fd_dot[2] = (p2[2] - p1[2])/dtime - p2[5]*(t2[n] - t1[n])/dtime;

      for (d = 0; d < 3; d++) {
        if (fabs(p1_dot[d] - fd_dot[d]) > etol) {
          printf("%s Edge %d iparam=%d, p1[%d]=%+le fabs(%+le - %+le) = %+le > %e\n",
                 shape, iedge+1, iparam, d, p1[d], p1_dot[d], fd_dot[d], fabs(p1_dot[d] - fd_dot[d]), etol);
          nerr++;
        }
      }

      //printf("p1_dot = (%+f, %+f, %+f)\n", p1_dot[0], p1_dot[1], p1_dot[2]);
      //printf("fd_dot = (%+f, %+f, %+f)\n", fd_dot[0], fd_dot[1], fd_dot[2]);
      //printf("\n");
    }
  }

  for (inode = 0; inode < nnode; inode++) {

    /* evaluate original node and velocities*/
    status = EG_evaluate_dot(enodes1[inode], NULL, NULL, p1, p1_dot);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* evaluate perturbed edge */
    status = EG_evaluate(enodes2[inode], NULL, p2);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* compute the configuration velocity based on finite difference */
    fd_dot[0] = (p2[0] - p1[0])/dtime;
    fd_dot[1] = (p2[1] - p1[1])/dtime;
    fd_dot[2] = (p2[2] - p1[2])/dtime;

    for (d = 0; d < 3; d++) {
      if (fabs(p1_dot[d] - fd_dot[d]) > etol) {
        printf("%s Node %d iparam=%d, p1[%d]=%+le fabs(%+le - %+le) = %+le > %e\n",
               shape, inode+1, iparam, d, p1[d], p1_dot[d], fd_dot[d], fabs(p1_dot[d] - fd_dot[d]), etol);
        nerr++;
      }
    }

    //printf("p1_dot = (%+f, %+f, %+f)\n", p1_dot[0], p1_dot[1], p1_dot[2]);
    //printf("fd_dot = (%+f, %+f, %+f)\n", fd_dot[0], fd_dot[1], fd_dot[2]);
    //printf("\n");
  }

cleanup:
  if (status != EGADS_SUCCESS) {
    printf(" Failure %d in %s\n", status, __func__);
  }
  EG_free(efaces1);
  EG_free(eedges1);
  EG_free(enodes1);

  EG_free(efaces2);
  EG_free(eedges2);
  EG_free(enodes2);

  return status + nerr;
}


/*****************************************************************************/
/*                                                                           */
/*  Box                                                                      */
/*                                                                           */
/*****************************************************************************/

int
pingBox(ego context)
{
  int    status = EGADS_SUCCESS;
  int    iparam, np1, nt1, iedge, nedge, iface, nface;
  double data[6], data_dot[6], params[3], dtime = 1e-7;
  const int    *pt1, *pi1, *ts1, *tc1;
  const double *t1, *x1, *uv1;
  ego    ebody1, ebody2, tess1, tess2;

  data[0] = 4; /* base coordinate */
  data[1] = 5;
  data[2] = 6;
  data[3] = 1; /* dx, dy, dz sizes */
  data[4] = 2;
  data[5] = 3;

  /* make the body */
  status = EG_makeSolidBody(context, BOX, data, &ebody1);
  if (status != EGADS_SUCCESS) goto cleanup;

  /* get the Faces from the Body */
  status = EG_getBodyTopos(ebody1, NULL, FACE, &nface, NULL);
  if (status != EGADS_SUCCESS) goto cleanup;

  /* get the Edges from the Body */
  status = EG_getBodyTopos(ebody1, NULL, EDGE, &nedge, NULL);
  if (status != EGADS_SUCCESS) goto cleanup;

  /* make the tessellation */
  params[0] =  0.2;
  params[1] =  0.01;
  params[2] = 12.0;
  status = EG_makeTessBody(ebody1, params, &tess1);

  for (iedge = 0; iedge < nedge; iedge++) {
    status = EG_getTessEdge(tess1, iedge+1, &np1, &x1, &t1);
    if (status != EGADS_SUCCESS) goto cleanup;
    printf(" BOX Edge %d np1 = %d\n", iedge+1, np1);
  }

  for (iface = 0; iface < nface; iface++) {
    status = EG_getTessFace(tess1, iface+1, &np1, &x1, &uv1, &pt1, &pi1,
                                            &nt1, &ts1, &tc1);
    if (status != EGADS_SUCCESS) goto cleanup;
    printf(" BOX Face %d np1 = %d\n", iface+1, np1);
  }

  /* zero out velocities */
  for (iparam = 0; iparam < 6; iparam++) data_dot[iparam] = 0;

  for (iparam = 0; iparam < 6; iparam++) {

    /* set the velocity of the original body */
    data_dot[iparam] = 1.0;
    status = EG_makeSolidBody_dot(ebody1, BOX, data, data_dot);
    if (status != EGADS_SUCCESS) goto cleanup;
    data_dot[iparam] = 0.0;

    status = EG_hasGeometry_dot(ebody1);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make a perturbed body for finite difference */
    data[iparam] += dtime;
    status = EG_makeSolidBody(context, BOX, data, &ebody2);
    if (status != EGADS_SUCCESS) goto cleanup;
    data[iparam] -= dtime;

    /* map the tessellation */
    status = EG_mapTessBody(tess1, ebody2, &tess2);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* ping the bodies */
    status = pingBodies(tess1, tess2, dtime, iparam, "BOX", 1e-7, 1e-7, 1e-7);
    if (status != EGADS_SUCCESS) goto cleanup;

    EG_deleteObject(tess2);
    EG_deleteObject(ebody2);
  }

  EG_deleteObject(tess1);
  EG_deleteObject(ebody1);

cleanup:
  if (status != EGADS_SUCCESS) {
    printf(" Failure %d in %s\n", status, __func__);
  }
  return status;
}


/*****************************************************************************/
/*                                                                           */
/*  Sphere                                                                   */
/*                                                                           */
/*****************************************************************************/

int
pingSphere(ego context)
{
  int    status = EGADS_SUCCESS;
  int    iparam, np1, nt1, iedge, nedge, iface, nface, sgn;
  double data[4], data_dot[4], params[3], dtime = 1e-7;
  const int    *pt1, *pi1, *ts1, *tc1;
  const double *t1, *x1, *uv1;
  ego    ebody1, ebody2, tess1, tess2;

  data[0] = 4; /* center */
  data[1] = 5;
  data[2] = 6;
  data[3] = 2; /* radius */

  for (sgn = -1; sgn <= 1; sgn += 2) {
    /* make the body */
    status = EG_makeSolidBody(context, sgn*SPHERE, data, &ebody1);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Faces from the Body */
    status = EG_getBodyTopos(ebody1, NULL, FACE, &nface, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Edges from the Body */
    status = EG_getBodyTopos(ebody1, NULL, EDGE, &nedge, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make the tessellation */
    params[0] =  0.4;
    params[1] =  0.2;
    params[2] = 20.0;
    status = EG_makeTessBody(ebody1, params, &tess1);

    for (iedge = 0; iedge < nedge; iedge++) {
      status = EG_getTessEdge(tess1, iedge+1, &np1, &x1, &t1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" SPHERE Edge %d np1 = %d\n", iedge+1, np1);
    }

    for (iface = 0; iface < nface; iface++) {
      status = EG_getTessFace(tess1, iface+1, &np1, &x1, &uv1, &pt1, &pi1,
                                              &nt1, &ts1, &tc1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" SPHERE Face %d np1 = %d\n", iface+1, np1);
    }

    /* zero out velocities */
    for (iparam = 0; iparam < 4; iparam++) data_dot[iparam] = 0;

    for (iparam = 0; iparam < 4; iparam++) {

      /* set the velocity of the original body */
      data_dot[iparam] = 1.0;
      status = EG_makeSolidBody_dot(ebody1, sgn*SPHERE, data, data_dot);
      if (status != EGADS_SUCCESS) goto cleanup;
      data_dot[iparam] = 0.0;

      status = EG_hasGeometry_dot(ebody1);
      if (status != EGADS_SUCCESS) goto cleanup;
      
      /* make a perturbed body for finite difference */
      data[iparam] += dtime;
      status = EG_makeSolidBody(context, sgn*SPHERE, data, &ebody2);
      if (status != EGADS_SUCCESS) goto cleanup;
      data[iparam] -= dtime;

      /* map the tessellation */
      status = EG_mapTessBody(tess1, ebody2, &tess2);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* ping the bodies */
      status = pingBodies(tess1, tess2, dtime, iparam, "SPHERE", 1e-7, 1e-7, 1e-7);
      if (status != EGADS_SUCCESS) goto cleanup;

      EG_deleteObject(tess2);
      EG_deleteObject(ebody2);
    }

    EG_deleteObject(tess1);
    EG_deleteObject(ebody1);
  }

cleanup:
  if (status != EGADS_SUCCESS) {
    printf(" Failure %d in %s\n", status, __func__);
  }
  return status;
}


/*****************************************************************************/
/*                                                                           */
/*  Cone                                                                     */
/*                                                                           */
/*****************************************************************************/

int
pingCone(ego context)
{
  int    status = EGADS_SUCCESS;
  int    iparam, np1, nt1, iedge, nedge, iface, nface, sgn;
  double data[7], data_dot[7], params[3], dtime = 1e-7;
  const int    *pt1, *pi1, *ts1, *tc1;
  const double *t1, *x1, *uv1;
  ego    ebody1, ebody2, tess1, tess2;

  data[0] = 5; /* vertex */
  data[1] = 4;
  data[2] = 6;
  data[3] = 1; /* base */
  data[4] = 2;
  data[5] = 3;
  data[6] = 4; /* radius */

  for (sgn = -1; sgn <= 1; sgn += 2) {
    /* make the body */
    status = EG_makeSolidBody(context, sgn*CONE, data, &ebody1);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Faces from the Body */
    status = EG_getBodyTopos(ebody1, NULL, FACE, &nface, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Edges from the Body */
    status = EG_getBodyTopos(ebody1, NULL, EDGE, &nedge, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make the tessellation */
    params[0] =  0.5;
    params[1] =  0.3;
    params[2] = 20.0;
    status = EG_makeTessBody(ebody1, params, &tess1);

    for (iedge = 0; iedge < nedge; iedge++) {
      status = EG_getTessEdge(tess1, iedge+1, &np1, &x1, &t1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" CONE Edge %d np1 = %d\n", iedge+1, np1);
    }

    for (iface = 0; iface < nface; iface++) {
      status = EG_getTessFace(tess1, iface+1, &np1, &x1, &uv1, &pt1, &pi1,
                                              &nt1, &ts1, &tc1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" CONE Face %d np1 = %d\n", iface+1, np1);
    }

    /* zero out velocities */
    for (iparam = 0; iparam < 7; iparam++) data_dot[iparam] = 0;

    for (iparam = 0; iparam < 7; iparam++) {

      /* set the velocity of the original body */
      data_dot[iparam] = 1.0;
      status = EG_makeSolidBody_dot(ebody1, sgn*CONE, data, data_dot);
      if (status != EGADS_SUCCESS) goto cleanup;
      data_dot[iparam] = 0.0;

      status = EG_hasGeometry_dot(ebody1);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* make a perturbed body for finite difference */
      data[iparam] += dtime;
      status = EG_makeSolidBody(context, sgn*CONE, data, &ebody2);
      if (status != EGADS_SUCCESS) goto cleanup;
      data[iparam] -= dtime;

      /* map the tessellation */
      status = EG_mapTessBody(tess1, ebody2, &tess2);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* ping the bodies */
      status = pingBodies(tess1, tess2, dtime, iparam, "CONE", 1e-7, 1e-7, 1e-7);
      if (status != EGADS_SUCCESS) goto cleanup;

      EG_deleteObject(tess2);
      EG_deleteObject(ebody2);
    }

    EG_deleteObject(tess1);
    EG_deleteObject(ebody1);
  }

cleanup:
  if (status != EGADS_SUCCESS) {
    printf(" Failure %d in %s\n", status, __func__);
  }
  return status;
}


/*****************************************************************************/
/*                                                                           */
/*  Cylinder                                                                 */
/*                                                                           */
/*****************************************************************************/

int
pingCylinder(ego context)
{
  int    status = EGADS_SUCCESS;
  int    iparam, np1, nt1, iedge, nedge, iface, nface, sgn;
  double data[7], data_dot[7], params[3], dtime = 1e-7;
  const int    *pt1, *pi1, *ts1, *tc1;
  const double *t1, *x1, *uv1;
  ego    ebody1, ebody2, tess1, tess2;

  data[0] = 5; /* vertex */
  data[1] = 4;
  data[2] = 6;
  data[3] = 1; /* base */
  data[4] = 2;
  data[5] = 3;
  data[6] = 4; /* radius */

  for (sgn = -1; sgn <= 1; sgn += 2) {
    /* make the body */
    status = EG_makeSolidBody(context, sgn*CYLINDER, data, &ebody1);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Faces from the Body */
    status = EG_getBodyTopos(ebody1, NULL, FACE, &nface, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Edges from the Body */
    status = EG_getBodyTopos(ebody1, NULL, EDGE, &nedge, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make the tessellation */
    params[0] =  0.4;
    params[1] =  0.2;
    params[2] = 20.0;
    status = EG_makeTessBody(ebody1, params, &tess1);

    for (iedge = 0; iedge < nedge; iedge++) {
      status = EG_getTessEdge(tess1, iedge+1, &np1, &x1, &t1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" CYLINDER Edge %d np1 = %d\n", iedge+1, np1);
    }

    for (iface = 0; iface < nface; iface++) {
      status = EG_getTessFace(tess1, iface+1, &np1, &x1, &uv1, &pt1, &pi1,
                                              &nt1, &ts1, &tc1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" CYLINDER Face %d np1 = %d\n", iface+1, np1);
    }

    /* zero out velocities */
    for (iparam = 0; iparam < 7; iparam++) data_dot[iparam] = 0;

    for (iparam = 0; iparam < 7; iparam++) {

      /* set the velocity of the original body */
      data_dot[iparam] = 1.0;
      status = EG_makeSolidBody_dot(ebody1, sgn*CYLINDER, data, data_dot);
      if (status != EGADS_SUCCESS) goto cleanup;
      data_dot[iparam] = 0.0;

      status = EG_hasGeometry_dot(ebody1);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* make a perturbed body for finite difference */
      data[iparam] += dtime;
      status = EG_makeSolidBody(context, sgn*CYLINDER, data, &ebody2);
      if (status != EGADS_SUCCESS) goto cleanup;
      data[iparam] -= dtime;

      /* map the tessellation */
      status = EG_mapTessBody(tess1, ebody2, &tess2);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* ping the bodies */
      status = pingBodies(tess1, tess2, dtime, iparam, "CYLINDER", 1e-7, 1e-7, 1e-7);
      if (status != EGADS_SUCCESS) goto cleanup;

      EG_deleteObject(tess2);
      EG_deleteObject(ebody2);
    }

    EG_deleteObject(tess1);
    EG_deleteObject(ebody1);
  }

cleanup:
  if (status != EGADS_SUCCESS) {
    printf(" Failure %d in %s\n", status, __func__);
  }
  return status;
}


/*****************************************************************************/
/*                                                                           */
/*  Torus                                                                    */
/*                                                                           */
/*****************************************************************************/

int
pingTorus(ego context)
{
  int    status = EGADS_SUCCESS;
  int    iparam, np1, nt1, iedge, nedge, iface, nface, sgn;
  double data[8], data_dot[8], params[3], dtime = 1e-7;
  const int    *pt1, *pi1, *ts1, *tc1;
  const double *t1, *x1, *uv1;
  ego    ebody1, ebody2, tess1, tess2;

  data[0] = 5; /* center */
  data[1] = 4;
  data[2] = 6;
  data[3] = 1; /* axis */
  data[4] = 2;
  data[5] = 3;
  data[6] = 4;   /* major radius */
  data[7] = 0.5; /* minor radius */

  /* sgn =1 because EG_mapTessBody does not work for a torus with 1 node */
  for (sgn = 1; sgn <= 1; sgn += 2) {

    /* make the body */
    status = EG_makeSolidBody(context, sgn*TORUS, data, &ebody1);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Faces from the Body */
    status = EG_getBodyTopos(ebody1, NULL, FACE, &nface, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* get the Edges from the Body */
    status = EG_getBodyTopos(ebody1, NULL, EDGE, &nedge, NULL);
    if (status != EGADS_SUCCESS) goto cleanup;

    /* make the tessellation */
    params[0] =  0.5;
    params[1] =  0.3;
    params[2] = 20.0;
    status = EG_makeTessBody(ebody1, params, &tess1);

    for (iedge = 0; iedge < nedge; iedge++) {
      status = EG_getTessEdge(tess1, iedge+1, &np1, &x1, &t1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" TORUS Edge %d np1 = %d\n", iedge+1, np1);
    }

    for (iface = 0; iface < nface; iface++) {
      status = EG_getTessFace(tess1, iface+1, &np1, &x1, &uv1, &pt1, &pi1,
                                              &nt1, &ts1, &tc1);
      if (status != EGADS_SUCCESS) goto cleanup;
      printf(" TORUS Face %d np1 = %d\n", iface+1, np1);
    }

    /* zero out velocities */
    for (iparam = 0; iparam < 8; iparam++) data_dot[iparam] = 0;

    for (iparam = 0; iparam < 8; iparam++) {

      /* set the velocity of the original body */
      data_dot[iparam] = 1.0;
      status = EG_makeSolidBody_dot(ebody1, sgn*TORUS, data, data_dot);
      if (status != EGADS_SUCCESS) goto cleanup;
      data_dot[iparam] = 0.0;

      status = EG_hasGeometry_dot(ebody1);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* make a perturbed body for finite difference */
      data[iparam] += dtime;
      status = EG_makeSolidBody(context, sgn*TORUS, data, &ebody2);
      if (status != EGADS_SUCCESS) goto cleanup;
      data[iparam] -= dtime;

      /* map the tessellation */
      status = EG_mapTessBody(tess1, ebody2, &tess2);
      if (status != EGADS_SUCCESS) goto cleanup;

      /* ping the bodies */
      status = pingBodies(tess1, tess2, dtime, iparam, "TORUS", 1e-7, 1e-7, 1e-7);
      if (status != EGADS_SUCCESS) goto cleanup;

      EG_deleteObject(tess2);
      EG_deleteObject(ebody2);
    }

    EG_deleteObject(tess1);
    EG_deleteObject(ebody1);
  }

cleanup:
  if (status != EGADS_SUCCESS) {
    printf(" Failure %d in %s\n", status, __func__);
  }
  return status;
}


int main(int argc, char *argv[])
{
  int status;
  ego context;

  /* create an EGADS context */
  status = EG_open(&context);
  if (status != EGADS_SUCCESS) {
    printf(" EG_open return = %d\n", status);
    return EXIT_FAILURE;
  }

  status = pingBox(context);
  if (status != EGADS_SUCCESS) goto cleanup;

  status = pingSphere(context);
  if (status != EGADS_SUCCESS) goto cleanup;

  status = pingCone(context);
  if (status != EGADS_SUCCESS) goto cleanup;

  status = pingCylinder(context);
  if (status != EGADS_SUCCESS) goto cleanup;

  status = pingTorus(context);
  if (status != EGADS_SUCCESS) goto cleanup;

cleanup:

  EG_close(context);

  /* these statements are in case we used an error return to go to cleanup */
  if (status != EGADS_SUCCESS) {
    printf(" Overall Failure %d\n", status);
    status = EXIT_FAILURE;
  } else {
    printf(" EGADS_SUCCESS!\n");
    status = EXIT_SUCCESS;
  }

  return status;
}
