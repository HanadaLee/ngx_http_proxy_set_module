#!/usr/bin/perl

# Tests for proxy_set with ngx_condition_module.

###############################################################################

use warnings;
use strict;

use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use Test::Nginx qw/ :DEFAULT /;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->has(qw/http proxy rewrite ngx_condition_module
	ngx_http_proxy_filter_module ngx_http_proxy_set_module/)->plan(17);

$t->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    server {
        listen       127.0.0.1:8081;
        server_name  backend;

        add_header X-Source $arg_source always;
        add_header X-Other fixed always;

        location / {
            return 200 backend;
        }
    }

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        condition special str_eq $arg_mode special;
        condition upstream_special str_eq $upstream_http_x_source special;

        location = /basic {
            set $existing initial;
            proxy_set $from_upstream "$upstream_http_x_source:$arg_suffix";
            proxy_set $existing updated-$upstream_http_x_other;
            proxy_set $args changed=yes;
            add_header X-From-Upstream $from_upstream always;
            add_header X-Existing $existing always;
            add_header X-Args $args always;
            proxy_pass http://127.0.0.1:8081;
        }

        location = /conditional {
            set $selected initial;

            when special {
                proxy_set $selected condition;
            }

            proxy_set $selected fallback;
            add_header X-Selected $selected always;
            proxy_pass http://127.0.0.1:8081;
        }

        location = /order {
            set $selected initial;
            proxy_set $selected first;

            when special {
                proxy_set $selected second;
            }

            add_header X-Selected $selected always;
            proxy_pass http://127.0.0.1:8081;
        }

        location = /upstream-condition {
            set $selected initial;

            when upstream_special {
                proxy_set $selected condition;
            }

            proxy_set $selected fallback;
            add_header X-Selected $selected always;
            proxy_pass http://127.0.0.1:8081;
        }
    }

    server {
        listen       127.0.0.1:8082;
        server_name  inheritance;

        condition child_selected str_eq $arg_mode special;

        set $inherited initial;
        set $unrelated initial;
        proxy_set $inherited parent-$upstream_http_x_source;
        proxy_set $unrelated parent-$upstream_http_x_other;
        add_header X-Inherited $inherited always;
        add_header X-Unrelated $unrelated always;

        location = /inherit {
            proxy_pass http://127.0.0.1:8081;
        }

        location = /override {
            proxy_set $inherited child;
            proxy_pass http://127.0.0.1:8081;
        }

        location = /conditional-inherit {
            when child_selected {
                proxy_set $inherited selected;
            }

            proxy_pass http://127.0.0.1:8081;
        }
    }
}

EOF

$t->run();

###############################################################################

my $basic = response('/basic?source=origin&suffix=tail', 8080);
is(header_value($basic, 'X-From-Upstream'), 'origin:tail',
	'new variable uses upstream and request values');
is(header_value($basic, 'X-Existing'), 'updated-fixed',
	'existing variable is updated in the response header phase');
is(header_value($basic, 'X-Args'), 'changed=yes',
	'variable set handler is invoked');

is(header_value(response('/conditional?mode=special', 8080), 'X-Selected'),
	'condition', 'matching condition selects its definition');
is(header_value(response('/conditional?mode=other', 8080), 'X-Selected'),
	'fallback', 'condition miss selects the next definition');
is(header_value(response('/order?mode=special', 8080), 'X-Selected'),
	'first', 'first unconditional definition wins over a later condition');

is(header_value(response('/upstream-condition?source=special', 8080),
	'X-Selected'), 'condition',
	'condition can inspect an upstream response header');
is(header_value(response('/upstream-condition?source=other', 8080),
	'X-Selected'), 'fallback',
	'upstream header condition miss selects the fallback');

my $inherited = response('/inherit?source=origin', 8082);
is(header_value($inherited, 'X-Inherited'), 'parent-origin',
	'parent definition inherits unchanged');
is(header_value($inherited, 'X-Unrelated'), 'parent-fixed',
	'all parent definitions inherit unchanged');

my $overridden = response('/override?source=origin', 8082);
is(header_value($overridden, 'X-Inherited'), 'child',
	'unconditional child definition disables the same parent definition');
is(header_value($overridden, 'X-Unrelated'), 'parent-fixed',
	'unrelated parent definition remains inherited');

my $child = response('/conditional-inherit?mode=special&source=origin', 8082);
is(header_value($child, 'X-Inherited'), 'selected',
	'matching conditional child definition wins');
is(header_value($child, 'X-Unrelated'), 'parent-fixed',
	'conditional child preserves unrelated parent definitions');

my $parent = response('/conditional-inherit?mode=other&source=origin', 8082);
is(header_value($parent, 'X-Inherited'), 'parent-origin',
	'parent definition remains when child condition misses');
is(header_value($parent, 'X-Unrelated'), 'parent-fixed',
	'parent fallback preserves unrelated definitions');

is(http_status($basic), 200, 'upstream response remains successful');

###############################################################################

sub header_value {
	my ($response, $name) = @_;
	my ($value) = $response =~ /^\Q$name\E:\s*(.*?)\x0d?$/mi;

	return $value;
}


sub http_status {
	my ($response) = @_;
	my ($status) = $response =~ m{^HTTP/\S+\s+(\d+)};

	return 0 + ($status || 0);
}


sub response {
	my ($uri, $port) = @_;

	return http("GET $uri HTTP/1.1\x0d\x0a"
		. "Host: localhost\x0d\x0a"
		. "Connection: close\x0d\x0a\x0d\x0a",
		PeerAddr => '127.0.0.1:' . port($port));
}

###############################################################################
