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


$(LDIR)/tsfoilAIM.so: $(ODIR)/tsfoilAIM.o $(LDIR)/libaimUtil.a \
			$(LDIR)/libutils.a
	$(CC) $(SOFLGS) -o $(LDIR)/tsfoilAIM.so $(ODIR)/tsfoilAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm

$(ODIR)/tsfoilAIM.o: tsfoilAIM.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils \
		tsfoilAIM.c -o $(ODIR)/tsfoilAIM.o

clean:
	-rm $(ODIR)/tsfoilAIM.o

cleanall:	clean
	-rm $(LDIR)/tsfoilAIM.so

dox:
	(cd doc; doxygen tsfoilAIM_dox.cfg; cd latex; make; mv refman.pdf TSFOILAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f tsfoilAIM.tag)
