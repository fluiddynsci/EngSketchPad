#
IDIR  = $(ESP_ROOT)/include
include $(IDIR)/$(ESP_ARCH)
LDIR  = $(ESP_ROOT)/lib
ifdef ESP_BLOC
ODIR  = $(ESP_BLOC)/obj
TDIR  = $(ESP_BLOC)/test
else
ODIR  = .
TDIR  = .
endif


VPATH = $(ODIR)

OBJS = base64-decode.o handshake.o client-handshake.o libwebsockets.o \
       extension-deflate-stream.o md5.o extension-x-google-mux.o parsers.o \
       extension.o sha-1.o browserMessage.o server.o wv.o fwv.o

$(TDIR)/server:	$(LDIR)/libwsserver.a $(ODIR)/servertest.o
	$(CC) -o $(TDIR)/server $(ODIR)/servertest.o -L$(LDIR) -lwsserver \
		-lpthread -lz -lm

$(ODIR)/servertest.o:	server.c 
	$(CC) -c $(COPTS) -DLWS_NO_FORK -DSTANDALONE server.c \
		-I$(IDIR) -I. -o $(ODIR)/servertest.o

$(LDIR)/libwsserver.a:	$(OBJS)
	touch $(LDIR)/libwsserver.a
	rm $(LDIR)/libwsserver.a
	(cd $(ODIR); ar $(LOPTS) $(LDIR)/libwsserver.a $(OBJS); $(RANLB) )

$(OBJS):	extension-deflate-stream.h libwebsockets.h \
		extension-x-google-mux.h private-libwebsockets.h \
		$(IDIR)/wsserver.h $(IDIR)/wsss.h
.c.o:
	$(CC) -c $(COPTS) -DLWS_NO_FORK -I$(IDIR) $< -o $(ODIR)/$@

clean:
	-(cd $(ODIR); rm $(OBJS) servertest.o )

cleanall:	clean
	-rm $(TDIR)/server $(LDIR)/libwsserver.a
