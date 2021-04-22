#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = "f118-A.csm"
print ('\n==> Loading geometry from file "'+filename+'"...')
f118 = myProblem.loadCAPS(filename)

# Set wide fuselage
f118.setGeometryVal("fuse:width", 60)

# Grow the htail:area
htail_area = f118.getGeometryVal("htail:area")
f118.setGeometryVal("htail:area", htail_area*2)

print ("old htail:area = ", htail_area)
print ("new htail:area = ", f118.getGeometryVal("htail:area"))

# Build and save geometry
print ('\n==> Bulding and saving geometry...')
f118.saveGeometry("f118_3_Save_Wide.egads")


# Build the Canard variant

# Reset the fuselage
f118.setGeometryVal("fuse:width", 20)

htail_area = f118.getGeometryVal("htail:area")
wing_area  = f118.getGeometryVal("wing:area")

# Swap wing and htail area
f118.setGeometryVal("htail:area", wing_area)
f118.setGeometryVal("wing:area", htail_area/2)

# Rebuild and save geometry
print ('\n==> Bulding and saving geometry...')
f118.saveGeometry("f118_3_Save_Canard.egads")
