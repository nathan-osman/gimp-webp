# Build the WebP Gimp plugin

all:
	export LIBS=-lwebp; \
	gimptool-2.0 --build src/file-webp.c

install:
	mkdir -p $(DESTDIR)$(exec_prefix)/lib/gimp/2.0/plug-ins
	cp file-webp $(DESTDIR)$(exec_prefix)/lib/gimp/2.0/plug-ins
