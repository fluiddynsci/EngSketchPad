/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             AVL AIM
 *
 *      Copyright 2014-2020, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 *      Modified by Ryan Durscher AFLR/RQVC May 10, 2016
 */

#include <string.h>
#include <math.h>
#include "aimUtil.h"
#include "caps.h"

#include "miscUtils.h" // Bring in miscellaneous utilities
#include "vlmUtils.h"
#include "vlmSpanSpace.h"

#ifdef WIN32
#define getcwd   _getcwd
#define PATH_MAX _MAX_PATH

#define strcasecmp  stricmp
#define strncasecmp _strnicmp
#define strtok_r   strtok_s

#else

#include <unistd.h>
#include <limits.h>
#endif

#define NUMINPUT  18
#define NUMOUT    94
#define MAXPOINT  200
#define PI        3.1415926535897931159979635
#define NINT(A)         (((A) < 0)   ? (int)(A-0.5) : (int)(A+0.5))

//#define DEBUG

/*!\mainpage Introduction
 * \tableofcontents
 *
 * \section overviewAVL AVL AIM Overview
 * The use of lower-dimensional design tools is clearly desirable in a multidisciplinary/multi-fidelity aero
 * design optimization setting. This is the crux of the Computational Aircraft Prototype Syntheses (CAPS) program.
 * In many ways describing geometry appropriate for AVL (the Athena Vortex Lattice) code is more cumbersome than
 * higher fidelity codes that require an Outer Mold Line. The goal is to make a CAPS AIM (Analysis Input Module)
 * that directly feeds input to AVL and extracts the output quantities of interest from AVL's execution. This
 * needs to be consistent with a build description that is hierarchical and multi-fidelity. That is, the build
 * description that generates the geometric data at this level can be further enhanced to produce the complete
 * OML of the aircraft design under consideration. As for the geometric description, AVL requires airfoil section
 * data specified at the appropriate locations that describe the <em> skeleton </em> of the aircraft. These
 * sections when <em> lofted </em> as groups and finally <em> unioned </em> together builds the OML. Clearly,
 * intercepting the state of the geometry before these higher-level operations are applied provides the data
 * appropriate for AVL. This naturally constructs a hierarchical geometric view where a design can progress
 * into higher fidelities and feedback can be achieved where we can go back to this level of description when need be.
 *
 * An outline of the AIM's inputs and outputs are provided in \ref aimInputsAVL and \ref aimOutputsAVL, respectively. An
 * alternative to the AIM's outputs for retrieving sensitivity information is provided in \ref aimBackDoorAVL.
 *
 * The accepted and expected geometric representation and analysis intentions are detailed in \ref geomRepIntentAVL. Similarly, other geometric attribution
 * that the AIM makes use is provided in \ref attributeAVL.
 *
 * Upon running preAnalysis the AIM generates two files, 1) "avlInput.txt" which contains the input information and
 * control sequence for AVL to execute and 2) "caps.avl" which contains the geometry to be analyzed.
 * To populate output data the AIM expects files, "capsTotalForce.txt", "capsStripForce.txt", "capsStatbilityDeriv.txt", "capsBodyAxisDeriv.txt", and
 * "capsHingeMoment.txt" to exist after running AVL
 * (see \ref aimOutputsAVL for additional information). An example execution for AVL looks like:
 *
 * \code{.sh}
 * avl caps < avlInput.txt > avlOutput.txt"
 * \endcode
 *
 * \section assumptionsAVL Assumptions
 * The AVL coordinate system assumption (X -- downstream, Y -- out the right wing, Z -- up) needs to be followed.
 *
 * Within <b> OpenCSM</b> there are a number of airfoil generation UDPs (User Defined Primitives). These include NACA 4
 * series, a more general NACA 4/5/6 series generator, Sobieczky's PARSEC parameterization and Kulfan's CST
 * parameterization. All of these UDPs generate <b> EGADS</b> <em> FaceBodies</em> where the <em>Face</em>'s underlying
 * <em>Surface</em>  is planar and the bounds of the <em>Face</em> is a closed set of <em>Edges</em> whose
 * underlying <em>Curves</em> contain the airfoil shape. In all cases, there is a <em>Node</em> that represents
 * the <em>Leading Edge</em> point and one or two <em>Nodes</em> at the <em>Trailing Edge</em> -- one if the
 * representation is for a sharp TE and the other if the definition is open or blunt. If there are 2 <em>Nodes</em>
 * at the back, then there are 3 <em>Edges</em> all together and closed, even though the airfoil definition
 * was left open at the TE. All of this information will be used to automatically fill in the AVL geometry description.
 *
 * The AVL Sections are automatically generated, one from each <em> FaceBody</em> and the details extracted from the geometry.
 * The <em> FaceBody</em> must contain at least two edges and two nodes, but may contain any number of <em> Edges</em> otherwise.
 * If the <em> FaceBody</em> contains more nodes, the node with the smallest <b> x</b> value is used to
 * define the leading edge, the node with the largest <b> x</b> defines the trailing edge. The airfoil may have a
 * single <em> Edge</em> that defines a straight blunt trailing edge. <b> Xle</b>, <b> Yle</b>, and <b> Zle</b>, are taken
 * from the <em> Node</em> associated with the <em> Leading Edge</em>. The <b> Chord</b> is computed by getting
 * the distance between the LE and TE (if there is a blunt trainling <em> Edge</em> in the <em> FaceBody</em>
 * the TE point is considered the mid-position on that <em> Edge</em>). <b> Ainc</b> is computed by
 * registering the chordal direction of the <em> FaceBody</em> against the X-Z plane. The airfoil shapes are generated by
 * sampling the <em> Curves</em> and put directly in the input file via the <b> AIRFOIL</b> keyword after being normalized.
 *
 * It should be noted that general construction in either <b> OpenCSM</b> or even <b> EGADS</b> will be supported
 * as long as the topology described above is used. But care should be taken when constructing the airfoil shape
 * so that a discontinuity (i.e.,  simply <em>C<sup>0</sup></em>) is not generated at the <em>Node</em> representing
 * the <em>Leading Edge</em>. This can be done by splining the entire shape as one and then intersecting the single
 *  <em>Edge</em> to place the LE <em>Node</em>.
 *
 *  The rest of the information and options required to fill out the AVL geometry input file (<b> xxx.avl</b>)
 *  will be found in the attributes attached to the <em>FaceBody</em> itself. The conventions used will be
 *  described in the next section.
 *
 * Also note that this first implementation is not intended to provide complete control over AVL. In particular,
 * there is no mention above of the <b> BODY</b>, <b> DESIGN</b>, <b> CLAF</b>, or <b> CDCL</b> AVL keywords.
 *
 * \section exampleAVL Examples
 * An example problem using the AVL AIM may be found at \ref avlExample, which
 * contains example *.csm input files and pyCAPS scripts designed to make use of the AVL AIM.  These example
 * scripts make extensive use of the \ref attributeAVL, \ref aimInputsAVL, and \ref aimOutputsAVL.
 *
 */

/*! \page avlExample AVL AIM Examples
 * This example contains a set of *.csm and pyCAPS (*.py) inputs that uses the AVL AIM.  A user should have knowledge
 * on the generation of parametric geometry in Engineering Sketch Pad (ESP) before attempting to integrate with any AIM.
 * Specifically, this example makes use of Design Parameters, Set Parameters, User Defined Primitive (UDP) and attributes
 * in ESP.
 *
 * The follow code details the process in a *.csm file that generates three airfoil sections to create a wing. Note
 * to execute in serveCSM a dictionary file must be included
 * "serveCSM $ESP_ROOT/CAPSexamples/csmData/avlWing.csm"
 *
 * The CSM script generates Bodies which are designed to be used by specific AIMs.
 * The AIMs that the Body is designed for is communicated to the CAPS framework via
 * the capsAIM string attribute. This is a semicolon-separated string with the list of
 * AIM names. Thus, the CSM author can give a clear indication to which AIMs should
 * use the Body. In this example, the list contains only the avlAIM:
 * \snippet avlWing.csm capsAIM
 *
 * Next we will define the design parameters to define the wing cross section and planform.
 * \snippet avlWing.csm designParameters
 *
 * The design parameters will then be used to set parameters for use internally to create geometry.
 * \snippet avlWing.csm setParameters
 *
 * Finally, the airfoils are created using the User Defined Primitive (UDP) naca. The inputs used for this
 * example to the UDP are Thickness and Camber. Cross sections are in the X-Y plane and are rotated to the X-Z plane.
 * Reference quantities must exist on any body, otherwise AVL defaults to 1.0 for Area, Span,
 * Chord and 0.0 for X,Y,Z moment References
 *
 * \snippet avlWing.csm createAirfoils
 *
 * An example pyCAPS script that uses the above csm file to run AVL is as follows.
 *
 * First the pyCAPS and os module needs to be imported.
 * \snippet avl_PyTest.py import
 *
 * Note if your Python major version is less than 3 (i.e. Python 2.7). The following statement should also be included
 * so that print statements work correctly.
 *
 * \snippet avl_PyTest.py importPrint
 *
 * Once the modules have been loaded the problem needs to be initiated.
 *
 * \snippet avl_PyTest.py initateProblem
 *
 * Next the *.csm file is loaded and design parameter is changed - area in the geometry. Any despmtr from the
 * avlWing.csm file is available inside the pyCAPS script. They are: thick, camber, area, aspect, taper,
 * sweep, washout, dihedral...
 *
 * \snippet avl_PyTest.py geometry
 *
 * The AVL AIM is then loaded with:
 * \snippet avl_PyTest.py loadAIM
 *
 * After the AIM is loaded the Mach number and angle of attack are set, though all \ref aimInputsAVL are available.
 * \snippet avl_PyTest.py setInputs
 *
 * Once all the inputs have been set, preAnalysis needs to be executed. During this operation, all the necessary files to
 * run AVL are generated and placed in the analysis working directory (analysisDir)
 * \snippet avl_PyTest.py preAnalysis
 *
 * An OS system call is then made from Python to execute AVL.
 * \snippet avl_PyTest.py runAVL
 *
 * A call to postAnalysis is then made to check to see if AVL executed successfully and the expected files were generated.
 * \snippet avl_PyTest.py postAnalysis
 *
 * Similar to the AIM inputs, after the execution of AVL and postAnalysis any of the AIM's output
 * variables (\ref aimOutputsAVL) are readily available; for example,
 * \snippet avl_PyTest.py output
 *
 * results in
 * \code
 * CXtot   -0.00033
 * CYtot   1e-05
 * CZtot   -0.30016
 * Cltot   -0.0
 * Cmtot   -0.19468
 * Cntot   -1e-05
 * Cl'tot  -0.0
 * Cn'tot  -1e-05
 * CLtot   0.30011
 * CDtot   0.00557
 * CDvis   0.0
 * CLff    0.29968
 * CYff    0.0
 * CDind   0.00557
 * CDff    0.00492
 * e       0.9691
 * \endcode
 *
 * Additionally, besides making a call to the AIM outputs, sensitivity values may be obtained in the following manner,
 * \snippet avl_PyTest.py sensitivity
 *
 * The avlAIM supports the control surface modeling functionality inside AVL.  Trailing edge control surfaces can be
 * added to the above example by making use of the vlmControlName attribute (see \ref attributeAVL regarding the attribution
 * specifics).  To add a <b>RightFlap</b> and <b>LeftFlap</b> to the
 * previous example *.csm file the naca UDP entries are augmented with the following attributes.
 *
 * \code {.py}
 * # left tip
 * udprim    naca      Thickness thick     Camber    camber
 * attribute vlmControl_LeftFlap 80 # Hinge line is at 80% of the chord
 * ...
 *
 * # root
 * udprim    naca      Thickness thick     Camber    camber
 * attribute vlmControl_LeftFlap 80 # Hinge line is at 80% of the chord
 * attribute vlmControl_RightFlap 80 # Hinge line is at 80% of the chord
 * ...
 *
 * # right tip
 * udprim    naca      Thickness thick     Camber    camber
 * attribute vlmControl_RightFlap 80 # Hinge line is at 80% of the chord
 * ...
 * \endcode
 *
 * Note how the root airfoil contains two attributes for both the left and right flaps.
 *
 * In the pyCAPS script the \ref aimInputsAVL, <b>AVL_Control</b>, must be defined.
 * \code {.py}
 * flap = {"controlGain"     : 0.5,
 *         "deflectionAngle" : 10.0}
 *
 *  myAnalysis.setAnalysisVal("AVL_Control", [("LeftFlap", flap), ("RightFlap", flap)])
 \endcode
 *
 * Notice how the information defined in the <b>flap</b> variable is assigned to the vlmControl<b>Name</b> portion of the
 * attributes added to the *.csm file.
 *
 */

/*! \page attributeAVL AIM Attributes
 * The following list of attributes drives the AVL geometric definition. Each <em>FaceBody</em> which relates to AVL
 * <b> Sections</b> will be marked up in an appropriate manner to drive the input file construction. Many attributes
 * are required and those that are optional are marked so in the description:
 *
 * - <b> capsReferenceArea</b>  [Optional: Default 1.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used as the SREF entry in the AVL input.
 *
 * - <b> capsReferenceChord</b>  [Optional: Default 1.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used as the CREF entry in the AVL input.
 *
 * - <b> capsReferenceSpan</b>  [Optional: Default 1.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used as the BREF entry in the AVL input.
 *
 * - <b> capsReferenceX</b>  [Optional: Default 0.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used as the Xref entry in the AVL input.
 *
 * - <b> capsReferenceY</b>  [Optional: Default 0.0] This attribute may exist on any <em> Body</em>.  Its
 * value will be used as the Yref entry in the AVL input.
 *
 * - <b> capsReferenceZ</b>  [Optional: Default 0.0] This attribute may exist on any <em> Body</em>.  Its
 *  value will be used as the Zref entry in the AVL input.
 *
 * - <b> capsGroup</b> This string attribute labels the <em> FaceBody</em> as to which AVL Surface the section
 *  is assigned. This should be something like: <em> Main\_Wing</em>, <em> Horizontal\_Tail</em>, etc. This informs
 *  the AVL AIM to collect all <em> FaceBodies</em> that match this attribute into a single AVL Surface.
 *
 * - <b> vlmControl"Name"</b> This string attribute attaches a control surface to the <em> FaceBody</em>. The hinge
 * location is defined as the double value between 0 or 1.0.  The range as percentage from 0 to 100
 * will also work. The name of the
 * control surface is the string information after vlmControl (or vlmControl\_).  For Example, to define a control
 * surface named Aileron the following are identical (<em>attribute vlmControlAileron 0.8 </em> or
 * <em>attribute vlmControl\_Aileron 80</em>) . Multiple <em> vlmControl</em> attributes, with different
 * names, can be defined on a single <em> FaceBody</em>.
 * <br> <br>By default control surfaces with percentages less than 0.5 (< 50%) are considered leading edge flaps, while values
 * greater than or equal to 0.5 (>= 50%) are considered trailing edge flaps. This behavior may be overwritten when setting up
 * the control surface in "AVL_Control" (see \ref aimInputsAVL) with the keyword "leOrTe" (see \ref vlmControl for additional details).
 *
 * - <b> vlmNumSpan </b> This attribute may be set on any given airfoil cross-section to overwrite the number of
 * spanwise horseshoe vortices placed on the surface (globally set - see keyword "numSpanPerSection" and "numSpanTotal" in \ref vlmSurface )
 * between two sections. Note, that the AIM internally sorts the sections in ascending y (or z) order, so care
 * should be taken to select the correct section for the desired intent.
 *
 * Note: The attribute <b> avlNumSpan </b> has been deprecated in favor of <b> vlmNumSpan </b>
 *
 * - <b> vlmSspace </b> This attribute may be set on any given airfoil cross-section in the range [-1 .. 1] specify the spanwise distribution function.
 *
 *
 */


// To be changed
/*  - <b> avlComponent</em>  [optional]. This integer (or real) attribute is examined for the AVL Component index.
 *  This may be on any or all <em> FaceBodies</em> associated with the AVL Surface.

- <b> avlKeyword</em>  [optional]. This string attribute may contain the word (or words): {\tt NOWAKE</em>, {\tt NOALBE</em> and/or {\tt NOLOAD</em>. Multiple keywords need
to be delimited by a colon (`:'). This is associated with the AVL Surface.


\item {\bf avlSecValues.} This real valued attribute must be of at least 5 {\it doubles} in length and contain:
   \begin{enumerate}
   \item {\tt Nchord} -- The number of chordwise horseshoe vortices placed on the surface.
   \item {\tt Cspace} -- The chordwise vortex spacing parameter.
   \item {\tt Nspan} -- The number of spanwise horseshoe vortices placed on the surface.
   \item {\tt Sspace} -- The spanwise vortex spacing parameter.
   \item {\tt iYdup} -- The Y symmetry for this Surface (0 -- do nothing, 1 -- duplicate about the Y=0.0 plane).
   \end{enumerate}
   It should be noted that the first 2 (and last) values refer to the entire AVL Surface/capsGroup and therefore the first occurrence of the \textbf{\textit {capsGroup}} attribute in the list of {\it bodies} sets these values in the AVL input file (though {\tt iYdup} can be set in any Section).

\item {\bf avlCntrl}\textbf{\textit {Name}}  [optional]. This optional numerical attribute indicates that this {\it FaceBody} Section is a member of the control surface named as part of the attribute name (for example:  {\tt avlCntrlAileron} specifies that this Section is part of the {\it Aileron} control surface). This real valued attribute must be of at least 6 {\it doubles} in length and contain:
   \begin{enumerate}
   \item {\tt gain} -- The control deflection gain.
   \item {\tt Xhinge} -- The x/c location of hinge.
   \item {\tt Xhvec} -- The first component of the vector giving hinge axis about which surface rotates.
   \item {\tt Yhvec} -- The second component.
   \item {\tt Zhvec} -- The third component.
   \item {\tt SgnDup } -- The signed magnitude (or simply the scale factor) for the deflection of the duplicated surface.
   \end{enumerate}
\end{itemize}
 */


typedef struct {
    // Analysis file path/directory
    char *analysisPath;

    mapAttrToIndexStruct controlMap;

} aimStorage;

static aimStorage *avlInstance = NULL;
static int         numInstance  = 0;


/* ********************** AVL AIM Helper Functions ************************** */
static int writeSection(FILE *fp, vlmSectionStruct *vlmSection)
{
    int status; // Function return status

    ego body;
    double *xyzLE;
    int Nspan;
    double chord, ainc, Sspace;

    // Attribute variables
    int          atype, alen;
    const int    *ints;
    const char   *string;
    const double *reals;

    // Get data from the section
    body  = vlmSection->ebody;
    xyzLE = vlmSection->xyzLE;
    chord = vlmSection->chord;
    ainc  = vlmSection->ainc;
    Nspan = vlmSection->Nspan;
    Sspace = vlmSection->Sspace;

    status = EG_attributeRet(body, "avlNumSpan", &atype, &alen, &ints, &reals, &string);
    if (status == EGADS_SUCCESS) {
        printf("*************************************************************\n");
        printf("Warning: avlNumSpan is DEPRICATED in favor of vlmNumSpan!!!\n");
        printf("         Please update the attribution.\n");
        printf("*************************************************************\n");

        if (atype != ATTRINT && atype != ATTRREAL && alen != 1) {
            printf ("Error: Attribute %s should be followed by a single integer\n", "avlNumSpan");
        }

        if (atype == ATTRINT) {
            Nspan = ints[0];
        }

        if (atype == ATTRREAL) {
            Nspan = (int) reals[0];
        }
    }

    fprintf(fp, "#Xle     Yls       Zle       Chord    Ainc  Nspan  Sspace\n");
    fprintf(fp, "SECTION\n%lf %lf %lf  %lf %lf  %d %lf\n\n",
            xyzLE[0], xyzLE[1], xyzLE[2], chord, ainc, Nspan, Sspace);
    fprintf(fp, "AIRFOIL\n");

    status = vlm_writeSection(fp,
                              vlmSection,
                              (int) true, // Normalize by chord (true/false)
                              (int) MAXPOINT);
    if (status != CAPS_SUCCESS) goto cleanup;

    status = CAPS_SUCCESS;

    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in writeSection, status = %d\n", status);

        return status;
}

static int writeMassFile(void *aimInfo, capsValue *aimInputs, const char *lengthUnitsIn, char massFilename[])
{
  int status; // Function return status

  const char *keyWord = NULL;
        char *keyValue = NULL; // Freeable

  int numString = 0;
  char **stringArray = NULL; // Freeable

  // Eigen value quantities
  double Lunit, Munit, Tunit;
  char *Lunits, *Munits, *Tunits;
  double gravity, density;
  char *Funits = NULL, *Dunits = NULL, *Iunits = NULL, *tmpUnits = NULL;
  double MLL;

  double mass, xyz[3], inertia[6]; // Inertia order = Ixx, Iyy, Izz, Ixy, Ixz, Iyz
  double *I= NULL; // Freeable

  char *value = NULL, *units = NULL, *errUnits = NULL, *errName = NULL, *errValue = NULL;
  const char *errMsg = NULL;
  capsTuple *massProp;
  int massPropLen, inertiaLen;

  int i, j; // Indexing

  FILE *fp = NULL;

  /* open the file and write the mass data */
  fp = fopen(massFilename, "w");
  if (fp == NULL) {
      printf("Unable to open file %s\n!", massFilename);

      status =  CAPS_IOERR;
      goto cleanup;
  }

  printf("Writing mass properties file: %s\n", massFilename);

  fprintf(fp, "#-------------------------------------------------\n");
  fprintf(fp, "#  Dimensional unit and parameter data.\n");
  fprintf(fp, "#  Mass & Inertia breakdown.\n");
  fprintf(fp, "#-------------------------------------------------\n");
  fprintf(fp, "\n");
  fprintf(fp, "#  Names and scalings for units to be used for trim and eigenmode calculations.\n");
  fprintf(fp, "#  The Lunit and Munit values scale the mass, xyz, and inertia table data below.\n");
  fprintf(fp, "#  Lunit value will also scale all lengths and areas in the AVL input file.\n");

  Lunit  = aimInputs[aim_getIndex(aimInfo, "Lunit", ANALYSISIN)-1].vals.real;
  Lunits = aimInputs[aim_getIndex(aimInfo, "Lunit", ANALYSISIN)-1].units;

  Munit  = aimInputs[aim_getIndex(aimInfo, "Munit", ANALYSISIN)-1].vals.real;
  Munits = aimInputs[aim_getIndex(aimInfo, "Munit", ANALYSISIN)-1].units;

  Tunit  = aimInputs[aim_getIndex(aimInfo, "Tunit", ANALYSISIN)-1].vals.real;
  Tunits = aimInputs[aim_getIndex(aimInfo, "Tunit", ANALYSISIN)-1].units;

  // conversion of Lunits into the units of the csm model
  status = aim_convert(aimInfo, lengthUnitsIn, 1.0, Lunits, &Lunit);
  if (status != CAPS_SUCCESS) goto cleanup;

  fprintf(fp, "Lunit = %lf %s\n", Lunit, Lunits);
  fprintf(fp, "Munit = %lf %s\n", Munit, Munits);
  fprintf(fp, "Tunit = %lf %s\n", Tunit, Tunits);
  fprintf(fp, "\n");
  fprintf(fp, "#-------------------------\n");
  fprintf(fp, "#  Gravity and density to be used as default values in trim setup.\n");
  fprintf(fp, "#  Must be in the units given above.\n");

  status = aim_unitRaise(aimInfo, Tunits, -2, &tmpUnits ); // 1/time^2
  if (status != CAPS_SUCCESS) goto cleanup;
  status = aim_unitMultiply(aimInfo, Lunits, tmpUnits, &Funits ); // length/time^2, e.g force
  if (status != CAPS_SUCCESS) goto cleanup;
  EG_free(tmpUnits); tmpUnits = NULL;

  status = aim_unitRaise(aimInfo, Lunits, -3, &tmpUnits ); // 1/length^3
  if (status != CAPS_SUCCESS) goto cleanup;
  status = aim_unitMultiply(aimInfo, Munits, tmpUnits, &Dunits ); // mass/length^3, e.g density
  if (status != CAPS_SUCCESS) goto cleanup;
  EG_free(tmpUnits); tmpUnits = NULL;

  status = aim_convert(aimInfo, aimInputs[aim_getIndex(aimInfo, "Gravity", ANALYSISIN)-1].units,
                                aimInputs[aim_getIndex(aimInfo, "Gravity", ANALYSISIN)-1].vals.real,
                                Funits, &gravity);
  if (status != CAPS_SUCCESS) goto cleanup;
  status = aim_convert(aimInfo, aimInputs[aim_getIndex(aimInfo, "Density", ANALYSISIN)-1].units,
                                aimInputs[aim_getIndex(aimInfo, "Density", ANALYSISIN)-1].vals.real,
                                Dunits, &density);
  if (status != CAPS_SUCCESS) goto cleanup;

  fprintf(fp, "g   = %lf\n", gravity/(Lunit/(Tunit*Tunit)));
  fprintf(fp, "rho = %lf\n", density/(Munit/pow(Lunit,3)));
  fprintf(fp, "\n");
  fprintf(fp, "#-------------------------\n");
  fprintf(fp, "#  Mass & Inertia breakdown.\n");
  fprintf(fp, "#  x y z  is location of item's own CG.\n");
  fprintf(fp, "#  Ixx... are item's inertias about item's own CG.\n");
  fprintf(fp, "#\n");
  fprintf(fp, "#  x,y,z system here must be exactly the same one used in the AVL input file\n");
  fprintf(fp, "#     (same orientation, same origin location, same length units)\n");
  fprintf(fp, "#\n");
  fprintf(fp, "#  mass     x     y     z       Ixx    Iyy    Izz   [ Ixy  Ixz  Iyz ]\n");
  fprintf(fp, "#\n");

  massProp    = aimInputs[aim_getIndex(aimInfo, "MassProp", ANALYSISIN)-1].vals.tuple;
  massPropLen = aimInputs[aim_getIndex(aimInfo, "MassProp", ANALYSISIN)-1].length;

  status = aim_unitRaise(aimInfo, Lunits, 2, &tmpUnits ); // length^2
  if (status != CAPS_SUCCESS) goto cleanup;
  status = aim_unitMultiply(aimInfo, Munits, tmpUnits, &Iunits ); // mass*length^2, e.g moment of inertia
  if (status != CAPS_SUCCESS) goto cleanup;
  EG_free(tmpUnits); tmpUnits = NULL;

  MLL = Munit * Lunit * Lunit;

  //status = writeMassProp(aimInfo, aimInputs, fp);
  //if (status != CAPS_SUCCESS) goto cleanup;

  printf("Parsing MassProp\n");
  for (i = 0; i< massPropLen; i++ ) {

      // Set error message strings
      errName = massProp[i].name;
      errValue = massProp[i].value;

      // Do we have a json string?
       if (strncmp( massProp[i].value, "{", 1) != 0) {
           errMsg = "  MassProp tuple value is expected to be a JSON string dictionary";
           status = CAPS_BADVALUE;
           goto cleanup;
       }

       keyWord = "mass";
       status = search_jsonDictionary( massProp[i].value, keyWord, &keyValue);
       if (status == CAPS_SUCCESS) {

           status = json_parseTuple(keyValue, &numString, &stringArray);
           if (status != CAPS_SUCCESS) goto cleanup;

           if (numString != 2) {
               errMsg = "  No units specified";
               status = CAPS_BADVALUE;
               goto cleanup;
           }

           units = stringArray[1];
           status = string_toDouble(stringArray[0], &mass);
           if (status != CAPS_SUCCESS) goto cleanup;

           status = aim_convert(aimInfo, units, mass, Munits, &mass);
           if (status != CAPS_SUCCESS)  { errUnits = Munits; goto cleanup; }

           EG_free(keyValue); keyValue = NULL;
           (void) string_freeArray(numString, &stringArray);
           units = NULL;

       } else {
           goto cleanup;
       }

       keyWord = "CG";
       status = search_jsonDictionary( massProp[i].value, keyWord, &keyValue);
       if (status == CAPS_SUCCESS) {

           status = json_parseTuple(keyValue, &numString, &stringArray);
           if (status != CAPS_SUCCESS) goto cleanup;

           if (numString != 2) {
               errMsg = "  No units specified";
               status = CAPS_BADVALUE;
               goto cleanup;
           }

           units = stringArray[1];

           status = string_toDoubleArray(stringArray[0], 3, xyz);
           if (status != CAPS_SUCCESS) goto cleanup;

           status = aim_convert(aimInfo, units, xyz[0], Lunits, &xyz[0]); if (status != CAPS_SUCCESS) { errUnits = Lunits; goto cleanup; }
           status = aim_convert(aimInfo, units, xyz[1], Lunits, &xyz[1]); if (status != CAPS_SUCCESS) { errUnits = Lunits; goto cleanup; }
           status = aim_convert(aimInfo, units, xyz[2], Lunits, &xyz[2]); if (status != CAPS_SUCCESS) { errUnits = Lunits; goto cleanup; }

           EG_free(keyValue); keyValue = NULL;
           (void) string_freeArray(numString, &stringArray);
           units = NULL;

       } else {
           goto cleanup;
       }

       keyWord = "massInertia";
       status = search_jsonDictionary( massProp[i].value, keyWord, &keyValue);
       if (status == CAPS_SUCCESS) {

           status = json_parseTuple(keyValue, &numString, &stringArray);
           if (status != CAPS_SUCCESS) goto cleanup;

           if (numString != 2) {
               errMsg = "  No units specified";
               status = CAPS_BADVALUE;
               goto cleanup;
           }

           units = stringArray[1];

           status = string_toDoubleDynamicArray(stringArray[0], &inertiaLen, &I);
           if (status != CAPS_SUCCESS) goto cleanup;

           for (j = 0; j< 6; j++) inertia[j] = 0; // Inertia order = Ixx, Iyy, Izz, Ixy, Ixz, Iyz

           for (j = 0; j < inertiaLen; j++) {
               status = aim_convert(aimInfo, units, I[j], Iunits, &inertia[j]); if (status != CAPS_SUCCESS) { errUnits = Iunits; goto cleanup; }
           }

           EG_free(I); I = NULL;
           EG_free(keyValue); keyValue = NULL;
           (void) string_freeArray(numString, &stringArray);
           units = NULL;

       } else {
           goto cleanup;
       }

      // Normalize away the Lunit, Munit, Tunit values as AVL will multiply everything by those values
      fprintf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf ! %s\n",
                  mass/Munit,
                  xyz[0]/Lunit, xyz[1]/Lunit, xyz[2]/Lunit,
                  inertia[0]/MLL, inertia[1]/MLL, inertia[2]/MLL, inertia[3]/MLL, inertia[4]/MLL, inertia[5]/MLL,
                  massProp[i].name);
  }

#if 0
  // I think this loop could be entirely deleted.
  for (i = 0; i < massPropLen; i++) {

    // set error message strings
    errName = massProp[i].name;
    errValue = massProp[i].value;

    // copy the value string so it can be manipulated
    value = EG_strdup(massProp[i].value);
    if (value == NULL) { status = EGADS_MALLOC; goto cleanup; }

    // replace any parens with square brackets
    while ((str = strstr(value, "(")) != NULL) str[0] = '[';
    while ((str = strstr(value, ")")) != NULL) str[0] = ']';

    // check the value string for errors
    // the string should look like:

    // [[mass,"kg"], [x,y,z,"m"], [Ixx, Iyy, Izz, Ixy, Ixz, Iyz, "kg*m2"]]

    // make sure it is long enough
    if (strlen(value) <= 2) { status = CAPS_BADVALUE; goto cleanup; }

    // clear initial and closing brackets
    if (value[              0] == '[') value[              0] = ' '; else { status = CAPS_BADVALUE; goto cleanup; }
    if (value[strlen(value)-1] == ']') value[strlen(value)-1] = ' '; else { status = CAPS_BADVALUE; goto cleanup; }

    // make sure there are 3 sub-arrays
    end = value + strlen(value);
    str = value;
    count = 0;
    while (str+1 < end && (str = strstr(str+1, "[")) != NULL) count++;
    if (count != 3)  { status = CAPS_BADVALUE; goto cleanup; }
    str = value;
    count = 0;
    while (str+1 < end && (str = strstr(str+1, "]")) != NULL) count++;
    if (count != 3)  { status = CAPS_BADVALUE; goto cleanup; }

    // check that there are 2 entries in the first sub-array and the end is a string
    end = strstr(value, "]"); if (end == NULL) { status = CAPS_BADVALUE; goto cleanup; }
    str = value;
    count = 0;
    while (str+1 < end && (str = strstr(str+1, ",")) != NULL && str < end) count++;
    if (str == NULL)      { status = CAPS_BADVALUE; goto cleanup; }
    if (count != 1)       { status = CAPS_BADVALUE; goto cleanup; }
    if (*(end-1) != '\"') { status = CAPS_BADVALUE; goto cleanup; }

    // check that there are 4 entries in the second sub-array and the end is a string
    end = strstr(str, "]"); if (end == NULL) { status = CAPS_BADVALUE; goto cleanup; }
    count = 0;
    while (str+1 < end && (str = strstr(str+1, ",")) != NULL && str < end) count++;
    if (str == NULL)      { status = CAPS_BADVALUE; goto cleanup; }
    if (count != 3)       { status = CAPS_BADVALUE; goto cleanup; }
    if (*(end-1) != '\"') { status = CAPS_BADVALUE; goto cleanup; }

    // check that there are 4 or 7 entries in the third sub-array and the end is a string
    end = strstr(str, "]"); if (end == NULL) { status = CAPS_BADVALUE; goto cleanup; }
    count = 0;
    while (str+1 < end && (str = strstr(str+1, ",")) != NULL && str < end) count++;
    if (count != 3 && count != 6) { status = CAPS_BADVALUE; goto cleanup; }
    if (*(end-1) != '\"')         { status = CAPS_BADVALUE; goto cleanup; }

    // remove all delimiters and other symbols
    while ((str = strstr(value, "[")) != NULL) str[0] = ' ';
    while ((str = strstr(value, ",")) != NULL) str[0] = ' ';
    while ((str = strstr(value, "]")) != NULL) str[0] = ' ';
    while ((str = strstr(value, "\"")) != NULL) str[0] = ' ';
    // the value string now contains:
    // mass kg   x y z m   Ixx Iyy Izz Ixy Ixz Iyz kg*m2

    str = value;
    end = value+strlen(value);

    Ixx = Iyy = Izz = Ixy = Ixz = Iyz = 0;

    // parse the value/unit pairs
    mass = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    units = strtok_r(str, " ", &str);
    status = aim_convert(aimInfo, units, mass, Munits, &mass); if (status != CAPS_SUCCESS)  { errUnits = Munits; goto cleanup; }

    x = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    y = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    z = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    units = strtok_r(str, " ", &str);
    status = aim_convert(aimInfo, units, x, Lunits, &x); if (status != CAPS_SUCCESS) { errUnits = Lunits; goto cleanup; }
    status = aim_convert(aimInfo, units, y, Lunits, &y); if (status != CAPS_SUCCESS) { errUnits = Lunits; goto cleanup; }
    status = aim_convert(aimInfo, units, z, Lunits, &z); if (status != CAPS_SUCCESS) { errUnits = Lunits; goto cleanup; }

    Ixx = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    Iyy = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    Izz = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    if (count == 6) {
    Ixy = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    Ixz = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    Iyz = strtod(str, &next); if (str != next) str = next; else { status = CAPS_BADVALUE; goto cleanup; }
    }
    units = strtok_r(str, " ", &str);
    status = aim_convert(aimInfo, units, Ixx, Iunits, &Ixx); if (status != CAPS_SUCCESS) { errUnits = Iunits; goto cleanup; }
    status = aim_convert(aimInfo, units, Iyy, Iunits, &Iyy); if (status != CAPS_SUCCESS) { errUnits = Iunits; goto cleanup; }
    status = aim_convert(aimInfo, units, Izz, Iunits, &Izz); if (status != CAPS_SUCCESS) { errUnits = Iunits; goto cleanup; }
    status = aim_convert(aimInfo, units, Ixy, Iunits, &Ixy); if (status != CAPS_SUCCESS) { errUnits = Iunits; goto cleanup; }
    status = aim_convert(aimInfo, units, Ixz, Iunits, &Ixz); if (status != CAPS_SUCCESS) { errUnits = Iunits; goto cleanup; }
    status = aim_convert(aimInfo, units, Iyz, Iunits, &Iyz); if (status != CAPS_SUCCESS) { errUnits = Iunits; goto cleanup; }

    // Normalize away the Lunit, Munit, Tunit values as AVL will multiply everything by those values
    fprintf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf ! %s\n",
                  mass/Munit,
                  x/Lunit, y/Lunit, z/Lunit,
                  Ixx/MLL, Iyy/MLL, Izz/MLL, Ixy/MLL, Ixz/MLL, Iyz/MLL, massProp[i].name);

    EG_free(value); value = NULL;

    errName = NULL;
    errValue = NULL;
  }
#endif


  status = CAPS_SUCCESS;

cleanup:

  if (status != CAPS_SUCCESS && errName != NULL && errValue != NULL) {
      printf("*********************************************************************\n");
      printf("Cannot parse mass properties for:\n");
      printf("  (\"%s\", %s)\n", errName, errValue);
      if (status == CAPS_UNITERR) {
      printf("\n");
      printf("  Unable to convert units \"%s\" to \"%s\"\n", units, errUnits);
      }
      if (errMsg != NULL && keyWord != NULL) {
      printf("\n");
      printf("%s for %s\n", errMsg, keyWord);
      } else if (errMsg != NULL) {
        printf("\n");
        printf("%s\n", errMsg);
      }
      printf("\n");
      printf("  The 'value' string should be of the form:\n");
      printf("\t{\"mass\":[mass,\"kg\"], \"CG\":[[x,y,z],\"m\"], \"massInertia\":[[Ixx, Iyy, Izz, Ixy, Ixz, Iyz], \"kg*m2\"]}\n");
      printf("*********************************************************************\n");
  }

  // close the file
  if (fp != NULL) fclose(fp);
  fp = NULL;

  EG_free(I);
  EG_free(value);
  EG_free(Funits);
  EG_free(Dunits);
  EG_free(Iunits);
  EG_free(tmpUnits);

  EG_free(keyValue);
  string_freeArray(numString, &stringArray);

  EG_free(I);

  return status;
}


static int read_Data(const char *file, const char *analysisPath, const char *key, double *data) {

    int i; //Indexing

    size_t     linecap = 0;
    char       currentPath[PATH_MAX], *valstr = NULL, *line = NULL;
    FILE       *fp = NULL;

    *data = 0.0;

    if (file == NULL) return CAPS_NULLVALUE;
    if (analysisPath == NULL) return CAPS_NULLVALUE;
    if (key == NULL) return CAPS_NULLVALUE;

    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) return CAPS_DIRERR;

    // Open the AVL output file
    fp = fopen(file, "r");
    if (fp == NULL) {

        chdir(currentPath);

        return CAPS_DIRERR;
    }

    // Scan the file for the string
    valstr = NULL;

    while (getline(&line, &linecap, fp) >= 0) {

        if (line == NULL) continue;

        // printf("Key = %s\n", key);

        valstr = strstr(line, key);

        if (valstr != NULL) {

            // Check string for ***
            for (i = 0; i < strlen(valstr); i++) {
                if (valstr[i] == '*'){
                    *data = 0.0;

                    printf("Unreal value for variable %s - setting it to 0.0\n", key);
                    break;
                }
            }
            // Found it -- get the value
            sscanf(&valstr[strlen(key)+1], "%lf", data);
            break;
        }
    }

    // Restore the path we came in with
    chdir(currentPath);
    fclose(fp);

    if (line != NULL) EG_free(line);

    return CAPS_SUCCESS;
}

static int read_StripForces(const char *analysisPath, int *length, capsTuple **surfaces_out) {

    int status = CAPS_SUCCESS;
    int i; //Indexing

    size_t     linecap = 0, vallen = 0, alen = 0;
    char       currentPath[PATH_MAX], *str = NULL, *line = NULL, *rest = NULL, *token = NULL, *value = NULL;
    capsTuple  *tuples = NULL, *surfaces = NULL;
    int        numSurfaces = 0, numDataEntry = 0;
    FILE       *fp = NULL;

    if (analysisPath == NULL) return CAPS_NULLVALUE;

    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) return CAPS_DIRERR;

    // Open the AVL output file
    fp = fopen("capsStripForce.txt", "r");
    if (fp == NULL) {

        chdir(currentPath);

        return CAPS_DIRERR;
    }

    while (true) {

        // Scan for the next surface
        while ((status = getline(&line, &linecap, fp)) >= 0) {
            if (line == NULL) continue;
            str = strstr(line, "Surface #");
            if (str != NULL) break;
        }
        if (status < 0 || str == NULL) {
          // reached end of file without finding another surface
          status = CAPS_SUCCESS;
          break;
        }

        // remove line feed charachters
        while ((str = strstr(line, "\n")) != NULL) str[0] = ' ';
        while ((str = strstr(line, "\r")) != NULL) str[0] = ' ';

        rest = strstr(line, "#");
        token = strtok_r(rest, " ", &rest); // skip the #
        token = strtok_r(rest, " ", &rest); // skip the surface number
        while(rest[0]              == ' ' && rest[0] != '\0') rest++;                      // remove leading spaces
        while(rest[strlen(rest)-1] == ' '                   ) rest[strlen(rest)-1] = '\0'; // remove trailing spaces
        if (rest[0] == '\0') {
            printf("ERROR: Could not find a strip force surface name for surface # %s!\n", token);
            status = CAPS_IOERR;
            goto cleanup;
        }

        // create a new surface and save off the name of the surface, and start the value JSON dictionary
        numSurfaces++;
        surfaces = (capsTuple *)EG_reall(surfaces, sizeof(capsTuple)*(numSurfaces));
        if (surfaces == NULL) { status = EGADS_MALLOC; goto cleanup; }

        surfaces[numSurfaces-1].name  = (char*)EG_alloc((strlen(rest)+1)*sizeof(char));
        surfaces[numSurfaces-1].value = (char*)EG_alloc((strlen("{")+1)*sizeof(char));
        if (surfaces[numSurfaces-1].name  == NULL) { status = EGADS_MALLOC; goto cleanup; }
        if (surfaces[numSurfaces-1].value == NULL) { status = EGADS_MALLOC; goto cleanup; }

        strcpy(surfaces[numSurfaces-1].name, rest);
        strcpy(surfaces[numSurfaces-1].value, "{");

        // Scan the file for the line just before the header string
        while ((status = getline(&line, &linecap, fp)) >= 0) {
            if (line == NULL) continue;
            str = strstr(line, "Strip Forces referred");
            if (str != NULL) break;
        }
        if (status < 0){
            status = CAPS_IOERR;
            goto cleanup;
        }

        // read the header, e.g.
        //   j      Yle    Chord     Area     c cl      ai      cl_norm  cl       cd       cdv    cm_c/4    cm_LE  C.P.x/c
        status = getline(&line, &linecap, fp);
        if (status < 0){
            status = CAPS_IOERR;
            goto cleanup;
        }

        // remove line feed charachters
        while ((str = strstr(line, "\n")) != NULL) str[0] = ' ';
        while ((str = strstr(line, "\r")) != NULL) str[0] = ' ';

        // change "c cl" to "c_cl" so it does not get treated as two headers
        str = strstr(line, "c cl");
        if (str == NULL){
            status = CAPS_IOERR;
            goto cleanup;
        }
        str[1] = '_';

        rest = line;
        // skip the "j" column
        token = strtok_r(rest, " ", &rest);

        numDataEntry = 0;
        while ((token = strtok_r(rest, " ", &rest))) {
            // resize the number of tuples
            tuples = (capsTuple *)EG_reall(tuples, sizeof(capsTuple)*(numDataEntry+1));
            if (tuples == NULL) { status = EGADS_MALLOC; goto cleanup; }

            tuples[numDataEntry].name  = (char*)EG_alloc((strlen(token)+1)*sizeof(char));
            tuples[numDataEntry].value = (char*)EG_alloc((strlen("["  )+1)*sizeof(char));
            if (tuples[numDataEntry].name  == NULL) { status = EGADS_MALLOC; goto cleanup; }
            if (tuples[numDataEntry].value == NULL) { status = EGADS_MALLOC; goto cleanup; }

            // restore the name with a space in it
            str = strstr(token, "c_cl");
            if (str != NULL){
              str[1] = ' ';
            }

            // copy over the header and start the list
            strcpy(tuples[numDataEntry].name, token);
            strcpy(tuples[numDataEntry].value, "[");
            numDataEntry++;
        }

        // read in the data columns
        while ((status = getline(&line, &linecap, fp)) >= 0) {

          str = strstr(line, "--------------");
          if (str != NULL) break; // end of the file

          // remove line feed characters
          while ((str = strstr(line, "\n")) != NULL) str[0] = ' ';
          while ((str = strstr(line, "\r")) != NULL) str[0] = ' ';

          rest = line;
          // skip the "j" column
          token = strtok_r(rest, " ", &rest);

          if (token == NULL) break; // end of data for this surface

          i = 0;
          while ((token = strtok_r(rest, " ", &rest))) {
              vallen = strlen(tuples[i].value);
              tuples[i].value = (char*)EG_reall(tuples[i].value, (vallen+strlen(token)+2)*sizeof(char));
              if (tuples[i].value  == NULL) { status = EGADS_MALLOC; goto cleanup; }

              // append the values to the list
              sprintf(tuples[i].value + vallen,"%s,", token );
              i++;
          }
        }
        if (status < 0){
            status = CAPS_IOERR;
            goto cleanup;
        }

        for (i = 0; i < numDataEntry; i++) {

            // close the lists by replacing the ',' with ']'
            tuples[i].value[strlen(tuples[i].value)-1] = ']';

            // collapse down the tuples for the surface
            vallen = strlen(surfaces[numSurfaces-1].value);
                // \" + name                 + \": + value                 + ,\0
            alen = 1 + strlen(tuples[i].name) + 2 + strlen(tuples[i].value) + 2;
            surfaces[numSurfaces-1].value = (char *)EG_reall(surfaces[numSurfaces-1].value, sizeof(char)*(vallen+alen));
            if (surfaces[numSurfaces-1].value == NULL) { status = EGADS_MALLOC; goto cleanup; }

            value = surfaces[numSurfaces-1].value + vallen;
            sprintf(value,"\"%s\":%s,", tuples[i].name, tuples[i].value );

            // release the memory now that it's been consumed
            EG_free(tuples[i].name ); tuples[i].name  = NULL;
            EG_free(tuples[i].value); tuples[i].value = NULL;
        }
        EG_free(tuples); tuples = NULL;
        numDataEntry = 0;

        // close the JSON dict by replacing the ',' with '}'
        surfaces[numSurfaces-1].value[strlen(surfaces[numSurfaces-1].value)-1] = '}';
    }

    *surfaces_out = surfaces;
    *length = numSurfaces;
    status = CAPS_SUCCESS;

cleanup:

    // Restore the path we came in with
    chdir(currentPath);
    if (fp != NULL) fclose(fp);

    if (status != CAPS_SUCCESS) {
        for (i = 0; i < numSurfaces; i++) {
            EG_free(surfaces[i].name);
            EG_free(surfaces[i].value);
        }
        EG_free(surfaces);
    }

    for (i = 0; i < numDataEntry; i++) {
        EG_free(tuples[i].name);
        EG_free(tuples[i].value);
    }
    EG_free(tuples);

    EG_free(line);

    return status;
}


static int read_EigenValues(const char *analysisPath, int *length, capsTuple **eigen_out) {

    int status = CAPS_SUCCESS;
    int i; //Indexing

    size_t     linecap = 0, vallen = 0;
    char       currentPath[PATH_MAX], *str = NULL, *line = NULL, *rest = NULL, *token = NULL;
    char       caseName[30];
    capsTuple  *eigen = NULL;
    int        numCase = 0, icase = 0;
    FILE       *fp = NULL;
    char eigenValueFile[] = "capsEigenValues.txt";

    if (analysisPath == NULL) return CAPS_NULLVALUE;

    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) return CAPS_DIRERR;

    // ignore if file does not exist
    if (file_exist(eigenValueFile) == (int) false) {
        chdir(currentPath);
        return CAPS_SUCCESS;
    }

    // Open the eigen value output file
    fp = fopen(eigenValueFile, "r");
    if (fp == NULL) {
        chdir(currentPath);
        return CAPS_DIRERR;
    }

    // Scan for header
    while ((status = getline(&line, &linecap, fp)) >= 0) {
        if (line == NULL) continue;
        str = strstr(line, "#   Run case");
        if (str != NULL) break;
    }
    if (status < 0 || str == NULL) {
      // reached end of file without finding any eigenvalues
      status = CAPS_SUCCESS;
      goto cleanup;
    }

    icase = 0;
    while ((status = getline(&line, &linecap, fp)) >= 0) {

        // remove line feed charachters
        while ((str = strstr(line, "\n")) != NULL) str[0] = ' ';
        while ((str = strstr(line, "\r")) != NULL) str[0] = ' ';

        rest = line;
        token = strtok_r(rest, " ", &rest); // get the case number

        icase = atoi(token);
        if (icase > numCase) {
            // create a new case to save off the eigen informaiton
            numCase = icase;
            eigen = (capsTuple *)EG_reall(eigen, sizeof(capsTuple)*(numCase));
            if (eigen == NULL) { status = EGADS_MALLOC; goto cleanup; }

            sprintf(caseName,"case %d", icase );
            eigen[icase-1].name  = EG_strdup(caseName);
            eigen[icase-1].value = EG_strdup("[");
            if (eigen[icase-1].name  == NULL) { status = EGADS_MALLOC; goto cleanup; }
            if (eigen[icase-1].value == NULL) { status = EGADS_MALLOC; goto cleanup; }
        }

        for (i = 0; i < 2; i++) {
            token = strtok_r(rest, " ", &rest); // get the real/imaginary part of the eigen value

            vallen = strlen(eigen[icase-1].value);
            eigen[icase-1].value = (char*)EG_reall(eigen[icase-1].value, (vallen+strlen(token)+3)*sizeof(char));
            if (eigen[icase-1].value  == NULL) { status = EGADS_MALLOC; goto cleanup; }

            // append the values to the list
            if (i == 0)
                sprintf(eigen[icase-1].value + vallen,"[%s,", token );
            else
                sprintf(eigen[icase-1].value + vallen,"%s],", token );
        }
    }

    // close the array by replacing the ',' with ']'
    for (icase = 0; icase < numCase; icase++)
        eigen[icase].value[strlen(eigen[icase].value)-1] = ']';

    *eigen_out = eigen;
    *length = numCase;
    status = CAPS_SUCCESS;

cleanup:

    // Restore the path we came in with
    chdir(currentPath);
    if (fp != NULL) fclose(fp);

    if (status != CAPS_SUCCESS) {
        for (i = 0; i < numCase; i++) {
            EG_free(eigen[i].name);
            EG_free(eigen[i].value);
        }
        EG_free(eigen);
    }

    EG_free(line);

    return status;
}

static int get_controlDeriv(void *aimInfo, char *analysisPath, int controlIndex, int outputIndex, double *data) {

    int status; // Function return status
    char *fileToOpen;

    char *coeff = NULL;
    char key[10];

    // Stability axis
    if        (outputIndex == aim_getIndex(aimInfo, "CLtot", ANALYSISOUT)) {
        fileToOpen = "capsStatbilityDeriv.txt";
        coeff = "CL";

    } else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT)) {
        fileToOpen = "capsStatbilityDeriv.txt";
        coeff = "CY";

    } else if (outputIndex == aim_getIndex(aimInfo, "Cl'tot", ANALYSISOUT)) {
        fileToOpen = "capsStatbilityDeriv.txt";
        coeff = "Cl";

    } else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT)) {
        fileToOpen = "capsStatbilityDeriv.txt";
        coeff = "Cm";

    } else if (outputIndex == aim_getIndex(aimInfo, "Cn'tot", ANALYSISOUT)) {
        fileToOpen = "capsStatbilityDeriv.txt";
        coeff = "Cn";

    // Body axis
    } else if (outputIndex == aim_getIndex(aimInfo, "CXtot", ANALYSISOUT)) {
        fileToOpen = "capsBodyAxisDeriv.txt";
        coeff = "CX";

    } else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT)) {
        fileToOpen = "capsBodyAxisDeriv.txt";
        coeff = "CY";

    } else if (outputIndex == aim_getIndex(aimInfo, "CZtot", ANALYSISOUT)) {
        fileToOpen = "capsBodyAxisDeriv.txt";
        coeff = "CZ";

    } else if (outputIndex == aim_getIndex(aimInfo, "Cltot", ANALYSISOUT)) {
        fileToOpen = "capsBodyAxisDeriv.txt";
        coeff = "Cl";

    } else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT)) {
        fileToOpen = "capsBodyAxisDeriv.txt";
        coeff = "Cm";

    } else if (outputIndex == aim_getIndex(aimInfo, "Cntot", ANALYSISOUT)) {
        fileToOpen = "capsBodyAxisDeriv.txt";
        coeff = "Cn";

    } else {
        printf("Unrecognized output variable for control derivatives!\n");
        status = CAPS_MISMATCH;
        goto cleanup;
    }

    sprintf(key, "%sd%d =", coeff, controlIndex);

    status = read_Data(fileToOpen, analysisPath, key, data);
    if (status != CAPS_SUCCESS) return status;

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        return status;
}

static int parse_controlName(int iIndex, char string[], int *controlNumber) {

    int status; // Function return status

    char control[] = "AVL_Control";

    char *controlName = NULL;

    *controlNumber = CAPSMAGIC;

    //printf("String = %s\n", string);

    if (strncasecmp(string, control, strlen(control)) != 0 ) {
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    controlName = strstr(string, ":");
    if (controlName == NULL) {
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // Increment pointer to remove ':'
    ++controlName;

    //printf("Temp = %s\n", temp);

    // Loop through and determine which control integer this name corresponds to?
    status = get_mapAttrToIndexIndex(&avlInstance[iIndex].controlMap,
            (const char *) controlName, controlNumber);
    if (status != CAPS_SUCCESS) goto cleanup;

    /*
    printf("ControlName = %s\n", controlName);
    printf("ControlIndex = %d\n", *controlNumber);
     */

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        return status;
}

/* ********************** Exposed AIM Functions ***************************** */

int aimInitialize(/*@unused@*/ int ngIn, /*@unused@*/ /*@null@*/ capsValue *gIn,
                  int *qeFlag, /*@null@*/ const char *unitSys, int *nIn,
                  int *nOut, int *nFields, char ***fnames, int **ranks)
{
    int flag;

    aimStorage *tmp;

#ifdef DEBUG
    printf("\n avlAIM/aimInitialize   ngIn = %d!\n", ngIn);
#endif

    flag    = *qeFlag;
    *qeFlag = 0;

    /* specify the number of analysis input and out "parameters" */
    *nIn     = NUMINPUT;
    *nOut    = NUMOUT;
    if (flag == 1) return CAPS_SUCCESS;

    /*! \page geomRepIntentAVL Geometry Representation and Analysis Intent
     * The geometric representation for the AVL AIM requires that "body(ies)" [or cross-sections], be a face
     * body(ies) (FACEBODY) with the attribute <b> capsAIM</b> include the string <b>avlAIM</b>.
     */

    // Specify the field variables this analysis can generate
    *nFields = 0;
    *ranks   = NULL;
    *fnames  = NULL;

    // Allocate avlInstance
    if (avlInstance == NULL) {
        avlInstance = (aimStorage *) EG_alloc(sizeof(aimStorage));
        if (avlInstance == NULL) return EGADS_MALLOC;
    } else {
        tmp = (aimStorage *) EG_reall(avlInstance, (numInstance+1)*sizeof(aimStorage));
        if (tmp == NULL) return EGADS_MALLOC;
        avlInstance = tmp;
    }

    // Analysis file path/directory
    avlInstance[numInstance].analysisPath = NULL;

    // Initiate control map
    (void) initiate_mapAttrToIndexStruct(&avlInstance[numInstance].controlMap);

    // Increment number of instances
    numInstance += 1;

    return (numInstance -1);
}


int aimInputs(/*@unused@*/ int inst, /*@unused@*/ void *aimInfo, int index,
              char **ainame, capsValue *defval)
{
    /*! \page aimInputsAVL AIM Inputs
     * The following list outlines the AVL inputs along with their default value available
     * through the AIM interface.
     */

#ifdef DEBUG
    printf(" avlAIM/aimInputs instance = %d  index = %d!\n", inst, index);
#endif

    if (index == 1) {
        *ainame           = EG_strdup("Mach");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsAVL
         * - <B> Mach = 0.0 </B> <br>
         *  Mach number.
         */

    } else if (index == 2) {
        *ainame           = EG_strdup("Alpha");
        defval->type      = Double;
        defval->dim       = Scalar;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->vals.real = 0.0;
        defval->nullVal   = IsNull;
        defval->lfixed    = Change;
        defval->sfixed    = Change;
        defval->units     = EG_strdup("degree");

        /*! \page aimInputsAVL
         * - <B> Alpha = NULL </B> <br>
         *  Angle of attack [degree]. Either CL or Alpha must be defined but not both.
         */

    } else if (index == 3) {
        *ainame           = EG_strdup("Beta");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = EG_strdup("degree");

        /*! \page aimInputsAVL
         * - <B> Beta = 0.0 </B> <br>
         *  Sideslip angle [degree].
         */

    } else if (index == 4) {
        *ainame           = EG_strdup("RollRate");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsAVL
         * - <B> RollRate = 0.0 </B> <br>
         *  Non-dimensional roll rate.
         */

    } else if (index == 5) {
        *ainame           = EG_strdup("PitchRate");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsAVL
         * - <B> PitchRate = 0.0 </B> <br>
         *  Non-dimensional pitch rate.
         */

    } else if (index == 6) {
        *ainame           = EG_strdup("YawRate");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsAVL
         * - <B> YawRate = 0.0 </B> <br>
         *  Non-dimensional yaw rate.
         */

    } else if (index == 7) {
        *ainame           = EG_strdup("CDp");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = NULL;

        /*! \page aimInputsAVL
         * - <B> CDp = 0.0 </B> <br>
         *  A fixed value of profile drag to be added to all simulations.
         */

    } else if (index == 8) {
        *ainame              = EG_strdup("AVL_Surface");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsAVL
         * - <B>AVL_Surface = NULL </B> <br>
         * See \ref vlmSurface for additional details.
         */

    } else if (index == 9) {
        *ainame              = EG_strdup("AVL_Control");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsAVL
         * - <B>AVL_Control = NULL </B> <br>
         * See \ref vlmControl for additional details.
         */

    } else if (index == 10) {
        *ainame           = EG_strdup("CL");
        defval->type      = Double;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->vals.real = 0.0;
        defval->nullVal    = IsNull;
        defval->lfixed    = Change;

        /*! \page aimInputsAVL
         * - <B> CL = NULL </B> <br>
         *  Coefficient of Lift.  AVL will solve for Angle of Attack.  Either CL or Alpha must be defined but not both.
         */

    } else if (index == 11) {
        *ainame              = EG_strdup("Moment_Center");
        defval->type          = Double;
        defval->dim           = Vector;
        defval->length        = 3;
        defval->nrow          = 3;
        defval->ncol          = 1;
        defval->units         = NULL;
        defval->vals.reals    = (double *) EG_alloc(defval->length*sizeof(double));
        if (defval->vals.reals == NULL) {
            return EGADS_MALLOC;
        } else {
            defval->vals.reals[0] = 0.0;
            defval->vals.reals[1] = 0.0;
            defval->vals.reals[2] = 0.0;
        }
        defval->nullVal       = IsNull;
        defval->lfixed        = Fixed;
        defval->sfixed        = Fixed;

        /*! \page aimInputsAVL
         * - <B>Moment_Center = NULL, [0.0, 0.0, 0.0]</B> <br>
         * Array values correspond to the Xref, Yref, and Zref variables.
         * Alternatively, the geometry (body) attributes "capsReferenceX", "capsReferenceY",
         * and "capsReferenceZ" may be used to specify the X-, Y-, and Z- reference centers, respectively
         * (note: values set through the AIM input will supersede the attribution values).
         */

    } else if (index == 12) {
        *ainame           = EG_strdup("Lunit");
        defval->type      = Double;
        defval->vals.real = 1.0;
        defval->units     = EG_strdup("m");
        defval->nullVal   = IsNull;

        /*! \page aimInputsAVL
         * - <B> Lunit = 1 m </B> <br>
         *  Reference length of the configuration for Eigen value analysis.
         *  The aircraft is scaled by this quantity
         */

    } else if (index == 13) {
        *ainame           = EG_strdup("Munit");
        defval->type      = Double;
        defval->vals.real = 1.0;
        defval->units     = EG_strdup("kg");

        /*! \page aimInputsAVL
         * - <B> Munit = 1 kg </B> <br>
         *  Reference mass of the configuration for Eigen value analysis. Units must be specified.
         *  Values in the MassProp are scaled by this quantity.
         */

    } else if (index == 14) {
        *ainame           = EG_strdup("Tunit");
        defval->type      = Double;
        defval->vals.real = 1.0;
        defval->units     = EG_strdup("s");

        /*! \page aimInputsAVL
         * - <B> Tunit = 1 s </B> <br>
         *  Time units for Eigen value analysis.
         */

    } else if (index == 15) {
        *ainame              = EG_strdup("MassProp");
        defval->type         = Tuple;
        defval->nullVal      = IsNull;
        //defval->units        = NULL;
        defval->dim          = Vector;
        defval->lfixed       = Change;
        defval->vals.tuple   = NULL;

        /*! \page aimInputsAVL
         * - <B>MassProp = NULL</B> <br>
         * Mass properties used for eigen value analysis
         * Structure for the mass property tuple = ("Name", "Value").
         * The "Name" of the mass component used for documenting the xxx.mass file.
         * The value is a JSON dictionary with values with unit pairs for mass, CG, and moments of inertia information
         * (e.g. "Value" = {"mass" : [mass,"kg"], "CG" : [[x,y,z],"m"], "massInertia" : [[Ixx, Iyy, Izz, Ixy, Ixz, Iyz], "kg*m2"]})
         * The components Ixy, Ixz, and Iyz are optional may be omitted.
         */

    } else if (index == 16) {
        *ainame           = EG_strdup("Gravity");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = EG_strdup("m/s^2");
        defval->nullVal   = IsNull;

        /*! \page aimInputsAVL
         * - <B> Gravity = NULL </B> <br>
         *  Magnitude of the gravitational force used for Eigen value analysis.
         */

    } else if (index == 17) {
        *ainame           = EG_strdup("Density");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = EG_strdup("kg/m^3");
        defval->nullVal   = IsNull;

        /*! \page aimInputsAVL
         * - <B> Density = NULL </B> <br>
         *  Air density used for Eigen value analysis.
         */

    } else if (index == 18) {
        *ainame           = EG_strdup("Velocity");
        defval->type      = Double;
        defval->vals.real = 0.0;
        defval->units     = EG_strdup("m/s");
        defval->nullVal   = IsNull;

        /*! \page aimInputsAVL
         * - <B> Velocity = NULL </B> <br>
         *  Velocity used for Eigen value analysis.
         */
    }

    /*else if (index == 11) {
     *ainame           = EG_strdup("Get_Stability");
        defval->type      = Boolean;
        defval->dim       = Vector;
        defval->length    = 1;
        defval->nrow      = 1;
        defval->ncol      = 1;
        defval->units     = NULL;
        defval->vals.integer = (int) false;
        defval->nullVal    = NotNull;
        defval->lfixed    = Fixed;

        ! \page aimInputsAVL
     * - <B> Get_Stability = False </B> <br>
     *  Execute the calculation of stability derivatives.

    }*/

#if NUMINPUT != 18
#error "NUMINPUTS is inconsistent with the list of inputs"
#endif

    return CAPS_SUCCESS;
}


int aimPreAnalysis(int iIndex, /*@unused@*/ void *aimInfo,
                   const char *analysisPath, /*@null@*/ capsValue *aimInputs, capsErrs **errs)
{
    int status; // Function return status

    int i, j, k, surf, section, control; // Indexing

    int numBody;

    int found = (int) false; // Boolean tester
    int eigenValues = (int) false;

    int atype, alen;

    char currentPath[PATH_MAX] = "None";

    double      Sref, Cref, Bref, Xref, Yref, Zref;
    const int    *ints;
    const char   *string, *intents;
    const double *reals;
    ego          *bodies;

    const char *lengthUnitsIn = NULL;

    // Eigen value quantities
    double Lunit, Tunit;
    char *Lunits = NULL, *Tunits = NULL;
    double velocity;
    char *Vunits = NULL;

    int stringLength = 0; // String manipulation

    // File I/O
    FILE *fp = NULL;
    char inputFilename[] = "avlInput.txt";
    char avlFilename[] = "caps.avl";
    char massFilename[] = "caps.mass";

    char totalForceFile[] = "capsTotalForce.txt";
    char stripForceFile[] = "capsStripForce.txt";
    char stabilityFile[] = "capsStatbilityDeriv.txt";
    char bodyAxisFile[] = "capsBodyAxisDeriv.txt";
    char hingeMomentFile[] = "capsHingeMoment.txt";
    char eigenValueFile[] = "capsEigenValues.txt";

    // AVL surface information
    int numAVLSurface = 0;
    vlmSurfaceStruct *avlSurface = NULL;
    int numSpanWise = 0;

    // AVL control surface information
    int numAVLControl = 0;
    vlmControlStruct *avlControl = NULL;

    int numControlName = 0; // Unique control names
    char **controlName = NULL;

    mapAttrToIndexStruct attrMap; // Attribute to index map

    // Control related variables
    //double xyzHingeMag, xyzHingeVec[3];


#ifdef DEBUG
    printf(" avlAIM/aimPreAnalysis instance = %d\n", inst);
#endif

    // Initialize reference values
    Sref = 1.0;
    Cref = 1.0;
    Bref = 1.0;

    Xref = 0.0;
    Yref = 0.0;
    Zref = 0.0;

    // NULL out errs
    *errs = NULL;

    // Save a copy of analysisPath
    avlInstance[iIndex].analysisPath = (char *) analysisPath;

    if (aimInputs == NULL) {
#ifdef DEBUG
        printf(" avlAIM/aimPreAnalysis aimInputs == NULL!\n");
#endif

        return CAPS_NULLVALUE;
    }

    status = aim_getBodies(aimInfo, &intents, &numBody, &bodies);
    if (status != CAPS_SUCCESS) {
#ifdef DEBUG
        printf(" avlAIM/aimPreAnalysis getBodies = %d!\n", status);
#endif
        return status;
    }

    if (numBody == 0 || bodies == NULL) {
        printf(" avlAIM/aimPreAnalysis No Bodies!\n");
        return CAPS_SOURCEERR;
    }

    // Destroy previous controlMap (in case it already exists)
    status = destroy_mapAttrToIndexStruct(&avlInstance[iIndex].controlMap);
    if (status != CAPS_SUCCESS) return status;

    // Container for attribute to index map
    status = initiate_mapAttrToIndexStruct(&attrMap);
    if (status != CAPS_SUCCESS) return status;

    // Get capsGroup name and index mapping to make sure all bodies have a capsGroup value
    status = create_CAPSGroupAttrToIndexMap(numBody,
                                            bodies,
                                            0, // Only search down to the body level of the EGADS body
                                            &attrMap);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get AVL surface information
    if (aimInputs[aim_getIndex(aimInfo, "AVL_Surface", ANALYSISIN)-1].nullVal == NotNull) {

        status = get_vlmSurface(aimInputs[aim_getIndex(aimInfo, "AVL_Surface", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "AVL_Surface", ANALYSISIN)-1].vals.tuple,
                                &attrMap,
                                1.0, // default Cspace
                                &numAVLSurface,
                                &avlSurface);

    } else {
        printf("No AVL_SURFACE tuple specified\n");
        status = CAPS_NOTFOUND;
    }
    if (status != CAPS_SUCCESS) goto cleanup;

    // Get AVL control surface information
    if (aimInputs[aim_getIndex(aimInfo, "AVL_Control", ANALYSISIN)-1].nullVal == NotNull) {

        status = get_vlmControl(aimInputs[aim_getIndex(aimInfo, "AVL_Control", ANALYSISIN)-1].length,
                                aimInputs[aim_getIndex(aimInfo, "AVL_Control", ANALYSISIN)-1].vals.tuple,
                                &numAVLControl,
                                &avlControl);

        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Accumulate section data
    status = vlm_getSections(numBody, bodies, NULL, attrMap, vlmGENERIC, numAVLSurface, &avlSurface);
    if (status != CAPS_SUCCESS) goto cleanup;

    // Loop through surfaces and transfer control surface data to sections
    for (surf = 0; surf < numAVLSurface; surf++) {

        status = get_ControlSurface(bodies,
                                    numAVLControl,
                                    avlControl,
                                    &avlSurface[surf]);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    // Compute auto spacing
    for (surf = 0; surf < numAVLSurface; surf++) {

        if (avlSurface[surf].NspanTotal > 0)
            numSpanWise = avlSurface[surf].NspanTotal;
        else if (avlSurface[surf].NspanSection > 0)
            numSpanWise = (avlSurface[surf].numSection-1)*avlSurface[surf].NspanSection;
        else {
            printf("Error: Only one of numSpanTotal and numSpanPerSection can be non-zero!\n");
            printf("       numSpanTotal      = %d\n", avlSurface[surf].NspanTotal);
            printf("       numSpanPerSection = %d\n", avlSurface[surf].NspanSection);
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        status = vlm_autoSpaceSpanPanels(numSpanWise, avlSurface[surf].numSection, avlSurface[surf].vlmSection);
        if (status != CAPS_SUCCESS) goto cleanup;
    }

    /*
    // Determine the hinge line for any flaps
    for (surf = 0; surf < numAVLSurface; surf++) {

        for (section = 0; section < avlSurface[surf].numSection; section++) {

            for (control = 0; control < avlSurface[surf].vlmSection[section].numControl; control++) {

                for (i = 0; i < avlSurface[surf].numSection; i++) {

                    if (i == section) continue;

                    for (j = 0; j < avlSurface[surf].vlmSection[i].numControl; j++) {

                        if( strcmp(avlSurface[surf].vlmSection[section].vlmControl[control].name,
                                   avlSurface[surf].vlmSection[i].vlmControl[j].name) == 0) {

                            xyzHingeVec[0] = avlSurface[surf].vlmSection[section].vlmControl[control].xyzHinge[0] -
                                             avlSurface[surf].vlmSection[i].vlmControl[j].xyzHinge[0];

                            xyzHingeVec[1] = avlSurface[surf].vlmSection[section].vlmControl[control].xyzHinge[1] -
                                             avlSurface[surf].vlmSection[i].vlmControl[j].xyzHinge[1];

                            xyzHingeVec[2] = avlSurface[surf].vlmSection[section].vlmControl[control].xyzHinge[2] -
                                             avlSurface[surf].vlmSection[i].vlmControl[j].xyzHinge[2];

                            xyzHingeMag = sqrt(pow(xyzHingeVec[0], 2) +
                                               pow(xyzHingeVec[1], 2) +
                                               pow(xyzHingeVec[2], 2));

                            avlSurface[surf].vlmSection[section].vlmControl[control].xyzHingeVec[0] = xyzHingeVec[0]/xyzHingeMag;
                            avlSurface[surf].vlmSection[section].vlmControl[control].xyzHingeVec[1] = xyzHingeVec[1]/xyzHingeMag;
                            avlSurface[surf].vlmSection[section].vlmControl[control].xyzHingeVec[2] = xyzHingeVec[2]/xyzHingeMag;

                        }
                    }
                }
            }
        }
    }
     */

    // look for eigen value analysis
    if ( (aimInputs[aim_getIndex(aimInfo, "MassProp", ANALYSISIN)-1].nullVal == NotNull) ||
         (aimInputs[aim_getIndex(aimInfo, "Gravity" , ANALYSISIN)-1].nullVal == NotNull) ||
         (aimInputs[aim_getIndex(aimInfo, "Density" , ANALYSISIN)-1].nullVal == NotNull) ||
         (aimInputs[aim_getIndex(aimInfo, "Velocity", ANALYSISIN)-1].nullVal == NotNull) ) {

        // Get length units
        status = check_CAPSLength(numBody, bodies, &lengthUnitsIn);
        if (status != CAPS_SUCCESS) {
          printf("***********************************************************************************\n");
          printf(" *** ERROR: masstranAIM: No units assigned *** capsLength is not set in *.csm file!\n");
          printf("***********************************************************************************\n");
          status = CAPS_BADVALUE;
          goto cleanup;
        }

        if ( !((aimInputs[aim_getIndex(aimInfo, "MassProp", ANALYSISIN)-1].nullVal == NotNull) &&
               (aimInputs[aim_getIndex(aimInfo, "Gravity" , ANALYSISIN)-1].nullVal == NotNull) &&
               (aimInputs[aim_getIndex(aimInfo, "Density" , ANALYSISIN)-1].nullVal == NotNull) &&
               (aimInputs[aim_getIndex(aimInfo, "Velocity", ANALYSISIN)-1].nullVal == NotNull)) ) {
            printf("******************************************************************************\n");
            printf(" All inputs 'MassProp', 'Gravity', 'Density', and 'Velocity'\n");
            printf(" must be set for AVL eigen value analysis.\n");
            printf(" Missing values for:\n");
            if (aimInputs[aim_getIndex(aimInfo, "MassProp", ANALYSISIN)-1].nullVal == IsNull) printf("    MassProp\n");
            if (aimInputs[aim_getIndex(aimInfo, "Gravity" , ANALYSISIN)-1].nullVal == IsNull) printf("    Gravity\n");
            if (aimInputs[aim_getIndex(aimInfo, "Density" , ANALYSISIN)-1].nullVal == IsNull) printf("    Density\n");
            if (aimInputs[aim_getIndex(aimInfo, "Velocity", ANALYSISIN)-1].nullVal == IsNull) printf("    Velocity\n");
            printf("******************************************************************************\n");
            status = CAPS_BADVALUE;
            goto cleanup;
        }

        eigenValues = (int) true; // compute eigen values
    }



    // Get where we are and set the path to our input
    (void) getcwd(currentPath, PATH_MAX);

    if (chdir(analysisPath) != 0) {
        status = CAPS_DIRERR;
        goto cleanup;
    }

    // Open and write the input to control the AVL session
    fp = fopen(inputFilename, "w");
    if (fp == NULL) {
        printf("Unable to open file %s\n!", inputFilename);

        status = CAPS_IOERR;
        goto cleanup;
    }

    // Print the session file for the AVL run

    fprintf(fp, "PLOP\n"); // Start with disabling graphics
    fprintf(fp, "G\n");    // toggle graphics
    fprintf(fp, "\n");     // main menu

    if (eigenValues == (int)true) {
        // apply the mass properties to move the CG
        // WARNING: mset will move the CG to (0,0,0) if no xxx.mass file is present
        fprintf(fp, "mset 0\n");
    }

    // Set operation parameters
    fprintf(fp, "OPER\n");

    if (aimInputs[aim_getIndex(aimInfo, "Alpha", ANALYSISIN)-1].nullVal ==  NotNull) {
        fprintf(fp, "A A ");//Alpha
        fprintf(fp, "%lf\n", aimInputs[1].vals.real);
    }

    if (aimInputs[aim_getIndex(aimInfo, "CL", ANALYSISIN)-1].nullVal ==  NotNull) {
        fprintf(fp, "A C ");//CL
        fprintf(fp, "%lf\n", aimInputs[9].vals.real);
    }

    fprintf(fp, "B B ");//Beta
    fprintf(fp, "%lf\n", aimInputs[aim_getIndex(aimInfo, "Beta", ANALYSISIN)-1].vals.real);

    fprintf(fp, "R R ");//Roll Rate pb/2V
    fprintf(fp, "%lf\n", aimInputs[aim_getIndex(aimInfo, "RollRate", ANALYSISIN)-1].vals.real);

    fprintf(fp, "P P ");//Pitch Rate qc/2v
    fprintf(fp, "%lf\n", aimInputs[aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)-1].vals.real);

    fprintf(fp, "Y Y ");//Yaw Rate rb/2v
    fprintf(fp, "%lf\n", aimInputs[aim_getIndex(aimInfo, "YawRate", ANALYSISIN)-1].vals.real);

    // Check for control surface information
    j = 1;
    numControlName = 0;
    for (surf = 0; surf < numAVLSurface; surf++) {

        for (i = 0; i < avlSurface[surf].numSection; i++) {

            section = avlSurface[surf].vlmSection[i].sectionIndex;

            for (control = 0; control < avlSurface[surf].vlmSection[section].numControl; control++) {

                // Check to see if control surface hasn't already been written
                found = (int) false;

                if (numControlName == 0) {

                    numControlName += 1;

                    controlName = (char **) EG_alloc(numControlName * sizeof(char *));
                    if (controlName == NULL) {
                        numControlName -= 1;
                        status = EGADS_MALLOC;
                        goto cleanup;
                    }

                    found = (int) false;

                } else {

                    for (k = 0; k < numControlName; k++) {
                        if (strcmp(controlName[k],
                                avlSurface[surf].vlmSection[section].vlmControl[control].name) == 0) {

                            found = (int) true;
                            break;
                        }
                    }

                    if (found == (int) true) continue;

                    numControlName += 1;

                    controlName = (char **) EG_reall(controlName, numControlName * sizeof(char *));
                }

                stringLength = strlen(avlSurface[surf].vlmSection[section].vlmControl[control].name);

                controlName[numControlName -1] = (char *) EG_alloc((stringLength+1) * sizeof(char));

                if (controlName[numControlName -1] == NULL) {
                    status = EGADS_MALLOC;
                    goto cleanup;
                }

                strcpy(controlName[numControlName -1],
                        avlSurface[surf].vlmSection[section].vlmControl[control].name);

                // Store control map for later use
                status = increment_mapAttrToIndexStruct(&avlInstance[iIndex].controlMap,
                        (const char *) controlName[numControlName-1]);
                if (status != CAPS_SUCCESS) goto cleanup;

                fprintf(fp, "D%d D%d %f\n",j, j, avlSurface[surf].vlmSection[section].vlmControl[control].deflectionAngle);
                j += 1;
            }
        }
    }

    /*if(aimInputs[7].nullVal != IsNull) {
        // write control surface settings
        if (aimInputs[7].length == 1){
            fprintf(fp, "D1 D1 ");//Pitch Rate qc/2v
            fprintf(fp, "%lf\n", aimInputs[7].vals.real);
        } else {
            for (i=0; i < aimInputs[7].length; i++) {
                fprintf(fp,"D%d D%d %lf\n",i+1,i+1,aimInputs[7].vals.reals[i]);
            }
        }
    }*/

    fprintf(fp, "M\n"); // Modify parameters
    fprintf(fp, "MN\n");//Mach
    fprintf(fp, "%lf\n", aimInputs[aim_getIndex(aimInfo, "Mach", ANALYSISIN)-1].vals.real);

    if (aimInputs[aim_getIndex(aimInfo, "Velocity", ANALYSISIN)-1].nullVal == NotNull) {

        Lunit  = aimInputs[aim_getIndex(aimInfo, "Lunit", ANALYSISIN)-1].vals.real;
        Lunits = aimInputs[aim_getIndex(aimInfo, "Lunit", ANALYSISIN)-1].units;

        Tunit  = aimInputs[aim_getIndex(aimInfo, "Tunit", ANALYSISIN)-1].vals.real;
        Tunits = aimInputs[aim_getIndex(aimInfo, "Tunit", ANALYSISIN)-1].units;

        status = aim_unitDivide(aimInfo, Lunits, Tunits, &Vunits ); // length/time, e.g speed
        if (status != CAPS_SUCCESS) goto cleanup;

        status = aim_convert(aimInfo, aimInputs[aim_getIndex(aimInfo, "Velocity", ANALYSISIN)-1].units,
                                      aimInputs[aim_getIndex(aimInfo, "Velocity", ANALYSISIN)-1].vals.real,
                                      Vunits, &velocity);
        if (status != CAPS_SUCCESS) goto cleanup;
        EG_free(Vunits); Vunits = NULL;

        fprintf(fp, "V\n");                            // Velocity
        fprintf(fp, "%lf\n", velocity/(Lunit/Tunit));  // set the value
    }
    fprintf(fp, "\n");                    // exit modify parameters

    fprintf(fp, "X\n"); // Execute the calculation

    fprintf(fp, "S\n\n"); // save caps.run file
    if (file_exist("caps.run") == (int) true) {
        fprintf(fp, "y\n");
    }

    // Get Total forces
    fprintf(fp,"FT\n");
    fprintf(fp,"%s\n", totalForceFile);
    if (file_exist(totalForceFile) == (int) true) {
        fprintf(fp, "O\n");
    }

    // Get strip forces
    fprintf(fp,"FS\n");
    fprintf(fp,"%s\n", stripForceFile);
    if (file_exist(stripForceFile) == (int) true) {
        fprintf(fp, "O\n");
    }

    // Get stability derivatives
    fprintf(fp,"ST\n");
    fprintf(fp,"%s\n", stabilityFile);
    if (file_exist(stabilityFile) == (int) true) {
        fprintf(fp, "O\n");
    }

    // Get stability (body axis) derivatives
    fprintf(fp,"SB\n");
    fprintf(fp,"%s\n", bodyAxisFile);
    if (file_exist(bodyAxisFile) == (int) true) {
        fprintf(fp, "O\n");
    }

    // Get hinge moments
    fprintf(fp,"HM\n");
    fprintf(fp,"%s\n", hingeMomentFile);
    if (file_exist(hingeMomentFile) == (int) true) {
        fprintf(fp, "O\n");
    }

    fprintf(fp, "\n"); // back to main menu

    if (eigenValues == (int)true) {
        fprintf(fp, "mode\n");                // enter eigen value analysis

        fprintf(fp, "n\n");                   // compute eigen values
        fprintf(fp, "w\n");                   // write eigen values to file
        fprintf(fp, "%s\n", eigenValueFile);
        if (file_exist(eigenValueFile) == (int) true) {
            fprintf(fp, "Y\n");
        }
        fprintf(fp, "\n"); // back to main menu
    }

    fprintf(fp, "Quit\n"); // Quit AVL
    fclose(fp);
    fp = NULL;

    /* open the file and write the avl data */
    fp = fopen(avlFilename, "w");
    if (fp == NULL) {
        printf("Unable to open file %s\n!", avlFilename);

        status =  CAPS_IOERR;
        goto cleanup;
    }

    // Loop over bodies and look for reference quantity attributes
    for (i=0; i < numBody; i++) {
        status = EG_attributeRet(bodies[i], "capsReferenceArea", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS) {

            if (atype == ATTRREAL && alen == 1) {
                Sref = (double) reals[0];
            } else {
                printf("capsReferenceArea should be followed by a single real value!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        status = EG_attributeRet(bodies[i], "capsReferenceChord", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS) {

            if (atype == ATTRREAL && alen == 1) {
                Cref = (double) reals[0];
            } else {
                printf("capsReferenceChord should be followed by a single real value!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        status = EG_attributeRet(bodies[i], "capsReferenceSpan", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS) {

            if (atype == ATTRREAL && alen == 1) {
                Bref = (double) reals[0];
            } else {
                printf("capsReferenceSpan should be followed by a single real value!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        status = EG_attributeRet(bodies[i], "capsReferenceX", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS) {

            if (atype == ATTRREAL && alen == 1) {
                Xref = (double) reals[0];
            } else {
                printf("capsReferenceX should be followed by a single real value!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        status = EG_attributeRet(bodies[i], "capsReferenceY", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS) {

            if (atype == ATTRREAL && alen == 1) {
                Yref = (double) reals[0];
            } else {
                printf("capsReferenceY should be followed by a single real value!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }

        status = EG_attributeRet(bodies[i], "capsReferenceZ", &atype, &alen, &ints, &reals, &string);
        if (status == EGADS_SUCCESS){

            if (atype == ATTRREAL && alen == 1) {
                Zref = (double) reals[0];
            } else {
                printf("capsReferenceZ should be followed by a single real value!\n");
                status = CAPS_BADVALUE;
                goto cleanup;
            }
        }
    }

    // Check for moment reference overwrites
    if (aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].nullVal ==  NotNull) {

        Xref =aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[0];
        Yref =aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[1];
        Zref =aimInputs[aim_getIndex(aimInfo, "Moment_Center", ANALYSISIN)-1].vals.reals[2];
    }

    fprintf(fp, "CAPS generated Configuration\n");
    fprintf(fp, "0.0         # Mach\n");                                                                 /* Mach */
    fprintf(fp, "0 0 0       # IYsym   IZsym   Zsym\n");                                                 /* IYsym   IZsym   Zsym */
    fprintf(fp, "%lf %lf %lf # Sref    Cref    Bref\n", Sref, Cref, Bref);                               /* Sref    Cref    Bref */
    fprintf(fp, "%lf %lf %lf # Xref    Yref    Zref\n", Xref, Yref, Zref);                               /* Xref    Yref    Zref */
    fprintf(fp, "%lf         # CDp\n", aimInputs[aim_getIndex(aimInfo, "CDp", ANALYSISIN)-1].vals.real); /* CDp */

    // Write out the Surfaces, one at a time
    for (surf = 0; surf < numAVLSurface; surf++) {

        printf("Writing surface - %s (ID = %d)\n", avlSurface[surf].name, surf);

        if (avlSurface[surf].numSection < 2) {
            printf("Surface %s only has %d Sections - it will be skipped!\n", avlSurface[surf].name,
                                                                              avlSurface[surf].numSection);
            continue;
        }

        fprintf(fp, "#\nSURFACE\n%s\n%d %lf\n\n", avlSurface[surf].name,
                                                  avlSurface[surf].Nchord,
                                                  avlSurface[surf].Cspace);

        if  (avlSurface[surf].compon != 0)
            fprintf(fp, "COMPONENT\n%d\n\n", avlSurface[surf].compon);

        if  (avlSurface[surf].iYdup  != 0)
            fprintf(fp, "YDUPLICATE\n0.0\n\n");

        if  (avlSurface[surf].nowake == (int) true) fprintf(fp, "NOWAKE\n");
        if  (avlSurface[surf].noalbe == (int) true) fprintf(fp, "NOALBE\n");
        if  (avlSurface[surf].noload == (int) true) fprintf(fp, "NOLOAD\n");

        if ( (avlSurface[surf].nowake == (int) true) ||
             (avlSurface[surf].noalbe == (int) true) ||
             (avlSurface[surf].noload == (int) true)) {

            fprintf(fp,"\n");
        }

        // Write the sections for each surface
        for (i = 0; i < avlSurface[surf].numSection; i++) {

            section = avlSurface[surf].vlmSection[i].sectionIndex;

            printf("\tSection %d of %d (ID = %d)\n", i+1, avlSurface[surf].numSection, section);

            // Write section data
            status = writeSection(fp, &avlSurface[surf].vlmSection[section]);
            if (status != CAPS_SUCCESS) goto cleanup;

            // Write control information for each section
            for (control = 0; control < avlSurface[surf].vlmSection[section].numControl; control++) {

                printf("\t  Control surface %d of %d \n", control + 1, avlSurface[surf].vlmSection[section].numControl);

                fprintf(fp, "CONTROL\n");
                fprintf(fp, "%s ", avlSurface[surf].vlmSection[section].vlmControl[control].name);

                fprintf(fp, "%f ", avlSurface[surf].vlmSection[section].vlmControl[control].controlGain);

                if (avlSurface[surf].vlmSection[section].vlmControl[control].leOrTe == 0) { // Leading edge (-)

                    fprintf(fp, "%f ", -avlSurface[surf].vlmSection[section].vlmControl[control].percentChord);

                } else { // Trailing edge (+)

                    fprintf(fp, "%f ", avlSurface[surf].vlmSection[section].vlmControl[control].percentChord);
                }

                fprintf(fp, "%f %f %f ", avlSurface[surf].vlmSection[section].vlmControl[control].xyzHingeVec[0],
                        avlSurface[surf].vlmSection[section].vlmControl[control].xyzHingeVec[1],
                        avlSurface[surf].vlmSection[section].vlmControl[control].xyzHingeVec[2]);

                fprintf(fp, "%f\n", (double) avlSurface[surf].vlmSection[section].vlmControl[control].deflectionDup);
            }
            fprintf(fp, "\n");
        }
    }

    // write mass data file for needed for eigen value analysis
    if (eigenValues == (int)true) {
      status = writeMassFile(aimInfo, aimInputs, lengthUnitsIn, massFilename);
      if (status != CAPS_SUCCESS) goto cleanup;
    }

    status = CAPS_SUCCESS;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in AVL preAnalysis() status = %d\n", status);

        EG_free(Vunits); Vunits = NULL;

        // Restore the path we came in with and get out
        if (strcmp(currentPath, "None") != 0) chdir(currentPath);

        if (fp != NULL) fclose(fp);

        // Free array of control name strings
        if (numControlName != 0 && controlName != NULL){
            (void) string_freeArray(numControlName, &controlName);
        }

        // Attribute to index map
        (void ) destroy_mapAttrToIndexStruct(&attrMap);

        // Destroy avlSurfaces
        if (avlSurface != NULL) {
            for (i = 0; i < numAVLSurface; i++) {
                (void) destroy_vlmSurfaceStruct(&avlSurface[i]);
            }
        }

        EG_free(avlSurface);
        numAVLSurface = 0;

        // Destroy avlControl
        if (avlControl != NULL) {
            for (i = 0; i < numAVLControl; i++) {
                (void) destroy_vlmControlStruct(&avlControl[i]);
            }
        }

        EG_free(avlControl);
        numAVLControl = 0;

        return status;
}


int aimOutputs(/*@unused@*/ int inst, /*@unused@*/ void *aimStruc, int index, char **aoname, capsValue *form)
{

    /*! \page aimOutputsAVL AIM Outputs
     * Optional outputs that echo the inputs.  These are parsed from the resulting output and can be used as a sanity check.
     */

#ifdef DEBUG
    printf(" avlAIM/aimOutputs instance = %d  index = %d!\n", inst, index);
#endif

    // Echo AVL flow conditions
    if (index == 1) {
        *aoname = EG_strdup("Alpha");

        /*! \page aimOutputsAVL
         * - <B> Alpha </B> = Angle of attack.
         */
    } else if (index == 2) {
        *aoname = EG_strdup("Beta");

        /*! \page aimOutputsAVL
         * - <B> Beta </B> = Sideslip angle.
         */
    } else if (index == 3) {
        *aoname = EG_strdup("Mach");

        /*! \page aimOutputsAVL
         * - <B> Mach </B> = Mach number.
         */

    } else if (index == 4) {
        *aoname = EG_strdup("pb/2V");

        /*! \page aimOutputsAVL
         * - <B> pb/2V </B> = Non-dimensional roll rate.
         */
    } else if (index == 5) {
        *aoname = EG_strdup("qc/2V");

        /*! \page aimOutputsAVL
         * - <B> qc/2V </B> = Non-dimensional pitch rate.
         */
    } else if (index == 6) {
        *aoname = EG_strdup("rb/2V");

        /*! \page aimOutputsAVL
         * - <B> rb/2V </B> = Non-dimensional yaw rate.
         */

    } else if (index == 7) {
        *aoname = EG_strdup("p'b/2V");

        /*! \page aimOutputsAVL
         * - <B> p'b/2V </B> = Non-dimensional roll acceleration.
         */

    } else if (index == 8) {
        *aoname = EG_strdup("r'b/2V");

        /*! \page aimOutputsAVL
         * - <B> r'b/2V </B> = Non-dimensional yaw acceleration.
         */

        // Body control derivatives
    } else if (index == 9) {
        *aoname = EG_strdup("CXtot");

        /*! \page aimOutputsAVL
         * Forces and moments:
         * - <B> CXtot </B> = X-component of total force in body axis
         */

    } else if (index == 10) {
        *aoname = EG_strdup("CYtot");

        /*! \page aimOutputsAVL
         * - <B> CYtot </B> = Y-component of total force in body axis
         */
    } else if (index == 11) {
        *aoname = EG_strdup("CZtot");

        /*! \page aimOutputsAVL
         * - <B> CZtot </B> = Z-component of total force in body axis
         */
    } else if (index == 12) {
        *aoname = EG_strdup("Cltot");

        /*! \page aimOutputsAVL
         * - <B> Cltot </B> = X-component of moment in body axis
         */
    } else if (index == 13) {
        *aoname = EG_strdup("Cmtot");

        /*! \page aimOutputsAVL
         * - <B> Cmtot </B> = Y-component of moment in body axis
         */
    } else if (index == 14) {
        *aoname = EG_strdup("Cntot");

        /*! \page aimOutputsAVL
         * - <B> Cntot </B> = Z-component of moment in body axis
         */
    } else if (index == 15) {
        *aoname = EG_strdup("Cl'tot");

        /*! \page aimOutputsAVL
         * - <B> Cl'tot </B> = x-component of moment in stability axis
         */
    } else if (index == 16) {
        *aoname = EG_strdup("Cn'tot");

        /*! \page aimOutputsAVL
         * - <B> Cn'tot </B> = z-component of moment in stability axis
         */
    } else if (index == 17) {
        *aoname = EG_strdup("CLtot");

        /*! \page aimOutputsAVL
         * - <B> CLtot </B> = total lift in stability axis
         */
    } else if (index == 18) {
        *aoname = EG_strdup("CDtot");

        /*! \page aimOutputsAVL
         * - <B> CDtot </B> = total drag in stability axis
         */
    } else if (index == 19) {
        *aoname = EG_strdup("CDvis");

        /*! \page aimOutputsAVL
         * - <B> CDvis </B> = viscous drag component
         */
    } else if (index == 20) {
        *aoname = EG_strdup("CLff");

        /*! \page aimOutputsAVL
         * - <B> CLff </B> = trefftz plane lift force
         */
    } else if (index == 21) {
        *aoname = EG_strdup("CYff");

        /*! \page aimOutputsAVL
         * - <B> CYff </B> = trefftz plane side force
         */
    } else if (index == 22) {
        *aoname = EG_strdup("CDind");

        /*! \page aimOutputsAVL
         * - <B> CDind </B> = induced drag force
         */
    } else if (index == 23) {
        *aoname = EG_strdup("CDff");

        /*! \page aimOutputsAVL
         * - <B> CDff </B> = trefftz plane drag force
         */
    } else if (index == 24) {
        *aoname = EG_strdup("e");

        /*! \page aimOutputsAVL
         * - <B> e = </B> Oswald Efficiency
         */
    } else if (index == 25) { // Alpha stability derivatives
        *aoname = EG_strdup("CLa");

    } else if (index == 26) {
        *aoname = EG_strdup("CYa");

    } else if (index == 27) {
        *aoname = EG_strdup("Cl'a");

    } else if (index == 28) {
        *aoname = EG_strdup("Cma");

    } else if (index == 29) {
        *aoname = EG_strdup("Cn'a");

        /*! \page aimOutputsAVL
         * Stability-axis derivatives - Alpha:
         * - <B>CLa</B> = z' force, CL, with respect to alpha.
         * - <B>CYa</B> = y force, CY, with respect to alpha.
         * - <B>Cl'a</B> = x' moment, Cl', with respect to alpha.
         * - <B>Cma</B> = y moment, Cm, with respect to alpha.
         * - <B>Cn'a</B> = z' moment, Cn', with respect to alpha.
         */

    } else if (index == 30) { // Beta stability derivatives
        *aoname = EG_strdup("CLb");

    } else if (index == 31) {
        *aoname = EG_strdup("CYb");

    } else if (index == 32) {
        *aoname = EG_strdup("Cl'b");

    } else if (index == 33) {
        *aoname = EG_strdup("Cmb");

    } else if (index == 34) {
        *aoname = EG_strdup("Cn'b");

        /*! \page aimOutputsAVL
         * Stability-axis derivatives - Beta:
         * - <B>CLb</B> = z' force, CL, with respect to beta.
         * - <B>CYb</B> = y force, CY, with respect to beta.
         * - <B>Cl'b</B> = x' moment, Cl', with respect to beta.
         * - <B>Cmb</B> = y moment, Cm, with respect to beta.
         * - <B>Cn'b</B> = z' moment, Cn', with respect to beta.
         */
    } else if (index == 35) { // Roll rate stability derivatives
        *aoname = EG_strdup("CLp'");

    } else if (index == 36) {
        *aoname = EG_strdup("CYp'");

    } else if (index == 37) {
        *aoname = EG_strdup("Cl'p'");

    } else if (index == 38) {
        *aoname = EG_strdup("Cmp'");

    } else if (index == 39) {
        *aoname = EG_strdup("Cn'p'");

        /*! \page aimOutputsAVL
         * Stability-axis derivatives - Roll rate, p':
         * - <B>CLp'</B> = z' force, CL, with respect to roll rate, p'.
         * - <B>CYp'</B> = y force, CY, with respect to roll rate, p'.
         * - <B>Cl'p'</B> = x' moment, Cl', with respect to roll rate, p'.
         * - <B>Cmp'</B> = y moment, Cm, with respect to roll rate, p'.
         * - <B>Cn'p'</B> = z' moment, Cn', with respect to roll rate, p'.
         */
    } else if (index == 40) { // Pitch rate stability derivatives
        *aoname = EG_strdup("CLq'");

    } else if (index == 41) {
        *aoname = EG_strdup("CYq'");

    } else if (index == 42) {
        *aoname = EG_strdup("Cl'q'");

    } else if (index == 43) {
        *aoname = EG_strdup("Cmq'");

    } else if (index == 44) {
        *aoname = EG_strdup("Cn'q'");

        /*! \page aimOutputsAVL
         * Stability-axis derivatives - Pitch rate, q':
         * - <B>CLq'</B> = z' force, CL, with respect to pitch rate, q'.
         * - <B>CYq'</B> = y force, CY, with respect to pitch rate, q'.
         * - <B>Cl'q'</B> = x' moment, Cl', with respect to pitch rate, q'.
         * - <B>Cmq'</B> = y moment, Cm, with respect to pitch rate, q'.
         * - <B>Cn'q'</B> = z' moment, Cn', with respect to pitch rate, q'.
         */
    } else if (index == 45) { // Yaw rate stability derivatives
        *aoname = EG_strdup("CLr'");

    } else if (index == 46) {
        *aoname = EG_strdup("CYr'");

    } else if (index == 47) {
        *aoname = EG_strdup("Cl'r'");

    } else if (index == 48) {
        *aoname = EG_strdup("Cmr'");

    } else if (index == 49) {
        *aoname = EG_strdup("Cn'r'");

        /*! \page aimOutputsAVL
         * Stability-axis derivatives - Yaw rate, r':
         * - <B>CLr'</B> = z' force, CL, with respect to yaw rate, r'.
         * - <B>CYr'</B> = y force, CY, with respect to yaw rate, r'.
         * - <B>Cl'r'</B> = x' moment, Cl', with respect to yaw rate, r'.
         * - <B>Cmr'</B> = y moment, Cm, with respect to yaw rate, r'.
         * - <B>Cn'r'</B> = z' moment, Cn', with respect to yaw rate, r'.
         */
    } else if (index == 50) { // Axial velocity stability derivatives
        *aoname = EG_strdup("CXu");

    } else if (index == 51) {
        *aoname = EG_strdup("CYu");

    } else if (index == 52) {
        *aoname = EG_strdup("CZu");

    } else if (index == 53) {
        *aoname = EG_strdup("Clu");

    } else if (index == 54) {
        *aoname = EG_strdup("Cmu");

    } else if (index == 55) {
        *aoname = EG_strdup("Cnu");

        /*! \page aimOutputsAVL
         * Body-axis derivatives - Axial velocity, u:
         * - <B>CXu</B> = x force, CX, with respect to axial velocity, u.
         * - <B>CYu</B> = y force, CY, with respect to axial velocity, u.
         * - <B>CZu</B> = z force, CZ, with respect to axial velocity, u.
         * - <B>Clu</B> = x moment, Cl, with respect to axial velocity, u.
         * - <B>Cmu</B> = y moment, Cm, with respect to axial velocity, u.
         * - <B>Cnu</B> = z moment, Cn, with respect to axial velocity, u.
         */
    } else if (index == 56) { // Sideslip velocity stability derivatives
        *aoname = EG_strdup("CXv");

    } else if (index == 57) {
        *aoname = EG_strdup("CYv");

    } else if (index == 58) {
        *aoname = EG_strdup("CZv");

    } else if (index == 59) {
        *aoname = EG_strdup("Clv");

    } else if (index == 60) {
        *aoname = EG_strdup("Cmv");

    } else if (index == 61) {
        *aoname = EG_strdup("Cnv");

        /*! \page aimOutputsAVL
         * Body-axis derivatives - Sideslip velocity, v:
         * - <B>CXv</B> = x force, CX, with respect to sideslip velocity, v.
         * - <B>CYv</B> = y force, CY, with respect to sideslip velocity, v.
         * - <B>CZv</B> = z force, CZ, with respect to sideslip velocity, v.
         * - <B>Clv</B> = x moment, Cl, with respect to sideslip velocity, v.
         * - <B>Cmv</B> = y moment, Cm, with respect to sideslip velocity, v.
         * - <B>Cnv</B> = z moment, Cn, with respect to sideslip velocity, v.
         */
    } else if (index == 62) { // Normal velocity stability derivatives
        *aoname = EG_strdup("CXw");
    } else if (index == 63) {
        *aoname = EG_strdup("CYw");
    } else if (index == 64) {
        *aoname = EG_strdup("CZw");
    } else if (index == 65) {
        *aoname = EG_strdup("Clw");
    } else if (index == 66) {
        *aoname = EG_strdup("Cmw");
    } else if (index == 67) {
        *aoname = EG_strdup("Cnw");

        /*! \page aimOutputsAVL
         * Body-axis derivatives - Normal velocity, w:
         * - <B>CXw</B> = x force, CX, with respect to normal velocity, w.
         * - <B>CYw</B> = y force, CY, with respect to normal velocity, w.
         * - <B>CZw</B> = z force, CZ, with respect to normal velocity, w.
         * - <B>Clw</B> = x moment, Cl, with respect to normal velocity, w.
         * - <B>Cmw</B> = y moment, Cm, with respect to normal velocity, w.
         * - <B>Cnw</B> = z moment, Cn, with respect to normal velocity, w.
         */
    } else if (index == 68) { // Roll rate stability derivatives
        *aoname = EG_strdup("CXp");
    } else if (index == 69) {
        *aoname = EG_strdup("CYp");
    } else if (index == 70) {
        *aoname = EG_strdup("CZp");
    } else if (index == 71) {
        *aoname = EG_strdup("Clp");
    } else if (index == 72) {
        *aoname = EG_strdup("Cmp");
    } else if (index == 73) {
        *aoname = EG_strdup("Cnp");

        /*! \page aimOutputsAVL
         * Body-axis derivatives - Roll rate, p:
         * - <B>CXp</B> = x force, CX, with respect to roll rate, p.
         * - <B>CYp</B> = y force, CY, with respect to roll rate, p.
         * - <B>CZp</B> = z force, CZ, with respect to roll rate, p.
         * - <B>Clp</B> = x moment, Cl, with respect to roll rate, p.
         * - <B>Cmp</B> = y moment, Cm, with respect to roll rate, p.
         * - <B>Cnp</B> = z moment, Cn, with respect to roll rate, p.
         */
    } else if (index == 74) { // Pitch rate stability derivatives
        *aoname = EG_strdup("CXq");
    } else if (index == 75) {
        *aoname = EG_strdup("CYq");
    } else if (index == 76) {
        *aoname = EG_strdup("CZq");
    } else if (index == 77) {
        *aoname = EG_strdup("Clq");
    } else if (index == 78) {
        *aoname = EG_strdup("Cmq");
    } else if (index == 79) {
        *aoname = EG_strdup("Cnq");

        /*! \page aimOutputsAVL
         * Body-axis derivatives - Pitch rate, q:
         * - <B>CXq</B> = x force, CX, with respect to pitch rate, q.
         * - <B>CYq</B> = y force, CY, with respect to pitch rate, q.
         * - <B>CZq</B> = z force, CZ, with respect to pitch rate, q.
         * - <B>Clq</B> = x moment, Cl, with respect to pitch rate, q.
         * - <B>Cmq</B> = y moment, Cm, with respect to pitch rate, q.
         * - <B>Cnq</B> = z moment, Cn, with respect to pitch rate, q.
         */
    } else if (index == 80) { // Yaw rate stability derivatives
        *aoname = EG_strdup("CXr");
    } else if (index == 81) {
        *aoname = EG_strdup("CYr");
    } else if (index == 82) {
        *aoname = EG_strdup("CZr");
    } else if (index == 83) {
        *aoname = EG_strdup("Clr");
    } else if (index == 84) {
        *aoname = EG_strdup("Cmr");
    } else if (index == 85) {
        *aoname = EG_strdup("Cnr");

        /*! \page aimOutputsAVL
         * Body-axis derivatives - Yaw rate, r:
         * - <B>CXr</B> = x force, CX, with respect to yaw rate, r.
         * - <B>CYr</B> = y force, CY, with respect to yaw rate, r.
         * - <B>CZr</B> = z force, CZ, with respect to yaw rate, r.
         * - <B>Clr</B> = x moment, Cl, with respect to yaw rate, r.
         * - <B>Cmr</B> = y moment, Cm, with respect to yaw rate, r.
         * - <B>Cnr</B> = z moment, Cn, with respect to yaw rate, r.
         */

    } else if (index == 86) {
        *aoname = EG_strdup("Xnp");
    } else if (index == 87) {
        *aoname = EG_strdup("Xcg");
    } else if (index == 88) {
        *aoname = EG_strdup("Ycg");
    } else if (index == 89) {
        *aoname = EG_strdup("Zcg");

        /*! \page aimOutputsAVL
         * Geometric output:
         * - <B>Xnp</B> = Neutral Point
         * - <B>Xcg</B> = x CG location
         * - <B>Ycg</B> = y CG location
         * - <B>Zcg</B> = z CG location
         */

    } else if (index == 90) {
        *aoname = EG_strdup("ControlStability");

        /*! \page aimOutputsAVL
         * Controls:
         * - <B>ControlStability</B> = a (or an array of) tuple(s) with a
         * structure of ("Control Surface Name", "JSON Dictionary") for all control surfaces in the stability axis frame.
         * The JSON dictionary has the form = {"CLtot":value,"CYtot":value,"Cl'tot":value,"Cmtot":value,"Cn'tot":value}
         */
    } else if (index == 91) {
        *aoname = EG_strdup("ControlBody");

        /*! \page aimOutputsAVL
         * - <B>ControlBody</B> = a (or an array of) tuple(s) with a
         * structure of ("Control Surface Name", "JSON Dictionary") for all control surfaces in the body axis frame.
         * The JSON dictionary has the form = {"CXtot":value,"CYtot":value,"CZtot":value,"Cltot":value,"Cmtot":value,"Cntot":value}
         */
    } else if (index == 92) {
        *aoname = EG_strdup("HingeMoment");

        /*! \page aimOutputsAVL
         * - <B>HingeMoment</B> = a (or an array of) tuple(s) with a
         * structure of ("Control Surface Name", "HingeMoment")
         */
    } else if (index == 93) {
        *aoname = EG_strdup("StripForces");

        /*! \page aimOutputsAVL
         * - <B>StripForces</B> = a (or an array of) tuple(s) with a
         * structure of ("Surface Name", "JSON Dictionary") for all surfaces.
         * The JSON dictionary has the form = {"cl":[value0,value1,value2],"cd":[value0,value1,value2]...}
         */
    } else if (index == 94) {
        *aoname = EG_strdup("EigenValues");

        /*! \page aimOutputsAVL
         * - <B>EigenValues</B> = a (or an array of) tuple(s) with a
         * structure of ("case #", "Array of eigen values").
         * The array of eigen values is of the form = [[real0,imaginary0],[real0,imaginary0],...]
         */

    } else {

        return CAPS_NOTFOUND;
    }

#if NUMOUT != 94
#error "NUMOUT is inconsistent with the list of outputs"
#endif

    if (index >= 90) {
        form->type = Tuple;
        form->units = NULL;
        form->vals.tuple = NULL;
        form->length = 0;
        form->lfixed  = form->sfixed = Change;

    } else {

        form->type = Double;
        form->units = NULL;
        form->vals.reals = NULL;
        form->vals.real = 0;

    }

    return CAPS_SUCCESS;
}


int aimCalcOutput(int iIndex, void *aimInfo,
                  const char *analysisPath, int index, capsValue *val, capsErrs **errors)
{

    int status; // Function return status

    int i; // Indexing

    char *key = NULL;
    char *fileToOpen = "capsTotalForce.txt";

    double tempVal[6];

    char jsonOut[200];

    /*
    fileToOpen= "capsTotalForce.txt";
    fileToOpen = "capsStatbilityDeriv.txt";
    fileToOpen= "capsBodyAxisDeriv.txt";
    fileToOpen = "capsHingeMoment.txt";
     */

#ifdef DEBUG
    const char *name;

    status = aim_getName(aimInfo, index, ANALYSISOUT, &name);
    printf(" avlAIM/aimCalcOutput instance = %d  index = %d  %s %d!\n", iIndex, index, name, status);
#endif

    *errors        = NULL;
    val->vals.real = 0.0;

    switch (index) {
    case 1:
        fileToOpen= "capsTotalForce.txt";
        key = "Alpha =";
        break;
    case 2:
        fileToOpen= "capsTotalForce.txt";
        key = "Beta  =";
        break;
    case 3:
        fileToOpen= "capsTotalForce.txt";
        key = "Mach  =";
        break;
    case 4:
        fileToOpen= "capsTotalForce.txt";
        key = "pb/2V =";
        break;
    case 5:
        fileToOpen= "capsTotalForce.txt";
        key = "qc/2V =";
        break;
    case 6:
        fileToOpen= "capsTotalForce.txt";
        key = "rb/2V =";
        break;
    case 7:
        fileToOpen= "capsTotalForce.txt";
        key = "p'b/2V =";
        break;
    case 8:
        fileToOpen= "capsTotalForce.txt";
        key = "r'b/2V =";
        break;
    case 9:
        fileToOpen= "capsTotalForce.txt";
        key = "CXtot =";
        break;
    case 10:
        fileToOpen= "capsTotalForce.txt";
        key = "CYtot =";
        break;
    case 11:
        fileToOpen= "capsTotalForce.txt";
        key = "CZtot =";
        break;
    case 12:
        fileToOpen= "capsTotalForce.txt";
        key = "Cltot =";
        break;
    case 13:
        fileToOpen= "capsTotalForce.txt";
        key = "Cmtot =";
        break;
    case 14:
        fileToOpen= "capsTotalForce.txt";
        key = "Cntot =";
        break;
    case 15:
        fileToOpen= "capsTotalForce.txt";
        key = "Cl'tot =";
        break;
    case 16:
        fileToOpen= "capsTotalForce.txt";
        key = "Cn'tot =";
        break;
    case 17:
        fileToOpen= "capsTotalForce.txt";
        key = "CLtot =";
        break;
    case 18:
        fileToOpen= "capsTotalForce.txt";
        key = "CDtot =";
        break;
    case 19:
        fileToOpen= "capsTotalForce.txt";
        key = "CDvis =";
        break;
    case 20:
        fileToOpen= "capsTotalForce.txt";
        key = "CLff  =";
        break;
    case 21:
        fileToOpen= "capsTotalForce.txt";
        key = "CYff  =";
        break;
    case 22:
        fileToOpen= "capsTotalForce.txt";
        key = "CDind =";
        break;
    case 23:
        fileToOpen= "capsTotalForce.txt";
        key = "CDff  =";
        break;
    case 24:
        fileToOpen= "capsTotalForce.txt";
        key = "e =";
        break;

        // Alpha stability derivatives
    case 25:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CLa =";
        break;

    case 26:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CYa =";
        break;

    case 27:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cla =";
        break;

    case 28:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cma =";
        break;

    case 29:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cna =";
        break;

        // Beta stability derivatives
    case 30:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CLb =";
        break;

    case 31:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CYb =";
        break;

    case 32:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Clb =";
        break;

    case 33:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cmb =";
        break;

    case 34:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cnb =";
        break;

        // Roll rate p' stability derivatives
    case 35:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CLp =";
        break;

    case 36:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CYp =";
        break;

    case 37:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Clp =";
        break;

    case 38:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cmp =";
        break;

    case 39:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cnp =";
        break;

        // Pitch rate q' stability derivatives
    case 40:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CLq =";
        break;

    case 41:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CYq =";
        break;

    case 42:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Clq =";
        break;

    case 43:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cmq =";
        break;

    case 44:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cnq =";
        break;

        // Yaw rate r' stability derivatives
    case 45:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CLr =";
        break;

    case 46:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "CYr =";
        break;

    case 47:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Clr =";
        break;

    case 48:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cmr =";
        break;

    case 49:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Cnr =";
        break;


        // Axial vel (body axis)  stability derivatives
    case 50:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CXu =";
        break;

    case 51:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CYu =";
        break;

    case 52:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CZu =";
        break;

    case 53:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Clu =";
        break;

    case 54:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cmu =";
        break;

    case 55:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cnu =";
        break;


    // Slidslip vel (body axis) stability derivatives
    case 56:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CXv =";
        break;

    case 57:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CYv =";
        break;

    case 58:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CZv =";
        break;

    case 59:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Clv =";
        break;

    case 60:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cmv =";
        break;

    case 61:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cnv =";
        break;

        // Normal vel (body axis)  stability derivatives
    case 62:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CXw =";
        break;

    case 63:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CYw =";
        break;

    case 64:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CZw =";
        break;

    case 65:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Clw =";
        break;

    case 66:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cmw =";
        break;

    case 67:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cnw =";
        break;

        // Roll rate (body axis)  stability derivatives
    case 68:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CXp =";
        break;

    case 69:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CYp =";
        break;

    case 70:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CZp =";
        break;

    case 71:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Clp =";
        break;

    case 72:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cmp =";
        break;

    case 73:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cnp =";
        break;

        // Pitch rate (body axis)  stability derivatives
    case 74:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CXq =";
        break;

    case 75:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CYq =";
        break;

    case 76:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CZq =";
        break;

    case 77:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Clq =";
        break;

    case 78:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cmq =";
        break;

    case 79:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cnq =";
        break;

        // Yaw rate (body axis)  stability derivatives
    case 80:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CXr =";
        break;

    case 81:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CYr =";
        break;

    case 82:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "CZr =";
        break;

    case 83:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Clr =";
        break;

    case 84:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cmr =";
        break;

    case 85:
        fileToOpen= "capsBodyAxisDeriv.txt";
        key = "Cnr =";
        break;

    case 86:
        fileToOpen = "capsStatbilityDeriv.txt";
        key = "Xnp =";
        break;

    case 87:
        fileToOpen= "caps.run";
        key = "X_cg      =";
        break;

    case 88:
        fileToOpen= "caps.run";
        key = "Y_cg      =";
        break;

    case 89:
        fileToOpen= "caps.run";
        key = "Z_cg      =";
        break;

    case 90:
        fileToOpen = "capsHingeMoment.txt";
        break;

    case 91:
        fileToOpen = "capsStripForce.txt";
        break;

    case 92:
        fileToOpen = "capsEigenValues.txt";
        break;
    }

    if (index <= 89) {

      if (key == NULL) {
          printf("No string key found!\n");
          status = CAPS_NOTFOUND;
          goto cleanup;
      }

      status = read_Data(fileToOpen, analysisPath, key, &val->vals.real);
      if (status != CAPS_SUCCESS) goto cleanup;

    } else if (index >= 90 && index <= 92) { // Need to build something for control output

        // Clear initial memory
        if (val->vals.tuple != NULL) {
            for (i = 0; i < val->length; i++) {
                EG_free(val->vals.tuple[i].name);
                EG_free(val->vals.tuple[i].value);
            }
            EG_free(val->vals.tuple);
            val->nrow = val->length = 0;
            val->vals.tuple = NULL;
        }

        // Initiate tuple base on number of control surfaces
        val->length = val->nrow = avlInstance[iIndex].controlMap.numAttribute;

        // nothing to do if there is no control attributes
        if (val->length == 0)
            return CAPS_SUCCESS;

        val->vals.tuple = (capsTuple *) EG_alloc(val->length*sizeof(capsTuple));
        if (val->vals.tuple == NULL) {
            status = EGADS_MALLOC;
            goto cleanup;
        }

        for (i = 0; i < val->length; i++) val->vals.tuple[i].name = val->vals.tuple[i].value = NULL;

        // Loop through control surfaces
        for (i = 0; i < avlInstance[iIndex].controlMap.numAttribute; i++) {

            val->vals.tuple[i].name = EG_strdup(avlInstance[iIndex].controlMap.attributeName[i]);

            // Stability axis
            if (index == 90) {
                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "CLtot", ANALYSISOUT), &tempVal[0]);
                if (status != CAPS_SUCCESS) goto cleanup;

                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "CYtot", ANALYSISOUT), &tempVal[1]);
                if (status != CAPS_SUCCESS) goto cleanup;


                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "Cl'tot", ANALYSISOUT), &tempVal[2]);
                if (status != CAPS_SUCCESS) goto cleanup;


                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT), &tempVal[3]);
                if (status != CAPS_SUCCESS) goto cleanup;


                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "Cn'tot", ANALYSISOUT), &tempVal[4]);
                if (status != CAPS_SUCCESS) goto cleanup;

                sprintf(jsonOut,"{\"%s\":%7.6f,\"%s\":%7.6f,\"%s\":%7.6f,\"%s\":%7.6f,\"%s\":%7.6f}", "CLtot",  tempVal[0],
                                                                                                      "CYtot",  tempVal[1],
                                                                                                      "Cl'tot", tempVal[2],
                                                                                                      "Cmtot",  tempVal[3],
                                                                                                      "Cn'tot", tempVal[4]);
                val->vals.tuple[i].value = EG_strdup(jsonOut);

            } else if (index == 91) {

                // Body axis
                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "CXtot", ANALYSISOUT), &tempVal[0]);
                if (status != CAPS_SUCCESS) goto cleanup;

                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "CYtot", ANALYSISOUT), &tempVal[1]);
                if (status != CAPS_SUCCESS) goto cleanup;

                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "CZtot", ANALYSISOUT), &tempVal[2]);
                if (status != CAPS_SUCCESS) goto cleanup;


                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "Cltot", ANALYSISOUT), &tempVal[3]);
                if (status != CAPS_SUCCESS) goto cleanup;


                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT), &tempVal[4]);
                if (status != CAPS_SUCCESS) goto cleanup;


                status = get_controlDeriv(aimInfo, (char *) analysisPath, avlInstance[iIndex].controlMap.attributeIndex[i],
                                          aim_getIndex(aimInfo, "Cntot", ANALYSISOUT), &tempVal[5]);
                if (status != CAPS_SUCCESS) goto cleanup;;

                sprintf(jsonOut,"{\"%s\":%7.6f,\"%s\":%7.6f,\"%s\":%7.6f,\"%s\":%7.6f,\"%s\":%7.6f,\"%s\":%7.6f}", "CXtot", tempVal[0],
                                                                                                                   "CYtot", tempVal[1],
                                                                                                                   "CZtot", tempVal[2],
                                                                                                                   "Cltot", tempVal[3],
                                                                                                                   "Cmtot", tempVal[4],
                                                                                                                   "Cntot", tempVal[5]);
                val->vals.tuple[i].value = EG_strdup(jsonOut);

            } else if (index == 92) {

                status = read_Data(fileToOpen, analysisPath, val->vals.tuple[i].name, &tempVal[0]);
                if (status != CAPS_SUCCESS) goto cleanup;

                sprintf(jsonOut, "%5.4e", tempVal[0]);

                val->vals.tuple[i].value = EG_strdup(jsonOut);

            } else {
                status = CAPS_NOTFOUND;
            }

        }

    } else if (index == 93) {

      // Clear initial memory
      if (val->vals.tuple != NULL) {
          for (i = 0; i < val->length; i++) {
              EG_free(val->vals.tuple[i].name);
              EG_free(val->vals.tuple[i].value);
          }
          EG_free(val->vals.tuple);
          val->nrow = val->length = 0;
          val->vals.tuple = NULL;
      }

      // Read in the strip forces
      status = read_StripForces(analysisPath, &val->length, &val->vals.tuple);
      if (status != CAPS_SUCCESS) goto cleanup;
      val->nrow = val->length;

    } else if (index == 94) {

      // Clear initial memory
      if (val->vals.tuple != NULL) {
          for (i = 0; i < val->length; i++) {
              EG_free(val->vals.tuple[i].name);
              EG_free(val->vals.tuple[i].value);
          }
          EG_free(val->vals.tuple);
          val->nrow = val->length = 0;
          val->vals.tuple = NULL;
      }

      // Read in the strip forces
      status = read_EigenValues(analysisPath, &val->length, &val->vals.tuple);
      if (status != CAPS_SUCCESS) goto cleanup;
      val->nrow = val->length;

    } else {

        printf("DEVELOPER Error! Unknown index %d\n", index);
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    status = CAPS_SUCCESS;

    cleanup:
        return status;
}


void aimCleanup()
{

    int iIndex;

#ifdef DEBUG
    printf(" avlAIM/aimCleanup!\n");
#endif


    for (iIndex = 0; iIndex < numInstance; iIndex++) {
        printf(" Cleaning up avlInstance - %d\n", iIndex);

        // Attribute to index map
        (void) destroy_mapAttrToIndexStruct(&avlInstance[iIndex].controlMap);

    }

    if (avlInstance != NULL) EG_free(avlInstance);
    avlInstance = NULL;
    numInstance = 0;

}


// Back function used to calculate sensitivities
int aimBackdoor(int iIndex, void *aimInfo, const char *JSONin, char **JSONout) {

    /*! \page aimBackDoorAVL AIM Back Door
     * The back door function of this AIM may be used as an alternative to retrieve sensitivity information produced by the
     * AIM. The JSONin string should be of the following form '{"mode": "sensitivity", "inputVar": "Name of Input Variable",
     * "outputVar": "Name of Output Variable"}', while the JSONout string will look like '{"sensitivity": value}'. Important: the
     * JSONout string is freeable! Invalid combinations of input and output variables returns a CAPS_MISMATCH error code.
     *
     * Acceptable values for the "Name of Input Variable" are as follows (definitions are consistent, where appropriate,
     *  with \ref aimInputsAVL):
     * - "Alpha"
     * - "Beta"
     * - "RollRate"
     * - "PitchRate"
     * - "YawRate"
     * - "AxialVelocity"
     * - "SideslipVelocity"
     * - "NormalVelocity"
     * - "AVL_Control:Name_of_Control_Surface", where Name_of_Control_Surface should be replaced with name of the desired
     * control surface
     *
     * Acceptable values for the "Name of Output Variable" are as follows (definitions are consistent with \ref aimOutputsAVL):
     * - "CLtot"
     * - "CYtot"
     * - "Cl'tot"
     * - "Cmtot"
     * - "Cn'tot"
     * - "CXtot"
     * - "CZtot"
     * - "Cltot"
     * - "Cntot"
     */

    int status; // Function return status

    int index;
    double data;
    capsValue val;
    capsErrs *errors;

    char *keyValue = NULL;
    char *keyWord = NULL;

    char *tempString = NULL; // Freeable

    int inputIndex = 0, outputIndex = 0, controlIndex;

    char *outputJSON = NULL;

    index = -99;

    *JSONout = NULL;

    if (avlInstance[iIndex].analysisPath == NULL) {
        printf("Analysis path hasn't been set - Need to run preAnalysis first!");
        return CAPS_DIRERR;
    }

    keyWord = "mode";
    status = search_jsonDictionary(JSONin, keyWord, &keyValue);
    if (status != CAPS_SUCCESS) goto cleanup;


    // Type of acceptable modes for this AIM backdoor
    if (strcasecmp(keyValue, "\"Sensitivity\"") != 0) {

        printf("Error: A valid mode wasn't found for AIMBackDoor!\n");
        status = CAPS_NOTFOUND;
        goto cleanup;
    }

    // We are using this to get sensitivities
    if (strcasecmp(keyValue, "\"Sensitivity\"") == 0) {

        if (keyValue != NULL) EG_free(keyValue);
        keyValue = NULL;

        // Input variable
        keyWord = "inputVar";
        status = search_jsonDictionary(JSONin, keyWord, &keyValue);
        if (status != CAPS_SUCCESS) goto cleanup;

        tempString = string_removeQuotation(keyValue);

        inputIndex = aim_getIndex(aimInfo, tempString, ANALYSISIN);
        if (inputIndex <= 0) {
            status = inputIndex;

            if (strcasecmp(tempString, "AxialVelocity") == 0) {
                inputIndex = NUMINPUT+1;

            } else if (strcasecmp(tempString, "SideslipVelocity") == 0) {
                inputIndex = NUMINPUT+2;

            } else if (strcasecmp(tempString, "NormalVelocity") == 0) {
                inputIndex = NUMINPUT+3;

                // Try to see if it is a control parameter
            } else if (parse_controlName(iIndex, tempString, &controlIndex) == CAPS_SUCCESS) {
                inputIndex = CAPSMAGIC;

            } else {

                printf("Error: Unable to get index for inputVar = %s\n", tempString);
                goto cleanup;
            }
        }

        if (tempString != NULL) EG_free(tempString);
        tempString = NULL;

        // Outout variable
        keyWord = "outputVar";
        status = search_jsonDictionary(JSONin, keyWord, &keyValue);
        if (status != CAPS_SUCCESS) goto cleanup;

        tempString = string_removeQuotation(keyValue);

        outputIndex = aim_getIndex(aimInfo, tempString, ANALYSISOUT);
        if (outputIndex <= 0) {
            status = outputIndex;
            printf("Error: Unable to get index for outputVar = %s\n", tempString);
            goto cleanup;
        }

        if (tempString != NULL) EG_free(tempString);
        tempString = NULL;

        //printf("InputIndex = %d\n", inputIndex);
        //printf("OutputIndex = %d\n", outputIndex);

        // Stability axis - Alpha
        if (outputIndex == aim_getIndex(aimInfo, "CLtot", ANALYSISOUT) &&
            inputIndex == aim_getIndex(aimInfo, "Alpha", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CLa", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Alpha", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYa", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cl'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Alpha", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cl'a", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Alpha", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cma", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cn'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Alpha", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cn'a", ANALYSISOUT);
        }

        // Stability axis - Beta
        else if (outputIndex == aim_getIndex(aimInfo, "CLtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Beta", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CLb", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Beta", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYb", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cl'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Beta", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cl'b", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Beta", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cmb", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cn'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "Beta", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cn'b", ANALYSISOUT);
        }

        // Stability axis - RollRate
        else if (outputIndex == aim_getIndex(aimInfo, "CLtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CLp'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYp'", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cl'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cl'p'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cmp'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cn'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cn'p'", ANALYSISOUT);
        }

        // Stability axis - PitchRate
        else if (outputIndex == aim_getIndex(aimInfo, "CLtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CLq'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYq'", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cl'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cl'q'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cmq'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cn'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cn'q'", ANALYSISOUT);
        }


        // Stability axis - YawRate
        else if (outputIndex == aim_getIndex(aimInfo, "CLtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CLr'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYr'", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cl'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cl'r'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cmr'", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cn'tot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cn'r'", ANALYSISOUT);
        }


        //////////////////////////////////////////////////////////

        // Body axis - AxialVelocity
        else if (outputIndex == aim_getIndex(aimInfo, "CXtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+1) {

            index = aim_getIndex(aimInfo,"CXu", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+1) {

            index = aim_getIndex(aimInfo,"CYu", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "CZtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+1) {

            index = aim_getIndex(aimInfo,"CZu", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cltot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+1) {

            index = aim_getIndex(aimInfo,"Clu", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+1) {

            index = aim_getIndex(aimInfo,"Cmu", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cntot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+1) {

            index = aim_getIndex(aimInfo,"Cnu", ANALYSISOUT);
        }

        // Body axis - Sideslip
        else if (outputIndex == aim_getIndex(aimInfo, "CXtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+2) {

            index = aim_getIndex(aimInfo,"CXv", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+2) {

            index = aim_getIndex(aimInfo,"CYv", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "CZtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+2) {

            index = aim_getIndex(aimInfo,"CZv", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cltot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+2) {

            index = aim_getIndex(aimInfo,"Clv", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+2) {

            index = aim_getIndex(aimInfo,"Cmv", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cntot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+2) {

            index = aim_getIndex(aimInfo,"Cnv", ANALYSISOUT);
        }

        // Body axis - Normal
        else if (outputIndex == aim_getIndex(aimInfo, "CXtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+3) {

            index = aim_getIndex(aimInfo,"CXw", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+3) {

            index = aim_getIndex(aimInfo,"CYw", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "CZtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+3) {

            index = aim_getIndex(aimInfo,"CZw", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cltot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+3) {

            index = aim_getIndex(aimInfo,"Clw", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+3) {

            index = aim_getIndex(aimInfo,"Cmw", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cntot", ANALYSISOUT) &&
                 inputIndex  == NUMINPUT+3) {

            index = aim_getIndex(aimInfo,"Cnw", ANALYSISOUT);
        }

        // Body axis - RollRate
        else if (outputIndex == aim_getIndex(aimInfo, "CXtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CXp", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYp", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "CZtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CZp", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cltot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Clp", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cmp", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cntot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "RollRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cnp", ANALYSISOUT);
        }

        // Body axis - PitchRate
        else if (outputIndex == aim_getIndex(aimInfo, "CXtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CXq", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYq", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "CZtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CZq", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cltot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Clq", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cmq", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cntot", ANALYSISOUT) &&
                 inputIndex == aim_getIndex(aimInfo, "PitchRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cnq", ANALYSISOUT);
        }

        // Body axis - YawRate
        else if (outputIndex == aim_getIndex(aimInfo, "CXtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CXr", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "CYtot", ANALYSISOUT) &&
                 inputIndex == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CYr", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "CZtot", ANALYSISOUT) &&
                 inputIndex == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"CZr", ANALYSISOUT);

        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cltot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Clr", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cmtot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cmr", ANALYSISOUT);
        }

        else if (outputIndex == aim_getIndex(aimInfo, "Cntot", ANALYSISOUT) &&
                 inputIndex  == aim_getIndex(aimInfo, "YawRate", ANALYSISIN)) {

            index = aim_getIndex(aimInfo,"Cnr", ANALYSISOUT);
        }
        else if (inputIndex == CAPSMAGIC) { // Control variable found
            index = CAPSMAGIC;
        }

        else {
            printf("Invalid combination of input and output variables.\n");
            status = CAPS_MISMATCH;
            goto cleanup;
        }

        // If we have a control variable
        if (index == CAPSMAGIC) {

            status = get_controlDeriv(aimInfo, avlInstance[iIndex].analysisPath, controlIndex, outputIndex, &data);
            if (status != CAPS_SUCCESS) goto cleanup;

            val.vals.real = data;


        } else { // Make the standard call

            status = aimCalcOutput(iIndex, aimInfo, avlInstance[iIndex].analysisPath, index, &val, &errors);
            if (status != CAPS_SUCCESS) goto cleanup;
        }

        outputJSON = (char *) EG_alloc(50*sizeof(char));

        sprintf(outputJSON, "{\"sensitivity\": %7.6f}", val.vals.real);
    }

    *JSONout = outputJSON;

    status = CAPS_SUCCESS;
    goto cleanup;

    cleanup:
        if (status != CAPS_SUCCESS) printf("Error: Premature exit in aimBackdoor, status = %d\n", status);

        if (keyValue != NULL) EG_free(keyValue);

        if (tempString != NULL) EG_free(tempString);

        return status;
}
