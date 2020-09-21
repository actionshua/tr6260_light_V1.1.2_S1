#ifndef SHA2_H
#define SHA2_H

#define SHA256_DIGEST_SIZE ( 256 / 8)
#define SHA256_BLOCK_SIZE  ( 512 / 8)

typedef struct {
    unsigned int tot_len;
    unsigned int len;
    unsigned char block[2 * SHA256_BLOCK_SIZE];
    unsigned int h[8];
} sha256_ctx;

void SwSha256(const unsigned char *message, unsigned int len, unsigned char *digest);


#endif /* !SHA2_H */

