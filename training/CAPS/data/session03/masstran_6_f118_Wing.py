#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = "f118-C.csm"
print ("\n==> Loading geometry from file \""+filename+"\"...")
myProblem.loadCAPS(filename, verbosity=0)

# Load the masstran aim with the wing
masstran = myProblem.loadAIM(aim = "masstranAIM", 
                             analysisDir = "workDir_masstranWing",
                             capsIntent="wing")

# Define material properties
unobtainium = {"density" : 7850}

# Set the material
masstran.setAnalysisVal("Material", ("Unobtainium", unobtainium))

# Define shell property
shell = {"propertyType"      : "Shell",
         "material"          : "Unobtainium",
         "membraneThickness" : 0.2} 

# Associate the shell property with capsGroups defined on the geometry
masstran.setAnalysisVal("Property", ("wing:faces", shell))

# run pre/post-analysis to execute masstran
masstran.preAnalysis()
masstran.postAnalysis()

# Extract the mass and CG for the wing
Mass = masstran.getAnalysisOutVal("Mass")
CG   = masstran.getAnalysisOutVal("CG")

print()
print("--> Mass =", Mass)
print("--> CG   =", CG)
