#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem, capsConvert

# Import os module
import os

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
wing = myProblem.loadCAPS(os.path.join("..","ESP","wing3.csm"))

# Change to Structures and VLM
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:VLM"             , 1)
wing.setGeometryVal("VIEW:ClampedStructure", 1)

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

# Load nastran aim
nastran = myProblem.loadAIM(aim         = "nastranAIM",
                           analysisDir = "workDir_NASTRAN_6_Flutter",
                           parents     = tess.aimName)

# Set analysis specific variables
Proj_Name = "nastran_modal"
nastran.setAnalysisVal("Proj_Name"       , Proj_Name)
nastran.setAnalysisVal("Mesh_File_Format", "Large")

# Set constraints
constraint = {"groupName"     : ["rootConstraint"],
              "dofConstraint" : 123456}
nastran.setAnalysisVal("Constraint", ("rootConstraint", constraint))

# Set materials
unobtainium  = {"youngModulus" : 2.2E6 ,
                "poissonRatio" : .5,
                "density"      : 7850}
madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E5 ,
                "poissonRatio" : .5,
                "density"      : 7850}
nastran.setAnalysisVal("Material", [("Unobtainium", unobtainium),
                                   ("Madeupium", madeupium)])

# Set properties for various Faces
skinShell  = {"propertyType"        : "Shell",
              "material"            : "unobtainium",
              "bendingInertiaRatio" : 1.0,
              "shearMembraneRatio"  : 0, # Turn of shear - no materialShear
              "membraneThickness"   : 0.05}
ribShell  = {"propertyType"        : "Shell",
             "material"            : "unobtainium",
             "bendingInertiaRatio" : 1.0,
             "shearMembraneRatio"  : 0, # Turn of shear - no materialShear
             "membraneThickness"   : 0.1}
sparShell  = {"propertyType"        : "Shell",
              "material"            : "madeupium",
              "bendingInertiaRatio" : 1.0,
              "shearMembraneRatio"  : 0, # Turn of shear - no materialShear
              "membraneThickness"   : 0.2}
nastran.setAnalysisVal("Property", [("leftWingSkin", skinShell),
                                    ("riteWingSkin", skinShell),
                                    ("wingRib"     ,  ribShell),
                                    ("wingSpar1"   , sparShell),
                                    ("wingSpar2"   , sparShell)])

# Set analysis type
nastran.setAnalysisVal("Analysis_Type", "AeroelasticFlutter")

# Aero with capsGroup for airfoil sections
wingVLM = {"groupName"         : "Wing",
           "numChord"          : 4,
           "numSpanPerSection" : 6}

# Note the surface name corresponds to the capsBound found in the *.csm file. This links
# the spline for the aerodynamic surface to the structural model
nastran.setAnalysisVal("VLM_Surface", ("upperWing", wingVLM))
    
# Set analysis 
flutter = { "analysisType"         : "AeroelasticFlutter",
            "extractionMethod"     : "Lanczos",
            "frequencyRange"       : [0.1, 300],
            "numEstEigenvalue"     : 4,
            "numDesiredEigenvalue" : 4, 
            "eigenNormaliztion"    : "MASS", 
            "aeroSymmetryXY"       : "ASYM",
            "aeroSymmetryXZ"       : "SYM",
            "analysisConstraint"   : ["rootConstraint"],
            "machNumber"           : 0.1029,
            "dynamicPressure"      : 0.5*capsConvert(1.2, "kg/m3", "kg/cm3")*(capsConvert(35, "m/s", "cm/s"))**2,
            "density"              : capsConvert(1.2, "kg/m3", "kg/cm3"), 
            "reducedFreq"          : [0.001, 0.01, 0.1, 0.12, 0.14, 0.16, 0.18, 0.25],
            }
nastran.setAnalysisVal("Analysis", [("Flutter", flutter)])

# Run AIM pre-analysis
print ("\n==> Running Nastran pre-analysis...")
nastran.preAnalysis()

print ("\n==> Running Nastran...")
####### Run NASTRAN ###################
currentDirectory = os.getcwd()   # Get current working directory
os.chdir(nastran.analysisDir)    # Move into test directory

# Run nastran via system call
os.system("nastran old=no notify=no batch=no scr=yes sdirectory=./ " + Proj_Name +  ".dat"); # Run Nastran via system call

os.chdir(currentDirectory)       # Move back to original directory
#######################################

# Run AIM post-analysis
nastran.postAnalysis()
