#include "../btreestore.h"

int main() {
    /*
        Testing a basic delete, where the node to be deleted is in a leaf node and also
        after its deleted the leaf node has enough keys and does not require any further rebalancing.
    */
    void * helper = init_store(4, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[5];
    strcpy((char *) plaintext, "Sup!");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    btree_insert(5, plaintext, 5, encryption_key, nonce, helper);
    btree_insert(6, plaintext, 5, encryption_key, nonce, helper);
    btree_insert(7, plaintext, 6, encryption_key, nonce, helper);
    btree_insert(8, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(3, plaintext, 14, encryption_key, nonce, helper);
    btree_insert(4, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(9, plaintext, 9, encryption_key, nonce, helper);

    print_btree(helper);
    printf("\n");

    btree_delete(4, helper);
    print_btree(helper);

    close_store(helper);

    return 0;
}