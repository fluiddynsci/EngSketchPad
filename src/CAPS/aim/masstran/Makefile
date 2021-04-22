#
ifndef ESP_ROOT
$(error ESP_ROOT must be set -- Please fix the environment...)
endif
#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
BDIR  = $(ESP_ROOT)/bin
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
TDIR  = $(ESP_BLOC)/test
else
ODIR  = .
TDIR  = $(ESP_ROOT)/bin
endif

default: $(LDIR)/masstranAIM.so $(BDIR)/Masstran

$(LDIR)/masstranAIM.so:	$(ODIR)/masstranAIM.o $(LDIR)/libutils.a \
		$(LDIR)/libaimUtil.a
	$(CC) $(SOFLGS) -o $(LDIR)/masstranAIM.so $(ODIR)/masstranAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm

$(ODIR)/masstranAIM.o:	masstranAIM.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils masstranAIM.c \
		-o $(ODIR)/masstranAIM.o

$(BDIR)/Masstran: Masstran.c
	$(CC) $(COPTS) Masstran.c -o $(BDIR)/Masstran -lm

clean:
	-rm -f $(ODIR)/masstranAIM.o 

cleanall:	clean
	-rm -f $(LDIR)/masstranAIM.so $(BDIR)/Masstran

dox:
	(cd doc; doxygen masstranAIM_dox.cfg; cd latex; make; mv refman.pdf MasstranAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f masstranAIM.tag)
