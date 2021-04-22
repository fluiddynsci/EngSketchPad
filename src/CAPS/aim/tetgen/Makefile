#
ifndef ESP_ROOT
$(error ESP_ROOT must be set -- Please fix the environment...)
endif
#
ifndef TETGEN
$(error TETGEN must be set -- Please fix the environment...)
endif
#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
else
ODIR  = .
endif


$(LDIR)/tetgenAIM.so:	$(ODIR)/tetgenAIM.o $(ODIR)/tetgen_Interface.o \
			$(ODIR)/tetgen.o $(ODIR)/predicates.o $(LDIR)/libutils.a $(LDIR)/libaimUtil.a
	$(CXX) $(SOFLGS) -o $(LDIR)/tetgenAIM.so $(ODIR)/tetgenAIM.o \
		$(ODIR)/tetgen_Interface.o $(ODIR)/tetgen.o \
		$(ODIR)/predicates.o -L$(LDIR) -lutils -laimUtil -locsm \
		-legads -ludunits2 $(RPATH) -lm

$(ODIR)/tetgenAIM.o:	tetgenAIM.cpp $(IDIR)/capsTypes.h ../utils/meshUtils.h
	$(CXX) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils tetgenAIM.cpp \
		-o $(ODIR)/tetgenAIM.o

$(ODIR)/tetgen_Interface.o:	tetgen_Interface.cpp ../utils/meshUtils.h
	$(CXX) -c $(COPTS) $(DEFINE) -I$(IDIR) -I$(TETGEN) -I../utils \
		-DTETLIBRARY tetgen_Interface.cpp -o $(ODIR)/tetgen_Interface.o

$(ODIR)/predicates.o:	$(TETGEN)/predicates.cxx
	$(CXX) -c $(COPTS) -O0 $(TETGEN)/predicates.cxx -o $(ODIR)/predicates.o

$(ODIR)/tetgen.o:	$(TETGEN)/tetgen.cxx
	$(CXX) -c $(COPTS) -DTETLIBRARY -w $(TETGEN)/tetgen.cxx -o $(ODIR)/tetgen.o

clean:
	-rm -f $(ODIR)/tetgen.o $(ODIR)/predicates.o $(ODIR)/tetgen_Interface.o \
		$(ODIR)/tetgenAIM.o

cleanall:	clean
	-rm -f $(LDIR)/tetgenAIM.so

dox:
	(cd doc; doxygen tetgenAIM_dox.cfg; cd latex; make; mv refman.pdf TetgenAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f tetgenAIM.tag)
