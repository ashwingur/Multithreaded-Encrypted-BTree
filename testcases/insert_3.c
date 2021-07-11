#include "../btreestore.h"

int main() {
    void * helper = init_store(4, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[1];
    uint64_t cipher[1];
    strcpy((char *) plaintext, "Hello");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {0,0,0,1};

    btree_insert(50, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(25, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(75, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(100, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(18, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(45, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(68, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(69, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(42, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(21, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(55, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(11, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(89, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(17, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(7, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(8, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(39, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(52, plaintext, 6, encryption_key, nonce, helper);
    
    print_btree(helper);
    close_store(helper);

    return 0;
}
