#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
# loadCAPS returns a class allowing interaction with bodies on the stack
# The geometry is not built with loadCAPS
filename = "f118-A.csm"
print ('\n==> Loading geometry from file "'+filename+'"...')
f118 = myProblem.loadCAPS(filename)

# The same geometry instance is available via myProblem.geometry
assert(f118 == myProblem.geometry)

# Build and view the geometry with the capsViewer
print ('\n==> Bulding and viewing geometry...')
f118.viewGeometry()

# Close CAPS (optional)
myProblem.closeCAPS()
