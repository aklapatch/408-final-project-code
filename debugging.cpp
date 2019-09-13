
#include "debugging.h"

void printAvailableBytes(void) {
    int counter;
    struct elem *head, *current, *nextone;
    current = head = (struct elem *)malloc(sizeof(struct elem));
    if (head == NULL) {
        printf("\r\n No memory available\r\n");
        return; /*No memory available.*/
    }

    counter = 0;
    // __disable_irq();
    do {
        counter++;
        current->next = (struct elem *)malloc(sizeof(struct elem));
        current = current->next;
    } while (current != NULL);
    /* Now counter holds the number of type elem
       structures we were able to allocate. We
       must free them all before returning. */
    current = head;
    do {
        nextone = current->next;
        free(current);
        current = nextone;
    } while (nextone != NULL);
    // __enable_irq();

    printf("\r\nYou have %d Bytes free\r\n", counter * FREEMEM_CELL);
}