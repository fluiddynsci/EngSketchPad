from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse

# Import SU2 Python interface module
from parallel_computation import parallel_computation as su2Run

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'SU2 and AFLR Pytest Example',
                                 prog = 'su2_and_AFLR4_AFLR3_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Change a design parameter - area in the geometry
myProblem.geometry.setGeometryVal("area", 50)

# Trigger build of the geometry
myGeometry.buildGeometry()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "SU2andAFRLMeshTest")

# Load AFLR4 aim
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                          altName = "aflr4",
                          analysisDir= workDir)

# Set project name so a mesh file is generated
aflr4.setAnalysisVal("Proj_Name", "AFLR4_Mesh")

# Set mesh ascii flag
aflr4.setAnalysisVal("Mesh_ASCII_Flag", True)

# Set output mesh format
aflr4.setAnalysisVal("Mesh_Format", "Tecplot")

# Set AIM verbosity
aflr4.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Set maximum and minimum edge lengths relative to capsMeshLength
aflr4.setAnalysisVal("max_scale", 0.5)
aflr4.setAnalysisVal("min_scale", 0.05)

# Run AIM pre-analysis
aflr4.preAnalysis()

#################################################
##  AFLR4  is internally ran during preAnalysis #
#################################################

# Run AIM post-analysis
aflr4.postAnalysis()

# Load AFLR3 aim - child of AFLR4 AIM
aflr3 = myProblem.loadAIM(aim = "aflr3AIM",
                          altName = "aflr3",
                          analysisDir= workDir,
                          parents = ["aflr4"])


# Set project name so a mesh file is generated
aflr3.setAnalysisVal("Proj_Name", "AFLR3_Mesh")

# Set mesh ascii flag
aflr3.setAnalysisVal("Mesh_ASCII_Flag", True)

# Set output mesh format -
aflr3.setAnalysisVal("Mesh_Format", "Tecplot")

# Set AIM verbosity
aflr3.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Run AIM pre-analysis
aflr3.preAnalysis()

#################################################
##  AFLR3  is internally ran during preAnalysis #
#################################################

# Run AIM post-analysis
aflr3.postAnalysis()

# Load SU2 aim - child of AFLR3 AIM
su2 = myProblem.loadAIM(aim = "su2AIM",
                          altName = "su2",
                          analysisDir = workDir,
                          parents = ["aflr3"])

projectName = "su2AFLRTest"

# Set project name
su2.setAnalysisVal("Proj_Name", projectName)

# Set AoA number
su2.setAnalysisVal("Alpha", 1.0)

# Set Mach number
su2.setAnalysisVal("Mach", 0.5901)

# Set Reynolds number
#su2.setAnalysisVal("Re", 10E5)

# Set equation type
su2.setAnalysisVal("Equation_Type","compressible")

# Set equation type
su2.setAnalysisVal("Physical_Problem","Euler")

# Set number of iterations
su2.setAnalysisVal("Num_Iter",5)

# Set overwrite su2 cfg if not linking to Python library
su2.setAnalysisVal("Overwrite_CFG", True)

# Set boundary conditions
bc1 = {"bcType" : "Viscous", "wallHeatFlux" : 0}
bc2 = {"bcType" : "Inviscid"}
su2.setAnalysisVal("Boundary_Condition", [("Wing1", bc1),
                                          ("Wing2", bc2),
                                          ("Farfield","farfield")])

# Specifcy the boundares used to compute forces
su2.setAnalysisVal("Surface_Monitor", ["Wing1", "Wing2"])

# Run AIM pre-analysis
su2.preAnalysis()

####### Run su2 ####################
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
       "Cd = " , su2.getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx = " , su2.getAnalysisOutVal("CMXtot"), \
       "Cmy = " , su2.getAnalysisOutVal("CMYtot"), \
       "Cmz = " , su2.getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print ("Cx = " , su2.getAnalysisOutVal("CXtot"), \
       "Cy = " , su2.getAnalysisOutVal("CYtot"), \
       "Cz = " , su2.getAnalysisOutVal("CZtot"))

print ("Pressure Contribution")
# Get Lift and Drag coefficients
print ("Cl_p = " , su2.getAnalysisOutVal("CLtot_p"), \
       "Cd_p = " , su2.getAnalysisOutVal("CDtot_p"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx_p = " , su2.getAnalysisOutVal("CMXtot_p"), \
       "Cmy_p = " , su2.getAnalysisOutVal("CMYtot_p"), \
       "Cmz_p = " , su2.getAnalysisOutVal("CMZtot_p"))

# Get Cx, Cy, and Cz, coefficients
print ("Cx_p = " , su2.getAnalysisOutVal("CXtot_p"), \
       "Cy_p = " , su2.getAnalysisOutVal("CYtot_p"), \
       "Cz_p = " , su2.getAnalysisOutVal("CZtot_p"))

print ("Viscous Contribution")
# Get Lift and Drag coefficients
print ("Cl_v = " , su2.getAnalysisOutVal("CLtot_v"), \
       "Cd_v = " , su2.getAnalysisOutVal("CDtot_v"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx_v = " , su2.getAnalysisOutVal("CMXtot_v"), \
       "Cmy_v = " , su2.getAnalysisOutVal("CMYtot_v"), \
       "Cmz_v = " , su2.getAnalysisOutVal("CMZtot_v"))

# Get Cx, Cy, and Cz, coefficients
print ("Cx_v = " , su2.getAnalysisOutVal("CXtot_v"), \
       "Cy_v = " , su2.getAnalysisOutVal("CYtot_v"), \
       "Cz_v = " , su2.getAnalysisOutVal("CZtot_v"))

# Close CAPS
myProblem.closeCAPS()
