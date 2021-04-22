from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'FUN3D and AFLR Pytest Example',
                                 prog = 'fun3d_and_AFLR4_AFLR3_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "FUN3DandAFRLMeshTest")


# Load CSM file and build the geometry explicitly
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
myGeometry.buildGeometry()

# Load AFLR4 aim
aflr4 = myProblem.loadAIM(aim = "aflr4AIM",
                               analysisDir= workDir)

# Set project name - so a mesh file is generated
aflr4.setAnalysisVal("Proj_Name", "pyCAPS_AFLR4_AFLR3")

# Set AIM verbosity
aflr4.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set output grid format since a project name is being supplied - Tecplot  file
aflr4.setAnalysisVal("Mesh_Format", "Tecplot")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Set maximum and minimum edge lengths relative to capsMeshLength
aflr4.setAnalysisVal("max_scale", 0.2)
aflr4.setAnalysisVal("min_scale", 0.01)

# Run AIM pre-analysis
aflr4.preAnalysis()

#######################################
## AFRL4 was ran during preAnalysis ##
#######################################

# Run AIM post-analysis
aflr4.postAnalysis()

#######################################
## Build volume mesh off of surface  ##
##  mesh(es) using AFLR3             ##
#######################################

# Load AFLR3 aim
aflr3 = myProblem.loadAIM(aim = "aflr3AIM",
                              analysisDir= workDir,
                              parents = aflr4.aimName)

# Set AIM verbosity
aflr3.setAnalysisVal("Mesh_Quiet_Flag", True if args.verbosity == 0 else False)

# Set project name - so a mesh file is generated
aflr3.setAnalysisVal("Proj_Name", "pyCAPS_AFLR4_AFLR3_VolMesh")

# Set output grid format since a project name is being supplied - Tecplot tetrahedral file
aflr3.setAnalysisVal("Mesh_Format", "Tecplot")

# Run AIM pre-analysis
aflr3.preAnalysis()

#######################################
## AFLR3 was ran during preAnalysis ##
#######################################

# Run AIM post-analysis
aflr3.postAnalysis()

# Load FUN3D aim - child of AFLR3 AIM
fun3d = myProblem.loadAIM(aim = "fun3dAIM",
                          analysisDir = workDir,
                          parents = aflr3.aimName)

# Set project name
fun3d.setAnalysisVal("Proj_Name", "fun3dAFLRTest")

# Reset the mesh ascii flag to false - This will generate a lb8.ugrid file
fun3d.setAnalysisVal("Mesh_ASCII_Flag", False)

# Set AoA number
fun3d.setAnalysisVal("Alpha", 1.0)

# Set Viscous term
fun3d.setAnalysisVal("Viscous", "laminar")

# Set Mach number
fun3d.setAnalysisVal("Mach", 0.5901)

# Set Reynolds number
fun3d.setAnalysisVal("Re", 10E5)

# Set equation type
fun3d.setAnalysisVal("Equation_Type","compressible")

# Set number of iterations
fun3d.setAnalysisVal("Num_Iter",10)

# Set CFL number schedule
fun3d.setAnalysisVal("CFL_Schedule",[0.5, 3.0])

# Set read restart option
fun3d.setAnalysisVal("Restart_Read","off")

# Set CFL number iteration schedule
fun3d.setAnalysisVal("CFL_Schedule_Iter", [1, 40])

# Set overwrite fun3d.nml if not linking to Python library
fun3d.setAnalysisVal("Overwrite_NML", True)

# Set boundary conditions
bc1 = {"bcType" : "Viscous", "wallTemperature" : 1}
bc2 = {"bcType" : "Inviscid", "wallTemperature" : 1.2}
fun3d.setAnalysisVal("Boundary_Condition", [("Wing1", bc1),
                                            ("Wing2", bc2),
                                            ("Farfield","farfield")])

# Run AIM pre-analysis
fun3d.preAnalysis()

####### Run fun3d ####################
print ("\n\nRunning FUN3D......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(fun3d.analysisDir) # Move into test directory

# Mesh is to large for a single core
if (args.noAnalysis == False):
    os.system("mpirun -np " + str(args.numberProc) + " nodet_mpi  --animation_freq -1 --volume_animation_freq -1 > Info.out"); # Run fun3d via system call

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
fun3d.postAnalysis()

# Get force results
print ("Total Force - Pressure + Viscous")
# Get Lift and Drag coefficients
print ("Cl = " , fun3d.getAnalysisOutVal("CLtot"), \
       "Cd = " , fun3d.getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx = " , fun3d.getAnalysisOutVal("CMXtot"), \
       "Cmy = " , fun3d.getAnalysisOutVal("CMYtot"), \
       "Cmz = " , fun3d.getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print ("Cx = " , fun3d.getAnalysisOutVal("CXtot"), \
       "Cy = " , fun3d.getAnalysisOutVal("CYtot"), \
       "Cz = " , fun3d.getAnalysisOutVal("CZtot"))

print ("Pressure Contribution")
# Get Lift and Drag coefficients
print ("Cl_p = " , fun3d.getAnalysisOutVal("CLtot_p"), \
       "Cd_p = " , fun3d.getAnalysisOutVal("CDtot_p"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx_p = " , fun3d.getAnalysisOutVal("CMXtot_p"), \
       "Cmy_p = " , fun3d.getAnalysisOutVal("CMYtot_p"), \
       "Cmz_p = " , fun3d.getAnalysisOutVal("CMZtot_p"))

# Get Cx, Cy, and Cz, coefficients
print ("Cx_p = " , fun3d.getAnalysisOutVal("CXtot_p"), \
       "Cy_p = " , fun3d.getAnalysisOutVal("CYtot_p"), \
       "Cz_p = " , fun3d.getAnalysisOutVal("CZtot_p"))

print ("Viscous Contribution")
# Get Lift and Drag coefficients
print ("Cl_v = " , fun3d.getAnalysisOutVal("CLtot_v"), \
       "Cd_v = " , fun3d.getAnalysisOutVal("CDtot_v"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx_v = " , fun3d.getAnalysisOutVal("CMXtot_v"), \
       "Cmy_v = " , fun3d.getAnalysisOutVal("CMYtot_v"), \
       "Cmz_v = " , fun3d.getAnalysisOutVal("CMZtot_v"))

# Get Cx, Cy, and Cz, coefficients
print ("Cx_v = " , fun3d.getAnalysisOutVal("CXtot_v"), \
       "Cy_v = " , fun3d.getAnalysisOutVal("CYtot_v"), \
       "Cz_v = " , fun3d.getAnalysisOutVal("CZtot_v"))

# Close CAPS
myProblem.closeCAPS()
