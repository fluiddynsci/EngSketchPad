from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
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
parser = argparse.ArgumentParser(description = 'Astros Composite PyTest Example',
                                 prog = 'astros_Composite_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaCantileverPlate.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load astros aim
myProblem.loadAIM( aim = "astrosAIM",
                   altName = "astros",
                   analysisDir= os.path.join(str(args.workDir[0]), "AstrosCompositeTest") )

# Set project name so a mesh file is generated
projectName = "pyCAPS_astros_Test"
myProblem.analysis["astros"].setAnalysisVal("Proj_Name", projectName)

myProblem.analysis["astros"].setAnalysisVal("Edge_Point_Max", 10)

myProblem.analysis["astros"].setAnalysisVal("Quad_Mesh", True)

myProblem.analysis["astros"].setAnalysisVal("File_Format", "Small")

myProblem.analysis["astros"].setAnalysisVal("Mesh_File_Format", "Large")

# Set analysis
eigen = { "extractionMethod"     : "MGIV",
          "frequencyRange"       : [0, 10000],
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 4,
          "eigenNormaliztion"    : "MASS"}
myProblem.analysis["astros"].setAnalysisVal("Analysis", ("EigenAnalysis", eigen))

# Set materials
unobtainium  = {"youngModulus" : 2.2E6 ,
                "poissonRatio" : .5,
                "density"      : 7850}

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E5 ,
                "poissonRatio" : .5,
                "density"      : 7850}
myProblem.analysis["astros"].setAnalysisVal("Material", [("Unobtainium", unobtainium),
                                                          ("Madeupium", madeupium)])

# Set property
shell  = {"propertyType"           : "Composite",
          "shearBondAllowable"     : 1.0e6,
          "bendingInertiaRatio"    : 1.0, # Default - not necesssary
          "shearMembraneRatio"     : 0, # Turn off shear - no materialShear
          "membraneThickness"      : 0.2,
          "compositeMaterial"      : ["Unobtainium", "Madeupium", "Madeupium"],
          "compositeFailureTheory" : "STRN",
          "compositeThickness"     : [1.2, 0.5, 2.0],
          "compositeOrientation"   : [30.6, 45, 50.4]}

myProblem.analysis["astros"].setAnalysisVal("Property", ("plate", shell))

# Set constraints
constraint = {"groupName" : ["plateEdge"],
              "dofConstraint" : 123456}

myProblem.analysis["astros"].setAnalysisVal("Constraint", ("cantilever", constraint))


# Run AIM pre-analysis
myProblem.analysis["astros"].preAnalysis()

####### Run Astros####################
print ("\n\nRunning Astros......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myProblem.analysis["astros"].analysisDir) # Move into test directory

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

# Run AIM post-analysis
myProblem.analysis["astros"].postAnalysis()

# Close CAPS
myProblem.closeCAPS()
