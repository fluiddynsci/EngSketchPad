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
parser = argparse.ArgumentParser(description = 'SU2 X43a Pytest Example',
                                 prog = 'su2_X43a_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

workDir = os.path.join(str(args.workDir[0]), "SU2X43aAnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdX43a.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Set intention - could also be set during .loadAIM
myProblem.capsIntent = "CFD"

# Change a design parameter - area in the geometry
myProblem.geometry.setGeometryVal("tailLength", 150)

# Load Tetgen aim
myProblem.loadAIM(aim = "tetgenAIM", analysisDir= ".")

# Set new EGADS body tessellation parameters
#myProblem.analysis["tetgenAIM"].setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])

# Preserve surface mesh while messhing
myProblem.analysis["tetgenAIM"].setAnalysisVal("Preserve_Surf_Mesh", True)

# Run AIM pre-analysis
myProblem.analysis["tetgenAIM"].preAnalysis()

# NO analysis is needed - Tetgen was already ran during preAnalysis

# Run AIM post-analysis
myProblem.analysis["tetgenAIM"].postAnalysis()

# Load SU2 aim - child of Tetgen AIM
myProblem.loadAIM(aim = "su2AIM",
                  altName = "su2",
                  analysisDir = workDir, parents = ["tetgenAIM"])

projectName = "x43a_Test"

# Set SU2 Version
myProblem.analysis["su2"].setAnalysisVal("SU2_Version","Falcon")

# Set project name
myProblem.analysis["su2"].setAnalysisVal("Proj_Name", projectName)

# Set AoA number
myProblem.analysis["su2"].setAnalysisVal("Alpha", 3.0)

# Set Mach number
myProblem.analysis["su2"].setAnalysisVal("Mach", 3.5)

# Set equation type
myProblem.analysis["su2"].setAnalysisVal("Equation_Type","Compressible")

# Set number of iterations
myProblem.analysis["su2"].setAnalysisVal("Num_Iter",5)

# Set overwrite cfg file
myProblem.analysis["su2"].setAnalysisVal("Overwrite_CFG", True)

# Set boundary conditions
inviscidBC = {"bcType" : "Inviscid"}
myProblem.analysis["su2"].setAnalysisVal("Boundary_Condition", [("x43A", inviscidBC),
                                                                ("Farfield","farfield")])

# Specifcy the boundares used to compute forces
myProblem.analysis["su2"].setAnalysisVal("Surface_Monitor", ["x43A"])

# Run AIM pre-analysis
myProblem.analysis["su2"].preAnalysis()

####### Run SU2 ####################
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myProblem.analysis["su2"].analysisDir) # Move into test directory

su2Run(projectName + ".cfg", args.numberProc) # Run SU2

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
myProblem.analysis["su2"].postAnalysis()

# Get force results
print ("Total Force - Pressure + Viscous")
# Get Lift and Drag coefficients
print ("Cl = " , myProblem.analysis["su2"].getAnalysisOutVal("CLtot"), \
       "Cd = " , myProblem.analysis["su2"].getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx = " , myProblem.analysis["su2"].getAnalysisOutVal("CMXtot"), \
       "Cmy = " , myProblem.analysis["su2"].getAnalysisOutVal("CMYtot"), \
       "Cmz = " , myProblem.analysis["su2"].getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print ("Cx = " , myProblem.analysis["su2"].getAnalysisOutVal("CXtot"), \
       "Cy = " , myProblem.analysis["su2"].getAnalysisOutVal("CYtot"), \
       "Cz = " , myProblem.analysis["su2"].getAnalysisOutVal("CZtot"))

# Close CAPS
myProblem.closeCAPS()
