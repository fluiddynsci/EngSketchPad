# Import other need modules
from __future__ import print_function

import unittest
import os
import argparse
import platform
import time

# Import pyCAPS class file
import pyCAPS
from pyCAPS import capsProblem

# Helper function to check if an executable exists
def which(program):
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None

class TestPointwise(unittest.TestCase):

    @classmethod
    def setUpClass(cls):

        # Initialize capsProblem object
        cls.myProblem = capsProblem()

        # Working directory prefix
        cls.workDir = "workDir_PointwiseTest"

        # Load CSM file
        cls.myGeometry = cls.myProblem.loadCAPS(os.path.join("..","csmData","cornerGeom.csm"))

    # This is executed prior to each test
    def setUp(self):
        if platform.system() == "Windows":
            try:
                PW_INSTALL = os.environ["PW_HOME"]
            except:
                self.skipTest("No pointwise executable")
        else:
            if which("pointwise") == None:
                self.skipTest("No pointwise executable")

    def run_pointwise(self, pointwise):

        # Run AIM pre-analysis
        pointwise.preAnalysis()

        ####### Run pointwise ####################
        print ("\n\nRunning pointwise......")
        currentDirectory = os.getcwd() # Get our current working directory

        os.chdir(pointwise.analysisDir) # Move into test directory

        # Run pointwise via system call
        # try up to 30 times
        CAPS_GLYPH = os.environ["CAPS_GLYPH"]
        for i in range(30):
            if platform.system() == "Windows":
                PW_HOME = os.environ["PW_HOME"]
                os.system(PW_HOME + "\win64\bin\tclsh.exe " + CAPS_GLYPH + "\\GeomToMesh.glf caps.egads capsUserDefaults.glf")
            else:
                os.system("pointwise -b " + CAPS_GLYPH + "/GeomToMesh.glf caps.egads capsUserDefaults.glf")

            time.sleep(1) # let the harddrive breathe
            if os.path.isfile('caps.GeomToMesh.gma') and os.path.isfile('caps.GeomToMesh.ugrid'): break
            time.sleep(10) # wait and try again

        os.chdir(currentDirectory) # Move back to top directory

        # make sure the execution was successful
        self.assertTrue(os.path.isfile(os.path.join(pointwise.analysisDir,'caps.GeomToMesh.gma')) and
                        os.path.isfile(os.path.join(pointwise.analysisDir,'caps.GeomToMesh.ugrid')))

        # Run AIM post-analysis
        pointwise.postAnalysis()

    def test_executeError(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "BoxError",
                                           capsIntent = ["box", "farfield"])
        # Run AIM post-analysis
        pointwise.preAnalysis()

        # Check for error when pointwise was not executed
        with self.assertRaises(pyCAPS.CAPSError) as e:
            pointwise.postAnalysis()

        self.assertEqual(e.exception.errorName, "CAPS_IOERR")

    def test_SingleBody(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        pointwise = myProblem.loadAIM(aim = "pointwiseAIM",
                                      analysisDir = self.workDir + "SingleBody")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 10)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 12)

        # Run
        self.run_pointwise(pointwise)

    def test_MultiBody(self):

        file = os.path.join("..","csmData","cfdMultiBody.csm")
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        pointwise = myProblem.loadAIM(aim = "pointwiseAIM",
                                      analysisDir = self.workDir + "MultiBody")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 10)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 12)

        # Run
        self.run_pointwise(pointwise)

    def test_reenter(self):

        file = "../csmData/cfdSingleBody.csm"
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        pointwise = myProblem.loadAIM(aim = "pointwiseAIM",
                                      analysisDir = self.workDir + "SingleBody2")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 10)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 12)

        # Run 1st time
        self.run_pointwise(pointwise)

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Run 2nd time coarser
        self.run_pointwise(pointwise)


    def test_box(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "Box",
                                           capsIntent = ["box", "farfield"])

        #pointwise.saveGeometry("Box")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)

    def off_test_cylinder(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "Cylinder",
                                           capsIntent = ["cylinder", "farfield"])

        #pointwise.saveGeometry("Cylinder")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)


    def test_cone(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "Cone",
                                           capsIntent = ["cone", "farfield"])

        #pointwise.saveGeometry("Cone")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)

    def off_test_torus(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "Torus",
                                           capsIntent = ["torus", "farfield"])

        #pointwise.saveGeometry("Torus")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)

    def test_sphere(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "Sphere",
                                           capsIntent = ["sphere", "farfield"])

        #pointwise.saveGeometry("Sphere")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)

        #pointwise.viewGeometry()

    def off_test_boxhole(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "BoxHole",
                                           capsIntent = ["boxhole", "farfield"])

        #pointwise.saveGeometry("BoxHole")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)

    def off_test_bullet(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "Bullet",
                                           capsIntent = ["bullet", "farfield"])

        #pointwise.saveGeometry("Bullet")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)

    def test_all(self):

        # Load pointwise aim
        pointwise = self.myProblem.loadAIM(aim = "pointwiseAIM",
                                           analysisDir = self.workDir + "All",
                                           capsIntent = ["box", "cone", "sphere", "farfield"]) #, "torus", "cylinder", "boxhole"

        #pointwise.saveGeometry("GeomAll")

        # Global Min/Max number of points on edges
        pointwise.setAnalysisVal("Connector_Initial_Dim", 6)
        pointwise.setAnalysisVal("Connector_Min_Dim", 3)
        pointwise.setAnalysisVal("Connector_Max_Dim", 8)

        # Just make sure it runs without errors...
        self.run_pointwise(pointwise)

        #pointwise.viewGeometry()

    @classmethod
    def tearDownClass(cls):

        # Close CAPS - Optional
        cls.myProblem.closeCAPS()

if __name__ == '__main__':
    unittest.main()
