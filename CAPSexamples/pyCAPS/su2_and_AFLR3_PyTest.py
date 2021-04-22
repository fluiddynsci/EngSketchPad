from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Import SU2 Python interface module
from parallel_computation import parallel_computation as su2Run


# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'SU2 and AFLR3 Pytest Example',
                                 prog = 'su2_and_AFLR3_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

workDir = os.path.join(str(args.workDir[0]), "SU2AFLR3AnalysisTest")

projectName = "pyCAPS_SU2_AFLR3"

# -----------------------------------------------------------------
# Initialize capsProblem object
# -----------------------------------------------------------------
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AFLR3 aim
myAnalysis = myProblem.loadAIM(aim = "aflr3AIM",
                               analysisDir= workDir)

# Set project name so a mesh file is generated
myAnalysis.setAnalysisVal("Proj_Name", projectName)

#myAnalysis.setAnalysisVal("Tess_Params", [50, .11, 20.0])

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

# Run AIM pre-analysis
myAnalysis.preAnalysis()

#################################################
##  AFLR3  is internally ran during preAnalysis #
#################################################

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Load SU2 aim - child of AFLR3 AIM
su2 = myProblem.loadAIM(aim = "su2AIM",
                  altName = "su2",
                  analysisDir = workDir, parents = ["aflr3AIM"])

# Set SU2 Version
su2.setAnalysisVal("SU2_Version","Falcon")

# Set project name
su2.setAnalysisVal("Proj_Name", projectName)

# Set AoA number
su2.setAnalysisVal("Alpha", 1.0)

# Set Mach number
su2.setAnalysisVal("Mach", 0.5901)

# Set equation type
su2.setAnalysisVal("Equation_Type","Compressible")

# Set number of iterations
su2.setAnalysisVal("Num_Iter",5)

# Set boundary conditions
inviscidBC1 = {"bcType" : "Inviscid"}
inviscidBC2 = {"bcType" : "Inviscid"}
su2.setAnalysisVal("Boundary_Condition", [("Wing1", inviscidBC1),
                                          ("Wing2", inviscidBC2),
                                          ("Farfield","farfield")])

# Specifcy the boundares used to compute forces
myProblem.analysis["su2"].setAnalysisVal("Surface_Monitor", ["Wing1", "Wing2"])

# Run AIM pre-analysis
su2.preAnalysis()

####### Run SU2 #######################
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(su2.analysisDir) # Move into test directory

su2Run(projectName + ".cfg", args.numberProc) # Run SU2

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
su2.postAnalysis()

# Get force results
print ("Total Force - Pressure + Viscous")
# Get Lift and Drag coefficients
print ("Cl = " , su2.getAnalysisOutVal("CLtot"), \
       "Cd = "  , su2.getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx = " , su2.getAnalysisOutVal("CMXtot"), \
       "Cmy = "  , su2.getAnalysisOutVal("CMYtot"), \
       "Cmz = "  , su2.getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print ("Cx = " , su2.getAnalysisOutVal("CXtot"), \
       "Cy = "  , su2.getAnalysisOutVal("CYtot"), \
       "Cz = "  , su2.getAnalysisOutVal("CZtot"))

# Close CAPS
myProblem.closeCAPS()
