#include "../btreestore.h"

int main() {
    /*
        Testing a delete where the leaf node required rebalancing with one of it's siblings only once
    */
    void * helper = init_store(4, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[5];
    strcpy((char *) plaintext, "Sup!");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    btree_insert(10, plaintext, 5, encryption_key, nonce, helper);
    btree_insert(20, plaintext, 5, encryption_key, nonce, helper);
    btree_insert(30, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(1, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(2, plaintext, 14, encryption_key, nonce, helper);
    btree_insert(3, plaintext, 2, encryption_key, nonce, helper);
    print_btree(helper);
    printf("\n"); 

    btree_delete(20, helper);
    print_btree(helper);
    printf("\n");

    // Getting a key from left sibling
    btree_delete(30, helper);
    print_btree(helper);

    btree_insert(20, plaintext, 5, encryption_key, nonce, helper);
    btree_delete(1, helper);

    // Must borrow key from right sibling
    printf("\n");
    print_btree(helper);
    btree_delete(2, helper);
    printf("\n");
    print_btree(helper);

    close_store(helper);

    return 0;
}