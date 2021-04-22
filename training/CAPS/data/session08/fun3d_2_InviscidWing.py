#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os module
import os

# f90nml is used to write fun3d inputs not available in the aim
import f90nml

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
filename = os.path.join("..","ESP","transport.csm")
transport = myProblem.loadCAPS(filename)

# Change to Inviscid CFD view
transport.setGeometryVal("VIEW:Concept"    , 0)
transport.setGeometryVal("VIEW:CFDInviscid", 1)

# Enable just wing
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 0)
transport.setGeometryVal("COMP:Htail"  , 0)
transport.setGeometryVal("COMP:Vtail"  , 0)
transport.setGeometryVal("COMP:Pod"    , 0)
transport.setGeometryVal("COMP:Control", 0)

#------------------------------------------------------------------------------#

# Load aflr4 AIM
aflr4 = myProblem.loadAIM(aim         = "aflr4AIM",
                          altName     = "aflr4",
                          analysisDir = "workDir_AFLR4")

# Farfield growth factor
aflr4.setAnalysisVal("ff_cdfr", 1.4)

# Scaling factor to compute AFLR4 'ref_len' parameter
aflr4.setAnalysisVal("Mesh_Length_Factor", 5)

# Edge mesh spacing discontinuity scaled interpolant and farfield meshing BC
aflr4.setAnalysisVal("Mesh_Sizing", [("Wing"    , {"edgeWeight":1.0}),
                                     ("Farfield", {"bcType":"Farfield"})])

# Run AIM pre-/post-analysis
aflr4.preAnalysis()
aflr4.postAnalysis()

#------------------------------------------------------------------------------#

# Load AFLR3 AIM - child of AFLR4 AIM
aflr3 = myProblem.loadAIM(aim         = "aflr3AIM",
                          analysisDir = "workDir_AFLR3",
                          parents     = aflr4.aimName)

# Run AIM pre-/post-analysis
aflr3.preAnalysis()
aflr3.postAnalysis()

#------------------------------------------------------------------------------#

# Load FUN3D AIM - child of AFLR3 AIM
fun3d = myProblem.loadAIM(aim         = "fun3dAIM",
                          altName     = "fun3d",
                          analysisDir = "workDir_FUN3D_2_InviscidWing",
                          parents     = aflr3.aimName)

# Set project name. Files written to analysisDir will have this name
projectName = "inviscidWing"
fun3d.setAnalysisVal("Proj_Name", projectName)

fun3d.setAnalysisVal("Alpha", 1.0)                   # AoA
fun3d.setAnalysisVal("Mach", 0.5)                    # Mach number
fun3d.setAnalysisVal("Equation_Type","Compressible") # Equation type
fun3d.setAnalysisVal("Num_Iter",5)                   # Number of iterations

# Set boundary conditions via capsGroup
inviscidBC = {"bcType" : "Inviscid"}
fun3d.setAnalysisVal("Boundary_Condition", [("Wing", inviscidBC),
                                            ("Farfield","farfield")])

# Use python to add inputs to fun3d.nml file
fun3d.setAnalysisVal("Use_Python_NML", True)

# Write boundary output variables to the fun3d.nml file directly
fun3dnml = f90nml.Namelist()
fun3dnml['boundary_output_variables'] = f90nml.Namelist()
fun3dnml['boundary_output_variables']['mach'] = True
fun3dnml['boundary_output_variables']['cp'] = True
fun3dnml['boundary_output_variables']['average_velocity'] = True

fun3dnml.write(os.path.join(fun3d.analysisDir,"fun3d.nml"), force=True)

# Run AIM pre-analysis
fun3d.preAnalysis()

####### Run FUN3D #####################
print ("\n==> Running FUN3D......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(fun3d.analysisDir) # Move into test directory

# Run fun3d via system call
os.system("nodet_mpi --animation_freq -1 --write_aero_loads_to_file > Info.out")

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
fun3d.postAnalysis()

# Get force results
print ("\n==> Total Forces and Moments")
# Get Lift and Drag coefficients
print ("--> Cl = ", fun3d.getAnalysisOutVal("CLtot"), \
           "Cd = ", fun3d.getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print ("--> Cmx = ", fun3d.getAnalysisOutVal("CMXtot"), \
           "Cmy = ", fun3d.getAnalysisOutVal("CMYtot"), \
           "Cmz = ", fun3d.getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print ("--> Cx = ", fun3d.getAnalysisOutVal("CXtot"), \
           "Cy = ", fun3d.getAnalysisOutVal("CYtot"), \
           "Cz = ", fun3d.getAnalysisOutVal("CZtot"))
