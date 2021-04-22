#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os and platform
import os
import platform
import time

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
transport = myProblem.loadCAPS(os.path.join("..","EGADS","CFDInviscid_Wing.egads"))

# Load pointwise aim
pointwise = myProblem.loadAIM(aim = "pointwiseAIM",
                              analysisDir = "workDir_1_InviscidTransportWing")

# Dump VTK files for visualization
pointwise.setAnalysisVal("Proj_Name", "TransportWing")
pointwise.setAnalysisVal("Mesh_Format", "VTK")

# Run AIM pre-analysis
pointwise.preAnalysis()

# Pointwise batch execution notes (PW_HOME referes to the workDir installation directory):
# Windows (DOS prompt) -
#     PW_HOME\win64\bin\tclsh %CAPS_GLYPH%\GeomToMesh.glf caps.egads capsUserDefaults.glf
# Linux/macOS (terminal) -
#     PW_HOME/pointwise -b $CAPS_GLYPH/GeomToMesh.glf caps.egads capsUserDefaults.glf

####### Run pointwise #################
currentDirectory = os.getcwd()   # Get current working directory
os.chdir(pointwise.analysisDir)  # Move into test directory

CAPS_GLYPH = os.environ["CAPS_GLYPH"]
for i in range(60):
    if "Windows" in platform.system():
        PW_HOME = os.environ["PW_HOME"]
        os.system('"' + PW_HOME + '\\win64\\bin\\tclsh.exe ' + CAPS_GLYPH + \
                  '\\GeomToMesh.glf" caps.egads capsUserDefaults.glf')
    else:
        os.system("pointwise -b " + CAPS_GLYPH + "/GeomToMesh.glf caps.egads capsUserDefaults.glf")

    time.sleep(1) # let the harddrive breathe
    if os.path.isfile('caps.GeomToMesh.gma') and os.path.isfile('caps.GeomToMesh.ugrid'): break
    time.sleep(20) # wait and try again

os.chdir(currentDirectory)       # Move back to top directory
#######################################

# Run AIM post-analysis
pointwise.postAnalysis()

# View the surface tessellation
pointwise.viewGeometry()
