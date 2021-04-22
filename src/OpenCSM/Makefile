#       Makefile for OpenCSM
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

BINLIST =	$(BDIR)/serveCSM \
		$(BDIR)/sensCSM
LIBLIST =	$(LDIR)/$(OSHLIB) \
		$(LDIR)/bezier.so \
		$(LDIR)/biconvex.so \
		$(LDIR)/box.so \
		$(LDIR)/catmull.so \
		$(LDIR)/compare.so \
		$(LDIR)/createBEM.so \
		$(LDIR)/createPoly.so \
		$(LDIR)/csm.so \
		$(LDIR)/droop.so \
		$(LDIR)/editAttr.so \
		$(LDIR)/ellipse.so \
		$(LDIR)/fitcurve.so \
		$(LDIR)/freeform.so \
		$(LDIR)/flend.so \
		$(LDIR)/ganged.so \
		$(LDIR)/guide.so \
		$(LDIR)/matchBodys.so \
		$(LDIR)/hex.so \
		$(LDIR)/import.so \
		$(LDIR)/kulfan.so \
		$(LDIR)/naca.so \
		$(LDIR)/nurbbody.so \
		$(LDIR)/parsec.so \
		$(LDIR)/pod.so \
		$(LDIR)/poly.so \
		$(LDIR)/printBbox.so \
		$(LDIR)/printBrep.so \
		$(LDIR)/radwaf.so \
		$(LDIR)/sample.so \
		$(LDIR)/sew.so \
		$(LDIR)/stag.so \
		$(LDIR)/stiffener.so \
		$(LDIR)/supell.so \
		$(LDIR)/waffle.so \
		$(LDIR)/PDT2.so
LIBFLIST =	$(LDIR)/naca456.so
INCLIST =	$(IDIR)/OpenCSM.h \
		$(IDIR)/common.h \
		$(IDIR)/udp.h \
		$(IDIR)/egg.h \
		$(IDIR)/udpUtilities.h \
		$(IDIR)/udpUtilities.c

ifdef FCOMP
default:	$(BINLIST) $(LIBLIST) $(LIBFLIST) $(INCLIST)
else
default:	$(BINLIST) $(LIBLIST)             $(INCLIST)
endif

#
#	binaries
#
$(BDIR)/serveCSM:	$(ODIR)/serveCSM.o $(LDIR)/$(OSHLIB)
	$(CC) -o $(BDIR)/serveCSM $(ODIR)/serveCSM.o $(RPATH) -L$(LDIR) \
		-locsm -lwsserver -legads -lpthread -lz -lm

$(ODIR)/serveCSM.o:	serveCSM.c OpenCSM.h common.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. serveCSM.c \
		-o $(ODIR)/serveCSM.o

$(BDIR)/sensCSM:	$(ODIR)/sensCSM.o $(LDIR)/$(OSHLIB)
	$(CC) -o $(BDIR)/sensCSM $(ODIR)/sensCSM.o $(RPATH) -L$(LDIR) -locsm \
		-locsm -legads -lm

$(ODIR)/sensCSM.o:	sensCSM.c OpenCSM.h common.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. sensCSM.c -o $(ODIR)/sensCSM.o

#
#	library
#
$(LDIR)/libocsm.so:	$(ODIR)/OpenCSM.o $(ODIR)/udp.o
	touch $(LDIR)/libocsm.so
	rm $(LDIR)/libocsm.so
	$(CC) -shared -Wl,-no-undefined -o $(LDIR)/libocsm.so \
		$(ODIR)/OpenCSM.o $(ODIR)/udp.o -L$(LDIR) -legads -ldl -lm

$(LDIR)/libocsm.dylib:	$(ODIR)/OpenCSM.o $(ODIR)/udp.o
	touch $(LDIR)/libocsm.dylib
	rm $(LDIR)/libocsm.dylib
	$(CC) -dynamiclib -o $(LDIR)/libocsm.dylib $(ODIR)/OpenCSM.o \
		$(ODIR)/udp.o $(RPATH) -L$(LDIR) -legads -ldl \
		-undefined error -install_name '@rpath/libocsm.dylib' \
		-compatibility_version $(EGREV) -current_version $(EGREV)

# CYGWIN
$(LDIR)/libocsm.dll:	$(ODIR)/OpenCSM.o $(ODIR)/udp.o
	touch $(LDIR)/libocsm.dll
	touch $(LDIR)/libocsm.dll.a
	rm $(LDIR)/libocsm.dll
	rm $(LDIR)/libocsm.dll.a
	$(CC) -shared -o $(LDIR)/libocsm.dll \
		-Wl,--out-implib,$(LDIR)/libocsm.dll.a \
		$(ODIR)/OpenCSM.o $(ODIR)/udp.o -L$(LDIR) -legads -ldl $(CPPSLB)

$(ODIR)/OpenCSM.o:	OpenCSM.c OpenCSM.h common.h
	$(CC) -c $(OCOPT) $(DEFINE) -I$(IDIR) -I. OpenCSM.c -o $(ODIR)/OpenCSM.o

$(ODIR)/udp.o:	udp.c
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udp.c -o $(ODIR)/udp.o

#
#	user-defined primitives (UDPs) and functions (UDFs)
#
$(LDIR)/bezier.so:	$(ODIR)/udpBezier.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/bezier.so
	rm $(LDIR)/bezier.so
	$(CC) $(SOFLGS) -o $(LDIR)/bezier.so $(ODIR)/udpBezier.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udpBezier.o:	udpBezier.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpBezier.c \
		-o $(ODIR)/udpBezier.o

$(LDIR)/biconvex.so:	$(ODIR)/udpBiconvex.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/biconvex.so
	rm $(LDIR)/biconvex.so
	$(CC) $(SOFLGS) -o $(LDIR)/biconvex.so $(ODIR)/udpBiconvex.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udpBiconvex.o:	udpBiconvex.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpBiconvex.c \
		-o $(ODIR)/udpBiconvex.o

$(LDIR)/box.so:	$(ODIR)/udpBox.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/box.so
	rm $(LDIR)/box.so
	$(CC) $(SOFLGS) -o $(LDIR)/box.so $(ODIR)/udpBox.o -L$(LDIR) -locsm -legads -lm

$(ODIR)/udpBox.o:	udpBox.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpBox.c -o $(ODIR)/udpBox.o

$(LDIR)/catmull.so:	$(ODIR)/udfCatmull.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/catmull.so
	rm $(LDIR)/catmull.so
	$(CC) $(SOFLGS) -o $(LDIR)/catmull.so $(ODIR)/udfCatmull.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfCatmull.o:	udfCatmull.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfCatmull.c \
		-o $(ODIR)/udfCatmull.o

$(LDIR)/compare.so:	$(ODIR)/udfCompare.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/compare.so
	rm $(LDIR)/compare.so
	$(CC) $(SOFLGS) -o $(LDIR)/compare.so $(ODIR)/udfCompare.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfCompare.o:	udfCompare.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfCompare.c \
		-o $(ODIR)/udfCompare.o

$(LDIR)/createBEM.so:	$(ODIR)/udfCreateBEM.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/createBEM.so
	rm $(LDIR)/createBEM.so
	$(CC) $(SOFLGS) -o $(LDIR)/createBEM.so $(ODIR)/udfCreateBEM.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfCreateBEM.o:	udfCreateBEM.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfCreateBEM.c \
		-o $(ODIR)/udfCreateBEM.o

$(LDIR)/createPoly.so:	$(ODIR)/udfCreatePoly.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/createPoly.so
	rm $(LDIR)/createPoly.so
	$(CC) $(SOFLGS) -o $(LDIR)/createPoly.so $(ODIR)/udfCreatePoly.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfCreatePoly.o:	udfCreatePoly.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfCreatePoly.c \
		-o $(ODIR)/udfCreatePoly.o

$(LDIR)/csm.so:	$(ODIR)/udpCsm.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/csm.so
	rm $(LDIR)/csm.so
	$(CC) $(SOFLGS) -o $(LDIR)/csm.so $(ODIR)/udpCsm.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpCsm.o:	udpCsm.c udpUtilities.c udpUtilities.h OpenCSM.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpCsm.c -o $(ODIR)/udpCsm.o

$(LDIR)/droop.so:	$(ODIR)/udfDroop.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/droop.so
	rm $(LDIR)/droop.so
	$(CC) $(SOFLGS) -o $(LDIR)/droop.so $(ODIR)/udfDroop.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udfDroop.o:	udfDroop.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfDroop.c \
		-o $(ODIR)/udfDroop.o

$(LDIR)/editAttr.so:	$(ODIR)/udfEditAttr.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/editAttr.so
	rm $(LDIR)/editAttr.so
	$(CC) $(SOFLGS) -o $(LDIR)/editAttr.so $(ODIR)/udfEditAttr.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfEditAttr.o:	udfEditAttr.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfEditAttr.c \
		-o $(ODIR)/udfEditAttr.o

$(LDIR)/ellipse.so:	$(ODIR)/udpEllipse.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/ellipse.so
	rm $(LDIR)/ellipse.so
	$(CC) $(SOFLGS) -o $(LDIR)/ellipse.so $(ODIR)/udpEllipse.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udpEllipse.o:	udpEllipse.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpEllipse.c \
		-o $(ODIR)/udpEllipse.o

$(LDIR)/fitcurve.so:	$(ODIR)/udpFitcurve.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/fitcurve.so
	rm $(LDIR)/fitcurve.so
	$(CC) $(SOFLGS) -o $(LDIR)/fitcurve.so $(ODIR)/udpFitcurve.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udpFitcurve.o:	udpFitcurve.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpFitcurve.c \
		-o $(ODIR)/udpFitcurve.o

$(LDIR)/freeform.so:	$(ODIR)/udpFreeform.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/freeform.so
	rm $(LDIR)/freeform.so
	$(CC) $(SOFLGS) -o $(LDIR)/freeform.so $(ODIR)/udpFreeform.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udpFreeform.o:	udpFreeform.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpFreeform.c \
		-o $(ODIR)/udpFreeform.o

$(LDIR)/flend.so:	$(ODIR)/udfFlend.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/flend.so
	rm $(LDIR)/flend.so
	$(CC) $(SOFLGS) -o $(LDIR)/flend.so $(ODIR)/udfFlend.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfFlend.o:	udfFlend.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfFlend.c \
		-o $(ODIR)/udfFlend.o

$(LDIR)/ganged.so:	$(ODIR)/udfGanged.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/ganged.so
	rm $(LDIR)/ganged.so
	$(CC) $(SOFLGS) -o $(LDIR)/ganged.so $(ODIR)/udfGanged.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfGanged.o:	udfGanged.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfGanged.c \
		-o $(ODIR)/udfGanged.o

$(LDIR)/guide.so:	$(ODIR)/udfGuide.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/guide.so
	rm $(LDIR)/guide.so
	$(CC) $(SOFLGS) -o $(LDIR)/guide.so $(ODIR)/udfGuide.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfGuide.o:	udfGuide.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfGuide.c \
		-o $(ODIR)/udfGuide.o

$(LDIR)/matchBodys.so:	$(ODIR)/udfMatchBodys.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/matchBodys.so
	rm $(LDIR)/matchBodys.so
	$(CC) $(SOFLGS) -o $(LDIR)/matchBodys.so $(ODIR)/udfMatchBodys.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfMatchBodys.o:	udfMatchBodys.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfMatchBodys.c \
		-o $(ODIR)/udfMatchBodys.o

$(LDIR)/hex.so:	$(ODIR)/udpHex.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/hex.so
	rm $(LDIR)/hex.so
	$(CC) $(SOFLGS) -o $(LDIR)/hex.so $(ODIR)/udpHex.o -L$(LDIR) -locsm -legads -lm

$(ODIR)/udpHex.o:	udpHex.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpHex.c -o $(ODIR)/udpHex.o

$(LDIR)/import.so:	$(ODIR)/udpImport.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/import.so
	rm $(LDIR)/import.so
	$(CC) $(SOFLGS) -o $(LDIR)/import.so $(ODIR)/udpImport.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpImport.o:	udpImport.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpImport.c \
		-o $(ODIR)/udpImport.o

$(LDIR)/kulfan.so:	$(ODIR)/udpKulfan.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/kulfan.so
	rm $(LDIR)/kulfan.so
	$(CC) $(SOFLGS) -o $(LDIR)/kulfan.so $(ODIR)/udpKulfan.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpKulfan.o:	udpKulfan.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpKulfan.c \
		-o $(ODIR)/udpKulfan.o

$(LDIR)/naca.so:	$(ODIR)/udpNaca.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/naca.so
	rm $(LDIR)/naca.so
	$(CC) $(SOFLGS) -o $(LDIR)/naca.so $(ODIR)/udpNaca.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpNaca.o:	udpNaca.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpNaca.c -o $(ODIR)/udpNaca.o

$(LDIR)/naca456.so:	$(ODIR)/udpNaca456.o $(ODIR)/naca456.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/naca456.so
	rm $(LDIR)/naca456.so
	$(FCOMP) $(SOFLGS) -o $(LDIR)/naca456.so $(ODIR)/udpNaca456.o \
		$(ODIR)/naca456.o -L$(LDIR) -locsm -legads -lm $(ESPFLIBS)

$(ODIR)/udpNaca456.o:	udpNaca456.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpNaca456.c \
		-o $(ODIR)/udpNaca456.o

$(ODIR)/naca456.o:	naca456.f90
ifeq ($(EFCOMP),ifort)
	$(FCOMP) -c $(FOPTS) naca456.f90 -o $(ODIR)/naca456.o -module $(ODIR)
else
	$(FCOMP) -c $(FOPTS) naca456.f90 -o $(ODIR)/naca456.o -J $(ODIR)
endif

$(LDIR)/nurbbody.so:	$(ODIR)/udpNurbbody.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/nurbbody.so
	rm $(LDIR)/nurbbody.so
	$(CC) $(SOFLGS) -o $(LDIR)/nurbbody.so $(ODIR)/udpNurbbody.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udpNurbbody.o:	udpNurbbody.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpNurbbody.c \
		-o $(ODIR)/udpNurbbody.o

$(LDIR)/parsec.so:	$(ODIR)/udpParsec.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/parsec.so
	rm $(LDIR)/parsec.so
	$(CXX) $(SOFLGS) -o $(LDIR)/parsec.so $(ODIR)/udpParsec.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpParsec.o:	udpParsec.cpp udpUtilities.c udpUtilities.h \
			$(IDIR)/Surreal/SurrealS.h
	$(CXX) -c $(CPPOPT) $(DEFINE) -I$(IDIR) -I. udpParsec.cpp \
		-o $(ODIR)/udpParsec.o

$(LDIR)/pod.so:	$(ODIR)/udpPod.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/pod.so
	rm $(LDIR)/pod.so
	$(CC) $(SOFLGS) -o $(LDIR)/pod.so $(ODIR)/udpPod.o -L$(LDIR) -locsm -legads -lm

$(ODIR)/udpPod.o:	udpPod.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpPod.c -o $(ODIR)/udpPod.o

$(LDIR)/poly.so:	$(ODIR)/udpPoly.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/poly.so
	rm $(LDIR)/poly.so
	$(CC) $(SOFLGS) -o $(LDIR)/poly.so $(ODIR)/udpPoly.o -L$(LDIR) -locsm -legads -lm

$(ODIR)/udpPoly.o:	udpPoly.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpPoly.c -o $(ODIR)/udpPoly.o

$(LDIR)/printBbox.so:	$(ODIR)/udfPrintBbox.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/printBbox.so
	rm $(LDIR)/printBbox.so
	$(CC) $(SOFLGS) -o $(LDIR)/printBbox.so $(ODIR)/udfPrintBbox.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfPrintBbox.o:	udfPrintBbox.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfPrintBbox.c \
		-o $(ODIR)/udfPrintBbox.o

$(LDIR)/printBrep.so:	$(ODIR)/udfPrintBrep.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/printBrep.so
	rm $(LDIR)/printBrep.so
	$(CC) $(SOFLGS) -o $(LDIR)/printBrep.so $(ODIR)/udfPrintBrep.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udfPrintBrep.o:	udfPrintBrep.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfPrintBrep.c \
		-o $(ODIR)/udfPrintBrep.o

$(LDIR)/radwaf.so:	$(ODIR)/udpRadwaf.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/radwaf.so
	rm $(LDIR)/radwaf.so
	$(CC) $(SOFLGS) -o $(LDIR)/radwaf.so $(ODIR)/udpRadwaf.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpRadwaf.o:	udpRadwaf.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpRadwaf.c \
		-o $(ODIR)/udpRadwaf.o

$(LDIR)/sample.so:	$(ODIR)/udpSample.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/sample.so
	rm $(LDIR)/sample.so
	$(CC) $(SOFLGS) -o $(LDIR)/sample.so $(ODIR)/udpSample.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpSample.o:	udpSample.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpSample.c \
		-o $(ODIR)/udpSample.o

$(LDIR)/sew.so:	$(ODIR)/udpSew.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/sew.so
	rm $(LDIR)/sew.so
	$(CC) $(SOFLGS) -o $(LDIR)/sew.so $(ODIR)/udpSew.o -L$(LDIR) -locsm -legads -lm

$(ODIR)/udpSew.o:	udpSew.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpSew.c -o $(ODIR)/udpSew.o

$(LDIR)/stag.so:	$(ODIR)/udpStag.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/stag.so
	rm $(LDIR)/stag.so
	$(CC) $(SOFLGS) -o $(LDIR)/stag.so $(ODIR)/udpStag.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpStag.o:	udpStag.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpStag.c -o $(ODIR)/udpStag.o

$(LDIR)/stiffener.so:	$(ODIR)/udfStiffener.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/stiffener.so
	rm $(LDIR)/stiffener.so
	$(CC) $(SOFLGS) -o $(LDIR)/stiffener.so $(ODIR)/udfStiffener.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udfStiffener.o:	udfStiffener.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udfStiffener.c -o $(ODIR)/udfStiffener.o

$(LDIR)/supell.so:	$(ODIR)/udpSupell.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/supell.so
	rm $(LDIR)/supell.so
	$(CC) $(SOFLGS) -o $(LDIR)/supell.so $(ODIR)/udpSupell.o -L$(LDIR) \
		-locsm -legads -lm

$(ODIR)/udpSupell.o:	udpSupell.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpSupell.c \
		-o $(ODIR)/udpSupell.o

$(LDIR)/waffle.so:	$(ODIR)/udpWaffle.o $(LDIR)/$(OSHLIB)
	touch $(LDIR)/waffle.so
	rm $(LDIR)/waffle.so
	$(CC) $(SOFLGS) -o $(LDIR)/waffle.so $(ODIR)/udpWaffle.o \
		-L$(LDIR) -locsm -legads -lm

$(ODIR)/udpWaffle.o:	udpWaffle.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. udpWaffle.c \
		-o $(ODIR)/udpWaffle.o

#
#	external grid generators
#
$(LDIR)/PDT2.so:	$(ODIR)/eggPDT2.o
	touch $(LDIR)/PDT2.so
	rm $(LDIR)/PDT2.so
	$(CC) $(SOFLGS) -o $(LDIR)/PDT2.so $(ODIR)/eggPDT2.o -L$(LDIR) \
		-legads -lm

$(ODIR)/eggPDT2.o:	eggPDT2.c udpUtilities.c udpUtilities.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I. eggPDT2.c \
		-o $(ODIR)/eggPDT2.o

#
#	include files
#
$(IDIR)/OpenCSM.h:	OpenCSM.h
	touch $(IDIR)/OpenCSM.h
	rm $(IDIR)/OpenCSM.h
	cp -p OpenCSM.h $(IDIR)/OpenCSM.h

$(IDIR)/common.h:	common.h
	touch $(IDIR)/common.h
	rm $(IDIR)/common.h
	cp -p common.h $(IDIR)/common.h

$(IDIR)/udp.h:	udp.h
	touch $(IDIR)/udp.h
	rm $(IDIR)/udp.h
	cp -p udp.h $(IDIR)/udp.h

$(IDIR)/egg.h:	egg.h
	touch $(IDIR)/egg.h
	rm $(IDIR)/egg.h
	cp -p egg.h $(IDIR)/egg.h

$(IDIR)/udpUtilities.h:	udpUtilities.h
	touch $(IDIR)/udpUtilities.h
	rm $(IDIR)/udpUtilities.h
	cp -p udpUtilities.h $(IDIR)/udpUtilities.h

$(IDIR)/udpUtilities.c:	udpUtilities.c
	touch $(IDIR)/udpUtilities.c
	rm $(IDIR)/udpUtilities.c
	cp -p udpUtilities.c $(IDIR)/udpUtilities.c

#
#	other targets
#
clean:
	(cd $(ODIR); rm -f udpFitcurve.o udpFreeform.o udpWaffle.o udpCsm.o \
		udpEllipse.o udpNaca.o udpHex.o udpImport.o serveCSM.o \
		OpenCSM.o udp.o udpBezier.o udpBox.o udpNurbbody.o udpParsec.o \
		udpSample.o udpSew.o udpRadwaf.o udpNaca456.o naca456.o \
		udpSupell.o udpStag.o udfStiffener.o udpKulfan.o udpBiconvex.o \
		udfCatmull.o udfCompare.o udfCreateBEM.o udfCreatePoly.o udfDroop.o \
		udpPod.o udpPoly.o udfEditAttr.o udfPrintBbox.o udfPrintBrep.o \
		udfFlend.o udfGanged.o udfGuide.o \
		udfMatchBodys.o eggPDT2.o sensCSM.o )

cleanall:	clean
	rm -f  $(BINLIST) $(LIBLIST) $(LIBFLIST) $(INCLIST)
