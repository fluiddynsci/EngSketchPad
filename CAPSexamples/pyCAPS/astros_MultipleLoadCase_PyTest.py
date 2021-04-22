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
parser = argparse.ArgumentParser(description = 'Astros Multiple Load Pytest Example',
                                 prog = 'astros_MultipleLoadCase_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ".", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaSimplePlate.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Create project name
projectName = "AstrosMultipleLoadPlate"

# Load Astros aim
myAnalysis = myProblem.loadAIM( aim = "astrosAIM",
                                altName = "astros",
                                analysisDir = os.path.join(str(args.workDir[0]), projectName) )

# Set project name so a mesh file is generated
myAnalysis.setAnalysisVal("Proj_Name", projectName)

# Set meshing parameters
myAnalysis.setAnalysisVal("Edge_Point_Max", 4)
myAnalysis.setAnalysisVal("Edge_Point_Min", 4)

myAnalysis.setAnalysisVal("Quad_Mesh", True)

myAnalysis.setAnalysisVal("Tess_Params", [.25,.01,15])

# Set analysis type
myAnalysis.setAnalysisVal("Analysis_Type", "Static");

# Set materials
madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 72.0E9 ,
                "poissonRatio": 0.33,
                "density" : 2.8E3}

myAnalysis.setAnalysisVal("Material", ("Madeupium", madeupium))

# Set properties
shell  = {"propertyType" : "Shell",
          "membraneThickness" : 0.006,
          "material"        : "madeupium",
          "bendingInertiaRatio" : 1.0, # Default
          "shearMembraneRatio"  : 5.0/6.0} # Default

myAnalysis.setAnalysisVal("Property", ("plate", shell))

# Set constraints
constraint = {"groupName" : "plateEdge",
              "dofConstraint" : 123456}

myAnalysis.setAnalysisVal("Constraint", ("edgeConstraint", constraint))

# Create multiple loads
numLoad = 5
loads = []
for i in range(numLoad):
    # Set load name
    name = "load_"+ str(i)

    # Set load values
    value =  {"groupName" : "plate",
              "loadType" : "Pressure",
              "pressureForce" : 1.e6*(i+1)}

    # Create temporary load tuple
    loadElement = (name, value)

    # Append loadElement to laod
    loads.append( loadElement)

# Set loads
myAnalysis.setAnalysisVal("Load", loads)

# Create multiple analysis cases
analysisCases = []
for i in range(numLoad):

    # Set analysis name
    name = loads[i][0]

    # set analysis value s
    value = {"analysisType" : "Static",
             "analysisLoad" : name}

    # Create temporary analysis tuple
    analysisElement = (name, value)

    # Append analysisElement to analysis cases
    analysisCases.append(analysisElement)

# Set analysis
myAnalysis.setAnalysisVal("Analysis", analysisCases)

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
######################################

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Close CAPS
myProblem.closeCAPS()
