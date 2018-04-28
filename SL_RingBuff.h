#ifndef SL_RINGBUF_H
#define SL_RINGBUF_H
// Simple queue/ringbuffer class

#include<Arduino.h>
#include <stdint.h>

typedef struct relement {
  uint32_t addr;
  uint8_t len;
  uint8_t *pkt;
} relement;

class SL_RingBuff {
  public:
	// initialize with number of elements in the queue
   SL_RingBuff(int size);
	// put an element into the ring, return 0 if ring is full
   int enqueue (uint8_t *val, uint8_t len, uint32_t addr);
   // return the number of elements in the ring
   int queuelevel();
   // get an element from the ring, return 0 if ring is empty
   uint8_t *dequeue (uint8_t *len, uint32_t *addr);

  private:
    int _ringsize = 0;
    int _head = 0;
    int _tail = 0;
    relement* _ring;
};
#endif
