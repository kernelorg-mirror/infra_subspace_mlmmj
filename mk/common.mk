CFLAGS?=	-O2 -pipe
OBJS=	${SRCS:.c=.o}
SHOBJS?=	${SRCS:.c=.pico}
DEPFILES=	${OBJS:.o=.Po} ${SHOBJS:.pico=.Ppico}
CFLAGS+=	$(CPPFLAGS)
CFLAGS+=	-Werror=implicit-function-declaration
CFLAGS+=	-Werror=return-type

# Plain -include: .if breaks GNU make and $(VAR:=) breaks OpenBSD's make(1);
# SRCS must be set by the including makefile so that DEPFILES is not empty.
-include ${DEPFILES}

.SUFFIXES: .pico

.c.o:
	$(CC) -Wall -Wextra -std=gnu17 -D_GNU_SOURCE=1 -MT $@ -MD -MP -MF $*.Tpo -o $@ -c $(CFLAGS) $(LOCAL_CFLAGS) $<
	mv $*.Tpo $*.Po

.c.pico:
	$(CC) -Wall -Wextra -std=gnu17 -D_GNU_SOURCE=1 -MT $@ -MD -MP -MF $*.Tpico -o $@ -c $(CFLAGS) $(LOCAL_CFLAGS) $(SHOBJ_CFLAGS) $<
	mv $*.Tpico $*.Ppico

clean: clean-files

clean-files:
	rm -f $(CLEAN_FILES)

distclean: distclean-files

distclean-files:
	rm -f $(DISTCLEAN_FILES)
