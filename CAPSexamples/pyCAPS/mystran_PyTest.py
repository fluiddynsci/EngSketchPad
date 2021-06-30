## [importModules]
# Import pyCAPS module
import pyCAPS

# Import os module
import os
import argparse
## [importModules]

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Mystran Pytest Example',
                                 prog = 'mystran_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ["."+os.sep], nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
## [localVariable]
projectName = "MystranModalWingBEM"
workDir = os.path.join(str(args.workDir[0]), projectName)
## [localVariable]

# Load CSM file
## [loadGeom]
geometryScript = os.path.join("..","csmData","feaWingBEM.csm")
myProblem = pyCAPS.Problem(problemName=workDir,
                           capsFile=geometryScript,
                           outLevel=args.verbosity)
## [loadGeom]

# Load egadsTess aim
myProblem.analysis.create( aim = "egadsTessAIM",
                           name = "tess" )

# No Tess vertexes on edges (minimial mesh)
myProblem.analysis["tess"].input.Edge_Point_Min = 2
myProblem.analysis["tess"].input.Edge_Point_Max = 2

# All quad grid
myProblem.analysis["tess"].input.Mesh_Elements = "Quad"

# Set global tessellation parameters
myProblem.analysis["tess"].input.Tess_Params = [1.,1.,30]

# Generate the surface mesh
myProblem.analysis["tess"].preAnalysis()
myProblem.analysis["tess"].postAnalysis()

# View the tessellation
#myProblem.analysis["tess"].geometry.view()

# Load mystran aim
## [loadMYSTRAN]
mystranAIM = myProblem.analysis.create(aim = "mystranAIM",
                                       name = "mystran" )
## [loadMYSTRAN]

##[setInputs]
# Link Surface_Mesh
mystranAIM.input["Mesh"].link(myProblem.analysis["tess"].output["Surface_Mesh"])

# Set project name so a mesh file is generated
mystranAIM.input.Proj_Name = projectName
##[setInputs]

##[setTuple]
# Set analysis
eigen = { "extractionMethod"     : "Lanczos",
          "frequencyRange"       : [0, 50],
          "numEstEigenvalue"     : 1,
          "eigenNormaliztion"    : "MASS"}
mystranAIM.input.Analysis = {"EigenAnalysis": eigen}

# Set materials
unobtainium  = {"youngModulus" : 2.2E11 ,
                "poissonRatio" : .33,
                "density"      : 7850}

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E9 ,
                "poissonRatio" : .5,
                "density"      : 7850}
mystranAIM.input.Material = {"Unobtainium": unobtainium,
                             "Madeupium"  : madeupium}

# Set property
shell  = {"propertyType"      : "Shell",
          "membraneThickness" : 0.2,
          "bendingInertiaRatio" : 1.0, # Default
          "shearMembraneRatio"  : 5.0/6.0} # Default }

mystranAIM.input.Property = {"Ribs_and_Spars": shell}
##[setTuple]

# Set constraints
constraint = {"groupName" : ["Rib_Constraint"],
              "dofConstraint" : 123456}

mystranAIM.input.Constraint = {"ribConstraint": constraint}

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
natrualFreq = mystranAIM.output.EigenFrequency

for mode, i in enumerate(natrualFreq):
    print ("Natural freq ( Mode", mode, ") = ", i, "(Hz)")
## [results]
