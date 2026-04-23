include $(MK)/common.mk

CLEAN_FILES+=	lib$(LIB).a lib$(LIB)_pic.a $(OBJS) $(SHOBJS) $(DEPFILES)

all: lib$(LIB).a lib$(LIB)_pic.a

lib$(LIB).a: $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

lib$(LIB)_pic.a: $(SHOBJS)
	$(AR) cr $@ $(SHOBJS)
	$(RANLIB) $@

install: install-lib

install-lib: lib$(LIB).a
	install -d -m 755 $(DESTDIR)$(libdir)
	install -m 644 lib$(LIB).a $(DESTDIR)$(libdir)/

check:

distclean: clean
