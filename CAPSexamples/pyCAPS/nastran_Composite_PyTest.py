from __future__ import print_function

# Import pyCAPS class file
import pyCAPS

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Nastran Composite Pytest Example',
                                 prog = 'nastran_Composite_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-outLevel", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
projectName = "MystranModalWingBEM"
workDir = os.path.join(str(args.workDir[0]), projectName)
geometryScript = os.path.join("..","csmData","feaCantileverPlate.csm")

myProblem = pyCAPS.Problem(problemName=workDir,
                         capsFile=geometryScript,
                         outLevel=args.outLevel)
geometry = myProblem.geometry

# Load nastran aim
myAnalysis = myProblem.analysis.create(aim = "nastranAIM",
                                       name = "nastran", 
                                       autoExec = False)

# Initialize capsProblem

# Set project name so a mesh file is generated
projectName = "pyCAPS_nastran_Test"
myAnalysis.input.Proj_Name = projectName
myAnalysis.input.Edge_Point_Max = 10
myAnalysis.input.Quad_Mesh = True
myAnalysis.input.File_Format = "Small"
myAnalysis.input.Mesh_File_Format = "Large"

# Set analysis
eigen = { "extractionMethod"     : "Lanczos",
          "frequencyRange"       : [0, 10000],
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 4,
          "eigenNormaliztion"    : "MASS"}
myAnalysis.input.Analysis = {"EigenAnalysis": eigen}

# Set materials
unobtainium  = {"youngModulus" : 2.2E6 ,
                "poissonRatio" : .5,
                "density"      : 7850}

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E5 ,
                "poissonRatio" : .5,
                "density"      : 7850}
myAnalysis.input.Material = {"Unobtainium": unobtainium,
                       "Madeupium": madeupium}

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

myAnalysis.input.Property = {"plate": shell}

# Set constraints
constraint = {"groupName" : ["plateEdge"],
              "dofConstraint" : 123456}

myAnalysis.input.Constraint = {"cantilever": constraint}


# Run AIM pre-analysis
myAnalysis.preAnalysis()

####### Run Nastran####################
print ("\n\nRunning Nastran......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myAnalysis.analysisDir) # Move into test directory

if (args.noAnalysis == False):
    os.system("nastran old=no notify=no batch=no scr=yes sdirectory=./ " + projectName +  ".dat"); # Run Nastran via system call

os.chdir(currentDirectory) # Move back to working directory
print ("Done running Nastran!")

# Run AIM post-analysis
myAnalysis.postAnalysis()

