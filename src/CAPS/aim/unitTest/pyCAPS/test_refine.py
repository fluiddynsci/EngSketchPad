from __future__ import print_function
import unittest

import os
import glob
import shutil

import pyCAPS

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

class TestREFINE(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.problemName = "workDir_refineTest"
        cls.iProb = 1
        cls.cleanUp()

        # Initialize a global Problem object
        cornerFile = os.path.join("..","csmData","cornerGeom.csm")
        cls.capsProblem = pyCAPS.Problem(cls.problemName, capsFile=cornerFile, outLevel=0)

    @classmethod
    def tearDownClass(cls):
        del cls.capsProblem
        cls.cleanUp()
        pass

    @classmethod
    def cleanUp(cls):

        # Remove analysis directories
        dirs = glob.glob( cls.problemName + '*')
        for dir in dirs:
            if os.path.isdir(dir):
                shutil.rmtree(dir)

    # This is executed prior to each test
    def setUp(self):
        if which("ref") == None:
            self.skipTest("No 'ref' executable")

#==============================================================================
    # Test inputs
    def test_inputs(self):

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input.ref = "mpiexec -n 4 refmpi"
        refine.input.Passes = 1
        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")
        refine.input.HessianFieldFile = os.path.join(refine.analysisDir, "hessian.sol")
        refine.input.MetricFieldFile = os.path.join(refine.analysisDir, "metric.sol")


#==============================================================================
    def test_cfdSingleBody(self):

        file = os.path.join("..","csmData","cfdSingleBody.csm")

        capsProblem = pyCAPS.Problem(self.problemName+str(self.iProb), capsFile=file, outLevel=0); self.__class__.iProb += 1

        aflr4 = capsProblem.analysis.create(aim = "aflr4AIM",
                                            name = "aflr4")

        # Modify local mesh sizing parameters
        aflr4.input.Mesh_Quiet_Flag = True

        #aflr4.input.max_scale = 2
        #aflr4.input.min_scale = 1
        aflr4.input.ff_cdfr   = 2
        aflr4.input.curv_factor = 2.0
        aflr4.input.erw_all = 0.0
        aflr4.input.mer_all = 0

        aflr4.input.Mesh_Length_Factor = 100

        aflr3 = capsProblem.analysis.create(aim = "aflr3AIM",
                                            name = "aflr3")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set project name
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = capsProblem.analysis.create(aim = "refineAIM",
                                             name = "refine")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])
        refine.input.Passes = 1 #Just for testing

        su2 = capsProblem.analysis.create(aim = "su2AIM")

        su2.input["Mesh"].link(refine.output["Mesh"])

        su2.input.Boundary_Condition = {"Wing1": {"bcType" : "Inviscid"},
                                        "Farfield":"farfield"}

        scalar_last = []
        for iadapt in range(3):
            su2.preAnalysis()
            su2.postAnalysis()

            xyz = refine.output.xyz

            refine.input["Mesh"].unlink()
            refine.input.ScalarFieldFile = os.path.join(su2.analysisDir, "scalar.sol")

            scalar = [1]*len(xyz)

            with open(refine.input.ScalarFieldFile, "w") as f:
                f.write("Dimension\n")
                f.write("3\n")
                f.write("SolAtVertices\n")
                f.write(str(len(scalar)) + " 1 1\n")
                for i in range(len(xyz)):
                    f.write(str(scalar[i]) + "\n")

            refine.input.Complexity = len(scalar)/2

            self.assertNotEqual(len(scalar_last), len(scalar))
            scalar_last = scalar

#==============================================================================
    def test_box_volume(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 1
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 0
        self.capsProblem.geometry.cfgpmtr.boxhole  = 0
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

        refine.input.ScalarFieldFile = None
        refine.input.HessianFieldFile = os.path.join(refine.analysisDir, "hessian.sol")

        hessian = [[1,0,0,1,0,1]]*len(xyz)

        with open(refine.input.HessianFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(hessian)) + " 1 3\n")
            for i in range(len(hessian)):
                for j in range(6):
                    f.write(str(hessian[i][j]) + " ")
                f.write("\n")

        refine.input.Complexity = len(hessian)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def off_test_box_surface(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 1
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 0
        self.capsProblem.geometry.cfgpmtr.boxhole  = 0
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = False

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr4.output["Surface_Mesh"])

        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [1]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def off_fails_test_cylinder(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 0
        self.capsProblem.geometry.cfgpmtr.cylinder = 1
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 0
        self.capsProblem.geometry.cfgpmtr.boxhole  = 0
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def off_fails_test_cone(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 0
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 1
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 0
        self.capsProblem.geometry.cfgpmtr.boxhole  = 0
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def off_fails_test_torus(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 0
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 1
        self.capsProblem.geometry.cfgpmtr.sphere   = 0
        self.capsProblem.geometry.cfgpmtr.boxhole  = 0
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def test_sphere(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 0
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 1
        self.capsProblem.geometry.cfgpmtr.boxhole  = 0
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def test_boxhole(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 0
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 0
        self.capsProblem.geometry.cfgpmtr.boxhole  = 1
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def off_fails_test_bullet(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 0
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 0
        self.capsProblem.geometry.cfgpmtr.boxhole  = 0
        self.capsProblem.geometry.cfgpmtr.bullet   = 1
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))

#==============================================================================
    def test_all(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 1
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 1
        self.capsProblem.geometry.cfgpmtr.boxhole  = 1
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.runAnalysis()

        xyz = refine.output.xyz

        self.assertNotEqual(len(xyz), len(scalar))


#==============================================================================
    def test_fun3d(self):

        self.capsProblem.geometry.cfgpmtr.single   = 1
        self.capsProblem.geometry.cfgpmtr.box      = 1
        self.capsProblem.geometry.cfgpmtr.cylinder = 0
        self.capsProblem.geometry.cfgpmtr.cone     = 0
        self.capsProblem.geometry.cfgpmtr.torus    = 0
        self.capsProblem.geometry.cfgpmtr.sphere   = 1
        self.capsProblem.geometry.cfgpmtr.boxhole  = 1
        self.capsProblem.geometry.cfgpmtr.bullet   = 0
        self.capsProblem.geometry.cfgpmtr.nodebody = 0

        # Load aflr4 aim
        aflr4 = self.capsProblem.analysis.create(aim = "aflr4AIM")

        aflr4.input.Mesh_Quiet_Flag = True

        aflr4.input.max_scale = 5.5
        aflr4.input.min_scale = 5
        aflr4.input.ff_cdfr   = 1.4

        #aflr4.geometry.save("box.egads")

        # Load aflr3 aim
        aflr3 = self.capsProblem.analysis.create(aim = "aflr3AIM")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        # Set output
        aflr3.input.Mesh_Quiet_Flag = True

        # Load refine aim
        refine = self.capsProblem.analysis.create(aim = "refineAIM", autoExec=False)

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Fun3D = True
        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        refine.runAnalysis()

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        refine.preAnalysis()

        #xyz = refine.output.xyz

        self.assertTrue(os.path.exists(os.path.join(refine.analysisDir, "refine_in_volume.solb")))


#==============================================================================
    def test_phase(self):
        file = os.path.join("..","csmData","cornerGeom.csm")

        problemName = self.problemName + "_Phase"
        capsProblem = pyCAPS.Problem(problemName, phaseName="Phase0", capsFile=file, outLevel=0)

        capsProblem.geometry.cfgpmtr.single   = 1
        capsProblem.geometry.cfgpmtr.box      = 1
        capsProblem.geometry.cfgpmtr.cylinder = 0
        capsProblem.geometry.cfgpmtr.cone     = 0
        capsProblem.geometry.cfgpmtr.torus    = 0
        capsProblem.geometry.cfgpmtr.sphere   = 0
        capsProblem.geometry.cfgpmtr.boxhole  = 0
        capsProblem.geometry.cfgpmtr.bullet   = 0
        capsProblem.geometry.cfgpmtr.nodebody = 0

        aflr4 = capsProblem.analysis.create(aim = "aflr4AIM", name="aflr4")

        aflr4.input.Mesh_Quiet_Flag = True
        aflr4.input.Mesh_Length_Factor = 10

        aflr3 = capsProblem.analysis.create(aim = "aflr3AIM", name="aflr3")

        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"])

        aflr3.input.Mesh_Quiet_Flag = True

        VolNumberOfNode_1    = aflr3.output.NumberOfNode
        VolNumberOfElement_1 = aflr3.output.NumberOfElement

        SurfNumberOfNode_1    = aflr4.output.NumberOfNode
        SurfNumberOfElement_1 = aflr4.output.NumberOfElement

        # Load refine aim
        refine = capsProblem.analysis.create(aim = "refineAIM", name="refine")

        refine.input["Mesh"].link(aflr3.output["Volume_Mesh"])

        refine.input.Complexity = 50
        refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        xyz = refine.output.xyz

        refine.input["Mesh"].unlink()

        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2
        refine.input.Passes = 1

        xyz = refine.output.xyz

        RefNumberOfNode_1 = len(xyz)

        capsProblem.closePhase()

        # Initialize Problem from the last phase and make a new phase
        capsProblem = pyCAPS.Problem(problemName, phaseName="Phase1", phaseStart="Phase0", outLevel=0)

        aflr4 = capsProblem.analysis["aflr4"]
        aflr3 = capsProblem.analysis["aflr3"]
        refine = capsProblem.analysis["refine"]

        # Check that the same outputs are still available
        self.assertEqual(VolNumberOfNode_1   , aflr3.output.NumberOfNode   )
        self.assertEqual(VolNumberOfElement_1, aflr3.output.NumberOfElement)

        self.assertEqual(SurfNumberOfNode_1   , aflr4.output.NumberOfNode   )
        self.assertEqual(SurfNumberOfElement_1, aflr4.output.NumberOfElement)

        xyz = refine.output.xyz

        self.assertEqual(RefNumberOfNode_1, len(xyz))

        refine.input.Passes = 1
        #refine.input.ScalarFieldFile = os.path.join(refine.analysisDir, "scalar.sol")

        # Coarsen the mesh
        scalar = [0]*len(xyz)
        for i in range(len(xyz)):
            scalar[i] = xyz[i][0]**2

        with open(refine.input.ScalarFieldFile, "w") as f:
            f.write("Dimension\n")
            f.write("3\n")
            f.write("SolAtVertices\n")
            f.write(str(len(scalar)) + " 1 1\n")
            for i in range(len(xyz)):
                f.write(str(scalar[i]) + "\n")

        refine.input.Complexity = len(scalar)/2

        xyz = refine.output.xyz

        RefNumberOfNode_2 = len(xyz)

        # Check that the counts have decreased
        self.assertNotEqual(RefNumberOfNode_1   , RefNumberOfNode_2   )

#==============================================================================
    def run_journal(self, capsProblem, line_exit):

        verbose = False

        line = 0
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        # Load egadsAIM
        if verbose: print(6*"-", line,"Load aflr4AIM")
        aflr4 = capsProblem.analysis.create(aim = "aflr4AIM"); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        if verbose: print(6*"-", line,"Modify Mesh_Quiet_Flag")
        aflr4.input.Mesh_Quiet_Flag = True; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        if verbose: print(6*"-", line,"Modify Mesh_Length_Factor")
        aflr4.input.Mesh_Length_Factor = 20; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        # Create the aflr3 AIM
        if verbose: print(6*"-", line,"Load aflr3AIM")
        aflr3 = capsProblem.analysis.create(aim = "aflr3AIM"); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        if verbose: print(6*"-", line,"Modify Mesh_Quiet_Flag")
        aflr3.input.Mesh_Quiet_Flag = True; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        # Link the surface mesh
        if verbose: print(6*"-", line,"Link Surface_Mesh")
        aflr3.input["Surface_Mesh"].link(aflr4.output["Surface_Mesh"]); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        # Run aflr4 explicitly
        #if verbose: print(6*"-", line,"Run AFLR4")
        #aflr4.runAnalysis(); line += 1
        #if line == line_exit: return line
        #if line_exit > 0: self.assertTrue(capsProblem.journaling())

        # Run aflr3 explicitly
        if verbose: print(6*"-", line,"Run AFLR3")
        aflr3.runAnalysis(); line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        if verbose: print(6*"-", line,"aflr3 VolNumberOfNode_1")
        VolNumberOfNode_1    = aflr3.output.NumberOfNode; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        if verbose: print(6*"-", line,"aflr3 VolNumberOfNode_1")
        VolNumberOfElement_1 = aflr3.output.NumberOfElement; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())


        # Coarsen the mesh
        if verbose: print(6*"-", line,"Modify Mesh_Length_Factor")
        aflr4.input.Mesh_Length_Factor = 40; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        if verbose: print(6*"-", line,"aflr3 VolNumberOfNode_2")
        VolNumberOfNode_2    = aflr3.output.NumberOfNode; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        if verbose: print(6*"-", line,"aflr3 VolNumberOfElement_2")
        VolNumberOfElement_2 = aflr3.output.NumberOfElement; line += 1
        if line == line_exit: return line
        if line_exit > 0: self.assertTrue(capsProblem.journaling())

        # Check that the counts have decreased
        self.assertGreater(VolNumberOfNode_1   , VolNumberOfNode_2   )
        self.assertGreater(VolNumberOfElement_1, VolNumberOfElement_2)

        # make sure the last call journals everything
        return line+2

#==============================================================================
    def off_test_journal(self):

        capsFile = os.path.join("..","csmData","cfdSingleBody.csm")
        problemName = self.problemName+str(self.iProb)

        capsProblem = pyCAPS.Problem(problemName, capsFile=capsFile, outLevel=0)

        # Run once to get the total line count
        line_total = self.run_journal(capsProblem, -1)

        capsProblem.close()
        shutil.rmtree(problemName)

        #print(80*"=")
        #print(80*"=")
        # Create the problem to start journaling
        capsProblem = pyCAPS.Problem(problemName, capsFile=capsFile, outLevel=0)
        capsProblem.close()

        for line_exit in range(line_total):
            #print(80*"=")
            capsProblem = pyCAPS.Problem(problemName, phaseName="Scratch", capsFile=capsFile, outLevel=0)
            self.run_journal(capsProblem, line_exit)
            capsProblem.close()

        self.__class__.iProb += 1

if __name__ == '__main__':
    unittest.main()
