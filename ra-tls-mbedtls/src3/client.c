#include "vars.h"

typedef int (*ra_tls_create_key_and_crt_der_func)(uint8_t** der_key, size_t* der_key_size,
                                                  uint8_t** der_crt, size_t* der_crt_size);

// extern void my_debug(void* ctx, int level, const char* file, int line, const char* str);

// extern int my_verify_callback(void* data, mbedtls_x509_crt* crt, int depth, uint32_t* flags);
//  extern int other_player_number;
//  extern int player_number_defined;
//  extern int number_of_players;
int ssl_client_setup_and_handshake() {
    void* ra_tls_attest_lib                                            = NULL;
    void* ra_tls_verify_lib                                            = NULL;
    ra_tls_create_key_and_crt_der_func ra_tls_create_key_and_crt_der_f = NULL;

    // Load RA-TLS attestation library if needed
    ra_tls_attest_lib = dlopen("libra_tls_verify_dcap_gramine.so", RTLD_LAZY);
    if (ra_tls_attest_lib) {
        ra_tls_create_key_and_crt_der_f = (ra_tls_create_key_and_crt_der_func)dlsym(
            ra_tls_attest_lib, "ra_tls_create_key_and_crt_der");
        if (!ra_tls_create_key_and_crt_der_f) {
            printf("Error loading RA-TLS create key and cert function\n");
            dlclose(ra_tls_attest_lib);
            ra_tls_attest_lib = NULL;
        }
    } else {
        printf("RA-TLS attestation library not found, proceeding without RA-TLS.\n");
    }

    ra_tls_verify_lib = dlopen("libra_tls_attest.so", RTLD_LAZY);
    if (ra_tls_verify_lib) {
        ra_tls_create_key_and_crt_der_f = (ra_tls_create_key_and_crt_der_func)dlsym(
            ra_tls_verify_lib, "ra_tls_create_key_and_crt_der");
        if (!ra_tls_create_key_and_crt_der_f) {
            printf("Error loading RA-TLS create key and cert function\n");
            dlclose(ra_tls_verify_lib);
            ra_tls_verify_lib = NULL;
        }
    } else {
        printf("RA-TLS attestation library not found, proceeding without RA-TLS.\n");
    }
    int ret;
    size_t len;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_net_context listen_fd;
    unsigned char buf[1024];
    const char* pers = "ssl_client";
    // get i(player_number) from the env variables and have a global variable
    // other_player_number(k)(which is from 0 to i-1);
    int other_player_number = 0;
    char* error;
    uint32_t flags;
    struct ra_tls_verify_callback_results my_verify_callback_results = {0};
    uint8_t* der_key                                                 = NULL;
    uint8_t* der_crt                                                 = NULL;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_pk_context pkey;
    mbedtls_x509_crt clicert;

    mbedtls_net_init(&listen_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_pk_init(&pkey);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
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

        //***$$$***
        if (ra_tls_attest_lib) {
            mbedtls_printf(
                "\n  . Creating the RA-TLS server cert and key (using dcap as "
                "attestation type)...");
            fflush(stdout);

            size_t der_key_size;
            size_t der_crt_size;

            ret = (*ra_tls_create_key_and_crt_der_f)(&der_key, &der_key_size, &der_crt,
                                                     &der_crt_size);
            if (ret != 0) {
                mbedtls_printf(" failed\n  !  ra_tls_create_key_and_crt_der returned %d\n\n", ret);
                goto exit;
            }

            ret = mbedtls_x509_crt_parse(&clicert, (unsigned char*)der_crt, der_crt_size);
            if (ret != 0) {
                mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret);
                goto exit;
            }

            ret = mbedtls_pk_parse_key(&pkey, (unsigned char*)der_key, der_key_size, /*pwd=*/NULL,
                                       0, mbedtls_ctr_drbg_random, &ctr_drbg);
            if (ret != 0) {
                mbedtls_printf(" failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret);
                goto exit;
            }

            mbedtls_printf(" ok\n");
        }
        //***$$$***

        mbedtls_printf("  . Connecting to tcp/%s/%s...", SERVER_NAME, SERVER_PORT);
        fflush(stdout);

        while (1) {
            ret = mbedtls_net_connect(&listen_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP);
            if (ret == 0)
                break;
            else {
                mbedtls_printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
                // mbedtls_printf("Retrying in %d seconds...\n", RETRY_DELAY);

                mbedtls_net_free(&listen_fd);
                mbedtls_net_init(&listen_fd);
                sleep(RETRY_DELAY);
            }
        }

        mbedtls_printf(" ok\n");

        mbedtls_printf("  . Setting up the SSL/TLS structure...");
        fflush(stdout);

        ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret != 0) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
            goto exit;
        }

        mbedtls_printf(" ok\n");

        fflush(stdout);

        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_printf(" ok\n");

        if (ra_tls_verify_lib) {
            /* use RA-TLS verification callback; this will overwrite CA chain set up above */
            mbedtls_printf("  . Installing RA-TLS callback ...");
            mbedtls_ssl_conf_verify(&conf, &my_verify_callback, &my_verify_callback_results);
            mbedtls_printf(" ok\n");
        }

        mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
        mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

        //***$$$***
        ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey);
        if (ret != 0) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
            goto exit;
        }
        //***$$$***

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

        mbedtls_ssl_set_bio(&ssl, &listen_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        mbedtls_printf("  . Performing the SSL/TLS handshake...");
        fflush(stdout);

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n", -ret);
                mbedtls_printf(
                    "  ! ra_tls_verify_callback_results:\n"
                    "    attestation_scheme=%d, err_loc=%d, \n",
                    my_verify_callback_results.attestation_scheme,
                    my_verify_callback_results.err_loc);
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

        mbedtls_printf("  . Verifying peer X.509 certificate...");

        flags = mbedtls_ssl_get_verify_result(&ssl);
        if (flags != 0) {
            char vrfy_buf[512];
            mbedtls_printf(" failed\n");
            mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
            mbedtls_printf("%s\n", vrfy_buf);

            /* verification failed for whatever reason, fail loudly */
            goto exit;
        } else {
            mbedtls_printf(" ok\n");
        }

        mbedtls_printf("  > Write to server:");
        fflush(stdout);

        len = sprintf((char*)buf, GET_REQUEST);

        PlayerInfo msg    = PLAYER_INFO__INIT;
        msg.kii_job_id    = "1920bb26-dsee-dzfw-vdsdsa14fds4";  // Example initialization
        msg.player_number = 1;                                  // Example initialization

        // Buffer for serialized data
        unsigned playlen = player_info__get_packed_size(&msg);
        if (playlen == 0) {
            fprintf(stderr, "Packing or serialization error\n");
        }

        void* buff = malloc(playlen);
        if (!buff) {
            fprintf(stderr, "Memory allocation error\n");
        }

        player_info__pack(&msg, buff);
        fprintf(stderr, "Writing %d serialized bytes\n", playlen);
        mbedtls_ssl_write(&ssl, buff, playlen);

        // code for packing the macshares and sending over the TLS dconnection again to the server
        SecretShare message   = SECRET_SHARE__INIT;
        message.mackeyshare_2 = "f0cf6099e629fd0bda2de3f9515ab72b";
        message.mackeyshare_p = "-88222337191559387830816715872691188861";
        message.seeds         = "adedefwklrewernf";
        unsigned lent         = secret_share__get_packed_size(&message);
        if (lent == 0) {
            fprintf(stderr, "packing or serialization error");
        }
        void* buffer = malloc(lent);
        if (!buffer) {
            fprintf(stderr, "Memory allocation error\n");
        }

        secret_share__pack(&message, buffer);
        fprintf(stderr, "Writing %d serialized bytes\n", lent);
        mbedtls_ssl_write(&ssl, buffer, lent);
        // EOC for sending
        //  code for unpacking the files from server side and result displaying
        uint8_t secret_buffer[MAX_MSG_SIZE];
        size_t secret_len = mbedtls_ssl_read(&ssl, secret_buffer, sizeof(secret_buffer));

        if (secret_len <= 0) {
            fprintf(stderr, "SSL read error: %ld\n", secret_len);
        }
        SecretShare* secret_message;
        secret_message = secret_share__unpack(NULL, secret_len, secret_buffer);
        if (secret_message == NULL) {
            fprintf(stderr, "Error unpacking incoming message\n");
        }

        // Display the message's fields
        printf("Received: mackeyshare_2=%s", secret_message->mackeyshare_2);  // required field
        printf("  mackeyshare_p=%s\n", secret_message->mackeyshare_p);
        printf("  seeds=%s\n", secret_message->seeds);
        // Free the unpacked message
        secret_share__free_unpacked(secret_message, NULL);
        mbedtls_ssl_close_notify(&ssl);
        exit_code = MBEDTLS_EXIT_SUCCESS;
    }
exit:

#ifdef MBEDTLS_ERROR_C
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif
    if (ra_tls_verify_lib)
        dlclose(ra_tls_verify_lib);

    mbedtls_net_free(&listen_fd);
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    free(der_key);
    free(der_crt);
    printf("Hello from client");
    // return ret;
    return 0;
}
