#include "bookorder.h"
#define BUFFER_SIZE 1024

consumerNode *consumer_table = NULL;
orderQueue *category_queue_table = NULL;
pthread_mutex_t work_mutex;
void *thread_function( void *arg );


void printArgs( int argc, char **argv ) {
  int i;
  for ( i = 0; i < argc; i++ ) {
    printf( "Arg%d: %s\n", i, *(argv + i) );
  }
}

void printUsage() { //INCOMPLETE
  printf( "Usage: bookorder <consumerdb file> <orders file> <catagory1[ catagory2 [catagory3...]>\n" );
}

void printOrders( orderNode *head ) {
  orderNode *ptr;
  for ( ptr = head; ptr; ptr = ptr->next ) {
    printf( "\t%s|%f|%d|%s\n", ptr->title, ptr->price, ptr->uid, ptr->category );
  }
}


void printConsumerHash() { 
  consumerNode *ptr, *tmp;
  
  HASH_ITER( hh, consumer_table, ptr, tmp ) {
    printf( "%s|%d|%f|%s|%s|%s\n", ptr->name, ptr->id, ptr->balance, ptr->street_addr, ptr->state, ptr->zip );
    printf( "\tSUCCESSFUL\n" );
    printOrders( ptr->successful_head );
    printf( "\tUNSUCCESSFUL\n" );
    printOrders( ptr->unsuccessful_head );
  }
}

void printQueue( orderQueue *q ) {
  orderNode *ptr;
  for ( ptr = q->head; ptr; ptr = ptr->next ) {
    printf( "\t%s|%f|%d|%s\n", ptr->title, ptr->price, ptr->uid, ptr->category );
  }
}

void printQueueHash() {
  orderQueue *ptr, *tmp;
  printf( "QUEUE HASH:\n" );
  HASH_ITER( hh, category_queue_table, ptr, tmp ) {
    printf( "###Queue Category:%s\n", ptr->category );
    printQueue( ptr );
  }
}


orderQueue *createQueue( char *category ) {
  orderQueue *new_q = (orderQueue *)malloc( sizeof( orderQueue ) );
  new_q->head = NULL;
  new_q->tail = NULL; /* initizlize the queue's list to null */
  new_q->category = (char *)malloc( sizeof( char )*(strlen( category ) + 1) );
  strcpy( new_q->category, category );
  pthread_mutex_init( &(new_q->q_mutex), NULL ); /* Initialize the mutex for later use */
  new_q->thread_complete = 1; 
  new_q->first = 1;
  return new_q;
}

void enqueue( orderNode *node, orderQueue *queue ) {
  if ( queue->tail == NULL ) { 
    queue->tail = node;
    queue->head = node;
  }
  else {
    queue->tail->next = node;
    queue->tail = queue->tail->next;
  }
}

orderNode *dequeue( orderQueue *queue ) {
  orderNode *ptr;

  if ( queue->head == NULL ) {
    return NULL;
  }

  if ( queue->head == queue->tail ) {
    ptr = queue->head;
    queue->head = NULL;
    queue->tail = NULL;
  }
  else {
    ptr = queue->head;
    queue->head = queue->head->next;
  }

  ptr->next = NULL; /* set next to null to completely detach */

  return ptr;

}

void loadCategoryQueues( char *categories ) {
  char *category = NULL;
  orderQueue *new_q = NULL;

  category = strtok( categories, " " );
  while ( category != NULL ) {
    new_q = createQueue( category );

    HASH_ADD_KEYPTR( hh, category_queue_table, new_q->category, strlen( new_q->category ), new_q ); /* Add the queue to the hashtable */

    category = strtok( NULL, " \n" );
  }
}

void loadConsumers( FILE *fp ) {
  char buf[BUFFER_SIZE];
  char *token;
  consumerNode *curr;
  while ( fgets( buf, BUFFER_SIZE, fp ) != NULL ) { /* each loop will extract a line from the file. One consumer entry */

    curr = (consumerNode *)malloc( sizeof( consumerNode ) );
    
    token = strtok( buf, "|" );
    curr->name = (char *)malloc( sizeof( char ) * strlen( token ) + 1 );
    strcpy( curr->name, token );

    token = strtok( NULL, "|" );
    curr->id = atoi( token + 1 ); /* + 1 to get rid of the space at the beginning of the word */
    
    token = strtok( NULL, "|" );
    curr->balance = atof( token + 1 ); 
    
    token = strtok( NULL, "|" );
    curr->street_addr = (char *)malloc( sizeof( char ) * strlen( token ) + 1 );
    strcpy( curr->street_addr, token + 1);

    token = strtok( NULL, "|" );
    curr->state = (char *)malloc( sizeof( char ) * strlen( token ) + 1 );
    strcpy( curr->state, token + 1);

    token = strtok( NULL, "|\n" );
    curr->zip = (char *)malloc( sizeof( char ) * strlen( token ) + 1 );
    strcpy( curr->zip, token + 1);

    curr->successful_head = NULL;
    curr->unsuccessful_head = NULL;

    HASH_ADD_INT( consumer_table, id, curr ); /* Add consumer info to the hash table */

  }
  
}

void ordersToFile( orderNode *head , FILE *fp, int success) {
  if ( head == NULL )
    return;

  ordersToFile( head->next, fp, success );
  if ( success ) 
    fprintf( fp, "%s| %f| %f\n", head->title, head->price, head->remaining );
  else 
    fprintf( fp, "%s| %f\n", head->title, head->price );
}

void consumerInfoToFile( char *outfile ) {
  FILE *fp;
  consumerNode *ptr, *tmp;

  fp = fopen( outfile, "w" );

  HASH_ITER( hh, consumer_table, ptr, tmp ) {
    fprintf( fp, "=== BEGIN CUSTOMER INFO ===\n### BALANCE ###\n" );
    fprintf( fp, "Customer name: %s\n", ptr->name );
    fprintf( fp, "Customer ID number: %d\n", ptr->id );
    fprintf( fp, "Remaining credit balance after all purchases (a dollar amount): %f\n", ptr->balance );
    fprintf( fp, "### SUCCESSFUL ORDERS ###\n" );
    ordersToFile( ptr->successful_head , fp, 1);
    fprintf( fp, "### UNSUCCESSFUL ORDERS ###\n" );
    ordersToFile( ptr->unsuccessful_head, fp, 0);
    fprintf( fp, "=== END CUSTOMER INFO ===\n\n" );
  }
  
}

void processOrder( orderNode *order ) {
  consumerNode *consumer;
  
  HASH_FIND_INT( consumer_table, &(order->uid), consumer );

  //printf( "TRANSACTION:\n\t%s|%d|%f ::::::: %s|%f|%d\n", consumer->name, consumer->id, consumer->balance, order->title, order->price, order->uid );

  if ( order->price <= consumer->balance ) { /* compare price of book order to consumer's balance */
    consumer->balance -= order->price;
    order->next = consumer->successful_head;
    order->remaining = consumer->balance;
    consumer->successful_head = order;
  }
  else { /* order->price > consumer->balance */
    order->next = consumer->unsuccessful_head;
    consumer->unsuccessful_head = order;
  }
  return;
}

void *consumer_thread_function( void *order_q ) {
  orderNode *ptr = NULL;
  orderQueue *q;
  q = (orderQueue *)order_q;


  pthread_mutex_lock( &(q->q_mutex) ); /* lock the queue so that we can iterate through it and process each node */

  while ( q->head != NULL ) {
    ptr = dequeue( q );
    processOrder( ptr );    
  }

  q->thread_complete = 1;
  
  pthread_mutex_unlock( &(q->q_mutex) ); 
  //printf( "Consumer exit\n" );
  pthread_exit( "Consumer exit" );

}

void *producer_thread_function( void *fp ) {
  char buf[BUFFER_SIZE];
  char *token;
  orderNode *curr;
  orderQueue *q_ptr = NULL;
  void *thread_result;

  while ( fgets( buf, BUFFER_SIZE, (FILE *)fp ) != NULL ) {

    curr = (orderNode *)malloc( sizeof( orderNode ) );
    
    token = strtok( buf, "|" );
    curr->title = (char *)malloc( sizeof( char ) * (strlen( token ) + 1) );
    strcpy( curr->title, token );
    curr->title[strlen( token )] = '\0';
    
    token = strtok( NULL, "|" );
    curr->price = atof( token + 1 );
    
    token = strtok( NULL, "|" );
    curr->uid = atoi( token + 1 );
    
    token = strtok( NULL, "|\n" );
    curr->category = (char *)malloc( sizeof( char ) * (strlen( token ) + 1) );
    strcpy( curr->category, token + 1 );
    curr->category[strlen( curr->category )] = '\0';

    curr->next = NULL;
    printf( "curr->category:%s\n", curr->category );
    printf( "length = %d\n", strlen(curr->category) );
    /* curr orderNode Ready to be enqueued */

    HASH_FIND_STR( category_queue_table, curr->category, q_ptr );

    pthread_mutex_lock( &q_ptr->q_mutex ); /* lock the queue while we enqueue a node */

    enqueue( curr, q_ptr );

    if ( q_ptr->thread_complete ) {
      if ( !q_ptr->first && (pthread_join( q_ptr->thread, &thread_result ) != 0) ) {
	perror( "Thread join failed" );
	exit( EXIT_FAILURE );	
      }

      q_ptr->first = 0;
      q_ptr->thread_complete = 0;

      if ( (pthread_create( &(q_ptr->thread), NULL, consumer_thread_function, q_ptr )) != 0 ) {
	perror( "Thread creation failed" );
	exit( EXIT_FAILURE );	
      }
    }    

    pthread_mutex_unlock( &(q_ptr->q_mutex) );

  }

  pthread_exit( "Producer exit" );

}

int main( int argc, char **argv ) {
  int res;
  pthread_t producer_thread;
  void *thread_result;
  FILE *consumerdb_fp;
  FILE *bookorders_fp;
  char *categories;
  orderQueue *ptr, *tmp;


  if ( argc != 4 ) {
    printUsage();
    exit( EXIT_FAILURE );
  }

  consumerdb_fp = fopen( *(argv + 1), "r" );
  bookorders_fp = fopen( *(argv + 2), "r" );
  categories = *(argv + 3);

  if ( !(consumerdb_fp) ) {
    fprintf( stderr, "File Not Found: %s\n", *(argv + 1) );
    exit( EXIT_FAILURE );
  }
  if ( !(bookorders_fp) ) {
    fprintf( stderr, "File Not Found: %s\n", *(argv + 1) );
    exit( EXIT_FAILURE );
  }

  loadConsumers( consumerdb_fp ); /* Store consumer information in memory */

  loadCategoryQueues( categories ); /* Build Structures for each of the catagories */
  
  res = pthread_create( &producer_thread, NULL, producer_thread_function, bookorders_fp );
  if ( res != 0 ) {
    perror( "Producer Thread creation failed" );
    exit( EXIT_FAILURE );
  }

  pthread_join( producer_thread, &thread_result );
  //printf( "Producer Thread joined, it returned: %s\n", (char *)thread_result );

  
  HASH_ITER( hh, category_queue_table, ptr, tmp ) { /* Iterating through the queue hash table to join everything that still may be working before we output */
    if ( !ptr->first && pthread_join( ptr->thread, &thread_result ) != 0 ) {
      perror( "Consumer Thread join failed" );
      exit( EXIT_FAILURE );
    }

  }

  consumerInfoToFile( "finaloutput.txt" );

  fclose( consumerdb_fp );
  fclose( bookorders_fp );
  exit( EXIT_SUCCESS );


}
