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


$(LDIR)/nastranAIM.so:	$(ODIR)/nastranAIM.o $(LDIR)/libutils.a $(LDIR)/libaimUtil.a
	$(CC) $(SOFLGS) -o $(LDIR)/nastranAIM.so $(ODIR)/nastranAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm $(CPPSLB)

$(ODIR)/nastranAIM.o:	nastranAIM.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils nastranAIM.c \
		-o $(ODIR)/nastranAIM.o

clean:
	-rm $(ODIR)/nastranAIM.o

cleanall:	clean
	-rm $(LDIR)/nastranAIM.so
	
dox:
	(cd doc; doxygen nastranAIM_dox.cfg; cd latex; make; mv refman.pdf NastranAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f nastranAIM.tag)
