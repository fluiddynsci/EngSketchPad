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
filename = os.path.join("..","..","data","ESP","transport.csm")
print ("\n==> Loading geometry from file \""+filename+"\"...")
transport = myProblem.loadCAPS(filename)

# Set geometry variables to enable Vortex Lattice bodies
print ("\n==> Setting Build Variables and Geometry Values...")

# Change to VLM view and OmlStructure
transport.setGeometryVal("VIEW:Concept",      0)
transport.setGeometryVal("VIEW:OmlStructure", 1)

# Enable fuselage and lifting surfaces
transport.setGeometryVal("COMP:Wing"   , 1)
transport.setGeometryVal("COMP:Fuse"   , 1)
transport.setGeometryVal("COMP:Htail"  , 1)
transport.setGeometryVal("COMP:Vtail"  , 1)
transport.setGeometryVal("COMP:Control", 0)

# Load masstran AIM
print ("\n==> Loading masstran aims...")
masstranFuse = myProblem.loadAIM(aim = "masstranAIM",
                                 analysisDir = "workDir_masstran_MultiAIM_fuse",
                                 altName = "masstranFuse",
                                 capsIntent = "fuse")
masstranWing = myProblem.loadAIM(aim = "masstranAIM",
                                 analysisDir = "workDir_masstran_MultiAIM_wing",
                                 altName = "masstranWing",
                                 capsIntent = "wing")
masstranTail = myProblem.loadAIM(aim = "masstranAIM",
                                 analysisDir = "workDir_masstran_MultiAIM_tail",
                                 altName = "masstranTail",
                                 capsIntent = "tail")

# Set materials
fuseium  = { "density" : 100 } # lb/ft^3
wingium  = { "density" : 200 } # lb/ft^3
tailium  = { "density" :  50 } # lb/ft^3
masstranFuse.setAnalysisVal("Material", ("fuseium", fuseium))
masstranWing.setAnalysisVal("Material", ("wingium", wingium))
masstranTail.setAnalysisVal("Material", ("tailium", tailium))

# Set property
fuseshell = {"propertyType"      : "Shell",
             "material"          : "fuseium",
             "membraneThickness" : 0.02} # ft
wingshell = {"propertyType"      : "Shell",
             "material"          : "wingium",
             "membraneThickness" : 0.03} # ft
tailshell = {"propertyType"      : "Shell",
             "material"          : "tailium",
             "membraneThickness" : 0.01} # ft


masstranFuse.setAnalysisVal("Property", ("fuseSkin", fuseshell))
masstranWing.setAnalysisVal("Property", ("wingSkin", wingshell))
masstranTail.setAnalysisVal("Property", [("htailSkin", tailshell),
                                         ("vtailSkin", tailshell)])

print ("\n==> Computing mass properties...")
masstranFuse.preAnalysis()
masstranFuse.postAnalysis()

masstranWing.preAnalysis()
masstranWing.postAnalysis()

masstranTail.preAnalysis()
masstranTail.postAnalysis()

fuse_mass = masstranFuse.getAnalysisOutVal("Mass")
fuse_CG   = masstranFuse.getAnalysisOutVal("CG")
fuse_I    = masstranFuse.getAnalysisOutVal("I_Vector")

fuse_skin = {"mass":[fuse_mass,"lb"],"CG":[fuse_CG,"ft"],"massInertia":[fuse_I,"lb*ft^2"]}

wing_mass = masstranWing.getAnalysisOutVal("Mass")
wing_CG   = masstranWing.getAnalysisOutVal("CG")
wing_I    = masstranWing.getAnalysisOutVal("I_Vector")

wing_skin = {"mass":[wing_mass,"lb"],"CG":[wing_CG,"ft"],"massInertia":[wing_I,"lb*ft^2"]}

tail_mass = masstranTail.getAnalysisOutVal("Mass")
tail_CG   = masstranTail.getAnalysisOutVal("CG")
tail_I    = masstranTail.getAnalysisOutVal("I_Vector")

tail_skin = {"mass":[tail_mass,"lb"],"CG":[tail_CG,"ft"],"massInertia":[tail_I,"lb*ft^2"]}

print ("\n==> Computed mass properties...")
print ("--> fuse_skin = ", fuse_skin)
print ("--> wing_skin = ", wing_skin)
print ("--> tail_skin = ", tail_skin)
