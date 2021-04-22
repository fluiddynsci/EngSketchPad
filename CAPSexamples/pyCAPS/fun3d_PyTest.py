from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'FUN3D Pytest Example',
                                 prog = 'fun3d_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load FUN3D aim
myProblem.loadAIM(aim = "fun3dAIM",
                  altName = "fun3d",
                  analysisDir = os.path.join(str(args.workDir[0]), "FUN3DAnalysisTest"))

# Set reference parameters
#myProblem.analysis["fun3d"].setAnalysisVal("Reference_Area",10) # Get value from capsReferenceArea instead
#myProblem.analysis["fun3d"].setAnalysisVal("Moment_Length",[1.4, 4.0]) # Get values from capsReferenceChord/Span
myProblem.analysis["fun3d"].setAnalysisVal("Moment_Center",[1.0, 0.0, 0.5])

# Set AoA number
myProblem.analysis["fun3d"].setAnalysisVal("Alpha", 1.0)

# Set Mach number
myProblem.analysis["fun3d"].setAnalysisVal("Mach", 1.372)

# Set equation type
myProblem.analysis["fun3d"].setAnalysisVal("Equation_Type","compressible")

# Set number of iterations
myProblem.analysis["fun3d"].setAnalysisVal("Num_Iter",10)

# Set CFL schedule
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule",[0.5, 3.0])

# Set CFL number iteration schedule
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule_Iter", [1, 40])

# Set boundary conditions
viscousBC  = {"bcType" : "Viscous" , "wallTemperature" : 1}
inviscidBC = {"bcType" : "Inviscid", "wallTemperature" : 1.2}
myProblem.analysis["fun3d"].setAnalysisVal("Boundary_Condition", [("Wing1", inviscidBC),
                                                                  ("Wing2", viscousBC),
                                                                  ("Farfield","Farfield")])

# Set overwrite fun3d.nml if not linking to Python library
myProblem.analysis["fun3d"].setAnalysisVal("Overwrite_NML", True)

# Set use Python to write fun3d.nml if linking to Python library
myProblem.analysis["fun3d"].setAnalysisVal("Use_Python_NML", True)

# Run AIM pre-analysis
myProblem.analysis["fun3d"].preAnalysis()

#######################################
#### Run FUN3D - (via system call) ####
#######################################
# Not runnning fun3d - no mesh

# Run AIM post-analysis
myProblem.analysis["fun3d"].postAnalysis()

# Close CAPS
myProblem.closeCAPS()
