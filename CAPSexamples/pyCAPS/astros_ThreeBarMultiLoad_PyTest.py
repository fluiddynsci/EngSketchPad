## [importPrint]
from __future__ import print_function
## [importPrint]

## [import]
# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import sys
import shutil
import argparse
## [import]

# ASTROS_ROOT should be the path containing ASTRO.D01 and ASTRO.IDX
try:
   ASTROS_ROOT = os.environ["ASTROS_ROOT"]
   os.putenv("PATH", ASTROS_ROOT + os.pathsep + os.getenv("PATH"))
except KeyError:
   print("Please set the environment variable ASTROS_ROOT")
   sys.exit(1)

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Astros Three Bar Multiple Load PyTest Example',
                                 prog = 'astros_ThreeBarMultiLoad_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
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
# Load astros aim
astrosAIM = myProblem.loadAIM( aim = "astrosAIM",
                               altName = "astros",
                               analysisDir= os.path.join(str(args.workDir[0]), "AstrosThreeBarMultiLoad") )
## [loadAIM]

## [setInputs]
# Set project name so a mesh file is generated
projectName = "threebar_astros_Test"
astrosAIM.setAnalysisVal("Proj_Name", projectName)
astrosAIM.setAnalysisVal("File_Format", "Free")
astrosAIM.setAnalysisVal("Mesh_File_Format", "Large")
astrosAIM.setAnalysisVal("Edge_Point_Max", 2);
astrosAIM.setAnalysisVal("Edge_Point_Min", 2);
astrosAIM.setAnalysisVal("Analysis_Type", "Static");
## [setInputs]

## [defineMaterials]
madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.0E7 ,
                "poissonRatio" : .33,
                "density"      : 0.1}

astrosAIM.setAnalysisVal("Material", [("Madeupium", madeupium)])
## [defineMaterials]

## [defineProperties]
rod  =   {"propertyType"      : "Rod",
          "material"          : "Madeupium",
          "crossSecArea"      : 1.0}

rod2  =   {"propertyType"     : "Rod",
          "material"          : "Madeupium",
          "crossSecArea"      : 2.0}

astrosAIM.setAnalysisVal("Property", [("bar1", rod),
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

astrosAIM.setAnalysisVal("Constraint", constraints)
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

astrosAIM.setAnalysisVal("Load", loads)
## [defineLoad]

## [defineAnalysis]
analysisCases = []

value = {"analysisType"         : "Static",
         "analysisConstraint"   : "conOne",
         "analysisLoad"         : "loadOne"}
analysisCases.append( ("analysisOne", value ) )

value = {"analysisType"         : "Static",
         "analysisConstraint"   : "conTwo",
         "analysisLoad"         : "loadTwo"}
analysisCases.append( ("analysisTwo", value ) )

myProblem.analysis["astros"].setAnalysisVal("Analysis", analysisCases)
## [defineAnalysis]

# Run AIM pre-analysis
## [preAnalysis]
astrosAIM.preAnalysis()
## [preAnalysis]

## [run]
print ("\n\nRunning Astros......")
currentDirectory = os.getcwd() # Get our current working directory
os.chdir(astrosAIM.analysisDir) # Move into test directory

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
## [run]

## [postAnalysis]
astrosAIM.postAnalysis()
## [postAnalysis]

# Close CAPS
## [close]
myProblem.closeCAPS()
## [close]
