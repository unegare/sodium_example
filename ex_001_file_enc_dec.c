#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sodium.h>

#define CHUNK_SIZE 4096

static int
xcha_encrypt(const char *target_file, const char *source_file,
        const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE];
    unsigned char  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
    fwrite(header, 1, sizeof header, fp_t);
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
        crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, rlen,
                                                   NULL, 0, tag);
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);
    fclose(fp_t);
    fclose(fp_s);
    return 0;
}

static int
xcha_decrypt(const char *target_file, const char *source_file,
        const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  buf_out[CHUNK_SIZE];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    int            ret = -1;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    if (fread(header, 1, sizeof header, fp_s) != sizeof header) {
        fprintf(stderr, "decrypt: header reading error\n");
        goto ret;
    }
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
        goto ret; //incomplete header
    }
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
                                                       buf_in, rlen, NULL, 0) != 0) {
            goto ret; //corrupted chunk
        }
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL && ! eof) {
            goto ret; //premature end (end of file reached before the end of the stream)
        }
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);

    ret = 0;
ret:
    fclose(fp_t);
    fclose(fp_s);
    return ret;
}

static int
save_key(const char *filepath, const unsigned char *key, size_t len)
{
    FILE *fout;
    if (!(fout = fopen(filepath, "wb"))) {
        perror(filepath);
        return -1;
    }
    if (fwrite(key, 1, (size_t) len, fout) != len) {
        fprintf(stderr, "Key saving error\n");
        fclose(fout);
        return -1;
    }
    fclose(fout);
    return 0;
}

static char *dir = NULL;
static char *fname = NULL;
static char *encrypted_path = NULL;
static char *decrypted_path = NULL;
static char *key_path = NULL;

static void
for_exit() {
    free(dir);
    free(fname);
    free(encrypted_path);
    free(decrypted_path);
    free(key_path);
}

int
main(int argc, char *argv[])
{
    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];

    int           opt = 0;
    char         *input_file = NULL;
//    char         *encrypted_path = NULL;
//    char         *decrypted_path = NULL;
    struct stat   fin_stats;

    atexit(for_exit);
    while ((opt = getopt(argc, argv, "i:")) != -1) {
        switch (opt) {
        case 'i': {
            char *ts1 = NULL;
            char *ts2 = NULL;
            if (input_file != NULL) {
                fprintf(stderr, "There must be only one input file\n");
                exit(EXIT_FAILURE);
            }
            input_file = optarg;
            if (!((ts1 = strdup(input_file)) && (ts2 = strdup(input_file)))) {
                free(ts1);
                free(ts2);
                fprintf(stderr, "Memory allocation error\n");
                exit(EXIT_FAILURE);
            }
            dir = strdup(dirname(ts1));
            fname = strdup(basename(ts2));
            free(ts1);
            free(ts2);
            if (!(dir && fname)) {
                fprintf(stderr, "Memory allocation error\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        default:
            fprintf(stderr, "Usage: %s -i input_file\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!input_file) {
        fprintf(stderr, "Usage: %s -i input_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (stat(input_file, &fin_stats) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if (!((encrypted_path = (char*)malloc(snprintf(NULL, 0, "%s/encrypted", dir)))
            && (decrypted_path = (char*)malloc(snprintf(NULL, 0, "%s/decrypted", dir)))
            && (key_path = (char*)malloc(snprintf(NULL, 0, "%s/key", dir))))) {
        perror("paths");
        exit(EXIT_FAILURE);
    }

    sprintf(encrypted_path, "%s/encrypted", dir);
    sprintf(decrypted_path, "%s/decrypted", dir);
    sprintf(key_path, "%s/key", dir);

    if (sodium_init() != 0) {
        return 1;
    }
    crypto_secretstream_xchacha20poly1305_keygen(key);
    if (xcha_encrypt(encrypted_path, input_file, key) != 0) {
        return 1;
    }
    if (xcha_decrypt(decrypted_path, encrypted_path, key) != 0) {
        return 1;
    }
    if (save_key(key_path, key, crypto_secretstream_xchacha20poly1305_KEYBYTES) != 0) {
        return 1;
    }
    return 0;
}
