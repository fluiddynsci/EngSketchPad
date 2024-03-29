# Import pyCAPS module
import pyCAPS

# Import os module
import os
import sys
import shutil

# Import SU2 python environment
from parallel_computation import parallel_computation as su2Run
from mesh_deformation import mesh_deformation as su2Deform

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
parser = argparse.ArgumentParser(description = 'Aeroelastic Displacement SU2 and Astros Example',
                                 prog = 'aeroelasticSimple_Displacement_SU2_and_Astros',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ["." + os.sep], nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot data")
parser.add_argument("-outLevel", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "AeroelasticSimple_Displacement_SU2_Astros")

# Create projectName vairbale
projectName = "aeroelasticSimple_Displacement_SA"

# Load CSM file
geometryScript = os.path.join("..","csmData","aeroelasticDataTransferSimple.csm")
myProblem = pyCAPS.Problem(problemName=workDir,
                           capsFile=geometryScript,
                           outLevel=args.outLevel)

# Load AIMs
surfMesh = myProblem.analysis.create(aim = "aflr4AIM",
                                     name= "aflr4",
                                     capsIntent = "Aerodynamic")

mesh = myProblem.analysis.create(aim = "aflr3AIM",
                                 name= "aflr3",
                                 capsIntent = "Aerodynamic")

mesh.input["Surface_Mesh"].link(surfMesh.output["Surface_Mesh"])

su2 = myProblem.analysis.create(aim = "su2AIM",
                                name = "su2",
                                capsIntent = "Aerodynamic")

su2.input["Mesh"].link(mesh.output["Volume_Mesh"])

astros = myProblem.analysis.create(aim = "astrosAIM",
                                   name = "astros",
                                   capsIntent = "Structure",
                                   autoExec = False)

# Create the data transfer connections
boundNames = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for boundName in boundNames:
    # Create the bound
    bound = myProblem.bound.create(boundName)

    # Create the vertex sets on the bound for su2 and astros analysis
    su2Vset    = bound.vertexSet.create(su2)
    astrosVset = bound.vertexSet.create(astros)

    # Create displacement data sets
    su2_Displacement    = su2Vset.dataSet.create("Displacement")
    astros_Displacement = astrosVset.dataSet.create("Displacement")

    # Link the data set
    su2_Displacement.link(astros_Displacement, "Interpolate")

    # Close the bound as complete (cannot create more vertex or data sets)
    bound.close()


# Farfield growth factor
surfMesh.input.ff_cdfr = 1.4

# Set maximum and minimum edge lengths relative to capsMeshLength
surfMesh.input.max_scale = 0.75
surfMesh.input.min_scale = 0.1

# Set inputs for volume mesh
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
su2.input.SU2_Version = "Harrier"
su2.input.Pressure_Scale_Factor = 0.5*refDensity*refVelocity**2
su2.input.Surface_Monitor = ["Skin"]

inviscid = {"bcType" : "Inviscid"}
su2.input.Boundary_Condition = {"Skin"     : inviscid,
                                "SymmPlane": "SymmetryY",
                                "Farfield" : "farfield"}

# BC names of surfaces that should be deformed (default all invisicd and viscous)
su2.input.Surface_Deform = ["Skin"]


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

# Static uniform pressure load
load = {"groupName": "Skin", "loadType" : "Pressure", "pressureForce": 1E8}
astros.input.Load = {"pressureInternal": load}


####### Astros #######################
# Run pre/post-analysis for astros and execute
print ("\nRunning PreAnalysis ......", "astros")
astros.preAnalysis()

# Run astros
print ("\n\nRunning Astros......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(astros.analysisDir) # Move into test directory

# Copy files needed to run astros
astros_files = ["ASTRO.D01",  # *.DO1 file
                "ASTRO.IDX"]  # *.IDX file
for file in astros_files:
    if not os.path.isfile(file):
        shutil.copy(ASTROS_ROOT + os.sep + file, file)

# Run Astros via system call
os.system("astros.exe < " + projectName +  ".dat > " + projectName + ".out");

os.chdir(currentDirectory) # Move back to top directory

print ("\nRunning PostAnalysis ......", "astros")
astros.postAnalysis()
#######################################

# Plot the dataTransfer
for boundName in boundNames:
    if args.noPlotData == False:
        try:
            print ("\tPlotting dataTransfer source......", boundName)
            myProblem.bound[boundName].vertexSet["astros"].dataSet["Displacement"].view()
            print ("\tPlotting dataTransfer destination......")
            myProblem.bound[boundName].vertexSet["su2"].dataSet["Displacement"].view()
        except ImportError:
            print("Unable to plot data")


####### SU2 ###########################
# Re-run the preAnalysis
print ("\nRunning PreAnalysis ......", "su2")
su2.preAnalysis()

# Run su2
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(su2.analysisDir) # Move into test directory

# Run SU2 mesh deformation
su2Deform(su2.input.Proj_Name + ".cfg", args.numberProc)

# Run SU2 CFD solver
su2Run(su2.input.Proj_Name + ".cfg", args.numberProc)

os.chdir(currentDirectory) # Move back to top directory

print ("\nRunning PostAnalysis ......", "su2")
# Run AIM post-analysis
su2.postAnalysis()
#######################################
