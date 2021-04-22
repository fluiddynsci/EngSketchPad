## [importPrint]
from __future__ import print_function
## [importPrint]

## [import]
# Import pyCAPS class file
import pyCAPS

# Import os module
import os

# Import argparse module
import argparse

# Import SU2 python environment
from parallel_computation import parallel_computation as su2Run
## [import]


# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'SU2 and Tetgen Pytest Example',
                                 prog = 'su2_and_Tetgen_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

workDir = os.path.join(str(args.workDir[0]), "SU2TetgenAnalysisTest")
# -----------------------------------------------------------------
# Initialize capsProblem object
# -----------------------------------------------------------------
## [initateProblem]
myProblem = pyCAPS.capsProblem()
## [initateProblem]

# -----------------------------------------------------------------
# Load CSM file and Change a design parameter - area in the geometry
# Any despmtr from the cfdMultiBody.csm  file are available inside the pyCAPS script.
# For example: thick, camber, area, aspect, taper, sweep, washout, dihedral
# -----------------------------------------------------------------
# Load CSM file
## [geometry]
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
## [geometry]

## [capsDespmtrs]
# Change a design parameter - area in the geometry
myProblem.geometry.setGeometryVal("area", 50)
## [capsDespmtrs]

# Load Tetgen aim
## [loadMeshAIM]
myMesh = myProblem.loadAIM(aim = "tetgenAIM", analysisDir = workDir)
## [loadMeshAIM]

## [setMeshpmtrs]
# Set new EGADS body tessellation parameters
myMesh.setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])

# Preserve surface mesh while meshing
myMesh.setAnalysisVal("Preserve_Surf_Mesh", True)
## [setMeshpmtrs]

# Run AIM pre-analysis
## [meshPreAnalysis]
myMesh.preAnalysis()
## [meshPreAnalysis]

## [meshExecute]
# NO analysis is needed - TetGen was already ran during preAnalysis
## [meshExecute]

# Run AIM post-analysis
## [meshPostAnalysis]
myMesh.postAnalysis()
## [meshPostAnalysis]

## [loadSu2]
# Load SU2 aim - child of Tetgen AIM
myAnalysis = myProblem.loadAIM(aim = "su2AIM",
                               analysisDir = workDir,
                               parents = myMesh.aimName)
## [loadSu2]

## [setSu2Inputs]

# Set SU2 Version
myAnalysis.setAnalysisVal("SU2_Version","Falcon")

# Set project name
myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_SU2_Tetgen")

# Set AoA number
myAnalysis.setAnalysisVal("Alpha", 1.0)

# Set Mach number
myAnalysis.setAnalysisVal("Mach", 0.5901)

# Set equation type
myAnalysis.setAnalysisVal("Equation_Type","Compressible")

# Set number of iterations
myAnalysis.setAnalysisVal("Num_Iter",10)

# Specifcy the boundares used to compute forces
myAnalysis.setAnalysisVal("Surface_Monitor", ["Wing1", "Wing2"])

## [setSu2Inputs]

# Set boundary conditions
## [setSu2BCs]
inviscidBC1 = {"bcType" : "Inviscid"}
inviscidBC2 = {"bcType" : "Inviscid"}
myAnalysis.setAnalysisVal("Boundary_Condition", [("Wing1", inviscidBC1),
                                                 ("Wing2", inviscidBC2),
                                                 ("Farfield","farfield")])
## [setSu2BCs]

# Run AIM pre-analysis
## [su2PreAnalysis]
myAnalysis.preAnalysis()
## [su2PreAnalysis]

####### Run SU2 ####################
## [su2Run]
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myAnalysis.analysisDir) # Move into test directory

su2Run(myAnalysis.getAnalysisVal("Proj_Name") + ".cfg", args.numberProc) # Run SU2

os.chdir(currentDirectory) # Move back to top directory
## [su2Run]
#######################################

# Run AIM post-analysis
## [su2PostAnalysis]
myAnalysis.postAnalysis()
## [su2PostAnalysis]

## [su2Output]
# Get force results
print("Total Force - Pressure + Viscous")
# Get Lift and Drag coefficients
print("Cl = " , myAnalysis.getAnalysisOutVal("CLtot"), \
      "Cd = "  , myAnalysis.getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print("Cmx = " , myAnalysis.getAnalysisOutVal("CMXtot"), \
      "Cmy = "  , myAnalysis.getAnalysisOutVal("CMYtot"), \
      "Cmz = "  , myAnalysis.getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print("Cx = " , myAnalysis.getAnalysisOutVal("CXtot"), \
      "Cy = "  , myAnalysis.getAnalysisOutVal("CYtot"), \
      "Cz = "  , myAnalysis.getAnalysisOutVal("CZtot"))
## [su2Output]

# Close CAPS
## [closeCAPS]
myProblem.closeCAPS()
## [closeCAPS]
