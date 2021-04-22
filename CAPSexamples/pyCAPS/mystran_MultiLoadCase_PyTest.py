from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Mystran MultiLoadCase Example',
                                 prog = 'mystran_MultiLoadCase_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create project name
projectName = "MystranMultiLoadPlate"

# Working directory
workDir = os.path.join(str(args.workDir[0]), projectName)

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaSimplePlate.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load mystran aim
myProblem.loadAIM(aim = "mystranAIM", altName = "mystran", analysisDir = workDir)

# Set project name so a mesh file is generated
myProblem.analysis["mystran"].setAnalysisVal("Proj_Name", projectName)

# Set meshing parameters
myProblem.analysis["mystran"].setAnalysisVal("Edge_Point_Max", 4)

myProblem.analysis["mystran"].setAnalysisVal("Quad_Mesh", True)

myProblem.analysis["mystran"].setAnalysisVal("Tess_Params", [.25,.01,15])

# Set analysis type
myProblem.analysis["mystran"].setAnalysisVal("Analysis_Type", "Static");

# Set materials
madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 72.0E9 ,
                "poissonRatio": 0.33,
                "density" : 2.8E3}

myProblem.analysis["mystran"].setAnalysisVal("Material", ("Madeupium", madeupium))

# Set properties
shell  = {"propertyType" : "Shell",
          "membraneThickness" : 0.006,
          "material"        : "madeupium",
          "bendingInertiaRatio" : 1.0, # Default
          "shearMembraneRatio"  : 5.0/6.0} # Default

myProblem.analysis["mystran"].setAnalysisVal("Property", ("plate", shell))

# Set constraints
constraint = {"groupName" : "plateEdge",
              "dofConstraint" : 123456}

myProblem.analysis["mystran"].setAnalysisVal("Constraint", ("edgeConstraint", constraint))

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
myProblem.analysis["mystran"].setAnalysisVal("Load", loads)

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
myProblem.analysis["mystran"].setAnalysisVal("Analysis", analysisCases)

# Run AIM pre-analysis
myProblem.analysis["mystran"].preAnalysis()

####### Run MYSTRAN ####################
print ("\n\nRunning MYSTRAN......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myProblem.analysis["mystran"].analysisDir) # Move into test directory

os.system("mystran.exe " + projectName +  ".dat"); # Run mystran via system call

os.chdir(currentDirectory) # Move back to working directory
print ("Done running MYSTRAN!")
######################################

# Run AIM post-analysis
myProblem.analysis["mystran"].postAnalysis()

# Close CAPS
myProblem.closeCAPS()
