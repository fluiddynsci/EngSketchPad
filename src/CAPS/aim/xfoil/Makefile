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


$(LDIR)/xfoilAIM.so:	$(ODIR)/xfoilAIM.o $(LDIR)/libaimUtil.a \
			$(LDIR)/libutils.a
	$(CC) $(SOFLGS) -o $(LDIR)/xfoilAIM.so $(ODIR)/xfoilAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm

$(ODIR)/xfoilAIM.o: xfoilAIM.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils \
		xfoilAIM.c -o $(ODIR)/xfoilAIM.o

clean:
	-rm $(ODIR)/xfoilAIM.o

cleanall:	clean
	-rm $(LDIR)/xfoilAIM.so

dox:
	(cd doc; doxygen xfoilAIM_dox.cfg; cd latex; make; mv refman.pdf XFOILAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f xfoilAIM.tag)
