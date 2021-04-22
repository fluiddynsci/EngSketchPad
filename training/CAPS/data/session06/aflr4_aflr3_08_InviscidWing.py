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
aflr4 = myProblem.loadAIM(aim         = "aflr4AIM",
                          altName     = "aflr4",
                          analysisDir = "workDir_AFLR4_AFLR3_8_InviscidWing")

# Dump VTK files for visualization
aflr4.setAnalysisVal("Proj_Name", "TransportWing")
aflr4.setAnalysisVal("Mesh_Format", "VTK")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Scaling factor to compute AFLR4 'ref_len' parameter
aflr4.setAnalysisVal("Mesh_Length_Factor", 5)

# Edge mesh spacing discontinuity scaled interpolant and farfield BC
aflr4.setAnalysisVal("Mesh_Sizing", [("Wing"    , {"edgeWeight":1.0}),
                                     ("Farfield", {"bcType":"Farfield"})])

# Run AIM pre-/post-analysis
aflr4.preAnalysis()
aflr4.postAnalysis()

# View the surface tessellation
#aflr4.viewGeometry()

#------------------------------------------------------------------------------#

# Load AFLR3 AIM to generate the volume mesh
aflr3 = myProblem.loadAIM(aim         = "aflr3AIM",
                          analysisDir = "workDir_AFLR4_AFLR3_8_InviscidWing",
                          parents     = aflr4.aimName)

# Dump VTK files for visualization
aflr3.setAnalysisVal("Proj_Name", "TransportWing")
aflr3.setAnalysisVal("Mesh_Format", "VTK")

# Run AIM pre-/post-analysis
aflr3.preAnalysis()
aflr3.postAnalysis()
