/*
  Author: Rishi Sadhir
  Email: rishi.sadhir@gmail.com
  Date: January 2012
  Purpose: An implementation of malloc (and free) using a first algorithm. My first C project.
 */


#include <stdio.h>

typedef struct node {
    void *address;
    int offset;
    int flag;
    struct node *next;
    struct node *prev;
} Node;


void *my_firstfit_malloc(int size);
void my_free(void *ptr);
Node *first_fit();
void printTree();
Node *coalesce(Node *a, Node *b);

static Node head;                     // This is a sentinel value
static Node tail;
static Node *current;                  // this is my list runner

int main()
{
    void *v = my_firstfit_malloc(100);    
    printTree();
    int *w = (int *)  my_firstfit_malloc(sizeof(int));    
    *w = 3;
    printTree();
    my_free(v);
    printTree();
    int *z = (int *)  my_firstfit_malloc(sizeof(int));    
    *z = 6;    
    printTree();
    printf("\n");
    my_free(z);
    printTree();

}

void *my_firstfit_malloc(int size)
{
    void *rslt;

    // Check to see if the Linked List has been initialized or not
    if (head.next == NULL)
    {
	Node *node_ptr = (Node *) sbrk(sizeof(Node));     // Put descriptor Node on the heap
	void *allocation_ptr = (void *) sbrk(size);       // Put the allocation on the heap

	node_ptr -> address = allocation_ptr;             // Store the address of the allocation in to the descriptor node 
	node_ptr -> offset = size;                        // Store the size of the allocation in the descriptor node
	node_ptr -> flag = 1;                             // Store the fact that it is not an empty allocation on to the descriptor node
	node_ptr -> next = &tail;
	node_ptr -> prev = &head;
	
	head.next = node_ptr;                             // Initialize linked list
	head.prev = NULL;
	tail.prev = node_ptr;
	tail.next = NULL;

	return (void *) node_ptr->address;
    }

    rslt = (void *) ((void *)first_fit(size)) + sizeof(Node);
    return rslt;
}


void my_free(void *ptr)
{
    int amount;
    current = ptr - sizeof(Node);
    if(current->prev == &head  &&  current->next == &tail)                                  
    {
	amount =  sizeof(Node) + current->offset;
	amount *= -1;
	head.next = NULL;
	tail.prev = NULL;
	sbrk(amount);
	// Decrement bro by sizeof(node) + offset
	// head.next and tail.prev null
    }
    else if(current->prev == &head  &&  current->next->flag == 0)
    {
	// A simple coalesce should do
	current = coalesce(current, current->next);
	current->flag = 0;
    }
    else if(current->prev == &head  &&  current->next->flag == 1)
    {
	// just flip the bit on current
	current->flag = 0;
    }
    else if(current->prev->flag == 1 &&  current->next == &tail)
    {
	// move back sbrk
	// prev.next = tail
	// tail.prev = prev
	amount = sizeof(Node) + current->offset;
	amount *= -1;
	tail.prev = current->prev;
	current->prev->next = &tail;
	sbrk(amount);
    }
    else if(current->prev->flag == 0  &&  current->next == &tail)
    {
	current = coalesce(current->prev, current);
	current->next = &tail;
	tail.prev = current;
    }
    else if(current->prev->flag == 1  &&  current->next->flag == 1)                                 //tested
    {
	// flip flag to 0
	current->flag = 0;
    }
    else if(current->prev->flag == 0  &&  current->next->flag == 0)
    {
	// Need to coalesce curr and next
	// coalsce prev and next
	coalesce(current->prev, coalesce(current, current->next));
    }
    else if(current->prev->flag == 0  &&  current->next->flag == 1)
    {
	// coalesce prev int current
	coalesce(current->prev, current);
	
    }
    else if(current->prev->flag == 1  &&  current->next->flag == 0)
    {
	// coalesce current and next
	coalesce(current, current->next);
    }
}

// Purpose: Returns a Node pointer to the start of the allocated memory
// Argument: The number of bytes that to allocate
//
//
Node *first_fit(int size)
{
    Node *rslt;                                                     // pointer to the Node that will be returned
    Node *split;
    int split_size;
    current = &head;
    
    while (current != &tail)                                        // While we havent reached the end of the linked list  
    {
	current = current->next;                              
	if (current->flag == 0  &&  current->offset >= size)        // if current is an attached allocation >= size, return it
	{
	    if(current->offset - size  >  sizeof(Node))             
	    {
		// Gotta split
		split_size = current->offset - size - sizeof(Node); 
		current->flag = 1;                                  
		current->offset = size;
		split = (Node *) (((void *)current) + sizeof(Node) + size);              // put the new node on the left over part of the allocation
		split->address = (Node *) (((void *)split) + sizeof(Node));              // point it to the remaining part of the leftover allocation
		split->offset = split_size;                                              // its offset is the that remaining size
		split->flag = 0;                                                         // its empty allocation
		split->next = current->next;                        
		split->prev = current;
		current->next = split;
		split->next->prev = split;
		
		return current;
		
	    }
	    else if (current->offset  -  size < sizeof(Node))
	    {
		// Gotta absorb
		current->flag = 1;
		return current;
	    }
	    else
	    {
		// Like a glove
		current->flag = 1;
		return current;
	    }
	}
    }
    // Reached tail, need to push heap for this allocation. Current is at tail.
    current = tail.prev;
    rslt = (Node *) sbrk(sizeof(Node));
    rslt->address = (void *) sbrk(size);
    rslt->offset = size;
    rslt->flag = 1;
    rslt->next = &tail;
    rslt->prev = current;
    current->next = rslt;
    tail.prev = rslt;
    return rslt;    
}


void printTree()
{
    printf("\nbrk is at %d\n\n", sbrk(0));
    
    current = &head;

    if (current->next == NULL)
    {
	printf("That shits empty bro!\n");
    } else {
	printf("~ ");
	while (current -> next != &tail)
	{
	    current = current->next;
	    printf("[%d, %d, %d, contents: %d] ~ ", current->flag, current->address, current->offset, *((int *) (current->address)));
	}
    }
    printf("\n");
}

// Purpose: absorb a in to b
//
//
//
Node *coalesce(Node *a, Node *b)
{
    // adjust size of a
    a->offset += b->offset + sizeof(Node);
    a->flag = 0;
    // make whatever was point to b, point to a
    b->next->prev = a;
    // make a point to whatever b was pointing to
    a->next = b->next;
    return a;
}
