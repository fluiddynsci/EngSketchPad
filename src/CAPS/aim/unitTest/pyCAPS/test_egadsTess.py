from __future__ import print_function
import unittest

import os
import shutil

import pyCAPS

class TestEGADS(unittest.TestCase):


    @classmethod
    def setUpClass(cls):

        # Initialize a global capsProblem object
        cls.myProblem = pyCAPS.capsProblem()

        # Working directory
        cls.workDir = "workDir_egadsTest"

        # Load CSM file
        cls.myGeometry = cls.myProblem.loadCAPS(os.path.join("..","csmData","cornerGeom.csm"))

    @classmethod
    def tearDownClass(cls):

        # Close CAPS - Optional
        cls.myProblem.closeCAPS()

        # Remove analysis directories
        if os.path.exists(cls.workDir):
            shutil.rmtree(cls.workDir)

    def test_invalid_Mesh_Lenght_Scale(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        myAnalysis = myProblem.loadAIM(aim = "egadsTessAIM",
                                       analysisDir = self.workDir)

        # Set project name so a mesh file is generated
        myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_AFLR4_Test")

        # Set output grid format since a project name is being supplied - Tecplot  file
        myAnalysis.setAnalysisVal("Mesh_Format", "Tecplot")

        # Set output grid format since a project name is being supplied - Tecplot  file
        myAnalysis.setAnalysisVal("Mesh_Length_Factor", -1)

        with self.assertRaises(pyCAPS.CAPSError) as e:
            myAnalysis.preAnalysis()

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

    def test_setAnalysisVals(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        myAnalysis = myProblem.loadAIM(aim = "egadsTessAIM",
                                       analysisDir = self.workDir)

        myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_AFLR4_Test")
        myAnalysis.setAnalysisVal("Mesh_Length_Factor", 1.05)
        myAnalysis.setAnalysisVal("Tess_Params", [0.1, 0.01, 15.0])
        myAnalysis.setAnalysisVal("Mesh_Format", "Tecplot")
        myAnalysis.setAnalysisVal("Mesh_ASCII_Flag", True)
        myAnalysis.setAnalysisVal("Edge_Point_Min", 1)
        myAnalysis.setAnalysisVal("Edge_Point_Max", 10)
        
        # Modify local mesh sizing parameters
        Mesh_Sizing = [("Farfield", {"tessParams"    : [0, 0.2, 30]})]
        myAnalysis.setAnalysisVal("Mesh_Sizing", Mesh_Sizing)

        myAnalysis.setAnalysisVal("Mesh_Elements", "Quad")
        myAnalysis.setAnalysisVal("Multiple_Mesh", True)
        myAnalysis.setAnalysisVal("TFI_Templates", False)

    def test_SingleBody_AnalysisOutVal(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        myAnalysis = myProblem.loadAIM(aim = "egadsTessAIM",
                                       analysisDir = self.workDir)

        # Modify local mesh sizing parameters
        Mesh_Sizing = [("Farfield", {"tessParams" : [0.3*80, 0.2*80, 30]})]
        myAnalysis.setAnalysisVal("Mesh_Sizing", Mesh_Sizing)

        # Run
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()
        
        # Assert AnalysisOutVals
        self.assertTrue(myAnalysis.getAnalysisOutVal("Done"))

        numNodes = myAnalysis.getAnalysisOutVal("NumberOfNode")
        self.assertGreater(numNodes, 0)
        numElements = myAnalysis.getAnalysisOutVal("NumberOfElement")
        self.assertGreater(numElements, 0)

    def test_MultiBody(self):

        file = os.path.join("..","csmData","cfdMultiBody.csm")
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        myAnalysis = myProblem.loadAIM(aim = "egadsTessAIM",
                                       analysisDir = self.workDir)
        
        # Modify local mesh sizing parameters
        Mesh_Sizing = [("Farfield", {"tessParams" : [0.3*80, 0.2*80, 30]})]
        myAnalysis.setAnalysisVal("Mesh_Sizing", Mesh_Sizing)

        # Run
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()

    def test_reenter(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        myAnalysis = myProblem.loadAIM(aim = "egadsTessAIM",
                                       analysisDir = self.workDir)

        # Modify local mesh sizing parameters
        Mesh_Sizing = [("Farfield", {"tessParams" : [0.3*80, 0.2*80, 30]})]
        myAnalysis.setAnalysisVal("Mesh_Sizing", Mesh_Sizing)

        myAnalysis.setAnalysisVal("Mesh_Length_Factor", 1)

        # Run 1st time
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()

        myAnalysis.setAnalysisVal("Mesh_Length_Factor", 2)

        # Run 2nd time coarser
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()


    def test_box(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "Box",
                                           capsIntent = ["box", "farfield"])

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

    def test_cylinder(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "Cylinder",
                                           capsIntent = ["cylinder", "farfield"])

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

    def test_cone(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "Cone",
                                           capsIntent = ["cone", "farfield"])

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

    def test_torus(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "Torus",
                                           capsIntent = ["torus", "farfield"])

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

        #egadsTess.viewGeometry()

    def test_sphere(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "Sphere",
                                           capsIntent = ["sphere", "farfield"])

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

        #egadsTess.viewGeometry()

    def test_boxhole(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "BoxHole",
                                           capsIntent = ["boxhole", "farfield"])

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

        #egadsTess.viewGeometry()

    def test_bullet(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "Bullet",
                                           capsIntent = ["bullet", "farfield"])

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

        #egadsTess.viewGeometry()

    def test_all(self):

        # Load egadsTess aim
        egadsTess = self.myProblem.loadAIM(aim = "egadsTessAIM",
                                           analysisDir = self.workDir + "All",
                                           capsIntent = ["box", "cylinder", "cone", "torus", "sphere", "boxhole", "bullet", "farfield"]) 

        # Just make sure it runs without errors...
        egadsTess.preAnalysis()
        egadsTess.postAnalysis()

        #egadsTess.viewGeometry()


if __name__ == '__main__':
    unittest.main()
