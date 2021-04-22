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
f118 = myProblem.loadCAPS(filename, verbosity=0)

# Load the masstran aim with all bodies
masstran = myProblem.loadAIM(aim = "masstranAIM",
                             analysisDir = "workDir_masstranCG")

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

shell_3 = {"propertyType"      : "Shell",
           "material"          : "unobtainium",
           "membraneThickness" : 0.3}

# Associate the shell property with capsGroups defined on the geometry
masstran.setAnalysisVal("Property", [("wing:faces" , shell_1), ("htail:faces", shell_1),
                                     ("fuse:nose"  , shell_1), ("fuse:tail"  , shell_1),
                                     ("vtail:faces", shell_2), ("fuse:side"  , shell_2),
                                     ("fuse:bottom", shell_3), ("fuse:top"   , shell_1)])

# run pre/post-analysis
masstran.preAnalysis()
masstran.postAnalysis()

# Get the un-modified CG location
CG = masstran.getAnalysisOutVal("CG")[0]

# Get the wing x-location
wing_xroot = f118.getGeometryVal("wing:xroot")

# Multipliers for the x-locaition
mults = [0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5]

CGs = []
for mult in mults:
    # Move the wing
    f118.setGeometryVal("wing:xroot", wing_xroot*mult)

    # run pre/post-analysis
    masstran.preAnalysis()
    masstran.postAnalysis()
    
    # Extract the mass and CG for the wing
    CGs.append(masstran.getAnalysisOutVal("CG")[0])

print()
print("-->Oringal CG =", CG)
print("-->CG sweep   =", CGs)
