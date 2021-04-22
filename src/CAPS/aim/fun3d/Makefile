#
ifndef ESP_ROOT
$(error ESP_ROOT must be set -- Please fix the environment...)
endif
#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
TDIR  = $(ESP_BLOC)/test
else
ODIR  = .
TDIR  = $(ESP_ROOT)/bin
endif


ifdef PYTHONINC

#Adding the rpath is needed when compiling with ESP_Python
PYTHONLPATH=$(filter -L%,$(PYTHONLIB))
PYTHONRPATH=$(PYTHONLPATH:-L%=-Wl,-rpath %)

$(LDIR)/fun3dAIM.so:	$(ODIR)/fun3dAIM.o $(ODIR)/fun3dUtils.o $(ODIR)/fun3dNamelist.o \
			$(LDIR)/libaimUtil.a $(LDIR)/libutils.a
	echo "Building FUN3D with Python"
	$(CC) $(SOFLGS) -o $(LDIR)/fun3dAIM.so $(ODIR)/fun3dAIM.o $(ODIR)/fun3dUtils.o \
		$(ODIR)/fun3dNamelist.o -L$(LDIR) -lutils -laimUtil -locsm \
		-legads -ludunits2 $(PYTHONLIB) -lm $(PYTHONRPATH)
else 
$(LDIR)/fun3dAIM.so:	$(ODIR)/fun3dAIM.o $(ODIR)/fun3dUtils.o $(LDIR)/libaimUtil.a \
			$(LDIR)/libutils.a
	echo "Building FUN3D without Python"
	$(CC) $(SOFLGS) -o $(LDIR)/fun3dAIM.so $(ODIR)/fun3dAIM.o $(ODIR)/fun3dUtils.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm
endif 

ifdef PYTHONINC

$(ODIR)/fun3dAIM.o: fun3dAIM.c fun3dUtils.h $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -DHAVE_PYTHON -I$(IDIR) -I../utils \
		-I$(PYTHONINC) fun3dAIM.c -o $(ODIR)/fun3dAIM.o

else
$(ODIR)/fun3dAIM.o: fun3dAIM.c fun3dUtils.h $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils \
		fun3dAIM.c -o $(ODIR)/fun3dAIM.o
endif 

$(ODIR)/fun3dNamelist.o: fun3dNamelist.c fun3dNamelist.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils \
		-I$(PYTHONINC) fun3dNamelist.c -o $(ODIR)/fun3dNamelist.o

$(ODIR)/fun3dUtils.o: fun3dUtils.c fun3dUtils.h $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils \
		fun3dUtils.c -o $(ODIR)/fun3dUtils.o
		
clean:
	-rm $(ODIR)/fun3dAIM.o $(ODIR)/fun3dUtils.o
ifdef PYTHONINC
	-rm $(ODIR)/fun3dNamelist.o
endif

cleanall:	clean
	-rm $(LDIR)/fun3dAIM.so

# Run Cython - development only	
cython:
	(cython fun3dNamelist.pyx -I../../pyCAPS/src/ --capi-reexport-cincludes) 

dox:
	(cd doc; doxygen fun3dAIM_dox.cfg; cd latex; make; mv refman.pdf FUN3DAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f fun3dAIM.tag)
