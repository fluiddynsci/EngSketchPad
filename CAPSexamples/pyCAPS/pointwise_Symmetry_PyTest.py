from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import time to sleep in-between attempts to call pointwise
import time

# Import argparse module
import os
import argparse
import platform

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Pointwise Pytest Example',
                                 prog = 'pointwise_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "PointwiseSymmetryAnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdSymmetry.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AFLR4 aim
pointwise = myProblem.loadAIM(aim = "pointwiseAIM",
                              analysisDir = workDir)

# Set project name so a mesh file is generated
pointwise.setAnalysisVal("Proj_Name", "pyCAPS_Pointwise_Test")
pointwise.setAnalysisVal("Mesh_Format", "Tecplot")

# Block level for viscous mesh
pointwise.setAnalysisVal("Block_Full_Layers"         , 1)
pointwise.setAnalysisVal("Block_Max_Layers"          , 100)

# Set mesh sizing parmeters, only Wing2 is viscous
viscousBC  = {"boundaryLayerSpacing" : 0.001}
pointwise.setAnalysisVal("Mesh_Sizing", [("Wing2", viscousBC)])

# Run AIM pre-analysis
pointwise.preAnalysis()

# Pointwise batch execution notes (PW_HOME referes to the Pointwise installation directory):
# Windows (DOS prompt) -
#     PW_HOME\win64\bin\tclsh %CAPS_GLYPH%\GeomToMesh.glf caps.egads capsUserDefaults.glf
# Linux/macOS (terminal) -
#     PW_HOME/pointwise -b $CAPS_GLYPH/GeomToMesh.glf caps.egads capsUserDefaults.glf

# Attempt to run pointwise repeatedly in case a licensce is not readily available
currentDir = os.getcwd()
os.chdir(pointwise.analysisDir)

CAPS_GLYPH = os.environ["CAPS_GLYPH"]
for i in range(30):
    if platform.system() == "Windows":
        PW_HOME = os.environ["PW_HOME"]
        os.system(PW_HOME + "\win64\bin\tclsh.exe " + CAPS_GLYPH + "\\GeomToMesh.glf caps.egads capsUserDefaults.glf")
    elif "CYGWIN" in platform.system():
        PW_HOME = os.environ["PW_HOME"]
        os.system(PW_HOME + "/win64/bin/tclsh.exe " + CAPS_GLYPH + "/GeomToMesh.glf caps.egads capsUserDefaults.glf")
    else:
        os.system("pointwise -b " + CAPS_GLYPH + "/GeomToMesh.glf caps.egads capsUserDefaults.glf")

    time.sleep(1) # let the harddrive breathe
    if os.path.isfile('caps.GeomToMesh.gma') and os.path.isfile('caps.GeomToMesh.ugrid'): break
    time.sleep(10) # wait and try again

os.chdir(currentDir)

# Run AIM post-analysis
pointwise.postAnalysis()

#pointwise.viewGeometry()

# Close CAPS
myProblem.closeCAPS()
