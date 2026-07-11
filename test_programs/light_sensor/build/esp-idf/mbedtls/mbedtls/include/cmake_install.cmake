# Install script for directory: /esp/esp-idf/components/mbedtls/mbedtls/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/user/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mbedtls" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/aes.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/aria.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/asn1.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/asn1write.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/base64.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/bignum.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/block_cipher.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/build_info.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/camellia.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ccm.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/chacha20.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/chachapoly.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/check_config.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/cipher.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/cmac.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/compat-2.x.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_crypto.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_from_psa.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_from_legacy.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_superset_legacy.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_ssl.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_adjust_x509.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config_psa.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/constant_time.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ctr_drbg.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/debug.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/des.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/dhm.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecdh.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecdsa.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecjpake.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecp.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/entropy.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/error.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/gcm.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/hkdf.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/hmac_drbg.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/lms.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/mbedtls_config.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md5.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/memory_buffer_alloc.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/net_sockets.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/nist_kw.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/oid.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pem.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pk.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs12.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs5.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs7.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform_time.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform_util.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/poly1305.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/private_access.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/psa_util.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ripemd160.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/rsa.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha1.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha256.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha3.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha512.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_cache.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_ciphersuites.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_cookie.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_ticket.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/threading.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/timing.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/version.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_crl.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_crt.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_csr.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/psa" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/build_info.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_auto_enabled.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_dependencies.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_key_pair_types.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_synonyms.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_builtin_composites.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_builtin_key_derivation.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_builtin_primitives.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_compat.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_config.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_common.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_composites.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_key_derivation.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_primitives.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_extra.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_legacy.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_platform.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_se_driver.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_sizes.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_struct.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_types.h"
    "/esp/esp-idf/components/mbedtls/mbedtls/include/psa/crypto_values.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/esp/workspace/test_programs/light_sensor/build/esp-idf/mbedtls/mbedtls/include/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
