import unittest

import os

import shutil

from sys import version_info as pyVersion

import pyCAPS

class TestSU2(unittest.TestCase):

    @classmethod
    def setUpClass(cls):

        cls.file = "../csmData/cfdMultiBody.csm"
        cls.analysisDir = "su2_UnitTest"

        cls.configFile = "su2_CAPS.cfg"
        cls.myProblem = pyCAPS.capsProblem()

        cls.myGeometry = cls.myProblem.loadCAPS(cls.file)

        cls.myAnalysis = cls.myProblem.loadAIM(aim = "su2AIM",
                                               analysisDir = cls.analysisDir)

    @classmethod
    def tearDownClass(cls):

        # Remove analysis directories
        if os.path.exists(cls.analysisDir):
            shutil.rmtree(cls.analysisDir)
        
        if os.path.exists(cls.analysisDir + '_1'):
            shutil.rmtree(cls.analysisDir + '_1')
            
        if os.path.exists(cls.analysisDir + '_2'):
            shutil.rmtree(cls.analysisDir + '_2')

#         # Remove created files
#         if os.path.isfile("myGeometry.egads"):
#             os.remove("myGeometry.egads")

        cls.myProblem.closeCAPS()

    # Try put an invalid boundary name
    def test_invalidBoundaryName(self):
        # Create a new instance
        myAnalysis = self.myProblem.loadAIM(aim = "su2AIM",
                                            analysisDir = self.analysisDir + "_1")
        
        myAnalysis.setAnalysisVal("Boundary_Condition", [("Wing1", {"bcType" : "Inviscid"}), # No capsGroup 'X'
                                                         ("X", {"bcType" : "Inviscid"}),
                                                         ("Farfield","farfield")])
        with self.assertRaises(pyCAPS.CAPSError) as e:
            myAnalysis.preAnalysis()
 
        self.assertEqual(e.exception.errorName, "CAPS_NOTFOUND")
        
    # Try an invalid boundary type
    def test_invalidBoundary(self):        
        # Create a new instance
        myAnalysis = self.myProblem.loadAIM(aim = "su2AIM",
                                            analysisDir = self.analysisDir + "_2")
        
        myAnalysis.setAnalysisVal("Boundary_Condition", ("Wing1", {"bcType" : "X"}))
        
        with self.assertRaises(pyCAPS.CAPSError) as e:
            myAnalysis.preAnalysis()
 
        self.assertEqual(e.exception.errorName, "CAPS_NOTFOUND")
        
    # Test re-enter 
    def test_reenter(self):
        
        self.myAnalysis.setAnalysisVal("Boundary_Condition", [("Wing1", {"bcType" : "Inviscid"}),
                                                              ("Wing2", {"bcType" : "Inviscid"}),
                                                              ("Farfield","farfield")])
        self.myAnalysis.preAnalysis()
        self.myAnalysis.postAnalysis()
        
        self.assertEqual(os.path.isfile(os.path.join(self.analysisDir, self.configFile)), True)
        
        os.remove(os.path.join(self.myAnalysis.analysisDir, self.configFile))
        
        self.myAnalysis.setAnalysisVal("Boundary_Condition", [("Wing1", {"bcType" : "Viscous"}),
                                                              ("Wing2", {"bcType" : "Inviscid"}),
                                                              ("Farfield","farfield")])
        self.myAnalysis.preAnalysis()
        self.myAnalysis.postAnalysis()
        
        self.assertEqual(os.path.isfile(os.path.join(self.myAnalysis.analysisDir, self.configFile)), True)
 
if __name__ == '__main__':
    unittest.main()
