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

# Enable lifting surfaces and controls
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 0)
transport.setGeometryVal("COMP:Htail"  , 1)
transport.setGeometryVal("COMP:Vtail"  , 1)
transport.setGeometryVal("COMP:Control", 1)


# Load AVL AIM
print ("\n==> Loading AVL aim...")
avl = myProblem.loadAIM(aim         = "avlAIM",
                        analysisDir = "workDir_avl_3_TransportControl")

# Check the geometry
#avl.viewGeometry()

# Set analysis specific variables
avl.setAnalysisVal("Mach",   0.5)
avl.setAnalysisVal("Alpha", 10.0)
avl.setAnalysisVal("Beta",   0.0)

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

# Set up control surface deflections based on the information in the csm file
controls = []

hinge = transport.getGeometryVal("wing:hinge")
for i in range(len(hinge)):
    controls.append(("WingControl_"+str(int(hinge[i][8])), {"deflectionAngle": hinge[i][0]}))

hinge = transport.getGeometryVal("htail:hinge")
for i in range(len(hinge)):
    controls.append(("HtailControl_"+str(int(hinge[i][8])), {"deflectionAngle": hinge[i][0]}))

hinge = transport.getGeometryVal("vtail:hinge")
controls.append(("VtailControl_"+str(int(hinge[8])), {"deflectionAngle": hinge[0]}))

avl.setAnalysisVal("AVL_Control", controls)

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

# Print CL derivatives
print ("\n==> Some stability derivatives")
print ("--> CLa  =", avl.getAnalysisOutVal("CLa") ) # - Alpha
print ("--> CLb  =", avl.getAnalysisOutVal("CLb") ) # - Beta
print ("--> CLp' =", avl.getAnalysisOutVal("CLp'")) # - Roll rate
print ("--> CLq' =", avl.getAnalysisOutVal("CLq'")) # - Pitch rate
print ("--> CLr' =", avl.getAnalysisOutVal("CLr'")) # - Yaw rate

# Get neutral point, CG and MAC
Xnp = avl.getAnalysisOutVal("Xnp")
Xcg = avl.getAnalysisOutVal("Xcg")
MAC = transport.getGeometryOutVal("wing:mac")

StaticMargin = (Xnp - Xcg)/MAC
print ("--> Xcg = ", Xcg)
print ("--> Xnp = ", Xnp)
print ("--> StaticMargin = ", StaticMargin)

ControlStability = avl.getAnalysisOutVal("ControlStability")
print("-->", ControlStability[0])
print("-->", ControlStability[1])
print("-->", ControlStability[2])
print("-->", ControlStability[3])
