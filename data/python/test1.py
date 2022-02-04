###################################################################
#                                                                 #
# test1.py --- start with: serveESP ../data/python/test           #
#                  Tool->Python     ../data/python/test1.py       #
#                  Tool->Python     ../data/python/test2.py       #
#                  Tool->Python     ../data/python/test3.py       #
#                                                                 #
#              Written by John Dannenhoffer @ Syracuse University #
#                                                                 #
###################################################################

from pyOCSM import ocsm
from pyOCSM import esp

# espModl is inherited from serveESP
espModl = ocsm.Ocsm(esp.GetModl(esp.GetEsp("python")))

# get the current volume
ipmtr = espModl.FindPmtr("myVol", 0, 0, 0)
myVol = espModl.GetValu(ipmtr, 1, 1)
print("From previous build")
print("    myVol:", myVol);   assert (abs(myVol[0]-24) < 1e-12)
