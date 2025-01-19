
/*
 * Copyright (C) Hanada
 */


#ifndef _NGX_HTTP_PROXY_VAR_SET_MODULE_H_INCLUDED_
#define _NGX_HTTP_PROXY_VAR_SET_MODULE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_int_t ngx_http_proxy_var_set_handler(ngx_http_request_t *r);
ngx_int_t ngx_http_grpc_var_set_handler(ngx_http_request_t *r);

#endif /* _NGX_HTTP_PROXY_VAR_SET_MODULE_H_INCLUDED_ */