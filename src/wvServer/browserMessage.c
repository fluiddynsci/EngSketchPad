/*
 *	The Web Viewer
 *
 *		Default CallBack
 *
 *      Copyright 2011-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>

#include "libwebsockets.h"


void browserMessage(/*@unused@*/ struct libwebsocket *wsi, char *text,
                    /*@unused@*/ int len)
{
  printf(" BuiltIn browserMessage: %s\n", text);
}
