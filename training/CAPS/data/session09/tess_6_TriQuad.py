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

# Set geometry variables for structure with complete OML
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:BoxStructure"    , 0)

# Load tess aim
tess = myProblem.loadAIM(aim = "egadsTessAIM",
                         analysisDir = "workDir_Tess")

# Set EGADS body tessellation parameters
tess.setAnalysisVal("Tess_Params", [0.1, 0.02, 20])

#------------------------------------------------------------------------------#

# Triangle tessellation
tess.setAnalysisVal("Mesh_Elements", "Tri")

# Run AIM pre/post-analysis
tess.preAnalysis()
tess.postAnalysis()

# View the tessellation
tess.viewGeometry()

#------------------------------------------------------------------------------#

# Regularized quad tessellation
tess.setAnalysisVal("Mesh_Elements", "Quad")

# Run AIM pre/post-analysis
tess.preAnalysis()
tess.postAnalysis()

# View the tessellation
tess.viewGeometry()

#------------------------------------------------------------------------------#

# Set geometry variables for structure with wing-box
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:BoxStructure"    , 1)

# Run AIM pre/post-analysis
tess.preAnalysis()
tess.postAnalysis()

# View the tessellation
tess.viewGeometry()

#------------------------------------------------------------------------------#
