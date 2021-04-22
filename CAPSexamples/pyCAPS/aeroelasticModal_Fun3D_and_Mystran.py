from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

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

# Initialize capsProblem object
myProblem = capsProblem()

# Create working directory variable 
workDir = os.path.join(str(args.workDir[0]), "AeroelasticModal_FM")

# Create projectName vairbale
projectName = "aeroelasticModal"

# Load CSM file 
geometryScript = os.path.join("..","csmData","aeroelasticDataTransferSimple.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AIMs 
mesh = myProblem.loadAIM(aim = "tetgenAIM", 
                         altName= "tetgen",
                         analysisDir = workDir + "_FUN3D",
                         capsIntent = "CFD")

  
fluid = myProblem.loadAIM(aim = "fun3dAIM", 
                          altName = "fun3d", 
                          analysisDir = workDir + "_FUN3D", 
                          parents = [mesh.aimName],
                          capsIntent = "CFD")

structure = myProblem.loadAIM(aim = "mystranAIM", 
                              altName = "mystran", 
                              analysisDir = workDir + "_MYSTRAN",
                              capsIntent = "STRUCTURE")

# Create an array of EigenVector names 
numEigenVector = 4
eigenVector = []
for i in range(numEigenVector):
    eigenVector.append("EigenVector_" + str(i+1))
    
transfers = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for i in transfers:
    myProblem.createDataTransfer(variableName = eigenVector,
                                 aimSrc = ["mystran"]*len(eigenVector),
                                 aimDest =["fun3d"]*len(eigenVector), 
                                 #transferMethod = , #["Conserve"] # Auto-assign Interpolate
                                 capsBound = i)

# Set inputs for tetgen 
mesh.setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])
mesh.setAnalysisVal("Preserve_Surf_Mesh", True)

# Set inputs for fun3d
speedofSound = 340.0 # m/s
refVelocity = 100.0 # m/s
refDensity = 1.2 # kg/m^3

fluid.setAnalysisVal("Proj_Name", projectName)
fluid.setAnalysisVal("Mesh_ASCII_Flag", False)
fluid.setAnalysisVal("Mach", refVelocity/speedofSound)
fluid.setAnalysisVal("Equation_Type","compressible")
fluid.setAnalysisVal("Viscous", "inviscid")
fluid.setAnalysisVal("Num_Iter",10)
fluid.setAnalysisVal("Time_Accuracy","2ndorderOPT")
fluid.setAnalysisVal("Time_Step",0.001*speedofSound)
fluid.setAnalysisVal("Num_Subiter", 30)
fluid.setAnalysisVal("Temporal_Error",0.01)

fluid.setAnalysisVal("CFL_Schedule",[1, 5.0])
fluid.setAnalysisVal("CFL_Schedule_Iter", [1, 40])
fluid.setAnalysisVal("Overwrite_NML", True)
fluid.setAnalysisVal("Restart_Read","off")

inviscid = {"bcType" : "Inviscid", "wallTemperature" : 1.1}
fluid.setAnalysisVal("Boundary_Condition", [("Skin", inviscid),
                                            ("SymmPlane", "SymmetryY"),
                                            ("Farfield","farfield")])

# Set inputs for mystran
structure.setAnalysisVal("Proj_Name", projectName)
structure.setAnalysisVal("Edge_Point_Max", 10)

structure.setAnalysisVal("Quad_Mesh", True)
structure.setAnalysisVal("Tess_Params", [.5, .1, 15])

structure.setAnalysisVal("Analysis_Type", "Modal");

eigen = { "extractionMethod"     : "MGIV", # "Lanczos",   
          "frequencyRange"       : [0.1, 200], 
          "numEstEigenvalue"     : 1,
          "numDesiredEigenvalue" : 5, 
          "eigenNormaliztion"    : "MASS", 
          "lanczosMode"          : 2,  # Default - not necesssary
          "lanczosType"          : "DPB"} # Default - not necesssary

structure.setAnalysisVal("Analysis", ("EigenAnalysis", eigen))


madeupium    = {"materialType" : "isotropic", 
                "youngModulus" : 72.0E9 , 
                "poissonRatio": 0.33, 
                "density" : 2.8E3}
structure.setAnalysisVal("Material", ("Madeupium", madeupium))

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

structure.setAnalysisVal("Property", [("Skin", skin),
                                      ("Rib_Root", ribSpar)])

constraint = {"groupName" : "Rib_Root", 
              "dofConstraint" : 123456}
structure.setAnalysisVal("Constraint", ("edgeConstraint", constraint))

####### Tetgen ########################
# Run pre/post-analysis for tetgen
print ("\nRunning PreAnalysis ......", "tetgen")
mesh.preAnalysis()

print ("\nRunning PostAnalysis ......", "tetgen")
mesh.postAnalysis()
#######################################

# Populate vertex sets in the bounds after the mesh generation is copleted
for j in transfers:
    myProblem.dataBound[j].fillVertexSets()

####### Mystrean #######################
# Run pre/post-analysis for mystran and execute
print ("\nRunning PreAnalysis ......", "mystran")
structure.preAnalysis()

# Run mystran
print ("\n\nRunning Mystran......")  
currentDirectory = os.getcwd() # Get our current working directory 

os.chdir(structure.analysisDir) # Move into test directory

os.system("mystran.exe " + projectName +  ".dat > Info.out") # Run fun3d via system call

if os.path.getsize("Info.out") == 0: # 
    print ("Mystran excution possibly failed\n")
    #myProblem.closeCAPS()

os.chdir(currentDirectory) # Move back to top directory 

print ("\nRunning PostAnalysis ......", "mystran")

# Run AIM post-analysis
structure.postAnalysis()
#######################################

#Execute the dataTransfer
print ("\nExecuting dataTransfer ......")
for j in transfers:
    for eigenName in eigenVector:
        myProblem.dataBound[j].executeTransfer(eigenName)
        #myProblem.dataBound[j].dataSetSrc[eigenName].viewData()
        #myProblem.dataBound[j].viewData()
    #myProblem.dataBound[j].writeTecplot(myProblem.analysis[i].analysisDir + "/Data")

# Retrive natural frequencies from the structural analysis
naturalFreq = structure.getAnalysisOutVal("EigenRadian") # rads/s
mass = structure.getAnalysisOutVal("EigenGeneralMass")

####### FUN3D ###########################
modalTuple = [] # Build up Modal Aeroelastic tuple  
for j in eigenVector:
    modeNum = int(j.strip("EigenVector_"))
    #print "ModeNumber = ", modeNum 
    
    value = {"frequency" : naturalFreq[modeNum-1], 
             "damping" : 0.99, 
             "generalMass" : mass[modeNum-1], 
             "generalVelocity" : 0.1}
    #print value
    modalTuple.append((j,value))
    
# Add Eigen information in fun3d     
fluid.setAnalysisVal("Modal_Aeroelastic", modalTuple)
fluid.setAnalysisVal("Modal_Ref_Velocity", refVelocity)
fluid.setAnalysisVal("Modal_Ref_Dynamic_Pressure", 0.5*refDensity*refVelocity*refVelocity)
fluid.setAnalysisVal("Modal_Ref_Length", 1.0)
        
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

# Close CAPS 
myProblem.closeCAPS()
