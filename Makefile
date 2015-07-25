
# This Makefile was generated by genmakefile.py, a Makefile generation script.
# For more information on genmakefile.py, refer to the genmakefile webpage at:
# http://www-personal.umich.edu/~gopalkri/MakefileGenerator/
# You may also contact the author of the script by email at: gopalkri@umich.edu
CC = gcc
LD = gcc

CFLAGS = `pkg-config gdk-pixbuf-2.0 --cflags`-ansi -Wall -pedantic -D_GNU_SOURCE -c -std=c99 -g
LFLAGS = -lGL -lglut -lGLU -lm `pkg-config gdk-pixbuf-2.0 --libs`
TEXTURE_PLATFORM_SOURCES = texture_gdk.c

OBJS = i3d_ass3.o texture_gdk.o texture_common.o mtl.o obj.o

PROG = ass3

TEXTURE_VIEWER_SOURCES = texture_common.c \
                         $(TEXTURE_PLATFORM_SOURCES)
TEXTURE_VIEWER_HEADERS = texture.h

default: printblank $(PROG)

printblank:
	@echo "" ; echo "" ; echo "" ; echo "" ; echo "" ;

$(PROG): $(OBJS)
	$(LD) $(LFLAGS) $(OBJS) -o $(PROG)

$(TEXTURE_VIEWER) : $(TEXTURE_VIEWER_SOURCES) $(TEXTURE_VIEWER_HEADERS)
	gcc -o $@ $(TEXTURE_VIEWER_SOURCES) $(CFLAGS) $(LFLAGS)

texture_gdk.o: texture_gdk.c texture.h
	$(CC) $(CFLAGS) texture_gdk.c

texture_common.o: texture_common.c texture.h
	$(CC) $(CFLAGS) texture_common.c

i3d_ass1.o: i3d_ass3.c texture.h
	$(CC) $(CFLAGS) i3d_ass3.c

obj.o: obj.c obj.h
	$(CC) $(CFLAGS) obj.c

mtl.o: mtl.c mtl.h
	$(CC) $(CFLAGS) mtl.c

clean:
	rm -rf *.o

real_clean:
	 rm -rf *.o $(PROG)
