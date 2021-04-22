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

# Build and print all available output parameters
print ("--> wing:wet    =", f118.getGeometryOutVal("wing:wet"    ) )
print ("--> wing:volume =", f118.getGeometryOutVal("wing:volume" ) )
print ()
print ("--> htail:wet    =", f118.getGeometryOutVal("htail:wet"   ) )
print ("--> htail:volume =", f118.getGeometryOutVal("htail:volume") )
print ()
print ("--> vtail:wet    =", f118.getGeometryOutVal("vtail:wet"   ) )
print ("--> vtail:volume =", f118.getGeometryOutVal("vtail:volume") )
print ()
# Accessing OUTPMTR that has not been set
print ("--> fuse:wet    =", f118.getGeometryOutVal("fuse:wet"    ) )
print ("--> fuse:volume =" , f118.getGeometryOutVal("fuse:volume" ) )
print ()
# Attempt to get a SET value not defined as OUTPMTR
print ("--> wing:span   =", f118.getGeometryOutVal("wing:span"   ) )


# Close CAPS
myProblem.closeCAPS()
