#include "btreestore.h"

void * init_store(uint16_t branching, uint8_t n_processors) {
    //fprintf(stderr, "Branching: %d\n", branching);
    if (branching < 3){
        branching = 3;
    }
    if (n_processors < 1){
        n_processors = 1;
    }
    struct b_tree* helper = malloc(sizeof(struct b_tree));
    helper->branching_factor = branching;
    helper->n_processors = n_processors;
    helper->root = malloc(sizeof(struct b_tree_node));
    helper->root->child_count = 0;
    helper->root->children = malloc(sizeof(struct b_tree_node*) * (helper->branching_factor + 1));
    helper->root->key_count = 0;
    helper->root->keys = malloc(sizeof(uint32_t) * helper->branching_factor);
    helper->root->values = malloc(sizeof(struct info*) * (helper->branching_factor));
    helper->root->parent = NULL;
    helper->node_count = 1;
    pthread_mutex_init(&helper->mutex, NULL);
    helper->threadpool = threadpool_create(n_processors);
    return (void *) helper;
}

void close_store(void * helper) {
    struct b_tree* tree = helper;
    // Free everything in helper first
    post_order_free(tree->root);
    threadpool_destroy(tree->threadpool);
    pthread_mutex_destroy(&tree->mutex);
    free(helper); 
    return;
}

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void* helper) {
    // fprintf(stderr, "Insert: %d\n", key);
    struct b_tree *tree = helper;
    pthread_mutex_lock(&tree->mutex);
    int flag = 0;
    struct b_tree_node *node = btree_search(key, NULL, tree->root, helper, &flag);
    if (flag == 1){
        // Key exists
        pthread_mutex_unlock(&tree->mutex);
        return 1;
    }
   
    // First create the info struct which will be the value to the key
    struct info *value = malloc(sizeof(struct info));
    // Encrypt the message
    // Calculate number of 64 bit blocks the data can fit into
    int block_count = (int) ceil(count / 8.0);
    void* plaintext_padded = calloc(block_count, sizeof(uint64_t));
    void* cipher = malloc(block_count * sizeof(uint64_t));
    uint64_t *temp2 = malloc(block_count * sizeof(uint64_t));
    memmove(plaintext_padded, plaintext, count);
    encrypt_tea_ctr_thread(plaintext_padded, encryption_key, nonce, cipher, block_count, helper, temp2);
    // Now cipher contains the encrypted data
    free(plaintext_padded);
    
    // Fill in the info struct
    value->data = cipher;
    value->temp2 = temp2;
    value->nonce = nonce;
    value->size = count;
    memmove(value->key, encryption_key, sizeof(uint32_t) * 4);

    // We know have our key and value ready to be inserted
    // Insert the key and value into it's relevant position
    if (node->key_count == 0){
        node->keys[0] = key;
        node->values[0] = value;
        node->key_count++;
    } else {
        add_key(node, key, value);
    }

    if (node->key_count < tree->branching_factor){
        // Node successfully inserted
        pthread_mutex_unlock(&tree->mutex);
        return 0;
    } else {
        // Must split the node and recursively rebalance the tree
        while (1){
            tree->node_count++;
            int median_index;
            // Find the median node
            if (node->key_count % 2 == 0){
                median_index = node->key_count / 2 - 1;
            } else {
                median_index = node->key_count / 2;
            }
            
            uint32_t median_key = node->keys[median_index];
            struct info *median_value = node->values[median_index];

            // Existing node will be the left of median, the right node will be new
            struct b_tree_node *right = malloc(sizeof(struct b_tree_node));
            right->child_count = 0;
            if (node->child_count > 0){
                // It is a non-leaf node, so update it's child count
                node->child_count = median_index + 1;
                right->child_count = node->key_count - median_index;
            }
            right->parent = node->parent;
            right->key_count = node->key_count - median_index - 1;
            right->keys = malloc(sizeof(uint32_t) * tree->branching_factor);
            right->values = malloc(sizeof(struct info*) * tree->branching_factor);
            right->children = malloc(sizeof(struct b_tree_node*) * (tree->branching_factor + 1));
            node->key_count = median_index;
            for (int i = 0; i < right->key_count; i++){
                // Update right's keys
                right->keys[i] = node->keys[median_index + i + 1];
                right->values[i] = node->values[median_index + i + 1];
            }
            for (int i = 0; i < right->child_count; i++){
                right->children[i] = node->children[median_index + i + 1];
                // It's children's parent is now this node, not the original left part
                right->children[i]->parent = right;
            }
            // if this is the root node create a new node and insert median into it
            if (node->parent == NULL){
                struct b_tree_node *new_root = malloc(sizeof(struct b_tree_node));
                new_root->keys = malloc(sizeof(uint32_t) * tree->branching_factor);
                new_root->values = malloc(sizeof(struct info*) * tree->branching_factor);
                new_root->children = malloc(sizeof(struct b_tree_node*) * (tree->branching_factor + 1));
                node->parent = new_root;
                right->parent = new_root;

                new_root->child_count = 2;
                new_root->key_count = 1;
                new_root->parent = NULL;
                new_root->keys[0] = median_key;
                new_root->values[0] = median_value;
                new_root->children[0] = node;
                new_root->children[1] = right;
                tree->root = new_root;
                tree->node_count++;
                break;
            }
            // Now that the 2 nodes are created, insert median node into correct position in parent
            // move all the children after median's index in the parent to the right by 1
            // Insert right node to the right of median node
            if (median_key > node->parent->keys[node->parent->key_count - 1]){
                node->parent->keys[node->parent->key_count] = median_key;
                node->parent->values[node->parent->key_count] = median_value;
                node->parent->children[node->parent->key_count + 1] = right;
            } else {
                for (int i = 0; i < node->parent->key_count; i++){
                    if (median_key < node->parent->keys[i]){
                        memmove(&node->parent->keys[i + 1], &node->parent->keys[i], 
                                sizeof(uint32_t) * (node->parent->key_count - i));
                        memmove(&node->parent->values[i + 1], &node->parent->values[i],
                                sizeof(struct info*) * (node->parent->key_count - i));
                        memmove(&node->parent->children[i + 1], &node->parent->children[i], sizeof(struct b_tree_node*) * (node->parent->child_count - i));
                        node->parent->keys[i] = median_key;
                        node->parent->values[i] = median_value;
                        node->parent->children[i + 1] = right;
                        break;
                    }
                }
            }
            node->parent->child_count++;
            node->parent->key_count++;
            if (node->parent->key_count >= tree->branching_factor){
                node = node->parent;
                continue;
            }
            break;
        }
    }
    pthread_mutex_unlock(&tree->mutex);
    return 0;
}

int btree_retrieve(uint32_t key, struct info * found, void * helper) {
    struct b_tree* tree = (struct b_tree*) helper;

    // Tries to find the key and fills in the struct
    int flag = 0;
    btree_search(key, found, tree->root, helper, &flag);

    // Result will be 1 if no key found, 0 if key found and struct was filled in
    if (flag == 0){
        return 1;
    } else {
        return 0;
    }
}

struct b_tree_node* btree_search(uint32_t key, struct info * found, struct b_tree_node* current_node, void *helper, int *flag){
    while (1){
        for (int i = 0; i < current_node->key_count; i++){
            if (current_node->keys[i] == key){
                if (found != NULL){
                    // Found the key, so fill in the info
                    found->data = current_node->values[i]->data;
                    memmove(found->key, current_node->values[i]->key, 16);
                    found->nonce = current_node->values[i]->nonce;
                    found->size = current_node->values[i]->size;
                    found->temp2 = current_node->values[i]->temp2;
                }
                *flag = 1;
                return current_node;
            }
        }
        if (current_node->child_count == 0){
            // Set the insertion node, so it can be used if calling btree insert
            *flag = 0;
            return current_node;
        }
        // Find the two keys between which the key should exist and recur on the child
        if (key > current_node->keys[current_node->key_count - 1]){
            // Key is greater than all keys in current node
            current_node = current_node->children[current_node->key_count];
            continue;
        }
        for (int i = 0; i < current_node->key_count; i++){
            // Key is greater than a node
            if (key < current_node->keys[i]){
                current_node = current_node->children[i];
                break;
            }
        }

    }
}

int btree_decrypt(uint32_t key, void * output, void * helper) {
    //fprintf(stderr, "Btree Decrypt: %d\n", key);
    // Checking if it exists
    struct info *found = malloc(sizeof(struct info));
    if (btree_retrieve(key, found, helper) == 1){
        free(found);
        return 1;
    }
    int block_count = (int) ceil(found->size / 8.0);
    void *temp = malloc(sizeof(uint64_t) * block_count);
    decrypt_tea_ctr_thread(found->data, found->key, found->nonce, temp, block_count, helper, found->temp2);
    memmove(output, temp, found->size);
    free(found);
    free(temp);
    return 0;
}

int btree_delete(uint32_t key, void *helper) {
    //fprintf(stderr, "Delete: %d\n", key);
    struct b_tree *tree = helper;
    pthread_mutex_lock(&tree->mutex);
    int flag = 0;
    struct b_tree_node *node = btree_search(key, NULL, tree->root, helper, &flag);
    if (flag == 0){
        // No key found
        pthread_mutex_unlock(&tree->mutex);
        return 1;
    }
    int index = 0;
    for (; node->keys[index] != key; index++);

    // If key is in an internal node
    if (node->child_count > 0){
        // Find the highest key in the left child node subtree separated by key
        struct b_tree_node *highest_key_node = get_highest_key_node(node->children[index]);
        // Swap key with greatest key
        // First move greatest key value to temporary location
        highest_key_node->keys[highest_key_node->key_count] = highest_key_node->keys[highest_key_node->key_count - 1];
        highest_key_node->values[highest_key_node->key_count] = highest_key_node->values[highest_key_node->key_count - 1];

        // Copy key over now
        highest_key_node->keys[highest_key_node->key_count - 1] = key;
        highest_key_node->values[highest_key_node->key_count - 1] = node->values[index];

        // Copy over highest key to original key's location
        node->keys[index] = highest_key_node->keys[highest_key_node->key_count];
        node->values[index] = highest_key_node->values[highest_key_node->key_count];

        // Keys are now swapped, so delete original key now
        free(highest_key_node->values[highest_key_node->key_count - 1]->data);
        free(highest_key_node->values[highest_key_node->key_count - 1]->temp2);
        free(highest_key_node->values[highest_key_node->key_count - 1]);
        highest_key_node->key_count--;

        // Make node the leaf node
        node = highest_key_node;
    } else {
        free(node->values[index]->data);
        free(node->values[index]->temp2);
        free(node->values[index]);
        // Shift the keys on the right to the left to fill in the empty space now
        memmove(&node->keys[index], &node->keys[index + 1], (node->key_count - index - 1) * sizeof(uint32_t));
        memmove(&node->values[index], &node->values[index + 1], (node->key_count - index - 1) * sizeof(struct node*));
        node->key_count--;
    }

    // If leaf node contains enough keys then we are done
    if (node->key_count >= ceil(tree->branching_factor / 2.0) - 1){
        pthread_mutex_unlock(&tree->mutex);
        return 0; 
    }

    // We know now the node is unbalanced (contains less than the minimum numbers of keys)
    // Step 4
    // Look at the siblings of this leaf node and see if it has more than the minimum keys
    rebalance_node(node, tree);
    pthread_mutex_unlock(&tree->mutex);
    return 0;
}

void rebalance_node(struct b_tree_node* node, struct b_tree* tree){
    //fprintf(stderr, "Reblance: %d with %d children\n", node->keys[0], node->child_count);
    // If the node's parent is null, then the node is the root, if the root is empty then node is new root
    if (node->parent == NULL){
        // It is the root
        if (node->child_count > 0 && node->key_count == 0){
            // If the root has less than the minimum keys (less than 1), then it is empty and so promote its
            // Single child to become the new root
            tree->root = node->children[0];
            tree->root->parent = NULL;
            free(node->children);
            free(node->keys);
            free(node->values);
            free(node);
            tree->node_count--;
        } 
        return;
    }

    int node_index = 0;
    for (int i = 0; i < node->parent->child_count; i++){
        if (node->parent->children[i] == node){
            node_index = i;
            break;
        }
    }
    if (node_index > 0){
        // Node therefore has a left sibling
        struct b_tree_node *left_sibling = node->parent->children[node_index - 1];
        // Checking if left node has more than the minimum keys
        if (left_sibling->key_count > ceil(tree->branching_factor / 2.0) - 1){
            // move K_left into the target node
            add_key(node, node->parent->keys[node_index - 1], node->parent->values[node_index - 1]);
            // Move largest key in left sibling to K_left's original position
            node->parent->keys[node_index - 1] = left_sibling->keys[left_sibling->key_count - 1];
            node->parent->values[node_index - 1] = left_sibling->values[left_sibling->key_count - 1];
            // If the left sibling has a child, move that over as well
            if (left_sibling->child_count > 0){
                memmove(&node->children[1],&node->children[0], node->child_count * sizeof(struct b_tree_node*));
                node->children[0] = left_sibling->children[left_sibling->child_count - 1];
                // Update child counts
                left_sibling->child_count--;
                node->child_count++;
            }
            left_sibling->key_count--;
            return;
        }  
    }
    if (node_index < node->parent->child_count - 1){
        // left sibling did not work, so try right now
        struct b_tree_node *right_sibling = node->parent->children[node_index + 1];
        if (right_sibling->key_count > ceil(tree->branching_factor / 2.0) - 1){
            // Move K_right to node
            add_key(node, node->parent->keys[node_index], node->parent->values[node_index]);
            // Move the smallest key in right sibling to K_right's original position
            node->parent->keys[node_index] = right_sibling->keys[0];
            node->parent->values[node_index] = right_sibling->values[0];

            // Shift everything in right sibling to the left by 1
            memmove(&right_sibling->keys[0], &right_sibling->keys[1], (right_sibling->key_count - 1) * sizeof(uint32_t));
            memmove(&right_sibling->values[0], &right_sibling->values[1], (right_sibling->key_count - 1) * sizeof(struct info*));
            
            // If the right sibling has a child, move that over as well
            if (right_sibling->child_count > 0){
                node->children[node->child_count] = right_sibling->children[0];
                memmove(&right_sibling->children[0],&right_sibling->children[1], right_sibling->child_count * sizeof(struct b_tree_node*));
                // Update child counts
                right_sibling->child_count--;
                node->child_count++;
            }
            right_sibling->key_count--;
            return;
        }
    }

    // Suitable sibling was not found, continue to step 6

    // Merge node with it's left sibling if it has one
    if (node_index > 0){
        // It has a left sibling
        struct b_tree_node *left_sibling = node->parent->children[node_index - 1];
        // Shift everything in node to the right to make room for left sibling and parent key
        memmove(&node->keys[1 + left_sibling->key_count], &node->keys[0], sizeof(uint32_t) * node->key_count);
        memmove(&node->values[ 1 + left_sibling->key_count], &node->values[0], sizeof(struct info*) * node->key_count);
        if (node->child_count > 0){
            memmove(&node->children[1 + left_sibling->key_count], &node->children[0], sizeof(struct b_tree_node*) * node->child_count);
        }
        // Move parent key and value
        node->keys[left_sibling->key_count] = node->parent->keys[node_index - 1];
        node->values[left_sibling->key_count] = node->parent->values[node_index - 1];

        // Move everything in left sibling to right sibling
        memmove(&node->keys[0], &left_sibling->keys[0], sizeof(uint32_t) * left_sibling->key_count);
        memmove(&node->values[0], &left_sibling->values[0], sizeof(struct info*) * left_sibling->key_count);
        if (left_sibling->child_count > 0){
            memmove(&node->children[0], &left_sibling->children[0], sizeof(struct b_tree_node*) * left_sibling->child_count);
        }

        // Now in parent node we can shift everything to the left by 1
        memmove(&node->parent->keys[node_index - 1], &node->parent->keys[node_index], sizeof(uint32_t) * (node->parent->key_count - node_index));
        memmove(&node->parent->values[node_index - 1], &node->parent->values[node_index], sizeof(struct info*) * (node->parent->key_count - node_index));
        memmove(&node->parent->children[node_index - 1], &node->parent->children[node_index], sizeof(struct b_tree_node*) * (node->parent->child_count - node_index));

        // Update parameters
        node->key_count += left_sibling->key_count + 1;
        node->child_count += left_sibling->child_count;
        node->parent->key_count--;
        node->parent->child_count--;

        // Update parent
        for (int i = 0; i < node->child_count; i++){
            node->children[i]->parent = node;
        }

        // Delete sibling
        free(left_sibling->keys);
        free(left_sibling->values);
        free(left_sibling->children);
        free(left_sibling);
        tree->node_count--;
    } else {
        // It has no left sibling so do the right sibling
        struct b_tree_node *right_sibling = node->parent->children[node_index + 1];

        // Move parent key and value
        node->keys[node->key_count] = node->parent->keys[node_index];
        node->values[node->key_count] = node->parent->values[node_index];

        // Move everything in right sibling to left sibling
        memmove(&node->keys[node->key_count + 1], &right_sibling->keys[0], sizeof(uint32_t) * right_sibling->key_count);
        memmove(&node->values[node->key_count + 1], &right_sibling->values[0], sizeof(struct info*) * right_sibling->key_count);
        if (right_sibling->child_count > 0){
            memmove(&node->children[node->child_count], &right_sibling->children[0], sizeof(struct b_tree_node*) * right_sibling->child_count);
        }

        // Now in parent node we can shift everything to the left
        memmove(&node->parent->keys[node_index], &node->parent->keys[node_index + 1], sizeof(uint32_t) * (node->parent->key_count - node_index - 1));
        memmove(&node->parent->values[node_index], &node->parent->values[node_index + 1], sizeof(struct info*) * (node->parent->key_count - node_index - 1));
        memmove(&node->parent->children[node_index + 1], &node->parent->children[node_index + 2], sizeof(struct b_tree_node*) * (node->parent->child_count - node_index - 1));  

        // Update parameters
        node->key_count += right_sibling->key_count + 1;
        node->child_count += right_sibling->child_count;
        node->parent->key_count--;
        node->parent->child_count--;

        // Update parent
        for (int i = 0; i < node->child_count; i++){
            node->children[i]->parent = node;
        }

        // Delete sibling
        free(right_sibling->keys);
        free(right_sibling->values);
        free(right_sibling->children);
        free(right_sibling);
        tree->node_count--;
    }
    // Recursive call on the parent if it now has less than the minimum required keys
    if (node->parent->key_count < ceil(tree->branching_factor / 2.0) - 1){
        rebalance_node(node->parent, tree);
    }
}

void add_key(struct b_tree_node *target_node, uint32_t key, struct info *value){
    if (target_node->key_count == 0){
        target_node->keys[0] = key;
        target_node->values[0] = value;
    } else if (key > target_node->keys[target_node->key_count - 1]){
        target_node->keys[target_node->key_count] = key;
        target_node->values[target_node->key_count] = value;
    } else {
        for (int i = 0; i < target_node->key_count; i++){
            if (key < target_node->keys[i]){
                // Insert the key and it's value into here
                // Shift everything to the right by 1
                memmove(&target_node->keys[i + 1], &target_node->keys[i], 
                        sizeof(uint32_t) * (target_node->key_count - i));
                memmove(&target_node->values[i + 1], &target_node->values[i],
                        sizeof(struct info*) * (target_node->key_count - i));
                target_node->keys[i] = key;
                target_node->values[i] = value;
                break;
            }
        }
    }
    target_node->key_count++;
}

struct b_tree_node *get_highest_key_node(struct b_tree_node *node){
    if (node->child_count != 0){
        return get_highest_key_node(node->children[node->child_count - 1]);
    } else {
        return node;
    }
}

uint64_t btree_export(void * helper, struct node ** list) {
    struct b_tree *tree = helper;
    *list = malloc(sizeof(struct node) * tree->node_count);
    int index = 0;
    pre_order_export(tree->root, &index, list);
    return index;
}

void pre_order_export(struct b_tree_node* node, int *index, struct node **list){
    (*list + *index)->num_keys = node->key_count;
    (*list + *index)->keys = malloc(sizeof(uint32_t) * node->key_count);

    for (int i = 0; i < node->key_count; i++){
        (*list + *index)->keys[i] = node->keys[i];
    }
    (*index)++;
    for (int i = 0; i < node->child_count; i++){
        pre_order_export(node->children[i], index, list);
    }
}

void print_btree(void *helper){
    struct b_tree *tree = helper;
    pre_order_print(tree->root);
}

void pre_order_print(struct b_tree_node *node){
    for (int i = 0; i < node->key_count; i++){
        if (i + 1 == node->key_count){
            printf("%d", node->keys[i]);
        } else {
            printf("%d,", node->keys[i]);
        }
    }
    if (node->parent != NULL){
        printf(" [%d]", node->parent->keys[0]);
    } else {
        printf(" [root]");
    }
    printf("\n");
    for (int i = 0; i < node->child_count; i++){
        pre_order_print(node->children[i]);
    }
}

void free_btree_export(struct node **list, size_t size){
    for (int i = 0; i < size; i++){
        free((*list + i)->keys);
    }
    free(*list);
}

void encrypt_tea_ctr_thread(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks, struct b_tree* helper, uint64_t *temp2) {
    if (num_blocks > THREAD_BLOCK_MIN){
        struct threadpool_t *threadpool = helper->threadpool;
        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        int flag = 0;
        struct encrypt_arg data;
        data.lock = &lock;
        data.flag = &flag;
        data.nonce = nonce;
        data.cipher = cipher;
        data.temp2 = temp2;
        data.plain = plain;
        data.key = key;
        int work_size = num_blocks / threadpool->n_threads;

        for (int i = 0; i < threadpool->n_threads; i++){
            pthread_mutex_lock(data.lock);
            // Unlock in the work function
            flag++;
            struct encrypt_arg_data *thread_data = malloc(sizeof(struct encrypt_arg_data));
            thread_data->start = i * work_size;
            thread_data->data = &data;
            if (i + 1 == threadpool->n_threads){
                thread_data->end = num_blocks;
            } else {
                thread_data->end = (i + 1) * work_size;
            }
            enqueue_task(encrypt_section, thread_data, threadpool);
            pthread_mutex_unlock(data.lock);
        }

        while (flag != 0){
            usleep(2000);
        }
    } else {
        uint64_t sum = 0;
        uint32_t temp_cipher[2];
        for (int i = 0; i < num_blocks; i++){
            temp_cipher[0] = i ^ nonce;
            temp_cipher[1] = (i ^ nonce) >> 32;
            sum = 0;
            for (int i = 0; i < 1024; i++){
                sum += DELTA;
                temp_cipher[0] += (((temp_cipher[1] << 4 ) + key[0]) ^ (temp_cipher[1] + sum) ^ ((temp_cipher[1] >> 5) + key[1]));
                temp_cipher[1] += (((temp_cipher[0] << 4) + key[2]) ^ (temp_cipher[0] + sum) ^ ((temp_cipher[0] >> 5) + key[3]));
            }
            cipher[i] = plain[i] ^ (*(uint64_t*)temp_cipher);
            temp2[i] = *(uint64_t*)temp_cipher;
        }
    }   
}

void decrypt_tea_ctr_thread(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks, struct b_tree *helper, uint64_t *temp2) {
    if (num_blocks > THREAD_BLOCK_MIN){
        struct threadpool_t *threadpool = helper->threadpool;
        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        int flag = 0;
        struct encrypt_arg data;
        data.lock = &lock;
        data.flag = &flag;
        data.nonce = nonce;
        data.cipher = cipher;
        data.temp2 = temp2;
        data.plain = plain;
        data.key = key;
        int work_size = num_blocks / threadpool->n_threads;

        for (int i = 0; i < threadpool->n_threads; i++){
            pthread_mutex_lock(data.lock);
            flag++;
            struct encrypt_arg_data *thread_data = malloc(sizeof(struct encrypt_arg_data));
            thread_data->start = i * work_size;
            thread_data->data = &data;
            if (i + 1 == threadpool->n_threads){
                thread_data->end = num_blocks;
            } else {
                thread_data->end = (i + 1) * work_size;
            }
            enqueue_task(decrypt_section, thread_data, threadpool);
            pthread_mutex_unlock(data.lock);
        }

        while (flag != 0){
            usleep(2000);
        }
    } else {
        uint64_t sum = 0;
        uint32_t temp_cipher[2];
        for (int i = 0; i < num_blocks; i++){
            plain[i] = cipher[i] ^ temp2[i];
        }
    }
    
}

void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks) {
    uint64_t temp1;
    uint64_t temp2;
    for (int i = 0; i < num_blocks; i++){
        temp1 = i ^ nonce;
        temp2 = TEA_encrypt((uint32_t*) &temp1, key);
        cipher[i] = plain[i] ^ temp2;
    } 
}

void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks) {
    uint64_t temp1;
    uint64_t temp2;    
    for (int i = 0; i < num_blocks; i++){
        temp1 = i ^ nonce;
        temp2 = TEA_encrypt((uint32_t*) &temp1, key);
        plain[i] = cipher[i] ^ temp2;
    }
}

void *encrypt_section(void *arg){
    struct encrypt_arg_data *data = arg;
    uint64_t *master_plain = data->data->plain;
    uint64_t *master_cipher = data->data->cipher;
    uint64_t *temp2 = data->data->temp2;
    uint32_t *key = data->data->key;
    uint64_t nonce = data->data->nonce;
    int start = data->start;
    int end = data->end;

    uint64_t sum = 0;
    uint32_t cipher[2];

    for (int i = start; i < end; i++){
        cipher[0] = i ^ nonce;
        cipher[1] = (i ^ nonce) >> 32;
        sum = 0;
        for (int i = 0; i < 1024; i++){
            sum += DELTA;
            cipher[0] += (((cipher[1] << 4 ) + key[0]) ^ (cipher[1] + sum) ^ ((cipher[1] >> 5) + key[1]));
            cipher[1] += (((cipher[0] << 4) + key[2]) ^ (cipher[0] + sum) ^ ((cipher[0] >> 5) + key[3]));
        }
        master_cipher[i] = master_plain[i] ^ (*(uint64_t*)cipher);
        temp2[i] = *(uint64_t*)cipher;
    }
    pthread_mutex_lock(data->data->lock);
    (*data->data->flag)--;
    pthread_mutex_unlock(data->data->lock);
    free(data);
}

void *decrypt_section(void *arg){
    struct encrypt_arg_data *data = arg;
    uint64_t *master_plain = data->data->plain;
    uint64_t *master_cipher = data->data->cipher;
    uint64_t *temp2 = data->data->temp2;
    uint32_t *key = data->data->key;
    uint64_t nonce = data->data->nonce;
    int start = data->start;
    int end = data->end;
    uint64_t sum = 0;
    uint32_t cipher[2];
    for (int i = start; i < end; i++){
        master_plain[i] = master_cipher[i] ^ temp2[i];
    }
    pthread_mutex_lock(data->data->lock);
    (*data->data->flag)--;
    pthread_mutex_unlock(data->data->lock);
    free(data);
}

uint64_t TEA_encrypt(uint32_t plain[2], uint32_t key[4]){
    uint64_t sum = 0;
    uint32_t cipher[2];
    cipher[0] = plain[0];
    cipher[1] = plain[1];
    for (int i = 0; i < 1024; i++){
        sum += DELTA;
        cipher[0] += (((cipher[1] << 4 ) + key[0]) ^ (cipher[1] + sum) ^ ((cipher[1] >> 5) + key[1]));
        cipher[1] += (((cipher[0] << 4) + key[2]) ^ (cipher[0] + sum) ^ ((cipher[0] >> 5) + key[3]));
    }
    return *((uint64_t*) cipher);
}

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]) {
    uint64_t sum = 0;
    unsigned int delta = 0x9E3779B9; 
    cipher[0] = plain[0];
    cipher[1] = plain[1];

    uint32_t temp1;
    uint32_t temp2;
    uint32_t temp3;
    uint32_t temp4;
    uint32_t temp5;
    uint32_t temp6;
    for (int i = 0; i < 1024; i++){
        sum = (sum + delta);
        temp1 = ((cipher[1] << 4 ) + key[0]);
        temp2 = (cipher[1] + sum);
        temp3 = ((cipher[1] >> 5) + key[1]);
        cipher[0] = (cipher[0] + (temp1 ^ temp2 ^ temp3));
        temp4 = ((cipher[0] << 4) + key[2]);
        temp5 = (cipher[0] + sum);
        temp6 = ((cipher[0] >> 5) + key[3]);
        cipher[1] = (cipher[1] + (temp4 ^ temp5 ^ temp6));
    }
    return;
}


void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]) {
    uint64_t sum = 0xDDE6E400;
    unsigned int delta = 0x9E3779B9;

    uint32_t temp1;
    uint32_t temp2;
    uint32_t temp3;
    uint32_t temp4;
    uint32_t temp5;
    uint32_t temp6;

    for (int i = 0; i < 1024; i++){
        temp4 = ((cipher[0] << 4) + key[2]);
        temp5 = (cipher[0] + sum);
        temp6 = ((cipher[0] >> 5) + key[3]);
        cipher[1] = (cipher[1] - (temp4 ^ temp5 ^ temp6));
        temp1 = ((cipher[1] << 4) + key[0]);
        temp2 = (cipher[1] + sum);
        temp3 = ((cipher[1] >> 5) + key[1]);
        cipher[0] = (cipher[0] - (temp1 ^ temp2 ^ temp3));
        sum = (sum - delta);
    }
    plain[0] = cipher[0];
    plain[1] = cipher[1];
    return;
}


void post_order_free(struct b_tree_node* node){
    // Call recursive on children first if they exist
    for (int i = 0; i < node->child_count; i++){
        post_order_free(node->children[i]);
    }
    free(node->children);
    for (int i = 0; i < node->key_count; i++){
        free(node->values[i]->data);
        free(node->values[i]->temp2);
        free(node->values[i]);
    }
    free(node->keys);
    free(node->values);
    free(node);
}

struct threadpool_t* threadpool_create(int n_threads){
    struct threadpool_t *pool = malloc(sizeof(struct threadpool_t));
    pool->n_threads = n_threads;
    pool->queue_size = 0;
    pool->threads = malloc(sizeof(pthread_t) * n_threads);
    pool->stop = 0;
    pthread_cond_init(&pool->cond, NULL);
    pthread_cond_init(&pool->terminate, NULL);
    pthread_mutex_init(&pool->lock, NULL);
    pool->threads_left = n_threads;
    for (int i = 0; i < n_threads; i++){
        pthread_create(&pool->threads[i], NULL, run_thread, pool);
    }
    return pool;
}

void enqueue_task(void*(*function)(void*), void * args, struct threadpool_t* threadpool){
    struct task *new_task = malloc(sizeof(struct task));
    new_task->function = function;
    new_task->args = args;
    new_task->next = NULL;
    pthread_mutex_lock(&threadpool->lock);
    // Push new task
    if (threadpool->queue_size == 0){
        threadpool->head = new_task;
        threadpool->tail = new_task;
    } else {
        threadpool->tail->next = new_task;
        threadpool->tail = new_task;
    }
    threadpool->queue_size++;
    pthread_mutex_unlock(&threadpool->lock);
    pthread_cond_signal(&threadpool->cond);
}

struct task *dequeue_task(struct threadpool_t *threadpool){
    struct task *task;
    if (threadpool->queue_size > 0){
        // There is a task available
        task = threadpool->head;
        threadpool->head = task->next;
        threadpool->queue_size--;
    } else {
        task = NULL;
    }
    return task;
}

void* run_thread(void *args){
    struct threadpool_t* threadpool = args;
    while (1){
        
        struct task *task;

        pthread_mutex_lock(&threadpool->lock);
        while (threadpool->queue_size == 0 && !threadpool->stop){
            pthread_cond_wait(&threadpool->cond, &threadpool->lock);
        }

        if (threadpool->stop){
                threadpool->threads_left--;
                pthread_mutex_unlock(&threadpool->lock);
                pthread_exit(NULL);
        }

        // Try get a task from the queue
        task = dequeue_task(threadpool);
        pthread_mutex_unlock(&threadpool->lock);
        if (task != NULL){
            task->function(task->args);
            free(task);
        }
    }
}

void threadpool_destroy(struct threadpool_t* threadpool){
    while (threadpool->queue_size > 0){
        usleep(1000);
    }
    threadpool->stop = 1;
    while (threadpool->threads_left > 0){
        pthread_cond_broadcast(&threadpool->cond);
        usleep(1000);
    }
    
    for (int i = 0; i < threadpool->n_threads; i++){
        pthread_join(threadpool->threads[i], NULL);
    }

    pthread_mutex_destroy(&threadpool->lock);
    pthread_cond_destroy(&threadpool->terminate);
    pthread_cond_destroy(&threadpool->cond);
    free(threadpool->threads);
    free(threadpool);
}