import unittest

import os

import pyCAPS

class TestValue(unittest.TestCase):

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

        cls.myValue = cls.myProblem.createValue("Altitude", [0.0, 30000.0, 60000.0], units="ft", limits=[0.0, 70000])

    @classmethod
    def tearDownClass(cls):

        # Remove analysis directories
        if os.path.exists(cls.analysisDir):
            os.rmdir(cls.analysisDir)

    # Value - object consistent
    def test_value(self):
        self.assertEqual(self.myValue, self.myProblem.value["Altitude"])

    # Value retrieval
    def test_createValue(self):

        self.assertEqual(self.myValue.name, "Altitude")
        self.assertEqual(self.myValue.value, [0.0, 30000.0, 60000.0])
        self.assertEqual(self.myValue.units, "ft")
        self.assertEqual(self.myValue.limits, [0.0, 70000])

    # Set values
    def test_setValue(self):

        myValue = self.myProblem.createValue("Altitude2", [0.0, 30000.0, 60000.0], units="ft", limits=[0.0, 70000])

        myValue.value = [0.0, 40000.0, 50000]
        self.assertEqual(myValue.value, [0.0, 40000.0, 50000])

        myValue.setVal([0.0, 45000.0, 55000])
        self.assertEqual(myValue.value, [0.0, 45000.0, 55000])

    # Get value
    def test_getVal(self):

        self.assertEqual(self.myValue.getVal(), [0.0, 30000.0, 60000.0])


    # Set limits
    def test_limits(self):

        myValue = self.myProblem.createValue("Altitude3", [0.0, 30000.0, 60000.0], units="ft", limits=[0.0, 70000])

        myValue.limits = [0.0, 80000.0]
        self.assertEqual(myValue.limits, [0.0, 80000.0])

        myValue.setLimits([0.0, 63000.0])
        self.assertEqual(myValue.limits, [0.0, 63000.0])

    # Get limits
    def test_getLimits(self):

        self.assertEqual(self.myValue.getLimits(), [0.0, 70000])

    # Convert units
    def test_convertUnits(self):

        convertValue = self.myValue.convertUnits("m")
        self.assertAlmostEqual(convertValue[2], 18288.0)

    # Array of strings
    def test_arrayString(self):

        myValue = self.myProblem.createValue("Strings", ["test1", "test2", "test3"])
        self.assertEqual(myValue.value, ["test1", "test2", "test3"])

    # Change the shape/length of fixed value
    def test_changefixedShapeAndLength(self):

        with self.assertRaises(pyCAPS.CAPSError) as e:
            self.myValue.value = 0.0

        self.assertEqual(e.exception.errorName, "CAPS_SHAPEERR")

    # Change the length of fixed shape value
    def test_changefixedShape(self):

        myValue = self.myProblem.createValue("FixedShape", [0.0, 30000.0, 60000.0], fixedLength=False)

        myValue.value = [0.0, 1.0]
        self.assertEqual(myValue.value, [0.0, 1.0])

    # Change the shape shape value
    def test_changefixedShape2(self):

        myValue = self.myProblem.createValue("FixedShape2", [0.0, 30000.0, 60000.0], fixedLength=False)
        with self.assertRaises(pyCAPS.CAPSError) as e:
            myValue.value = [[0.0, 1.0], [2.0, 3.0]]

        self.assertEqual(e.exception.errorName, "CAPS_SHAPEERR")

    # Change the length and shape
    def test_changeLengthAndShape(self):

        myValue = self.myProblem.createValue("FreeShape", 0.0, fixedLength = False, fixedShape=False)

        myValue.value = [0.0, 1.0]
        self.assertEqual(myValue.value, [0.0, 1.0])

    # Autolink
    def test_autolink(self):

        myValue = self.myProblem.createValue("Beta", 1.0, units="degree")
        self.myProblem.autoLinkValue("Beta")

        myValue = self.myProblem.createValue("Alpha", 10.0, units="degree")
        self.myProblem.autoLinkValue(myValue)

        # Run preAnalysis to force the value update
        self.myAnalysis.preAnalysis()

        self.assertEqual(self.myAnalysis.getAnalysisVal("Beta"), 1.0)
        self.assertEqual(self.myAnalysis.getAnalysisVal("Alpha"), 10.0)

    def test_matrixValue(self):

        value = self.myProblem.createValue("matrixValue", [[1,2], [3,4], [5,6]])
        self.assertEqual(value.value, [[1,2], [3,4], [5,6]])

    def test_rowValue(self):

        value = self.myProblem.createValue("rowValue", [1,2,3])
        self.assertEqual(value.value, [1,2,3])

    def test_colValue(self):

        # Should get convert back to a row
        value = self.myProblem.createValue("colValue", [[1,2,3]])
        self.assertEqual(value.value, [1,2,3])

    def test_inconsistentValueShape(self):

        with self.assertRaises(ValueError):
            value = self.myProblem.createValue("inconValue", [[1,2,3], [0]])

if __name__ == '__main__':
    unittest.main()
