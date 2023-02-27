

#pragma once


#ifdef _WIN32
#define SOFTWARE_MOUSE_CURSOR
#endif


// Add some helpful methods if defined

//#define DEBUG

// implement optional primitiveNext
#define IMPLEMENT_PRIMITIVE_NEXT

// implement optional primitiveAtEnd
#define IMPLEMENT_PRIMITIVE_AT_END

// implement optional primitiveNextPut
#define IMPLEMENT_PRIMITIVE_NEXT_PUT

// implement optional primitiveScanCharacters
#define IMPLEMENT_PRIMITIVE_SCANCHARS


// The Smalltalk-80 VM generates a tremendous amount of circular references as it runs
//  -- primarily a MethodContext that references a BlockContext (from a temp field) that
// has a back reference to that MethodContext (the sender field). If a reference counting only
// scheme is used, then free object table entries will eventually be consumed. If, on the other hand,
// a GC only approach is used then memory will fill up with contexts and GC will happen fairly
// frequently. Therefore, the hybrid reference counting approach with full garbage collection
// when too much circular garbage accumulates is recommended.

// GM_MARK_SWEEP and GC_REF_COUNT are not mutually exclusive!
// You can define *BOTH* for a hybrid collector which ref counts until
// memory is exhausted (cyclical data) and then does a full GC

// Mark and sweep collection when memory full
#define GC_MARK_SWEEP

// Ref counting
#define GC_REF_COUNT

// Define to use recursive marking for ref counting/GC
// If undefined the stack space efficient pointer reversal approach described
// on page 678 of G&R is used.
// Not recommended, and only included for completeness.

//#define RECURSIVE_MARKING

// Perform range checks etc. at runtime
//#define RUNTIME_CHECKING

// Use various tricks to speed up the thing
#define PERFORMANCE



