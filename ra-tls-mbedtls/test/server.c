
#include "vars.h"

typedef int (*ra_tls_create_key_and_crt_der_func)(uint8_t** der_key, size_t* der_key_size,
                                                  uint8_t** der_crt, size_t* der_crt_size);

typedef int (*ra_tls_verify_callback_extended_der_f)(
    uint8_t* der_crt, size_t der_crt_size, struct ra_tls_verify_callback_results* results);
typedef void (*ra_tls_set_measurement_callback_f)(int (*f_cb)(
    const char* mrenclave, const char* mrsigner, const char* isv_prod_id, const char* isv_svn));

int ssl_server_setup_and_handshake(
    ra_tls_create_key_and_crt_der_func ra_tls_create_key_and_crt_der_f,
    ra_tls_verify_callback_extended_der_f ra_tls_verify_callback_extended_der_f,
    ra_tls_set_measurement_callback_f ra_tls_set_measurement_callback_f) {
    int ret          = 0;
    const char* pers = "ssl_server";
    unsigned char buf[1024];
    struct ra_tls_verify_callback_results my_verify_callback_results = {0};

    // Initialize mbedtls structures
    mbedtls_net_context listen_fd, client_fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;

    uint8_t* der_key = NULL;
    uint8_t* der_crt = NULL;
    size_t der_key_size, der_crt_size;

    mbedtls_net_init(&listen_fd);
    mbedtls_net_init(&client_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_x509_crt_init(&srvcert);
    mbedtls_pk_init(&pkey);

    uint32_t flags;
    void* ra_tls_verify_lib;

    // Seed the random number generator
    printf("  . Seeding the random number generator...");
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char*)pers, strlen(pers))) != 0) {
        printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
    }
    printf(" ok\n");

    // Generate RA-TLS certificate if function pointer is provided
    if (ra_tls_create_key_and_crt_der_f) {
        printf("  . Creating the RA-TLS server cert and key...");
        ret = (*ra_tls_create_key_and_crt_der_f)(&der_key, &der_key_size, &der_crt, &der_crt_size);
        if (ret != 0) {
            printf(" failed\n  ! ra_tls_create_key_and_crt_der returned %d\n\n", ret);
            goto exit;
        }

        ret = mbedtls_x509_crt_parse(&srvcert, der_crt, der_crt_size);
        if (ret != 0) {
            printf(" failed\n  ! mbedtls_x509_crt_parse returned %d\n\n", ret);
            goto exit;
        }

        ret = mbedtls_pk_parse_key(&pkey, der_key, der_key_size, NULL, 0, mbedtls_ctr_drbg_random,
                                   &ctr_drbg);
        if (ret != 0) {
            printf(" failed\n  ! mbedtls_pk_parse_key returned %d\n\n", ret);
            goto exit;
        }
        printf(" ok\n");
    }

    mbedtls_printf("  . Bind on https://localhost:%s/ ...", SERVER_PORT);
    fflush(stdout);

    ret = mbedtls_net_bind(&listen_fd, NULL, SERVER_PORT, MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_net_bind returned %d\n\n", ret);
        goto exit;
    }
    mbedtls_printf(" ok\n");

    mbedtls_printf("  . Setting up the SSL data....");
    fflush(stdout);
    // SSL Configuration
    ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        goto exit;
    }

    // mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);

    // // if (ra_tls_verify_lib) {
    // /* use RA-TLS verification callback; this will overwrite CA chain set up above */
    // mbedtls_printf("  . Installing RA-TLS callback ...");
    // mbedtls_ssl_conf_verify(&conf, &my_verify_callback, &my_verify_callback_results);
    // mbedtls_printf(" ok\n");
    // // }

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
    // mbedtls_debug_set_threshold(4);

    ret = mbedtls_ssl_setup(&ssl, &conf);
    if (ret != 0) {
        printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        goto exit;
    }
    mbedtls_printf(" ok\n");

    ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        goto exit;
    }

reset:
    if (other_player_number < player_number_defined) {
        other_player_number++;
    } else {
        goto exit;
    }
    // Wait for a client connection
    mbedtls_net_free(&client_fd);

    mbedtls_ssl_session_reset(&ssl);

    mbedtls_printf("  . Waiting for a remote connection ...");
    fflush(stdout);

    ret = mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL);
    if (ret != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_net_accept returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(&ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    mbedtls_printf(" ok\n");

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
        }

        goto exit;
    }

    mbedtls_printf(" ok\n");

    // mbedtls_printf("  . Verifying peer X.509 certificate...");

    // //****$$****
    // flags = mbedtls_ssl_get_verify_result(&ssl);
    // if (flags != 0) {
    //     char vrfy_buf[512];
    //     mbedtls_printf(" failed\n");
    //     mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
    //     mbedtls_printf("%s\n", vrfy_buf);

    //     /* verification failed for whatever reason, fail loudly */
    //     goto exit;
    // } else {
    //     mbedtls_printf(" ok\n");
    // }
    //****$$****

    // mbedtls_printf("  < Read from client:");
    // fflush(stdout);
    // PlayerInfo* msg;
    // uint8_t buff[MAX_MSG_SIZE];
    // size_t play_len;

    // do {
    //     play_len = sizeof(buff) - 1;
    //     memset(buff, 0, sizeof(buff));
    //     ret = mbedtls_ssl_read(&ssl, buff, play_len);

    //     if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    //         continue;

    //     if (ret <= 0) {
    //         switch (ret) {
    //             case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
    //                 mbedtls_printf(" connection was closed gracefully\n");
    //                 break;

    //             case MBEDTLS_ERR_NET_CONN_RESET:
    //                 mbedtls_printf(" connection was reset by peer\n");
    //                 break;

    //             default:
    //                 mbedtls_printf(" mbedtls_ssl_read returned -0x%x\n", -ret);
    //                 break;
    //         }

    //         break;
    //     }

    //     play_len = ret;
    //     mbedtls_printf(" %ld bytes read\n\n%s", play_len, (char*)buff);

    //     if (ret > 0)
    //         break;
    // } while (1);

    // msg = player_info__unpack(NULL, play_len, buff);
    // if (msg == NULL) {
    //     fprintf(stderr, "Error unpacking incoming message\n");
    // }

    // // Display the message's fields
    // printf("Received: kii_job_id=%s", msg->kii_job_id);  // required field
    // printf("  player_number=%d\n", msg->player_number);
    // int returncode = verify_player_details(msg->kii_job_id, msg->player_number, kii_job_id_str,
    //                                        player_number_defined);
    // if (returncode == -1) {
    //     printf("KII_JOB_ID or PLAYER_ID not valid.\n");
    //     int rcd = mbedtls_ssl_close_notify(&ssl);
    //     while (rcd < 0) {
    //         rcd = mbedtls_ssl_close_notify(&ssl);
    //     }
    // }

    // // code for macshares and reading
    // uint8_t buffer[MAX_MSG_SIZE];
    // size_t msg_len;
    // do {
    //     msg_len = sizeof(buffer) - 1;
    //     memset(buf, 0, sizeof(buffer));
    //     ret = mbedtls_ssl_read(&ssl, buffer, msg_len);

    //     if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    //         continue;

    //     if (ret <= 0) {
    //         switch (ret) {
    //             case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
    //                 mbedtls_printf(" connection was closed gracefully\n");
    //                 break;

    //             case MBEDTLS_ERR_NET_CONN_RESET:
    //                 mbedtls_printf(" connection was reset by peer\n");
    //                 break;

    //             default:
    //                 mbedtls_printf(" mbedtls_ssl_read returned -0x%x\n", -ret);
    //                 break;
    //         }

    //         break;
    //     }

    //     msg_len = ret;
    //     mbedtls_printf(" %ld bytes read\n\n%s", msg_len, (char*)buffer);

    //     if (ret > 0)
    //         break;
    // } while (1);

    // SecretShare* message;
    // message = secret_share__unpack(NULL, msg_len, buffer);
    // if (message == NULL) {
    //     fprintf(stderr, "Error unpacking incoming message\n");
    // }

    // // Display the message's fields
    // printf("Received: mackeyshare_2=%s", message->mackeyshare_2);  // required field
    // printf("  mackeyshare_p=%s\n", message->mackeyshare_p);
    // printf("  seeds=%s\n", message->seeds);
    // // Free the unpacked message
    // secret_share__free_unpacked(message, NULL);

    // // code for sending the macshares and seed values from the server to client side

    // SecretShare secret_message   = SECRET_SHARE__INIT;
    // secret_message.mackeyshare_2 = "f0cf6099e629fd0bda2de3f9515ab72b";
    // secret_message.mackeyshare_p = "-88222337191559387830816715872691188861";
    // secret_message.seeds         = "adedefwklrewernfserver";
    // unsigned lenth               = secret_share__get_packed_size(&secret_message);
    // if (lenth == 0) {
    //     fprintf(stderr, "packing or serialization error");
    // }
    // void* secret_buffer = malloc(lenth);
    // if (!secret_buffer) {
    //     fprintf(stderr, "Memory allocation error\n");
    // }

    // secret_share__pack(&secret_message, secret_buffer);
    // fprintf(stderr, "Writing %d serialized bytes\n", lenth);
    // while ((ret = mbedtls_ssl_write(&ssl, secret_buffer, lenth)) <= 0) {
    //     if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
    //         mbedtls_printf(" failed\n  ! peer closed the connection\n\n");
    //         goto reset;
    //     }

    //     if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
    //         mbedtls_printf(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
    //         goto exit;
    //     }
    // }

    // lenth = ret;
    // mbedtls_printf(" %d bytes written\n\n%s\n", lenth, (char*)secret_buffer);

    // fflush(stdout);

    // pack

    mbedtls_printf("  . Closing the connection...");

    while ((ret = mbedtls_ssl_close_notify(&ssl)) < 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_close_notify returned %d\n\n", ret);
            goto reset;
        }
    }

    mbedtls_printf(" ok\n");

    ret = 0;
    // goto reset;

#ifdef MBEDTLS_ERROR_C
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif

exit:
#ifdef MBEDTLS_ERROR_C
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif
    //     mbedtls_net_free(&listen_fd);
    //     mbedtls_net_free(&client_fd);
    //     mbedtls_x509_crt_free(&srvcert);
    //     mbedtls_pk_free(&pkey);
    //     mbedtls_ssl_free(&ssl);
    //     mbedtls_ssl_config_free(&conf);
    //     mbedtls_ctr_drbg_free(&ctr_drbg);
    //     mbedtls_entropy_free(&entropy);

    //     if (der_key)
    //         free(der_key);
    //     if (der_crt)
    //         free(der_crt);

    //     mbedtls_printf("other_player_number is %d", other_player_number);
    //     return ret;

    // cleanup:
    // Clean up resources
    mbedtls_net_free(&listen_fd);
    mbedtls_net_free(&client_fd);
    mbedtls_x509_crt_free(&srvcert);
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    if (der_key)
        free(der_key);
    if (der_crt)
        free(der_crt);

    mbedtls_printf("other_player_number is %d", other_player_number);
    return ret;
}