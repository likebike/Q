# A really useful way to see the default 'make' rules, which you can use to understand how Makefiles *really* work:  make -p

CPPFLAGS+=-Wall -std=c99 -O3
BUILD=$(LINK.c) $(LOADLIBES) $(LDLIBS)

SOURCES=Q.c abspath.c abspath.h dir_base_name.h makedirs.c makedirs.h nanosleep.h strdup.h sebC.c sebC.h Makefile

all: Q

Q: $(SOURCES)
	# General use:
	$(BUILD) -o Q Q.c abspath.c makedirs.c sebC.c

php: php54_large.cgi
php54_large.cgi: $(SOURCES)
	# Useful for global replacement:
	$(BUILD) -o php52-q.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php52-r.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=3 -DMAX_Q=20
	$(BUILD) -o php53-q.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php53-r.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=3 -DMAX_Q=20
	$(BUILD) -o php54-q.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php54-r.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=3 -DMAX_Q=20
	# Useful for single-app overrides:
	$(BUILD) -o php52_normal.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php52.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=3 -DMAX_Q=20
	$(BUILD) -o php53_normal.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php53.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=3 -DMAX_Q=20
	$(BUILD) -o php54_normal.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php54.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=3 -DMAX_Q=20
	$(BUILD) -o php52_large.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php52.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=8 -DMAX_Q=20
	$(BUILD) -o php53_large.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php53.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=8 -DMAX_Q=20
	$(BUILD) -o php54_large.cgi Q.c abspath.c makedirs.c sebC.c -DCOMMAND=/home/php-cgi/php54.cgi -DLOCKFILE='~/.tmp/cgi_$$SERVER_NAME.lock' -DCONCURRENCY=8 -DMAX_Q=20

clean:
	rm -f Q php54_large.cgi php53_large.cgi php52_large.cgi php54_normal.cgi php53_normal.cgi php52_normal.cgi php54-q.cgi php53-q.cgi php52-q.cgi

publish_www: all
	$(eval VERSION:=$(shell git describe master))
	$(eval OS:=centos6)
	# ( cd ..; tar czf Q-$(VERSION).tar.gz Q/; mv Q-$(VERSION).tar.gz ~/www/bigfiles/ )
	for x in Q *.cgi; do mv $$x ~/www/bigfiles/$(OS)/$$x-$(VERSION); done
	( cd ~/www/bigfiles/$(OS)/; for x in $$(ls Q *.cgi); do ln -sf $$x-$(VERSION) $$x; done )

