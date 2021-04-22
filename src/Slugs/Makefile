#	Makefile for Slugs
#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
BDIR  = $(ESP_BLOC)/bin
else
ODIR  = .
BDIR  = $(ESP_ROOT)/bin
endif

all:	$(BDIR)/Slugs $(BDIR)/FitTest

$(BDIR)/Slugs:	$(ODIR)/Slugs.o $(ODIR)/Fitter.o $(ODIR)/RedBlackTree.o $(ODIR)/Tessellate.o
	$(CXX) -o $(BDIR)/Slugs $(ODIR)/Slugs.o $(ODIR)/Fitter.o $(ODIR)/RedBlackTree.o \
		$(ODIR)/Tessellate.o $(RPATH) -L$(LDIR) -lwsserver -legads \
		-lpthread -lz -lm

$(ODIR)/Slugs.o:	Slugs.c Fitter.h RedBlackTree.h Tessellate.h $(IDIR)/common.h \
			$(IDIR)/egads.h $(IDIR)/egadsTypes.h \
			$(IDIR)/egadsErrors.h $(IDIR)/wsserver.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. Slugs.c -o $(ODIR)/Slugs.o

$(BDIR)/FitTest:	$(ODIR)/FitTest.o $(ODIR)/Fitter.o 
	$(CXX) -o $(BDIR)/FitTest $(ODIR)/FitTest.o $(ODIR)/Fitter.o -L$(LDIR) -legads \
		-lpthread -lz -lm

$(ODIR)/FitTest.o:	FitTest.c Fitter.h $(IDIR)/common.h \
			$(IDIR)/egads.h $(IDIR)/egadsTypes.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. FitTest.c -o $(ODIR)/FitTest.o

$(ODIR)/Fitter.o:	Fitter.c Fitter.h $(IDIR)/common.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. Fitter.c \
		-o $(ODIR)/Fitter.o

$(ODIR)/RedBlackTree.o:	RedBlackTree.c RedBlackTree.h $(IDIR)/common.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. RedBlackTree.c \
		-o $(ODIR)/RedBlackTree.o

$(ODIR)/Tessellate.o:	Tessellate.c Tessellate.h RedBlackTree.h \
			$(IDIR)/common.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. Tessellate.c \
		-o $(ODIR)/Tessellate.o

clean:
	-rm $(ODIR)/Slugs.o $(ODIR)/FitTest.o $(ODIR)/RedBlackTree.o $(ODIR)/Tessellate.o \
		$(ODIR)/Fitter.o

cleanall:	clean
	-rm $(BDIR)/Slugs $(BDIR)/FitTest
