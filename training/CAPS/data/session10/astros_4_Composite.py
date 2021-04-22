#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os module
import os
import shutil

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = os.path.join("..","ESP","wing3.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
wing = myProblem.loadCAPS(filename)

# Set geometry variables to enable ASTROS mode
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:BoxStructure"    , 1)

#------------------------------------------------------------------------------#

# Load EGADS tess aim
tess = myProblem.loadAIM(aim = "egadsTessAIM",
                         analysisDir = "workDir_Tess")

# No Tess vertexes on edges (minimial mesh)
tess.setAnalysisVal("Edge_Point_Max", 2)

# Use regularized quads
tess.setAnalysisVal("Mesh_Elements", "Quad")

# Run AIM pre/post-analysis to generate the surface mesh
print ("\n==> Generating Mesh...")
tess.preAnalysis()
tess.postAnalysis()

#------------------------------------------------------------------------------#

# Load astros aim
astros = myProblem.loadAIM(aim         = "astrosAIM",
                           analysisDir = "workDir_ASTROS_4_Composite",
                           parents     = tess.aimName)

# Set analysis specific variables
Proj_Name = "astros_modal"
astros.setAnalysisVal("Proj_Name"       , Proj_Name)
astros.setAnalysisVal("Mesh_File_Format", "Large")

# Define loads
leftLoad = {"loadType"         : "GridForce",
            "forceScaleFactor" : 1.e6,
            "directionVector"  : [0.0, 0.0, 1.0]}
riteLoad = {"loadType"         : "GridForce",
            "forceScaleFactor" : 2.e6,
            "directionVector"  : [0.0, 0.0, 1.0]}
astros.setAnalysisVal("Load", [("leftPointLoad", leftLoad ),
                               ("ritePointLoad", riteLoad )])

# Set analysis type
astros.setAnalysisVal("Analysis_Type", "Static")

# Set constraints
constraint = {"groupName"     : ["rootConstraint"],
              "dofConstraint" : 123456}
astros.setAnalysisVal("Constraint", ("rootConstraint", constraint))

# Set materials
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
astros.setAnalysisVal("Material", [("Aluminum", Aluminum),
                                   ("Graphite_epoxy", Graphite_epoxy)])

# Set properties
skinShell  = {"propertyType"           : "Composite",
              "shearBondAllowable"     : 1.0e6,
              "bendingInertiaRatio"    : 1.0,
              "shearMembraneRatio"     : 0, # Turn off shear - no materialShear
              "compositeMaterial"      : ["Graphite_epoxy"]*8,
              "compositeThickness"     : [0.00525]*8,
              "compositeOrientation"   : [0, 0, 0, 0, -45, 45, -45, 45],
              "symmetricLaminate"      : True,
              "compositeFailureTheory" : "STRAIN" }
ribShell  = {"propertyType"         : "Shell",
             "material"             : "Aluminum",
             "bendingInertiaRatio"  : 1.0,
             "shearMembraneRatio"   : 0, # Turn of shear - no materialShear
             "membraneThickness"    : 0.125 }
sparShell  = {"propertyType"         : "Shell",
             "material"             : "Aluminum",
             "bendingInertiaRatio"  : 1.0,
             "shearMembraneRatio"   : 0, # Turn of shear - no materialShear
             "membraneThickness"    : 0.125 }
astros.setAnalysisVal("Property", [("leftWingSkin", skinShell),
                                   ("riteWingSkin", skinShell),
                                   ("wingRib"     ,  ribShell),
                                   ("wingSpar1"   , sparShell),
                                   ("wingSpar2"   , sparShell)])

# Run AIM pre-analysis
print ("\n==> Running ASTROS pre-analysis...")
astros.preAnalysis()

print ("\n==> Running Astros...")
####### Run ASTROS ####################
currentDirectory = os.getcwd()   # Get current working directory
os.chdir(astros.analysisDir)     # Move into test directory

# Declare ASTROS install directory
astrosInstallDir = os.environ['ESP_ROOT'] + os.sep + "bin" + os.sep

# Copy files needed to run astros
files = ["ASTRO.D01", "ASTRO.IDX"]
for file in files:
    try:
        shutil.copy2(astrosInstallDir + file, file)
    except:
        print ('Unable to copy "' + file + '"')
        raise

# Run micro-ASTROS via system call
os.system("mastros.exe < " + Proj_Name +  ".dat > " + Proj_Name + ".out")

# Remove temporary files
for file in files:
    if os.path.isfile(file):
        os.remove(file)

os.chdir(currentDirectory)       # Move back to top directory
#######################################

# Run AIM post-analysis
astros.postAnalysis()

print ("\n--> Maximum displacements:")
print ("--> Tmax" , astros.getAnalysisOutVal("Tmax" ))
print ("--> T1max", astros.getAnalysisOutVal("T1max"))
print ("--> T2max", astros.getAnalysisOutVal("T2max"))
print ("--> T3max", astros.getAnalysisOutVal("T3max"))
