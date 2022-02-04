###################################################################
#                                                                 #
# plotter1.py -- sample line plotting                             #
#                                                                 #
#              Written by John Dannenhoffer @ Syracuse University #
#                                                                 #
###################################################################

from pyEGADS import egads
from pyOCSM  import ocsm
from pyOCSM  import esp

import math
import time

esp.TimLoad("plotter", esp.GetEsp("python"), "")

# one line
esp.TimMesg("plotter", "new|Example of one line|x-axis label|y-axis label|")
esp.TimMesg("plotter", "add|0;1;2;3;4|0;1;4;9;16|b-|")
esp.TimMesg("plotter", "show")
#esp.TimHold("python", "plotter")

# two lines
esp.TimMesg("plotter", "new|Example of two lines|x-axis label|y-axis label|")
esp.TimMesg("plotter", "add|0;1;2;3;4|0;1;4;9;16|r-|")
esp.TimMesg("plotter", "add|0;1;2;3;4|4;5;6;7;8|g*|")
esp.TimMesg("plotter", "show")
#esp.TimHold("python", "plotter")

# moving sine wave
tdata = ""
sdata = ""
for i in range(33):
    t = i/32.0 * 2 * math.pi
    s = math.sin(t)

    tdata += str(t) + ";"
    sdata += str(s) + ";"

for i in range(33):
    t = i/32.0 * 2 * math.pi
    s = math.sin(t)
    
    esp.TimMesg("plotter", "new|Moving dot|t|sin(t)|")
    esp.TimMesg("plotter", "add|"+tdata+"|"+sdata+"|k:|")
    esp.TimMesg("plotter", "add|"+str(t)+"|"+str(s)+"|ro|")
    esp.TimMesg("plotter", "show")
    time.sleep(0.5)

#esp.TimHold("python", "plotter")
esp.TimQuit("plotter")
