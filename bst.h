/*
 * Copyright (c) 2018 Polycom Inc
 *
 * Distortion Player for Utopia Debug Window
 *
 * Author: SONGYI (yi.song@polycom.com)
 * Date Created: 20180830
 */

#ifndef _BINARY_SEARCH_TREE_
#define _BINARY_SEARCH_TREE_

#include <stdio.h>
#include <math.h>
#include <iostream>

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#define EPSILON 1e-3
#define EQUAL(a, b) (fabs((a)-(b)) < EPSILON)

class BinarySearchTree
{
public:
    struct tnode
    {
        float key;
        float value;
        struct tnode *left;
        struct tnode *right;
        struct tnode *parent;
    };

    typedef struct tnode *btree;

public:
    BinarySearchTree()
    {
        root = NULL;
    }

    virtual ~BinarySearchTree()
    {
        destroy_tree(root);
    }

    void Insert(float key, float value)
    {
        struct tnode *p = alloc_tree_node();
        if(NULL != p)
        {
            p->key = key;
            p->value = value;
            insert_node(root, p);
        }
        else
            printf("error: out of space! %s\n", __FUNCTION__);
    }

    void SmartInsert(float Key[], float Val[], int N)
    {
        if(Key != NULL && Val != NULL && N > 0)
        {
            if(root == NULL)
            {
                int mid = N /2;
                Insert(Key[mid], Val[mid]);
                if(mid -1 >= 0)
                    insert_middle(Key, Val, 0, mid-1); // [0, mid-1]
                if(mid + 1 <= N -1)
                    insert_middle(Key, Val, mid+1, N-1); // [mid+1, N-1]
            }
            else
            {
                insert_middle(Key, Val, 0, N-1);
            }
        }
    }

    void Search(float key, float &sin_key, float &sin_val, float &dex_key, float &dex_val)
    {
        struct tnode *node = search_node(root, key);
        struct tnode *sibling;

        if(node == NULL)
        {
            printf("fatal error: Ahhhhhhhhhhhhhhhh~\n");
            return;
        }

        if(key < node->key)
        {
            sibling = predecessor(node);
            if(sibling == NULL)
                sibling = successor(node);

            sin_key = sibling->key;
            sin_val = sibling->value;
            dex_key = node->key;
            dex_val = node->value;

        }
        else
        {
            sibling = successor(node);
            if(sibling == NULL)
                sibling = predecessor(node);

            sin_key = node->key;
            sin_val = node->value;
            dex_key = sibling->key;
            dex_val = sibling->value;
        }
    }

    void Draw()
    {
        std::vector<std::vector<float> > layers;
        const int DEPTH = Depth();
        layers.resize(DEPTH, std::vector<float>());
        preorder_walk(root, layers, 0);

        int i, j, count=0;
        for(i = 0; i < layers.size(); i++)
        {
            std::cout << i+1 << "L:        ";
            for(j = 0; j< layers[i].size(); j++)
            {
                std::cout << layers[i][j] << ", ";
            }
            std::cout << std::endl << std::endl;
            count += j;
        }

        printf("Total leaf nodes: %d\n\n", count);
    }

    int Depth()
    {
        return cal_depth(root);
    }

private:
    struct tnode *alloc_tree_node()
    {
        struct tnode *p = new struct tnode;
        if(p == NULL)
            printf("error: out of space in %s\n", __FUNCTION__);
        memset(p, 0, sizeof(struct tnode));
        return p;
    }

    void free_tree_node(struct tnode *&p)
    {
        if(p != NULL)
        {
            delete p;
            p = NULL;
        }
    }

    void destroy_tree(btree t)  // LRN Post-Order
    {
        if(t != NULL)
        {
            if(t->left != NULL) // to reduce recursion
                destroy_tree(t->left);
            if(t->right != NULL)
                destroy_tree(t->right);
            free_tree_node(t);
        }
    }

    void insert_node(btree &t, struct tnode *node)
    {
        struct tnode *pos = NULL;
        struct tnode *p = t;

        while(p != NULL)
        {
            pos = p; // pos will be a leaf node when p traverse to NIL
            if(node->key < p->key)
                p = p->left;
            else
                p = p->right;
        }

        node->parent = pos;

        if(pos == NULL) // empty tree
        {
            t = node;
            printf("info: create binary search tree: %f, %f\n", node->key, node->value);
        }
        else
        {
            if(node->key < pos->key)
                pos->left = node;
            else
                pos->right = node;
        }
    }

    struct tnode *search_node(btree t, float x)
    {
#ifdef RECURSIVE_VERSION
        if(t == NULL || is_the_closest(t, x))
            return t;
        if(x < t->key)
            return search_node(t->left, x);
        else
            return search_node(t->right, x);
#else // ITERATIVE_VERSION is faster
        while(t != NULL && !(is_the_closest(t, x)))
        {
            if(x < t->key)
                t = t->left;
            else
                t = t->right;
        }
        return t;
#endif
    }

    struct tnode *successor(struct tnode *node)
    {
        if(node == NULL)
            return node;

        if(node->right != NULL)
            return minimum(node->right);

        // case 2: has no right subtree
        // to search a parent that bigger (its left child is previous parent)
        struct tnode *p = node->parent;
        while(p != NULL && p->right == node)
        {
            node = p;
            p = p->parent;
        }
        return p;
    }

    struct tnode *predecessor(struct tnode *node)
    {
        if(node == NULL)
            return node;

        if(node->left != NULL)
            return maximum(node->left);

        struct tnode *p = node->parent;
        while(p != NULL && p->left == node)
        {
            node = p;
            p = p->parent;
        }
        return p;
    }

    struct tnode *minimum(btree t)
    {
        if(t == NULL)
            return t;
        while(t->left != NULL)
            t = t->left;
        return t;
    }

    struct tnode *maximum(btree t)
    {
        if(t == NULL)
            return t;
        while(t->right != NULL)
            t = t->right;
        return t;
    }

    int cal_depth(btree t)
    {
        int depth = 0, h1 = 0, h2 = 0;

        if(t == NULL)
            return depth;

        h1 = cal_depth(t->left);
        h2 = cal_depth(t->right);
        depth = max(h1, h2) + 1;
        return depth;
    }

    bool is_the_closest(struct tnode *t, float x)
    {
        // there are bugs in this function
        // but writing bst class is aim to prove whether std::map is a tree
        // as a result std::map is truely a tree and about 6,7 milliseconds faster
        float current_diff = fabs(t->key - x);
        float nextdiff = 0.0F;

        if(x < t->key)
        {
            if(t->left != NULL)
                nextdiff = fabs(t->left->key - x);
            else
                return true;
        }
        else
        {
            if(t->right != NULL)
                nextdiff = fabs(t->right->key - x);
            else
                return true;
        }

        if(current_diff < nextdiff)
            return true;
        else
            return false;
    }

    void preorder_walk(btree t, std::vector<std::vector<float> >& layers, int i)
    {
        if(t != NULL)
        {
            layers[i].push_back(t->key);
            preorder_walk(t->left, layers, i+1);
            preorder_walk(t->right, layers, i+1);
        }
    }

    void insert_middle(float Key[], float Val[], int i, int j)
    {
        if(i > j)
        {
            printf("error: impossible!%s\n", __FUNCTION__);
            return;
        }

        if(i == j)
        {
            Insert(Key[i], Val[i]);
        }
        else if(i + 1 == j)
        {
            Insert(Key[i], Val[i]);
            Insert(Key[j], Val[j]);
        }
        else
        {
            int mid = (i + j) / 2;
            Insert(Key[mid], Val[mid]);
            if(mid - 1 >= i)
                insert_middle(Key, Val, i, mid-1);
            if(mid + 1 <= j)
                insert_middle(Key, Val, mid+1, j);
        }
    }

private:
    struct tnode *root;
};

#endif
