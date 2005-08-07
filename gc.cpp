#include "gc.h"
#include <deque>
#include <climits>
#include <iostream>

bool gc__nowcollecting = false;
static std::deque<Collectable *> queue;

void scheduleCollect(Collectable *obj) {
    queue.push_back(obj);
}

void doCollect() {
    while (queue.size()) {
        Collectable *c = queue.front();
        queue.pop_front();
        assert(c->refcount >= 0);
        if (c->refcount == 0) {
            gc__nowcollecting = true;
            try {
                delete c;
            } catch (std::exception &e) {
                gc__nowcollecting = false;
                throw e;
            }
            gc__nowcollecting = false;
        }
    }
}
