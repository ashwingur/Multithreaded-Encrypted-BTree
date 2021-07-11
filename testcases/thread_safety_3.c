#include "../btreestore.h"
#define SIZE 10
#define AMOUNT 500

/*
    Deleting simultaneously
*/

void *insert_keys(void *args){
    uint64_t plaintext[SIZE];
    memset(plaintext, 0, sizeof(uint64_t) * SIZE);
    uint64_t nonce = 420;
    uint32_t encryption_key[4] = {-1242664931,-888618071,-788595554,-1797464743};

    for (int i = 0; i < AMOUNT * 2; i++){
        btree_insert(i, plaintext, SIZE, encryption_key, nonce, args);
    }
}

void *delete_keys(void *args){
    for (int i = 0; i < AMOUNT; i++){
        btree_delete(i, args);
    }
}

void *delete_keys_2(void *args){
    for (int i = AMOUNT; i < AMOUNT * 2; i++){
        btree_delete(i, args);
    }
}

int main() {
    void * helper = init_store(20, 4);
    struct b_tree *tree = helper;

    pthread_t threads[3];

    pthread_create(&threads[0], NULL, insert_keys, helper);
    pthread_join(threads[0], NULL);
    pthread_create(&threads[1], NULL, delete_keys_2, helper);
    pthread_create(&threads[2], NULL, delete_keys, helper);

    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    print_btree(helper);

    close_store(helper);

    return 0;
}
