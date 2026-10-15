all:
	@set -e ; for d in $(DIRS); do \
		$(MAKE) -C $$d ; \
	done

install:
	@set -e ; for d in $(DIRS); do \
		$(MAKE) -C $$d install ; \
	done

uninstall:
	@set -e ; for d in $(DIRS); do \
		$(MAKE) -C $$d uninstall ; \
	done

clean:
	@set -e; for d in $(DIRS); do \
		$(MAKE) -C $$d clean ; \
	done

check:
	@set -e; for d in $(DIRS); do \
		$(MAKE) -C $$d check ; \
	done

distclean: clean
	@set -e; for d in $(DIRS); do \
		if [ -f $$d/Makefile ]; then $(MAKE) -C $$d distclean ; fi ; \
	done
