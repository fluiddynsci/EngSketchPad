## [importPrint]
from __future__ import print_function
## [importPrint]

## [import]
# Import pyCAPS class file
from pyCAPS import capsProblem

# Import modules
import os
import sys
import shutil
import argparse
## [import]

## [environment]
# ASTROS_ROOT should be the path containing ASTRO.D01 and ASTRO.IDX
try:
   ASTROS_ROOT = os.environ["ASTROS_ROOT"]
   os.putenv("PATH", ASTROS_ROOT + os.pathsep + os.getenv("PATH"))
except KeyError:
   print("Please set the environment variable ASTROS_ROOT")
   sys.exit(1)
## [environment]

## [arguments]
# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Astros CompositeWingFreq PyTest Example',
                                 prog = 'astros_CompositeWingFreq_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ".", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()
## [arguments]

## [initateProblem]
# Initialize pyCAPS object
myProblem = capsProblem()
## [initateProblem]

## [geometry]
# Load CSM file
geometryScript = os.path.join("..","csmData","compositeWing.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
## [geometry]

## [loadAIM]
# Load astros aim
myAnalysis = myProblem.loadAIM( aim = "astrosAIM",
                                altName = "astros",
                                analysisDir = os.path.join(str(args.workDir[0]), "AstrosCompositeWing_Freq") )
## [loadAIM]

# Setup bound information for visualization
numEigenVector = 5
eigenVector = []
for i in range(numEigenVector):
    eigenVector.append("EigenVector_" + str(i+1))

transfers = ["wing"]
for i in transfers:
    myProblem.createDataBound(variableName = eigenVector,
                              aimSrc = ["astros"]*len(eigenVector),
                              capsBound = i)

## [setInputs]
# Set project name so a mesh file is generated
projectName = "astros_CompositeWing"
myAnalysis.setAnalysisVal("Proj_Name", projectName)
myAnalysis.setAnalysisVal("Edge_Point_Max", 10)
myAnalysis.setAnalysisVal("File_Format", "Small")
myAnalysis.setAnalysisVal("Mesh_File_Format", "Large")
myAnalysis.setAnalysisVal("Analysis_Type", "Modal");
## [setInputs]

## [defineMaterials]
Aluminum  = {"youngModulus" : 10.5E6 ,
             "poissonRatio" : 0.3,
             "density"      : 0.1/386,
             "shearModulus" : 4.04E6}

Graphite_epoxy = {"materialType"        : "Orthotropic",
                  "youngModulus"        : 20.8E6 ,
                  "youngModulusLateral" : 1.54E6,
                  "poissonRatio"        : 0.327,
                  "shearModulus"        : 0.80E6,
                  "density"             : 0.059/386,
                  "tensionAllow"        : 11.2e-3,
                  "tensionAllowLateral" : 4.7e-3,
                  "compressAllow"       : 11.2e-3,
                  "compressAllowLateral": 4.7e-3,
                  "shearAllow"          : 19.0e-3,
                  "allowType"           : 1}

myAnalysis.setAnalysisVal("Material", [("Aluminum", Aluminum),
                                       ("Graphite_epoxy", Graphite_epoxy)])
## [defineMaterials]

## [defineProperties]
aluminum  = {"propertyType"         : "Shell",
             "material"             : "Aluminum",
             "bendingInertiaRatio"  : 1.0, # Default - not necesssary
             "shearMembraneRatio"   : 0, # Turn of shear - no materialShear
             "membraneThickness"    : 0.125 }

composite  = {"propertyType"           : "Composite",
              "shearBondAllowable"     : 1.0e6,
              "bendingInertiaRatio"    : 1.0, # Default - not necesssary
              "shearMembraneRatio"     : 0, # Turn off shear - no materialShear
              "compositeMaterial"      : ["Graphite_epoxy"]*8,
              "compositeThickness"     : [0.00525]*8,
              "compositeOrientation"   : [0, 0, 0, 0, -45, 45, -45, 45],
              "symmetricLaminate"      : True,
              "compositeFailureTheory" : "STRAIN" }

#myAnalysis.setAnalysisVal("Property", ("wing", aluminum))
myAnalysis.setAnalysisVal("Property", ("wing", composite))
## [defineProperties]

## [defineConstraints]
constraint = {"groupName" : "root",
              "dofConstraint" : 123456}

myAnalysis.setAnalysisVal("Constraint", ("root", constraint))
## [defineConstraints]

## [defineAnalysis]
eigen = { "extractionMethod"     : "MGIV",
          "frequencyRange"       : [0, 1000],
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 10,
          "eigenNormaliztion"    : "MASS"}

myAnalysis.setAnalysisVal("Analysis", ("EigenAnalysis", eigen))
## [defineAnalysis]

## [preAnalysis]
myAnalysis.preAnalysis()
## [preAnalysis]

## [run]
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
    os.system("astros.exe < " + projectName +  ".dat > " + projectName + ".out"); # Run Astros via system call

os.chdir(currentDirectory) # Move back to working directory
print ("Done running Astros!")
######################################
## [run]

## [postAnalysis]
# Run AIM post-analysis
myAnalysis.postAnalysis()
## [postAnalysis]

# Get Eigen-frequencies
print ("\nGetting results for natural frequencies.....")
natrualFreq = myProblem.analysis["astros"].getAnalysisOutVal("EigenFrequency")

mode = 1
for i in natrualFreq:
    print ("Natural freq (Mode {:d}) = ".format(mode) + '{:.5f} '.format(i) + "(Hz)")
    mode += 1

for i in transfers:
    for eigenName in eigenVector:
        #myProblem.dataBound[i].executeTransfer(eigenName)
        #myProblem.dataBound[i].dataSetSrc[eigenName].viewData()
        #myProblem.dataBound[i].dataSetSrc[eigenName].writeTecplot(filename = myAnalysis.analysisDir + "/Data")
        #myProblem.dataBound[i].viewData()
        pass
    myProblem.dataBound[i].writeTecplot(myAnalysis.analysisDir + "/Data_All_Eigen")


## [close]
# Close CAPS
myProblem.closeCAPS()
## [close]
