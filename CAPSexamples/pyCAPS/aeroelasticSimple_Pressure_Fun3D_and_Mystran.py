# Import pyCAPS module
import pyCAPS

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Aeroelastic Pressure Fun3D and Mystran Example',
                                 prog = 'aeroelasticSimple_Pressure_Fun3D_and_Mystran',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ["." + os.sep], nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot data")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "AeroelasticSimple_Pressure_fun3d_Mystran")

# Create projectName vairbale
projectName = "aeroelasticSimple_Pressure"

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

fun3d = myProblem.analysis.create(aim = "fun3dAIM", 
                                  name = "fun3d", 
                                  capsIntent = "CFD")

fun3d.input["Mesh"].link(mesh.output["Volume_Mesh"])

mystran = myProblem.analysis.create(aim = "mystranAIM",
                                    name = "mystran",
                                    capsIntent = "STRUCTURE")

# Create the data transfer connections
boundNames = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for boundName in boundNames:
    # Create the bound
    bound = myProblem.bound.create(boundName)
    
    # Create the vertex sets on the bound for fun3d and mystran analysis
    fun3dVset   = bound.vertexSet.create(fun3d)
    mystranVset = bound.vertexSet.create(mystran)

    # Create pressure data sets
    fun3d_Pressure   = fun3dVset.dataSet.create("Pressure", pyCAPS.fType.FieldOut)
    mystran_Pressure = mystranVset.dataSet.create("Pressure", pyCAPS.fType.FieldIn)

    # Link the data set
    mystran_Pressure.link(fun3d_Pressure, "Conserve")
    
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

fun3d.input.Proj_Name = projectName
fun3d.input.Mesh_ASCII_Flag = False
fun3d.input.Mach = refVelocity/speedofSound
fun3d.input.Equation_Type = "compressible"
fun3d.input.Viscous = "inviscid"
fun3d.input.Num_Iter = 10
fun3d.input.CFL_Schedule = [1, 5.0]
fun3d.input.CFL_Schedule_Iter = [1, 40]
fun3d.input.Overwrite_NML = True
fun3d.input.Restart_Read = "off"

inviscid = {"bcType" : "Inviscid", "wallTemperature" : 1.1}
fun3d.input.Boundary_Condition = {"Skin"     : inviscid,
                                  "SymmPlane": "SymmetryY",
                                  "Farfield" : "farfield"}

# Set inputs for mystran
mystran.input.Proj_Name = projectName
mystran.input.Edge_Point_Max = 3
mystran.input.Edge_Point_Min = 3

mystran.input.Quad_Mesh = True
mystran.input.Tess_Params = [.5, .1, 15]
mystran.input.Analysis_Type = "Static"

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 72.0E9 ,
                "poissonRatio" : 0.33,
                "density" : 2.8E3}
mystran.input.Material = {"Madeupium": madeupium}

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

mystran.input.Property = {"Skin"    : skin,
                          "Rib_Root": ribSpar}

constraint = {"groupName" : "Rib_Root",
              "dofConstraint" : 123456}
mystran.input.Constraint = {"edgeConstraint": constraint}


####### EGADS ########################
# Run pre/post-analysis for tetgen
print ("\nRunning PreAnalysis ......", "tetgen")
surfMesh.preAnalysis()

print ("\nRunning PostAnalysis ......", "tetgen")
surfMesh.postAnalysis()
#######################################

####### Tetgen ########################
# Run pre/post-analysis for tetgen
print ("\nRunning PreAnalysis ......", "tetgen")
mesh.preAnalysis()

print ("\nRunning PostAnalysis ......", "tetgen")
mesh.postAnalysis()
#######################################

####### Run fun3d ####################
# Set scaling factor for pressure
fun3d.input.Pressure_Scale_Factor = 0.5*refDensity*refVelocity**2

print ("\n\nRunning FUN3D......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(fun3d.analysisDir) # Move into test directory

cmdLineOpt = "--write_aero_loads_to_file --animation_freq -1"

os.system("nodet_mpi " + cmdLineOpt + " > Info.out"); # Run fun3d via system call

if os.path.getsize("Info.out") == 0: #
    raise SystemError("FUN3D excution failed\n")

os.chdir(currentDirectory) # Move back to top directory
#######################################

####### Run mystran ####################
load = {"loadType" : "PressureExternal"} # Add pressure load card
mystran.input.Load = {"pressureAero": load}

# Re-run the preAnalysis
print ("\nRunning PreAnalysis ......", "mystran")
mystran.preAnalysis()

# Run mystran
print ("\n\nRunning Mystran......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(mystran.analysisDir) # Move into test directory

#--write_aero_loads_to_file
os.system("mystran.exe " + projectName +  ".dat > Info.out") # Run fun3d via system call

if os.path.getsize("Info.out") == 0: #
    raise SystemError("Mystran excution failed\n")

os.chdir(currentDirectory) # Move back to top directory

print ("\nRunning PostAnalysis ......", i)
# Run AIM post-analysis
mystran.postAnalysis()
#######################################
