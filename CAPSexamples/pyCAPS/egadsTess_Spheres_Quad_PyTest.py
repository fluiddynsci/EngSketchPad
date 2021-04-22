from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'EGADS Tess Shheres Quad PyTest Example',
                                 prog = 'egadsTess_Spheres_Quad_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "egadsTessSpheresAnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file and build the geometry explicitly
geometryScript = os.path.join("..","csmData","spheres.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AFLR4 aim
myAnalysis = myProblem.loadAIM(aim = "egadsTessAIM",
                               analysisDir = workDir)

# Set project name so a mesh file is generated
myAnalysis.setAnalysisVal("Proj_Name", "egadsTessMesh")

# Set new EGADS body tessellation parameters
myAnalysis.setAnalysisVal("Tess_Params", [.1, 0.1, 20.0])

# Set output grid format since a project name is being supplied - Tecplot file
myAnalysis.setAnalysisVal("Mesh_Format", "Tecplot")

myAnalysis.setAnalysisVal("Mesh_Elements", "Quad")

#Mesh_Sizing = [("Spheres",  {"tessParams" : [0.5, .1, 30]})]

#myAnalysis.setAnalysisVal("Mesh_Sizing", Mesh_Sizing)

# Run AIM pre-analysis
myAnalysis.preAnalysis()

##########################################
## egadsTess was ran during preAnalysis ##
##########################################

# Run AIM post-analysis
myAnalysis.postAnalysis()

#myAnalysis.viewGeometry()

# Close CAPS
myProblem.closeCAPS()
