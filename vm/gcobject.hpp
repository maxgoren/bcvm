#ifndef gcobject_hpp
#define gcobject_hpp

enum GCType {
    STRING, FUNCTION, CLOSURE, LIST, CLASS, NILPTR
};

struct GCObject {
    bool marked;
    bool isAR; 
    GCObject() {
        isAR = false;
        marked = false;
    }
};


#endif