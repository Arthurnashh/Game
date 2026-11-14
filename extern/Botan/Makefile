# Compiler Options
CXX           = g++
LIB_OPT       = -O3 -finline-functions
CHECK_OPT     = -O2
MACH_OPT      = 
LANG_FLAGS    = -D_REENTRANT -Wno-long-long
WARN_FLAGS    = -W -Wall
LINK_TO       = -lrt

# Version Numbers
VERSION       = 1.10.1
SERIES        = 1.10

# Installation Settings
DESTDIR       = /usr/local

BINDIR        = $(DESTDIR)/bin
LIBDIR        = $(DESTDIR)/lib
HEADERDIR     = $(DESTDIR)/include/botan-$(SERIES)/botan
DOCDIR        = $(DESTDIR)/share/doc/botan-$(VERSION)
PKGCONF_DIR   = $(LIBDIR)/pkgconfig

CONFIG_SCRIPT = build/botan-config-1.10
PKGCONFIG     = build/botan-1.10.pc

# Aliases for Common Programs
AR               = ar crs
COPY             = cp
COPY_R           = cp -r
CD               = @cd
ECHO             = @echo
INSTALL_CMD_EXEC = install -m 755
INSTALL_CMD_DATA = install -m 644
LN               = ln -fs
MKDIR            = @mkdir
MKDIR_INSTALL    = @umask 022; mkdir -p -m 755
RANLIB           = true
RM               = @rm -f
RM_R             = @rm -rf

# File Lists
CHECK         = check

HEADERS       = ./src/algo_base/algo_base.h \
                ./src/algo_base/buf_comp.h \
                ./src/algo_base/key_spec.h \
                ./src/algo_base/sym_algo.h \
                ./src/algo_base/symkey.h \
                ./src/algo_factory/algo_factory.h \
                ./src/alloc/allocate.h \
                ./src/alloc/secmem.h \
                ./src/asn1/alg_id.h \
                ./src/asn1/asn1_int.h \
                ./src/asn1/asn1_obj.h \
                ./src/asn1/asn1_oid.h \
                ./src/asn1/asn1_str.h \
                ./src/asn1/ber_dec.h \
                ./src/asn1/der_enc.h \
                ./src/asn1/x509_dn.h \
                ./src/block/aes/aes.h \
                ./src/block/block_cipher.h \
                ./src/cert/certstore/certstor.h \
                ./src/cert/pkcs10/pkcs10.h \
                ./src/cert/x509cert/x509_ext.h \
                ./src/cert/x509cert/x509_obj.h \
                ./src/cert/x509cert/x509cert.h \
                ./src/cert/x509crl/crl_ent.h \
                ./src/cert/x509crl/x509_crl.h \
                ./src/cert/x509store/x509stor.h \
                ./src/codec/base64/base64.h \
                ./src/codec/hex/hex.h \
                ./src/codec/pem/pem.h \
                ./src/engine/engine.h \
                ./src/entropy/entropy_src.h \
                ./src/filters/basefilt.h \
                ./src/filters/buf_filt.h \
                ./src/filters/codec_filt/b64_filt.h \
                ./src/filters/codec_filt/hex_filt.h \
                ./src/filters/data_snk.h \
                ./src/filters/data_src.h \
                ./src/filters/filter.h \
                ./src/filters/filters.h \
                ./src/filters/key_filt.h \
                ./src/filters/modes/cbc/cbc.h \
                ./src/filters/modes/mode_pad/mode_pad.h \
                ./src/filters/pipe.h \
                ./src/filters/secqueue.h \
                ./src/hash/hash.h \
                ./src/hash/mdx_hash/mdx_hash.h \
                ./src/hash/sha1/sha160.h \
                ./src/hash/sha2_32/sha2_32.h \
                ./src/hash/sha2_64/sha2_64.h \
                ./src/kdf/kdf.h \
                ./src/kdf/mgf1/mgf1.h \
                ./src/libstate/botan.h \
                ./src/libstate/global_state.h \
                ./src/libstate/init.h \
                ./src/libstate/libstate.h \
                ./src/libstate/look_pk.h \
                ./src/libstate/lookup.h \
                ./src/libstate/oid_lookup/oids.h \
                ./src/libstate/scan_name.h \
                ./src/mac/hmac/hmac.h \
                ./src/mac/mac.h \
                ./src/math/bigint/bigint.h \
                ./src/math/bigint/divide.h \
                ./src/math/mp/mp_types.h \
                ./src/math/numbertheory/numthry.h \
                ./src/math/numbertheory/pow_mod.h \
                ./src/math/numbertheory/reducer.h \
                ./src/pbe/get_pbe.h \
                ./src/pbe/pbe.h \
                ./src/pbe/pbes1/pbes1.h \
                ./src/pbe/pbes2/pbes2.h \
                ./src/pbkdf/pbkdf.h \
                ./src/pbkdf/pbkdf1/pbkdf1.h \
                ./src/pbkdf/pbkdf2/pbkdf2.h \
                ./src/pk_pad/eme.h \
                ./src/pk_pad/eme_pkcs/eme_pkcs.h \
                ./src/pk_pad/emsa.h \
                ./src/pk_pad/emsa1/emsa1.h \
                ./src/pk_pad/emsa3/emsa3.h \
                ./src/pk_pad/emsa4/emsa4.h \
                ./src/pk_pad/hash_id/hash_id.h \
                ./src/pubkey/blinding.h \
                ./src/pubkey/if_algo/if_algo.h \
                ./src/pubkey/keypair/keypair.h \
                ./src/pubkey/pk_keys.h \
                ./src/pubkey/pk_ops.h \
                ./src/pubkey/pkcs8.h \
                ./src/pubkey/pubkey.h \
                ./src/pubkey/pubkey_enums.h \
                ./src/pubkey/rsa/rsa.h \
                ./src/pubkey/x509_key.h \
                ./src/rng/auto_rng/auto_rng.h \
                ./src/rng/hmac_rng/hmac_rng.h \
                ./src/rng/rng.h \
                ./src/stream/stream_cipher.h \
                ./src/utils/bswap.h \
                ./src/utils/charset.h \
                ./src/utils/cpuid.h \
                ./src/utils/datastor/datastor.h \
                ./src/utils/exceptn.h \
                ./src/utils/get_byte.h \
                ./src/utils/loadstor.h \
                ./src/utils/mem_ops.h \
                ./src/utils/parsing.h \
                ./src/utils/rotate.h \
                ./src/utils/time.h \
                ./src/utils/types.h \
                ./src/utils/ui.h \
                ./src/utils/version.h \
                build/build.h

LIBOBJS       = build/lib/algo_base_symkey.o \
                build/lib/algo_factory.o \
                build/lib/algo_factory_prov_weight.o \
                build/lib/alloc_mem_pool.o \
                build/lib/alloc_system_alloc_defalloc.o \
                build/lib/asn1_alg_id.o \
                build/lib/asn1_asn1_alt.o \
                build/lib/asn1_asn1_att.o \
                build/lib/asn1_asn1_int.o \
                build/lib/asn1_asn1_oid.o \
                build/lib/asn1_asn1_str.o \
                build/lib/asn1_asn1_tm.o \
                build/lib/asn1_ber_dec.o \
                build/lib/asn1_der_enc.o \
                build/lib/asn1_x509_dn.o \
                build/lib/block_aes.o \
                build/lib/cert_certstore_certstor.o \
                build/lib/cert_pkcs10.o \
                build/lib/cert_x509cert_x509_ext.o \
                build/lib/cert_x509cert_x509_obj.o \
                build/lib/cert_x509cert.o \
                build/lib/cert_x509crl_crl_ent.o \
                build/lib/cert_x509crl_x509_crl.o \
                build/lib/cert_x509store_x509stor.o \
                build/lib/codec_base64.o \
                build/lib/codec_hex.o \
                build/lib/codec_pem.o \
                build/lib/engine_core_engine_core_modes.o \
                build/lib/engine_core_engine_def_pk_ops.o \
                build/lib/engine_core_engine_def_powm.o \
                build/lib/engine_core_engine_lookup_block.o \
                build/lib/engine_core_engine_lookup_hash.o \
                build/lib/engine_core_engine_lookup_mac.o \
                build/lib/engine_core_engine_lookup_pbkdf.o \
                build/lib/engine_core_engine_lookup_stream.o \
                build/lib/engine.o \
                build/lib/entropy_dev_random.o \
                build/lib/filters_algo_filt.o \
                build/lib/filters_basefilt.o \
                build/lib/filters_buf_filt.o \
                build/lib/filters_codec_filt_b64_filt.o \
                build/lib/filters_codec_filt_hex_filt.o \
                build/lib/filters_data_snk.o \
                build/lib/filters_data_src.o \
                build/lib/filters_filter.o \
                build/lib/filters_modes_cbc.o \
                build/lib/filters_modes_mode_pad.o \
                build/lib/filters_out_buf.o \
                build/lib/filters_pipe.o \
                build/lib/filters_pipe_io.o \
                build/lib/filters_pipe_rw.o \
                build/lib/filters_secqueue.o \
                build/lib/hash_mdx_hash.o \
                build/lib/hash_sha1_sha160.o \
                build/lib/hash_sha2_32.o \
                build/lib/hash_sha2_64.o \
                build/lib/kdf.o \
                build/lib/kdf_mgf1.o \
                build/lib/libstate_get_enc.o \
                build/lib/libstate_global_rng.o \
                build/lib/libstate_global_state.o \
                build/lib/libstate_init.o \
                build/lib/libstate.o \
                build/lib/libstate_lookup.o \
                build/lib/libstate_oid_lookup_oids.o \
                build/lib/libstate_policy.o \
                build/lib/libstate_scan_name.o \
                build/lib/mac_hmac.o \
                build/lib/mac.o \
                build/lib/math_bigint_big_code.o \
                build/lib/math_bigint_big_io.o \
                build/lib/math_bigint_big_ops2.o \
                build/lib/math_bigint_big_ops3.o \
                build/lib/math_bigint_big_rand.o \
                build/lib/math_bigint.o \
                build/lib/math_bigint_divide.o \
                build/lib/math_mp_mp_asm.o \
                build/lib/math_mp_mp_comba.o \
                build/lib/math_mp_mp_karat.o \
                build/lib/math_mp_mp_misc.o \
                build/lib/math_mp_mp_monty.o \
                build/lib/math_mp_mp_mulop.o \
                build/lib/math_mp_mp_shift.o \
                build/lib/math_numbertheory_dsa_gen.o \
                build/lib/math_numbertheory_jacobi.o \
                build/lib/math_numbertheory_make_prm.o \
                build/lib/math_numbertheory_mp_numth.o \
                build/lib/math_numbertheory_numthry.o \
                build/lib/math_numbertheory_pow_mod.o \
                build/lib/math_numbertheory_powm_fw.o \
                build/lib/math_numbertheory_powm_mnt.o \
                build/lib/math_numbertheory_primes.o \
                build/lib/math_numbertheory_reducer.o \
                build/lib/math_numbertheory_ressol.o \
                build/lib/mutex_noop_mutex_mux_noop.o \
                build/lib/pbe_get_pbe.o \
                build/lib/pbe_pbes1.o \
                build/lib/pbe_pbes2.o \
                build/lib/pbkdf_pbkdf1.o \
                build/lib/pbkdf_pbkdf2.o \
                build/lib/pk_pad_eme.o \
                build/lib/pk_pad_eme_pkcs.o \
                build/lib/pk_pad_emsa1.o \
                build/lib/pk_pad_emsa3.o \
                build/lib/pk_pad_emsa4.o \
                build/lib/pk_pad_hash_id.o \
                build/lib/pubkey_blinding.o \
                build/lib/pubkey_if_algo.o \
                build/lib/pubkey_keypair.o \
                build/lib/pubkey_pk_algs.o \
                build/lib/pubkey_pk_keys.o \
                build/lib/pubkey_pkcs8.o \
                build/lib/pubkey.o \
                build/lib/pubkey_pubkey_enums.o \
                build/lib/pubkey_rsa.o \
                build/lib/pubkey_workfactor.o \
                build/lib/pubkey_x509_key.o \
                build/lib/rng_hmac_rng.o \
                build/lib/rng.o \
                build/lib/stream_stream_cipher.o \
                build/lib/utils_assert.o \
                build/lib/utils_charset.o \
                build/lib/utils_cpuid.o \
                build/lib/utils_datastor.o \
                build/lib/utils_mlock.o \
                build/lib/utils_parsing.o \
                build/lib/utils_time.o \
                build/lib/utils_ui.o \
                build/lib/utils_version.o

CHECKOBJS     = build/checks/bench.o \
                build/checks/bigint.o \
                build/checks/cvc_tests.o \
                build/checks/dolook.o \
                build/checks/ec_tests.o \
                build/checks/ecdh.o \
                build/checks/ecdsa.o \
                build/checks/misc.o \
                build/checks/pk.o \
                build/checks/pk_bench.o \
                build/checks/timer.o \
                build/checks/validate.o

LIB_FLAGS     = $(LIB_OPT) $(MACH_OPT) $(LANG_FLAGS) $(WARN_FLAGS)
CHECK_FLAGS   = $(CHECK_OPT) $(LANG_FLAGS) $(WARN_FLAGS)

LIBRARIES     = $(STATIC_LIB)

LIBNAME       = libbotan
STATIC_LIB    = $(LIBNAME)-$(SERIES).a

all: $(LIBRARIES)

# Build Commands
build/lib/algo_base_symkey.o: ./src/algo_base/symkey.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/algo_factory.o: ./src/algo_factory/algo_factory.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/algo_factory_prov_weight.o: ./src/algo_factory/prov_weight.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/alloc_mem_pool.o: ./src/alloc/mem_pool/mem_pool.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/alloc_system_alloc_defalloc.o: ./src/alloc/system_alloc/defalloc.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_alg_id.o: ./src/asn1/alg_id.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_asn1_alt.o: ./src/asn1/asn1_alt.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_asn1_att.o: ./src/asn1/asn1_att.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_asn1_int.o: ./src/asn1/asn1_int.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_asn1_oid.o: ./src/asn1/asn1_oid.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_asn1_str.o: ./src/asn1/asn1_str.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_asn1_tm.o: ./src/asn1/asn1_tm.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_ber_dec.o: ./src/asn1/ber_dec.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_der_enc.o: ./src/asn1/der_enc.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/asn1_x509_dn.o: ./src/asn1/x509_dn.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/block_aes.o: ./src/block/aes/aes.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_certstore_certstor.o: ./src/cert/certstore/certstor.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_pkcs10.o: ./src/cert/pkcs10/pkcs10.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_x509cert_x509_ext.o: ./src/cert/x509cert/x509_ext.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_x509cert_x509_obj.o: ./src/cert/x509cert/x509_obj.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_x509cert.o: ./src/cert/x509cert/x509cert.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_x509crl_crl_ent.o: ./src/cert/x509crl/crl_ent.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_x509crl_x509_crl.o: ./src/cert/x509crl/x509_crl.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/cert_x509store_x509stor.o: ./src/cert/x509store/x509stor.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/codec_base64.o: ./src/codec/base64/base64.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/codec_hex.o: ./src/codec/hex/hex.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/codec_pem.o: ./src/codec/pem/pem.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_core_modes.o: ./src/engine/core_engine/core_modes.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_def_pk_ops.o: ./src/engine/core_engine/def_pk_ops.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_def_powm.o: ./src/engine/core_engine/def_powm.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_lookup_block.o: ./src/engine/core_engine/lookup_block.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_lookup_hash.o: ./src/engine/core_engine/lookup_hash.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_lookup_mac.o: ./src/engine/core_engine/lookup_mac.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_lookup_pbkdf.o: ./src/engine/core_engine/lookup_pbkdf.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine_core_engine_lookup_stream.o: ./src/engine/core_engine/lookup_stream.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/engine.o: ./src/engine/engine.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/entropy_dev_random.o: ./src/entropy/dev_random/dev_random.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_algo_filt.o: ./src/filters/algo_filt.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_basefilt.o: ./src/filters/basefilt.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_buf_filt.o: ./src/filters/buf_filt.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_codec_filt_b64_filt.o: ./src/filters/codec_filt/b64_filt.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_codec_filt_hex_filt.o: ./src/filters/codec_filt/hex_filt.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_data_snk.o: ./src/filters/data_snk.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_data_src.o: ./src/filters/data_src.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_filter.o: ./src/filters/filter.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_modes_cbc.o: ./src/filters/modes/cbc/cbc.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_modes_mode_pad.o: ./src/filters/modes/mode_pad/mode_pad.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_out_buf.o: ./src/filters/out_buf.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_pipe.o: ./src/filters/pipe.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_pipe_io.o: ./src/filters/pipe_io.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_pipe_rw.o: ./src/filters/pipe_rw.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/filters_secqueue.o: ./src/filters/secqueue.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/hash_mdx_hash.o: ./src/hash/mdx_hash/mdx_hash.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/hash_sha1_sha160.o: ./src/hash/sha1/sha160.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/hash_sha2_32.o: ./src/hash/sha2_32/sha2_32.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/hash_sha2_64.o: ./src/hash/sha2_64/sha2_64.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/kdf.o: ./src/kdf/kdf.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/kdf_mgf1.o: ./src/kdf/mgf1/mgf1.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_get_enc.o: ./src/libstate/get_enc.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_global_rng.o: ./src/libstate/global_rng.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_global_state.o: ./src/libstate/global_state.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_init.o: ./src/libstate/init.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate.o: ./src/libstate/libstate.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_lookup.o: ./src/libstate/lookup.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_oid_lookup_oids.o: ./src/libstate/oid_lookup/oids.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_policy.o: ./src/libstate/policy.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/libstate_scan_name.o: ./src/libstate/scan_name.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/mac_hmac.o: ./src/mac/hmac/hmac.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/mac.o: ./src/mac/mac.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_bigint_big_code.o: ./src/math/bigint/big_code.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_bigint_big_io.o: ./src/math/bigint/big_io.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_bigint_big_ops2.o: ./src/math/bigint/big_ops2.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_bigint_big_ops3.o: ./src/math/bigint/big_ops3.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_bigint_big_rand.o: ./src/math/bigint/big_rand.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_bigint.o: ./src/math/bigint/bigint.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_bigint_divide.o: ./src/math/bigint/divide.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_mp_mp_asm.o: ./src/math/mp/mp_asm.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_mp_mp_comba.o: ./src/math/mp/mp_comba.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_mp_mp_karat.o: ./src/math/mp/mp_karat.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_mp_mp_misc.o: ./src/math/mp/mp_misc.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_mp_mp_monty.o: ./src/math/mp/mp_monty.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_mp_mp_mulop.o: ./src/math/mp/mp_mulop.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_mp_mp_shift.o: ./src/math/mp/mp_shift.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_dsa_gen.o: ./src/math/numbertheory/dsa_gen.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_jacobi.o: ./src/math/numbertheory/jacobi.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_make_prm.o: ./src/math/numbertheory/make_prm.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_mp_numth.o: ./src/math/numbertheory/mp_numth.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_numthry.o: ./src/math/numbertheory/numthry.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_pow_mod.o: ./src/math/numbertheory/pow_mod.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_powm_fw.o: ./src/math/numbertheory/powm_fw.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_powm_mnt.o: ./src/math/numbertheory/powm_mnt.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_primes.o: ./src/math/numbertheory/primes.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_reducer.o: ./src/math/numbertheory/reducer.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/math_numbertheory_ressol.o: ./src/math/numbertheory/ressol.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/mutex_noop_mutex_mux_noop.o: ./src/mutex/noop_mutex/mux_noop.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pbe_get_pbe.o: ./src/pbe/get_pbe.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pbe_pbes1.o: ./src/pbe/pbes1/pbes1.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pbe_pbes2.o: ./src/pbe/pbes2/pbes2.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pbkdf_pbkdf1.o: ./src/pbkdf/pbkdf1/pbkdf1.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pbkdf_pbkdf2.o: ./src/pbkdf/pbkdf2/pbkdf2.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pk_pad_eme.o: ./src/pk_pad/eme.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pk_pad_eme_pkcs.o: ./src/pk_pad/eme_pkcs/eme_pkcs.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pk_pad_emsa1.o: ./src/pk_pad/emsa1/emsa1.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pk_pad_emsa3.o: ./src/pk_pad/emsa3/emsa3.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pk_pad_emsa4.o: ./src/pk_pad/emsa4/emsa4.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pk_pad_hash_id.o: ./src/pk_pad/hash_id/hash_id.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_blinding.o: ./src/pubkey/blinding.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_if_algo.o: ./src/pubkey/if_algo/if_algo.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_keypair.o: ./src/pubkey/keypair/keypair.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_pk_algs.o: ./src/pubkey/pk_algs.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_pk_keys.o: ./src/pubkey/pk_keys.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_pkcs8.o: ./src/pubkey/pkcs8.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey.o: ./src/pubkey/pubkey.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_pubkey_enums.o: ./src/pubkey/pubkey_enums.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_rsa.o: ./src/pubkey/rsa/rsa.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_workfactor.o: ./src/pubkey/workfactor.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/pubkey_x509_key.o: ./src/pubkey/x509_key.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/rng_hmac_rng.o: ./src/rng/hmac_rng/hmac_rng.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/rng.o: ./src/rng/rng.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/stream_stream_cipher.o: ./src/stream/stream_cipher.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_assert.o: ./src/utils/assert.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_charset.o: ./src/utils/charset.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_cpuid.o: ./src/utils/cpuid.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_datastor.o: ./src/utils/datastor/datastor.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_mlock.o: ./src/utils/mlock.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_parsing.o: ./src/utils/parsing.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_time.o: ./src/utils/time.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_ui.o: ./src/utils/ui.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@

build/lib/utils_version.o: ./src/utils/version.cpp
	$(CXX) -Ibuild/include $(LIB_FLAGS) -c $? -o $@


build/checks/bench.o: ./checks/bench.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/bigint.o: ./checks/bigint.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/cvc_tests.o: ./checks/cvc_tests.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/dolook.o: ./checks/dolook.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/ec_tests.o: ./checks/ec_tests.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/ecdh.o: ./checks/ecdh.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/ecdsa.o: ./checks/ecdsa.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/misc.o: ./checks/misc.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/pk.o: ./checks/pk.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/pk_bench.o: ./checks/pk_bench.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/timer.o: ./checks/timer.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@

build/checks/validate.o: ./checks/validate.cpp
	$(CXX) -Ibuild/include $(CHECK_FLAGS) -c $? -o $@


# Link Commands
$(CHECK): $(LIBRARIES) $(CHECKOBJS)
	$(CXX) $(CHECKOBJS) $(STATIC_LIB) $(LINK_TO) -o $(CHECK)

$(STATIC_LIB): $(LIBOBJS)
	$(RM) $(STATIC_LIB)
	$(AR) $(STATIC_LIB) $(LIBOBJS)
	$(RANLIB) $(STATIC_LIB)

# Fake Targets
.PHONY = docs clean distclean install static

static: $(STATIC_LIB)

docs:
	$(COPY) readme.txt build/docs
	$(COPY) doc/*.txt build/docs/manual

clean:
	$(RM_R) build/lib/* build/checks/*
	$(RM) $(LIBRARIES) $(SYMLINK) $(CHECK)

distclean: clean
	$(RM_R) build
	$(RM) Makefile* $(CONFIG_SCRIPT) $(PKGCONFIG)
	$(RM) botan_all.cpp botan_all.h

install: $(LIBRARIES) docs
	$(ECHO) "Installing Botan into $(DESTDIR)... "
	$(MKDIR_INSTALL) $(DOCDIR)
	$(COPY_R) build/docs/* $(DOCDIR)

	$(MKDIR_INSTALL) $(HEADERDIR)
	for i in $(HEADERS); do \
	   $(INSTALL_CMD_DATA) $$i $(HEADERDIR); \
	 done

	$(MKDIR_INSTALL) $(LIBDIR)
	$(INSTALL_CMD_DATA) $(STATIC_LIB) $(LIBDIR)

	$(MKDIR_INSTALL) $(BINDIR)
	$(INSTALL_CMD_EXEC) $(CONFIG_SCRIPT) $(BINDIR)

	$(MKDIR_INSTALL) $(PKGCONF_DIR)
	$(INSTALL_CMD_DATA) $(PKGCONFIG) $(PKGCONF_DIR)
