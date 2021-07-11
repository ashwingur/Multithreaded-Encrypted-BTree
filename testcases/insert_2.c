#include "../btreestore.h"

int main() {
    void * helper = init_store(3, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[1];
    uint64_t cipher[1];
    strcpy((char *) plaintext, "Hello");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {0,0,0,1};
    
    btree_insert(2, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(3, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(1, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(8, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(80, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(5, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(6, plaintext, 6, encryption_key, nonce, helper);
    
    print_btree(helper);
    close_store(helper);

    return 0;
}
