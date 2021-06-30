// ESP-ereped.js implements ErepEd functions for the Engineering Sketch Pad (ESP)
// written by John Dannenhoffer

// Copyright (C) 2010/2021  John F. Dannenhoffer, III (Syracuse University)
//
// This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Lesser General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//    MA  02110-1301  USA

// interface functions that ESP-ereped.js provides
//    erep.launch()
//    erep.cmdLoad()
//    erep.cmdUndo()
//    erep.cmdSolve()
//    erep.cmdSave()
//    erep.cmdQuit()
//
//    erep.cmdHome()
//    erep.cmdLeft()
//    erep.cmdRite()
//    erep.cmdBotm()
//    erep.cmdTop()
//    erep.cmdIn()
//    erep.cmdOut()
//
//    erep.keyPress(e)
//    erep.keyDown(e)
//    erep.keyUp(e)
//    erep.keyPressPart1(myKeyPress)
//    erep.keyPressPart2(picking, gprim)

// functions associated with ErepEd
//    erep.initialize(getNewName)
//    erep.redraw()

"use strict";


//
// callback when ErepEd is launched
//
erep.launch = function () {
    // alert("in erep.launch()");

    // close the Tools menu
    var menu = document.getElementsByClassName("toolMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showToolMenu")) {
            menu[i].classList.remove(  "showToolMenu");
        }
    }

    // make sure we do not enter if we are looking at an Erep already
    if (wv.plotType == 6) {
        alert("You cannot enter the Erep Editor while looking at an EBody\n"+
              "Switch the \"DisplayType\" back to \"0\" first");
        return;
    }

    // initialize GUI variables
    erep.attrName  = "_erep";
    erep.uncolored = 0;
    erep.keptNodes = 0;
    erep.keptEdges = 0;
    erep.lastColor = -1;
    erep.dihedral  = -1;
    erep.colors    = [];

    // spectrum of colors
    erep.spectrum = [0.87, 0.87, 0.87,       // light grey
                     1.00, 0.87, 0.87,       // light red
                     0.87, 1.00, 0.87,       // light green
                     0.87, 0.87, 1.00,       // light blue
                     0.87, 1.00, 1.00,       // light cyan
                     1.00, 0.87, 1.00,       // light magenta
                     1.00, 1.00, 0.87,       // light yellow
                     1.00, 0.50, 0.50,       // medium red
                     0.50, 1.00, 0.50,       // medium green
                     0.50, 0.50, 1.00,       // medium blue
                     0.50, 1.00, 1.00,       // medium cyan
                     1.00, 0.50, 1.00,       // medium magenta
                     1.00, 1.00, 0.50];      // medium yellow

    // uncolor all entities
    for (var gprim in wv.sceneGraph) {
        erep.colors[gprim] = -1;
    }
    
    // initialize the application
    if (erep.initialize(1) > 0) {
        return;
    }

    // change done button legend
    var button = document.getElementById("doneMenuBtn");
    button["innerHTML"] = "ErepEd";
    button.style.backgroundColor = "#3FFF3F";

    // change solve button legend
    button = document.getElementById("solveButton");
    button["innerHTML"] = "ShowEBodys";
    button.style.backgroundColor = "#FFFF3F";

    // update the picture
    wv.sceneUpd = 1;

};


//
// erep.cmdload - not implemented
//
erep.cmdLoad = function () {
    alert("erep.cmdLoad() is not implemented");
};


//
// callback when "ErepEd->Undo" is pressed (called by ESP.html)
//
erep.cmdUndo = function () {
    alert("erep.cmdUndo() is not implemented");
};


//
// callback when "solveButton" is pressed
//
erep.cmdSolve = function () {
    // alert("in erep.cmdSolve()");

    // if we are now showing the Ebody, switch back to showing the Bodys
    var button = document.getElementById("solveButton");
    if (button["innerHTML"] == "EditErep") {
        button["innerHTML"] = "ShowEBodys";

        // get the Body number
        var ibody = 0;
        for (var gprim in wv.sceneGraph) {
            var gprimList = gprim.trim().split(" ");

            if (gprimList[0] == "Body") {
                ibody = gprimList[1];
                break;
            }
        }

        browserToServer("makeEBody|"+ibody+"|0|.|");
        
        return;
    }

    if (erep.dihedral < 0) {
        var dihedral = prompt("Enter maximum dihedral angle", "5");
        if (dihedral === null) {
            return;
        } else if (isNaN(dihedral)) {
            alert("Illegal number format");
            return;
        } else if (dihedral < 0 || dihedral > 90) {
            alert("Out of range (0-90)");
            return;
        }

        erep.dihedral = dihedral;
    }
        
    button["innerHTML"] = "EditErep";

    // get the Body number
    var ibody = 0;
    for (var gprim in wv.sceneGraph) {
        var gprimList = gprim.trim().split(" ");

        if (gprimList[0] == "Body") {
            ibody = gprimList[1];
            break;
        }
    }

    var mesg = "makeEBody|" + ibody + "|" + erep.dihedral + "|";
    

    // add Nodes to the mesg
    mesg += erep.keptNodes + ";";

    for (var gprim in wv.sceneGraph) {
        var gprimList = gprim.trim().split(" ");

        if (gprimList[gprimList.length-2] == "Node") {
            if (erep.colors[gprim] == -3) {
                mesg += gprimList[gprimList.length-1] + ";";
            }
        }
    }

    // add Edges to the mesg
    mesg += erep.keptEdges + ";";

    for (var gprim in wv.sceneGraph) {
        var gprimList = gprim.trim().split(" ");

        if (gprimList[gprimList.length-2] == "Edge") {
            if (erep.colors[gprim] == -2) {
                mesg += gprimList[gprimList.length-1] + ";";
            }
        }
    }

    // add Faces (in groups) to the mesg
    var colored = 0;
    for (gprim in wv.sceneGraph) {
        if (erep.colors[gprim] > 0) {
            colored++;
        }
    }

    for (var igroup = 1; igroup < 99999; igroup++) {
        var nface = 0;

        for (gprim in wv.sceneGraph) {
            if (erep.colors[gprim] == igroup) {
                nface++;
            }
        }

        if (nface > 1) {
            mesg += nface + ";";

            for (gprim in wv.sceneGraph) {
                if (erep.colors[gprim] == igroup) {
                    gprimList = gprim.trim().split(" ");

                    mesg += gprimList[gprimList.length-1] + ";";
                    colored--;
                }
            }
        }

        if (colored <= 0) {
            break;
        }
    }
    
    browserToServer(mesg+"0|");
};


//
// callback when "ErepEd->Save" is pressed (called by ESP.html)
//
erep.cmdSave = function () {
    // alert("in erep.cmdSave()");

    var ibrch    = brch.length;
    var nchange  = 0;
    var prevBody = -1;

    // close the ErepEd menu
    var menu = document.getElementsByClassName("doneMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showDoneMenu")) {
            menu[i].classList.remove(  "showDoneMenu");
        }
    }

    // we cannot save if we are currently viewing the EBodys
    var button = document.getElementById("solveButton");
    if (button["innerHTML"] != "ShowEBodys") {
        alert("You must be in editing mode to Save (press "+button["innerHTML"]+")");
        return;
    }

    // if we do not have a dihedral yet, ask for it now
    if (erep.dihedral < 0) {
        var dihedral = prompt("Enter maximum dihedral angle", "5");
        if (dihedral === null) {
            return;
        } else if (isNaN(dihedral)) {
            alert("Illegal number format");
            return;
        } else if (dihedral < 0 || dihedral > 90) {
            alert("Out of range (0-90)");
            return;
        }

        erep.dihedral = dihedral;
    }

    // send the changed attributes on Faces back to the server
    for (var gprim in wv.sceneGraph) {
        if (gprim.indexOf(" Face ") < 0) continue;
        
        var icolor = erep.colors[gprim];
        if (icolor > 0) {

            // skip if we already have the attribute
            var attrs = wv.sgData[gprim];
            for (var i = 0; i < attrs.length; i+=2) {
                if (attrs[i] == erep.attrName && attrs[i+1] == icolor) {
                    icolor = 0;
                    break;
                }
            }
            if (icolor == 0) {
                continue;
            }

            // add SELECT and ATTRIBUTE statements
            var nameList = gprim.trim().split(" ");

            if (nameList.length == 4) {
                postMessage("adding new atribute to "+gprim);

                if (nameList[1] != prevBody) {
                    browserToServer("newBrch|"+ibrch+"|select|$BODY|"+nameList[1]+"|||||||||");
                    ibrch++;

                    prevBody = nameList[1];

                    browserToServer("setAttr|"+ibrch+"|_erepAttr|1|$"+erep.attrName+"|");
                    browserToServer("setAttr|"+ibrch+"|_erepAngle|1|"+erep.dihedral+"|");
                }

                browserToServer("newBrch|"+ibrch+"|select|$FACE|"+nameList[3]+"|||||||||");
                ibrch++;

                browserToServer("setAttr|"+ibrch+"|"+erep.attrName+"|1|"+icolor+"|");

                nchange++;
            } else {
                alert("could not set Attribute on named (unnumbered) Body");
            }
        }
    }

    // send the changes to the .Keep attribute on Edges back to the server
    for (var gprim in wv.sceneGraph) {
        if (gprim.indexOf(" Edge ") < 0) continue;

        try {
            var attrs = wv.sgData[gprim];

            // Edge is black (so it should not be kept)
            if (erep.colors[gprim] == -1) {
                for (var i = 0; i < attrs.length; i+=2) {
                    if (attrs[i] == ".Keep") {
                        postMessage("removing .Keep from "+gprim);
                        
                        var nameList = gprim.trim().split(" ");

                        browserToServer("newBrch|"+ibrch+"|select|$BODY|"+nameList[1]+"|||||||||");
                        ibrch++;

                        browserToServer("newBrch|"+ibrch+"|select|$EDGE|"+nameList[3]+"|||||||||");
                        ibrch++;

                        browserToServer("setAttr|"+ibrch+"|.Keep|1|<DeLeTe>|");

                        nchange++;
                        
                        break;
                    }
                }

            // Edge is red (so it should be kept)
            } else {
                var found = 0;
                for (var i = 0; i < attrs.length; i+=2) {
                    if (attrs[i] == ".Keep") {
                        found++;
                        break;
                    }
                }

                if (found == 0) {
                    postMessage("adding .Keep to "+gprim);
                    
                    var nameList = gprim.trim().split(" ");
                    
                    browserToServer("newBrch|"+ibrch+"|select|$BODY|"+nameList[1]+"|||||||||");
                    ibrch++;
                    
                    browserToServer("newBrch|"+ibrch+"|select|$EDGE|"+nameList[3]+"|||||||||");
                    ibrch++;
                    
                    browserToServer("setAttr|"+ibrch+"|.Keep|1|1|");
                    
                    nchange++;
                }
            }
        } catch (e) {
        }
    }

    // send the changes to the .Keep attribute on Nodes back to the server
    for (var gprim in wv.sceneGraph) {
        if (gprim.indexOf(" Node ") < 0) continue;
        
        // Node is black (so it should not be kept)
        if (erep.colors[gprim] == -1) {
            for (var i = 0; i < attrs.length; i+=2) {
                if (attrs[i] == ".Keep") {
                    postMessage("removing .Keep from "+gprim);
                        
                    var nameList = gprim.trim().split(" ");

                    browserToServer("newBrch|"+ibrch+"|select|$BODY|"+nameList[1]+"|||||||||");
                    ibrch++;

                    browserToServer("newBrch|"+ibrch+"|select|$NODE|"+nameList[3]+"|||||||||");
                    ibrch++;

                    browserToServer("setAttr|"+ibrch+"|.Keep|1|<DeLeTe>|");

                    nchange++;

                    break;
                }
            }

        // Node is red (so it should be kept)
        } else {
            var found = 0;
            for (var i = 0; i < attrs.length; i+=2) {
                if (attrs[i] == ".Keep") {
                    found++;
                    break;
                }
            }

            if (found == 0) {
                postMessage("adding .Keep to "+gprim);
                        
                var nameList = gprim.trim().split(" ");

                browserToServer("newBrch|"+ibrch+"|select|$BODY|"+nameList[1]+"|||||||||");
                ibrch++;

                browserToServer("newBrch|"+ibrch+"|select|$NODE|"+nameList[3]+"|||||||||");
                ibrch++;

                browserToServer("setAttr|"+ibrch+"|.Keep|1|1|");

                nchange++;
            }
        }
    }

    // add a DUMP if any changes were made
//    if (nchange > 0) {
        browserToServer("newBrch|"+ibrch+"|dump|"+erep.attrName+".egads|||||||||");

        postMessage("====> Re-build is needed <====");
//    }

    // update the key window if any changes were made
    if (nchange > 0) {
        erep.updateKeyWindow();
    }

    changeMode(0);

    // get an updated version of the Branches and activate Build button
    wv.brchStat = 0;

    activateBuildButton();
};


//
// callback when "ErepEd->Quit" is pressed (called by ESP.html)
//
erep.cmdQuit = function () {
    // alert("in erep.cmdQuit()");

    // close the ErepEd menu
    var menu = document.getElementsByClassName("doneMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showDoneMenu")) {
            menu[i].classList.remove(  "showDoneMenu");
        }
    }

    // change solve button legend
    var button = document.getElementById("solveButton");
    button["innerHTML"] = "Up to date";
    button.style.backgroundColor = "#FFFFFF";

    changeMode(0);

    // get original colors back
    browserToServer("build|-1|");
};


//
// callback when "homeButton" is pressed (calles by ESP.html)
//
//erep.cmdHome = function () {
//    main.cmdHome();
//};


//
// callback when "leftButton" is pressed (calles by ESP.html)
//
//erep.cmdLeft = function () {
//    main.cmdLeft();
//};


//
// callback when "riteButton" is pressed (calles by ESP.html)
//
//erep.cmdRite = function () {
//    main.cmdRite();
//};


//
// callback when "botmButton" is pressed (calles by ESP.html)
//
//erep.cmdBotm = function () {
//    main.cmdBotm();
//};


//
// callback when "topButton" is pressed (calles by ESP.html)
//
//erep.cmdTop = function () {
//    main.cmdTop();
//};


//
// callback when "inButton" is pressed (calles by ESP.html)
//
//erep.cmdIn = function () {
//    main.cmdIn();
//};


//
// callback when "outButton" is pressed (calles by ESP.html)
//
//erep.cmdOut = function () {
//    main.cmdOut();
//};


//
// callback when a key is pressed
//
//erep.keyPress = function (e) {
//    main.keyPress(e);
//};


//
// callback when an arror... or shift is pressed (needed for Chrome)
//
//erep.keyDown = function (e) {
//    main.keyDown(e);
//};


//
// callback when a shift is released (needed for Chrome)
//
//erep.keyUp = function (e) {
//    main.keyUp(e);
//};


//
// callback when the scene graph has been updated
//
erep.sceneUpdated = function () {
    // alert("in erep.sceneUpdated()");
    
    var button = document.getElementById("solveButton");
    if (button["innerHTML"] != "EditErep") {
        erep.redraw();
    }
};


//
// callback for first part of a keypress that is not recognized by wvUpdateUI
//
erep.keyPressPart1 = function(myKeyPress) {
    // alert("in erep.keyPressPart1(myKeyPress="+myKeyPress+")");

    var button = document.getElementById("solveButton");
    var done = 0;

    if (myKeyPress == 'c') {
        if (button["innerHTML"] != "ShowEBodys") {
            alert("You must be in editing mode (press "+button["innerHTML"]+")");
            return;
        }
    
        wv.picking  = 99;
        wv.pick     = 1;
        wv.sceneUpd = 1;

        done = 1;
    } else if (myKeyPress == 'k') {
        if (button["innerHTML"] != "ShowEBodys") {
            alert("You must be in editing mode (press "+button["innerHTML"]+")");
            return;
        }
    
        wv.picking  = 107;
        wv.pick     = 1;
        wv.sceneUpd = 1;

        done = 1;
    } else if (myKeyPress == 'C') {
        if (button["innerHTML"] != "ShowEBodys") {
            alert("You must be in editing mode (press "+button["innerHTML"]+")");
            return;
        }
    
        var attrName = prompt("Enter Attribute name to import");
        if (attrName === null) {
            return;
        }
        
        var attrValu = prompt("Enter Attribute value to import (or -1 for all)");
        if (attrValu === null) {
            return;
        }

        // user provided an attribute value
        var nchange = 0;
        
        if (attrValu > 0) {
            var newColor = prompt("Enter <0 for first unused color\n"+
                                  "or    =0 to uncolor\n"+
                                  "or    >0 to select color", erep.lastColor);
            if (newColor === null) {
                return;
            }

            // if user responded with negative, find first available color
            var icolor = 1;
            while (newColor < 0) {
                newColor = icolor;
                for (var gprim1 in wv.sceneGraph) {
                    if (erep.colors[gprim1] == newColor) {
                        newColor = -1;
                        icolor++;
                        break;
                    }
                }
            }

            for (var gprim in wv.sceneGraph) {
                var gprimList = gprim.trim().split(" ");
                if        ((gprimList.length == 3 && gprimList[1] == "Face") ||
                           (gprimList.length == 4 && gprimList[2] == "Face")   ) {
                    try {
                        var attrs = wv.sgData[gprim];

                        for (var i = 0; i < attrs.length; i+=2) {
                            if (attrs[i] == attrName && Number(attrs[i+1]) == Number(attrValu)) {
                                if (erep.colors[gprim] != newColor) {
                                    erep.colors[gprim] = newColor;
                                    erep.uncolored--;

                                    if (newColor == 0) {
                                        icolor = 0;
                                    } else {
                                        icolor = 1 + (newColor - 1) % 12;
                                    }
                                    wv.sceneGraph[gprim].fColor = [erep.spectrum[3*icolor], erep.spectrum[3*icolor+1], erep.spectrum[3*icolor+2]];
                                    
                                    nchange++;
                                    postMessage("Changed color of "+gprim+" to "+newColor);
                                }
                            }
                        }
                    } catch (x) {
                        var attrs = [];
                    }
                }
            }

            // remember last color (for next time)
            erep.lastColor = newColor;

        // user asked for all
        } else {
            var newColor = prompt("Enter first color (or -1 for fist available)", "-1");
            if (newColor === null) {
                return;
            }

            // if user responded with negative, find last color
            newColor = 0;
            for (var gprim1 in wv.sceneGraph) {
                if (erep.colors[gprim1] > newColor) {
                    newColor = erep.colors[gprim1];
                }
            }

            for (var gprim in wv.sceneGraph) {
                var gprimList = gprim.trim().split(" ");
                if        ((gprimList.length == 3 && gprimList[1] == "Face") ||
                           (gprimList.length == 4 && gprimList[2] == "Face")   ) {
                    try {
                        var attrs = wv.sgData[gprim];

                        for (var i = 0; i < attrs.length; i+=2) {
                            if (attrs[i] == attrName) {
                                var tempColor = Number(newColor) + Number(attrs[i+1]);
                                if (erep.colors[gprim] != tempColor) {
                                    erep.colors[gprim] = tempColor;
                                    erep.uncolored--;

                                    icolor = 1 + (tempColor - 1) % 12;
                                    wv.sceneGraph[gprim].fColor = [erep.spectrum[3*icolor], erep.spectrum[3*icolor+1], erep.spectrum[3*icolor+2]];
                                    
                                    nchange++;
                                    postMessage("Changed color of "+gprim+" to "+tempColor);
                                }
                            }
                        }
                    } catch (x) {
                        var attrs = [];
                    }
                }
            }
        }

        if (nchange > 0) {
            // update the Key Window
            erep.updateKeyWindow();

            // update the picture
            wv.sceneUpd = 1;
        }

        done = 1;
    }

    return done;
};


//
// callback for second part of a keypress that is not recognized by wvUpdateUI
//
erep.keyPressPart2 = function(picking, gprim) {
    // alert("in erep.keyPressPart2(picking="+picking+"   gprim="+gprim+")");

    // second part of 'c' operation
    if (picking == 99) {
        if (gprim.indexOf(" Face ") < 0) {
            alert("Only Faces can be colored");
            return;
        }
        
        var oldColor  = erep.colors[gprim];
        var nextColor = oldColor;

        // Face is currently not colored, so suggest last color (or 1 if first time)
        if (nextColor == 0) {
            if (erep.lastColor < 0) {
                nextColor = 1;
            } else {
                nextColor = erep.lastColor;
            }
        }

        // ask user for new color
        var newColor = prompt("Enter <0 for first unused color\n"+
                              "or    =0 to uncolor\n"+
                              "or    >0 to select color", nextColor);
        if (newColor === null) {
            return;
        }

        // if user responded with negative, find first available color
        var icolor = 1;
        while (newColor < 0) {
            newColor = icolor;
            for (var gprim1 in wv.sceneGraph) {
                if (erep.colors[gprim1] == newColor) {
                    newColor = -1;
                    icolor++;
                    break;
                }
            }
        }

        // save the color and change the color of the Face
        erep.colors[gprim] = newColor;
        postMessage("Changed color of "+gprim+" to "+newColor);

        if (newColor == 0) {
            icolor = 0;
        } else {
            icolor = 1 + (newColor - 1) % 12;
        }
        wv.sceneGraph[gprim].fColor = [erep.spectrum[3*icolor], erep.spectrum[3*icolor+1], erep.spectrum[3*icolor+2]];

        // remember last color (for next time)
        erep.lastColor = newColor;

        // adjust number of colored Faces
        if (oldColor == 0 && newColor > 0) {
            erep.uncolored--;
        } else if (oldColor > 0 && newColor == 0) {
            erep.uncolored++;
        }

        // update the Key Window
        erep.updateKeyWindow();

    // second part of 'k' operation
    } else if (picking == 107) {
        if (gprim.indexOf(" Node ") < 0 && gprim.indexOf(" Edge ") < 0) {
            alert("Only Nodes and Edges can be kept");
            return;
        }

        var isEdge = 1;
        if (gprim.indexOf(" Node ") >= 0) {
            isEdge = 0;
        }

        // toggle the color of a Node
        if (isEdge == 0) {
            if (erep.colors[gprim] == -1) {
                wv.sceneGraph[gprim].pColor = [1., 0., 0.];
                erep.colors[gprim] = -3;
                
                postMessage("Marking "+gprim+" to be kept");
                erep.keptNodes++;
                erep.updateKeyWindow();
            } else {
                wv.sceneGraph[gprim].pColor = [0., 0., 0.];
                erep.colors[gprim] = -1;

                postMessage("Unmarking "+gprim+" to be kept");
                erep.keptNodes--;
                erep.updateKeyWindow();
            }
        } else {
            if (erep.colors[gprim] == -1) {
                wv.sceneGraph[gprim].lColor = [1., 0., 0.];
                erep.colors[gprim] = -2;

                postMessage("Marking "+gprim+" to be kept");
                erep.keptEdges++;
                erep.updateKeyWindow();
            } else {
                wv.sceneGraph[gprim].lColor = [0., 0., 0.];
                erep.colors[gprim] = -1;

                postMessage("Unmarking "+gprim+" to be kept");
                erep.keptEdges--;
                erep.updateKeyWindow();
            }
        }
    }
};


//
// function to update the key window
//
erep.updateKeyWindow = function () {
    var mesg = "Erep Editor\n\n";

    mesg +=    "attrName          = " + erep.attrName  + "\n";
    mesg +=    "# uncolored Faces = " + erep.uncolored + "  (in grey)\n";
    mesg +=    "# kept Nodes      = " + erep.keptNodes + "  (in red)\n";
    mesg +=    "# kept Edges      = " + erep.keptEdges + "  (in red)\n\n";

    mesg +=    "Valid commands:\n";
    mesg +=    "  'c' color or uncolor a Face\n";
    mesg +=    "  'C' color Faces by attribute\n";
    mesg +=    "  'k' keep or unkeep a Node or Edge";

    var pre  = document.createElement("pre");
    var text = document.createTextNode(mesg);
    pre.appendChild(text);

    var erepstat = document.getElementById("erepedStatus");
    
    erepstat.replaceChild(pre, erepstat.lastChild);
}

// /////////////////////////////////////////////////////////////////////


//
// Initialize ErepEd
//
erep.initialize = function (getNewName) {
    // alert("in erep.initialize(getNewName="+getNewName+")");

    if (getNewName == 1) {
        var attrName = prompt("Enter attribute name:", erep.attrName);
        if (attrName === null) {
            return 1;
        } else if (attrName.length <= 0) {
            return 1;
        } else {
            erep.attrName = attrName;
        }
    }

    // make sure that any Faces that have erep.attrName have it as
    //    only a single number
    for (var gprim in wv.sceneGraph) {
        var gprimList = gprim.trim().split(" ");
        if        ((gprimList.length == 3 && gprimList[1] == "Face") ||
                   (gprimList.length == 4 && gprimList[2] == "Face")   ) {
            if (erep.colors[gprim] == -1) {
                erep.colors[gprim] = 0;
                erep.uncolored++;

                try {
                    var attrs = wv.sgData[gprim];

                    for (var i = 0; i < attrs.length; i+=2) {
                        if (attrs[i] == erep.attrName) {
                            if (parseInt(attrs[i+1]) != Number(attrs[i+1])) {
                                alert("\""+erep.attrName+"\" cannot be used because "+gprim+
                                      "\nalready has that attribute, and it is not a single integer");
                                erep.attrname = "";
                                return 0;
                            } else if (Number(attrs[i+1]) < 0) {
                                alert("\""+erep.attrName+"\" cannot be used because "+gprim+
                                      "\nalready has that attribute, and it has a negative value");
                                erep.attrname = "";
                                return 0;
                            } else {
                                erep.colors[gprim] = Number(attrs[i+1]);
                                erep.uncolored--;
                            }
                        }
                    }
                } catch (e) {
                }
            }
        }
    }

    erep.redraw();

    // change mode
    changeMode(10);

    // update the Key Window
    erep.updateKeyWindow();

    return 0;
};


//
// redraw the erep
//
erep.redraw = function () {
    // alert("in erep.redraw()");

    for (var gprim in wv.sceneGraph) {
        var gprimList = gprim.trim().split(" ");

        // color Edges black (or red if they have a .Keep attribute */
        if ((gprimList.length == 3 && gprimList[1] == "Edge") ||
            (gprimList.length == 4 && gprimList[2] == "Edge")   ) {
            if (erep.colors[gprim] == -2) {
                wv.sceneGraph[gprim].lColor = [1.0, 0.0, 0.0];
            } else {
                wv.sceneGraph[gprim].lColor = [0.0, 0.0, 0.0];
                erep.colors[  gprim] = -1;

                try {
                    var attrs = wv.sgData[gprim];

                    for (var i = 0; i < attrs.length; i+=2) {
                        if (attrs[i] == ".Keep") {
                            erep.keptEdges++;
                            wv.sceneGraph[gprim].lColor = [1.0, 0.0, 0.0];
                            erep.colors[  gprim] = -2;
                            break;
                        }
                    }
                } catch (x) {
                }
            }

        // color Nodes black (or red if they have a .Keep attribute */
        } else if ((gprimList.length == 3 && gprimList[1] == "Node") ||
                   (gprimList.length == 4 && gprimList[2] == "Node")   ) {
            if (erep.colors[gprim] == -3) {
                wv.sceneGraph[gprim].pColor = [1.0, 0.0, 0.0];
            } else {
                wv.sceneGraph[gprim].pColor = [0.0, 0.0, 0.0];
                erep.colors[  gprim] = -1;

                try {
                    var attrs = wv.sgData[gprim];

                    for (var i = 0; i < attrs.length; i+=2) {
                        if (attrs[i] == ".Keep") {
                            erep.keptNodes++;
                            wv.sceneGraph[gprim].pColor = [1.0, 0.0, 0.0];
                            erep.colors[  gprim] = -3;
                            break;
                        }
                    }
                } catch (x) {
                }
            }
        }
    }

    // color the Faces
    for (gprim in wv.sceneGraph) {
        if (erep.colors[gprim] == 0) {
            wv.sceneGraph[gprim].fColor = [erep.spectrum[0], erep.spectrum[1], erep.spectrum[2]];
        } else if (erep.colors[gprim] > 0) {
            var icolor = 1 + (erep.colors[gprim]-1) % 12;
            wv.sceneGraph[gprim].fColor = [erep.spectrum[3*icolor], erep.spectrum[3*icolor+1], erep.spectrum[3*icolor+2]];
        }
    }
};

