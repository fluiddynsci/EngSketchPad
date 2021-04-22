                        ESP: The Engineering Sketch Pad
                             Rev 1.18 -- June 2020


0. Warnings!

    Windows 7 & 8 are no longer supported, only Windows 10 is tested. 
    This also means that older versions of MS Visual Studio are no longer 
    being tested either. Only MSVS versions 2015 and up are fully supported.

    This ESP release no longer tests against Python 2.7. It should still
    work, but we strongly advise going to at least Python 3.7. Also, we
    now only support OpenCASCADE at Rev 7.3 or higher. And these must be
    the versions taken from the ESP website (and not from elsewhere). At
    this point we recommend 7.3.1 and are testing 7.4.1, which you can find 
    currently in the ESP website subdirectory "otherOCCs".

    Apple's OSX Catalina (10.15) is a REAL problem. You cannot download the
    distributions using a browser. For instructions on how to get ESP see 
    OSXcatalina.txt on the web site.


1. Prerequisites

    The most significant prerequisite for this software is OpenCASCADE.
    This ESP release only supports the prebuilt versions marked 7.3.1 
    and 7.4.1, which are available at http://acdl.mit.edu/ESP. Please DO 
    NOT report any problems with any other versions of OpenCASCADE, much 
    effort has been spent in "hardening" the OpenCASCADE code. It is advised 
    that all ESP users update to 7.3.1/7.4.1 because of better robustness, 
    performance. If you are still on a LINUX box with a version of gcc less 
    than 4.8, you may want to consider an upgrade, so that at least 7.3.1 
    can be used.

    Another prerequisite is a WebGL/Websocket capable Browser. In general 
    these include Mozilla's FireFox, Google Chrome and Apple's Safari. 
    Internet Explorer/Edge is NOT supported because of a problem in their
    Websockets implementation.  Also, note that there are some  problems with 
    Intel Graphics and some WebGL Browsers. For LINUX, "zlib" development is 
    required.

    CAPS has a dependency on UDUNITS2, and potentially on Python and other 
    applications. See Section 2.3.

1.1 Source Distribution Layout

    In the discussions that follow, $ESP_ROOT will be used as the name of the 
    directory that contains:

    README.txt        - this file
    bin               - a directory that will contain executables
    CAPSexamples      - a directory with CAPS examples
    config            - files that allow for automatic configuration
    data              - test and example scripts
    doc               - documentation
    ESP               - web client code for the Engineering Sketch Pad
    externApps        - the ESP connections to 3rd party Apps (outside of CAPS)
    include           - location for all ESP header files
    lib               - a directory that will contain libraries, shared objects
                        and DLLs
    SLUGS             - the browser code for Slugs (web Slugs client)
    src               - source files (contains EGADS, CAPS, wvServer & OpenCSM)
    training          - training slides and examples
    udc               - a collection of User Defined Components
    wvClient          - simple examples of Web viewing used by EGADS

1.2 Release Notes

1.2.0 UDUNITS & Windows

    There appears to be a fundamental flaw with the package "expat" (the XML 
    parser) used by UDUNITS (a CAPS dependency) and Windows' use of drive 
    letters. "expat" uses the current drive to look for the files to be 
    included in the unit definitions. This only works if ESP and the place 
    where you are running a CAPS app are on the same drive. Note: this may be
    fixed in a future release.

1.2.1 EGADS

    There has been 2 significant updates made to EGADS from Rev 1.17:

    1) EGADSlite has been refactored in order to support CUDA and GPUs
    2) The tessellator has been improved in both quality and robustness

1.2.2 ESP

    In addition to many big fixes (see $ESP_ROOT/src/OpenCSMnotes.txt
    for a full list), the significant upgrades are:

    1) New/updated commands/statements and functions
     a) Node, Edges, and Faces can now be SELECTed by bounding boxes
     b) SUBTRACT can now be applied to coplanar SheetBodys
     c) SCALE can now scale about a scaling center
     d) SELECT ADD can now add Faces, Edges, or Nodes by index
     e) SELECT SUB can now subtract entities by index
     f) GROUP with a negative argument ungroups
     g) CONNECT generates degenerate Faces when edgeList* contains a zero
     h) SWEEP can be applied to a FaceBody
     i) new SSLOPE allows a user to specify the slope at the beginning
        or end of a SPLINE in a sketch
     j) COMBINE command now returns a SheetBody if the Shell created is
        not closed
     k) SELECTing via attributes has been extended to have attribute
        values that are strings, integer(s), or real(s)
   2) New command line arguments
     a) -skipTess allows a user to skip the tessellation on the Bodys on
        the stack at the end
     b) -printStack allows a user to print the constants of the stack
        after every Branch
     c) -batch is automatically selected when -skipTess is specified
   3) New/updated UDPs, UDCs, or UDFs
     a) applyTparams.udc puts .tParams on Body based upon its size
     b) calcCG.udc computes the CG of all Bodys on the stack
     c) dpEllipse is modified to have nedge and thbeg input parameters
     d) editAttrUdf now allows PATBEG/PATEND statements
   4) OpenCSM updates
     a) update default tessellation parameters
     b) allow UDFs to receive any number of input Bodys (back to Mark
        or beginning of stack)
     c) add ocsmUpdateDespmtrs to allow a user to update the DESPMTR
        values from a file
     d) add __filename__ to files processed by -loadEgads and -dumpEgads
     e) remove tmp_OpenCSM files at beginning of ocsmLoad
     f) Edges that come from Booleans no longer have the Attributes
        of possibly-coincident Edges in one of the parents
     g) significantly speed up finishing all Bodys
   5) ESP updates
     a) allow user to add an EVALUATE statement from ESP interface
     b) add CFGPMTR highlighting and hints in ESP
     c) unpost File or Tool menu if File, Tool, StepThur, Help,
        UpToDate, or Undo button is pressed
     d) make groups for at- and at-at-parameters in ESP
     e) in serveCSM -sensTess, show Face tufts in blue and Edge tufts in red
     f) add option to ESP to turn on/off all Nodes, Edges, Faces, or Csystems
     g) allow plotfile to contain triangles (if jmax==-2)

1.2.3 Known issues in v1.18:

    Sensitivities for BLENDS with C0 are done by finite differences.


2. Building the Software

    The config subdirectory contains scripts that need to be used to generate 
    the environment both to build and run the software here. There are two 
    different build procedures based on the OS:

    If using Windows, skip to section 2.2.

2.1 Linux and MAC OSX

    The configuration is built using the path where the OpenCASCADE runtime 
    distribution can be found.  This path can be located in an OpenCASCADE 
    distribution by looking for a subdirectory that includes an "inc" or 
    "include" directory and either a "lib" or "$ARCH/lib" (where $ARCH is 
    the name of your architecture) directory.  Once that is found, execute 
    the commands:

        % cd $ESP_ROOT/config
        % ./makeEnv **name_of_OpenCASCADE_directory_containing_inc_and_lib**

    An optional second argument to makeEnv is required if the distribution 
    of OpenCASCADE has multiple architectures. In this case it is the 
    subdirectory name that contains the libraries for the build of interest 
    (CASARCH).

    This procedure produces 2 files at the top level: ESPenv.sh and
    ESPenv.csh.  These are the environments for both sh (bash) and csh 
    (tcsh) respectively.  The appropriate file can be "source"d or 
    included in the user's startup scripts. This must be done before either 
    building and/or running the software. For example, if using the csh or 
    tcsh:

        % cd $ESP_ROOT
        % source ESPenv.csh

    or if using bash:

        $ cd $ESP_ROOT
        $ source ESPenv.sh

    Skip to section 2.3.

2.2 Windows Configuration

    IMPORTANT: The ESP distribution and OpenCASCADE MUST be unpackaged 
               into a location ($ESP_ROOT) that has NO spaces in the path!

    The configuration is built from the path where the OpenCASCADE runtime 
    distribution can be found. MS Visual Studio is required and a command 
    shell where the 64bit C/C++ compiler should be opened and the following 
    executed in that window (note that MS VS 2015, 2017 and 2019 are all 
    fully supported). The Windows environment is built simply by going to 
    the config subdirectory and executing the script "winEnv" in a bash 
    shell (run from the command window):

        C:\> cd %ESP_ROOT%\config
        C:\> bash winEnv D:\OpenCASCADE7.3.1

    winEnv (like makeEnv) has an optional second argument that is only 
    required if the distribution of OpenCASCADE has multiple architectures. 
    In this case it is the subdirectory name that contains the libraries 
    for the build of interest (CASARCH).

    This procedure produces a single file at the top level: ESPenv.bat.  
    This file needs to be executed before either building and/or running 
    the software. This is done with:

        C:\> cd %ESP_ROOT%
        C:\> ESPenv

    Check that the method that you used to unzip the distribution created 
    directories named %ESP_ROOT%\bin and %ESP_ROOT%\lib. If it did not, 
    create them using the commands:

        C:\> cd %ESP_ROOT%
        C:\> mkdir bin
        C:\> mkdir lib

2.3 CAPS Options

    CAPS requires the use of the Open Source Project UDUNITS2 for all unit 
    conversions. Since there are no prebuilt package distributions for the 
    MAC and Windows, the CAPS build procedure copies prebuilt DLL/DyLibs 
    to the lib directory of ESP. Because most Linux distributions contain 
    a UDUNITS2 package, another procedure is used. If the UDUNITS2 
    development package is loaded in the OS, then nothing is done. If not 
    loaded, then a pre-built shared object is moved to the ESP lib directory.
    Be careful if you install UDUNITS2 from your OS package manager at some
    later time.

2.3.1 Python with CAPS (pyCAPS)

    Python may be used with CAPS to provide testing, general scripting and
    demonstration capabilities. The Python development package is required
    under Linux. The building of pyCAPS is turned on by 2 environment 
    variables:

    PYTHONINC  is the include path to find the Python includes for building
    PYTHONLIB  is a description of the location of the Python libraries and
               the library to use

    The execution of pyCAPS requires a single environment variable:

    PYTHONPATH is a Python environment variable that needs to have the path
               $ESP_ROOT/lib included.

    For MACs and LINUX the configuration procedure inserts these environment 
    variables with the locations it finds by executing the version of Python 
    available in the shell performing the build. If makeEnv emits any errors
    related to Python, the resultant environment file(s) will need to be 
    updated in order to use Python (the automatic procedure has failed).

    For Windows ESPenv.bat must be edited (unless configured from a command 
    prompt that has both the MSVS and Python environments), the "rem" 
    removed and the appropriate information set (if Python exists on the 
    machine). Also note that the bit size (32 or 64) of Python that gets 
    used on Windows must be consistent with the build of ESP, which is 
    64bit.

    For Example on Windows (after downloading and installing Python on C:):
      set PYTHONINC=C:\Python37\include
      set PYTHONLIB=C:\Python37\Libs\python37.lib

2.3.2 3rd Party Environment Variables

    CAPS is driven by a plugin technology using AIMs (Analysis Interface  
    Modules). These AIMs allow direct coupling between CAPS and the external 
    meshers and solvers. Many are built by default (where there are no 
    licensing problems or other dependencies). The CAPS build subsystem will 
    respond to the following (if these are not set, then the AIMs for these 
    systems will not be built):

    AFLR (See Section 4.1):
      AFLR      is the path where the AFLR distribution has been deposited
      AFLR_ARCH is the architecture flag to use (MacOSX-x86-64, Linux-x86-64,
                WIN64) -- note that this is set by the config procedure

    AWAVE
      AWAVE is the location to find the FORTRAN source for AWAVE

    TETGEN
      TETGEN is the path where the TetGen distribution has been unpacked

2.3.3 The Cart3D Design Framework

    The application ESPxddm is the ESP connection to the Cart3D Design
    Framework. On LINUX this requires that the libxml2 development package
    be installed. If it is, then ESPxddm is built, otherwise it is not.

2.3.4 CAPS AIM Documentation

    The CAPS documentation can be seen in PDF form from within the directory
    $ESP_ROOT/doc/CAPSdoc. Or in html by $ESP_ROOT/doc/CAPSdoc/html/index.html.

2.4 The Build

    For any of the operating systems, after properly setting the environment 
    in the command window (or shell), follow this simple procedure:

        % cd $ESP_ROOT/src
        % make

    or

        C:\> cd $ESP_ROOT\src
        C:\> make

    You can use "make clean" which will clean up all object modules or 
    "make cleanall" to remove all objects, executables, libraries, shared 
    objects and dynamic libraries.


3.0 Running

3.1 serveCSM and ESP

    To start ESP there are two steps: (1) start the "server" and (2) start 
    the "browser". This can be done in a variety of ways, but the two most 
    common follow.

3.1.1 Procedure 1: have ESP automatically started from serveCSM

    If it exists, the ESP_START environment variable contains the command 
    that should be executed to start the browser once the server has 
    created its scene graph.  On a MAC, you can set this variable with 
    commands such as

        % setenv ESP_START "open -a /Applications/Firefox.app $ESP_ROOT/ESP/ESP.html"

    or

        % export ESP_START="open -a /Applications/Firefox.app $ESP_ROOT/ESP/ESP.html"

    depending on the shell in use.  The commands in other operating systems 
    will differ slightly, depending on how the browser can be started from 
    the command line, for example for Windows it may be:

        % set ESP_START=""C:\Program Files (x86)\Mozilla Firefox\firefox.exe" %ESP_ROOT%\ESP\ESP.html"

    To run the program, use:

         % cd $ESP_ROOT/bin
         % ./serveCSM ../data/tutorial1

3.1.2 Procedure 2: start the browser manually

    If the ESP_START environment variable does not exist, issuing the
    commands:

        % cd $ESP_ROOT/bin
        % ./serveCSM ../data/tutorial1

    will start the server.  The last lines of output from serveCSM tells 
    the user that the server is waiting for a browser to attach to it. 
    This can be done by starting a browser (FireFox and GoogleChrome have 
    been tested) and loading the file:

        $ESP_ROOT/ESP/ESP.html

    Whether you used procedure 1 or 2, as long as the browser stays connected 
    to serveCSM, serveCSM will stay alive and handle requests sent to it from 
    the browser. Once the last browser that is connected to serveCSM exits, 
    serveCSM will shut down.

    Note that the default "port" used by serveCSM is 7681. One can change 
    the port in the call to serveCSM with a command such as:

        % cd $ESP_ROOT/bin
        % ./serveCSM ../data/tutorial1 -port 7788

    Once the browser starts, you will be prompted for a "hostname:port".  
    Make the appropriate response depending on the network situation. Once 
    the ESP GUI is functional, press the "help" button in the upper left 
    if you want to execute the tutorial.

3.2 egads2cart

    This example takes an input geometry file and generates a Cart3D "tri" 
    file. The acceptable input is STEP, EGADS or OpenCASCADE BRep files 
    (which can all be generated from an OpenCSM "dump" command).

        % cd $ESP_ROOT/bin
        % ./egads2cart geomFilePath [angle relSide relSag]

3.3 vTess and wvClient

    vTess allows for the examination of geometry through its discrete
    representation. Like egads2cart, the acceptable geometric input is STEP, 
    EGADS or OpenCASCADE BRep files. vTess acts like serveCSM and wvClient 
    should be used like ESP in the discussion in Section 3.1 above.

        % cd $ESP_ROOT/bin
        % ./vTess geomFilePath [angle maxlen sag]

3.4 Executing CAPS through Python

    % python pyCAPSscript.py  (Note: many example Python scripts can be 
                                     found in $ESP_ROOT/CAPSexamples/pyCAPS)

3.5 CAPS Portion of the Training

    The examples and exercises in the $ESP_ROOT/training rely on 3rd party 
    software. The PreBuilt distributions contain all executables needed to
    run the CAPS part of the training except for ParaView (which is freely
    available on the web) and Pointwise. 


4.0 Notes on 3rd Party Analyses

4.1 The AFLR suite

    Building the AFLR AIMs (AFLR2, AFLR3 and AFLR4) requires AFLR_LIB at
    10.4.2 or higher. Note that built versions of the so/DLLs can be 
    found in the PreBuilt distributions and should be able to be used with 
    ESP that you build by placing them in the $ESP_ROOT/lib directory.

4.2 Athena Vortex Lattice

    The interface to AVL is designed for V3.36, and the avl executable
    is provided in $ESP_ROOT/bin.
    
4.3 Astros and mAstros

    Both Astros and a limited version (microAstros or more simply) mAstros
    can be run with the Astros AIM. An mAstros executable is part of this
    distribution and is used with the CAPS training material. The pyCAPS 
    Astros examples use the environment variable ASTROS_ROOT (set to the 
    path where Astros can be found) to locate Astros and it's runtime files.
    If not set it defaults to mAstros execution. 

4.4 Cart3D

    The interfaces to Cart3D will only work with V1.5.5.

4.5 Fun3D

    The Fun3D AIM supports Fun3D V12.4 or higher.

4.6 Mystran

    On Windows, follow the install instructions MYSTRAN-Install-Manual.pdf
    carefully. The CAPS examples function only if MYSTRAN.ini in the
    Mystran bin directory is an empty file and the MYSTRAN_directory 
    environment variable points to the Mystran bin directory.

    Mystran currently only functions on Windows if CAPS is compiled with 
    MSVC 2015 or higher. This may be addressed in future releases. 

4.7 NASTRAN

    Nastran bdf files are only correct on Windows if CAPS is compiled with 
    MSVC 2015 or higher. This may be addressed in future releases. 

4.8 Pointwise

    The CAPS connection to Pointwise is now handled internally but requires,
    at a minimum Pointwise V18.2 R2, but V18.3 R1 is recommended. This setup
    performs automatic unstructured meshing. Note that the environment variable 
    CAPS_GLYPH is set by the ESP configure script and points to the Glyph 
    scripts that should be used with CAPS and the current release of Pointwise.

4.9 SU2

    Supported versions are: 4.1.1 (Cardinal), 5.0.0 (Raven), 6.1.0, and 6.2.0
    (Falcon). SU2 version 6.0 will work except for the use of displacements 
    in a Fluid/Structure Interaction setting.
    
4.10 xfoil

    The interface to xfoil is designed for V6.99, and the xfoil executable
    is provided in $ESP_ROOT/bin. Note that multiple 'versions' of xfoil
    6.99 have been released with differing output file formats.
