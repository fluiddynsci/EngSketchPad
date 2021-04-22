# Import other need modules
from __future__ import print_function

import unittest
import os
import argparse

# Import pyCAPS class file
import pyCAPS
from pyCAPS import capsProblem


class TestMasstran(unittest.TestCase):

#    @classmethod
#    def setUpClass(self):
    def setUp(self):

        # Initialize capsProblem object
        self.myProblem = capsProblem()

        # Load CSM file
        self.myGeometry = self.myProblem.loadCAPS("../csmData/masstran.csm")

        # Load masstran aim
        self.masstran = self.myProblem.loadAIM(aim = "masstranAIM")

        # Min/Max number of points on an edge for quadding
        self.masstran.setAnalysisVal("Edge_Point_Min", 2)
        self.masstran.setAnalysisVal("Edge_Point_Max", 20)

        #self.masstran.setAnalysisVal("Tess_Params", [0.25, 0.001, 15.0])

        # Generate quad meshes
        self.masstran.setAnalysisVal("Quad_Mesh", 1)

        # Set materials
        madeupium    = {"materialType" : "isotropic",
                        "density"      : 10}
        unobtainium  = {"materialType" : "isotropic",
                        "density"      : 20}

        self.masstran.setAnalysisVal("Material", [("madeupium", madeupium),("unobtainium",unobtainium)])

        # Set properties
        self.shell1 = {"propertyType"      : "Shell",
                       "membraneThickness" : 2.0,
                       "material"          : "madeupium"}

        self.shell2 = {"propertyType"      : "Shell",
                       "membraneThickness" : 3.0,
                       "material"          : "unobtainium"}

    def test_plate(self):

        # set the propery for this analysis
        self.masstran.setAnalysisVal("Property", ("box1", self.shell1))

        # Only work with plate
        self.myGeometry.setGeometryVal("VIEW:plate", 1)
        self.myGeometry.setGeometryVal("VIEW:box1", 0)
        self.myGeometry.setGeometryVal("VIEW:box2", 0)
        self.myGeometry.setGeometryVal("VIEW:box3", 0)

        # compute Mass properties
        self.masstran.preAnalysis()
        self.masstran.postAnalysis()

        # properties of 1 plate
        Area     = self.masstran.getAnalysisOutVal("Area")
        Mass     = self.masstran.getAnalysisOutVal("Mass")
        Centroid = self.masstran.getAnalysisOutVal("Centroid")
        CG       = self.masstran.getAnalysisOutVal("CG")
        I        = self.masstran.getAnalysisOutVal("I_Vector")
        II       = self.masstran.getAnalysisOutVal("I_Tensor")

        self.assertEqual(3, len(Centroid))
        self.assertEqual(3, len(CG))
        self.assertEqual(6, len(I))
        self.assertEqual(3, len(II))
        for row in II:
            self.assertEqual(3, len(row))

        Ixx = I[0]; Iyy = I[1]; Izz = I[2]
        Ixy = I[3]; Ixz = I[4]; Iyz = I[5]

        areaTrue = 2*3
        m = 2*10*areaTrue # Mass of the plate
        IxxTrue = 1./12.*m*3**2
        IyyTrue = 1./12.*m*2**2
        IzzTrue = 1./12.*m*(2**2 + 3**2)

        self.assertAlmostEqual(areaTrue, Area, 9)
        self.assertAlmostEqual(m, Mass, 9)

        self.assertAlmostEqual(1.0, Centroid[0], 9)
        self.assertAlmostEqual(1.5, Centroid[1], 9)
        self.assertAlmostEqual(0.0, Centroid[2], 9)

        self.assertAlmostEqual(1.0, CG[0], 9)
        self.assertAlmostEqual(1.5, CG[1], 9)
        self.assertAlmostEqual(0.0, CG[2], 9)

        self.assertAlmostEqual(Ixx/IxxTrue, 1.0, 2)
        self.assertAlmostEqual(Iyy/IyyTrue, 1.0, 2)
        self.assertAlmostEqual(Izz/IzzTrue, 1.0, 2)

        self.assertAlmostEqual(0.0, Ixy, 9)
        self.assertAlmostEqual(0.0, Ixz, 9)
        self.assertAlmostEqual(0.0, Iyz, 9)

        self.assertAlmostEqual(Ixx, self.masstran.getAnalysisOutVal("Ixx"), 9)
        self.assertAlmostEqual(Iyy, self.masstran.getAnalysisOutVal("Iyy"), 9)
        self.assertAlmostEqual(Izz, self.masstran.getAnalysisOutVal("Izz"), 9)
        self.assertAlmostEqual(Ixy, self.masstran.getAnalysisOutVal("Ixy"), 9)
        self.assertAlmostEqual(Ixz, self.masstran.getAnalysisOutVal("Ixz"), 9)
        self.assertAlmostEqual(Iyz, self.masstran.getAnalysisOutVal("Iyz"), 9)

        self.assertAlmostEqual( Ixx, II[0][0], 9)
        self.assertAlmostEqual(-Ixy, II[0][1], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Ixy, II[1][0], 9)
        self.assertAlmostEqual( Iyy, II[1][1], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual( Izz, II[2][2], 9)


    def test_box1(self):

        # set the propery for this analysis
        self.masstran.setAnalysisVal("Property", ("box1", self.shell1))

        # Only work with 1 box now
        self.myGeometry.setGeometryVal("VIEW:plate", 0)
        self.myGeometry.setGeometryVal("VIEW:box1", 1)
        self.myGeometry.setGeometryVal("VIEW:box2", 0)
        self.myGeometry.setGeometryVal("VIEW:box3", 0)

        # compute Mass properties
        self.masstran.preAnalysis()
        self.masstran.postAnalysis()

        # properties of 1 box
        Area     = self.masstran.getAnalysisOutVal("Area")
        Mass     = self.masstran.getAnalysisOutVal("Mass")
        Centroid = self.masstran.getAnalysisOutVal("Centroid")
        CG       = self.masstran.getAnalysisOutVal("CG")
        I        = self.masstran.getAnalysisOutVal("I_Vector")
        II       = self.masstran.getAnalysisOutVal("I_Tensor")

        self.assertEqual(3, len(Centroid))
        self.assertEqual(3, len(CG))
        self.assertEqual(6, len(I))
        self.assertEqual(3, len(II))
        for row in II:
            self.assertEqual(3, len(row))

        Ixx = I[0]; Iyy = I[1]; Izz = I[2]
        Ixy = I[3]; Ixz = I[4]; Iyz = I[5]

        nFace = 6
        areaTrue = 1
        m = 2*10 # Mass of one face of the box
        massTrue = m*nFace
        
        # CG location of each face
        fCG = {0:[0.0, 0.5, 0.5], 1:[1.0, 0.5, 0.5], # x-min/max
               2:[0.5, 0.0, 0.5], 3:[0.5, 1.0, 0.5], # y-min/max
               4:[0.5, 0.5, 0.0], 5:[0.5, 0.5, 1.0]} # z-min/max

        # length of each face
        fLen = {0:[0.0, 1.0, 1.0], 1:[0.0, 1.0, 1.0], # x-min/max
                2:[1.0, 0.0, 1.0], 3:[1.0, 0.0, 1.0], # y-min/max
                4:[1.0, 1.0, 0.0], 5:[1.0, 1.0, 0.0]} # z-min/max

        IxxTrue = 0
        IyyTrue = 0
        IzzTrue = 0
        for i in range(6):
            IxxTrue += m*(fCG[i][1]**2 + fCG[i][2]**2) + 1/12.*m*(fLen[i][1]**2 + fLen[i][2]**2)
            IyyTrue += m*(fCG[i][0]**2 + fCG[i][2]**2) + 1/12.*m*(fLen[i][0]**2 + fLen[i][2]**2)
            IzzTrue += m*(fCG[i][0]**2 + fCG[i][1]**2) + 1/12.*m*(fLen[i][0]**2 + fLen[i][1]**2)

        IxxTrue -= massTrue*(CG[1]**2 + CG[2]**2)
        IyyTrue -= massTrue*(CG[0]**2 + CG[2]**2)
        IzzTrue -= massTrue*(CG[0]**2 + CG[1]**2)

        self.assertAlmostEqual(nFace*areaTrue, Area, 9)
        self.assertAlmostEqual(nFace*m, Mass, 9)

        self.assertAlmostEqual(0.5, Centroid[0], 9)
        self.assertAlmostEqual(0.5, Centroid[1], 9)
        self.assertAlmostEqual(0.5, Centroid[2], 9)

        self.assertAlmostEqual(0.5, CG[0], 9)
        self.assertAlmostEqual(0.5, CG[1], 9)
        self.assertAlmostEqual(0.5, CG[2], 9)

        self.assertAlmostEqual(Ixx/IxxTrue, 1.0, 2)
        self.assertAlmostEqual(Iyy/IyyTrue, 1.0, 2)
        self.assertAlmostEqual(Izz/IzzTrue, 1.0, 2)

        self.assertAlmostEqual(0.0, Ixy, 9)
        self.assertAlmostEqual(0.0, Ixz, 9)
        self.assertAlmostEqual(0.0, Iyz, 9)

        self.assertAlmostEqual(Ixx, self.masstran.getAnalysisOutVal("Ixx"), 9)
        self.assertAlmostEqual(Iyy, self.masstran.getAnalysisOutVal("Iyy"), 9)
        self.assertAlmostEqual(Izz, self.masstran.getAnalysisOutVal("Izz"), 9)
        self.assertAlmostEqual(Ixy, self.masstran.getAnalysisOutVal("Ixy"), 9)
        self.assertAlmostEqual(Ixz, self.masstran.getAnalysisOutVal("Ixz"), 9)
        self.assertAlmostEqual(Iyz, self.masstran.getAnalysisOutVal("Iyz"), 9)

        self.assertAlmostEqual( Ixx, II[0][0], 9)
        self.assertAlmostEqual(-Ixy, II[0][1], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Ixy, II[1][0], 9)
        self.assertAlmostEqual( Iyy, II[1][1], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual( Izz, II[2][2], 9)


    def test_box2(self):
 
        # set the propery for this analysis
        self.masstran.setAnalysisVal("Property", ("box1", self.shell1))

        # Only work with 2 box now
        self.myGeometry.setGeometryVal("VIEW:plate", 0)
        self.myGeometry.setGeometryVal("VIEW:box1", 0)
        self.myGeometry.setGeometryVal("VIEW:box2", 1)
        self.myGeometry.setGeometryVal("VIEW:box3", 0)
 
        # compute Mass properties
        self.masstran.preAnalysis()
        self.masstran.postAnalysis()
 
        # properties of 1 box lacking 1 face
        Area     = self.masstran.getAnalysisOutVal("Area")
        Mass     = self.masstran.getAnalysisOutVal("Mass")
        Centroid = self.masstran.getAnalysisOutVal("Centroid")
        CG       = self.masstran.getAnalysisOutVal("CG")
        I        = self.masstran.getAnalysisOutVal("I_Vector")
        II       = self.masstran.getAnalysisOutVal("I_Tensor")
 
        self.assertEqual(3, len(Centroid))
        self.assertEqual(3, len(CG))
        self.assertEqual(6, len(I))
        self.assertEqual(3, len(II))
        for row in II:
            self.assertEqual(3, len(row))

        Ixx = I[0]; Iyy = I[1]; Izz = I[2]
        Ixy = I[3]; Ixz = I[4]; Iyz = I[5]

        nFace = 5
        areaTrue = 1
        m = 2*10 # Mass of one face of the box
        massTrue = m*nFace
        
        # CG location of each face
        fCG = {0:[2.0, 0.5, 0.5], 1:[3.0, 0.5, 0.5], # x-min/max
               2:[2.5, 0.0, 0.5], 3:[2.5, 1.0, 0.5], # y-min/max
               4:[2.5, 0.5, 0.0], 5:[2.5, 0.5, 1.0]} # z-min/max

        # length of each face
        fLen = {0:[0.0, 1.0, 1.0], 1:[0.0, 1.0, 1.0], # x-min/max
                2:[1.0, 0.0, 1.0], 3:[1.0, 0.0, 1.0], # y-min/max
                4:[1.0, 1.0, 0.0], 5:[1.0, 1.0, 0.0]} # z-min/max

        IxxTrue = 0
        IyyTrue = 0
        IzzTrue = 0
        for i in range(1,6):
            IxxTrue += m*(fCG[i][1]**2 + fCG[i][2]**2) + 1/12.*m*(fLen[i][1]**2 + fLen[i][2]**2)
            IyyTrue += m*(fCG[i][0]**2 + fCG[i][2]**2) + 1/12.*m*(fLen[i][0]**2 + fLen[i][2]**2)
            IzzTrue += m*(fCG[i][0]**2 + fCG[i][1]**2) + 1/12.*m*(fLen[i][0]**2 + fLen[i][1]**2)

        IxxTrue -= massTrue*(CG[1]**2 + CG[2]**2)
        IyyTrue -= massTrue*(CG[0]**2 + CG[2]**2)
        IzzTrue -= massTrue*(CG[0]**2 + CG[1]**2)
        
        self.assertAlmostEqual(nFace*areaTrue, Area, 9)
        self.assertAlmostEqual(massTrue, Mass, 9)
 
        # compute centroid as averge of last 5 faces plus the shift by 2
        Cx = 2+(4*0.5+1)/nFace
 
        self.assertAlmostEqual(Cx , Centroid[0], 9)
        self.assertAlmostEqual(0.5, Centroid[1], 9)
        self.assertAlmostEqual(0.5, Centroid[2], 9)
 
        self.assertAlmostEqual(Cx,  CG[0], 9)
        self.assertAlmostEqual(0.5, CG[1], 9)
        self.assertAlmostEqual(0.5, CG[2], 9)

        self.assertAlmostEqual(Ixx/IxxTrue, 1.0, 2)
        self.assertAlmostEqual(Iyy/IyyTrue, 1.0, 2)
        self.assertAlmostEqual(Izz/IzzTrue, 1.0, 2)

        self.assertAlmostEqual(0.0, Ixy, 4)
        self.assertAlmostEqual(0.0, Ixz, 4)
        self.assertAlmostEqual(0.0, Iyz, 4)

        self.assertAlmostEqual(Ixx, self.masstran.getAnalysisOutVal("Ixx"), 9)
        self.assertAlmostEqual(Iyy, self.masstran.getAnalysisOutVal("Iyy"), 9)
        self.assertAlmostEqual(Izz, self.masstran.getAnalysisOutVal("Izz"), 9)
        self.assertAlmostEqual(Ixy, self.masstran.getAnalysisOutVal("Ixy"), 9)
        self.assertAlmostEqual(Ixz, self.masstran.getAnalysisOutVal("Ixz"), 9)
        self.assertAlmostEqual(Iyz, self.masstran.getAnalysisOutVal("Iyz"), 9)

        self.assertAlmostEqual( Ixx, II[0][0], 9)
        self.assertAlmostEqual(-Ixy, II[0][1], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Ixy, II[1][0], 9)
        self.assertAlmostEqual( Iyy, II[1][1], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual( Izz, II[2][2], 9)


    def test_box3(self):
 
        # set the propery for this analysis
        self.masstran.setAnalysisVal("Property", [("box1", self.shell1),("box2", self.shell2)])

        # Only work with 3 box now
        self.myGeometry.setGeometryVal("VIEW:plate", 0)
        self.myGeometry.setGeometryVal("VIEW:box1", 0)
        self.myGeometry.setGeometryVal("VIEW:box2", 0)
        self.myGeometry.setGeometryVal("VIEW:box3", 1)
 
        # compute Mass properties
        self.masstran.preAnalysis()
        self.masstran.postAnalysis()
 
        # properties of 1 box lacking 1 face
        Area     = self.masstran.getAnalysisOutVal("Area")
        Mass     = self.masstran.getAnalysisOutVal("Mass")
        Centroid = self.masstran.getAnalysisOutVal("Centroid")
        CG       = self.masstran.getAnalysisOutVal("CG")
        I        = self.masstran.getAnalysisOutVal("I_Vector")
        II       = self.masstran.getAnalysisOutVal("I_Tensor")

        self.assertEqual(3, len(Centroid))
        self.assertEqual(3, len(CG))
        self.assertEqual(6, len(I))
        for row in II:
            self.assertEqual(3, len(row))

        Ixx = I[0]; Iyy = I[1]; Izz = I[2]
        Ixy = I[3]; Ixz = I[4]; Iyz = I[5]

        nFace = 6
        areaTrue = 1
        
        # Mass of one face of the box
        m = {0:2*10, 1:3*20,
             2:2*10, 3:3*20,
             4:2*10, 5:3*20}
        
        # total Mass
        massTrue = 0
        for i in range(6):
            massTrue += m[i]
        
        # CG location of each face
        fCG = {0:[4.0, 0.5, 0.5], 1:[5.0, 0.5, 0.5], # x-min/max
               2:[4.5, 0.0, 0.5], 3:[4.5, 1.0, 0.5], # y-min/max
               4:[4.5, 0.5, 0.0], 5:[4.5, 0.5, 1.0]} # z-min/max

        # length of each face
        fLen = {0:[0.0, 1.0, 1.0], 1:[0.0, 1.0, 1.0], # x-min/max
                2:[1.0, 0.0, 1.0], 3:[1.0, 0.0, 1.0], # y-min/max
                4:[1.0, 1.0, 0.0], 5:[1.0, 1.0, 0.0]} # z-min/max

        IxxTrue = 0
        IyyTrue = 0
        IzzTrue = 0
        IxyTrue = 0
        IxzTrue = 0
        IyzTrue = 0
        for i in range(6):
            IxxTrue += m[i]*(fCG[i][1]**2 + fCG[i][2]**2) + 1/12.*m[i]*(fLen[i][1]**2 + fLen[i][2]**2)
            IyyTrue += m[i]*(fCG[i][0]**2 + fCG[i][2]**2) + 1/12.*m[i]*(fLen[i][0]**2 + fLen[i][2]**2)
            IzzTrue += m[i]*(fCG[i][0]**2 + fCG[i][1]**2) + 1/12.*m[i]*(fLen[i][0]**2 + fLen[i][1]**2)

            IxyTrue += m[i]*(fCG[i][0]*fCG[i][1])
            IxzTrue += m[i]*(fCG[i][0]*fCG[i][2])
            IyzTrue += m[i]*(fCG[i][1]*fCG[i][2])

        IxxTrue -= massTrue*(CG[1]**2 + CG[2]**2)
        IyyTrue -= massTrue*(CG[0]**2 + CG[2]**2)
        IzzTrue -= massTrue*(CG[0]**2 + CG[1]**2)

        IxyTrue -= massTrue*(CG[0]*CG[1])
        IxzTrue -= massTrue*(CG[0]*CG[2])
        IyzTrue -= massTrue*(CG[1]*CG[2])

        self.assertAlmostEqual(nFace*areaTrue, Area, 9)
        self.assertAlmostEqual(massTrue, Mass, 9)
 
        self.assertAlmostEqual(4.5, Centroid[0], 9)
        self.assertAlmostEqual(0.5, Centroid[1], 9)
        self.assertAlmostEqual(0.5, Centroid[2], 9)
  
        CGx = 0
        CGy = 0
        CGz = 0
        for i in range(6):
            CGx += fCG[i][0]*m[i]
            CGy += fCG[i][1]*m[i]
            CGz += fCG[i][2]*m[i]
        CGx /= massTrue
        CGy /= massTrue
        CGz /= massTrue

        self.assertAlmostEqual(CGx, CG[0], 9)
        self.assertAlmostEqual(CGy, CG[1], 9)
        self.assertAlmostEqual(CGz, CG[2], 9)

        self.assertAlmostEqual(Ixx/IxxTrue, 1.0, 2)
        self.assertAlmostEqual(Iyy/IyyTrue, 1.0, 2)
        self.assertAlmostEqual(Izz/IzzTrue, 1.0, 2)

        self.assertAlmostEqual(IxyTrue, Ixy, 4)
        self.assertAlmostEqual(IxzTrue, Ixz, 4)
        self.assertAlmostEqual(IyzTrue, Iyz, 4)

        self.assertAlmostEqual(Ixx, self.masstran.getAnalysisOutVal("Ixx"), 9)
        self.assertAlmostEqual(Iyy, self.masstran.getAnalysisOutVal("Iyy"), 9)
        self.assertAlmostEqual(Izz, self.masstran.getAnalysisOutVal("Izz"), 9)
        self.assertAlmostEqual(Ixy, self.masstran.getAnalysisOutVal("Ixy"), 9)
        self.assertAlmostEqual(Ixz, self.masstran.getAnalysisOutVal("Ixz"), 9)
        self.assertAlmostEqual(Iyz, self.masstran.getAnalysisOutVal("Iyz"), 9)

        self.assertAlmostEqual( Ixx, II[0][0], 9)
        self.assertAlmostEqual(-Ixy, II[0][1], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Ixy, II[1][0], 9)
        self.assertAlmostEqual( Iyy, II[1][1], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual(-Ixz, II[0][2], 9)
        self.assertAlmostEqual(-Iyz, II[1][2], 9)
        self.assertAlmostEqual( Izz, II[2][2], 9)

#    @classmethod
#    def tearDownClass(self):
    def tearDown(self):

        # Close CAPS - Optional
        self.myProblem.closeCAPS()

if __name__ == '__main__':
    unittest.main()
