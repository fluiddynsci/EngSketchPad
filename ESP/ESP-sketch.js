// ESP-sketch.js implements Sketcher functions for the Engineering Sketch Pad (ESP)
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

// interface functions that ESP-sketch.js provides
//    sket.launch()
//    sket.initialize()
//    sket.cmdLoad()
//    sket.cmdUndo()
//    sket.cmdSolve()
//    sket.cmdSave()
//    sket.cmdQuit()
//
//    sket.cmdHome()
//    sket.cmdLeft()
//    sket.cmdRite()
//    sket.cmdBotm()
//    sket.cmdTop()
//    sket.cmdIn()
//    sket.cmdOut()
//
//    sket.mouseDown(e)
//    sket.mouseMove(e)
//    sket.mouseUp(e)
//
//    sket.keyPress(e)

// functions associated with the Sketcher
//    sketcherSolvePre()
//    sketcherSolvePost(text)
//    sketcherSave()
//    sketcherUndo()
//    sketcherSaveUndo()
//
//    sketcherUpdateSegData(iseg)
//    sketcherDraw()
//    sketcherDrawBezier(context, Xbezier, Ybezier)
//    sketcherDrawSpline(context, Xspline, Yspline)
//    sketcherGetClosestPoint()
//    sketcherGetClosestSegment()
//    sketcherGetClosestConstraint()

//    sket.ibrch                     // Branch index that launched Sketcher
//    sket.mode                      // 0 initiaizing
//                                   // 1 drawing
//                                   // 2 setting curvature
//                                   // 3 constraining
//                                   // 4 setting width
//                                   // 5 setting depth
//                                   // 6 solved
//    sket.movingPoint = -1;         // moving point while constraining (or -1 for none)
//    sket.basePoint = undefined;    // base Point for W or D constraint
//    sket.skbegX    = undefined;    // expression for X from SKBEG
//    sket.skbegY    = undefined;    // expression for Y from SKBEG
//    sket.skbegZ    = undefined;    // expression for Z from SKBEG
//    sket.relative  = 0;            // =1 if relative   from SKBEG
//    sket.suggest   = undefined;    // suggested changes
//    sket.heads[]                   // last  Point in a chain of Segments
//    sket.tails[]                   // first Point in a chain of Segments
//    sket.halo      =  5;           // pixels for determining "closeness" while drawing
//    sket.offTop    =  0;           // offset to upper-left corner of the canvas
//    sket.offLeft   =  0;

//    sket.scale     = undefined;    // drawing scale factor
//    sket.xorig     = undefined;    // x-origin for drawing
//    sket.yorig     = undefined;    // y-origin for drawing

//    sket.pnt.x[   i]               // x-coordinate
//    sket.pnt.y[   i]               // y-coordinate
//    sket.pnt.lbl[ i]               // constraint label

//    sket.seg.type[i]               // Segment type (L, C, S, or B)
//    sket.seg.ibeg[i]               // Point at beginning
//    sket.seg.iend[i]               // Point at end
//    sket.seg.xm[  i]               // x-coordinate at middle
//    sket.seg.ym[  i]               // y-coordinate at middle
//    sket.seg.lbl[ i]               // constraint label
//
//    sket.seg.dip[ i]               // dip (>0 for convex, <0 for concave)
//    sket.seg.xc[  i]               // x-coordinate at center
//    sket.seg.yc[  i]               // y-coordinate at center
//    sket.seg.rad[ i]               // radius (screen coordinates)
//    sket.seg.tbeg[i]               // theta at beginning (radians)
//    sket.seg.tend[i]               // theta at end       (radians)

//    sket.var.name[ i]              // variable name
//    sket.var.value[i]              // variable value

//    sket.con.type[  i]             // constraint type (X, Y, P, T, A for Point)
//                                   //                 (H, V, I, L, R, or S for Segment)
//    sket.con.index1[i]             // primary Point or Segment index
//    sket.con.index2[i]             // secondary Point or Segment index
//    sket.con.value[ i]             // (optional) constraint value

//    sket.maxundo                   // maximum   undos in Sketcher
//    sket.nundo                     // number of undos in Sketcher
//    sket.undo[i].mode              // undo snapshot (or undefined)
//    sket.undo[i].pnt
//    sket.undo[i].seg
//    sket.undo[i].var
//    sket.undo[i].con

"use strict";


//
// callback when "Enter Sketcher" is pressed in editBrchForm (called by ESP.html)
//
sket.launch = function () {
    // alert("sket.launch()");

    // close the Tools menu
    var menu = document.getElementsByClassName("toolMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showToolMenu")) {
            menu[i].classList.remove(  "showToolMenu");
        }
    }

    sket.ibrch = -1;
    var name   = "";

    // get the Tree node
    try {
        var id    = wv.menuEvent["target"].id;
        var inode = Number(id.substring(4,id.length-4));

        // get the Branch name
        name = myTree.name[inode].replace(/\u00a0/g, "").replace(/>/g, "");

        // get the Branch index
        for (var jbrch = 0; jbrch < brch.length; jbrch++) {
            if (brch[jbrch].name == name) {
                sket.ibrch = jbrch + 1;
            }
        }
        if (sket.ibrch <= 0) {
            alert(name+" not found");
            return;
        }

    /* if there is no Tree node, this implies that the sketecher
       was launched from the menu */
    } catch (err) {
        /* make a list of the Branches that contain a PATVAR */
        var nlist  = 0;
        var list   = [];
        var iskbeg = -1;
        for (var ibrch = 0; ibrch < brch.length; ibrch++) {
            if        (brch[ibrch].type == "skbeg") {
                iskbeg = ibrch;
            } else if (brch[ibrch].type == "skvar") {
                list[nlist] = iskbeg;
                nlist++;
            }
        }

        /* if there are PATBEG statements, find out from the user 
           which one should be editted */
        if (nlist > 0) {
            var mesg = "Enter\n0 to create new Sketch at end\n";

            for (var ilist = 0; ilist < nlist; ilist++) {
                mesg += (ilist+1) + " to edit Sketch starting at "+brch[list[ilist]].name+"\n";
            }

            var ans = prompt(mesg, "0");
            if (ans === null) {
                return;
            } else if (ans == 0) {
                sket.ibrch = brch.length + 1;
                postMessage("Adding new sketch at (0,0,0) to end of Branches");
                browserToServer("newBrch|"+(sket.ibrch-1)+"|skbeg|0|0|0|1||||||");
                browserToServer("getBrchs|");
                browserToServer("loadSketch|"+sket.ibrch+"|");
                wv.nchanges++;
                changeMode(-1);
                return;
            } else if (ans > 0 && ans <= nlist) {
                sket.ibrch = list[ans-1] + 1;
            } else {
                alert("Answer "+ans+" must be between 0 and "+nlist);
                return;
            }

        /* otherwise, put a PATBEG/PATEND after the last Branch */
        } else {
            sket.ibrch = brch.length + 1;
            postMessage("Adding new sketch at (0,0,0) to end of Branches");
            browserToServer("newBrch|"+(sket.ibrch-1)+"|skbeg|0|0|0|1||||||");
            browserToServer("getBrchs|");
            browserToServer("loadSketch|"+sket.ibrch+"|");
            wv.nchanges++;
            changeMode(-1);
            return;
        }
    }
    
    // unhighlight the first column of the Tree
    unhighlightColumn1();

    // send the message to the server
    browserToServer("loadSketch|"+sket.ibrch+"|");

    // inactivate buttons until build is done
    changeMode(-1);

    // initialize the Sketcher
    sket.initialize();
};


//
// Initialize a Sketch
//
sket.initialize = function () {
    // alert("sket.initialize()");

    sket.basePoint = undefined;
    sket.skbegX    = undefined;
    sket.skbegY    = undefined;
    sket.skbegZ    = undefined;
    sket.relative  =  0;
    sket.suggest   = undefined;
    sket.heads     = [];
    sket.tails     = [];
    sket.halo      =  5;      // pixels for determining "closeness" while drawing
    wv.keyPress    = -1;      // last key pressed
    wv.keyCode     = -1;
    wv.button      = -1;      // button pressed
    wv.modifier    =  0;      // modifier (shift,alt,ctrl) bitflag

    // set up Sketcher offsets
    var sketcher = document.getElementById("sketcher");
    sket.offTop    = sketcher.offsetTop;
    sket.offLeft   = sketcher.offsetLeft;

    // initialize the arrays
    sket.pnt        = {};
    sket.pnt.x      = [];
    sket.pnt.y      = [];
    sket.pnt.lbl    = [];

    sket.seg        = {};
    sket.seg.type   = [];
    sket.seg.ibeg   = [];
    sket.seg.iend   = [];
    sket.seg.xm     = [];
    sket.seg.ym     = [];
    sket.seg.lbl    = [];

    sket.seg.dip    = [];
    sket.seg.xc     = [];
    sket.seg.yc     = [];
    sket.seg.rad    = [];
    sket.seg.tbeg   = [];
    sket.seg.tend   = [];

    sket.var        = {};
    sket.var.name   = [];
    sket.var.value  = [];

    sket.con        = {};
    sket.con.type   = [];
    sket.con.index1 = [];
    sket.con.index2 = [];
    sket.con.value  = [];

    // mapping between "physical" and "screen" coordinates
    //   x_physical = sket.scale * (x_screen - sket.xorig)
    //   y_physical = sket.scale * (sket.yorig - y_screen)

    // commands that set sket.xorig: "x"
    // commands that set sket.yorig: "y"
    // commands that set sket.scale: "l", "r", "w", or "d"
    //                               "x" if another "x" exists
    //                               "y" if another "y" exists

    // reset scaling info
    sket.scale = undefined;
    sket.xorig = undefined;
    sket.yorig = undefined;

    // initialize undo
    sket.maxundo = 10;
    sket.nundo   =  0;
    sket.undo    = new Array;
    for (var iundo = 0; iundo < sket.maxundo; iundo++) {
        sket.undo[iundo] = undefined;
    }

    // set mode to initializing
    sket.mode = 0;

    // change done button legend
    var button = document.getElementById("doneMenuBtn");
    button["innerHTML"] = "Sketch";

    // draw the current Sketch
    sketcherDraw();
};


//
// load a Sketch from a semicolon-separated strings sent from serveCSM
//
sket.cmdLoad = function (begs, vars, cons, segs) {
    // alert("sket.cmdLoad()");
    // alert("   begs="+begs);
    // alert("   vars="+vars);
    // alert("   cons="+cons);
    // alert("   segs="+segs);

    // break up the inputs
    var begsList = begs.split(";");
    var varsList = vars.split(";");
    var consList = cons.split(";");
    var segsList = segs.split(";");

    // get the number of points, segments, variables, and constraints
    var npnt = (varsList.length - 1) / 3;
    var ncon = (consList.length - 1) / 4;
    var nseg = (segsList.length - 1) / 3;

    // store the base expressions from the SKBEG statement
    sket.skbegX   = begsList[0];
    sket.skbegY   = begsList[2];
    sket.skbegZ   = begsList[4];
    sket.relative = begsList[6];
    sket.suggest  = undefined;

    // find the extrema of the coordinates
    var xmin = Number(begsList[1]);
    var xmax = Number(begsList[1]);
    var ymin = Number(begsList[3]);
    var ymax = Number(begsList[3]);

    for (var ipnt = 1; ipnt < npnt; ipnt++) {
        xmin = Math.min(xmin, Number(varsList[3*ipnt  ]));
        xmax = Math.max(xmax, Number(varsList[3*ipnt  ]));
        ymin = Math.min(ymin, Number(varsList[3*ipnt+1]));
        ymax = Math.max(ymax, Number(varsList[3*ipnt+1]));
    }

    // get the size of the canvas
    var canvas = document.getElementById("sketcher");
    var width  = canvas.clientWidth;
    var height = canvas.clientHeight;

    if (npnt > 1) {
        var test1 = (xmax - xmin) / width;
        var test2 = (ymax - ymin) / height;

        // set up sizing so that Sketch fills middle 50 percent of the window
        sket.scale = 2 * Math.max(test1, test2);
        sket.xorig = (width  - (xmin + xmax) / sket.scale) / 2;
        sket.yorig = (height + (ymin + ymax) / sket.scale) / 2;

    // not enough data to set scales, so guess something base upon the SKBEG statement
    } else {
        sket.scale = 0.01;
        sket.xorig = width  / 2 - begsList[1] / sket.scale;
        sket.yorig = height / 2 + begsList[3] / sket.scale;
    }

    // create the points
    sket.pnt.x   = [];
    sket.pnt.y   = [];
    sket.pnt.lbl = [];

    for (ipnt = 0; ipnt < npnt; ipnt++) {
//        sket.pnt.x[  ipnt] = Math.floor(sket.xorig + Number(varsList[3*ipnt  ]) / sket.scale);
//        sket.pnt.y[  ipnt] = Math.floor(sket.yorig - Number(varsList[3*ipnt+1]) / sket.scale);
        sket.pnt.x[  ipnt] = sket.xorig + Number(varsList[3*ipnt  ]) / sket.scale;
        sket.pnt.y[  ipnt] = sket.yorig - Number(varsList[3*ipnt+1]) / sket.scale;
        sket.pnt.lbl[ipnt] = "";
    }

    // overwrite the first point by the begsList
    if (npnt > 0) {
//        sket.pnt.x[0] = Math.floor(sket.xorig + Number(begsList[1]) / sket.scale);
//        sket.pnt.y[0] = Math.floor(sket.yorig - Number(begsList[3]) / sket.scale);
        sket.pnt.x[0] = sket.xorig + Number(begsList[1]) / sket.scale;
        sket.pnt.y[0] = sket.yorig - Number(begsList[3]) / sket.scale;
    }

    // create the segments
    sket.seg.type = [];
    sket.seg.ibeg = [];
    sket.seg.iend = [];
    sket.seg.xm   = [];
    sket.seg.ym   = [];
    sket.seg.lbl  = [];

    sket.seg.dip  = [];
    sket.seg.xc   = [];
    sket.seg.yc   = [];
    sket.seg.rad  = [];
    sket.seg.tbeg = [];
    sket.seg.tend = [];

    for (var iseg = 0; iseg < nseg; iseg++) {
        sket.seg.type[iseg] =        segsList[3*iseg  ];
        sket.seg.ibeg[iseg] = Number(segsList[3*iseg+1]) - 1;
        sket.seg.iend[iseg] = Number(segsList[3*iseg+2]) - 1;
        sket.seg.lbl[ iseg] = "";

        sket.seg.dip[ iseg] = 0;
        sket.seg.xc[  iseg] = 0;
        sket.seg.yc[  iseg] = 0;
        sket.seg.rad[ iseg] = 0;
        sket.seg.tbeg[iseg] = 0;
        sket.seg.tend[iseg] = 0;

        // update the segment if an arc
        if (sket.seg.type[iseg] == "C") {
            if (iseg < npnt-1) {
                sket.seg.dip[iseg] = Number(varsList[3*iseg+5]) / sket.scale;
            } else {
                sket.seg.dip[iseg] = Number(varsList[       2]) / sket.scale;
            }
        }

        sketcherUpdateSegData(iseg);
    }

    // create the variables
    sket.var.name  = [];
    sket.var.value = [];

    var nvar = 0;
    for (var ivar = 0; ivar < npnt; ivar++) {
        sket.var.name[ nvar] = sprintf("::x[%]", ivar+1);
        sket.var.value[nvar] = varsList[3*ivar  ];
        nvar++;

        sket.var.name[ nvar] = sprintf("::y[%]", ivar+1);
        sket.var.value[nvar] = varsList[3*ivar+1];
        nvar++;

        if (varsList.length > 0 && Number(varsList[3*ivar+2]) != 0) {
            sket.var.name[ nvar] = sprintf("::d[%]", ivar+1);
            sket.var.value[nvar] = varsList[3*ivar+2];
            nvar++;
        }
    }

    // create the constraints
    sket.con.type   = [];
    sket.con.index1 = [];
    sket.con.index2 = [];
    sket.con.value  = [];

    for (var icon = 0; icon < ncon; icon++) {
        sket.con.type[  icon] =        consList[4*icon  ];
        sket.con.index1[icon] = Number(consList[4*icon+1]) - 1;
        sket.con.index2[icon] = Number(consList[4*icon+2]) - 1;
        sket.con.value[ icon] =        consList[4*icon+3];

        if (sket.con.type[icon] == "W" || sket.con.type[icon] == "D") {
            // not associated with Point or Segment
        } else if (sket.con.index2[icon] < 0) {
            sket.con.index2[icon] += 1;
            sket.pnt.lbl[sket.con.index1[icon]] += sket.con.type[icon];
        } else {
            sket.seg.lbl[sket.con.index1[icon]] += sket.con.type[icon];
        }
    }

    // if there were no constraints, create X and Y constraints at first Point
    if (ncon == 0) {
//        sket.pnt.x[  0] = Math.floor(sket.xorig + Number(begsList[1]) / sket.scale);
//        sket.pnt.y[  0] = Math.floor(sket.yorig - Number(begsList[3]) / sket.scale);
        sket.pnt.x[  0] = sket.xorig + Number(begsList[1]) / sket.scale;
        sket.pnt.y[  0] = sket.yorig - Number(begsList[3]) / sket.scale;
        sket.pnt.lbl[0] = "XY";

        sket.var.name[ 0] = "::x[1]";
        sket.var.value[0] = sket.pnt.x[0];

        sket.var.name[ 1] = "::y[1]";
        sket.var.value[1] = sket.pnt.y[0];

        if (sket.relative == 0) {
            sket.con.type[  0] = "X";
            sket.con.index1[0] = 0;
            sket.con.index2[0] = -1;
            sket.con.value[ 0] = sket.skbegX;

            sket.con.type[  1] = "Y";
            sket.con.index1[1] = 0;
            sket.con.index2[1] = -1;
            sket.con.value[ 1] = sket.skbegY;
        } else {
            sket.con.type[  0] = "X";
            sket.con.index1[0] = 0;
            sket.con.index2[0] = -1;
            sket.con.value[ 0] = 0;

            sket.con.type[  1] = "Y";
            sket.con.index1[1] = 0;
            sket.con.index2[1] = -1;
            sket.con.value[ 1] = 0;
        }
    }

    // set the mode depending on status of Sketch
    if (sket.seg.type.length == 0) {
        sket.mode = 1;
    } else if (sket.seg.iend[sket.seg.iend.length-1] != 0) {
        sket.mode = 1;
    } else {
        sket.mode = 3;
    }

    // draw the current Sketch
    sketcherDraw();
};


//
// callback when "undoButton" is pressed
//
sket.cmdUndo = function () {
    // alert("in cmdUndo()");

    if (wv.server != "serveCSM") {
        alert("cmdUndo is not implemented for "+wv.server);
    } else {
        sketcherUndo();
    }
};


//
// callback when "solveButton" is pressed
//
sket.cmdSolve = function () {
    // alert("in cmdSolve()");

    var button  = document.getElementById("solveButton");
    var buttext = button["innerHTML"];

    if (buttext == "Press to Solve") {
        if (wv.curMode != 7) {
            alert("Command disabled,  Press 'Cancel' or 'OK' first");
            return;
        }

        // save undo info
        sketcherSaveUndo();

        // send solve request to the server
        sketcherSolvePre();

    } else if (buttext == "Initializing...") {
        alert("The Sketcher is initializing.  Please be patient");

    } else if (buttext == "Drawing...") {
        alert("Use \"L\" to add a line segment\n" +
              "    \"C\" to add a circular arc\n" +
              "    \"S\" to add a spline point\n" +
              "    \"B\" to add a Bezier point\n" +
              "    \"Z\" to add a zero-length segment, or\n" +
              "    \"O\" to leave the Sketch open");

    } else if (buttext == "Setting R...") {
        alert("Press left mouse to set radius");

    } else if (buttext == "Constraining...") {

        // send solve request to the server
        sketcherSolvePre();

    } else if (buttext == "nothing") {
        alert("If at a point, use\n" +
              "    \"X\" to set the X-coordinate\n" +
              "    \"Y\" to set the Y-coordinate\n" +
              "    \"P\" to set perpendicularity\n" +
              "    \"T\" to set tangency\n" +
              "    \"A\" to set angle (positive to left)\n" +
              "    \"W\" to set width (dx) to another point\n" +
              "    \"D\" to set depth (dy) to another point\n" +
              "    \"<\" to delete a constraint\n" +
              "If at a cirarc segment, use\n" +
              "    \"R\" to set radius\n" +
              "    \"S\" to set sweep ccw sweep angle\n" +
              "    \"X\" to set X-coordinate of center\n" +
              "    \"Y\" to set Y-coordinate of center\n" +
              "    \"<\" to delete\n" +
              "If at a line segment, use\n" +
              "    \"H\" to set a horizontal constraint\n" +
              "    \"V\" to set a vertical constraint\n", +
              "    \"I\" to set inclination (from right horizontal)\n" +
              "    \"L\" to set length\n"+
              "    \"<\" to delete\n" +
              "If at a W or D constraint, use\n" +
              "    \"<\" to delete");

    } else if (buttext == "Setting W...") {
        alert("Move to another point and press left mouse to set width (dx)");

    } else if (buttext == "Setting D...") {
        alert("move to another point and press left mouse to set depth (dy)");

    } else {
        alert("Unexpected button text:\""+buttext+"\"");
    }
};


//
// callback when "Sketch->Save" is pressed (called by ESP.html)
//
sket.cmdSave = function () {
    // alert("in sket.cmdSave()");

    // close the Sketch menu
    var menu = document.getElementsByClassName("doneMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showDoneMenu")) {
            menu[i].classList.remove(  "showDoneMenu");
        }
    }

    if (wv.server != "serveCSM") {
        alert("cmdSave is not implemented for "+wv.server);
        return;

    } else if (wv.curMode == 7) {
        sketcherSave();
        wv.brchStat = 0;

    } else {
        alert("Command disabled.  Press 'Cancel' or 'OK' first");
        return;

    }
};


//
// callback when "Sketch->Quit" is pressed (called by ESP.html)
//
sket.cmdQuit = function () {
    // alert("in sket.cmdQuit()");

    // close the Sketch menu
    var menu = document.getElementsByClassName("doneMenu-contents");
    for (var i = 0; i < menu.length; i++) {
        var openMenu = menu[i];
        if (menu[i].classList.contains("showDoneMenu")) {
            menu[i].classList.remove(  "showDoneMenu");
        }
    }

    if (wv.curMode != 7) {
        alert("Command disabled.  Press 'Cancel' or 'OK' first");
        return;
    } else if (wv.server != "serveCSM") {
        alert("cmdQuit is not implemented for "+wv.server);
        return;
    }

    changeMode(0);
    activateBuildButton();
};


//
// callback when "homeButton" is pressed
//
sket.cmdHome = function () {
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;

    if (npnt <= 1) {
        alert("Not enough data to rescale");
        return;
    } else if (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At least one length or radius must be set before rescaling  (sket.scale="+sket.scale+")");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At lest one \"X\" must be set before rescaling  (sket.xorig="+sket.xorig+")");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At lest one \"Y\" must be set before rescaling  (sket.yorig="+sket.yorig+")");
        return;
    }

    // get the extrema of the data
    var xmin = +1.0e+10;
    var xmax = -1.0e+10;
    var ymin = +1.0e+10;
    var ymax = -1.0e+10;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        xmin = Math.min(xmin, sket.scale * (sket.pnt.x[ipnt] - sket.xorig));
        xmax = Math.max(xmax, sket.scale * (sket.pnt.x[ipnt] - sket.xorig));
        ymin = Math.min(ymin, sket.scale * (sket.yorig - sket.pnt.y[ipnt]));
        ymax = Math.max(ymax, sket.scale * (sket.yorig - sket.pnt.y[ipnt]));
    }

    // get the size of the canvas
    var canvas = document.getElementById("sketcher");
    var width  = canvas.clientWidth;
    var height = canvas.clientHeight;

    var test1 = (xmax - xmin) / width;
    var test2 = (ymax - ymin) / height;

    // set up sizing so that Sketch fills middle 50 percent of the window
    var newScale = 2 * Math.max(test1, test2);
    var newXorig = (width  - (xmin + xmax) / newScale) / 2;
    var newYorig = (height + (ymin + ymax) / newScale) / 2;

    // convert the Points by the new scale factors
    for (ipnt = 0; ipnt < npnt; ipnt++) {
        var xx = sket.scale * (sket.pnt.x[ipnt] - sket.xorig);
        var yy = sket.scale * (sket.yorig - sket.pnt.y[ipnt]);

//        sket.pnt.x[ipnt] = Math.floor(newXorig + xx / newScale);
//        sket.pnt.y[ipnt] = Math.floor(newYorig - yy / newScale);
        sket.pnt.x[ipnt] = newXorig + xx / newScale;
        sket.pnt.y[ipnt] = newYorig - yy / newScale;
    }

    for (var iseg = 0; iseg < nseg; iseg++) {
        sket.seg.dip[iseg] = sket.seg.dip[iseg] * sket.scale / newScale;

        sketcherUpdateSegData(iseg);
    }

    sket.scale = newScale;
    sket.xorig = newXorig;
    sket.yorig = newYorig;

    sketcherDraw();
};


//
// callback when "leftButton" is pressed
//
sket.cmdLeft = function () {
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;

    if (npnt <= 1) {
        alert("Not enough data to rescale");
        return;
    } else if (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At least one length or radius must be set before rescaling  (sket.scale="+sket.scale+")");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At lest one \"X\" must be set before rescaling  (sket.xorig="+sket.xorig+")");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At lest one \"Y\" must be set before rescaling  (sket.yorig="+sket.yorig+")");
        return;
    }

    var newXorig = sket.xorig - 100;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        var xx = sket.scale * (sket.pnt.x[ipnt] - sket.xorig);
//        sket.pnt.x[ipnt] = Math.floor(newXorig + xx / sket.scale);
        sket.pnt.x[ipnt] = newXorig + xx / sket.scale;
    }

    for (var iseg = 0; iseg < nseg; iseg++) {
        sketcherUpdateSegData(iseg);
    }

    sket.xorig = newXorig;

    sketcherDraw();
};


//
// callback when "riteButton" is pressed
//
sket.cmdRite = function () {
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;

    if (npnt <= 1) {
        alert("Not enough data to rescale");
        return;
    } else if (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At least one length or radius must be set before rescaling  (sket.scale="+sket.scale+")");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At lest one \"X\" must be set before rescaling  (sket.xorig="+sket.xorig+")");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At lest one \"Y\" must be set before rescaling  (sket.yorig="+sket.yorig+")");
        return;
    }

    var newXorig = sket.xorig + 100;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        var xx = sket.scale * (sket.pnt.x[ipnt] - sket.xorig);
//        sket.pnt.x[ipnt] = Math.floor(newXorig + xx / sket.scale);
        sket.pnt.x[ipnt] = newXorig + xx / sket.scale;
    }

    for (var iseg = 0; iseg < nseg; iseg++) {
        sketcherUpdateSegData(iseg);
    }

    sket.xorig = newXorig;

    sketcherDraw();
};


//
// callback when "botmButton" is pressed
//
sket.cmdBotm = function () {
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;

    if (npnt <= 1) {
        alert("Not enough data to rescale");
        return;
    } else if (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At least one length or radius must be set before rescaling  (sket.scale="+sket.scale+")");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At lest one \"X\" must be set before rescaling  (sket.xorig="+sket.xorig+")");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At lest one \"Y\" must be set before rescaling  (sket.yorig="+sket.yorig+")");
        return;
    }

    var newYorig = sket.yorig + 100;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        var yy = sket.scale * (sket.yorig - sket.pnt.y[ipnt]);
//        sket.pnt.y[ipnt] = Math.floor(newYorig - yy / sket.scale);
        sket.pnt.y[ipnt] = newYorig - yy / sket.scale;
    }

    for (var iseg = 0; iseg < nseg; iseg++) {
        sketcherUpdateSegData(iseg);
    }

    sket.yorig = newYorig;

    sketcherDraw();
};


//
// callback when "topButton" is pressed
//
sket.cmdTop = function () {
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;

    if (npnt <= 1) {
        alert("Not enough data to rescale");
        return;
    } else if (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At least one length or radius must be set before rescaling  (sket.scale="+sket.scale+")");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At lest one \"X\" must be set before rescaling  (sket.xorig="+sket.xorig+")");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At lest one \"Y\" must be set before rescaling  (sket.yorig="+sket.yorig+")");
        return;
    }

    var newYorig = sket.yorig - 100;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        var yy = sket.scale * (sket.yorig - sket.pnt.y[ipnt]);
//        sket.pnt.y[ipnt] = Math.floor(newYorig - yy / sket.scale);
        sket.pnt.y[ipnt] = newYorig - yy / sket.scale;
    }

    for (var iseg = 0; iseg < nseg; iseg++) {
        sketcherUpdateSegData(iseg);
    }

    sket.yorig = newYorig;

    sketcherDraw();
};


//
// callback when "inButton" is pressed
//
sket.cmdIn = function () {
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;

    if (npnt <= 1) {
        alert("Not enough data to rescale");
        return;
    } else if (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At least one length or radius must be set before rescaling  (sket.scale="+sket.scale+")");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At lest one \"X\" must be set before rescaling  (sket.xorig="+sket.xorig+")");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At lest one \"Y\" must be set before rescaling  (sket.yorig="+sket.yorig+")");
        return;
    }

    var newScale = 0.5 * sket.scale;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        var xx = sket.scale * (sket.pnt.x[ipnt] - sket.xorig);
        var yy = sket.scale * (sket.yorig - sket.pnt.y[ipnt]);

//        sket.pnt.x[ipnt] = Math.floor(sket.xorig + xx / newScale);
//        sket.pnt.y[ipnt] = Math.floor(sket.yorig - yy / newScale);
        sket.pnt.x[ipnt] = sket.xorig + xx / newScale;
        sket.pnt.y[ipnt] = sket.yorig - yy / newScale;
    }

    for (var iseg = 0; iseg < nseg; iseg++) {
        sket.seg.dip[iseg] = sket.seg.dip[iseg] * sket.scale / newScale;

        sketcherUpdateSegData(iseg);
    }

    sket.scale = newScale;

    sketcherDraw();
};


//
// callback when "outButton" is pressed
//
sket.cmdOut = function () {
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;

    if (npnt <= 1) {
        alert("Not enough data to rescale");
        return;
    } else if (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At least one length or radius must be set before rescaling  (sket.scale="+sket.scale+")");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At lest one \"X\" must be set before rescaling  (sket.xorig="+sket.xorig+")");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At lest one \"Y\" must be set before rescaling  (sket.yorig="+sket.yorig+")");
        return;
    }

    var newScale = 2.0 * sket.scale;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        var xx = sket.scale * (sket.pnt.x[ipnt] - sket.xorig);
        var yy = sket.scale * (sket.yorig - sket.pnt.y[ipnt]);

//        sket.pnt.x[ipnt] = Math.floor(sket.xorig + xx / newScale);
//         sket.pnt.y[ipnt] = Math.floor(sket.yorig - yy / newScale);
        sket.pnt.x[ipnt] = sket.xorig + xx / newScale;
        sket.pnt.y[ipnt] = sket.yorig - yy / newScale;
    }

    for (var iseg = 0; iseg < nseg; iseg++) {
        sket.seg.dip[iseg] = sket.seg.dip[iseg] * sket.scale / newScale;

        sketcherUpdateSegData(iseg);
    }

    sket.scale = newScale;

    sketcherDraw();
};


//
// callback when any mouse is pressed  (when wv.curMode==7)
//
sket.mouseDown = function (e) {
    if (!e) var e = event;

    wv.startX   = e.clientX - sket.offLeft - 1;
    wv.startY   = e.clientY - sket.offTop  - 1;

    wv.button   = e.button;

                    wv.modifier  = 0;
    if (e.shiftKey) wv.modifier |= 1;
    if (e.altKey  ) wv.modifier |= 2;
    if (e.ctrlKey ) wv.modifier |= 4;

    // set default key presses that are bound to mouse clicks
    if        (sket.mode == 1) {
        var evnt = new Object();
        evnt.keyCode  = 0;
        evnt.charCode = 108;   // "l"

        sket.keyPress(evnt);
    } else if (sket.mode == 2) {
        var evnt = new Object();
        evnt.keyCode  = 0;
        evnt.charCode = 108;   // "l"

        sket.keyPress(evnt);
    } else if (sket.mode == 3) {
        wv.cursorX = wv.startX;
        wv.cursorY = wv.startY;

        sket.movingPoint = sketcherGetClosestPoint();
    } else if (sket.mode == 4) {
        var evnt = new Object();
        evnt.keyCode  = 0;
        evnt.charCode = 124;   // "w"

        sket.keyPress(evnt);
    } else if (sket.mode == 5) {
        var evnt = new Object();
        evnt.keyCode  = 0;
        evnt.charCode = 100;   // "d"

        sket.keyPress(evnt);
    }
};


//
// callback when the mouse moves
//
sket.mouseMove = function (e) {
    if (!e) var e = event;

    wv.cursorX  = e.clientX - sket.offLeft - 1;
    wv.cursorY  = e.clientY - sket.offTop  - 1;

                    wv.modifier  = 0;
    if (e.shiftKey) wv.modifier |= 1;
    if (e.altKey  ) wv.modifier |= 2;
    if (e.ctrlKey ) wv.modifier |= 4;

    // if sket.mode==1, then the "proposed" new Segment
    //    is taken care of in sketcherDraw
    if (sket.mode == 1) {
        sketcherDraw();

    // if sket.mode==3, then move the point at (wv.startX, wv.startY)
    //    to the current location
    } else if (sket.mode == 3) {
        if (sket.movingPoint >= 0) {
            sket.pnt.x[sket.movingPoint] = wv.cursorX;
            sket.pnt.y[sket.movingPoint] = wv.cursorY;

            for (var iseg = 0; iseg < sket.seg.type.length; iseg++) {
                sketcherUpdateSegData(iseg);
            }

            sketcherDraw();
        }

    // if sket.mode==4 or 5, then the "proposed" new W or D constraint
    //    is taken care of in sketcherDraw
    } else if (sket.mode == 4 || sket.mode == 5) {
        sketcherDraw();

    // if sket.mode==2, then we need to use the cursor location to update
    //    the dip for the last Segment
    } else if (sket.mode == 2) {
        var iseg = sket.seg.type.length - 1;

        var xa = sket.pnt.x[sket.seg.ibeg[iseg]];
        var ya = sket.pnt.y[sket.seg.ibeg[iseg]];
        var xb = sket.pnt.x[sket.seg.iend[iseg]];
        var yb = sket.pnt.y[sket.seg.iend[iseg]];

        // put the cursor on the arc
        var xe  = wv.cursorX;
        var ye  = wv.cursorY;

        var D = (ya - ye) * (xb - xe) - (yb - ye) * (xa - xe);
        if (Math.abs(D) > 1e-6) {
            var s = ((xb - xa) * (xb - xe) - (ya - yb) * (yb - ye)) / D;

            var xc = (xa + xe + s * (ya - ye)) / 2;
            var yc = (ya + ye + s * (xe - xa)) / 2;

            var R = Math.sqrt((xc-xa) * (xc-xa) + (yc-ya) * (yc-ya));
            var L = Math.sqrt((xb-xa) * (xb-xa) + (yb-ya) * (yb-ya));

            if ((xe-xa)*(yb-ya) > (ye-ya)*(xb-xa)) {
                if ((xb-xa)*(yc-ya) > (yb-ya)*(xc-xa)) {
                    sket.seg.dip[iseg] = -R + Math.sqrt(R*R - L*L/4);
                } else {
                    sket.seg.dip[iseg] = -R - Math.sqrt(R*R - L*L/4);
                }
            } else {
                if ((xb-xa)*(yc-ya) > (yb-ya)*(xc-xa)) {
                    sket.seg.dip[iseg] = +R + Math.sqrt(R*R - L*L/4);
                } else {
                    sket.seg.dip[iseg] = +R - Math.sqrt(R*R - L*L/4);
                }
            }

            sketcherUpdateSegData(iseg);
        } else {
            sket.seg.dip[ iseg] = 0;
            sket.seg.xm[  iseg] = (xa + xb) / 2;
            sket.seg.ym[  iseg] = (ya + yb) / 2;
            sket.seg.xc[  iseg] = 0;
            sket.seg.yc[  iseg] = 0;
            sket.seg.rad[ iseg] = 0;
            sket.seg.tbeg[iseg] = 0;
            sket.seg.tend[iseg] = 0;
        }

        sketcherDraw();
    }
};


//
// callback when the mouse is released
//
sket.mouseUp = function (e) {
    sket.movingPoint = -1;
};


//
// process a key press in the Sketcher
//
sket.keyPress = function (e) {
    // alert("sket.keyPress");

    // record info about keypress
    wv.keyPress = e.charCode;
    wv.keyCode  = e.keyCode;

                    wv.modifier  = 0;
    if (e.shiftKey) wv.modifier |= 1;
    if (e.altKey  ) wv.modifier |= 2;
    if (e.ctrlKey ) wv.modifier |= 4;

    var canvas  = document.getElementById("sketcher");
    var context = canvas.getContext("2d");

    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;
    var nvar = sket.var.name.length;
    var ncon = sket.con.type.length;

    var myKeyPress = String.fromCharCode(wv.keyPress);

    // 'ctrl-h' --- home
    if ((wv.keyPress == 104 && wv.modifier == 4 && wv.keyCode == 0) ||
        (wv.keyPress ==   8 && wv.modifier == 4 && wv.keyCode == 8)   ) {
        sket.cmdHome();
        return;

    // '<Home>' -- home
    } else if (wv.keyPress == 0 && wv.keyCode == 36) {
        sket.cmdHome();
        return;

    // 'ctrl-i' - zoom in (same as <PgUp> without shift)
    } else if ((wv.keyPress == 105 && wv.modifier == 4 && wv.keyCode ==  0) ||
               (wv.keyPress ==   9 && wv.modifier == 4 && wv.keyCode ==  9)   ) {
        sket.cmdIn();
        return;

    // '<PgDn>' -- zoom in
    } else if (wv.keyPress == 0 && wv.keyCode == 33) {
        sket.cmdIn();
        return;

    // '+' -- zoom in
    } else if (wv.keyPress == 43 && wv.modifier == 1) {
        sket.cmdIn();
        return;

    // 'ctrl-o' - zoom out (same as <PgDn> without shift)
    } else if ((wv.keyPress == 111 && wv.modifier == 4 && wv.keyCode ==  0) ||
               (wv.keyPress ==  15 && wv.modifier == 4 && wv.keyCode == 15)   ) {
        sket.cmdOut();
        return;

    // '<PgUp>' -- zoom out
    } else if (wv.keyPress == 0 && wv.keyCode == 34) {
        sket.cmdOut();
        return;

    // '-' -- zoomout
    } else if (wv.keyPress == 45 && wv.modifier == 0) {
        sket.cmdOut();
        return;

    // 'ctrl-r' - translate right
    } else if ((wv.keyPress == 114 && wv.modifier == 4 && wv.keyCode ==  0) ||
               (wv.keyPress ==  18 && wv.modifier == 4 && wv.keyCode == 18)   ) {
        sket.cmdRite();
        return;

    // '<Right>' -- translate right
    } else if (wv.keyPress == 0 && wv.keyCode == 39) {
        sket.cmdRite();
        return;

    // 'ctrl-l' - translate left
    } else if ((wv.keyPress == 108 && wv.modifier == 4 && wv.keyCode ==  0) ||
               (wv.keyPress ==  12 && wv.modifier == 4 && wv.keyCode == 12)   ) {
        sket.cmdLeft();
        return;

    // '<Left>' -- translate left
    } else if (wv.keyPress == 0 && wv.keyCode == 37) {
        sket.cmdLeft();
        return;

    // 'ctrl-t' - translate up
    } else if ((wv.keyPress == 116 && wv.modifier == 4 && wv.keyCode ==  0) ||
               (wv.keyPress ==  20 && wv.modifier == 4 && wv.keyCode == 20)   ) {
        sket.cmdTop();
        return;

    // '<Up>' -- translate up
    } else if (wv.keyPress == 0 && wv.keyCode == 38) {
        sket.cmdTop();
        return;

    // 'ctrl-b' - translate down
    } else if ((wv.keyPress ==  98 && wv.modifier == 4 && wv.keyCode ==  0) ||
               (wv.keyPress ==   2 && wv.modifier == 4 && wv.keyCode ==  2)   ) {
        sket.cmdBotm();
        return;

    // '<Down>' -- translate down
    } else if (wv.keyPress == 0 && wv.keyCode == 40) {
        sket.cmdBotm();
        return;

    // '@' - coordinates of cursor
    } else if (myKeyPress == "@" || myKeyPress == '2') {
        var xx = sket.scale * (wv.cursorX - sket.xorig);
        var yy = sket.scale * (sket.yorig - wv.cursorY);
        postMessage("Cursor is at ("+xx+","+yy+")");

    // '*' - rescale based upon best info
    } else if (myKeyPress == '*' || myKeyPress == '8') {
        var xold = sket.scale * (wv.cursorX - sket.xorig);
        var yold = sket.scale * (sket.yorig - wv.cursorY);

        var xnew = prompt("Enter approximate X location at cursor: ", xold);
        if (xnew !== null) {
            var ynew = prompt("Enter approximate Y location at cursor: ", yold);
            if (ynew !== null) {
                postMessage("Resetting scale based upon cursor location");

                var Lphys = Math.sqrt(xnew*xnew + ynew*ynew);
                var Lscr  = Math.sqrt(Math.pow(wv.cursorX-sket.xorig,2)
                                     +Math.pow(wv.cursorY-sket.yorig,2));
                sket.scale = Lphys / Lscr;
            }
        }

    // sket.mode 1 options ('l', 'c', 's', 'b', 'z', or 'o' to leave sketch open)
    } else if (sket.mode == 1) {
        sketcherSaveUndo();

        // determine the coordinates (and constraints) for this placement
        var xnew = wv.cursorX;
        var ynew = wv.cursorY;
        var cnew = "";

        for (var ipnt = npnt-1; ipnt >= 0; ipnt--) {
            if (Math.abs(wv.cursorX-sket.pnt.x[ipnt]) < sket.halo) {
                xnew = sket.pnt.x[ipnt];
                if (ipnt == npnt-1) {
                    cnew = "V";
                }
                break;
            }
        }
        for (ipnt = npnt-1; ipnt >= 0; ipnt--) {
            if (Math.abs(wv.cursorY-sket.pnt.y[ipnt]) < sket.halo) {
                ynew = sket.pnt.y[ipnt];
                if (ipnt == npnt-1) {
                    cnew = "H";
                }
                break;
            }
        }

        // "l" --- add a line Segment
        if (myKeyPress == "l" || myKeyPress == "L") {
            var ibeg = npnt - 1;
            var iend = 0;

            // if this point is within halo of previous point, reject it
            if (Math.abs(sket.pnt.x[npnt-1]-xnew) < sket.halo &&
                Math.abs(sket.pnt.y[npnt-1]-ynew) < sket.halo   ) {
                postMessage("cannot create zero-length line segment\n use \"Z\" command instead");
                return;
            }

            // if this Point does not close the Sketch, add a new Point and its variables
            if (Math.abs(sket.pnt.x[0]-xnew) > sket.halo ||
                Math.abs(sket.pnt.y[0]-ynew) > sket.halo   ) {
                iend = npnt;
                sket.pnt.x[  iend] = xnew;
                sket.pnt.y[  iend] = ynew;
                sket.pnt.lbl[iend] = "";

                sket.var.name[ nvar  ] = sprintf("::x[%]", npnt+1);
                sket.var.value[nvar  ] = sprintf("%",    xnew  );

                sket.var.name[ nvar+1] = sprintf("::y[%]", npnt+1);
                sket.var.value[nvar+1] = sprintf("%",    ynew  );
            }

            // add the new Segment
            sket.seg.type[nseg] = "L";
            sket.seg.ibeg[nseg] = ibeg;
            sket.seg.iend[nseg] = iend;
            sket.seg.xm[  nseg] = (sket.pnt.x[ibeg] + sket.pnt.x[iend]) / 2;
            sket.seg.ym[  nseg] = (sket.pnt.y[ibeg] + sket.pnt.y[iend]) / 2;
            sket.seg.lbl[ nseg] = "";

            sket.seg.dip[ nseg] = 0;
            sket.seg.xc[  nseg] = 0;
            sket.seg.yc[  nseg] = 0;
            sket.seg.rad[ nseg] = 0;
            sket.seg.tbeg[nseg] = 0;
            sket.seg.tend[nseg] = 0;

            // add a new "horizontal" or "vertical" constraint
            if        (cnew == "H") {
                sket.con.type[  ncon] = "H";
                sket.con.index1[ncon] = ibeg;
                sket.con.index2[ncon] = iend;
                sket.con.value[ ncon] = 0;

                sket.seg.lbl[nseg] += "H";
            } else if (cnew == "V") {
                sket.con.type[  ncon] = "V";
                sket.con.index1[ncon] = ibeg;
                sket.con.index2[ncon] = iend;
                sket.con.value[ ncon] = 0;

                sket.seg.lbl[nseg] += "V";
            }

            // if Sketch is closed, post a message and swith mode
            if (iend == 0) {
                sket.mode = 3;
            }

        // "c" --- add a circular arc
        } else if (myKeyPress == "c" || myKeyPress == "C") {
            var ibeg = npnt - 1;
            var iend = 0;

            // if this point is within halo of previous point, reject it
            if (Math.abs(sket.pnt.x[npnt-1]-xnew) < sket.halo &&
                Math.abs(sket.pnt.y[npnt-1]-ynew) < sket.halo   ) {
                postMessage("cannot create zero-length circular arc");
                return;
            }

            // if this Point does not close the Sketch, add a new Point and its variables
            if (Math.abs(sket.pnt.x[0]-xnew) > sket.halo ||
                Math.abs(sket.pnt.y[0]-ynew) > sket.halo) {
                iend          = npnt;
                sket.pnt.x[  iend] = xnew;
                sket.pnt.y[  iend] = ynew;
                sket.pnt.lbl[iend] = "";

                sket.var.name[ nvar  ] = sprintf("::x[%]", npnt+1);
                sket.var.value[nvar  ] = sprintf("%",    xnew  );

                sket.var.name[ nvar+1] = sprintf("::y[%]", npnt+1);
                sket.var.value[nvar+1] = sprintf("%",    ynew  );
            }

            // add the new Segment
            sket.seg.type[nseg] = "C";
            sket.seg.ibeg[nseg] = ibeg;
            sket.seg.iend[nseg] = iend;
            sket.seg.xm[  nseg] = (sket.pnt.x[ibeg] + sket.pnt.x[iend]) / 2;
            sket.seg.ym[  nseg] = (sket.pnt.y[ibeg] + sket.pnt.y[iend]) / 2;
            sket.seg.lbl[ nseg] = "";

            sket.seg.dip[ nseg] = 0;
            sket.seg.xc[  nseg] = 0;
            sket.seg.yc[  nseg] = 0;
            sket.seg.rad[ nseg] = 0;
            sket.seg.tbeg[nseg] = 0;
            sket.seg.tend[nseg] = 0;

            nvar = sket.var.name.length;

            sket.var.name[ nvar] = sprintf("::d[%]", sket.seg.type.length);
            sket.var.value[nvar] = "0";

            sket.mode = 2;

        // "s" --- add spline Point
        } else if (myKeyPress == "s" || myKeyPress == "S") {
            var ibeg = npnt - 1;
            var iend = 0;

            // if this point is within halo of previous point, reject it
            if (Math.abs(sket.pnt.x[npnt-1]-xnew) < sket.halo &&
                Math.abs(sket.pnt.y[npnt-1]-ynew) < sket.halo   ) {
                postMessage("cannot create zero-length spline segment");
                return;
            }

            // if this Point does not close the Sketch, add a new Point and its variables
            if (Math.abs(sket.pnt.x[0]-xnew) > sket.halo ||
                Math.abs(sket.pnt.y[0]-ynew) > sket.halo   ) {
                iend = npnt;
                sket.pnt.x[  iend] = xnew;
                sket.pnt.y[  iend] = ynew;
                sket.pnt.lbl[iend] = "";

                sket.var.name[ nvar  ] = sprintf("::x[%]", npnt+1);
                sket.var.value[nvar  ] = sprintf("%",    xnew  );

                sket.var.name[ nvar+1] = sprintf("::y[%]", npnt+1);
                sket.var.value[nvar+1] = sprintf("%",    ynew  );
            }

            // add the new Segment
            sket.seg.type[nseg] = "S";
            sket.seg.ibeg[nseg] = ibeg;
            sket.seg.iend[nseg] = iend;
            sket.seg.xm[  nseg] = (sket.pnt.x[ibeg] + sket.pnt.x[iend]) / 2;
            sket.seg.ym[  nseg] = (sket.pnt.y[ibeg] + sket.pnt.y[iend]) / 2;
            sket.seg.lbl[ nseg] = "";

            sket.seg.dip[ nseg] = 0;
            sket.seg.xc[  nseg] = 0;
            sket.seg.yc[  nseg] = 0;
            sket.seg.rad[ nseg] = 0;
            sket.seg.tbeg[nseg] = 0;
            sket.seg.tend[nseg] = 0;

            // if Sketch is closed, post a message and swith mode
            if (iend == 0) {
                sket.mode = 3;
            }

        // "b" --- add Bezier Point
        } else if (myKeyPress == "b" || myKeyPress == "B") {
            var ibeg = npnt - 1;
            var iend = 0;

            // if this point is within halo of last point, reject it
            if (Math.abs(sket.pnt.x[npnt-1]-xnew) < sket.halo &&
                Math.abs(sket.pnt.y[npnt-1]-ynew) < sket.halo   ) {
                postMessage("cannot create zero-length bezier segment");
                return;
            }

            // if this Point does not close the Sketch, add a new Point and its variables
            if (Math.abs(sket.pnt.x[0]-xnew) > sket.halo ||
                Math.abs(sket.pnt.y[0]-ynew) > sket.halo   ) {
                iend = npnt;
                sket.pnt.x[  iend] = xnew;
                sket.pnt.y[  iend] = ynew;
                sket.pnt.lbl[iend] = "";

                sket.var.name[ nvar  ] = sprintf("::x[%]", npnt+1);
                sket.var.value[nvar  ] = sprintf("%",    xnew  );

                sket.var.name[ nvar+1] = sprintf("::y[%]", npnt+1);
                sket.var.value[nvar+1] = sprintf("%",    ynew  );
            }

            // add the new Segment
            sket.seg.type[nseg] = "B";
            sket.seg.ibeg[nseg] = ibeg;
            sket.seg.iend[nseg] = iend;
            sket.seg.xm[  nseg] = (sket.pnt.x[ibeg] + sket.pnt.x[iend]) / 2;
            sket.seg.ym[  nseg] = (sket.pnt.y[ibeg] + sket.pnt.y[iend]) / 2;
            sket.seg.lbl[ nseg] = "";

            sket.seg.dip[ nseg] = 0;
            sket.seg.xc[  nseg] = 0;
            sket.seg.yc[  nseg] = 0;
            sket.seg.rad[ nseg] = 0;
            sket.seg.tbeg[nseg] = 0;
            sket.seg.tend[nseg] = 0;

            // add a new "horizontal" or "vertical" constraint
            if        (cnew == "H") {
                sket.con.type[  ncon] = "H";
                sket.con.index1[ncon] = ibeg;
                sket.con.index2[ncon] = iend;
                sket.con.value[ ncon] = 0;

                sket.seg.lbl[nseg] += "H";
            } else if (cnew == "V") {
                sket.con.type[  ncon] = "V";
                sket.con.index1[ncon] = ibeg;
                sket.con.index2[ncon] = iend;
                sket.con.value[ ncon] = 0;

                sket.seg.lbl[nseg] += "V";
            }

            // if Sketch is closed, post a message and swith mode
            if (iend == 0) {
                sket.mode = 3;
            }

        // "z" --- add a zero-length Segment (to break a spline or Bezier)
        } else if (myKeyPress == 'z') {
            var ibeg = npnt - 1;
            var iend = 0;

            // make sure this follows a Bezier or spline
            if (sket.seg.type[nseg-1] != "B" && sket.seg.type[nseg-1] != "S") {
                postMessage("zero-length segment can only come after spline or bezier");
                return;
            }

            // add a new Point and its variables
            sket.pnt.lbl[ibeg] = "Z";

            iend = npnt;
            sket.pnt.x[  iend] = sket.pnt.x[ibeg];
            sket.pnt.y[  iend] = sket.pnt.y[ibeg];
            sket.pnt.lbl[iend] = "Z";

            sket.var.name[ nvar  ] = sprintf("::x[%]", npnt);
            sket.var.value[nvar  ] = sprintf("%",    xnew  );

            sket.var.name[ nvar+1] = sprintf("::y[%]", npnt);
            sket.var.value[nvar+1] = sprintf("%",    ynew  );

            // add a new (linear) Segment
            sket.seg.type[nseg] = "L";
            sket.seg.ibeg[nseg] = ibeg;
            sket.seg.iend[nseg] = iend;
            sket.seg.xm[  nseg] = sket.pnt.x[ibeg];
            sket.seg.ym[  nseg] = sket.pnt.y[ibeg];
            sket.seg.lbl[ nseg] = "Z";

            sket.seg.dip[ nseg] = 0;
            sket.seg.xc[  nseg] = 0;
            sket.seg.yc[  nseg] = 0;
            sket.seg.rad[ nseg] = 0;
            sket.seg.tbeg[nseg] = 0;
            sket.seg.tend[nseg] = 0;

            // add the constraints
            sket.con.type[  ncon] = "Z";
            sket.con.index1[ncon] = ibeg;
            sket.con.index2[ncon] = -2;
            sket.con.value[ ncon] = 0;

            sket.con.type[  ncon+1] = "Z";
            sket.con.index1[ncon+1] = iend;
            sket.con.index2[ncon+1] = -3;
            sket.con.value[ ncon+1] = 0;

        // "o" --- end (open) Sketch
        } else if (myKeyPress == "o") {
            sket.mode = 3;

        } else {
            postMessage("Valid options are 'l', 'c', 's', 'b', 'z', and 'o'");
        }

    // sket.mode 2 option (any keyPress or mouseDown)
    } else if (sket.mode == 2) {

        // <any> --- set curvature
        if (sket.seg.iend[sket.seg.iend.length-1] == 0) {
            sket.mode = 3;
        } else {
            sket.mode = 1;
        }

    // sket.mode 3 options
    // if at a Point:          'x', 'y', 'p', 't', 'a', 'w', 'd' or '<' to delete
    // if at a cirarc Segment: 'r', 's', 'x', 'y',               or '<' to delete
    // if at          Segment: 'h', 'v', 'i', 'l',               or '<' to delete
    } else if (sket.mode == 3 || sket.mode == 6) {
        sketcherSaveUndo();

        // "x" --- set X coordinate of nearest Point or arc Segment
        if (myKeyPress == "x" || myKeyPress == "X") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestPoint();
            var jbest = sketcherGetClosestSegment();
            if (jbest >= 0 && sket.seg.type[jbest] != "C") {
                jbest = -1;
            }

            var dibest = 999999;
            if (ibest >= 0) {
                dibest = Math.abs(wv.cursorX - sket.pnt.x[ibest])
                       + Math.abs(wv.cursorY - sket.pnt.y[ibest]);
            }
            var djbest = 999999;
            if (jbest >= 0) {
                djbest = Math.abs(wv.cursorX - sket.seg.xm[jbest])
                       + Math.abs(wv.cursorY - sket.seg.ym[jbest]);
            }

            var xvalue;
            if (ibest < 0) {
                alert("No Point found");
            } else if (dibest < djbest) {
                if (sket.pnt.lbl[ibest].indexOf("X") >= 0) {
                    for (var icon = 0; icon < ncon; icon++) {
                        if (sket.con.type[icon] == "X" && sket.con.index1[icon] == ibest) {
                            if (ibest == 0 && sket.relative == 1) {
                                alert("X cannot be changed at first point if in relative mode");
                            } else {
                                xvalue = prompt("Enter x value: ", sket.con.value[icon]);
                                if (xvalue !== null) {
                                    sket.con.value[icon] = xvalue;
                                }
                            }
                            break;
                        }
                    }
                } else {
                    if (sket.scale === undefined && sket.xorig === undefined) {
                        xvalue = prompt("Enter x value: ");
                        if (xvalue !== null) {
                            for (var icon = 0; icon < ncon; icon++) {
                                if (sket.con.type[icon] == "X" && sket.con.index1[icon] != ibest) {
                                    sket.scale = (xvalue - sket.con.value[icon]) / (sket.pnt.x[ibest] - sket.pnt.x[sket.con.index1[icon]]);
                                    sket.xorig = sket.pnt.x[sket.con.index1[icon]] - sket.con.value[icon] / sket.scale;
                                    break;
                                }
                            }
                        }
                    } else if (sket.xorig === undefined) {
                        xvalue = prompt("Enter x value: ");
                        if (xvalue !== null) {
                            sket.xorig = sket.pnt.x[ibest] + xvalue / sket.scale;
                        }
                    } else if (sket.scale === undefined) {
                        xvalue = prompt("Enter x value: ");
                        if (xvalue !== null) {
                            for (var icon = 0; icon < ncon; icon++) {
                                if (sket.con.type[icon] == "X" && sket.con.index1[icon] != ibest) {
                                    sket.scale = (xvalue - sket.con.value[icon]) / (sket.pnt.x[ibest] - sket.xorig);
                                    break;
                                }
                            }
                        }
                    } else {
                        xvalue = prompt("Enter x value: ", sket.scale*(sket.pnt.x[ibest]-sket.xorig));
                    }

                    if (xvalue !== null) {
                        sket.con.type[  ncon] = "X";
                        sket.con.index1[ncon] = ibest;
                        sket.con.index2[ncon] = -1;
                        sket.con.value[ ncon] = xvalue;

                        sket.pnt.lbl[ibest] += "X";
                    }
                }
            } else {
                if (sket.seg.lbl[jbest].indexOf("X") >= 0) {
                    for (var icon = 0; icon < ncon; icon++) {
                        if (sket.con.type[  icon] == "X" &&
                            sket.con.index1[icon] == sket.seg.ibeg[jbest] &&
                            sket.con.index2[icon] == sket.seg.iend[jbest]   ) {
                            xvalue = prompt("Enter x value: ", sket.con.value[icon]);
                            if (xvalue !== null) {
                                sket.con.value[icon] = xvalue;
                            }
                            break;
                        }
                    }
                } else {
                    xvalue = prompt("Enter x value: ");
                    if (xvalue !== null) {
                        sket.con.type[  ncon] = "X";
                        sket.con.index1[ncon] = sket.seg.ibeg[jbest];
                        sket.con.index2[ncon] = sket.seg.iend[jbest];
                        sket.con.value[ ncon] = xvalue;

                        sket.seg.lbl[jbest] += "X";
                    }
                }
            }

        // "y" --- set Y coordinate of nearest Point or arc Segment
        } else if (myKeyPress == "y" || myKeyPress == "Y") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestPoint();
            var jbest = sketcherGetClosestSegment();
            if (jbest >= 0 && sket.seg.type[jbest] != "C") {
                jbest = -1;
            }

            var dibest = 999999;
            if (ibest >= 0) {
                dibest = Math.abs(wv.cursorX - sket.pnt.x[ibest])
                       + Math.abs(wv.cursorY - sket.pnt.y[ibest]);
            }
            var djbest = 999999;
            if (jbest >= 0) {
                djbest = Math.abs(wv.cursorX - sket.seg.xm[jbest])
                       + Math.abs(wv.cursorY - sket.seg.ym[jbest]);
            }

            var yvalue;
            if (ibest < 0) {
                alert("No Point found");
            } else if (dibest < djbest) {
                if (sket.pnt.lbl[ibest].indexOf("Y") >= 0) {
                    for (var icon = 0; icon < ncon; icon++) {
                        if (sket.con.type[icon] == "Y" && sket.con.index1[icon] == ibest) {
                            if (ibest == 0 && sket.relative == 1) {
                                alert("Y cannot be changed at first point if in relative mode");
                            } else {
                                yvalue = prompt("Enter y value: ", sket.con.value[icon]);
                                if (yvalue !== null) {
                                    sket.con.value[icon] = yvalue;
                                }
                            }
                            break;
                        }
                    }
                } else {
                    if (sket.scale === undefined && sket.yorig === undefined) {
                        yvalue = prompt("Enter y value: ");
                        if (yvalue !== null) {
                            for (var icon = 0; icon < ncon; icon++) {
                                if (sket.con.type[icon] == "Y" && sket.con.index1[icon] != ibest) {
                                    sket.scale = (sket.con.value[icon] - yvalue) / (sket.pnt.y[ibest] - sket.pnt.y[sket.con.index1[icon]]);
                                    sket.yorig = sket.pnt.y[sket.con.index1[icon]] + sket.con.value[icon] / sket.scale;
                                    break;
                                }
                            }
                        }
                    } else if (sket.yorig === undefined) {
                        yvalue = prompt("Enter y value: ");
                        if (yvalue !== null) {
                            sket.yorig = sket.pnt.y[ibest] - yvalue / sket.scale;
                        }
                    } else if (sket.scale === undefined) {
                        yvalue = prompt("Enter y value: ");
                        if (yvalue !== null) {
                            for (var icon = 0; icon < ncon; icon++) {
                                if (sket.con.type[icon] == "Y" && sket.con.index1[icon] != ibest) {
                                    sket.scale = (sket.con.value[icon] - yvalue) / (sket.pnt.y[ibest] - sket.yorig);
                                    break;
                                }
                            }
                        }
                    } else {
                        yvalue = prompt("Enter y value: ", sket.scale*(sket.yorig-sket.pnt.y[ibest]));
                    }

                    if (yvalue !== null) {
                        sket.con.type[  ncon] = "Y";
                        sket.con.index1[ncon] = ibest;
                        sket.con.index2[ncon] = -1;
                        sket.con.value[ ncon] = yvalue;

                        sket.pnt.lbl[ibest] += "Y";
                    }
                }
            } else {
                if (sket.seg.lbl[jbest].indexOf("Y") >= 0) {
                    for (var icon = 0; icon < ncon; icon++) {
                        if (sket.con.type[  icon] == "Y" &&
                            sket.con.index1[icon] == sket.seg.ibeg[jbest] &&
                            sket.con.index2[icon] == sket.seg.iend[jbest]   ) {
                            yvalue = prompt("Enter y value: ", sket.con.value[icon]);
                            if (xvalue !== null) {
                                sket.con.value[icon] = yvalue;
                            }
                            break;
                        }
                    }
                } else {
                    yvalue = prompt("Enter y value: ");
                    if (yvalue !== null) {
                        sket.con.type[  ncon] = "Y";
                        sket.con.index1[ncon] = sket.seg.ibeg[jbest];
                        sket.con.index2[ncon] = sket.seg.iend[jbest];
                        sket.con.value[ ncon] = yvalue;

                        sket.seg.lbl[jbest] += "Y";
                    }
                }
            }

        // "p" --- perpendicularity
        } else if (myKeyPress == "p" || myKeyPress == "P") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestPoint();
            if (ibest < 0) {
                alert("No Point found");
            } else if (sket.pnt.lbl[ibest].indexOf("P") >= 0) {
                alert("'P' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("T") >= 0) {
                alert("'T' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("A") >= 0) {
                alert("'A' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("Z") >= 0) {
                alert("'Z' constraint already at this Point");
            } else {
                sket.con.type[  ncon] = "P";
                sket.con.index1[ncon] = ibest;
                sket.con.index2[ncon] = -1;
                sket.con.value[ ncon] = 0;

                sket.pnt.lbl[ibest] += "P";
            }

        // "t" --- tangency
        } else if (myKeyPress == "t" || myKeyPress == "T") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestPoint();
            if (ibest < 0) {
                alert("No Point found");
            } else if (sket.pnt.lbl[ibest].indexOf("P") >= 0) {
                alert("'P' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("T") >= 0) {
                alert("'T' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("A") >= 0) {
                alert("'A' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("Z") >= 0) {
                alert("'Z' constraint already at this Point");
            } else {
                sket.con.type[  ncon] = "T";
                sket.con.index1[ncon] = ibest;
                sket.con.index2[ncon] = -1;
                sket.con.value[ ncon] = 0;

                sket.pnt.lbl[ibest] += "T";
            }

        // "a" --- angle (positive to left)
        } else if (myKeyPress == 'a' || myKeyPress == "A") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestPoint();
            var avalue;
            if (ibest < 0) {
                alert("No Point found");
            } else if (sket.pnt.lbl[ibest].indexOf("P") >= 0) {
                alert("'P' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("T") >= 0) {
                alert("'T' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("Z") >= 0) {
                alert("'Z' constraint already at this Point");
            } else if (sket.pnt.lbl[ibest].indexOf("A") >= 0) {
                for (var icon = 0; icon < ncon; icon++) {
                    if (sket.con.type[icon] == "A" && sket.con.index1[icon] == ibest) {
                        avalue = prompt("Enter angle (deg): ", sket.con.value[icon]);
                        if (avalue !== null) {
                            sket.con.value[icon] = avalue;
                        }
                        break;
                    }
                }
            } else {
                var ibeg = -1;
                var iend = -1;
                for (var iseg = 0; iseg < sket.seg.type.length; iseg++) {
                    if (sket.seg.iend[iseg] == ibest) {
                        ibeg = sket.seg.ibeg[iseg];
                    }
                    if (sket.seg.ibeg[iseg] == ibest) {
                        iend = sket.seg.iend[iseg];
                    }
                }

                if (ibeg < 0 || iend < 0) {
                    alert("Angle cannot be specified at endpoints");
                    return;
                }

                var ang1 = Math.atan2(sket.pnt.y[ibest]-sket.pnt.y[ibeg ],
                                      sket.pnt.x[ibest]-sket.pnt.x[ibeg ]) * 180 / Math.PI;
                var ang2 = Math.atan2(sket.pnt.y[iend ]-sket.pnt.y[ibest],
                                      sket.pnt.x[iend ]-sket.pnt.x[ibest]) * 180 / Math.PI;
                avalue = ang1 - ang2;
                while (avalue < -180) {
                    avalue += 360;
                }
                while (avalue > 180) {
                    avalue -= 360;
                }

                avalue = prompt("Enter angle (deg): ", avalue);

                if (avalue !== null) {
                    sket.con.type[  ncon] = "A";
                    sket.con.index1[ncon] = ibest;
                    sket.con.index2[ncon] = -1;
                    sket.con.value[ ncon] = avalue;

                    sket.pnt.lbl[ibest] += "A";
                }
            }

        // "h" --- Segment is horizontal
        } else if (myKeyPress == "h" || myKeyPress == "H") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestSegment();
            if (ibest < 0) {
                alert("No Segment found");
            } else if (sket.seg.lbl[ibest].indexOf("H") >= 0) {
                alert("'H' constraint already on this Segment");
            } else if (sket.seg.lbl[ibest].indexOf("V") >= 0) {
                alert("'V' constraint already on this Segment");
            } else if (sket.seg.lbl[ibest].indexOf("I") >= 0) {
                alert("'I' constraint already on this Segment");
            } else if (Math.abs(sket.seg.dip[ibest]) > 1e-3) {
                alert("'H' constraint cannot be set on a Cirarc");
            } else {
                var ibeg  = sket.seg.ibeg[ibest];
                var iend  = sket.seg.iend[ibest];

                sket.con.type[  ncon] = "H";
                sket.con.index1[ncon] = ibeg;
                sket.con.index2[ncon] = iend;
                sket.con.value[ ncon] = 0;

                sket.seg.lbl[ibest] += "H";
            }

        // "v" --- Segment is vertical
        } else if (myKeyPress == "v" || myKeyPress == "V") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestSegment();
            if (ibest < 0) {
                alert("No Segment found");
            } else if (sket.seg.lbl[ibest].indexOf("V") >= 0) {
                alert("'V' constraint already on this Segment");
            } else if (sket.seg.lbl[ibest].indexOf("H") >= 0) {
                alert("'H' constraint already on this Segment");
            } else if (sket.seg.lbl[ibest].indexOf("I") >= 0) {
                alert("'I' constraint already on this Segment");
            } else if (Math.abs(sket.seg.dip[ibest]) > 1e-3) {
                alert("'V' constraint cannot be set on a Cirarc");
            } else {
                var ibeg  = sket.seg.ibeg[ibest];
                var iend  = sket.seg.iend[ibest];

                sket.con.type[  ncon] = "V";
                sket.con.index1[ncon] = ibeg;
                sket.con.index2[ncon] = iend;
                sket.con.value[ ncon] = 0;

                sket.seg.lbl[ibest] += "V";
            }

        // "i" --- inclination of Segment
        } else if (myKeyPress == "i" || myKeyPress == "I") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestSegment();
            var ivalue;
            if (ibest < 0) {
                alert("No Segment found");
            } else if (sket.seg.lbl[ibest].indexOf("H") >= 0) {
                alert("'H' constraint already on this Segment");
            } else if (sket.seg.lbl[ibest].indexOf("V") >= 0) {
                alert("'V' constraint already on this Segment");
            } else if (Math.abs(sket.seg.dip[ibest]) > 1e-3) {
                alert("'I' constraint cannot be set on a Cirarc");
            } else if (sket.seg.lbl[ibest].indexOf("I") >= 0) {
                for (var icon = 0; icon < ncon; icon++) {
                    if (sket.con.type[ icon] == "I" && sket.con.index1[icon] == ibest) {
                        ivalue = prompt("Enter inclination angle (deg): ", sket.con.value[icon]);
                        if (ivalue !== null) {
                            sket.con.value[icon] = ivalue;
                        }
                        break;
                    }
                }
            } else {
                var ibeg  = sket.seg.ibeg[ibest];
                var iend  = sket.seg.iend[ibest];

                var iprompt = Math.atan2(sket.pnt.y[ibeg]-sket.pnt.y[iend], sket.pnt.x[iend]-sket.pnt.x[ibeg]) * 180/Math.PI;

                ivalue = prompt("Enter inclination angle (deg): ", iprompt);
                if (ivalue !== null) {
                    sket.con.type[  ncon] = "I";
                    sket.con.index1[ncon] = ibeg;
                    sket.con.index2[ncon] = iend;
                    sket.con.value[ ncon] = ivalue;

                    sket.seg.lbl[ibest] += "I";
                }
            }

        // "l" --- length of Segment
        } else if (myKeyPress == "l" || myKeyPress == "L") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestSegment();
            var lvalue;
            if (ibest < 0) {
                alert("No Segment found");
            } else if (Math.abs(sket.seg.dip[ibest]) > 1e-3) {
                alert("'L' constraint cannot be set on a Cirarc");
            } else if (sket.seg.lbl[ibest].indexOf("L") >= 0) {
                for (var icon = 0; icon < ncon; icon++) {
                    if (sket.con.type[ icon] == "L" && sket.con.index1[icon] == ibest) {
                        lvalue = prompt("Enter length: ", sket.con.value[icon]);
                        if (lvalue !== null) {
                            sket.con.value[icon] = lvalue;
                        }
                        break;
                    }
                }
            } else {
                var ibeg  = sket.seg.ibeg[ibest];
                var iend  = sket.seg.iend[ibest];

                var xbeg = sket.pnt.x[ibeg];
                var ybeg = sket.pnt.y[ibeg];
                var xend = sket.pnt.x[iend];
                var yend = sket.pnt.y[iend];
                var len  = Math.sqrt(Math.pow(xend-xbeg, 2) + Math.pow(yend-ybeg, 2));

                if (sket.scale === undefined) {
                    lvalue = prompt("Enter length: ");
                    if (lvalue !== null) {
                        sket.scale = lvalue / len;
                    }
                } else {
                    lvalue = prompt("Enter length: ", sket.scale*len);
                }

                if (lvalue !== null) {
                    sket.con.type[  ncon] = "L";
                    sket.con.index1[ncon] = ibeg;
                    sket.con.index2[ncon] = iend;
                    sket.con.value[ ncon] = lvalue;

                    sket.seg.lbl[ibest] += "L";
                }
            }

        // "r" --- radius of Segment
        } else if (myKeyPress == "r" || myKeyPress == "R") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestSegment();
            var rvalue;
            if (ibest < 0) {
                alert("No Cirarc found");
            } else if (Math.abs(sket.seg.dip[ibest]) < 1e-3) {
                alert("'R' constraint cannot be set for a Linseg");
            } else if (sket.seg.lbl[ibest].indexOf("R") >= 0) {
                for (var icon = 0; icon < ncon; icon++) {
                    if (sket.con.type[ icon] == "R" && sket.con.index1[icon] == ibest) {
                        rvalue = prompt("Enter radius: ", sket.con.value[icon]);
                        if (rvalue !== null) {
                            sket.con.value[icon] = rvalue;
                        }
                        break;
                    }
                }
            } else {
                var ibeg = sket.seg.ibeg[ibest];
                var iend = sket.seg.iend[ibest];

                var xa = sket.pnt.x[ibeg];
                var ya = sket.pnt.y[ibeg];
                var xb = sket.pnt.x[iend];
                var yb = sket.pnt.y[iend];

                var d  = sket.seg.dip[ibest];

                var L   = Math.sqrt((xb-xa) * (xb-xa) + (yb-ya) * (yb-ya));
                var rad = (L*L + 4*d*d) / (8*d);

                if (sket.scale === undefined) {
                    if (Number(sket.seg.dip[ibest]) >= 0) {
                        rvalue = prompt("Enter radius (should be positive as drawn): ");
                    } else {
                        rvalue = prompt("Enter radius (should be negative as drawn): ");
                    }
                    if (rvalue !== null) {
                        sket.scale = rvalue / rad;
                    }
                } else {
                    rvalue = prompt("Enter radius: ", sket.scale*rad);
                }

                if (rvalue !== null) {
                    sket.con.type[  ncon] = "R";
                    sket.con.index1[ncon] = ibeg;
                    sket.con.index2[ncon] = iend;
                    sket.con.value[ ncon] = rvalue;

                    sket.seg.lbl[ibest] += "R";
                }
            }

        // "s" --- sweep angle of Segment
        } else if (myKeyPress == "s" || myKeyPress == "S") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestSegment();
            var avalue;
            if (ibest < 0) {
                alert("No Cirarc found");
            } else if (Math.abs(sket.seg.dip[ibest]) < 1e-3) {
                alert("'S' constraint cannot be set for a Linseg");
            } else if (sket.seg.lbl[ibest].indexOf("S") >= 0) {
                for (var icon = 0; icon < ncon; icon++) {
                    if (sket.con.type[icon] == "S" && sket.con.index1[icon] == ibest) {
                        avalue = prompt("Enter sweep angle (deg): ", sket.con.value[icon]);
                        if (avalue !== null) {
                            sket.con.value[icon] = avalue;
                        }
                        break;
                    }
                }
            } else {
                var ibeg = sket.seg.ibeg[ibest];
                var iend = sket.seg.iend[ibest];

                if (Number(sket.seg.dip[ibest]) >= 0) {
                    avalue = prompt("Enter sweep angle (deg, should be positive as drawn): ");
                } else {
                    avalue = prompt("Enter sweep angle (deg, should be negative as drawn): ");
                }
                if (avalue !== null) {
                    sket.con.type[  ncon] = "S";
                    sket.con.index1[ncon] = ibeg;
                    sket.con.index2[ncon] = iend;
                    sket.con.value[ ncon] = avalue;

                    sket.seg.lbl[ibest] += "S";
                }
            }

        // "w" --- width (dx) between Points
        } else if (myKeyPress == "w" || myKeyPress == "W") {
            var bestPnt = sketcherGetClosestPoint();
            var bestCon = sketcherGetClosestConstraint();

            if (bestCon !== undefined) {
                var xmid = (  sket.pnt.x[sket.con.index1[bestCon]]
                            + sket.pnt.x[sket.con.index2[bestCon]]) / 2;
                var ymid = (2*sket.pnt.y[sket.con.index1[bestCon]]
                            + sket.pnt.y[sket.con.index2[bestCon]]) / 3;

                var dpnt = Math.abs(sket.pnt.x[bestPnt] - wv.cursorX)
                         + Math.abs(sket.pnt.y[bestPnt] - wv.cursorY);
                var dcon = Math.abs(xmid                - wv.cursorX)
                         + Math.abs(ymid                - wv.cursorY);

                if (dcon < dpnt) {
                    var wvalue = prompt("Enter width: ", sket.con.value[bestCon]);
                    if (wvalue !== null) {
                        sket.con.value[bestCon] = wvalue;
                    }

                    sket.bestPoint = undefined;
                    sket.mode      = 3;
                    sket.suggest   = undefined;
                } else {
                    sket.basePoint = bestPnt;
                    sket.mode      = 4;
                    sket.suggest   = undefined;
                }
            } else {
                sket.basePoint = bestPnt;
                sket.mode      = 4;
                sket.suggest   = undefined;
            }

        // "d" --- depth (dy) between Points
        } else if (myKeyPress == "d" || myKeyPress == "D") {
            var bestPnt = sketcherGetClosestPoint();
            var bestCon = sketcherGetClosestConstraint();

            if (bestCon !== undefined) {
                var xmid = (2*sket.pnt.x[sket.con.index1[bestCon]]
                            + sket.pnt.x[sket.con.index2[bestCon]]) / 3;
                var ymid = (  sket.pnt.y[sket.con.index1[bestCon]]
                            + sket.pnt.y[sket.con.index2[bestCon]]) / 2;

                var dpnt = Math.abs(sket.pnt.x[bestPnt] - wv.cursorX)
                         + Math.abs(sket.pnt.y[bestPnt] - wv.cursorY);
                var dcon = Math.abs(xmid                - wv.cursorX)
                         + Math.abs(ymid                - wv.cursorY);

                if (dcon < dpnt) {
                    var dvalue = prompt("Enter depth: ", sket.con.value[bestCon]);
                    if (dvalue !== null) {
                        sket.con.value[bestCon] = dvalue;
                    }

                    sket.bestPoint = undefined;
                    sket.mode      = 3;
                    sket.suggest   = undefined;
                } else {
                    sket.basePoint = bestPnt;
                    sket.mode      = 5;
                    sket.suggest   = undefined;
                }
            } else {
                sket.basePoint = bestPnt;
                sket.mode      = 5;
                sket.suggest   = undefined;
            }

        // "<" --- remove constraint(s) from nearset Point or Segment
        } else if (myKeyPress == "<") {
            sket.mode    = 3;
            sket.suggest = undefined;

            var ibest = sketcherGetClosestPoint();
            var jbest = sketcherGetClosestSegment();
            var kbest = sketcherGetClosestConstraint();

            var dibest = 999999;
            if (ibest >= 0) {
                dibest = Math.abs(wv.cursorX - sket.pnt.x[ibest])
                       + Math.abs(wv.cursorY - sket.pnt.y[ibest]);
            }
            var djbest = 999999;
            if (jbest >= 0) {
                djbest = Math.abs(wv.cursorX - sket.seg.xm[jbest])
                       + Math.abs(wv.cursorY - sket.seg.ym[jbest]);
            }
            var dkbest = 999999;
            if (kbest !== undefined) {
                if        (sket.con.type[kbest] == "W") {
                    var xmid = (  sket.pnt.x[sket.con.index1[kbest]]
                                + sket.pnt.x[sket.con.index2[kbest]]) / 2;
                    var ymid = (2*sket.pnt.y[sket.con.index1[kbest]]
                                + sket.pnt.y[sket.con.index2[kbest]]) / 3;

                    dkbest = Math.abs(wv.cursorX - xmid)
                           + Math.abs(wv.cursorY - ymid);
                } else if (sket.con.type[kbest] == "D") {
                    var xmid = (2*sket.pnt.x[sket.con.index1[kbest]]
                                + sket.pnt.x[sket.con.index2[kbest]]) / 3;
                    var ymid = (  sket.pnt.y[sket.con.index1[kbest]]
                                + sket.pnt.y[sket.con.index2[kbest]]) / 2;

                    dkbest = Math.abs(wv.cursorX - xmid)
                           + Math.abs(wv.cursorY - ymid);
                }
            }

            // remove (selected) constraint at Point ibest
            if (ibest >= 0 && dibest < djbest && dibest < dkbest) {
                if (sket.pnt.lbl[ibest].length == 0) {
                    postMessage("no constraint at Point "+ibest);
                } else if (sket.pnt.lbl[ibest] == "Z") {
                    postMessage("cannot delete a 'Z' constraint");
                } else {
                    var jtype;
                    if (sket.pnt.lbl[ibest].length == 1) {
                        jtype = sket.pnt.lbl[ibest];
                    } else {
                        jtype = prompt("More than one constraint at Point "+ibest+
                                      "\nEnter one character from \""+sket.pnt.lbl[ibest]+
                                      "\" to remove");
                        if (jtype === null) {
                            jtype = "&";
                        }
                    }
                    jtype = jtype.toUpperCase();

                    if        (ibest == 0 && sket.relative == 1 && jtype == "X") {
                        alert("X cannot be deleted from first point if in relative mode");
                    } else if (ibest == 0 && sket.relative == 1 && jtype == "Y") {
                        alert("Y cannot be deleted from first point if in relative mode");
                    } else {
                        for (var icon = 0; icon < sket.con.type.length; icon++) {
                            if (sket.con.index1[icon] == ibest &&
                                sket.con.index2[icon] <  0     &&
                                sket.con.type[  icon] == jtype   ) {
                                postMessage("removing \""+jtype+"\" contraint from Point "+ibest);

                                sket.con.type.splice(  icon, 1);
                                sket.con.index1.splice(icon, 1);
                                sket.con.index2.splice(icon, 1);
                                sket.con.value.splice( icon, 1);

                                sket.pnt.lbl[ibest] = sket.pnt.lbl[ibest].split(jtype).join("");
                                break;
                            }
                        }
                    }
                }

            // remove constraints at Segment jbest
            } else if (jbest >= 0 && djbest < dkbest) {
                if (sket.seg.lbl[jbest].length == 0) {
                    postMessage("no constraint at Segment "+jbest);
                } else if (sket.seg.lbl[ibest] == "Z") {
                    postMessage("cannot delete a 'Z' constraint");
                } else {
                    var jtype;
                    if (sket.seg.lbl[jbest].length == 1) {
                        jtype = sket.seg.lbl[jbest];
                    } else {
                        jtype = prompt("More than one constraint at Segment "+jbest+
                                      "\nEnter one character from \""+sket.seg.lbl[jbest]+
                                      "\" to remove");
                        if (jtype === null) {
                            jtype = "&";
                        }
                    }
                    jtype = jtype.toUpperCase();

                    for (var icon = 0; icon < sket.con.type.length; icon++) {
                        if (sket.con.index1[icon] == jbest &&
                            sket.con.index2[icon] >= 0     &&
                            sket.con.type[  icon] == jtype   ) {
                            postMessage("removing \""+jtype+"\" constraint from Segment "+jbest);

                            sket.con.type.splice(  icon, 1);
                            sket.con.index1.splice(icon, 1);
                            sket.con.index2.splice(icon, 1);
                            sket.con.value.splice( icon, 1);

                            sket.seg.lbl[jbest] = sket.seg.lbl[jbest].split(jtype).join("");
                            break;
                        }
                    }
                }

            // remove H or W constraint
            } else {
                postMessage("removing constraint "+kbest);

                sket.con.type.splice(  kbest, 1);
                sket.con.index1.splice(kbest, 1);
                sket.con.index2.splice(kbest, 1);
                sket.con.value.splice( kbest, 1);
            }

        // "?" --- examine Point or Segment
        } else if (myKeyPress == "?") {
            var ibest = sketcherGetClosestPoint();
            var jbest = sketcherGetClosestSegment();

            var dibest = 999999;
            if (ibest >= 0) {
                dibest = Math.abs(wv.cursorX - sket.pnt.x[ibest])
                       + Math.abs(wv.cursorY - sket.pnt.y[ibest]);
            }
            var djbest = 999999;
            if (jbest >= 0) {
                djbest = Math.abs(wv.cursorX - sket.seg.xm[jbest])
                       + Math.abs(wv.cursorY - sket.seg.ym[jbest]);
            }

            // examine Point ibest
            if (ibest >= 0 && dibest < djbest) {
                if (sket.pnt.lbl[ibest].length == 0) {
                    postMessage("Point "+(ibest+1)+" has no constraints");
                } else {
                    postMessage("Point "+(ibest+1)+" has constraints: "+sket.pnt.lbl[ibest]);
                }

            // examine Segment jbest
            } else if (jbest >= 0) {
                if        (sket.seg.type[jbest] == "L") {
                    var segtype = "linseg";
                } else if (sket.seg.type[jbest] == "C") {
                    var segtype = "cirarc";
                } else if (sket.seg.type[jbest] == "S") {
                    var segtype = "spline";
                } else if (sket.seg.type[jbest] == "B") {
                    var segtype = "bezier";
                } else {
                    var segtype = "**unknown type**";
                }
                if (sket.seg.lbl[jbest].length == 0) {
                    postMessage("Segment "+(jbest+1)+" ("+segtype+") has no constraints");
                } else {
                    postMessage("Segment "+(jbest+1)+" ("+segtype+") has constraints: "+sket.seg.lbl[jbest]);
                }
            }

        // "@" --- dump sket structure
        } else if (myKeyPress == "@") {
            postMessage("sket.pnt->"+JSON.stringify(sket.pnt));
            postMessage("sket.seg->"+JSON.stringify(sket.seg));
            postMessage("sket.var->"+JSON.stringify(sket.var));
            postMessage("sket.con->"+JSON.stringify(sket.con));

        // invalid option
        } else {
            var ibest = sketcherGetClosestPoint();
            var jbest = sketcherGetClosestSegment();

            var dibest = 999999;
            if (ibest >= 0) {
                dibest = Math.abs(wv.cursorX - sket.pnt.x[ibest])
                       + Math.abs(wv.cursorY - sket.pnt.y[ibest]);
            }
            var djbest = 999999;
            if (jbest >= 0) {
                djbest = Math.abs(wv.cursorX - sket.seg.xm[jbest])
                       + Math.abs(wv.cursorY - sket.seg.ym[jbest]);
            }

            // remove constraints at Point ibest
            if (ibest >= 0 && dibest < djbest) {
                postMessage("Valid options are 'x', 'y', 'p', 't', 'w', 'd', and '<'");
            } else {
                postMessage("Valid options are 'h', 'v', 'i', 'l', 'r', 's', and '<'");
            }
        }

        // if .scale has been set, try to update .xorig or .yorig
        //    if a suitable constraint exists
        if (sket.scale !== undefined) {
            if (sket.xorig === undefined) {
                for (var icon = 0; icon < ncon; icon++) {
                    if (sket.con.type[icon] == "X") {
                        sket.xorig = sket.pnt.x[sket.con.index1[icon]] - sket.con.value[icon] / sket.scale;
                        break;
                    }
                }
            }
            if (sket.yorig === undefined) {
                for (var icon = 0; icon < ncon; icon++) {
                    if (sket.con.type[icon] == "Y") {
                        sket.yorig = sket.pnt.y[sket.con.index1[icon]] + sket.con.value[icon] / sket.scale;
                        break;
                    }
                }
            }
        }

    // sket.mode 4 options: w
    } else if (sket.mode == 4) {
        sket.mode    = 3;
        sket.suggest = undefined;

        var target = sketcherGetClosestPoint();

        if (target == sket.basePoint) {
            alert("Width not set since base and target Point are the same");
        } else {
            var wvalue = (sket.pnt.x[target] - sket.pnt.y[sket.basePoint]) * sket.scale;
            if (wvalue > 0) {
                wvalue = prompt("Enter width (should be positive as drawn): ", wvalue);
            } else {
                wvalue = prompt("Enter width (should be negative as drawn): ", wvalue);
            }
            if (wvalue !== null) {
                sket.con.type[  ncon] = "W";
                sket.con.index1[ncon] = sket.basePoint;
                sket.con.index2[ncon] = target;
                sket.con.value[ ncon] = wvalue;
            }
        }

        sket.basePoint = undefined;

    // sket.mode 5 options: d
    } else if (sket.mode == 5) {
        sket.mode    = 3;
        sket.suggest = undefined;

        var target = sketcherGetClosestPoint();

        if (target == sket.basePoint) {
            alert("Depth not set since base and target Point are the same");
        } else {
            var dvalue = (sket.pnt.y[sket.basePoint] - sket.pnt.y[target]) * sket.scale;
            if (dvalue > 0) {
                dvalue = prompt("Enter depth (should be positive as drawn): ", dvalue);
            } else {
                dvalue = prompt("Enter depth (should be negative as drawn): ", dvalue);
            }
            if (dvalue !== null) {
                sket.con.type[  ncon] = "D";
                sket.con.index1[ncon] = sket.basePoint;
                sket.con.index2[ncon] = target;
                sket.con.value[ ncon] = dvalue;
            }
        }

        sket.basePoint = undefined;
    }

    sketcherDraw();
};

////////////////////////////////////////////////////////////////////////////////


//
// called when "solveButton" is pressed
//
var sketcherSolvePre = function () {
    // alert("sketcherSolvePre()");

    var npnt = sket.pnt.x.length;
    var nvar = sket.var.name.length;
    var ncon = sket.con.type.length;
    var nseg = sket.seg.type.length;

    // if (nvar > ncon) {
    //     alert("Sketch is under-constrained since nvar="+nvar+" but ncon="+ncon);
    //     return;
    // } else if (nvar < ncon) {
    //     alert("Sketch is over-constrained since nvar="+nvar+" but ncon="+ncon);
    //     return;
    // }

    if (sket.xorig === undefined || isNaN(sket.xorig) ||
        sket.yorig === undefined || isNaN(sket.yorig) ||
        sket.scale === undefined || isNaN(sket.scale)   ) {

        // get the size of the canvas
        var canvas = document.getElementById("sketcher");
        var width  = canvas.clientWidth;
        var height = canvas.clientHeight;

        // guess that scale of body is "5" and that it is centered at origin
        sket.scale = 5 / width;
        sket.xorig = width  / 2;
        sket.yorig = height / 2;
    }

    // create a message to send to the server
    var message = "solveSketch|";

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        if (ipnt == 0) {
            var im1 = npnt - 1;
        } else {
            var im1 = ipnt - 1;
        }

        message += Number(sket.scale * (sket.pnt.x[ipnt] - sket.xorig)).toFixed(6) + ";";
        message += Number(sket.scale * (sket.yorig - sket.pnt.y[ipnt])).toFixed(6) + ";";

        if (ipnt > 0) {
            message += Number(sket.scale *  sket.seg.dip[ipnt-1]      ).toFixed(6) + ";";
        } else if (sket.seg.iend[nseg-1] == 0) {
            message += Number(sket.scale *  sket.seg.dip[npnt-1]      ).toFixed(6) + ";";
        } else {
            message += "0;";
        }
    }

    message += "|";

    for (var icon = 0; icon < ncon; icon++) {
        message += sket.con.type[icon] + ";" + (sket.con.index1[icon]+1) + ";";
        if (sket.con.index2[icon] >= 0) {
            message += (sket.con.index2[icon] + 1) + ";" + sket.con.value[icon] + ";";
        } else {
            message +=  sket.con.index2[icon]      + ";" + sket.con.value[icon] + ";";
        }
    }

    message += "|";

    browserToServer(message);

//    // change button legend
//    var button = document.getElementById("solveButton");
//    button["innerHTML"] = "Solving...";
//    button.style.backgroundColor = "#FFFF3F";
};


//
// update the variables associated with a Sketch
//
var sketcherSolvePost = function (text) {
    // alert("sketcherSolvePost(text="+text+")");

    // break up based upon the "|"
    var textList = text.split("|");

    if        (textList[1].substring(0,7) == "ERROR::") {
        postMessage(textList[1]);
        alert("Sketch was not successfully solved, but no hints are available\n" +
              "Note: try adding a R or S constraint to a circular arc.");
        return;
    } else if (textList[1].substring(0,4) == "*del") {
        sket.suggest = textList[1];
        sketcherDraw();
        postMessage("Delete one of the constraints in red (using < key)\n" +
                    "Note: others may be possible as well.");
        return;
    } else if (textList[1].substring(0,4) == "*add") {
        sket.suggest = textList[1];
        sketcherDraw();
        postMessage("Add one of the constraints in green\n" +
                    "Note: others may be possible as well.");
        return;
    }

    var npnt = 0;
    for (var i = 0; i < textList[1].length; i++) {
        if (textList[1].charAt(i) == ';') {
            npnt++;
        }
    }
    npnt /= 3;

    // update the values (which are separated by semicolons)
    var values = textList[1].split(";");

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        if (ipnt == 0) {
            var im1 = npnt - 1;
        } else {
            var im1 = ipnt - 1;
        }

//        sket.pnt.x[  ipnt] = Math.floor(sket.xorig + Number(values[3*ipnt  ]) / sket.scale);
//        sket.pnt.y[  ipnt] = Math.floor(sket.yorig - Number(values[3*ipnt+1]) / sket.scale);
//        sket.seg.dip[im1 ] = Math.floor(             Number(values[3*ipnt+2]) / sket.scale);
        sket.pnt.x[  ipnt] = sket.xorig + Number(values[3*ipnt  ]) / sket.scale;
        sket.pnt.y[  ipnt] = sket.yorig - Number(values[3*ipnt+1]) / sket.scale;
        sket.seg.dip[im1 ] =              Number(values[3*ipnt+2]) / sket.scale;
    }

    // update data associated with segments
    for (var iseg = 0; iseg < sket.seg.type.length; iseg++) {
        sketcherUpdateSegData(iseg);
    }

    // set the mode
    sket.mode = 6;

    // redraw the Sketch with the updated values
    sketcherDraw();
};


//
// called when "save" is pressed
//
var sketcherSave = function () {
    // alert("in sketcherSave()");

    // if the scaling is not set, make sure that at least one length is given
    //    (or else NaN will be sent back to .csm)
    if        (sket.scale === undefined || isNaN(sket.scale)) {
        alert("At lest one length or radius must be set before saving");
        return;
    } else if (sket.xorig === undefined || isNaN(sket.xorig)) {
        alert("At least one \"X\" must be set before saving");
        return;
    } else if (sket.yorig === undefined || isNaN(sket.yorig)) {
        alert("At least one \"Y\" must be set before saving");
        return;
    }

    // alert user that an unsolved Sketch may cause .csm to not solve
    if (sket.mode != 6) {
        if (confirm("Sketch is not solved and may break build.  Continue?") !== true) {
            return;
        }
    }

    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;
    var ncon = sket.con.type.length;

    // build a string of the variables
    var vars = "";
    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        vars += Number(sket.scale * (sket.pnt.x[ipnt] - sket.xorig)).toFixed(6) + ";";
        vars += Number(sket.scale * (sket.yorig - sket.pnt.y[ipnt])).toFixed(6) + ";";

        if (ipnt > 0) {
            vars += Number(sket.scale *  sket.seg.dip[ipnt-1]      ).toFixed(6) + ";";
        } else if (sket.seg.iend[nseg-1] == 0) {
            vars += Number(sket.scale *  sket.seg.dip[npnt-1]      ).toFixed(6) + ";";
        } else {
            vars += "0;";
        }
    }

    // build a string of the constraints
    var cons = "";
    for (var icon = 0; icon < ncon; icon++) {
        cons += sket.con.type[icon] + ";" + (sket.con.index1[icon]+1) + ";";
        if (sket.con.index2[icon] >= 0) {
            cons += (sket.con.index2[icon] + 1) + ";" + sket.con.value[icon] + ";";
        } else {
            cons +=  sket.con.index2[icon]      + ";" + sket.con.value[icon] + ";";
        }
    }
    cons = cons.replace(/\s/g, "");

    // build a string of the segments
    var segs = "";
    for (var iseg = 0; iseg < nseg; iseg++) {
        segs += sket.seg.type[iseg] + ";" + (sket.seg.ibeg[iseg]+1) + ";" +
                                            (sket.seg.iend[iseg]+1) + ";";
    }

    // create the message to be sent to the server
    var mesg = ""+sket.ibrch+"|"+vars+"|"+cons+"|"+segs+"|";

    // because of an apparent limit on the size of text
    //    messages that can be sent from the browser to the
    //    server, we need to send the new file back in
    //    pieces and then reassemble on the server
    var maxMessageSize = 1000;

    var ichar = 0;
    var part  = mesg.substring(ichar, ichar+maxMessageSize);
    browserToServer("saveSketchBeg|"+part);
    ichar += maxMessageSize;

    while (ichar < mesg.length) {
        part = mesg.substring(ichar, ichar+maxMessageSize);
        browserToServer("saveSketchMid|"+part);
        ichar += maxMessageSize;
    }

    browserToServer("saveSketchEnd|");
};


//
// undo last Sketch command
//
var sketcherUndo = function () {
    // alert("sketcherUndo()");

    var iundo = (sket.nundo-1) % sket.maxundo;

    // if undo information does not exist, inform the user
    if (sket.undo[iundo]     === undefined ||
        sket.undo[iundo].pnt === undefined ||
        sket.undo[iundo].seg === undefined ||
        sket.undo[iundo].var === undefined ||
        sket.undo[iundo].con === undefined   ) {
        alert("There is nothing to undo");
        return;
    }

    // remove old Sketch info and initialize the arrays
    sket.pnt        = {};
    sket.pnt.x      = [];
    sket.pnt.y      = [];
    sket.pnt.lbl    = [];

    sket.seg        = {};
    sket.seg.type   = [];
    sket.seg.ibeg   = [];
    sket.seg.iend   = [];
    sket.seg.xm     = [];
    sket.seg.ym     = [];
    sket.seg.lbl    = [];

    sket.seg.dip    = [];
    sket.seg.xc     = [];
    sket.seg.yc     = [];
    sket.seg.rad    = [];
    sket.seg.tbeg   = [];
    sket.seg.tend   = [];

    sket.var        = {};
    sket.var.name   = [];
    sket.var.value  = [];

    sket.con        = {};
    sket.con.type   = [];
    sket.con.index1 = [];
    sket.con.index2 = [];
    sket.con.value  = [];

    // copy the undo information into Sketch information
    sket.mode = sket.undo[iundo].mode;

    for (var ipnt = 0; ipnt < sket.undo[iundo].pnt.x.length; ipnt++) {
        sket.pnt.x[  ipnt] =  sket.undo[iundo].pnt.x[  ipnt];
        sket.pnt.y[  ipnt] =  sket.undo[iundo].pnt.y[  ipnt];
        sket.pnt.lbl[ipnt] =  sket.undo[iundo].pnt.lbl[ipnt];
    }
    for (var iseg = 0; iseg < sket.undo[iundo].seg.type.length; iseg++) {
        sket.seg.type[iseg] = sket.undo[iundo].seg.type[iseg];
        sket.seg.ibeg[iseg] = sket.undo[iundo].seg.ibeg[iseg];
        sket.seg.iend[iseg] = sket.undo[iundo].seg.iend[iseg];
        sket.seg.xm[  iseg] = sket.undo[iundo].seg.xm[  iseg];
        sket.seg.ym[  iseg] = sket.undo[iundo].seg.ym[  iseg];
        sket.seg.lbl[ iseg] = sket.undo[iundo].seg.lbl[ iseg];
        sket.seg.dip[ iseg] = sket.undo[iundo].seg.dip[ iseg];
        sket.seg.xc[  iseg] = sket.undo[iundo].seg.xc[  iseg];
        sket.seg.yc[  iseg] = sket.undo[iundo].seg.yc[  iseg];
        sket.seg.rad[ iseg] = sket.undo[iundo].seg.rad[ iseg];
        sket.seg.tbeg[iseg] = sket.undo[iundo].seg.tbeg[iseg];
        sket.seg.tend[iseg] = sket.undo[iundo].seg.tend[iseg];
    }
    for (var ivar = 0; ivar <  sket.undo[iundo].var.name.length; ivar++) {
        sket.var.name[ ivar] = sket.undo[iundo].var.name[ ivar];
        sket.var.value[ivar] = sket.undo[iundo].var.value[ivar];
    }
    for (var icon = 0; icon <   sket.undo[iundo].con.type.length; icon++){
        sket.con.type[  icon] = sket.undo[iundo].con.type[  icon];
        sket.con.index1[icon] = sket.undo[iundo].con.index1[icon];
        sket.con.index2[icon] = sket.undo[iundo].con.index2[icon];
        sket.con.value[ icon] = sket.undo[iundo].con.value[ icon];
    }

    // remove the undo information
    sket.undo[iundo] = {};

    // decrement number of undos
    sket.nundo--;

    // update the display
    sketcherDraw();
};


//
// save an undo snapshot
//
var sketcherSaveUndo = function () {
    // alert("sketcherSaveUndo()");

    var iundo = sket.nundo % sket.maxundo;

    // remove old undo info and initialize the arrays
    sket.undo[iundo]            = {};

    sket.undo[iundo].pnt        = {};
    sket.undo[iundo].pnt.x      = [];
    sket.undo[iundo].pnt.y      = [];
    sket.undo[iundo].pnt.lbl    = [];

    sket.undo[iundo].seg        = {};
    sket.undo[iundo].seg.type   = [];
    sket.undo[iundo].seg.ibeg   = [];
    sket.undo[iundo].seg.iend   = [];
    sket.undo[iundo].seg.xm     = [];
    sket.undo[iundo].seg.ym     = [];
    sket.undo[iundo].seg.lbl    = [];

    sket.undo[iundo].seg.dip    = [];
    sket.undo[iundo].seg.xc     = [];
    sket.undo[iundo].seg.yc     = [];
    sket.undo[iundo].seg.rad    = [];
    sket.undo[iundo].seg.tbeg   = [];
    sket.undo[iundo].seg.tend   = [];

    sket.undo[iundo].var        = {};
    sket.undo[iundo].var.name   = [];
    sket.undo[iundo].var.value  = [];

    sket.undo[iundo].con        = {};
    sket.undo[iundo].con.type   = [];
    sket.undo[iundo].con.index1 = [];
    sket.undo[iundo].con.index2 = [];
    sket.undo[iundo].con.value  = [];

    // copy the Sketch information into the undo storage
    sket.undo[iundo].mode = sket.mode;

    for (var ipnt = 0; ipnt < sket.pnt.x.length; ipnt++) {
        sket.undo[iundo].pnt.x[  ipnt] = sket.pnt.x[  ipnt];
        sket.undo[iundo].pnt.y[  ipnt] = sket.pnt.y[  ipnt];
        sket.undo[iundo].pnt.lbl[ipnt] = sket.pnt.lbl[ipnt];
    }
    for (var iseg = 0; iseg < sket.seg.type.length; iseg++) {
        sket.undo[iundo].seg.type[iseg] = sket.seg.type[iseg];
        sket.undo[iundo].seg.ibeg[iseg] = sket.seg.ibeg[iseg];
        sket.undo[iundo].seg.iend[iseg] = sket.seg.iend[iseg];
        sket.undo[iundo].seg.xm[  iseg] = sket.seg.xm[  iseg];
        sket.undo[iundo].seg.ym[  iseg] = sket.seg.ym[  iseg];
        sket.undo[iundo].seg.lbl[ iseg] = sket.seg.lbl[ iseg];
        sket.undo[iundo].seg.dip[ iseg] = sket.seg.dip[ iseg];
        sket.undo[iundo].seg.xc[  iseg] = sket.seg.xc[  iseg];
        sket.undo[iundo].seg.yc[  iseg] = sket.seg.yc[  iseg];
        sket.undo[iundo].seg.rad[ iseg] = sket.seg.rad[ iseg];
        sket.undo[iundo].seg.tbeg[iseg] = sket.seg.tbeg[iseg];
        sket.undo[iundo].seg.tend[iseg] = sket.seg.tend[iseg];
    }
    for (var ivar = 0; ivar < sket.var.name.length; ivar++) {
        sket.undo[iundo].var.name[ ivar] = sket.var.name[ ivar];
        sket.undo[iundo].var.value[ivar] = sket.var.value[ivar];
    }
    for (var icon = 0; icon < sket.con.type.length; icon++){
        sket.undo[iundo].con.type[  icon] = sket.con.type[  icon];
        sket.undo[iundo].con.index1[icon] = sket.con.index1[icon];
        sket.undo[iundo].con.index2[icon] = sket.con.index2[icon];
        sket.undo[iundo].con.value[ icon] = sket.con.value[ icon];
    }

    // increment number of undos
    sket.nundo++;
};


//
// update the variables associated with a segment
//
var sketcherUpdateSegData = function (iseg) {
    // alert("sketcherUpdateSegData(iseg="+iseg+")");

    sket.seg.xm[iseg] = (sket.pnt.x[sket.seg.ibeg[iseg]] + sket.pnt.x[sket.seg.iend[iseg]]) / 2;
    sket.seg.ym[iseg] = (sket.pnt.y[sket.seg.ibeg[iseg]] + sket.pnt.y[sket.seg.iend[iseg]]) / 2;

    if (Math.abs(sket.seg.dip[iseg]) > 1.0e-6) {
        var xa = sket.pnt.x[sket.seg.ibeg[iseg]];
        var ya = sket.pnt.y[sket.seg.ibeg[iseg]];
        var xb = sket.pnt.x[sket.seg.iend[iseg]];
        var yb = sket.pnt.y[sket.seg.iend[iseg]];
        var d  =            sket.seg.dip[ iseg];

        var L  = Math.sqrt((xb-xa) * (xb-xa) + (yb-ya) * (yb-ya));
        var R  = (L*L + 4*d*d) / (8*d);

        sket.seg.xc[iseg] = (xa + xb) / 2 + (R - d) * (yb - ya) / L;
        sket.seg.yc[iseg] = (ya + yb) / 2 - (R - d) * (xb - xa) / L;

        sket.seg.rad[ iseg] = R;
        sket.seg.tbeg[iseg] = Math.atan2(ya-sket.seg.yc[iseg], xa-sket.seg.xc[iseg]);
        sket.seg.tend[iseg] = Math.atan2(yb-sket.seg.yc[iseg], xb-sket.seg.xc[iseg]);

        if (R > 0) {
            sket.seg.tbeg[iseg] -= 2 * Math.PI;
            while (sket.seg.tbeg[iseg] < sket.seg.tend[iseg]) {
                sket.seg.tbeg[iseg] += 2 * Math.PI;
            }
        } else {
            sket.seg.tbeg[iseg] += 2 * Math.PI;
            while (sket.seg.tbeg[iseg] > sket.seg.tend[iseg]) {
                sket.seg.tbeg[iseg] -= 2 * Math.PI;
            }
        }

        var tavg = (sket.seg.tbeg[iseg] + sket.seg.tend[iseg]) / 2;
        sket.seg.xm[iseg] = sket.seg.xc[iseg] + Math.cos(tavg) * Math.abs(R);
        sket.seg.ym[iseg] = sket.seg.yc[iseg] + Math.sin(tavg) * Math.abs(R);
    }
};


//
// Draw the current Sketch
//
var sketcherDraw = function () {
    // alert("sketcherDraw()");

    // set up context
    var canvas  = document.getElementById("sketcher");
    var context = canvas.getContext("2d");

    // clear the screen
    context.clearRect(0, 0, canvas.width, canvas.height);

    context.lineCap  = "round";
    context.lineJoin = "round";

    // determine number of Points and Segments in Sketch
    var npnt = sket.pnt.x.length;
    var nseg = sket.seg.type.length;
    var ncon = sket.con.type.length;

    // return if no Points
    if (npnt <= 0) {
        var button = document.getElementById("solveButton");
        button.style.backgroundColor =  "#FFFF3F";
        button["innerHTML"] = "Initializing...";

        return;
    }

    // axes
    context.strokeStyle = "yellow";
    context.lineWidth   = 1;

    context.beginPath();
    context.moveTo(sket.pnt.x[0], -100);
    context.lineTo(sket.pnt.x[0], 1000);
    context.stroke();

    context.beginPath();
    context.moveTo(-100, sket.pnt.y[0]);
    context.lineTo(1000, sket.pnt.y[0]);
    context.stroke();

    // draw the part of the Sketch that is complete
    if (nseg > 0) {

        // we are editting the Cirarc at the end, so it has to
        //    be drawn separately (since it will be drawn in red)
        if (sket.mode == 2) {

            // the part of the Sketch that is done
            context.strokeStyle = "black";
            context.lineWidth   = 3;
            context.beginPath();

            var ibeg = sket.seg.ibeg[0];
            var iend = sket.seg.iend[0];
            context.moveTo(sket.pnt.x[ibeg], sket.pnt.y[ibeg]);

            for (var iseg = 0; iseg < nseg-1; iseg++) {
                iend = sket.seg.iend[iseg];

                if (Math.abs(sket.seg.dip[iseg]) < 1e-6) {
                    context.lineTo(sket.pnt.x[iend], sket.pnt.y[iend]);
                } else if (sket.seg.rad[iseg] > 0) {
                    context.arc(sket.seg.xc[iseg], sket.seg.yc[iseg],
                                +sket.seg.rad[iseg], sket.seg.tbeg[iseg], sket.seg.tend[iseg], 1);
                } else {
                    context.arc(sket.seg.xc[iseg], sket.seg.yc[iseg],
                                -sket.seg.rad[iseg], sket.seg.tbeg[iseg], sket.seg.tend[iseg], 0);
                }
            }

            context.stroke();

            // the Cirarc at the end which is to be rendered in red
            context.strokeStyle = "red";
            context.beginPath();

            if (sket.seg.dip[nseg-1] > 0) {
                context.arc(sket.seg.xc[nseg-1], sket.seg.yc[nseg-1],
                            +sket.seg.rad[nseg-1], sket.seg.tbeg[nseg-1], sket.seg.tend[nseg-1], 1);
            } else {
                context.arc(sket.seg.xc[nseg-1], sket.seg.yc[nseg-1],
                            -sket.seg.rad[nseg-1], sket.seg.tbeg[nseg-1], sket.seg.tend[nseg-1], 0);
            }

            context.stroke();

        // we are NOT editting the Cirarc at the end
        } else {
            context.strokeStyle = "black";
            context.lineWidth   = 3;
            context.beginPath();

            var ibeg = sket.seg.ibeg[0];
            var iend = sket.seg.iend[0];
            context.moveTo(sket.pnt.x[ibeg], sket.pnt.y[ibeg]);

            for (var iseg = 0; iseg < nseg; iseg++) {
                iend = sket.seg.iend[iseg];

                if (sket.seg.type[iseg] == "B") {
                    var Xbezier = [];
                    var Ybezier = [];
                    var nbezier = 2;

                    Xbezier[0] = sket.pnt.x[ibeg];
                    Ybezier[0] = sket.pnt.y[ibeg];
                    Xbezier[1] = sket.pnt.x[iend];
                    Ybezier[1] = sket.pnt.y[iend];

                    while (iseg < nseg-1) {
                        if (sket.seg.type[iseg+1] == "B") {
                            iseg++;
                            iend = sket.seg.iend[iseg];
                            Xbezier[nbezier] = sket.pnt.x[iend];
                            Ybezier[nbezier] = sket.pnt.y[iend];
                            nbezier++;
                        } else {
                            break;
                        }
                    }

                    sketcherDrawBezier(context, Xbezier, Ybezier);
//$$$             } else if (sket.seg.type[iseg] == "S") {
//$$$                 var Xspline = [];
//$$$                 var Yspline = [];
//$$$                 var nspline = 2;
//
//$$$                 Xspline[0] = sket.pnt.x[ibeg];
//$$$                 Yspline[0] = sket.pnt.y[ibeg];
//$$$                 Xspline[1] = sket.pnt.x[iend];
//$$$                 Yspline[1] = sket.pnt.y[iend];
//
//$$$                 while (iseg < nseg-1) {
//$$$                     if (sket.seg.type[iseg+1] == "S") {
//$$$                         iseg++;
//$$$                         iend = sket.seg.iend[iseg];
//$$$                         Xspline[nspline] = sket.pnt.x[iend];
//$$$                         Yspline[nspline] = sket.pnt.y[iend];
//$$$                         nspline++;
//$$$                     } else {
//$$$                         break;
//$$$                     }
//$$$                 }
//
//$$$                 sketcherDrawSpline(context, Xspline, Yspline);
                } else if (Math.abs(sket.seg.dip[iseg]) < 1e-6) {
                    context.lineTo(sket.pnt.x[iend], sket.pnt.y[iend]);
                } else if (sket.seg.dip[iseg] > 0) {
                    context.arc(sket.seg.xc[iseg], sket.seg.yc[iseg],
                                +sket.seg.rad[iseg], sket.seg.tbeg[iseg], sket.seg.tend[iseg], 1);
                } else {
                    context.arc(sket.seg.xc[iseg], sket.seg.yc[iseg],
                                -sket.seg.rad[iseg], sket.seg.tbeg[iseg], sket.seg.tend[iseg], 0);
                }

                ibeg = iend;
            }

            context.stroke();

            // if Sketch is closed, fill it
            if (iend == sket.seg.ibeg[0]) {
                if (sket.var.name.length == sket.con.type.length) {
                    context.fillStyle = "#c0f0c0";  // light green
                    context.fill();
                } else {
                    context.fillStyle = "#e0e0e0";  // grey
                    context.fill();
                }
            }

            // if draw straight lines for Segments associated with splines
            //    and beziers
            context.strokeStyle = "cyan";
            context.lineWidth   = 1;
            for (iseg = 0; iseg < nseg; iseg++) {
                if (sket.seg.type[iseg] == "B" ||
                    sket.seg.type[iseg] == "S"   ) {

                    ibeg = sket.seg.ibeg[iseg];
                    iend = sket.seg.iend[iseg];
                    context.beginPath();
                    context.moveTo(sket.pnt.x[ibeg], sket.pnt.y[ibeg]);
                    context.lineTo(sket.pnt.x[iend], sket.pnt.y[iend]);
                    context.stroke();
                }
            }

            // we are creating W constraint
            if (sket.mode == 4) {
                var xmid = (  sket.pnt.x[sket.basePoint] + wv.cursorX) / 2;
                var ymid = (2*sket.pnt.y[sket.basePoint] + wv.cursorY) / 3;

                context.strokeStyle = "red";
                context.lineWidth   = 1;
                context.beginPath();
                context.moveTo(sket.pnt.x[sket.basePoint], sket.pnt.y[sket.basePoint]);
                context.lineTo(sket.pnt.x[sket.basePoint], ymid                      );
                context.lineTo(wv.cursorX,                 ymid                      );
                context.lineTo(wv.cursorX,                 wv.cursorY                );
                context.stroke();
            }

            // we are creating D constraint
            if (sket.mode == 5) {
                var xmid = (2*sket.pnt.x[sket.basePoint] + wv.cursorX) / 3;
                var ymid = (  sket.pnt.y[sket.basePoint] + wv.cursorY) / 2;

                context.strokeStyle = "red";
                context.lineWidth   = 1;
                context.beginPath();
                context.moveTo(sket.pnt.x[sket.basePoint], sket.pnt.y[sket.basePoint]);
                context.lineTo(xmid,                       sket.pnt.y[sket.basePoint]);
                context.lineTo(xmid,                       wv.cursorY                );
                context.lineTo(wv.cursorX,                 wv.cursorY                );
                context.stroke();
            }
        }
    }

    // draw the Points
    context.fillStyle = "black";
    context.fillRect(sket.pnt.x[0]-3, sket.pnt.y[0]-3, 7, 7);

    for (var ipnt = 1; ipnt < npnt; ipnt++) {
        context.fillRect(sket.pnt.x[ipnt]-2, sket.pnt.y[ipnt]-2, 5, 5);
    }

    // display constraint labels associated with the Points
    for (ipnt = 0; ipnt < npnt; ipnt++) {
        if (sket.pnt.lbl[ipnt] != "") {
            context.font         = "10px Verdana";
            context.textAlign    = "center";
            context.textBaseline = "middle";

            var width  = context.measureText(sket.pnt.lbl[ipnt]).width;
            var height = 8;

            context.fillStyle    = "yellow";
            context.fillRect(sket.pnt.x[ipnt]-width/2-2, sket.pnt.y[ipnt]-height/2-2, width+4, height+4);

            context.fillStyle    = "black";
            context.fillText(sket.pnt.lbl[ipnt], sket.pnt.x[ipnt], sket.pnt.y[ipnt]);
        }
    }

    // display constraint labels associated with the Segments
    for (var iseg = 0; iseg < nseg; iseg++) {
        if (sket.seg.lbl[iseg] != "") {
            context.font         = "12px Verdana";
            context.textAlign    = "center";
            context.textBaseline = "middle";

            var width  = context.measureText(sket.seg.lbl[iseg]).width;
            var height = 8;

            context.fillStyle    = "yellow";
            context.fillRect(sket.seg.xm[iseg]-width/2-2, sket.seg.ym[iseg]-height/2-2, width+4, height+4);

            context.fillStyle    = "black";
            context.fillText(sket.seg.lbl[iseg], sket.seg.xm[iseg], sket.seg.ym[iseg]);
        }
    }

    // display width and depth constraints
    for (var icon = 0; icon < ncon; icon++) {
        if (sket.con.type[icon] == "W" || sket.con.type[icon] == "D") {
            var ibeg = sket.con.index1[icon];
            var iend = sket.con.index2[icon];

            context.strokeStyle = "green";
            context.lineWidth   = 1;
            context.beginPath();
            context.moveTo(sket.pnt.x[ibeg], sket.pnt.y[ibeg]);
            if (sket.con.type[icon] == "W") {
                var xmid = (  sket.pnt.x[ibeg] + sket.pnt.x[iend]) / 2;
                var ymid = (2*sket.pnt.y[ibeg] + sket.pnt.y[iend]) / 3;

                context.lineTo(sket.pnt.x[ibeg], ymid);
                context.lineTo(sket.pnt.x[iend], ymid);
            } else {
                var xmid = (2*sket.pnt.x[ibeg] + sket.pnt.x[iend]) / 3;
                var ymid = (  sket.pnt.y[ibeg] + sket.pnt.y[iend]) / 2;

                context.lineTo(xmid, sket.pnt.y[ibeg]);
                context.lineTo(xmid, sket.pnt.y[iend]);
            }
            context.lineTo(sket.pnt.x[iend], sket.pnt.y[iend]);
            context.stroke();

            context.font         = "12px Verdana";
            context.textAlign    = "center";
            context.textBaseline = "middle";

            if (sket.con.type[icon] == "W") {
                var label = "W";
            } else {
                var label = "D";
            }
            var width  = context.measureText(label).width;
            var height = 8;

            context.fillStyle = "yellow";
            context.fillRect(xmid-width/2-2, ymid-height/2-2, width+4, height+4);

            context.fillStyle = "black";
            context.fillText(label, xmid, ymid);
        }
    }

    if (sket.suggest) {
        // suggested Sketch deletions
        if        (sket.suggest.substring(0,4) == "*del") {
            var suggestions = sket.suggest.split(";");

            for (ipnt = 0; ipnt < sket.pnt.x.length; ipnt++) {
                var lbl = "";

                for (var ient = 1; ient < suggestions.length; ient+=3) {
                    if (suggestions[ient+1] == ipnt+1 &&
                        suggestions[ient+2] == -1     ) {
                        lbl += suggestions[ient];
                    }
                }

                if (lbl.length > 0) {
                    context.font         = "10px Verdana";
                    context.textAlign    = "center";
                    context.textBaseline = "middle";

                    var width  = context.measureText(lbl).width;
                    var height = 8;

                    context.fillStyle    = "red";

                    context.clearRect(sket.pnt.x[ipnt]-width/2-2, sket.pnt.y[ipnt]-2*height-2, width+4, height+4);
                    context.fillText(lbl, sket.pnt.x[ipnt], sket.pnt.y[ipnt]-3*height/2);
                }
            }

            for (ipnt = 0; ipnt < sket.pnt.x.length; ipnt++) {
                var lbl = "";

                for (var ient = 1; ient < suggestions.length; ient+=3) {
                    if (suggestions[ient+1] == ipnt+1 &&
                        suggestions[ient+2] >  -1       ) {
                        lbl += suggestions[ient];
                    }
                }

                if (lbl.length > 0) {
                    context.font         = "10px Verdana";
                    context.textAlign    = "center";
                    context.textBaseline = "middle";

                    var width  = context.measureText(lbl).width;
                    var height = 8;

                    context.fillStyle    = "red";

                    context.clearRect(sket.seg.xm[ipnt]-width/2-2, sket.seg.ym[ipnt]-2*height-2, width+4, height+4);
                    context.fillText(lbl, sket.seg.xm[ipnt], sket.seg.ym[ipnt]-3*height/2);
                }
            }

            return;

        // suggested Sketch additions
        } else if (sket.suggest.substring(0,4) == "*add") {
            var suggestions = sket.suggest.split(";");

            for (ipnt = 0; ipnt < sket.pnt.x.length; ipnt++) {
                var lbl = "";

                for (var ient = 1; ient < suggestions.length; ient+=3) {
                    if (suggestions[ient+1] == ipnt+1 &&
                        suggestions[ient+2] == -1     ) {
                        lbl += suggestions[ient];
                    }
                }

                if (lbl.length > 0) {
                    context.font         = "10px Verdana";
                    context.textAlign    = "center";
                    context.textBaseline = "middle";

                    var width  = context.measureText(lbl).width;
                    var height = 8;

                    context.fillStyle    = "green";

                    context.clearRect(sket.pnt.x[ipnt]-width/2-2, sket.pnt.y[ipnt]-2*height-2, width+4, height+4);
                    context.fillText(lbl, sket.pnt.x[ipnt], sket.pnt.y[ipnt]-3*height/2);
                }
            }

            for (ipnt = 0; ipnt < sket.pnt.x.length; ipnt++) {
                var lbl = "";

                for (var ient = 1; ient < suggestions.length; ient+=3) {
                    if (suggestions[ient+1] == ipnt+1 &&
                        suggestions[ient+2] >  -1       ) {
                        lbl += suggestions[ient];
                    }
                }

                if (lbl.length > 0) {
                    context.font         = "10px Verdana";
                    context.textAlign    = "center";
                    context.textBaseline = "middle";

                    var width  = context.measureText(lbl).width;
                    var height = 8;

                    context.fillStyle    = "green";

                    context.clearRect(sket.seg.xm[ipnt]-width/2-2, sket.seg.ym[ipnt]-2*height-2, width+4, height+4);
                    context.fillText(lbl, sket.seg.xm[ipnt], sket.seg.ym[ipnt]-3*height/2);
                }
            }

            return;
        }
    }

    // if we are in mode 1, draw a line from the last Point to the cursor
    if (sket.mode == 1) {

        // nominal line to cursor is blue
        var color = "blue";

        // if horizontal, make it orange
        if (Math.abs(sket.pnt.y[npnt-1]-wv.cursorY) < sket.halo) {
            color = "orange";

        // otherwise see if it alignes with another point
        } else {
            for (var jpnt = 0; jpnt < npnt-1; jpnt++) {
                if (Math.abs(sket.pnt.y[jpnt]-wv.cursorY) < sket.halo) {
                    context.strokeStyle = "orange";
                    context.lineWidth   = 1;

                    context.beginPath();
                    context.moveTo(sket.pnt.x[jpnt], sket.pnt.y[jpnt]);
                    context.lineTo(wv.cursorX,  wv.cursorY );
                    context.stroke();
                    break;
                }
            }
        }

        // if vertical, make it orange
        if (Math.abs(sket.pnt.x[npnt-1]-wv.cursorX) < sket.halo) {
            color = "orange";

        // otherwise see if it alignes with another point
        } else {
            for (var jpnt = 0; jpnt < npnt-1; jpnt++) {
                if (Math.abs(sket.pnt.x[jpnt]-wv.cursorX) < sket.halo) {
                    context.strokeStyle = "orange";
                    context.lineWidth   = 1;

                    context.beginPath();
                    context.moveTo(sket.pnt.x[jpnt], sket.pnt.y[jpnt]);
                    context.lineTo(wv.cursorX,  wv.cursorY );
                    context.stroke();
                    break;
                }
            }
        }

        // if the cursor is close to the first Point, indicate that
        if (Math.abs(sket.pnt.x[0]-wv.cursorX) < sket.halo &&
            Math.abs(sket.pnt.y[0]-wv.cursorY) < sket.halo   ) {
            context.strokeStyle = "orange";
            context.lineWidth   = 5;
            context.beginPath();
            context.arc(sket.pnt.x[0], sket.pnt.y[0], 17, 0, 2*Math.PI);
            context.stroke();
        }

        // draw the line to the cursor
        context.strokeStyle = color;
        context.lineWidth   = 2;

        context.beginPath();
        context.moveTo(sket.pnt.x[npnt-1], sket.pnt.y[npnt-1]);
        context.lineTo(wv.cursorX,    wv.cursorY   );
        context.stroke();
    }

    // have the Build button show the current Sketcher status
    var button = document.getElementById("solveButton");
    button.style.backgroundColor =  "#FFFF3F";

    if        (sket.mode == 0) {
        button["innerHTML"] = "Initializing...";
    } else if (sket.mode == 1) {
        button["innerHTML"] = "Drawing...";
    } else if (sket.mode == 2) {
        button["innerHTML"] = "Setting R...";
//  } else if (sket.mode == 3) {
//      handled below
    } else if (sket.mode == 4) {
        button["innerHTML"] = "Setting W...";
    } else if (sket.mode == 5) {
        button["innerHTML"] = "Setting D...";
    } else if (sket.mode == 6) {
        button["innerHTML"] = "Up to date";
        button.style.backgroundColor = null;
        document.getElementById("doneMenuBtn").style.backgroundColor = "#3FFF3F";
    } else if (sket.var.name.length != sket.con.type.length) {
        button["innerHTML"] = "Constraining...";
        button.style.backgroundColor = "#FFFF3F"
        document.getElementById("doneMenuBtn").style.backgroundColor = null;
    } else {
        button["innerHTML"] = "Press to Solve";
        button.style.backgroundColor = "#3FFF3F";
    }

    // post informtion about current mode in blframe
    var skstat = document.getElementById("sketcherStatus");

    var mesg = "ndof=" + sket.var.name.length + "   ncon=" + sket.con.type.length + "\n";
    if        (sket.mode == 1) {
        mesg += "Valid commands are:\n";
        mesg += "  'l'   add linseg\n";
        mesg += "  'c'   add cirarc\n";
        mesg += "  's'   add spline\n";
        mesg += "  'b'   add bezier\n";
        mesg += "  'z'   add zero-length segment\n";
        mesg += "  'o'   complete (open) sketch\n";
    } else if (sket.mode == 2) {
        mesg += "Hit any character to set curvature\n";
    } else if (sket.mode == 3) {
        mesg += "Valid constraints at points\n";
        mesg += "  'x' (fix x)     'y' (fix y)\n";
        mesg += "  'p' (perp)      't' (tangent)\n";
        mesg += "  'a' (angle)\n";
        mesg += "  'w' (width)     'd' (depth)\n";
        mesg += "Valid constraints on segments\n";
        mesg += "  'h' (horiz)     'v' (vertical)\n";
        mesg += "  'i' (incline)   'l' (length)\n";
        mesg += "Valid constraints on cirarcs\n";
        mesg += "  'r' (radius)    's' (sweep angle)\n";
        mesg += "Valid commands anywhere\n";
        mesg += "  '<' (delete)    '?' (info)\n";
    } else if (sket.mode == 4) {
        mesg += "Hit any character to set width\n";
    } else if (sket.mode == 5) {
        mesg += "Hit any character to set depth\n";
    }

    var pre  = document.createElement("pre");
    var text = document.createTextNode(mesg);
    pre.appendChild(text);

    skstat.replaceChild(pre, skstat.lastChild);
};


// draw a Bezier curve (with evaluations via Hoerner-like scheme)
//
var sketcherDrawBezier = function (context, Xbezier, Ybezier) {
    // alert("sketcherDrawBezier()");

    var degree = Xbezier.length - 1;

    for (var j = 1; j <= 4*degree; j++) {
        var t  = j / (4.0 * degree);
        var t1 = 1.0 - t;

        var fact   = 1.0;
        var bicoef = 1;

        var xx = Xbezier[0] * t1;
        var yy = Ybezier[0] * t1;

        for (var i = 1; i < degree; i++) {
            fact    *= t;
            bicoef  *= (degree - i + 1) / i;

            xx = (xx + fact * bicoef * Xbezier[i]) * t1;
            yy = (yy + fact * bicoef * Ybezier[i]) * t1;
        }

        xx += fact * t * Xbezier[degree];
        yy += fact * t * Ybezier[degree];

        context.lineTo(xx, yy);
    }
};


// draw a Spline curve (with evaluations via Hoerner-like scheme)
//
var sketcherDrawSpline = function (context, Xspline, Yspline) {
    // alert("sketcherDrawSpline()");

    var degree = Xspline.length - 1;

    // convert Spline points into cubic Bezier control points
    var Xbezier = [];
    var Ybezier = [];
    var gam     = [];

    var bet    = 4;
    Xbezier[1] = (6 * Xspline[1] - Xspline[0]) / bet;
    Ybezier[1] = (6 * Yspline[1] - Yspline[0]) / bet;

    for (var i = 1; i < degree-2; i++) {
        gam[i]       = 1 / bet;
        bet          =  4 - gam[i];
        Xbezier[i+1] = (6 * Xspline[i+1] - Xbezier[i]) / bet;
        Ybezier[i+1] = (6 * Yspline[i+1] - Ybezier[i]) / bet;
    }

    gam[degree-2]     = 1 / bet;
    bet               =  4 - gam[degree-2];
    Xbezier[degree-1] = (6 * Xspline[degree-1] - Xspline[degree] - Xbezier[degree-2]) / bet;
    Ybezier[degree-1] = (6 * Yspline[degree-1] - Yspline[degree] - Ybezier[degree-2]) / bet;

    for (i = degree-2; i > 0; i--) {
        Xbezier[i] -= gam[i] * Xbezier[i+1];
        Ybezier[i] -= gam[i] * Ybezier[i+1];
    }

    Xbezier[0] = Xspline[0];
    Ybezier[0] = Yspline[0];

    Xbezier[degree] = Xspline[degree];
    Ybezier[degree] = Yspline[degree];

    // draw the Splines
    for (i = 1; i <= degree; i++) {
        for (var j = 1; j < 5; j++) {
            var t  = 0.25 * j;
            var t1 = 1.0 - t;

            var xx = (t1 * t1 * t1) * (    Xspline[i-1]                 )
                   + (t1 * t1 * t ) * (2 * Xbezier[i-1] +     Xbezier[i])
                   + (t1 * t  * t ) * (    Xbezier[i-1] + 2 * Xbezier[i])
                   + (t  * t  * t ) * (                       Xspline[i]);
            var yy = (t1 * t1 * t1) * (    Yspline[i-1]                 )
                   + (t1 * t1 * t ) * (2 * Ybezier[i-1] +     Ybezier[i])
                   + (t1 * t  * t ) * (    Ybezier[i-1] + 2 * Ybezier[i])
                   + (t  * t  * t ) * (                       Yspline[i]);
            context.lineTo(xx,yy);
        }
    }
};


//
// get Point closest to the cursor
//
var sketcherGetClosestPoint = function () {
    // alert("sketcherGetClosestPoint()");

    var npnt  = sket.pnt.x.length;
    var ibest = -1;
    var dbest = 999999;

    for (var ipnt = 0; ipnt < npnt; ipnt++) {
        var dtest = Math.abs(wv.cursorX - sket.pnt.x[ipnt])
                  + Math.abs(wv.cursorY - sket.pnt.y[ipnt]);
        if (dtest < dbest) {
            ibest = ipnt;
            dbest = dtest;
        }
    }

    return ibest;
};


//
// get Segment closest to the cursor
//
var sketcherGetClosestSegment = function () {
    // alert("sketcherGetClosestSegment()");

    var nseg  = sket.seg.type.length;
    var ibest = -1;
    var dbest = 999999;

    for (var iseg = 0; iseg < nseg; iseg++) {
        var dtest = Math.abs(wv.cursorX - sket.seg.xm[iseg])
                  + Math.abs(wv.cursorY - sket.seg.ym[iseg]);
        if (dtest < dbest) {
            ibest = iseg;
            dbest = dtest;
        }
    }

    return ibest;
};


//
// get W or D constrint closest to the cursor
//
var sketcherGetClosestConstraint = function () {
    // alert("sketcherGetClosestConstraint()");

    var ncon  = sket.con.type.length;
    var ibest = undefined;
    var dbest = 999999;

    for (var icon = 0; icon < ncon; icon++) {
        if        (sket.con.type[icon] == "W") {
            var xmid = (  sket.pnt.x[sket.con.index1[icon]]
                        + sket.pnt.x[sket.con.index2[icon]]) / 2;
            var ymid = (2*sket.pnt.y[sket.con.index1[icon]]
                        + sket.pnt.y[sket.con.index2[icon]]) / 3;
        } else if (sket.con.type[icon] == "D") {
            var xmid = (2*sket.pnt.x[sket.con.index1[icon]]
                        + sket.pnt.x[sket.con.index2[icon]]) / 3;
            var ymid = (  sket.pnt.y[sket.con.index1[icon]]
                        + sket.pnt.y[sket.con.index2[icon]]) / 2;
        } else {
            continue;
        }

        var dtest = Math.abs(wv.cursorX - xmid) + Math.abs(wv.cursorY - ymid);
        if (dtest < dbest) {
            ibest = icon;
            dbest = dtest;
        }
    }

    return ibest;
}
