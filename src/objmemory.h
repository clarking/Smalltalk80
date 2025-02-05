//
//  objmemory.h
//  Smalltalk-80
//
//  Created by Dan Banay on 2/20/20.
//  Copyright © 2020 Dan Banay. All rights reserved.
//
//  MIT License
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//

#pragma once

#include <cstdint>
#include <cassert>
#include <functional>
#include "hal.h"
#include "filesystem.h"
#include "realwordmemory.h"
#include "oops.h"

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


// Define to use recursive marking for ref counting/GC
// If undefined the stack space efficient pointer reversal approach described
// on page 678 of G&R is used. Not recommended, and only included for completeness.
//#define RECURSIVE_MARKING


#ifdef RUNTIME_CHECKING
#define RUNTIME_CHECK2(c,f,l) runtime_check(c, "RUNTIME ERROR: (" #c ") at: " f "(" #l ")")
#define RUNTIME_CHECK1(c,f,l) RUNTIME_CHECK2(c,f,l)
#define RUNTIME_CHECK(cond) RUNTIME_CHECK1(cond,  __FILE__, __LINE__)
#else
#define RUNTIME_CHECK(cond) ((void)0)
#endif


// G&R pg. 664 - Object Table Related Constants
// Object Table Segment (last segment) contains the Object Table followed by the
// head of the OT free pointer list
// +-------------------------+
// |                         | <--- ObjectTableStart
// |                         |
// |                         |
// |      Object Table       |
// |                         |
// |                         |
// +-------------------------+
// |     FreePointerList     |
// +-------------------------+
// |////// UNUSED WORD //////|
// +-------------------------+
//

#define ObjectTableSegment  (SegmentCount - 1)
#define ObjectTableStart    0
#define ObjectTableSize     (SegmentSize - 2)
// The smallest number that is too large to represent in an eight-bit count field; that is, 256.
#define HugeSize    256 // G&R pg 661

// The location of the head of the linked list of free object table entries
#define FreePointerList (ObjectTableStart + ObjectTableSize) // G&R pg. 664

// G&R pg. 664 - Object Table Related Constants
// The smallest size of chunk that is not stored on a list whose chunk share the same size.
// (Theindex of the last free chunk list).
#define BigSize 20
#define FirstFreeChunkListSize (BigSize + 1)

// Heap Constants G&R pg. 658

// The number of heaps segments used in the implementation.
// We reserve the last segment for the Object Table and use the remaining for the heap
#define HeapSegmentCount (SegmentCount - 1)

// Each heap segment is organized as follows:
//
// +-------------------------+
// |                         |
// |                         |
// |     Object Storage      |
// |                         |
// |                         |<--- HeapSpaceStop (last word)
// +-------------------------+
// |   Array of BigSize+1    |<--- FirstFreeChunkList
// |   Free Chunks Linked    |
// |   List Heads            |
// |                         |<--- LastFreeChunkList
// +-------------------------+
//

// The index of the first memory segmentused to store the heap
#define FirstHeapSegment    0
#define LastHeapSegment     (FirstHeapSegment + HeapSegmentCount - 1)

// The address of the last location used in each heap segment.
#define HeapSpaceStop   (SegmentSize - FirstFreeChunkListSize - 1)
#define HeaderSize      (2) // The number of words in an object header(2).
// If HeaderSize changes, revisit forAllOtherObjectsAccessibleFrom_suchThat_do
// where we test if the offset passes the class field...

// The location of the head of the linked list of free chunks of size zero. Comes right
// after the last word for object storage.
#define FirstFreeChunkList  (HeapSpaceStop + 1)

// The bluebook incorrectly uses LastFreeChunkList in all places it is used! The
// headOfFreeChunkList:inSegment: and headOfFreeChunkList:inSegment:put methods take
// a SIZE as the first parameter not a location.
// The location of the head of the linked list of free chunks of size BigSize or larger.
// static const int LastFreeChunkList =  FirstFreeChunkList + BigSize;

// Any sixteen-bit value that cannot be an object table index, e.g.,2**16~1.
#define NonPointer  65535

// Last special oop
// (See SystemTracer in Smalltalk.sources)
#define LastSpecialOop  52

// Snapshots
// Object space starts at offset 512 in the image
#define ObjectSpaceBaseInImage  512

#ifdef GC_MARK_SWEEP

class IGCNotification {
public:
	// About to garbage collect. Client should call addRoot to specify roots of the world
	virtual void prepareForCollection() = 0;
	
	// Garbage collection has been completed
	virtual void collectionCompleted() = 0;
};

class ObjectMemory {
public:
	ObjectMemory(IHardwareAbstractionLayer *halInterface, IGCNotification *notification = 0);

#else
	class ObjectMemory {
	public:
		ObjectMemory(IHardwareAbstractionLayer *halInterface);
#endif
	
	bool loadSnapshot(IFileSystem *fileSystem, const char *imageFileName);
	
	bool saveSnapshot(IFileSystem *fileSystem, const char *imageFileName);
	
	// --- BCIInterface ---

	#define oopsLeft ObjectMemory::freeOops
	#define coreLeft ((std::uint32_t)ObjectMemory::freeWords)
	
	void garbageCollect();
	
	int storePointer_ofObject_withValue(int fieldIndex, int objectPointer, int valuePointer);
	
	int storeWord_ofObject_withValue(int wordIndex, int objectPointer, int valueWord);
	
	inline void increaseReferencesTo(int objectPointer) {

#ifdef GC_REF_COUNT
		countUp(objectPointer);
#endif
	}
	
	int initialInstanceOf(int classPointer);
	
	inline void decreaseReferencesTo(int objectPointer) {
	
#ifdef GC_REF_COUNT
		countDown(objectPointer);
#endif
	}
	
// isIntegerValue:
#define isIntegerValue(valueWord) (valueWord >= -16384 && valueWord <= 16383)

#ifdef DEBUG
#define cantBeIntegerObject(objectPointer) \
			assert(!isIntegerObject(objectPointer)); \
			if (isIntegerObject(objectPointer)) \
				hal->error("A small integer has no object table entry");
#else
#define cantBeIntegerObject(objectPointer) \
            isIntegerObject(objectPointer) ? hal->error("A small integer has no object table entry"):(void)0;
#endif
	
	//isIntegerObject:	 ^(objectPointer bitAnd: 1) = 1
#define isIntegerObject(objectPointer) (((objectPointer) & 1) == 1)
	
	inline int ot_bits_to(int objectPointer, int firstBitIndex, int lastBitIndex) {
		// self cantBeIntegerObject: objectPointer.
		// ^wordMemory segment: ObjectTableSegment
		//	 word: ObjectTableStart + objectPointer
		//	 bits: firstBitIndex
		//	 to: lastBitIndex
		cantBeIntegerObject(objectPointer);
		return segment_word_bits_to(ObjectTableSegment,
		                            ObjectTableStart + objectPointer,
		                            firstBitIndex, lastBitIndex);
	}
	
	//cantBeIntegerObject(objectPointer);
#define locationBitsOf(objectPointer) \
        (segment_word(ObjectTableSegment, ObjectTableStart + objectPointer + 1))

#define segmentBitsOf(objectPointer)  ot_bits_to(objectPointer, 12, 15)

#define heapChunkOf_byte(objectPointer, offset) \
        segment_word_byte(segmentBitsOf(objectPointer), locationBitsOf(objectPointer) + offset / 2, offset % 2)

#define heapChunkOf_word(objectPointer, offset) \
        segment_word(segmentBitsOf(objectPointer), locationBitsOf(objectPointer) + offset)
	
	// ^self heapChunkOf: objectPointer word: 0
#define sizeBitsOf(objectPointer) \
         heapChunkOf_word(objectPointer, 0)
	
	inline int fetchWordLengthOf(int objectPointer) {
		// ^(self sizeBitsOf: objectPointer) - HeaderSize
		return sizeBitsOf(objectPointer) - HeaderSize;
	}
	
	// fetchWord:ofObject:
	inline int fetchWord_ofObject(int wordIndex, int objectPointer) {
		/* "source"
		 ^self heapChunkOf: objectPointer word: HeaderSize + wordIndex
		*/
		
		RUNTIME_CHECK(wordIndex >= 0 && wordIndex < fetchWordLengthOf(objectPointer));
		return heapChunkOf_word(objectPointer, HeaderSize + wordIndex);
	}
	
	// integerValueOf: ^objectPointer/2
	// Right shifting a negative number is undefined according to the standard.
	// return ((std::int16_t) objectPointer) >> 1;
#define  integerValueOf(objectPointer) ((std::int16_t) ((objectPointer) & 0xfffe) / 2)
	
	void swapPointersOf_and(int firstPointer, int secondPointer);
	
	int instantiateClass_withWords(int classPointer, int length);
	
	int instantiateClass_withBytes(int classPointer, int length);
	
	bool hasObject(int objectPointer);
	
	int instantiateClass_withPointers(int classPointer, int length);
	
	inline int fetchByte_ofObject(int byteIndex, int objectPointer) {
		// ^self heapChunkOf: objectPointer byte: (HeaderSize*2 + byteIndex)
		return heapChunkOf_byte(objectPointer, (HeaderSize * 2 + byteIndex));
	}
	
	inline int fetchPointer_ofObject(int fieldIndex, int objectPointer) {
		// ^self heapChunkOf: objectPointer word: HeaderSize + fieldIndex
		RUNTIME_CHECK(fieldIndex >= 0 && fieldIndex < fetchWordLengthOf(objectPointer));
		return heapChunkOf_word(objectPointer, HeaderSize + fieldIndex);
	}
	
	// ^wordMemory segment: (self segmentBitsOf: objectPointer)
	//     word: ((self locationBitsOf: objectPointer) + offset)
	
	
	// ^self heapChunkOf: objectPointer word: 1
#define  classBitsOf(objectPointer) heapChunkOf_word(objectPointer, 1)
	
	inline int fetchClassOf(int objectPointer) {
		
		// Note that fetchClassOf:objectPointer returns IntegerClass (the object table index of SmallInteger)
		// if its argument is an immediate integer. G&R pg 686
		
		// (self isIntegerObject: objectPointer)
		//	 ifTrue: [^IntegerClass] "ERROR IntegerClass not defined"
		//	 ifFalse: [^self classBitsOf: objectPointer]
		
		if (isIntegerObject(objectPointer))
			return ClassSmallInteger;
		
		return classBitsOf(objectPointer);
	}
	
	//integerObjectOf: ^(value bitShift: 1) + 1
#define integerObjectOf(value)  (((value) << 1) | 1)
	
	//  ^self ot: objectPointer bits: 8 to: 8
#define oddBitOf(objectPointer) ot_bits_to(objectPointer, 8, 8)
	
	// ^self ot: objectPointer bits: 10 to: 10
#define freeBitOf(objectPointer) ot_bits_to(objectPointer, 10, 10)
	
	inline int fetchByteLengthOf(int objectPointer) {
		// "ERROR in selector of next line"
		// ^(self fetchWordLengthOf: objectPointer)*2 - (self oddBitOf: objectPointer)
		return fetchWordLengthOf(objectPointer) * 2 - oddBitOf(objectPointer);
	}
	
	int instanceAfter(int objectPointer);
	
	inline int heapChunkOf_byte_put(int objectPointer, int offset, int value) {
		// ^wordMemory segment: (self segmentBitsOf: objectPointer)
		//     word: ((self locationBitsOf: objectPointer) + (offset//2))
		//     byte: (offset\\2) put: value
		return segment_word_byte_put(segmentBitsOf(objectPointer), locationBitsOf(objectPointer) + (offset / 2),
		                             offset % 2, value);
	}
	
	inline int storeByte_ofObject_withValue(int byteIndex, int objectPointer, int valueByte) {
		// ^self heapChunkOf: objectPointer
		//     byte: (HeaderSize*2 + byteIndex)
		//     put: valueByte
		return heapChunkOf_byte_put(objectPointer, HeaderSize * 2 + byteIndex, valueByte);
	}
	
	// --- ObjectPointers ---


#ifdef GC_MARK_SWEEP
	
	void addRoot(int rootObjectPointer) //dbanay
	{
		markObjectsAccessibleFrom(rootObjectPointer);
	}

#endif

private:
	
	// --- Compaction ---
	
	int sweepCurrentSegmentFrom(int lowWaterMark);
	
	void compactCurrentSegment();
	
	void releasePointer(int objectPointer);
	
	void reverseHeapPointersAbove(int lowWaterMark);
	
	int abandonFreeChunksInSegment(int segment);
	
	int allocateChunk(int size);

#ifdef GC_MARK_SWEEP
	// --- MarkingGarbage ---
	
	void reclaimInaccessibleObjects();
	
	int markObjectsAccessibleFrom(int rootObjectPointer);
	
	void markAccessibleObjects();
	
	void rectifyCountsAndDeallocateGarbage();
	
	void zeroReferenceCounts();

#endif
	
	// --- NonpointerObjs ---
	
	int lastPointerOf(int objectPointer);
	
	int spaceOccupiedBy(int objectPointer);
	
	int allocate_odd_pointer_extra_class(int size, int oddBit, int pointerBit, int extraWord, int classPointer);
	
	// --- UnallocatedSpc ---
	
	int headOfFreePointerList();
	
	void toFreeChunkList_add(int size, int objectPointer);
	
	int headOfFreeChunkList_inSegment_put(int size, int segment, int objectPointer);
	
	int removeFromFreePointerList();
	
	void toFreePointerListAdd(int objectPointer);
	
	int removeFromFreeChunkList(int size);
	
	void resetFreeChunkList_inSegment(int size, int segment);
	
	int headOfFreeChunkList_inSegment(int size, int segment);
	
	int headOfFreePointerListPut(int objectPointer);
	
	// --- RefCntGarbage ---
	
	int countDown(int rootObjectPointer);
	
	int countUp(int objectPointer);
	
	void deallocate(int objectPointer);
	
	int forAllOtherObjectsAccessibleFrom_suchThat_do(int objectPointer,
	                                                 const std::function<bool(int)> &predicate,
	                                                 const std::function<void(int)> &action);
	
	int forAllObjectsAccessibleFrom_suchThat_do(int objectPointer,
	                                            const std::function<bool(int)> &predicate,
	                                            const std::function<void(int)> &action);
	
	// --- ObjectTableEnt ---
	
	inline int ot_bits_to_put(int objectPointer, int firstBitIndex, int lastBitIndex, int value) {
		
		// self cantBeIntegerObject: objectPointer.
		// ^wordMemory segment: ObjectTableSegment
		// 	 word: ObjectTableStart + objectPointer
		// 	 bits: firstBitIndex
		// 	 to: lastBitIndex
		// 	 put: value
		cantBeIntegerObject(objectPointer);
		return segment_word_bits_to_put(ObjectTableSegment, ObjectTableStart + objectPointer, firstBitIndex,
		                                lastBitIndex, value);
	}
	
	
	// ^self ot: objectPointer bits: 9 to: 9 put: value
#define  pointerBitOf_put(objectPointer, value) ot_bits_to_put(objectPointer, 9, 9, value)
	
	// ^self ot: objectPointer bits: 12 to: 15 put: value
#define  segmentBitsOf_put(objectPointer, value) ot_bits_to_put(objectPointer, 12, 15, value)
	
	inline int heapChunkOf_word_put(int objectPointer, int offset, int value) {
		// ^wordMemory segment: (self segmentBitsOf: objectPointer)
		//     word: ((self locationBitsOf: objectPointer) + offset)
		//     put: value
		return segment_word_put(segmentBitsOf(objectPointer), locationBitsOf(objectPointer) + offset, value);
	}
	
	// self cantBeIntegerObject: objectPointer.
	// ^wordMemory segment: ObjectTableSegment
	//     word: ObjectTableStart + objectPointer + 1
	
	inline int ot(int objectPointer) {
		// self cantBeIntegerObject: objectPointer.
		// ^wordMemory segment: ObjectTableSegment
		//     word: ObjectTableStart + objectPointer
		
		cantBeIntegerObject(objectPointer);
		return segment_word(ObjectTableSegment, ObjectTableStart + objectPointer);
	}
	
	// ^self ot: objectPointer bits: 10 to: 10 put: value
#define freeBitOf_put(objectPointer, value) ot_bits_to_put(objectPointer, 10, 10, value)
	
	// ^self heapChunkOf: objectPointer word: 1 put: value
#define classBitsOf_put(objectPointer, value) heapChunkOf_word_put(objectPointer, 1, value)
	
	
	inline int locationBitsOf_put(int objectPointer, int value) {
		
		// self cantBeIntegerObject: objectPointer.
		// ^wordMemory segment: ObjectTableSegment
		//     word: ObjectTableStart + objectPointer + 1
		//     put: value
		cantBeIntegerObject(objectPointer);
		return segment_word_put(ObjectTableSegment, ObjectTableStart + objectPointer + 1, value);
	}
	
	// ^self ot: objectPointer bits: 8 to: 8 put: value
#define oddBitOf_put(objectPointer, value) ot_bits_to_put(objectPointer, 8, 8, value)
	
	inline int ot_put(int objectPointer, int value) {
		// self cantBeIntegerObject: objectPointer.
		// ^wordMemory segment: ObjectTableSegment
		//	 word: ObjectTableStart + objectPointer
		//	 put: value
		cantBeIntegerObject(objectPointer);
		return segment_word_put(ObjectTableSegment, ObjectTableStart + objectPointer, value);
	}
	
	// ^self ot: objectPointer bits: 0 to: 7 put: value
#define countBitsOf_put(objectPointer, value)  ot_bits_to_put(objectPointer, 0, 7, value);
	
	// ^self ot: objectPointer bits: 0 to: 7
#define countBitsOf(objectPointer) ot_bits_to(objectPointer, 0, 7)
	

	// ^self heapChunkOf: objectPointer word: 0 put: value
#define sizeBitsOf_put(objectPointer, value) heapChunkOf_word_put(objectPointer, 0, value)
	
	// ^self ot: objectPointer bits: 9 to: 9
#define  pointerBitOf(objectPointer) ot_bits_to(objectPointer, 9, 9)
	
	// --- Allocation ---
	
	int obtainPointer_location(int size, int location);
	
	int attemptToAllocateChunk(int size);
	
	int attemptToAllocateChunkInCurrentSegment(int size);
	
	void outOfMemoryError();
	
	int auditFreeOops();

#ifdef RUNTIME_CHECKING
	inline void runtime_check(bool condition, const char *errorMessage)
	{
		if (!condition)
		{
			assert(0);
			hal->error(errorMessage);
		}
	}
#endif

	public:
	
	// Special Register G&R pg. 667
	// The index of the heap segment currently being used for allocation
	static int currentSegment;
	static int freeWords; // free words remaining (make primitiveFreeCore "fast")
	
	// An a table entry with a free bit set OR that contains a reference to a free chunk
	// (free bit clear but count field zero) of memory is counted as a free oop
	static int freeOops;  // free OT entries (make primitiveFreeOops "fast")
	
private:
	bool loadObjectTable(IFileSystem *fileSystem, int fd);
	
	static bool padToPage(IFileSystem *fileSystem, int fd);
	
	bool loadObjects(IFileSystem *fileSystem, int fd);
	
	bool saveObjects(IFileSystem *fileSystem, int fd);

#ifdef GC_MARK_SWEEP
	IGCNotification *gcNotification;
#endif
	// Interface to the host operating system
	IHardwareAbstractionLayer *hal;
};

