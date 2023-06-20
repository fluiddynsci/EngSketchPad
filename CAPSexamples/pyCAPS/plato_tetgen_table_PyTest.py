# Import pyCAPS module
import pyCAPS

# Import os module
import os
import argparse

# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'Plato Cyli-Box Pytest Example',
                                 prog = 'plato_tetgen_cyli_box_PyTest',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = ["." + os.sep], nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-outLevel", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Load CSM file
geometryScript = os.path.join("..","csmData","table.csm")
problem = pyCAPS.Problem(problemName = os.path.join(str(args.workDir[0]), "PlatoMultiDomainTest"),
                         capsFile=geometryScript,
                         outLevel=args.outLevel)

# Use egads for surface tessellation
surface = problem.analysis.create(aim='egadsTessAIM', name='egads')

# Set new EGADS body tessellation parameters
surface.input.Tess_Params = [.1, 0.001, 20.0]

# Load TetGen aim with the surface mesh as the parent
volume = problem.analysis.create(aim='tetgenAIM', name='tetgen')

# Link the surface mesh
volume.input["Surface_Mesh"].link(surface.output["Surface_Mesh"])

# Generate a mesh for each body
volume.input.Multiple_Mesh = 'MultiDomain'

# Set the volume analysis values
volume.input.Proj_Name = 'volume'
volume.input.Mesh_Format = 'VTK'

# Generate the volume mesh
volume.runAnalysis()

# Make the plato AIM
plato = problem.analysis.create(aim='platoAIM', name='plato')

plato.input["Mesh"].link(volume.output["Volume_Mesh"])

plato.preAnalysis()
plato.postAnalysis()
