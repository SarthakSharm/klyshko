#include "vars.h"

typedef int (*ra_tls_create_key_and_crt_der_func)(uint8_t** der_key, size_t* der_key_size,
                                                  uint8_t** der_crt, size_t* der_crt_size);

typedef int (*ra_tls_verify_callback_extended_der_f)(
    uint8_t* der_crt, size_t der_crt_size, struct ra_tls_verify_callback_results* results);
typedef void (*ra_tls_set_measurement_callback_f)(int (*f_cb)(
    const char* mrenclave, const char* mrsigner, const char* isv_prod_id, const char* isv_svn));

int ssl_client_setup_and_handshake(
    ra_tls_create_key_and_crt_der_func ra_tls_create_key_and_crt_der_f,
    ra_tls_verify_callback_extended_der_f ra_tls_verify_callback_extended_der_f,
    ra_tls_set_measurement_callback_f ra_tls_set_measurement_callback_f) {
    int ret;
    size_t len;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_net_context server_fd;
    uint32_t flags;
    unsigned char buf[1024];
    const char* pers = "ssl_client";

    char* error;
    struct ra_tls_verify_callback_results my_verify_callback_results = {0};
    ra_tls_verify_callback_extended_der_f                            = NULL;
    ra_tls_set_measurement_callback_f                                = NULL;

    // void* ra_tls_verify_lib = NULL;
    // void* ra_tls_attest_lib = NULL;

    uint8_t* der_key = NULL;
    uint8_t* der_crt = NULL;
    size_t der_key_size;
    size_t der_crt_size;

    // ra_tls_verify_callback_extended_der_f                            = NULL;
    // ra_tls_set_measurement_callback_f                                = NULL;
    // struct ra_tls_verify_callback_results my_verify_callback_results = {0};

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_pk_context pkey;
    mbedtls_x509_crt clicert;

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_pk_init(&pkey);
    mbedtls_x509_crt_init(&clicert);

    for (other_player_number = number_of_players - 1;
         other_player_number >= player_number_defined + 1; other_player_number--) {
        // logic for client connection and secret sharing.

        mbedtls_printf("\n  . Seeding the random number generator...");
        fflush(stdout);

        ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    (const unsigned char*)pers, strlen(pers));
        if (ret != 0) {
            mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
            goto exit;
        }

        mbedtls_printf(" ok\n");
    }

    // //***$$$***
    // if (ra_tls_attest_lib) {
    mbedtls_printf(
        "\n  . Creating the RA-TLS server cert and key (using dcap as "
        "attestation type)...");
    fflush(stdout);

    ret = (*ra_tls_create_key_and_crt_der_f)(&der_key, &der_key_size, &der_crt, &der_crt_size);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  ra_tls_create_key_and_crt_der returned %d\n\n", ret);
        goto exit;
    }

    ret = mbedtls_x509_crt_parse(&clicert, (unsigned char*)der_crt, der_crt_size);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret);
        goto exit;
    }

    ret = mbedtls_pk_parse_key(&pkey, (unsigned char*)der_key, der_key_size, /*pwd=*/NULL, 0,
                               mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_printf(" ok\n");
    // }
    //***$$$***

    mbedtls_printf("  . Connecting to tcp/%s/%s...", SERVER_NAME, SERVER_PORT);
    fflush(stdout);

    while (1) {
        ret = mbedtls_net_connect(&server_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP);
        if (ret == 0)
            break;
        else {
            mbedtls_printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
            // mbedtls_printf("Retrying in %d seconds...\n", RETRY_DELAY);

            mbedtls_net_free(&server_fd);
            mbedtls_net_init(&server_fd);
            sleep(RETRY_DELAY);
        }
    }

    mbedtls_printf(" ok\n");

    mbedtls_printf("  . Setting up the SSL/TLS structure...");
    fflush(stdout);

    ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_printf(" ok\n");

    fflush(stdout);

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_printf(" ok\n");

    // if (ra_tls_verify_lib) {
    /* use RA-TLS verification callback; this will overwrite CA chain set up above */
    mbedtls_printf("  . Installing RA-TLS callback ...");
    mbedtls_ssl_conf_verify(&conf, &my_verify_callback, &my_verify_callback_results);
    mbedtls_printf(" ok\n");
    // }

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
    // mbedtls_debug_set_threshold(4);

    ret = mbedtls_ssl_setup(&ssl, &conf);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        goto exit;
    }

    ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    mbedtls_printf("  . Performing the SSL/TLS handshake...");
    fflush(stdout);

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n", -ret);
            mbedtls_printf(
                "  ! ra_tls_verify_callback_results:\n"
                "    attestation_scheme=%d, err_loc=%d, \n",
                my_verify_callback_results.attestation_scheme, my_verify_callback_results.err_loc);
            switch (my_verify_callback_results.attestation_scheme) {
                case RA_TLS_ATTESTATION_SCHEME_DCAP:
                    mbedtls_printf(
                        "    dcap.func_verify_quote_result=0x%x, "
                        "dcap.quote_verification_result=0x%x\n\n",
                        my_verify_callback_results.dcap.func_verify_quote_result,
                        my_verify_callback_results.dcap.quote_verification_result);
                    break;
                default:
                    mbedtls_printf("  ! unknown attestation scheme!\n\n");
                    break;
            }

            goto exit;
        }
    }

    mbedtls_printf(" ok\n");

    // mbedtls_printf("  . Verifying peer X.509 certificate...");

    // flags = mbedtls_ssl_get_verify_result(&ssl);
    // if (flags != 0) {
    //     char vrfy_buf[512];
    //     mbedtls_printf(" failed\n");
    //     mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
    //     mbedtls_printf("%s\n", vrfy_buf);

    /* verification failed for whatever reason, fail loudly */
    //     goto exit;
    // } else {ra_tls_verify_callback_extended_der_f
    //     mbedtls_printf(" ok\n");
    // }

    mbedtls_ssl_close_notify(&ssl);
    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

#ifdef MBEDTLS_ERROR_C
    if (exit_code != MBEDTLS_EXIT_SUCCESS) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif
    // if (ra_tls_verify_lib)
    //     dlclose(ra_tls_verify_lib);

    mbedtls_net_free(&server_fd);
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    free(der_key);
    free(der_crt);
    mbedtls_x509_crt_free(&clicert);
    mbedtls_pk_free(&pkey);

    mbedtls_printf("other_player_number is %d", other_player_number);
    return exit_code;
}