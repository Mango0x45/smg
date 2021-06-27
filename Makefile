.POSIX:

PREFIX = /usr/local

target   = smg
manpages = smg.1 smg.5

CC     = cc
CFLAGS = -O3 -pedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes \
		-DPROGNAME=\"${target}\"
GFLAGS = --ignore-case

DP = ${DESTDIR}${PREFIX}

all: ${target}
${target}: src/smg.c src/smg.h
	${CC} ${CFLAGS} -o $@ src/smg.c

man: ${target} man/smg.1.smg man/smg.5.smg
	command -v gzip >/dev/null && GZIP=1; \
	for i in 1 5; do \
		[ -z "$$GZIP" ] && ./${target} man/smg.$$i.smg >smg.$$i || \
			./${target} man/smg.$$i.smg | gzip -c9 >smg.$$i.gz; \
	done


clean:
	rm -f ${target} smg.[15] smg.[15].gz hash.c

debug: CFLAGS += -DDEBUG
debug: clean all

hash:
	gperf ${GFLAGS} src/metatags.gperf | sed -n '/^static unsigned int/,/^}/p' >hash.c

format:
	clang-format -i --style=file src/*.[ch]

test: debug
	tests/test.sh

install: ${target} man
	mkdir -p ${DP}/bin ${DP}/share/man/man1 ${DP}/share/man/man5
	cp ${target} ${DP}/bin/${target}
	cp smg.1* ${DP}/share/man/man1
	cp smg.5* ${DP}/share/man/man5

uninstall:
	rm -f ${DP}/bin/${target} ${DP}/share/man/man[15]/smg.[15] \
		${DP}/share/man/man[15]/smg.[15].gz

.PHONY: man clean debug hash format test install uninstall
