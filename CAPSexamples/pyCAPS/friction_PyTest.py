from __future__ import print_function

## [import]
# Import capsProblem from pyCAPS
from pyCAPS import capsProblem

# Import os module
import os
import argparse
## [import]

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'FRICTION Pytest Example',
                                 prog = 'friction_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ".", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# -----------------------------------------------------------------
# Initialize capsProblem object
# -----------------------------------------------------------------
## [initateProblem]
myProblem = capsProblem()
## [initateProblem]

# -----------------------------------------------------------------
# Load CSM file
# -----------------------------------------------------------------
## [geometry]
geometryScript = os.path.join("..","csmData","frictionWingTailFuselage.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
myGeometry.setGeometryVal("area", 10.0)
## [geometry]

# Create working directory variable
## [localVariable]
workDir = os.path.join(str(args.workDir[0]), "FrictionAnalysisTest")
## [localVariable]

# -----------------------------------------------------------------
# Load desired aim
# -----------------------------------------------------------------
print ("Loading AIM")
## [loadAIM]
myAnalysis = myProblem.loadAIM(	aim = "frictionAIM",
                                analysisDir = workDir )
## [loadAIM]

# -----------------------------------------------------------------
# Set new Mach/Alt parameters
# -----------------------------------------------------------------
print ("Setting Mach & Altitude Values")
## [setInputs]
myAnalysis.setAnalysisVal("Mach", [0.5, 1.5])

# Note: friction wants kft (defined in the AIM) - Automatic unit conversion to kft
myAnalysis.setAnalysisVal("Altitude", [9000, 18200.0], units= "m")
## [setInputs]

# -----------------------------------------------------------------
# Run AIM pre-analysis
# -----------------------------------------------------------------
## [preAnalysis]
myAnalysis.preAnalysis()
## [preAnalysis]

# -----------------------------------------------------------------
# Run Friction
# -----------------------------------------------------------------
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myAnalysis.analysisDir) # Move into test directory

if (args.noAnalysis == False): # Don't run friction if noAnalysis is set
	os.system("friction frictionInput.txt frictionOutput.txt > Info.out")

os.chdir(currentDirectory) # Move back to working directory

# -----------------------------------------------------------------
# Run AIM post-analysis
# -----------------------------------------------------------------
## [postAnalysis]
myAnalysis.postAnalysis()
## [postAnalysis]

# -----------------------------------------------------------------
# Get Output Data from Friction
# -----------------------------------------------------------------
## [output]
Cdtotal = myAnalysis.getAnalysisOutVal("CDtotal")
CdForm  = myAnalysis.getAnalysisOutVal("CDform")
CdFric  = myAnalysis.getAnalysisOutVal("CDfric")
## [output]

print("Total drag =", Cdtotal )
print("Form drag =", CdForm)
print("Friction drag =", CdFric)
# -----------------------------------------------------------------
# Close CAPS
# -----------------------------------------------------------------
myProblem.closeCAPS()
