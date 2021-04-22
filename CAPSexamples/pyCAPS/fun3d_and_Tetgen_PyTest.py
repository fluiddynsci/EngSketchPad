# Import other need modules
## [importModules]
from __future__ import print_function

import os
## [importModules]

import argparse

# Import pyCAPS class file
## [importpyCAPS]
from pyCAPS import capsProblem
## [importpyCAPS]

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'FUN3D and Tetgen Pytest Example',
                                 prog = 'fun3d_and_Tetgen_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()


# Initialize capsProblem object
## [initateProblem]
myProblem = capsProblem()
## [initateProblem]

# Create working directory variable
## [localVariable]
workDir = os.path.join(str(args.workDir[0]), "FUN3DTetgenAnalysisTest")
## [localVariable]

# Load CSM file
## [loadGeom]
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)
## [loadGeom]

# Change a design parameter - area in the geometry
## [setGeom]
myGeometry.setGeometryVal("area", 50)
## [setGeom]

# Write out a new egads file for the geometry
myGeometry.saveGeometry("pyCAPS_EGADS",
                        directory = workDir,
                        extension="egads") # .egads extension added by default

# Load Tetgen aim
## [loadMesh]
meshAIM = myProblem.loadAIM(aim = "tetgenAIM",
                            analysisDir = workDir)
## [loadMesh]

## [setMesh]
# Set new EGADS body tessellation parameters
meshAIM.setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])

# Preserve surface mesh while meshing
meshAIM.setAnalysisVal("Preserve_Surf_Mesh", True)
## [setMesh]

# Run AIM pre-analysis
## [preAnalysisMesh]
meshAIM.preAnalysis()
## [preAnalysisMesh]

# NO analysis is needed - Tetgen was already ran during preAnalysis

# Run AIM post-analysis
## [postAnalysisMesh]
meshAIM.postAnalysis()
## [postAnalysisMesh]

# Get analysis info.
myProblem.analysis["tetgenAIM"].getAnalysisInfo()

## [loadFUN3D]
# Load FUN3D aim - child of Tetgen AIM
fun3dAIM = myProblem.loadAIM(aim = "fun3dAIM",
                             altName = "fun3d",
                             analysisDir = workDir, parents = meshAIM.aimName)
## [loadFUN3D]

##[setFUN3D]
# Set project name
fun3dAIM.setAnalysisVal("Proj_Name", "fun3dTetgenTest")

fun3dAIM.setAnalysisVal("Mesh_ASCII_Flag", False)

# Set AoA number
myProblem.analysis["fun3d"].setAnalysisVal("Alpha", 1.0)

# Set Mach number
myProblem.analysis["fun3d"].setAnalysisVal("Mach", 0.5901)

# Set equation type
fun3dAIM.setAnalysisVal("Equation_Type","compressible")

# Set Viscous term
myProblem.analysis["fun3d"].setAnalysisVal("Viscous", "inviscid")

# Set number of iterations
myProblem.analysis["fun3d"].setAnalysisVal("Num_Iter",10)

# Set CFL number schedule
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule",[0.5, 3.0])

# Set read restart option
fun3dAIM.setAnalysisVal("Restart_Read","off")

# Set CFL number iteration schedule
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule_Iter", [1, 40])

# Set overwrite fun3d.nml if not linking to Python library
myProblem.analysis["fun3d"].setAnalysisVal("Overwrite_NML", True)

##[setFUN3D]

# Set boundary conditions
##[setFUN3DBC]
inviscidBC1 = {"bcType" : "Inviscid", "wallTemperature" : 1}
inviscidBC2 = {"bcType" : "Inviscid", "wallTemperature" : 1.2}
fun3dAIM.setAnalysisVal("Boundary_Condition", [("Wing1", inviscidBC1),
                                               ("Wing2", inviscidBC2),
                                               ("Farfield","farfield")])
##[setFUN3DBC]

# Run AIM pre-analysis
## [preAnalysisFUN3D]
fun3dAIM.preAnalysis()
## [preAnalysisFUN3D]

####### Run fun3d ####################
print ("\n\nRunning FUN3D......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(fun3dAIM.analysisDir) # Move into test directory

if (args.noAnalysis == False):
    os.system("nodet_mpi --animation_freq -1 --write_aero_loads_to_file> Info.out"); # Run fun3d via system call

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
## [postAnalysiFUN3D]
fun3dAIM.postAnalysis()
## [postAnalysiFUN3D]

# Get force results
##[results]
print ("Total Force - Pressure + Viscous")
# Get Lift and Drag coefficients
print ("Cl = " , fun3dAIM.getAnalysisOutVal("CLtot"),
       "Cd = " , fun3dAIM.getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx = " , fun3dAIM.getAnalysisOutVal("CMXtot"),
       "Cmy = " , fun3dAIM.getAnalysisOutVal("CMYtot"),
       "Cmz = " , fun3dAIM.getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print ("Cx = " , fun3dAIM.getAnalysisOutVal("CXtot"),
       "Cy = " , fun3dAIM.getAnalysisOutVal("CYtot"),
       "Cz = " , fun3dAIM.getAnalysisOutVal("CZtot"))

print ("Pressure Contribution")
# Get Lift and Drag coefficients
print ("Cl_p = " , fun3dAIM.getAnalysisOutVal("CLtot_p"),
       "Cd_p = " , fun3dAIM.getAnalysisOutVal("CDtot_p"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx_p = " , fun3dAIM.getAnalysisOutVal("CMXtot_p"),
       "Cmy_p = " , fun3dAIM.getAnalysisOutVal("CMYtot_p"),
       "Cmz_p = " , fun3dAIM.getAnalysisOutVal("CMZtot_p"))

# Get Cx, Cy, and Cz, coefficients
print ("Cx_p = " , fun3dAIM.getAnalysisOutVal("CXtot_p"),
       "Cy_p = " , fun3dAIM.getAnalysisOutVal("CYtot_p"),
       "Cz_p = " , fun3dAIM.getAnalysisOutVal("CZtot_p"))

print ("Viscous Contribution")
# Get Lift and Drag coefficients
print ("Cl_v = " , fun3dAIM.getAnalysisOutVal("CLtot_v"),
       "Cd_v = " , fun3dAIM.getAnalysisOutVal("CDtot_v"))

# Get Cmx, Cmy, and Cmz coefficients
print ("Cmx_v = " , fun3dAIM.getAnalysisOutVal("CMXtot_v"),
       "Cmy_v = " , fun3dAIM.getAnalysisOutVal("CMYtot_v"),
       "Cmz_v = " , fun3dAIM.getAnalysisOutVal("CMZtot_v"))

# Get Cx, Cy, and Cz, coefficients
print ("Cx_v = " , fun3dAIM.getAnalysisOutVal("CXtot_v"),
       "Cy_v = " , fun3dAIM.getAnalysisOutVal("CYtot_v"),
       "Cz_v = " , fun3dAIM.getAnalysisOutVal("CZtot_v"))
##[results]

# Print out dictionary of component forces
for i in fun3dAIM.getAnalysisOutVal("Forces"):
    print ("Boundary name: ", i[0])
    for j in i[1].keys():
        print (" ", j," = ", i[1][j])

# Close CAPS
## [closeCAPS]
myProblem.closeCAPS()
## [closeCAPS]
