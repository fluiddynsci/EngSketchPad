/*
 ************************************************************************
 *                                                                      *
 * udpUtilities.h -- header for udp*.c                                  *
 *                                                                      *
 *              Written by John Dannenhoffer @ Syracuse University      *
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

#ifndef _UDPUTILITIES_H_
#define _UDPUTILITIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "egads.h"
#include "udp.h"

//$$$#define DEBUG_UDP

#define CINT    const int
#define CDOUBLE const double
#define CCHAR   const char

/* definition of the udp structures */
typedef struct {
    void      *val;           /* values (across rows) */
    double    *dot;           /* velocities */

    int       size;           /* total size (nrow*ncol) */
    int       nrow;           /* number of rows */
    int       ncol;           /* number of columns */
} udparg_T;

typedef struct {
    ego        ebody;
    udparg_T   arg[NUMUDPARGS];
    int        *bodyList;
    void       *data;         /* private data */
} udp_T;

/* storage for UDPs */
static int    numUdp = 0;     /* number of UDPs */
static udp_T  *udps  = NULL;  /* array  of UDPs */

#endif  /* _UDPUTILITIES_H_ */

