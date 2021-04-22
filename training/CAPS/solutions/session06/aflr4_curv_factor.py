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
filename = os.path.join("..","..","data","EGADS","CFDInviscid_Wing.egads")
transport = myProblem.loadCAPS(filename)

# Load AFLR4 AIM
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                          analysisDir = "workDir_AFLR4")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Scaling factor to compute AFLR4 'ref_len' parameter via
aflr4.setAnalysisVal("Mesh_Length_Factor", 1)

# Edge mesh spacing discontinuity scaled interpolant and farfield BC
aflr4.setAnalysisVal("Mesh_Sizing", [("Wing"    , {"edgeWeight":1.0}),
                                     ("Farfield", {"bcType":"Farfield"})])

# The spacing is derived from the curvature factor divided
# by the curvature.
# Curvature = 1 / Curvature_Radius
# Spacing = curv_factor / Curvature
for curv_factor in [0.01, 0.05, 0.1]:
    aflr4.setAnalysisVal("curv_factor", curv_factor)

    # Run AIM pre-/post-analysis
    aflr4.preAnalysis()
    aflr4.postAnalysis()

    # View the surface tessellation
    aflr4.viewGeometry()
