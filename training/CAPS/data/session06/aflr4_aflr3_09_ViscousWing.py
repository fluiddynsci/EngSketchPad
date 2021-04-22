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
filename = os.path.join("..","EGADS","CFDViscous_Wing.egads")
transport = myProblem.loadCAPS(filename)

# Load AFLR4 AIM
aflr4 = myProblem.loadAIM(aim         = "aflr4AIM",
                          altName     = "aflr4",
                          analysisDir = "workDir_AFLR4_AFLR3_9_ViscousWing")

# Dump VTK files for visualization
aflr4.setAnalysisVal("Proj_Name", "TransportWing")
aflr4.setAnalysisVal("Mesh_Format", "VTK")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Mesh scaling
aflr4.setAnalysisVal("Mesh_Length_Factor", 5)

# Farfield BC
aflr4.setAnalysisVal("Mesh_Sizing", [("Farfield", {"bcType":"Farfield"})])

# Run AIM pre-/post-analysis
aflr4.preAnalysis()
aflr4.postAnalysis()

# View the surface tessellation
#aflr4.viewGeometry()

# Load AFLR3 AIM to generate the volume mesh
aflr3 = myProblem.loadAIM(aim         = "aflr3AIM",
                          analysisDir = "workDir_AFLR4_AFLR3_9_ViscousWing",
                          parents     = aflr4.aimName)

# Dump VTK files for visualization
aflr3.setAnalysisVal("Proj_Name", "TransportWing")
aflr3.setAnalysisVal("Mesh_Format", "VTK")

# Specify boundary layer maximum layers.
# Initial spacing and minimum thickness are scaled by capsMeshLength
aflr3.setAnalysisVal("BL_Max_Layers", 10)
aflr3.setAnalysisVal("BL_Initial_Spacing", 0.01)
aflr3.setAnalysisVal("BL_Thickness", 0.1)

# Run AIM pre-/post-analysis
aflr3.preAnalysis()
aflr3.postAnalysis()
