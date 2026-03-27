CFLAGS?=	-O2 -pipe
OBJS=	${SRCS:.c=.o}
DEPFILES=	${OBJS:.o=.Po}
CFLAGS+=	$(CPPFLAGS)
CFLAGS+=	-Werror=implicit-function-declaration
CFLAGS+=	-Werror=return-type

DEPFILES_NONEMPTY=	$(DEPFILES) /nonexistent
-include $(DEPFILES_NONEMPTY:=)

.c.o:
	$(CC) -Wall -Wextra -std=gnu99 -D_GNU_SOURCE=1 -pedantic -Wsign-compare -MT $@ -MD -MP -MF $*.Tpo -o $@ -c $(CFLAGS) $(LOCAL_CFLAGS) $<
	mv $*.Tpo $*.Po
