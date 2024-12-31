#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/build_info.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509.h"
#include "ra_tls.h"
#include "secretsharing.pb-c.h"

#define DEBUG_LEVEL     0
#define MALICIOUS_STR   "MALICIOUS DATA"
#define mbedtls_fprintf fprintf
#define mbedtls_printf  printf

#define SRV_CRT_PATH         "ssl/server.crt"
#define SRV_KEY_PATH         "ssl/server.key"
#define SERVER_PORT          "4444"
#define SERVER_NAME          "localhost"
#define GET_REQUEST          "GET / HTTP/1.0\r\n\r\n"
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#define DEBUG_LEVEL          0
#define MAX_MSG_SIZE         1024
#define RETRY_DELAY          1

#ifndef EXTERN
#define EXTERN extern
#endif

EXTERN int other_player_number;
EXTERN int player_number_defined;
EXTERN int number_of_players;
EXTERN char* kii_job_id_str;

EXTERN int ssl_client_setup_and_handshake();
EXTERN int ssl_server_setup_and_handshake();

EXTERN void my_debug(void* ctx, int level, const char* file, int line, const char* str);
EXTERN int verify_player_details(char* kii_job_id, int player_number, char* kii_job_id_defined,
                                 int player_number_defined);
EXTERN int my_verify_callback(void* data, mbedtls_x509_crt* crt, int depth, uint32_t* flags);