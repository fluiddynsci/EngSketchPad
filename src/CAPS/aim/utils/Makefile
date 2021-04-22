#
ifndef ESP_ARCH
$(error ESP_ARCH must be set -- Please fix the environment...)
endif
#
#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
BDIR  = $(ESP_ROOT)/bin
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
else
ODIR  = .
endif

VPATH = $(ODIR)

OBJS  =	meshUtils.o cfdUtils.o miscUtils.o feaUtils.o vlmUtils.o nastranUtils.o tecplotUtils.o
OBJSP =	vlmSpanSpace.o

default: $(LDIR)/libutils.a $(IDIR)/meshTypes.h

$(LDIR)/libutils.a:	$(OBJS) $(OBJSP)
	touch $(LDIR)/libutils.a
	rm $(LDIR)/libutils.a
	(cd $(ODIR); ar -rs $(LDIR)/libutils.a $(OBJS) $(OBJSP) )

$(OBJS): %.o:	%.c 
	$(CC) -c $(COPTS) $(DEFINE) -I../include -I$(IDIR) -I. $< -o $(ODIR)/$@

$(OBJSP): %.o:	%.cpp
	$(CXX) -c $(CPPOPT) $(DEFINE) -I../include -I$(IDIR) -I. $< -o $(ODIR)/$@

$(IDIR)/meshTypes.h: meshTypes.h
	touch $(IDIR)/meshTypes.h
	rm $(IDIR)/meshTypes.h
	cp -p meshTypes.h $(IDIR)/meshTypes.h

clean:
	-(cd $(ODIR); rm $(OBJS) $(OBJSP))

cleanall:	clean
	-rm $(LDIR)/libutils.a
