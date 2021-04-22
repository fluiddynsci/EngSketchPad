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
print ("\n==> Loading masstran aim...")
masstran = myProblem.loadAIM(aim = "masstranAIM",
                             analysisDir = "workDir_masstran_MultiMat")

# Set materials
fuseium  = { "density" : 100 } # lb/ft^3
wingium  = { "density" : 200 } # lb/ft^3
tailium  = { "density" :  50 } # lb/ft^3
masstran.setAnalysisVal("Material", [("fuseium", fuseium),
                                     ("wingium", wingium),
                                     ("tailium", tailium)])

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


masstran.setAnalysisVal("Property", [("fuseSkin", fuseshell),
                                     ("wingSkin", wingshell),
                                     ("htailSkin", tailshell),
                                     ("vtailSkin", tailshell)])

print ("\n==> Computing mass properties...")
masstran.preAnalysis()
masstran.postAnalysis()

aircraft_mass = masstran.getAnalysisOutVal("Mass")
aircraft_CG   = masstran.getAnalysisOutVal("CG")
aircraft_I    = masstran.getAnalysisOutVal("I_Vector")

aircraft_skin = {"mass":[aircraft_mass,"lb"],"CG":[aircraft_CG,"ft"],"massInertia":[aircraft_I,"lb*ft^2"]}

print ("\n==> Computed mass properties...")
print ("--> aircraft_skin = ", aircraft_skin)
