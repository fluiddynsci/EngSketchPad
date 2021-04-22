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
filename = "avlPlaneVanilla.csm"
print ("\n==> Loading geometry from file \""+filename+"\"...")
myProblem.loadCAPS(filename)

# Load avl aim
print ("\n==> Loading avlAIM")
avl = myProblem.loadAIM(aim = "avlAIM",
                        analysisDir = "workDir_avl")

print ("\n==> Setting analysis values")
avl.setAnalysisVal("Alpha", 1.0)

# Set meshing parameters for each surface
wing = {"numChord"     : 4,
        "numSpanTotal" : 24}

htail = {"numChord"     : 4,
         "numSpanTotal" : 16}

vtail = {"numChord"     : 4,
         "numSpanTotal" : 10}

# Associate the surface parameters with capsGroups defined on the geometry
avl.setAnalysisVal("AVL_Surface", [("Wing" , wing ),
                                   ("Htail", htail),
                                   ("Vtail", vtail)])

# Run AIM pre-analysis
print ("\n==> Running preAnalysis")
avl.preAnalysis()

####### Run avl ####################
print ("\n\n==> Running avl......")

currentDirectory = os.getcwd() # Get current working directory
os.chdir(avl.analysisDir)      # Move into test directory

# Run avl via system call
os.system("avl caps < avlInput.txt > avlOutput.txt");

os.chdir(currentDirectory)     # Move back to top directory
######################################

# Run AIM post-analysis
print ("\n==> Running postAnalysis")
avl.postAnalysis()

# Print some output values
print ("--> CLtot =", avl.getAnalysisOutVal("CLtot"))
print ("--> CDtot =", avl.getAnalysisOutVal("CDtot"))
print ("--> CDind =", avl.getAnalysisOutVal("CDind"))
print ("--> e     =", avl.getAnalysisOutVal("e"))
