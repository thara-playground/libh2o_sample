#include <signal.h>
#include <stdio.h>

#include "h2o.h"

static h2o_globalconf_t config;
static h2o_context_t ctx;
static h2o_accept_ctx_t accept_ctx;

static int hello(h2o_handler_t *self, h2o_req_t *req);
static int create_listener(uint32_t port);
static void on_accept(uv_stream_t *listener, int status);

int main() {
    signal(SIGPIPE, SIG_IGN);

    h2o_config_init(&config);

    h2o_hostconf_t* hostconf = h2o_config_register_host(&config, h2o_iovec_init(H2O_STRLIT("default")), 65535);

    char* path = "/hello";

    h2o_pathconf_t* pathconf = h2o_config_register_path(hostconf, path, 0);
    h2o_handler_t* handler;
    handler = h2o_create_handler(pathconf, sizeof(*handler));
    handler->on_req = hello;

    uv_loop_t loop;
    uv_loop_init(&loop);
    h2o_context_init(&ctx, &loop, &config);

    accept_ctx.ctx = &ctx;
    accept_ctx.hosts = config.hosts;

    if (create_listener(8080) != 0) {
        fprintf(stderr, "failed to listen to 127.0.0.1:8080:%s\n", strerror(errno));
        return 1;
    }

    uv_run(ctx.loop, UV_RUN_DEFAULT);

    return 0;
}

static int hello(h2o_handler_t *self, h2o_req_t *req) {
    static h2o_generator_t generator = { NULL, NULL };

    h2o_iovec_t body = h2o_strdup(&req->pool, "hello world\n", SIZE_MAX);

    req->res.status = 200;
    req->res.reason = "OK";
    req->res.content_length = body.len;
    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE, NULL, H2O_STRLIT("text/plain; charset=utf-8"));

    h2o_start_response(req, &generator);
    h2o_send(req, &body, 1, 1);

    return 0;
}

static int create_listener(uint32_t port) {
    static uv_tcp_t listener;
    struct sockaddr_in addr;
    int r;

    uv_tcp_init(ctx.loop, &listener);
    uv_ip4_addr("127.0.0.1", port, &addr);
    if ((r = uv_tcp_bind(&listener, (struct sockaddr *)&addr, 0)) != 0) {
        fprintf(stderr, "uv_tcp_bind:%s\n", uv_strerror(r));
        goto ERROR;
    }
    if ((r = uv_listen((uv_stream_t *)&listener, 128, on_accept)) != 0) {
        fprintf(stderr, "uv_listen:%s\n", uv_strerror(r));
        goto ERROR;
    }

    return 0;

ERROR:
    uv_close((uv_handle_t *)&listener, NULL);
    return r;
}

static void on_accept(uv_stream_t *listener, int status) {
    if (status != 0)
        return;

    uv_tcp_t* conn = h2o_mem_alloc(sizeof(*conn));
    uv_tcp_init(listener->loop, conn);

    if (uv_accept(listener, (uv_stream_t *)conn) != 0) {
        uv_close((uv_handle_t *)conn, (uv_close_cb)free);
        return;
    }

    h2o_socket_t* sock = h2o_uv_socket_create((uv_stream_t *)conn, (uv_close_cb)free);
    h2o_accept(&accept_ctx, sock);
}