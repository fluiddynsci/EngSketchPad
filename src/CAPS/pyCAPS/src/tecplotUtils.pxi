#
# Written by Dr. Ryan Durscher AFRL/RQVC
# 
# This software has been cleared for public release on 25 Jul 2018, case number 88ABW-2018-3793.

from struct import unpack as structUnpack

cdef class capsTecplotZone:
    
    cdef:
        
        readonly object zoneName
        
        public numElement
        public numNode
        
        public i
        public j
        public k
       
        readonly object variable # Needed to keep track of the order in which the data was stored
        readonly object data
        readonly object connectivity
        
        public object dataPacking
        public object zoneType
        public fileName
        
        readonly _lineOffset
    
    def __cinit__(self, name,  variables, fileName=None):
        
        self.zoneName = name
        
        self.numElement = None
        self.numNode = None
        
        self.i = None
        self.j = None
        self.k = None
        
        self.fileName = fileName
        
        self._lineOffset = 0
        
        if not isinstance(variables, list):
             self.variable = [variables]
        else:
             self.variable = variables
                
        self.data = {}
        for i in self.variable:
            if i: # Deal with empty characters in list 
                self.data[i] = [] # Dictionary [variable] = list of data 
                
        self.connectivity = []# List of list element connectivity 
            
        self.zoneType = "Ordered".upper()
        
        self.dataPacking = "Block".upper()
        
    def _switchIJK(self): # Help to handle archaic version of ASCII formated file 
        
        if "FE" in self.zoneType:
            self.i = None
            self.j = None
            self.k = None
       
        else:
            self.i = self.numNode
            self.j = self.numElement
        
            self.numNode = None
            self.numElement = None
            
    def _checkZoneType(self, zoneType):
        
        if "ORDERED" in zoneType:
            self.zoneType = "ORDERED"
            
        elif "FELINESEG" in zoneType:
            self.zoneType = "FELINESEG"
            
        elif "FETRIANGLE" in zoneType:
            self.zoneType = "FETRIANGLE" 
            
        elif "FEQUADRILATERAL" in zoneType:
            self.zoneType = "FEQUADRILATERAL"
            
        elif "FETETRAHEDRON" in zoneType:
            self.zoneType = "FETETRAHEDRON"
            
        elif "FEBRICK" in zoneType:
            self.zoneType = "FEBRICK"
            
        elif "FEPOLYGON" in zoneType:
            self.zoneType = "FEPOLYGON" 
            
        elif "FEPOLYHEDRAL" in zoneType:
            self.zoneType = "FEPOLYHEDRAL" 
            
        else:
            print "Invalid ZONETYPE - ", zoneType
            raise TypeError
        
    def _checkDataPacking(self, dataPacking):
        
        if "BLOCK" in dataPacking:
            self.dataPacking = "BLOCK"
            
        elif "POINT" in dataPacking:
            self.dataPacking = "POINT"
            
        else:
            print "Invalid DATAPACKING - ", dataPacking
            raise TypeError
    
    # Return true for correct zone on a given line, false otherwise
    def _checkZoneTitle(self, line, zoneCount):
        
        if "Zone_" in self.zoneName:
            
            if self.zoneName != "Zone_" + str(zoneCount):
                return False 
        else:
      
            # Check for zone title 
            if not regSearch(r'[,\s]T\s*=' , line, regMultiLine | regIgnoreCase):
                return False
            
            # Check for zone name 
            if not regSearch(self.zoneName, line, regMultiLine | regIgnoreCase):
                return False
            
        return True
        
    # Parse a zone header
    def readZoneHeader(self):
        
        reZone = regCompile(r'Zone',  regMultiLine | regIgnoreCase)
        
        reNode  = regCompile(r'Nodes\s*=\s*(\d*)', regMultiLine | regIgnoreCase)
        reNode1 = regCompile(r'[,\s]N\s*=\s*(\d*)', regMultiLine | regIgnoreCase)
        reNode2 = regCompile(r'[,\s]I\s*=\s*(\d*)', regMultiLine | regIgnoreCase)
        
        reElement  = regCompile(r'Elements\s*=\s*(\d*)', regMultiLine | regIgnoreCase)
        reElement1 = regCompile(r'[,\s]E\s*=\s*(\d*)', regMultiLine | regIgnoreCase)
        reElement2 = regCompile(r'[,\s]J\s*=\s*(\d*)', regMultiLine | regIgnoreCase)
        
        reK = regCompile(r'K\s*=\s*(\d*)', regMultiLine | regIgnoreCase)
        
        rePack = regCompile(r'DataPacking\s*=\s*(.*)', regMultiLine | regIgnoreCase)
        rePack1 = regCompile(r'[,\s]F\s*=\s*(.*)', regMultiLine | regIgnoreCase)
        
        reZoneType = regCompile(r'ZoneType\s*=\s*(.*)', regMultiLine | regIgnoreCase)
        
        zoneCount = 0
        self._lineOffset = 0
        with open(self.fileName, 'r') as fp:
            
            # Loop through the file and find the zone
            for line in fp.readlines():
                self._lineOffset += len(line)
                
                if not reZone.search(line):
#                 if not regSearch(r'Zone', line, regMultiLine | regIgnoreCase):    
                    continue
                
                zoneCount += 1
                if not self._checkZoneTitle(line, zoneCount-1):
                    continue
                
                # Nodes or i
#                 numNode = regSearch(r'Nodes\s*=\s*(\d*)', line, regMultiLine | regIgnoreCase)
                numNode = reNode.search(line)
                if not numNode:
#                     numNode = regSearch(r'[,\s]N\s*=\s*(\d*)', line, regMultiLine | regIgnoreCase)
                    numNode = reNode1.search(line)
                    if not numNode:
#                         numNode = regSearch(r'[,\s]I\s*=\s*(\d*)', line, regMultiLine | regIgnoreCase)
                        numNode = reNode2.search(line)
                        
                        if not numNode:
                            numNode = None
            
                if numNode:
                    numNode = int(numNode.group(1).replace(",",""))
            
                self.numNode = numNode
            
                # Elements or j
#                 numElement = regSearch(r'Elements\s*=\s*(\d*)', line, regMultiLine | regIgnoreCase)
                numElement = reElement.search(line)
                if not numElement:
#                     numElement = regSearch(r'[,\s]E\s*=\s*(\d*)', line, regMultiLine | regIgnoreCase)
                    numElement = reElement1.search(line)
                    
                    if not numElement:
#                         numElement = regSearch(r'[,\s]J\s*=\s*(\d*)', line, regMultiLine | regIgnoreCase)
                        numElement = reElement2.search(line)
                        if not numElement:
                            numElement = None
            
                if numElement:
                    numElement = int(numElement.group(1).replace(",",""))
                    
                self.numElement = numElement
                
                # k
#                 k = regSearch(r'K\s*=\s*(\d*)', line, regMultiLine | regIgnoreCase)
                k = reK.search(line)
                if k:
                    k = int(k.group(1).replace(",",""))
                else:
                    k = 1
                    
                self.k = k
                
#                 dataPacking = regSearch(r'DataPacking\s*=\s*(.*)', line, regMultiLine | regIgnoreCase)
                dataPacking = rePack.search(line)
                if not dataPacking:
#                     dataPacking = regSearch(r'[,\s]F\s*=\s*(.*)', line, regMultiLine | regIgnoreCase)
                    dataPacking = rePack.search(line)
            
                if dataPacking:
                    self._checkDataPacking(dataPacking.group(1).upper())
                
                else:
                    print "No DATAPACKING specified - defaulting to", self.dataPacking
                
#                 zoneType = regSearch(r'ZoneType\s*=\s*(.*)', line, regMultiLine | regIgnoreCase)
                zoneType = reZoneType.search(line)
                if zoneType:
                    
                    self._checkZoneType(zoneType.group(1).upper())
                
                else:
                    if "FE" in dataPacking.group(1).upper(): # "FEPOINT and FEBLOCK is archaic
                        print "No ZONETYPE specified, but 'FE' found for DataPacking, defaulting to 'FEQUADRILATERAL'"
                        self._checkZoneType("FEQUADRILATERAL")
                    
                    else:
                        print "No ZONETYPE specified - defaulting to", self.zoneType
                
                break # Quit after we have read the zone we were look for
            
        self._switchIJK()
    
    def _checkConnectivity(self, length, line):
        print "ZoneType set to", self.zoneType, "but connectivity length is", length
        print "Line -"
        print line
                           
    # Given a open file parse it to populate the data and connectivity information
    # Assumed the file pointer is on the correct line!
    def _readZoneData(self, repeatConnect = False):
        
        if not self.data:
            print "Data dictionary hasn't been setup yet!"
            raise ValueError
        
        if self.zoneType.upper() == "ORDERED":
            if not self.i:
                print "'I' hasn't been setup yet for ordered data!"
                raise ValueError
            
            if not self.j:
                print "'J' hasn't been setup yet for ordered data!"
                raise ValueError
            
            if not self.k:
                print "'K' hasn't been setup yet for ordered data!"
                raise ValueError
        else:
            if not self.numElement:
                print "Number of elements hasn't been setup yet!"
                raise ValueError
        
            if not self.numNode:
                print "Number of nodes hasn't been setup yet!"
                raise ValueError
    
        print "Reading zone -", self.zoneName
#         print "Line offset-  ", self._lineOffset
        
        # Open the file and read all the lines
        with open(self.fileName, 'r') as fp:
            fp.seek(self._lineOffset)
            allLine = fp.readlines()

# Obsolete with the introduction of _lineOffset
#         # Loop through the file and find the zone
#         zoneCount = 0
#         for i, line in enumerate(allLine):
#
#             if not regSearch(r'Zone', line, regMultiLine | regIgnoreCase):
#                 continue
#                 
#             zoneCount += 1
#             if not self._checkZoneTitle(line, zoneCount-1):
#                 continue
#         
#             break
#         
#         # Remove proceeding lines - don't care about anything prior to our zone 
#         for j in range(i,-1,-1):
#             allLine.pop(j)
                 
        if self.dataPacking.upper() == "POINT":
            
            if self.zoneType.upper() == "ORDERED":
                size = self.i*self.j*self.k
            else:
                size = self.numNode
            
            # Pre-allocate data array
            for j, key in enumerate(self.variable):
                self.data[key] = [None]*size
            
            # Read data 
            for i, line in enumerate(allLine):
                line = line.lstrip().rstrip().split()
                
                for j, key in enumerate(self.variable):
                    self.data[key][i] = float(line[j])
#                     self.data[key].append(float(line.lstrip().rstrip().split(" ")[j]))

                if self.zoneType.upper() == "ORDERED":
                    if i == self.i*self.j*self.k -1:
                        break
                else:
                    if i == self.numNode-1:
                        break

            if self.zoneType.upper() == "ORDERED": # Don't have connectivity data for "ordered" zones
                return  # We are done
            
#             print len(self.data["x"]), self.numNode, self.data["x"][0], self.data["x"][-1]
            
            # Remove all the lines we just read
            allLine = allLine[i+1:]
            
            # Pre-allocate connectivity
            self.connectivity = [None]*self.numElement
            
            # Read connectivity
            if (self.zoneType.upper() == "FELINESEG" or
                self.zoneType.upper() == "FETRIANGLE" or 
                self.zoneType.upper() == "FEQUADRILATERAL" or 
                self.zoneType.upper() == "FETETRAHEDRON" or 
                self.zoneType.upper() == "FEBRICK"):
                
                for i, line in enumerate(allLine):
                    connect = [None]*8
                
                    temp = line.lstrip().rstrip().replace("\n","").split()
                    length = 0
                    for j in temp:
                        
                        val = int(j)
                            
                        if not repeatConnect: # Should we allow for repeated connectivity indices?
                            
                            if val not in connect:
                                connect[length] = val
                                length += 1
                        else:
                            connect[length] = val
                            length += 1
                    
                    if self.zoneType.upper() == "FELINESEG" and length != 2:
                        self._checkConnectivity(length, line)
                        
                    elif self.zoneType.upper() == "FETRIANGLE" and length != 3:
                        self._checkConnectivity(length, line)
                        
                    elif self.zoneType.upper() == "FEQUADRILATERAL" and (length != 4 and length != 3):
                        self._checkConnectivity(length, line)
                        
                    elif self.zoneType.upper() == "FETETRAHEDRON" and length != 4:
                        self._checkConnectivity(length, line)
                        
                    elif self.zoneType.upper() == "FEBRICK" and length >= 8:
                        self._checkConnectivity(length, line)
                         
                    self.connectivity[i] = connect[0:length]
                
                    if i == self.numElement-1:
                        break
            else:
                print "Tecplot zone reader does not fully support", self.zoneType, "zones yet!"
                raise TypeError
                
        elif self.dataPacking.upper() == "BLOCK":
            print "Tecplot zone reader does not fully support ",  self.dataPacking, "packing yet!"
            raise TypeError
        
        else:
            print "Unsupport packing type ",  self.dataPacking, "packing yet!"
            raise TypeError
                
        return 
    
cdef class capsTecplot:
    
    cdef: 
        readonly object title 
        readonly object variable 
        
        readonly object zone 
        
        public object fileName 
    
    # \param fileName Name of the file that 
    def __cinit__(self, fileName=None):
      
        self.title = None
        self.variable = []
         
        # Dictionary con
        self.zone = {} 
         
        self.fileName = fileName

    # Read the header information for the files 
    def loadHeader(self):
        
        if not self.fileName: 
            print "No fileName as been specified"
            raise IOError
        
        with open(self.fileName, 'r') as fp:
            allLine = fp.readlines() # Read all the lines and up them in one big string
            
            # Find title
            for line in allLine:
                title= regSearch(r'Title\s*=\s*(.*)', line, regMultiLine | regIgnoreCase)
                if not title:
                    continue
                else:
                    self.title = title.group(1).replace('"', "").replace("'", "")
                    break
            
            if not title:
                print "No title found - defaulting to", self.title
                
            # Find variables 
            for line in allLine:
                variables = regSearch(r'Variables\s*=\s*(.*)', line, regMultiLine | regIgnoreCase)
                if not variables:
                    continue
                else:
                    break
            
            if not variables:
                print "No variables found!"
                raise ValueError
            else:
                variables = variables.group(1)
                
            if "," in variables:
                variables = variables.split(",")
            elif " " in variables:
                variables = variables.split(" ")
            else:
                print "Unable to parse variable list - ", variables
                raise TypeError
            
            for i in variables:
                if i: # Deal with empty characters in list
                    self.variable.append(i.replace('"', "").replace("'", "").replace(" ", "").replace("\n", ""))
            
            # Figure out how many zones there are 
            for line in allLine:
                
                # Look for Zone line 
                zone = regSearch(r"Zone", line, regMultiLine | regIgnoreCase) 
                if zone:
                    
                    # Get title
                    zone = regSearch(r"\s*T\s*=\s*(\".*\")", line, regMultiLine | regIgnoreCase) # First look for "
                    if not zone:
                        
                        zone = regSearch(r"\s*T\s*=\s*(\'.*\')", line, regMultiLine | regIgnoreCase) # Next try '
                        if not zone:
                            
                            title = "Zone_" + str(len(self.zone.keys()))
                            
                            print "No zone title found!, Defaulting to", title
                        
                        else:
                            title = zone.group(1).replace('"', "").replace("'", "")
                    else:
                        title = zone.group(1).replace('"', "").replace("'", "")
                  
                else:
                    continue
            
                # Setup zone dictionary 
                self.zone[title] = capsTecplotZone(title, self.variable, self.fileName) # Set the zone name and variables in the zone
            
            if not self.zone:
                print "No zones found!"
                raise ValueError
            
    # Load all zones in the file  
    def loadZone(self):
        
        if not self.zone:
            self.loadHeader()
        
        for i in self.zone.keys():
            self.zone[i].readZoneHeader()
            self.zone[i]._readZoneData()


cdef class capsUnstructMesh:
    
    cdef:
        
        object filename
                
        public name 
        
        readonly numElement
        readonly numNode
       
        readonly variable
        readonly xyz 
        readonly object data
        readonly object connectivity
        readonly object oneBias
        
        readonly object meshType
    
    def __cinit__(self, filename):
        
        self.name = "UnstructuredMesh"
        self.numElement = 0
        self.numNode = 0
        
        self.xyz = []
        self.connectivity = []

        self.variable = []
        self.data = {}
        
        self.filename = filename
        
        self.meshType = "unknown"
        
        self.oneBias = True
        
    def loadFile(self):
        
        if ".origami" in self.filename:
            self._loadOrigami()
        elif ".ugrid" in self.filename:
            self._loadAFLR3()
        elif ".stl" in self.filename:
            self._loadSTL()
        else:
            print "Unable to determine mesh format!" 
            raise ValueError
    
    def _printLoading(self):
        print "Loading ( type =", self.meshType, ") file", self.filename
        
    def _parseFileName(self):
        self.name = os.path.basename(self.filename)
        
    def _loadAFLR3(self):
        
        sint = 4 # bytes
        sdouble = 8 # bytes 
        
        self._parseFileName()
        
        self.meshType = "aflr3"
                
        self.oneBias = True
        
        self.variable = ["BCID"]
        self.data["BCID"] = []
        
        numTriangle = 0
        numQuadrilateral = 0
        numTetrahedral = 0
        numPyramid = 0
        numPrism = 0
        numHexahedral = 0
        
        self._printLoading()
        with open(self.filename, 'r') as fp:
            
            # Little endian  or big endian 
            if "lb8" in self.name or "b8" in self.name:
            
                self.numNode     = structUnpack("i", fp.read(sint))[0]
                
                numTriangle      = structUnpack("i", fp.read(sint))[0]
                numQuadrilateral = structUnpack("i", fp.read(sint))[0]
                numTetrahedral   = structUnpack("i", fp.read(sint))[0]
                numPyramid       = structUnpack("i", fp.read(sint))[0]
                numPrism         = structUnpack("i", fp.read(sint))[0]
                numHexahedral    = structUnpack("i", fp.read(sint))[0]
                
                self.numElement = numTriangle + \
                                  numQuadrilateral + \
                                  numTetrahedral + \
                                  numPyramid + \
                                  numPrism + \
                                  numHexahedral
                
                #print self.numNode, numTriangle, numQuadrilateral, numTetrahedral, numPyramid, numPrism, numHexahedral
                
                # Nodes 
                for i in range(self.numNode):
                    self.xyz.append( [ structUnpack("d", fp.read(sdouble))[0], 
                                       structUnpack("d", fp.read(sdouble))[0], 
                                       structUnpack("d", fp.read(sdouble))[0] ])
                
                # Triangles 
                for i in range(numTriangle):
                    
                    self.connectivity.append([ structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0]])
                    
                # Quadrilaterals 
                for i in range(numQuadrilateral):
                    
                    self.connectivity.append([ structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0]])
                
                # Boundary ids
                self.data["BCID"] = [0 for x in range(numTriangle + numQuadrilateral)]
                
                for i in range(numTriangle + numQuadrilateral):
                    self.data["BCID"][i] = structUnpack("i", fp.read(sint))[0]
                
                # Tetrahedrals
                for i in range(numTetrahedral):
                    
                    self.connectivity.append([ structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0]])
                    
                # Pyramids
                for i in range(numPyramid):
                    
                    self.connectivity.append([ structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0]])
                    
                # Prisms
                for i in range(numPrism):
                    
                    self.connectivity.append([ structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0],
                                               structUnpack("i", fp.read(sint))[0]])
                # Hexahedrals
                for i in range(numHexahedral):
                    
                    self.connectivity.append([ structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0], 
                                               structUnpack("i", fp.read(sint))[0],
                                               structUnpack("i", fp.read(sint))[0],
                                               structUnpack("i", fp.read(sint))[0],
                                               structUnpack("i", fp.read(sint))[0]])
            # ASCII 
            elif ".ugrid" in self.filename:
                
                line = fp.readline()
                if "," in line:
                    temp = line.split(",")
                else:
                    temp = line.split(" ")
     
                self.numNode     = int(temp[0]) 
                
                numTriangle      = int(temp[1]) 
                numQuadrilateral = int(temp[2]) 
                numTetrahedral   = int(temp[3]) 
                numPyramid       = int(temp[4]) 
                numPrism         = int(temp[5]) 
                numHexahedral    = int(temp[6]) 
                
                self.numElement = numTriangle + \
                                  numQuadrilateral + \
                                  numTetrahedral + \
                                  numPyramid + \
                                  numPrism + \
                                  numHexahedral
                
                # Nodes 
                for i in range(self.numNode):
                    temp = fp.readline().split(" ")
                    
                    self.xyz.append( [ float(temp[0]), 
                                       float(temp[1]), 
                                       float(temp[2]) ])
                    
                # Triangles 
                for i in range(numTriangle):
                    temp = fp.readline().split(" ")
                    
                    self.connectivity.append([ int(temp[0]), 
                                               int(temp[1]), 
                                               int(temp[2])])
                    
                # Quadrilaterals 
                for i in range(numQuadrilateral):
                    temp = fp.readline().split(" ")
                    
                    self.connectivity.append([ int(temp[0]), 
                                               int(temp[1]), 
                                               int(temp[2]),
                                               int(temp[3])])
                
                # Boundary ids
                self.data["BCID"] = [0 for x in range(numTriangle + numQuadrilateral)]
                
                for i in range(numTriangle + numQuadrilateral):
                    temp = fp.readline().split(" ")
                    
                    self.data["BCID"][i] = int(temp[0])
                    
                # Tetrahedrals
                for i in range(numTetrahedral):
                    temp = fp.readline().split(" ")
                     
                    self.connectivity.append([ int(temp[0]), 
                                               int(temp[1]), 
                                               int(temp[2]),
                                               int(temp[3])])
                    
                # Pyramids
                for i in range(numPyramid):
                    temp = fp.readline().split(" ")
                    
                    self.connectivity.append([ int(temp[0]), 
                                               int(temp[1]), 
                                               int(temp[2]),
                                               int(temp[3]),
                                               int(temp[4])])
                    
                # Prisms
                for i in range(numPrism):
                    temp = fp.readline().split(" ")
                    
                    self.connectivity.append([ int(temp[0]), 
                                               int(temp[1]), 
                                               int(temp[2]),
                                               int(temp[3]),
                                               int(temp[4]),
                                               int(temp[5])])
                # Hexahedrals
                for i in range(numHexahedral):
                    temp = fp.readline().split(" ")
                    
                    self.connectivity.append([ int(temp[0]), 
                                               int(temp[1]), 
                                               int(temp[2]),
                                               int(temp[3]),
                                               int(temp[4]),
                                               int(temp[5]),
                                               int(temp[6]),
                                               int(temp[7])])
            else:
                raise TypeError
        
    def _loadOrigami(self):
        
        self._parseFileName()
        
        self.meshType = "origami"
        
        self.oneBias = True
        
        nodeCount = 0
        elementCount = 0
        
        self._printLoading()
        with open(self.filename, 'r') as fp:
            
            for line in fp:
                
                if line.startswith("#"):
                    continue
                    
                if self.numNode == 0 and self.numElement == 0:
                    
                    self.numNode = int(line.split(" ")[0])
                    self.numElement = int(line.split(" ")[1])
                    
                    self.xyz = [[] for x in range(self.numNode)]
                    self.connectivity = [[] for x in range(self.numElement)]
                    
                    self.variable = ["ClosetNeighbor_1", "ClosetNeighbor_2", "FoldLine"]
                    
                    self.data["ClosetNeighbor_1"] = [0 for x in range(self.numElement)]
                    self.data["ClosetNeighbor_2"] = [0 for x in range(self.numElement)]
                    self.data["FoldLine"] = [0 for x in range(self.numElement)]
                    
                    continue
                    
                if nodeCount < self.numNode:
                    xyz = line.split(" ")
                    
                    index = int(xyz[0])-1
                    
                    self.xyz[index].append(float(xyz[1]))
                    self.xyz[index].append(float(xyz[2]))
                    self.xyz[index].append(float(xyz[3]))
                    
                    nodeCount += 1
                    
                    continue
                
                if elementCount < self.numElement:
                    
                    element = line.split(" ")
                    
                    index = int(element[0])-1
                    
                    self.connectivity[index].append(int(element[1]))
                    self.connectivity[index].append(int(element[2]))
                    
                    self.data["ClosetNeighbor_1"][index] = int(element[3])
                    self.data["ClosetNeighbor_2"][index] = int(element[4])
                    self.data["FoldLine"][index]         = int(element[5])
                    
                    elementCount += 1
                    
                    continue
                
    def _loadSTL(self):
        
        def sortNode(node, connect):
            # Does the vertex already exist?           
            try:
                index = self.xyz.index(node)
                connect.append(index+1) # Yes, get the index for the connectivity
                            
            except ValueError: # No! Append it 
                self.xyz.append(node)
                
                self.numNode += 1
                connect.append(self.numNode)
            except:
                raise
            
        sint  = 4 # bytes
        schar = 1 # bytes
        sfloat = 4 # bytes 
        sshort = 2 # bytes
                    
        self._parseFileName()
        
        self.meshType = "stl"
        
        self.oneBias = True
        
        nodeCount = 0
        elementCount = 0
        
        self._printLoading()
        
        ascii = True
        
        with open(self.filename, 'r') as fp:
            
            # ASCII or binary
            line = fp.read(schar*80)
            
            if "solid" in line: # ASCII
                
                print "'solid' tag found! Assuming STL is in ASCII format...."
                
                fp.seek(0) # Return to the beginning and get the whole line
                
                line = fp.readline()
                
                line = line.split()
                if len(line) > 1:
                    self.name = str(line[1])
                else:
                    self.name = self.filename
             
            else: # Binary
                    
                print "No 'solid' tag found! Assuming STL is in binary format...."
            
                ascii = False
                    
                fp.seek(0)
                
                self.name = str(fp.read(schar*80)) #structUnpack("s", fp.read(schar*80))[0]
                    
                self.numElement = structUnpack("I", fp.read(sint))[0]
            
            if ascii: # STL appears to be ASCII       

                for line in fp:
                    if "outer loop" in line:
                        
                        connect = []
                        for line in fp:
                            
                            if "vertex" in line:
                                data = line.split()
                                # Read vertex
                                node = [float(data[1]), 
                                        float(data[2]),
                                        float(data[3]) ]
                            
                                # Determine if node is new or not 
                                sortNode(node, connect)
                                
                            if "endloop" in line:
                                self.connectivity.append(connect)
                                self.numElement += 1
                                
                                break 
                    
                    if "endsolid" in line:
                        break
                    
            else: # Binary 
            
                for i in range(self.numElement):
                    
                    # Read the normals
                    fp.read(sfloat)
                    fp.read(sfloat)
                    fp.read(sfloat)
                    
                    connect = []
                    for j in range(3):
                        
                        # Read vertex
                        node = [structUnpack("f", fp.read(sfloat))[0], 
                                structUnpack("f", fp.read(sfloat))[0], 
                                structUnpack("f", fp.read(sfloat))[0]]
                        
                        # Determine if node is new or not 
                        sortNode(node, connect)
                    
                    # Add element to connectivity 
                    self.connectivity.append(connect)
                         
                    # Read attribute bytes - currently not doing anything with it - could add it to the data 
                    attrByte = structUnpack("H", fp.read(sshort))[0]
                    if attrByte != 0:
                        fp.read(attrByte)
                    
                     
       # print self.numNode, self.numElement 
        
        if self.numNode == 0 or self.numElement == 0:
            print "No nodes or elements created.... something went wrong"
            raise ValueError
                          