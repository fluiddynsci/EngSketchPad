#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os module
import os
import shutil

# Import SU2 python environment
from parallel_computation import parallel_computation as su2Run
from mesh_deformation import mesh_deformation as su2Deform

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
projectName = "aeroelasticIterative"

# Set the number of transfer iterations
numTransferIteration = 2

# Create working directory variable
workDir = "Aeroelastic_Iterative"

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

# Load SU2 AIMs
su2 = myProblem.loadAIM(aim         = "su2AIM",
                        altName     = "su2",
                        analysisDir = workDir + "_SU2",
                        parents     = aflr3.aimName)

# Set inputs for su2
speedofSound = 340.0 # m/s
refVelocity = 100.0 # m/s
refDensity = 1.2 # kg/m^3

su2.setAnalysisVal("Proj_Name", projectName)
su2.setAnalysisVal("SU2_Version", "Falcon")
su2.setAnalysisVal("Mach", refVelocity/speedofSound)
su2.setAnalysisVal("Equation_Type","compressible")
su2.setAnalysisVal("Num_Iter",5) # Way too few to converge the solver, but this is an example
su2.setAnalysisVal("Output_Format", "Tecplot")
su2.setAnalysisVal("Overwrite_CFG", True)
su2.setAnalysisVal("Pressure_Scale_Factor",0.5*refDensity*refVelocity**2)
su2.setAnalysisVal("Surface_Monitor", "Wing")

inviscidBC = {"bcType" : "Inviscid"}
su2.setAnalysisVal("Boundary_Condition", [("Wing", inviscidBC),
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

# Set analysis type
astros.setAnalysisVal("Analysis_Type", "Static")

# External pressure load to astros that we will inherited from su2
load = {"loadType" : "PressureExternal"}
astros.setAnalysisVal("Load", ("pressureAero", load ))

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

# Create the data transfer connections
transfers = ["upperWing", "lowerWing", "leftTip", "riteTip"]
for bound in transfers:
    myProblem.createDataTransfer(variableName   = ["Pressure", "Displacement"],
                                 aimSrc         = [su2.aimName, astros.aimName],
                                 aimDest        = [astros.aimName, su2.aimName],
                                 transferMethod = ["Conserve", "Interpolate"],
                                 initValueDest  = [None, (0,0,0)],
                                 capsBound      = bound )

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

# Declare ASTROS install directory
astrosInstallDir = os.environ['ESP_ROOT'] + os.sep + "bin" + os.sep

# Copy files needed to run astros
files = ["ASTRO.D01", "ASTRO.IDX"]
for file in files:
    try:
        shutil.copy2(astrosInstallDir + file, astros.analysisDir + os.sep + file)
    except:
        print ("Unable to copy \"", file, "\"")
        raise SystemError

# Aeroelastic iteration loop
for iter in range(numTransferIteration):

    #Execute the dataTransfer of displacements to su2
    #initValueDest is used on the first iteration
    print ("\n\nExecuting dataTransfer \"Displacement\"......")
    for bound in transfers:
        myProblem.dataBound[bound].executeTransfer("Displacement")

    ####### SU2 ###########################
    print ("\n==> Running SU2 pre-analysis...")
    su2.preAnalysis()

    #------------------------------
    print ("\n\nRunning SU2......")
    currentDirectory = os.getcwd() # Get our current working directory

    os.chdir(su2.analysisDir) # Move into test directory

    numberProc = 1
    # Run SU2 mesh deformation
    if iter > 0:
        su2Deform(projectName + ".cfg", numberProc)

    # Run SU2 solver
    su2Run(projectName + ".cfg", numberProc)

    shutil.copy("surface_flow_" + projectName + ".dat", "surface_flow_" + projectName + "_Iteration_" + str(iter) + ".dat")
    os.chdir(currentDirectory) # Move back to top directory
    #------------------------------

    print ("\n==> Running SU2 post-analysis...")
    su2.postAnalysis()
    #######################################

    #Execute the dataTransfer of Pressure to astros
    print ("\n\nExecuting dataTransfer \"Pressure\"......")
    for bound in transfers:
        myProblem.dataBound[bound].executeTransfer("Pressure")

    ####### ASTROS ########################
    print ("\n==> Running ASTROS pre-analysis...")
    astros.preAnalysis()

    #------------------------------
    print ("\n\nRunning Astros......")
    currentDirectory = os.getcwd() # Get our current working directory

    os.chdir(astros.analysisDir) # Move into test directory

    # Run mASTROS via system call
    os.system("mastros.exe < " + projectName +  ".dat > " + projectName + ".out");

    os.chdir(currentDirectory) # Move back to top directory
    #------------------------------

    print ("\n==> Running ASTROS post-analysis...")
    astros.postAnalysis()
    #######################################
