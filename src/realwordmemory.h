//
//  realwordmemory.h
//  Smalltalk-80
//
//  Created by Dan Banay on 2/25/20.
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

// Segmented Memory Model as described in G&R pg. 656

#define SegmentCount  16
#define SegmentSize   65536 /* in words */

extern std::uint16_t real_memory[SegmentCount][SegmentSize];

#define segment_word(s, w)  real_memory[s][w]

#define segment_word_put(s, w, value) real_memory[s][w] = value

#define segment_word_byte(s,  w, byteNumber) ((std::uint8_t *) &real_memory[(s)][(w)])[(byteNumber)]

#define segment_word_byte_put(s, w, byteNumber, value)  ((std::uint8_t *) &real_memory[(s)][(w)])[(byteNumber)] = (value)

// The most significant bit in a word will be referred to with
// the index 0 and the least significant with the index 15. G&R 657

#define segment_word_bits_to(s, w, firstBitIndex, lastBitIndex) \
	(real_memory[s][w] >> (15 - lastBitIndex)) & ((1 << (lastBitIndex - firstBitIndex + 1)) - 1)

#define  segment_word_bits_to_put(s, w, firstBitIndex, lastBitIndex, value) \
	real_memory[s][w] = (real_memory[s][w] & ~(((1 << (lastBitIndex - firstBitIndex + 1)) - 1) << (15 - lastBitIndex))) | (value << (15 - lastBitIndex))
	