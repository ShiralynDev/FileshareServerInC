#include <stdlib.h>
#include <string.h>
#include <stdint.h>

char* base64UrlsafeEncode(const unsigned char *data, size_t input_length, size_t *output_length) {
    static const char encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    static const int mod_table[] = {0, 2, 1};

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = (char *)malloc(*output_length + 1);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded_data[j++] = encoding_table[(triple >> 18) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 12) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    encoded_data[*output_length] = '\0';
    return encoded_data;
}

static char *build_decoding_table(const char *encoding_table) {
    char *decoding_table = malloc(256);
    if (!decoding_table) return NULL;

    for (int i = 0; i < 256; i++)
        decoding_table[i] = -1;

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;

    return decoding_table;
}

size_t base64Decode(const char *encoded_data, unsigned char **decoded_data, size_t *decoded_length, int urlsafe) {
    static const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static const char *base64_urlsafe_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    const char *encoding_table = urlsafe ? base64_urlsafe_chars : base64_chars;
    char *decoding_table = build_decoding_table(encoding_table);
    if (!decoding_table) return 0;

    size_t input_length = strlen(encoded_data);
    size_t output_length = input_length / 4 * 3;

    if (encoded_data[input_length - 1] == '=') output_length--;
    if (encoded_data[input_length - 2] == '=') output_length--;

    *decoded_data = (unsigned char *)calloc(output_length, 1);
    if (!*decoded_data) {
        free(decoding_table);
        return 0;
    }

    size_t j = 0;
    uint32_t triple = 0;
    int collected = 0;

    for (size_t i = 0; i < input_length; ++i) {
        char c = encoded_data[i];

        if (c == '=') break;  // Stop decoding when padding '=' is encountered

        int val = decoding_table[(unsigned char)c];
        if (val == -1) continue; // Skip any invalid characters

        triple = (triple << 6) | val;
        collected++;

        if (collected == 4) {
            (*decoded_data)[j++] = (triple >> 16) & 0xFF;
            (*decoded_data)[j++] = (triple >> 8) & 0xFF;
            (*decoded_data)[j++] = triple & 0xFF;
            triple = 0;
            collected = 0;
        }
    }

    // Handle remaining bytes (1 or 2) after the loop
    if (collected == 3) {
        (*decoded_data)[j++] = (triple >> 10) & 0xFF;
        (*decoded_data)[j++] = (triple >> 2) & 0xFF;
    } else if (collected == 2) {
        (*decoded_data)[j++] = (triple >> 4) & 0xFF;
    }

    *decoded_length = j;
    *decoded_data = realloc(*decoded_data, j);

    free(decoding_table);
    return *decoded_length;
}
