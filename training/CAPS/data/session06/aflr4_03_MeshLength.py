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

# Mark capsGroup="Farfield" with a Farfield bcType
aflr4.setAnalysisVal("Mesh_Sizing", ("Farfield", {"bcType":"Farfield"}))

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Scaling factor to compute AFLR4 'ref_len' parameter via
# ref_len = capsMeshLength * Mesh_Length_Factor
aflr4.setAnalysisVal("Mesh_Length_Factor", 5)

# Relative scale of maximum spacing bound relative to ref_len
# max_spacing = max_scale * ref_len
aflr4.setAnalysisVal("max_scale", 0.1)

# Relative scale of minimum spacing bound relative to ref_len
# min_spacing = min_scale * ref_len
aflr4.setAnalysisVal("min_scale", 0.01)

# Absolute scale of minimum spacing bound for proximity
# abs_min_spacing = abs_min_scale * ref_len
aflr4.setAnalysisVal("abs_min_scale", 0.01)

# Use mesh length factor to make a series of meshes (not a family)
for Mesh_Length_Factor in [1, 3, 9]:
    aflr4.setAnalysisVal("Mesh_Length_Factor", Mesh_Length_Factor)

    # Run AIM pre-/post-analysis
    aflr4.preAnalysis()
    aflr4.postAnalysis()

    # View the surface tessellation
    aflr4.viewGeometry()
