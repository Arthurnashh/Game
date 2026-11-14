#ifndef _SIGNATURES_HPP
#define _SIGNATURES_HPP

#include <El/DataSignatures/dataSignatures.hpp>

int VerifySignature(const void *data, int size, const DSSignature &signature);

#endif
