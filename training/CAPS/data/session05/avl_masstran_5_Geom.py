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

# Change to VLM view and OmlStructure
transport.setGeometryVal("VIEW:Concept",      0)
transport.setGeometryVal("VIEW:VLM",          1)
transport.setGeometryVal("VIEW:OmlStructure", 1)

# Enable fuselage and lifting surfaces
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 1)
transport.setGeometryVal("COMP:Htail"  , 1)
transport.setGeometryVal("COMP:Vtail"  , 1)
transport.setGeometryVal("COMP:Control", 0)

avl = myProblem.loadAIM(aim = "avlAIM",
                        analysisDir = "workDir_avl_5_Geom")

print ("AVL geometry")
avl.viewGeometry()

masstran = myProblem.loadAIM(aim = "masstranAIM",
                             analysisDir = "workDir_masstran_5_Geom")

print ("Masstran geometry")
masstran.viewGeometry()
