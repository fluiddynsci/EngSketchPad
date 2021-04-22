from __future__ import print_function

# Import pyCAPS and os module
import pyCAPS
import os

# Import argparse
import argparse
# Setup and read command line options. Please note that this isn't required for pyCAPS
parser = argparse.ArgumentParser(description = 'autoLink Pytest Link Example',
                                 prog = 'autoLink_PyTest.py',
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)

#Setup the available commandline options
parser.add_argument('-workDir', default = "./", nargs=1, type=str, help = 'Set working/run directory')
parser.add_argument('-noPlotData', action='store_true', default = False, help = "Don't plot data")
parser.add_argument("-verbosity", default = 1, type=int, choices=[0, 1, 2], help="Set output verbosity")
args = parser.parse_args()

def runAnalysis(myProblem):

    for i in myProblem.analysis:

        myAnalysis = myProblem.analysis[i]

        # Skip analysis for AVL for Mach numbers > .75
        if myProblem.value["Mach"].value > 0.75 and myAnalysis.aimName == "avlAIM":
            continue
        # Skip analysis for AWave for Mach < 1.0
        if myProblem.value["Mach"].value < 1.0 and myAnalysis.aimName == "awaveAIM":
            continue

        # Run AIM pre-analysis
        myAnalysis.preAnalysis()

        # Run the tool
        print ("Running", myAnalysis.aimName)
        currentDirectory = os.getcwd() # Get our current working directory
        os.chdir(myAnalysis.analysisDir) # Move into test directory

        os.system(myAnalysis.getAttribute("execute")) # Execute attribute defined above

        os.chdir(currentDirectory) # Move back to working directory

        # Run AIM post-analysis
        myAnalysis.postAnalysis()

def combineDrag(myProblem):

    drags = ["CDtot", "CDwave", "CDtotal"] # CDtotal = Cdform+Cdfric
    CDTotal = 0
    for i in myProblem.analysis:

        myAnalysis = myProblem.analysis[i]

        # Skip analysis for AVL for Mach numbers > .75 - We wont have any analysis
        if myProblem.value["Mach"].value > 0.75 and myAnalysis.aimName == "avlAIM":
            continue
        # Skip analysis for AWave for Mach < 1.0 - We wont have any analysis
        if myProblem.value["Mach"].value < 1.0 and myAnalysis.aimName == "awaveAIM":
            continue

        outValues = myAnalysis.getAnalysisOutVal()

        for dragType in drags:
            if dragType in outValues:
                CDTotal += outValues[dragType]

    return CDTotal

# Initialize capsProblem object
myProblem = pyCAPS.capsProblem()

# Load geometry
geometryScript = os.path.join("..","csmData","linearAero.csm")
myGeometry = myProblem.loadCAPS(geometryScript, verbosity=args.verbosity)

# Set verbosity to minimal
myProblem.setVerbosity("minimal")

# Create working directory variable
workDir = os.path.join(str(args.workDir[0]), "LinkTest_" )

# Load desired aim
fric = myProblem.loadAIM(aim = "frictionAIM",
                         analysisDir = workDir + "Friction")

awave = myProblem.loadAIM(aim = "awaveAIM",
                          analysisDir = workDir + "Awave")

avl = myProblem.loadAIM(aim = "avlAIM",
                        analysisDir = workDir + "AVL")

# Add attribute to analyses to define execution functions
fric.addAttribute("execute",  "friction frictionInput.txt frictionOutput.txt > Info.out")
awave.addAttribute("execute", "awave awaveInput.txt > Info.out")
avl.addAttribute("execute",   "avl caps < avlInput.txt > avlOutput.txt")

# Create mission parameters
mach     = myProblem.createValue("Mach", 0.1) # Mach number
alpha    = myProblem.createValue("Alpha", 0.0, "degree") # Angle of attack
altitude = myProblem.createValue("Altitude", 5000.0, "ft") # Flying altitude

machRange     = [mach.value + i*0.1 for i in range(7)]
altitudeRange = [altitude.value + i*5000 for i in range(12)]

# Autolink the mission parameters to inputs of the loaded AIMs
myProblem.autoLinkValue()

# Set geometry inputs
# despmtr   thick     0.12      frac of local chord
# despmtr   camber    0.04      frac of local chord
# despmtr   tlen      5.00      length from wing LE to Tail LE
# despmtr   toff      0.5       tail offset
#
# despmtr   area      10.0
# despmtr   aspect    6.00
# despmtr   taper     0.60
# despmtr   sweep     20.0      deg (of c/4)
#
# despmtr   washout  -5.00      deg (down at tip)
# despmtr   dihedral  4.00      deg
myGeometry.setGeometryVal("area", 7.5)

# Set additional information for AVL
avlSurfaces = []
for i in avl.getAttributeVal("capsGroup"):

     if "tail" in i.lower():
         surf = {"numChord"     : 8,
                 "spaceChord"   : 1.0,
                 "numSpanTotal" : 6,
                 "spaceSpan"    : 1.0}
     elif "wing" in i.lower():
         surf = {"numChord"     : 10,
                 "spaceChord"   : 0.5,
                 "numSpanTotal" : 40,
                 "spaceSpan"    : 1.0}
     else:
         continue # Not interested in other capsGroups

     avlSurfaces.append((i, surf))

# Add surface information to AVL
avl.setAnalysisVal("AVL_Surface", avlSurfaces)

#Create a list of lists for storage
Cd = [] # altitude index

# Loop through ranges
for alt in altitudeRange:

    Cd.append([]) # mach index

    for ma in machRange:

        # Set mission parameters - AIMs will be updated automatically due to link
        mach.value = ma
        altitude.value = alt

        # Run through the analyses
        runAnalysis(myProblem)

        Cd[-1].append(combineDrag(myProblem))

# Close CAPS
myProblem.closeCAPS()

# Print out drag values
print("Cd = ", Cd)

if (args.noPlotData == False):
    # Create a contour plot in matplotlib and numpy is available
    try:
        import matplotlib.pyplot as plt

        import numpy

        X = numpy.arange(min(machRange), max(machRange)+0.0001, machRange[1] - machRange[0])
        Y = numpy.arange(min(altitudeRange), max(altitudeRange)+0.0001, altitudeRange[1] - altitudeRange[0])
        Z = numpy.array(Cd)

        CS = plt.contourf(X, Y, Z, 30)

        plt.title('Coefficient of Drag')
        plt.xlabel('Mach number')
        plt.ylabel('Altitude (ft)')

        # Make a colorbar for the ContourSet returned by the contourf call.
        cbar = plt.colorbar(CS)
        cbar.ax.set_ylabel('Cd')

        plt.show()

    except:
        print("Unable to plot data")
