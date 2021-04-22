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
else
ODIR  = .
endif


$(LDIR)/egadsTessAIM.so:	$(ODIR)/egadsTessAIM.o $(LDIR)/libutils.a $(LDIR)/libaimUtil.a
	touch $(LDIR)/egadsTessAIM.so
	rm $(LDIR)/egadsTessAIM.so
	$(CC) $(SOFLGS) -o $(LDIR)/egadsTessAIM.so $(ODIR)/egadsTessAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 $(RPATH) -lm

$(ODIR)/egadsTessAIM.o:	egadsTessAIM.c $(IDIR)/capsTypes.h \
			../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils egadsTessAIM.c \
		-o $(ODIR)/egadsTessAIM.o

clean:
	-rm $(ODIR)/egadsTessAIM.o

cleanall:	clean
	-rm $(LDIR)/egadsTessAIM.so

	
dox:
	(cd doc; doxygen egadsTessAIM_dox.cfg; cd latex; make; mv refman.pdf egadsTessAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f egadsTessAIM.tag)
