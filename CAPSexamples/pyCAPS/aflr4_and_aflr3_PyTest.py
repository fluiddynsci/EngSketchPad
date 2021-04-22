from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AFLR4 and AFLR3 Pytest Example',
                                 prog = 'aflr4_and_aflr3_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Set analysis directory
workDir = os.path.join(str(args.workDir[0]), "AFLRAnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file and build the geometry explicitly
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
myGeometry.buildGeometry()

# Load AFLR4 aim
mySurfMesh = myProblem.loadAIM(aim = "aflr4AIM",
                               analysisDir= workDir)

# Set project name - so a mesh file is generated
mySurfMesh.setAnalysisVal("Proj_Name", "pyCAPS_AFLR4_AFLR3")

# Set AIM verbosity
mySurfMesh.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set output grid format since a project name is being supplied - Tecplot  file
mySurfMesh.setAnalysisVal("Mesh_Format", "Tecplot")

# Farfield growth factor
mySurfMesh.setAnalysisVal("ff_cdfr", 1.4)

# Set maximum and minimum edge lengths relative to capsMeshLength
mySurfMesh.setAnalysisVal("max_scale", 0.6)
mySurfMesh.setAnalysisVal("min_scale", 0.06)

# Run AIM pre-analysis
mySurfMesh.preAnalysis()

######################################
## AFRL4 was ran during preAnalysis ##
######################################

# Run AIM post-analysis
mySurfMesh.postAnalysis()

######################################
## Build volume mesh off of surface ##
##  mesh(es) using AFLR3            ##
######################################

# Load AFLR3 aim
myVolMesh = myProblem.loadAIM(aim = "aflr3AIM",
                              analysisDir= workDir,
                              parents = mySurfMesh.aimName)

# Set AIM verbosity
myVolMesh.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set project name - so a mesh file is generated
myVolMesh.setAnalysisVal("Proj_Name", "pyCAPS_AFLR4_AFLR3_VolMesh")

# Set output grid format since a project name is being supplied - Tecplot tetrahedral file
myVolMesh.setAnalysisVal("Mesh_Format", "Tecplot")

# Set either global or local boundary layer thickness spacings
# These are coarse numbers just as an example, not a recommenation for good CFD solutions
useGlobal = False

if useGlobal:
    myVolMesh.setAnalysisVal("BL_Initial_Spacing", 0.01)
    myVolMesh.setAnalysisVal("BL_Thickness", 0.1)

else:
    inviscidBC = {"boundaryLayerThickness" : 0.0, "boundaryLayerSpacing" : 0.0}
    viscousBC  = {"boundaryLayerThickness" : 0.1, "boundaryLayerSpacing" : 0.01}

    # Set mesh sizing parmeters
    myVolMesh.setAnalysisVal("Mesh_Sizing", [("Wing1", viscousBC), ("Wing2", inviscidBC)])

# Run AIM pre-analysis
myVolMesh.preAnalysis()

######################################
## AFLR3 was ran during preAnalysis ##
######################################

# Run AIM post-analysis
myVolMesh.postAnalysis()

# Close CAPS
myProblem.closeCAPS()
