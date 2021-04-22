#------------------------------------------------------------------------------#

# Allow print statement to be compatible between Python 2 and 3
from __future__ import print_function

# Import pyCAPS class
from pyCAPS import capsProblem

# Import os
import os

# Import matplotlib
from matplotlib import pyplot as plt

#------------------------------------------------------------------------------#

def run_xfoil(xfoil):
    # Run AIM pre-analysis
    print ("\n==> Running preAnalysis")
    xfoil.preAnalysis()
    
    ####### Run xfoil ####################
    print ("\n\n==> Running xFoil......")
    
    currentDirectory = os.getcwd() # Get current working directory
    os.chdir(xfoil.analysisDir)    # Move into test directory
    
    # Run xfoil via system call
    os.system("xfoil < xfoilInput.txt > Info.out"); 
    
    os.chdir(currentDirectory)     # Move back to top directory
    ######################################
    
    # Run AIM post-analysis
    print ("\n==> Running postAnalysis")
    xfoil.postAnalysis()

#------------------------------------------------------------------------------#

# Initialize capsProblem object
myProblem = capsProblem()

# Load geometry [.csm] file
filename = "naca.csm"
print ("\n==> Loading geometry from file \""+filename+"\"...")
naca = myProblem.loadCAPS(filename)

# Load xfoil aim
print ("\n==> Loading xfoilAIM")
xfoil = myProblem.loadAIM(aim = "xfoilAIM",
                          analysisDir = "workDir_xfoil_4_Plot")

print ("\n==> Setting analysis values")
# Set Mach and Reynolds number
xfoil.setAnalysisVal("Mach", 0.5 )
xfoil.setAnalysisVal("Re", 1.0E6 )

# Set list of Alpha
xfoil.setAnalysisVal("Alpha", [-4.0, -2.0, 0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0] )

# List of cambers to analyze
Cambers = [0.00, 0.01, 0.04, 0.07]

# Crete the figure with 2 axes and sent the axis labels
fig, ax = plt.subplots(1, 2, figsize=(10,4))
ax[0].set_xlabel("Alpha")
ax[0].set_ylabel("Cl")
ax[1].set_xlabel("Cd")
ax[1].set_ylabel("Cl")

for camber in Cambers:
    # Modify the camber
    naca.setGeometryVal("camber", camber)
    
    # Run xfoil
    run_xfoil(xfoil)
    
    # Retrieve and plot Alpha, Cl and Cd
    print ("\n==> Retrieve analysis results")
    Alpha = xfoil.getAnalysisOutVal("Alpha")
    Cl = xfoil.getAnalysisOutVal("CL")
    Cd = xfoil.getAnalysisOutVal("CD")

    ax[0].plot(Alpha, Cl, '-o', label = "Camber = " + str(camber))
    ax[1].plot(Cd, Cl, '-o', label = "Camber = " + str(camber))

# Show the legend on the 2nd axes
ax[1].legend()

# Show the plot
print ("\n==> Plotting analysis results (close plot window to finish Python script")
plt.show()
