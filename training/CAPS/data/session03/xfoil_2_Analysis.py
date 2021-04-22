#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os
import os

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = "naca.csm"
print ("\n==> Loading geometry from file \""+filename+"\"...")
myProblem.loadCAPS(filename)

# Load xfoil aim
print ("\n==> Loading xfoilAIM")
xfoil = myProblem.loadAIM(aim = "xfoilAIM",
                          analysisDir = "workDir_xfoil_2")

print ("\n==> Setting analysis values")
# Set Mach and Reynolds number
xfoil.setAnalysisVal("Mach", 0.5 )
xfoil.setAnalysisVal("Re", 1.0E6 )

# Set list of Alpha
xfoil.setAnalysisVal("Alpha", [0.0, 3.0, 5.0, 7.0, 8.0] )


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


# Retrieve Alpha, Cl and Cd
print ("\n==> Retrieve analysis results")
Alpha = xfoil.getAnalysisOutVal("Alpha")
Cl    = xfoil.getAnalysisOutVal("CL")
Cd    = xfoil.getAnalysisOutVal("CD")

print()
print("--> Alpha =", Alpha)
print("--> Cl    =", Cl)
print("--> Cd    =", Cd)
print()
