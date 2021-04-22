# Import other need moduls
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
parser = argparse.ArgumentParser(description = 'Masstran Pytest Example',
                                 prog = 'masstran_PyTest',
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

# Load CSM file
## [loadGeom]
geometryScript = os.path.join("..","csmData","feaWingBEM.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
## [loadGeom]

## [structureMesh]
# Load egadsTess aim
myProblem.loadAIM( aim = "egadsTessAIM",
                   altName = "tess" )

# All triangles in the grid
myProblem.analysis["tess"].setAnalysisVal("Mesh_Elements", "Quad")

# Set global tessellation parameters
myProblem.analysis["tess"].setAnalysisVal("Tess_Params", [.05,.5,15])

# Generate the surface mesh
myProblem.analysis["tess"].preAnalysis()
myProblem.analysis["tess"].postAnalysis()
## [structureMesh]


# Load masstran aim
## [loadMasstran]
masstranAIM = myProblem.loadAIM(aim = "masstranAIM",
                               altName = "masstran",
                               parents = ["tess"] )
## [loadMasstran]

##[setInputs]
# Set meshing inputs
myProblem.analysis["masstran"].setAnalysisVal("Edge_Point_Min", 2)
myProblem.analysis["masstran"].setAnalysisVal("Edge_Point_Max", 3)

myProblem.analysis["masstran"].setAnalysisVal("Quad_Mesh", True)
##[setInputs]

##[setTuple]
# Set materials
unobtainium  = {"youngModulus" : 2.2E11 ,
                "poissonRatio" : .33,
                "density"      : 7850}

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E9 ,
                "poissonRatio" : .5,
                "density"      : 7850}
masstranAIM.setAnalysisVal("Material", [("Unobtainium", unobtainium),
                                        ("Madeupium", madeupium)])

# Set property
shell  = {"propertyType"      : "Shell",
          "membraneThickness" : 0.2,
          "bendingInertiaRatio" : 1.0, # Default
          "shearMembraneRatio"  : 5.0/6.0} # Default }

masstranAIM.setAnalysisVal("Property", ("Ribs_and_Spars", shell))
##[setTuple]

# Run AIM pre/post-analysis to perform the calculation
## [prepostAnalysis]
masstranAIM.preAnalysis()
masstranAIM.postAnalysis()
## [prepostAnalysis]

## [results]
# Get mass properties
print ("\nGetting results mass properties.....\n")
Area     = masstranAIM.getAnalysisOutVal("Area")
Mass     = masstranAIM.getAnalysisOutVal("Mass")
Centroid = masstranAIM.getAnalysisOutVal("Centroid")
CG       = masstranAIM.getAnalysisOutVal("CG")
Ixx      = masstranAIM.getAnalysisOutVal("Ixx")
Iyy      = masstranAIM.getAnalysisOutVal("Iyy")
Izz      = masstranAIM.getAnalysisOutVal("Izz")
Ixy      = masstranAIM.getAnalysisOutVal("Ixy")
Ixz      = masstranAIM.getAnalysisOutVal("Ixz")
Iyz      = masstranAIM.getAnalysisOutVal("Iyz")
I        = masstranAIM.getAnalysisOutVal("I_Vector")
II       = masstranAIM.getAnalysisOutVal("I_Tensor")

print("Area     ", Area)
print("Mass     ", Mass)
print("Centroid ", Centroid)
print("CG       ", CG)
print("Ixx      ", Ixx)
print("Iyy      ", Iyy)
print("Izz      ", Izz)
print("Ixy      ", Ixy)
print("Ixz      ", Ixz)
print("Iyz      ", Iyz)
print("I        ", I)
print("II       ", II)
## [results]

# Close CAPS
## [closeCAPS]
myProblem.closeCAPS()
## [closeCAPS]
