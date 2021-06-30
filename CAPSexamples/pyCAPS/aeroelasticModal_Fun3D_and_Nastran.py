# Import pyCAPS module
import pyCAPS

# Import os module   
import os

# Import shutil module
import shutil

# Import argparse
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Aeroelastic Modal Fun3D and Mystran Example',
                                 prog = 'aeroelasticModal_Fun3D_and_Mystran',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable 
workDir = os.path.join(str(args.workDir[0]), "AeroelasticModal_FN")

# Create projectName vairbale
projectName = "aeroelasticModal"

# Load CSM file 
geometryScript = os.path.join("..","csmData","aeroelasticDataTransferSimple.csm")
myProblem = pyCAPS.Problem(problemName=workDir,
                           capsFile=geometryScript,
                           outLevel=args.verbosity)

# Load AIMs 
surfMesh = myProblem.analysis.create(aim = "egadsTessAIM", 
                                     name= "egads",
                                     capsIntent = "CFD")

mesh = myProblem.analysis.create(aim = "tetgenAIM", 
                                 name= "tetgen",
                                 capsIntent = "CFD")

mesh.input["Surface_Mesh"].link(surfMesh.output["Surface_Mesh"])

fluid = myProblem.analysis.create(aim = "fun3dAIM", 
                                  name = "fun3d", 
                                  capsIntent = "CFD")

fluid.input["Mesh"].link(mesh.output["Volume_Mesh"])

structure = myProblem.analysis.create(aim = "mystranAIM", 
                                      name = "mystran", 
                                      capsIntent = "STRUCTURE")

# Create an array of EigenVector names 
numEigenVector = 4
eigenVectors = []
for i in range(numEigenVector):
    eigenVectors.append("EigenVector_" + str(i+1))
    
boundNames = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for boundName in boundNames:
    # Create the bound
    bound = myProblem.bound.create(boundName)
    
    # Create the vertex sets on the bound for fun3d and astros analysis
    fluidVset     = bound.vertexSet.create(fluid)
    structureVset = bound.vertexSet.create(structure)
    
    # Create eigenVector data sets
    for eigenVector in eigenVectors:
        fluid_eigenVector     = fluidVset.dataSet.create(eigenVector, pyCAPS.fType.FieldIn)
        structure_eigenVector = structureVset.dataSet.create(eigenVector, pyCAPS.fType.FieldOut)

        # Link the data sets
        fluid_eigenVector.link(structure_eigenVector, "Conserve")
    
    # Close the bound as complete (cannot create more vertex or data sets)
    bound.close()

# Set inputs for egads 
surfMesh.input.Tess_Params = [.05, 0.01, 20.0]

# Set inputs for tetgen 
mesh.input.Preserve_Surf_Mesh = True
mesh.input.Mesh_Quiet_Flag = True if args.verbosity == 0 else False

# Set inputs for fun3d
speedofSound = 340.0 # m/s
refVelocity = 100.0 # m/s
refDensity = 1.2 # kg/m^3

fluid.input.Proj_Name = projectName
fluid.input.Mesh_ASCII_Flag = False
fluid.input.Mach = refVelocity/speedofSound
fluid.input.Equation_Type = "compressible"
fluid.input.Viscous = "inviscid"
fluid.input.Num_Iter = 10
fluid.input.Time_Accuracy = "2ndorderOPT"
fluid.input.Time_Step = 0.001*speedofSound
fluid.input.Num_Subiter = 30
fluid.input.Temporal_Error = 0.01

fluid.input.CFL_Schedule = [1, 5.0]
fluid.input.CFL_Schedule_Iter = [1, 40]
fluid.input.Overwrite_NML = True
fluid.input.Restart_Read = "off"

inviscid = {"bcType" : "Inviscid", "wallTemperature" : 1.1}
fluid.input.Boundary_Condition = {"Skin"     : inviscid,
                                  "SymmPlane": "SymmetryY",
                                  "Farfield" : "farfield"}

# Set inputs for nastran
structure.input.Proj_Name = projectName
structure.input.Edge_Point_Max = 10

structure.input.Quad_Mesh = True
structure.input.Tess_Params = [.5, .1, 15]

structure.input.Analysis_Type = "Modal"

eigen = { "extractionMethod"     : "MGIV", # "Lanczos",   
          "frequencyRange"       : [0.1, 200], 
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 5, 
          "eigenNormaliztion"    : "MASS", 
          "lanczosMode"          : 2,  # Default - not necesssary
          "lanczosType"          : "DPB"} # Default - not necesssary

structure.input.Analysis = {"EigenAnalysis": eigen}


madeupium    = {"materialType" : "isotropic", 
                "youngModulus" : 72.0E9 , 
                "poissonRatio": 0.33, 
                "density" : 2.8E3}
structure.input.Material = {"Madeupium": madeupium}

skin  = {"propertyType" : "Shell", 
         "membraneThickness" : 0.0015, 
         "material"        : "madeupium", 
         "bendingInertiaRatio" : 1.0, # Default           
         "shearMembraneRatio"  : 5.0/6.0} # Default 

ribSpar  = {"propertyType" : "Shell", 
            "membraneThickness" : 0.05, 
            "material"        : "madeupium", 
            "bendingInertiaRatio" : 1.0, # Default           
            "shearMembraneRatio"  : 5.0/6.0} # Default 

structure.input.Property = {"Skin"    : skin,
                            "Rib_Root": ribSpar}

constraint = {"groupName" : "Rib_Root", 
              "dofConstraint" : 123456}
structure.input.Constraint = {"edgeConstraint": constraint}

####### EGADS ########################
# Run pre/post-analysis for egads
print ("\nRunning PreAnalysis ......", "egads")
surfMesh.preAnalysis()

print ("\nRunning PostAnalysis ......", "egads")
surfMesh.postAnalysis()
#######################################

####### Tetgen ########################
# Run pre/post-analysis for tetgen
print ("\nRunning PreAnalysis ......", "tetgen")
mesh.preAnalysis()

print ("\nRunning PostAnalysis ......", "tetgen")
mesh.postAnalysis()
#######################################


####### Nastran #######################
# Run pre/post-analysis for nastran and execute
print ("\nRunning PreAnalysis ......", "nastran")
structure.preAnalysis()

# Run nastran
print ("\n\nRunning nastran......")  
currentDirectory = os.getcwd() # Get our current working directory 

os.chdir(structure.analysisDir) # Move into test directory

os.system("nastran old=no notify=no batch=no scr=yes sdirectory=./ " + projectName +  ".dat"); # Run Nastran via system call

os.chdir(currentDirectory) # Move back to top directory 

print ("\nRunning PostAnalysis ......", "nastran")
# Run AIM post-analysis
structure.postAnalysis()
#######################################

# Retrive natural frequencies from the structural analysis
naturalFreq = structure.output.EigenRadian # rads/s
mass = structure.output.EigenGeneralMass

####### FUN3D ###########################
modalTuple = {} # Build up Modal Aeroelastic dict  
for j in eigenVectors:
    modeNum = int(j.strip("EigenVector_"))
    #print "ModeNumber = ", modeNum 
    
    value = {"frequency" : naturalFreq[modeNum-1], 
             "damping" : 0.99, 
             "generalMass" : mass[modeNum-1], 
             "generalVelocity" : 0.1}
    #print value
    modalTuple[j] = value
    
# Add Eigen information in fun3d     
fluid.input.Modal_Aeroelastic = modalTuple
fluid.input.Modal_Ref_Velocity = refVelocity
fluid.input.Modal_Ref_Dynamic_Pressure = 0.5*refDensity*refVelocity*refVelocity
fluid.input.Modal_Ref_Length = 1.0
        
# Run the preAnalysis 
print ("\nRunning PreAnalysis ......", "fun3d")
fluid.preAnalysis()

####### Run fun3d ####################
print ("\n\nRunning FUN3D......")  
currentDirectory = os.getcwd() # Get our current working directory 

os.chdir(fluid.analysisDir) # Move into test directory

cmdLineOpt = "--moving_grid --aeroelastic_internal --animation_freq -1"

os.system("mpirun -np 5 nodet_mpi " + cmdLineOpt + " > Info.out"); # Run fun3d via system call
    
if os.path.getsize("Info.out") == 0: # 
    print ("FUN3D excution failed\n")
    myProblem.closeCAPS()
    raise SystemError

os.chdir(currentDirectory) # Move back to top directory

print ("\nRunning PostAnalysis ......", "fun3d")
# Run AIM post-analysis
fluid.postAnalysis()
