/*
 ************************************************************************
 *                                                                      *
 * tim.c -- functions for the Tool Integration Modules                  *
 *                                                                      *
 *              Written by John Dannenhoffer @ Syracuse University      *
 *                                                                      *
 ************************************************************************
 */

/*
 * Copyright (C) 2010/2022  John F. Dannenhoffer, III (Syracuse University)
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
#include <assert.h>

#ifdef WIN32
   #define DLL        HINSTANCE
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
   #include <winsock2.h>
   #define snprintf     _snprintf
   #define strcasecmp   stricmp
   #define SLEEP(msec)  Sleep(msec)
#else
   #define DLL void *
   #include <dlfcn.h>
   #include <dirent.h>
   #include <limits.h>
   #include <unistd.h>
   #define SLEEP(msec)  usleep(1000*msec)
#endif

#define  SHOW_STATES  0

#include "egads.h"
#include "OpenCSM.h"
#include "common.h"
#include "tim.h"
#include "emp.h"

#include <unistd.h>

#include "wsserver.h"

#define MAX_TIMS 32

typedef int (*timDLLfunc) (void);

static char      *timName[  MAX_TIMS];
static DLL        timDLL[   MAX_TIMS];
static int        timState [MAX_TIMS];
static void      *timEsp[   MAX_TIMS];
static void      *timThread[MAX_TIMS];
static void      *timMutex[ MAX_TIMS];
static int        timUnset[ MAX_TIMS];
static int        timHold[  MAX_TIMS];

// func pointers  name in DLL       name in OpenCSM     description
// -------------  ---------------   ------------------  -----------------------------------------------

// tim_Load[]     timLoad()         tim_load()          open a tim
typedef int (*TIM_Load) (esp_T *ESP, /*@null@*/void *data);
static        TIM_Load  tim_Load[MAX_TIMS];

// tim_Mesg[]     timMesg()         tim_mesg()          get command, process, and return response
typedef int (*TIM_Mesg) (esp_T *ESP, char command[]);
static        TIM_Mesg  tim_Mesg[MAX_TIMS];

// tim_Save[]     timSave()         tim_save()          save tim data and close tim instance
typedef int (*TIM_Save) (esp_T *ESP);
static        TIM_Save  tim_Save[MAX_TIMS];

// tim_Quit[]     timQuit()         tim_quit()          close tim instance without saving
typedef int (*TIM_Quit) (esp_T *ESP, int unload);
static        TIM_Quit  tim_Quit[MAX_TIMS];

//                                  tim_free()          frees up memory associated with TIMs

// tim_BegPython[] timBegPython
typedef int (*TIM_BegPython) ();
static        TIM_BegPython  tim_BegPython[MAX_TIMS];

// tim_EndPython[] timEndPython
typedef int (*TIM_EndPython) ();
static        TIM_EndPython  tim_EndPython[MAX_TIMS];

static int tim_nTim = 0;
static int tim_PythonState = 0;  /* default value */
                                 /* =1  python needs to be initialized */
                                 /* =2  python has been    initialized */

/* structure to communicate between threads */
typedef struct {
    TIM_Mesg routine;
    char     *command;
    void     *data;
    int      *state;
} emp_T;

/* prototypes */
//$$$int             tim_load(char myTimName[], esp_T *ESP, void *data);
//$$$void           *tim_getEsp(char myTimName[]);
//$$$int             tim_mesg(char myTimName[], char command[]);
//$$$int             tim_hold(char myTimName[], char overlay[]);
//$$$int             tim_lift(char myTimName[]);
//$$$int             tim_bcst(char myTimName[], char text[]);
//$$$int             tim_save(char myTimName[]);
//$$$int             tim_quit(char myTimName[]);
//$$$void            tim_lock();
//$$$void            tim_free();
//$$$int             update_esp();

static /*@null@*/DLL timDLopen(const char *name);
static void          timDLclose(/*@unused@*/ /*@only@*/ DLL dll);
static timDLLfunc    timDLget(DLL dll, const char *symname, /*@null@*/ const char *name);
static int           timDLoaded(const char *name);
static int           timDYNload(const char *name);

       int           GetToken(char text[], int nskip, char sep, char *token[]);
static void          timExec(void *);
static int           OverlayInContMode(char command[], char myTimName[]);

/*@-mustfreefresh@*/

//#define TRACE_BROADCAST(BUFFER)  if (strlen(BUFFER) > 0) printf("<<< server2browser: %.80s\n", BUFFER)
#ifndef TRACE_BROADCAST
   #define TRACE_BROADCAST(BUFFER)
#endif


/***********************************************************************/
/*                                                                     */
/*   tim_load - load a TIM                                             */
/*                                                                     */
/***********************************************************************/

int
tim_load(char       myTimName[],        /* (in)  name of TIM */
         esp_T      *ESP,               /* (in)  pointer to ESP structure */
/*@null@*/void      *data)              /* (in)  user-provided data */
{
    int     status = SUCCESS;           /* (out) return status */

    int     i, j;

    /* --------------------------------------------------------------- */

    if (SHOW_STATES) printf("tim_load(%s)\n", myTimName);

    /* skip this TIM if spawned from pyscript while in continuation mode */
    if (OverlayInContMode("tim_load", myTimName) == 1) {
        goto cleanup;
    }

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, try to load it now */
    if (i == -1) {
        i = timDYNload(myTimName);
        if (i < 0) {
            printf("ERROR:: tim_load(%s) could not be dynamically loaded\n", myTimName);
            status = i;
            goto cleanup;
        }

        timState[i] = TIM_INACTIVE;
        if (SHOW_STATES) printf("in tim_load(%s): setting TIM_INACTIVE\n", myTimName);
    }

    /* if not currently "inactive", return an error */
    if (timState[i] != TIM_INACTIVE) {
        printf("ERROR:: tim_load(%s) is not inactive, state=%d\n", myTimName, timState[i]);
        status = EGADS_SEQUERR;
        goto cleanup;
    }

    timState[i] = TIM_LOADING;
    if (SHOW_STATES) printf("in tim_load(%s): setting TIM_LOADING\n", myTimName);

    /* if this is python, load it now */
    if (strcmp(myTimName, "pyscript") == 0 && tim_PythonState == 0) {
        /* setting this causes tim_lift (which runs in the main thread)
           to initialize python and numpy */
        tim_PythonState = 1;

        /* wait until it is loaded */
        for (j = 0; j < 100; j++) {
            if (tim_PythonState == 2) break;

            SLEEP(100);
        }
    }

    /* save the data and call the load routine in the TIM */
    timEsp[i] = ESP;

    status = tim_Load[i](ESP, data);
    if (status == EGADS_SEQUERR) {
        timState[i] = TIM_NOTAVAIL;
        if (SHOW_STATES) printf("in tim_load(%s): setting TIM_NOTAVAIL\n", myTimName);
        status = EGADS_SUCCESS;
        goto cleanup;
    } else if (status < EGADS_SUCCESS) {
        printf("ERROR:: tim_load(%s) returned status=%d\n", myTimName, status);
        goto cleanup;
    }

    /* remember if we should run this tim in the same or a new thread */
    timHold[i] = status;

    /* mark the TIM as being ready */
    if (timState[i] != TIM_INACTIVE) {
        timState[i] = TIM_READY;
        if (SHOW_STATES) printf("in tim_load(%s): setting TIM_READY\n", myTimName);
    }

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   tim_getEsp - get the ESP structure (needed by esp.py)             */
/*                                                                     */
/***********************************************************************/

/*@null@*/void *
tim_getEsp(char myTimName[])            /* (in)  name of TIM */
{
    int  i;

    /* --------------------------------------------------------------- */

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, return an error */
    if (i == -1) {
        printf("WARNING:: tim_getEsp(%s) is not currently loaded\n", myTimName);
        return NULL;
    }

//cleanup:
    return timEsp[i];
}


/***********************************************************************/
/*                                                                     */
/*   tim_mesg - process a message in the TIM (main execution method)   */
/*                                                                     */
/***********************************************************************/

int
tim_mesg(char   myTimName[],            /* (in)  name of TIM */
         char   command[])              /* (in)  command to process */
{
    int     status = SUCCESS;           /* (out) return status */

    int     i;
    emp_T   *myEmp=NULL;

    ROUTINE(tim_mesg);

    /* --------------------------------------------------------------- */

    if (SHOW_STATES) printf("tim_mesg(%s, %s)\n", myTimName, command);

    /* skip this TIM if spawned from pyscript while in continuation mode */
    if (OverlayInContMode("tim_mesg", myTimName) == 1) {
        goto cleanup;
    }

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, return an error */
    if (i == -1) {
        printf("ERROR:: tim_mesg(%s) is not currently loaded\n", myTimName);
//$$$        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    /* if not available, return immediately */
    if (timState[i] == TIM_NOTAVAIL) {
        goto cleanup;
    }

    /* wait here if a previous spawned process is still running */
    if (timThread[i] != NULL) {
        EMP_ThreadWait(timThread[i]);
    }

    /* allow overlay to run when currently executing */
    if (timState[i] == TIM_EXECUTING) {
        if (strcmp(myTimName, "viewer")    == 0 ||
            strcmp(myTimName, "plotter")   == 0 ||
            strcmp(myTimName, "flowchart") == 0   ) {
            timState[i] = TIM_READY;
            if (SHOW_STATES) printf("tim_mesg(%s): changing TIM_EXECUTING to TIM_READT\n", myTimName);
        }
    }

    /* make sure TIM is currently ready */
    if (timState[i] != TIM_READY) {
        printf("ERROR:: tim_mesg(%s) is not in ready, state=%d\n", myTimName, timState[i]);
        status = EGADS_SEQUERR;
        goto cleanup;
    }

    /* destroy a thread that may be left over from previous call */
    if (timThread[i] != NULL) {
        EMP_ThreadDestroy(timThread[i]);
        timThread[i] = NULL;
    }

    timState[i] = TIM_EXECUTING;
    if (SHOW_STATES) printf("in tim_mesg(%s): setting TIM_EXECUTING\n", myTimName);

    /* if we are holding until released, call the message routine directly */
    if (timHold[i] == 1) {
        status = tim_Mesg[i](timEsp[i], command);
        if (status < EGADS_SUCCESS) {
            printf("ERROR:: tim_mesg(%s, %s) returned status=%d\n", myTimName, command, status);
            goto cleanup;
        }

        /* mark the TIM as being ready */
        timState[i] = TIM_READY;
        if (SHOW_STATES) printf("in tim_mesg(%s): setting TIM_READY\n", myTimName);

    /* otherwise we need to create a thread in which the message routine will run */
    } else {

        MALLOC(myEmp, emp_T, 1);

        myEmp->routine = tim_Mesg[i];
        myEmp->command = NULL;
        myEmp->data    = timEsp[i];
        /*@ignore@*/
        myEmp->state   = &(timState[i]);
        /*@end@*/

        MALLOC(myEmp->command, char, STRLEN(command)+1);
        strcpy(myEmp->command, command);

        timThread[i] = EMP_ThreadCreate(timExec, (void*)(myEmp));
    }

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   tim_hold - hold the TIM until the lock is released                */
/*                                                                     */
/***********************************************************************/

int
tim_hold(char   myTimName[],            /* (in)  name of TIM to hold */
         char   overlay[])              /* (in)  name of overlay to lift hold */
{
    int     status = SUCCESS;           /* (out) return status */

    int     i;
    char    message[MAX_EXPR_LEN];
    esp_T   *ESP=NULL;

    ROUTINE(tim_mesg);

    /* --------------------------------------------------------------- */

    if (SHOW_STATES) printf("tim_hold(%s, %s)\n", myTimName, overlay);

    /* skip this TIM if spawned from pyscript while in continuation mode */
    if (OverlayInContMode("tim_hold", overlay) == 1) {
        goto cleanup;
    }

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, return an error */
    if (i == -1) {
        printf("ERROR:: tim_hold(%s) is not currently loaded\n", myTimName);
        goto cleanup;
    }

    /* if not available, return immediately */
    if (timState[i] == TIM_NOTAVAIL) {
        goto cleanup;
    }

    /* send a message to the browsers to tell them which hold to lift */
    snprintf(message, MAX_EXPR_LEN, "overlayBeg|%s|%s|", myTimName, overlay);
    tim_bcst(myTimName, message);

    /* now wait until the browswer lifts the hold */
    EMP_LockSet(timMutex[i]);

    /* this thread should not set the lock, so it is immediately unlocked */
    EMP_LockRelease(timMutex[i]);

    /* wait for the lock to be set again */
    while (EMP_LockTest(timMutex[i]) == 0) {
        SLEEP(100);
    }

    /* reset the EGADS thread */
    ESP = (esp_T *)(timEsp[i]);
    if (ESP->MODL->context != NULL) {
        status = EG_updateThread(ESP->MODL->context);
        CHECK_STATUS(EG_updateThread);
    }

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   tim_lift - ask tim_lock to lift the hold                          */
/*                                                                     */
/***********************************************************************/

int
tim_lift(char   myTimName[])            /* (in)  name of TIM */
{
    int     status = SUCCESS;           /* (out) return status */

    int     i;

    ROUTINE(tim_lift);

    /* --------------------------------------------------------------- */

    if (SHOW_STATES) printf("tim_lift(%s)\n", myTimName);

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, return an error */
    if (i == -1) {
        printf("ERROR:: tim_lift(%s) is not currently loaded\n", myTimName);
//$$$        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    timUnset[i] = 1;

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   tim_bcst - broadcast a message to all browsers                    */
/*                                                                     */
/***********************************************************************/

int
tim_bcst(char   myTimName[],            /* (in)  name of TIM */
         char   text[])                 /* (in)  text to broadcast */
{
    int     status = SUCCESS;           /* (out) return status */

    int     i;
    esp_T   *ESP=NULL;

    /* --------------------------------------------------------------- */

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, return an error */
    if (i == -1) {
        printf("ERROR:: tim_bcst(%s) is not currently loaded\n", myTimName);
//$$$        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    /* if not available, return immediately */
    if (timState[i] == TIM_NOTAVAIL) {
        goto cleanup;
    }

    ESP = (esp_T *) timEsp[i];

    if (ESP == NULL) {
        goto cleanup;
    }

    /* only broadcast if wv is currently running */
    if (ESP->cntxt == NULL) {
        goto cleanup;
    }

    /* if we have a MODL and its outLevel >= 2, write tracing info to STDOUT */
    if (ESP->MODL != NULL) {
        if (ocsmSetOutLevel(-1) >= 2) {
            if (strlen(text) > 0) printf("\n<<< server2browser: %s\n", text);
        }
    }

    /* broadcast the message */
    TRACE_BROADCAST( text);
    wv_broadcastText(text);

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   tim_save - save data associated with TIM and close it             */
/*                                                                     */
/***********************************************************************/

int
tim_save(char   myTimName[])            /* (in)  name of TIM */
{
    int     status = SUCCESS;           /* (out) return status */

    int     i;

    /* --------------------------------------------------------------- */

    if (SHOW_STATES) printf("tim_save(%s)\n", myTimName);

    /* skip this TIM if spawned from pyscript while in continuation mode */
    if (OverlayInContMode("tim_save", myTimName) == 1) {
        goto cleanup;
    }

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, return an error */
    if (i == -1) {
        printf("ERROR:: tim_save(%s) is not currently loaded\n", myTimName);
//$$$        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    /* if not available, return immediately */
    if (timState[i] == TIM_NOTAVAIL) {
        goto cleanup;
    }

    /* wait here if a previous spawned process is still running */
    if (timThread[i] != NULL) {
        EMP_ThreadWait(timThread[i]);
    }

    /* make sure TIM is currently ready */
    if (timState[i] != TIM_READY) {
        printf("ERROR:: tim_save(%s) is not in ready, state=%d\n", myTimName, timState[i]);
        status = EGADS_SEQUERR;
        goto cleanup;
    }

    timState[i] = TIM_CLOSING;
    if (SHOW_STATES) printf("in tim_save(%s): setting TIM_CLOSING\n", myTimName);

    /* call the save routine in the TIM */
    status = tim_Save[i](timEsp[i]);
    if (status < EGADS_SUCCESS) {
        printf("ERROR:: tim_save(%s) returned status=%d\n", myTimName, status);
        goto cleanup;
    }

    /* destroy the thread associated with this TIM */
    if (timThread[i] != NULL) {
        EMP_ThreadDestroy(timThread[i]);
        timThread[i] = NULL;
    }

    /* mark the TIM as being inactive */
    timState[i] = TIM_INACTIVE;
    if (SHOW_STATES) printf("in tim_save(%s): setting TIM_INACTIVE\n", myTimName);

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   tim_quit - remove data associated with TIM and close it           */
/*                                                                     */
/***********************************************************************/

int
tim_quit(char   myTimName[])            /* (in)  name of TIM */
{
    int     status = SUCCESS;           /* (out) return status */

    int     i;

    /* --------------------------------------------------------------- */

    if (SHOW_STATES) printf("tim_quit(%s)\n", myTimName);

    /* skip this TIM if spawned from pyscript while in continuation mode */
    if (OverlayInContMode("tim_quit", myTimName) == 1) {
        goto cleanup;
    }

    /* determine if dynamic library is already loaded */
    i = timDLoaded(myTimName);

    /* if not loaded, return an error */
    if (i == -1) {
        printf("ERROR:: tim_quit(%s) is not currently loaded\n", myTimName);
//$$$        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    /* if not available, return immediately */
    if (timState[i] == TIM_NOTAVAIL) {
        goto cleanup;
    }

    /* wait here if a previous spawned process is still running */
    if (timThread[i] != NULL) {
        EMP_ThreadWait(timThread[i]);
    }

    /* if this is called during TimLoad, simply reset the state so that
       it can be loaded again */
    if (timState[i] == TIM_LOADING) {
        timState[i] = TIM_INACTIVE;
        if (SHOW_STATES) printf("in tim_quit(%s): setting TIM_INACTIVE\n", myTimName);
        goto cleanup;
    }

    /* make sure TIM is currently ready */
    if (timState[i] != TIM_READY) {
        printf("ERROR:: tim_quit(%s) is not in ready or loading, state=%d\n", myTimName, timState[i]);
        status = EGADS_SEQUERR;
        goto cleanup;
    }

    timState[i] = TIM_CLOSING;
    if (SHOW_STATES) printf("in tim_quit(%s): setting TIM_CLOSING\n", myTimName);

    /* call the quit routine in the TIM */
    status = tim_Quit[i](timEsp[i], 0);
    if (status < EGADS_SUCCESS) {
        printf("ERROR:: tim_quit(%s) returned status=%d\n", myTimName, status);
        goto cleanup;
    }

    /* destroy the thread associated with this TIM and reset the parent pointer */
    if (timThread[i] != NULL) {
        EMP_ThreadDestroy(timThread[i]);
        timThread[i] = NULL;
    }

    /* mark the TIM as being inactive */
    timState[i] = TIM_INACTIVE;
    if (SHOW_STATES) printf("in tim_quit(%s): setting TIM_INACTIVE\n", myTimName);

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   tim_lock - set lock on all TIMs                                   */
/*                                                                     */
/***********************************************************************/

void
tim_lock(int finalize)
{
    int i, j, status; //, ival;

    ROUTINE(tim_lock);

    /* --------------------------------------------------------------- */

    /* finalize python if needed */
    if (finalize == 1) {
        if (tim_PythonState == 2) {
            for (j = 0; j < tim_nTim; j++) {
                if (strcmp(timName[j], "pyscript") == 0) {
                    status = tim_EndPython[j]();
                    if (status != SUCCESS) {
                        printf("ERROR:: tim_EndPython -> status=%d\n", status);
                    }
                    goto cleanup;
                }
            }
            printf("ERROR:: tim_EndPython: could not find suitable j (tim_nTim=%d)\n", tim_nTim);
        }

        goto cleanup;
    }

    /* loop through the TIMs that are currently loaded */
    for (i = 0; i < tim_nTim; i++) {

        /* if the flag asks that the lock be released, do so now */
        if (timUnset[i] == 1) {
            EMP_LockRelease(timMutex[i]);

        /* if not set, set it now */
        } else if (EMP_LockTest(timMutex[i]) == 0) {
            EMP_LockSet(timMutex[i]);
        }

        timUnset[i] = 0;
    }

    /* if python needs to be initialized (because of an initializtion
       problem in numpy), do it now */
    if (tim_PythonState == 1) {
        for (j = 0; j < tim_nTim; j++) {
            if (strcmp(timName[j], "pyscript") == 0) {
                status = tim_BegPython[j]();
                if (status != 0) printf("ERROR:: tim_BegPython -> status=%d\n", status);
                break;
            }
        }

        tim_PythonState = 2;
    }

cleanup:
    return;
}


/***********************************************************************/
/*                                                                     */
/*   tim_free - free all data associated with TIMs (called at end)     */
/*                                                                     */
/***********************************************************************/

void
tim_free()
{
    int i, status;

    /* --------------------------------------------------------------- */

    if (SHOW_STATES) printf("tim_free()\n");

    /* loop through the TIMs that are currently loaded, and quit them all */
    for (i = 0; i < tim_nTim; i++) {

        /* free up the data associated with the TIM */
        if (timEsp[i] != NULL) {
            status = tim_Quit[i](timEsp[i], 1);
            if (status < EGADS_SUCCESS) {
                printf("ERROR:: tim_free(%s) quit returned status=%d\n", timName[i], status);
                goto cleanup;
            }
        }

        /* free up any threads */
        if (timThread[i] != NULL) {
            EMP_ThreadDestroy(timThread[i]);
            timThread[i] = NULL;
        }

        /* free up any mutexs */
        if (timMutex[i] != NULL) {
            EMP_LockDestroy(timMutex[i]);
            timMutex[i] = NULL;
        }

        /* remove the TIM name from the dispatch table */
        if (timName[i] != NULL) {
            free(timName[i]);
        }

        /* close the dynamically loaded routines */
        timDLclose(timDLL[i]);
    }

    /* we have no more active TIMs */
    tim_nTim = 0;

cleanup:
    return;
}


/***********************************************************************/
/*                                                                     */
/*   update_display - update ESP after rebuilding                      */
/*                                                                     */
/***********************************************************************/

int
update_esp()
{

    int    i, status=EGADS_SUCCESS;
//$$$    int    nbody, builtTo;
    esp_T  *ESP=NULL;

    /* --------------------------------------------------------------- */

    /* determine if dynamic library is already loaded */
    i = timDLoaded("pyscript");

    /* if not loaded, return an error */
    if (i == -1) {
        printf("ERROR:: tim_bcst(%s) is not currently loaded\n", "pyscript");
//$$$        status = EGADS_NOTFOUND;
        goto cleanup;
    }

    ESP = (esp_T *) timEsp[i];

    /* rebuild (hopefully just recycling) */
//$$$    nbody = 0;
//$$$    status = ocsmBuild(ESP->MODL, 0, &builtTo, &nbody, NULL);
//$$$    if (status < EGADS_SUCCESS) goto cleanup;

    /* tessellate the Bodys if not already tessellated */
    status = ocsmTessellate(ESP->MODL, 0);
    if (status < EGADS_SUCCESS) goto cleanup;

    /* update the Cvals */
    if (ESP->CAPS != NULL) {
        wv_postMessage(0, "timMesg|capsMode|getCvals|");
    }

    /* update the Pmtrs */
    wv_postMessage(0, "getPmtrs|");

    /* update the Brchs */
    if (ESP->CAPS == NULL) {
        wv_postMessage(0, "getBrchs|");
    }

cleanup:
    return status;
}


/***********************************************************************/
/*                                                                     */
/*   Utility Functions                                                 */
/*       timDLopen  - open a TIM                                       */
/*       timDLclose - close a TIM                                      */
/*       timDLget   - get function associated with a TIM               */
/*       timDLoaded - determine if a TIM is loaded                     */
/*       timDYNload - dynamically load a TIM                           */
/*       GetToken   - extract a token from a string                    */
/*                                                                     */
/***********************************************************************/

static /*@null@*/ DLL
timDLopen(const char *name)
{
    int             i, len;
    DLL             dll;
    char            *full, *env;
#ifdef WIN32
    WIN32_FIND_DATA ffd;
    char            dir[MAX_PATH];
    size_t          length_of_arg;
    HANDLE hFind =  INVALID_HANDLE_VALUE;
#else
    /*@-unrecog@*/
    char            dir[PATH_MAX];
    /*@+unrecog@*/
    struct dirent   *de;
    DIR             *dr;
#endif

    /* --------------------------------------------------------------- */

    env = getenv("ESP_ROOT");
    if (env == NULL) {
        printf("WARNING:: Could not find $ESP_ROOT\n");
        return NULL;
    }

    if (name == NULL) {
        printf("WARNING:: Dynamic Loader invoked with NULL name\n");
        return NULL;
    }
    len  = strlen(name);
    full = (char *) malloc((len+5)*sizeof(char));
    if (full == NULL) {
        printf("WARNING:: Dynamic Loader MALLOC Error\n");
        return NULL;
    }

    for (i = 0; i < len; i++) {
        full[i] = name[i];
    }
    full[len  ] = '.';
#ifdef WIN32
    full[len+1] = 'D';
    full[len+2] = 'L';
    full[len+3] = 'L';
    full[len+4] =  0;
    snprintf(dir, MAX_PATH, "%s\\lib\\*", env);
    hFind = FindFirstFile(dir, &ffd);
    if (INVALID_HANDLE_VALUE == hFind) {
        printf("WARNING:: Dynamic Loader could not open %s\n", dir);
        free(full);
        return NULL;
    }
    i = 0;
    do {
        if (strcasecmp(full, ffd.cFileName) == 0) i++;
    } while (FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);
    if (i > 1) {
        printf("WARNING:: Dynamic Loader more than 1 file: %s\n", full);
        free(full);
        return NULL;
    }

    if (i == 1) {
        hFind = FindFirstFile(dir, &ffd);
        do {
            if (strcasecmp(full, ffd.cFileName) == 0) break;
        } while (FindNextFile(hFind, &ffd) != 0);
        dll = LoadLibrary(ffd.cFileName);
        FindClose(hFind);
    } else {
        dll = LoadLibrary(full);
    }

#else

    full[len+1] = 's';
    full[len+2] = 'o';
    full[len+3] =  0;
    snprintf(dir, PATH_MAX, "%s/lib", env);
    dr = opendir(dir);
    if (dr == NULL) {
        printf("WARNING:: Dynamic Loader could not open %s\n", dir);
        free(full);
        return NULL;
    }
    i = 0;
    while ((de = readdir(dr)) != NULL)
        /*@-unrecog@*/
        if (strcasecmp(full, de->d_name) == 0) i++;
        /*@+unrecog@*/
    closedir(dr);
    if (i > 1) {
        printf("WARNING:: Dynamic Loader more than 1 file: %s\n", full);
        free(full);
        return NULL;
    }

    if (i == 1) {
        dr = opendir(dir);
        while ((de = readdir(dr)) != NULL)
            if (strcasecmp(full, de->d_name) == 0) break;
        /*@-nullderef@*//*@-unrecog@*/
        dll = dlopen(de->d_name, RTLD_NOW | RTLD_NODELETE | RTLD_GLOBAL);
        /*@+nullderef@*//*@+unrecog@*/
        closedir(dr);
    } else {
        /*@-nullderef@*//*@-unrecog@*/
        dll = dlopen(full, RTLD_NOW | RTLD_NODELETE | RTLD_GLOBAL);
        /*@+nullderef@*//*@+unrecog@*/
    }
#endif

    if (!dll) {
        printf("WARNING:: Dynamic Loader for %s not found\n", full);
#ifndef WIN32
        printf("              %s\n", dlerror());
#endif
        free(full);
        return NULL;
    }
    free(full);

    return dll;
}


/* ------------------------------------------------------------------------- */

static void
timDLclose(/*@unused@*/ /*@only@*/ DLL dll)
{
#ifdef WIN32
    FreeLibrary(dll);
#else
    dlclose(dll);
#endif
}


/* ------------------------------------------------------------------------- */

static timDLLfunc
timDLget(DLL dll,
         const char *symname,
         /*@null@*/ const char *name)
{
    timDLLfunc data;

    /* --------------------------------------------------------------- */

#ifdef WIN32
    data = (timDLLfunc) GetProcAddress(dll, symname);
#else
    /*@-castfcnptr@*/
    data = (timDLLfunc) dlsym(dll, symname);
    /*@+castfcnptr@*/
#endif
    if ((data == NULL) && (name != NULL))
        printf("WARNING:: Couldn't get symbol %s in %s\n", symname, name);
    return data;
}


/* ------------------------------------------------------------------------- */

static int
timDLoaded(const char *name)
{
    int i;

    /* --------------------------------------------------------------- */

    for (i = 0; i < tim_nTim; i++) {
        if (strcasecmp(name, timName[i]) == 0) {
            return i;
        }
    }

    return -1;
}


/* ------------------------------------------------------------------------- */

static int
timDYNload(const char *name)
{
    int i, len, ret;
    DLL dll;

    /* --------------------------------------------------------------- */

    if (tim_nTim >= MAX_TIMS) {
        printf("WARNING:: Number of Primitives %d > %d\n", tim_nTim, MAX_TIMS);
        return EGADS_INDEXERR;
    }
    dll = timDLopen(name);
    if (dll == NULL) return EGADS_NULLOBJ;

    ret = tim_nTim;
    tim_Load[ret] = (TIM_Load) timDLget(dll, "timLoad", name);
    tim_Mesg[ret] = (TIM_Mesg) timDLget(dll, "timMesg", name);
    tim_Save[ret] = (TIM_Save) timDLget(dll, "timSave", name);
    tim_Quit[ret] = (TIM_Quit) timDLget(dll, "timQuit", name);
    if ((tim_Load[ret] == NULL) ||
        (tim_Mesg[ret] == NULL) ||
        (tim_Save[ret] == NULL) ||
        (tim_Quit[ret] == NULL)   ) {
        timDLclose(dll);
        return EGADS_EMPTY;
    }

    if (strcmp(name, "pyscript") == 0) {
        tim_BegPython[ret] = (TIM_BegPython) timDLget(dll, "timBegPython", name);
        tim_EndPython[ret] = (TIM_EndPython) timDLget(dll, "timEndPython", name);
    } else {
        tim_BegPython[ret] = NULL;
        tim_EndPython[ret] = NULL;
    }

    len  = strlen(name) + 1;
    timName[ ret] = (char *) malloc(len*sizeof(char));
    if (timName[ret] == NULL) {
        printf("WARNING:: timName could not be malloced\n");
        timDLclose(dll);
        return EGADS_MALLOC;
    }
    for (i = 0; i < len; i++) {
        timName[ret][i] = name[i];
    }

    timDLL[   ret] = dll;
    timState[ ret] = TIM_INACTIVE;
    timEsp[   ret] = NULL;
    timThread[ret] = NULL;
    timMutex[ ret] = EMP_LockCreate();
    if (timMutex[ret] == NULL) {
        printf("WARNING:: timMutex could not be created\n");
        /*@-dependenttrans@*/
        timDLclose(dll);
        /*@+dependenttrans@*/
        free(timName[ret]);
        return EGADS_MALLOC;
    }
    timUnset[ ret] = 0;
    timHold[  ret] = 0;
    tim_nTim++;

    if (SHOW_STATES) printf("in timDYNload(%s): setting TIM_INACTIVE\n", name);

    return ret;
}


/***********************************************************************/
/*                                                                     */
/*   GetToken - pull a token out of a string                           */
/*                                                                     */
/***********************************************************************/

int
GetToken(char   text[],                 /* (in)  full text */
         int    nskip,                  /* (in)  tokens to skip */
         char   sep,                    /* (in)  separator character */
         char   *token[])               /* (out) token (freeable) */
{
    int    status = SUCCESS;

    int    lentok, i, count, iskip;

    ROUTINE(GetToken);

    /* --------------------------------------------------------------- */

    MALLOC(*token, char, strlen(text));

    (*token)[0] = '\0';
    lentok   = 0;

    /* convert tabs to spaces */
    for (i = 0; i < STRLEN(text); i++) {
        if (text[i] == '\t') {
            text[i] = ' ';
        }
    }

    /* count the number of separators */
    count = 0;
    for (i = 0; i < STRLEN(text); i++) {
        if (text[i] == sep) {
            count++;
        }
    }

    if (count < nskip) return 0;

    /* skip over nskip tokens */
    i = 0;
    for (iskip = 0; iskip < nskip; iskip++) {
        while (text[i] != sep) {
            i++;
        }
        i++;
    }

    /* if token we are looking for is empty, return 0 */
    if (text[i] == sep) {
        (*token)[0] = '0';
        (*token)[1] = '\0';
    }

    /* extract the token we are looking for */
    while (text[i] != sep && text[i] != '\0') {
        (*token)[lentok++] = text[i++];
        (*token)[lentok  ] = '\0';
    }

cleanup:
    if (status == SUCCESS) {
        return STRLEN(*token);
    } else {
        return status;
    }
}


/***********************************************************************/
/*                                                                     */
/*   timExec - routine that actually executes a message                */
/*                                                                     */
/***********************************************************************/

static void
timExec(void *s)
{
    emp_T  *myEmp = (emp_T *) s;

    ROUTINE(timExec);

    /* --------------------------------------------------------------- */

    /* execute timMesg */
    myEmp->routine((esp_T *)(myEmp->data), myEmp->command);

    /* we are now redy */
    *(myEmp->state) = TIM_READY;
    if (SHOW_STATES) printf("in timExec: setting TIM_READY\n");

    /* free up the emp_T structure (that was allocated in tim_mesg) */
    FREE(myEmp->command);
    FREE(myEmp);
}


/***********************************************************************/
/*                                                                     */
/*   OverlayInContMode - check if we are trying to execute an overlay  */
/*                 while in continuation mode                          */
/*                                                                     */
/***********************************************************************/

static int
OverlayInContMode(char   command[],     /* (in)  command name */
                  char   myTimName[])   /* (in)  name of TIM */
{
    int      status = 0;                /* (out) =1 if an overlay in continuation mode */

    esp_T    *ESP=NULL;

    ROUTINE(OverlayInContMode);

    /* --------------------------------------------------------------- */

    if (tim_nTim < 1) goto cleanup;

    /* check to see if:
     * the new TIM is not "pyscript"
     * the parent is "pyscript" and
     * the parent is currently running is "CaPsTeMpFiLe.py" */
    if (strcmp(myTimName, "pyscript") != 0) {
        ESP = (esp_T *)timEsp[tim_nTim-1];
        SPLINT_CHECK_FOR_NULL(ESP);

        if (ESP->nudata > 0) {
            if (strcmp(ESP->timName[ESP->nudata-1], "pyscript") == 0) {
                if (strcmp((char *)(ESP->udata[ESP->nudata-1]), "CaPsTeMpFiLe.py") == 0) {
                    printf("skipping \"%s %s\" since we are in continuation mode \n", command, myTimName);
                    status = 1;
                }
            }
        }
    }

cleanup:
    return status;
}
