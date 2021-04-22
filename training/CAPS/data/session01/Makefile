#	Makefile for c versions of pyCAPS scripts

#	IDIR     directory for include    files
#	LDIR     directory for library    files
#	ODIR     directory for object     files
#	TDIR     directory for executable files (actually the test directory)

IDIR = $(ESP_ROOT)/include

include $(IDIR)/$(ESP_ARCH)

LDIR = $(ESP_ROOT)/lib

ifdef ESP_BLOC
   ODIR = $(ESP_BLOC)/obj
   TDIR = $(ESP_BLOC)/test
else
   ODIR = .
   TDIR = $(ESP_ROOT)/bin
endif

# default target(s)
default:	$(TDIR)/template_avl

# template_avl
$(TDIR)/template_avl:	$(ODIR)/template_avl.o $(LDIR)/$(CSHLIB)
	$(CC) -o $(TDIR)/template_avl $(ODIR)/template_avl.o -L$(LDIR) -lcaps -legads $(RPATH) -lm -ldl

$(ODIR)/template_avl.o:	template_avl.c $(IDIR)/caps.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) template_avl.c -o $(ODIR)/template_avl.o

# clean .o file(s)
clean:
	-rm $(ODIR)/template_avl.o

# clean .o and executable files
cleanall:	clean
	-rm $(TDIR)/template_avl
