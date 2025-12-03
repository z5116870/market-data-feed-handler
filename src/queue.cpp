#include <immintrin.h>
#include "queue.h"
#include "parse.h"
#include "cpu.h"
#include "sequencer.h"

// This thread runs the parsing function async, by popping ParsingBuffer pointers from the parsing queue and
// passing it to parseMessage, then passing the pointers back to the freeQueue
void parserThread(std::shared_ptr<SPSCQ<ParsingBuffer*>> parseQueuePtr, std::shared_ptr<SPSCQ<ParsingBuffer*>> freeQueuePtr, int cpu_num) {
    // set CPU affinity and raise priority to ensure maximum CPU share
    pinToCpu(cpu_num);
    setPriority(98);

    ParsingBuffer *next;
    while(GlobalState::runParser.load(std::memory_order_relaxed)) {
        // If the queue has an element, pop it, parse it and return the buffer back to the
        // free pool so the network thread can copy another UDP payload into it
        if(parseQueuePtr->pop(next)) { 
            parseMessage(next->data, next->size);
            freeQueuePtr->push(next);
            continue;
        }
        _mm_pause();
    }
}