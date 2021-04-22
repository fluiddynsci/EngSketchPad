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

# Scale components
aflr4.setAnalysisVal("Mesh_Sizing", [("Farfield", {"bcType":"Farfield"}),
                                     ("Fuselage", {"scaleFactor":4}),
                                     ("Wing"    , {"scaleFactor":4}),
                                     ("Htail"   , {"scaleFactor":4}),
                                     ("Vtail"   , {"scaleFactor":4}),
                                     ("Pod"     , {"scaleFactor":1})])

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Run AIM pre-/post-analysis
aflr4.preAnalysis()
aflr4.postAnalysis()

# View the surface tessellation
aflr4.viewGeometry()
