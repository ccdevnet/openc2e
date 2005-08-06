#ifndef GC_H
#define GC_H 1

#include <cassert>
#include <cstddef>
#include <climits>

void scheduleCollect(class Collectable *obj);
void doCollect(void);

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
        Collectable() : refcount(1), scheduled(false) { release(); }
        ~Collectable() {
            refcount = INT_MIN;
        }
};
        

#endif

