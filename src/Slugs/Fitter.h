/*
 ************************************************************************
 *                                                                      *
 * Fitter.h -- header file for Fitter                                   *
 *                                                                      *
 *              Written by John Dannenhoffer @ Syracuse University      *
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

#ifndef _FITTER_H_
#define _FITTER_H_

#if defined(_WIN32) && defined(BUILD_DLL)
    #define EXTERN extern _declspec(dllexport)
#else
    #define EXTERN extern
#endif

/* return codes */
#define FIT_SUCCESS     0   /* success */
#define FIT_BITFLAG    -1   /* illegal value for bitflag */
#define FIT_SMOOTH     -2   /* illegal value for smooth */
#define FIT_NCP        -3   /* more control points than cloud points */
#define FIT_DEGENERATE -4   /* degenerate geometry */
#define FIT_MALLOC     -5   /* problem mallocing memory */
#define FIT_SINGULAR   -6   /* matrix diagonal is too small */

#if defined (__cplusplus)
    extern "C" {
#endif

/* wrapper to fit B-spline curve with one call */
EXTERN
int    fit1dCloud(int     m,            /* (in)  number of points in cloud */
                  int     bitflag,      /* (in)  1=ordered, 2=uPeriodic, 8=intGiven */
                  double  XYZcloud[],   /* (in)  array  of points in cloud (x,y,z,x,...) */
                  int     n,            /* (in)  number of control points */
                  double  cp[],         /* (in)  array  of control points (first and last are set) */
                                        /* (out) array  of control points (all set) */
                  double  smooth,       /* (in)  initial control net smoothing parameter */
                  double  Tcloud[],     /* (out) T-parameters of points in cloud */
                  double  *normf,       /* (out) RMS of distances between cloud and fit */
                  double  *maxf,        /* (out) maximum distance between cloud and fit */
        /*@null@*/double  *dotmin,      /* (out) minimum normalized dot product of control polygon */
        /*@null@*/int     *nmin,        /* (out) minimum number of cloud points in any interval */
                  int     *numiter,     /* (out) number of iterations executed */
        /*@null@*/FILE    *fp);         /* (in)  file for progress outputs (or NULL) */

/* initialize the B-spline curve fitter */
EXTERN
int    fit1d_init(int     m,            /* (in)  number of points in cloud */
                  int     bitflag,      /* (in)  1=ordered, 2=uPeriodic, 8=intGiven */
                  double  smooth,       /* (in)  control net smoothing parameter */
                  double  XYZcloud[],   /* (in)  array  of points in cloud (x,y,z,x,...) */
                  int     n,            /* (in)  number of control points */
                  double  cp[],         /* (in)  array  of control points (first and last set) */
        /*@null@*/FILE    *fp,          /* (in)  file pointer for progress prints */
                  double  *normf,       /* (out) initial RMS between cloud and fit */
                  double  *maxf,        /* (out) maximum distance between cloud and fit */
                  void    **context);   /* (out) context */

/* take one step of the B-spline curve fitter */
EXTERN
int    fit1d_step(void    *context,     /* (in)  context */
                  double  smooth,       /* (in)  control net smoothing parameter */
                  double  *normf,       /* (out) RMS of dsitances between cloud and fit */
                  double  *maxf);       /* (out) maximum distance between cloud and fit */

/* clean up after B-spline curve fitter */
EXTERN
int    fit1d_done(void    *context,     /* (in)  context */
                  double  Tcloud[],     /* (out) T-parameters of points in cloud */
                  double  cp[],         /* (out) array of control points */
                  double  *normf,       /* (out) RMS of distances between cloud and fit */
                  double  *maxf,        /* (out) maximum distance between cloud and fit */
        /*@null@*/double  *dotmin,      /* (out) minimum normalized dot product of control polygon (or NULL) */
        /*@null@*/int     *nmin);       /* (out) minimum number of cloud points in any interval (or NULL) */

/* free up memory associated with curve fitter */
EXTERN
void   fit1d_free(void    *context);    /* (in)  context */

/* wrapper to fit B-splnie surface with one call */
EXTERN
int    fit2dCloud(int     m,            /* (in)  number of points in cloud */
                  int     bitflag,      /* (in)  2=uPeriodic, 4=vPeriodic, 8=intGiven */
                  double  XYZcloud[],   /* (in)  array  of points in cloud (x,y,z,x,...) */
                  int     nu,           /* (in)  number of control points in U direction */
                  int     nv,           /* (in)  number of control points in V direction */
                  double  cp[],         /* (in)  array  of control points (boundaries set) */
                                        /* (out) array  of control points (all set) */
                  double  smooth,       /* (in)  initial control net smoothing prmeter */
                  double  UVcloud[],    /* (out) UV-parameters of points in cloud (u,v,u,...) */
                  double  *normf,       /* (out) RMS of distances between cloud and fit */
                  double  *maxf,        /* (out) maximum distance between cloud and fit */
        /*@null@*/int     *nmin,        /* (out) minimum number of cloud points in any interval */
                  int     *numiter,     /* (out) number of iterations executed */
        /*@null@*/FILE    *fp);         /* (in)  file for progress outputs (or NULL) */

/* initialize the B-spline surface fitter */
EXTERN
int    fit2d_init(int     m,            /* (in)  number of points in cloud */
                  int     bitflag,      /* (in)  2=uPeriodic, 4=vPeriodic, 8=int */
                  double  smooth,       /* (in)  control net smoothing parameter */
                  double  XYZcloud[],   /* (in)  array  of points in cloud (x,y,z,x,...) */
                  int     nu,           /* (in)  number of control points in u direction */
                  int     nv,           /* (in)  number of control points in v direction */
                  double  cp[],         /* (in)  array  of control points (with outline set) */
        /*@null@*/FILE    *fp,          /* (in)  file pointer for progress prints */
                  double  *normf,       /* (out) initial RMS between cloud and fit */
                  double  *maxf,        /* (out) maximum distance between cloud and fit */
                  void    **context);   /* (out) context */

/* take one step of the B-spline surface fitter */
EXTERN
int    fit2d_step(void    *context,     /* (in)  context */
                  double  smooth,       /* (in)  smoothing parameter */
                  double  *normf,       /* (out) RMS of distances between cloud and fit */
                  double  *maxf);       /* (out) maximum distance between cloud and fit */

/* clean up after B-spline surface fitter */
EXTERN
int    fit2d_done(void    *context,     /* (in)  context */
                  double  UVcloud[],    /* (out) UV-parameters of points in cloud */
                  double  cp[],         /* (out) array of control points */
                  double  *normf,       /* (out) RMS of distances between cloud and fit */
                  double  *maxf,        /* (out) maximum distance between cloud and fit */
        /*@null@*/int     *nmin);       /* (out) minimum number of cloud points in any interval (or NULL) */

/* free up memory associated with surface fitter */
EXTERN
void   fit2d_free(void    *context);    /* (in)  context */

#if defined (__cplusplus)
    }
#endif

#undef EXTERN

#ifdef GRAFIC
int    plotCurve(int    m,              /* (in)  number of points in cloud */
                 double XYZcloud[],     /* (in)  array  of points in cloud */
       /*@null@*/double Tcloud[],       /* (in)  T-parameters of points in cloud (or NULL) */
                 int    n,              /* (in)  number of control points */
                 double cp[],           /* (in)  array  of control points */
                 double normf,          /* (in)  RMS of distances between cloud and fit */
                 double dotmin,         /* (in)  minimum noralized dot product of control polygon */
                 int    nmin);          /* (in)  minimum number of cloud points in any interval */

int    plotSurface(int    m,            /* (in)  number of points in cloud */
                   double XYZcloud[],   /* (in)  array  of points in cloud */
         /*@null@*/double UVcloud[],    /* (in)  UV-parameters of points in cloud (or NULL) */
                   int    n,            /* (in)  number of control points in each direction */
                   double cp[],         /* (in)  array  of control points */
                   double normf,        /* (in)  RMS of distances between cloud and fit */
                   int    nmin);        /* (out) minimum number of cloud points in any interval */
#endif

#endif   /* _FITTER_H_ */
