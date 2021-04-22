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

AFLR2_LIB  = -laflr2c -legen
AFLR2_LIB += -lrec2 -ldftr2 -ldftr3 -ldgeom -lotb -lqtb
AFLR2_LIB += -lice2 -lug2 -lug3 -lug_gq -lug

ifeq ("$(ESP_ARCH)","LINUX64")
AFLR2_LIB += -lrt
endif

# Used to add debug information to AFLR2
# Please do not remove this - MCG
# AFLR2_FILES=$(wildcard $(AFLR)/src/aflr2c/*.c)
# AFLR2_OBJS=$(patsubst %.c,%.o,$(AFLR2_FILES))

# OBJS=$(AFLR2_OBJS)

$(LDIR)/aflr2AIM.so:	$(ODIR)/aflr2AIM.o $(ODIR)/aflr2_Interface.o \
			$(LDIR)/libutils.a $(LDIR)/libaimUtil.a $(OBJS)
	touch $(LDIR)/aflr2AIM.so
	rm $(LDIR)/aflr2AIM.so
	$(CC) $(SOFLGS) -o $(LDIR)/aflr2AIM.so $(ODIR)/aflr2AIM.o \
		$(ODIR)/aflr2_Interface.o $(OBJS) -L$(AFLR)/$(AFLR_ARCH)/lib \
		$(AFLR2_LIB) -L$(LDIR) -lutils \
		-laimUtil -locsm -legads -ludunits2 $(RPATH) $(CPPSLB) -lm

$(ODIR)/aflr2AIM.o:	$(ODIR)/aflr2_version aflr2AIM.c $(IDIR)/capsTypes.h \
			../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src -I../utils aflr2AIM.c \
		-o $(ODIR)/aflr2AIM.o

$(ODIR)/aflr2_Interface.o:	$(ODIR)/aflr2_version aflr2_Interface.c ../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src \
		-I../utils aflr2_Interface.c -o $(ODIR)/aflr2_Interface.o

# Compile and execute to check the version number
.DELETE_ON_ERROR:
$(ODIR)/aflr2_version:	aflr2_version.c $(AFLR)/src/aflr2c/aflr2c_version.h 
	$(CC) $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src aflr2_version.c \
		-L$(AFLR)/$(AFLR_ARCH)/lib -laflr2c -lm \
		-o $(ODIR)/aflr2_version
	$(ODIR)/aflr2_version

ifdef OBJS
$(OBJS): %.o:	%.c
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src $< -o $@
endif

clean:
	-rm -f $(ODIR)/aflr2_Interface.o $(ODIR)/aflr2AIM.o $(ODIR)/aflr2_version $(OBJS)

cleanall:	clean
	-rm -f $(LDIR)/aflr2AIM.so

dox:
	(cd doc; doxygen aflr2AIM_dox.cfg; cd latex; make; mv refman.pdf AFLR2AIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f aflr2AIM.tag)
