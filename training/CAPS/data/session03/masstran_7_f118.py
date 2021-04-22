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

# Load the masstran aim with all the bodies
masstran = myProblem.loadAIM(aim = "masstranAIM", 
                             analysisDir = "workDir_masstranAll")

# Define material properties
unobtainium = {"density" : 7850}
madeupium   = {"density" : 6890}

# Set the materials
masstran.setAnalysisVal("Material", [("Unobtainium", unobtainium),
                                     ("Madeupium", madeupium)])

# Define shell properties
shell_1 = {"propertyType"      : "Shell",
           "material"          : "unobtainium",
           "membraneThickness" : 0.2} 

shell_2 = {"propertyType"      : "Shell",
           "material"          : "madeupium",
           "membraneThickness" : 0.3}

# Associate the shell property with capsGroups defined on the geometry
masstran.setAnalysisVal("Property", [("wing:faces" , shell_1), ("htail:faces", shell_1), 
                                     ("fuse:nose"  , shell_1), ("fuse:tail"  , shell_1),
                                     ("vtail:faces", shell_2), ("fuse:side"  , shell_2)])

# run pre/post-analysis
masstran.preAnalysis()
masstran.postAnalysis()

# Extract the mass and CG for the wing
Mass = masstran.getAnalysisOutVal("Mass")
CG   = masstran.getAnalysisOutVal("CG")

print()
print("-->Mass =", Mass)
print("-->CG   =", CG)
