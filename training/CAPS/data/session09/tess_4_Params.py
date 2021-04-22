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
filename = os.path.join("..","ESP","wing3.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
wing = myProblem.loadCAPS(filename)

# Set geometry variables for structural IML/OML
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:BoxStructure"    , 0)

# Load AFLR4 aim
tess = myProblem.loadAIM(aim = "egadsTessAIM",
                         analysisDir = "workDir_Tess")

# Dissable TFI and Templates that generate "structured" triangular meshes
tess.setAnalysisVal("TFI_Templates", False)

maxLength = 0.10 # bound on maximum segment length (0 - any length)
deviation = 0.01 # deviation from triangle centroid to geometry
dihedral  = 15   # maximum interior dihedral angle between triangle facets

# Set EGADS body tessellation parameters
tess.setAnalysisVal("Tess_Params", [maxLength, deviation, dihedral])

# Impact of chaning deviation
for dev in [0.01, 0.005, 0.001]:
    tess.setAnalysisVal("Tess_Params", [0, dev, 30])
    
    # Run AIM pre/post-analysis
    tess.preAnalysis()
    tess.postAnalysis()
    
    # View the tessellation
    tess.viewGeometry()

