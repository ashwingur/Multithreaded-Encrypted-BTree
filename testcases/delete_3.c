#include "../btreestore.h"

int main() {
    /*
        Testing a delete with recursive rebalancing steps involved
    */
    void * helper = init_store(4, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[5];
    strcpy((char *) plaintext, "Sup!");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    btree_insert(1, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(2, plaintext, 14, encryption_key, nonce, helper);
    btree_insert(3, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(4, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(5, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(6, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(7, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(8, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(9, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(10, plaintext, 2, encryption_key, nonce, helper);
    print_btree(helper);
    printf("\n");

    btree_delete(8, helper);
    print_btree(helper);
    printf("\n");
    
    btree_delete(9, helper);
    print_btree(helper);
    printf("\n");
    btree_delete(6, helper);
    print_btree(helper);
    printf("\n");
    btree_delete(3, helper);
    print_btree(helper);

    close_store(helper);

    return 0;
}