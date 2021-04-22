#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import OS
import os

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = os.path.join("..","..","data","session02","f118-B.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
f118 = myProblem.loadCAPS(filename, verbosity=0)

#------------------------------------------------------------------------------#
# capsAIM == $masstranAIM and (capsIntent == $wing or capsIntent == $fuse or  capsIntent == $htail)
masstran = myProblem.loadAIM(aim = "masstranAIM", capsIntent=["wing","fuse", "htail"],
                             analysisDir="masstranWingTailFuse")

# Show the geometry used by the AIM
print("==> Geometry with capsIntent=['wing','fuse', 'htail']")
masstran.viewGeometry()
#------------------------------------------------------------------------------#

# Close CAPS
myProblem.closeCAPS()
