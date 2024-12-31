
#define EXTERN
#include "vars.h"
// extern int ssl_server_setup_and_handshake();
// extern int ssl_client_setup_and_handshake();
// edited
// edited ash06
int (*ra_tls_verify_callback_extended_der_f)(uint8_t* der_crt, size_t der_crt_size,
                                             struct ra_tls_verify_callback_results* results);

/* RA-TLS: if specified in command-line options, use our own callback to verify SGX measurements */
void (*ra_tls_set_measurement_callback_f)(int (*f_cb)(const char* mrenclave, const char* mrsigner,
                                                      const char* isv_prod_id,
                                                      const char* isv_svn));
// ash06

typedef int (*ra_tls_create_key_and_crt_der_func)(uint8_t** der_key, size_t* der_key_size,
                                                  uint8_t** der_crt, size_t* der_crt_size);

void my_debug(void* ctx, int level, const char* file, int line, const char* str) {
    ((void)level);

    mbedtls_fprintf((FILE*)ctx, "%s:%04d: %s\n", file, line, str);
    fflush((FILE*)ctx);
}
// edited
static int parse_hex(const char* hex, void* buffer, size_t buffer_size) {
    if (strlen(hex) != buffer_size * 2)
        return -1;

    for (size_t i = 0; i < buffer_size; i++) {
        if (!isxdigit(hex[i * 2]) || !isxdigit(hex[i * 2 + 1]))
            return -1;
        sscanf(hex + i * 2, "%02hhx", &((uint8_t*)buffer)[i]);
    }
    return 0;
}

int verify_player_details(char* kii_job_id, int player_number, char* kii_job_id_defined,
                          int player_number_defined) {
    if (strcmp(kii_job_id, kii_job_id_defined) == 0 && player_number == player_number_defined) {
        return 0;
    } else {
        return -1;
    }
}
// edited
static ssize_t file_read(const char* path, char* buf, size_t count) {
    FILE* f = fopen(path, "r");
    if (!f)
        return -errno;

    ssize_t bytes = fread(buf, 1, count, f);
    if (bytes <= 0) {
        int errsv = errno;
        fclose(f);
        return -errsv;
    }

    int close_ret = fclose(f);
    if (close_ret < 0)
        return -errno;

    return bytes;
}

/* expected SGX measurements in binary form */
static char g_expected_mrenclave[32];
static char g_expected_mrsigner[32];
static char g_expected_isv_prod_id[2];
static char g_expected_isv_svn[2];

static bool g_verify_mrenclave   = false;
static bool g_verify_mrsigner    = false;
static bool g_verify_isv_prod_id = false;
static bool g_verify_isv_svn     = false;

/* RA-TLS: our own callback to verify SGX measurements */
static int my_verify_measurements(const char* mrenclave, const char* mrsigner,
                                  const char* isv_prod_id, const char* isv_svn) {
    assert(mrenclave && mrsigner && isv_prod_id && isv_svn);

    if (g_verify_mrenclave && memcmp(mrenclave, g_expected_mrenclave, sizeof(g_expected_mrenclave)))
        return -1;

    if (g_verify_mrsigner && memcmp(mrsigner, g_expected_mrsigner, sizeof(g_expected_mrsigner)))
        return -1;

    if (g_verify_isv_prod_id &&
        memcmp(isv_prod_id, g_expected_isv_prod_id, sizeof(g_expected_isv_prod_id)))
        return -1;

    if (g_verify_isv_svn && memcmp(isv_svn, g_expected_isv_svn, sizeof(g_expected_isv_svn)))
        return -1;

    return 0;
}

/* RA-TLS: mbedTLS-specific callback to verify the x509 certificate */
int my_verify_callback(void* data, mbedtls_x509_crt* crt, int depth, uint32_t* flags) {
    if (depth != 0) {
        /* the cert chain in RA-TLS consists of single self-signed cert, so we expect depth 0 */
        return MBEDTLS_ERR_X509_INVALID_FORMAT;
    }
    if (flags) {
        /* mbedTLS sets flags to signal that the cert is not to be trusted (e.g., it is not
         * correctly signed by a trusted CA; since RA-TLS uses self-signed certs, we don't care
         * what mbedTLS thinks and ignore internal cert verification logic of mbedTLS */
        *flags = 0;
    }
    return ra_tls_verify_callback_extended_der_f(crt->raw.p, crt->raw.len,
                                                 (struct ra_tls_verify_callback_results*)data);
}
// commented because the env is inside SGX
static bool getenv_client_inside_sgx() {
    char* str = getenv("RA_TLS_CLIENT_INSIDE_SGX");
    if (!str)
        return false;

    return !strcmp(str, "1") || !strcmp(str, "true") || !strcmp(str, "TRUE");
}

// initiate the connection and then ma

int main(int argc, char** argv) {
    int ret;
    // get i(player_number) from the env variables and have a global variable
    // other_player_number(k)(which is from 0 to i-1); int other_player_number = 0;
    other_player_number                   = 0;
    ra_tls_verify_callback_extended_der_f = NULL;
    ra_tls_set_measurement_callback_f     = NULL;

    //***
    ra_tls_create_key_and_crt_der_func ra_tls_create_key_and_crt_der_f = NULL;
    //***
    printf("Entered the CRG main function.");
    // creating cert, need to do that in client side also
    void* ra_tls_attest_lib = NULL;
    void* ra_tls_verify_lib = NULL;

    // // Load RA-TLS attestation library if needed
    // ra_tls_attest_lib = dlopen("libra_tls_attest.so", RTLD_LAZY);
    // if (ra_tls_attest_lib) {
    //     ra_tls_verify_callback_extended_der_f =
    //         dlsym(ra_tls_attest_lib, "ra_tls_create_key_and_crt_der");
    //     if (!ra_tls_verify_callback_extended_der_f) {
    //         printf("Error loading RA-TLS create key and cert function\n");
    //         dlclose(ra_tls_attest_lib);
    //         ra_tls_attest_lib = NULL;
    //     }
    // } else {
    //     printf("RA-TLS attestation library not found, proceeding without RA-TLS.\n");
    // }
    //***
    // Load RA-TLS attestation library if needed
    printf("line 156\n");
    ra_tls_attest_lib = dlopen("libra_tls_attest.so", RTLD_LAZY);
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
    printf("line 169\n");

    //***
    ra_tls_verify_lib = dlopen("libra_tls_verify_dcap_gramine.so", RTLD_LAZY);
    if (ra_tls_verify_lib) {
        ra_tls_verify_callback_extended_der_f =
            dlsym(ra_tls_verify_lib, "ra_tls_create_key_and_crt_der");
        if (!ra_tls_verify_callback_extended_der_f) {
            printf("Error loading RA-TLS create key and cert function\n");
            dlclose(ra_tls_verify_lib);
            ra_tls_verify_lib = NULL;
        }
    } else {
        printf("RA-TLS attestation library not found, proceeding without RA-TLS.\n");
    }
    // code for getting the environment variables inside the main function
    const char* env_names[] = {
        "KII_TUPLES_PER_JOB", "KII_SHARED_FOLDER",     "KII_TUPLE_FILE",
        "KII_PLAYER_NUMBER",  "KII_PLAYER_COUNT",      "KII_JOB_ID",
        "KII_TUPLE_TYPE",     "KII_PLAYER_ENDPOINT_1", "KII_PLAYER_ENDPOINT_0"};

    char* env_values[sizeof(env_names) / sizeof(env_names[0])];

    // Loop through each environment variable
    for (int i = 0; i < sizeof(env_names) / sizeof(env_names[0]); i++) {
        env_values[i] = getenv(env_names[i]);

        // Check if the environment variable exists and print the appropriate message
        if (env_values[i] == NULL) {
            fprintf(stderr, "Error: Environment variable %s not found.\n", env_names[i]);
        }
    }

    kii_job_id_str              = env_values[5];  // KII_JOB_ID
    char* player_number_str     = env_values[3];  // KII_PLAYER_NUMBER
    char* number_of_players_str = env_values[4];
    // Convert to integers
    // int kii_job_id_defined = kii_job_id_str ? atoi(kii_job_id_str) : 0;  // Check for NULL first
    player_number_defined = player_number_str ? atoi(player_number_str) : 0;
    number_of_players     = number_of_players_str ? atoi(number_of_players_str) : 0;
    // EOC for getting the env variables and storing them inside main fuction

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

    char attestation_type_str[32] = {0};
    ret = file_read("/dev/attestation/attestation_type", attestation_type_str,
                    sizeof(attestation_type_str) - 1);

    strncpy(attestation_type_str, "dcap", sizeof(attestation_type_str) - 1);
    attestation_type_str[sizeof(attestation_type_str) - 1] = '\0';

    if (argc > 1) {
        if (argc != 5) {
            mbedtls_printf(
                "USAGE: %s %s <expected mrenclave> <expected mrsigner>"
                " <expected isv_prod_id> <expected isv_svn>\n"
                "       (first two in hex, last two as decimal; set to 0 to ignore)\n",
                argv[0], argv[1]);
            return 1;
        }

        mbedtls_printf(
            "[ using our own SGX-measurement verification callback"
            " (via command line options) ]\n");

        g_verify_mrenclave   = true;
        g_verify_mrsigner    = true;
        g_verify_isv_prod_id = true;
        g_verify_isv_svn     = true;

        (*ra_tls_set_measurement_callback_f)(my_verify_measurements);

        if (!strcmp(argv[1], "0")) {
            mbedtls_printf("  - ignoring MRENCLAVE\n");
            g_verify_mrenclave = false;
        } else if (parse_hex(argv[1], g_expected_mrenclave, sizeof(g_expected_mrenclave)) < 0) {
            mbedtls_printf("Cannot parse MRENCLAVE!\n");
            return 1;
        }

        if (!strcmp(argv[2], "0")) {
            mbedtls_printf("  - ignoring MRSIGNER\n");
            g_verify_mrsigner = false;
        } else if (parse_hex(argv[2], g_expected_mrsigner, sizeof(g_expected_mrsigner)) < 0) {
            mbedtls_printf("Cannot parse MRSIGNER!\n");
            return 1;
        }

        if (!strcmp(argv[3], "0")) {
            mbedtls_printf("  - ignoring ISV_PROD_ID\n");
            g_verify_isv_prod_id = false;
        } else {
            errno                = 0;
            uint16_t isv_prod_id = (uint16_t)strtoul(argv[3], NULL, 10);
            if (errno) {
                mbedtls_printf("Cannot parse ISV_PROD_ID!\n");
                return 1;
            }
            memcpy(g_expected_isv_prod_id, &isv_prod_id, sizeof(isv_prod_id));
        }

        if (!strcmp(argv[4], "0")) {
            mbedtls_printf("  - ignoring ISV_SVN\n");
            g_verify_isv_svn = false;
        } else {
            errno            = 0;
            uint16_t isv_svn = (uint16_t)strtoul(argv[4], NULL, 10);
            if (errno) {
                mbedtls_printf("Cannot parse ISV_SVN\n");
                return 1;
            }
            memcpy(g_expected_isv_svn, &isv_svn, sizeof(isv_svn));
        }
    }
    // indices concern
    // end of the code ash06
    if (player_number_defined == 0)
        ssl_client_setup_and_handshake();
    ssl_server_setup_and_handshake(ra_tls_create_key_and_crt_der_f);

    ssl_client_setup_and_handshake(ra_tls_create_key_and_crt_der_f);
}
