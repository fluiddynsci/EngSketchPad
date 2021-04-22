#!/bin/bash -e

stat=0
notExpectedTests=""
notRun=""
LOGFILENAME=cCAPSlog.txt

expectCSuccess ()
{
    # $1 = program
    # $2 = execution directory
    # $3 = run analysis 
     
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    local status
    set -x
    echo "$1 test;" | tee -a $LOGFILE
    time $VALGRIND_COMMAND $1 $2 $3 | tee -a $LOGFILE; status=${PIPESTATUS[0]}
    set +x
    if [[ $status == 0 ]]; then
    echo | tee -a $LOGFILE
    echo "=============================================" | tee -a $LOGFILE
    echo "CAPS verification (using c-executables) case $1 passed (as expected)" | tee -a $LOGFILE
    echo "=============================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    else
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo "CAPS verification (using c-executables) case $1 failed (as NOT expected)" | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    stat=1
    notExpectedTests="$notExpectedTests\n$1"
    fi
}

# function to run a case that is expected to fail
expectCFailure ()
{
    # $1 = program
    # $2 = execution directory
    # $3 = run analysis 
    
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    local status
    echo "$1 test;" | tee -a $LOGFILE
    set -x
    time $VALGRIND_COMMAND $1 $2 $3 | tee -a $LOGFILE; status=${PIPESTATUS[0]}
    set +x
    if [[ $status == 0 ]]; then
    echo | tee -a $LOGFILE
    echo "=============================================" | tee -a $LOGFILE
    echo "CAPS verification (using c-executables) case $1 passed (as NOT expected)" | tee -a $LOGFILE
    echo "=============================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    stat=1
    notExpectedTests="$notExpectedTests\n$1"
    else
    echo | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo "CAPS verification (using c-executables) case $1 failed (as expected)" | tee -a $LOGFILE
    echo "=================================================" | tee -a $LOGFILE
    echo | tee -a $LOGFILE
    fi
}

# Get the current directory
curDir=${PWD##*/} 
regTestDir=${PWD}


if [[ "$curDir" != "regressionTest" || ! -d "../csmData" || ! -d "../cCAPS" ]]; then
  echo "execute_CTestRegression.sh must be executed in the regressionTest directory!"
  echo "Current directory: $regTestDir"
  exit 1
fi

# make the log file abosolute path to the current directory
LOGFILE=$regTestDir/$LOGFILENAME

# Remove old log file
rm -f $LOGFILE
touch $LOGFILE

# Move into the cCAPS directory
cd ../cCAPS

export cRegDir=./runDirectory
if [[ -d "$cRegDir" ]]; then
  rm -rf $cRegDir
fi
mkdir -p $cRegDir

######### Execute c-Test examples ########

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

###################################################
if [[ "$TYPE" == "MINIMAL" || "$TYPE" == "ALL" ]]; then
    echo "Running.... MINIMAL c-Tests"
    testsRan=1
fi

###### Linear aero. examples ######
if [[ "$TYPE" == "LINEARAERO" || "$TYPE" == "ALL" ]]; then
    echo "Running.... LINEARAERO c-Tests"
    
    ###### AVL ###### 
    if [ "`which avl`" != "" ]; then
        expectCSuccess "./avlTest" $cRegDir "1" 
    else
        cp $regTestDir/datafiles/capsTotalForce.txt ${cRegDir}/
        cp $regTestDir/datafiles/capsStripForce.txt ${cRegDir}/
        expectCSuccess "./avlTest" $cRegDir "0" 
    fi

    ######  FRICTION ###### 
    if [ "`which friction`" != "" ]; then
        expectCSuccess "./frictionTest" $cRegDir "1"
    else
        cp $regTestDir/datafiles/frictionOutput.txt ${cRegDir}/
        expectCSuccess "./frictionTest" $cRegDir "0"
    fi

    testsRan=1
fi

###### Meshing examples ######
if [[ "$TYPE" == "MESH" || "$TYPE" == "ALL" ]]; then
    echo "Running.... MESH c-Tests"

   # if [ "$AFLR" != "" ]; then
   #     
   #     if [ "$TETGEN" != "" ]; then
   #     fi
   # fi
    
   # if [ "$TETGEN" != "" ]; then
   # fi

    if [ "$OS" == "Windows_NT" ]; then
        POINTWISE_AIM=pointwiseAIM.dll
    else
        POINTWISE_AIM=pointwiseAIM.so
    fi
    
    # pointwise
    #if [[ "`which pointwise`" != "" && "$VALGRIND_COMMAND" == "" && -f $ESP_ROOT/lib/$POINTWISE_AIM ]]; then
    #    expectCSuccess "./pointwiseTest" $cRegDir "0" 
    #else
        notRun="$notRun\npointwise"
    #fi

    testsRan=1
fi

###### CFD examples ######
if [[ "$TYPE" == "CFD" || "$TYPE" == "ALL" ]]; then
    echo "Running.... CFD c-Tests"
    
    ###### FUN3D ######
    expectCSuccess "./fun3dTest" $cRegDir

    if [ "$AFLR" != "" ]; then

        expectCSuccess "./fun3dAFLR2Test" $cRegDir 
        expectCSuccess "./fun3dAFLR4Test" $cRegDir 
    else
        notRun="$notRun\nAFLR"
    fi

    if [ "$TETGEN" != "" ]; then
        expectCSuccess "./fun3dTetgenTest" $cRegDir 
    else
        notRun="$notRun\nTetGen"
    fi

    testsRan=1
fi

###### Structural examples ######
if [[ "$TYPE" == "STRUCTURE" || "$TYPE" == "ALL" ]]; then
    echo "Running.... STRUCTURE c-Tests"
    
    ######  Mystran ###### 
    if [ "`which mystran.exe`" != "" ] && [ "$OS" != "Windows_NT" ]; then
        expectCSuccess "./mystranTest" $cRegDir "1" 
    else
        notRun="$notRun\nMystran"
    fi 

    ######  HSM ###### 
    if [ -f $ESP_ROOT/lib/hsmAIM.$EXT  ]; then
        expectCSuccess "./hsmCantileverPlateTest" $cRegDir 
        expectCSuccess "./hsmSimplePlateTest" $cRegDir 
        expectCSuccess "./hsmJoinedPlateTest" $cRegDir 
    else
        notRun="$notRun\nHSM"
    fi 

    testsRan=1
fi

###### Aeroelastic examples ######
if [[ "$TYPE" == "AEROELASTIC" || "$TYPE" == "ALL" ]]; then
    echo "Running.... AEROELASTIC c-Tests"
    
    if [ "$TETGEN" != "" ]; then
    
        if [[ "`which mystran.exe`" != "" && "`which nodet_mpi`" != "" ]]; then
           expectCSuccess "./aeroelasticTest" $cRegDir "1" 
        else
           cp $regTestDir/datafiles/aeroelasticSimple_body1.dat ${cRegDir}/
           cp $regTestDir/datafiles/aeroelasticSimple_ddfdrive_bndry3.dat ${cRegDir}/
           cp $regTestDir/datafiles/aeroelasticSimple.F06 ${cRegDir}/
           expectCSuccess "./aeroelasticTest" $cRegDir "0"
        fi

            # SU2 6.0.0 on Windows does not work with displacements
        if [[ "`which mystran.exe`" != "" && "$OS" != "Windows_NT" ]]; then
           expectCSuccess "./aeroelasticSimple_Iterative_SU2_and_MystranTest" $cRegDir "1" 
        fi
        
    fi

    testsRan=1
fi


# Cleanup if we have success
if [[ $stat == 0 ]]; then
    rm -fr $cRegDir
fi

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
