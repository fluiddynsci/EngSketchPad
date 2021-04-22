## [importPrint]
from __future__ import print_function
## [importPrint]

# Import pyCAPS and os module
## [import]
import pyCAPS
import os
import argparse
## [import]

## [argparse]
# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Cart3D Pytest Example',
                                 prog = 'cart_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "cart3dTest")
## [argparse]

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
geometryScript = os.path.join("..","csmData","cfd_airfoilSection.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
## [geometry]

# -----------------------------------------------------------------
# Load desired aim
# -----------------------------------------------------------------
print ("Loading AIM")
## [loadAIM]
myAnalysis = myProblem.loadAIM(aim = "cart3dAIM",
                               analysisDir = workDir)
## [loadAIM]
# -----------------------------------------------------------------
# Also available are all aimInput values
# Set new Mach/Alt parameters
# -----------------------------------------------------------------

## [setInputs]
myAnalysis.setAnalysisVal("Mach", 0.95)
myAnalysis.setAnalysisVal("alpha", 2.0)
myAnalysis.setAnalysisVal("maxCycles", 10)
myAnalysis.setAnalysisVal("nDiv", 6)
myAnalysis.setAnalysisVal("maxR", 12)


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
print ("Running CART3D")
currentDirectory = os.getcwd() # Get our current working directory
os.chdir(myAnalysis.analysisDir) # Move into test directory
os.system("flowCart -v -T");
os.chdir(currentDirectory) # Move back to working directory
## [runAVL]

# -----------------------------------------------------------------
# Run AIM post-analysis
# -----------------------------------------------------------------
## [postAnalysis]
myAnalysis.postAnalysis()
## [postAnalysis]


print ("C_A  " + str(myAnalysis.getAnalysisOutVal("C_A")))
print ("C_Y  " + str(myAnalysis.getAnalysisOutVal("C_Y")))
print ("C_N  " + str(myAnalysis.getAnalysisOutVal("C_N")))
print ("C_D  " + str(myAnalysis.getAnalysisOutVal("C_D")))
print ("C_S  " + str(myAnalysis.getAnalysisOutVal("C_S")))
print ("C_L  " + str(myAnalysis.getAnalysisOutVal("C_L")))
print ("C_l  " + str(myAnalysis.getAnalysisOutVal("C_l")))
print ("C_m  " + str(myAnalysis.getAnalysisOutVal("C_m")))
print ("C_n  " + str(myAnalysis.getAnalysisOutVal("C_n")))
print ("C_M_x  " + str(myAnalysis.getAnalysisOutVal("C_M_x")))
print ("C_M_y  " + str(myAnalysis.getAnalysisOutVal("C_M_y")))
print ("C_M_z  " + str(myAnalysis.getAnalysisOutVal("C_M_z")))

## [output]
# -----------------------------------------------------------------
# Close CAPS
# -----------------------------------------------------------------
myProblem.closeCAPS()
