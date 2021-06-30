# Import other need modules
from __future__ import print_function

import unittest
import os
import glob
import shutil

# Import pyCAPS class file
import pyCAPS
from pyCAPS import Problem


# Geometry is verified with avl by plotting the camber using the commands:
#
# avl caps
# oper
# g
# ca

class TestavlAIM(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        
        # Create working directory variable
        cls.probemName = "workDir_avlAnalysisTest"

        cls.cleanUp()

        # Initialize Problem object
        cls.myProblem = Problem(cls.probemName, capsFile="../csmData/avlSections.csm", outLevel=0)
    
    @classmethod
    def tearDownClass(cls):
        del cls.myProblem
        cls.cleanUp()

    @classmethod
    def cleanUp(cls):
        
        # Remove problemName directories
        dirs = glob.glob( cls.probemName + '*')
        for dir in dirs:
            if os.path.isdir(dir):
                shutil.rmtree(dir)

    def run_avl(self, avl):
        # Run AIM pre-analysis
        avl.preAnalysis()

        ####### Run avl ####################
        print ("\n\nRunning avl......")
        currentDirectory = os.getcwd() # Get our current working directory

        os.chdir(avl.analysisDir) # Move into test directory

        # Run avl via system call
        os.system("avl caps < avlInput.txt > avlOutput.txt");

        os.chdir(currentDirectory) # Move back to top directory

        # Run AIM post-analysis
        avl.postAnalysis()

#==============================================================================
    def test_numSpan(self):

        # Load avl aim
        avl = self.myProblem.analysis.create(aim = "avlAIM")

        # Set new Mach/Alt parameters
        avl.input.Mach  = 0.5
        avl.input.Alpha = 1.0
        avl.input.Beta  = 0.0

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanTotal"      : 24, # Can specify both Total and PerSection
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.input.AVL_Surface = {"Wing": wing}

        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        wing = {"groupName"  : "Wing",
                "numChord"   : 8,
                "spaceChord" : 1.0,
                "numSpan"    : 24, # numSpan is depricated
                "spaceSpan"  : 1.0}

        avl.input.AVL_Surface = {"Wing": wing}

        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

#==============================================================================
    def test_alpha_custom_increment(self):
        
        # Load avl aim
        avl = self.myProblem.analysis.create(aim = "avlAIM")

        # Set new Mach/Alt parameters
        avl.input.Mach  = 0.5
        avl.input.Alpha = 1.0
        avl.input.Beta  = 0.0

        wing = {"groupName"         : "Wing", 
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.input.AVL_Surface = {"Wing": wing}

        Alpha      = [0.0, 3.0, 9.0]
        CLtotTrue  = [0.23815, 0.41208, 0.74935]
        CDtotTrue  = [0.01285, 0.02492, 0.06719]

        for i in range(0,len(Alpha)):
            # Set custom AoA
            avl.input.Alpha = Alpha[i]

            # run avl
            self.run_avl(avl)

            # Retrieve results
            CLtot = avl.output.CLtot
            CDtot = avl.output.CDtot
            #print("Alpha = ", Alpha[i])
            #print("CLtot = ", CLtot)
            #print("CDtot = ", CDtot)

            self.assertAlmostEqual(CLtotTrue[i], CLtot, 4)
            self.assertAlmostEqual(CDtotTrue[i], CDtot, 4)

#==============================================================================
    def test_wing_tail(self):
        
        # Load avl aim
        avl = self.myProblem.analysis.create(aim = "avlAIM")

        # Set new Mach/Alt parameters
        avl.input.Mach  = 0.5
        avl.input.Alpha = 1.0
        avl.input.Beta  = 0.0

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        htail = {"groupName"         : "hTail",
                 "numChord"          : 8,
                 "spaceChord"        : 0.7,
                 "numSpanTotal"      : 10}

        vtail = {"numChord"          : 8,
                 "spaceChord"        : 1.0,
                 "numSpanTotal"      : 8,
                 "spaceSpan"         : 0.5}

        avl.input.AVL_Surface = {"Wing" : wing,
                                 "hTail": htail,
                                 "vTail": vtail}

        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.output.CLtot
        CDtot = avl.output.CDtot
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.16271
        CDtotTrue  = 0.02286
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)

#==============================================================================
    def test_wing_Vtail(self):
        
        # Load avl aim
        avl = self.myProblem.analysis.create(aim = "avlAIM")

        # Set new Mach/Alt parameters
        avl.input.Mach  = 0.5
        avl.input.Alpha = 1.0
        avl.input.Beta  = 0.0

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        vtail = {"numChord"          : 8,
                 "spaceChord"        : 1.0,
                 "numSpanTotal"      : 8,
                 "spaceSpan"         : 1.0}

        avl.input.AVL_Surface = {"Wing"  : wing,
                                 "VTail1": vtail,
                                 "VTail2": vtail}

        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.output.CLtot
        CDtot = avl.output.CDtot
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.14642
        CDtotTrue  = 0.02022
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)

#==============================================================================
    def test_geom_change(self):
        
        # Load avl aim
        avl = self.myProblem.analysis.create(aim = "avlAIM")

        # Set new Mach/Alt parameters
        avl.input.Mach  = 0.5
        avl.input.Alpha = 1.0
        avl.input.Beta  = 0.0

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.input.AVL_Surface = {"Wing": wing}

        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.output.CLtot
        CDtot = avl.output.CDtot
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.29638
        CDtotTrue  = 0.01617
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)
    
        # change the geometry and rerun
        camber = self.myProblem.geometry.despmtr.camber
        self.myProblem.geometry.despmtr.camber = 1.1*camber
        
        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.output.CLtot
        CDtot = avl.output.CDtot
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.32388
        CDtotTrue  = 0.01801
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)

        # reset the geometry
        self.myProblem.geometry.despmtr.camber = camber

#==============================================================================
    def test_MassProp_noUnits(self):

       # Load avl aim
        avl = self.myProblem.analysis.create(aim = "avlAIM")

       # Set new Mach/Alt parameters
        avl.input.Mach  = 0.5
        avl.input.Alpha = 1.0
        avl.input.Beta  = 0.0

        wing = {"groupName"         : "Wing", # Notice Wing is the value for the capsGroup attribute
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.input.AVL_Surface = {"Wing": wing}

        mass = 0.1773
        x =  0.02463
        y = 0.
        z = 0.2239
        Ixx = 1.350
        Iyy = 0.7509
        Izz = 2.095

        avl.input.MassProp = {"Aircraft": {"mass":mass, "CG":[x,y,z], "massInertia":[Ixx, Iyy, Izz]}}

       # check there are errors if information is missing
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.Gravity  = 9.8         # m/s
        avl.input.Density  = 1.22557083  # kg/m^3
        avl.input.Velocity = 19.67167008 # "m/s"

       # make sure there are no errsos
        avl.preAnalysis()

        avl.input.MassProp = {"Aircraft":{"mass":mass, "CG":[x,y,z], "massInertia":[Ixx, Iyy, Izz, 1.0, 2.0, 3.0]},
                              "Engine"  :{"mass":mass, "CG":[x,y,z], "massInertia":[Ixx, Iyy, Izz]}}

       # again should not cause errors
        avl.preAnalysis()

       # test error handling of the mass properties parsing

        avl.input.MassProp = {"Aircraft": "1"}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.MassProp = {"Aircraft": "foo"}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.MassProp = {"Aircraft": {"mass":mass, "CG":[x,y,z]}}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_NOTFOUND")

        avl.input.MassProp = {"Aircraft": {"mass":"mass", "CG":[x,y,z], "massInertia":[Ixx, Iyy, Izz]}}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")


#==============================================================================
    def test_MassProp_Units(self):

        m    = pyCAPS.Unit("meter")
        kg   = pyCAPS.Unit("kg")
        s    = pyCAPS.Unit("s")
        K    = pyCAPS.Unit("Kelvin")
        deg  = pyCAPS.Unit("degree")
        ft   = pyCAPS.Unit("ft")
        slug = pyCAPS.Unit("slug")


       # Load avl aim
        avl = self.myProblem.analysis.create(aim = "avlAIM",
                                             unitSystem={"mass":kg, "length":m, "time":s, "temperature":K})

       # Set new Mach/Alt parameters
        avl.input.Mach  = 0.5
        avl.input.Alpha = 1.0 * deg
        avl.input.Beta  = 0.0 * deg

        wing = {"groupName"         : "Wing", # Notice Wing is the value for the capsGroup attribute
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.input.AVL_Surface = {"Wing": wing}

        mass = 0.1773
        x =  0.02463
        y = 0.
        z = 0.2239
        Ixx = 1.350
        Iyy = 0.7509
        Izz = 2.095

        avl.input.MassProp = {"Aircraft": {"mass":mass * kg, "CG":[x,y,z] * m, "massInertia":[Ixx, Iyy, Izz] * kg*m**2}}

       # check there are errors if information is missing
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.Gravity  = 32.18 * ft/s**2
        avl.input.Density  = 0.002378 * slug/ft**3 
        avl.input.Velocity = 64.5396 * ft/s

       # make sure there are no errsos
        avl.preAnalysis()

        avl.input.MassProp = {"Aircraft":{"mass":mass * kg, "CG":[x,y,z] * m, "massInertia":[Ixx, Iyy, Izz, 1.0, 2.0, 3.0] * kg*m**2},
                              "Engine"  :{"mass":mass * kg, "CG":[x,y,z] * m, "massInertia":[Ixx, Iyy, Izz] * kg*m**2}}

       # again should not cause errors
        avl.preAnalysis()

       # test error handling of the mass properties parsing

        avl.input.MassProp = {"Aircraft": "1"}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.MassProp = {"Aircraft": "foo"}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.MassProp = {"Aircraft": {"mass":[mass,"kg"], "CG":[[x,y,z],"m"]}}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_NOTFOUND")

        avl.input.MassProp = {"Aircraft": {"mass":(mass), "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.MassProp = {"Aircraft": {"mass":[mass,"kg"], "CG":[x,y,z], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.MassProp = {"Aircraft": {"mass":[mass,"kg"], "CG":[[x,y,z],"m"], "massInertia":[Ixx, Iyy, Izz]}}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.input.MassProp = {"Aircraft": {"mass":("mass","kg"), "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}}
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

if __name__ == '__main__':
    unittest.main()
