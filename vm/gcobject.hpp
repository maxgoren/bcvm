#ifndef gcobject_hpp
#define gcobject_hpp

struct GCObject {
    bool marked;
    bool isAR; 
    GCObject() {
        isAR = false;
        marked = false;
    }
};


#endif