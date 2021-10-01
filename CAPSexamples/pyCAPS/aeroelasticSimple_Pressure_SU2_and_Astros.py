# Import pyCAPS module
import pyCAPS

# Import os module
import os
import sys
import shutil

# Import SU2 python environment
from parallel_computation import parallel_computation as su2Run

# ASTROS_ROOT should be the path containing ASTRO.D01 and ASTRO.IDX
try:
   ASTROS_ROOT = os.environ["ASTROS_ROOT"]
   os.putenv("PATH", ASTROS_ROOT + os.pathsep + os.getenv("PATH"))
except KeyError:
   print("Please set the environment variable ASTROS_ROOT")
   sys.exit(1)

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Aeroelastic Pressure SU2 and Astros Example',
                                 prog = 'aeroelasticSimple_Pressure_SU2_and_Astros',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ["." + os.sep], nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot data")
parser.add_argument("-outLevel", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "AeroelasticSimple_Pressure_SU2_Astros")

# Create projectName vairbale
projectName = "aeroelasticSimple_Pressure_SA"

# Load CSM file
geometryScript = os.path.join("..","csmData","aeroelasticDataTransferSimple.csm")
myProblem = pyCAPS.Problem(problemName=workDir,
                           capsFile=geometryScript,
                           outLevel=args.outLevel)

# Load AIMs
surfMesh = myProblem.analysis.create(aim = "egadsTessAIM",
                                     name= "egads",
                                     capsIntent = "CFD")

mesh = myProblem.analysis.create(aim = "tetgenAIM",
                                 name= "tetgen",
                                 capsIntent = "CFD")

mesh.input["Surface_Mesh"].link(surfMesh.output["Surface_Mesh"])

su2 = myProblem.analysis.create(aim = "su2AIM",
                                name = "su2",
                                capsIntent = "CFD")

su2.input["Mesh"].link(mesh.output["Volume_Mesh"])

astros = myProblem.analysis.create(aim = "astrosAIM",
                                   name = "astros",
                                   capsIntent = "STRUCTURE",
                                   autoExec = True)

# Create the data transfer connections
boundNames = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for boundName in boundNames:
    # Create the bound
    bound = myProblem.bound.create(boundName)

    # Create the vertex sets on the bound for su2 and astros analysis
    su2Vset    = bound.vertexSet.create(su2)
    astrosVset = bound.vertexSet.create(astros)

    # Create displacement data sets
    su2_Pressure    = su2Vset.dataSet.create("Pressure", pyCAPS.fType.FieldOut)
    astros_Pressure = astrosVset.dataSet.create("Pressure", pyCAPS.fType.FieldIn)

    # Link the data set
    astros_Pressure.link(su2_Pressure, "Conserve")

    # Close the bound as complete (cannot create more vertex or data sets)
    bound.close()

# Set inputs for egads
surfMesh.input.Tess_Params = [.05, 0.01, 20.0]
surfMesh.input.Edge_Point_Max = 4

# Set inputs for tetgen
mesh.input.Preserve_Surf_Mesh = True
mesh.input.Mesh_Quiet_Flag = True if args.outLevel == 0 else False

# Set inputs for su2
speedofSound = 340.0 # m/s
refVelocity = 100.0 # m/s
refDensity = 1.2 # kg/m^3

su2.input.Proj_Name = projectName
su2.input.Mach = refVelocity/speedofSound
su2.input.Equation_Type = "compressible"
su2.input.Num_Iter = 3
su2.input.Output_Format = "Tecplot"
su2.input.SU2_Version = "Blackbird"
su2.input.Pressure_Scale_Factor = 0.5*refDensity*refVelocity**2
su2.input.Surface_Monitor = ["Skin"]

inviscid = {"bcType" : "Inviscid"}
su2.input.Boundary_Condition = {"Skin"     : inviscid,
                                "SymmPlane": "SymmetryY",
                                "Farfield" : "farfield"}

# Set inputs for astros
astros.input.Proj_Name = projectName
astros.input.Edge_Point_Max = 10
astros.input.Edge_Point_Min = 10

astros.input.Quad_Mesh = True
astros.input.Tess_Params = [.5, .1, 15]
astros.input.Analysis_Type = "Static"

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 72.0E9 ,
                "poissonRatio": 0.33,
                "density" : 2.8E3}
astros.input.Material = {"Madeupium": madeupium}

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

astros.input.Property = {"Skin"    : skin,
                         "Rib_Root": ribSpar}

constraint = {"groupName" : "Rib_Root",
              "dofConstraint" : 123456}
astros.input.Constraint = {"edgeConstraint": constraint}

# External pressure load to astros that we will inherited from su2
load = {"loadType" : "PressureExternal"}
astros.input.Load = {"pressureAero": load}


####### SU2 ###########################
# Re-run the preAnalysis
print ("\nRunning PreAnalysis ......", "su2")
su2.preAnalysis()

# Run su2
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(su2.analysisDir) # Move into test directory

try:
    su2Run(su2.input.Proj_Name + ".cfg", args.numberProc) # Run SU2 CFD solver
except:
    raise SystemError("SU2 FAILED!")

os.chdir(currentDirectory) # Move back to top directory

print ("\nRunning PostAnalysis ......", "su2")
# Run AIM post-analysis
su2.postAnalysis()
#######################################

# Plot the dataTransfer
for boundName in boundNames:
    if args.noPlotData == False:
        try:
            print ("\tPlotting dataTransfer source......", boundName)
            myProblem.bound[boundName].vertexSet["su2"].dataSet["Pressure"].view()
            print ("\tPlotting dataTransfer destination......")
            myProblem.bound[boundName].vertexSet["astros"].dataSet["Pressure"].view()
        except ImportError:
            print("Unable to plot data")


####### Astros #######################
# Run analysis for astros 
print ("\nRunning ......", "astros")
astros.runAnalysis()
#######################################

