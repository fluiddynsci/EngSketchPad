from __future__ import print_function
import unittest

import os
import glob
import shutil

import pyCAPS

class TestAFLR3(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.problemName = "workDir_aflr3Test"
        cls.iProb = 1
        cls.cleanUp()

        # Initialize a global Problem object
        cornerFile = os.path.join("..","csmData","cornerGeom.csm")
        cls.myProblem = pyCAPS.Problem(cls.problemName, capsFile=cornerFile, outLevel=0)

    @classmethod
    def tearDownClass(cls):
        del cls.myProblem
        cls.cleanUp()
        pass

    @classmethod
    def cleanUp(cls):

        # Remove analysis directories
        dirs = glob.glob( cls.problemName + '*')
        for dir in dirs:
            if os.path.isdir(dir):
                shutil.rmtree(dir)

#==============================================================================
    def test_setInput(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        aflr3 = myProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input.Proj_Name = "pyCAPS_AFLR3_Test"
        aflr3.input.Mesh_Quiet_Flag = True
        aflr3.input.Mesh_Format = "AFLR3"
        aflr3.input.Mesh_ASCII_Flag = True
        aflr3.input.Mesh_Gen_Input_String = ""
        aflr3.input.Multiple_Mesh = False
        
        inviscidBC = {"boundaryLayerThickness" : 0.0, "boundaryLayerSpacing" : 0.0}
        viscousBC  = {"boundaryLayerThickness" : 0.1, "boundaryLayerSpacing" : 0.01}
    
        # Set mesh sizing parmeters
        aflr3.input.Mesh_Sizing = {"Wing1": viscousBC, "Wing2": inviscidBC}

        aflr3.input.BL_Thickness = 0.1
        aflr3.input.BL_Max_Layers = 10
        aflr3.input.BL_Max_Layer_Diff = 5

#==============================================================================
    def test_reenter(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        aflr4 = myProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True
        aflr4.input.Mesh_Length_Factor = 20

        aflr3 = myProblem.analysis.create(aim = "aflr3AIM",
                                          name = "aflr3")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        aflr3.input.Mesh_Quiet_Flag = True
        aflr3.input.Proj_Name = "test"
        aflr3.input.Mesh_Format = "Tecplot"

        # Explicitly trigger mesh generation
        aflr3.runAnalysis()

        VolNumberOfNode_1    = aflr3.output.NumberOfNode
        VolNumberOfElement_1 = aflr3.output.NumberOfElement

        SurfNumberOfNode_1    = aflr4.output.NumberOfNode
        SurfNumberOfElement_1 = aflr4.output.NumberOfElement

        aflr4.input.Mesh_Length_Factor = 40

        # Explicitly trigger mesh generation again
        aflr3.runAnalysis()

        VolNumberOfNode_2    = aflr3.output.NumberOfNode
        VolNumberOfElement_2 = aflr3.output.NumberOfElement

        SurfNumberOfNode_2    = aflr4.output.NumberOfNode
        SurfNumberOfElement_2 = aflr4.output.NumberOfElement

        # Check that the counts have decreased
        self.assertGreater(VolNumberOfNode_1   , VolNumberOfNode_2   )
        self.assertGreater(VolNumberOfElement_1, VolNumberOfElement_2)

        self.assertGreater(SurfNumberOfNode_1   , SurfNumberOfNode_2   )
        self.assertGreater(SurfNumberOfElement_1, SurfNumberOfElement_2)

#==============================================================================
    def test_transp(self):

        file = os.path.join("..","csmData","cfdMultiBody.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        aflr4 = myProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True
        aflr4.input.Mesh_Length_Factor = 20

        aflr3 = myProblem.analysis.create(aim = "aflr3AIM",
                                          name = "aflr3")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        aflr3.input.Mesh_Quiet_Flag = True
        aflr3.input.Proj_Name = "test"
        aflr3.input.Mesh_Format = "Tecplot"
    
        # # Set mesh sizing parmeters
        aflr3.input.Mesh_Sizing = {"Wake": {"bcType":"TRANSP_UG3_GBC"}}
        
        # Explicitly trigger mesh generation
        aflr3.runAnalysis()
        
        # Set mesh sizing parmeters
        aflr3.input.Mesh_Sizing = {"Wake": {"bcType":"TRANSP_SRC_UG3_GBC"}}
        
        # Explicitly trigger mesh generation
        aflr3.runAnalysis()
        
        # Set mesh sizing parmeters
        aflr3.input.Mesh_Sizing = {"Wake": {"bcType":"TRANSP_INTRNL_UG3_GBC"}}
        
        # Explicitly trigger mesh generation
        aflr3.runAnalysis()
        
#==============================================================================
    def test_Multiple_Mesh(self):

        # Initialize Problem object
        cornerFile = os.path.join("..","csmData","cornerGeom.csm")
        myProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=cornerFile, outLevel=0); self.__class__.iProb += 1

        # Load aflr4 aim
        aflr4 = myProblem.analysis.create(aim = "aflr4AIM",
                                          name = "aflr4",
                                          capsIntent = ["box", "cylinder", "cone", "torus", "sphere", "bullet", "boxhole"])
                
        aflr4.input.Mesh_Quiet_Flag = True

        # Load aflr3 aim
        aflr3 = myProblem.analysis.create(aim = "aflr3AIM", 
                                          name = "aflr3",
                                          capsIntent = ["box", "cylinder", "cone", "torus", "sphere", "bullet", "boxhole"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Proj_Name = "aflr3Mesh"
        aflr3.input.Mesh_Quiet_Flag = True

        aflr3.input.Multiple_Mesh = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()


#==============================================================================
    def test_box(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["box", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["box", "farfield"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()


#==============================================================================
    def test_cylinder(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["cylinder", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("cylinder.egads")

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["cylinder", "farfield"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()

#==============================================================================
    def test_cone(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["cone", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("cone.egads")

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["cone", "farfield"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()

#==============================================================================
    def test_torus(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["torus", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("torus.egads")

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["torus", "farfield"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()

#==============================================================================
    def test_sphere(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["sphere", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("sphere.egads")

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["sphere", "farfield"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()

#==============================================================================
    def test_boxhole(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["boxhole", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("boxhole.egads")

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["boxhole", "farfield"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()

#==============================================================================
    def test_bullet(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["bullet", "farfield"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("bullet.egads")

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["bullet", "farfield"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()

#==============================================================================
    def test_all(self):

        # Load aflr4 aim
        aflr4 = self.myProblem.analysis.create(aim = "aflr4AIM",
                                               capsIntent = ["box", "cylinder", "cone", "torus", "sphere", "farfield", "bullet", "boxhole"])

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 0.5
        aflr4.input.min_scale = 0.1
        aflr4.input.ff_cdfr   = 1.4

        # Load aflr3 aim
        aflr3 = self.myProblem.analysis.create(aim = "aflr3AIM", 
                                               capsIntent = ["box", "cylinder", "cone", "torus", "sphere", "farfield", "bullet", "boxhole"])

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Just make sure it runs without errors...
        aflr3.runAnalysis()

#==============================================================================
    def test_phase(self):
        file = os.path.join("..","csmData","cfdSingleBody.csm")
        
        problemName = self.problemName + "_Phase"
        myProblem = pyCAPS.Problem(problemName, phaseName="Phase0", capsFile=file, outLevel=0)

        aflr4 = myProblem.analysis.create(aim = "aflr4AIM",
                                          name = "aflr4")
        
        aflr4.input.Mesh_Quiet_Flag = True
        aflr4.input.Mesh_Length_Factor = 20

        aflr3 = myProblem.analysis.create(aim = "aflr3AIM",
                                          name = "aflr3")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        aflr3.input.Mesh_Quiet_Flag = True
        aflr3.input.Proj_Name = "test"
        aflr3.input.Mesh_Format = "Tecplot"

        VolNumberOfNode_1    = aflr3.output.NumberOfNode
        VolNumberOfElement_1 = aflr3.output.NumberOfElement

        SurfNumberOfNode_1    = aflr4.output.NumberOfNode
        SurfNumberOfElement_1 = aflr4.output.NumberOfElement

        myProblem.closePhase()

        # Initialize Problem from the last phase and make a new phase
        myProblem = pyCAPS.Problem(problemName, phaseName="Phase1", phaseStart="Phase0", outLevel=0)

        aflr4 = myProblem.analysis["aflr4"]
        aflr3 = myProblem.analysis["aflr3"]

        # Check that the same outputs are still available
        self.assertEqual(VolNumberOfNode_1   , aflr3.output.NumberOfNode   )
        self.assertEqual(VolNumberOfElement_1, aflr3.output.NumberOfElement)

        self.assertEqual(SurfNumberOfNode_1   , aflr4.output.NumberOfNode   )
        self.assertEqual(SurfNumberOfElement_1, aflr4.output.NumberOfElement)

        # Coarsen the mesh
        aflr4.input.Mesh_Length_Factor = 40

        VolNumberOfNode_2    = aflr3.output.NumberOfNode
        VolNumberOfElement_2 = aflr3.output.NumberOfElement

        SurfNumberOfNode_2    = aflr4.output.NumberOfNode
        SurfNumberOfElement_2 = aflr4.output.NumberOfElement

        # Check that the counts have decreased
        self.assertGreater(VolNumberOfNode_1   , VolNumberOfNode_2   )
        self.assertGreater(VolNumberOfElement_1, VolNumberOfElement_2)

        self.assertGreater(SurfNumberOfNode_1   , SurfNumberOfNode_2   )
        self.assertGreater(SurfNumberOfElement_1, SurfNumberOfElement_2)


#==============================================================================
    def run_journal(self, myProblem, line_exit):

        verbose = False

        line = 0
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        # Load egadsAIM
        if verbose: print(6*"-", line,"Load aflr4AIM")
        aflr4 = myProblem.analysis.create(aim = "aflr4AIM"); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        if verbose: print(6*"-", line,"Modify Mesh_Quiet_Flag")
        aflr4.input.Mesh_Quiet_Flag = True; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        if verbose: print(6*"-", line,"Modify Mesh_Length_Factor")
        aflr4.input.Mesh_Length_Factor = 20; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        # Create the aflr3 AIM
        if verbose: print(6*"-", line,"Load aflr3AIM")
        aflr3 = myProblem.analysis.create(aim = "aflr3AIM"); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        if verbose: print(6*"-", line,"Modify Mesh_Quiet_Flag")
        aflr3.input.Mesh_Quiet_Flag = True; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        # Link the surface mesh
        if verbose: print(6*"-", line,"Link Surface_Mesh")
        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"]); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        # Run aflr4 explicitly
        #if verbose: print(6*"-", line,"Run AFLR4")
        #aflr4.runAnalysis(); line += 1
        #if line == line_exit: return line
        #if line_exit > 0: self.assertTrue(myProblem.journaling())

        # Run aflr3 explicitly
        if verbose: print(6*"-", line,"Run AFLR3")
        aflr3.runAnalysis(); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        if verbose: print(6*"-", line,"aflr3 VolNumberOfNode_1")
        VolNumberOfNode_1    = aflr3.output.NumberOfNode; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())
        
        if verbose: print(6*"-", line,"aflr3 VolNumberOfNode_1")
        VolNumberOfElement_1 = aflr3.output.NumberOfElement; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())


        # Coarsen the mesh
        if verbose: print(6*"-", line,"Modify Mesh_Length_Factor")
        aflr4.input.Mesh_Length_Factor = 40; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())

        if verbose: print(6*"-", line,"aflr3 VolNumberOfNode_2")
        VolNumberOfNode_2    = aflr3.output.NumberOfNode; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())
        
        if verbose: print(6*"-", line,"aflr3 VolNumberOfElement_2")
        VolNumberOfElement_2 = aflr3.output.NumberOfElement; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(myProblem.journaling())
        
        # Check that the counts have decreased
        self.assertGreater(VolNumberOfNode_1   , VolNumberOfNode_2   )
        self.assertGreater(VolNumberOfElement_1, VolNumberOfElement_2)

        # make sure the last call journals everything
        return line+2

#==============================================================================
    def test_journal(self):

        capsFile = os.path.join("..","csmData","cfdSingleBody.csm")
        problemName = self.problemName+str(self.iProb)

        myProblem = pyCAPS.Problem(problemName, capsFile=capsFile, outLevel=0)

        # Run once to get the total line count
        line_total = self.run_journal(myProblem, -1)

        myProblem.close()
        shutil.rmtree(problemName)

        #print(80*"=")
        #print(80*"=")
        # Create the problem to start journaling
        myProblem = pyCAPS.Problem(problemName, capsFile=capsFile, outLevel=0)
        myProblem.close()

        for line_exit in range(line_total):
            #print(80*"=")
            myProblem = pyCAPS.Problem(problemName, phaseName="Scratch", capsFile=capsFile, outLevel=0)
            self.run_journal(myProblem, line_exit)
            myProblem.close()

        self.__class__.iProb += 1

if __name__ == '__main__':
    unittest.main()
