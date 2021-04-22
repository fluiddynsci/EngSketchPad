#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem
from pyCAPS import CAPSError

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
filename = "naca.csm"
print ("\n==> Loading geometry from file \""+filename+"\"...")
myProblem.loadCAPS(filename)

# Load xfoil aim
print ("\n==> Loading xfoilAIM")
xfoil = myProblem.loadAIM(aim = "xfoilAIM",
                          analysisDir = "workDir_xfoil_3")

print("\n1. No Errors ", "-"*80)

# Set Mach and Reynolds number
xfoil.setAnalysisVal("Mach", 0.5 )
xfoil.setAnalysisVal("Re", 1.0E6 )

# Set list of Alpha
print("\n==> Setting alpha sequence")
xfoil.setAnalysisVal("Alpha", [0.0, 3.0, 5.0, 7.0, 8.0] )

# Run xfoil
run_xfoil(xfoil)

# Retrieve Cl
Cl = xfoil.getAnalysisOutVal("CL")
print("\n--> Cl    =", Cl)

print("\n2. DIRTY AnalysisVal Error ", "-"*80)

# Set a new alphas
print("\n==> Setting new alpha sequence")
xfoil.setAnalysisVal("Alpha", [1.0, 2.0] )

# Try to retrieve Cl without executing pre/postAnalysis
print("\n==> Attempting to get Cl")
try:
    Cl = xfoil.getAnalysisOutVal("CL")
    print("\n--> Cl    =", Cl)
except CAPSError as e:
    print("\n==> CAPSError =", e)

# Run xfoil
run_xfoil(xfoil)

# Retrieve Cl
print("\n==> Attempting to get Cl")
Cl = xfoil.getAnalysisOutVal("CL")
print("\n--> Cl    =", Cl)

print("\n3. DIRTY GeometryVal Error ", "-"*80)

# Modify a geometric parameter
print("\n==> Modifying camber")
myProblem.geometry.setGeometryVal("camber", 0.07)

# Try to retrieve Cl without executing pre/postAnalysis
print("\n==> Attempting to get Cl")
try:
    Cl = xfoil.getAnalysisOutVal("CL")
    print("\n--> Cl    =", Cl)
except CAPSError as e:
    print("\n==> CAPSError =", e)

# Run xfoil
run_xfoil(xfoil)

# Retrieve Cl
print("\n==> Attempting to get Cl")
Cl = xfoil.getAnalysisOutVal("CL")
print("\n--> Cl    =", Cl)

print("\n4. DIRTY pre- but no postAnalysis Error ", "-"*80)

# Modify mach number
print("\n==> Modifying Mach")
xfoil.setAnalysisVal("Mach", 0.3 )

# Run AIM pre-analysis
print ("\n==> Running preAnalysis but not running postAnalysis")
xfoil.preAnalysis()

# Retrieve Cl
print("\n==> Attempting to get Cl")
try:
    Cl = xfoil.getAnalysisOutVal("CL")
    print("\n--> Cl    =", Cl)
except CAPSError as e:
    print("\n==> CAPSError =", e)

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

# Retrieve Cl
print("\n==> Attempting to get Cl")
Cl = xfoil.getAnalysisOutVal("CL")
print("\n--> Cl    =", Cl)

print("\n5. CLEAN Error ", "-"*80)

# Don't modify any analysis or geometry values

try:
    # Run xfoil
    run_xfoil(xfoil)
except CAPSError as e:
    print("\n==> CAPSError =", e)
