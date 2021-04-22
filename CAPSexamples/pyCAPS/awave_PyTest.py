from __future__ import print_function

import argparse

## [import]
# Import capsProblem from pyCAPS
from pyCAPS import capsProblem

# Import os module
import os
## [import]

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Awave Pytest Example',
                                 prog = 'awave_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ".", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize myProblem object
## [initateProblem]
myProblem = capsProblem()
## [initateProblem]

# Load CSM file
## [geometry]
geometryScript = os.path.join("..","csmData","awaveWingTailFuselage.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
myGeometry.setGeometryVal("area", 10.0)
## [geometry]

# Create working directory variable
## [localVariable]
workDir = "AwaveAnalysisTest"
## [localVariable]
workDir = os.path.join(str(args.workDir[0]), workDir)

# Load desired aim
## [loadAIM]
myAnalysis = myProblem.loadAIM(	aim = "awaveAIM",
                                analysisDir = workDir )
## [loadAIM]

# Set new Mach and Angle of Attack parameters
## [setInputs]
myAnalysis.setAnalysisVal("Mach" , [ 1.2, 1.5])
myAnalysis.setAnalysisVal("Alpha", [ 0.0, 2.0])
## [setInputs]

# Run AIM pre-analysis
## [preAnalysis]
myAnalysis.preAnalysis()
## [preAnalysis]

# Run AIM
print (" Running AWAVE ")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myAnalysis.analysisDir) # Move into test directory

if (args.noAnalysis == False): # Don't run friction if noAnalysis is set
    os.system("awave awaveInput.txt > Info.out");

os.chdir(currentDirectory) # Move back to working directory

# Run AIM post-analysis
print (" Running Post Analysis")
## [postAnalysis]
myAnalysis.postAnalysis()
## [postAnalysis]

## [output]
CdWave = myAnalysis.getAnalysisOutVal("CDwave");
## [output]

print("CdWave = ", CdWave)

# Close CAPS
myProblem.closeCAPS()

# Check assertation
assert abs(CdWave[0]-1.133332) <= 1E-3
assert abs(CdWave[1]-0.216380) <= 1E-4
