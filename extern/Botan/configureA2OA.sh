#!/bin/bash
./configure.py --os=linux --cc-bin=`which lsbc++` --cpu=x86 --with-tr1=system --gen-amalgamation --disable-shared --no-autoload --enable-modules=rsa,x509cert,x509store,sha1,pkcs10,emsa4,emsa3,emsa1,eme_pkcs,auto_rng,pbes1,pbes2

mv botan_all.cpp botan_all_linux.cpp
mv botan_all.h botan_all_linux.h

