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

# Load CSM file
transport = myProblem.loadCAPS(os.path.join("..","ESP","transport.csm"), verbosity = 0)

#------------------------------------------------------------------------------#

# Change to Inviscid CFD view
transport.setGeometryVal("VIEW:Concept"    , 0)
transport.setGeometryVal("VIEW:CFDInviscid", 1)
transport.setGeometryVal("VIEW:CFDViscous" , 0)

# Enable just wing
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 0)
transport.setGeometryVal("COMP:Htail"  , 0)
transport.setGeometryVal("COMP:Vtail"  , 0)
transport.setGeometryVal("COMP:Pod"    , 0)
transport.setGeometryVal("COMP:Control", 0)

# Save egads file of the geometry
print("==> Generating CFDInviscid_Wing")
transport.saveGeometry("CFDInviscid_Wing.egads")

# Enable wing+pod
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 0)
transport.setGeometryVal("COMP:Htail"  , 0)
transport.setGeometryVal("COMP:Vtail"  , 0)
transport.setGeometryVal("COMP:Pod"    , 1)
transport.setGeometryVal("COMP:Control", 0)

# Save egads file of the geometry
print("==> Generating CFDInviscid_WingPod")
transport.saveGeometry("CFDInviscid_WingPod.egads")

# Enable transport
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 1)
transport.setGeometryVal("COMP:Htail"  , 1)
transport.setGeometryVal("COMP:Vtail"  , 1)
transport.setGeometryVal("COMP:Pod"    , 1)
transport.setGeometryVal("COMP:Control", 1)

# Save egads file of the geometry
print("==> Generating CFDInviscid_Transport")
transport.saveGeometry("CFDInviscid_Transport.egads")

#------------------------------------------------------------------------------#

# Change to Viscous CFD view
transport.setGeometryVal("VIEW:Concept"    , 0)
transport.setGeometryVal("VIEW:CFDInviscid", 0)
transport.setGeometryVal("VIEW:CFDViscous" , 1)

# Enable just wing
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 0)
transport.setGeometryVal("COMP:Htail"  , 0)
transport.setGeometryVal("COMP:Vtail"  , 0)
transport.setGeometryVal("COMP:Pylon"  , 0)
transport.setGeometryVal("COMP:Pod"    , 0)
transport.setGeometryVal("COMP:Control", 0)

# Save egads file of the geometry
print("==> Generating CFDViscous_Wing")
transport.saveGeometry("CFDViscous_Wing.egads")

# Enable wing+pod
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 0)
transport.setGeometryVal("COMP:Htail"  , 0)
transport.setGeometryVal("COMP:Vtail"  , 0)
transport.setGeometryVal("COMP:Pylon"  , 1)
transport.setGeometryVal("COMP:Pod"    , 1)
transport.setGeometryVal("COMP:Control", 0)

# Save egads file of the geometry
print("==> Generating CFDViscous_WingPod")
transport.saveGeometry("CFDViscous_WingPod.egads")

# Enable transport
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 1)
transport.setGeometryVal("COMP:Htail"  , 1)
transport.setGeometryVal("COMP:Vtail"  , 1)
transport.setGeometryVal("COMP:Pylon"  , 1)
transport.setGeometryVal("COMP:Pod"    , 1)
transport.setGeometryVal("COMP:Control", 1)

# Save egads file of the geometry
print("==> Generating CFDViscous_Transport")
transport.saveGeometry("CFDViscous_Transport.egads")
