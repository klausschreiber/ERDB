include Makefile.in

DIRS	= external-sort buffer testing schema

all: external-sort buffer-test testing schema schema-test

external-sort: force_look
	cd external-sort; $(MAKE) $(MFLAGS)

buffer-test: buffer testing
	$(LD) $(LDFLAGS) -o buffer-test ./buffer/BufferManager.o ./buffer/BufferFrame.o ./testing/buffertest.o

buffer: force_look
	$(MAKE) $(MFLAGS) -C buffer

testing: force_look
	$(MAKE) $(MFLAGS) -C testing
	
schema: force_look
	$(MAKE) $(MFLAGS) -C schema

schema-test: schema buffer
	$(LD) $(LDFLAGS) -o schema-test ./buffer/BufferManager.o ./buffer/BufferFrame.o ./schema/SchemaManager.o ./schema/Schema.o
	

clean:
	$(RM) -rf buffer-test schema-test 0 1 2 3 4 5 6 7 8 9 
	for d in $(DIRS); do (cd $$d; $(MAKE) clean); done
	
force_look:
	true
