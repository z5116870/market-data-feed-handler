#include "queue.h"
#include "parse.h"
#include "cpu.h"

// This thread runs the parsing function async, by popping ParsingBuffer pointers from its queue and
// passing it to parseMessage
void parserThread(std::shared_ptr<SPSCQ<ParsingBuffer*>> parseQueuePtr, std::shared_ptr<SPSCQ<ParsingBuffer*>> freeQueuePtr, int cpu_num) {
    // set CPU affinity and raise priority to ensure maximum CPU share
    pinToCpu(cpu_num);
    raisePriority();

    ParsingBuffer *next;
    while(1) {
        // If the queue has an element, pop it, parse it and return the buffer back to the
        // free pool so the network thread can copy another UDP payload into it
        if(parseQueuePtr->pop(next)) { 
            parseMessage(next->data, next->size);
            freeQueuePtr->push(next);
        }
    }
}