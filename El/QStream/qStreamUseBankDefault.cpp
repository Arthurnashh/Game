#include <El/elementpch.hpp>
#include "iQFBank.hpp"
#include "qStream.hpp"

static QFBankFunctions GQStreamQFBankFunctions;
QFBankFunctions *FileBufferBankFunctions::_defaultQFBankFunctions = &GQStreamQFBankFunctions;
