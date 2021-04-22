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

# Load CSM file
wing = myProblem.loadCAPS(os.path.join("..","ESP","wing3.csm"))

# Enable structures and CFD views
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:CFDInviscid"     , 1)

#------------------------------------------------------------------------------#

# Create projectName vairbale
projectName = "aeroelasticModal"

# Create working directory variable
workDir = "workDir_AeroelasticModal"

#------------------------------------------------------------------------------#

# Load aflr4 AIM
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                          altName = "aflr4",
                          analysisDir = workDir + "_AFLR4")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Scaling factor to compute AFLR4 'ref_len' parameter
aflr4.setAnalysisVal("Mesh_Length_Factor", 5)

# Edge mesh spacing discontinuity scaled interpolant and farfield BC
aflr4.setAnalysisVal("Mesh_Sizing", [("Wing"    , {"edgeWeight":1.0}),
                                     ("Farfield", {"bcType":"Farfield"})])

#------------------------------------------------------------------------------#

# Load AFLR3 AIM - child of AFLR4 AIM
aflr3 = myProblem.loadAIM(aim         = "aflr3AIM",
                          analysisDir = workDir + "_AFLR3",
                          parents     = aflr4.aimName)

#------------------------------------------------------------------------------#

fun3d = myProblem.loadAIM(aim         = "fun3dAIM",
                          altName     = "fun3d",
                          analysisDir = workDir + "_FUN3D",
                          parents     = aflr3.aimName)

# Set inputs for fun3d
speedofSound = 340.0 # m/s
refVelocity = 100.0 # m/s
refDensity = 1.2 # kg/m^3

fun3d.setAnalysisVal("Proj_Name", projectName)
fun3d.setAnalysisVal("Mesh_ASCII_Flag", False)
fun3d.setAnalysisVal("Mach", refVelocity/speedofSound)
fun3d.setAnalysisVal("Equation_Type","compressible")
fun3d.setAnalysisVal("Viscous", "inviscid")
fun3d.setAnalysisVal("Num_Iter",10)
fun3d.setAnalysisVal("Time_Accuracy","2ndorderOPT")
fun3d.setAnalysisVal("Time_Step",0.001*speedofSound)
fun3d.setAnalysisVal("Num_Subiter", 30)
fun3d.setAnalysisVal("Temporal_Error",0.01)

fun3d.setAnalysisVal("CFL_Schedule",[1, 5.0])
fun3d.setAnalysisVal("CFL_Schedule_Iter", [1, 40])
fun3d.setAnalysisVal("Overwrite_NML", True)
fun3d.setAnalysisVal("Restart_Read","off")

inviscidBC = {"bcType" : "Inviscid"}
fun3d.setAnalysisVal("Boundary_Condition", [("Wing", inviscidBC),
                                            ("Farfield","farfield")])
#------------------------------------------------------------------------------#

# Load EGADS tess aim
tess = myProblem.loadAIM(aim         = "egadsTessAIM",
                         analysisDir = workDir + "_Tess")

# No Tess vertexes on edges (minimial mesh)
tess.setAnalysisVal("Edge_Point_Max", 2)

# Use regularized quads
tess.setAnalysisVal("Mesh_Elements", "Quad")

#------------------------------------------------------------------------------#

# Load ASTROS AIMs
astros = myProblem.loadAIM(aim         = "astrosAIM",
                           altName     = "astros",
                           analysisDir = workDir + "_ASTROS",
                           parents     = tess.aimName)

# Set inputs for astros
astros.setAnalysisVal("Proj_Name", projectName)

astros.setAnalysisVal("Analysis_Type", "Modal");

# Set analysis type
eigen = { "analysisType"         : "Modal",
          "extractionMethod"     : "SINV",
          "frequencyRange"       : [0, 10],
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 10,
          "eigenNormaliztion"    : "MASS"}
astros.setAnalysisVal("Analysis", ("EigenAnalysis", eigen))

# Set constraints
constraint = {"groupName"     : ["rootConstraint"],
              "dofConstraint" : 123456}
astros.setAnalysisVal("Constraint", ("rootConstraint", constraint))

# Set materials
unobtainium  = {"youngModulus" : 2.2E6 ,
                "poissonRatio" : .5,
                "density"      : 7850}
madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.2E5 ,
                "poissonRatio" : .5,
                "density"      : 7850}
astros.setAnalysisVal("Material", [("Unobtainium", unobtainium),
                                   ("Madeupium", madeupium)])

# Set properties
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
astros.setAnalysisVal("Property", [("leftWingSkin", skinShell),
                                   ("riteWingSkin", skinShell),
                                   ("wingRib"     ,  ribShell),
                                   ("wingSpar1"   , sparShell),
                                   ("wingSpar2"   , sparShell)])

#------------------------------------------------------------------------------#

# Create an array of EigenVector names
numEigenVector = 3
eigenVector = []
for i in range(numEigenVector):
    eigenVector.append("EigenVector_" + str(i+1))

# Create the capsBounds for data transfer
transfers = ["upperWing", "lowerWing", "leftTip", "riteTip"]
for bound in transfers:
    myProblem.createDataTransfer(variableName   = eigenVector,
                                 aimSrc         = [astros.aimName]*numEigenVector,
                                 aimDest        = [fun3d.aimName]*numEigenVector,
                                 transferMethod = ["Conserve"]*numEigenVector,
                                 capsBound      = bound)

#------------------------------------------------------------------------------#

# Run AIM pre/post-analysis to generate the meshes
for aim in [aflr4.aimName, aflr3.aimName, tess.aimName]:
    myProblem.analysis[aim].preAnalysis()
    myProblem.analysis[aim].postAnalysis()

#------------------------------------------------------------------------------#

# Populate vertex sets in the bounds after the mesh generation is copleted
for bound in transfers:
    myProblem.dataBound[bound].fillVertexSets()

#------------------------------------------------------------------------------#

# Run AIM pre-analysis
print ("\n==> Running ASTROS pre-analysis...")
astros.preAnalysis()

####### Run ASTROS ####################
print ("\n==> Running Astros...")

# Get our current working directory (so that we can return here after running ASTROS)
currentPath = os.getcwd()

# Move into temp directory
os.chdir(astros.analysisDir)

# Declare ASTROS install directory
astrosInstallDir = os.environ['ESP_ROOT'] + os.sep + "bin" + os.sep

# Copy files needed to run astros
files = ["ASTRO.D01", "ASTRO.IDX"]
for file in files:
    try:
        shutil.copy2(astrosInstallDir + file, file)
    except:
        print ("Unable to copy \"", file, "\"")
        raise SystemError

# Run micro-ASTROS via system call
os.system("mastros.exe < " + projectName +  ".dat > " + projectName + ".out")

# Remove temporary files
for file in files:
    if os.path.isfile(file):
        os.remove(file)

# Move back to original directory
os.chdir(currentPath)
#######################################

# Run AIM post-analysis
astros.postAnalysis()

#------------------------------------------------------------------------------#

#Execute the dataTransfer
print ("\nExecuting dataTransfer ......")
for bound in transfers:
    for eigenName in eigenVector:
        myProblem.dataBound[bound].executeTransfer(eigenName)

#------------------------------------------------------------------------------#

# Retrive natural frequencies from the structural analysis
naturalFreq = astros.getAnalysisOutVal("EigenRadian") # rads/s
mass        = astros.getAnalysisOutVal("EigenGeneralMass")

modalTuple = [] # Build up Modal Aeroelastic tuple
for j in eigenVector:
    modeNum = int(j.strip("EigenVector_"))

    value = {"frequency" : naturalFreq[modeNum-1],
             "damping" : 0.99,
             "generalMass" : mass[modeNum-1],
             "generalVelocity" : 0.1}
    #print value
    modalTuple.append((j,value))

# Add Eigen information in fun3d
fun3d.setAnalysisVal("Modal_Aeroelastic", modalTuple)
fun3d.setAnalysisVal("Modal_Ref_Velocity", refVelocity)
fun3d.setAnalysisVal("Modal_Ref_Dynamic_Pressure", 0.5*refDensity*refVelocity*refVelocity)
fun3d.setAnalysisVal("Modal_Ref_Length", 1.0)

# Run the preAnalysis
print ("\n==> Running FUN3D pre-analysis...")
fun3d.preAnalysis()

####### Run fun3d ####################
print ("\n\nRunning FUN3D......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(fun3d.analysisDir) # Move into test directory

cmdLineOpt = "--moving_grid --aeroelastic_internal --animation_freq -1"

# Run fun3d via system call
os.system("mpirun -np 5 nodet_mpi " + cmdLineOpt + " > Info.out");

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
fun3d.postAnalysis()
