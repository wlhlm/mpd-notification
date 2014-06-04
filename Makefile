# mpd-notification - simple libnotify integration for musicpd
# See LICENSE file for copyright and license details.

# mpd-notification version
VERSION = 0.1

# Programm name
NAME = mpd-notification

# Customize below to fit your system paths
PREFIX ?= /usr/local
BINDIR = ${PREFIX}/bin
MANDIR = ${PREFIX}/share/man/man1

# Includes and libs
LIBS = glib-2.0 libmpdclient libnotify
INCS = `pkg-config --cflags --libs ${LIBS}`

# Flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -DNAME=\"${NAME}\" -D_POSIX_C_SOURCE=2
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}

# Enable debugging symbols
ifdef DEBUG
	CFLAGS += -g
endif

# Compiler and linker
CC ?= cc

# Source files
SRC = mpd-notification.c
MANSRC = mpd-notification.1

all: options ${NAME}

options:
	@echo ${NAME} build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CC       = ${CC}"

config.h:
	@echo Creating $@ from config.def.h
	@cp config.def.h $@

$(NAME): ${SRC} config.h
	@${CC} -o ${NAME} ${SRC} ${CFLAGS}

clean:
	@echo Cleaning
	@rm -f ${NAME} config.h

install: all
	@echo Installing executable file to ${DESTDIR}${BINDIR}
	@install -d ${DESTDIR}${BINDIR} ${DESTDIR}${MANDIR}
	@install -m 755 ${NAME} ${DESTDIR}${BINDIR}
	@install -m 644 ${MANSRC} ${DESTDIR}${MANDIR}
	@sed "s/VERSION/${VERSION}/g" -i ${DESTDIR}${MANDIR}/${MANSRC}
	@sed "s/PROGNAME/${NAME}/g" -i ${DESTDIR}${MANDIR}/${MANSRC}

uninstall:
	@echo Removing executable file from ${DESTDIR}${BINDIR}
	@rm -f ${DESTDIR}${BINDIR}/${NAME}
	@echo Removing man page from ${DESTDIR}${MANDIR}
	@rm -f ${DESTDIR}${MANDIR}/${NAME}.1

.PHONY: all options clean install uninstall
