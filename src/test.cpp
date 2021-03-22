#include "web.h"
#include "event2/event.h"
#include "event2/http.h"
#include <unistd.h>
// 如果是cpp,需要添加这两个
#include <memory.h>
#include <stdlib.h>

// 编译命令
// g++ test.cpp web.c -o http-server -levent -O0 -std=c++0x -Wall -g2 -ggdb -pthread  -lstdc++ -fpermissive

struct options {
	int port;
	int iocp;
	int verbose;

	int unlink;
	const char *unixsock;
	const char *docroot;
};

static const struct table_entry {
	const char *extension;
	const char *content_type;
} content_type_table[] = {
	{ "txt", "text/plain" },
	{ "c", "text/plain" },
	{ "h", "text/plain" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ NULL, NULL },
};

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void dump_request_cb(struct evhttp_request *req, void *arg)
{
	web_put_header(req, "Content-Type", "text/plain");

	char psMsg[1024 * 1024 * 2];
	memset(psMsg, 0, 1024 * 1024 * 2);
	if(!web_read(req, psMsg, 1024 * 1024 * 2)){
		printf("Error web_read\n");
		web_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	if(!web_write(req, "asdfadf")){
		web_error(req, HTTP_BADREQUEST, 0);
		return;
	}
	web_write(req, "asdfadf");
	web_write(req, "asdfadf");
	web_write(req, "asdfadf");
	web_write(req, "asdfadf");

	puts(psMsg);
	web_put_status(req, 200, "OK");
	return;
}


static void print_usage(FILE *out, const char *prog, int exit_code)
{
	fprintf(out,
		"Syntax: %s [ OPTS ] <docroot>\n"
		" -p      - port\n"
		" -U      - bind to unix socket\n"
		" -u      - unlink unix socket before bind\n"
		" -I      - IOCP\n"
		" -v      - verbosity, enables libevent debug logging too\n", prog);
	exit(exit_code);
}

static struct options parse_opts(int argc, char **argv)
{
	struct options o;
	int opt;

	memset(&o, 0, sizeof(o));

	while ((opt = getopt(argc, argv, "hp:U:uIv")) != -1) {
		switch (opt) {
			case 'p': o.port = atoi(optarg); break;
			case 'U': o.unixsock = optarg; break;
			case 'u': o.unlink = 1; break;
			case 'I': o.iocp = 1; break;
			case 'v': ++o.verbose; break;
			case 'h': print_usage(stdout, argv[0], 0); break;
			default : fprintf(stderr, "Unknown option %c\n", opt); break;
		}
	}

	if (optind >= argc || (argc - optind) > 1) {
		print_usage(stdout, argv[0], 1);
	}
	o.docroot = argv[optind];

	return o;
}

int main(int argc, char **argv)
{
	struct event_config *cfg = NULL;
	struct event_base *base = NULL;
	struct evhttp *http = NULL;
	struct evhttp_bound_socket *handle = NULL;
	struct evconnlistener *lev = NULL;
	struct event *term = NULL;

	struct options o = parse_opts(argc, argv);
	// printf("%s", o.port);
	web_init();
	web_add_service("/dump", dump_request_cb);
	web_start(9999);
	
	return 0;
}