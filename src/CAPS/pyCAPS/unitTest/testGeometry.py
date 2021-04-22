import unittest

import os

from sys import version_info as pyVersion

import pyCAPS

class TestGeometry(unittest.TestCase):

    @classmethod
    def setUpClass(cls):

        cls.file = "unitGeom.csm"
        cls.analysisDir = "UnitTest"

        cls.myProblem = pyCAPS.capsProblem()

        cls.myGeometry = cls.myProblem.loadCAPS(cls.file, "basicTest", verbosity=0)

    @classmethod
    def tearDownClass(cls):

        # Remove analysis directories
        if os.path.exists(cls.analysisDir):
            os.rmdir(cls.analysisDir)

        # Remove created files
        if os.path.isfile("myGeometry.egads"):
            os.remove("myGeometry.egads")

        if os.path.isfile("unitGeom.egads"):
            os.remove("unitGeom.egads")

        if os.path.isfile("myGeometry.html"):
            os.remove("myGeometry.html")

        if os.path.isfile("geomView_0.png"):
            os.remove("geomView_0.png")

        if os.path.isfile("geomView_1.png"):
            os.remove("geomView_1.png")

        if os.path.isfile("geomView_2.png"):
            os.remove("geomView_2.png")

    # Try loading two geometries into the problem
    def test_loadCAPS(self):

        with self.assertRaises(pyCAPS.CAPSError) as e:
            myGeometry = self.myProblem.loadCAPS(self.file, verbosity=0)

        self.assertEqual(e.exception.errorName, "CAPS_BADVALUE")

    # Retrieve geometry attribute
    def test_getAttributeVal(self):

        attributeList = self.myGeometry.getAttributeVal("capsGroup", attrLevel="Body")
        self.assertEqual(attributeList.sort(), ['Farfield', 'Wing2', 'Wing1'].sort())

        attributeList = self.myGeometry.getAttributeVal("capsGroup", attrLevel="5")
        self.assertEqual(attributeList.sort(), ['Farfield', 'Wing2', 'Wing1'].sort())

        attributeList = self.myGeometry.getAttributeVal("capsGroup", attrLevel="Node")
        self.assertEqual(attributeList.sort(), ['Farfield', 'Wing2', 'Wing1'].sort())

        # Check CSYSTEM attribute on body
        attributeList = self.myGeometry.getAttributeVal("leftWingSkin", attrLevel="Body")

        self.assertEqual(len(attributeList), 3)
        for j in range(3):
            self.assertEqual(len(attributeList[j]), 3)

        leftWingSkin = [[ 3.77246051677845,  0               ,  0.004591314188236],
                        [10.92812876825235, -2.04493123703332, 26.203935380022607],
                        [-0.44706657917624, -7.07106781186547, -0.365374367002283]]
        for j in range(3):
            for i in range(3):
                self.assertAlmostEqual(attributeList[j][i], leftWingSkin[j][i], 5)


    # Retrieve geometry attribute maps
    def test_getAttributeMap(self):

        attributeMap = self.myGeometry.getAttributeMap()
        Body1 = attributeMap['Body 1']
        Body2 = attributeMap['Body 2']

        keys = list(Body1['Body'].keys())
        self.assertEqual(keys.sort(), ['capsGroup', 'capsReferenceArea', '.tParam', 'capsIntent', 'capsAIM', 'capsReferenceSpan', 'leftWingSkin', 'riteWingSkin', 'capsReferenceChord'].sort())

        keys = list(Body1['Faces'].keys())
        self.assertEqual(keys, [1, 2, 3, 4, 5, 6, 7, 8])

        self.assertEqual(Body1['Faces'][1]['capsGroup'], 'Wing1')

        keys = list(Body1['Edges'].keys())
        self.assertEqual(keys, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        keys = list(Body1['Nodes'].keys())
        self.assertEqual(keys, [1, 2, 3, 4, 5, 6, 7, 8, 9])

        keys = list(Body2['Body'].keys())
        self.assertEqual(keys.sort(), ['capsGroup', 'capsReferenceArea', '.tParam', 'capsIntent', 'capsAIM', 'capsReferenceSpan', 'leftWingSkin', 'riteWingSkin', 'capsReferenceChord'].sort())

        keys = list(Body2['Faces'].keys())
        self.assertEqual(keys, [1, 2, 3, 4, 5, 6, 7, 8])

        keys = list(Body2['Edges'].keys())
        self.assertEqual(keys, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        keys = list(Body2['Nodes'].keys())
        self.assertEqual(keys, [1, 2, 3, 4, 5, 6, 7, 8, 9])

        # check the reverse map
        attributeMap = self.myGeometry.getAttributeMap(reverseMap = True)
        Body1 = attributeMap['Body 1']
        Body2 = attributeMap['Body 2']
        Body3 = attributeMap['Body 3']

        keys = list(Body1.keys())
        self.assertEqual(keys.sort(), ['capsGroup', 'capsReferenceArea', '.tParam', 'capsIntent', 'capsAIM', 'capsReferenceSpan', 'leftWingSkin', 'riteWingSkin', 'capsReferenceChord'].sort())

        Wing1 = Body1['capsGroup']['Wing1']
        self.assertEqual(Wing1, {'Body': True, 'Nodes': [], 'Edges': [], 'Faces': [1, 2, 3, 4, 5, 6, 7, 8]})

        capsAIM = Body1['capsAIM']
        self.assertEqual(capsAIM, {'fun3dAIM;su2AIM;egadsTessAIM;aflr4AIM;tetgenAIM;aflr3AIM': {'Body': True, 'Nodes': [], 'Edges': [], 'Faces': []}})

        Wing2 = Body2['capsGroup']['Wing2']
        self.assertEqual(Wing2, {'Body': True, 'Nodes': [], 'Edges': [], 'Faces': [1, 2, 3, 4, 5, 6, 7, 8]})

        capsAIM = Body2['capsAIM']
        self.assertEqual(capsAIM, {'fun3dAIM;su2AIM;egadsTessAIM;aflr4AIM;tetgenAIM;aflr3AIM': {'Body': True, 'Nodes': [], 'Edges': [], 'Faces': []}})

        Farfield = Body3['capsGroup']['Farfield']
        self.assertEqual(Farfield, {'Body': True, 'Nodes': [], 'Edges': [], 'Faces': [1, 2]})

        capsAIM = Body3['capsAIM']
        self.assertEqual(capsAIM, {'fun3dAIM;su2AIM;egadsTessAIM;aflr4AIM;tetgenAIM;aflr3AIM': {'Body': True, 'Nodes': [], 'Edges': [], 'Faces': []}})

    # Set/get geometry value
    def test_setGeometryVal(self):

        self.myGeometry.setGeometryVal("lesweep", 20.0)
        self.assertEqual(self.myGeometry.getGeometryVal("lesweep"), 20.0)

    # Build geometry - force regeneration
    def test_buildGeometryVal(self):

        self.myGeometry.setGeometryVal("lesweep", 12.0)
        self.myGeometry.buildGeometry()

    # Get geometry in values
    def test_getGeometryVal(self):

        # Get all input values
        valueDict = self.myGeometry.getGeometryVal()
        self.assertEqual(valueDict["aspect"], 5.0)

        self.assertEqual(valueDict["despMat"],[[11,12],[13,14], [15,16]])

        # Get just the names of the input values
        valueList = self.myGeometry.getGeometryVal(namesOnly = True)
        self.assertEqual("twist" in valueList, True)

    # Get geometry out values
    def test_getGeometryOutVal(self):

        # Get all output values
        valueDict = self.myGeometry.getGeometryOutVal()
        self.assertAlmostEqual(valueDict["span"], 14.14, 2)
        self.assertAlmostEqual(valueDict["cmean"], 2.82843, 5)

        self.assertEqual(valueDict["dummyRow"],[1,2,3])
        self.assertEqual(valueDict["dummyCol"],[3,2,1])
        self.assertEqual(valueDict["dummyMat"],[[1,2],[3,4], [5,6]])

        # Get just the names of the output values
        valueList = self.myGeometry.getGeometryOutVal(namesOnly = True)
        self.assertEqual("cmean" in valueList, True)

    # Get geometry out values - if geometry has not been set
    def test_getGeometryOutVal_NotSet(self):

        # Not set with OutParam
        with self.assertRaises(pyCAPS.CAPSError) as e:
            self.myGeometry.getGeometryOutVal("xtip")

        self.assertEqual(e.exception.errorName, "CAPS_NOTFOUND")

        # Geometry not built yet
        #myProblem = pyCAPS.capsProblem()
        #myGeometry = myProblem.loadCAPS(self.file, verbosity=0)

        #cmean = myGeometry.getGeometryOutVal("cmean")
        #self.assertEqual(cmean, None)

        #valueDict = myGeometry.getGeometryOutVal()
        #self.assertEqual(valueDict["span"], None)

    # Geometry - object consistent
    def test_geometry(self):

        self.assertEqual(self.myGeometry, self.myProblem.geometry)

    # View geometry
    def test_viewGeometry(self):

        try:

            self.myGeometry.viewGeometry(filename = "geomView", showImage =False)

            self.assertTrue(os.path.isfile("geomView_0.png"))
            self.assertTrue(os.path.isfile("geomView_1.png"))
            self.assertTrue(os.path.isfile("geomView_2.png"))

        except ImportError as e: # The machine doesn't have matplotlib
            self.assertTrue(True)

        except pyCAPS.CAPSError as e:
            raise e
        except: # Ignore other errors  TclError being thrown up by matplotlib on some machines
            pass


    # Save geometry
    def test_saveGeometry(self):

        self.myGeometry.saveGeometry()
        self.assertTrue(os.path.isfile("myGeometry.egads"))

    # Create html tree
    @unittest.skipIf(pyVersion.major > 2, "Odd behavior in Python 3")
    def test_createTree(self):

        self.myGeometry.createTree(internetAccess = True,
                                   embedJSON = True,
                                   reverseMap = True)
        self.assertTrue(os.path.isfile("myGeometry.html"))

    # Get parameters from EGADS file
    def test_staticGeometry(self):

        # Make sure geometry is built to dump unitGeom.egads (should be replaced with saveGeometry)
        self.myGeometry.buildGeometry()

        # Make a new problem and load the egads file
        egProblem = pyCAPS.capsProblem()

        egGeometry = egProblem.loadCAPS("unitGeom.egads", "staticGeomTest", verbosity=0)

        self.assertEqual(egGeometry.getGeometryVal("aspect"), 5.0)
        self.assertEqual(egGeometry.getGeometryVal("series"), 8412)

        valueDict = egGeometry.getGeometryOutVal()
        self.assertAlmostEqual(valueDict["span"], 14.14, 2)
        self.assertAlmostEqual(valueDict["cmean"], 2.82843, 5)

        # Parameters are now read only because we started from an egads file
        with self.assertRaises(pyCAPS.CAPSError) as e:
            egGeometry.setGeometryVal("lesweep", 20.0)

        self.assertEqual(e.exception.errorName, "CAPS_READONLYERR")

if __name__ == '__main__':
    unittest.main()
