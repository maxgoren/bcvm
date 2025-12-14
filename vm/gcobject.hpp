#ifndef gcobject_hpp
#define gcobject_hpp

struct GCObject {
    bool marked;
    GCObject* next;
    GCObject() {
        marked = false;
        next = nullptr;
    }
};


#endif