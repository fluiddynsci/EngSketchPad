from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Tetgen Pytest Example',
                                 prog = 'tetgen_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "TetgenAnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load TetGen aim
myProblem.loadAIM(aim = "tetgenAIM", analysisDir= workDir)

# Set new EGADS tessellation parameters
myProblem.analysis["tetgenAIM"].setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])

# Set Tetgen: maximum radius-edge ratio
myProblem.analysis["tetgenAIM"].setAnalysisVal("Quality_Rad_Edge", 1.5)

# Set surface mesh preservation
myProblem.analysis["tetgenAIM"].setAnalysisVal("Preserve_Surf_Mesh", True)

# Set project name
myProblem.analysis["tetgenAIM"].setAnalysisVal("Proj_Name", "pyCAPS_Tetgen_Test")

# Set output grid format since a project name is being supplied - Tecplot tetrahedral file
myProblem.analysis["tetgenAIM"].setAnalysisVal("Mesh_Format", "Tecplot")
myProblem.analysis["tetgenAIM"].setAnalysisVal("Mesh_ASCII_Flag", False)


# Get analysis info.
myProblem.analysis["tetgenAIM"].getAnalysisInfo()

# Run AIM pre-analysis
myProblem.analysis["tetgenAIM"].preAnalysis()

#######################################
## Tetgen was ran during preAnalysis ##
#######################################

# Run AIM post-analysis
myProblem.analysis["tetgenAIM"].postAnalysis()

# Close CAPS
myProblem.closeCAPS()
