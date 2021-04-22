from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse


# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AFLR3 Pytest Example',
                                 prog = 'aflr3_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "AFLR3AnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AFLR3 aim
myAnalysis = myProblem.loadAIM(aim = "aflr3AIM",
                               analysisDir= workDir)

# Set project name so a mesh file is generated
myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_AFLR3_Test")

myAnalysis.setAnalysisVal("Tess_Params", [0.1, .01, 20.0])

myAnalysis.setAnalysisVal("Mesh_ASCII_Flag", True)

myAnalysis.setAnalysisVal("Mesh_Format", "AFLR3")

myAnalysis.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set either global or local boundary layer thickness spacings
# These are coarse numbers just as an example, not a recommenation for good CFD solutions
useGlobal = False

if useGlobal:
    myAnalysis.setAnalysisVal("BL_Initial_Spacing", 0.01)
    myAnalysis.setAnalysisVal("BL_Thickness", 0.1)

else:
    inviscidBC = {"boundaryLayerThickness" : 0.0, "boundaryLayerSpacing" : 0.0}
    viscousBC  = {"boundaryLayerThickness" : 0.1, "boundaryLayerSpacing" : 0.01}

    # Set mesh sizing parmeters
    myAnalysis.setAnalysisVal("Mesh_Sizing", [("Wing1", viscousBC), ("Wing2", inviscidBC)])

# mbl=2 generate a structured-layer (SL) grid
# mblelc=2 combine elements in BL region to form pentahedra with five
#            nodes (pyramid) or six nodes (prism) and hexahedra
# mquadp = 1 then add a transition pyramid element to every boundary surface quad-face
myAnalysis.setAnalysisVal("Mesh_Gen_Input_String", "mbl=2 mblelc=2 mquadp=1")

# Run AIM pre-analysis
myAnalysis.preAnalysis()

#################################################
##  AFLR3  is internally ran during preAnalysis #
#################################################

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Close CAPS
myProblem.closeCAPS()
