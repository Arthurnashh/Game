############################################################
#
#  GNU Makefile for $/Essence subproject
#
#  7.10.2002, PE
#
############################################################

#  etiquette:

.SUFFIXES:
.SUFFIXES: .cpp .o

#  compiler:

INCLUDES= -I$(ROOT) -I$(ROOT)/Poseidon
VPATH   = ../Algorithms:../Common:../Containers:../Framework:../Memory:../Strings:../Types:../Threads:../Files:../ErrorProp:../ReportStack

#  object files to build:

SOURCES = crc32.cpp platform.cpp bitmask.cpp containersTemplSpec.cpp maps.cpp  \
appFrame.cpp netlog.cpp potime.cpp debugAlloc.cpp fastAlloc.cpp rString.cpp  \
lLinks.cpp softLinks.cpp pocritical.cpp posemaphore.cpp pothread.cpp  \
threadSync.cpp filenames.cpp errorProp.cpp reportStack.cpp

CSOURCES = 

ADDSOURCES =  useMallocMemFunctions.cpp useAppFrameDefault.cpp
OBJS    = $(addprefix $(TARGET)/, $(SOURCES:.cpp=.o))
ADDOBJS = $(addprefix $(LIBTARGET)/, $(ADDSOURCES:.cpp=.o))
DEPS    = $(SOURCES:.cpp=.d) $(ADDSOURCES:.cpp=.d)
LIBRARY = $(LIBTARGET)/libessence.a

#  targets:

all:    Makefile $(LIBRARY) $(ADDOBJS)

Makefile: $(DEPS)
	$(TOUCH) $@

$(DEPS): %.d: %.cpp
	set -e; $(CCC) -M $(INCLUDES) $(CPPFLAGS) $< \
	    | sed 's^\($*\)\.o[ :]*^$(TARGET)/\1.o $@ : ^g' > $@; \
	    [ -s $@ ] || $(RM) $@

$(OBJS): $(TARGET)/%.o: %.cpp
	$(CCC) $(INCLUDES) $(CPPFLAGS) -c -o $@ $<

$(ADDOBJS): $(LIBTARGET)/%.o: %.cpp
	$(CCC) $(INCLUDES) $(CPPFLAGS) -c -o $@ $<

$(LIBRARY): $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
	$(AR2) $@

.PHONY: clean

clean:
	$(RM) d/*.o r/*.o rs/*.o $(ADDOBJS) *.d

include $(DEPS)
