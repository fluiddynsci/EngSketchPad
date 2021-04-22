#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os
import os

#------------------------------------------------------------------------------#

def run_xfoil(xfoil):
    # Run AIM pre-analysis
    print ("\n==> Running preAnalysis")
    xfoil.preAnalysis()

    ####### Run xfoil ####################
    print ("\n\n==> Running xFoil......")

    currentDirectory = os.getcwd() # Get current working directory
    os.chdir(xfoil.analysisDir)    # Move into test directory

    # Run xfoil via system call
    os.system("xfoil < xfoilInput.txt > Info.out");

    os.chdir(currentDirectory)     # Move back to top directory
    ######################################

    # Run AIM post-analysis
    print ("\n==> Running postAnalysis")
    xfoil.postAnalysis()

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = os.path.join("..","..","data","session03","naca.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
naca = myProblem.loadCAPS(filename)

# Load xfoil aim
print ("\n==> Loading xfoilAIM")
xfoil = myProblem.loadAIM(aim = "xfoilAIM",
                          analysisDir = "workDir_xfoil_Thick")

print ("\n==> Setting analysis values")
# Set Mach and Reynolds number
xfoil.setAnalysisVal("Mach", 0.5 )
xfoil.setAnalysisVal("Re", 1.0E6 )

# Set list of Alpha
xfoil.setAnalysisVal("Alpha", [0.0, 1.0, 3.0] )

# List of thicknesses to analyze
Thickness = [0.08, 0.10, 0.12, 0.16]

Alpha = []; Cl = []; Cd = []
for thick in Thickness:
    # Modify the thickness
    naca.setGeometryVal("thick", thick)

    # Run xfoil
    run_xfoil(xfoil)

    # Append Alpha, Cl and Cd
    print ("\n==> Retrieve analysis results")
    Alpha.append(xfoil.getAnalysisOutVal("Alpha"))
    Cl.append(xfoil.getAnalysisOutVal("CL"))
    Cd.append(xfoil.getAnalysisOutVal("CD"))

print()
print("--> Thickness =", Thickness)
print("--> Alpha     =", Alpha)
print("--> Cl        =", Cl)
print("--> Cd        =", Cd)
print()
