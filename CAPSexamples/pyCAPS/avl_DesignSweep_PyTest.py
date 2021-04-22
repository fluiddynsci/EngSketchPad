from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module
import os

# Import argparse
import argparse
# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'AVL Pytest Design Sweep Example',
                                 prog = 'avl_DesignSweep_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot data")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "AVLAnalysisSweep")

# Initialize capsProblem object
myProblem = capsProblem()

# Load CSM file
geometryScript = os.path.join("..","csmData","avlWingTail.csm")
myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

machNumber = [0.05*i for i in range(1, 20)] # Build Mach sweep with a list comprehension

sweepDesPmtr = [5, 10, 20] # Geometry design paramters

Cd = list() # Create a blank list
Cl = list()

# Loop through sweep design parametrs
for ii in range(len(sweepDesPmtr)):

    # Set new geometry design parameter
    myProblem.geometry.setGeometryVal("sweep", sweepDesPmtr[ii])

    for i in range(len(machNumber)):
        analysisID = "avlSweep_" +str(sweepDesPmtr[ii]) + "_Ma_"+ str(machNumber[i])

        myProblem.loadAIM(aim = "avlAIM", altName = analysisID, analysisDir = os.path.join(workDir,analysisID))

        # Set Mach number
        myProblem.analysis[analysisID].setAnalysisVal("Mach", machNumber[i])

        # Set Surface information
        wing = {"groupName"    : "Wing",
                "numChord"     : 8,
                "spaceChord"   : 1.0,
                "numSpanTotal" : 24,
                "spaceSpan"    : 1.0}

        tail = {"numChord"     : 8,
                "spaceChord"   : 1.0,
                "numSpanTotal" : 12,
                "spaceSpan"    : 1.0}

        myProblem.analysis[analysisID].setAnalysisVal("AVL_Surface", [("Wing", wing),
                                                                      ("Vertical_Tail", tail)])

        # Run AIM pre-analysis
        myProblem.analysis[analysisID].preAnalysis()

        # Run AVL
        print (" Running AVL (sweep angle = " + str(sweepDesPmtr[ii])+ ") at Mach number = " + str(machNumber[i]) +"!")
        currentDirectory = os.getcwd() # Get our current working directory

        os.chdir(myProblem.analysis[analysisID].analysisDir) # Move into test directory
        os.system("avl caps < avlInput.txt > avlOutput.txt");

        os.chdir(currentDirectory) # Move back to working directory

        # Run AIM post-analysis
        myProblem.analysis[analysisID].postAnalysis()

        # Get Drag coefficient
        Cd.append(myProblem.analysis[analysisID].getAnalysisOutVal("CDtot"))

        # Get Lift coefficient
        Cl.append(myProblem.analysis[analysisID].getAnalysisOutVal("CLtot"))

        # Print results
        print ("Cd = " + str(Cd[i]))
        print ("Cl = " + str(Cl[i]))

# Close CAPS
myProblem.closeCAPS()

if (args.noPlotData == False):
    # Import pyplot module
    try:
        from matplotlib import pyplot as plt
    except:
        print ("Unable to import matplotlib.pyplot module. Drag polar will not be plotted")
        plt = None

    # Plot data
    if plt is not None:

        # Plot first sweep angle
        plt.plot(Cd[1:len(machNumber)],
                 Cl[1:len(machNumber)],
                 'o--',
                 label = "Sweep Angle = " + str(sweepDesPmtr[0]) + "$^o$")

        # Plot second sweep angle
        plt.plot(Cd[len(machNumber)+1:2*len(machNumber)],
                 Cl[len(machNumber)+1:2*len(machNumber)],
                 's-',
                 label = "Sweep Angle = " + str(sweepDesPmtr[1]) + "$^o$")

        # Plot third sweep angle
        plt.plot(Cd[2*len(machNumber)+1:3*len(machNumber)],
                 Cl[2*len(machNumber)+1:3*len(machNumber)],
                 'x-.',
                 label = "Sweep Angle = " + str(sweepDesPmtr[2]) + "$^o$")
        plt.legend(loc = "lower right")
        plt.title("Drag Polar for Various Sweep Angles\n (window must be closed to terminate Python script)")
        plt.xlabel("$C_D$")
        plt.ylabel("$C_L$")
        plt.show()

# Check assertation
print( "Cl[0] = ", Cl[0] )
print( "Cd[0] = ", Cd[0] )
assert abs(Cl[0]-0.20094) <= 1E-4 and abs(Cd[0]-0.00257) <= 1E-4
