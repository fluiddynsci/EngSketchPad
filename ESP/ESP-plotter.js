// ESP-plotter.js implements plotter functions for the Engineering Sketch Pad (ESP)
// written by John Dannenhoffer

// Copyright (C) 2010/2022  John F. Dannenhoffer, III (Syracuse University)
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

// interface functions that ESP-plotter.js provides
//    plotter.launch()
//    plotter.cmdUndo()
//    plotter.cmdSolve()
//    plotter.cmdSave()
//    plotter.cmdQuit()
//
//    plotter.cmdHome()                    not provided
//    plotter.cmdLeft()                    not provided
//    plotter.cmdRite()                    not provided
//    plotter.cmdBotm()                    not provided
//    plotter.cmdTop()                     not provided
//    plotter.cmdIn()                      not provided
//    plotter.cmdOut()                     not provided
//
//    plotter.mouseDown(e)                 not provided
//    plotter.mouseMove(e)                 not provided
//    plotter.mouseUp(e)                   not provided
//    plotter.mouseWheel(e)                not provided
//    plotter.mouseLeftCanvas(e)           not provided
//
//    plotter.keyPress(e)                  not provided
//    plotter.keyDown(e)                   not provided
//    plotter.keyUp(e)                     not provided
//    plotter.keyPressPart1(myKeyPress)    not provided
//    plotter.keyPressPart2(picking, gprim)not provided
//    plotter.updateKeyWindow()
//
//    plotter.timLoadCB(text)
//    plotter.timSaveCB(text)
//    plotter.timQuitCB(text)
//    plotter.timMesgCB(text)

"use strict";


//
// callback when Plotter is launched
//
plotter.launch = function () {
    alert("in plotter.launch()");

};


//
// callback when "Plotter->Undo" is pressed (called by ESP.html)
//
plotter.cmdUndo = function () {
    alert("plotter.cmdUndo() is not implemented");
};


//
// callback when "solveButton" is pressed
//
plotter.cmdSolve = function () {
    alert("in plotter.cmdSolve() is not implemented");

};


//
// callback when "Plotter->Save" is pressed (called by ESP.html)
//
plotter.cmdSave = function () {
    alert("in plotter.cmdSave() is not implemented");

};


//
// callback when "Plotter->Quit" is pressed (called by ESP.html)
//
plotter.cmdQuit = function () {
    alert("in plotter.cmdQuit() is not implemented");

};


//
// callback when "homeButton" is pressed (calles by ESP.html)
//
//plotter.cmdHome = function () {
//    main.cmdHome();
//};


//
// callback when "leftButton" is pressed (calles by ESP.html)
//
//plotter.cmdLeft = function () {
//    main.cmdLeft();
//};


//
// callback when "riteButton" is pressed (calles by ESP.html)
//
//plotter.cmdRite = function () {
//    main.cmdRite();
//};


//
// callback when "botmButton" is pressed (calles by ESP.html)
//
//plotter.cmdBotm = function () {
//    main.cmdBotm();
//};


//
// callback when "topButton" is pressed (calles by ESP.html)
//
//plotter.cmdTop = function () {
//    main.cmdTop();
//};


//
// callback when "inButton" is pressed (calles by ESP.html)
//
//plotter.cmdIn = function () {
//    main.cmdIn();
//};


//
// callback when "outButton" is pressed (calles by ESP.html)
//
//plotter.cmdOut = function () {
//    main.cmdOut();
//};


//
// callback when any mouse is pressed in canvas
//
//plotter.mouseDown = function(e) {
//    main.mouseDown(e);
//}


//
// callback when any mouse moves in canvas
//
//plotter.mouseMove = function(e) {
//    main.mouseMove(e);
//}


//
// callback when the mouse is released in canvas
//
//plotter.mouseUp = function(e) {
//    main.mouseUp(e);
//}


//
// callback when the mouse wheel is rolled in canvas
//
//plotter.mouseWheel = function(e) {
//    main.mouseWheel(e);
//}


//
// callback when the mouse leaves the canvas
//
//plotter.mouseLeftCanvas = function(e) {
//    main.mouseLeftCanvas(e);
//}


//
// callback when a key is pressed
//
//plotter.keyPress = function (e) {
//    main.keyPress(e);
//}


//
// callback when an arror... or shift is pressed (needed for Chrome)
//
//plotter.keyDown = function (e) {
//    main.keyDown(e);
//};


//
// callback when a shift is released (needed for Chrome)
//
//plotter.keyUp = function (e) {
//    main.keyUp(e);
//};


//
// callback for first part of a keypress that is not recognized by wvUpdateUI
//
//plotter.keyPressPart1 = function(myKeyPress) {
//    // alert("in plotter.keyPressPart1(myKeyPress="+myKeyPress+")");
//    return 0;
//};


//
// callback for second part of a keypress that is not recognized by wvUpdateUI
//
//plotter.keyPressPart2 = function(picking, gprim) {
//    // alert("in plotter.keyPressPart2(picking="+picking+"   gprim="+gprim+")");
//};


//
// function to update the key window
//
plotter.updateKeyWindow = function () {
    alert("in plotter.updateKeyWindow()");

};


//
// callback when timLoad returns
//
plotter.timLoadCB = function (text) {
    // alert("in plotter.timLoadCB: "+text);

};


//
// callback when timSave returns
//
plotter.timSaveCB = function (text) {
    // alert("in plotter.timSaveCB: "+text);

};


//
// callback when timQuit returns
//
plotter.timQuitCB = function (text) {
    // alert("in plotter.timQuitCB: "+text);

};


//
// callback when timMesg returns
//
plotter.timMesgCB = function (text) {
    // alert("in plotter.timMesgCB: "+text);

};


// /////////////////////////////////////////////////////////////////////

