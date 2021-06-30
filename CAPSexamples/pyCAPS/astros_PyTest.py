# Import pyCAPS module
import pyCAPS

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
parser = argparse.ArgumentParser(description = 'Astros PyTest Example',
                                 prog = 'astros_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ["." + os.sep], nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

workDir = os.path.join(str(args.workDir[0]), "AstrosModalWingBEM")

# Load CSM file
geometryScript = os.path.join("..","csmData","feaWingBEM.csm")
myProblem = pyCAPS.Problem(problemName=workDir,
                           capsFile=geometryScript, 
                           outLevel=args.verbosity)


# Create egadsTess aim
myProblem.analysis.create( aim = "egadsTessAIM",
                           name = "tess" )

# All triangles in the grid
myProblem.analysis["tess"].input.Mesh_Elements = "Quad"

# Set global tessellation parameters
myProblem.analysis["tess"].input.Tess_Params = [.05,.5,15]

# Generate the surface mesh
myProblem.analysis["tess"].preAnalysis()
myProblem.analysis["tess"].postAnalysis()


# Create astros aim
myProblem.analysis.create( aim = "astrosAIM",
                           name = "astros" )

# Link the Surface_Mesh
myProblem.analysis["astros"].input["Mesh"].link(myProblem.analysis["tess"].output["Surface_Mesh"])

# Set project name so a mesh file is generated
projectName = "pyCAPS_astros_Test"
myProblem.analysis["astros"].input.Proj_Name = projectName

myProblem.analysis["astros"].input.Edge_Point_Max = 6

myProblem.analysis["astros"].input.Quad_Mesh = True

#myProblem.analysis["astros"].input.File_Format = "Free"

myProblem.analysis["astros"].input.Mesh_File_Format = "Large"

# Set analysis
eigen = { "extractionMethod"     : "MGIV",
          "frequencyRange"       : [0, 10000],
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 10,
          "eigenNormaliztion"    : "MASS"}
myProblem.analysis["astros"].input.Analysis = {"EigenAnalysis": eigen}

# Set materials
unobtainium  = {"youngModulus" : 2.2E6 ,
                "poissonRatio" : .5,
                "density"      : 7850}

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E5 ,
                "poissonRatio" : .5,
                "density"      : 7850}
myProblem.analysis["astros"].input.Material = {"Unobtainium": unobtainium,
                                               "Madeupium": madeupium}

# Set property
shell  = {"propertyType"      : "Shell",
          "material"          : "unobtainium",
	      "bendingInertiaRatio" : 1.0, # Default - not necesssary
          "shearMembraneRatio"  : 0, # Turn of shear - no materialShear
          "membraneThickness" : 0.2}

myProblem.analysis["astros"].input.Property = {"Ribs_and_Spars": shell}

# Set constraints
constraint = {"groupName" : ["Rib_Constraint"],
              "dofConstraint" : 123456}

myProblem.analysis["astros"].input.Constraint = {"ribConstraint": constraint}


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

# Get Eigen-frequencies
print ("\nGetting results for natural frequencies.....")
natrualFreq = myProblem.analysis["astros"].output.EigenFrequency

mode = 1
for i in natrualFreq:
    print ("Natural freq (Mode {:d}) = ".format(mode) + '{:.5f} '.format(i) + "(Hz)")
    mode += 1
