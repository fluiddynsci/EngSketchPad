#------------------------------------------------------------------------------#
# Copyright (C) 2018  John F. Dannenhoffer, III (Syracuse University)
#
# This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Lesser General Public
#    License as published by the Free Software Foundation; either
#    version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
#    License along with this library; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#     MA  02110-1301  USA
#------------------------------------------------------------------------------#

# Allow print statement to be compatible between different versions of Python
from __future__ import print_function

# Import os
import os

#------------------------------------------------------------------------------#

# Function to run AVL (called at bottom of file if top-level)
def run_avl(filename):
    """
    run AVL to compute force coefficients

    inputs:
        filename    name of .csm or .cpc file
    outputs:
        coefs       dictionary containing force/moment coefficients:
                    CLtot  lift coefficient
                    CDtot  drag coefficient
                    CXtot  X-force coefficient
                    CYtot  Y-force coefficient
                    CZtot  Z-force coefficient
                    Cltot  rolling  moment coefficient
                    Cmtot  pitching moment coefficient
                    Cntot  yawing   moment coefficient
                    e      Oswald efficiency factor
    """

    # Allow Python to interact with OS
    import os
    import sys

    # Import pyCAPS class
    from pyCAPS import capsProblem

    # Add ".csm" if filename does not end with either .csm or .cdc
    if filename[-4:] != ".csm" and filename[-4:] != ".cpc":
        filename += ".csm"

    # Initialize capsProblem object
    myProblem = capsProblem()

    # Set working directory for all temporary files
    workDir = "AVL_Analysis"

    # Load geometry [.csm] file
    print ("\n==> Loading geometry from file \""+filename+"\"...")
    myGeometry = myProblem.loadCAPS(filename)

    # Set geometry variables to enable Avl mode
    print ("\n==> Setting Build Variables and Geometry Values...")
    try:
        myGeometry.setGeometryVal("VIEW:Concept", 0)
        myGeometry.setGeometryVal("VIEW:VLM",     1)
    except:
        print ("File \""+filename+"\" not set up to run Avl")
        raise SystemError

    # Set other geometry specific variables
#    myGeometry.setGeometryVal("wing:aspect", 12.0)

    # Load AVL AIM
    print ("\n==> Loading AVL aim...")
    avl = myProblem.loadAIM(aim         = "avlAIM",
                            analysisDir = workDir)

    # Set analysis specific variables
    avl.setAnalysisVal("Mach",   0.5)
    avl.setAnalysisVal("Alpha", 10.0)
    avl.setAnalysisVal("Beta",   0.0)

    # If .csm file has a Wing, set up AVL-specific variables for the wing
    try:
        haveWing = myGeometry.getGeometryVal("COMP:Wing")
        wing = {"numChord"     : 10,   # number of chordwise vorticies
                "spaceChord"   : 1.0,  # chordwise vortex spacing parameter (cosine)
                "numSpanTotal" : 30,   # number of spanwise  vorticies
                "spaceSpan"    : 1.0}  # spanwise  vortex spacing parameter (cosine)
    except:
        haveWing = 0

    # If .csm file has a Htail, set up AVL-specific variables for the htail
    try:
        haveHtail = myGeometry.getGeometryVal("COMP:Htail")
        htail = {"numChord"     : 10,   # number of chordwise vorticies
                 "spaceChord"   : 1.0,  # chordwise vortex spacing parameter (cosine)
                 "numSpanTotal" : 20,   # number of spanwise  vorticies
                 "spaceSpan"    : 1.0}  # spanwise  vortex spacing parameter (cosine)
    except:
        haveHtail = 0

    # Tell AVL about the surfaces
    if haveWing != 0 and haveHtail != 0:
        avl.setAnalysisVal("AVL_Surface", [("Wing", wing),("Htail",htail)])
    elif haveWing != 0:
        avl.setAnalysisVal("AVL_Surface", [("Wing", wing)])
    else:
        print (".csm file does not have a wing (which is required)")
        raise SystemError

    # Set up control surface deflections
    outHinge = []

    try:
        hinge = myGeometry.getGeometryVal("wing:hinge")
        for i in range(len(hinge)):
            outHinge.append(("WingHinge"+str(i+1), {"deflectionAngle": hinge[i][0]}))
    except:
        pass

    try:
        hinge = myGeometry.getGeometryVal("htail:hinge")
        for i in range(len(hinge)):
            outHinge.append(("HtailHinge"+str(i+1), {"deflectionAngle": hinge[i][0]}))
    except:
        pass

    if len(outHinge) > 0:
        avl.setAnalysisVal("AVL_Control", outHinge)

    # Run AIM pre-analysis
    print ("\n==> Running AVL pre-analysis...")
    avl.preAnalysis()

    # Run AVL
    print ("\n==> Running AVL...")

    # Get our current working directory (so that we can return here after running AVL)
    currentPath = os.getcwd()

    # Move into temp directory
    os.chdir(avl.analysisDir)

    # Run AVL via system call
    os.system("avl caps < avlInput.txt > avlOutput.txt")

    # Move back to original directory
    os.chdir(currentPath)

    # Run AIM post-analysis
    print ("\n==> Running AVL post-analysis...")
    avl.postAnalysis()

    # Build dictionary of force/moment coefficients
    coefs = {}

    # Forces/moments in body axis
    coefs["CXtot"] = avl.getAnalysisOutVal("CXtot")
    coefs["CYtot"] = avl.getAnalysisOutVal("CYtot")
    coefs["CZtot"] = avl.getAnalysisOutVal("CZtot")
    coefs["Cltot"] = avl.getAnalysisOutVal("Cltot")
    coefs["Cmtot"] = avl.getAnalysisOutVal("Cmtot")
    coefs["Cntot"] = avl.getAnalysisOutVal("Cntot")

    # Forces/moments in stability axis
    coefs["CLtot"] = avl.getAnalysisOutVal("CLtot")
    coefs["CDtot"] = avl.getAnalysisOutVal("CDtot")

    # Oswald efficiency factor
    coefs["e"    ] = avl.getAnalysisOutVal("e"    )

    # Close CAPS
    myProblem.closeCAPS()

    print("\n================ run_avl.py complete ==================\n")

    # Return the force/moment coefficients
    return coefs

#------------------------------------------------------------------------------#

# run this function if run as a script or with "python -m", but not when imported
if __name__ == "__main__":

    # Allow Python to interact with OS
    import sys

    # Get the name of the input file (default to wing1.csm)
    if len(sys.argv) < 2:
        filename = os.path.join("..","ESP","wing1.csm")
    else:
        filename = sys.argv[1]

    coefs = run_avl(filename)

    print ("\nForce/moment coefficients:")
    for name, value in coefs.items():
        print (name.ljust(7) + ": " + repr(value))
