from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AFLR2 Pytest Example',
                                 prog = 'aflr2_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "AFLR2AnalysisTest")

# Project name
projectName = "pyCAPS_AFLR2_Test"

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfd2D.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load aflr2 aim
myMesh = myProblem.loadAIM(aim = "aflr2AIM", analysisDir = workDir, capsIntent="CFD")

# Set project name
myMesh.setAnalysisVal("Proj_Name", projectName)

# Set AIM verbosity
myMesh.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set airfoil edge parameters
airfoil = {"numEdgePoints" : 100, "edgeDistribution" : "Tanh", "initialNodeSpacing" : [0.001, 0.001]}

# Set meshing parameters
myMesh.setAnalysisVal("Mesh_Sizing", [("Airfoil"   , airfoil),
                                      ("TunnelWall", {"numEdgePoints" : 50}),
                                      ("InFlow",     {"numEdgePoints" : 25}),
                                      ("OutFlow",    {"numEdgePoints" : 25}),
                                      ("2DSlice",    {"tessParams" : [0.50, .01, 45]})])

# Make quad/tri instead of just quads
myMesh.setAnalysisVal("Mesh_Gen_Input_String", "mquad=1 mpp=3")

# Set output grid format since a project name is being supplied - Tecplot  file
myMesh.setAnalysisVal("Mesh_Format", "Tecplot")

# Run AIM pre-analysis
myMesh.preAnalysis()

# NO analysis is needed - AFLR2 was already ran during preAnalysis

# Run AIM post-analysis
myMesh.postAnalysis()

# Close CAPS
myProblem.closeCAPS()
