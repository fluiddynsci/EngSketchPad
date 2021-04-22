/*
 ************************************************************************
 *                                                                      *
 * template_avl.c -- c version of template_avl.py                       *
 *                                                                      *
 *            Written by  Nitin Bhagat @ UDRI                           *
 *            Modified by John Dannenhoffer @ Syracuse University       *
 *                                                                      *
 ************************************************************************
 */

/*
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

#include "caps.h"
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
   #define unlink      _unlink
   #define getcwd      _getcwd
   #define snprintf    _snprintf
   #define PATH_MAX    _MAX_PATH
   #define strcasecmp  stricmp
#else
   #include <limits.h>
   #include <unistd.h>
#endif

#define STANDALONE 1

/***********************************************************************/
/*                                                                     */
/*   helper functions for dealing with getting/setting values          */
/*                                                                     */
/***********************************************************************/

static int
setValueD(capsObj probObj,              /* (in)  problem object */
          int     type,                 /* (in)  variable type */
          char    name[],               /* (in)  variable name */
          double  value)                /* (in)  value to set */
{
    int  status = CAPS_SUCCESS;

    capsObj valObj;
    const double     *data;
    int              vlen;
    const char       *units;
    enum capsvType   vtype;

    int       nErr;
    capsErrs  *errors;

    /* --------------------------------------------------------------- */

    status = caps_childByName(probObj, VALUE, type, name, &valObj);
    if (status != CAPS_SUCCESS) {
        printf("caps_childByName(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

    status = caps_getValue(valObj, &vtype, &vlen, (const void**)(&data), &units, &nErr, &errors);
    if (status != CAPS_SUCCESS) {
        printf("caps_getValue(%s) -> status=%d\n", name, status);
        goto cleanup;
    } else if (vtype != Double) {
        printf("caps_setValue(%s) is a expecting a Double: vtype = %d\n", name, vtype);
        status = CAPS_BADTYPE;
        goto cleanup;
    }

    status = caps_setValue(valObj, 1, 1, (void *)(&value));
    if (status != CAPS_SUCCESS) {
        printf("caps_setValue(%s) -> status=%d\n", name, status);
        goto cleanup;
    }

cleanup:
    return status;
}

static int
getValueD(capsObj   analObj,            /* (in)  analysis object */
          int       type,               /* (in)  variable type */
          char      name[],             /* (in)  variable name */
          int       *nErr,              /* (out) number of errors */
          capsErrs  **errors,           /* (out) array  of errors */
          double    *value)             /* (out) value */
{
    int  status = CAPS_SUCCESS;

    capsObj          valObj;
    const double     *data;
    int              vlen;
    const char       *units;
    enum capsvType   vtype;

    /* --------------------------------------------------------------- */

    *value = 0;

    status = caps_childByName(analObj, VALUE, type, name, &valObj);
    if (status != CAPS_SUCCESS) {
        goto cleanup;
    }

    status = caps_getValue(valObj, &vtype, &vlen, (const void**)(&data), &units, nErr, errors);
    if (status != CAPS_SUCCESS) {
        printf("caps_getValue -> status=%d\n", status);
        goto cleanup;
    } else if (vtype != Double || vlen != 1) {
        printf("caps_getValue(%s) was a expecting a single Double\n", name);
        status = CAPS_BADTYPE;
        goto cleanup;
    }

    *value = data[0];

cleanup:
    return status;
}

/***********************************************************************/
/*                                                                     */
/*   run_avl - run AVL with given .csm file                            */
/*                                                                     */
/***********************************************************************/

int
run_avl(char   filename[],              /* (in)  name of .csm or .cdc file */
        double coefs[])                 /* (out) force/mement coefficients */
                                        /*       [0] CLtot  lift    coefficient */
                                        /*       [1] CDtot  drag    coefficient */
                                        /*       [2] CXtot  X-force coefficient */
                                        /*       [3] CYtot  Y-force coefficient */
                                        /*       [4] CZtot  Z-force coefficient */
                                        /*       [5] Cltot  yaw     coefficient */
                                        /*       [6] Cmtot  pitch   coefficient */
                                        /*       [7] Cntot  roll    coefficient */
                                        /*       [8] e      Oswald efficiency */
{
    int        status = 0;              /* (out) return status */

    int i; // Indexing

    // CAPS objects
    capsObj          myGeometry, avlObj, valObj;
    capsErrs         *errors;
    capsOwn          current;

    // CAPS return values
    int              nErr, vlenWing=0, vlenHtail=0, ihinge, jhinge, nsurface=0, nhinge=0;
    double           *wHinge, *htHinge, temp;
    char             *name, *stringVal=NULL, workDir[PATH_MAX], currentPath[PATH_MAX], tempStr[1024];
    const char       *units;
    enum capsvType   vtype;
    enum capsoType   type;
    enum capssType   subtype;
    capsObj          link, parent;
    capsTuple        *surfaceTuple=NULL, *hingeTuple=NULL;

    /* --------------------------------------------------------------- */

    /* Add ".csm" if filename does not end with eiter .csm or .cdc */
    if (strcmp(&(filename[strlen(filename)-4]), ".csm") != 0 &&
        strcmp(&(filename[strlen(filename)-4]), ".cdc") != 0   ) {
        strcat(filename, ".csm");
    }

    /* Set working directory for all temporary files */
    strncpy(workDir, "AVL_Analysis", 128);

    /* Make the working directory if it does not exist */
    status = chdir(workDir);
    if (status != 0) {
        printf("\"%s\" does not exist", workDir);
        status = mkdir(workDir, 0x1FF);     /* rwxrwxrwx */
        if (status == 0) {
            printf(" and was made\n");
        } else {
            printf(" and could not be made\n");
            exit(EXIT_FAILURE);
        }
    } else {
        chdir("..");
    }

    /* Initialize capsProblem object from [.csm] file */
    printf("\n==> Loading geometry from file \"%s\"...", filename);
    status = caps_open(filename, "AVL_Example", &myGeometry);
    if (status != CAPS_SUCCESS) {printf("caps_open -> status=%d\n", status); goto cleanup;}

    /* Set geometry variables to enable Avl mode */
    printf("\n==> Setting Build Variables and Geometry Values...\n");

    status = setValueD(myGeometry, GEOMETRYIN, "VIEW:Concept", 0);
    if (status != CAPS_SUCCESS) {printf("setValueD(VIEW:Concept) -> status=%d\n", status); goto cleanup;}

    status = setValueD(myGeometry, GEOMETRYIN, "VIEW:VLM", 1);
    if (status != CAPS_SUCCESS) {printf("setValueD(VIEW:VLM) -> status=%d\n", status); goto cleanup; }

    /* Set other geometry specific variables */
//$$$    status = setValueD(myGeometry, GEOMETRYIN, "wing:aspect", 12);
//$$$    if (status != CAPS_SUCCESS) {printf("setValueD(wong:aspect) -> status=%d\n", status); goto cleanup;}

    /* Load AVL AIM */
    printf("\n==> Loading AVL aim...\n");
    status = caps_load(myGeometry, "avlAIM", workDir, NULL, NULL, 0, NULL, &avlObj);
    if (status != CAPS_SUCCESS) {printf("caps_load -> status=%d\n", status); goto cleanup;}

    /* Set analysis specific variables */
    status = setValueD(avlObj, ANALYSISIN, "Mach", 0.5);
    if (status != CAPS_SUCCESS) {printf("setValueD(Mach) -> status=%d\n", status); goto cleanup;}

    status = setValueD(avlObj, ANALYSISIN, "Alpha", 10.0);
    if (status != CAPS_SUCCESS) {printf("setValueD(Alpha) -> status=%d\n", status); goto cleanup;}

    status = setValueD(avlObj, ANALYSISIN, "Beta", 0.0);
    if (status != CAPS_SUCCESS) {printf("setValueD(Beta) -> status=%d\n", status); goto cleanup;}

    /* Tuple that will accumulate AVL_Surface info */
    surfaceTuple = (capsTuple *) EG_alloc(2*sizeof(capsTuple));  /* enough room for Wing and Htail */
    if (surfaceTuple == NULL) {status = CAPS_NULLVALUE; goto cleanup;}

    /* If .csm file has a Wing, set up AVL-specific variables for the wing */
    status = getValueD(myGeometry, GEOMETRYIN, "COMP:Wing", &nErr, &errors, &temp);
    if (status == CAPS_SUCCESS) {
        surfaceTuple[nsurface].name  = EG_strdup("Wing");
        surfaceTuple[nsurface].value = EG_strdup("{\"numChord\": 10, \"spaceChord\": 1.0, \"numSpanTotal\": 30, \"spaceSpan\": 1.0}");
        nsurface++;
    }

    /* If .csm file has a Htail, set up AVL-specific variables for the htail */
    status = getValueD(myGeometry, GEOMETRYIN, "COMP:Htail", &nErr, &errors, &temp);
    if (status == CAPS_SUCCESS) {
        surfaceTuple[nsurface].name  = EG_strdup("Htail");
        surfaceTuple[nsurface].value = EG_strdup("{\"numChord\": 10, \"spaceChord\": 1.0, \"numSpanTotal\": 20, \"spaceSpan\": 1.0}");
        nsurface++;
    }

    /* Tell AVL about the surfaces */
    status = caps_childByName(avlObj, VALUE, ANALYSISIN, "AVL_Surface", &valObj);
    if (status != CAPS_SUCCESS) {printf("caps_childByName -> status=%d\n", status); goto cleanup;}

    status = caps_setValue(valObj, nsurface, 1, (void **) surfaceTuple);
    if (status != CAPS_SUCCESS) {printf("caps_setValue(AVL_Surface) -> status=%d\n", status); goto cleanup;}

    /* Set up control surface deflections */
    status = caps_childByName(myGeometry, VALUE, GEOMETRYIN, "wing:hinge", &valObj);
    if (status != CAPS_SUCCESS) {printf("caps_childByName(wing:hinge) -> status=%d\n", status);}

    if (status == CAPS_SUCCESS) {
        status = caps_getValue(valObj, &vtype, &vlenWing, (const void**)(&wHinge), &units, &nErr, &errors);
        if (status != CAPS_SUCCESS) {printf("caps_getValue(wing:hinge) -> status=%d\n", status); goto cleanup;}

        if (vtype != Double || vlenWing%9 != 0) {
            printf("vlen must be multiple of 9 Doubles\n");
            status = CAPS_BADTYPE;
            goto cleanup;
        }

        nhinge += vlenWing / 9;
    }

    status = caps_childByName(myGeometry, VALUE, GEOMETRYIN, "htail:hinge", &valObj);
    if (status != CAPS_SUCCESS) {printf("caps_childByName(htail:hinge) -> status=%d\n", status);}

    if (status == CAPS_SUCCESS) {
        status = caps_getValue(valObj, &vtype, &vlenHtail, (const void**)(&htHinge), &units, &nErr, &errors);
        if (status != CAPS_SUCCESS) {printf("caps_getValue(hting:hinge) -> status=%d\n", status); goto cleanup;}

        if (vtype != Double || vlenHtail%9 != 0) {
            printf("vlen must be multiple of 9 Doubles\n");
            status = CAPS_BADTYPE;
            goto cleanup;
        }

        nhinge += vlenHtail / 9;
    }

    if (nhinge > 0) {
        ihinge = 0;

        status = caps_childByName(avlObj, VALUE, ANALYSISIN, "AVL_Control", &valObj);
        if (status != CAPS_SUCCESS) {printf("caps_childByName(AVL_Control) -> status=%d\n", status); goto cleanup;}

        hingeTuple = (capsTuple *) EG_alloc(nhinge*sizeof(capsTuple));
        if (hingeTuple == NULL) {status = CAPS_NULLVALUE; goto cleanup;}

        /* make entry for each wing control surface */
        for (jhinge = 0; jhinge < vlenWing/9; jhinge++) {
            snprintf(tempStr, 1023, "WingHinge%d", jhinge+1);
            hingeTuple[ihinge].name  = EG_strdup(tempStr);

            snprintf(tempStr, 1023, "{\"deflectionAngle\": %f}", wHinge[9*jhinge]);
            hingeTuple[ihinge].value = EG_strdup(tempStr);

            ihinge++;
        }

        for (jhinge = 0; jhinge < vlenHtail/9; jhinge++) {
            snprintf(tempStr, 1023, "HtailHinge%d", jhinge+1);
            hingeTuple[ihinge].name  = EG_strdup(tempStr);

            snprintf(tempStr, 1023, "{\"deflectionAngle\": %f}", htHinge[9*jhinge]);
            hingeTuple[ihinge].value = EG_strdup(tempStr);

            ihinge++;
        }

        status = caps_setValue(valObj, nhinge, 1, (void **)hingeTuple);
        if (status != CAPS_SUCCESS) {printf("caps_setValue(AVL_Control) -> status=%d\n", status); goto cleanup;}
    }

    /* Rum AIM pre-analysis */
    printf("\n==> Running AVL pre-analysis...\n");
    status = caps_preAnalysis(avlObj, &nErr, &errors);
    if (status != CAPS_SUCCESS) {printf("caps_preAnalysis -> status=%d\n", status); goto cleanup;}

    /* Run AVL */
    printf("\n ==> Running AVL...\n");

    /* Get our current working directory (so that we can return here after running AVL) */

    // Execute avl via system call
    (void) getcwd(currentPath, PATH_MAX);

    /* Move into temp directory */
    status = chdir(workDir);
    if (status != 0) {printf("Could not change to %s directory\n", workDir); goto cleanup;}

    /* Run AVL via system call */
#ifdef WIN32
    system("avl.exe caps < avlInput.txt > avlOutput.txt");
#else
    system("avl     caps < avlInput.txt > avlOutput.txt");
#endif

    /* Move back to original directory */
    (void) chdir(currentPath);

    /* Run AIM post-analysis */
    printf("\n==> Running AVL post-analysis...\n");
    status = caps_info(myGeometry, &name, &type, &subtype, &link, &parent, &current);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = caps_postAnalysis(avlObj, current, &nErr, &errors);
    if (status != CAPS_SUCCESS) {printf("caps_postAnalysis -> status=%d\n", status); goto cleanup;}

    /* Build array of force/moment coefficients */
    status = getValueD(avlObj, ANALYSISOUT, "CLtot", &nErr, &errors, &(coefs[0]));
    if (status != CAPS_SUCCESS) {printf("getValueD(CLtot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "CDtot", &nErr, &errors, &(coefs[1]));
    if (status != CAPS_SUCCESS) {printf("getValueD(CDtot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "CXtot", &nErr, &errors, &(coefs[2]));
    if (status != CAPS_SUCCESS) {printf("getValueD(CXtot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "CYtot", &nErr, &errors, &(coefs[3]));
    if (status != CAPS_SUCCESS) {printf("getValueD(CYtot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "CZtot", &nErr, &errors, &(coefs[4]));
    if (status != CAPS_SUCCESS) {printf("getValueD(CZtot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "Cltot", &nErr, &errors, &(coefs[5]));
    if (status != CAPS_SUCCESS) {printf("getValueD(Cltot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "Cmtot", &nErr, &errors, &(coefs[6]));
    if (status != CAPS_SUCCESS) {printf("getValueD(Cmtot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "Cntot", &nErr, &errors, &(coefs[7]));
    if (status != CAPS_SUCCESS) {printf("getValueD(Cntot) -> status=%d\n", status); goto cleanup;}

    status = getValueD(avlObj, ANALYSISOUT, "e",     &nErr, &errors, &(coefs[8]));
    if (status != CAPS_SUCCESS) {printf("getValueD(e) -> status=%d\n", status); goto cleanup;}

cleanup:
    if (status != CAPS_SUCCESS) printf("\n\nPremature exit - status = %d\n", status);

    if (surfaceTuple != NULL) {
        for (i = 0; i < nsurface; i++) {
            if (surfaceTuple[i].name  != NULL) EG_free(surfaceTuple[i].name);
            if (surfaceTuple[i].value != NULL) EG_free(surfaceTuple[i].value);
        }
        EG_free(surfaceTuple);
    }

    if (hingeTuple != NULL) {
        for (i = 0; i < nhinge; i++) {
            if (hingeTuple[i].name  != NULL) EG_free(hingeTuple[i].name);
            if (hingeTuple[i].value != NULL) EG_free(hingeTuple[i].value);
        }
        EG_free(hingeTuple);
    }

    if (stringVal != NULL) EG_free(stringVal);

    (void) caps_close(myGeometry);

    return status;
}

/***********************************************************************/
/*                                                                     */
/*   main - main program                                               */
/*                                                                     */
/***********************************************************************/

#ifdef STANDALONE
int
main(int       argc,                    /* (in)  number of arguments */
     char      *argv[])                 /* (in)  array  of arguments */
{
    int        status = 0;              /* (out) return status */

    double     coefs[9];
    char       filename[PATH_MAX];

    /* --------------------------------------------------------------- */

    /* Get the name of the input file (default to ../ESP/wing1.csm) */
    if (argc < 2) {
#ifdef WIN32
        strncpy(filename, "..\ESP\wing1.csm", PATH_MAX-1);
#else
        strncpy(filename, "../ESP/wing1.csm", PATH_MAX-1);
#endif
    } else {
        strncpy(filename, argv[1],     PATH_MAX-1);
    }

    status = run_avl(filename, coefs);
    if (status != CAPS_SUCCESS) {
        printf("run_avl -> status=%d\n", status);
        goto cleanup;
    }

    printf("\nForce/moment coefficients:\n");
    printf("    CLtot = %f\n", coefs[0]);
    printf("    CDtot = %f\n", coefs[1]);
    printf("    CXtot = %f\n", coefs[2]);
    printf("    CYtot = %f\n", coefs[3]);
    printf("    CZtot = %f\n", coefs[4]);
    printf("    Cltot = %f\n", coefs[5]);
    printf("    Cmtot = %f\n", coefs[6]);
    printf("    Cntot = %f\n", coefs[7]);
    printf("    e     = %f\n", coefs[8]);

cleanup:
    return status;
}
#endif
