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

# Load aflr4 aim
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                          analysisDir = "workDir_AFLR4")

# View the bodies
aflr4.viewGeometry()

# Mark capsGroup="Farfield" with a Farfield bcType
aflr4.setAnalysisVal("Mesh_Sizing", 
                     ("Farfield", {"bcType":"Farfield"}))

# Run AIM pre-analysis
aflr4.preAnalysis()

# AFLR4 executes in memory with preAnalysis

# Run AIM post-analysis
aflr4.postAnalysis()

# View the surface tessellation
aflr4.viewGeometry()
