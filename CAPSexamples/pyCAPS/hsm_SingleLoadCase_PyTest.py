from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'HSM Pytest Example',
                                 prog = 'hsm_SingleLoadCase_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create project name
projectName = "HSMSingleLoadPlate"

# Working directory
workDir = os.path.join(str(args.workDir[0]), projectName)

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaSimplePlate.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load mystran aim
myAnalysis = myProblem.loadAIM(aim = "hsmAIM",
                  analysisDir = workDir)

# Set project name so a mesh file is generated
myAnalysis.setAnalysisVal("Proj_Name", projectName)

# Set meshing parameters
myAnalysis.setAnalysisVal("Edge_Point_Max", 10)
myAnalysis.setAnalysisVal("Edge_Point_Min", 9)

myAnalysis.setAnalysisVal("Quad_Mesh", True)

myAnalysis.setAnalysisVal("Tess_Params", [.25,.01,15])

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
          "shearMembraneRatio"  : 5.0/6.0} # Default

myAnalysis.setAnalysisVal("Property", ("plate", shell))

# Set constraints
constraint = {"groupName" : "plateEdge",
              "dofConstraint" : 123456}

myAnalysis.setAnalysisVal("Constraint", ("edgeConstraint", constraint))

# Set load
load = {"groupName" : "plate",
        "loadType" : "Pressure",
        "pressureForce" : 2.e6}

# Set loads
myAnalysis.setAnalysisVal("Load", ("appliedPressure", load ))

# Run AIM pre-analysis - Runs HSM
myAnalysis.preAnalysis()

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Close CAPS
myProblem.closeCAPS()
