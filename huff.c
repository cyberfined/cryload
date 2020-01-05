#include "huff.h"
#include "lib.h"

typedef struct htree {
    struct htree *left;
    struct htree *right;
    uint8_t byte;
} htree;

static inline size_t read_tree(htree *nodes, char *buf, size_t buf_len) {
    htree *cur = nodes;
    int next_node = 1;
    uint8_t read_byte = 0, byte, bits = 0;
    size_t ret_len = 0;

    cur->left = cur->right = NULL;

    while(ret_len < buf_len) {
        if(bits == 0) {
            bits = 8;
            byte = buf[ret_len++];
        }
        bits--;

        if(read_byte > 0) {
            read_byte--;
            cur->byte |= ((byte >> bits) & 1) << read_byte;

            if(read_byte == 0) {
                htree *p = cur->right;
                cur->right = NULL;
                if(p)
                    cur = p;
                else
                    break;
            }
            continue;
        }

        if(byte & (1 << bits)) {
            cur->byte = 0;
            read_byte = 8;
        } else {
            htree *left = &nodes[next_node++];
            htree *right = &nodes[next_node++];
            left->left = right->left = right->right = NULL;
            cur->left = left;
            left->right = right;
            if(cur->right)
                right->right = cur->right;
            cur->right = right;
            cur = left;
        }
    }

    return ret_len;
}

static inline void decode(char *in_buf, char *out_buf, htree *tree, size_t in_buf_len, size_t out_buf_len) {
    htree *node = tree;
    for(size_t i = 0; i < in_buf_len; i++) {
        uint8_t byte = in_buf[i];
        for(int j = 7; j >= 0; j--) {
            if(byte & (1 << j))
                node = node->right;
            else
                node = node->left;

            if(!node->left && !node->right) {
                *out_buf++ = node->byte;
                node = tree;
                out_buf_len--;
                if(out_buf_len == 0)
                    return;
            }
        }
    }
}

char* huffman_decode(char *in_buf, size_t in_len, size_t *ret_len) {
    uint32_t out_len;
    memcpy(&out_len, in_buf, sizeof(out_len));
    in_buf += sizeof(out_len);
    in_len -= sizeof(out_len);

    htree nodes[512];
    size_t in_buf_off = read_tree(nodes, in_buf, in_len);
    in_buf += in_buf_off;
    in_len -= in_buf_off;

    char *out_buf = mmap(NULL, out_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(out_buf == MAP_FAILED) {
        return NULL;
    }

    decode(in_buf, out_buf, nodes, in_len, out_len);

    *ret_len = out_len;
    return out_buf;
}
