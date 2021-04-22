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
default:	$(LDIR)/frictionAIM.so $(TDIR)/friction
else
default:	$(LDIR)/frictionAIM.so
endif


$(LDIR)/frictionAIM.so:	$(ODIR)/frictionAIM.o $(LDIR)/libutils.a  $(LDIR)/libaimUtil.a
	$(CC) $(SOFLGS) -o $(LDIR)/frictionAIM.so $(ODIR)/frictionAIM.o \
		-L$(LDIR) -lutils -laimUtil -locsm -legads -ludunits2 -lm

$(ODIR)/frictionAIM.o:	frictionAIM.c $(IDIR)/capsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -Wno-format -I$(IDIR) -I../utils frictionAIM.c \
		-o $(ODIR)/frictionAIM.o

$(TDIR)/friction:	$(ODIR)/friction.o
	$(FCOMP) -o $(TDIR)/friction $(ODIR)/friction.o

$(ODIR)/friction.o:	friction_eja_mod.f
	$(FCOMP) -c $(FOPTS) friction_eja_mod.f -o $(ODIR)/friction.o

clean:
	-rm $(ODIR)/frictionAIM.o $(ODIR)/friction.o

cleanall:	clean
	-rm $(LDIR)/frictionAIM.so $(TDIR)/friction

dox:
	(cd doc; doxygen frictionAIM_dox.cfg; cd latex; make; mv refman.pdf FRICTIONAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f frictionAIM.tag)	
