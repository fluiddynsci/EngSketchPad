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
parser = argparse.ArgumentParser(description = 'SU2 and Delaundo PyTest Example',
                                 prog = 'su2_and_Delaundo_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "SU2DelaundoAnalysisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfd2D.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load delaundo aim
myProblem.loadAIM(aim = "delaundoAIM", analysisDir = workDir)


# Set airfoil edge parameters
airfoil = {"numEdgePoints" : 100, "edgeDistribution" : "Tanh", "initialNodeSpacing" : [0.001, 0.001]}

# Set meshing parameters
myProblem.analysis["delaundoAIM"].setAnalysisVal("Mesh_Sizing", [("Airfoil"   , airfoil),
                                                                 ("TunnelWall", {"numEdgePoints" : 25}),
                                                                 ("InFlow",     {"numEdgePoints" : 25}),
                                                                 ("OutFlow",    {"numEdgePoints" : 25}),
                                                                 ("2DSlice",    {"tessParams" : [0.50, .01, 45]})])

myProblem.analysis["delaundoAIM"].setAnalysisVal("Delta_Thickness", .1)
myProblem.analysis["delaundoAIM"].setAnalysisVal("Max_Aspect", 90.0)

# Set project name and output mesh type
projectName = "delaundoMesh"
myProblem.analysis["delaundoAIM"].setAnalysisVal("Proj_Name", projectName )

myProblem.analysis["delaundoAIM"].setAnalysisVal("Mesh_Format", "Tecplot")

# Run AIM pre-analysis
myProblem.analysis["delaundoAIM"].preAnalysis()

####### Run delaundo ####################
print ("\n\nRunning Delaundo......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myProblem.analysis["delaundoAIM"].analysisDir) # Move into test directory

file = open("Input.sh", "w") # Create a simple input shell script with are control file name
file.write(projectName + ".ctr\n")
file.close()

os.system("delaundo < Input.sh > Info.out"); # Run delaundo via system call

os.chdir(currentDirectory) # Move back to top directory

# Run AIM post-analysis
myProblem.analysis["delaundoAIM"].postAnalysis()

# Retrieve Surface mesh
myProblem.analysis["delaundoAIM"].getAnalysisOutVal("Mesh")

# Load SU2 aim - child of delaundo AIM
myProblem.loadAIM(aim = "su2AIM",
                  altName = "su2",
                  analysisDir = workDir, parents = ["delaundoAIM"])

# Set project name
projectName = "pyCAPS_SU2_Delaundo"
myProblem.analysis["su2"].setAnalysisVal("Proj_Name", projectName)

# Set SU2 Version
myProblem.analysis["su2"].setAnalysisVal("SU2_Version", "Falcon")

# Set Physical problem
myProblem.analysis["su2"].setAnalysisVal("Physical_Problem", "Euler")

# Set AoA number
myProblem.analysis["su2"].setAnalysisVal("Alpha", 0.0)

# Set equation type
myProblem.analysis["su2"].setAnalysisVal("Equation_Type","Compressible")

# Set number of iterations
myProblem.analysis["su2"].setAnalysisVal("Num_Iter",10)

# Set output file format
myProblem.analysis["su2"].setAnalysisVal("Output_Format","Tecplot")

# Set overwrite cfg file
myProblem.analysis["su2"].setAnalysisVal("Overwrite_CFG", True)

# Set the aim to 2D mode
myProblem.analysis["su2"].setAnalysisVal("Two_Dimensional", True)

Mach = 0.4
gamma = 1.4
p = 101325.0
T = 288.15

# Set Mach number
myProblem.analysis["su2"].setAnalysisVal("Mach", Mach)

# Set boundary conditions
aifoilBC = {"bcType" : "InViscid"}
wallBC = {"bcType" : "InViscid"}

backPressureBC = {"bcType" : "SubsonicOutflow",
                  "staticPressure" : p}

inflow = {"bcType" : "SubsonicInflow",
          "totalPressure" : p*(1+ (gamma-1.0)/2.0*Mach**2)**(gamma/(gamma-1)), # Isentropic relation
          "totalTemperature" : T*(1+ (gamma-1.0)/2.0*Mach**2),
          "uVelocity" : 1.0,
          "vVelocity" : 0.0,
          "wVelocity" : 0.0}

myProblem.analysis["su2"].setAnalysisVal("Boundary_Condition", [("Airfoil"   , aifoilBC),
                                                                ("TunnelWall", wallBC),
                                                                ("InFlow",  inflow),
                                                                ("OutFlow", backPressureBC)])

# Specifcy the boundares used to compute forces
myProblem.analysis["su2"].setAnalysisVal("Surface_Monitor", ["Airfoil"])

# Run AIM pre-analysis
myProblem.analysis["su2"].preAnalysis()

####### Run su2 ####################
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myProblem.analysis["su2"].analysisDir) # Move into test directory

su2Run(projectName + ".cfg", args.numberProc); # Run SU2

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
myProblem.analysis["su2"].postAnalysis()

print ("Total Force - Pressure + Viscous")
# Get Lift and Drag coefficients
print ("Cl = " , myProblem.analysis["su2"].getAnalysisOutVal("CLtot"), \
       "Cd = " , myProblem.analysis["su2"].getAnalysisOutVal("CDtot"))

# Get Cmz coefficient
print ("Cmz = " , myProblem.analysis["su2"].getAnalysisOutVal("CMZtot"))

# Get Cx and Cy coefficients
print ("Cx = " , myProblem.analysis["su2"].getAnalysisOutVal("CXtot"), \
       "Cy = " , myProblem.analysis["su2"].getAnalysisOutVal("CYtot"))

print ("Pressure Contribution")
# Get Lift and Drag coefficients
print ("Cl_p = " , myProblem.analysis["su2"].getAnalysisOutVal("CLtot_p"), \
       "Cd_p = " , myProblem.analysis["su2"].getAnalysisOutVal("CDtot_p"))

# Get Cmz coefficient
print ("Cmz_p = " , myProblem.analysis["su2"].getAnalysisOutVal("CMZtot_p"))

# Get Cx and Cy coefficients
print ("Cx_p = " , myProblem.analysis["su2"].getAnalysisOutVal("CXtot_p"), \
       "Cy_p = " , myProblem.analysis["su2"].getAnalysisOutVal("CYtot_p"))

print ("Viscous Contribution")
# Get Lift and Drag coefficients
print ("Cl_v = " , myProblem.analysis["su2"].getAnalysisOutVal("CLtot_v"), \
       "Cd_v = " , myProblem.analysis["su2"].getAnalysisOutVal("CDtot_v"))

# Get Cmz coefficient
print ("Cmz_v = " , myProblem.analysis["su2"].getAnalysisOutVal("CMZtot_v"))

# Get Cx and Cy coefficients
print ("Cx_v = " , myProblem.analysis["su2"].getAnalysisOutVal("CXtot_v"), \
       "Cy_v = " , myProblem.analysis["su2"].getAnalysisOutVal("CYtot_v"))

# Close CAPS
myProblem.closeCAPS()
