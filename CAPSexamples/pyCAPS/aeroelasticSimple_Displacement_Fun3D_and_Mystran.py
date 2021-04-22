from __future__ import print_function

# Import pyCAPS class file
from pyCAPS import capsProblem

# Import os module   
try: 
    import os
except:
    print ("Unable to import os module")
    raise SystemExit

# Initialize capsProblem object
myProblem = capsProblem()

# Create working directory variable 
workDir = "AeroelasticSimple_Displacement"

# Create projectName vairbale
projectName = "aeroelasticSimple_Displacement"

# Set the number of transfer iterations
numTransferIteration = 1

# Load CSM file 
geometryScript = os.path.join("..","csmData","aeroelasticDataTransferSimple.csm")
myProblem.loadCAPS(geometryScript)

# Load AIMs 
myProblem.loadAIM(aim = "tetgenAIM", 
                  altName= "tetgen",
                  analysisDir = workDir + "_FUN3D",
                  capsIntent = "CFD")

  
myProblem.loadAIM(aim = "fun3dAIM", 
                  altName = "fun3d", 
                  analysisDir = workDir + "_FUN3D", 
                  parents = ["tetgen"],
                  capsIntent = "CFD")

myProblem.loadAIM(aim = "mystranAIM", 
                  altName = "mystran", 
                  analysisDir = workDir + "_MYSTRAN",
                  capsIntent = "STRUCTURE")

transfers = ["Skin_Top", "Skin_Bottom", "Skin_Tip"]
for i in transfers:
    myProblem.createDataTransfer(variableName = "Displacement",
                                 aimSrc = "mystran",
                                 aimDest ="fun3d",
                                 transferMethod = "Interpolate", # "Conserve", #, 
                                 capsBound = i)

# Set inputs for tetgen 
myProblem.analysis["tetgen"].setAnalysisVal("Tess_Params", [.05, 0.01, 20.0])
myProblem.analysis["tetgen"].setAnalysisVal("Preserve_Surf_Mesh", True)

# Set inputs for fun3d
myProblem.analysis["fun3d"].setAnalysisVal("Proj_Name", projectName)
myProblem.analysis["fun3d"].setAnalysisVal("Mesh_ASCII_Flag", False)
myProblem.analysis["fun3d"].setAnalysisVal("Mach", 0.3)
myProblem.analysis["fun3d"].setAnalysisVal("Equation_Type","compressible")
myProblem.analysis["fun3d"].setAnalysisVal("Viscous", "inviscid")
myProblem.analysis["fun3d"].setAnalysisVal("Num_Iter",10)
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule",[1, 5.0])
myProblem.analysis["fun3d"].setAnalysisVal("CFL_Schedule_Iter", [1, 40])
myProblem.analysis["fun3d"].setAnalysisVal("Overwrite_NML", True)
myProblem.analysis["fun3d"].setAnalysisVal("Restart_Read","off")

inviscid = {"bcType" : "Inviscid", "wallTemperature" : 1.1}
myProblem.analysis["fun3d"].setAnalysisVal("Boundary_Condition", [("Skin", inviscid),
                                                                  ("SymmPlane", "SymmetryY"),
                                                                  ("Farfield","farfield")])

# Set inputs for mystran
myProblem.analysis["mystran"].setAnalysisVal("Proj_Name", projectName)
myProblem.analysis["mystran"].setAnalysisVal("Edge_Point_Max", 10)
myProblem.analysis["mystran"].setAnalysisVal("Edge_Point_Min", 10)

myProblem.analysis["mystran"].setAnalysisVal("Quad_Mesh", True)
myProblem.analysis["mystran"].setAnalysisVal("Tess_Params", [.5, .1, 15])
myProblem.analysis["mystran"].setAnalysisVal("Analysis_Type", "Static");

madeupium    = {"materialType" : "isotropic", 
                "youngModulus" : 72.0E9 , 
                "poissonRatio": 0.33, 
                "density" : 2.8E3}
myProblem.analysis["mystran"].setAnalysisVal("Material", ("Madeupium", madeupium))

skin  = {"propertyType" : "Shell", 
         "membraneThickness" : 0.06, 
         "material"        : "madeupium", 
         "bendingInertiaRatio" : 1.0, # Default           
         "shearMembraneRatio"  : 5.0/6.0} # Default 

ribSpar  = {"propertyType" : "Shell", 
            "membraneThickness" : 0.6, 
            "material"        : "madeupium", 
            "bendingInertiaRatio" : 1.0, # Default           
            "shearMembraneRatio"  : 5.0/6.0} # Default 

myProblem.analysis["mystran"].setAnalysisVal("Property", [("Skin", skin),
                                                          ("Rib_Root", ribSpar)])

constraint = {"groupName" : "Rib_Root", 
              "dofConstraint" : 123456}
myProblem.analysis["mystran"].setAnalysisVal("Constraint", ("edgeConstraint", constraint))

load = {"groupName": "Skin", "loadType" : "Pressure", "pressureForce": 1E8}
myProblem.analysis["mystran"].setAnalysisVal("Load", ("pressureInternal", load ))   
    
    
# Run pre/post-analysis for tetgen and fun3d - execute mystran
aim = ["tetgen", "mystran"]
for i in aim:
    print ("\nRunning PreAnalysis ......", i)
    myProblem.analysis[i].preAnalysis()

    if "mystran" in i:
     
        # Run mystran
        print ("\n\nRunning Mystran......")  
        currentDirectory = os.getcwd() # Get our current working directory 

        os.chdir(myProblem.analysis[i].analysisDir) # Move into test directory

        os.system("mystran.exe " + projectName +  ".dat > Info.out") # Run fun3d via system call

        if os.path.getsize("Info.out") == 0: # 
            print ("Mystran excution failed\n")
            myProblem.closeCAPS()
            raise SystemError
            
        os.chdir(currentDirectory) # Move back to top directory 
        
    if "tetgen" in i:    
        print ("\nRunning PostAnalysis ......", i)
        # Run AIM post-analysis
        myProblem.analysis[i].postAnalysis()

# Run aim pre-analysis, excute codes, and run aim post-analysis
aim = ["fun3d"]
for i in aim:
    print ("\nRunning PreAnalysis ......", i)
    myProblem.analysis[i].preAnalysis()
    
    print ("\nRunning PostAnalysis ......", i)
    myProblem.analysis[i].postAnalysis()

aim = ["mystran", "fun3d"]
for i in aim:
    
    if "mystran" in i:
        print ("\nRunning PostAnalysis ......", i)
        # Run AIM post-analysis
        myProblem.analysis[i].postAnalysis()

        #Execute the dataTransfer
        print ("\nExecuting dataTransfer ......")
        for j in transfers: 
            myProblem.dataBound[j].executeTransfer()
           
            myProblem.dataBound[j].dataSetSrc["Displacement"].viewData() 
            #myProblem.dataBound[j].viewData("Displacement", filename = j + "Displacement", showImage=False, colormap="Purples")
            
    if "fun3d" in i:

        # Peturb fun3d Analysis to make it unclean     
        myProblem.analysis[i].setAnalysisVal("Mach", 0.3)
                
        # Re-run the preAnalysis 
        print ("\nRunning PreAnalysis ......", i)
        myProblem.analysis[i].preAnalysis()

        ####### Run fun3d ####################
        print ("\n\nRunning FUN3D......")  
        currentDirectory = os.getcwd() # Get our current working directory 
        
        os.chdir(myProblem.analysis[i].analysisDir) # Move into test directory
        
        cmdLineOpt = "--read_surface_from_file --animation_freq -1"
    
        os.system("mpirun -np 5 nodet_mpi " + cmdLineOpt + " > Info.out"); # Run fun3d via system call
            
        if os.path.getsize("Info.out") == 0: # 
            print ("FUN3D excution failed\n")
            myProblem.closeCAPS()
            raise SystemError
        
        os.chdir(currentDirectory) # Move back to top directory
        
        print ("\nRunning PostAnalysis ......", i)
        # Run AIM post-analysis
        myProblem.analysis[i].postAnalysis()
 
# Close CAPS 
myProblem.closeCAPS()
