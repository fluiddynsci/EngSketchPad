from __future__ import print_function

# Import pyCAPS class file
import pyCAPS

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Tetgen Holes Pytest Example',
                                 prog = 'tetgen_Holes_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "TetgenHolesTest")

# Initialize capsProblem object
problem = pyCAPS.capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","regions.csm")
geometry = problem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Use egads for surface tessellation
surface = problem.loadAIM(aim='egadsTessAIM', altName='S')

# Set new EGADS body tessellation parameters
surface.setAnalysisVal("Tess_Params", [.1, 0.001, 20.0])

# Generate the surface mesh
surface.preAnalysis()
surface.postAnalysis()

# Load TetGen aim with the surface mesh as the parent
volume = problem.loadAIM(aim='tetgenAIM', analysisDir= workDir, altName='V', parents=['S'])

# Set the volume analysis values
volume.setAnalysisVal('Proj_Name', 'volume')
volume.setAnalysisVal('Mesh_Format', 'Tecplot')

# Sepecify a hole point to remove a region in the mesh
holes = [ ('a', { 'seed' : [0.0, 0.0, -1.0] })  ]
volume.setAnalysisVal('Holes', holes)

# Generate the volume mesh
volume.preAnalysis()
volume.postAnalysis()

# Close CAPS
problem.closeCAPS()
