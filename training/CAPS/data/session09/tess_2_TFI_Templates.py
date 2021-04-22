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

# Run AIM pre/post-analysis
tess.preAnalysis()
tess.postAnalysis()

# View the tessellation
tess.viewGeometry()

# Dissable TFI and Templates that generate "structured" triangular meshes
tess.setAnalysisVal("TFI_Templates", False)

# Run AIM pre/post-analysis
tess.preAnalysis()
tess.postAnalysis()

# View the tessellation
tess.viewGeometry()
