#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

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
                          analysisDir = "workDir_xfoil_1")

# Set Mach number
xfoil.setAnalysisVal("Mach", 0.5 )

# Print the modified mach number
mach = xfoil.getAnalysisVal("Mach")
print("\n==> Modified Mach =", mach)

# Print the default value of None
print("\n==> Default Alpha =", xfoil.getAnalysisVal("Alpha"))

# Set Alpha number
xfoil.setAnalysisVal("Alpha", 2.5 )
print("\n==> Modified Alpha =", xfoil.getAnalysisVal("Alpha"))

# Set list of Alpha
xfoil.setAnalysisVal("Alpha", [0.0, 3.0, 5.0, 7.0, 8.0] )
print("\n==> Modified Alpha =", xfoil.getAnalysisVal("Alpha"))

# Unset Alpha back to None
xfoil.setAnalysisVal("Alpha", None )
print("\n==> Unset Alpha =", xfoil.getAnalysisVal("Alpha"))
print()
