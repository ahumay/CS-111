// NAME: Anthony Humay
// EMAIL: ahumay@ucla.edu
// ID: 304731856
#include <stdio.h> 
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// 1a
#include <sys/wait.h>
#include <termios.h>
#include <poll.h>

// 1b
#include <sys/types.h>
#include <zlib.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/stat.h>

// 2a
#include "SortedList.h"
#include <sched.h>

/**
 * variable to enable diagnositc yield calls
extern int opt_yield;
#define INSERT_YIELD    0x01    // yield in insert critical section
#define DELETE_YIELD    0x02    // yield in delete critical section
#define LOOKUP_YIELD    0x04    // yield in lookup/length critical esction
*/

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *  The specified element will be inserted in to
 *  the specified list, which will be kept sorted
 *  in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
    int off_flag = 1; // debug
    if (list -> next == NULL && off_flag){
        if (opt_yield & INSERT_YIELD){
            sched_yield();
        }
        list -> prev = element;
        list -> next = element;
        SortedListElement_t * n = list -> next;
        if (n == NULL){
            off_flag = 1;
        }
        element -> prev = list;
        element -> next = list;
    } else {
        SortedListElement_t * n = list -> next;
        SortedListElement_t * x = list -> next;
        while (n -> key != NULL) {
            if (strcmp(n -> key, element -> key) < 0){
                n = n -> next;
                x = x -> next;
            } else {
                break;
            }
        }
        if (opt_yield & INSERT_YIELD){
            sched_yield();
        }
        element -> prev = n -> prev;
        element -> next = n;
        n -> prev = element;
        n -> prev -> next = element;
    }

    if (element -> prev){
        return;
    }
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *  The specified element will be removed from whatever
 *  list it is currently in.
 *
 *  Before doing the deletion, we check to make sure that
 *  next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete(SortedListElement_t * element){
    SortedListElement_t * x = element -> next;
    int off_flag = 1;
    if (element -> prev -> next == element && element -> next -> prev == element && off_flag) {
        if (opt_yield & DELETE_YIELD){
            sched_yield();
        }
        if (x == NULL){
            off_flag = 1;
        }
        element -> next -> prev = element -> prev;
        element -> prev -> next = element -> next;
        return 0;
    }
    return 1;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *  The specified list will be searched for an
 *  element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
    if (list == NULL){
        return NULL;
    }
    if (list -> key != NULL){
        return NULL;
    }
    if (key == NULL){
        return NULL;
    }
    SortedListElement_t * n = list -> next;
    int off_flag = 1;
    while (n != list && off_flag){
        if (strcmp(n -> key, key) == 0){
            return n;
        }
        if (opt_yield & LOOKUP_YIELD){
            sched_yield();
        }
        n = n -> next;
    }
    return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *  While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *     -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list){
    if (list == NULL){
        return -1;
    }
    if (list -> key != NULL){
        return -1;
    }

    SortedListElement_t * n = list -> next;
    int cnt = 0;
    while (n != list){
        if (n != n -> next -> prev){
            return -1;
        }
        if (n != n -> prev -> next){
            return -1;
        }

        if (opt_yield & LOOKUP_YIELD){
            sched_yield();
        }
        n = n -> next;
        cnt++;
    }
    return cnt;
}