from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Nastran Aeroelastic Pytest Example',
                                 prog = 'nastran_Aeroelastic_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaSimplePlate.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Create project name
projectName = "NastranMultiLoadPlate"

# Load nastran aim
myProblem.loadAIM(aim = "nastranAIM",
                  altName = "nastran",
                  analysisDir = os.path.join(str(args.workDir[0]), projectName),
                  capsIntent = "STRUCTURE")

# Set project name so a mesh file is generated
myProblem.analysis["nastran"].setAnalysisVal("Proj_Name", projectName)

# Set meshing parameters
myProblem.analysis["nastran"].setAnalysisVal("Edge_Point_Max", 5)
myProblem.analysis["nastran"].setAnalysisVal("Edge_Point_Min", 5)

myProblem.analysis["nastran"].setAnalysisVal("Quad_Mesh", True)

myProblem.analysis["nastran"].setAnalysisVal("Tess_Params", [.25,.01,15])

# Set analysis type
myProblem.analysis["nastran"].setAnalysisVal("Analysis_Type", "Static");

# Set materials
madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 72.0E9 ,
                "poissonRatio": 0.33,
                "density" : 2.8E3}

myProblem.analysis["nastran"].setAnalysisVal("Material", ("Madeupium", madeupium))

# Set properties
shell  = {"propertyType" : "Shell",
          "membraneThickness" : 0.006,
          "material"        : "madeupium",
          "bendingInertiaRatio" : 1.0, # Default
          "shearMembraneRatio"  : 5.0/6.0} # Default

myProblem.analysis["nastran"].setAnalysisVal("Property", ("plate", shell))

# Set constraints
constraint = {"groupName" : "plateEdge",
              "dofConstraint" : 123456}

myProblem.analysis["nastran"].setAnalysisVal("Constraint", ("edgeConstraint", constraint))

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
myProblem.analysis["nastran"].setAnalysisVal("Load", loads)

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
myProblem.analysis["nastran"].setAnalysisVal("Analysis", analysisCases)

# Run AIM pre-analysis
myProblem.analysis["nastran"].preAnalysis()

####### Run Nastran ####################
print ("\n\nRunning Nastran......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myProblem.analysis["nastran"].analysisDir) # Move into test directory

if (args.noAnalysis == False):
    os.system("nastran old=no notify=no batch=no scr=yes sdirectory=./ " + projectName +  ".dat"); # Run Nastran via system call

os.chdir(currentDirectory) # Move back to working directory
print ("Done running Nastran!")
######################################

# Run AIM post-analysis
myProblem.analysis["nastran"].postAnalysis()

# Close CAPS
myProblem.closeCAPS()
