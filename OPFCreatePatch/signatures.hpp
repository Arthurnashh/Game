#ifndef _SIGNATURES_HPP
#define _SIGNATURES_HPP

#include <El/DataSignatures/dataSignatures.hpp>

bool CalculateSignature(DSSignature &signature, const char *data, int size, const DSKey &privateKey);

#endif
