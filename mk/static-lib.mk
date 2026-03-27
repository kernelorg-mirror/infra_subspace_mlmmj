include $(MK)/common.mk

all: lib$(LIB).a

lib$(LIB).a: $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -f lib$(LIB).a $(OBJS) $(DEPFILES)

install:
