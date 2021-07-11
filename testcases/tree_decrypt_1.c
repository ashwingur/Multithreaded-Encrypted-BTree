#include "../btreestore.h"

int main() {
    void * helper = init_store(3, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[5];
    strcpy((char *) plaintext, "Hello");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    btree_insert(2, plaintext, 5, encryption_key, nonce, helper);
    btree_insert(7, plaintext, 5, encryption_key, nonce, helper);
    btree_insert(5, plaintext, 6, encryption_key, nonce, helper);
    strcpy((char *) plaintext, "My name is Ashwin!");
    btree_insert(15, plaintext, 19, encryption_key, nonce, helper);
    btree_insert(14, plaintext, 14, encryption_key, nonce, helper);
    btree_insert(11, plaintext, 2, encryption_key, nonce, helper);
    btree_insert(1, plaintext, 9, encryption_key, nonce, helper);

    struct info *found = malloc(sizeof(struct info));

    uint64_t output[10];
    btree_decrypt(15, output, helper);
    printf("%s\n", (char*) output);

    close_store(helper);
    free(found);

    return 0;
}
