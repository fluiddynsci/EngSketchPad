## [importPrint]
from __future__ import print_function
## [importPrint]

# Import pyCAPS and os module
## [import]
import pyCAPS

import os
## [import]

import argparse
# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AVL Eigen Value Pytest Example',
                                 prog = 'avl_EigenValue_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# -----------------------------------------------------------------
# Initialize capsProblem object
# -----------------------------------------------------------------
## [initateProblem]
myProblem = pyCAPS.capsProblem()
## [initateProblem]

# -----------------------------------------------------------------
# Load CSM file and Change a design parameter - area in the geometry
# Any despmtr from the avlWing.csm file are available inside the pyCAPS script
# They are: thick, camber, area, aspect, taper, sweep, washout, dihedral
# -----------------------------------------------------------------

## [geometry]
geometryScript = os.path.join("..","csmData","avlWing.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

myGeometry.setGeometryVal("area", 10.0)
## [geometry]

# Create working directory variable
## [localVariable]
workDir = "AVLEigenTest"
## [localVariable]
workDir = os.path.join(str(args.workDir[0]), workDir)

# -----------------------------------------------------------------
# Load desired aim
# -----------------------------------------------------------------
print ("Loading AIM")
## [loadAIM]
myAnalysis = myProblem.loadAIM(aim = "avlAIM",
                               analysisDir = workDir)
## [loadAIM]
# -----------------------------------------------------------------
# Also available are all aimInput values
# Set new Mach/Alt parameters
# -----------------------------------------------------------------

## [setInputs]
myAnalysis.setAnalysisVal("Mach", 0.5)
myAnalysis.setAnalysisVal("Alpha", 1.0)
myAnalysis.setAnalysisVal("Beta", 0.0)

wing = {"groupName"    : "Wing", # Notice Wing is the value for the capsGroup attribute
        "numChord"     : 8,
        "spaceChord"   : 1.0,
        "numSpanTotal" : 24,
        "spaceSpan"    : 1.0}

myAnalysis.setAnalysisVal("AVL_Surface", [("Wing", wing)])

mass = 0.1773
x    =  0.02463
y    = 0.
z    = 0.2239
Ixx  = 1.350
Iyy  = 0.7509
Izz  = 2.095

myAnalysis.setAnalysisVal("Lunit", 1, units="m")
myAnalysis.setAnalysisVal("MassProp", ("Aircraft",{"mass":[mass,"kg"], "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}))
myAnalysis.setAnalysisVal("Gravity", 32.18, units="ft/s^2")
myAnalysis.setAnalysisVal("Density", 0.002378, units="slug/ft^3")
myAnalysis.setAnalysisVal("Velocity", 64.5396, units="ft/s")
## [setInputs]

# -----------------------------------------------------------------
# Run AIM pre-analysis
# -----------------------------------------------------------------
## [preAnalysis]
myAnalysis.preAnalysis()
## [preAnalysis]

# -----------------------------------------------------------------
# Run AVL
# -----------------------------------------------------------------
## [runAVL]
print ("Running AVL")
currentDirectory = os.getcwd() # Get our current working directory
os.chdir(myAnalysis.analysisDir) # Move into test directory

os.system("avl caps < avlInput.txt > avlOutput.txt");

os.chdir(currentDirectory) # Move back to working directory
## [runAVL]

# -----------------------------------------------------------------
# Run AIM post-analysis
# -----------------------------------------------------------------
## [postAnalysis]
myAnalysis.postAnalysis()
## [postAnalysis]

# -----------------------------------------------------------------
# Get Output Data from AVL
# These calls access aimOutput data
# -----------------------------------------------------------------

## [eigen values]
EigenValues = myAnalysis.getAnalysisOutVal("EigenValues")
print ("EigenValues ", EigenValues)
## [eigen values]

# -----------------------------------------------------------------
# Close CAPS
# -----------------------------------------------------------------
myProblem.closeCAPS()
