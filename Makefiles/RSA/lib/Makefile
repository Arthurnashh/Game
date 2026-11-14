############################################################
#
#  GNU Makefile for $/RSA subproject
#
#  24.9.2003, PE
#
#  environment: ICC = 1 to use Intel compiler (icc)
#
############################################################

#  etiquette:

.SUFFIXES:
.SUFFIXES: .c .cpp .o

#  programs:

INCLUDES=
VPATH   = ..
DEFINES = -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -D_IO_MTSAFE_IO -DHAVE_STPCPY

#  object files to build:

CSOURCES= logger.c memory.c mpi-bit.c mpi-inline.c mpi-pow.c mpih-add1.c mpih-cmp.c \
          mpih-div.c mpih-lshift.c mpih-mul.c mpih-mul1.c mpih-mul2.c mpih-mul3.c \
          mpih-rshift.c mpih-sub1.c mpiutil.c
SOURCES = $(CSOURCES) rsa.cpp
COBJS   = $(addprefix $(TARGET)/, $(CSOURCES:.c=.o))
OBJS    = $(COBJS) $(TARGET)/rsa.o
CDEPS   = $(CSOURCES:.c=.d)
DEPS    = $(CDEPS) rsa.d
LIBRARY = $(LIBTARGET)/librsa.a

#  targets:

all:    Makefile $(LIBRARY)

Makefile: $(DEPS)
	$(TOUCH) $@

$(CDEPS): %.d: %.c
	set -e; $(CC) -M $(INCLUDES) $(CPPFLAGS) $(DEFINES) $< \
	    | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	    [ -s $@ ] || $(RM) $@

rsa.d:  rsa.cpp
	set -e; $(CC) -M $(INCLUDES) $(CPPFLAGS) $(DEFINES) $< \
	    | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	    [ -s $@ ] || $(RM) $@

$(COBJS): $(TARGET)/%.o: %.c
	$(CC) $(INCLUDES) $(CPPFLAGS) $(DEFINES) -c -o $@ $<

$(TARGET)/rsa.o: rsa.cpp
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
