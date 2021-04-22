from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import SU2 python environment
from parallel_computation import parallel_computation as su2Run

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Aeroelastic Pressure SU2 and Mystran Example',
                                 prog = 'aeroelasticSimple_Pressure_SU2_and_Mystran',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot data")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "AeroelasticSimple_Pressure")

# Create projectName vairbale
projectName = "aeroelasticSimple_Pressure_SM"

# Load CSM file
geometryScript = os.path.join("..","csmData","aeroelasticDataTransferSimple.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AIMs
myMesh = myProblem.loadAIM(aim = "tetgenAIM",
                           altName= "tetgen",
                           analysisDir = workDir + "_SU2",
                           capsIntent = "CFD")


su2 = myProblem.loadAIM(aim = "su2AIM",
                        altName = "su2",
                        analysisDir = workDir + "_SU2",
                        parents = ["tetgen"],
                        capsIntent = "CFD")

mystran = myProblem.loadAIM(aim = "mystranAIM",
                            altName = "mystran",
                            analysisDir = workDir + "_MYSTRAN",
                            capsIntent = "STRUCTURE")

transfers = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for i in transfers:
    myProblem.createDataTransfer(variableName = "Pressure",
                                 aimSrc = "su2",
                                 aimDest ="mystran",
                                 transferMethod = "Interpolate", # "Conserve", #
                                 capsBound = i)

# Set inputs for tetgen
myMesh.setAnalysisVal("Tess_Params", [.3, 0.01, 20.0])
myMesh.setAnalysisVal("Preserve_Surf_Mesh", True)
myMesh.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set inputs for su2
speedofSound = 340.0 # m/s
refVelocity = 100.0 # m/s
refDensity = 1.2 # kg/m^3

su2.setAnalysisVal("Proj_Name", projectName)
su2.setAnalysisVal("Mach", refVelocity/speedofSound)
su2.setAnalysisVal("Equation_Type","compressible")
su2.setAnalysisVal("Num_Iter",5)
su2.setAnalysisVal("Output_Format", "Tecplot")
su2.setAnalysisVal("SU2_Version", "Falcon") # "Falcon", "Raven"
su2.setAnalysisVal("Pressure_Scale_Factor",0.5*refDensity*refVelocity**2)
su2.setAnalysisVal("Surface_Monitor", ["Skin"])

inviscid = {"bcType" : "Inviscid"}
su2.setAnalysisVal("Boundary_Condition", [("Skin", inviscid),
                                          ("SymmPlane", "SymmetryY"),
                                          ("Farfield","farfield")])

# Set inputs for mystran
mystran.setAnalysisVal("Proj_Name", projectName)
mystran.setAnalysisVal("Edge_Point_Max", 10)
mystran.setAnalysisVal("Edge_Point_Min", 10)

mystran.setAnalysisVal("Quad_Mesh", True)
mystran.setAnalysisVal("Tess_Params", [.5, .1, 15])
mystran.setAnalysisVal("Analysis_Type", "Static");

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 72.0E9 ,
                "poissonRatio": 0.33,
                "density" : 2.8E3}
mystran.setAnalysisVal("Material", ("Madeupium", madeupium))

skin  = {"propertyType" : "Shell",
         "membraneThickness" : 0.06,
         "material"        : "madeupium",
         "bendingInertiaRatio" : 1.0, # Default
         "shearMembraneRatio"  : 5.0/6.0} # Default

ribSpar  = {"propertyType" : "Shell",
            "membraneThickness" : 0.6,
            "material"        : "madeupium",
            "bendingInertiaRatio" : 1.0, # Default
            "shearMembraneRatio"  : 5.0/6.0} # Default

mystran.setAnalysisVal("Property", [("Skin", skin),
                                    ("Rib_Root", ribSpar)])

constraint = {"groupName" : "Rib_Root",
              "dofConstraint" : 123456}
mystran.setAnalysisVal("Constraint", ("edgeConstraint", constraint))

# External pressure load to mystran that we will inherited from su2
load = {"loadType" : "PressureExternal"}
mystran.setAnalysisVal("Load", ("pressureAero", load ))

####### Tetgen ########################
# Run pre/post-analysis for tetgen
print ("\nRunning PreAnalysis ......", "tetgen")
myMesh.preAnalysis()

print ("\nRunning PostAnalysis ......", "tetgen")
myMesh.postAnalysis()
#######################################


# Populate vertex sets in the bounds after the mesh generation is copleted
for j in transfers:
    myProblem.dataBound[j].fillVertexSets()

####### SU2 ###########################
# Re-run the preAnalysis
print ("\nRunning PreAnalysis ......", "su2")
su2.preAnalysis()

# Run su2
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(su2.analysisDir) # Move into test directory

try:
    su2Run(su2.getAnalysisVal("Proj_Name") + ".cfg", args.numberProc) # Run SU2 CFD solver
except:
    print("SU2 FAILED!")
    myProblem.closeCAPS()
    raise SystemError

os.chdir(currentDirectory) # Move back to top directory

print ("\nRunning PostAnalysis ......", "su2")
# Run AIM post-analysis
su2.postAnalysis()
#######################################


# Execute and plot the dataTransfer
print ("\nExecuting dataTransfer ......")
for j in transfers:
    myProblem.dataBound[j].executeTransfer()
    if (args.noPlotData == False):
        try:
            print ("\tPlotting dataTransfer source......", j)
            myProblem.dataBound[j].dataSetSrc["Pressure"].viewData()
            print ("\tPlotting dataTransfer destination......")
            myProblem.dataBound[j].dataSetDest["Pressure"].viewData()
            #myProblem.dataBound[j].viewData("Pressure",
            #                                filename = os.path.join(su2.analysisDir, j + "_Pressure"),
            #                                showImage=False,
            #                                colormap="Purples")
        except:
            print("Unable to plot data")


####### Mystran #######################
# Run pre/post-analysis for mystran and execute
print ("\nRunning PreAnalysis ......", "mystran")
mystran.preAnalysis()

# Run mystran
print ("\n\nRunning Mystran......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(mystran.analysisDir) # Move into test directory

os.system("mystran.exe " + projectName +  ".dat > Info.out") # Run su2 via system call

if os.path.getsize("Info.out") == 0:
    print ("Mystran excution failed\n")
    myProblem.closeCAPS()
    raise SystemError

os.chdir(currentDirectory) # Move back to top directory

print ("\nRunning PostAnalysis ......", "mystran")
mystran.postAnalysis()
#######################################


# Close CAPS
myProblem.closeCAPS()
