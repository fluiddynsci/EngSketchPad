from __future__ import print_function
import unittest

import os
import shutil
import sys

import pyCAPS

try:
    ASTROS_ROOT = os.environ["ASTROS_ROOT"]
except KeyError:
   print("Please set the environment variable ASTROS_ROOT")

# Helper function to check if an executable exists
def which(program):
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    try:
        exe_file = os.path.join(ASTROS_ROOT, "astros.exe")
        if is_exe(exe_file):
            return exe_file
    except:
        pass

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


class TestAstros(unittest.TestCase):


#    @classmethod
#    def setUpClass(cls):

#    @classmethod
#    def tearDownClass(cls):

    # This is executed prior to each test
    def setUp(self):
        self.astros_exe = which("astros.exe")
        if self.astros_exe == None:
            self.skipTest("No astros.exe executable")
            return

        # Initialize capsProblem object
        self.myProblem = pyCAPS.capsProblem()


    def run_astros(self, astros):

        projectName = astros.getAnalysisVal("Proj_Name")

        # Run AIM pre-analysis
        astros.preAnalysis()

        ####### Run astros####################
        print ("\n\nRunning Astros......")
        currentDirectory = os.getcwd() # Get our current working directory

        os.chdir(astros.analysisDir) # Move into test directory

        # Copy files needed to run astros
        astros_files = ["ASTRO.D01",  # *.DO1 file
                        "ASTRO.IDX"]  # *.IDX file
        for file in astros_files:
            if not os.path.isfile(file):
                shutil.copy2(ASTROS_ROOT + os.sep + file, file)

        # Run Astros via system call
        os.system(self.astros_exe + " < " + projectName +  ".dat > " + projectName + ".out");

        os.chdir(currentDirectory) # Move back to working directory

        print ("Done running Astros!")
        ########################################

        # Run AIM post-analysis
        astros.postAnalysis()


    def test_Plate(self):

        filename = os.path.join("..","csmData","feaSimplePlate.csm")
        myGeometry = self.myProblem.loadCAPS(filename, verbosity=0)

        astros = self.myProblem.loadAIM(aim = "astrosAIM",
                                        analysisDir = "workDir_astrosPlate")

        astros.setAnalysisVal("Edge_Point_Min", 3)
        astros.setAnalysisVal("Edge_Point_Max", 4)

        astros.setAnalysisVal("Quad_Mesh", True)

        astros.setAnalysisVal("Tess_Params", [.25,.01,15])


        # Set analysis type
        astros.setAnalysisVal("Analysis_Type", "Static");

        # Set materials
        madeupium    = {"materialType" : "isotropic",
                        "youngModulus" : 72.0E9 ,
                        "poissonRatio": 0.33,
                        "density" : 2.8E3}

        astros.setAnalysisVal("Material", ("Madeupium", madeupium))

        # Set properties
        shell  = {"propertyType" : "Shell",
                  "membraneThickness" : 0.006,
                  "material"        : "madeupium",
                  "bendingInertiaRatio" : 1.0, # Default
                  "shearMembraneRatio"  : 5.0/6.0} # Default

        astros.setAnalysisVal("Property", ("plate", shell))

        # Set constraints
        constraint = {"groupName" : "plateEdge",
                      "dofConstraint" : 123456}

        astros.setAnalysisVal("Constraint", ("edgeConstraint", constraint))

        # Set load
        load = {"groupName" : "plate",
                "loadType" : "Pressure",
                "pressureForce" : 2.e6}

        # Set loads
        astros.setAnalysisVal("Load", ("appliedPressure", load ))


        astros.setAnalysisVal("File_Format", "Small")
        astros.setAnalysisVal("Mesh_File_Format", "Small")
        astros.setAnalysisVal("Proj_Name", "astrosPlateSmall")

        # Run Small format
        self.run_astros(astros)

        astros.setAnalysisVal("File_Format", "Large")
        astros.setAnalysisVal("Mesh_File_Format", "Large")
        astros.setAnalysisVal("Proj_Name", "astrosPlateLarge")

        # Run Large format
        self.run_astros(astros)

        astros.setAnalysisVal("File_Format", "Free")
        astros.setAnalysisVal("Mesh_File_Format", "Free")
        astros.setAnalysisVal("Proj_Name", "astrosPlateFree")

        # Run Free format
        self.run_astros(astros)

    def test_Aeroelastic(self):

        filename = os.path.join("..","csmData","feaWingOMLAero.csm")
        myGeometry = self.myProblem.loadCAPS(filename)

        astros = self.myProblem.loadAIM(aim = "astrosAIM",
                                        analysisDir = "workDir_astrosAero")

        astros.setAnalysisVal("Proj_Name", "astrosAero")

        astros.setAnalysisVal("Edge_Point_Min", 3)
        astros.setAnalysisVal("Edge_Point_Max", 4)

        astros.setAnalysisVal("Quad_Mesh", True)

        # Set analysis type
        astros.setAnalysisVal("Analysis_Type", "Aeroelastic")

        # Set analysis
        trim1 = { "analysisType" : "AeroelasticStatic",
                  "trimSymmetry" : "SYM",
                  "analysisConstraint" : ["ribConstraint"],
                  "analysisSupport" : ["ribSupport"],
                  "machNumber"     : 0.5,
                  "dynamicPressure": 50000,
                  "density" : 1.0,
                  "rigidVariable"  : "ANGLEA",
                  "rigidConstraint": ["URDD3"],
                  "magRigidConstraint" : [-50],
                  }


        astros.setAnalysisVal("Analysis", [("Trim1", trim1)])

        # Set materials
        unobtainium  = {"youngModulus" : 2.2E6 ,
                        "poissonRatio" : .5,
                        "density"      : 7850}

        astros.setAnalysisVal("Material", ("Unobtainium", unobtainium))

        # Set property
        shell  = {"propertyType"        : "Shell",
                  "membraneThickness"   : 0.2,
                  "bendingInertiaRatio" : 1.0, # Default
                  "shearMembraneRatio"  : 5.0/6.0} # Default }

        shell2  = {"propertyType"       : "Shell",
                  "membraneThickness"   : 0.002,
                  "bendingInertiaRatio" : 1.0, # Default
                  "shearMembraneRatio"  : 5.0/6.0} # Default }

        astros.setAnalysisVal("Property", [("Rib_Root", shell),
                                           ("Skin", shell2)])


        # Defined Connections
        connection = {"dofDependent"   : 123456,
                      "connectionType" : "RigidBody"}

        astros.setAnalysisVal("Connect",("Rib_Root", connection))


        # Set constraints
        constraint = {"groupName"     : ["Rib_Root_Point"],
                      "dofConstraint" : 12456}

        astros.setAnalysisVal("Constraint", ("ribConstraint", constraint))

        # Set supports
        support = {"groupName" : ["Rib_Root_Point"],
                   "dofSupport": 3}

        astros.setAnalysisVal("Support", ("ribSupport", support))


        # Aero
        wing = {"groupName"    : "Wing",
                "numChord"     : 8,
                "numSpanTotal" : 24}

        # Note the surface name corresponds to the capsBound found in the *.csm file. This links
        # the spline for the aerodynamic surface to the structural model
        astros.setAnalysisVal("VLM_Surface", ("Skin_Top", wing))

        # Run
        self.run_astros(astros)


if __name__ == '__main__':
    unittest.main()
