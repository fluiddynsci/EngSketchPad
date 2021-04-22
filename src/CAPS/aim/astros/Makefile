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


$(LDIR)/astrosAIM.so:	$(ODIR)/astrosAIM.o $(ODIR)/astrosUtils.o $(LDIR)/libutils.a $(LDIR)/libaimUtil.a
	$(CC) $(SOFLGS) -o $(LDIR)/astrosAIM.so $(ODIR)/astrosAIM.o  $(ODIR)/astrosUtils.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm $(CPPSLB) 

$(ODIR)/astrosAIM.o:	astrosAIM.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils astrosAIM.c \
		-o $(ODIR)/astrosAIM.o
		
$(ODIR)/astrosUtils.o:	astrosUtils.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils astrosUtils.c \
		-o $(ODIR)/astrosUtils.o

clean:
	-rm $(ODIR)/astrosAIM.o $(ODIR)/astrosUtils.o

cleanall:	clean
	-rm $(LDIR)/astrosAIM.so
	
dox:
	(cd doc; doxygen astrosAIM_dox.cfg; cd latex; make; mv refman.pdf AstrosAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f astrosAIM.tag)
