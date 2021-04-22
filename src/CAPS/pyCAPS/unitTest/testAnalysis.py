import unittest

import os

from sys import version_info as pyVersion
from sys import version_info

import pyCAPS

class TestAnalysis(unittest.TestCase):

    @classmethod
    def setUpClass(cls):

        cls.file = "unitGeom.csm"
        cls.analysisDir = "UnitTest"

        cls.myProblem = pyCAPS.capsProblem()

        cls.myGeometry = cls.myProblem.loadCAPS(cls.file, "basicTest", verbosity=0)

        cls.myAnalysis = cls.myProblem.loadAIM(aim = "fun3dAIM",
                                               analysisDir = cls.analysisDir)

    @classmethod
    def tearDownClass(cls):

        # Remove analysis directories
        if os.path.exists(cls.analysisDir):
            os.rmdir(cls.analysisDir)

        for i in range(11):
            if os.path.exists(cls.analysisDir+str(i)):
                os.rmdir(cls.analysisDir+str(i))

        # Remove created files
        if os.path.isfile("myAnalysisGeometry.egads"):
            os.remove("myAnalysisGeometry.egads")

        if os.path.isfile("fun3dAIM.html"):
            os.remove("fun3dAIM.html")

        if os.path.isfile("d3.v3.min.js"):
            os.remove("d3.v3.min.js")

    # Same AIM twice
    def test_loadAIm(self):

        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "2")


        self.assertEqual(("fun3dAIM"   in self.myProblem.analysis.keys() and
                          "fun3dAIM_1" in self.myProblem.analysis.keys()), True)

    # Same directory
    def test_sameDir(self):

        with self.assertRaises(pyCAPS.CAPSError) as e:
            myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                                analysisDir = self.analysisDir)

        self.assertEqual(e.exception.errorName, "CAPS_DIRERR")

    # Copy AIM
    def test_copyAIM(self):
        myAnalysis = None
        myAnalysis = self.myProblem.loadAIM(altName = "fun3d2",
                                            analysisDir = self.analysisDir + "3",
                                            copyAIM = "fun3dAIM")

        self.assertNotEqual(myAnalysis, None)

    # Multiple intents
    def test_multipleIntents(self):
        myAnalysis = None
        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "4",
                                            capsIntent = ["Test", "CFD"])

        self.assertNotEqual(myAnalysis, None)

#     # Bad intent
#     def test_badIntent(self):
#         with self.assertRaises(pyCAPS.CAPSError) as e:
#             myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
#                                                 analysisDir = self.analysisDir + "5",
#                                                 capsIntent = "BadIntent")
#
#         self.assertEqual(e.exception.errorName, "CAPS_BADINTENT")
#
#     # Bad intents
#     def test_badIntents(self):
#         with self.assertRaises(pyCAPS.CAPSError) as e:
#             myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
#                                                 analysisDir = self.analysisDir + "6",
#                                                 capsIntent = ["CFD", "BadIntent"])
#
#         self.assertEqual(e.exception.errorName, "CAPS_BADINTENT")
#
#     # Invalid intent
#     def test_invalidIntent(self):
#         with self.assertRaises(pyCAPS.CAPSError) as e:
#             myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
#                                                 analysisDir = self.analysisDir + "7",
#                                                 capsIntent = "Structure")
#
#         self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

    # Retrieve geometry attribute
    def test_getAttributeVal(self):

        attributeList = self.myAnalysis.getAttributeVal("capsGroup", attrLevel="Body")
        self.assertEqual(attributeList.sort(), ['Farfield', 'Wing2', 'Wing1'].sort())

        attributeList = self.myAnalysis.getAttributeVal("capsGroup", attrLevel="5")
        self.assertEqual(attributeList.sort(), ['Farfield', 'Wing2', 'Wing1'].sort())

        attributeList = self.myAnalysis.getAttributeVal("capsGroup", attrLevel="Node")
        self.assertEqual(attributeList.sort(), ['Farfield', 'Wing2', 'Wing1'].sort())

    # Set analysis value with units
    def test_setAnalysisVal(self):
        self.myAnalysis.setAnalysisVal("Beta", 0.174533, units="radian")
        self.assertEqual(int(self.myAnalysis.getAnalysisVal("Beta")), 10) # Should be converted to degree

    # Set/Unset analysis value
    def test_setAnalysisVal(self):
        self.assertEqual(self.myAnalysis.getAnalysisVal("Mach"),None)
        self.myAnalysis.setAnalysisVal("Mach", 0.5)
        self.assertEqual(self.myAnalysis.getAnalysisVal("Mach"), 0.5)
        self.myAnalysis.setAnalysisVal("Mach", None)
        self.assertEqual(self.myAnalysis.getAnalysisVal("Mach"),None)

    # Get analysis in values
    def test_getAnalysisVal(self):

        # Get all input values
        valueDict = self.myAnalysis.getAnalysisVal()
        self.assertEqual(valueDict["Overwrite_NML"], False)

        # Get just the names of the input values
        valueList = self.myAnalysis.getAnalysisVal(namesOnly = True)
        self.assertEqual("Proj_Name" in valueList, True)

    # Get a string and add it to another string -> Make sure byte<->str<->unicode is correct between Python 2 and 3
    def test_AnalysisString(self):

        string = "myTest"
        self.myAnalysis.setAnalysisVal("Proj_Name", string);

        myString = self.myAnalysis.getAnalysisVal("Proj_Name")
        self.assertEqual(myString+"_TEST", "myTest_TEST")

    # Get analysis out values
    def test_getAnalysisOutVal(self):

        # Get all output values - analysis should be dirty since pre hasn't been run
        with self.assertRaises(pyCAPS.CAPSError) as e:
            valueDict = self.myAnalysis.getAnalysisOutVal()

        self.assertEqual(e.exception.errorName, "CAPS_DIRTY")

        # Get just the names of the output values
        valueList = self.myAnalysis.getAnalysisOutVal(namesOnly = True)
        self.assertEqual("CLtot" in valueList, True)


    # Analysis - object consistent
    def test_analysis(self):

        self.assertEqual(self.myAnalysis, self.myProblem.analysis["fun3dAIM"])

    # Create html tree
    @unittest.skipIf(pyVersion.major > 2, "Odd behavior in Python 3")
    def test_createTree(self):

        inviscid = {"bcType" : "Inviscid", "wallTemperature" : 1.1}
        self.myAnalysis.setAnalysisVal("Boundary_Condition", [("Skin", inviscid),
                                                              ("SymmPlane", "SymmetryY"),
                                                              ("Farfield","farfield")])

        # Create tree
        self.myAnalysis.createTree(internetAccess = False,
                                   embedJSON = True,
                                   analysisGeom = True,
                                   reverseMap = True)
        self.assertTrue(os.path.isfile("fun3dAIM.html"))
        self.assertTrue(os.path.isfile("d3.v3.min.js"))

    # Get analysis info
    def test_getAnalysisInfo(self):

        # Make sure it returns a integer
        state = self.myAnalysis.getAnalysisInfo()
        self.assertEqual(isinstance(state, int), True)

        # Make sure it returns a dictionary
        infoDict = self.myAnalysis.getAnalysisInfo(printinfo = False,  infoDict = True)
        self.assertEqual(infoDict["analysisDir"], self.analysisDir)

    # Get analysis info with a NULL intent
    def test_getAnalysisInfoIntent(self):

        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "5"
                                            ) # Use default capsIntent

        infoDict = myAnalysis.getAnalysisInfo(printinfo = False,  infoDict = True)
        self.assertEqual(infoDict["intent"], None)

        # Set the default capsIntent
        self.myProblem.capsIntent = "CFD"
        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "6"
                                            ) # Use default capsIntent

        infoDict = myAnalysis.getAnalysisInfo(printinfo = False,  infoDict = True)
        self.assertEqual(infoDict["intent"], "CFD")

        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "7",
                                            capsIntent = None) # Use no capsIntent

        infoDict = myAnalysis.getAnalysisInfo(printinfo = False,  infoDict = True)
        self.assertEqual(infoDict["intent"], None)

        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "8",
                                            capsIntent = "") # Use no capsIntent

        infoDict = myAnalysis.getAnalysisInfo(printinfo = False,  infoDict = True)
        self.assertEqual(infoDict["intent"], None)

        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "9",
                                            capsIntent = "STRUCTURE") # Use different capsIntent

        infoDict = myAnalysis.getAnalysisInfo(printinfo = False,  infoDict = True)
        self.assertEqual(infoDict["intent"], "STRUCTURE")

        myAnalysis = self.myProblem.loadAIM(aim = "fun3dAIM",
                                            analysisDir = self.analysisDir + "10",
                                            capsIntent = ["CFD","STRUCTURE"]) # Use multiple capsIntent

        infoDict = myAnalysis.getAnalysisInfo(printinfo = False,  infoDict = True)
        self.assertEqual(infoDict["intent"], "CFD;STRUCTURE")

        # Rest the default capsIntent
        self.myProblem.capsIntent = None

    # Adding/getting attributes
    def test_attributes(self):

        # Check list attributes
        self.myAnalysis.addAttribute("testAttr", [1, 2, 3])
        self.assertEqual(self.myAnalysis.getAttribute("testAttr"), [1,2,3])

        # Check float attributes
        self.myAnalysis.addAttribute("testAttr_2", 10.0)
        self.assertEqual(self.myAnalysis.getAttribute("testAttr_2"), 10.0)

        # Check string attributes
        self.myAnalysis.addAttribute("testAttr_3", "anotherAttribute")
        self.assertEqual(self.myAnalysis.getAttribute("testAttr_3"), "anotherAttribute")

        # Check over writing attribute
        self.myAnalysis.addAttribute("testAttr_2", 30.0)
        self.assertEqual(self.myAnalysis.getAttribute("testAttr_2"), 30.0)

        #self.myAnalysis.addAttribute("arrayOfStrings", ["1", "2", "3", "4"])
        #self.assertEqual(self.myAnalysis.getAttribute("arrayOfStrings"), ["1", "2", "3", "4"])
    
    # Save geometry
    def test_saveGeometry(self):
        
        self.myAnalysis.saveGeometry("myAnalysisGeometry")
        self.assertTrue(os.path.isfile("myAnalysisGeometry.egads"))

if __name__ == '__main__':
    unittest.main()
