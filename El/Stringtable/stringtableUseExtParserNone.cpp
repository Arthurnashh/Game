#include <El/elementpch.hpp>
#include "iExtParser.hpp"

static StringtableExtParserFunctions GStringtableExtParserNone;
extern StringtableExtParserFunctions *GStringtableExtParserFunctions = &GStringtableExtParserNone;
