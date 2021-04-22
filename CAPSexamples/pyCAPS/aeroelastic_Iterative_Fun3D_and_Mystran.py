from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import shutil
import math

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Aeroelastic Fun3D and Mystran Example',
                                 prog = 'aeroelastic_Iterative_Fun2_and_Mystran',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "Aeroelastic_Iterative_FM")

# Create projectName vairbale
projectName = "aeroelastic_Iterative"

# Set the number of transfer iterations
numTransferIteration = 2

# Load CSM file
geometryScript = os.path.join("..","csmData","aeroelasticDataTransfer.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AIMs
myProblem.loadAIM(aim = "tetgenAIM",
                  altName= "tetgen",
                  analysisDir = workDir + "_FUN3D",
                  capsIntent = "CFD")


myProblem.loadAIM(aim = "fun3dAIM",
                  altName = "fun3d",
                  analysisDir = workDir + "_FUN3D",
                  parents = ["tetgen"],
                  capsIntent = "CFD")

myProblem.loadAIM(aim = "mystranAIM",
                  altName = "mystran",
                  analysisDir = workDir + "_MYSTRAN",
                  capsIntent = "STRUCTURE")

transfers = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for i in transfers:
    myProblem.createDataTransfer(variableName = ["Pressure", "Displacement"],
                                 aimSrc = ["fun3d", "mystran"],
                                 aimDest =["mystran", "fun3d"],
                                 transferMethod = ["Conserve", "Conserve"], #["Conserve", "Interpolate"]
                                 initValueDest  = [None, (0,0,0)],
                                 capsBound = i)

# Set inputs for tetgen
myProblem.analysis["tetgen"].setAnalysisVal("Tess_Params", [.3, 0.01, 20.0])
myProblem.analysis["tetgen"].setAnalysisVal("Preserve_Surf_Mesh", True)

# Set inputs for fun3d
Mach = 0.3
Rair = 1716 #ft^2/ (s^2* R)
refTemperature = 491.4 # R
gamma = 1.4
speedofSound = math.sqrt(gamma*refTemperature*Rair) # ft/s

refVelocity = speedofSound*Mach # ft/s
refDensity = 0.0023366 # slug/ft^3
refPressure = (refDensity*speedofSound**2)/gamma

AoA = 10 # angle of attack

myProblem.analysis["fun3d"].setAnalysisVal("Proj_Name", projectName)
myProblem.analysis["fun3d"].setAnalysisVal("Mesh_ASCII_Flag", False)
myProblem.analysis["fun3d"].setAnalysisVal("Mach", Mach)
myProblem.analysis["fun3d"].setAnalysisVal("Alpha", AoA)
myProblem.analysis["fun3d"].setAnalysisVal("Equation_Type","compressible")
myProblem.analysis["fun3d"].setAnalysisVal("Viscous", "inviscid")
myProblem.analysis["fun3d"].setAnalysisVal("Num_Iter",1000)
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule",[1, 5.0])
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule_Iter", [1, 40])
myProblem.analysis["fun3d"].setAnalysisVal("Overwrite_NML", True)
myProblem.analysis["fun3d"].setAnalysisVal("Restart_Read","off")
myProblem.analysis[i].setAnalysisVal("Pressure_Scale_Factor", 0.5*refDensity*refVelocity**2)

inviscid = {"bcType" : "Inviscid", "wallTemperature" : 1.1}
myProblem.analysis["fun3d"].setAnalysisVal("Boundary_Condition", [("Skin", inviscid),
                                                                  ("SymmPlane", "SymmetryY"),
                                                                  ("Farfield","farfield")])

# Set inputs for mystran
myProblem.analysis["mystran"].setAnalysisVal("Proj_Name", projectName)
myProblem.analysis["mystran"].setAnalysisVal("Edge_Point_Max", 3)

myProblem.analysis["mystran"].setAnalysisVal("Quad_Mesh", True)
myProblem.analysis["mystran"].setAnalysisVal("Tess_Params", [.5, .1, 15])
myProblem.analysis["mystran"].setAnalysisVal("Analysis_Type", "Static");

# External pressure load to mystran that we will inherited from fun3d
load = {"loadType" : "PressureExternal", "loadScaleFactor" : -1.0}
myProblem.analysis["mystran"].setAnalysisVal("Load", ("pressureAero", load ))

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 1.5E9, # lbf/ft^2
                "poissonRatio": 0.33,
                "density" : 5.5} #slug/ft^3

unobtainium  = {"materialType" : "isotropic",
                "youngModulus" : 1.0E10, # lbf/ft^2
                "poissonRatio": 0.33,
                "density" : 6.0} #slug/ft^3

myProblem.analysis["mystran"].setAnalysisVal("Material", [("Madeupium", madeupium),
                                                          ("Unobtainium", unobtainium)])

skin  = {"propertyType" : "Shell",
         "membraneThickness" : 0.0052, # ft - 1/16 in
         "material"        : "madeupium",
         "bendingInertiaRatio" : 1.0, # Default
         "shearMembraneRatio"  : 5.0/6.0} # Default

ribSpar  = {"propertyType" : "Shell",
            "membraneThickness" : 0.0104, # ft  - 0.25 in
            "material"        : "unobtainium",
            "bendingInertiaRatio" : 1.0, # Default
            "shearMembraneRatio"  : 5.0/6.0} # Default

myProblem.analysis["mystran"].setAnalysisVal("Property", [("Skin", skin),
                                                          ("Ribs_and_Spars", ribSpar),
                                                          ("Rib_Root", ribSpar)])
constraint = {"groupName" : "Rib_Root",
              "dofConstraint" : 123456}
myProblem.analysis["mystran"].setAnalysisVal("Constraint", ("edgeConstraint", constraint))


####### Tetgen ########################
# Run pre/post-analysis for tetgen
print ("\nRunning PreAnalysis ......", "tetgen")
myProblem.analysis["tetgen"].preAnalysis()

print ("\nRunning PostAnalysis ......", "tetgen")
myProblem.analysis["tetgen"].postAnalysis()
#######################################

# Populate vertex sets in the bounds after the mesh generation is copleted
for j in transfers:
    myProblem.dataBound[j].fillVertexSets()

# Aeroelastic iteration loop
for iter in range(numTransferIteration):

    #Execute the dataTransfer of displacements to su2
    #initValueDest is used on the first iteration
    print ("\n\nExecuting dataTransfer \"Displacement\"......")
    for j in transfers:
        myProblem.dataBound[j].executeTransfer("Displacement")

    ####### FUN3D #########################
    print ("\nRunning PreAnalysis ......", "fun3d")
    myProblem.analysis["fun3d"].preAnalysis()

    #------------------------------
    print ("\n\nRunning FUN3D......")
    currentDirectory = os.getcwd() # Get our current working directory

    os.chdir(myProblem.analysis["fun3d"].analysisDir) # Move into test directory

    #--write_aero_loads_to_file --aeroelastic_external
    cmdLineOpt = "--write_aero_loads_to_file --animation_freq -1"
    if iter != 0:
        cmdLineOpt = cmdLineOpt + " --read_surface_from_file"

    os.system("mpirun -np 10 nodet_mpi " + cmdLineOpt + " > Info.out"); # Run fun3d via system call

    if os.path.getsize("Info.out") == 0: #
        print ("FUN3D excution failed\n")
        myProblem.closeCAPS()
        raise SystemError

    shutil.copy(projectName + "_tec_boundary.plt", projectName + "_tec_boundary" + "_Interation_" + str(iter) + ".plt")
    os.chdir(currentDirectory) # Move back to top directory
    #------------------------------

    print ("\nRunning PostAnalysis ......", "fun3d")
    # Run AIM post-analysis
    myProblem.analysis["fun3d"].postAnalysis()
    #######################################

    #Execute the dataTransfer of Pressure to mystran
    print ("\n\nExecuting dataTransfer \"Pressure\"......")
    for j in transfers:
        myProblem.dataBound[j].executeTransfer("Pressure")

    ####### Mystran #######################
    print ("\nRunning PreAnalysis ......", "mystran")
    myProblem.analysis["mystran"].preAnalysis()

    #------------------------------
    print ("\n\nRunning Mystran......")
    currentDirectory = os.getcwd() # Get our current working directory

    os.chdir(myProblem.analysis["mystran"].analysisDir) # Move into test directory

    os.system("mystran.exe " + projectName +  ".dat > Info.out") # Run mystran via system call

    if os.path.getsize("Info.out") == 0:
        print ("Mystran excution failed\n")
        myProblem.closeCAPS()
        raise SystemError

    os.chdir(currentDirectory) # Move back to top directory
    #------------------------------

    print ("\nRunning PostAnalysis ......", "mystran")
    myProblem.analysis["mystran"].postAnalysis()
    #######################################

# Close CAPS
myProblem.closeCAPS()
