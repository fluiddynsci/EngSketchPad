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

$(TDIR)/frictionTest:	$(ODIR)/frictionTest.o $(LDIR)/$(CSHLIB) | $(TDIR)
	$(CC) -o $(TDIR)/frictionTest $(ODIR)/frictionTest.o -L$(LDIR) -lcaps -locsm -legads \
		-ludunits2 $(RPATH) -lm -ldl

$(ODIR)/frictionTest.o:	frictionTest.c $(IDIR)/caps.h | $(ODIR)
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) frictionTest.c -o $(ODIR)/frictionTest.o

ifdef ESP_BLOC
$(ODIR):
	mkdir -p $@

$(TDIR):
	mkdir -p $@
endif

clean:
	-rm -f $(ODIR)/frictionTest.o

cleanall:	clean
	-rm -f $(TDIR)/frictionTest
