from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'FUN3D and Tetgen Alpha Sweep PyTest Example',
                                 prog = 'fun3d_and_Tetgen_AlphaSweep_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "FUN3DTetgenAlphaSweep")

# Load CSM file
geometryScript = os.path.join("..","csmData","cfdMultiBody.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load Tetgen aim
myProblem.loadAIM(aim = "tetgenAIM", analysisDir = workDir)

# Set new EGADS body tessellation parameters
myProblem.analysis["tetgenAIM"].setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])

# Preserve surface mesh while messhing
myProblem.analysis["tetgenAIM"].setAnalysisVal("Preserve_Surf_Mesh", True)

# Get analysis info.
myProblem.analysis["tetgenAIM"].getAnalysisInfo()

# Run AIM pre-analysis
myProblem.analysis["tetgenAIM"].preAnalysis()

# NO analysis is needed - Tetgen was already ran during preAnalysis

# Run AIM post-analysis
myProblem.analysis["tetgenAIM"].postAnalysis()

# Get analysis info.
myProblem.analysis["tetgenAIM"].getAnalysisInfo()

Alpha = [-5, 0, 5, 10]
Cl = []
Cd = []
for a in Alpha:
    analysisID = "Alpha_" + str(a)
    # Load FUN3D aim - child of Tetgen AIM
    myProblem.loadAIM(aim = "fun3dAIM",
                      altName = analysisID,
                      analysisDir = workDir + "/" + analysisID, parents = ["tetgenAIM"])

    # Set project name
    myProblem.analysis[analysisID].setAnalysisVal("Proj_Name", analysisID)

    myProblem.analysis[analysisID].setAnalysisVal("Mesh_ASCII_Flag", False)

    # Set AoA number
    myProblem.analysis[analysisID].setAnalysisVal("Alpha", a)

    # Set Mach number
    myProblem.analysis[analysisID].setAnalysisVal("Mach", 0.25)

    # Set equation type
    myProblem.analysis[analysisID].setAnalysisVal("Equation_Type","compressible")

    # Set Viscous term
    myProblem.analysis[analysisID].setAnalysisVal("Viscous", "inviscid")

    # Set number of iterations
    myProblem.analysis[analysisID].setAnalysisVal("Num_Iter",10)

    # Set CFL number schedule
    myProblem.analysis[analysisID].setAnalysisVal("CFL_Schedule",[0.5, 3.0])

    # Set read restart option
    myProblem.analysis[analysisID].setAnalysisVal("Restart_Read","off")

    # Set CFL number iteration schedule
    myProblem.analysis[analysisID].setAnalysisVal("CFL_Schedule_Iter", [1, 40])

    # Set overwrite fun3d.nml if not linking to Python library
    myProblem.analysis[analysisID].setAnalysisVal("Overwrite_NML", True)

    # Set boundary conditions
    inviscidBC1 = {"bcType" : "Inviscid", "wallTemperature" : 1}
    inviscidBC2 = {"bcType" : "Inviscid", "wallTemperature" : 1.2}
    myProblem.analysis[analysisID].setAnalysisVal("Boundary_Condition", [("Wing1", inviscidBC1),
                                                                         ("Wing2", inviscidBC2),
                                                                         ("Farfield","farfield")])

    # Run AIM pre-analysis
    myProblem.analysis[analysisID].preAnalysis()

    ####### Run fun3d ####################
    print ("\n\nRunning FUN3D......")
    currentDirectory = os.getcwd() # Get our current working directory

    os.chdir(myProblem.analysis[analysisID].analysisDir) # Move into test directory

    os.system("nodet_mpi --animation_freq -1000 > Info.out"); # Run fun3d via system call

    haveFUN3D = True # See if FUN3D ran
    if os.path.getsize("Info.out") == 0: #
        print ("FUN3D excution failed\n")
        haveFUN3D = False

    os.chdir(currentDirectory) # Move back to top directory
    #######################################

    # Run AIM post-analysis
    myProblem.analysis[analysisID].postAnalysis()

    # Get force results
    if haveFUN3D:
        Cl.append(myProblem.analysis[analysisID].getAnalysisOutVal("CLtot"))
        Cd.append(myProblem.analysis[analysisID].getAnalysisOutVal("CDtot"))

print ("Alpha, CL, CD")
for i in range(len(Alpha)):
    print (Alpha[i], Cl[i], Cd[i])

# Close CAPS
myProblem.closeCAPS()
