/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Cart3D AIM
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <string.h>
#include <math.h>

#include "bodyTess.h"
#include "writeTrix.h"
#include "capsTypes.h"
#include "aimUtil.h"
#include "miscUtils.h"

#ifdef WIN32
#define getcwd     _getcwd
#define snprintf   _snprintf
#define strcasecmp stricmp
#define PATH_MAX   _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


#define DEBUG
#define NUMINPUT  26                 /* number of Cart3D inputs */
#define NUMOUT    12                 /* number of outputs */



/* currently setup for a single body */
typedef struct {
  int  nface;
  int  nvert;
  int  ntris;
  const char *apath;

  // Meshing parameters
  double tessParam[3];
  double outerBox;
  int nDiv, maxR, sharpFeatureDivisions, nMultiGridLevels;

} c3dAIM;

/*!\mainpage Introduction
 *
 * \section overviewCART3D CART3D AIM Overview
 * CART3D
 *
 * \section assumptionsCART3D Assumptions
 *
 * This documentation contains four sections to document the use of the CART3D AIM.
 * \ref examplesCART3D contains example *.csm input files and pyCAPS scripts designed to make use of the CART3D AIM.  These example
 * scripts make extensive use of the \ref attributeCART3D and CART3D \ref aimInputsCART3D and CART3D \ref aimOutputsCART3D.
 *
 * \section dependeciesCART3D Dependencies
 * ESP client of libxddm. For XDDM documentation, see $CART3D/doc/xddm/xddm.html. The library uses XML Path Language (XPath) to
 * navigate the elements of XDDM documents. For XPath tutorials, see the web,
 * e.g. www.developer.com/net/net/article.php/3383961/NET-and-XML-XPath-Queries.htm
 *
 * Dependency: libxml2, www.xmlsoft.org. This library is usually present on
 * most systems, check existence of 'xml2-config' script
 */

/*! \page examplesCART3D CART3D Examples
 *
 * This example contains a set of *.csm and pyCAPS (*.py) inputs that uses the CART3D AIM.  A user should have knowledge
 * on the generation of parametric geometry in Engineering Sketch Pad (ESP) before attempting to integrate with any AIM.
 * Specifically, this example makes use of Design Parameters, Set Parameters, User Defined Primitive (UDP) and attributes
 * in ESP.
 *
 */

/*! \page attributeCART3D CART3D attributes
 * The following list of attributes drives the CART3D geometric definition.
 *
 * - <b> capsAIM</b> This attribute is a CAPS requirement to indicate the analysis the geometry
 * representation supports.
 *
 *  - <b> capsReferenceArea</b>  [Optional: Default 1.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used as the Reference_Area entry in the CART3D input.
 *
 *  - <b> capsReferenceChord</b>  [Optional: Default 1.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used as the Reference_Length entry in the CART3D input.
 *
 *  - <b> capsReferenceX</b>  [Optional: Default 0.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used in the Moment_Point entry in the CART3D input.
 *
 *  - <b> capsReferenceY</b>  [Optional: Default 0.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used in the Moment_Point entry in the CART3D input.
 *
 *  - <b> capsReferenceZ</b>  [Optional: Default 0.0] This attribute may exist on any <em> Body</em>.  Its
 *  value will be used in the Moment_Point entry in the CART3D input.
 *
 */


static c3dAIM *cartInstance = NULL;
static int      numInstance = 0;



/* ********************** Exposed AIM Functions ***************************** */

int
aimInitialize(int ngIn, /*@unused@*/ /*@null@*/ capsValue *gIn, int *qeFlag,
              /*@unused@*/ const char *unitSys, int *nIn, int *nOut,
              int *nFields, char ***fnames, int **ranks)
{
  int    *ints, flag;
  char   **strs;
  c3dAIM *tmp;

#ifdef DEBUG
  printf("\n Cart3DAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif
  flag    = *qeFlag;
  *qeFlag = 0;

  /* specify the number of analysis input and out "parameters" */
  *nIn     = NUMINPUT;
  *nOut    = NUMOUT;
  if (flag == 1) return CAPS_SUCCESS;

  /* get the storage */
  if (cartInstance == NULL) {
    cartInstance = (c3dAIM *) EG_alloc(sizeof(c3dAIM));
    if (cartInstance == NULL) return EGADS_MALLOC;
  } else {
    tmp = (c3dAIM *) EG_reall(cartInstance, (numInstance+1)*sizeof(c3dAIM));
    if (tmp == NULL) return EGADS_MALLOC;
    cartInstance = tmp;
  }
  cartInstance[numInstance].nface = 0;
  cartInstance[numInstance].nvert = 0;
  cartInstance[numInstance].ntris = 0;
  cartInstance[numInstance].apath = NULL;


  /* Set meshing parameters  */
  cartInstance[numInstance].tessParam[0] = 0.025;
  cartInstance[numInstance].tessParam[1] = 0.001;
  cartInstance[numInstance].tessParam[2] = 15.00;
  cartInstance[numInstance].outerBox = 30;
  cartInstance[numInstance].nDiv = 5;
  cartInstance[numInstance].maxR = 11;
  cartInstance[numInstance].sharpFeatureDivisions = 2;
  cartInstance[numInstance].nMultiGridLevels = 1;

  /* specify the field variables this analysis can generate */
  *nFields = 4;
  ints     = (int *)   EG_alloc(*nFields*sizeof(int));
  strs     = (char **) EG_alloc(*nFields*sizeof(char *));
  if ((ints == NULL) || (strs == NULL)) {
    if (ints != NULL) EG_free(ints);
    if (strs != NULL) EG_free(strs);
    EG_free(cartInstance);
    cartInstance = NULL;
    return EGADS_MALLOC;
  }
  strs[0]  = EG_strdup("Cp");
  ints[0]  = 1;
  strs[1]  = EG_strdup("Density");
  ints[1]  = 1;
  strs[2]  = EG_strdup("Velocity");
  ints[2]  = 3;
  strs[3]  = EG_strdup("Pressure");
  ints[3]  = 1;
  *ranks   = ints;
  *fnames  = strs;

  /* Increment number of instances */
  numInstance++;

  return numInstance-1;
}

/* ********************** Exposed AIM Functions ***************************** */
int
aimInputs(int iIndex, /*@unused@*/ void *aimInfo, int index, char **name,
          capsValue *defval)
{
  int status = CAPS_SUCCESS;

    /*! \page aimInputsCART3D AIM Inputs
     * The following list outlines the CART3D inputs along with their default value available
     * through the AIM interface.
     */

#ifdef DEBUG
  printf(" Cart3DAIM/aimInputs  instance = %d  index = %d!\n", iIndex, index);
#endif

  if (index == 1) {
    *name              = EG_strdup("Tess_Params");
    defval->type       = Double;
    defval->dim        = Vector;
    defval->length     = 3;
    defval->nrow       = 1;
    defval->ncol       = 3;
    defval->units      = NULL;
    defval->vals.reals = (double *) EG_alloc(defval->length*sizeof(double));
    if (defval->vals.reals == NULL) {
      status = EGADS_MALLOC;
    } else {
      defval->vals.reals[0] = 0.025;
      defval->vals.reals[1] = 0.001;
      defval->vals.reals[2] = 15.00;
    }

      /*! \page aimInputsCART3D
     * - <B> Tess_Params = [double, double, double]</B>. <Default [0.025, 0.001, 15.00]> <br>
     * These parameters are used to create the surface mesh for CART3D. Their order definition is as follows.
     *  1. Max Edge Length (0 is any length)
     *  2. Max Sag or distance from mesh segment and actual curved geometry
     *  3. Max angle in degrees between triangle facets
     */

  } else if (index == 2) {
    *name             = EG_strdup("outer_box");
    defval->type      = Double;
    defval->vals.real = 30.;
      /*! \page aimInputsCART3D
     * - <B> outer_box = double</B>. <Default 30> <br>
     * Factor of outer boundary box based on geometry length scale defined by the  diagonal of the
     * 3D tightly fitting bounding box around body being modeled.
     */
  } else if (index == 3) {
    *name                = EG_strdup("nDiv");
    defval->type         = Integer;
    defval->vals.integer = 5;
      /*! \page aimInputsCART3D
     * - <B> nDiv = int</B>. <Default 5> <br>
     * nominal # of divisions in backgrnd mesh
     */

  } else if (index == 4) {
    *name                = EG_strdup("maxR");
    defval->type         = Integer;
    defval->vals.integer = 11;
      /*! \page aimInputsCART3D
     * - <B> maxR = int</B>. <Default 11> <br>
     * Max Num of cell refinements to perform
     */
  } else if (index == 5) {
    *name             = EG_strdup("Mach");
    defval->type      = Double;
    defval->vals.real = 0.76;
      /*! \page aimInputsCART3D
     * - <B> Mach = double</B>. <Default 0.76> <br>
     */
  } else if (index == 6) {
    *name             = EG_strdup("alpha");
    defval->type      = Double;
    defval->vals.real = 0.0;
      /*! \page aimInputsCART3D
     * - <B> alpha = double</B>. <Default 0.0> <br>
     * Angle of Attach in Degrees
     */
  } else if (index == 7) {
    *name             = EG_strdup("beta");
    defval->type      = Double;
    defval->vals.real = 0.0;
      /*! \page aimInputsCART3D
     * - <B> beta = double</B>. <Default 0.0> <br>
     * Side Slip Angle in Degrees
     */
  } else if (index == 8) {
    *name             = EG_strdup("gamma");
    defval->type      = Double;
    defval->vals.real = 1.4;
      /*! \page aimInputsCART3D
     * - <B> gamma = double</B>. <Default 1.4> <br>
     * Ratio of specific heats (default is air)
     */
  } else if (index == 9) {
    *name                = EG_strdup("maxCycles");
    defval->type         = Integer;
    defval->vals.integer = 1000;
      /*! \page aimInputsCART3D
     * - <B> maxCycles = int</B>. <Default 1000> <br>
     * Number of iterations
     */
  } else if (index == 10) {
    *name                = EG_strdup("SharpFeatureDivisions");
    defval->type         = Integer;
    defval->vals.integer = 2;
      /*! \page aimInputsCART3D
     * - <B> SharpFeatureDivisions = int</B>. <Default 2> <br>
     * nominal # of ADDITIONAL divisions in backgrnd mesh around sharp features
     */
  } else if (index == 11) {
    *name                = EG_strdup("nMultiGridLevels");
    defval->type         = Integer;
    defval->vals.integer = 1;
      /*! \page aimInputsCART3D
     * - <B> nMultiGridLevels = int</B>. <Default 1> <br>
     * number of multigrid levels in the mesh (1 is a single mesh)
     */
  } else if (index == 12) {
  *name                = EG_strdup("MultiGridCycleType");
  defval->type         = Integer;
  defval->vals.integer = 2;
    /*! \page aimInputsCART3D
     * - <B> MultiGridCycleType = int</B>. <Default 2> <br>
     * MultiGrid cycletype: 1 = "V-cycle", 2 = "W-cycle"
     * 'sawtooth' cycle is: V-cycle with MultiGridPreSmoothing = 1, MultiGridPostSmoothing = 0
     */
  }  else if (index == 13) {
  *name                = EG_strdup("MultiGridPreSmoothing");
  defval->type         = Integer;
  defval->vals.integer = 1;
    /*! \page aimInputsCART3D
     * - <B> MultiGridPreSmoothing = int</B>. <Default 1> <br>
     * number of pre-smoothing  passes in multigrid
     */
  } else if (index == 14) {
  *name                = EG_strdup("MultiGridPostSmoothing");
  defval->type         = Integer;
  defval->vals.integer = 1;
    /*! \page aimInputsCART3D
     * - <B> MultiGridPostSmoothing = int</B>. <Default 1> <br>
     * number of post-smoothing  passes in multigrid
     */
  } else if (index == 15) {
  *name             = EG_strdup("CFL");
  defval->type      = Double;
  defval->vals.real = 1.2;
    /*! \page aimInputsCART3D
     * - <B> CFL = double</B>. <Default 1.2> <br>
     * CFL number typically between 0.9 and 1.4
     */
  } else if (index == 16) {
  *name                = EG_strdup("Limiter");
  defval->type         = Integer;
  defval->vals.integer = 2;
    /*! \page aimInputsCART3D
     * - <B> Limiter = int</B>. <Default 2> <br>
     * organized in order of increasing dissipation. <br>
     * 0 = no Limiter, 1 = Barth-Jespersen, 2 = van Leer, 3 = sin limiter, 4 = van Albada, 5 = MinMod
     */
  } else if (index == 17) {
  *name                = EG_strdup("FluxFun");
  defval->type         = Integer;
  defval->vals.integer = 0;
    /*! \page aimInputsCART3D
     * - <B> FluxFun = int</B>. <Default 0> <br>
     * 0 = van Leer, 1 = van Leer-Hanel, 2 = Colella 1998, 3 = HLLC (alpha test)
     */
  } else if (index == 18) {
  *name                = EG_strdup("iForce");
  defval->type         = Integer;
  defval->vals.integer = 10;
    /*! \page aimInputsCART3D
     * - <B> iForce = int</B>. <Default 10> <br>
     * Report force & mom. information every iForce cycles
     */
  } else if (index == 19) {
  *name                = EG_strdup("iHist");
  defval->type         = Integer;
  defval->vals.integer = 1;
    /*! \page aimInputsCART3D
     * - <B> iHist = int</B>. <Default 1> <br>
     * Update 'history.dat' every iHist cycles
     */
  } else if (index == 20) {
  *name                = EG_strdup("nOrders");
  defval->type         = Integer;
  defval->vals.integer = 8;
    /*! \page aimInputsCART3D
     * - <B> nOrders = int</B>. <Default 8> <br>
     * Num of orders of Magnitude reduction in residual
     */

  } else if (index == 21) {
    *name               = EG_strdup("Xslices");
    defval->type          = Double;
    defval->lfixed        = Change;
    defval->sfixed        = Change;
    defval->nullVal       = IsNull;
    defval->dim           = Vector;
      /*! \page aimInputsCART3D
     * - <B> Xslices = double </B> <br> OR
     * - <B> Xslices = [double, ... , double] </B> <br>
     * X slice locations created in output.
     */
  } else if (index == 22) {
    *name               = EG_strdup("Yslices");
    defval->type          = Double;
    defval->lfixed        = Change;
    defval->sfixed        = Change;
    defval->nullVal       = IsNull;
    defval->dim           = Vector;

     /*! \page aimInputsCART3D
    * - <B> Yslices = double </B> <br> OR
    * - <B> Yslices = [double, ... , double] </B> <br>
    * Y slice locations created in output.
      */
  } else if (index == 23) {
    *name               = EG_strdup("Zslices");
    defval->type          = Double;
    defval->lfixed        = Change;
    defval->sfixed        = Change;
    defval->nullVal       = IsNull;
    defval->dim           = Vector;

     /*! \page aimInputsCART3D
    * - <B> Zslices = double </B> <br> OR
    * - <B> Zslices = [double, ... , double] </B> <br>
    * Z slice locations created in output.
    */
  }
  else if (index == 24) {
    *name = EG_strdup("Model_X_axis");
    defval->type = String;
    defval->units = NULL;
    defval->vals.string = EG_strdup("-Xb");
    
    /*! \page aimInputsCART3D
     * - <B> Model_X_axis = string </B> <br>
     * Model_X_axis defines x-axis orientation.
     */
  }
  else if (index == 25) {
    *name = EG_strdup("Model_Y_axis");
    defval->type = String;
    defval->units = NULL;
    defval->vals.string = EG_strdup("Yb");
    
    /*! \page aimInputsCART3D
     * - <B> Model_Y_axis = string </B> <br>
     * Model_Y_axis defines y-axis orientation.
     */
  }
  else if (index == 26) {
    *name = EG_strdup("Model_Z_axis");
    defval->type = String;
    defval->units = NULL;
    defval->vals.string = EG_strdup("-Zb");
    
    /*! \page aimInputsCART3D
     * - <B> Model_Z_axis = string </B> <br>
     * Model_Z_axis defines z-axis orientation.
     */
  }
  return status;
}

/* ********************** Exposed AIM Functions ***************************** */
int
aimPreAnalysis(int iIndex, void *aimInfo, const char *apath, capsValue *inputs,
               capsErrs **errs)
{
  int          i, j, varid, status, nBody, nface, nedge, nvert, ntri, *tris;
  int          atype, alen;
  double       params[3], box[6], size, *xyzs, Sref, Cref, Xref, Yref, Zref;
  char         line[128], cpath[PATH_MAX];
  const int    *ints;
  const char   *string, *intents;
  const double *reals;
  ego          *bodies, tess;
  verTags      *vtags;
  FILE         *fp;

  *errs = NULL;
#ifdef DEBUG
  printf(" Cart3DAIM/aimPreAnalysis instance = %d!\n", iIndex);
#endif
  cartInstance[iIndex].apath = apath;

  /* get where we are and set the path to our input */
  (void) getcwd(cpath, PATH_MAX);
  if (chdir(apath) != 0) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimPreAnalysis Cannot chdir to %s!\n", apath);
#endif
    return CAPS_DIRERR;
  }
  
  if (inputs == NULL) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimPreAnalysis -- NULL inputs!\n");
#endif
    return CAPS_NULLOBJ;
  }

  // Get AIM bodies
  status = aim_getBodies(aimInfo, &intents, &nBody, &bodies);
  if (status != CAPS_SUCCESS) {
    chdir(cpath);
    return status;
  }

  if (nBody != 1) {
    printf(" Cart3DAIM/aimPreAnalysis instance = %d, nBody = %d!\n",
           iIndex, nBody);
    chdir(cpath);
    return CAPS_SOURCEERR;
  }

  // Initialize reference values
  Sref = Cref = 1.0;
  Xref = Yref = Zref = 0.0;

  // Loop over bodies and look for reference quantity attributes
  for (i = 0; i < nBody; i++) {
    status = EG_attributeRet(bodies[i], "capsReferenceArea", &atype, &alen,
                             &ints, &reals, &string);
    if ((status == EGADS_SUCCESS) && (atype == ATTRREAL)) Sref = (double) reals[0];
    
    status = EG_attributeRet(bodies[i], "capsReferenceChord", &atype, &alen,
                             &ints, &reals, &string);
    if ((status == EGADS_SUCCESS) && (atype == ATTRREAL)) Cref = (double) reals[0];
    
    status = EG_attributeRet(bodies[i], "capsReferenceX", &atype, &alen,
                             &ints, &reals, &string);
    if ((status == EGADS_SUCCESS) && (atype == ATTRREAL)) Xref = (double) reals[0];
    
    status = EG_attributeRet(bodies[i], "capsReferenceY", &atype, &alen,
                             &ints, &reals, &string);
    if ((status == EGADS_SUCCESS) && (atype == ATTRREAL)) Yref = (double) reals[0];
    
    status = EG_attributeRet(bodies[i], "capsReferenceZ", &atype, &alen,
                             &ints, &reals, &string);
    if ((status == EGADS_SUCCESS) && (atype == ATTRREAL)) Zref = (double) reals[0];
  }

  // Only do the meshing if the geometry has been changed or a meshing input variable has been changed
  if (aim_newGeometry(aimInfo) == CAPS_SUCCESS ||
      cartInstance[iIndex].tessParam[0]          != inputs[aim_getIndex(aimInfo, "Tess_Params",  ANALYSISIN)-1].vals.reals[0] ||
      cartInstance[iIndex].tessParam[1]          != inputs[aim_getIndex(aimInfo, "Tess_Params",  ANALYSISIN)-1].vals.reals[1] ||
      cartInstance[iIndex].tessParam[2]          != inputs[aim_getIndex(aimInfo, "Tess_Params",  ANALYSISIN)-1].vals.reals[2] ||
      cartInstance[iIndex].outerBox              != inputs[aim_getIndex(aimInfo, "outer_box",  ANALYSISIN)-1].vals.real ||
      cartInstance[iIndex].nDiv                  != inputs[aim_getIndex(aimInfo, "nDiv",  ANALYSISIN)-1].vals.integer ||
      cartInstance[iIndex].maxR                  != inputs[aim_getIndex(aimInfo, "maxR",  ANALYSISIN)-1].vals.integer ||
      cartInstance[iIndex].sharpFeatureDivisions != inputs[aim_getIndex(aimInfo, "SharpFeatureDivisions",  ANALYSISIN)-1].vals.integer ||
      cartInstance[iIndex].nMultiGridLevels      != inputs[aim_getIndex(aimInfo, "nMultiGridLevels",  ANALYSISIN)-1].vals.integer) {

    // Set new meshing parameters
    cartInstance[iIndex].tessParam[0] = inputs[aim_getIndex(aimInfo, "Tess_Params",  ANALYSISIN)-1].vals.reals[0];
    cartInstance[iIndex].tessParam[1] = inputs[aim_getIndex(aimInfo, "Tess_Params",  ANALYSISIN)-1].vals.reals[1];
    cartInstance[iIndex].tessParam[2] = inputs[aim_getIndex(aimInfo, "Tess_Params",  ANALYSISIN)-1].vals.reals[2];

    cartInstance[iIndex].outerBox = inputs[aim_getIndex(aimInfo, "outer_box",  ANALYSISIN)-1].vals.real;
    cartInstance[iIndex].nDiv     = inputs[aim_getIndex(aimInfo, "nDiv",  ANALYSISIN)-1].vals.integer;
    cartInstance[iIndex].maxR     = inputs[aim_getIndex(aimInfo, "maxR",  ANALYSISIN)-1].vals.integer;
    cartInstance[iIndex].sharpFeatureDivisions = inputs[aim_getIndex(aimInfo, "SharpFeatureDivisions",  ANALYSISIN)-1].vals.integer;
    cartInstance[iIndex].nMultiGridLevels      = inputs[aim_getIndex(aimInfo, "nMultiGridLevels",  ANALYSISIN)-1].vals.integer;

    status = EG_getBoundingBox(bodies[0], box);
    if (status != EGADS_SUCCESS) return status;

    size = sqrt((box[3]-box[0])*(box[3]-box[0]) +
          (box[4]-box[1])*(box[4]-box[1]) +
          (box[5]-box[2])*(box[5]-box[2]));
    printf(" Body size = %lf\n", size);

    params[0] = inputs[0].vals.reals[0]*size;
    params[1] = inputs[0].vals.reals[1]*size;
    params[2] = inputs[0].vals.reals[2];
    printf(" Tessellating Inst=%d with  MaxEdge = %lf  Sag = %lf  Angle = %lf\n",
        iIndex, params[0], params[1], params[2]);

    status = EG_makeTessBody(bodies[0], params, &tess);
    if (status != EGADS_SUCCESS) {
      chdir(cpath);
      return status;
    }

    /* cleanup old data and get new tessellation */
    status = bodyTess(tess, &nface, &nedge, &nvert, &xyzs, &vtags, &ntri, &tris);
    if (status != EGADS_SUCCESS) {
      EG_deleteObject(tess);
      chdir(cpath);
      return status;
    }
    cartInstance[iIndex].nface = nface;
    cartInstance[iIndex].nvert = nvert;
    cartInstance[iIndex].ntris = ntri;

    /* write the tri file */
    status = writeTrix("Components.i.tri", NULL, 1, nvert, xyzs, 0, NULL,
             ntri, tris);
    EG_free(tris);
    EG_free(vtags);
    EG_free(xyzs);
    if (status != 0) {
      printf(" writeTrix return = %d\n", status);
      EG_deleteObject(tess);
      chdir(cpath);
      return CAPS_IOERR;
    }

    /* store away the tessellation */
    status = aim_setTess(aimInfo, tess);
    if (status != 0) {
      printf(" aim_setTess return = %d\n", status);
      EG_deleteObject(tess);
      chdir(cpath);
      return status;
    }

    snprintf(line, 128, "autoInputs -r %lf -nDiv %d -maxR %d",
         inputs[1].vals.real, inputs[2].vals.integer,
         inputs[3].vals.integer);
    printf(" Executing: %s\n", line);
    status = system(line);
    if (status != 0) {
      printf(" ERROR: autoInputs return = %d\n", status);
      chdir(cpath);
      return CAPS_EXECERR;
    }

    snprintf(line, 128, "cubes -reorder -sf %d",
        inputs[9].vals.integer);
    printf(" Executing: %s\n", line);
    status = system(line);
    if (status != 0) {
      printf(" ERROR: cubes return = %d\n", status);
      chdir(cpath);
      return CAPS_EXECERR;
    }

    snprintf(line, 128, "mgPrep -n %d",
        inputs[10].vals.integer);
    printf(" Executing: %s\n", line);
    status = system(line);
    if (status != 0) {
      printf(" ERROR: mgPrep return = %d\n", status);
      chdir(cpath);
      return CAPS_EXECERR;
    }
  }

  /* create and output input.cntl */
  fp = fopen("input.cntl", "w");
  if (fp == NULL) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimCalcOutput Cannot open input.cntl!\n");
#endif
    chdir(cpath);
    return CAPS_DIRERR;
  }

  fprintf(fp, "$__Case_Information:\n\n");
  fprintf(fp, "Mach     %lf\n", inputs[4].vals.real);
  fprintf(fp, "alpha    %lf\n", inputs[5].vals.real);
  fprintf(fp, "beta     %lf\n", inputs[6].vals.real);
  fprintf(fp, "gamma    %lf\n", inputs[7].vals.real);

  fprintf(fp, "\n$__File_Name_Information:\n\n");
  fprintf(fp, "MeshInfo           Mesh.c3d.Info\n");
  fprintf(fp, "MeshFile           Mesh.mg.c3d\n\n");

  fprintf(fp, "$__Solver_Control_Information:\n\n");
  fprintf(fp, "RK        0.0695     1\n");
  fprintf(fp, "RK        0.1602     0\n");
  fprintf(fp, "RK        0.2898     0\n");
  fprintf(fp, "RK        0.5060     0\n");
  fprintf(fp, "RK        1.0        0\n\n");
  fprintf(fp, "CFL           %lf\n", inputs[14].vals.real);
  fprintf(fp, "Limiter       %d\n",  inputs[15].vals.integer);
  fprintf(fp, "FluxFun       %d\n",  inputs[16].vals.integer);
  fprintf(fp, "maxCycles     %d\n",  inputs[8].vals.integer);
  fprintf(fp, "Precon        0\n");
  fprintf(fp, "wallBCtype    0\n");
  fprintf(fp, "nMGlev        %d\n",   inputs[10].vals.integer);
  fprintf(fp, "MG_cycleType  %d\n",   inputs[11].vals.integer);
  fprintf(fp, "MG_nPre       %d\n",   inputs[12].vals.integer);
  fprintf(fp, "MG_nPost      %d\n\n", inputs[13].vals.integer);

  fprintf(fp, "$__Boundary_Conditions:\n\n");
/*                   # BC types: 0 = FAR FIELD
                                 1 = SYMMETRY
                                 2 = INFLOW  (specify all)
                                 3 = OUTFLOW (simple extrap)  */
  fprintf(fp, "Dir_Lo_Hi     0   0 0\n"); /* (0/1/2) direction - Low BC - Hi BC */
  fprintf(fp, "Dir_Lo_Hi     1   0 0\n");
  fprintf(fp, "Dir_Lo_Hi     2   0 0\n\n");

  fprintf(fp, "$__Convergence_History_reporting:\n\n");
  fprintf(fp, "iForce     %d\n", inputs[17].vals.integer);
  fprintf(fp, "iHist      %d\n", inputs[18].vals.integer);
  fprintf(fp, "nOrders    %d\n", inputs[19].vals.integer);
  fprintf(fp, "refArea    %lf\n", Sref);
  fprintf(fp, "refLength  %lf\n", Cref);

  fprintf(fp, "\n$__Partition_Information:\n\n");
  fprintf(fp, "nPart      1\n");
  fprintf(fp, "type       1\n");

  fprintf(fp, "\n$__Post_Processing:\n\n");
  varid = 20;
  if (inputs[varid].nullVal != IsNull) {
    if (inputs[varid].length == 1) {
      fprintf(fp,"Xslices %lf\n",inputs[varid].vals.real);
    } else {
      fprintf(fp,"Xslices");
      for (j = 0; j < inputs[varid].length; j++) {
        fprintf(fp," %lf",inputs[varid].vals.reals[j]);
      }
      fprintf(fp,"\n");
    }
  }

  varid = 21;
  if (inputs[varid].nullVal != IsNull) {
    if (inputs[varid].length == 1) {
      fprintf(fp,"Yslices %lf\n",inputs[varid].vals.real);
    } else {
      fprintf(fp,"Yslices");
      for (j = 0; j < inputs[varid].length; j++) {
        fprintf(fp," %lf",inputs[varid].vals.reals[j]);
      }
      fprintf(fp,"\n");
    }
  }

  varid = 22;
    if (inputs[varid].nullVal != IsNull) {
      if (inputs[varid].length == 1) {
        fprintf(fp,"Zslices %lf\n",inputs[varid].vals.real);
      } else {
        fprintf(fp,"Zslices");
        for (j = 0; j < inputs[varid].length; j++) {
          fprintf(fp," %lf",inputs[varid].vals.reals[j]);
        }
        fprintf(fp,"\n");
      }
    }

  fprintf(fp, "\n$__Force_Moment_Processing:\n\n");
/* ... Axis definitions (with respect to body axis directions (Xb,Yb,Zb)
                         w/ usual stability and control orientation)  */
  fprintf(fp, "Model_X_axis  %s\n", inputs[23].vals.string);
  fprintf(fp, "Model_Y_axis  %s\n", inputs[24].vals.string);
  fprintf(fp, "Model_Z_axis  %s\n", inputs[25].vals.string);
  fprintf(fp, "Reference_Area   %lf all\n", Sref);
  fprintf(fp, "Reference_Length %lf all\n", Cref);
  fprintf(fp, "Force entire\n\n");
  fprintf(fp, "Moment_Point %lf %lf %lf entire\n",Xref ,Yref ,Zref );

  chdir(cpath);
  fclose(fp);

  chdir(cpath);
  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************
int
aimOutputs(/*@unused@*/ int iIndex, /*@unused@*/ void *aimStruc, int index,
           char **aoname, capsValue *form)
{
  char *names[12] = {"C_A", "C_Y", "C_N", "C_D", "C_S", "C_L", "C_l", "C_m", "C_n", "C_M_x", "C_M_y", "C_M_z"};
#ifdef DEBUG
  printf(" Cart3DAIM/aimOutputs instance = %d  index = %d!\n", iIndex, index);
#endif

  /*! \page aimOutputsCART3D AIM Outputs
  * Integrated force outputs on the entire body are available as outputs from the loadsCC.dat output file.
  * - <B> C_A</B>. entire Axial Force
  * - <B> C_Y</B>. entire Lateral Force
  * - <B> C_N</B>. entire Normal Force
  * - <B> C_D</B>. entire Drag Force
  * - <B> C_S</B>. entire Side Force
  * - <B> C_L</B>. entire Lift Force
  * - <B> C_l</B>. entire  Rolling Moment
  * - <B> C_m</B>. Pitching Moment
  * - <B> C_n</B>. Yawing Moment
  * - <B> C_M_x</B>. X Aero Moment
  * - <B> C_M_y</B>. Y Aero Moment
  * - <B> C_M_z</B>. Z Aero Moment
  */

  *aoname    = EG_strdup(names[index-1]);
  form->type = Double;

  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************
int
aimCalcOutput(/*@unused@*/ int iIndex, /*@unused@*/ void *aimInfo, const char *apath,
              int index, capsValue *val, capsErrs **errors)
{
  int        offset;
  size_t     linecap = 0;
  const char comp = ':';
  char       cpath[PATH_MAX], *valstr, *line = NULL;
  FILE       *fp;
  char       *start[12]= { "entire   Axial Force (C_A):",
                           "entire Lateral Force (C_Y):",
                           "entire  Normal Force (C_N):",
                           "entire    Drag Force (C_D):",
                           "entire    Side Force (C_S):",
                           "entire    Lift Force (C_L):",
                           "entire  Rolling Moment",
                           "entire Pitching Moment",
                           "entire   Yawing Moment",
                           "entire   X Aero Moment",
                           "entire   Y Aero Moment",
                           "entire   Z Aero Moment" };

  if (index < 7) {
    offset = 27;
  } else {
    offset = 1;
  }

/*
#ifdef DEBUG
  printf(" Cart3DAIM/aimCalcOutput instance = %d  index = %d!\n", iIndex, index);
#endif
*/
  *errors = NULL;

  /* get where we are and set the path to our output */
  (void) getcwd(cpath, PATH_MAX);
  if (chdir(apath) != 0) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimCalcOutput Cannot chdir to %s!\n", apath);
#endif
    return CAPS_DIRERR;
  }

  /* open the Cart3D loads file */
  fp = fopen("loadsCC.dat", "r");
  if (fp == NULL) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimCalcOutput Cannot open Output file!\n");
#endif
    chdir(cpath);
    return CAPS_DIRERR;
  }

  /* scan the file for the string */
  valstr = NULL;
  while (getline(&line, &linecap, fp) >= 0) {
    if (line == NULL) continue;
    valstr = strstr(line, start[index-1]);
    if (valstr != NULL) {
      if (index > 6) {
        valstr = strchr(line,comp);
      }
      
#ifdef DEBUG
      printf("valstr > |%s|\n",valstr);
#endif
      /* found it -- get the value */
      sscanf(&valstr[offset], "%lf", &val->vals.real);
      break;
    }
  }

  chdir(cpath);
  fclose(fp);
  if (line != NULL) EG_free(line);

  if (valstr == NULL) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimCalcOutput Cannot find %s in Output file!\n",
           start[index-1]);
#endif
    return CAPS_NOTFOUND;
  }

  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************

void
aimCleanup()
{
#ifdef DEBUG
  printf(" Cart3DAIM/aimCleanup!\n");
#endif

  if (cartInstance != NULL) EG_free(cartInstance);
  cartInstance = NULL;
  numInstance  = 0;
}

int
aimFreeDiscr(capsDiscr *discr)
{
  int i;

#ifdef DEBUG
  printf(" Cart3DAIM/aimFreeDiscr instance = %d!\n", discr->instance);
#endif

  /* free up this capsDiscr */
  if (discr->mapping != NULL) EG_free(discr->mapping);
  if (discr->verts   != NULL) EG_free(discr->verts);
  if (discr->celem   != NULL) EG_free(discr->celem);
  if (discr->types   != NULL) {
    for (i = 0; i < discr->nTypes; i++) {
      if (discr->types[i].gst   != NULL) EG_free(discr->types[i].gst);
      if (discr->types[i].dst   != NULL) EG_free(discr->types[i].dst);
      if (discr->types[i].matst != NULL) EG_free(discr->types[i].matst);
      if (discr->types[i].tris  != NULL) EG_free(discr->types[i].tris);
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
  discr->ptrm     = NULL;

  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************
int
aimDiscr(char *tname, capsDiscr *discr)
{
  int          i, j, k, kk, n, ibod, stat, inst, nFace, atype, alen, tlen, nGlobal;
  int          npts, ntris, nBody, global, *storage, *pairs = NULL, *vid = NULL;
  const int    *ints, *ptype, *pindex, *tris, *nei;
  const double *reals, *xyz, *uv;
  const char   *string, *intents;
  ego          tess, body, *bodies, *faces;

  inst = discr->instance;
#ifdef DEBUG
  printf(" Cart3DAIM/aimDiscr: tname = %s, instance = %d!\n", tname, inst);
#endif
  if ((inst < 0) || (inst >= numInstance)) return CAPS_BADINDEX;

  /* currently this works with 1 body! */
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" Cart3DAIM/aimDiscr: %d aim_getBodies = %d!\n", inst, stat);
    return stat;
  }
  stat = aimFreeDiscr(discr);
  if (stat != CAPS_SUCCESS) return stat;

  /* find any faces with our boundary marker */
  for (n = i = 0; i < nBody; i++) {
    stat = EG_getBodyTopos(bodies[i], NULL, FACE, &nFace, &faces);
    if (stat != EGADS_SUCCESS) {
      printf(" Cart3DAIM: getBodyTopos (Face) = %d for Body %d!\n", stat, i+1);
      return stat;
    }
    if (nFace != cartInstance[inst].nface) {
      printf(" Cart3DAIM: getBodyTopos nFace = %d, cached = %d!\n", nFace,
             cartInstance[inst].nface);
      return CAPS_MISMATCH;
    }
    for (j = 0; j < nFace; j++) {
      stat = EG_attributeRet(faces[j], "capsBound", &atype, &alen,
                             &ints, &reals, &string);
      if (stat  != EGADS_SUCCESS)     continue;
      if (atype != ATTRSTRING)        continue;
      if (strcmp(string, tname) != 0) continue;
#ifdef DEBUG
      printf(" Cart3DAIM/aimDiscr: Body %d/Face %d matches %s!\n",
             i+1, j+1, tname);
#endif
      n++;
    }
    EG_free(faces);
  }
  if (n == 0) {
    printf(" Cart3DAIM/aimDiscr: No Faces match %s!\n", tname);
    return CAPS_SUCCESS;
  }

  /* store away the faces */
  stat  = EGADS_MALLOC;
  pairs = (int *) EG_alloc(2*n*sizeof(int));
  if (pairs == NULL) {
    printf(" Cart3DAIM: alloc on pairs = %d!\n", n);
    goto bail;
  }
  npts = ntris = 0;
  for (n = i = 0; i < nBody; i++) {
    stat = EG_getBodyTopos(bodies[i], NULL, FACE, &nFace, &faces);
    if (stat != EGADS_SUCCESS) {
      printf(" Cart3DAIM: getBodyTopos (Face) = %d for Body %d!\n", stat, i+1);
      goto bail;
    }
    for (j = 0; j < nFace; j++) {
      stat = EG_attributeRet(faces[j], "capsBound", &atype, &alen,
                             &ints, &reals, &string);
      if (stat  != EGADS_SUCCESS)     continue;
      if (atype != ATTRSTRING)        continue;
      if (strcmp(string, tname) != 0) continue;
      pairs[2*n  ] = i+1;
      pairs[2*n+1] = j+1;
      n++;
      stat = EG_getTessFace(bodies[i+nBody], j+1, &alen, &xyz, &uv, &ptype,
                            &pindex, &tlen, &tris, &nei);
      if (stat != EGADS_SUCCESS) {
        printf(" Cart3DAIM: EG_getTessFace %d = %d for Body %d!\n",
               j+1, stat, i+1);
        continue;
      }
      npts  += alen;
      ntris += tlen;
    }
    EG_free(faces);
  }
  if ((npts == 0) || (ntris == 0)) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimDiscr: ntris = %d, npts = %d!\n", ntris, npts);
#endif
    stat = CAPS_SOURCEERR;
    goto bail;
  }
  discr->nElems = ntris;

  /* specify our single element type */
  stat = EGADS_MALLOC;
  discr->nTypes = 1;
  discr->types  = (capsEleType *) EG_alloc(sizeof(capsEleType));
  if (discr->types == NULL) goto bail;
  discr->types[0].nref  = 3;
  discr->types[0].ndata = 0;            /* data at geom reference positions */
  discr->types[0].ntri  = 1;
  discr->types[0].nmat  = 0;            /* match points at geom ref positions */
  discr->types[0].tris  = NULL;
  discr->types[0].gst   = NULL;
  discr->types[0].dst   = NULL;
  discr->types[0].matst = NULL;

  discr->types[0].tris   = (int *) EG_alloc(3*sizeof(int));
  if (discr->types[0].tris == NULL) goto bail;
  discr->types[0].tris[0] = 1;
  discr->types[0].tris[1] = 2;
  discr->types[0].tris[2] = 3;

  discr->types[0].gst   = (double *) EG_alloc(6*sizeof(double));
  if (discr->types[0].gst == NULL) goto bail;
  discr->types[0].gst[0] = 0.0;
  discr->types[0].gst[1] = 0.0;
  discr->types[0].gst[2] = 1.0;
  discr->types[0].gst[3] = 0.0;
  discr->types[0].gst[4] = 0.0;
  discr->types[0].gst[5] = 1.0;

  /* get the tessellation and
     make up a simple linear continuous triangle discretization */
#ifdef DEBUG
  printf(" Cart3DAIM/aimDiscr: ntris = %d, npts = %d!\n", ntris, npts);
#endif
  discr->mapping = (int *) EG_alloc(2*npts*sizeof(int));
  if (discr->mapping == NULL) goto bail;
  discr->elems = (capsElement *) EG_alloc(ntris*sizeof(capsElement));
  if (discr->elems == NULL) goto bail;
  storage = (int *) EG_alloc(6*ntris*sizeof(int));
  if (storage == NULL) goto bail;
  discr->ptrm = storage;

  for (ibod = kk = k = i = 0; i < n; i++) {
    while (pairs[2*i] != ibod) {
      stat = EG_statusTessBody(bodies[pairs[2*i]-1 + nBody], &body, &j,
                               &nGlobal);
      if ((stat < EGADS_SUCCESS) || (nGlobal == 0)) {
        printf(" Cart3DAIM/aimDiscr: EG_statusTessBody = %d, nGlobal = %d!\n",
               stat, nGlobal);
        goto bail;
      }
      if (vid != NULL) EG_free(vid);
      stat = EGADS_MALLOC;
      vid  = (int *) EG_alloc(nGlobal*sizeof(int));
      if (vid == NULL) goto bail;
      for (j = 0; j < nGlobal; j++) vid[j] = 0;
      ibod++;
    }
    if (vid == NULL) {
      printf(" Cart3DAIM/aimDiscr: vid == NULL!\n");
      goto bail;
    }
    tess = bodies[pairs[2*i]-1 + nBody];
    stat = EG_getTessFace(tess, pairs[2*i+1], &alen, &xyz, &uv, &ptype, &pindex,
                          &tlen, &tris, &nei);
    if (stat != EGADS_SUCCESS) continue;
    for (j = 0; j < alen; j++) {
      stat = EG_localToGlobal(tess, pairs[2*i+1], j+1, &global);
      if (stat != EGADS_SUCCESS) {
        printf(" Cart3DAIM/aimDiscr: %d %d - %d %d EG_localToGlobal = %d!\n",
               pairs[2*i], j+1, ptype[j], pindex[j], stat);
        goto bail;
      }
      if (vid[global-1] != 0) continue;
      discr->mapping[2*k  ] = ibod;
      discr->mapping[2*k+1] = global;
      vid[global-1]         = k+1;
      k++;
    }
    for (j = 0; j < tlen; j++, kk++) {
      discr->elems[kk].bIndex      = ibod;
      discr->elems[kk].tIndex      = 1;
      discr->elems[kk].eIndex      = pairs[2*i+1];
/*@-immediatetrans@*/
      discr->elems[kk].gIndices    = &storage[6*kk];
/*@+immediatetrans@*/
      discr->elems[kk].dIndices    = NULL;
      discr->elems[kk].eTris.tq[0] = j+1;
      stat = EG_localToGlobal(tess, pairs[2*i+1], tris[3*j  ], &global);
      if (stat != EGADS_SUCCESS)
        printf(" Cart3DAIM/aimDiscr: tri %d/0 EG_localToGlobal = %d\n",
               j+1, stat);
      storage[6*kk  ] = vid[global-1];
      storage[6*kk+1] = tris[3*j  ];
      stat = EG_localToGlobal(tess, pairs[2*i+1], tris[3*j+1], &global);
      if (stat != EGADS_SUCCESS)
        printf(" Cart3DAIM/aimDiscr: tri %d/1 EG_localToGlobal = %d\n",
               j+1, stat);
      storage[6*kk+2] = vid[global-1];
      storage[6*kk+3] = tris[3*j+1];
      stat = EG_localToGlobal(tess, pairs[2*i+1], tris[3*j+2], &global);
      if (stat != EGADS_SUCCESS)
        printf(" Cart3DAIM/aimDiscr: tri %d/2 EG_localToGlobal = %d\n",
               j+1, stat);
      storage[6*kk+4] = vid[global-1];
      storage[6*kk+5] = tris[3*j+2];
    }
  }
  discr->nPoints = k;

  /* free up our stuff */
  if (vid   != NULL) EG_free(vid);
  if (pairs != NULL) EG_free(pairs);

  return CAPS_SUCCESS;

bail:
  if (vid   != NULL) EG_free(vid);
  if (pairs != NULL) EG_free(pairs);
  aimFreeDiscr(discr);

  return stat;
}

// ********************** AIM Function Break *****************************
int
aimLocateElement(capsDiscr *discr, double *params, double *param, int *eIndex,
                 double *bary)
{
  int    i, in[3], stat, ismall;
  double we[3], w, smallw = -1.e300;

  if (discr == NULL) return CAPS_NULLOBJ;

  for (ismall = i = 0; i < discr->nElems; i++) {
    in[0] = discr->elems[i].gIndices[0] - 1;
    in[1] = discr->elems[i].gIndices[2] - 1;
    in[2] = discr->elems[i].gIndices[4] - 1;
    stat  = EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]],
                          param, we);
    if (stat == EGADS_SUCCESS) {
      *eIndex = i+1;
      bary[0] = we[1];
      bary[1] = we[2];
      return CAPS_SUCCESS;
    }
    w = we[0];
    if (we[1] < w) w = we[1];
    if (we[2] < w) w = we[2];
    if (w > smallw) {
      ismall = i+1;
      smallw = w;
    }
  }

  /* must extrapolate! */
  if (ismall == 0) return CAPS_NOTFOUND;
  in[0] = discr->elems[ismall-1].gIndices[0] - 1;
  in[1] = discr->elems[ismall-1].gIndices[2] - 1;
  in[2] = discr->elems[ismall-1].gIndices[4] - 1;
  EG_inTriExact(&params[2*in[0]], &params[2*in[1]], &params[2*in[2]],
                param, we);
  *eIndex = ismall;
  bary[0] = we[1];
  bary[1] = we[2];
/*
  printf(" aimLocateElement: extropolate to %d (%lf %lf %lf)  %lf\n",
         ismall, we[0], we[1], we[2], smallw);
 */
  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************
int
aimTransfer(capsDiscr *discr, const char *name, int npts, int rank, double *data,
            /*@unused@*/ char **units)
{
  int    i, j, k, stat;
  char   cpath[PATH_MAX];
  double *rvec;

#ifdef DEBUG
  printf(" Cart3DAIM/aimTransfer name = %s  instance = %d  npts = %d/%d!\n",
         name, discr->instance, npts, rank);
#endif

  (void) getcwd(cpath, PATH_MAX);
  if (chdir(cartInstance[discr->instance].apath) != 0) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimTransfer Cannot chdir to %s!\n",
           cartInstance[discr->instance].apath);
#endif
    return CAPS_DIRERR;
  }

  rvec = (double *) EG_alloc(rank*cartInstance[discr->instance].nvert*
                             sizeof(double));
  if (rvec == NULL) {
#ifdef DEBUG
    printf(" Cart3DAIM/aimTransfer Cannot allocate %dx%d!\n",
           rank, cartInstance[discr->instance].nvert);
#endif
    chdir(cpath);
    return EGADS_MALLOC;
  }

  /* try and read the trix file */
  stat = readTrix("Components.i.trix", name,
                  cartInstance[discr->instance].nvert, rank, rvec);
  chdir(cpath);
  if (stat != 0) {
    EG_free(rvec);
    return CAPS_IOERR;
  }

  /* move the appropriate parts of the tessellation to data */
  for (i = 0; i < npts; i++) {
    /* assumes only single body otherwise need to look at body! */
    k = discr->mapping[2*i+1] - 1;
    for (j = 0; j < rank; j++) data[rank*i+j] = rvec[rank*k+j];
  }

  EG_free(rvec);
  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************
int
aimInterpolation(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                 double *bary, int rank, double *data, double *result)
{
  int    in[3], i;
  double we[3];
/*
#ifdef DEBUG
  printf(" Cart3DAIM/aimInterpolation  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" Cart3DAIM/Interpolation: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);

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

// ********************** AIM Function Break *****************************
int
aimInterpolateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex,
                  double *bary, int rank, double *r_bar, double *d_bar)
{
  int    in[3], i;
  double we[3];
/*
#ifdef DEBUG
  printf(" Cart3DAIM/aimInterpolateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" Cart3DAIM/InterpolateBar: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);

  we[0] = 1.0 - bary[0] - bary[1];
  we[1] = bary[0];
  we[2] = bary[1];
  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  for (i = 0; i < rank; i++) {
/*  result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2];  */
    d_bar[rank*in[0]+i] += we[0]*r_bar[i];
    d_bar[rank*in[1]+i] += we[1]*r_bar[i];
    d_bar[rank*in[2]+i] += we[2]*r_bar[i];
  }

  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************
int
aimIntegration(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
               /*@null@*/ double *data, double *result)
{
  int        i, in[3], stat, ptype, pindex, nBody;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;
/*
#ifdef DEBUG
  printf(" Cart3DAIM/aimIntegration  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" Cart3DAIM/aimIntegration: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" Cart3DAIM/aimIntegration: %d aim_getBodies = %d!\n",
           discr->instance, stat);
    return stat;
  }

  /* element indices */

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;

  stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                      discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (stat != CAPS_SUCCESS)
    printf(" Cart3DAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[0], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                      discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (stat != CAPS_SUCCESS)
    printf(" Cart3DAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[1], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                      discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (stat != CAPS_SUCCESS)
    printf(" Cart3DAIM/aimIntegration: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[2], stat);

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

  for (i = 0; i < rank; i++)
    result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area;

  return CAPS_SUCCESS;
}

// ********************** AIM Function Break *****************************
int
aimIntegrateBar(capsDiscr *discr, /*@unused@*/ const char *name, int eIndex, int rank,
                double *r_bar, double *d_bar)
{
  int        i, in[3], stat, ptype, pindex, nBody;
  double     x1[3], x2[3], x3[3], xyz1[3], xyz2[3], xyz3[3], area;
  const char *intents;
  ego        *bodies;
/*
#ifdef DEBUG
  printf(" Cart3DAIM/aimIntegrateBar  %s  instance = %d!\n",
         name, discr->instance);
#endif
*/
  if ((eIndex <= 0) || (eIndex > discr->nElems))
    printf(" Cart3DAIM/aimIntegrateBar: eIndex = %d [1-%d]!\n",
           eIndex, discr->nElems);
  stat = aim_getBodies(discr->aInfo, &intents, &nBody, &bodies);
  if (stat != CAPS_SUCCESS) {
    printf(" Cart3DAIM/aimIntegrateBar: %d aim_getBodies = %d!\n",
           discr->instance, stat);
    return stat;
  }

  /* element indices */

  in[0] = discr->elems[eIndex-1].gIndices[0] - 1;
  in[1] = discr->elems[eIndex-1].gIndices[2] - 1;
  in[2] = discr->elems[eIndex-1].gIndices[4] - 1;
  stat = EG_getGlobal(bodies[discr->mapping[2*in[0]]+nBody-1],
                      discr->mapping[2*in[0]+1], &ptype, &pindex, xyz1);
  if (stat != CAPS_SUCCESS)
    printf(" CART3DAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[0], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[1]]+nBody-1],
                      discr->mapping[2*in[1]+1], &ptype, &pindex, xyz2);
  if (stat != CAPS_SUCCESS)
    printf(" CART3DAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[1], stat);
  stat = EG_getGlobal(bodies[discr->mapping[2*in[2]]+nBody-1],
                      discr->mapping[2*in[2]+1], &ptype, &pindex, xyz3);
  if (stat != CAPS_SUCCESS)
    printf(" CART3DAIM/aimIntegrateBar: %d EG_getGlobal %d = %d!\n",
           discr->instance, in[2], stat);

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
