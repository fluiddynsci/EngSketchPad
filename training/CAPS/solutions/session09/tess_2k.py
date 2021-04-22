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
filename = os.path.join("..","..","data","ESP","wing3.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
wing = myProblem.loadCAPS(filename)

# Set geometry variables for structure with complete OML
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:BoxStructure"    , 1)

# Load tess aim
tess = myProblem.loadAIM(aim = "egadsTessAIM",
                         analysisDir = "workDir_Tess")

# Set EGADS body tessellation parameters
tess.setAnalysisVal("Tess_Params", [1.0, 1.0, 20])

# Modify local mesh sizing parameters
Mesh_Sizing = [("leadingEdge"    , {"numEdgePoints" : 4}),
               ("rootLeadingEdge", {"numEdgePoints" : 9})]

tess.setAnalysisVal("Mesh_Sizing", Mesh_Sizing)

#------------------------------------------------------------------------------#

# Triangle tessellation
tess.setAnalysisVal("Mesh_Elements", "Quad")

# Run AIM pre/post-analysis
tess.preAnalysis()
tess.postAnalysis()

# View the tessellation
tess.viewGeometry()

#------------------------------------------------------------------------------#

# Print the total element count
print("--> Toal Elements", tess.getAnalysisOutVal("NumberOfElement"))

