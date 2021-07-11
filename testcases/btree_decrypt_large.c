#include "../btreestore.h"
#define SIZE 100000

int main() {
    void * helper = init_store(3, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[SIZE];
    memset(plaintext, 0, sizeof(uint64_t) * SIZE);
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    btree_insert(2, plaintext, SIZE, encryption_key, nonce, helper);
    btree_insert(3, plaintext, SIZE, encryption_key, nonce, helper);
    btree_insert(4, plaintext, SIZE, encryption_key, nonce, helper);
    btree_insert(5, plaintext, SIZE, encryption_key, nonce, helper);
    btree_insert(6, plaintext, SIZE, encryption_key, nonce, helper);
    btree_insert(7, plaintext, SIZE, encryption_key, nonce, helper);
    btree_insert(8, plaintext, SIZE, encryption_key, nonce, helper);
    btree_insert(9, plaintext, SIZE, encryption_key, nonce, helper);

    uint64_t output[SIZE];
    btree_decrypt(2, output, helper);
    btree_decrypt(3, output, helper);
    btree_decrypt(4, output, helper);
    btree_decrypt(5, output, helper);
    btree_decrypt(6, output, helper);
    btree_decrypt(7, output, helper);
    btree_decrypt(8, output, helper);
    btree_decrypt(9, output, helper);

    close_store(helper);

    return 0;
}
