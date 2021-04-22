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
filename = os.path.join("..","ESP","wing3.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
wing = myProblem.loadCAPS(filename)

#------------------------------------------------------------------------------#

# Enable the structural mode with the full skin
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:BoxStructure"    , 0)

# View the full geometry
wing.viewGeometry()

#------------------------------------------------------------------------------#

# Enable the structural mode for just the box
wing.setGeometryVal("VIEW:Concept"         , 0)
wing.setGeometryVal("VIEW:ClampedStructure", 1)
wing.setGeometryVal("VIEW:BoxStructure"    , 1)

# View the box geometry
wing.viewGeometry()

#------------------------------------------------------------------------------#
