from __future__ import print_function

# Import myProblem class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse
import argparse
# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AVL Contral Surface Pytest Example',
                                 prog = 'avl_ControlSurf_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "." + os.sep, nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "AVLControlAnalsyisTest")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","avlWingTail.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Change a design parameter - area in the geometry
myProblem.geometry.setGeometryVal("area", 10.0)

# Load desired aim
print ("Loading AIM")
myAnalysis = myProblem.loadAIM(aim = "avlAIM", analysisDir = workDir)

print ("Setting design parameters")

# Set new Mach/Alt parameters
myAnalysis.setAnalysisVal("Mach", 0.5)
myAnalysis.setAnalysisVal("Alpha", 1.0)
myAnalysis.setAnalysisVal("Beta", 0.0)
myAnalysis.setAnalysisVal("RollRate", 0.0)
myAnalysis.setAnalysisVal("PitchRate", 0.0)
myAnalysis.setAnalysisVal("YawRate", 0.0)

wing = {"groupName"    : "Wing",
        "numChord"     : 8,
        "spaceChord"   : 1.0,
        "numSpanTotal" : 24,
        "spaceSpan"    : 1.0}

tail = {"numChord"     : 8,
        "spaceChord"   : 1.0,
        "numSpanTotal" : 12,
        "spaceSpan"    : 1.0}

myAnalysis.setAnalysisVal("AVL_Surface", [("Wing", wing), ("Vertical_Tail", tail)])


# Set control surface parameters
flapLE = {"controlGain"    : 0.5,
          "deflectionAngle" : 25.0}

flapTE = {"controlGain"    : 1.0,
          "deflectionAngle" : 15.0}


myAnalysis.setAnalysisVal("AVL_Control", [("WingRightLE", flapLE),
                                          ("WingRightTE", flapTE),
                                         #("WingLeftLE", flapLE), # Don't change anything
                                         #("WingLeftTE", flapTE)
                                          ("Tail"  , flapTE)])

# Run AIM pre-analysis
myAnalysis.preAnalysis()

# Run AVL
print ("Running AVL")
currentDirectory = os.getcwd() # Get our current working directory

os.chdir(myAnalysis.analysisDir) # Move into test directory
os.system("avl caps < avlInput.txt > avlOutput.txt");

os.chdir(currentDirectory) # Move back to working directory

# Run AIM post-analysis
myAnalysis.postAnalysis()

# Get Output Data from AVL
print ("Alpha  " + str(myAnalysis.getAnalysisOutVal("Alpha")))
print ("Beta   " + str(myAnalysis.getAnalysisOutVal("Beta")))
print ("Mach   " + str(myAnalysis.getAnalysisOutVal("Mach")))
print ("pb/2V  " + str(myAnalysis.getAnalysisOutVal("pb/2V")))
print ("qc/2V  " + str(myAnalysis.getAnalysisOutVal("qc/2V")))
print ("rb/2V  " + str(myAnalysis.getAnalysisOutVal("rb/2V")))
print ("p'b/2V " + str(myAnalysis.getAnalysisOutVal("p'b/2V")))
print ("r'b/2V " + str(myAnalysis.getAnalysisOutVal("r'b/2V")))
print ("CXtot  " + str(myAnalysis.getAnalysisOutVal("CXtot")))
print ("CYtot  " + str(myAnalysis.getAnalysisOutVal("CYtot")))
print ("CZtot  " + str(myAnalysis.getAnalysisOutVal("CZtot")))
print ("Cltot  " + str(myAnalysis.getAnalysisOutVal("Cltot")))
print ("Cmtot  " + str(myAnalysis.getAnalysisOutVal("Cmtot")))
print ("Cntot  " + str(myAnalysis.getAnalysisOutVal("Cntot")))
print ("Cl'tot " + str(myAnalysis.getAnalysisOutVal("Cl'tot")))
print ("Cn'tot " + str(myAnalysis.getAnalysisOutVal("Cn'tot")))
print ("CLtot  " + str(myAnalysis.getAnalysisOutVal("CLtot")))
print ("CDtot  " + str(myAnalysis.getAnalysisOutVal("CDtot")))
print ("CDvis  " + str(myAnalysis.getAnalysisOutVal("CDvis")))
print ("CLff   " + str(myAnalysis.getAnalysisOutVal("CLff")))
print ("CYff   " + str(myAnalysis.getAnalysisOutVal("CYff")))
print ("CDind  " + str(myAnalysis.getAnalysisOutVal("CDind")))
print ("CDff   " + str(myAnalysis.getAnalysisOutVal("CDff")))
print ("e      " + str(myAnalysis.getAnalysisOutVal("e")))

Cl = myAnalysis.getAnalysisOutVal("CLtot")
Cd = myAnalysis.getAnalysisOutVal("CDtot")

# Close CAPS
myProblem.closeCAPS()

# Check assertation
assert abs(Cl-0.61999) <= 1E-3 and abs(Cd-0.02984) <= 1E-3
