#include "../btreestore.h"

int main() {
    /*
        Testing a large amount of deletions on a big tree
    */
    void * helper = init_store(7, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[5];
    strcpy((char *) plaintext, "Sup!");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    for (int i = 0; i < 1000; i++){
        btree_insert(i, plaintext, 19, encryption_key, nonce, helper);
    }

    for (int i = 999; i >= 0; i--){
        if (i == 300){
            continue;
        }
        btree_delete(i, helper);
    }

    print_btree(helper);
    close_store(helper);

    return 0;
}
