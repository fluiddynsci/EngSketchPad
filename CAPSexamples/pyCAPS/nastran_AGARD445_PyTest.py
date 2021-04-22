from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Nastran AGARD445.6 Pytest Example',
                                 prog = 'nastran_AGARD445_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()


# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaAGARD445.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Create project name
projectName = "NastranModalAGARD445"

# Change the sweepAngle and span of the Geometry - Demo purposes
#myProblem.geometry.setGeometryVal("sweepAngle", 5) # From 45 to 5 degrees
#myProblem.geometry.setGeometryVal("semiSpan", 5)   # From 2.5 ft to 5 ft

# Load nastran aim
myAnalysis = myProblem.loadAIM(aim = "nastranAIM",
                               altName = "nastran",
                               analysisDir = os.path.join(str(args.workDir[0]), projectName) )

# Set project name so a mesh file is generated
myAnalysis.setAnalysisVal("Proj_Name", projectName)

# Set meshing parameters
myAnalysis.setAnalysisVal("Edge_Point_Max", 10)
myAnalysis.setAnalysisVal("Edge_Point_Min", 6)

myAnalysis.setAnalysisVal("Quad_Mesh", True)

myAnalysis.setAnalysisVal("Tess_Params", [.25,.01,15])

# Set analysis type
myAnalysis.setAnalysisVal("Analysis_Type", "Modal");

# Set analysis inputs
eigen = { "extractionMethod"     : "MGIV", # "Lanczos",
          "frequencyRange"       : [0.1, 200],
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 2,
          "eigenNormaliztion"    : "MASS",
          "lanczosMode"          : 2,  # Default - not necesssary
          "lanczosType"          : "DPB"} # Default - not necesssary

myAnalysis.setAnalysisVal("Analysis", ("EigenAnalysis", eigen))

# Set materials
mahogany    = {"materialType"        : "orthotropic",
               "youngModulus"        : 0.457E6 ,
               "youngModulusLateral" : 0.0636E6,
               "poissonRatio"        : 0.31,
               "shearModulus"        : 0.0637E6,
               "shearModulusTrans1Z" : 0.00227E6,
               "shearModulusTrans2Z" : 0.00227E6,
               "density"             : 3.5742E-5}

myAnalysis.setAnalysisVal("Material", ("Mahogany", mahogany))

# Set properties
shell  = {"propertyType" : "Shell",
          "membraneThickness" : 0.82,
          "material"        : "mahogany",
          "bendingInertiaRatio" : 1.0, # Default - not necesssary
          "shearMembraneRatio"  : 5.0/6.0} # Default - not necesssary

myAnalysis.setAnalysisVal("Property", ("yatesPlate", shell))

# Set constraints
constraint = {"groupName" : "constEdge",
              "dofConstraint" : 123456}

myAnalysis.setAnalysisVal("Constraint", ("edgeConstraint", constraint))

# Run AIM pre-analysis
myAnalysis.preAnalysis()

####### Run Nastran ####################
print ("\n\nRunning Nastran......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myAnalysis.analysisDir) # Move into test directory

if (args.noAnalysis == False):
    os.system("nastran old=no notify=no batch=no scr=yes sdirectory=./ " + projectName +  ".dat"); # Run Nastran via system call

os.chdir(currentDirectory) # Move back to working directory

print ("Done running Nastran!")
########################################

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Get Eigen-frequencies
print ("\nGetting results natural frequencies.....")
natrualFreq = myAnalysis.getAnalysisOutVal("EigenFrequency")

mode = 1
for i in natrualFreq:
    print ("Natural freq (Mode {:d}) = ".format(mode) + '{:.2f} '.format(i) + "(Hz)")
    mode += 1

# Close CAPS
myProblem.closeCAPS()
