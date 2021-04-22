// ESP-gloves.js implements Gloves functions for the Engineering Sketch Pad (ESP)
// written by John Dannenhoffer and Bob Haimes

// Copyright (C) 2010/2020  John F. Dannenhoffer, III (Syracuse University)
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

// interface functions that ESP-gloves.js provides
//    glov.launch()
//    glov.initialize()
//    glov.cmdLoad()
//    glov.cmdUndo()
//    glov.cmdSolve()
//    glov.cmdSave()
//    glov.cmdQuit()
//
//    glov.cmdHome()
//    glov.cmdLeft()
//    glov.cmdRite()
//    glov.cmdBotm()
//    glov.cmdTop()
//    glov.cmdIn()
//    glov.cmdOut()
//
//    glov.mouseDown(e)
//    glov.mouseMove(e)
//    glov.mouseUp(e)
//
//    glov.keyPress(e)
//    glov.getKeyDown(e)
//    glov.getKeyUp(e)

// functions associated with Gloves
//    glovesCompCoords()
//    glovesDraw()
//    glovesDrawOutline()

"use strict";


//
// callback when Gloves is launched
//
glov.launch = function () {
    // alert("glov.launch()");

    // close the Tools menu
    var menu = document.getElementsByClassName("toolMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showToolMenu")) {
            menu[i].classList.remove(  "showToolMenu");
        }
    }

    // change done button legend
    var button = document.getElementById("doneMenuBtn");
    button["innerHTML"] = "Gloves";
    button.style.backgroundColor = "#3FFF3F";

    // change solve button legend
    button = document.getElementById("solveButton");
    button["innerHTML"] = "Update";
    button.style.backgroundColor = "#FFFF3F";

    // initialize Gloves
    glov.initialize();

    // show Gloves
    glovesDraw();

    // set mode to Gloves
    changeMode(8);

}


//
// Initialize Gloves
//
glov.initialize = function () {
    // alert("glov.initialize");

    // initialize points
    glov.xpnt      = [];         // x-screen coordinates of points
    glov.ypnt      = [];         // y-screen coordinates of points
    glov.labl      = {};         // list of variables that can be changed at each point

    // initialize GUI variables
    glov.mode      =  0;         // current mode
                                 // =0 initialization
                                 // =1 point selected and menu posted
                                 // =2 design variable selected and drag basis defined
    glov.offsetX   = 100;        // x-screen = glov.offsetX + X * glov.scale
    glov.offsetY   = 200;        // y-screen = glov.offsetY - y * glov.scale
    glov.scale     =  50;
    glov.cursorX   = -1;         // x-screen coordinate of cursor
    glov.cursorY   = -1;         // y-screen coordinate of cursor
    glov.dragX     = undefined;  // x-screen at start of drag
    glov.dragY     = undefined;  // y-screen at start of drag
    glov.dragBase  = undefined;  // variable value at start of drag
    glov.dragging  = false;      // true during drag operation
    glov.startX    =  0;         // start of dragging operation
    glov.startY    =  0;
    glov.button    = -1;         // button pressed
    glov.modifier  =  0;         // modifier (shift,alt,cntl) bitflag
    glov.flying    =  1;         // flying multiplier ( do not set to 0)
    glov.halo      = 10;         // size of halo around points

    glov.menuitems = {};         // bounding box of each menu item

    glov.curPnt    = -1;         // point that is currently highlighted (or -1)
    glov.curMenu   = -1;         // menu item that is currently highlighed (or -1)
    glov.curDvar   = -1;         // design variable being editted (or -1)

    glov.uiMatrix  = new J3DIMatrix4();

    // set up mouse listeners
    var canvas = document.getElementById("gloves");
    canvas.addEventListener('mousedown',  glov.getMouseDown,    false);
    canvas.addEventListener('mousemove',  glov.getMouseMove,    false);
    canvas.addEventListener('mouseup',    glov.getMouseUp,      false);
    canvas.addEventListener("wheel",      glov.getMouseRoll,    false);
    canvas.addEventListener('mouseout',   glov.mouseLeftCanvas, false);

    document.addEventListener("keypress",   glov.getKeyPress,     false);
    document.addEventListener("keydown",    glov.getKeyDown,      false);
    document.addEventListener("keyup",      glov.getKeyUp,        false);

    // initialize context properties
    var context = canvas.getContext("2d");
    context.font         = "14px Verdana";
    context.textAlign    = "left";
    context.textBaseline = "top";
    context.lineCap      = "round";
    context.lineJoin     = "round";

    // initialize design variables
    glov.cmdLoad();

    // initialize uiMatrix
//    glov.uiMatrix.makeIdentity();
//    glov.uiMatrix.scale(glov.scale);
    var mVals = new Array (53,-48,  5, 0,
                           33, 42, 48, 0,
                          -36,-33, 53, 0,
                            0,  0,  0, 1);
    glov.uiMatrix.load(mVals);
    glov.offsetX = 100;
    glov.offsetY = 500;
    glov.scale   = 72;

    // update the screen
    glovesDraw();
}


//
// glov.cmdload - initialize design variables
//
glov.cmdLoad = function () {
    glov.dvar = [];

    glov.dvar[ 0] = {};
    glov.dvar[ 0].name = "fuse:length";
    glov.dvar[ 0].valu = 10.0;
    glov.dvar[ 0].lbnd = 1.0;
    glov.dvar[ 0].ubnd = 20.0;

    glov.dvar[ 1] = {};
    glov.dvar[ 1].name = "fuse:width";
    glov.dvar[ 1].valu = 2.0;
    glov.dvar[ 1].lbnd = 0.1;
    glov.dvar[ 1].ubnd = 10.0;

    glov.dvar[ 2] = {};
    glov.dvar[ 2].name = "fuse:height";
    glov.dvar[ 2].valu = 2.0;
    glov.dvar[ 2].lbnd = 0.1;
    glov.dvar[ 2].ubnd = 10.0;

    glov.dvar[ 3] = {};
    glov.dvar[ 3].name = "wing:Xroot";
    glov.dvar[ 3].valu = 2.0;
    glov.dvar[ 3].lbnd = 0.1;
    glov.dvar[ 3].ubnd = 20.0;

    glov.dvar[ 4] = {};
    glov.dvar[ 4].name = "wing:Zroot";
    glov.dvar[ 4].valu = 0.9;
    glov.dvar[ 4].lbnd = 0.1;
    glov.dvar[ 4].ubnd = 10.0;

    glov.dvar[ 5] = {};
    glov.dvar[ 5].name = "wing:area";
    glov.dvar[ 5].valu = 24.0;
    glov.dvar[ 5].lbnd = 0.1;
    glov.dvar[ 5].ubnd = 100.0;

    glov.dvar[ 6] = {};
    glov.dvar[ 6].name = "wing:aspect";
    glov.dvar[ 6].valu = 5.0;
    glov.dvar[ 6].lbnd = 0.1;
    glov.dvar[ 6].ubnd = 20.0;

    glov.dvar[ 7] = {};
    glov.dvar[ 7].name = "wing:taper";
    glov.dvar[ 7].valu = 0.8;
    glov.dvar[ 7].lbnd = 0.1;
    glov.dvar[ 7].ubnd = 2.0;

    glov.dvar[ 8] = {};
    glov.dvar[ 8].name = "wing:sweep";
    glov.dvar[ 8].valu = 15.0;
    glov.dvar[ 8].lbnd = -45.0;
    glov.dvar[ 8].ubnd =  45.0;

    glov.dvar[ 9] = {};
    glov.dvar[ 9].name = "wing:dihedral";
    glov.dvar[ 9].valu = 5.0;
    glov.dvar[ 9].lbnd = -10.0;
    glov.dvar[ 9].ubnd =  10.0;

    glov.dvar[10] = {};
    glov.dvar[10].name = "wing:thick";
    glov.dvar[10].valu = 0.2;
    glov.dvar[10].lbnd = 0.01;
    glov.dvar[10].ubnd = 1.0;

    glov.dvar[11] = {};
    glov.dvar[11].name = "htail:Xroot";
    glov.dvar[11].valu = 8.5
    glov.dvar[11].lbnd = 0.1;
    glov.dvar[11].ubnd = 20.0;

    glov.dvar[12] = {};
    glov.dvar[12].name = "htail:Zroot";
    glov.dvar[12].valu = 0.9;
    glov.dvar[12].lbnd = 0.1;
    glov.dvar[12].ubnd = 10.0;

    glov.dvar[13] = {};
    glov.dvar[13].name = "htail:area";
    glov.dvar[13].valu = 3.0;
    glov.dvar[13].lbnd = 0.1;
    glov.dvar[13].ubnd = 100.0;

    glov.dvar[14] = {};
    glov.dvar[14].name = "htail:aspect";
    glov.dvar[14].valu = 5.0;
    glov.dvar[14].lbnd = 0.1;
    glov.dvar[14].ubnd = 20.0;

    glov.dvar[15] = {};
    glov.dvar[15].name = "htail:taper";
    glov.dvar[15].valu = 0.8;
    glov.dvar[15].lbnd = 0.1;
    glov.dvar[15].ubnd = 2.0;

    glov.dvar[16] = {};
    glov.dvar[16].name = "htail:sweep";
    glov.dvar[16].valu = 15.0;
    glov.dvar[16].lbnd = -45.0;
    glov.dvar[16].ubnd =  45.0;

    glov.dvar[17] = {};
    glov.dvar[17].name = "htail:dihedral";
    glov.dvar[17].valu = 5.0;
    glov.dvar[17].lbnd = -10.0;
    glov.dvar[17].ubnd =  10.0;

    glov.dvar[18] = {};
    glov.dvar[18].name = "htail:thick";
    glov.dvar[18].valu = 0.2;
    glov.dvar[18].lbnd = 0.01;
    glov.dvar[18].ubnd = 1.0;
};


//
// callback when "Gloves->Undo" is pressed (called by ESP.html)
//
glov.cmdUndo = function () {
    alert("glov.cmdUndo is not implemented");
}


//
// callback when "solveButton" is pressed
//
glov.cmdSolve = function () {
    alert("glov.cmdSolve is not implmented\nIt will update the underlying config");
}


//
// callback when "Gloves->Save" is pressed (called by ESP.html)
//
glov.cmdSave = function () {
    alert("glov.cmdSave is not implemented\nUse Quit button instead");
}


//
// callback when "Gloves->Quit" is pressed (called by ESP.html)
//
glov.cmdQuit = function () {
    // alert("in glov.cmdQuit()");

    // close the Gloves menu
    var menu = document.getElementsByClassName("doneMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showDoneMenu")) {
            menu[i].classList.remove(  "showDoneMenu");
        }
    }

    if (wv.curMode != 8) {
        alert("Command disabled.  Press 'Cancel' or 'OK' first");
        return;
    } else if (wv.server != "serveCSM") {
        alert("cmdQuit is not implemented for "+wv.server);
        return;
    }

    changeMode(0);
    activateBuildButton();
}


//
// callback when "homeButton" is pressed
//
glov.cmdHome = function () {
    var mVals = new Array (53,-48,  5, 0,
                           33, 42, 48, 0,
                          -36,-33, 53, 0,
                            0,  0,  0, 1);
    glov.uiMatrix.load(mVals);
    glov.offsetX = 100;
    glov.offsetY = 500;
    glov.scale   = 72;
}


//
// callback when "leftButton" is pressed
//
glov.cmdLeft = function () {
}


//
// callback when "riteButton" is pressed
//
glov.cmdRite = function () {
}


//
// callback when "botmButton" is pressed
//
glov.cmdBotm = function () {
}


//
// callback when "topButton" is pressed
//
glov.cmdTop = function () {
}


//
// callback when "inButton" is pressed
//
glov.cmdIn = function () {
}


//
// callback when "outButton" is pressed
//
glov.cmdOut = function () {
}


//
// callback when any mouse is pressed  (when wv.curMode==7)
//
glov.mouseDown = function (e) {
    // alert("glov.mouseDown(e="+e+")");

    if (!e) var e = event;

                    glov.modifier  = 0;
    if (e.shiftKey) glov.modifier |= 1;
    if (e.altKey  ) glov.modifier |= 2;
    if (e.ctrlKey ) glov.modifier |= 4;

    // turn on dragging mode
    if (glov.modifier != 0) {
        var canvas = document.getElementById("gloves");

        glov.startX   = e.clientX - canvas.offsetLeft - 1;
        glov.startY   = e.clientY - canvas.offsetTop  - 1;
        glov.dragging = true;
        glov.button   = e.button;
    }
}


//
// callback when the mouse moves
//
glov.mouseMove = function (e) {
    // alert("glov.mouseMove(e="+e+")");

    if (!e) var e = event;

    var canvas = document.getElementById("gloves");

    glov.cursorX  = e.clientX - canvas.offsetLeft - 1;
    glov.cursorY  = e.clientY - canvas.offsetTop  - 1;

    // deal with mouse movement
    if (glov.modifier && glov.dragging) {

        // cntrl is down (rotate)
        if (glov.modifier == 4) {
            var angleX =  (glov.startY - glov.cursorY) / 4.0 / glov.flying;
            var angleY =  (glov.startX - glov.cursorX) / 4.0 / glov.flying;
            if ((angleX != 0.0) || (angleY != 0.0)) {
                glov.uiMatrix.rotate(angleX, 1,0,0);
                glov.uiMatrix.rotate(angleY, 0,1,0);
            }

        // alt-shift is down (rotate)
        } else if (glov.modifier == 3) {
            var angleX =  (glov.startY - glov.cursorY) / 4.0 / glov.flying;
            var angleY =  (glov.startX - glov.cursorX) / 4.0 / glov.flying;
            if ((angleX != 0.0) || (angleY != 0.0)) {
                glov.uiMatrix.rotate(angleX, 1,0,0);
                glov.uiMatrix.rotate(angleY, 0,1,0);
            }

        // alt is down (translate)
        } else if (glov.modifier == 2) {
            var transX = (glov.cursorX - glov.startX) / glov.flying;
            var transY = (glov.cursorY - glov.startY) / glov.flying;
            glov.offsetX += transX;
            glov.offsetY += transY;

        // shift is down (zoom)
        } else if (glov.modifier == 1) {
            if (glov.cursorY != glov.startY) {
                var scale = Math.exp((glov.startY - glov.cursorY) / 512.0 / glov.flying);
                glov.uiMatrix.scale(scale, scale, scale);
                glov.scale *= scale;
            }
        }

        // if not flying, then update the start coordinates
        if (glov.flying <= 1) {
            glov.startX = glov.cursorX;
            glov.startY = glov.cursorY;
        }

        glovesDraw();
        return;
    }

    // highlight point within halo
    if (glov.mode == 0) {
        for (var ipnt = 0; ipnt < glov.xpnt.length; ipnt++) {
            if (Math.abs(glov.xpnt[ipnt]-glov.cursorX) < glov.halo &&
                Math.abs(glov.ypnt[ipnt]-glov.cursorY) < glov.halo   ) {
                glov.curPnt = ipnt;
                glov.mode   = 1;
                break;
             }
        }

    // select menu item
    } else if (glov.mode == 1) {
        glov.curMenu = -1;
        glov.curDvar = -1;
        for (var item = 0; item < glov.menuitems.length; item++) {
            if (glov.cursorX >= glov.menuitems[item].xmin-glov.halo &&
                glov.cursorX <= glov.menuitems[item].xmax+glov.halo &&
                glov.cursorY >= glov.menuitems[item].ymin           &&
                glov.cursorY <= glov.menuitems[item].ymax             ) {
                glov.curMenu = item;
                for (var idvar = 0; idvar < glov.dvar.length; idvar++) {
                    if (glov.dvar[idvar].name == glov.menuitems[item].labl) {
                        glov.curDvar = idvar;
                        break;
                    }
                }

                break;
            }
        }

        if (glov.curMenu < 0) {
            if (Math.abs(glov.xpnt[glov.curPnt]-glov.cursorX) > glov.halo ||
                Math.abs(glov.ypnt[glov.curPnt]-glov.cursorY) > glov.halo   ) {
                glov.mode = 0;
            }
        }

    // update design variable by tracking mouse
    } else if (glov.mode == 2) {

        // vary design variable until selected point is at cursor
        var idvar = glov.curDvar;
        var dvar0 = glov.dvar[idvar].valu;
        var dist0 = (glov.xpnt[glov.curPnt] - glov.cursorX) * (glov.xpnt[glov.curPnt] - glov.cursorX)
                  + (glov.ypnt[glov.curPnt] - glov.cursorY) * (glov.ypnt[glov.curPnt] - glov.cursorY);
        var delta = 0.1 * Math.max(dvar0, 1);
        if (dvar0 >= glov.dvar[idvar].ubnd) {
            delta *= -1;
        }

        while (dist0 > 5) {

            // modify the design variable and compute distance to cursor
            var dvar1 = Math.min(Math.max(glov.dvar[idvar].lbnd, dvar0+delta), glov.dvar[idvar].ubnd);
            glov.dvar[idvar].valu = dvar1;
            glovesCompCoords();
            var dist1 = (glov.xpnt[glov.curPnt] - glov.cursorX) * (glov.xpnt[glov.curPnt] - glov.cursorX)
                      + (glov.ypnt[glov.curPnt] - glov.cursorY) * (glov.ypnt[glov.curPnt] - glov.cursorY);

            // propose next step
            if (dvar1 <= glov.dvar[idvar].lbnd || dvar1 >= glov.dvar[idvar].ubnd) {
                break;
            } else if (Math.abs(dist0 - dist1) < 5) {
                break;
            } else if (dist1 < dist0) {
                dvar0  = dvar1;
                dist0  = dist1;
                delta *= 1.2;
            } else {
                delta *= -.8;
            }
        }
    }

    glovesDraw();
}


//
// callback when the mouse is released
//
glov.mouseUp = function (e) {
    // alert("glov.mouseUp(e="+e+")");

    if (glov.modifier == 0) {

        try {
            if        (glov.mode == 1) {
                glov.dragBase = glov.dvar[glov.curDvar].valu;

                glov.mode  = 2;
                glov.dragX = glov.cursorX;
                glov.dragY = glov.cursorY;
            } else if (glov.mode == 2) {
                glov.mode = 0;
            }
        } catch (err) {
            // nothing is selected
        }
    }

    glovesDraw();
}


//
// process a key press in Gloves
//
glov.keyPress = function (e) {
    // alert("glov.keyPress(e="+e+")");

                    glov.modifier  = 0;
    if (e.shiftKey) glov.modifier |= 1;
    if (e.altKey  ) glov.modifier |= 2;
    if (e.ctrlKey ) glov.modifier |= 4;

    // if <esc> was pressed, return to base mode
    if (e.charCode == 0 && e.keyCode == 27) {
        if (glov.mode == 2 && glov.curDvar >= 0) {
            glov.dvar[glov.curDvar].valu = glov.dragBase;
        }

        glov.mode = 0;

    // if <return> was pressed, treat it as a MouseUp
    } else if (e.charCode == 0 && e.keyCode == 13) {
        glov.getMouseUp();

    // if "h" was pressed, go to home view
    } else if (e.charCode == 104 && e.keyCode == 0) {
//        glov.offsetX = 100;
//        glov.offsetY = 200;
//        glov.scale   =  50;
//        glov.uiMatrix.makeIdentity();
//        glov.uiMatrix.scale(glov.scale);
        mVals = new Array (53,-48,  5, 0,
                           33, 42, 48, 0,
                          -36,-33, 53, 0,
                            0,  0,  0, 1);
        glov.uiMatrix.load(mVals);
        glov.offsetX = 100;
        glov.offsetY = 500;
        glov.scale   = 72;

        // if "p" was pressed, print matrix"
    } else if (e.charCode == 112 && e.keyCode == 0) {
        var mVals = glov.uiMatrix.getAsArray();
        
        console.log("mVals: "+mVals[ 0].toPrecision(3)+" "+mVals[ 1].toPrecision(3)+" "+mVals[ 2].toPrecision(3)+" "+mVals[ 3].toPrecision(3));
        console.log("       "+mVals[ 4].toPrecision(3)+" "+mVals[ 5].toPrecision(3)+" "+mVals[ 6].toPrecision(3)+" "+mVals[ 7].toPrecision(3));
        console.log("       "+mVals[ 8].toPrecision(3)+" "+mVals[ 9].toPrecision(3)+" "+mVals[10].toPrecision(3)+" "+mVals[11].toPrecision(3));
        console.log("offsetX: "+glov.offsetX);
        console.log("offsetY: "+glov.offsetY);
        console.log("scale:   "+glov.scale  );

    // if "l" was pressed, rotate left
    } else if ((e.charCode == 108 && e.keyCode ==  0) ||
               (e.charCode ==   0 && e.keyCode == 37)   ) {
        glov.uiMatrix.rotate(+5, 0,1,0);

    // if "r" was pressed, rotate rite
    } else if ((e.charCode == 114 && e.keyCode ==  0) ||
               (e.charCode ==   0 && e.keyCode == 39)   ) {
        glov.uiMatrix.rotate(-5, 0,1,0);

    // if "u" was pressed, rotate up
    } else if ((e.charCode == 117 && e.keyCode ==  0) ||
               (e.charCode ==   0 && e.keyCode == 38)   ) {
        glov.uiMatrix.rotate(+5, 1,0,0);

    // if "d" was pressed, rotate down
    } else if ((e.charCode == 100 && e.keyCode ==  0) ||
               (e.charCode ==   0 && e.keyCode == 40)   ) {
        glov.uiMatrix.rotate(-5, 1,0,0);

    // if "i" or "+" was pressed, zoom in
    } else if ((e.charCode == 105 && e.keyCode ==  0) ||
               (e.charCode ==  43 && e.keyCode ==  0) ||
               (e.charCode ==   0 && e.keyCode == 33)   ) {
        glov.uiMatrix.scale(2.0, 2.0, 2.0);

    // if "o" or "-" was pressed, zoom out
    } else if ((e.charCode == 111 && e.keyCode ==  0) ||
               (e.charCode ==  45 && e.keyCode ==  0) ||
               (e.charCode ==   0 && e.keyCode == 34)   ) {
        glov.uiMatrix.scale(0.5, 0.5, 0.5);

    } else {
        console.log("e.charCode="+e.charCode+"   e.keyCode="+e.keyCode+"   glov.modifier="+glov.modifier);
    }

    glovesDraw();
}


//
// callback when a key is down
//
glov.getKeyDown = function (e) {
    if (e.charCode == 0 && e.keyCode == 16) {    // shift
        glov.modifier = 1;
    }
};


//
// callback when a key is up
//
glov.getKeyUp = function (e) {
    if (e.charCode == 0 && e.keyCode == 16) {    // shift
        glov.modifier = 0;
    }
};

////////////////////////////////////////////////////////////////////////////////


//
// glovesCompCoords - compute screen coordinates of the points
//
var glovesCompCoords = function () {
    var fuse_length    = glov.dvar[ 0].valu;
    var fuse_width     = glov.dvar[ 1].valu;
    var fuse_height    = glov.dvar[ 2].valu;
    var wing_Xroot     = glov.dvar[ 3].valu;
    var wing_Zroot     = glov.dvar[ 4].valu;
    var wing_area      = glov.dvar[ 5].valu;
    var wing_aspect    = glov.dvar[ 6].valu;
    var wing_taper     = glov.dvar[ 7].valu;
    var wing_sweep     = glov.dvar[ 8].valu * Math.PI / 180;
    var wing_dihedral  = glov.dvar[ 9].valu * Math.PI / 180;
    var wing_thick     = glov.dvar[10].valu;
    var htail_Xroot    = glov.dvar[11].valu;
    var htail_Zroot    = glov.dvar[12].valu;
    var htail_area     = glov.dvar[13].valu;
    var htail_aspect   = glov.dvar[14].valu;
    var htail_taper    = glov.dvar[15].valu;
    var htail_sweep    = glov.dvar[16].valu * Math.PI / 180;
    var htail_dihedral = glov.dvar[17].valu * Math.PI / 180;
    var htail_thick    = glov.dvar[18].valu;

    var wing_span   = Math.sqrt(wing_area * wing_aspect);
    var wing_cmean  = wing_area / wing_span;
    var wing_croot  = 2 * wing_cmean / (1 + wing_taper);
    var wing_ctip   = wing_taper * wing_croot;

    var htail_span  = Math.sqrt(htail_area * htail_aspect);
    var htail_cmean = htail_area / htail_span;
    var htail_croot = 2 * htail_cmean / (1 + htail_taper);
    var htail_ctip  = htail_taper * htail_croot;

    var x = [];
    var y = [];
    var z = [];

    x[ 0] = 0;
    y[ 0] = 0;
    z[ 0] = 0;
    glov.labl[ 0] = "";

    x[ 1] = fuse_length;
    y[ 1] = 0;
    z[ 1] = 0;
    glov.labl[ 1] = "fuse:length";

    x[ 2] = 0;
    y[ 2] = -fuse_width/2;
    z[ 2] = 0;
    glov.labl[ 2] = "fuse:width";

    x[ 3] = fuse_length;
    y[ 3] = -fuse_width/2;
    z[ 3] = 0;
    glov.labl[ 3] = "fuse:length;fuse:width";

    x[ 4] = 0;
    y[ 4] = 0;
    z[ 4] = fuse_height;
    glov.labl[ 4] = "fuse:height";

    x[ 5] = fuse_length;
    y[ 5] = 0;
    z[ 5] = fuse_height;
    glov.labl[ 5] = "fuse:length;fuse:height";

    x[ 6] = 0;
    y[ 6] = -fuse_width/2;
    z[ 6] = fuse_height;
    glov.labl[ 6] = "fuse:width;fuse:height";

    x[ 7] = fuse_length;
    y[ 7] = -fuse_width/2;
    z[ 7] = fuse_height;
    glov.labl[ 7] = "fuse:length;fuse:width;fuse:height";

    x[ 8] = wing_Xroot;
    y[ 8] = 0;
    z[ 8] = wing_Zroot;
    glov.labl[ 8] = "wing:Xroot;wing:Zroot";

    x[ 9] = wing_Xroot + wing_croot;
    y[ 9] = 0;
    z[ 9] = wing_Zroot;
    glov.labl[ 9] = "wing:Xroot;wing:Zroot;wing:area;wing:aspect;wing:taper";

    x[10] = wing_Xroot + wing_span/2 * Math.sin(wing_sweep);
    y[10] =       - wing_span/2;
    z[10] = wing_Zroot + wing_span/2 * Math.sin(wing_dihedral);
    glov.labl[10] = "wing:Xroot;wing:Zroot;wing:area;wing:aspect;wing:sweep;wing:dihedral";

    x[11] = wing_Xroot + wing_span/2 * Math.sin(wing_sweep) + wing_ctip;
    y[11] =       - wing_span/2;
    z[11] = wing_Zroot + wing_span/2 * Math.sin(wing_dihedral);
    glov.labl[11] = "wing:Xroot;wing:Zroot;wing:area;wing:aspect;wing:taper;wing:sweep;wing:dihedral";

    x[12] = wing_Xroot;
    y[12] = 0;
    z[12] = wing_Zroot + wing_thick * wing_croot;
    glov.labl[12] = "wing:Xroot;wing:Zroot;wing:area;wing:aspect;wing:taper;wing:thick";

    x[13] = wing_Xroot + wing_croot;
    y[13] = 0;
    z[13] = wing_Zroot + wing_thick * wing_croot;
    glov.labl[13] = "wing:Xroot;wing:Zroot;wing:area;wing:aspect;wing:taper;wing:thick";

    x[14] = wing_Xroot + wing_span/2 * Math.sin(wing_sweep);
    y[14] =       - wing_span/2;
    z[14] = wing_Zroot + wing_span/2 * Math.sin(wing_dihedral) + wing_thick * wing_ctip;
    glov.labl[14] = "wing:Xroot;wing:Zroot;wing:area;wing:aspect;wing:sweep;wing:dihedral;wing:thick";

    x[15] = wing_Xroot + wing_span/2 * Math.sin(wing_sweep) + wing_ctip;
    y[15] =       - wing_span/2;
    z[15] = wing_Zroot + wing_span/2 * Math.sin(wing_dihedral) + wing_thick * wing_ctip;
    glov.labl[15] = "wing:Xroot;wing:Zroot;wing:area;wing:aspect;wing:taper;wing:sweep;wing:dihedral;wing:thick";

    x[16] = htail_Xroot;
    y[16] = 0;
    z[16] = htail_Zroot;
    glov.labl[16] = "htail:Xroot;htail:Zroot";

    x[17] = htail_Xroot + htail_croot;
    y[17] = 0;
    z[17] = htail_Zroot;
    glov.labl[17] = "htail:Xroot;htail:Zroot;htail:area;htail:aspect;htail:taper";

    x[18] = htail_Xroot + htail_span/2 * Math.sin(htail_sweep);
    y[18] =       - htail_span/2;
    z[18] = htail_Zroot + htail_span/2 * Math.sin(htail_dihedral);
    glov.labl[18] = "htail:Xroot;htail:Zroot;htail:area;htail:aspect;htail:sweep;htail:dihedral";

    x[19] = htail_Xroot + htail_span/2 * Math.sin(htail_sweep) + htail_ctip;
    y[19] =       - htail_span/2;
    z[19] = htail_Zroot + htail_span/2 * Math.sin(htail_dihedral);
    glov.labl[19] = "htail:Xroot;htail:Zroot;htail:area;htail:aspect;htail:taper;htail:sweep;htail:dihedral";

    x[20] = htail_Xroot;
    y[20] = 0;
    z[20] = htail_Zroot + htail_thick * htail_croot;
    glov.labl[20] = "htail:Xroot;htail:Zroot;htail:area;htail:aspect;htail:taper;htail:thick";

    x[21] = htail_Xroot + htail_croot;
    y[21] = 0;
    z[21] = htail_Zroot + htail_thick * htail_croot;
    glov.labl[21] = "htail:Xroot;htail:Zroot;htail:area;htail:aspect;htail:taper;htail:thick";

    x[22] = htail_Xroot + htail_span/2 * Math.sin(htail_sweep);
    y[22] =       - htail_span/2;
    z[22] = htail_Zroot + htail_span/2 * Math.sin(htail_dihedral) + htail_thick * htail_ctip;
    glov.labl[22] = "htail:Xroot;htail:Zroot;htail:area;htail:aspect;htail:sweep;htail:dihedral;htail:thick";

    x[23] = htail_Xroot + htail_span/2 * Math.sin(htail_sweep) + htail_ctip;
    y[23] =       - htail_span/2;
    z[23] = htail_Zroot + htail_span/2 * Math.sin(htail_dihedral) + htail_thick * htail_ctip;
    glov.labl[23] = "htail:Xroot;htail:Zroot;htail:area;htail:aspect;htail:taper;htail:sweep;htail:dihedral;htail:thick";

    // convert physical coordinates to screen coordinates
    var mVals = glov.uiMatrix.getAsArray();

    for (var i = 0; i < x.length; i++) {
        glov.xpnt[i] = glov.offsetX + mVals[0] * x[i] + mVals[1] * y[i] + mVals[2] * z[i] + mVals[3];
        glov.ypnt[i] = glov.offsetY - mVals[4] * x[i] - mVals[5] * y[i] - mVals[6] * z[i] - mVals[7];
    }
};


//
// glovesDraw - update the screen
//
var glovesDraw = function () {

    // set up context
    var canvas = document.getElementById("gloves");
    if (canvas.getContext) {
        var context = canvas.getContext("2d");

        // clear the screen
        context.clearRect(0, 0, canvas.width, canvas.height);

        // write the prompt
        context.fillStyle = "black";
        if        (glov.dragging == 1 && glov.modifier == 1) {
            context.fillText("Zoom mode.  Click mouse to exit", 5, 5);
        } else if (glov.dragging == 1 && glov.modifier == 2) {
            context.fillText("Translate mode.  Click mouse to exit", 5, 5);
        } else if (glov.dragging == 1 && glov.modifier == 3) {
            context.fillText("Rotate mode.  Click mouse to exit", 5, 5);
        } else if (glov.dragging == 1 && glov.modifier == 4) {
            context.fillText("Rotate mode.  Click mouse to exit", 5, 5);
        } else if (glov.mode == 0) {
            context.fillText("Hover over a point to edit", 5, 5);
        } else if (glov.mode == 1) {
            context.fillText("Click on a design variable to edit (or <esc> to quit)", 5, 5);
        } else if (glov.mode == 2) {
            context.fillText("Move mouse to change \""+glov.dvar[glov.curDvar].name+"\" (<click> to save or <esc> to quit)", 5, 5);
        }

        // write current design variable values
        for (var idvar = 0; idvar < glov.dvar.length; idvar++) {
            context.fillText(glov.dvar[idvar].name + "=" +glov.dvar[idvar].valu.toPrecision(3), 5, (idvar+2)*15);
        }

        // find the point coordinates on canvas
        glovesCompCoords();

        // draw the outline
        context.lineWidth = 1;
        context.strokeStyle = "black";

        glovesDrawOutline(context);

        // draw the points
        if (glov.mode == 0) {
            context.lineWidth = 5;
            context.fillStyle = "black";
            for (var ipnt = 0; ipnt < glov.xpnt.length; ipnt++) {
                context.fillRect(glov.xpnt[ipnt]-3, glov.ypnt[ipnt]-3, 7, 7);
            }

        // draw selected point and post menu
        } else if (glov.mode == 1) {

            // draw selected point
            context.lineWidth = 5;
            context.fillStyle = "red";
            context.fillRect(glov.xpnt[glov.curPnt]-3, glov.ypnt[glov.curPnt]-3, 7, 7);

            // create menu
            var menuitems = glov.labl[glov.curPnt].split(";");
            var width     = context.measureText(menuitems[0]).width;
            for (var item = 1; item < menuitems.length; item++) {
                var foo = context.measureText(menuitems[item]).width;
                if (foo > width) {
                    width = foo;
                }
            }
            var height = 16;

            glov.menuitems = [];
            for (var imenu = 0; imenu < menuitems.length; imenu++) {
                glov.menuitems[imenu] = {};
                glov.menuitems[imenu].labl = menuitems[imenu];
                glov.menuitems[imenu].xmin = glov.xpnt[glov.curPnt] + 10;
                glov.menuitems[imenu].xmax = glov.xpnt[glov.curPnt] + 10 + width + 4;
                glov.menuitems[imenu].ymin = glov.ypnt[glov.curPnt] + imenu * height - height / 2 - 2;
                glov.menuitems[imenu].ymax = glov.ypnt[glov.curPnt] + imenu * height - height / 2 - 2 + height + 2;
            }

            // post menu
            for (imenu = 0; imenu < menuitems.length; imenu++) {
                if (imenu == glov.curMenu) {
                    context.fillStyle = "blue";
                    context.fillRect(glov.menuitems[imenu].xmin-1,
                                     glov.menuitems[imenu].ymin-2,
                                     glov.menuitems[imenu].xmax-glov.menuitems[imenu].xmin,
                                     glov.menuitems[imenu].ymax-glov.menuitems[imenu].ymin);

                    context.fillStyle = "white";
                    context.fillText(glov.menuitems[imenu].labl, glov.menuitems[imenu].xmin, glov.menuitems[imenu].ymin);
                } else {
                    context.fillStyle = "white";
                    context.fillRect(glov.menuitems[imenu].xmin-1,
                                     glov.menuitems[imenu].ymin-2,
                                     glov.menuitems[imenu].xmax-glov.menuitems[imenu].xmin,
                                     glov.menuitems[imenu].ymax-glov.menuitems[imenu].ymin);

                    context.fillStyle = "black";
                    context.fillText(glov.menuitems[imenu].labl, glov.menuitems[imenu].xmin, glov.menuitems[imenu].ymin);
                }
            }

        // draw selected point with arraow
        } else if (glov.mode == 2) {

            // draw selected point
            context.lineWidth = 5;
            context.fillStyle = "red";
            context.fillRect(glov.xpnt[glov.curPnt]-3, glov.ypnt[glov.curPnt]-3, 7, 7);

        }

    }
};


//
// glovesDrawOutline - connect points with lines
var glovesDrawOutline = function (context) {
    // fuselage centerline
    context.fillStyle = "#FFFF00";
    context.globalAlpha = 0.5;
    context.beginPath();
    context.moveTo(glov.xpnt[ 0], glov.ypnt[ 0]);
    context.lineTo(glov.xpnt[ 1], glov.ypnt[ 1]);
    context.lineTo(glov.xpnt[ 5], glov.ypnt[ 5]);
    context.lineTo(glov.xpnt[ 4], glov.ypnt[ 4]);
    context.closePath();
    context.fill();
    context.globalAlpha = 1.0;

    // fuselage
    context.beginPath();
    context.moveTo(glov.xpnt[ 0], glov.ypnt[ 0]);
    context.lineTo(glov.xpnt[ 1], glov.ypnt[ 1]);
    context.lineTo(glov.xpnt[ 3], glov.ypnt[ 3]);
    context.lineTo(glov.xpnt[ 2], glov.ypnt[ 2]);
    context.lineTo(glov.xpnt[ 6], glov.ypnt[ 6]);
    context.lineTo(glov.xpnt[ 7], glov.ypnt[ 7]);
    context.lineTo(glov.xpnt[ 5], glov.ypnt[ 5]);
    context.lineTo(glov.xpnt[ 4], glov.ypnt[ 4]);
    context.lineTo(glov.xpnt[ 0], glov.ypnt[ 0]);
    context.lineTo(glov.xpnt[ 2], glov.ypnt[ 2]);
    context.lineTo(glov.xpnt[ 6], glov.ypnt[ 6]);
    context.lineTo(glov.xpnt[ 4], glov.ypnt[ 4]);
    context.lineTo(glov.xpnt[ 5], glov.ypnt[ 5]);
    context.lineTo(glov.xpnt[ 1], glov.ypnt[ 1]);
    context.lineTo(glov.xpnt[ 3], glov.ypnt[ 3]);
    context.lineTo(glov.xpnt[ 7], glov.ypnt[ 7]);
    context.stroke();

    // wing
    context.beginPath();
    context.moveTo(glov.xpnt[ 8], glov.ypnt[ 8]);
    context.lineTo(glov.xpnt[ 9], glov.ypnt[ 9]);
    context.lineTo(glov.xpnt[11], glov.ypnt[11]);
    context.lineTo(glov.xpnt[10], glov.ypnt[10]);
    context.lineTo(glov.xpnt[14], glov.ypnt[14]);
    context.lineTo(glov.xpnt[15], glov.ypnt[15]);
    context.lineTo(glov.xpnt[13], glov.ypnt[13]);
    context.lineTo(glov.xpnt[12], glov.ypnt[12]);
    context.lineTo(glov.xpnt[ 8], glov.ypnt[ 8]);
    context.lineTo(glov.xpnt[10], glov.ypnt[10]);
    context.lineTo(glov.xpnt[14], glov.ypnt[14]);
    context.lineTo(glov.xpnt[12], glov.ypnt[12]);
    context.lineTo(glov.xpnt[13], glov.ypnt[13]);
    context.lineTo(glov.xpnt[ 9], glov.ypnt[ 9]);
    context.lineTo(glov.xpnt[11], glov.ypnt[11]);
    context.lineTo(glov.xpnt[15], glov.ypnt[15]);
    context.stroke();

    // htail
    context.beginPath();
    context.moveTo(glov.xpnt[16], glov.ypnt[16]);
    context.lineTo(glov.xpnt[17], glov.ypnt[17]);
    context.lineTo(glov.xpnt[19], glov.ypnt[19]);
    context.lineTo(glov.xpnt[18], glov.ypnt[18]);
    context.lineTo(glov.xpnt[22], glov.ypnt[22]);
    context.lineTo(glov.xpnt[23], glov.ypnt[23]);
    context.lineTo(glov.xpnt[21], glov.ypnt[21]);
    context.lineTo(glov.xpnt[20], glov.ypnt[20]);
    context.lineTo(glov.xpnt[16], glov.ypnt[16]);
    context.lineTo(glov.xpnt[18], glov.ypnt[18]);
    context.lineTo(glov.xpnt[22], glov.ypnt[22]);
    context.lineTo(glov.xpnt[20], glov.ypnt[20]);
    context.lineTo(glov.xpnt[21], glov.ypnt[21]);
    context.lineTo(glov.xpnt[17], glov.ypnt[17]);
    context.lineTo(glov.xpnt[19], glov.ypnt[19]);
    context.lineTo(glov.xpnt[23], glov.ypnt[23]);
    context.stroke();
};
