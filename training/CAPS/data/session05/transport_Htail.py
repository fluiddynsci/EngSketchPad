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
transport = myProblem.loadCAPS(filename, verbosity=0)

# loop over some horizonal tail volume coefficients
# and retrieve the tail area by solving the equation
# htail:vc = htail:area * htail:L / wing:area * wing:mac
for htail_vc in [0.1, 0.45, 0.7]:
    # set the htail:vc
    transport.setGeometryVal("htail:vc", htail_vc)

    # retrieve the horizontal tail area
    htail_area = transport.getGeometryOutVal("htail:area")
    print ()
    print ("--> htail:vc = ", htail_vc, " gives htail:area = ", htail_area)
    print ()

    # view the geometry with the capsViewer
    transport.viewGeometry()
