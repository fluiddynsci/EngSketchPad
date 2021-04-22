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
filename = os.path.join("..","EGADS","CFDViscous_Transport.egads")
transport = myProblem.loadCAPS(filename)

# Load AFLR4 AIM
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                          altName = "aflr4",
                          analysisDir = "workDir_AFLR4_AFLR3_11_ViscousTransport")

# Dump VTK files for visualization
aflr4.setAnalysisVal("Proj_Name", "Transport")
aflr4.setAnalysisVal("Mesh_Format", "VTK")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Mesh scaling
aflr4.setAnalysisVal("Mesh_Length_Factor", 10)

# Edge mesh spacing discontinuity scaled interpolant and farfield BC
aflr4.setAnalysisVal("Mesh_Sizing", [("Farfield", {"bcType":"Farfield"})])

# Run AIM pre-/post-analysis
aflr4.preAnalysis()
aflr4.postAnalysis()

# View the surface tessellation
aflr4.viewGeometry()

# Load AFLR3 AIM to generate the volume mesh
aflr3 = myProblem.loadAIM(aim = "aflr3AIM",
                          analysisDir = "workDir_AFLR4_AFLR3_11_ViscousTransport",
                          parents = aflr4.aimName)

# Dump VTK files for visualization
aflr3.setAnalysisVal("Proj_Name", "Transport")
aflr3.setAnalysisVal("Mesh_Format", "VTK")

# Set wall spacing for capsGroup = Wing and capsGroup = Pod
airplaneWall = {"boundaryLayerSpacing" : 0.02, "boundaryLayerThickness":0.1}
controlWall  = {"boundaryLayerSpacing" : 0.01, "boundaryLayerThickness":0.1}
aflr3.setAnalysisVal("Mesh_Sizing", [("Wing", airplaneWall),
                                     ("Htail", airplaneWall),
                                     ("Vtail", airplaneWall),
                                     ("Fuselage", airplaneWall),
                                     ("Pylon", airplaneWall),
                                     ("Pod", airplaneWall),
                                     ("Control", controlWall)])

# Specify boundary layer maximum layers.
aflr3.setAnalysisVal("BL_Max_Layers", 5)

# Run AIM pre-/post-analysis
aflr3.preAnalysis()
aflr3.postAnalysis()
