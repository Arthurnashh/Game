############################################################
#
#  GNU Makefile for $/Element subproject
#
#  3.11.2002, PE
#
############################################################

#  etiquette:

.SUFFIXES:
.SUFFIXES: .cpp .o

#  compiler:

INCLUDES= -I$(ROOT) -I$(ROOT)/Poseidon -I$(ROOT)/Poseidon/extern -I/usr/lib/jvm/java-6-openjdk-i386/include -I/usr/lib/jvm/java-6-openjdk-i386/include/linux
VPATH   = ../ParamFile:../Evaluator:../Common:../QStream:../PreprocC:../Stringtable:../XML:../Network:../Calendar:../Modules:../Math:../FreeOnDemand:../CRC:../ParamArchive:../Clipboard:../Debugging:../Color:../Time:../DebugWindow:../Enum:../Statistics:../HierWalker:../Breakpoint:../Speech/DPCM:../Speech/Microsoft:../Speech/LPC:../Speech/PCM:../Speech/Speex:../Speech:../CorelAnalys:../DebugConsole:../HiResTime:../Activity:../Rectangle:../BIDebugEngine:../FileBinMake:../CDKey:../SimpleExpression:../Graph:../DataSignatures:../PerlinNoise:../FileServer:../SoundFile:../MultiThread2/common/src-linux:../MultiThread2/multithread/src-linux:../MultiThread2/multithread/src-portable:../MultiThread2/common/src-portable:../Multicore:../ActiveObject:../MemoryStore:../LZO:../Credits

#  object files to build:

SOURCES = classDbParamFile.cpp paramFile.cpp paramFileCtx.cpp paramFileUseEvalExpress.cpp  \
paramFileUseLocalizeStringtable.cpp paramFileUsePreprocC.cpp paramFileUseSumCalcCrc.cpp  \
evaluatorExpress.cpp evalUseBIDebugEngine.cpp evalUseLocalizeStringtable.cpp  \
express.cpp expressCompiler.cpp expressInterpreter.cpp scriptVM.cpp  \
scriptVMUsePreprocC.cpp globalAlive.cpp perfLog.cpp randomGen.cpp  \
randomJames.cpp fileCompress.cpp packFiles.cpp qbStream.cpp qStream.cpp  \
qStreamUseBankQFBank.cpp serializeBin.cpp ssCompress.cpp preproc.cpp  \
preprocC.cpp localizeStringtable.cpp stringtable.cpp stringtableParserXML.cpp  \
stringtableUseExtParserXML.cpp stringtableUsePreprocC.cpp inet.cpp  \
xml.cpp directchannel.cpp netapi.cpp netchannel.cpp netmessage.cpp  \
netpeer.cpp netpool.cpp peerfactory.cpp eventToCallback.cpp pocalendar.cpp  \
modules.cpp math3dK.cpp math3dP.cpp math3DPK.cpp math3dT.cpp mathOpt.cpp  \
matrixP3.cpp memFreeReq.cpp crc.cpp paramArchive.cpp paramArchiveDb.cpp  \
paramArchiveUseDbParamFile.cpp clipboard.cpp debugTrap.cpp imexhnd.cpp  \
mapFile.cpp colorsK.cpp colorsP.cpp time.cpp debugWin.cpp dynEnum.cpp  \
enumNames.cpp idString.cpp statistics.cpp hierWalker.cpp breakpoint.cpp  \
dpcm.cpp mscelp.cpp openlpc.cpp pcm.cpp speex.cpp vonApp.cpp vonCodec.cpp  \
vonMemory.cpp corelAnalys.cpp debugConsole.cpp hiResTime.cpp activity.cpp  \
rectangle.cpp biDebugEngine.cpp fileBinMake.cpp serial.cpp simpleExpression.cpp  \
clique.cpp dataSignatures.cpp sha1.cpp sha1wrapper.cpp perlinNoise.cpp  \
multithread.cpp soundFile.cpp BlockerSimpleBase.cpp  \
CriticalSection.cpp ThreadBase.cpp ThreadHook.cpp Tls.cpp waitFunctions.cpp  \
activeObject.cpp lzo1x_dStream.cpp \
memFreeReqUseSystem.cpp \
Blocker.cpp \
common_linux.cpp PosixEvent.cpp PosixMutex.cpp \
PosixSemaphore.cpp PosixThread.cpp \
Synchronized.cpp Assert.cpp fileServer.cpp jobs.cpp multicore.cpp memStore.cpp \
credits.cpp jvmImpl.cpp

CSOURCES = lzo1x_d1.c lzo1x_d4.c lzo_init.c pinit.c plzo1x_1f.c 

ADDSOURCES = 
OBJS    = $(addprefix $(TARGET)/, $(SOURCES:.cpp=.o))
COBJS   = $(addprefix $(TARGET)/, $(CSOURCES:.c=.o))
ADDOBJS = $(addprefix $(LIBTARGET)/, $(ADDSOURCES:.cpp=.o))
DEPS    = $(SOURCES:.cpp=.d) $(ADDSOURCES:.cpp=.d)
CDEPS   = $(CSOURCES:.c=.d)
LIBRARY = $(LIBTARGET)/libelement.a

#  targets:

all:    Makefile $(LIBRARY) $(ADDOBJS)

Makefile: $(DEPS)
	$(TOUCH) $@

$(DEPS): %.d: %.cpp
	set -e; $(CCC) -M $(INCLUDES) $(CPPFLAGS) $< \
	    | sed 's^\($*\)\.o[ :]*^$(TARGET)/\1.o $@ : ^g' > $@; \
	    [ -s $@ ] || $(RM) $@

$(CDEPS): %.d: %.c
	set -e; $(CCC) -M $(INCLUDES) $(CPPFLAGS) $< \
	    | sed 's^\($*\)\.o[ :]*^$(TARGET)/\1.o $@ : ^g' > $@; \
	    [ -s $@ ] || $(RM) $@

$(OBJS): $(TARGET)/%.o: %.cpp
	$(CCC) $(INCLUDES) $(CPPFLAGS) -c -o $@ $<

$(ADDOBJS): $(LIBTARGET)/%.o: %.cpp
	$(CCC) $(INCLUDES) $(CPPFLAGS) -c -o $@ $<

$(COBJS): $(TARGET)/%.o: %.c
	$(CCC) $(INCLUDES) $(CPPFLAGS) -c -o $@ $<

#  some exceptions (special compilation)
$(TARGET)/Tls.o: ../MultiThread2/multithread/src-linux/Tls.cpp
	$(CCC) $(INCLUDES) $(CPPFLAGS) -fexceptions -c -o $@ $<

$(LIBRARY): $(OBJS) $(COBJS)
	$(RM) $@
	$(AR) $@ $(OBJS) $(COBJS)
	$(AR2) $@

.PHONY: clean

clean:
	$(RM) d/*.o r/*.o rs/*.o $(ADDOBJS) *.d

include $(DEPS)
