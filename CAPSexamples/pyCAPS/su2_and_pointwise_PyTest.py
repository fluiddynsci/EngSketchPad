from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import modules
import os
import argparse
import platform

# Import SU2 Python interface module
from parallel_computation import parallel_computation as su2Run


# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'SU2 and Pointwise Pytest Example',
                                 prog = 'su2_and_pointwise_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

workDir = os.path.join(str(args.workDir[0]), "SU2PointwiseAnalysisTest")

projectName = "pyCAPS_SU2_Pointwise"

# -----------------------------------------------------------------
# Initialize capsProblem object
# -----------------------------------------------------------------
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load pointwise aim
pointwise = myProblem.loadAIM(aim = "pointwiseAIM",
                              analysisDir= workDir)

# Global Min/Max number of points on edges
pointwise.setAnalysisVal("Connector_Initial_Dim", 10)
pointwise.setAnalysisVal("Connector_Min_Dim", 3)
pointwise.setAnalysisVal("Connector_Max_Dim", 15)

# Run AIM pre-analysis
pointwise.preAnalysis()

####### Run pointwise ####################
currentDir = os.getcwd()
os.chdir(pointwise.analysisDir)

CAPS_GLYPH = os.environ["CAPS_GLYPH"]
if platform.system() == "Windows":
    PW_HOME = os.environ["PW_HOME"]
    os.system(PW_HOME + "\win64\bin\tclsh.exe " + CAPS_GLYPH + " caps.egads capsUserDefaults.glf")
else:
    os.system("pointwise -b " + CAPS_GLYPH + " caps.egads capsUserDefaults.glf")

os.chdir(currentDir)
##########################################

# Run AIM post-analysis
pointwise.postAnalysis()

# Load SU2 aim - child of AFLR3 AIM
su2 = myProblem.loadAIM(aim = "su2AIM",
                        altName = "su2",
                        analysisDir= workDir, parents = ["pointwiseAIM"])

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

# Set the output file formats
su2.setAnalysisVal("Output_Format", "Tecplot")

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

####### Run SU2 ######################
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
