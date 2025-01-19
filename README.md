# Name

`ngx_http_proxy_var_set_module` allows setting the variable to the given value during processing of proxy response.

# Table of Content

- [Name](#name)
- [Table of Content](#table-of-content)
- [Status](#status)
- [Synopsis](#synopsis)
- [Installation](#installation)
- [Directives](#directives)
  - [proxy\_var\_set](#proxy_var_set)
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
        proxy_var_set $no_cache $upstream_http_custom_header1;
        proxy_no_cache $no_cache;
        proxy_pass http://example.upstream.com;
    }
}
```

# Installation

To use theses modules, configure your nginx branch with `--add-module=/path/to/ngx_http_proxy_var_set_module`.

# Directives

## proxy_var_set

**Syntax:** *proxy_var_set $variable value;*

**Default:** *-*

**Context:** *http, server, location*

Sets the request variable to the given value during processing of proxy response. The value may contain variables from request or response, such as $upstream_http_*.
These directives are inherited from the previous configuration level only when there is no directive for the same variable defined at the current level.

# Author

Hanada im@hanada.info

# License

This Nginx module is licensed under [BSD 2-Clause License](LICENSE).
