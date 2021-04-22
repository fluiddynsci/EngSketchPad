#!/bin/bash -e

stat=0
notExpectedTests=""
notRun=""
LOGFILENAME=pyCAPSlog.txt

echo
echo "================================================="
echo "Using python : `which python`"
echo "================================================="
echo


expectPythonSuccess ()
{
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    local status
    set -x
    echo "$1 test;" | tee -a $LOGFILE
    time $VALGRIND_COMMAND python $1 -verbosity=0 -workDir=$2 $3  | tee -a $LOGFILE; status=${PIPESTATUS[0]}
    set +x
    if [[ $status == 0 ]]; then
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo "CAPS verification (using pyCAPS) case $1 passed (as expected)" | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    else
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo "CAPS verification (using pyCAPS) case $1 failed (as NOT expected)" | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    stat=1
    notExpectedTests="$notExpectedTests\n$1"
    fi
}

# function to run a case that is expected to fail
expectPythonFailure ()
{
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    local status
    set -x
    echo "$1 test;" | tee -a $LOGFILE
    time $VALGRIND_COMMAND python $1 -verbosity=0 -workDir=$2 $3 | tee -a $LOGFILE; status=${PIPESTATUS[0]}
    set +x
    if [[ $status == 0 ]]; then
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo "CAPS verification (using pyCAPS) case $1 passed (as NOT expected)" | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    stat=1
    notExpectedTests="$notExpectedTests\n$1"
    else
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo "CAPS verification (using pyCAPS) case $1 failed (as expected)" | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    fi
}

# Get the current directory
curDir=${PWD##*/} 
regTestDir=${PWD}

if [[ "$curDir" != "regressionTest" || ! -d "../csmData" || ! -d "../pyCAPS" ]]; then
    echo "execute_PyTestRegression.sh must be executed in the regressionTest directory!"
    echo "Current directory: $regTestDir"
    exit 1
fi

# make the log file abosolute path to the current directory
LOGFILE=$regTestDir/$LOGFILENAME

# Remove old log file
rm -f $LOGFILE
touch $LOGFILE

# Move into the pyCAPS directory
cd ../pyCAPS

export capsPythonRegDir=runDirectory
if [[ -d "$cRegDir" ]]; then
    rm -rf $cRegDir
fi
mkdir -p $capsPythonRegDir

######### Execute pyTest examples ########

TYPE="ALL"
testsRan=0

if [ "$1" != "" ]; then
    TYPE="$1"
else
    echo "No options provided...defaulting to ALL"
fi

if [ "$OS" == "Windows_NT" ]; then
    EXT=dll
else
    EXT=so
fi

#### Some minimal fast running tests ####
if [ "$TYPE" == "MINIMAL" ]; then
    echo "Running.... MINIMAL PyTests"

    if [ "`which avl`" != "" ]; then
         expectPythonSuccess "avl_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\navl"
    fi

    if [ "`which xfoil`" != "" ]; then
        expectPythonSuccess "xfoil_PyTest.py" $capsPythonRegDir -noPlotData
    else
        notRun="$notRun\nxfoil"
    fi

    if [ "`which awave`" != "" ]; then
        expectPythonSuccess "awave_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nawave"
    fi

    if [ -f $ESP_ROOT/lib/aflr2AIM.$EXT ]; then
        expectPythonSuccess "aflr2_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nAFLR"
    fi

    if [[ "`which delaundo`" != "" || "`which delaundo.exe`" != "" ]]; then
        expectPythonSuccess "delaundo_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\ndelaundo"
    fi

    if [ "$ASTROS_ROOT" != "" ]; then
        expectPythonSuccess "astros_ThreeBar_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_Flutter_15degree.py" $capsPythonRegDir
    else
        notRun="$notRun\nAstros"
    fi

    expectPythonSuccess "masstran_PyTest.py" $capsPythonRegDir

    testsRan=1
fi

###### Linear aero. examples ######
if [[ "$TYPE" == "LINEARAERO" || "$TYPE" == "ALL" ]]; then
    echo "Running.... LINEARAERO PyTests"
    
    ###### AVL ######
    if [ "`which avl`" != "" ]; then
        expectPythonSuccess "avl_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "avl_AutoSpan_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "avl_DesignSweep_PyTest.py" $capsPythonRegDir -noPlotData
        expectPythonSuccess "avl_ControlSurf_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "avl_EigenValue_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\navl"
    fi
    
    ###### awave ######
    if [ "`which awave`" != "" ]; then
        expectPythonSuccess "awave_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nawave"
    fi
    
    ###### friction ######
    if [ "`which friction`" != "" ]; then
        expectPythonSuccess "friction_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nfriction"
    fi
    
    ###### tsFoil ######
    if [[ "`which tsfoil`" != "" && "$OS" != "Windows_NT" ]]; then
        expectPythonSuccess "tsfoil_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\ntsfoil"
    fi
    
    ###### XFoil ######
    if [ "`which xfoil`" != "" ]; then
       expectPythonSuccess "xfoil_PyTest.py" $capsPythonRegDir -noPlotData
    else
        notRun="$notRun\nxfoil"
    fi

    ###### AutoLink with awave, friction, and avl ######
    if [[ "`which awave`" != "" && "`which friction`" != "" && "`which avl`" != "" ]]; then
        expectPythonSuccess "autoLink_Pytest.py" $capsPythonRegDir -noPlotData
    fi

    testsRan=1
fi

###### Meshing examples ######
if [[ "$TYPE" == "MESH" || "$TYPE" == "ALL" ]]; then
    echo "Running.... MESH PyTests"

    # EGADS Tess
    expectPythonSuccess "egadsTess_PyTest.py" $capsPythonRegDir
    expectPythonSuccess "egadsTess_Spheres_Quad_PyTest.py" $capsPythonRegDir
    expectPythonSuccess "egadsTess_Nose_Quad_PyTest.py" $capsPythonRegDir
 
    # Example of controlling quading
    expectPythonSuccess "quading_Pytest.py" $capsPythonRegDir
    
    # AFLR
    if [[ -f $ESP_ROOT/lib/aflr2AIM.$EXT && \
          -f $ESP_ROOT/lib/aflr3AIM.$EXT && \
          -f $ESP_ROOT/lib/aflr4AIM.$EXT ]]; then
        expectPythonSuccess "aflr2_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "aflr3_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "aflr4_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "aflr4_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "aflr4_Symmetry_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "aflr4_Generic_Missile.py" $capsPythonRegDir
        expectPythonSuccess "aflr4_TipTreat_PyTest.py" $capsPythonRegDir -noPlotData
    
        if [ -f $ESP_ROOT/lib/tetgenAIM.$EXT ]; then
            expectPythonSuccess "aflr4_and_Tetgen_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "aflr4_tetgen_Regions_PyTest.py" $capsPythonRegDir
        fi
    else
        notRun="$notRun\nAFLR"
    fi
    
    # Tetgen
    if [ -f $ESP_ROOT/lib/tetgenAIM.$EXT ]; then
        expectPythonSuccess "tetgen_PyTest.py" $capsPythonRegDir

        # TODO: Sort out why this hangs on windoze...
        if [ "$OS" != "Windows_NT" ]; then
            expectPythonSuccess "tetgen_Holes_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "tetgen_Regions_PyTest.py" $capsPythonRegDir
        fi
    else
        notRun="$notRun\nTetGen"
    fi
   
    # Delaundo 
    if [[ "`which delaundo`" != "" || "`which delaundo.exe`" != "" ]]; then
        expectPythonSuccess "delaundo_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\ndelaundo"
    fi

    # pointwise
    if [[ "`which pointwise`" != "" && -f $ESP_ROOT/lib/pointwiseAIM.$EXT ]]; then
        expectPythonSuccess "pointwise_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "pointwise_Symmetry_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nPointwise"
    fi

    testsRan=1
fi

###### CFD examples ######
if [[ "$TYPE" == "CFD" || "$TYPE" == "ALL" ]]; then
    echo "Running.... CFD PyTests"
    
    if [ "$SU2_RUN" != "" ] && [ "$OS" != "Windows_NT" ]; then
        if [ -f $ESP_ROOT/lib/aflr2AIM.$EXT && \ 
             -f $ESP_ROOT/lib/aflr3AIM.$EXT ]; then
            expectPythonSuccess "su2_and_AFLR2_NodeDist_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "su2_and_AFLR2_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "su2_and_AFLR3_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "su2_and_AFLR4_AFLR3_PyTest.py" $capsPythonRegDir
        fi
        if [[ "`which delaundo`" != "" || "`which delaundo.exe`" != "" ]]; then
            expectPythonSuccess "su2_and_Delaundo_PyTest.py" $capsPythonRegDir
        fi
        if [ -f $ESP_ROOT/lib/tetgenAIM.$EXT ]; then
            expectPythonSuccess "su2_and_Tetgen_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "su2_X43a_PyTest.py" $capsPythonRegDir
        fi
    else
        notRun="$notRun\nSU2"
    fi
   
    if [ "`which flowCart`" != "" ]; then
        ulimit -s unlimited || true # Cart3D requires unlimited stack size
        expectPythonSuccess "cart_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nCart3D"
    fi

    if [ "`which nodet_mpi`" != "" ]; then
        if [ -f $ESP_ROOT/lib/aflr2AIM.$EXT && \
             -f $ESP_ROOT/lib/aflr3AIM.$EXT && \
             -f $ESP_ROOT/lib/aflr4AIM.$EXT ]; then
            expectPythonSuccess "fun3d_and_AFLR2_NodeDist_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "fun3d_and_AFLR2_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "fun3d_and_AFLR4_AFLR3_PyTest.py" $capsPythonRegDir
        fi
        if [[ "`which delaundo`" != "" || "`which delaundo.exe`" != "" ]]; then
            expectPythonSuccess "fun3d_and_Delaundo_PyTest.py" $capsPythonRegDir
        fi
        if [ -f $ESP_ROOT/lib/tetgenAIM.$EXT ]; then
            expectPythonSuccess "fun3d_and_Tetgen_AlphaSweep_PyTest.py" $capsPythonRegDir
            expectPythonSuccess "fun3d_and_Tetgen_PyTest.py" $capsPythonRegDir
        fi
        expectPythonSuccess "fun3d_and_egadsTess_ArbShape_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "fun3d_and_egadsTess_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "fun3d_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nFun3D"
    fi

    testsRan=1
fi

###### Structural examples ######
if [[ "$TYPE" == "STRUCTURE" || "$TYPE" == "ALL" ]]; then
    echo "Running.... STRUCTURE PyTests"

    ###### masstran ######
    expectPythonSuccess "masstran_PyTest.py" $capsPythonRegDir
    expectPythonSuccess "masstran_AGARD445_PyTest.py" $capsPythonRegDir

    ######  HSM ###### 
    if [ -f $ESP_ROOT/lib/hsmAIM.$EXT  ]; then
        expectPythonSuccess "hsm_SingleLoadCase_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "hsm_CantileverPlate_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nHSM"
    fi 

    ###### Astros ######
    if [ "$ASTROS_ROOT" != "" ]; then
        expectPythonSuccess "astros_Aeroelastic_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_AGARD445_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_Composite_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_CompositeWingFreq_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_Flutter_15degree.py" $capsPythonRegDir
        expectPythonSuccess "astros_MultipleLoadCase_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_SingleLoadCase_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_ThreeBarFreq_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_ThreeBarLinkDesign_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_ThreeBarMultiLoad_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_ThreeBar_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "astros_Trim_15degree.py" $capsPythonRegDir
 
        # TODO: Create these examples
        #expectPythonSuccess "astros_CompositeWingDesign_PyTest.py" $capsPythonRegDir
        #expectPythonSuccess "astros_OptimizationMultiLoad_PyTest.py" $capsPythonRegDir
        #expectPythonSuccess "astros_ThreeBarDesign_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nAstros"
    fi

    ###### Nastran ######
    if [ "`which nastran`" != "" ]; then
        expectPythonSuccess "nastran_Aeroelastic_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_AGARD445_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_Composite_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_CompositeWingDesign_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_CompositeWingFreq_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_Flutter_15degree.py" $capsPythonRegDir
        expectPythonSuccess "nastran_MultiLoadCase_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_OptimizationMultiLoad_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_SingleLoadCase_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_ThreeBarDesign_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_ThreeBarFreq_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_ThreeBarLinkDesign_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_ThreeBarMultiLoad_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_ThreeBar_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "nastran_Trim_15degree.py" $capsPythonRegDir
    else
        notRun="$notRun\nNastran"
    fi

    ###### Mystran ######
    if [ "`which mystran.exe`" != "" ]; then
        if [[ "$OS" != "Windows_NT" ]]; then
            # Mystran runs out of heap memory on windows running this example
            expectPythonSuccess "mystran_PyTest.py" $capsPythonRegDir
        fi
        expectPythonSuccess "mystran_AGARD445_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "mystran_SingleLoadCase_PyTest.py" $capsPythonRegDir
        expectPythonSuccess "mystran_MultiLoadCase_PyTest.py" $capsPythonRegDir
    else
        notRun="$notRun\nMystran"
    fi

    testsRan=1
fi

###### Aeroelastic examples ######
if [[ "$TYPE" == "AEROELASTIC" || "$TYPE" == "ALL" ]]; then
    echo "Running.... AEROELASTIC PyTests"

    ###### Astros ######
    if [ "$ASTROS_ROOT" != "" ] && [ "$SU2_RUN" != "" ] && [ "$OS" != "Windows_NT" ]; then
        # SU2 6.0.0 on Windows is no longer supported
        expectPythonSuccess "aeroelasticSimple_Pressure_SU2_and_Astros.py" $capsPythonRegDir -noPlotData
        expectPythonSuccess "aeroelasticSimple_Displacement_SU2_and_Astros.py" $capsPythonRegDir -noPlotData
        expectPythonSuccess "aeroelasticSimple_Iterative_SU2_and_Astros.py" $capsPythonRegDir

        expectPythonSuccess "aeroelastic_Iterative_SU2_and_Astros.py" $capsPythonRegDir
    else
        notRun="$notRun\nAstros"
    fi

    ###### Mystran ######
    if [ "`which mystran.exe`" != "" ] && [ "$SU2_RUN" != "" ] && [ "$OS" != "Windows_NT" ]; then
        # SU2 6.0.0 on Windows does not work with displacements
        expectPythonSuccess "aeroelasticSimple_Pressure_SU2_and_Mystran.py" $capsPythonRegDir -noPlotData
        expectPythonSuccess "aeroelasticSimple_Displacement_SU2_and_Mystran.py" $capsPythonRegDir -noPlotData
        expectPythonSuccess "aeroelasticSimple_Iterative_SU2_and_Mystran.py" $capsPythonRegDir
    else
        notRun="$notRun\nMystran"
    fi

    testsRan=1
fi

# Cleanup if we have success
if [[ $stat == 0 ]]; then
    rm -fr $capsPythonRegDir
fi

echo ""
echo "*************************************************"
echo "*************************************************"
echo ""

# Make sure something actually was executed
if [[ $testsRan == 0 ]]; then
    echo "error: unknown test selection : $TYPE"
    echo
    exit 1
fi

if [[ "$notRun" != "" ]]; then
    echo "================================================="
    echo "Did not run examples for:"
    printf $notRun
    echo ""
    echo "================================================="
fi

if [[ $stat != 0 ]]; then
    echo "================================================="
    echo "Summary of examples NOT finished as expected"
    printf $notExpectedTests
    echo ""
    echo ""
    echo "See $LOGFILENAME for more details."
    echo "================================================="
else
    echo ""
    echo "All tests pass!"
    echo ""
fi

TESSFAIL=`grep EG_fillTris $LOGFILE | wc -l`

if [[ $TESSFAIL != 0 ]]; then
    echo ""
    echo "================================================="    
    awk '/test;$/ { test = $0 }; /EG_fillTris/ { printf("%-35s EGADS Tess Failure\n", test, $0) }' $LOGFILE
    echo "================================================="
fi

# exit with the status
exit $stat
