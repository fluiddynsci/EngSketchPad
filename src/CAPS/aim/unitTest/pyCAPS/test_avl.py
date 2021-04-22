# Import other need modules
from __future__ import print_function

import unittest
import os
import argparse

# Import pyCAPS class file
import pyCAPS
from pyCAPS import capsProblem


# Geometry is verified with avl by plotting the camber using the commands:
#
# avl caps
# oper
# g
# ca

class TestavlAIM(unittest.TestCase):

    @classmethod
    def setUpClass(self):
#    def setUp(self):

        # Initialize capsProblem object
        self.myProblem = capsProblem()

        # Load CSM file
        self.myGeometry = self.myProblem.loadCAPS("../csmData/avlSections.csm")

        # Create working directory variable
        self.workDir = "workDir_avlAnalysisTest"

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

    def test_numSpan(self):

        # Load avl aim
        avl = self.myProblem.loadAIM(aim = "avlAIM",
                                     analysisDir = self.workDir + "tmp")

        # Set new Mach/Alt parameters
        avl.setAnalysisVal("Mach", 0.5)
        avl.setAnalysisVal("Alpha", 1.0)
        avl.setAnalysisVal("Beta", 0.0)

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanTotal"      : 24, # Can specify both Total and PerSection
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.setAnalysisVal("AVL_Surface", [("Wing", wing)])

        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        wing = {"groupName"  : "Wing",
                "numChord"   : 8,
                "spaceChord" : 1.0,
                "numSpan"    : 24, # numSpan is depricated
                "spaceSpan"  : 1.0}

        avl.setAnalysisVal("AVL_Surface", [("Wing", wing)])

        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

    def test_alpha_custom_increment(self):
        
        # Load avl aim
        avl = self.myProblem.loadAIM(aim = "avlAIM",
                                     analysisDir = self.workDir + "Alpha")

        # Set new Mach/Alt parameters
        avl.setAnalysisVal("Mach", 0.5)
        avl.setAnalysisVal("Alpha", 1.0)
        avl.setAnalysisVal("Beta", 0.0)

        wing = {"groupName"         : "Wing", 
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.setAnalysisVal("AVL_Surface", [("Wing", wing)])

        Alpha      = [0.0, 3.0, 9.0]
        CLtotTrue  = [0.23815, 0.41208, 0.74935]
        CDtotTrue  = [0.01285, 0.02492, 0.06719]

        for i in range(0,len(Alpha)):
            # Set custom AoA
            avl.setAnalysisVal("Alpha", Alpha[i])

            # run avl
            self.run_avl(avl)

            # Retrieve results
            CLtot = avl.getAnalysisOutVal("CLtot")
            CDtot = avl.getAnalysisOutVal("CDtot")
            #print("Alpha = ", Alpha[i])
            #print("CLtot = ", CLtot)
            #print("CDtot = ", CDtot)

            self.assertAlmostEqual(CLtotTrue[i], CLtot, 4)
            self.assertAlmostEqual(CDtotTrue[i], CDtot, 4)

    def test_wing_tail(self):
        
        # Load avl aim
        avl = self.myProblem.loadAIM(aim = "avlAIM",
                                     analysisDir = self.workDir + "Tail")

        # Set new Mach/Alt parameters
        avl.setAnalysisVal("Mach", 0.5)
        avl.setAnalysisVal("Alpha", 1.0)
        avl.setAnalysisVal("Beta", 0.0)

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        htail = {"groupName"         : "hTail",
                 "numChord"          : 8,
                 "spaceChord"        : 1.0,
                 "numSpanTotal"      : 8,
                 "spaceSpan"         : 1.0}

        vtail = {"numChord"          : 8,
                 "spaceChord"        : 1.0,
                 "numSpanTotal"      : 8,
                 "spaceSpan"         : 1.0}

        avl.setAnalysisVal("AVL_Surface", [("Wing", wing),("hTail", htail),
                                           ("vTail", vtail)])

        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.getAnalysisOutVal("CLtot")
        CDtot = avl.getAnalysisOutVal("CDtot")
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.15794
        CDtotTrue  = 0.02268
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)


    def test_wing_Vtail(self):
        
        # Load avl aim
        avl = self.myProblem.loadAIM(aim = "avlAIM",
                                     analysisDir = self.workDir + "VTail")

        # Set new Mach/Alt parameters
        avl.setAnalysisVal("Mach", 0.5)
        avl.setAnalysisVal("Alpha", 1.0)
        avl.setAnalysisVal("Beta", 0.0)

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        vtail = {"numChord"          : 8,
                 "spaceChord"        : 1.0,
                 "numSpanTotal"      : 8,
                 "spaceSpan"         : 1.0}

        avl.setAnalysisVal("AVL_Surface", [("Wing", wing),
                                           ("VTail1", vtail),("VTail2", vtail)])

        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.getAnalysisOutVal("CLtot")
        CDtot = avl.getAnalysisOutVal("CDtot")
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.14642
        CDtotTrue  = 0.02022
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)


    def test_geom_change(self):
        
        # Load avl aim
        avl = self.myProblem.loadAIM(aim = "avlAIM",
                                     analysisDir = self.workDir + "Geom")

        # Set new Mach/Alt parameters
        avl.setAnalysisVal("Mach", 0.5)
        avl.setAnalysisVal("Alpha", 1.0)
        avl.setAnalysisVal("Beta", 0.0)

        wing = {"groupName"         : "Wing",
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.setAnalysisVal("AVL_Surface", [("Wing", wing)])

        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.getAnalysisOutVal("CLtot")
        CDtot = avl.getAnalysisOutVal("CDtot")
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.29638
        CDtotTrue  = 0.01617
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)
    
        # change the geometry and rerun
        camber = self.myGeometry.getGeometryVal("camber")
        self.myGeometry.setGeometryVal("camber", 1.1*camber)
        
        # run avl
        self.run_avl(avl)

        # Retrieve results
        CLtot = avl.getAnalysisOutVal("CLtot")
        CDtot = avl.getAnalysisOutVal("CDtot")
        #print("CLtot = ", CLtot)
        #print("CDtot = ", CDtot)

        CLtotTrue  = 0.32388
        CDtotTrue  = 0.01801
        self.assertAlmostEqual(CLtotTrue, CLtot, 4)
        self.assertAlmostEqual(CDtotTrue, CDtot, 4)

        # reset the geometry
        self.myGeometry.setGeometryVal("camber", camber)

    def test_MassProp(self):

       # Load avl aim
        avl = self.myProblem.loadAIM(aim = "avlAIM",
                                     analysisDir = self.workDir + "Mass")

       # Set new Mach/Alt parameters
        avl.setAnalysisVal("Mach", 0.5)
        avl.setAnalysisVal("Alpha", 1.0)
        avl.setAnalysisVal("Beta", 0.0)

        wing = {"groupName"         : "Wing", # Notice Wing is the value for the capsGroup attribute
                "numChord"          : 8,
                "spaceChord"        : 1.0,
                "numSpanPerSection" : 12,
                "spaceSpan"         : 1.0}

        avl.setAnalysisVal("AVL_Surface", [("Wing", wing)])

        mass = 0.1773
        x =  0.02463
        y = 0.
        z = 0.2239
        Ixx = 1.350
        Iyy = 0.7509
        Izz = 2.095

        avl.setAnalysisVal("MassProp", ("Aircraft",{"mass":[mass,"kg"], "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}))

       # check there are errors if information is missing
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.setAnalysisVal("Lunit", 1, units="ft")
        avl.setAnalysisVal("Gravity", 32.18, units="ft/s^2")
        avl.setAnalysisVal("Density", 0.002378, units="slug/ft^3")
        avl.setAnalysisVal("Velocity", 64.5396, units="ft/s")

       # make sure there are no errsos
        avl.preAnalysis()

        avl.setAnalysisVal("MassProp", [("Aircraft",{"mass":[mass,"kg"], "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz, 1.0, 2.0, 3.0], "kg*m^2"]}),
                                        ("Engine",  {"mass":[mass,"kg"], "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]})])

       # again should not cause errors
        avl.preAnalysis()

       # test error handling of the mass properties parsing

        avl.setAnalysisVal("MassProp", ("Aircraft", "1"))
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.setAnalysisVal("MassProp", ("Aircraft", "foo"))
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.setAnalysisVal("MassProp", ("Aircraft", {"mass":[mass,"kg"], "CG":[[x,y,z],"m"]}))
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_NOTFOUND")

        avl.setAnalysisVal("MassProp", ("Aircraft", {"mass":(mass), "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}))
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.setAnalysisVal("MassProp", ("Aircraft",{"mass":[mass,"kg"], "CG":[x,y,z], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}))
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.setAnalysisVal("MassProp", ("Aircraft",{"mass":[mass,"kg"], "CG":[[x,y,z],"m"], "massInertia":[Ixx, Iyy, Izz]}))
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

        avl.setAnalysisVal("MassProp", ("Aircraft",{"mass":("mass","kg"), "CG":[[x,y,z],"m"], "massInertia":[[Ixx, Iyy, Izz], "kg*m^2"]}))
        with self.assertRaises(pyCAPS.CAPSError) as e:
            avl.preAnalysis()
        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

    @classmethod
    def tearDownClass(self):
#    def tearDown(self):

        # Close CAPS - Optional
        self.myProblem.closeCAPS()

if __name__ == '__main__':
    unittest.main()
