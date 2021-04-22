# Import other need moduls
## [importModules]
from __future__ import print_function

import os
import argparse
## [importModules]


# Import pyCAPS class file
## [importpyCAPS]
from pyCAPS import capsProblem
## [importpyCAPS]

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Mystran Pytest Example',
                                 prog = 'mystran_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
## [localVariable]
projectName = "MystranModalWingBEM"
workDir = os.path.join(str(args.workDir[0]), projectName)
## [localVariable]

# Initialize capsProblem object
## [initateProblem]
myProblem = capsProblem()
## [initateProblem]

# Load CSM file
## [loadGeom]
geometryScript = os.path.join("..","csmData","feaWingBEM.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
## [loadGeom]


# Load egadsTess aim
myProblem.loadAIM( aim = "egadsTessAIM",
                   altName = "tess" )

# No Tess vertexes on edges (minimial mesh)
myProblem.analysis["tess"].setAnalysisVal("Edge_Point_Min", 2)
myProblem.analysis["tess"].setAnalysisVal("Edge_Point_Max", 2)

# All quad grid
myProblem.analysis["tess"].setAnalysisVal("Mesh_Elements", "Quad")

# Set global tessellation parameters
myProblem.analysis["tess"].setAnalysisVal("Tess_Params", [1.,1.,30])

# Generate the surface mesh
myProblem.analysis["tess"].preAnalysis()
myProblem.analysis["tess"].postAnalysis()

# View the tessellation
#myProblem.analysis["tess"].viewGeometry()

# Load mystran aim
## [loadMYSTRAN]
mystranAIM = myProblem.loadAIM(aim = "mystranAIM",
                               altName = "mystran",
                               analysisDir= workDir,
                               parents = ["tess"] )
## [loadMYSTRAN]

##[setInputs]
# Set project name so a mesh file is generated
mystranAIM.setAnalysisVal("Proj_Name", projectName)
##[setInputs]

##[setTuple]
# Set analysis
eigen = { "extractionMethod"     : "Lanczos",
          "frequencyRange"       : [0, 50],
          "numEstEigenvalue"     : 1,
          "eigenNormaliztion"    : "MASS"}
mystranAIM.setAnalysisVal("Analysis", ("EigenAnalysis", eigen))

# Set materials
unobtainium  = {"youngModulus" : 2.2E11 ,
                "poissonRatio" : .33,
                "density"      : 7850}

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E9 ,
                "poissonRatio" : .5,
                "density"      : 7850}
mystranAIM.setAnalysisVal("Material", [("Unobtainium", unobtainium),
                                       ("Madeupium", madeupium)])

# Set property
shell  = {"propertyType"      : "Shell",
          "membraneThickness" : 0.2,
          "bendingInertiaRatio" : 1.0, # Default
          "shearMembraneRatio"  : 5.0/6.0} # Default }

mystranAIM.setAnalysisVal("Property", ("Ribs_and_Spars", shell))

# Set constraints
constraint = {"groupName" : ["Rib_Constraint"],
              "dofConstraint" : 123456}

mystranAIM.setAnalysisVal("Constraint", ("ribConstraint", constraint))
##[setTuple]

# Run AIM pre-analysis
## [preAnalysis]
mystranAIM.preAnalysis()
## [preAnalysis]

####### Run Mystran####################
print ("\n\nRunning MYSTRAN......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(mystranAIM.analysisDir) # Move into test directory

if (args.noAnalysis == False):
    os.system("mystran.exe " + projectName +  ".dat"); # Run mystran via system call

os.chdir(currentDirectory) # Move back to working directory
print ("Done running MYSTRAN!")

# Run AIM post-analysis
## [postAnalysis]
mystranAIM.postAnalysis()
## [postAnalysis]

# Get Eigen-frequencies
## [results]
print ("\nGetting results for natural frequencies.....")
natrualFreq = mystranAIM.getAnalysisOutVal("EigenFrequency")

mode = 1
for i in natrualFreq:
    print ("Natural freq ( Mode", mode, ") = ", i, "(Hz)")
    mode += 1
## [results]

# Close CAPS
## [closeCAPS]
myProblem.closeCAPS()
## [closeCAPS]
