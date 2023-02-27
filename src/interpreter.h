//
//  interpreter.h
//  Smalltalk-80
//
//  Created by Dan Banay on 3/31/20.
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

#include <string>
#include "objmemory.h"
#include "filesystem.h"
#include "hal.h"
#include "bitblt.h"

class Interpreter
#ifdef GC_MARK_SWEEP
	: IGCNotification
#endif
{
public:
	
	Interpreter(IHardwareAbstractionLayer *halInterface, IFileSystem *fileSystemInterface);
	
	bool init();
	
	void cycle();
	
	inline void checkLowMemoryConditions() {
		checkLowMemory = true;
	}
	
	void asynchronousSignal(int aSemaphore);
	
	int lastBytecode() {
		// Debug/testing
		return currentBytecode;
	}
	
	int getDisplayBits(int width, int height);
	
	inline int fetchWord_ofDislayBits(int wordIndex, int displayBits) {
		// Allow read-only access to display form data
		return memory.fetchWord_ofObject(wordIndex, displayBits);
	}

private:
	
	void error(const char *message);
	
	// --- ArrayStrmPrims ---
	
	void primitiveAtEnd();
	
	void checkIndexableBoundsOf_in(int index, int array);
	
	void primitiveNextPut();
	
	int lengthOf(int array);
	
	void primitiveNext();
	
	void dispatchSubscriptAndStreamPrimitives();
	
	void primitiveStringAt();
	
	void primitiveAt();
	
	void primitiveSize();
	
	void primitiveStringAtPut();
	
	int subscript_with(int array, int index);
	
	void primitiveAtPut();
	
	void subscript_with_storing(int array, int index, int value);
	
	// --- Contexts ---
	
	void storeContextRegisters();
	
	inline void unPop(int number) {
		//stackPointer <- stackPointer + number
		stackPointer = stackPointer + number;
	}
	
	bool isBlockContext(int contextPointer);
	
	inline void push(int object) {
		/* "source"
			stackPointer <- stackPointer + 1.
			memory storePointer: stackPointer
				ofObject: activeContext
				withValue: object
		*/
		
		stackPointer = stackPointer + 1;
		memory.storePointer_ofObject_withValue(stackPointer, activeContext, object);
		
	}
	
	inline int instructionPointerOfContext(int contextPointer) {
		/* "source"
			^self fetchInteger: InstructionPointerIndex
				ofObject: contextPointer
		*/
		
		return fetchInteger_ofObject(InstructionPointerIndex, contextPointer);
	}
	
	void newActiveContext(int aContext);
	
	inline int argumentCountOfBlock(int blockPointer) {
		/* "source"
			^self fetchInteger: BlockArgumentCountIndex
				ofObject: blockPointer
		*/
		
		return fetchInteger_ofObject(BlockArgumentCountIndex, blockPointer);
	}
	
	inline int literal(int offset) {
		/* "source"
			^self literal: offset
				ofMethod: method
		*/
		
		return literal_ofMethod(offset, method);
	}
	
	inline int sender() {
		/* "source"
			^memory fetchPointer: SenderIndex
				ofObject: homeContext
		*/
		
		return memory.fetchPointer_ofObject(SenderIndex, homeContext);
	}
	
	inline int temporary(int offset) {
		/* "source"
			^memory fetchPointer: offset + TempFrameStart
				ofObject: homeContext
		*/
		
		return memory.fetchPointer_ofObject(offset + TempFrameStart, homeContext);
	}
	
	inline int caller() {
		/* "source"
			^memory fetchPointer: SenderIndex
				ofObject: activeContext
		*/
		return memory.fetchPointer_ofObject(SenderIndex, activeContext);
	}
	
	inline void pop(int number) {
		/* "source"
			stackPointer <- stackPointer - number
		*/
		
		stackPointer = stackPointer - number;
	}
	
	inline void storeStackPointerValue_inContext(int value, int contextPointer) {
		/* "source"
			self storeInteger: StackPointerIndex
				ofObject: contextPointer
				withValue: value
		*/
		
		storeInteger_ofObject_withValue(StackPointerIndex, contextPointer, value);
	}
	
	inline int stackValue(int offset) {
		/* "source"
			^memory fetchPointer: stackPointer - offset
				ofObject: activeContext
		*/
		return memory.fetchPointer_ofObject(stackPointer - offset, activeContext);
	}
	
	inline int stackTop() {
		/* "source"
			^memory fetchPointer: stackPointer
				ofObject: activeContext
		*/
		
		return memory.fetchPointer_ofObject(stackPointer, activeContext);
	}
	
	inline int popStack() {
		int stackTop;
		
		/* "source"
			stackTop <- memory fetchPointer: stackPointer
					ofObject: activeContext.
			stackPointer <- stackPointer - 1.
			^stackTop
		*/
		
		stackTop = memory.fetchPointer_ofObject(stackPointer, activeContext);
		stackPointer = stackPointer - 1;
		return stackTop;
	}
	
	void fetchContextRegisters();
	
	inline void storeInstructionPointerValue_inContext(int value, int contextPointer) {
		/* "source"
			self storeInteger: InstructionPointerIndex
				ofObject: contextPointer
				withValue: value
		*/
		
		storeInteger_ofObject_withValue(InstructionPointerIndex, contextPointer, value);
	}
	
	inline int stackPointerOfContext(int contextPointer) {
		/* "source"
			^self fetchInteger: StackPointerIndex
				ofObject: contextPointer
		*/
		
		return fetchInteger_ofObject(StackPointerIndex, contextPointer);
	}
	
	
	// --- IOPrims ---
	
	void dispatchInputOutputPrimitives();
	
	void primitiveMousePoint();
	
	void primitiveCursorLocPut();
	
	void primitiveCursorLink();
	
	void primitiveInputSemaphore();
	
	void primitiveSampleInterval();
	
	void primitiveInputWord();
	
	void updateDisplay(int destForm, int updatedHeight, int updatedWidth, int updatedX, int updatedY);
	
	void primitiveCopyBits();
	
	void primitiveSnapshot();
	
	void primitiveTimeWordsInto();
	
	void primitiveTickWordsInto();
	
	void primitiveSignalAtTick();
	
	void primitiveBeCursor();
	
	void primitiveBeDisplay();
	
	void primitiveScanCharacters();
	
	void primitiveDrawLoop();
	
	void primitiveStringReplace();
	
	// --- Classes ---
	
	bool lookupMethodInDictionary(int dictionary);
	
	inline bool isPointers(int classPointer) {
		int pointersFlag;
		
		/* "source"
			pointersFlag <- self extractBits: 0 to: 0
					of: (self instanceSpecificationOf: classPointer).
			^pointersFlag = 1
		*/
		pointersFlag = extractBits_to_of(0, 0, instanceSpecificationOf(classPointer));
		return pointersFlag == 1;
	}
	
	inline int superclassOf(int classPointer) {
		/* "source"
			^memory fetchPointer: SuperclassIndex
				ofObject: classPointer
		*/
		return memory.fetchPointer_ofObject(SuperclassIndex, classPointer);
	}
	
	inline int fixedFieldsOf(int classPointer) {
		/* "source"
			^self extractBits: 4 to: 14
				of: (self instanceSpecificationOf: classPointer)
		*/
		
		return extractBits_to_of(4, 14, instanceSpecificationOf(classPointer));
	}
	
	inline bool isWords(int classPointer) {
		int wordsFlag;
		
		/* "source"
			wordsFlag <- self extractBits: 1 to: 1
					of: (self instanceSpecificationOf: classPointer).
			^wordsFlag = 1
		*/
		
		wordsFlag = extractBits_to_of(1, 1, instanceSpecificationOf(classPointer));
		return wordsFlag == 1;
	}
	
	inline int hash(int objectPointer) {
		/* "source"
			^objectPointer bitShift: -1
		*/
		
		return objectPointer >> 1;
		
	}
	
	inline bool isIndexable(int classPointer) {
		int indexableFlag;
		
		/* "source"
			indexableFlag <- self extractBits: 2 to: 2
						of: (self instanceSpecificationOf: classPointer).
			^indexableFlag = 1
		*/
		
		indexableFlag = extractBits_to_of(2, 2, instanceSpecificationOf(classPointer));
		return indexableFlag == 1;
	}
	
	inline int instanceSpecificationOf(int classPointer) {
		/* "source"
			^memory fetchPointer: InstanceSpecificationIndex
				ofObject: classPointer
		*/
		
		return memory.fetchPointer_ofObject(InstanceSpecificationIndex, classPointer);
	}
	
	void createActualMessage();
	
	bool lookupMethodInClass(int cls);
	
	// --- ReturnBytecode ---
	
	void returnToActiveContext(int aContext);
	
	void returnBytecode();
	
	void nilContextFields();
	
	void returnValue_to(int resultPointer, int contextPointer);
	
	// --- ControlPrims ---
	
	void synchronousSignal(int aSemaphore);
	
	void primitiveBlockCopy();
	
	void primitiveResume();
	
	void primitivePerformWithArgs();
	
	int wakeHighestPriority();
	
	void primitivePerform();
	
	void primitiveValueWithArgs();
	
	int removeFirstLinkOfList(int aLinkedList);
	
	void primitiveWait();
	
	void primitiveFlushCache();
	
	void suspendActive();
	
	int activeProcess();
	
	inline int schedulerPointer() {
		/* "source"
			^memory fetchPointer: ValueIndex
				ofObject: SchedulerAssociationPointer
		*/
		return memory.fetchPointer_ofObject(ValueIndex, SchedulerAssociationPointer);
	}
	
	void addLastLink_toList(int aLink, int aLinkedList);
	
	void dispatchControlPrimitives();
	
	void checkProcessSwitch();
	
	void primitiveSignal();
	
	int isEmptyList(int aLinkedList);
	
	void primitiveSuspend();
	
	void primitiveValue();
	
	int firstContext();
	
	void transferTo(int aProcess);
	
	void resume(int aProcess);
	
	void sleep(int aProcess);
	
	// --- SystemPrims ---
	
	void primitiveClass();
	
	void dispatchSystemPrimitives();
	
	void primitiveEquivalent();
	
	void primitiveCoreLeft();
	
	void primitiveQuit();
	
	void primitiveExitToDebugger();
	
	void primitiveOopsLeft();
	
	void primitiveSignalAtOopsLeftWordsLeft();
	
	void dispatchPrivatePrimitives();
	
	// Posix filesystem primitives -- dbanay
	void primitiveBeSnapshotFile();
	
	void primitivePosixFileOperation();
	
	void primitivePosixDirectoryOperation();
	
	void primitivePosixLastErrorOperation();
	
	void primitivePosixErrorStringOperation();
	
	// --- PrimitiveTest ---
	
	inline void success(bool successValue) {
		/* "source"
		 success <- successValue & success
		 */
		successFlag = successFlag && successValue;
	}
	
	void dispatchPrimitives();
	
	int positive16BitValueOf(int integerPointer);
	
	std::uint32_t positive32BitValueOf(int integerPointer);
	
	inline void initPrimitive() {
		/* "source"
		 success <- true
		 */
		
		successFlag = true;
	}
	
	inline bool success() {
		 // ^success
		return successFlag;
	}
	
	bool primitiveResponse();
	
	void quickInstanceLoad();
	
	void arithmeticSelectorPrimitive();
	
	int primitiveFail() {
		//  success <- false
		successFlag = false;
		return 0; // invalid oop
	}
	
	inline void pushInteger(int integerValue) {
		/* "source"
			self push: (memory integerObjectOf: integerValue)
		*/
		push(memory.integerObjectOf(integerValue));
	}
	
	// quickReturnSelf
	inline void quickReturnSelf() {
		// self is on stack top
	}
	
	int positive16BitIntegerFor(int integerValue);
	
	int positive32BitIntegerFor(int integerValue);
	
	int popInteger();
	
	int specialSelectorPrimitiveResponse();
	
	void commonSelectorPrimitive();
	
	// --- Initialization ---
	
	void initializeMethodCache();
	
	// --- ArithmeticPrim ---
	
	void primitiveMod();
	
	void dispatchArithmeticPrimitives();
	
	void primitiveEqual();
	
	void primitiveBitOr();
	
	void primitiveDivide();
	
	void primitiveMultiply();
	
	inline void dispatchLargeIntegerPrimitives() {
		//	self primitiveFail
		primitiveFail();
	}
	
	void primitiveBitAnd();
	
	void primitiveSubtract();
	
	void dispatchIntegerPrimitives();
	
	void primitiveGreaterOrEqual();
	
	void primitiveAdd();
	
	void primitiveNotEqual();
	
	void primitiveQuo();
	
	void dispatchFloatPrimitives();
	
	void primitiveAsFloat();
	
	void primitiveFloatAdd();
	
	void primitiveFloatSubtract();
	
	void primitiveFloatLessThan();
	
	void primitiveFloatGreaterThan();
	
	void primitiveFloatLessOrEqual();
	
	void primitiveFloatGreaterOrEqual();
	
	void primitiveFloatEqual();
	
	void primitiveFloatNotEqual();
	
	void primitiveFloatMultiply();
	
	void primitiveFloatDivide();
	
	void primitiveTruncated();
	
	void primitiveFractionalPart();
	
	inline void primitiveExponent() {
		primitiveFail(); // optional
	}
	
	inline void primitiveTimesTwoPower() {
		primitiveFail(); // optional
	}
	
	void primitiveLessOrEqual();
	
	void primitiveMakePoint();
	
	void primitiveBitXor();
	
	void primitiveLessThan();
	
	void primitiveBitShift();
	
	void primitiveGreaterThan();
	
	void primitiveDiv();
	
	// --- SendBytecodes ---
	
	void sendSelector_argumentCount(int selector, int count);
	
	void findNewMethodInClass(int cls);
	
	void activateNewMethod();
	
	void sendSpecialSelectorBytecode();
	
	void doubleExtendedSuperBytecode();
	
	void sendBytecode();
	
	void doubleExtendedSendBytecode();
	
	void sendSelectorToClass(int classPointer);
	
	void sendLiteralSelectorBytecode();
	
	void singleExtendedSuperBytecode();
	
	void singleExtendedSendBytecode();
	
	void extendedSendBytecode();
	
	void executeNewMethod();
	
	// --- MainLoop ---
	
	void dispatchOnThisBytecode();
	
	int fetchByte();
	
	void interpret();

private:
	// --- CompiledMethod ---
	
	inline int headerOf(int methodPointer) {
		/* "source"
			^memory fetchPointer: HeaderIndex
				ofObject: methodPointer
		*/
		
		return memory.fetchPointer_ofObject(HeaderIndex, methodPointer);
	}
	
	inline int literalCountOf(int methodPointer) {
		/* "source"
			^self literalCountOfHeader: (self headerOf: methodPointer)
		*/
		
		return literalCountOfHeader(headerOf(methodPointer));
	}
	
	int primitiveIndexOf(int methodPointer);
	
	int argumentCountOf(int methodPointer);
	
	inline int literalCountOfHeader(int headerPointer) {
		/* "source"
			^self extractBits: 9 to: 14
				of: headerPointer
		*/
		
		return extractBits_to_of(9, 14, headerPointer);
	}
	
	inline int fieldIndexOf(int methodPointer) {
		/* "source"
			^self extractBits: 3 to: 7
				of: (self headerOf: methodPointer)
		*/
		return extractBits_to_of(3, 7, headerOf(methodPointer));
	}
	
	int methodClassOf(int methodPointer);
	
	inline int literal_ofMethod(int offset, int methodPointer) {
		/* "source"
			^memory fetchPointer: offset + LiteralStart
				ofObject: methodPointer
		*/
		
		return memory.fetchPointer_ofObject(offset + LiteralStart, methodPointer);
	}
	
	inline int temporaryCountOf(int methodPointer) {
		/* "source"
			^self extractBits: 3 to: 7
				of: (self headerOf: methodPointer)
		*/
		
		return extractBits_to_of(3, 7, headerOf(methodPointer));
	}
	
	inline int largeContextFlagOf(int methodPointer) {
		/* "source"
			^self extractBits: 8 to: 8
				of: (self headerOf: methodPointer)
		*/
		
		return extractBits_to_of(8, 8, headerOf(methodPointer));
	}
	
	inline int objectPointerCountOf(int methodPointer) {
		/* "source"
			^(self literalCountOf: methodPointer) + LiteralStart
		*/
		
		return literalCountOf(methodPointer) + LiteralStart;
	}
	
	inline int headerExtensionOf(int methodPointer) {
		int literalCount;
		
		/* "source"
			literalCount <- self literalCountOf: methodPointer.
			^self literal: literalCount - 2
				ofMethod: methodPointer
		*/
		
		literalCount = literalCountOf(methodPointer);
		return literal_ofMethod(literalCount - 2, methodPointer);
	}
	
	inline int flagValueOf(int methodPointer) {
		/* "source"
			^self extractBits: 0 to: 2
				of: (self headerOf: methodPointer)
		*/
		
		return extractBits_to_of(0, 2, headerOf(methodPointer));
	}
	
	inline int initialInstructionPointerOfMethod(int methodPointer) {
		/* "source"
			^((self literalCountOf: methodPointer) + LiteralStart) * 2 + 1
		*/
		
		return (literalCountOf(methodPointer) + LiteralStart) * 2 + 1;
	}
	
	// --- StackBytecodes ---
	
	inline void pushLiteralVariableBytecode() {
		int fieldIndex;
		
		/* "source"
			fieldIndex <- self extractBits: 11 to: 15
					of: currentBytecode.
			self pushLiteralVariable: fieldIndex
		*/
		fieldIndex = extractBits_to_of(11, 15, currentBytecode);
		pushLiteralVariable(fieldIndex);
	}
	
	inline void pushLiteralConstant(int literalIndex) {
		/* "source"
			self push: (self literal: literalIndex)
		*/
		
		push(literal(literalIndex));
	}
	
	inline void popStackBytecode() {
		/* "source"
			self popStack
		*/
		popStack();
	}
	
	void storeAndPopReceiverVariableBytecode();
	
	void extendedStoreBytecode();
	
	void pushLiteralConstantBytecode();
	
	void storeAndPopTemporaryVariableBytecode();
	
	void extendedStoreAndPopBytecode();
	
	inline void pushReceiverBytecode() {
		/* "source"
			self push: receiver
		*/
		
		push(receiver);
	}
	
	inline void duplicateTopBytecode() {
		/* "source"
			^self push: self stackTop
		*/
		
		push(stackTop());
	}
	
	inline void pushReceiverVariableBytecode() {
		int fieldIndex;
		
		/* "source"
			fieldIndex <- self extractBits: 12 to: 15
					of: currentBytecode.
			self pushReceiverVariable: fieldIndex
		*/
		
		fieldIndex = extractBits_to_of(12, 15, currentBytecode);
		pushReceiverVariable(fieldIndex);
	}
	
	inline void pushActiveContextBytecode() {
		/* "source"
			self push: activeContext
		*/
		push(activeContext);
	}
	
	void stackBytecode();
	
	void extendedPushBytecode();
	
	inline void pushTemporaryVariable(int temporaryIndex) {
		/* "source"
			self push: (self temporary: temporaryIndex)
		*/
		
		push(temporary(temporaryIndex));
	}
	
	inline void pushReceiverVariable(int fieldIndex) {
		/* "source"
			self push: (memory fetchPointer: fieldIndex
					ofObject: receiver)
		*/
		push(memory.fetchPointer_ofObject(fieldIndex, receiver));
	}
	
	void pushConstantBytecode();
	
	inline void pushTemporaryVariableBytecode() {
		int fieldIndex;
		
		/* "source"
			fieldIndex <- self extractBits: 12 to: 15
					of: currentBytecode.
			self pushTemporaryVariable: fieldIndex
		*/
		
		fieldIndex = extractBits_to_of(12, 15, currentBytecode);
		pushTemporaryVariable(fieldIndex);
	}
	
	inline void pushLiteralVariable(int literalIndex) {
		int association;
		
		/* "source"
			association <- self literal: literalIndex.
			self push: (memory fetchPointer: ValueIndex
					ofObject: association)
		*/
		association = literal(literalIndex);
		push(memory.fetchPointer_ofObject(ValueIndex, association));
	}
	
	// --- JumpBytecodes ---
	
	inline void sendMustBeBoolean() {
		/* "source"
			self sendSelector: MustBeBooleanSelector
				argumentCount: 0
		*/
		
		sendSelector_argumentCount(MustBeBooleanSelector, 0);
	}
	
	inline void shortUnconditionalJump() {
		int offset;
		
		/* "source"
			offset <- self extractBits: 13 to: 15
					of: currentBytecode.
			self jump: offset + 1
		*/
		
		offset = extractBits_to_of(13, 15, currentBytecode);
		jump(offset + 1);
	}
	
	inline void jump(int offset) {
		/* "source"
			instructionPointer <- instructionPointer + offset
		*/
		
		instructionPointer = instructionPointer + offset;
	}
	
	void jumpIf_by(int condition, int offset);
	
	void longConditionalJump();
	
	inline void longUnconditionalJump() {
		int offset;
		
		/* "source"
			offset <- self extractBits: 13 to: 15
					of: currentBytecode.
			self jump: offset - 4 * 256 + self fetchByte
		*/
		offset = extractBits_to_of(13, 15, currentBytecode);
		jump((offset - 4) * 256 + fetchByte());
	}
	
	void jumpBytecode();
	
	inline void shortConditionalJump() {
		int offset;
		
		/* "source"
			offset <- self extractBits: 13 to: 15
					of: currentBytecode.
			self jumpIf: FalsePointer
				by: offset + 1
		*/
		
		offset = extractBits_to_of(13, 15, currentBytecode);
		jumpIf_by(FalsePointer, offset + 1);
	}
	
	// --- IntegerAccess ---
	
	void storeInteger_ofObject_withValue(int fieldIndex, int objectPointer, int integerValue);
	
	inline int extractBits_to_of(int firstBitIndex, int lastBitIndex, int anInteger) {
		/* "source"
		 ^(anInteger bitShift: lastBitIndex - 15)
		 bitAnd: (2 raisedTo: lastBitIndex - firstBitIndex + 1) - 1
		 */
		
		std::uint16_t mask = (1 << (lastBitIndex - firstBitIndex + 1)) - 1;
		std::uint16_t shift = anInteger >> (15 - lastBitIndex);
		return shift & mask;
		
	}
	
	void transfer_fromIndex_ofObject_toIndex_ofObject(
		int count,
		int firstFrom,
		int fromOop,
		int firstTo,
		int toOop);
	
	inline int lowByteOf(int anInteger) {
		/* "source"
			^self extractBits: 8 to: 15
				of: anInteger
		*/
		
		return extractBits_to_of(8, 15, anInteger);
	}
	
	int fetchInteger_ofObject(int fieldIndex, int objectPointer);
	
	inline int highByteOf(int anInteger) {
		/* "source"
			^self extractBits: 0 to: 7
				of: anInteger
		*/
		return extractBits_to_of(0, 7, anInteger);
	}
	
	// --- StoreMgmtPrims ---
	
	inline void checkInstanceVariableBoundsOf_in(int index, int object) {
		/* "source"
			class <- memory fetchClassOf: object.
			self success: index >= 1.
			self success: index <= (self lengthOf: object)
		*/
		//cls = memory.fetchClassOf(object);
		success(index >= 1);
		success(index <= lengthOf(object));
	}
	
	void primitiveNewMethod();
	
	void primitiveAsOop();
	
	void primitiveSomeInstance();
	
	void primitiveObjectAt();
	
	void primitiveNextInstance();
	
	void primitiveNew();
	
	void primitiveAsObject();
	
	void primitiveNewWithArg();
	
	void primitiveInstVarAtPut();
	
	void primitiveObjectAtPut();
	
	void primitiveInstVarAt();
	
	void primitiveBecome();
	
	void dispatchStorageManagementPrimitives();
	
	// --Float Access--
	
	void pushFloat(float f);
	
	inline float extractFloat(int objectPointer) {
		std::uint32_t uint32 =
			(memory.fetchWord_ofObject(1, objectPointer) << 16) | memory.fetchWord_ofObject(0, objectPointer);
		return *(float *) &uint32;
	}
	
	float popFloat();
	
	bool isInLowMemoryCondition();

#ifdef GC_MARK_SWEEP
	
	void prepareForCollection();
	
	void collectionCompleted();

#endif

private:
	
	// initializePointIndices
	static const int XIndex = 0;
	static const int YIndex = 1;
	static const int ClassPointSize = 2;
	
	// initializeStreamIndices
	static const int StreamArrayIndex = 0;
	static const int StreamIndexIndex = 1;
	static const int StreamReadLimitIndex = 2;
	static const int StreamWriteLimitIndex = 3;
	
	// initializeSchedulerIndices
	// Class ProcessorScheduler
	static const int ProcessListsIndex = 0;
	static const int ActiveProcessIndex = 1;

	// Class LinkedList
	static const int FirstLinkIndex = 0;
	static const int LastLinkIndex = 1;

	// Class Semaphore
	static const int ExcessSignalsIndex = 2;

	// Class Link
	static const int NextLinkIndex = 0;

	// Class Process
	static const int SuspendedContextIndex = 1;
	static const int PriorityIndex = 2;
	static const int MyListIndex = 3;
	
	// initializeMessageIndices
	static const int MessageSelectorIndex = 0;
	static const int MessageArgumentsIndex = 1;
	static const int MessageSize = 2;
	
	// initializeClassIndices

	// Class Class
	static const int SuperclassIndex = 0;
	static const int MessageDictionaryIndex = 1;
	static const int InstanceSpecificationIndex = 2;

	// Fields of a message dictionary
	static const int MethodArrayIndex = 1;
	static const int SelectorStart = 2;
	
	// initializeSmallIntegers

	// SmallIntegers"
	static const int MinusOnePointer = 65535;
	static const int ZeroPointer = 1;
	static const int OnePointer = 3;
	static const int TwoPointer = 5;
	
	// initializeContextIndices

	// Class MethodContext
	static const int SenderIndex = 0;
	static const int InstructionPointerIndex = 1;
	static const int StackPointerIndex = 2;
	static const int MethodIndex = 3;
	static const int ReceiverIndex = 5;
	static const int TempFrameStart = 6;

	// Class BlockContext
	static const int CallerIndex = 0;
	static const int BlockArgumentCountIndex = 3;
	static const int InitialIPIndex = 4;
	static const int HomeIndex = 5;
	
	// initializeAssociationIndex
	static const int ValueIndex = 1;
	
	// initializeCharacterIndex
	static const int CharacterValueIndex = 0;
	
	// initializeMethodIndices

	// Class CompiledMethod
	static const int HeaderIndex = 0;
	static const int LiteralStart = 1;
	
	// Forms
	static const int BitsInForm = 0;
	static const int WidthInForm = 1;
	static const int HeightInForm = 2;
	static const int OffsetInForm = 3;
	
	// Files
	static const int FileNameIndex = 1;    // fileName field of File
	
	// "Registers"
	int activeContext;
	int homeContext;
	int method;
	int receiver;
	int instructionPointer;
	int stackPointer;
	int currentBytecode;
	bool successFlag;
	
	// Class related registers
	int messageSelector;
	int argumentCount;
	int newMethod;
	int primitiveIndex;
	
	// Process-related Registers (pg 642)
	// The newProcessWaiting register will be true if a process switch is called for and false otherwise.
	bool newProcessWaiting;
	
	// If newProcessWaiting is true then the newProcess register will point to the Process to be transferred to.
	int newProcess;
	
	// The semaphoreList register points to an Array used by the interpreter to buffer Semaphores that should be signaled.
	// This is an Array in Interpreter, not in the object memory. It will be a table in a machine-language interpreter.
	
	int semaphoreList[4096];
	
	// The semaphoreIndex register hold the index of the last Semaphore in the semaphoreList buffer.
	int semaphoreIndex;
	
	// Using an array of int for method cache to remain faithful as possible to the bluebook
	// Any size change will require changes to hash function in findNewMethodInClass
	int methodCache[1024];
	
	ObjectMemory memory;
	
	// dbanay - primitiveSignalAtOopsLeftWordsLeft support
	bool checkLowMemory;
	bool memoryIsLow;
	int lowSpaceSemaphore;
	int oopsLeftLimit;
	std::uint32_t wordsLeftLimit;
	
	IHardwareAbstractionLayer *hal;
	IFileSystem *fileSystem;
	int currentDisplay;
	int currentDisplayWidth;
	int currentDisplayHeight;
	int currentCursor;
	
	// Return a std::string for a string or symbol oop
	std::string stringFromObject(int strOop);
	
	int stringObjectFor(const char *s);

#ifdef DEBUG
	std::string selectorName(int selector);
	std::string classNameOfObject(int objectPointer);
	std::string className(int classPointer);
#endif

};

