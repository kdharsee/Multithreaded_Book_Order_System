#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "uthash.h"

/* 
 * Object defining a Book Order Node. Attributes include 
 * book title, book price, remaining credit, id of the purchaser,
 * book category, and a pointer to the next Book Order node. 
 */

typedef struct orderNode {
  char *title;
  double price;
  double remaining;
  int uid;
  char *category;
  struct orderNode *next;
} orderNode;

/*
 * Object defining a Consumer Node for a linked list comprising of Consumer Nodes.
 * Attributes include customer name, customer id, the customer's credit balance, 
 * shipping information (address, state, zip), the head for a list representing
 * the customer's successful orders, the head for a list representing the 
 * customer's unsuccessful orders, and a hash table handler.
 * 
 */
typedef struct consumerNode {
  char *name;
  int id;
  double balance;
  char *street_addr;
  char *state;
  char *zip;
  orderNode *successful_head;
  orderNode *unsuccessful_head;
  UT_hash_handle hh;
} consumerNode;

/*
 * Struct defining a queue for the Order Node Linked List. 
 * It is comprised of pointers to its head and tail,
 * a unique category name, mutex, thread
 * a flag indicating that the thread is done,
 * a flag indicating that if the thread has been run yet,
 * and a hash table handler.
 * 
*/

typedef struct orderQueue {
  orderNode *head;
  orderNode *tail;
  char *category;
  pthread_mutex_t q_mutex;
  pthread_t thread; 
  int thread_complete;
  int first;
  UT_hash_handle hh;
} orderQueue;

/*
 * Prints the command-line arguments which are 
 * the order list, the customer database and categories.
*/
void printArgs( int argc, char **argv );

/*
 * Prints the proper format for the command-line arguments
*/
void printUsage();

/*
 * Prints the content of the hash table
*/
void printHash();

/*
 * Prints the linked list of book orders
*/

void printOrders( orderNode *head );

/*
 * Prints all customer nodes within the hash table
*/

void printConsumerHash();

/*
 * Prints contents of the queue, given a particular queue
*/

void printQueue( orderQueue *q );

/*
 * Print the queue hash table to the terminal.
*/

void printQueueHash();

/*
 * Creates a queue given a category
*/

orderQueue *createQueue( char *category );

/*
 * Inserts a Book Order node into a Book Order queueu 
*/

void enqueu( orderNode *node, orderQueue *queue );

/*
 * Removes a Book Order node into a Book Order queueu
*/

orderNode *dequeue( orderQueue *queue );

/*
 * Loads the category queues given the categories
*/

void loadCategoryQueues( char *categories );

/*
 * Creates Customer Nodes, given a file containing customer information
*/

void loadConsumers( FILE *fp );

/*
 * Adds Book Order node to file, and returns 1 if success, 0 otherwise.
*/

void ordersToFile( orderNode *head, FILE *fp, int success );

/*
 * Adds Consumer Node information to the outfile.
*/

void consumerInfoToFile( char *outfile );

/*
 * Consumer thread function that processes Book Order Nodes.
*/

void processOrder( orderNode *order );

/*
 * Function used by consumer thread to process Book Order Nodes,
 * given a queue.
*/

void *consumer_thread_function( void *order_q );

/*
 * Function used by producer thread to allocate Book Order Nodes to
 * appropriate queue.
*/

void *producer_thread_function( void *arg );
