from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import modules
import os
import sys
import shutil
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Masstran AGARD445.6 Pytest Example',
                                 prog = 'masstran_AGARD445_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ".", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noAnalysis', action='store_true', default = False, help = "Don't run analysis code")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","feaAGARD445.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Load astros aim
myAnalysis = myProblem.loadAIM(aim = "masstranAIM",
                               altName = "masstran")

# Set meshing parameters
myAnalysis.setAnalysisVal("Edge_Point_Max", 10)
myAnalysis.setAnalysisVal("Edge_Point_Min", 6)

myAnalysis.setAnalysisVal("Quad_Mesh", True)

myAnalysis.setAnalysisVal("Tess_Params", [.25,.01,15])

# Set materials
mahogany    = {"materialType"        : "orthotropic",
               "youngModulus"        : 0.457E6 ,
               "youngModulusLateral" : 0.0636E6,
               "poissonRatio"        : 0.31,
               "shearModulus"        : 0.0637E6,
               "shearModulusTrans1Z" : 0.00227E6,
               "shearModulusTrans2Z" : 0.00227E6,
               "density"             : 3.5742E-5}

myAnalysis.setAnalysisVal("Material", ("Mahogany", mahogany))

# Set properties
shell  = {"propertyType" : "Shell",
          "membraneThickness" : 0.82,
          "material"        : "mahogany",
          "bendingInertiaRatio" : 1.0, # Default - not necesssary
          "shearMembraneRatio"  : 5.0/6.0} # Default - not necesssary

myAnalysis.setAnalysisVal("Property", ("yatesPlate", shell))

# Run AIM pre-analysis
myAnalysis.preAnalysis()

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Get mass properties
print ("\nGetting results mass properties.....\n")
Area     = myAnalysis.getAnalysisOutVal("Area")
Mass     = myAnalysis.getAnalysisOutVal("Mass")
Centroid = myAnalysis.getAnalysisOutVal("Centroid")
CG       = myAnalysis.getAnalysisOutVal("CG")
Ixx      = myAnalysis.getAnalysisOutVal("Ixx")
Iyy      = myAnalysis.getAnalysisOutVal("Iyy")
Izz      = myAnalysis.getAnalysisOutVal("Izz")
Ixy      = myAnalysis.getAnalysisOutVal("Ixy")
Ixz      = myAnalysis.getAnalysisOutVal("Ixz")
Iyz      = myAnalysis.getAnalysisOutVal("Iyz")
I        = myAnalysis.getAnalysisOutVal("I_Vector")
II       = myAnalysis.getAnalysisOutVal("I_Tensor")

print("Area     ", Area)
print("Mass     ", Mass)
print("Centroid ", Centroid)
print("CG       ", CG)
print("Ixx      ", Ixx)
print("Iyy      ", Iyy)
print("Izz      ", Izz)
print("Ixy      ", Ixy)
print("Ixz      ", Ixz)
print("Iyz      ", Iyz)
print("I        ", I)
print("II       ", II)

# Close CAPS
myProblem.closeCAPS()
