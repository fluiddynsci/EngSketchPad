#
ifndef ESP_ROOT
$(error ESP_ROOT must be set -- Please fix the environment...)
endif
#
ifndef AFLR
$(error AFLR must be set -- Please fix the environment...)
endif
#
ifndef AFLR_ARCH
$(error AFLR_ARCH must be set -- Please fix the environment...)
endif
#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
else
ODIR  = .
endif

AFLR3_LIB  = -laflr3 -laflr4 -lsurf_auto -lrec3 -lotb -lug3 -lug
AFLR3_LIB += -lanbl3 -laflr2c -lrec2 -lbl1
AFLR3_LIB += -ldgeom -legen -lug2 -lice2 -lice3  -ldftr2 -ldftr3
AFLR3_LIB += -lug_io -lug_gq -lug_cpp -lcgns -lqtb

ifeq ("$(ESP_ARCH)","LINUX64")
AFLR3_LIB += -lrt
endif

###############################################################################
# Used to add debug information to AFLR3
# Please do not remove this - MCG
#AFLR3_FILES=$(wildcard $(AFLR)/src/aflr3/*.c)
#AFLR3_OBJS=$(patsubst %.c,%.o,$(AFLR3_FILES))

#UG_FILES=$(wildcard $(AFLR)/src/ug/*.c)
#UG_OBJS=$(patsubst %.c,%.o,$(UG_FILES))

#OBJS=$(AFLR3_OBJS) $(UG_OBJS)
###############################################################################

$(LDIR)/aflr3AIM.so:	$(ODIR)/aflr3AIM.o $(ODIR)/aflr3_Interface.o \
			$(LDIR)/libutils.a  $(LDIR)/libaimUtil.a $(OBJS)
	touch $(LDIR)/aflr3AIM.so
	rm $(LDIR)/aflr3AIM.so
	$(CC) $(SOFLGS) -o $(LDIR)/aflr3AIM.so $(ODIR)/aflr3AIM.o \
		$(ODIR)/aflr3_Interface.o $(OBJS) -L$(AFLR)/$(AFLR_ARCH)/lib \
		$(AFLR3_LIB) $(AFLR3_LIB) $(AFLR3_LIB) -L$(LDIR) -lutils \
		-laimUtil -locsm -legads -ludunits2 $(RPATH) -lm

$(ODIR)/aflr3AIM.o:	$(ODIR)/aflr3_version aflr3AIM.c $(IDIR)/capsTypes.h \
			../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I../utils aflr3AIM.c \
		-o $(ODIR)/aflr3AIM.o

$(ODIR)/aflr3_Interface.o:	$(ODIR)/aflr3_version aflr3_Interface.c ../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -D_ENABLE_BL_ -I$(IDIR) \
		-I$(AFLR)/src/aflr3 -I$(AFLR)/src -I../utils \
		aflr3_Interface.c -o $(ODIR)/aflr3_Interface.o

# Compile and execute to check the version number
.DELETE_ON_ERROR:
$(ODIR)/aflr3_version:	aflr3_version.c $(AFLR)/src/aflr3/aflr3_version.h 
	$(CC) $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src aflr3_version.c \
		-L$(AFLR)/$(AFLR_ARCH)/lib -laflr3 \
		-o $(ODIR)/aflr3_version
	$(ODIR)/aflr3_version

ifdef OBJS
$(OBJS): %.o:	%.c
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src $< -o $@
endif

clean:
	-rm -f $(ODIR)/aflr3_Interface.o $(ODIR)/aflr3AIM.o $(ODIR)/aflr3_version $(OBJS)

cleanall:	clean
	-rm -f $(LDIR)/aflr3AIM.so

dox:
	(cd doc; doxygen aflr3AIM_dox.cfg; cd latex; make; mv refman.pdf AFLR3AIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f aflr3AIM.tag)
