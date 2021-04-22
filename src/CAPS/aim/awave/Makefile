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

ifdef FCOMP
default:	$(LDIR)/awaveAIM.so $(TDIR)/awave
else
default:	$(LDIR)/awaveAIM.so
endif


$(LDIR)/awaveAIM.so:	$(ODIR)/awaveAIM.o $(LDIR)/libutils.a $(LDIR)/libaimUtil.a
	$(CC) $(SOFLGS) -o $(LDIR)/awaveAIM.so $(ODIR)/awaveAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm

$(ODIR)/awaveAIM.o:	awaveAIM.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils awaveAIM.c \
		-o $(ODIR)/awaveAIM.o

$(TDIR)/awave:	$(ODIR)/awave.o
	$(FCOMP) -o $(TDIR)/awave $(ODIR)/awave.o

$(ODIR)/awave.o:	$(AWAVE)
	$(FCOMP) -c $(FOPTS) $(AWAVE) -o $(ODIR)/awave.o

clean:
	-rm -f $(ODIR)/awave.o $(ODIR)/awaveAIM.o

cleanall:	clean
	-rm -f $(LDIR)/awaveAIM.so $(TDIR)/awave

dox:
	(cd doc; doxygen awaveAIM_dox.cfg; cd latex; make; mv refman.pdf AWAVEAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f awaveAIM.tag)	
