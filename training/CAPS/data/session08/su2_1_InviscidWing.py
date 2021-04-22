#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os module
import os

# Import SU2 python environment
from parallel_computation import parallel_computation as su2Run

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

# Load SU2 AIM - child of AFLR3 AIM
su2 = myProblem.loadAIM(aim         = "su2AIM",
                        altName     = "su2",
                        analysisDir = "workDir_SU2_1_InviscidWing",
                        parents     = aflr3.aimName)

# Set project name. Files written to analysisDir will have this name
projectName = "inviscidWing"
su2.setAnalysisVal("Proj_Name", projectName)

su2.setAnalysisVal("Alpha", 1.0)                   # AoA
su2.setAnalysisVal("Mach", 0.5)                    # Mach number
su2.setAnalysisVal("Equation_Type","Compressible") # Equation type
su2.setAnalysisVal("Num_Iter",5)                   # Number of iterations

# Set boundary conditions via capsGroup
inviscidBC = {"bcType" : "Inviscid"}
su2.setAnalysisVal("Boundary_Condition", [("Wing", inviscidBC),
                                          ("Farfield","farfield")])

# Specifcy the boundares used to compute forces
su2.setAnalysisVal("Surface_Monitor", ["Wing"])

# Set SU2 Version
su2.setAnalysisVal("SU2_Version","Falcon")

# Run AIM pre-analysis
su2.preAnalysis()

####### Run SU2 #######################
print ("\n\nRunning SU2......")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(su2.analysisDir) # Move into test directory

# Run SU2 with specified number of partitions
su2Run(projectName + ".cfg", partitions = 1)

os.chdir(currentDirectory) # Move back to top directory
#######################################

# Run AIM post-analysis
su2.postAnalysis()

print ("\n==> Total Forces and Moments")
# Get Lift and Drag coefficients
print ("--> Cl = ", su2.getAnalysisOutVal("CLtot"), \
           "Cd = ", su2.getAnalysisOutVal("CDtot"))

# Get Cmx, Cmy, and Cmz coefficients
print ("--> Cmx = ", su2.getAnalysisOutVal("CMXtot"), \
           "Cmy = ", su2.getAnalysisOutVal("CMYtot"), \
           "Cmz = ", su2.getAnalysisOutVal("CMZtot"))

# Get Cx, Cy, Cz coefficients
print ("--> Cx = ", su2.getAnalysisOutVal("CXtot"), \
           "Cy = ", su2.getAnalysisOutVal("CYtot"), \
           "Cz = ", su2.getAnalysisOutVal("CZtot"))
