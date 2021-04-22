from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import modules
import os
import sys
import shutil
import argparse

# ASTROS_ROOT should be the path containing ASTRO.D01 and ASTRO.IDX
try:
   ASTROS_ROOT = os.environ["ASTROS_ROOT"]
   os.putenv("PATH", ASTROS_ROOT + os.pathsep + os.getenv("PATH"))
except KeyError:
   print("Please set the environment variable ASTROS_ROOT")
   sys.exit(1)

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Astros AGARD445.6 Pytest Example',
                                 prog = 'astros_AGARD445_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ".", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaAGARD445.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Create project name
projectName = "AstrosModalAGARD445"

# Change the sweepAngle and span of the Geometry - Demo purposes
#myProblem.geometry.setGeometryVal("sweepAngle", 5) # From 45 to 5 degrees
#myProblem.geometry.setGeometryVal("semiSpan", 5)   # From 2.5 ft to 5 ft

# Load astros aim
myAnalysis = myProblem.loadAIM(aim = "astrosAIM",
                               altName = "astros",
                               analysisDir = str(args.workDir[0]) + os.sep + projectName)

# Set project name
myAnalysis.setAnalysisVal("Proj_Name", projectName)

# Set meshing parameters
myAnalysis.setAnalysisVal("Edge_Point_Max", 10)
myAnalysis.setAnalysisVal("Edge_Point_Min", 6)

myAnalysis.setAnalysisVal("Quad_Mesh", True)

myAnalysis.setAnalysisVal("Tess_Params", [.25,.01,15])

# Set analysis type
myAnalysis.setAnalysisVal("Analysis_Type", "Modal");

# Set analysis inputs
eigen = { "extractionMethod"     : "MGIV", # "Lanczos",
          "frequencyRange"       : [0.1, 200],
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 2,
          "eigenNormaliztion"    : "MASS",
	      "lanczosMode"          : 2,  # Default - not necesssary
          "lanczosType"          : "DPB"} # Default - not necesssary

myAnalysis.setAnalysisVal("Analysis", ("EigenAnalysis", eigen))

# Set materials
mahogany    = {"materialType"        : "orthotropic",
               "youngModulus"        : 0.457E6 ,
               "youngModulusLateral" : 0.0636E6,
               "poissonRatio"        : 0.31,
               "shearModulus"        : 0.0637E6,
               "shearModulusTrans1Z" : 0.00227E6,
               "shearModulusTrans2Z" : 0.00227E6,
               "density"             : 3.5742E-5}

myAnalysis.setAnalysisVal("Material", ("Mahogany", mahogany))

# Set properties
shell  = {"propertyType" : "Shell",
          "membraneThickness" : 0.82,
          "material"        : "mahogany",
          "bendingInertiaRatio" : 1.0, # Default - not necesssary
          "shearMembraneRatio"  : 5.0/6.0} # Default - not necesssary

myAnalysis.setAnalysisVal("Property", ("yatesPlate", shell))

# Set constraints
constraint = {"groupName" : "constEdge",
              "dofConstraint" : 123456}

myAnalysis.setAnalysisVal("Constraint", ("edgeConstraint", constraint))

# Run AIM pre-analysis
myAnalysis.preAnalysis()

####### Run Astros ####################
print ("\n\nRunning Astros......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myAnalysis.analysisDir) # Move into test directory

# Copy files needed to run astros
astros_files = ["ASTRO.D01",  # *.DO1 file
                "ASTRO.IDX"]  # *.IDX file
for file in astros_files:
    if not os.path.isfile(file):
        shutil.copy(ASTROS_ROOT + os.sep + file, file)

if (args.noAnalysis == False):
    # Run Astros via system call
    os.system("astros.exe < " + projectName +  ".dat > " + projectName + ".out");

os.chdir(currentDirectory) # Move back to working directory

print ("Done running Astros!")
########################################

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Get Eigen-frequencies
print ("\nGetting results natural frequencies.....")
natrualFreq = myAnalysis.getAnalysisOutVal("EigenFrequency")

mode = 1
for i in natrualFreq:
    print ("Natural freq (Mode {:d}) = ".format(mode) + '{:.2f} '.format(i) + "(Hz)")
    mode += 1

# Close CAPS
myProblem.closeCAPS()
