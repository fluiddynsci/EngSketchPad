from __future__ import print_function

# Import pyCAPS class file
import pyCAPS

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AFLR4 Tetgen Region Pytest Example',
                                 prog = 'aflr4_tetgen_Region_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Working directory
workDir = os.path.join(str(args.workDir[0]), "AFLR4TetgenRegionTest")

# Initialize capsProblem object
problem = pyCAPS.capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","regions.csm")
geometry = problem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Use egads for surface tessellation
surface = problem.loadAIM(aim='aflr4AIM', altName='S')

# Set maximum and minimum edge lengths relative to capsMeshLength
surface.setAnalysisVal("max_scale", 0.2)
surface.setAnalysisVal("min_scale", 0.01)

# Generate the surface mesh
surface.preAnalysis()
surface.postAnalysis()

# Load TetGen aim with the surface mesh as the parent
volume = problem.loadAIM(aim='tetgenAIM', analysisDir= workDir, altName='V', parents=['S'])

# Set the volume analysis values
volume.setAnalysisVal('Proj_Name', 'volume')
volume.setAnalysisVal('Mesh_Format', 'VTK')

# Sepecify a point in each region with the different ID's
regions = [ ('a', { 'id' : 10, 'seed' : [0.0, 0.0, -1.0] }),
            ('b', { 'id' : 20, 'seed' : [0.0, 0.0,  1.0] }) ]
volume.setAnalysisVal('Regions', regions)

# Generate the volume mesh
volume.preAnalysis()
volume.postAnalysis()

# Close CAPS
problem.closeCAPS()
