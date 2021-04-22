#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = "f118-B.csm"
print ("\n==> Loading geometry from file \""+filename+"\"...")
f118 = myProblem.loadCAPS(filename, verbosity=0)

# Show all bodies on the stack
print("==> Geometry on the stack")
f118.viewGeometry()
#------------------------------------------------------------------------------#

#------------------------------------------------------------------------------#
# capsAIM == $masstranAIM
masstranAll = myProblem.loadAIM(aim = "masstranAIM", 
                                analysisDir="masstranALL", altName="All")

# The AIM instance is also available in the capsProblem.analysis dict
assert(masstranAll == myProblem.analysis["All"])

# Show the geometry used by the AIM
print("==> Geometry used by masstranAll instance with no capsIntent")
masstranAll.viewGeometry()
#------------------------------------------------------------------------------#


#------------------------------------------------------------------------------#
# capsAIM == $masstranAIM and capsIntent == $wing
myProblem.loadAIM(aim = "masstranAIM", capsIntent="wing", 
                  analysisDir="masstranWing", altName="Wing")

# Show the geometry used by the AIM
print("==> Geometry used by Wing instance with capsIntent='wing'")
myProblem.analysis["Wing"].viewGeometry()
#------------------------------------------------------------------------------#


#------------------------------------------------------------------------------#
# capsAIM == $masstranAIM and capsIntent == $tail
masstranTail = myProblem.loadAIM(aim = "masstranAIM", capsIntent="tail", 
                                 analysisDir="masstranTail", altName="Tail")

# Show the geometry used by the AIM
print("==> Geometry used by Tail instance with capsIntent='tail'")
masstranTail.viewGeometry()
#------------------------------------------------------------------------------#


#------------------------------------------------------------------------------#
# capsAIM == $masstranAIM and (capsIntent == $wing or capsIntent == $fuse)
myProblem.loadAIM(aim = "masstranAIM", capsIntent=["wing","fuse"], 
                  analysisDir="masstranWingFuse", altName="WingFuse")

# Show the geometry used by the AIM
print("==> Geometry used by WingFuse instance with capsIntent=['wing','fuse']")
myProblem.analysis["WingFuse"].viewGeometry()
#------------------------------------------------------------------------------#


# Close CAPS
myProblem.closeCAPS()
