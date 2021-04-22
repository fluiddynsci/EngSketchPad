from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AFLR4 Generic Missile PyTest Example',
                                 prog = 'aflr4_Generic_Missile_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot surface meshes")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "AFLR4GenericMissileAnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file and build the geometry explicitly
geometryScript = os.path.join("..","csmData","generic_missile.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AFLR4 aim
myAnalysis = myProblem.loadAIM(aim = "aflr4AIM",
                               analysisDir = workDir)

# Set AIM verbosity
myAnalysis.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set output grid format since a project name is being supplied - Tecplot  file
myAnalysis.setAnalysisVal("Mesh_Format", "Tecplot")

# Farfield growth factor
myAnalysis.setAnalysisVal("ff_cdfr", 1.4)

# Set maximum and minimum edge lengths relative to capsMeshLength
myAnalysis.setAnalysisVal("max_scale", 0.1)
myAnalysis.setAnalysisVal("min_scale", 0.01)

myAnalysis.setAnalysisVal("Mesh_Length_Factor", 0.25)

# Set project name so a mesh file is generated for each configuration
myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_AFLR4_Missile_Test")

# Dissable curvature refinement when AFLR4 cannot generate a mesh
# myAnalysis.setAnalysisVal("Mesh_Gen_Input_String", "auto_mode=0")

# Run AIM pre-analysis
myAnalysis.preAnalysis()

#######################################
## AFRL4 was ran during preAnalysis ##
#######################################

# Run AIM post-analysis
myAnalysis.postAnalysis()

