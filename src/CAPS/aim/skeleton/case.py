from __future__ import print_function

from pyCAPS import capsProblem

# Instantiate our CAPS problem "myProblem" 
print("Initiating capsProblem")
myProblem = capsProblem()

print("Loading file into our capsProblem")
myGeometry = myProblem.loadCAPS("case.csm")

# Load our skeletal aim 
skel = myProblem.loadAIM(aim = "skeletonAIM", analysisDir = ".",   
                         capsIntent = "CFD")

##[analysisValueSetandGet]
# Get current value of our first input
value = skel.getAnalysisVal("skeletonAIMin")
print("Default skeletonAIMin =", value)
skel.setAnalysisVal("skeletonAIMin", 6.0)
value = skel.getAnalysisVal("skeletonAIMin")
print("Current skeletonAIMin =", value)

##[analysisPreAndPost]
# Execute pre-Analysis
skel.preAnalysis()

# Run AIM - os.system call, python interface, etc.....

# Execute post-Analysis (notify CAPS that analysis is done)
skel.postAnalysis()

# Get an output
value = skel.getAnalysisOutVal("skeletonAIMout")
print("Computed skeletonAIMout =", value)
##[analysisPreAndPost]

# Close our problems
print("Closing our problem")
myProblem.closeCAPS()
