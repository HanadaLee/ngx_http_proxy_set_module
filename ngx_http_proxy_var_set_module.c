
/*
 * Copyright (C) Hanada
 */


#include "ngx_http_proxy_var_set_module.h"


typedef struct {
    ngx_int_t                  index;
    ngx_http_complex_value_t   value;
    ngx_http_set_variable_pt   set_handler;
    ngx_http_complex_value_t  *filter;
    ngx_int_t                  negative;
} ngx_http_proxy_var_set_variable_t;


typedef struct {
    ngx_array_t               *vars;
    ngx_array_t               *grpc_vars;
} ngx_http_proxy_var_set_loc_conf_t;


static char *ngx_http_proxy_var_set(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_grpc_var_set(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static ngx_int_t ngx_http_proxy_var_set_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static void *ngx_http_proxy_var_set_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_proxy_var_set_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);


static ngx_command_t  ngx_http_proxy_var_set_commands[] = {

    { ngx_string("proxy_var_set"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE23,
      ngx_http_proxy_var_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("grpc_var_set"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE23,
      ngx_http_grpc_var_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /* TODO: support other upstream content handlers */

      ngx_null_command
};


static ngx_http_module_t  ngx_http_proxy_var_set_module_ctx = {
    NULL,                                     /* preconfiguration */
    NULL,                                     /* postconfiguration */

    NULL,                                     /* create main conf */
    NULL,                                     /* init main conf */

    NULL,                                     /* create srv conf */
    NULL,                                     /* merge srv conf */

    ngx_http_proxy_var_set_create_loc_conf,   /* create loc conf */
    ngx_http_proxy_var_set_merge_loc_conf     /* merge loc conf */
};


ngx_module_t  ngx_http_proxy_var_set_module = {
    NGX_MODULE_V1,
    &ngx_http_proxy_var_set_module_ctx,       /* module context */
    ngx_http_proxy_var_set_commands,          /* module directives */
    NGX_HTTP_MODULE,                          /* module type */
    NULL,                                     /* init master */
    NULL,                                     /* init module */
    NULL,                                     /* init process */
    NULL,                                     /* init thread */
    NULL,                                     /* exit thread */
    NULL,                                     /* exit process */
    NULL,                                     /* exit master */
    NGX_MODULE_V1_PADDING
};


ngx_int_t
ngx_http_proxy_var_set_handler(ngx_http_request_t *r)
{
    ngx_str_t                            val;
    ngx_http_variable_t                 *v;
    ngx_http_variable_value_t           *vv;
    ngx_http_proxy_var_set_loc_conf_t   *plcf;
    ngx_http_proxy_var_set_variable_t   *pv, *last;
    ngx_http_core_main_conf_t           *cmcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "proxy var set handler");

    plcf = ngx_http_get_module_loc_conf(r, ngx_http_proxy_var_set_module);

    if (plcf->vars == NULL) {
        return NGX_OK;
    }

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    v = cmcf->variables.elts;

    pv = plcf->vars->elts;
    last = pv + plcf->vars->nelts;

    while (pv < last) {

        if (pv->filter) {
            if (ngx_http_complex_value(r, pv->filter, &val)
                    != NGX_OK) {
                return NGX_ERROR;
            }

            if (val.len == 0 || (val.len == 1 && val.data[0] == '0')) {
                if (!pv->negative) {
                    continue;
                }
            } else {
                if (pv->negative) {
                    continue;
                }
            }
        }

        /*
         * explicitly set new value to make sure it will be available after
         * internal redirects
         */

        vv = &r->variables[pv->index];

        if (ngx_http_complex_value(r, &pv->value, &val) != NGX_OK) {
            return NGX_ERROR;
        }

        vv->valid = 1;
        vv->not_found = 0;
        vv->data = val.data;
        vv->len = val.len;

        if (pv->set_handler) {
            /*
             * set_handler only available in cmcf->variables_keys, so we store
             * it explicitly
             */

            pv->set_handler(r, vv, v[pv->index].data);
        }

        pv++;
    }

    return NGX_OK;
}


ngx_int_t
ngx_http_grpc_var_set_handler(ngx_http_request_t *r)
{
    ngx_str_t                            val;
    ngx_http_variable_t                 *v;
    ngx_http_variable_value_t           *vv;
    ngx_http_proxy_var_set_loc_conf_t   *plcf;
    ngx_http_proxy_var_set_variable_t   *pv, *last;
    ngx_http_core_main_conf_t           *cmcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "grpc var set handler");

    plcf = ngx_http_get_module_loc_conf(r, ngx_http_proxy_var_set_module);

    if (plcf->grpc_vars == NULL) {
        return NGX_OK;
    }

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    v = cmcf->variables.elts;

    pv = plcf->grpc_vars->elts;
    last = pv + plcf->grpc_vars->nelts;

    while (pv < last) {

        if (pv->filter) {
            if (ngx_http_complex_value(r, pv->filter, &val)
                    != NGX_OK) {
                return NGX_ERROR;
            }

            if (val.len == 0 || (val.len == 1 && val.data[0] == '0')) {
                if (!pv->negative) {
                    continue;
                }
            } else {
                if (pv->negative) {
                    continue;
                }
            }
        }

        /*
         * explicitly set new value to make sure it will be available after
         * internal redirects
         */

        vv = &r->variables[pv->index];

        if (ngx_http_complex_value(r, &pv->value, &val) != NGX_OK) {
            return NGX_ERROR;
        }

        vv->valid = 1;
        vv->not_found = 0;
        vv->data = val.data;
        vv->len = val.len;

        if (pv->set_handler) {
            /*
             * set_handler only available in cmcf->variables_keys, so we store
             * it explicitly
             */

            pv->set_handler(r, vv, v[pv->index].data);
        }

        pv++;
    }

    return NGX_OK;
}


static char *
ngx_http_proxy_var_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_var_set_loc_conf_t *plcf = conf;

    ngx_str_t                           *value;
    ngx_http_variable_t                 *v;
    ngx_http_proxy_var_set_variable_t   *pv;
    ngx_str_t                            s;
    ngx_http_compile_complex_value_t     ccv;

    value = cf->args->elts;

    if (value[1].data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    value[1].len--;
    value[1].data++;

    if (plcf->vars == NGX_CONF_UNSET_PTR) {
        plcf->vars = ngx_array_create(cf->pool, 1,
                                    sizeof(ngx_http_proxy_var_set_variable_t));
        if (plcf->vars == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    pv = ngx_array_push(plcf->vars);
    if (pv == NULL) {
        return NGX_CONF_ERROR;
    }

    v = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (v == NULL) {
        return NGX_CONF_ERROR;
    }

    pv->index = ngx_http_get_variable_index(cf, &value[1]);
    if (pv->index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    if (v->get_handler == NULL) {
        v->get_handler = ngx_http_proxy_var_set_variable;
        v->data = (uintptr_t) pv;
    }

    pv->set_handler = v->set_handler;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &pv->value;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts == 4) {

        if (ngx_strncmp(value[3].data, "if=", 3) == 0) {
            s.len = value[3].len - 3;
            s.data = value[3].data + 3;
            pv->negative = 0;

        } else if (ngx_strncmp(value[3].data, "if!=", 4) == 0) {
            s.len = value[3].len - 4;
            s.data = value[3].data + 4;
            pv->negative = 1;

        } else {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                "invalid parameter \"%V\"", &value[3]);
            return NGX_CONF_ERROR;
        }

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &s;
        ccv.complex_value = ngx_palloc(cf->pool,
                                    sizeof(ngx_http_complex_value_t));
        if (ccv.complex_value == NULL) {
            return NGX_CONF_ERROR;
        }

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        pv->filter = ccv.complex_value;

    } else {
        pv->negative = 0;
        pv->filter = NULL;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_grpc_var_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_var_set_loc_conf_t *plcf = conf;

    ngx_str_t                           *value;
    ngx_http_variable_t                 *v;
    ngx_http_proxy_var_set_variable_t   *pv;
    ngx_str_t                            s;
    ngx_http_compile_complex_value_t     ccv;

    value = cf->args->elts;

    if (value[1].data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    value[1].len--;
    value[1].data++;

    if (plcf->grpc_vars == NGX_CONF_UNSET_PTR) {
        plcf->grpc_vars = ngx_array_create(cf->pool, 1,
                                    sizeof(ngx_http_proxy_var_set_variable_t));
        if (plcf->grpc_vars == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    pv = ngx_array_push(plcf->grpc_vars);
    if (pv == NULL) {
        return NGX_CONF_ERROR;
    }

    v = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (v == NULL) {
        return NGX_CONF_ERROR;
    }

    pv->index = ngx_http_get_variable_index(cf, &value[1]);
    if (pv->index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    if (v->get_handler == NULL) {
        v->get_handler = ngx_http_proxy_var_set_variable;
        v->data = (uintptr_t) pv;
    }

    pv->set_handler = v->set_handler;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &pv->value;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts == 4) {

        if (ngx_strncmp(value[3].data, "if=", 3) == 0) {
            s.len = value[3].len - 3;
            s.data = value[3].data + 3;
            pv->negative = 0;

        } else if (ngx_strncmp(value[3].data, "if!=", 4) == 0) {
            s.len = value[3].len - 4;
            s.data = value[3].data + 4;
            pv->negative = 1;

        } else {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                "invalid parameter \"%V\"", &value[3]);
            return NGX_CONF_ERROR;
        }

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &s;
        ccv.complex_value = ngx_palloc(cf->pool,
                                    sizeof(ngx_http_complex_value_t));
        if (ccv.complex_value == NULL) {
            return NGX_CONF_ERROR;
        }

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        pv->filter = ccv.complex_value;

    } else {
        pv->negative = 0;
        pv->filter = NULL;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_proxy_var_set_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "proxy / grpc var set variable");

    v->not_found = 1;

    return NGX_OK;
}


static void *
ngx_http_proxy_var_set_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_proxy_var_set_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_proxy_var_set_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->vars = NGX_CONF_UNSET_PTR;
    conf->grpc_vars = NGX_CONF_UNSET_PTR;

    return conf;
}


static char *
ngx_http_proxy_var_set_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child)
{
    ngx_http_proxy_var_set_loc_conf_t  *prev = parent;
    ngx_http_proxy_var_set_loc_conf_t  *conf = child;

    ngx_http_proxy_var_set_variable_t  *pvars, *cvars, *new_var;
    ngx_uint_t                          i, j, found;

    if (conf->vars == NGX_CONF_UNSET_PTR) {
        conf->vars = (prev->vars == NGX_CONF_UNSET_PTR) ? NULL : prev->vars;

    } else if (prev->vars != NGX_CONF_UNSET_PTR && prev->vars != NULL) {

        pvars = prev->vars->elts;
        cvars = conf->vars->elts;

        for (i = 0; i < prev->vars->nelts; i++) {

            found = 0;

            for (j = 0; j < conf->vars->nelts; j++) {
                if (cvars[j].index == pvars[i].index) {
                    found = 1;
                    break;
                }
            }

            if (!found) {

                new_var = ngx_array_push(conf->vars);
                if (new_var == NULL) {
                    return NGX_CONF_ERROR;
                }

                *new_var = pvars[i];
            }
        }
    }

    if (conf->grpc_vars == NGX_CONF_UNSET_PTR) {
        conf->grpc_vars = (prev->vars == NGX_CONF_UNSET_PTR) ? NULL : prev->vars;

    } else if (prev->vars != NGX_CONF_UNSET_PTR && prev->vars != NULL) {

        pvars = prev->vars->elts;
        cvars = conf->grpc_vars->elts;

        for (i = 0; i < prev->vars->nelts; i++) {

            found = 0;

            for (j = 0; j < conf->grpc_vars->nelts; j++) {
                if (cvars[j].index == pvars[i].index) {
                    found = 1;
                    break;
                }
            }

            if (!found) {

                new_var = ngx_array_push(conf->grpc_vars);
                if (new_var == NULL) {
                    return NGX_CONF_ERROR;
                }

                *new_var = pvars[i];
            }
        }
    }

    return NGX_CONF_OK;
}