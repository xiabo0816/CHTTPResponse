/*
  A trivial static http webserver using Libevent's evhttp.

  This is not the best code in the world, and it does some fairly stupid stuff
  that you would never want to do in a production webserver. Caveat hackor!

 */

/* Compatibility for possible missing IPv6 declarations */
#include "web.h"

#include "event2/util-internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <getopt.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else /* !_WIN32 */
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif /* _WIN32 */
#include <signal.h>

#ifdef EVENT__HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef EVENT__HAVE_AFUNIX_H
#include <afunix.h>
#endif

#include "event2/event.h"
#include "event2/http.h"
#include "event2/listener.h"
#include "event2/buffer.h"
#include "event2/util.h"
#include "event2/keyvalq_struct.h"
#include "event2/http_struct.h"

#ifdef _WIN32
#include "event2/thread.h"
#endif /* _WIN32 */

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif

#ifdef _WIN32
#ifndef stat
#define stat _stat
#endif
#ifndef fstat
#define fstat _fstat
#endif
#ifndef open
#define open _open
#endif
#ifndef close
#define close _close
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#endif /* _WIN32 */

char g_uri_root[512];

/*
 *  Control block definition
 */
struct http_server {
	struct event_config *cfg;
	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;
	struct evconnlistener *lev;
	struct event *term;
};

static struct http_server g_http_server = { 0 };

int web_read(struct evhttp_request *req, char* output_buffer, int output_buffer_size){
	#define MYDEBUG 1

	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;

#ifdef MYDEBUG
	switch (evhttp_request_get_command(req)) {
	case EVHTTP_REQ_GET: cmdtype = "GET"; break;
	case EVHTTP_REQ_POST: cmdtype = "POST"; break;
	case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
	case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
	case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
	case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
	case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
	case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
	case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
	default: cmdtype = "unknown"; break;
	}

	printf("Received a %s request for %s\nHeaders:\n", cmdtype, evhttp_request_get_uri(req));
	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
		header = header->next.tqe_next) {
		printf("  %s: %s\n", header->key, header->value);
	}
#endif

	// size_t buffer_length = evbuffer_get_length(buf);
	// size_t buffer_len = evbuffer_get_length(buf);
	buf = evhttp_request_get_input_buffer(req);
	if (output_buffer_size < evbuffer_get_length(buf)){
		printf("Error: output_buffer_size(%s) < buffer_length(%s)\n", output_buffer_size, evbuffer_get_length(buf));
		return 0;
	}
	
	evbuffer_remove(buf, output_buffer, output_buffer_size - 1);
	// printf("%s\n", output_buffer);
	return 1;
}

int web_write(struct evhttp_request *req, char* buffer){
	struct evbuffer *evb = NULL;
	/* This holds the content we're sending. */
	evb = evbuffer_new();
	evbuffer_add_printf(evb, buffer);
	evbuffer_add_buffer(req->output_buffer, evb);
	evbuffer_free(evb);
	return 1;
}

int web_put_status(struct evhttp_request *req, int status, char* msg){
	evhttp_send_reply(req, status, msg, NULL);
}

int web_put_header(struct evhttp_request *req, char* name, char* value){
	evhttp_add_header(evhttp_request_get_output_headers(req), name, value);
}

int web_end_header(struct evhttp_request *req){

}

/*
*  Initialize the http service
*/
int web_init()
{
	g_http_server.cfg = NULL;
	g_http_server.base = NULL;
	g_http_server.http = NULL;
	g_http_server.handle = NULL;
	g_http_server.lev = NULL;
	g_http_server.term = NULL;

#ifdef _WIN32
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD(2, 2);
		WSAStartup(wVersionRequested, &wsaData);
	}
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		return 0;
	}
#endif

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	/** Read env like in regress */
	// event_enable_debug_logging(EVENT_DBG_ALL);

	g_http_server.cfg = event_config_new();
#ifdef _WIN32
	if (o.iocp) {
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
		evthread_use_windows_threads();
		event_config_set_num_cpus_hint(cfg, 8);
#endif
		event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
	}
#endif

	g_http_server.base = event_base_new_with_config(g_http_server.cfg);
	if (!g_http_server.base) {
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		return 0;
	}
	event_config_free(g_http_server.cfg);
	g_http_server.cfg = NULL;

	/* Create a new evhttp object to handle requests. */
	g_http_server.http = evhttp_new(g_http_server.base);
	if (!g_http_server.http) {
		fprintf(stderr, "couldn't create evhttp. Exiting.\n");
		return 0;
	}

	return 1;
}

/*
*  Add a new service
*/
int web_add_service(char* path, void* request_callback)
{
	/* The /dump URI will dump all requests to stdout and say 200 ok. */
	evhttp_set_cb(g_http_server.http, path, request_callback, NULL);
	/* We want to accept arbitrary requests, so we need to set a "generic"
	 * cb.  We can also add callbacks for specific paths. */
	// evhttp_set_gencb(g_http_server.http, send_document_cb, &o);
	return 1;
}

static void do_term(int sig, short events, void *arg)
{
	struct event_base *base = arg;
	event_base_loopbreak(base);
	fprintf(stderr, "Got %i, Terminating\n", sig);
}


static int display_listen_sock(struct evhttp_bound_socket *handle)
{
	struct sockaddr_storage ss;
	evutil_socket_t fd;
	ev_socklen_t socklen = sizeof(ss);
	char addrbuf[128];
	void *inaddr;
	const char *addr;
	int got_port = -1;

	fd = evhttp_bound_socket_get_fd(handle);
	memset(&ss, 0, sizeof(ss));
	if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
		perror("getsockname() failed");
		return 1;
	}

	if (ss.ss_family == AF_INET) {
		got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
		inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
	} else if (ss.ss_family == AF_INET6) {
		got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
		inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
	}
#ifdef EVENT__HAVE_STRUCT_SOCKADDR_UN
	else if (ss.ss_family == AF_UNIX) {
		printf("Listening on <%s>\n", ((struct sockaddr_un*)&ss)->sun_path);
		return 0;
	}
#endif
	else {
		fprintf(stderr, "Weird address family %d\n",
		    ss.ss_family);
		return 1;
	}

	addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf, sizeof(addrbuf));
	if (addr) {
		printf("Listening on %s:%d\n", addr, got_port);
		evutil_snprintf(g_uri_root, sizeof(g_uri_root),
		    "http://%s:%d",addr,got_port);
	} else {
		fprintf(stderr, "evutil_inet_ntop failed\n");
		return 1;
	}

	return 0;
}

/*
*  Start running
*/
int web_start(int port)
{
	g_http_server.handle = evhttp_bind_socket_with_handle(g_http_server.http, "0.0.0.0", port);
	if (!g_http_server.handle) {
		fprintf(stderr, "couldn't bind to port %d. Exiting.\n", port);
		return 0;
	}

	if (display_listen_sock(g_http_server.handle)) {
		return 0;
	}

	g_http_server.term = evsignal_new(g_http_server.base, SIGINT, do_term, g_http_server.base);
	if (!g_http_server.term)
		return 0;
	if (event_add(g_http_server.term, NULL))
		return 0;

	return event_base_dispatch(g_http_server.base);
}

int web_error(struct evhttp_request *req, int error, const char *reason){
	evhttp_send_error(req, error, reason);
	return 1;
}

/*
*  Finalize ws services
*/
void web_final()
{
	
#ifdef _WIN32
	WSACleanup();
#endif

	if (g_http_server.cfg)
		event_config_free(g_http_server.cfg);
	if (g_http_server.http)
		evhttp_free(g_http_server.http);
	if (g_http_server.term)
		event_free(g_http_server.term);
	if (g_http_server.base)
		event_base_free(g_http_server.base);

}

