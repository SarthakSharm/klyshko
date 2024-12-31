#define EXTERN
#include "vars.h"

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

typedef int (*ra_tls_create_key_and_crt_der_func)(uint8_t** der_key, size_t* der_key_size,
                                                  uint8_t** der_crt, size_t* der_crt_size);

int main(int argc, char** argv) {
    printf("Entered the CRG main function.");
    int ret;
    other_player_number     = 0;
    void* ra_tls_attest_lib = NULL;

    //**
    ra_tls_create_key_and_crt_der_func ra_tls_create_key_and_crt_der_f = NULL;
    // ACCESSING ATTESTATION LIBS
    //  Load RA-TLS attestation library if needed

    char attestation_type_str[32] = {0};
    ret = file_read("/dev/attestation/attestation_type", attestation_type_str,
                    sizeof(attestation_type_str) - 1);

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

    //**

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
    int kii_job_id_defined = kii_job_id_str ? atoi(kii_job_id_str) : 0;  // Check for NULL
    player_number_defined  = player_number_str ? atoi(player_number_str) : 0;
    number_of_players      = number_of_players_str ? atoi(number_of_players_str) : 0;
    // EOC for getting the env variables and storing them inside main fuction

    //     //***

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

    // strncpy(attestation_type_str, "dcap", sizeof(attestation_type_str) - 1);
    // attestation_type_str[sizeof(attestation_type_str) - 1] = '\0';

    //
    if (player_number_defined == 0)
        ssl_client_setup_and_handshake(argv[1], argv[2], argv[3], argv[4],
                                       ra_tls_create_key_and_crt_der_f);
    ssl_server_setup_and_handshake(argv[1], argv[2], argv[3], argv[4],
                                   ra_tls_create_key_and_crt_der_f);
}