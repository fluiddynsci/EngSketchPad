from __future__ import print_function
import unittest

import os
import shutil

import pyCAPS

class Testaflr2(unittest.TestCase):


    @classmethod
    def setUpClass(cls):

        cls.analysisDir = "workDir_aflr2Test"

    @classmethod
    def tearDownClass(cls):

        # Remove analysis directories
        if os.path.exists(cls.analysisDir):
            shutil.rmtree(cls.analysisDir)

    def test_setAnalysisVals(self):

        file = "../csmData/cfd2D.csm"
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        myAnalysis = myProblem.loadAIM(aim = "aflr2AIM",
                                       analysisDir = self.analysisDir)

        myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_aflr2_Test")
        myAnalysis.setAnalysisVal("Tess_Params", [.25,.01,15])
        myAnalysis.setAnalysisVal("Mesh_Quiet_Flag", True)
        myAnalysis.setAnalysisVal("Mesh_Format", "Tecplot")
        myAnalysis.setAnalysisVal("Mesh_ASCII_Flag", True)
        myAnalysis.setAnalysisVal("Mesh_Gen_Input_String", "mquad=1 mpp=3")
        myAnalysis.setAnalysisVal("Edge_Point_Min", 5)
        myAnalysis.setAnalysisVal("Edge_Point_Max", 300)

        airfoil = {"numEdgePoints" : 100, "edgeDistribution" : "Tanh", "initialNodeSpacing" : [0.001, 0.001]}
        myAnalysis.setAnalysisVal("Mesh_Sizing", [("Airfoil"   , airfoil),
                                                  ("TunnelWall", {"numEdgePoints" : 50}),
                                                  ("InFlow",     {"numEdgePoints" : 25}),
                                                  ("OutFlow",    {"numEdgePoints" : 25}),
                                                  ("2DSlice",    {"tessParams" : [0.50, .01, 45]})])

        # Run
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()

    def test_reenter(self):

        file = "../csmData/cfd2D.csm"
        myProblem = pyCAPS.capsProblem()

        myGeometry = myProblem.loadCAPS(file)

        myAnalysis = myProblem.loadAIM(aim = "aflr2AIM",
                                       analysisDir = self.analysisDir)

        myAnalysis.setAnalysisVal("Mesh_Format", "Tecplot")

        airfoil = {"numEdgePoints" : 100, "edgeDistribution" : "Tanh", "initialNodeSpacing" : [0.001, 0.001]}
        myAnalysis.setAnalysisVal("Mesh_Sizing", [("Airfoil"   , airfoil),
                                                  ("TunnelWall", {"numEdgePoints" : 50}),
                                                  ("InFlow",     {"numEdgePoints" : 25}),
                                                  ("OutFlow",    {"numEdgePoints" : 25}),
                                                  ("2DSlice",    {"tessParams" : [0.50, .01, 45]})])

        myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_aflr2_Tri")

        # Run 1st time to make triangle mesh
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()

        myAnalysis.setAnalysisVal("Mesh_Gen_Input_String", "mquad=1 mpp=3")
        myAnalysis.setAnalysisVal("Proj_Name", "pyCAPS_aflr2_Quad")

        # Run 2nd time to make quad mesh
        myAnalysis.preAnalysis()
        myAnalysis.postAnalysis()

if __name__ == '__main__':
    unittest.main()
