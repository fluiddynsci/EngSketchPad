# Import pyCAPS module
import pyCAPS

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AFLR4 Symmetry PyTest Example',
                                 prog = 'aflr4_Symmetry_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ["." + os.sep], nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-outLevel", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "AFLRSymmetryAnalysisTest")

# Load CSM file and build the geometry explicitly
geometryScript = os.path.join("..","csmData","cfdSymmetry.csm")
myProblem = pyCAPS.Problem(problemName = workDir,
                           capsFile=geometryScript, 
                           outLevel=args.outLevel)

# Load AFLR4 aim
mySurfMesh = myProblem.analysis.create(aim = "aflr4AIM")

# Mesing boundary conditions
mySurfMesh.input.Mesh_Sizing = {"Farfield": {"bcType":"Farfield"},
                                "Symmetry": {"bcType":"Symmetry"}}

# Set project name so a mesh file is generated
mySurfMesh.input.Proj_Name = "pyCAPS_AFLR4_Test"

# Set AIM verbosity
mySurfMesh.input.Mesh_Quiet_Flag = True if args.outLevel == 0 else False

# Set output grid format since a project name is being supplied - Tecplot  file
mySurfMesh.input.Mesh_Format = "Tecplot"

# Farfield growth factor
mySurfMesh.input.ff_cdfr = 1.4

# Generate quads and tris
# mySurfMesh.input.Mesh_Gen_Input_String = "mquad=1 mpp=3"

# Set maximum and minimum edge lengths relative to capsMeshLength
mySurfMesh.input.max_scale = 0.2
mySurfMesh.input.min_scale = 0.01


######################################
## AFRL4 executes automatically     ##
######################################

######################################
## Build volume mesh off of surface ##
##  mesh(es) using AFLR3            ##
######################################

# Load AFLR3 aim
myVolMesh = myProblem.analysis.create(aim = "aflr3AIM")

# Link the surface mesh
myVolMesh.input["Surface_Mesh"].link(mySurfMesh.output["Surface_Mesh"])

# Set AIM verbosity
myVolMesh.input.Mesh_Quiet_Flag = True if args.outLevel == 0 else False

# Set project name - so a mesh file is generated
myVolMesh.input.Proj_Name = "pyCAPS_AFLR3_VolMesh"

# Set output grid format since a project name is being supplied - Tecplot tetrahedral file
myVolMesh.input.Mesh_Format = "Tecplot"

# Set either global or local boundary layer thickness spacings
# These are coarse numbers just as an example, not a recommenation for good CFD solutions
useGlobal = False

if useGlobal:
    myVolMesh.input.BL_Initial_Spacing = 0.01
    myVolMesh.input.BL_Thickness = 0.1

else:
    inviscidBC = {"boundaryLayerThickness" : 0.0, "boundaryLayerSpacing" : 0.0}
    viscousBC  = {"boundaryLayerThickness" : 0.1, "boundaryLayerSpacing" : 0.01}

    # Set mesh sizing parmeters
    myVolMesh.input.Mesh_Sizing = {"Wing1": viscousBC, "Wing2": inviscidBC}

# Run AIM
myVolMesh.runAnalysis()