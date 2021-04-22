from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os
import sys

# Import shutil module
import shutil

# Import SU2 python environment
from parallel_computation import parallel_computation as su2Run
from mesh_deformation import mesh_deformation as su2Deform

# Import argparse module
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Aeroelastic SU2 and Mystran Example',
                                 prog = 'aeroelasticSimple_Iterative_SU2_and_Mystran',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-numberProc', default = 1, nargs=1, type=float, help = 'Number of processors')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "AeroelasticSimple_Iterative")

# Create projectName vairbale
projectName = "aeroelasticSimple_Iterative_SM"

# Set the number of transfer iterations
numTransferIteration = 2

# Load CSM file
geometryScript = os.path.join("..","csmData","aeroelasticDataTransferSimple.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load AIMs
myProblem.loadAIM(aim = "tetgenAIM",
                  altName= "tetgen",
                  analysisDir = workDir + "_SU2",
                  capsIntent = "CFD")


myProblem.loadAIM(aim = "su2AIM",
                  altName = "su2",
                  analysisDir = workDir + "_SU2",
                  parents = ["tetgen"],
                  capsIntent = "CFD")

myProblem.loadAIM(aim = "mystranAIM",
                  altName = "mystran",
                  analysisDir = workDir + "_MYSTRAN",
                  capsIntent = "STRUCTURE")

transfers = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for i in transfers:
    myProblem.createDataTransfer(variableName   = ["Pressure", "Displacement"],
                                 aimSrc         = ["su2", "mystran"],
                                 aimDest        = ["mystran", "su2"],
                                 transferMethod = ["Interpolate", "Interpolate"], #["Conserve", "Interpolate"],
                                 initValueDest  = [None, (0,0,0)],
                                 capsBound      = i )

# Set inputs for tetgen
myProblem.analysis["tetgen"].setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])
myProblem.analysis["tetgen"].setAnalysisVal("Preserve_Surf_Mesh", True)

# Set inputs for su2
speedofSound = 340.0 # m/s
refVelocity = 100.0 # m/s
refDensity = 1.2 # kg/m^3

myProblem.analysis["su2"].setAnalysisVal("Proj_Name", projectName)
myProblem.analysis["su2"].setAnalysisVal("SU2_Version", "Falcon") # "Falcon", "Raven"
myProblem.analysis["su2"].setAnalysisVal("Mach", refVelocity/speedofSound)
myProblem.analysis["su2"].setAnalysisVal("Equation_Type","compressible")
myProblem.analysis["su2"].setAnalysisVal("Num_Iter",5) # Way too few to converge the solver, but this is an example
myProblem.analysis["su2"].setAnalysisVal("Output_Format", "Tecplot")
myProblem.analysis["su2"].setAnalysisVal("Overwrite_CFG", True)
myProblem.analysis["su2"].setAnalysisVal("Pressure_Scale_Factor",0.5*refDensity*refVelocity**2)
myProblem.analysis["su2"].setAnalysisVal("Surface_Monitor", "Skin")

inviscid = {"bcType" : "Inviscid"}
myProblem.analysis["su2"].setAnalysisVal("Boundary_Condition", [("Skin", inviscid),
                                                                ("SymmPlane", "SymmetryY"),
                                                                ("Farfield","farfield")])

# Set inputs for mystran
myProblem.analysis["mystran"].setAnalysisVal("Proj_Name", projectName)
myProblem.analysis["mystran"].setAnalysisVal("Edge_Point_Min", 3)
myProblem.analysis["mystran"].setAnalysisVal("Edge_Point_Max", 10)

myProblem.analysis["mystran"].setAnalysisVal("Quad_Mesh", True)
myProblem.analysis["mystran"].setAnalysisVal("Tess_Params", [.5, .1, 15])
myProblem.analysis["mystran"].setAnalysisVal("Analysis_Type", "Static");

# External pressure load to mystran that we will inherited from su2
load = {"loadType" : "PressureExternal"}
myProblem.analysis["mystran"].setAnalysisVal("Load", ("pressureAero", load ))

madeupium    = {"materialType" : "isotropic",
                "youngModulus" : 72.0E9 ,
                "poissonRatio": 0.33,
                "density" : 2.8E3}
myProblem.analysis["mystran"].setAnalysisVal("Material", ("Madeupium", madeupium))

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

myProblem.analysis["mystran"].setAnalysisVal("Property", [("Skin", skin),
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

    ####### SU2 ###########################
    print ("\nRunning PreAnalysis ......", "su2")
    myProblem.analysis["su2"].preAnalysis()

    #------------------------------
    print ("\n\nRunning SU2......")
    currentDirectory = os.getcwd() # Get our current working directory

    os.chdir(myProblem.analysis["su2"].analysisDir) # Move into test directory

    # Run SU2 mesh deformation
    if iter > 0:
        # Work around python 3 bug in su2Deform function
        if sys.version_info[0] < 3:
            su2Deform(projectName + ".cfg", args.numberProc)
        else:
            os.system("SU2_DEF " + projectName + ".cfg")

    # Run SU2 solver
    su2Run(projectName + ".cfg", args.numberProc)

    shutil.copy("surface_flow_" + projectName + ".dat", "surface_flow_" + projectName + "_Iteration_" + str(iter) + ".dat")
    os.chdir(currentDirectory) # Move back to top directory
    #------------------------------

    print ("\nRunning PostAnalysis ......", "su2")
    myProblem.analysis["su2"].postAnalysis()
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
