include Makefile.in

DIRS	= external-sort buffer

all: external-sort buffer-test testing

external-sort: force_look
	cd external-sort; $(MAKE) $(MFLAGS)

buffer-test: buffer testing
	$(LD) $(LDFLAGS) -o buffer-test ./buffer/BufferManager.o ./buffer/BufferFrame.o ./testing/buffertest.o

buffer: force_look
	$(MAKE) $(MFLAGS) -C buffer

testing: force_look
	$(MAKE) $(MFLAGS) -C testing
	
	

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean); done
	
force_look:
	true
