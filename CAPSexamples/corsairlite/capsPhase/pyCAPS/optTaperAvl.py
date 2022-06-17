import pyCAPS
import os
from corsairlite.optimization.solve import solve
from corsairlite.optimization import Variable, RuntimeConstraint
from corsairlite.optimization import Formulation, Constant
from corsairlite.analysis.models.model import Model
from corsairlite import units

class AVLModel(Model):
    """RuntimeConstraint Model for running AVL. Takes in a pyCAPS Problem Object"""
    def __init__(self, problemObj):
        super(AVLModel, self).__init__()
        self.description = 'This model computes the oswald efficiency given taper ratio using AVL'
        self.inputs.append( Variable("lam", 1.0, "", "Taper ratio"))
        self.outputs.append( Variable("e", 1.0, "", "Oswald"))
        self.availableDerivative = 1
        self.problemObj = problemObj

    def BlackBox(*args, **kwargs):
        args = list(args)       # convert tuple to list
        self = args.pop(0)      # pop off the self argument

        # parse inputs to BlackBox from RuntimeConstraint
        runCases, returnMode, extra = self.parseInputs(*args, **kwargs)

        # get the AVL aim
        avl = self.problemObj.analysis["avl"]

        # update geometry with current iteration taper
        lam = runCases[0]['lam'].to('').magnitude
        self.problemObj.geometry.despmtr["wing:taper"].value = lam

        # get oswald efficiency from AVL
        outputE = avl.output["e"].value
        y = outputE*units.dimensionless

        # finite difference to get d(oswald)/d(taper)
        self.problemObj.geometry.despmtr["wing:taper"].value = lam+0.01
        e_pos = avl.output["e"].value
        self.problemObj.geometry.despmtr["wing:taper"].value = lam-0.01
        e_neg= avl.output["e"].value
        deriv = (e_pos - e_neg) / 0.02
        dydx = [deriv * units.dimensionless]

        return [y, dydx]


myProblem = pyCAPS.Problem("ostrich", 
                           capsFile=os.path.join("..","csm","wing2.csm"), 
                           phaseName = "1.2", 
                           phaseStart="1.1", 
                           outLevel=0)

# Get/create the avl AIM and set inputs
if "avl" in myProblem.analysis:
    avl = myProblem.analysis["avl"]
else:
    avl = myProblem.analysis.create(aim  = "avlAIM", name = "avl")

# set the non-geometric AVL inputs
if "CLcruise" in myProblem.parameter:
    # Cruise CL from optimization of phase 1.1
    avl.input["CL"  ].value = myProblem.parameter["CLcruise"].value
else:
    avl.input["CL"  ].value = 0.95
    
avl.input["Mach"].value = 0
avl.input["Beta"].value = 0
avl.input.AVL_Surface   = {"wing" : {"numChord"     : 4,
                                     "numSpanTotal" : 24}}


# Get wing area and aspect ratio from phase 1.1 to use as constants in optimization
S1  = myProblem.geometry.despmtr["wing:area"].value
AR1 = myProblem.geometry.despmtr["wing:aspect"].value

# Constants
S  = Constant(name="S", value=S1, units="m^2", description="Wing area")
AR = Constant(name="AR", value=AR1, units="", description="Wing aspect ratio")

# Variables
lam = Variable(name="lam", guess=0.3, units="", description="Wing taper ratio")
e   = Variable(name="e",   guess=1.0, units="", description="Oswald efficiency factor")

# Objective function
objective = 1/e

# Constraints
avlModel = AVLModel(myProblem)
constraints = [RuntimeConstraint([e],['=='],[lam],avlModel)]

formulation = Formulation(objective, constraints)
formulation.solverOptions.solver = 'cvxopt'
formulation.solverOptions.solveType = 'slcp'
formulation.solverOptions.progressFilename = None
rs = solve(formulation)

print(rs.result(5))

rs_lam = rs.variables['lam'].to('').magnitude
rs_e   = rs.variables['e'].to('').magnitude

# put the best result into the CAPS Problem
myProblem.geometry.despmtr["wing:taper"].value = rs_lam
myProblem.geometry.build()

# Save optimized oswald efficiency as a CAPS value
if "oswald" in myProblem.parameter:
    myProblem.parameter["oswald"].value = rs_e
else:
    myProblem.parameter.create("oswald", rs_e)

# Close the phase
myProblem.closePhase()