/*
 *	The Web Viewer
 *
 *		WV simple server code
 *
 *      Copyright 2011-2022, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef WIN32
#include <pthread.h>
#endif

#include "libwebsockets.h"
#include "wsss.h"

#ifdef STANDALONE
#include "wsserver.h"
#endif

  /* prototypes used & not in the above */
  extern /*@null@*/ /*@out@*/ /*@only@*/ void *wv_alloc(int nbytes);
  extern /*@null@*/ /*@only@*/ 
         void *wv_realloc(/*@null@*/ /*@only@*/ /*@returned@*/ void *ptr, 
                          int nbytes);
  extern void  wv_free(/*@null@*/ /*@only@*/ void *ptr);
  
  extern int   wv_sendGPrim(void *wsi, wvContext *cntxt, unsigned char *buf, 
                            int flag);
  extern void  wv_freeGPrim(wvGPrim gprim);
  extern void  wv_destroyContext(wvContext **context);
  extern void  wv_prepareForSends(wvContext *context);
  extern void  wv_finishSends(wvContext *context);

#ifndef STANDALONE
  /* UI test message call-back */
  extern void  browserMessage(/*@null@*/ void *userPtr,
                              /*@null@*/ struct libwebsocket *wsi, char *buf,
                              int len);
#endif


enum wv_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_GPRIM_BINARY,
	PROTOCOL_UI_TEXT,

	/* always last */
	WV_PROTOCOL_COUNT
};



#ifdef WIN32

static int ThreadCreate(void (*entry)(), void *arg)
{
  return _beginthread(entry, 0, arg);
}

#else

static int ThreadCreate(void (*entry)(), void *arg)
{
  pthread_t thread;
  pthread_attr_t attr;
  int stat;

  pthread_attr_init(&attr);
  stat = pthread_create(&thread, &attr,
                        (void*(*) (void *)) entry, arg);
  if (stat != 0) return -1;
  return 1;
}

#endif


typedef struct {
        int                         nClient;    /* # active clients */
        struct libwebsocket         **wsi;      /* the text wsi per client */
        int                         loop;       /* 1 for continue;
                                                   0 for done */
        int                         index;      /* server index */
        struct libwebsocket_context *WScontext; /* WebSocket context */
        wvContext                   *WVcontext; /* WebViewer context */
        int                         nMess;      /* number of messages  or  -1
                                                   for outside browserMessage */
        char                        **mess;     /* stored messages */
        unsigned char               xbuf[LWS_SEND_BUFFER_PRE_PADDING + BUFLEN +
                                         LWS_SEND_BUFFER_POST_PADDING];
} wvServer;


static int      nServers = 0;
static wvServer *servers = NULL;


/* this protocol server (always the first one) just knows how to do HTTP */

static int callback_http(/*@unused@*/ struct libwebsocket_context *context,
		         struct libwebsocket *wsi,
		         enum libwebsocket_callback_reasons reason, 
                         void *user, void *in, /*@unused@*/ size_t len)
{
	char client_name[128];
	char client_ip[128];

	switch (reason) {

	case LWS_CALLBACK_HTTP:
		fprintf(stderr, "serving HTTP URI %s\n", (char *)in);

		if (in && strcmp(in, "/favicon.ico") == 0) {
			if (libwebsockets_serve_http_file(wsi,
			     "favicon.ico", "image/x-icon"))
				fprintf(stderr, "Failed to send favicon\n");
			break;
		}

		/* send the script... when it runs it'll start websockets */

		if (libwebsockets_serve_http_file(wsi,
				  "wv.html", "text/html"))
			fprintf(stderr, "Failed to send HTTP file\n");
		break;

	/*
	 * callback for confirming to continue with client IP appear in
	 * protocol 0 callback since no websocket protocol has been agreed
	 * yet.  You can just ignore this if you won't filter on client IP
	 * since the default uhandled callback return is 0 meaning let the
	 * connection continue.
	 */

	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:

		libwebsockets_get_peer_addresses(user, client_name,
			     sizeof(client_name), client_ip, sizeof(client_ip));

		fprintf(stdout, "Received network connect from %s (%s)\n",
							client_name, client_ip);

		/* if we returned non-zero from here, we kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/*
 * this is just an example of parsing handshake headers, this is not needed
 * unless filtering is allowing connections by the header content
 *
 */

static void
dump_handshake_info(struct lws_tokens *lwst)
{
	int n;
	static const char *token_names[WSI_TOKEN_COUNT] = {
		/*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
		/*[WSI_TOKEN_HOST]		=*/ "Host",
		/*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
		/*[WSI_TOKEN_KEY1]		=*/ "key 1",
		/*[WSI_TOKEN_KEY2]		=*/ "key 2",
		/*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
		/*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
		/*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
		/*[WSI_TOKEN_DRAFT]		=*/ "Draft",
		/*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",

		/* new for 04 */
		/*[WSI_TOKEN_KEY]		=*/ "Key",
		/*[WSI_TOKEN_VERSION]		=*/ "Version",
		/*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",

		/* new for 05 */
		/*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",

		/* client receives these */
		/*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
		/*[WSI_TOKEN_NONCE]		=*/ "Nonce",
		/*[WSI_TOKEN_HTTP]		=*/ "Http",
		/*[WSI_TOKEN_MUXURL]            =*/ "MuxURL",
	};
	
	for (n = 0; n < WSI_TOKEN_COUNT; n++) {
		if (lwst[n].token == NULL) continue;
		fprintf(stdout, "    %s = %s\n", token_names[n], lwst[n].token);
	}
}


/* gPrim binary protocol */

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 *
 */

struct per_session_data__gprim_binary {
	int status;
};

static int
callback_gprim_binary(struct libwebsocket_context *context,
                      struct libwebsocket *wsi,
                      enum libwebsocket_callback_reasons reason,
                      void *user, /*@unused@*/ void *in, size_t len)
{
	int slot;
	struct per_session_data__gprim_binary *pss = user;

        for (slot = 0; slot < nServers; slot++)
                if (servers[slot].WScontext == context) break;
        if (slot == nServers) {
                fprintf(stderr, "ERROR no Slot!");
                exit(1);          
        }

	switch (reason) {

        /*
         * invoked when initial connection is made
         */
         
	case LWS_CALLBACK_ESTABLISHED:
		fprintf(stdout, "callback_gprim_binary: LWS_CALLBACK_ESTABLISHED\n");
		pss->status = 0;
		break;

	/*
	 * in this protocol, we just use the broadcast action as the chance to
	 * send our own connection-specific data and ignore the broadcast info
	 * that is available in the 'in' parameter
	 */

	case LWS_CALLBACK_BROADCAST:
                if (pss->status == 0) {
                  if (servers[slot].WVcontext == NULL) return 0;
                  /* send the init packet */
                  wv_sendGPrim(wsi, servers[slot].WVcontext, 
                               &servers[slot].xbuf[LWS_SEND_BUFFER_PRE_PADDING],
                                1);
                  pss->status++;
                } else if (pss->status == 1) {
                  /* send the first suite of gPrims */
                  wv_sendGPrim(wsi, servers[slot].WVcontext,
                               &servers[slot].xbuf[LWS_SEND_BUFFER_PRE_PADDING],
                               -1);
                  pss->status++;
                } else {
                  /* send the updated suite of gPrims */
                  wv_sendGPrim(wsi, servers[slot].WVcontext, 
                               &servers[slot].xbuf[LWS_SEND_BUFFER_PRE_PADDING],
                                0);
                }
		break;

	case LWS_CALLBACK_RECEIVE:
		fprintf(stderr, "gprim-binary: rx %d\n", (int) len);
                /* we should not get here! */
		break;
                
        case LWS_CALLBACK_CLOSED:
                fprintf(stdout, "callback_gprim_binary: LWS_CALLBACK_CLOSED\n");
                break;
        
	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info((struct lws_tokens *) user);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/* ui_text protocol */

struct per_session_data__ui_text {
	struct libwebsocket *wsi;
};

static int
callback_ui_text(struct libwebsocket_context *context,
                 struct libwebsocket *wsi,
                 enum libwebsocket_callback_reasons reason,
                 void *user, void *in, size_t len)
{
	int    i, j, slot, n;
        char   *mess;
        void   *userPtr;
	struct per_session_data__ui_text *pss = user;
        struct libwebsocket **tmp;

        for (slot = 0; slot < nServers; slot++)
                if (servers[slot].WScontext == context) break;
        if (slot == nServers) {
                fprintf(stderr, "ERROR no Slot!");
                exit(1);          
        }

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		fprintf(stdout, "callback_ui_text: LWS_CALLBACK_ESTABLISHED\n");
		pss->wsi = wsi;
                n        = servers[slot].nClient + 1;
                if (servers[slot].wsi == NULL) {
                  tmp = (struct libwebsocket **)
                        wv_alloc(n*sizeof(struct libwebsocket *));
                } else {
                  tmp = (struct libwebsocket **)
                        wv_realloc(servers[slot].wsi,
                                   n*sizeof(struct libwebsocket *));
                }
                if (tmp == NULL) {
                    fprintf(stderr, "callback_ui_text: Malloc Problem!\n");
                } else {
                    servers[slot].wsi = tmp;
                    servers[slot].wsi[servers[slot].nClient] = wsi;
                    servers[slot].nClient++;
                }
		break;

	case LWS_CALLBACK_BROADCAST:
		n = libwebsocket_write(wsi, in, len, LWS_WRITE_TEXT);
		if (n < 0) fprintf(stderr, "ui-text write failed\n");
		break;

	case LWS_CALLBACK_RECEIVE:
                servers[slot].nMess = 0;
                userPtr = servers[slot].WVcontext->userPtr;
                if (servers[slot].WVcontext->callback == NULL) {
                  browserMessage(userPtr, wsi, in, len);
                } else {
                  servers[slot].WVcontext->callback(userPtr, wsi, in, len);
                }
                for (j = 0; j < servers[slot].nMess; j++) {
                  len  = strlen(servers[slot].mess[j]);
                  mess =        servers[slot].mess[j];
                  if (servers[slot].WVcontext->callback == NULL) {
                    browserMessage(userPtr, NULL, mess, len);
                  } else {
                    servers[slot].WVcontext->callback(userPtr, NULL, mess, len);
                  }
                }
                if (servers[slot].mess != NULL) {
                  for (j = 0; j < servers[slot].nMess; j++)
                    wv_free(servers[slot].mess[j]);
                  wv_free(servers[slot].mess);
                }
                servers[slot].mess  = NULL;
                servers[slot].nMess = -1;
		break;
            
        case LWS_CALLBACK_CLOSED:
              fprintf(stdout, "callback_ui_text: LWS_CALLBACK_CLOSED\n");
              for (n = i = 0; i < servers[slot].nClient; i++) {
                  if (servers[slot].wsi[i] == wsi) {
                    if (servers[slot].mess != NULL) {
                      for (j = 0; j < servers[slot].nMess; j++)
                        wv_free(servers[slot].mess[j]);
                      wv_free(servers[slot].mess);
                    }
                    continue;
                  }
                  servers[slot].wsi[n] = servers[slot].wsi[i];
                  n++;
              }
              servers[slot].nClient--;
              if (n != servers[slot].nClient)
                  fprintf(stderr, "callback_ui_text: Client Problem!\n");
              if (servers[slot].nClient <= 0) {
                  wv_free(servers[slot].wsi);
                  servers[slot].wsi  = NULL;
                  servers[slot].loop = 0;
              }
              break;

	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info((struct lws_tokens *) user);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */

  static struct libwebsocket_protocols wv_protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		0			/* per_session_data_size */
	},
	{
		"gprim-binary-protocol",
		callback_gprim_binary,
		sizeof(struct per_session_data__gprim_binary),
	},
	{
		"ui-text-protocol",
		callback_ui_text,
		sizeof(struct per_session_data__ui_text)
	},
	{
		NULL, NULL, 0		/* End of list */
	}
  };


static void serverThread(wvServer *server)
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 128 +
                          LWS_SEND_BUFFER_POST_PADDING];

	buf[LWS_SEND_BUFFER_PRE_PADDING] = 'x';

	while (server->loop) {

                usleep(50000);
                libwebsocket_service(server->WScontext, 0);

                wv_prepareForSends(server->WVcontext);

		/*
		 * This broadcasts to all gprim-binary-protocol connections
		 *
		 * We're just sending a character 'x', in these examples the
		 * callbacks send their own per-connection content.
		 *
		 * We take care of pre-and-post padding allocation.
		 */

		libwebsockets_broadcast(&wv_protocols[PROTOCOL_GPRIM_BINARY],
                                        &buf[LWS_SEND_BUFFER_PRE_PADDING], 1);
                
                /* clean up after all has been sent */
                wv_finishSends(server->WVcontext);
	}
        
        /* mark the thread as down */
        wv_destroyContext(&server->WVcontext);
        libwebsocket_context_destroy(server->WScontext);
        server->loop = -1;
}


int wv_startServer(int port, char *interface, char *cert_path, char *key_path,
                   int opts, /*@only@*/ wvContext *WVcontext)
{
        int      slot;
        wvServer *tserver;
        struct libwebsocket_context *context;

	fprintf(stdout, "\nwv libwebsockets server thread start\n\n");

	context = libwebsocket_create_context(port, interface, wv_protocols,
				              libwebsocket_internal_extensions,
				              cert_path, key_path, -1, -1, opts);
	if (context == NULL) {
		fprintf(stderr, "libwebsocket init failed!\n");
		return -1;
	}

        for (slot = 0; slot < nServers; slot++)
                if (servers[slot].loop == -1) break;
          
        if (slot == nServers) {
                if (nServers == 0) {
                        tserver = (wvServer *) wv_alloc(sizeof(wvServer));
                } else {
                        tserver = (wvServer *) wv_realloc(servers, 
                                           (nServers+1)*sizeof(wvServer));
                }
                if (tserver == NULL) {
                        libwebsocket_context_destroy(context);
                        fprintf(stderr, "Server malloc failed!\n");
                        return -1;
                }
                servers = tserver;
                nServers++;
        }
        servers[slot].loop      = 1;
        servers[slot].nClient   = 0;
        servers[slot].index     = slot;
        servers[slot].wsi       = NULL;
        servers[slot].WScontext =   context;
        servers[slot].WVcontext = WVcontext;
        servers[slot].nMess     = -1;
        servers[slot].mess      = NULL;
        memset(servers[slot].xbuf, 0, LWS_SEND_BUFFER_PRE_PADDING + BUFLEN +
                                      LWS_SEND_BUFFER_POST_PADDING);

        /* spawn off the server thread */
        if (ThreadCreate(serverThread, &servers[slot]) == -1) {
                libwebsocket_context_destroy(context);
                wv_destroyContext(&WVcontext);
#ifndef __clang_analyzer__
                servers[slot].loop = -1;
#endif
                fprintf(stderr, "thread init failed!\n");
                return -1;
        }

	return slot;
}


void wv_cleanupServers()
{
  int i, j;
  
  /* called by the main thread to shutdown and cleanup all server threads
     if a server is active, make its loop 0 to notify it to stop, then wait */
  for (i = 0; i < nServers; i++) {
    if (servers[i].loop == 1) {
      servers[i].loop = 0;
      /* wait - with a limit */
      for (j = 0; j < 100; j++) {
        if (servers[i].loop == -1) break;
        usleep(5000);
      }
    }
  }

  /* cleanup associated message memory & contexts */
  for (i = 0; i < nServers; i++) {
#ifndef __clang_analyzer__
    if (servers[i].mess != NULL) {
      for (j = 0; j < servers[i].nMess; j++)
        wv_free(servers[i].mess[j]);
      wv_free(servers[i].mess);
    }
    if (servers[i].loop == -1) continue;
    wv_destroyContext(&servers[i].WVcontext);
    libwebsocket_context_destroy(servers[i].WScontext);
#endif
  }
  wv_free(servers);
   servers = NULL;
  nServers = 0;
}


int wv_statusServer(int index)
{
  int loop;

  if ((index < 0) || (index >= nServers)) return -2;
  loop = servers[index].loop;
  if (loop == -1) loop = 0;
  return loop;
}


int wv_nClientServer(int index)
{
  if ((index < 0) || (index >= nServers)) return -2;
  return servers[index].nClient;
}


int wv_activeInterfaces(int index, int *Nwsi, void ***wsi)
{
  *Nwsi = 0;
  *wsi  = NULL;
  if ((index < 0) || (index >= nServers)) return -2;
  
  *Nwsi = servers[index].nClient;
  *wsi  = (void **) servers[index].wsi;
  return 0;
}


void wv_killInterface(int index, void *wsix)
{
  struct libwebsocket *wsi;
  
  if ((index < 0) || (index >= nServers)) return;
  
  wsi = (struct libwebsocket *) wsix;
  libwebsocket_close_and_free_session(servers[index].WScontext, wsi,
                                      LWS_CLOSE_STATUS_GOINGAWAY);
}


int wv_postMessage(int index, char *text)
{
  int  len;
  void *userPtr;
  
  if ((index < 0) || (index >= nServers)) return -2;
  if (text == NULL) return -1;
  if (servers[index].nMess >= 0) return -3;
  
  len     = strlen(text);
  userPtr = servers[index].WVcontext->userPtr;
  if (servers[index].WVcontext->callback == NULL) {
    browserMessage(userPtr, NULL, text, len);
  } else {
    servers[index].WVcontext->callback(userPtr, NULL, text, len);
  }
  
  return 0;
}


int wv_makeMessage(void *wsi, char *text)
{
  int  i, slot, len;
  char **ptrs;

  if (text == NULL)            return -1;
  if (wsi  == NULL)            return -1;
  for (slot = 0; slot < nServers; slot++) {
    for (i = 0; i < servers[slot].nClient; i++)
      if (servers[slot].wsi[i] == wsi) break;
    if (i < servers[slot].nClient) break;
  }
  if (slot == nServers)        return -2;
  if (servers[slot].nMess < 0) return -3;
  
  len = strlen(text) + 1;
  if (servers[slot].mess == NULL) {
    ptrs = (char **) wv_alloc(sizeof(char *));
    servers[slot].nMess = 0;
  } else {
    ptrs = (char **) wv_realloc( servers[slot].mess,
                                (servers[slot].nMess+1)*sizeof(char *));
  }
  if (ptrs == NULL) return -4;
  ptrs[servers[slot].nMess] = (char *) wv_alloc(len*sizeof(char));
  if (ptrs[servers[slot].nMess] == NULL) {
    servers[slot].mess = ptrs;
    return -4;
  }
  for (i = 0; i < len; i++) ptrs[servers[slot].nMess][i] = text[i];
  servers[slot].mess = ptrs;
  servers[slot].nMess++;
  
  return 0;
}


void wv_sendText(struct libwebsocket *wsi, char *text)
{
  int  n;
  unsigned char *message;
  
  if (text == NULL) {
    fprintf(stderr, "ERROR: sendText called with NULL text!");
    return;
  }
  n = strlen(text) + 1;
  message = (unsigned char *) wv_alloc((n+LWS_SEND_BUFFER_PRE_PADDING +
                                        LWS_SEND_BUFFER_POST_PADDING)*
                                        sizeof(unsigned char));
  if (message == NULL) {
    fprintf(stderr, "ERROR: sendText Malloc with length %d!", n);
    return;    
  }
  memcpy(&message[LWS_SEND_BUFFER_PRE_PADDING], text, n);
  n = libwebsocket_write(wsi, &message[LWS_SEND_BUFFER_PRE_PADDING], 
                         n, LWS_WRITE_TEXT);
  if (n < 0) fprintf(stderr, "sendText write failed\n");
  wv_free(message);
}


void wv_broadcastText(char *text)
{
  int  n;
  unsigned char *message;
  
  if (text == NULL) {
    fprintf(stderr, "ERROR: broadcastText called with NULL text!");
    return;
  }
  n = strlen(text) + 1;
  message = (unsigned char *) wv_alloc((n+LWS_SEND_BUFFER_PRE_PADDING +
                                        LWS_SEND_BUFFER_POST_PADDING)*
                                        sizeof(unsigned char));
  if (message == NULL) {
    fprintf(stderr, "ERROR: broadcastText Malloc with length %d!", n);
    return;    
  }
  memcpy(&message[LWS_SEND_BUFFER_PRE_PADDING], text, n);
  libwebsockets_broadcast(&wv_protocols[PROTOCOL_UI_TEXT],
                          &message[LWS_SEND_BUFFER_PRE_PADDING], n);
  wv_free(message);
}


int wv_sendBinaryData(struct libwebsocket *wsi, unsigned char *buf, int iBuf)
{
  return libwebsocket_write(wsi, buf, iBuf, LWS_WRITE_BINARY);
}


#ifdef STANDALONE

static void createBox(wvContext *cntxt, char *name, int attr, float *offset)
{
    int    i, n, attrs;
    wvData items[5];

    // box
    //    v6----- v5
    //   /|      /|
    //  v1------v0|
    //  | |     | |
    //  | |v7---|-|v4
    //  |/      |/
    //  v2------v3
    //
    // vertex coords array
    float vertices[] = {
           1, 1, 1,  -1, 1, 1,  -1,-1, 1,   1,-1, 1,    // v0-v1-v2-v3 front
           1, 1, 1,   1,-1, 1,   1,-1,-1,   1, 1,-1,    // v0-v3-v4-v5 right
           1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,    // v0-v5-v6-v1 top
          -1, 1, 1,  -1, 1,-1,  -1,-1,-1,  -1,-1, 1,    // v1-v6-v7-v2 left
          -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,    // v7-v4-v3-v2 bottom
           1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1 };  // v4-v7-v6-v5 back

    // normal array
    float normals[] = {
           0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1,     // v0-v1-v2-v3 front
           1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0,     // v0-v3-v4-v5 right
           0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0,     // v0-v5-v6-v1 top
          -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,     // v1-v6-v7-v2 left
           0,-1, 0,   0,-1, 0,   0,-1, 0,   0,-1, 0,     // v7-v4-v3-v2 bottom
           0, 0,-1,   0, 0,-1,   0, 0,-1,   0, 0,-1 };   // v4-v7-v6-v5 back

    // color array
    unsigned char colors[] = {
           0, 0, 255,    0, 0, 255,    0, 0, 255,    0, 0, 255,    // v0-v1-v2-v3
           255, 0, 0,    255, 0, 0,    255, 0, 0,    255, 0, 0,    // v0-v3-v4-v5
           0, 255, 0,    0, 255, 0,    0, 255, 0,    0, 255, 0,    // v0-v5-v6-v1
           255, 255, 0,  255, 255, 0,  255, 255, 0,  255, 255, 0,  // v1-v6-v7-v2
           255, 0, 255,  255, 0, 255,  255, 0, 255,  255, 0, 255,  // v7-v4-v3-v2
           0, 255, 255,  0, 255, 255,  0, 255, 255,  0, 255, 255}; // v4-v7-v6-v5

    // index array
    int indices[] = {
           0, 1, 2,   0, 2, 3,    // front
           4, 5, 6,   4, 6, 7,    // right
           8, 9,10,   8,10,11,    // top
          12,13,14,  12,14,15,    // left
          16,17,18,  16,18,19,    // bottom
          20,21,22,  20,22,23 };  // back
          
    // other index array
    int oIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
                       16,17,18,19,20,21,22,23 };
      
    for (i = 0; i < 72; i+=3) {
      vertices[i  ] += offset[0];
      vertices[i+1] += offset[1];
      vertices[i+2] += offset[2];
    }

    if ((i=wv_setData(WV_REAL32, 24, vertices, WV_VERTICES, &items[0])) < 0)
      printf(" wv_setData = %d for %s/item 0!\n", i, name);
    if ((i=wv_setData(WV_INT32,  36, indices, WV_INDICES,   &items[1])) < 0)
      printf(" wv_setData = %d for %s/item 1!\n", i, name);
    if ((i=wv_setData(WV_UINT8,  24, colors, WV_COLORS,     &items[2])) < 0)
      printf(" wv_setData = %d for %s/item 2!\n", i, name);
    if ((i=wv_setData(WV_REAL32, 24, normals, WV_NORMALS,   &items[3])) < 0)
      printf(" wv_setData = %d for %s/item 3!\n", i, name);
    n     = 4;
    attrs = attr;
    if (strcmp(name,"Box#1") == 0) {
      if ((i=wv_setData(WV_INT32, 24, oIndices, WV_PINDICES, &items[4])) < 0)
        printf(" wv_setData = %d for %s/item 4!\n", i, name);
      n++;
      attrs |= WV_POINTS;
    }
    if (strcmp(name,"Box#2") == 0) {
      if ((i=wv_setData(WV_INT32, 24, oIndices, WV_LINDICES, &items[4])) < 0)
        printf(" wv_setData = %d for %s/item 4!\n", i, name);
      n++;
      attrs |= WV_LINES;
    }

    if ((i=wv_addGPrim(cntxt, name, WV_TRIANGLE, attrs, n, items)) < 0)
      printf(" wv_addGPrim = %d for %s!\n", i, name);
}


static void createLines(wvContext *cntxt, char *name, int attr)
{
    int    i;
    wvData items[2];

    // box
    //    v6----- v5
    //   /|      /|
    //  v1------v0|
    //  | |     | |
    //  | |v7---|-|v4
    //  |/      |/
    //  v2------v3
    //
    // vertex coords array
    float vertices[] = {
           1, 1, 1,  -1, 1, 1,  -1,-1, 1,   1,-1, 1,    // v0-v1-v2-v3 front
           1, 1, 1,   1,-1, 1,   1,-1,-1,   1, 1,-1,    // v0-v3-v4-v5 right
           1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,    // v0-v5-v6-v1 top
          -1, 1, 1,  -1, 1,-1,  -1,-1,-1,  -1,-1, 1,    // v1-v6-v7-v2 left
          -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,    // v7-v4-v3-v2 bottom
           1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1 };  // v4-v7-v6-v5 back

    // index array
    int indices[] = {
           0, 1,  1, 2,  2, 3,  3, 0,   // front
           4, 5,  5, 6,  6, 7,  7, 4,   // right
           8, 9,  9,10, 10,11, 11, 8,   // top
          12,13, 13,14, 14,15, 15,12,   // left
          16,17, 17,18, 18,19, 19,16,   // bottom
          20,21, 21,22, 22,23, 23,20 }; // back

    if ((i=wv_setData(WV_REAL32, 24, vertices, WV_VERTICES, &items[0])) < 0)
      printf(" wv_setData = %d for %s/item 0!\n", i, name);
    if ((i=wv_setData(WV_INT32,  48, indices, WV_INDICES,   &items[1])) < 0)
      printf(" wv_setData = %d for %s/item 1!\n", i, name);
               
    if ((i=wv_addGPrim(cntxt, name, WV_LINE, attr, 2, items)) < 0)
      printf(" wv_addGPrim = %d for %s!\n", i, name);
}

static void createPoints(wvContext *cntxt, char *name, int attr, float *offset)
{
    int    i;
    wvData items[2];
    float  colors[3] = {0.6, 0.6, 0.6};

    // box
    //    v6----- v5
    //   /|      /|
    //  v1------v0|
    //  | |     | |
    //  | |v7---|-|v4
    //  |/      |/
    //  v2------v3
    //
    // vertex coords array
    float vertices[] = {
           1, 1, 1,  -1, 1, 1,  -1,-1, 1,   1,-1, 1,    // v0-v1-v2-v3 front
           1, 1, 1,   1,-1, 1,   1,-1,-1,   1, 1,-1,    // v0-v3-v4-v5 right
           1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,    // v0-v5-v6-v1 top
          -1, 1, 1,  -1, 1,-1,  -1,-1,-1,  -1,-1, 1,    // v1-v6-v7-v2 left
          -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,    // v7-v4-v3-v2 bottom
           1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1 };  // v4-v7-v6-v5 back
      
    for (i = 0; i < 72; i+=3) {
      vertices[i  ] += offset[0];
      vertices[i+1] += offset[1];
      vertices[i+2] += offset[2];
    }

    if ((i=wv_setData(WV_REAL32, 24, vertices, WV_VERTICES, &items[0])) < 0)
      printf(" wv_setData = %d for %s/item 0!\n", i, name);
    /* set a uniform color (len == 1) */
    if ((i= wv_setData(WV_REAL32, 1, colors, WV_COLORS, &items[1])) < 0)
      printf(" wv_setData = %d for %s/item 1!\n", i, name);

    if ((i=wv_addGPrim(cntxt, name, WV_POINT, attr, 2, items)) < 0)
      printf(" wv_addGPrim = %d for %s!\n", i, name);

}


int main(/*@unused@*/ int argc, /*@unused@*/ char **argv)
{
        int       cnt = 1;
        wvContext *cntxt;
        float     eye[3]    = {0.0, 0.0, 7.0};
        float     center[3] = {0.0, 0.0, 0.0};
        float     up[3]     = {0.0, 1.0, 0.0};
        float     offset[3] = {0.0, 0.0, 0.0};
        
        // create the WebViewer context

        cntxt = wv_createContext(0, 30.0, 1.0, 10.0, eye, center, up);
        if (cntxt == NULL) {
                printf(" failed to create wvContext!\n");
                return -1;
        }
        
        // make the scene

        createBox(cntxt, "Box#1", WV_ON|WV_SHADING|WV_ORIENTATION, offset);

        offset[0] = offset[1] = offset[2] =  0.1;
        createBox(cntxt, "Box#2", WV_ON|WV_TRANSPARENT, offset);
        
        createLines(cntxt, "Lines", WV_ON);
        
        offset[0] = offset[1] = offset[2] = -0.1;
        createPoints(cntxt, "Points", WV_ON, offset);

        // start the server code

        if (wv_startServer(7681, NULL, NULL, NULL, 0, cntxt) == 0) {

                /* we have a single valid server -- do nothing */
                while (wv_statusServer(0)) {
                      usleep(500000);
                      if (cnt%25 == 0) wv_postMessage(0, "sent Message");
                      cnt++;
                }

        }
        
        wv_cleanupServers();
        return 0;
}

#endif
