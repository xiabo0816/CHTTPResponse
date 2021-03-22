#ifndef _WEB_H_
#define _WEB_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct evhttp_request evhttp_request;
extern int web_init();
extern int web_add_service(char* path, void* request_callback);
extern int web_start(int port);
extern void web_final();
extern int web_read(struct evhttp_request *req, char* output_buffer, int output_buffer_size);
extern int web_write(struct evhttp_request *req, char* buffer);
extern int web_put_status(struct evhttp_request *req, int status, char* msg);
extern int web_put_header(struct evhttp_request *req, char* name, char* value);
extern int web_error(struct evhttp_request *req, int error, const char *reason);

/******************************************************************************
 *
 * thread support for ts.c
 *
 *****************************************************************************/
typedef struct mutex_t mutex_t;
extern mutex_t* mutex_create(void);
extern void mutex_destroy(mutex_t*);
extern void mutex_secure(mutex_t*);
extern void mutex_release(mutex_t*);
typedef struct thread_t thread_t;
typedef void thread_proc_t(void* arg);
extern thread_t* thread_create(thread_proc_t* tp, void* arg);

#ifdef __cplusplus
}
#endif

#endif /* _WEB_H_ */
