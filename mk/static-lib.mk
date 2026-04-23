include $(MK)/common.mk

all: lib$(LIB).a lib$(LIB)_pic.a

lib$(LIB).a: $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

lib$(LIB)_pic.a: $(SHOBJS)
	$(AR) cr $@ $(SHOBJS)
	$(RANLIB) $@

clean:
	rm -f lib$(LIB).a lib$(LIB)_pic.a $(OBJS) $(SHOBJS) $(DEPFILES)

install: lib$(LIB).a
	install -d -m 755 $(DESTDIR)$(libdir)
	install -m 644 lib$(LIB).a $(DESTDIR)$(libdir)/

check:

distclean: clean
