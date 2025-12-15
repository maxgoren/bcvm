#ifndef gcobject_hpp
#define gcobject_hpp

struct GCObject {
    bool marked;
    bool isAR; 
    GCObject* next;
    GCObject() {
        isAR = false;
        marked = false;
        next = nullptr;
    }
};


#endif