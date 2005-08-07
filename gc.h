#ifndef GC_H
#define GC_H 1

#include <cassert>
#include <cstddef>
#include <iostream>

void scheduleCollect(class Collectable *obj);
void doCollect(void);
extern bool gc__nowcollecting;

// Do _NOT_ exception out of a Collectable constructor.
class Collectable {
    private:
        int refcount;
        bool scheduled;
        friend void doCollect(void);
    public:
        void release() {
            assert(refcount > 0);
            if (!--refcount && !scheduled) {
                scheduled = true;
                scheduleCollect(this);
            }
        }

        void retain() {
            assert(++refcount);
        }
        Collectable() : refcount(1), scheduled(false) {
            release();
        }
        ~Collectable() {
            assert(gc__nowcollecting);
            refcount = -400;
        }
};
        

#endif

