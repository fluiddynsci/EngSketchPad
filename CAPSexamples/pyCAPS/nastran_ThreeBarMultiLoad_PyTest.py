## [importPrint]
from __future__ import print_function
## [importPrint]

## [import]
# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse
## [import]

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Nastran Three Bar Pytest Example',
                                 prog = 'nastran_ThreeBar_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

## [initateProblem]
# Initialize capsProblem object
myProblem = capsProblem()
## [initateProblem]

## [geometry]
# Load CSM file
geometryScript = os.path.join("..","csmData","feaThreeBar.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
## [geometry]

## [loadAIM]
# Load nastran aim
nastranAIM = myProblem.loadAIM(aim = "nastranAIM",
                               altName = "nastran",
                               analysisDir= os.path.join(str(args.workDir[0]), "NastranThreeBarMultiLoad"),
                               capsIntent = "STRUCTURE")
## [loadAIM]

## [setInputs]
# Set project name so a mesh file is generated
projectName = "threebar_nastran_Test"
nastranAIM.setAnalysisVal("Proj_Name", projectName)
nastranAIM.setAnalysisVal("File_Format", "Free")
nastranAIM.setAnalysisVal("Mesh_File_Format", "Large")
nastranAIM.setAnalysisVal("Edge_Point_Max", 2);
nastranAIM.setAnalysisVal("Edge_Point_Min", 2);
nastranAIM.setAnalysisVal("Analysis_Type", "Static");
## [setInputs]

## [defineMaterials]
madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.0E7 ,
                "poissonRatio" : .33,
                "density"      : 0.1}

nastranAIM.setAnalysisVal("Material", [("Madeupium", madeupium)])
## [defineMaterials]

## [defineProperties]
rod  =   {"propertyType"      : "Rod",
          "material"          : "Madeupium",
          "crossSecArea"      : 1.0}

rod2  =   {"propertyType"     : "Rod",
          "material"          : "Madeupium",
          "crossSecArea"      : 2.0}

nastranAIM.setAnalysisVal("Property", [("bar1", rod),
                                       ("bar2", rod2),
                                       ("bar3", rod)])
## [defineProperties]

# Set constraints
## [defineConstraints]
constraints = []

constraint = {"groupName"         : ["boundary"],
              "dofConstraint"     : 123456}
tmp = ( "conOne", constraint )
constraints.append( tmp )

constraint = {"groupName"         : ["boundary"],
              "dofConstraint"     : 123}
tmp = ( "conTwo", constraint )
constraints.append( tmp )

nastranAIM.setAnalysisVal("Constraint", constraints)
## [defineConstraints]

## [defineLoad]
loads = []

load = {"groupName"         : "force",
        "loadType"          : "GridForce",
        "forceScaleFactor"  : 20000.0,
        "directionVector"   : [0.8, -0.6, 0.0]}
loads.append( ("loadOne", load) )

load = {"groupName"         : "force",
        "loadType"          : "GridForce",
        "forceScaleFactor"  : 20000.0,
        "directionVector"   : [-0.8, -0.6, 0.0]}
loads.append( ("loadTwo", load) )

nastranAIM.setAnalysisVal("Load", loads)
## [defineLoad]

## [defineAnalysis]
analysisCases = []

value = {"analysisType"         : "Static",
         "analysisConstraint"     : "conOne",
         "analysisLoad"         : "loadOne"}
analysisCases.append( ("analysisOne", value ) )

value = {"analysisType"         : "Static",
         "analysisConstraint"     : "conTwo",
         "analysisLoad"         : "loadTwo"}
analysisCases.append( ("analysisTwo", value ) )

myProblem.analysis["nastran"].setAnalysisVal("Analysis", analysisCases)
## [defineAnalysis]

# Run AIM pre-analysis
## [preAnalysis]
nastranAIM.preAnalysis()
## [preAnalysis]

## [run]
print ("\n\nRunning Nastran......")
currentDirectory = os.getcwd() # Get our current working directory
os.chdir(nastranAIM.analysisDir) # Move into test directory
if (args.noAnalysis == False): os.system("nastran old=no notify=no batch=no scr=yes sdirectory=./ " + projectName +  ".dat"); # Run Nastran via system call
os.chdir(currentDirectory) # Move back to working directory
print ("Done running Nastran!")
## [run]

## [postAnalysis]
nastranAIM.postAnalysis()
## [postAnalysis]

# Close CAPS
## [close]
myProblem.closeCAPS()
## [close]
