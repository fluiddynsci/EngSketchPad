from __future__ import print_function
import unittest

import os
import glob
import shutil

import pyCAPS

class TestAFLR4(unittest.TestCase):


    @classmethod
    def setUpClass(cls):
        cls.problemName = "workDir_aflr4Test"
        cls.iProb = 1
        cls.cleanUp()

        # Initialize a global Problem object
        cornerFile = os.path.join("..","csmData","cornerGeom.csm")
        cls.myProblem = pyCAPS.Problem(cls.problemName, capsFile=cornerFile, outLevel=0)

    @classmethod
    def tearDownClass(cls):
        del cls.myProblem
        cls.cleanUp()

    @classmethod
    def cleanUp(cls):

        # Remove problem directories
        dirs = glob.glob( cls.problemName + '*')
        for dir in dirs:
            if os.path.isdir(dir):
                shutil.rmtree(dir)

    def test_invalid_Mesh_Lenght_Scale(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        myAnalysis = myProblem.analysis.create(aim = "aflr4AIM")

        myAnalysis.input.Mesh_Quiet_Flag = True

        # Set project name so a mesh file is generated
        myAnalysis.input.Proj_Name = "pyCAPS_AFLR4_Test"

        # Set output grid format since a project name is being supplied - Tecplot  file
        myAnalysis.input.Mesh_Format = "Tecplot"

        myAnalysis.input.Mesh_Length_Factor = -1

        with self.assertRaises(pyCAPS.CAPSError) as e:
            myAnalysis.preAnalysis()

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

    def test_setInput(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        myAnalysis = myProblem.analysis.create(aim = "aflr4AIM")

        myAnalysis.input.Proj_Name = "pyCAPS_AFLR4_Test"
        myAnalysis.input.Mesh_Quiet_Flag = True
        myAnalysis.input.Mesh_Format = "Tecplot"
        myAnalysis.input.Mesh_ASCII_Flag = True
        myAnalysis.input.Mesh_Gen_Input_String = "auto_mode=0"
        myAnalysis.input.ff_cdfr = 1.4
        myAnalysis.input.min_ncell = 20
        myAnalysis.input.no_prox = True
        myAnalysis.input.abs_min_scale = 0.01
        myAnalysis.input.BL_thickness = 0.01
        myAnalysis.input.curv_factor = 0.4
        myAnalysis.input.max_scale = 0.1
        myAnalysis.input.min_scale = 0.01
        myAnalysis.input.Mesh_Length_Factor = 1.05
        myAnalysis.input.erw_all = 0.7

    def test_SingleBody_AnalysisOutVal(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        myAnalysis = myProblem.analysis.create(aim = "aflr4AIM")

        myAnalysis.input.Mesh_Quiet_Flag = True

        # Run
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()
        
        # Assert AnalysisOutVals
        self.assertTrue(myAnalysis.output.Done)

        numNodes = myAnalysis.output.NumberOfNode
        self.assertGreater(numNodes, 0)
        numElements = myAnalysis.output.NumberOfElement
        self.assertGreater(numElements, 0)

    def test_MultiBody(self):

        file = os.path.join("..","csmData","cfdMultiBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        myAnalysis = myProblem.analysis.create(aim = "aflr4AIM")

        myAnalysis.input.Mesh_Quiet_Flag = True

        # Run
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()

    def test_reenter(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        myAnalysis = myProblem.analysis.create(aim = "aflr4AIM")

        myAnalysis.input.Mesh_Quiet_Flag = True

        myAnalysis.input.Mesh_Length_Factor = 1

        # Run 1st time
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()

        myAnalysis.input.Mesh_Length_Factor = 2

        # Run 2nd time coarser
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()


    def test_box(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "Box",
                                           capsIntent = ["box", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

    def test_cylinder(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "Cylinder",
                                           capsIntent = ["cylinder", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

    def test_cone(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "Cone",
                                           capsIntent = ["cone", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

    def test_torus(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "Torus",
                                           capsIntent = ["torus", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

        #aflr4.viewGeometry()

    def test_sphere(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "Sphere",
                                           capsIntent = ["sphere", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

        #aflr4.viewGeometry()

    def off_test_boxhole(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "BoxHole",
                                           capsIntent = ["boxhole", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

        #aflr4.viewGeometry()

    def test_bullet(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "Bullet",
                                           capsIntent = ["bullet", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

        #aflr4.viewGeometry()

    def test_all(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                           name = "All",
                                           capsIntent = ["box", "cylinder", "cone", "torus", "sphere", "farfield"]) #, "boxhole"

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Just make sure it runs without errors...
        aflr4.preAnalysis()
        aflr4.postAnalysis()

        #aflr4.viewGeometry()


if __name__ == '__main__':
    unittest.main()
