#include "gc.h"
#include <deque>
#include <climits>

static std::deque<Collectable *> queue;

void scheduleCollect(Collectable *obj) {
    queue.push_back(obj);
}

void doCollect() {
    while (queue.size()) {
        Collectable *c = queue.front();
        queue.pop_front();
        if (c->refcount == 0) {
            c->refcount = INT_MIN;
            delete c;
        }
    }
}
