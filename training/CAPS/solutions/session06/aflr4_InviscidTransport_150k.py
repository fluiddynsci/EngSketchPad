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

# Load CSM file
filename = os.path.join("..","..","data","EGADS","CFDInviscid_Transport.egads")
transport = myProblem.loadCAPS(filename)

# Load AFLR4 AIM
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                          analysisDir = "workDir_AFLR4")

# Scaling factor to compute AFLR4 'ref_len' parameter via
# ref_len = capsMeshLength * Mesh_Length_Factor
aflr4.setAnalysisVal("Mesh_Length_Factor", 2)

# The spacing is derived from the curvature factor divided
# by the curvature.
# Curvature = 1 / Curvature_Radius
# Spacing = curv_factor / Curvature
aflr4.setAnalysisVal("curv_factor", 0.2)

# Scale components
aflr4.setAnalysisVal("Mesh_Sizing", [("Farfield", {"bcType":"Farfield"}),
                                     ("Fuselage", {"scaleFactor":1}),
                                     ("Wing"    , {"scaleFactor":2, "edgeWeight":0.0}),
                                     ("Htail"   , {"scaleFactor":2, "edgeWeight":0.0}),
                                     ("Vtail"   , {"scaleFactor":2, "edgeWeight":0.0}),
                                     ("Pod"     , {"scaleFactor":1})])

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Run AIM pre-/post-analysis
aflr4.preAnalysis()
aflr4.postAnalysis()

# View the surface tessellation
aflr4.viewGeometry()

# Print the total element count
print("--> Toal Elements", aflr4.getAnalysisOutVal("NumberOfElement"))
