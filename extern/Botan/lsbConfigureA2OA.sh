#!/bin/bash
./configure.py --os=linux --cc=gcc --cpu=x86 --with-tr1=system --disable-shared --no-autoload --enable-modules=rsa,x509cert,x509store,sha1,pkcs10,emsa4,emsa3,emsa1,eme_pkcs,auto_rng,pbes1,pbes2,dev_random

