include $(MK)/common.mk

PROGNAME=	$(PROG)$(EXEEXT)

CLEAN_FILES+=	$(PROGNAME) $(OBJS) $(DEPFILES)

all: $(PROGNAME)

$(PROGNAME): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LOCAL_LDFLAGS)

install: $(PROGNAME)
	install -d -m 755 $(DESTDIR)$(bindir)
	install -m 755 $(PROGNAME) $(DESTDIR)$(bindir)/

check:

distclean: clean
