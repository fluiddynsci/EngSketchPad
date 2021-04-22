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
filename = os.path.join("..","ESP","transport.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
transport = myProblem.loadCAPS(filename)

# Change to VLM view
transport.setGeometryVal("VIEW:Concept", 0)
transport.setGeometryVal("VIEW:VLM"    , 1)

# view the geometry with the capsViewer
print ("\n==> Viewing transport bodies...")
transport.viewGeometry()

# Load AVL AIM
print ("\n==> Loading AVL aim...")
avl = myProblem.loadAIM(aim         = "avlAIM",
                        analysisDir = "workDir_avl_1_TransportGeom")

# view avl bodies with the capsViewer
print ("\n==> Viewing avl bodies...")
avl.viewGeometry()
