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
parser = argparse.ArgumentParser(description = 'SU2 and AFLR2 PyTest Example',
                                 prog = 'su2_and_AFLR2_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Project name
projectName = "pyCAPS_su2_aflr2"

#NREL's S809 Airfoil coordinates
#          x          y
S809 = [1.000000,  0.000000,
        0.996203,  0.000487,
        0.985190,  0.002373,
        0.967844,  0.005960,
        0.945073,  0.011024,
        0.917488,  0.017033,
        0.885293,  0.023458,
        0.848455,  0.030280,
        0.807470,  0.037766,
        0.763042,  0.045974,
        0.715952,  0.054872,
        0.667064,  0.064353,
        0.617331,  0.074214,
        0.567830,  0.084095,
        0.519832,  0.093268,
        0.474243,  0.099392,
        0.428461,  0.101760,
        0.382612,  0.101840,
        0.337260,  0.100070,
        0.292970,  0.096703,
        0.250247,  0.091908,
        0.209576,  0.085851,
        0.171409,  0.078687,
        0.136174,  0.070580,
        0.104263,  0.061697,
        0.076035,  0.052224,
        0.051823,  0.042352,
        0.031910,  0.032299,
        0.016590,  0.022290,
        0.006026,  0.012615,
        0.000658,  0.003723,
        0.000204,  0.001942,
        0.000000, -0.000020,
        0.000213, -0.001794,
        0.001045, -0.003477,
        0.001208, -0.003724,
        0.002398, -0.005266,
        0.009313, -0.011499,
        0.023230, -0.020399,
        0.042320, -0.030269,
        0.065877, -0.040821,
        0.093426, -0.051923,
        0.124111, -0.063082,
        0.157653, -0.073730,
        0.193738, -0.083567,
        0.231914, -0.092442,
        0.271438, -0.099905,
        0.311968, -0.105281,
        0.353370, -0.108181,
        0.395329, -0.108011,
        0.438273, -0.104552,
        0.481920, -0.097347,
        0.527928, -0.086571,
        0.576211, -0.073979,
        0.626092, -0.060644,
        0.676744, -0.047441,
        0.727211, -0.035100,
        0.776432, -0.024204,
        0.823285, -0.015163,
        0.866630, -0.008204,
        0.905365, -0.003363,
        0.938474, -0.000487,
        0.965086,  0.000743,
        0.984478,  0.000775,
        0.996141,  0.000290,
        1.000000,  0.000000]

# Shape design variable is 1x200 need to fill in the rest of the array with "unused values"
for i in range(0, 200-len(S809)):
    S809.append(1.0E6)

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfd2DArbShape.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "SU2AFLR2ArbShapeAnalysisTest")

# Change a design parameter - area in the geometry
myProblem.geometry.setGeometryVal("xy", S809)

print ("Saving geometry")
myProblem.geometry.saveGeometry("GenericShape",
                                directory = workDir,
                                extension = "egads")

# Set intention - could also be set during .loadAIM
myProblem.capsIntent = "CFD"

# Load aflr2 aim
myMesh = myProblem.loadAIM(aim = "aflr2AIM", analysisDir = workDir)

# Set project name
myMesh.setAnalysisVal("Proj_Name", projectName)

myMesh.setAnalysisVal("Mesh_Sizing", [("Airfoil"   , {"numEdgePoints" : 400}),
                                      ("TunnelWall", {"numEdgePoints" : 50}),
                                      ("InFlow",     {"numEdgePoints" : 25}),
                                      ("OutFlow",    {"numEdgePoints" : 25}),
                                      ("2DSlice",    {"tessParams" : [0.50, .01, 45]})])

# Make quad/tri instead of just quads
myMesh.setAnalysisVal("Mesh_Gen_Input_String", "mquad=1 mpp=3")

# Set output grid format since a project name is being supplied - Tecplot  file
myMesh.setAnalysisVal("Mesh_Format", "Tecplot")

# Set verbosity
myMesh.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Run AIM pre-analysis
myMesh.preAnalysis()

# NO analysis is needed - AFLR2 was already ran during preAnalysis

# Run AIM post-analysis
myMesh.postAnalysis()

# Load SU2 aim - child of aflr2 AIM
su2 = myProblem.loadAIM(aim = "su2AIM",
                        altName = "su2",
                        analysisDir = workDir, parents = ["aflr2AIM"])

# Set SU2 Version
su2.setAnalysisVal("SU2_Version","Falcon")

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
myProblem.analysis["su2"].setAnalysisVal("Output_Format","Tecplot")

# Set Mach number
su2.setAnalysisVal("Mach", 0.4)

# Set boundary conditions
inviscidBC = {"bcType" : "Inviscid"}

backPressureBC = {"bcType" : "SubsonicOutflow",
                  "staticPressure" : 101300.0}

inflow = {"bcType" : "SubsonicInflow",
          "totalPressure" : 102010.0,
          "totalTemperature" : 288.6,
          "uVelocity" : 1.0,
          "vVelocity" : 0.0,
          "wVelocity" : 0.0}

su2.setAnalysisVal("Boundary_Condition", [("Airfoil"   , inviscidBC),
                                          ("TunnelWall", inviscidBC),
                                          ("InFlow", inflow),
                                          ("OutFlow",backPressureBC)])

# Specifcy the boundares used to compute forces
su2.setAnalysisVal("Surface_Monitor", ["Airfoil"])

# Run AIM pre-analysis
su2.preAnalysis()

####### Run SU2 ####################
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

# Close CAPS
myProblem.closeCAPS()
