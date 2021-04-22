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
parser = argparse.ArgumentParser(description = 'SU2 and AFLR2 Node Distance PyTest Example',
                                 prog = 'su2_and_AFLR2_NodeDist_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Project name
projectName = "pyCAPS_SU2_aflr2"

# Load CSM file
geometryScript = os.path.join("..","csmData","cfd2D.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "SU2AFLR2NodeDistAnalysisTest")

# Set intention - could also be set during .loadAIM
myProblem.capsIntent = "CFD"

# Load aflr2 aim
myProblem.loadAIM(aim = "aflr2AIM", analysisDir = workDir)

airfoil = {"numEdgePoints" : 40, "edgeDistribution" : "Tanh", "initialNodeSpacing" : [0.01, 0.01]}

myProblem.analysis["aflr2AIM"].setAnalysisVal("Mesh_Sizing", [("Airfoil"   , airfoil),
                                                              ("TunnelWall", {"numEdgePoints" : 50}),
                                                              ("InFlow",     {"numEdgePoints" : 25}),
                                                              ("OutFlow",    {"numEdgePoints" : 25}),
                                                              ("2DSlice",    {"tessParams" : [0.50, .01, 45]})])

# Make quad/tri instead of just quads
#myProblem.analysis["aflr2AIM"].setAnalysisVal("Mesh_Gen_Input_String", "mquad=1 mpp=3")

# Run AIM pre-analysis
myProblem.analysis["aflr2AIM"].preAnalysis()

# NO analysis is needed - AFLR2 was already ran during preAnalysis

# Run AIM post-analysis
myProblem.analysis["aflr2AIM"].postAnalysis()

# Load SU2 aim - child of aflr2 AIM
su2 = myProblem.loadAIM(aim = "su2AIM",
                        altName = "su2",
                        analysisDir = workDir, parents = ["aflr2AIM"])

# Set project name
su2.setAnalysisVal("Proj_Name", projectName)

# Set AoA number
su2.setAnalysisVal("Alpha", 0.0)

# Set equation type
su2.setAnalysisVal("Equation_Type","Compressible")

# Set number of iterations
su2.setAnalysisVal("Num_Iter",10)

# Set the aim to 2D mode
su2.setAnalysisVal("Two_Dimensional", True)

# Set output file format
su2.setAnalysisVal("Output_Format","Tecplot")

# Set boundary conditions
inviscidBC = {"bcType" : "Inviscid"}

# Set Mach number
Mach = 0.4
gamma = 1.4
Pinf = 101300.0
Tinf = 280

# Set Mach number
su2.setAnalysisVal("Mach", Mach)

# Set boundary conditions
wallBC = {"bcType" : "Inviscid"}

backPressureBC = {"bcType" : "SubsonicOutflow",
                  "staticPressure" : Pinf}

inflow = {"bcType" : "SubsonicInflow",
          "totalPressure" : Pinf*(1+ (gamma-1.0)/2.0*Mach**2)**(gamma/(gamma-1)), # Isentropic relation
          "totalTemperature" : Tinf*(1+ (gamma-1.0)/2.0*Mach**2),
          "uVelocity" : 1.0,
          "vVelocity" : 0.0,
          "wVelocity" : 0.0}

su2.setAnalysisVal("Boundary_Condition", [("Airfoil"   , wallBC),
                                          ("TunnelWall", wallBC),
                                          ("InFlow", inflow),
                                          ("OutFlow",backPressureBC)])

# Specifcy the boundares used to compute forces
su2.setAnalysisVal("Surface_Monitor", ["Airfoil"])

# Run AIM pre-analysis
su2.preAnalysis()

####### Run su2 ####################
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(su2.analysisDir) # Move into test directory

su2Run(projectName + ".cfg", args.numberProc); # Run SU2

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
su2.postAnalysis()

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

# Save away CAPS problem
#myProblem.saveCAPS()

# Close CAPS
myProblem.closeCAPS()
