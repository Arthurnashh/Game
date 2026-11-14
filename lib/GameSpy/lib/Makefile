############################################################
#
#  GNU Makefile for $/Poseidon/lib/GameSpy subproject
#
#  27.9.2002, PE
#
############################################################

#  etiquette:

.SUFFIXES:
.SUFFIXES: .cpp .o

#  compiler:

INCLUDES= -I.. -I../cengine -I../queryreporting
VPATH   = ..:../cengine:../queryreporting

#  object files to build:

SOURCES = darray.c hashtable.c md5c.c nonport.c gserver.c gserverlist.c gutil.c \
          gqueryreporting.c

OBJS    = $(addprefix $(TARGET)/, $(SOURCES:.c=.o))
DEPS    = $(SOURCES:.c=.d)
LIBRARY = $(LIBTARGET)/libgamespy.a

#  targets:

all:    Makefile $(LIBRARY)

Makefile: $(DEPS)
	$(TOUCH) $@

$(DEPS): %.d: %.c
	set -e; $(CC) -M $(INCLUDES) $(CPPFLAGS) $< \
	    | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	    [ -s $@ ] || $(RM) $@

$(OBJS): $(TARGET)/%.o: %.c
	$(CC) $(INCLUDES) $(CPPFLAGS) -c -o $@ $<

$(LIBRARY): $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
	$(AR2) $@

.PHONY: clean

clean:
	$(RM) d/*.o r/*.o rs/*.o *.d

include $(DEPS)
