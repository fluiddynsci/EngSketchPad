#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os
import os

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename=os.path.join("..","ESP","transport.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
transport = myProblem.loadCAPS(filename)

# Set geometry variables to enable Vortex Lattice bodies
print ("\n==> Setting Build Variables and Geometry Values...")

# Change to VLM view
transport.setGeometryVal("VIEW:Concept", 0)
transport.setGeometryVal("VIEW:VLM",     1)

# Enable lifting surfaces without controls
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 0)
transport.setGeometryVal("COMP:Htail"  , 1)
transport.setGeometryVal("COMP:Vtail"  , 1)
transport.setGeometryVal("COMP:Control", 0)


# Load AVL AIM
print ("\n==> Loading AVL aim...")
avl = myProblem.loadAIM(aim         = "avlAIM",
                        analysisDir = "workDir_avl_4_TransportEigen")

# Check the geometry
#avl.viewGeometry()

# Set analysis specific variables
avl.setAnalysisVal("Mach",  0.5)
avl.setAnalysisVal("Alpha", 1.5)
avl.setAnalysisVal("Beta",  0.0)

# Set meshing parameters for each surface
wing = {"numChord"     : 4,
        "numSpanTotal" : 80}

htail = {"numChord"     : 4,
         "numSpanTotal" : 30}

vtail = {"numChord"     : 4,
         "numSpanTotal" : 10}

# Associate the surface parameters with capsGroups defined on the geometry
avl.setAnalysisVal("AVL_Surface", [("Wing" , wing ),
                                   ("Htail", htail),
                                   ("Vtail", vtail)])

I = "massInertia"
# Inspired by the b737.mass avl example file
#                     mass                CGx  CGy CGz             Ixx     Iyy     Izz
cockpit   = {"mass":[ 3000, "lb"], "CG":[[  8,  0,  5], "ft"], I:[[0.   ,  0.   ,  0.   ], "lb*ft^2"]}
wing      = {"mass":[19420, "lb"], "CG":[[ 78,  0, -1], "ft"], I:[[8.0e6,  0.1e6,  8.1e6], "lb*ft^2"]}
fuselage  = {"mass":[33720, "lb"], "CG":[[105,  0,  2], "ft"], I:[[0.7e6, 18.9e6, 19.6e6], "lb*ft^2"]}
tailcone  = {"mass":[  310, "lb"], "CG":[[145,  0,  0], "ft"], I:[[0.   ,  0.   ,  0.   ], "lb*ft^2"]}
Htail     = {"mass":[  528, "lb"], "CG":[[160,  0,  2], "ft"], I:[[0.0e6,  0.0e6,  0.0e6], "lb*ft^2"]}
Vtail     = {"mass":[  616, "lb"], "CG":[[100,  0,  8], "ft"], I:[[0.1e6,  0.0e6,  0.1e6], "lb*ft^2"]}
Main_gear = {"mass":[ 4500, "lb"], "CG":[[ 76,  0, -4], "ft"], I:[[0.5e6,  0.0  ,  0.5e6], "lb*ft^2"]}
Nose_gear = {"mass":[ 1250, "lb"], "CG":[[ 36,  0, -5], "ft"], I:[[0.   ,  0.   ,  0.   ], "lb*ft^2"]}

avl.setAnalysisVal("MassProp", [("cockpit"  , cockpit  ),
                                ("wing"     , wing     ),
                                ("fuselage" , fuselage ),
                                ("tailcone" , tailcone ),
                                ("Htail"    , Htail    ),
                                ("Vtail"    , Vtail    ),
                                ("Main_gear", Main_gear),
                                ("Nose_gear", Nose_gear)])
avl.setAnalysisVal("Gravity" , 32.18   , units="ft/s^2"   )
avl.setAnalysisVal("Density" , 0.002378, units="slug/ft^3")
avl.setAnalysisVal("Velocity", 250.0   , units="m/s"      )

# Run AIM pre-analysis
print ("\n==> Running preAnalysis")
avl.preAnalysis()

####### Run avl ####################
print ("\n\n==> Running avl......")

currentDirectory = os.getcwd() # Get current working directory
os.chdir(avl.analysisDir)      # Move into test directory

# Run avl via system call
os.system("avl caps < avlInput.txt > avlOutput.txt");

os.chdir(currentDirectory)     # Move back to top directory
######################################

# Run AIM post-analysis
print ("\n==> Running postAnalysis")
avl.postAnalysis()

print ("\n==> Outputs")
# Get neutral point, CG and MAC
Xnp = avl.getAnalysisOutVal("Xnp")
Xcg = avl.getAnalysisOutVal("Xcg")
MAC = transport.getGeometryOutVal("wing:mac")

StaticMargin = (Xnp - Xcg)/MAC
print ("--> Xcg = ", Xcg)
print ("--> Xnp = ", Xnp)
print ("--> StaticMargin = ", StaticMargin)

# Get the eigen values
# ("case #", "Array of eigen values").
# The array of eigen values is of the form = [[real0,imaginary0],[real0,imaginary0],...]

EigenValues = avl.getAnalysisOutVal("EigenValues")
print ("--> EigenValues ", EigenValues)

# Extract just the real eigen values
EigenValues = EigenValues[1]
RealEigen = []
for i in range(len(EigenValues)):
    RealEigen.append(EigenValues[i][0])
print ()
print ("--> Real Eigen Values =", RealEigen)
