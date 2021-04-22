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

 
$(LDIR)/delaundoAIM.so:	$(ODIR)/delaundoAIM.o $(LDIR)/libutils.a $(LDIR)/libaimUtil.a
	touch $(LDIR)/delaundoAIM.so
	rm $(LDIR)/delaundoAIM.so
	$(CC) $(SOFLGS) -o $(LDIR)/delaundoAIM.so $(ODIR)/delaundoAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 $(RPATH) -lm

$(ODIR)/delaundoAIM.o:	delaundoAIM.c $(IDIR)/capsTypes.h \
			../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils delaundoAIM.c \
		-o $(ODIR)/delaundoAIM.o

clean:
	-rm $(ODIR)/delaundoAIM.o

cleanall:	clean
	-rm $(LDIR)/delaundoAIM.so

	
dox:
	(cd doc; doxygen delaundoAIM_dox.cfg; cd latex; make; mv refman.pdf delaundoAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f delaundoAIM.tag)
