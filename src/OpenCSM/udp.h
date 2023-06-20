/*
 ************************************************************************
 *                                                                      *
 * OpenCSM/EGADS -- Extended User Defined Primitive/Function Interface  *
 *                                                                      *
 *            Written by John Dannenhoffer @ Syracuse University        *
 *                                                                      *
 ************************************************************************
 */

/*
 * Copyright (C) 2010/2023  John F. Dannenhoffer, III (Syracuse University)
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

#ifndef _UDP_H_
#define _UDP_H_

/* define needed for WIN32 */
#ifdef WIN32
    #define snprintf _snprintf
#endif

/* --------------------------------------------------------------------- */
/* (additional) attribute types */
/* --------------------------------------------------------------------- */

//      ATTRINT      1   // +ATTRINT     for integer input
                         // -ATTRINT     for integer output
//      ATTRREAL     2   // +ATTRREAL    for double  input
                         // -ATTRREAL    for double  output
//      ATTRSTRING   3   // +ATTRSTRING  for string  input
                         // -ATTRSTRING  *** cannot be used ***
#define ATTRREALSEN  4   // +ATTRREALSEN for double input  (with sensitivities)
                         // -ATTRREALSEN for double output (has  sensitivities)
#define ATTRFILE     5   // +ATTRFILE    for input file
                         // -ATTRFILE    *** cannot be used ***
#define ATTRREBUILD  6   // +ATTRREBUILD to force rebuild
                         // -ATTRREBUILD *** cannot be used ***
//      ATTRCSYS    12   //              *** cannot be used ***
//      ATTRPTR     13   //              *** cannot be used ***

#define OCSM_NODE 600
#define OCSM_EDGE 601
#define OCSM_FACE 602

/* --------------------------------------------------------------------- */
/* prototypes for routines to be provided by a UDP/UDF */
/* --------------------------------------------------------------------- */

/* REQUIRED */
/* excute the primitive */
int udpExecute(ego  context,            /* (in)  EGADS context */
               ego  *ebody,             /* (out) Body pointer */
               int  *nMesh,             /* (out) number of associated meshes */
               char *string[]);         /* (out) error message (or NULL if none) (freeable) */

/* OPTIONAL */
/* return sensitivity derivatives for the "real" argument */
int udpSensitivity(ego    ebody,        /* (in)  Body pointer (matches return from udpExecute) */
                   int    npts,         /* (in)  number of points at which sensitivities are computed */
                   int    entType,      /* (in)  entity type: OCSM_NODE, OCSM_EDGE, or OCSM_FACE */
                   int    entIndex,     /* (in)  entity index (bias-1) */
                   double uvs[],        /* (in)  array of parametric coordinates (t or uv) */
                   double vels[]);      /* (out) array of velocities (3*npts long) */

/* OPTIONAL */
/* return meshes associated with the primitive */
int udpMesh(ego body,                   /* (in)  Body pointer (matches return from udpExecute) */
            int  imesh,                 /* (in)  mesh index (bias-1) */
            int  *imax,                 /* (out) i-dimension of mesh */
            int  *jmax,                 /* (out) j-dimension of mesh */
            int  *kmax,                 /* (out) k-dimension of mesh */
            double *mesh[]);            /* (out) array of mesh points (freeable) */

/* --------------------------------------------------------------------- */
/* functions called by OpenCSM.c (based upon above and functions in udpUtilities.c */
/* --------------------------------------------------------------------- */

/* initialize & get info about the list of arguments */
/* udpInitialize in udpUtilities.c */
extern int
udp_initialize(char   primName[],       /* (in)  primitive name */
               int    *nArgs,           /* (out) number of arguments */
               char   **name[],         /* (out) array of argument names */
               int    *type[],          /* (out) array of argument types (see below) */
               int    *idefault[],      /* (out) array of integer default values */
               double *ddefault[]);     /* (out) array of double  default values */

/* get number of Bodys expected in call to udp_executePrim */
/* udpNumBodys() in udpUtilities.c */
extern int
udp_numBodys(char primName[]);          /* (out) number of expected Bodys */

/* get list of Bodys associated with primitive */
/* udpBodyList() in udpUtilities.c */
extern int
udp_bodyList(char primName[],           /* (in)  primitive name */
             ego  body,                 /* (in)  Body pointer (matches return from udpExecute) */
             const int *bodyList[]);    /* (in)  0-terminated list of Bodys used by UDF */

/* set the argument list back to default */
/* udpReset(0) in udpUtilities.c */
extern int
udp_clrArguments(char primName[]);      /* (in)  primitive name */

/* clean the udp cache */
/* udpClean() in udpUtilities.c */
extern int
udp_clean(char primName[],              /* (in)  primitive name */
          ego  body);                   /* (in)  Body pointer to clean (matches return from udpExecute) */

/* set an argument -- characters are converted based on type */
/* udpSetArgument() in udpUtilities.c */
extern int
udp_setArgument(char primName[],        /* (in)  primitive name */
                char name[],            /* (in)  argument name */
                void *value,            /* (in)  argument value(s) */
                int  nrow,              /* (in)  number of rows or characters */
                int  ncol,              /* (in)  number of columns (or 0 for string) */
      /*@null@*/char message[]);        /* (out) error message (or NULL if none) (freeable) */

/* execute */
/* udpExecute() in UDP/UDF */
extern int
udp_executePrim(char primName[],        /* (in)  primitive name */
                ego  context,           /* (in)  EGADS context */
                ego  *body,             /* (out) Body (or Model) produced (destroy with EG_deleteObject) */
                int  *nMesh,            /* (out) number of associated meshes */
                char *string[]);        /* (out) error message (or NULL if none) (freeable) */

/* get an output parameter -- characters are converted based on type */
/* udpGet() in udpUtilities.c */
extern int
udp_getOutput(char primName[],          /* (in)  primitive name */
              ego  body,                /* (in)  Body pointer (matches return from udpExecute) */
              char name[],              /* (in)  parameter name */
              int  *nrow,               /* (out) number of rows */
              int  *ncol,               /* (out) number of columns */
              void *val[],              /* (out) parameter value(s) (freeable) */
              void *dot[],              /* (out) parameter velocities, or NULL (freeable) */
              char message[]);          /* (out) error message (or NULL if none) (freeable) */

/* return the OverSet Mesh */
/* udpMesh() in UDP/UDF or udpUtilities.c (if not in UDP/UDF) */
extern int
udp_getMesh(char   primName[],          /* (in)  primitive name */
            ego    body,                /* (in)  Body pointer (mateches return from udpExecute) */
            int    iMesh,               /* (in)  mesh index (bias-1( */
            int    *imax,               /* (out) i-dimension of mesh */
            int    *jmax,               /* (out) j-dimension of mesh */
            int    *kmax,               /* (out) k-dimension of mesh */
            double *mesh[]);            /* (out) mesh points (freeable) */

/* set a design velocity -- characters are converted based on type */
/* udpVel() in udpUtilities.c */
extern int
udp_setVelocity(char   primName[],      /* (in)  primitive name */
                ego    body,            /* (in)  Body pointer (matches return from udpExecute) */
                char   name[],          /* (in)  parameter name */
                double value[],         /* (in)  parameter velocities */
                int    nvalue);         /* (in)  number of velocities */

/* return sensitivity derivatives for the "real/sens" argument */
/* udpSensitivity() in UDP/UDF or udpUtilities.c (if not in UDP/UDF) */
extern int
udp_sensitivity(char   primName[],      /* (in)  primitive name */
                ego    body,            /* (in)  Body pointer (matches return from udpExecute) */
                int    npts,            /* (in)  number of points */
                int    entType,         /* (in)  entity type (OCSM_NODE, OCSM_EDGE, or OCSM_FACE) */
                int    entIndex,        /* (in)  entity index (bias-1) */
      /*@null@*/double uvs[],           /* (in)  parametric coordinates (of NULL for tessellation points) */
                double vels[]);         /* (out) array of velocities (3*npts in length) */

/* unload and cleanup for all UDP/UDFs */
/* udpReset(1) in udpUtilities.c */
extern void
udp_cleanupAll();

#endif  /* _UDP_H_ */
