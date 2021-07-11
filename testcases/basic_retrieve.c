#include "../btreestore.h"

int main() {
    void * helper = init_store(3, 4);
    struct b_tree *tree = helper;

    uint64_t plaintext[1];
    uint64_t cipher[1];
    strcpy((char *) plaintext, "Hello");
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {0,0,0,1};

    btree_insert(1, plaintext, 6, encryption_key, nonce, helper);
    strcpy((char *) plaintext, "Yeeet");
    btree_insert(2, plaintext, 6, encryption_key, nonce, helper);
    strcpy((char *) plaintext, "Seven");
    btree_insert(3, plaintext, 6, encryption_key, nonce, helper);
    strcpy((char *) plaintext, "Eight");
    btree_insert(4, plaintext, 6, encryption_key, nonce, helper);
    strcpy((char *) plaintext, "Epic!");
    btree_insert(5, plaintext, 6, encryption_key, nonce, helper);
    strcpy((char *) plaintext, "ffour");
    btree_insert(6, plaintext, 6, encryption_key, nonce, helper);

    struct info *found = malloc(sizeof(struct info));
    
    if (btree_retrieve(5, found, helper) != 0){
        printf("Retrieve did not return 0 when it should have\n");
    }
    printf("%ld, %d%d%d%d\n", found->nonce, found->key[0], found->key[1], found->key[2], found->key[3]);

    uint64_t result[1];
    decrypt_tea_ctr((uint64_t*) found->data, found->key, found->nonce, result, ceil(found->size / 8.0));
    printf("Inputted: Epic!, got: %s\n", (char*) result);

    close_store(helper);
    free(found);

    return 0;
}
