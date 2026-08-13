# Name

`ngx_http_proxy_set_module` allows setting variables during the proxy response header phase.

# Table of Content

- [Name](#name)
- [Table of Content](#table-of-content)
- [Status](#status)
- [Synopsis](#synopsis)
- [Installation](#installation)
- [Conditional syntax](#conditional-syntax)
- [Directives](#directives)
  - [proxy\_set](#proxy_set)
- [Author](#author)
- [License](#license)

# Status

This Nginx module is currently considered experimental. Issues and PRs are welcome if you encounter any problems.

# Synopsis

```nginx
server {
    listen 127.0.0.1:80;
    server_name localhost;

    location / {
        set $no_cache "";
        condition has_no_cache is_not_empty $upstream_http_custom_header1;
        when has_no_cache {
            proxy_set $no_cache $upstream_http_custom_header1;
        }
        proxy_no_cache $no_cache;
        proxy_pass http://example.upstream.com;
    }
}
```

# Installation

This module depends on `ngx_http_proxy_filter_module`; add the proxy filter module first:

```sh
./configure --add-module=/path/to/ngx_http_proxy_filter_module \
            --add-module=/path/to/ngx_http_proxy_set_module
```

To enable named conditions, add `ngx_condition_module` statically in the same nginx configuration.

# Conditional syntax

Conditional syntax is selected at compile time. With `ngx_condition_module`, place `proxy_set` inside an `http`, `server`, or `location` `when` block; `if=` and `if!=` are rejected. Without it, `when` is unavailable and legacy `if=`/`if!=` remain supported. A rule whose condition does not match is skipped so the next definition of the same variable can be evaluated.

# Directives

## proxy_set

**Syntax:** *proxy_set $variable value;*

**Default:** *-*

**Context:** *http, server, location, http when, server when, location when*

Sets the request variable to the given value during the proxy response header phase. The value may contain variables from request or response, such as `$upstream_http_*`.
These directives are inherited from the previous configuration level only when there is no directive for the same variable defined at the current level.

# Author

Hanada im@hanada.info

# License

This Nginx module is licensed under [BSD 2-Clause License](LICENSE).
