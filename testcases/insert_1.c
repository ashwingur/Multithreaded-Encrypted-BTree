#include "../btreestore.h"

int main() {
    void * helper = init_store(3, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[1];
    uint64_t cipher[1];
    strcpy((char *) plaintext, "Hello");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {0,0,0,1};
    
    btree_insert(20, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(10, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(15, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(17, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(25, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(27, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(16, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(18, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(19, plaintext, 6, encryption_key, nonce, helper);

    print_btree(helper);
    close_store(helper);

    return 0;
}
