include Makefile.in

DIRS	= external-sort buffer

all: external-sort

external-sort: force_look
	cd external-sort; $(MAKE) $(MFLAGS)

buffer: force_look
	cd buffer; $(MAKE) $(MFLAGS)

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean); done
	
force_look:
	true
