// ESP-gloves.js implements Gloves functions for the Engineering Sketch Pad (ESP)
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
//    glovesAddFuse(name)
//    glovesAddWing(name)
//    glovesAddVtail(name)
//    glovesAddLabelsAndHeder(icomp)
//    glovesCompCoords(icomp)
//    glovesDraw()
//    glovesDrawOutline(context, icomp)

"use strict";


//
// callback when Gloves is launched
//
glov.launch = function () {
    // alert("in glov.launch()");

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
    button["innerHTML"] = "AddComponent";
    button.style.backgroundColor = "#FFFF3F";

    // initialize GUI variables
    glov.mode      =  0;         // current mode
                                 // =0 initialization
                                 // =1 point selected and menu posted
                                 // =2 design variable selected and drag basis defined

    glov.view      = "Isometric view: ";
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

    glov.curComp   = -1;         // component that is currently highlighted (or -1)
    glov.curPnt    = -1;         // point     that is currently highlighted (or -1)
    glov.curMenu   = -1;         // menu item that is currently highlighed (or -1)
    glov.curDvar   = -1;         // design variable being editted (or -1)

    glov.filename  = "";         // name of .csm file

    // initialize case data
    glov.comp      = [];         // list of components
    glov.dvar      = [];         // list of DESPMTRs

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

    // change the legends of the various orientation buttons
    document.getElementById("homeButton")["innerHTML"] = "I";
    document.getElementById("leftButton")["innerHTML"] = "";
    document.getElementById("riteButton")["innerHTML"] = "S";
    document.getElementById("botmButton")["innerHTML"] = "F";
    document.getElementById("topButton" )["innerHTML"] = "T";

    // change mode here so that getCsmFile works properly
    changeMode(9);

    // if there is an input file, load Gloves from it
    if (wv.filenames != "|") {

        // get the latest csm file
        try {
            changeMode(9);

            var filelist = wv.filenames.split("|");
            browserToServer("getCsmFile|"+filelist[1]+"|");
            wv.fileindx = 1;
        } catch (e) {
            // could not send message
        }

    // otherwise, ask the user for a component
    } else {

        glov.cmdSolve();

        // if there are stll no components loaded, quit Gloves
        if (glov.comp.length == 0) {
            glov.cmdQuit();

        // of there is at least one component, show it/them
        } else {
            glovesDraw();
            glov.cmdHome();
        }
    }
};


//
// Initialize Gloves
//
glov.initialize = function () {
    // alert("in glov.initialize()");

};


//
// glov.cmdload - load Gloves from .csm file
//
glov.cmdLoad = function () {
    // alert("in glov.cmdLoad()");

    // make sure wv.curFile exists
    if (wv.curFile.length == 0) {
        alert("curFile is empty");
        return;
    }

    // look for the Gloves header
    var ibeg = wv.curFile.search("### begin Gloves section ###\n\n");
    if (ibeg < 0) {
        alert("begin not found");
        return;
    }
    ibeg += 30;

    var iend = wv.curFile.search("### end Gloves section ###\n");
    if (iend < 0) {
        alert("end not found");
        return;
    }
    iend --;

    var fileLines = wv.curFile.substring(ibeg, iend).split("\n");
    var icomp     = 0;
    var idvar     = 0;

    for (var iline = 0; iline < fileLines.length; iline++) {
        var tokens = fileLines[iline].split(/ +/);

        if        (tokens[0] == "#") {
            
        } else if (tokens[0] == "CFGPMTR") {
            
        } else if (tokens[0] == "DIMENSION") {
            
        } else if (tokens[0] == "SET" && tokens[1] == "compName") {
            glov.comp[icomp] = {};
            glov.comp[icomp].name = tokens[2].substring(1);

        } else if (tokens[0] == "UDPRIM") {
            glov.comp[icomp].type = tokens[1];
            glov.comp[icomp].head = "";
            glov.comp[icomp].dvar = -1;
            glov.comp[icomp].bbox = [];
            glov.comp[icomp].xpnt = [];
            glov.comp[icomp].ypnt = [];
            glov.comp[icomp].labl = {};

            glovesAddLabelsAndHeader(icomp);

            icomp++;

        } else if (tokens[0] == "DESPMTR") {
            glov.dvar[idvar] = {};
            glov.dvar[idvar].name = tokens[1];
            glov.dvar[idvar].valu = Number(tokens[2]);

        } else if (tokens[0] == "LBOUND") {
            glov.dvar[idvar].lbnd = Number(tokens[2]);

        } else if (tokens[0] == "UBOUND") {
            glov.dvar[idvar].ubnd = Number(tokens[2]);

            idvar++;
        }
    }

    // update the comp.dvar entries
    for (icomp = 0; icomp < glov.comp.length; icomp++) {
        for (idvar = 0; idvar < glov.dvar.length; idvar++) {
            var parts = glov.dvar[idvar].name.split(":");
            if (parts[0] == glov.comp[icomp].name) {
                glov.comp[icomp].dvar = idvar;
                break;
            }
        }
    }
};


//
// callback when "Gloves->Undo" is pressed (called by ESP.html)
//
glov.cmdUndo = function () {
    alert("glov.cmdUndo is not implemented");
};


//
// callback when "solveButton" is pressed
//
glov.cmdSolve = function () {
    // alert("in glov.cmdSolve()");

    var ctype = prompt("Enter component type\n"  +
                       "  1 for wing/htail\n"    +
                       "  2 for fuselage\n"      +
                       "  3 for vtail");
    if (ctype == null) {
        return;
    } else if (ctype == "") {
        return;
    }

    var cname = prompt("Enter component name (aphanumeric only)");
    if (cname == null) {
        return;
    } else if (cname == "") {
        return;
    }

    if        (Number(ctype) == 1) {
        glovesAddWing(cname);
        glovesDraw();
    } else if (Number(ctype) == 2) {
        glovesAddFuse(cname);
        glovesDraw();
    } else if (Number(ctype) == 3) {
        glovesAddVtail(cname);
        glovesDraw();
    } else {
        alert("Component not implemented yet");
    }
};


//
// callback when "Gloves->Save" is pressed (called by ESP.html)
//
glov.cmdSave = function () {
    // alert("in glov.cmdSave()");

    // close the Gloves menu
    var menu = document.getElementsByClassName("doneMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showDoneMenu")) {
            menu[i].classList.remove(  "showDoneMenu");
        }
    }

    // use filename if it exists
    if (wv.filenames != "|") {
        var filelist = wv.filenames.split("|");
        glov.filename = filelist[1];

    // otherwise ask the user for the name of the .csm file to write
    } else {
        var filename = prompt("Enter name of .csm file to write", glov.filename);
        if (filename == null) {
            alert("No filename given");
            return;
        } else if (filename.length < 1) {
            alert("No filename given");
            return;
        }

        if (filename.search(/\.csm$/) < 0) {
            filename += ".csm";
        }

        glov.filename = filename;
    }

    // create an image of the new file
    var fileimage = "";

    // part of curFile before Gloves section
    if (wv.curFile.length > 0) {
        var filelines = wv.curFile.split("\n");

        for (var iline = 0; iline < filelines.length; iline++) {
            if (filelines[iline].substring(0, 28) == "### begin Gloves section ###") {
                break;
            }
            fileimage += filelines[iline] + "\n";
        }
    }

    fileimage += "### begin Gloves section ###\n\n";

    // component headers
    fileimage += "# Component headers\n";
    for (var icomp = 0; icomp < glov.comp.length; icomp++) {
        if (glov.comp[icomp].head.length > 0) {
            fileimage += glov.comp[icomp].head+"\n";
        }
    }
    fileimage += "\n";

    // current components
    fileimage += "# Components\n";
    for (var icomp = 0; icomp < glov.comp.length; icomp++) {
        fileimage += "SET       compName     $"+glov.comp[icomp].name+"\n";
        fileimage += "UDPRIM    "+              glov.comp[icomp].type+"\n\n";
    }

    // current design variables (with bounds)
    fileimage += "# DESPMTRs\n";
    for (var idvar = 0; idvar < glov.dvar.length; idvar++) {
        fileimage += "DESPMTR   "+glov.dvar[idvar].name+"  "+glov.dvar[idvar].valu+"\n";
        fileimage += "LBOUND    "+glov.dvar[idvar].name+"  "+glov.dvar[idvar].lbnd+"\n";
        fileimage += "UBOUND    "+glov.dvar[idvar].name+"  "+glov.dvar[idvar].ubnd+"\n\n";
    }

    fileimage += "### end Gloves section ###\n";

    // part of curFile after Gloves section
    if (wv.curFile.length > 0) {
        var filelines = wv.curFile.split("\n");

        var postscript = 0
        for (var iline = 0; iline < filelines.length; iline++) {
            if (filelines[iline].substring(0, 26) == "### end Gloves section ###") {
                postscript = 1;
                continue;
            }

            if (postscript == 1) {
                fileimage += filelines[iline] + "\n";
            }
        }
    }
    // because of an apparent limit on the size of text
    //    messages that can be sent from the browser to the
    //    server, we need to send the new file back in
    //    pieces and then reassemble on the server
    var maxMessageSize = 800;

    var ichar    = 0;
    var part     = fileimage.substring(ichar, ichar+maxMessageSize);

    browserToServer("setCsmFileBeg|"+glov.filename+"|"+part);
    ichar += maxMessageSize;

    while (ichar < fileimage.length) {
        part = fileimage.substring(ichar, ichar+maxMessageSize);
        browserToServer("setCsmFileMid|"+part);
        ichar += maxMessageSize;
    }

    browserToServer("setCsmFileEnd|");

    // remember the edited .csm file
    wv.curFile = glov.filename;

    postMessage("'"+glov.filename+"' file has been changed.");
    wv.fileindx = -1;

    // get an updated version of the Parameters and Branches
    wv.pmtrStat = 0;
    wv.brchStat = 0;

    // remove the contents of the file from memory
    wv.curFile = "";

    // reset the number of changes
    wv.nchanges = 0;

    // change button labels back to original labels
    document.getElementById("homeButton")["innerHTML"] = "H";
    document.getElementById("leftButton")["innerHTML"] = "L";
    document.getElementById("riteButton")["innerHTML"] = "R";
    document.getElementById("botmButton")["innerHTML"] = "B";
    document.getElementById("topButton" )["innerHTML"] = "T";

    changeMode(0);
    activateBuildButton();
};


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

    // change button labels back to original labels
    document.getElementById("homeButton")["innerHTML"] = "H";
    document.getElementById("leftButton")["innerHTML"] = "L";
    document.getElementById("riteButton")["innerHTML"] = "R";
    document.getElementById("botmButton")["innerHTML"] = "B";
    document.getElementById("topButton" )["innerHTML"] = "T";

    // change solve button legend
    var button = document.getElementById("solveButton");
    button["innerHTML"] = "Up to date";
    button.style.backgroundColor = "#FFFFFF";

    changeMode(0);
};


//
// callback when "homeButton" is pressed
//
glov.cmdHome = function () {
    // alert("in glov.cmsHome()");

    glov.view = "Isometric view: ";

    var bbox = [];
    for (var icomp = 0; icomp < glov.comp.length; icomp++) {
        glovesCompCoords(icomp);

        if (icomp == 0) {
            bbox[0] = glov.comp[0].bbox[0];
            bbox[1] = glov.comp[0].bbox[1];
            bbox[2] = glov.comp[0].bbox[2];
            bbox[3] = glov.comp[0].bbox[3];
            bbox[4] = glov.comp[0].bbox[4];
            bbox[5] = glov.comp[0].bbox[5];
        } else {
            bbox[0] = Math.min(bbox[0], glov.comp[icomp].bbox[0]);
            bbox[1] = Math.max(bbox[1], glov.comp[icomp].bbox[1]);
            bbox[2] = Math.min(bbox[2], glov.comp[icomp].bbox[2]);
            bbox[3] = Math.max(bbox[3], glov.comp[icomp].bbox[3]);
            bbox[4] = Math.min(bbox[4], glov.comp[icomp].bbox[4]);
            bbox[5] = Math.max(bbox[5], glov.comp[icomp].bbox[5]);
        }
    }

    var canvas = document.getElementById("gloves");
    var fact   = 50;
    var cosy   = Math.cos(60 * Math.PI / 180);
    var siny   = Math.sin(60 * Math.PI / 180);
    var cosx   = Math.cos(30 * Math.PI / 180);
    var sinx   = Math.sin(30 * Math.PI / 180);
    var xbar   = (bbox[0] + bbox[1]) / 2;
    var ybar   = (bbox[2] + bbox[3]) / 2;
    var zbar   = (bbox[4] + bbox[5]) / 2;

    var mVals  = [];

    mVals[ 0] =  fact          * cosy;
    mVals[ 1] = -fact                                                     * siny;
    mVals[ 2] =  0;
    mVals[ 3] =  fact * (-xbar * cosy                       - zbar        * siny) + canvas.width  / 2;

    mVals[ 4] =  fact          * sinx * siny;
    mVals[ 5] =  fact                                              * sinx * cosy;
    mVals[ 6] =  fact                                * cosx;
    mVals[ 7] =  fact * (-xbar * sinx * siny  - ybar * cosx + zbar * sinx * cosy) + canvas.height / 2;

    mVals[ 8] = -fact          * cosx * siny;
    mVals[ 9] = -fact                                             * cosx * cosy;
    mVals[10] =  fact                               * sinx;
    mVals[11] =  fact * ( xbar * cosx * siny - ybar * sinx - zbar * cosx * cosy);

    mVals[12] = 0;
    mVals[13] = 0;
    mVals[14] = 0;
    mVals[15] = 1;

    glov.uiMatrix.load(mVals)

    glovesDraw();
};


//
// callback when "leftButton" is pressed
//
glov.cmdLeft = function () {
    // alert("in glov.cmdLeft()");

};


//
// callback when "riteButton" is pressed
//
glov.cmdRite = function () {
    // alert("in glov.cmdRite()");

    glov.view = "Side view: ";

    var bbox = [+Infinity, -Infinity, +Infinity, -Infinity, +Infinity, -Infinity];
    for (var icomp = 0; icomp < glov.comp.length; icomp++) {
        glovesCompCoords(icomp);

        bbox[0] = Math.min(bbox[0], glov.comp[icomp].bbox[0]);
        bbox[1] = Math.max(bbox[1], glov.comp[icomp].bbox[1]);
        bbox[2] = Math.min(bbox[2], glov.comp[icomp].bbox[2]);
        bbox[3] = Math.max(bbox[3], glov.comp[icomp].bbox[3]);
        bbox[4] = Math.min(bbox[4], glov.comp[icomp].bbox[4]);
        bbox[5] = Math.max(bbox[5], glov.comp[icomp].bbox[5]);
    }

    var canvas = document.getElementById("gloves");
    var factx = canvas.width  / (bbox[1] - bbox[0]);
    var facty = canvas.height / (bbox[5] - bbox[4]);

    var fact = 0.80 * Math.min(factx, facty);
    var dx   = (canvas.width  - fact * (bbox[1] + bbox[0])) / 2;
    var dy   = (canvas.height - fact * (bbox[5] + bbox[4])) / 2;

    var mVals = new Array (fact, 0,    0,    dx,
                           0,    0,    fact, dy,
                           0,    fact, 0,    0,
                           0,    0,    0,    1);
    glov.uiMatrix.load(mVals);

    glovesDraw();
};


//
// callback when "botmButton" is pressed
//
glov.cmdBotm = function () {
    // alert("in glov.cmdBotm()");

    glov.view = "Front view: ";

    var bbox = [+Infinity, -Infinity, +Infinity, -Infinity, +Infinity, -Infinity];
    for (var icomp = 0; icomp < glov.comp.length; icomp++) {
        glovesCompCoords(icomp);

        bbox[0] = Math.min(bbox[0], glov.comp[icomp].bbox[0]);
        bbox[1] = Math.max(bbox[1], glov.comp[icomp].bbox[1]);
        bbox[2] = Math.min(bbox[2], glov.comp[icomp].bbox[2]);
        bbox[3] = Math.max(bbox[3], glov.comp[icomp].bbox[3]);
        bbox[4] = Math.min(bbox[4], glov.comp[icomp].bbox[4]);
        bbox[5] = Math.max(bbox[5], glov.comp[icomp].bbox[5]);
    }

    var canvas = document.getElementById("gloves");
    var factx = canvas.width  / (bbox[3] - bbox[2]);
    var facty = canvas.height / (bbox[5] - bbox[4]);

    var fact = 0.80 * Math.min(factx, facty);
    var dx   = (canvas.width  - fact * (bbox[3] + bbox[2])) / 2;
    var dy   = (canvas.height - fact * (bbox[5] + bbox[4])) / 2;

    var mVals = new Array (0,    fact, 0,    dx,
                           0,    0,    fact, dy,
                           fact, 0,    0,    0,
                           0,    0,    0,    1);
    glov.uiMatrix.load(mVals);

    glovesDraw();
};


//
// callback when "topButton" is pressed
//
glov.cmdTop = function () {
    // alert("in glov.cmdTop()");

    glov.view = "Top view: ";

    var bbox = [+Infinity, -Infinity, +Infinity, -Infinity, +Infinity, -Infinity];
    for (var icomp = 0; icomp < glov.comp.length; icomp++) {
        glovesCompCoords(icomp);

        bbox[0] = Math.min(bbox[0], glov.comp[icomp].bbox[0]);
        bbox[1] = Math.max(bbox[1], glov.comp[icomp].bbox[1]);
        bbox[2] = Math.min(bbox[2], glov.comp[icomp].bbox[2]);
        bbox[3] = Math.max(bbox[3], glov.comp[icomp].bbox[3]);
        bbox[4] = Math.min(bbox[4], glov.comp[icomp].bbox[4]);
        bbox[5] = Math.max(bbox[5], glov.comp[icomp].bbox[5]);
    }

    var canvas = document.getElementById("gloves");
    var factx = canvas.width  / (bbox[1] - bbox[0]);
    var facty = canvas.height / (bbox[3] - bbox[2]);

    var fact = 0.80 * Math.min(factx, facty);
    var dx   = (canvas.width  - fact * (bbox[1] + bbox[0])) / 2;
    var dy   = (canvas.height - fact * (bbox[3] + bbox[2])) / 2;

    var mVals = new Array (fact, 0,    0,    dx,
                           0,    fact, 0,    dy,
                           0,    0,    fact, 0,
                           0,    0,    0,    1);
    glov.uiMatrix.load(mVals);

    glovesDraw();
};


//
// callback when "inButton" is pressed
//
glov.cmdIn = function () {
    // alert("in glov.cmdIn()");

    var canvas = document.getElementById("gloves");
    var fact   = 1.25;

    var mVals  = glov.uiMatrix.getAsArray();

    mVals[ 0] *= fact;
    mVals[ 1] *= fact;
    mVals[ 2] *= fact;
    mVals[ 4] *= fact;
    mVals[ 5] *= fact;
    mVals[ 6] *= fact;
    mVals[ 8] *= fact;
    mVals[ 9] *= fact;
    mVals[10] *= fact;
    mVals[11] *= fact;

    mVals[ 3] = mVals[ 3] * fact + canvas.width  * (1 - fact) / 2;
    mVals[ 7] = mVals[ 7] * fact + canvas.height * (1 - fact) / 2;

    glov.uiMatrix.load(mVals);

    glovesDraw();
};


//
// callback when "outButton" is pressed
//
glov.cmdOut = function () {
    // alert("in glov.cmdOut()");

    var canvas = document.getElementById("gloves");
    var fact   = 0.8;

    var mVals  = glov.uiMatrix.getAsArray();

    mVals[ 0] *= fact;
    mVals[ 1] *= fact;
    mVals[ 2] *= fact;
    mVals[ 4] *= fact;
    mVals[ 5] *= fact;
    mVals[ 6] *= fact;
    mVals[ 8] *= fact;
    mVals[ 9] *= fact;
    mVals[10] *= fact;
    mVals[11] *= fact;

    mVals[ 3] = mVals[ 3] * fact + canvas.width  * (1 - fact) / 2;
    mVals[ 7] = mVals[ 7] * fact + canvas.height * (1 - fact) / 2;

    glov.uiMatrix.load(mVals);

    glovesDraw();
};


//
// callback when any mouse is pressed  (when wv.curMode==7)
//
glov.mouseDown = function (e) {
    // alert("in glov.mouseDown(e="+e+")");

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
};


//
// callback when the mouse moves
//
glov.mouseMove = function (e) {
    // alert("in glov.mouseMove(e="+e+")");

    if (!e) var e = event;

    var canvas = document.getElementById("gloves");

    glov.cursorX  = e.clientX - canvas.offsetLeft - 1;
    glov.cursorY  = e.clientY - canvas.offsetTop  - 1;

    // highlight point within halo
    if (glov.mode == 0) {
        glov.curComp = -1;
        glov.curPnt  = -1;

        for (var icomp = 0; icomp < glov.comp.length; icomp++) {
            for (var ipnt = 0; ipnt < glov.comp[icomp].xpnt.length; ipnt++) {
                if (Math.abs(glov.comp[icomp].xpnt[ipnt]-glov.cursorX) < glov.halo &&
                    Math.abs(glov.comp[icomp].ypnt[ipnt]-glov.cursorY) < glov.halo   ) {
                        glov.curComp = icomp;
                        glov.curPnt  = ipnt;
                        glov.mode    = 1;
                        break;
                }
            }
            if (glov.mode == 1) {
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
            if (Math.abs(glov.comp[glov.curComp].xpnt[glov.curPnt]-glov.cursorX) > glov.halo ||
                Math.abs(glov.comp[glov.curComp].ypnt[glov.curPnt]-glov.cursorY) > glov.halo   ) {
                glov.mode = 0;
            }
        }

    // update design variable by tracking mouse
    } else if (glov.mode == 2) {

        // vary design variable until selected point is at cursor
        var idvar = glov.curDvar;
        var dvar0 = glov.dvar[idvar].valu;
        var dist0 = (glov.comp[glov.curComp].xpnt[glov.curPnt] - glov.cursorX) * (glov.comp[glov.curComp].xpnt[glov.curPnt] - glov.cursorX)
                  + (glov.comp[glov.curComp].ypnt[glov.curPnt] - glov.cursorY) * (glov.comp[glov.curComp].ypnt[glov.curPnt] - glov.cursorY);
        var delta = 0.1 * Math.max(dvar0, 1);
        if (dvar0 >= glov.dvar[idvar].ubnd) {
            delta *= -1;
        }

        while (dist0 > 5) {

            // modify the design variable and compute distance to cursor
            var dvar1 = Math.min(Math.max(glov.dvar[idvar].lbnd, dvar0+delta), glov.dvar[idvar].ubnd);
            glov.dvar[idvar].valu = dvar1;

            for (var icomp = 0; icomp < glov.comp.length; icomp++) {
                glovesCompCoords(icomp);
            }

            var dist1 = (glov.comp[glov.curComp].xpnt[glov.curPnt] - glov.cursorX) * (glov.comp[glov.curComp].xpnt[glov.curPnt] - glov.cursorX)
                      + (glov.comp[glov.curComp].ypnt[glov.curPnt] - glov.cursorY) * (glov.comp[glov.curComp].ypnt[glov.curPnt] - glov.cursorY);

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
};


//
// callback when the mouse is released
//
glov.mouseUp = function (e) {
    // alert("in glov.mouseUp(e="+e+")");

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
};


//
// process a key press in Gloves
//
glov.keyPress = function (e) {
    // alert("in glov.keyPress(e="+e+")");

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

    } else {
        console.log("e.charCode="+e.charCode+"   e.keyCode="+e.keyCode+"   glov.modifier="+glov.modifier);
    }

    glovesDraw();
};


//
// callback when a key is down
//
glov.getKeyDown = function (e) {
    // alert("in glov.getKeyDown(e="+e+")");

    if (e.charCode == 0 && e.keyCode == 16) {    // shift
        glov.modifier = 1;
    }
};


//
// callback when a key is up
//
glov.getKeyUp = function (e) {
    // alert("in glov.getKeyUp(e="+e+")");

    if (e.charCode == 0 && e.keyCode == 16) {    // shift
        glov.modifier = 0;
    }
};

////////////////////////////////////////////////////////////////////////////////


//
// glovesAddFuse - add a fuselage component
//
var glovesAddFuse = function (name) {
    // alert("in glovesAddFuse(name="+name+")");

    var idvar = glov.dvar.length;
    var icomp = glov.comp.length;

    var nsect = 4;

    glov.comp[icomp] = {};
    glov.comp[icomp].name = name;
    glov.comp[icomp].type = "$$/glovesFuse";
    glov.comp[icomp].head = "";
    glov.comp[icomp].dvar = idvar;
    glov.comp[icomp].bbox = [];
    glov.comp[icomp].xpnt = [];
    glov.comp[icomp].ypnt = [];
    glov.comp[icomp].labl = {};

    glovesAddLabelsAndHeader(icomp);

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":xloc[1]";
    glov.dvar[idvar].valu =   0.0;
    glov.dvar[idvar].lbnd =   0.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ymax[1]";
    glov.dvar[idvar].valu =   0.5;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ztop[1]";
    glov.dvar[idvar].valu =   0.0;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":zbot[1]";
    glov.dvar[idvar].valu =  -0.5;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":xloc[2]";
    glov.dvar[idvar].valu =   2.0;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ymax[2]";
    glov.dvar[idvar].valu =   1.5;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ztop[2]";
    glov.dvar[idvar].valu =   1.0;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":zbot[2]";
    glov.dvar[idvar].valu =  -1.0;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":xloc[3]";
    glov.dvar[idvar].valu =   8.0;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ymax[3]";
    glov.dvar[idvar].valu =   1.5;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ztop[3]";
    glov.dvar[idvar].valu =   1.0;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":zbot[3]";
    glov.dvar[idvar].valu =  -1.0;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":xloc[4]";
    glov.dvar[idvar].valu =  15.0;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ymax[4]";
    glov.dvar[idvar].valu =   0.5;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":ztop[4]";
    glov.dvar[idvar].valu =   1.0;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":zbot[4]";
    glov.dvar[idvar].valu =   0.5;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    icomp++;
};


//
// glovaesAddWing - add a wing component
//
var glovesAddWing = function (name) {
    // alert("in glovesAddWing(name="+name+")");

    var idvar = glov.dvar.length;
    var icomp = glov.comp.length;

    glov.comp[icomp] = {};
    glov.comp[icomp].name = name;
    glov.comp[icomp].type = "$$/glovesWing";
    glov.comp[icomp].head = "";
    glov.comp[icomp].dvar = idvar;
    glov.comp[icomp].bbox = [];
    glov.comp[icomp].xpnt = [];
    glov.comp[icomp].ypnt = [];
    glov.comp[icomp].labl = {};

    glovesAddLabelsAndHeader(icomp);

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":Xroot";
    glov.dvar[idvar].valu = 3.0;
    glov.dvar[idvar].lbnd = 0.1;
    glov.dvar[idvar].ubnd = 20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":Zroot";
    glov.dvar[idvar].valu =  -0.5;
    glov.dvar[idvar].lbnd = -10.0;
    glov.dvar[idvar].ubnd =  10.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":area";
    glov.dvar[idvar].valu =  48.0;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd = 100.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":aspect";
    glov.dvar[idvar].valu =   5.0;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":taper";
    glov.dvar[idvar].valu =   0.5;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =   2.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":sweep";
    glov.dvar[idvar].valu =  30.0;
    glov.dvar[idvar].lbnd = -45.0;
    glov.dvar[idvar].ubnd =  45.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":dihedral";
    glov.dvar[idvar].valu =   5.0;
    glov.dvar[idvar].lbnd = -10.0;
    glov.dvar[idvar].ubnd =  10.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":thick";
    glov.dvar[idvar].valu =   0.1;
    glov.dvar[idvar].lbnd =   0.01;
    glov.dvar[idvar].ubnd =   1.0;
    idvar++;

    icomp++;
};


//
// glovaesAddVtail - add a wing component
//
var glovesAddVtail = function (name) {
    // alert("in glovesAddVtail(name="+name+")");

    var idvar = glov.dvar.length;
    var icomp = glov.comp.length;

    glov.comp[icomp] = {};
    glov.comp[icomp].name = name;
    glov.comp[icomp].type = "$$/glovesVtail";
    glov.comp[icomp].head = "";
    glov.comp[icomp].dvar = idvar;
    glov.comp[icomp].bbox = [];
    glov.comp[icomp].xpnt = [];
    glov.comp[icomp].ypnt = [];
    glov.comp[icomp].labl = {};

    glovesAddLabelsAndHeader(icomp);

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":Xroot";
    glov.dvar[idvar].valu = 13.3;
    glov.dvar[idvar].lbnd = 0.1;
    glov.dvar[idvar].ubnd = 20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":Yroot";
    glov.dvar[idvar].valu = 0.0;
    glov.dvar[idvar].lbnd = -20.0;
    glov.dvar[idvar].ubnd = 20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":Zroot";
    glov.dvar[idvar].valu =   0.8;
    glov.dvar[idvar].lbnd = -10.0;
    glov.dvar[idvar].ubnd =  10.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":area";
    glov.dvar[idvar].valu =   3.0;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd = 100.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":aspect";
    glov.dvar[idvar].valu =   2.5;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =  20.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":taper";
    glov.dvar[idvar].valu =   0.5;
    glov.dvar[idvar].lbnd =   0.1;
    glov.dvar[idvar].ubnd =   2.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":sweep";
    glov.dvar[idvar].valu =  30.0;
    glov.dvar[idvar].lbnd = -45.0;
    glov.dvar[idvar].ubnd =  45.0;
    idvar++;

    glov.dvar[idvar] = {};
    glov.dvar[idvar].name = name+":thick";
    glov.dvar[idvar].valu =   0.1;
    glov.dvar[idvar].lbnd =   0.01;
    glov.dvar[idvar].ubnd =   1.0;
    idvar++;

    icomp++;
};


//
// glovesAddLabelsAndHeader - add labels and header to the given component
//
var glovesAddLabelsAndHeader = function (icomp) {
    // alert("glovesAddLabelsAndHeader(icomp="+icomp+")");

    var name = glov.comp[icomp].name;

    // fuselage
    if (glov.comp[icomp].type == "$$/glovesFuse") {
        var nsect = 4;        

        glov.comp[icomp].labl[ 0] = name+":xloc[1];"
            +                       name+":ymax[1];"
            +                       name+":zbot[1]";
        glov.comp[icomp].labl[ 1] = name+":xloc[1];"
            +                       name+":ymax[1];"
            +                       name+":zbot[1]";
        glov.comp[icomp].labl[ 2] = name+":xloc[1];"
            +                       name+":ymax[1];"
            +                       name+":ztop[1]";
        glov.comp[icomp].labl[ 3] = name+":xloc[1];"
            +                       name+":ymax[1];"
            +                       name+":ztop[1]";
        glov.comp[icomp].labl[ 4] = name+":xloc[2];"
            +                       name+":ymax[2];"
            +                       name+":zbot[2]";
        glov.comp[icomp].labl[ 5] = name+":xloc[2];"
            +                       name+":ymax[2];"
            +                       name+":zbot[2]";
        glov.comp[icomp].labl[ 6] = name+":xloc[2];"
            +                       name+":ymax[2];"
            +                       name+":ztop[2]";
        glov.comp[icomp].labl[ 7] = name+":xloc[2];"
            +                       name+":ymax[2];"
            +                       name+":ztop[2]";
        glov.comp[icomp].labl[ 8] = name+":xloc[3];"
            +                       name+":ymax[3];"
            +                       name+":zbot[3]";
        glov.comp[icomp].labl[ 9] = name+":xloc[3];"
            +                       name+":ymax[3];"
            +                       name+":zbot[3]";
        glov.comp[icomp].labl[10] = name+":xloc[3];"
            +                       name+":ymax[3];"
            +                       name+":ztop[3]";
        glov.comp[icomp].labl[11] = name+":xloc[3];"
            +                       name+":ymax[3];"
            +                       name+":ztop[3]";
        glov.comp[icomp].labl[12] = name+":xloc[4];"
            +                       name+":ymax[4];"
            +                       name+":zbot[4]";
        glov.comp[icomp].labl[13] = name+":xloc[4];"
            +                       name+":ymax[4];"
            +                       name+":zbot[4]";
        glov.comp[icomp].labl[14] = name+":xloc[4];"
            +                       name+":ymax[4];"
            +                       name+":ztop[4]";
        glov.comp[icomp].labl[15] = name+":xloc[4];"
            +                       name+":ymax[4];"
            +                       name+":ztop[4]";

        glov.comp[icomp].head = "CFGPMTR   "+name+":nsect   "+nsect+"\n"
            +                   "DIMENSION "+name+":xloc    "+nsect+"  1\n"
            +                   "DIMENSION "+name+":ymax    "+nsect+"  1\n"
            +                   "DIMENSION "+name+":zbot    "+nsect+"  1\n"
            +                   "DIMENSION "+name+":ztop    "+nsect+"  1";

    // wing or htail
    } else if (glov.comp[icomp].type == "$$/glovesWing") {
        glov.comp[icomp].labl[ 0] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":sweep;"
            +                       name+":dihedral";
        glov.comp[icomp].labl[ 1] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":sweep;"
            +                       name+":dihedral";
        glov.comp[icomp].labl[ 2] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":sweep;"
            +                       name+":dihedral;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 3] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":sweep;"
            +                       name+":dihedral";
        glov.comp[icomp].labl[ 4] = name+":Xroot;"
            +                       name+":Zroot";
        glov.comp[icomp].labl[ 5] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper";
        glov.comp[icomp].labl[ 6] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 7] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 8] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":sweep;"
            +                       name+":dihedral";
        glov.comp[icomp].labl[ 9] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":sweep;"
            +                       name+":dihedral";
        glov.comp[icomp].labl[10] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":sweep;"
            +                       name+":dihedral;"
            +                       name+":thick";
        glov.comp[icomp].labl[11] = name+":Xroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":sweep;"
            +                       name+":dihedral";

    // vtail
    } else if (glov.comp[icomp].type == "$$/glovesVtail") {
        glov.comp[icomp].labl[ 0] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 1] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 2] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 3] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 4] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":sweep;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 5] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":sweep;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 6] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":sweep;"
            +                       name+":thick";
        glov.comp[icomp].labl[ 7] = name+":Xroot;"
            +                       name+":Yroot;"
            +                       name+":Zroot;"
            +                       name+":area;"
            +                       name+":aspect;"
            +                       name+":taper;"
            +                       name+":sweep;"
            +                       name+":thick";

    }
}

//
// glovesCompCoords - compute screen coordinates of the points
//
var glovesCompCoords = function (icomp) {
    // alert("glovesCompCoords(icomp="+icomp+")");

    var x = [];
    var y = [];
    var z = [];

    // fuselage
    if (glov.comp[icomp].type == "$$/glovesFuse") {
        var ndvar = glov.comp[icomp].dvar;

        var nsect = 4;
        for (var i = 0; i < nsect; i++) {
            var xloc = glov.dvar[ndvar++].valu;
            var ymax = glov.dvar[ndvar++].valu;
            var ztop = glov.dvar[ndvar++].valu;
            var zbot = glov.dvar[ndvar++].valu;

            x[4*i  ] =  xloc;
            y[4*i  ] = -ymax;
            z[4*i  ] =  zbot;

            x[4*i+1] =  xloc;
            y[4*i+1] = +ymax;
            z[4*i+1] =  zbot;

            x[4*i+2] =  xloc;
            y[4*i+2] = -ymax;
            z[4*i+2] =  ztop;

            x[4*i+3] =  xloc;
            y[4*i+3] = +ymax;
            z[4*i+3] =  ztop;
        }

    // wing or htail
    } else if (glov.comp[icomp].type == "$$/glovesWing") {
        var ndvar     = glov.comp[icomp].dvar;
        var Xroot     = glov.dvar[ndvar++].valu;
        var Zroot     = glov.dvar[ndvar++].valu;
        var area      = glov.dvar[ndvar++].valu;
        var aspect    = glov.dvar[ndvar++].valu;
        var taper     = glov.dvar[ndvar++].valu;
        var sweep     = glov.dvar[ndvar++].valu * Math.PI / 180;
        var dihedral  = glov.dvar[ndvar++].valu * Math.PI / 180;
        var thick     = glov.dvar[ndvar++].valu;

        var span   = Math.sqrt(area * aspect);
        var cmean  = area / span;
        var croot  = 2 * cmean / (1 + taper);
        var ctip   = taper * croot;

        x[ 0] = Xroot + span/2 * Math.sin(sweep);
        y[ 0] =         span/2;
        z[ 0] = Zroot + span/2 * Math.sin(dihedral);

        x[ 1] = Xroot + span/2 * Math.sin(sweep) + ctip;
        y[ 1] =         span/2;
        z[ 1] = Zroot + span/2 * Math.sin(dihedral);

        x[ 2] = Xroot + span/2 * Math.sin(sweep);
        y[ 2] =         span/2;
        z[ 2] = Zroot + span/2 * Math.sin(dihedral) + thick * ctip;

        x[ 3] = Xroot + span/2 * Math.sin(sweep) + ctip;
        y[ 3] =         span/2;
        z[ 3] = Zroot + span/2 * Math.sin(dihedral) + thick * ctip;

        x[ 4] = Xroot;
        y[ 4] = 0;
        z[ 4] = Zroot;

        x[ 5] = Xroot + croot;
        y[ 5] = 0;
        z[ 5] = Zroot;

        x[ 6] = Xroot;
        y[ 6] = 0;
        z[ 6] = Zroot + thick * croot;

        x[ 7] = Xroot + croot;
        y[ 7] = 0;
        z[ 7] = Zroot + thick * croot;

        x[ 8] = Xroot + span/2 * Math.sin(sweep);
        y[ 8] =       - span/2;
        z[ 8] = Zroot + span/2 * Math.sin(dihedral);

        x[ 9] = Xroot + span/2 * Math.sin(sweep) + ctip;
        y[ 9] =       - span/2;
        z[ 9] = Zroot + span/2 * Math.sin(dihedral);

        x[10] = Xroot + span/2 * Math.sin(sweep);
        y[10] =       - span/2;
        z[10] = Zroot + span/2 * Math.sin(dihedral) + thick * ctip;

        x[11] = Xroot + span/2 * Math.sin(sweep) + ctip;
        y[11] =       - span/2;
        z[11] = Zroot + span/2 * Math.sin(dihedral) + thick * ctip;

    // vtail
    } else if (glov.comp[icomp].type == "$$/glovesVtail") {
        var ndvar     = glov.comp[icomp].dvar;
        var Xroot     = glov.dvar[ndvar++].valu;
        var Yroot     = glov.dvar[ndvar++].valu;
        var Zroot     = glov.dvar[ndvar++].valu;
        var area      = glov.dvar[ndvar++].valu;
        var aspect    = glov.dvar[ndvar++].valu;
        var taper     = glov.dvar[ndvar++].valu;
        var sweep     = glov.dvar[ndvar++].valu * Math.PI / 180;
        var thick     = glov.dvar[ndvar++].valu;

        var span   = Math.sqrt(area * aspect);
        var cmean  = area / span;
        var croot  = 2 * cmean / (1 + taper);
        var ctip   = taper * croot;

        x[ 0] = Xroot;
        y[ 0] = Yroot - thick * croot;
        z[ 0] = Zroot;

        x[ 1] = Xroot + croot;
        y[ 1] = Yroot - thick * croot;
        z[ 1] = Zroot;

        x[ 2] = Xroot;
        y[ 2] = Yroot + thick * croot;
        z[ 2] = Zroot;

        x[ 3] = Xroot + croot;
        y[ 3] = Yroot + thick * croot;
        z[ 3] = Zroot;

        x[ 4] = Xroot + span * Math.sin(sweep);
        y[ 4] = Yroot - thick * ctip;
        z[ 4] = Zroot + span;

        x[ 5] = Xroot + span * Math.sin(sweep) + ctip;
        y[ 5] = Yroot - thick * ctip;
        z[ 5] = Zroot + span;

        x[ 6] = Xroot + span * Math.sin(sweep);
        y[ 6] = Yroot + thick * ctip;
        z[ 6] = Zroot + span;

        x[ 7] = Xroot + span * Math.sin(sweep) + ctip;
        y[ 7] = Yroot + thick * ctip;
        z[ 7] = Zroot + span;

    } else {
        alert("Unknown component type: "+glov.comp[icomp].type);
    }

    // convert physical coordinates to screen coordinates
    var canvas = document.getElementById("gloves");
    var mVals  = glov.uiMatrix.getAsArray();

    for (var i = 0; i < x.length; i++) {
        glov.comp[icomp].xpnt[i] =                 mVals[0] * x[i] + mVals[1] * y[i] + mVals[2] * z[i] + mVals[3];
        glov.comp[icomp].ypnt[i] = canvas.height - mVals[4] * x[i] - mVals[5] * y[i] - mVals[6] * z[i] - mVals[7];
    }

    // keep track of bounding box
    glov.comp[icomp].bbox[0] = Math.min(...x);
    glov.comp[icomp].bbox[1] = Math.max(...x);
    glov.comp[icomp].bbox[2] = Math.min(...y);
    glov.comp[icomp].bbox[3] = Math.max(...y);
    glov.comp[icomp].bbox[4] = Math.min(...z);
    glov.comp[icomp].bbox[5] = Math.max(...z);
};


//
// glovesDraw - update the screen
//
var glovesDraw = function () {
    // alert("in glovesDraw()");

    // set up context
    var canvas = document.getElementById("gloves");
    if (canvas.getContext) {
        var context = canvas.getContext("2d");

        // clear the screen
        context.clearRect(0, 0, canvas.width, canvas.height);

        // write the prompt
        context.fillStyle = "black";
        if (glov.mode == 0) {
            context.fillText(glov.view+"Hover over a point to edit", 5, 5);
        } else if (glov.mode == 1) {
            context.fillText(glov.view+"Click on a design variable to edit (or <esc> to quit)", 5, 5);
        } else if (glov.mode == 2) {
            context.fillText(glov.view+"Move mouse to change \""+glov.dvar[glov.curDvar].name+"\" (<click> to save or <esc> to quit)", 5, 5);

            // write current design variable values
            var ifirst =  0;
            var ilast  = -1;

            if (glov.curComp >= 0 && glov.curComp < glov.comp.length-1) {
                ifirst = glov.comp[glov.curComp].dvar;
                ilast  = glov.comp[glov.curComp+1].dvar ;
            } else if (glov.curComp == glov.comp.length-1) {
                ifirst = glov.comp[glov.curComp].dvar;
                ilast  = glov.dvar.length;
            }

            for (var idvar = ifirst; idvar < ilast; idvar++) {
                context.fillText(glov.dvar[idvar].name + "=" +glov.dvar[idvar].valu.toPrecision(3), 5, (idvar-ifirst+2)*15);
            }
        }

        // find the point coordinates on canvas
        for (var icomp = 0; icomp < glov.comp.length; icomp++) {
            glovesCompCoords(icomp);
        }

        // draw the outline
        context.lineWidth = 1;
        context.strokeStyle = "black";

        for (icomp = 0; icomp < glov.comp.length; icomp++) {
            glovesDrawOutline(context, icomp);
        }

        // draw the points
        if (glov.mode == 0) {
            context.lineWidth = 5;
            context.fillStyle = "black";
            for (icomp = 0; icomp < glov.comp.length; icomp++) {
                for (var ipnt = 0; ipnt < glov.comp[icomp].xpnt.length; ipnt++) {
                    context.fillRect(glov.comp[icomp].xpnt[ipnt]-3, glov.comp[icomp].ypnt[ipnt]-3, 7, 7);
                }
            }

        // draw selected point and post menu
        } else if (glov.mode == 1) {

            // draw selected point
            context.lineWidth = 5;
            context.fillStyle = "red";
            context.fillRect(glov.comp[glov.curComp].xpnt[glov.curPnt]-3, glov.comp[glov.curComp].ypnt[glov.curPnt]-3, 7, 7);

            // create menu
            var menuitems = glov.comp[glov.curComp].labl[glov.curPnt].split(";");
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
                glov.menuitems[imenu].xmin = glov.comp[glov.curComp].xpnt[glov.curPnt] + 10;
                glov.menuitems[imenu].xmax = glov.comp[glov.curComp].xpnt[glov.curPnt] + 10 + width + 4;
                glov.menuitems[imenu].ymin = glov.comp[glov.curComp].ypnt[glov.curPnt] + imenu * height - height / 2 - 2;
                glov.menuitems[imenu].ymax = glov.comp[glov.curComp].ypnt[glov.curPnt] + imenu * height - height / 2 - 2 + height + 2;
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

        // draw selected point with red square
        } else if (glov.mode == 2) {

            // draw selected point
            context.lineWidth = 5;
            context.fillStyle = "red";
            context.fillRect(glov.comp[glov.curComp].xpnt[glov.curPnt]-3, glov.comp[glov.curComp].ypnt[glov.curPnt]-3, 7, 7);

        }
    }
};


//
// glovesDrawOutline - connect points with lines
//
var glovesDrawOutline = function (context, icomp) {
    // alert("in glovesDrawOutline(icomp="+icomp+")");

    for (var k = 0; k < glov.comp[icomp].xpnt.length; k += 4) {
        context.beginPath();
        context.moveTo(glov.comp[icomp].xpnt[k  ], glov.comp[icomp].ypnt[k  ]);
        context.lineTo(glov.comp[icomp].xpnt[k+1], glov.comp[icomp].ypnt[k+1]);
        context.lineTo(glov.comp[icomp].xpnt[k+3], glov.comp[icomp].ypnt[k+3]);
        context.lineTo(glov.comp[icomp].xpnt[k+2], glov.comp[icomp].ypnt[k+2]);
        context.lineTo(glov.comp[icomp].xpnt[k  ], glov.comp[icomp].ypnt[k  ]);
        context.stroke();
    }

    for (var i = 0; i < 4; i++) {
        context.beginPath();
        context.moveTo(glov.comp[icomp].xpnt[i], glov.comp[icomp].ypnt[i]);
        for (k = 4; k < glov.comp[icomp].xpnt.length; k += 4) {
            context.lineTo(glov.comp[icomp].xpnt[k+i], glov.comp[icomp].ypnt[k+i]);
        }
        context.stroke();
    }
};
