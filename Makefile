CC = gcc
INCLUDES = oufs_lib.h oufs.h vdisk.h
LIB = oufs_lib_support.o vdisk.o

.c.o: $(INCLUDES)
	$(CC) -c $< -o $@

all: zinspect zformat zmkdir zfilez zrmdir ztouch zcreate zmore zappend zlink zremove

zinspect: zinspect.o $(LIB)
	$(CC) -o zinspect zinspect.o $(LIB)

zformat: zformat.o $(LIB)
	$(CC) -o zformat zformat.o $(LIB)

zmkdir: zmkdir.o $(LIB)
	$(CC) -o zmkdir zmkdir.o $(LIB)

zfilez: zfilez.o $(LIB)
	$(CC) -o zfilez zfilez.o $(LIB)

zrmdir: zrmdir.o $(LIB)
	$(CC) -o zrmdir zrmdir.o $(LIB)

ztouch: ztouch.o $(LIB)
	$(CC) -o ztouch ztouch.o $(LIB)

zcreate: zcreate.o $(LIB)
	$(CC) -o zcreate zcreate.o $(LIB)

zmore: zmore.o $(LIB)
	$(CC) -o zmore zmore.o $(LIB)

zappend: zappend.o $(LIB)
	$(CC) -o zappend zappend.o $(LIB)

zlink: zlink.o $(LIB)
	$(CC) -o zlink zlink.o $(LIB)

zremove: zremove.o $(LIB)
	$(CC) -o zremove zremove.o $(LIB)

clean:
	-rm *.o $(objects) zinspect zformat zmkdir zfilez zrmdir ztouch zcreate zmore zappend zlink zremove vdisk1