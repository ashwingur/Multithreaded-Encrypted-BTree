#include "../btreestore.h"

int main() {
    /*
        Testing duplicate key insertion
    */
    void * helper = init_store(4, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[5];
    strcpy((char *) plaintext, "Sup!");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    btree_insert(3, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(4, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(20, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(22, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(21, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(80, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(1, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(3, plaintext, 19, encryption_key, nonce, helper);
    uint64_t output[5];
    btree_decrypt(50, &output, helper);
    btree_delete(60, helper);
    btree_delete(80, helper);
    btree_delete(80, helper);

    print_btree(helper);
    close_store(helper);

    return 0;
}
