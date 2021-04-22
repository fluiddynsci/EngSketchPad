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
filename = os.path.join("..","EGADS","CFDInviscid_Wing.egads")
transport = myProblem.loadCAPS(filename)

# Load AFLR4 AIM
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                          analysisDir = "workDir_AFLR4")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Scaling factor to compute AFLR4 'ref_len' parameter via
# ref_len = capsMeshLength * Mesh_Length_Factor
aflr4.setAnalysisVal("Mesh_Length_Factor", 5)

# Edge mesh spacing can be scaled on surfaces based on
# discontinuity level between adjacent surfaces on both sides of the edge.
# The level of discontinuity potentially reducing the edge spacing.
# The edgeWeight scale factor weight is used as an interpolation weight
# between the unmodified spacing and the modified spacing.
aflr4.setAnalysisVal("Mesh_Sizing", [("Wing"    , {"edgeWeight":1.0}),
                                     ("Farfield", {"bcType":"Farfield"})])

# Run AIM pre-/post-analysis
aflr4.preAnalysis()
aflr4.postAnalysis()

# View the surface tessellation
aflr4.viewGeometry()
