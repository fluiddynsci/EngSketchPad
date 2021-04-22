from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'FUN3D and EGADS Tessellation PyTest Example',
                                 prog = 'fun3d_and_egadsTess_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "FUN3DegadsTessAnalysisTest")

# Load CSM file
geometryScript = os.path.join("..","csmData","cfd2D.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load egadsTess aim
myProblem.loadAIM(aim = "egadsTessAIM", analysisDir = workDir)

# Set new EGADS body tessellation parameters
myProblem.analysis["egadsTessAIM"].setAnalysisVal("Tess_Params", [.01, 0.001, 20.0])
#myProblem.analysis["egadsTessAIM"].setAnalysisVal("Tess_Params", [1.0, 0.01, 20.0])

# Set project name and output mesh type
myProblem.analysis["egadsTessAIM"].setAnalysisVal("Proj_Name", "egadsTessMesh")

myProblem.analysis["egadsTessAIM"].setAnalysisVal("Mesh_Format", "Tecplot")

# Run AIM pre-analysis
myProblem.analysis["egadsTessAIM"].preAnalysis()

# NO analysis is needed - egadsTess was already ran during preAnalysis

# Run AIM post-analysis
myProblem.analysis["egadsTessAIM"].postAnalysis()

# Load FUN3D aim - child of egadsTess AIM
myProblem.loadAIM(aim = "fun3dAIM",
                  altName = "fun3d",
                  analysisDir = workDir, parents = ["egadsTessAIM"])

# Set project name
myProblem.analysis["fun3d"].setAnalysisVal("Proj_Name", "pyCAPS_FUN3D_egadsTess")

myProblem.analysis["fun3d"].setAnalysisVal("Mesh_ASCII_Flag", True)

# Set AoA number
myProblem.analysis["fun3d"].setAnalysisVal("Alpha", 0.0)

# Set equation type
myProblem.analysis["fun3d"].setAnalysisVal("Equation_Type","compressible")

# Set Viscous term
myProblem.analysis["fun3d"].setAnalysisVal("Viscous", "inviscid")

# Set number of iterations
myProblem.analysis["fun3d"].setAnalysisVal("Num_Iter",500)

# Set CFL number schedule
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule",[1, 40])

# Set read restart option
myProblem.analysis["fun3d"].setAnalysisVal("Restart_Read","off")

# Set CFL number iteration schedule
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule_Iter", [1, 200])

# Set overwrite fun3d.nml if not linking to Python library
myProblem.analysis["fun3d"].setAnalysisVal("Overwrite_NML", True)

# Set the aim to 2D mode
myProblem.analysis["fun3d"].setAnalysisVal("Two_Dimensional", True)

# Set Mach number
Mach = 0.4
gamma = 1.4

myProblem.analysis["fun3d"].setAnalysisVal("Mach", Mach)

# Set boundary conditions
inviscidBC = {"bcType" : "Inviscid", "wallTemperature" : 1}

backPressureBC = {"bcType" : "SubsonicOutflow",
                  "staticPressure" : 1}

inflow = {"bcType" : "SubsonicInflow",
          "totalPressure" : (1+ (gamma-1.0)/2.0*Mach**2)**(gamma/(gamma-1)), # Isentropic relation
          "totalTemperature" : (1+ (gamma-1.0)/2.0*Mach**2)}

myProblem.analysis["fun3d"].setAnalysisVal("Boundary_Condition", [("Airfoil"   , inviscidBC),
                                                                  ("TunnelWall", inviscidBC),
                                                                  ("InFlow", inflow),
                                                                  ("OutFlow",backPressureBC),
                                                                  ("2DSlice", "SymmetryY")])

# Run AIM pre-analysis
myProblem.analysis["fun3d"].preAnalysis()

####### Run fun3d ####################
print ("\n\nRunning FUN3D......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myProblem.analysis["fun3d"].analysisDir) # Move into test directory

os.system("nodet_mpi --animation_freq -10 > Info.out"); # Run fun3d via system call

haveFUN3D = True # See if FUN3D ran
if os.path.getsize("Info.out") == 0: #
    print ("FUN3D excution failed\n")
    haveFUN3D = False

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
myProblem.analysis["fun3d"].postAnalysis()

# Get force results
if haveFUN3D:
    print ("Total Force - Pressure + Viscous")
    # Get Lift and Drag coefficients
    print ("Cl = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CLtot"), \
           "Cd = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CDtot"))

    # Get Cmx, Cmy, and Cmz coefficients
    print ("Cmx = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMXtot"), \
           "Cmy = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMYtot"), \
           "Cmz = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMZtot"))

    # Get Cx, Cy, Cz coefficients
    print ("Cx = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CXtot"), \
           "Cy = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CYtot"), \
           "Cz = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CZtot"))

    print ("Pressure Contribution")
    # Get Lift and Drag coefficients
    print ("Cl_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CLtot_p"), \
           "Cd_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CDtot_p"))

    # Get Cmx, Cmy, and Cmz coefficients
    print ("Cmx_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMXtot_p"), \
           "Cmy_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMYtot_p"), \
           "Cmz_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMZtot_p"))

    # Get Cx, Cy, and Cz, coefficients
    print ("Cx_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CXtot_p"), \
           "Cy_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CYtot_p"), \
           "Cz_p = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CZtot_p"))

    print ("Viscous Contribution")
    # Get Lift and Drag coefficients
    print ("Cl_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CLtot_v"), \
           "Cd_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CDtot_v"))

    # Get Cmx, Cmy, and Cmz coefficients
    print ("Cmx_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMXtot_v"), \
           "Cmy_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMYtot_v"), \
           "Cmz_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CMZtot_v"))

    # Get Cx, Cy, and Cz, coefficients
    print ("Cx_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CXtot_v"), \
           "Cy_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CYtot_v"), \
           "Cz_v = " , myProblem.analysis["fun3d"].getAnalysisOutVal("CZtot_v"))

# Save away CAPS problem
#myProblem.saveCAPS()

# Close CAPS
myProblem.closeCAPS()
