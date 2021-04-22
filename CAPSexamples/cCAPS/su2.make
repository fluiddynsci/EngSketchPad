#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
TDIR  = $(ESP_BLOC)/examples/cCAPS
else
ODIR  = .
TDIR  = .
endif

$(TDIR)/su2Test:	$(ODIR)/su2Test.o $(LDIR)/$(CSHLIB)
	$(CC) -o $(TDIR)/su2Test $(ODIR)/su2Test.o \
		-L$(LDIR) -lcaps -locsm -legads -ludunits2 $(RPATH) -lm -ldl

# $(TDIR)/fun3dTest:  $(ODIR)/fun3dTest.o $(LDIR)/$(CSHLIB)
# 	$(CC) -o $(TDIR)/fun3dTest $(ODIR)/fun3dTest.o \
# 	-L$(LDIR) -lcaps -legads $(RPATH) -lm -ldl
# 	
# $(TDIR)/fun3dAFLR4Test:  $(ODIR)/fun3dAFLR4Test.o $(LDIR)/$(CSHLIB)
# 	$(CC) -o $(TDIR)/fun3dAFLR4Test $(ODIR)/fun3dAFLR4Test.o \
# 	-L$(LDIR) -lcaps -legads $(RPATH) -lm -ldl
# 
# $(TDIR)/fun3dTetgenTest:  $(ODIR)/fun3dTetgenTest.o $(LDIR)/$(CSHLIB)
# 	$(CC) -o $(TDIR)/fun3dTetgenTest $(ODIR)/fun3dTetgenTest.o \
# 	-L$(LDIR) -lcaps -legads $(RPATH) -lm -ldl
# 
# $(TDIR)/aeroelasticTest:  $(ODIR)/aeroelasticTest.o $(LDIR)/$(CSHLIB)
# 	$(CC) -o $(TDIR)/aeroelasticTest $(ODIR)/aeroelasticTest.o \
# 	-L$(LDIR) -lcaps -legads $(RPATH) -lm -ldl
# 	
# $(TDIR)/aeroelasticIterativeTest:  $(ODIR)/aeroelasticIterativeTest.o $(LDIR)/$(CSHLIB)
# 	$(CC) -o $(TDIR)/aeroelasticIterativeTest $(ODIR)/aeroelasticIterativeTest.o \
# 	-L$(LDIR) -lcaps -legads $(RPATH) -lm -ldl


$(ODIR)/su2Test.o:    su2Test.c $(IDIR)/caps.h | $(ODIR)
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) su2Test.c -o $(ODIR)/su2Test.o

# $(ODIR)/fun3dAFLR4Test.o:    fun3dAFLR4Test.c $(IDIR)/caps.h | $(ODIR)
# 	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) fun3dAFLR4Test.c -o $(ODIR)/fun3dAFLR4Test.o
# 
# $(ODIR)/fun3dTetgenTest.o:    fun3dTetgenTest.c $(IDIR)/caps.h | $(ODIR)
# 	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) fun3dTetgenTest.c -o $(ODIR)/fun3dTetgenTest.o
# 	
# $(ODIR)/aeroelasticTest.o:    aeroelasticTest.c $(IDIR)/caps.h | $(ODIR)
# 	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) aeroelasticTest.c -o $(ODIR)/aeroelasticTest.o
# 	
# $(ODIR)/aeroelasticIterativeTest.o:    aeroelasticIterativeTest.c $(IDIR)/caps.h | $(ODIR)
# 	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) aeroelasticIterativeTest.c -o $(ODIR)/aeroelasticIterativeTest.o		

ifdef ESP_BLOC
$(ODIR):
	mkdir -p $@

$(TDIR):
	mkdir -p $@
endif

clean:
#	-rm -f $(ODIR)/fun3dTest.o $(ODIR)/fun3dAFLR4Test.o $(ODIR)/aeroelasticTest.o $(ODIR)/aeroelasticIterativeTest.o
	-rm -f $(ODIR)/su2Test.o 

cleanall:	clean
#	-rm -f $(TDIR)/fun3dTest $(TDIR)/fun3dAFLR4Test $(TDIR)/aeroelasticTest  $(TDIR)/aeroelasticIterativeTest
	-rm -f $(TDIR)/su2Test 
