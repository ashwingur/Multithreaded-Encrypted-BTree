#ifndef BTREESTORE_H
#define BTREESTORE_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define DELTA 0x9E3779B9
#define THREAD_BLOCK_MIN 50

struct info {
    uint32_t size;   // Size of actual stored data in bytes
    uint32_t key[4]; // Encryption key
    uint64_t nonce;  // Nonce data
    void * data;     // Pointer to the encrypted stored data
    uint64_t *temp2; // Help increase performance during decrypt
};

struct node {
    uint16_t num_keys;
    uint32_t * keys;
};

struct b_tree {
    // Stores the helper data
    struct b_tree_node *root;
    int branching_factor;
    int n_processors;
    int node_count;
    struct b_tree_node *insertion_node;
    struct threadpool_t *threadpool;
    pthread_mutex_t mutex;
};


// Threadpool structs
struct task{
    void *(*function)(void *);
    void *args;
    struct task *next;
};

struct threadpool_t {
    pthread_mutex_t lock;
    struct task *head;
    struct task *tail;
    int queue_size;
    int n_threads;
    pthread_t *threads;
    pthread_cond_t cond;
    pthread_cond_t terminate;
    int threads_left;
    int stop;
};

struct encrypt_arg {
    uint64_t * cipher;
    uint64_t * temp2;
    uint32_t *key;
    uint64_t nonce;
    uint64_t * plain;
    pthread_mutex_t *lock;
    int *flag;
};
struct encrypt_arg_data {
    int start;
    int end;
    struct encrypt_arg *data;
};


struct b_tree_node {
    int key_count;
    int child_count;
    uint32_t *keys;
    struct info **values;
    struct b_tree_node **children;
    struct b_tree_node *parent;
};

struct threadpool_t* threadpool_create(int n_threads);
void threadpool_destroy(struct threadpool_t* threadpool);
void enqueue_task(void*(*function)(void*), void * args, struct threadpool_t* threadpool);
struct task *dequeue_task(struct threadpool_t *threadpool);
void* run_thread(void *args);

void * init_store(uint16_t branching, uint8_t n_processors);

void close_store(void * helper);
void post_order_free(struct b_tree_node* node);

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void * helper);
void split_node(struct b_tree_node *node, struct b_tree *tree);

int btree_retrieve(uint32_t key, struct info * found, void * helper);
// The actual search happens in this recursive function, used inside btree_retrieve
struct b_tree_node *btree_search(uint32_t key, struct info * found, struct b_tree_node* current_node, void *helper, int *flag);

int btree_decrypt(uint32_t key, void * output, void * helper);

int btree_delete(uint32_t key, void * helper);
void rebalance_node(struct b_tree_node* node, struct b_tree* tree);
struct b_tree_node *get_highest_key_node(struct b_tree_node *node);

void add_key(struct b_tree_node *target_node, uint32_t key, struct info *value);

uint64_t btree_export(void * helper, struct node ** list);
void pre_order_export(struct b_tree_node* node, int *index, struct node **list);
void free_btree_export(struct node **list, size_t size);

void print_btree(void *helper);
void pre_order_print(struct b_tree_node *node);

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]);

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]);

void encrypt_tea_ctr_thread(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks, struct b_tree* helper, uint64_t *temp2);
void decrypt_tea_ctr_thread(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks, struct b_tree *helper, uint64_t *temp2);
void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks);
void *encrypt_section(void *arg);
void *decrypt_section(void *arg);
void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks);

uint64_t TEA_encrypt(uint32_t plain[2], uint32_t key[4]);
#endif