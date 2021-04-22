#
IDIR  = $(ESP_ROOT)\include
!include $(IDIR)\$(ESP_ARCH).$(MSVC)
LDIR  = $(ESP_ROOT)\lib
!IFDEF ESP_BLOC
ODIR  = $(ESP_BLOC)\obj
TDIR  = $(ESP_BLOC)\examples\cCAPS
!ELSE
ODIR  = .
TDIR  = .
!ENDIF

TESTLIST = $(TDIR)\fun3dTest.exe

!ifdef TETGEN
TESTLIST = $(TESTLIST) \
		$(TDIR)\fun3dTetgenTest.exe \
		$(TDIR)\aeroelasticTest.exe \
!endif

!ifdef AFLR
TESTLIST = $(TESTLIST) $(TDIR)\fun3dAFLR2Test.exe $(TDIR)\fun3dAFLR4Test.exe
!endif

default: $(TESTLIST)

$(TDIR)\fun3dTest.exe:  $(ODIR)\fun3dTest.obj $(LDIR)\caps.lib
	cl /Fe$(TDIR)/fun3dTest.exe $(ODIR)\fun3dTest.obj \
		/link /LIBPATH:$(LDIR) caps.lib egads.lib

$(TDIR)\fun3dAFLR2Test.exe:  $(ODIR)\fun3dAFLR2Test.obj $(LDIR)\caps.lib
	cl /Fe$(TDIR)/fun3dAFLR2Test.exe $(ODIR)\fun3dAFLR2Test.obj \
		/link /LIBPATH:$(LDIR) caps.lib egads.lib

$(TDIR)\fun3dAFLR4Test.exe:  $(ODIR)\fun3dAFLR4Test.obj $(LDIR)\caps.lib
	cl /Fe$(TDIR)/fun3dAFLR4Test.exe $(ODIR)\fun3dAFLR4Test.obj \
		/link /LIBPATH:$(LDIR) caps.lib egads.lib

$(TDIR)\fun3dTetgenTest.exe:  $(ODIR)\fun3dTetgenTest.obj $(LDIR)\caps.lib
	cl /Fe$(TDIR)/fun3dTetgenTest.exe $(ODIR)\fun3dTetgenTest.obj \
		/link /LIBPATH:$(LDIR) caps.lib egads.lib

$(TDIR)\aeroelasticTest.exe:  $(ODIR)\aeroelasticTest.obj $(LDIR)\caps.lib
	cl /Fe$(TDIR)/aeroelasticTest.exe $(ODIR)\aeroelasticTest.obj \
		/link /LIBPATH:$(LDIR) caps.lib egads.lib

$(ODIR)\fun3dTest.obj:  fun3dTest.c $(IDIR)\caps.h
	cl /c $(COPTS) $(DEFINE) -I$(IDIR) fun3dTest.c /Fo$(ODIR)\fun3dTest.obj

$(ODIR)\fun3dAFLR2Test.obj:  fun3dAFLR2Test.c $(IDIR)\caps.h
	cl /c $(COPTS) $(DEFINE) -I$(IDIR) fun3dAFLR2Test.c /Fo$(ODIR)\fun3dAFLR2Test.obj

$(ODIR)\fun3dAFLR4Test.obj:  fun3dAFLR4Test.c $(IDIR)\caps.h
	cl /c $(COPTS) $(DEFINE) -I$(IDIR) fun3dAFLR4Test.c /Fo$(ODIR)\fun3dAFLR4Test.obj

$(ODIR)\fun3dTetgenTest.obj:  fun3dTetgenTest.c $(IDIR)\caps.h
	cl /c $(COPTS) $(DEFINE) -I$(IDIR) fun3dTetgenTest.c /Fo$(ODIR)\fun3dTetgenTest.obj
    
$(ODIR)\aeroelasticTest.obj:  aeroelasticTest.c $(IDIR)\caps.h
	cl /c $(COPTS) $(DEFINE) -I$(IDIR) aeroelasticTest.c /Fo$(ODIR)\aeroelasticTest.obj

clean:
	-del $(ODIR)\fun3dTest.obj \
		$(ODIR)\fun3dAFLR2Test.obj \
		$(ODIR)\fun3dAFLR4Test.obj \
		$(ODIR)\fun3dTetgenTest.obj \
		$(ODIR)\aeroelasticTest.obj

cleanall:	clean
	-del $(TDIR)\fun3dTest.exe \
		$(TDIR)\fun3dAFLR2Test.exe \
		$(TDIR)\fun3dAFLR4Test.exe \
		$(TDIR)\fun3dTetgenTest.exe \
		$(TDIR)\aeroelasticTest.exe
