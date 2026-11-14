############################################################
#
#  GNU Makefile for $/ogg subproject
#
#  24.9.2003, PE
#
#  environment: ICC = 1 to use Intel compiler (icc)
#
############################################################

#  etiquette:

.SUFFIXES:
.SUFFIXES: .c .o

#  programs:

INCLUDES= -I../libogg/include -I../libvorbis/include -I../libvorbis/lib
VPATH   =  ../libogg/src:../libvorbis/lib
DEFINES = -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -D_IO_MTSAFE_IO

#  object files to build:

SOURCES = bitwise.c framing.c analysis.c barkmel.c bitrate.c block.c codebook.c \
          envelope.c floor0.c floor1.c info.c lookup.c lpc.c lsp.c mapping0.c mdct.c \
          psy.c registry.c res0.c sharedbook.c smallft.c synthesis.c \
          vorbisenc.c vorbisfile.c window.c
OBJS    = $(addprefix $(TARGET)/, $(SOURCES:.c=.o))
DEPS    = $(SOURCES:.c=.d)
LIBRARY = $(LIBTARGET)/libogg.a

#  targets:

all:    Makefile $(LIBRARY)

Makefile: $(DEPS)
	$(TOUCH) $@

$(DEPS): %.d: %.c
	set -e; $(CC) -M $(INCLUDES) $(CPPFLAGS) $(DEFINES) $< \
	    | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	    [ -s $@ ] || $(RM) $@

$(OBJS): $(TARGET)/%.o: %.c
	$(CC) $(INCLUDES) $(CPPFLAGS) $(DEFINES) -c -o $@ $<

$(LIBRARY): $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
	$(AR2) $@

.PHONY: clean

clean:
	find . -name "*.d" -exec $(RM) {} \;
	$(RM) d/*.o r/*.o sr/*.o

include $(DEPS)
