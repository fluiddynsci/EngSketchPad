#	Makefile for udfTile
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

IRITDEF = -DLINUX386 -D__UNIX__ -DSTRICMP -DUSLEEP -DTIMES -DRAND -DIRIT_HAVE_XML_LIB

IRITLIB = -lIritExtLib  -lIritGrapLib -lIritUserLib -lIritRndrLib \
          -lIritBoolLib -lIritPrsrLib -lIritVMdlLib -lIritMdlLib \
          -lIritMvarLib -lIritTrimLib -lIritTrivLib -lIritTrngLib \
          -lIritSymbLib -lIritCagdLib -lIritGeomLib -lIritMiscLib \
          -lIritXtraLib

$(LDIR)/tile.so:	$(ODIR)/udfTile.o
	touch $(LDIR)/tile.so
	rm $(LDIR)/tile.so
	$(CC) $(SOFLGS) -o $(LDIR)/tile.so $(ODIR)/udfTile.o -L$(LDIR) -legads -locsm -L$(IRIT_LIB) $(IRITLIB) $(IRITLIB) $(RPATH) -lm

$(ODIR)/udfTile.o:	udfTile.c udpUtilities.c udpUtilities.h OpenCSM.h
	$(CC) -c $(COPTS) $(DEFINE) $(IRITDEF) -I$(IDIR) -I. -I$(IRIT_INC) udfTile.c -o $(ODIR)/udfTile.o

clean:
	(cd $(ODIR); rm -f udfTile.o )

cleanall:	clean
	(cd $(LDIR); rm -f tile.so )
