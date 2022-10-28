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

OBJS =	$(ODIR)/platoAIM.o

$(LDIR)/platoAIM.so:	$(OBJS)  $(LDIR)/libaimUtil.a
	$(CC) $(SOFLGS) -o $(LDIR)/platoAIM.so $(OBJS) \
		-L$(LDIR) -laimUtil -locsm -legads -ludunits2 -ldl -lm

$(ODIR)/platoAIM.o:	platoAIM.c $(IDIR)/capsTypes.h \
			$(IDIR)/aimUtil.h  $(IDIR)/aimMesh.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../meshWriter/exodusWriter -I../utils platoAIM.c \
		-o $(ODIR)/platoAIM.o

lint:
	$(LINT) -I../../include -I$(IDIR) -I../utils -I../meshWriter/exodusWriter $(wildcard *.c) -exportlocal -uniondef -nestcomment -boolops

clean:
	-rm $(OBJS)

cleanall:	clean
	-rm $(LDIR)/platoAIM.so

dox: doc/html/index.html
doc/html/index.html: \
				$(wildcard *.c) $(wildcard ../utils/*.c) $(wildcard doc/*.cfg) $(wildcard doc/*.tex) $(wildcard doc/*.xml) $(wildcard doc/*.dox)
	(cd doc; doxygen platoAIM_dox.cfg; cd latex; make; mv refman.pdf platoAIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f platoAIM.tag)
