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

AFLR4_LIB  = -laflr4 -laflr2c
AFLR4_LIB += -lsurf_auto -lsurf_auto_cpp -laflr4 -laflr3
AFLR4_LIB += -legen -lotb -lrec2 -lrec3 -ldftr3 -ldftr2 -ldgeom -lqtb
AFLR4_LIB += -lice2 -lice3 -lug3 -lug2 -lug -lug_mp

ifeq ("$(ESP_ARCH)","LINUX64")
AFLR4_LIB += -lrt
endif

# CAPS compiles egads_aflr4 library to use the latest egads.h file
VPATH             = $(AFLR)/src/egads_aflr4
EGADS_AFLR4_FILES = $(wildcard $(AFLR)/src/egads_aflr4/*.c)
EGADS_AFLR4_OBJS  = $(subst $(AFLR)/src/egads_aflr4,$(ODIR),$(EGADS_AFLR4_FILES:.c=.o))
EGADS_INC_FILES   = $(wildcard $(IDIR)/egads*.h)

###############################################################################
# Used to add debug information to AFLR4
# Please do not remove this - MCG
#VPATH      += $(AFLR)/src/aflr4
#AFLR4_FILES = $(wildcard $(AFLR)/src/aflr4/*.c)
#AFLR4_OBJS  = $(subst $(AFLR)/src/aflr4,$(ODIR),$(AFLR4_FILES:.c=.o))

#VPATH          += $(AFLR)/src/surf_auto
#SURF_AUTO_FILES = $(wildcard $(AFLR)/src/surf_auto/*.c)
#SURF_AUTO_OBJS  = $(subst $(AFLR)/src/surf_auto,$(ODIR),$(SURF_AUTO_FILES:.c=.o))

#VPATH    += $(AFLR)/src/ug3
#UG3_FILES = $(wildcard $(AFLR)/src/ug3/*.c)
#UG3_OBJS  = $(subst $(AFLR)/src/ug3,$(ODIR),$(UG3_FILES:.c=.o))

#VPATH              += $(AFLR)/src/surf_auto_cpp
#SURF_AUTO_CPP_FILES = $(wildcard $(AFLR)/src/surf_auto_cpp/*.cpp)
#SURF_AUTO_CPP_OBJS  = $(subst $(AFLR)/src/surf_auto_cpp,$(ODIR),$(SURF_AUTO_CPP_FILES:.cpp=.o))

OBJS=$(EGADS_AFLR4_OBJS) $(AFLR4_OBJS) $(SURF_AUTO_OBJS) $(UG3_OBJS)
#OBJSP=$(SURF_AUTO_CPP_OBJS)
###############################################################################

$(LDIR)/aflr4AIM.so:	$(ODIR)/aflr4_version $(ODIR)/aflr4AIM.o $(ODIR)/aflr4_Interface.o \
			$(LDIR)/libutils.a $(LDIR)/libaimUtil.a $(OBJS) $(OBJSP)
	touch $(LDIR)/aflr4AIM.so
	rm $(LDIR)/aflr4AIM.so
	$(CC) $(SOFLGS) -o $(LDIR)/aflr4AIM.so $(ODIR)/aflr4AIM.o \
		$(ODIR)/aflr4_Interface.o $(OBJS) $(OBJSP) -L$(AFLR)/$(AFLR_ARCH)/lib \
		$(AFLR4_LIB) -L$(LDIR) -lutils \
		-laimUtil -locsm -legads -ludunits2 $(RPATH) $(CPPSLB) -lm

$(ODIR)/aflr4AIM.o:	$(ODIR)/aflr4_version aflr4AIM.c $(IDIR)/capsTypes.h \
			../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src -I../utils aflr4AIM.c \
		-o $(ODIR)/aflr4AIM.o

$(ODIR)/aflr4_Interface.o:	$(ODIR)/aflr4_version aflr4_Interface.c ../utils/meshUtils.h
	$(CC) -c $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src \
		-I../utils aflr4_Interface.c -o $(ODIR)/aflr4_Interface.o

# Compile and execute to check the version number
.DELETE_ON_ERROR:
$(ODIR)/aflr4_version:	aflr4_version.c $(AFLR)/src/aflr4/aflr4_version.h 
	$(CC) $(COPTS) $(DEFINE) -I$(IDIR) -I$(AFLR)/src aflr4_version.c \
		-L$(AFLR)/$(AFLR_ARCH)/lib -laflr4 -lm \
		-o $(ODIR)/aflr4_version
	$(ODIR)/aflr4_version

# Build for each AFLR C source file
$(ODIR)/%.o: %.c $(ODIR)/aflr4_version $(EGADS_INC_FILES)
	$(CC) -c $(COPTS) $(DEFINE) -w -I$(IDIR) -I$(AFLR)/src $< -o $@

ifdef OBJSP
# Build for each AFLR C++ source file
$(ODIR)/%.o: %.cpp $(ODIR)/aflr4_version
	$(CXX) -c $(CPPOPT) $(DEFINE) -I$(AFLR)/src $< -o $@
endif

clean:
	-rm -f $(ODIR)/aflr4_Interface.o $(ODIR)/aflr4AIM.o $(ODIR)/aflr4_version $(OBJS) $(OBJSP)

cleanall:	clean
	-rm -f $(LDIR)/aflr4AIM.so

dox:
	(cd doc; doxygen aflr4AIM_dox.cfg; cd latex; make; mv refman.pdf AFLR4AIM.pdf)

doxclean:
	(cd doc; rm -f INPUT; rm -rf html latex; rm -f aflr4AIM.tag)
