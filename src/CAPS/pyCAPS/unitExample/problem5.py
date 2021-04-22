#Use case set verbosity of the problem
# Import pyCAPS module (Linux and OSx = pyCAPS.so file; Windows = pyCAPS.pyd file) 
import pyCAPS

# Instantiate our CAPS problem "myProblem" 
print("Initiating capsProblem")
myProblem = pyCAPS.capsProblem()

# Load a *.csm file "./csmData/cfdMultiBody.csm" into our newly created problem. The 
# project name "basicTest" may be optionally set here;  if no argument is provided
# the CAPS file provided is used as the project name. 
print("Loading file into our capsProblem")
myGeometry = myProblem.loadCAPS("./csmData/cfdMultiBody.csm", "basicTest", verbosity="debug")


# Change verbosity to minimal - 0 (integer value) 
myProblem.setVerbosity("minimal")

# Change verbosity to standard - 1 (integer value)
myProblem.setVerbosity("standard")

# Change verbosity to back to minimal using integer value - 0
myProblem.setVerbosity(0)

# Change verbosity to back to debug using integer value - 2
myProblem.setVerbosity(2)

# Give wrong value 
myProblem.setVerbosity(10)


# Close our problems
print("Closing our problem")
myProblem.closeCAPS()
