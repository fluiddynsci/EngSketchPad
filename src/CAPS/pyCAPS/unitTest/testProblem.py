import unittest

import os

from sys import version_info as pyVersion

import pyCAPS

class TestProblem(unittest.TestCase):

    @classmethod
    def setUpClass(cls):

        cls.file = "unitGeom.csm"
        cls.analysisDir = "UnitTest"
        cls.myProblem = pyCAPS.capsProblem()

        cls.myGeometry = cls.myProblem.loadCAPS(cls.file, "basicTest", verbosity=0)

        cls.myAnalysis = cls.myProblem.loadAIM(aim = "fun3dAIM",
                                                altName = "fun3d",
                                                analysisDir = cls.analysisDir,
                                                capsIntent = "CFD")
    @classmethod
    def tearDownClass(cls):

        # Remove analysis directories
        if os.path.exists(cls.analysisDir):
            os.rmdir(cls.analysisDir)

        # Remove created files
        if os.path.isfile("myProblem.html"):
            os.remove("myProblem.html")

        if os.path.isfile("saveCAPS.caps"):
            os.remove("saveCAPS.caps")

    # Multiple problems
    def test_loadCAPSMulti(self):

        myProblemNew = pyCAPS.capsProblem()
        myProblemNew.loadCAPS(self.file, "basicTest", verbosity="debug")

        self.assertNotEqual(self.myProblem, myProblemNew)

    # Close CAPS
    def test_closeCAPS(self):

        myProblem = pyCAPS.capsProblem()
        myProblem.loadCAPS(self.file, "basicTest", verbosity=0)
        myProblem.closeCAPS()

    # Geometry - object consistent
    def test_geometry(self):

        self.assertEqual(self.myGeometry, self.myProblem.geometry)

    # Analysis dictionary - object consistent
    def test_analysis(self):

        self.assertEqual(self.myAnalysis, self.myProblem.analysis["fun3d"])

    # Load AIM creates a directory
    def test_loadAIMDir(self):

        self.assertTrue(os.path.isdir(self.myAnalysis.analysisDir))

    # Set verbosity
    def test_setVerbosity(self):

        self.myProblem.setVerbosity("minimal")

        self.myProblem.setVerbosity("standard")

        self.myProblem.setVerbosity(0)

        self.myProblem.setVerbosity(2)

        with self.assertRaises(pyCAPS.CAPSError) as e:
            self.myProblem.setVerbosity(10)

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

    # Create html tree
    @unittest.skipIf(pyVersion.major > 2, "Odd behavior in Python 3")
    def test_createTree(self):

        self.myProblem.createTree()

        self.assertTrue(os.path.isfile("myProblem.html"))

    # Adding/getting attributes
    def test_attributes(self):

        # Check list attributes
        self.myProblem.addAttribute("testAttr", [1, 2, 3])
        self.assertEqual(self.myProblem.getAttribute("testAttr"), [1,2,3])

        # Check float attributes
        self.myProblem.addAttribute("testAttr_2", 10.0)
        self.assertEqual(self.myProblem.getAttribute("testAttr_2"), 10.0)

        # Check string attributes
        self.myProblem.addAttribute("testAttr_3", "anotherAttribute")
        self.assertEqual(self.myProblem.getAttribute("testAttr_3"), "anotherAttribute")

        # Check over writing attribute
        self.myProblem.addAttribute("testAttr_2", 30.0)
        self.assertEqual(self.myProblem.getAttribute("testAttr_2"), 30.0)

        #self.myProblem.addAttribute("arrayOfStrings", ["1", "2", "3", "4"])
        #self.assertEqual(self.myProblem.getAttribute("arrayOfStrings"), ["1", "2", "3", "4"])

    # Create value
    def test_createValue(self):

        myValue = self.myProblem.createValue("Alpha", 10, units="degree")
        self.assertEqual(myValue.name, "Alpha")

    # Save CAPS and reload
    def test_saveCAPS(self):

        # Run pre and post to get the analysis in a "clean" state
        self.myAnalysis.setAnalysisVal("Mach", 0.1)
        self.myAnalysis.preAnalysis()
        self.myAnalysis.postAnalysis()

        # Add a value/parameter to problem
        myValue = self.myProblem.createValue("Mach", 0.5)

        self.myProblem.saveCAPS()

        myProblemNew = pyCAPS.capsProblem()
        myProblemNew.loadCAPS("saveCAPS.caps", verbosity=0)

        myAnalysisNew = myProblemNew.analysis["fun3d"]

        # Check analysis directories
        self.assertEqual(self.myAnalysis.analysisDir, myAnalysisNew.analysisDir)

        # Check set input value set
        self.assertEqual(self.myAnalysis.getAnalysisVal("Mach"), myAnalysisNew.getAnalysisVal("Mach"))

        # Check capValue
        self.assertEqual(self.myProblem.value["Mach"].value, myProblemNew.value["Mach"].value)

if __name__ == '__main__':
    unittest.main()
