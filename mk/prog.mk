include $(MK)/common.mk

PROGNAME=	$(PROG)$(EXEEXT)

all: $(PROGNAME)

$(PROGNAME): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LOCAL_LDFLAGS)

clean:
	rm -f $(PROGNAME) $(OBJS) $(DEPFILES)

install: $(PROGNAME)
	install -d -m 755 $(DESTDIR)$(bindir)
	install -m 755 $(PROGNAME) $(DESTDIR)$(bindir)/

check:

distclean: clean
