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

# Modify the fuselage
f118.setGeometryVal("fuse:width" , 1.5*f118.getGeometryVal("fuse:width"))
f118.setGeometryVal("fuse:height", 1.2*f118.getGeometryVal("fuse:height"))

# Modify the wing
f118.setGeometryVal("wing:xroot" , 0.9*f118.getGeometryVal("wing:xroot"))
f118.setGeometryVal("wing:aspect", 1.6*f118.getGeometryVal("wing:aspect"))

# Modify the htail
f118.setGeometryVal("htail:area" ,   3*f118.getGeometryVal("htail:area"))
f118.setGeometryVal("htail:thick", 1.7*f118.getGeometryVal("htail:thick"))

# Modify the vtail
f118.setGeometryVal("vtail:area"  , 1.2*f118.getGeometryVal("vtail:area"))
f118.setGeometryVal("vtail:aspect", 0.5*f118.getGeometryVal("vtail:aspect"))

# Build and view the geometry with the capsViewer
print ('\n==> Bulding and viewing geometry...')
f118.viewGeometry()

# Close CAPS
myProblem.closeCAPS()
