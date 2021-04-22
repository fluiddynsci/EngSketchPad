# Import other need modules
## [importModules]
from __future__ import print_function

import os
import argparse
## [importModules]

# Import pyCAPS class file
## [importpyCAPS]
from pyCAPS import capsProblem
## [importpyCAPS]

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'tsFoil Pytest Example',
                                 prog = 'tsfoil_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
## [initateProblem]
myProblem = capsProblem()
## [initateProblem]

# Create working directory variable
## [localVariable]
workDir = os.path.join(str(args.workDir[0]), "tsFoilAnalysisTest")
## [localVariable]

# Load CSM file
## [loadGeom]
geometryScript = os.path.join("..","csmData","airfoilSection.csm")
myGeometry = myProblem.loadCAPS(geometryScript)
## [loadGeom]

# Change a design parameter - area in the geometry
## [setGeom]
myGeometry.setGeometryVal("camber", 0.05)
## [setGeom]

# Load tsfoil aim
## [loadTSFOIL]
tsfoil = myProblem.loadAIM(aim = "tsfoilAIM",
                          analysisDir = workDir)
## [loadTSFOIL]

##[setTSFOIL]
# Set Mach number, Reynolds number
tsfoil.setAnalysisVal("Mach", .6 )
#tsfoil.setAnalysisVal("Re", 1.0E6 )

# Set custom AoA
tsfoil.setAnalysisVal("Alpha", 0.0)

##[setTSFOIL]

# Run AIM pre-analysis
## [preAnalysiTSFOIL]
tsfoil.preAnalysis()
## [preAnalysiTSFOIL]

####### Run tsfoil ####################
print ("\n\nRunning tsFoil......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(tsfoil.analysisDir) # Move into test directory

if (args.noAnalysis == False): # Don't run tsfoil if noAnalysis is set
    os.system("tsfoil2 < tsfoilInput.txt > Info.out"); # Run tsfoil via system call

os.chdir(currentDirectory) # Move back to top directory

# Run AIM post-analysis
## [postAnalysiTSFOIL]
tsfoil.postAnalysis()
## [postAnalysiTSFOIL]

## [results]
print ("Getting results")
# Retrieve Cl and Cd
Cl = tsfoil.getAnalysisOutVal("CL")
print("Cl = ", Cl)

Cd = tsfoil.getAnalysisOutVal("CD")
print("Cd = ", Cd)

# Wave drag
Cd_Wave = tsfoil.getAnalysisOutVal("CD_Wave")
print("Cd Wave = ", Cd_Wave)

# Momement coeff
Cm = tsfoil.getAnalysisOutVal("CM")
print("Cm = ", Cm)

# Critical Cp
Cp = tsfoil.getAnalysisOutVal("Cp_Critical")
print("Cp _Critcal = ", Cp)

## [results]

# Close CAPS
## [closeCAPS]
myProblem.closeCAPS()
## [closeCAPS]
