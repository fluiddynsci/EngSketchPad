#	NMakefile for udfTile
#
IDIR  = $(ESP_ROOT)\include
!include $(IDIR)\$(ESP_ARCH).$(MSVC)
LDIR  = $(ESP_ROOT)\lib
BDIR  = $(ESP_ROOT)\bin
!ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)\obj
!else
ODIR  = .
!endif

IRIT_DEF = -D__WINNT__ -D_WIN64 -D_AMD64_ -DRANDOM_IRIT

$(LDIR)\tile.dll:	$(ODIR)\udfTile.obj
	-del $(LDIR)\tile.dll
	link /out:$(LDIR)\tile.dll /dll /def:udp.def $(ODIR)\udfTile.obj $(LDIR)\ocsm.lib $(LDIR)\egads.lib $(IRIT_LIB)\irit64.lib
	$(MCOMP) /manifest $(LDIR)\tile.dll.manifest /outputresource:$(LDIR)\tile.dll;2

$(ODIR)\udfTile.obj:	udfTile.c udpUtilities.c udpUtilities.h OpenCSM.h
	cl /c $(COPTS) $(DEFINE) $(IRIT_DEF) /I$(IDIR) /I$(IRIT_INC) udfTile.c /Fo$(ODIR)\udfTile.obj

clean:
	-del $(ODIR)\udfTile.obj

cleanall:	clean
	-del $(LDIR)\tile.dll
