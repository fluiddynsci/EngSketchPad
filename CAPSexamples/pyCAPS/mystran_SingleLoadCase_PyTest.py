from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'MYSTRAN Pytest Example',
                                 prog = 'mystran_SingleLoadCase_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create project name
projectName = "MystranSingleLoadPlate"

# Working directory
workDir = os.path.join(str(args.workDir[0]), projectName)

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaSimplePlate.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load mystran aim
myProblem.loadAIM(aim = "mystranAIM",
                  altName = "mystran",
                  analysisDir = workDir)

# Set project name so a mesh file is generated
myProblem.analysis["mystran"].setAnalysisVal("Proj_Name", projectName)

# Set meshing parameters
myProblem.analysis["mystran"].setAnalysisVal("Edge_Point_Max", 4)

myProblem.analysis["mystran"].setAnalysisVal("Quad_Mesh", True)

myProblem.analysis["mystran"].setAnalysisVal("Tess_Params", [.25,.01,15])

# Set analysis type
myProblem.analysis["mystran"].setAnalysisVal("Analysis_Type", "Static")

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

# Set load
load = {"groupName" : "plate",
        "loadType" : "Pressure",
        "pressureForce" : 2.e6}

# Set loads
myProblem.analysis["mystran"].setAnalysisVal("Load", ("appliedPressure", load ))

# Set analysis
# No analysis case information needs to be set for a single static load case

# Run AIM pre-analysis
myProblem.analysis["mystran"].preAnalysis()

####### Run MyStran ####################
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
