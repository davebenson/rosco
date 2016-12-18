CFLAGS = -std=c99 @CFLAGS@

# Files which are used BY the code-generator, but do NOT use the
# code generator, hence can be made into a helper library
# for bootstrapping.
rosco_bootstrap_c_files = \
src/dsk/dsk-base64-encoder.c \
src/dsk/dsk-http-server-stream.c \
src/dsk/dsk-date.c \
src/dsk/dsk-memory-sink.c \
src/dsk/dsk-logger.c \
src/dsk/dsk-error.c \
src/dsk/dsk-unquote-printable.c \
src/dsk/dsk-byte-doubler.c \
src/dsk/dsk-ts0.c \
src/dsk/dsk-codepage.c \
src/dsk/dsk-url.c \
src/dsk/dsk-http-protocol.c \
src/dsk/dsk-octet-listener.c \
src/dsk/dsk-octet-io.c \
src/dsk/dsk-xml-escaper.c \
src/dsk/dsk-xml-binding-parser.c \
src/dsk/dsk-ethernet-address.c \
src/dsk/dsk-cgi.c \
src/dsk/dsk-ip-address.c \
src/dsk/dsk-http-header-printing.c \
src/dsk/dsk-hook.c \
src/dsk/dsk-whitespace-trimmer.c \
src/dsk/dsk-xml.c \
src/dsk/dsk-json-parser.c \
src/dsk/dsk-http-header-parsing.c \
src/dsk/dsk-utf8-fixer.c \
src/dsk/dsk-strv.c \
src/dsk/dsk-dispatch.c \
src/dsk/dsk-c-quoter.c \
src/dsk/dsk-xml-parser.c \
src/dsk/dsk-octet-filter-identity.c \
src/dsk/dsk-inlines.c \
src/dsk/dsk-ts0-builtins.c \
src/dsk/dsk-ascii.c \
src/dsk/dsk-mime.c \
src/dsk/dsk-daemon.c \
src/dsk/dsk-memory-source.c \
src/dsk/dsk-hex-encoder.c \
src/dsk/dsk-main.c \
src/dsk/dsk-octet-filter-source.c \
src/dsk/dsk-octet-pipe.c \
src/dsk/dsk-cleanup.c \
src/dsk/dsk-cmdline.c \
src/dsk/dsk-fd.c \
src/dsk/dsk-octet-connection.c \
src/dsk/dsk-octet-listener-socket.c \
src/dsk/dsk-url-decoder.c \
src/dsk/dsk-base64.c \
src/dsk/dsk-url-encoder.c \
src/dsk/dsk-dns-protocol.c \
src/dsk/dsk-mem-pool.c \
src/dsk/dsk-utf8.c \
src/dsk/dsk-checksum.c \
src/dsk/dsk-ctoken.c \
src/dsk/dsk-c-unquoter.c \
src/dsk/dsk-octet-filter-chain.c \
src/dsk/dsk-buffer.c \
src/dsk/dsk-print.c \
src/dsk/dsk-http-client-stream.c \
src/dsk/dsk-file-util.c \
src/dsk/dsk-hex-decoder.c \
src/dsk/dsk-json-output.c \
src/dsk/dsk-json.c \
src/dsk/dsk-dns-client.c \
src/dsk/dsk-octet-fd.c \
src/dsk/dsk-dir.c \
src/dsk/dsk-json-prettier.c \
src/dsk/dsk-octet-filter.c \
src/dsk/dsk-zlib.c \
src/dsk/dsk-common.c \
src/dsk/dsk-utf8-to-utf16.c \
src/dsk/dsk-nosql-node.c \
src/dsk/dsk-xml-binding.c \
src/dsk/dsk-http-server.c \
src/dsk/codepages/codepage-latin1.c \
src/dsk/codepages/codepage-CP1253.c \
src/dsk/codepages/codepage-ISO-8859-10.c \
src/dsk/codepages/codepage-ISO-8859-9.c \
src/dsk/codepages/codepage-ISO-8859-8.c \
src/dsk/codepages/codepage-CP1251.c \
src/dsk/codepages/codepage-CP1254.c \
src/dsk/codepages/codepage-ISO-8859-15.c \
src/dsk/codepages/codepage-ISO-8859-16.c \
src/dsk/codepages/codepage-ISO-8859-7.c \
src/dsk/codepages/codepage-ISO-8859-6.c \
src/dsk/codepages/codepage-CP1256.c \
src/dsk/codepages/codepage-ISO-8859-2.c \
src/dsk/codepages/codepage-ISO-8859-3.c \
src/dsk/codepages/codepage-ISO-8859-1.c \
src/dsk/codepages/codepage-ISO-8859-4.c \
src/dsk/codepages/codepage-CP874.c \
src/dsk/codepages/codepage-CP1250.c \
src/dsk/codepages/codepage-ISO-8859-5.c \
src/dsk/codepages/codepage-CP1257.c \
src/dsk/dsk-mime-multipart.c \
src/dsk/dsk-pattern.c \
src/dsk/dsk-rand.c \
src/dsk/dsk-object.c \
src/dsk/dsk-bz2lib.c \
src/dsk/dsk-network-interface-list.c \
src/dsk/dsk-client-stream.c \
src/dsk/dsk-websocket.c \
src/dsk/dsk-http-internals.c \
src/dsk/dsk-base64-decoder.c \
src/dsk/dsk-codegen.c \
src/dsk/dsk-byte-undoubler.c \
src/dsk/dsk-udp-socket.c \
src/dsk/dsk-path.c \
src/dsk/dsk-quote-printable.c \
src/rosco-type-system.c

bin_PROGRAMS = rosco-codegen
rosco_codegen_SOURCES = \
$(rosco_bootstrap_c_files) \
src/tools/rosco-codegen-main.c
rosco_codegen_LDADD = \
-lbz2 -lz

generated/rosco/%.c generated/rosco/%.h: srv/rosco/%.srv rosco-codegen
	./rosco-codegen --src-dir=. --service=rosco/$* --dest=generated

lib_LIBRARIES = librosco.a
librosco_a_SOURCES =  \
$(rosco_bootstrap_c_files) \
generated/rosco/GetLoggers.c \
src/rosco-node.c
