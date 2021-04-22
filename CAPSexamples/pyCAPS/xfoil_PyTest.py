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
parser = argparse.ArgumentParser(description = 'xFoil Pytest Example',
                                 prog = 'xfoil_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot data")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
## [initateProblem]
myProblem = capsProblem()
## [initateProblem]

# Create working directory variable
## [localVariable]
workDir = "xFoilAnalysisTest"
## [localVariable]
workDir = os.path.join(str(args.workDir[0]), workDir)

# Load CSM file
## [loadGeom]
geometryScript = os.path.join("..","csmData","airfoilSection.csm")
myGeometry = myProblem.loadCAPS(geometryScript)
## [loadGeom]

# Change a design parameter - area in the geometry
## [setGeom]
myGeometry.setGeometryVal("camber", 0.1)
## [setGeom]

# Load xfoil aim
## [loadXFOIL]
xfoil = myProblem.loadAIM(aim = "xfoilAIM",
                          analysisDir = workDir,
                          capsIntent = "LINEARAERO")
## [loadXFOIL]

##[setXFOIL]
# Set Mach number, Reynolds number
xfoil.setAnalysisVal("Mach", 0.5 )
xfoil.setAnalysisVal("Re", 1.0E6 )

# Set custom AoA
xfoil.setAnalysisVal("Alpha", [0.0, 3.0, 5.0, 7.0, 9.0, 11, 13, 14, 15.0])

# Set AoA seq
xfoil.setAnalysisVal("Alpha_Increment", [1.0, 2.0, 0.10])

# Set custom Cl
xfoil.setAnalysisVal("CL", 0.1)

# Set Cl seq
xfoil.setAnalysisVal("CL_Increment", [0.8, 3, .25])
##[setXFOIL]

# Append the polar file if it already exists - otherwise the AIM will delete the file
xfoil.setAnalysisVal("Append_PolarFile", True)

# Run AIM pre-analysis
## [preAnalysiXFOIL]
xfoil.preAnalysis()
## [preAnalysiXFOIL]

####### Run xfoil ####################
print ("\n\nRunning xFoil......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(xfoil.analysisDir) # Move into test directory

if (args.noAnalysis == False): # Don't run xfoil if noAnalysis is set
    os.system("xfoil < xfoilInput.txt > Info.out"); # Run xfoil via system call

os.chdir(currentDirectory) # Move back to top directory

# Run AIM post-analysis
## [postAnalysiXFOIL]
xfoil.postAnalysis()
## [postAnalysiXFOIL]

## [results]
# Retrieve Cl and Cd
Cl = xfoil.getAnalysisOutVal("CL")
print("Cl = ", Cl)

Cd = xfoil.getAnalysisOutVal("CD")
print("Cd = ", Cd)

# Angle of attack
Alpha = xfoil.getAnalysisOutVal("Alpha")
print("Alpha = ", Alpha)

# Transition location
TranX = xfoil.getAnalysisOutVal("Transition_Top")
print("Transition location = ", TranX)
## [results]

if (args.noAnalysis == False and args.noPlotData == False):
    # Import pyplot module
    try:
        from matplotlib import pyplot as plt
    except:
        print ("Unable to import matplotlib.pyplot module. Drag polar will not be plotted")
        plt = None

    # Plot data
    if plt is not None:

        # Plot first sweep angle
        plt.plot(Alpha,
                 Cl,
                 'o',
                  label = "Cl")

        plt.plot(Alpha,
                 Cd,
                 'x',
                 label="Cd")

        plt.legend(loc = "upper left")
        plt.title("Lift and Drag Coefficients for Various Angles of Attack\n (window must be closed to terminate Python script)")
        plt.xlabel("$Angle of Attack ^o$")
        plt.ylabel("$C_L, C_D$")
        plt.show()

# Close CAPS
## [closeCAPS]
myProblem.closeCAPS()
## [closeCAPS]

# Check assertation
assert abs(Cl[0]-1.2455) <= 1E-4 and abs(Cd[0]-0.01565) <= 1E-4
