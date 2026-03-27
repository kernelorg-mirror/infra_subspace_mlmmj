include $(MK)/common.mk

PROGNAME=	$(PROG)

all: $(PROGNAME)

$(PROGNAME): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LOCAL_LDFLAGS)

clean:
	rm -f $(PROGNAME) $(OBJS) $(DEPFILES)

install:
