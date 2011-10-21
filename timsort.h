/**
 * Copyright 2011 Haoran Yang
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TIMSORT_H
#define TIMSORT_H

#include <functional>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdint.h>

// ==================
// Declaration
// ==================

template <typename RandomAccessIterator>
inline void TimSort(RandomAccessIterator first, RandomAccessIterator last);

template <typename RandomAccessIterator, typename Compare>
inline void TimSort(RandomAccessIterator first, RandomAccessIterator last, Compare compare);

// ==================
// Implementation
// ==================

class TimSortImpl
{
private:
    static const size_t kMaxMinRunLength = 32;  // The maximum minrun length

    // Based on the merging strategy, the run length in the stack is a fibonacci sequence.
    // Then 100 deep stack can cover a huge number of elements. That's enough.
    static const size_t kMaxMergeStackSize = 100;

    static const size_t kMinGallop = 7;

    // The initial size of merge area. This value can be changed for performance.
    static const size_t kInitMergeAreaSize = 256;

public:
    template <typename RandomAccessIterator>
    static void Sort(RandomAccessIterator first, RandomAccessIterator last);

    template <typename RandomAccessIterator, typename Compare>
    static void Sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp);

private:
    /**
     * The run is [first, last)
     */
    template <typename RandomAccessIterator>
    struct Run
    {
        RandomAccessIterator first;
        RandomAccessIterator last;

        inline size_t GetLength() const
        {
            int32_t length = std::distance(first, last);
            assert(length >= 0);
            return length;
        }
    };

    template <typename RandomAccessIterator>
    struct MergeState
    {
        size_t mArraySize;  // The input array size

        // Maintain a stack for merging
        size_t mNumRunInStack;
        Run<RandomAccessIterator> mStack[TimSortImpl::kMaxMergeStackSize];

        size_t mMinGallop;
        
        // The temporary area for merging two runs.
        std::vector<typename RandomAccessIterator::value_type> mMergeArea;

        MergeState(size_t arraySize)
            : mArraySize(arraySize), mNumRunInStack(0), mMinGallop(TimSortImpl::kMinGallop) 
        {
            mMergeArea.reserve(TimSortImpl::kInitMergeAreaSize);
        };

        // Increase the merge area size if necessary.
        // The size is growed as exponentially to amortize linear time complexity.
        inline void EnsureMergeAreaSize(uint32_t requiredSize)
        {
            if (mMergeArea.size() < requiredSize) {
                // Compute the smallest power of 2 > requiredSize for 32-bit number
                uint32_t newSize = requiredSize;
                newSize |= newSize >> 1;
                newSize |= newSize >> 2;
                newSize |= newSize >> 4;
                newSize |= newSize >> 8;
                newSize |= newSize >> 16;
                newSize++;

                // Unlucky, the input requiredSize is too large (>= 2^31)
                // we can not round it up to power of 2.
                if (newSize <= 0) {
                    newSize = requiredSize;
                } else {
                    // We need array_size/2 merging area at most.
                    newSize = std::min<uint32_t>(newSize, mArraySize >> 1);
                }
                
                mMergeArea.reserve(newSize);
            }
        }
    };

    template <typename RandomAccessIterator, typename Compare>
    static void BinaryInsertionSort(RandomAccessIterator first, RandomAccessIterator last, Compare comp);

    /**
     * Calculate the min run length. 
     * Trying to make n/minrun_size is exact the power of 2. If impossible, then close to, but strictly less than, an exact power of 2.
     * If n < MAX_MINRUN_SIZE, return n
     * else if n is an exact power of 2, return MAX_MINRUN_SIZE/2
     * else return an integer k, such that n/k is close to, but strictly less than, an exact power of 2.
     * @param n The array size.
     */
    static inline size_t CalcMinRunLength(size_t n);

    /**
     * Make the descending run be ascending. The run is [first, last)
     * REQUIRES: The input sequence must be descending.
     */
    template <typename RandomAccessIterator>
    static inline void ReverseRun(RandomAccessIterator first, RandomAccessIterator last);

    /**
     * Detect the run and make it ascending if necessary on the given range [first, last).
     * @return Return the right boundary of the run. i.e. The run is [first, return).
     */
    template <typename RandomAccessIterator, typename Compare>
    static RandomAccessIterator DetectRunAndMakeAscending(RandomAccessIterator first, RandomAccessIterator last, Compare comp);

    /**
     * Trying to merge the runs in the stack in a stable way.
     * Two invariants are keeped while merging. Assume A, B and C are the lengths of three rightmost not-ye merged runs:
     * 1. A > B + C
     * 2. B > C
     * Rule 1 implies that the run lengths in the stack grow as fast as the Fibonacci numbers. Then the stack depth is always less than
     * log_base_phi(N), where phi = (1 + sqrt(5)) / 2 ~= 1.618.
     *
     * When it's OK to merge, suppose we need merge A, B and C three runs. Then the rule is:
     * Choose the smaller runs between A and C to merge with B.
     * Note: we can NOT merge A and C firstly, then merge with B, since this breaks the stability of the sort.
     */
    template <typename RandomAccessIterator, typename Compare>
    static inline void TryMerge(MergeState<RandomAccessIterator> &state, Compare comp);

    template <typename RandomAccessIterator, typename Compare>
    static void ForceMerge(MergeState<RandomAccessIterator> &state, Compare comp);

    /**
     * Merge the two runs at the given position of the stack(mStack[stackPos] and mStack[stackPos + 1]).
     */
    template <typename RandomAccessIterator, typename Compare>
    static void MergeAt(MergeState<RandomAccessIterator> &state, size_t stackPos, Compare comp);

    /**
     * Merge two adjacent runs in place in stable way. 
     * This function is called only when:
     * 1. The size of range A [firstA, lastA) smaller than or equal to the size of range B [firstB, lastB) AND
     * 2. The first element of A is greater than first element of B AND
     * 3. The last element of A is greater than last element of B.
     */
    template <typename RandomAccessIterator, typename Compare>
    static void MergeLow(
            MergeState<RandomAccessIterator> &state, RandomAccessIterator firstA, RandomAccessIterator lastA,
            RandomAccessIterator firstB, RandomAccessIterator lastB, Compare comp);

    /**
     * Merge two adjacent runs in place in stable way. 
     * This function is called only when:
     * 1. The size of range A [firstA, lastA) larger than or equal to the size of range B [firstB, lastB) AND
     * 2. The first element of A is greater than first element of B AND
     * 3. The last element of A is greater than last element of B.
     */
    template <typename RandomAccessIterator, typename Compare>
    static void MergeHigh(
            MergeState<RandomAccessIterator> &state, RandomAccessIterator firstA, RandomAccessIterator lastA,
            RandomAccessIterator firstB, RandomAccessIterator lastB, Compare comp);

    /**
     * Returns an iterator pointing to the first element in the sorted range [first,last) which does not compare less than value.
     * The semantic of this function is the same as std::lower_bound().
     * @param hint The position where to begin the search. The closer hint is to the result, the faster this function will run.
     */
    template <typename RandomAccessIterator, typename Compare>
    static RandomAccessIterator GallopLeft(
            RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator hint,
            const typename RandomAccessIterator::value_type &value, Compare comp);

    /**
     * Returns an iterator pointing to the first element in the sorted range [first,last) which compares greater than value.
     * The semantic of this function is the same as std::upper_bound().
     * @param hint The position where to begin the search. The closer hint is to the result, the faster this function will run.
     */
    template <typename RandomAccessIterator, typename Compare>
    static RandomAccessIterator GallopRight(
            RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator hint,
            const typename RandomAccessIterator::value_type &value, Compare comp);

    // for unit test
    friend class TimSortUT;
};

template <typename RandomAccessIterator, typename Compare>
void TimSortImpl::BinaryInsertionSort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    assert(first < last);

    for (RandomAccessIterator i = first + 1; i < last; ++i) {
        // The sequence [first, i) is in order. Binary search the right position
        typename RandomAccessIterator::value_type value = *i;
        RandomAccessIterator j = std::upper_bound(first, i, value, comp);

        std::copy_backward(j, i, i + 1);
        *j = value;
    }
}

inline size_t TimSortImpl::CalcMinRunLength(size_t n)
{
    assert(n > 0);

    size_t bumper = 0;

    while (n >= kMaxMinRunLength) {
        bumper |= (n & 1);
        n >>= 1;
    }

    return n + bumper;
}

template <typename RandomAccessIterator>
inline void TimSortImpl::ReverseRun(RandomAccessIterator first, RandomAccessIterator last)
{
    --last;
    while (first < last)
    {
        std::swap(*first, *last);
        ++first;
        --last;
    }
}

template <typename RandomAccessIterator, typename Compare>
RandomAccessIterator TimSortImpl::DetectRunAndMakeAscending(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    // There is none or only one element in the given range, return
    RandomAccessIterator p = first;
    if (p >= last || ++p >= last) {
        return p;
    }

    bool isAscending = *(p - 1) <= *p;

    if (isAscending) {
        while (++p < last && comp(*p, *(p - 1)) == false) {};
    } else {
        while (++p < last && comp(*p, *(p - 1))) {};
        ReverseRun(first, p);
    }

    return p;
}

// Suppose A, B and C are the rightmost three runs in the stack.
// Starting merging if following conditions are broken:
// 1. A > B + C
// 2. B > C
template <typename RandomAccessIterator, typename Compare>
inline void TimSortImpl::TryMerge(MergeState<RandomAccessIterator> &state, Compare comp)
{
    while (state.mNumRunInStack > 1) {
        int32_t pos = state.mNumRunInStack - 2;
        if (pos > 0 && state.mStack[pos - 1].GetLength() <= state.mStack[pos].GetLength() + state.mStack[pos + 1].GetLength()) {
            // Choose the smaller one between A and C to merge with B.
            pos -= static_cast<size_t>(state.mStack[pos - 1].GetLength() < state.mStack[pos + 1].GetLength());
            MergeAt(state, pos, comp);
        } else if (state.mStack[pos].GetLength() <= state.mStack[pos].GetLength()) {
            MergeAt(state, pos, comp);
        } else {
            // All rules are obeyed, do not need merge.
            break;
        }
    }
}

template <typename RandomAccessIterator, typename Compare>
inline void TimSortImpl::ForceMerge(MergeState<RandomAccessIterator> &state, Compare comp)
{
    while (state.mNumRunInStack > 1) {
        int32_t pos = state.mNumRunInStack - 2;

        // Choose the smaller one between A and C to merge with B.
        size_t length0 = distance(state.mStack[pos - 1].first, state.mStack[pos - 1].last);
        size_t length1 = distance(state.mStack[pos + 1].first, state.mStack[pos + 1].last);
        pos -= static_cast<size_t>(pos > 0 && length0 < length1);

        MergeAt(state, pos, comp);
    }
}

template <typename RandomAccessIterator, typename Compare>
void TimSortImpl::MergeAt(MergeState<RandomAccessIterator> &state, size_t stackPos, Compare comp)
{
    assert(stackPos == state.mNumRunInStack - 2 || stackPos == state.mNumRunInStack - 3);

#if 0
    std::vector<typename RandomAccessIterator::value_type> tmpStorage;
    tmpStorage.resize(state.mStack[stackPos].GetLength() + state.mStack[stackPos + 1].GetLength());
    std::merge(
        state.mStack[stackPos].first,     state.mStack[stackPos].last,
        state.mStack[stackPos + 1].first, state.mStack[stackPos + 1].last,
        tmpStorage.begin(), comp);
    std::copy(tmpStorage.begin(), tmpStorage.end(), state.mStack[stackPos].first);
#endif

    RandomAccessIterator firstA = state.mStack[stackPos].first;
    RandomAccessIterator lastA = state.mStack[stackPos].last;
    RandomAccessIterator firstB = state.mStack[stackPos + 1].first;
    RandomAccessIterator lastB = state.mStack[stackPos + 1].last;
    size_t lengthA = 0;
    size_t lengthB = 0;

    // Adjust the stack entries
    state.mStack[stackPos].last = state.mStack[stackPos + 1].last;
    // If we merge the 3rd-last run and 2sec-last run, then adjust 1st-last run to the 2sec-last run's position.
    if (stackPos == state.mNumRunInStack - 3) {
        state.mStack[stackPos + 1] = state.mStack[stackPos + 2];
    }
    --state.mNumRunInStack;

    // Figure out where the first element of B goes in A.
    // The prior elements of A can be ignored since they are already in place.
    RandomAccessIterator pA = GallopRight(firstA, lastA, firstA, *firstB, comp);
    lengthA = distance(pA, lastA);
    if (lengthA == 0) {
        return;
    }

    // Figure out where the last element of A goes in B.
    // The subsequent elements of B can be ignored since they are already in place.
    RandomAccessIterator pB = GallopLeft(firstB, lastB, lastB - 1, *(lastA - 1), comp);
    lengthB = distance(firstB, pB);
    if (lengthB == 0) {
        return;
    }

    if (lengthA <= lengthB) {
        MergeLow(state, pA, lastA, firstB, pB, comp);
    } else {
        MergeHigh(state, pA, lastA, firstB, pB, comp);
    }
}

template <typename RandomAccessIterator, typename Compare>
void TimSortImpl::MergeLow(
        TimSortImpl::MergeState<RandomAccessIterator> &state, RandomAccessIterator firstA, RandomAccessIterator lastA,
        RandomAccessIterator firstB, RandomAccessIterator lastB, Compare comp)
{
    // The number of left elems have not been merged yet. Init to the input two arrays' size.
    typename RandomAccessIterator::difference_type lengthA = distance(firstA, lastA);
    typename RandomAccessIterator::difference_type lengthB = distance(firstB, lastB);

    assert(0 < lengthA && lengthA <= lengthB &&   // A must be smaller than B AND
           lastA == firstB &&                     // A and B are adjacent AND
           comp(*firstB, *firstA) &&              // first_elem_of_A > first_elem_of_B AND
           comp(*(lastB - 1), *(lastA - 1)));     // last_elem_of_A > last_elem_of_B

    state.EnsureMergeAreaSize(lengthA);

    // Copy the run A (the smaller one) into the temporary merge area
    std::copy(firstA, lastA, state.mMergeArea.begin());

    RandomAccessIterator cursorA = state.mMergeArea.begin();  // point to the first of A, now in the merge area
    lastA = cursorA + lengthA;                                // point to the last of A, now in the merge area
    RandomAccessIterator cursorB = firstB;                    // point to the first of B
    RandomAccessIterator cursorDest = firstA;                 // point to the dest area, i.e. the original A's position

    // Move first element of run B in the dest area since caller guarantee that A[0] > B[0]
    *cursorDest = *cursorB;
    ++cursorDest;
    ++cursorB;
    --lengthB;

    // Use local one for performance.
    size_t minGallop = state.mMinGallop;

    // Hanle degenerate case.
    // If now B is empty, this implies that A is also empty since lengthA <= lengthB
    if (lengthB == 0) {
        goto LABEL_COPY_MERGE_AREA_TO_DEST;
    }

    // If lengthA is 1, then move B to A and append A[0].
    if (lengthA == 1) {
        goto LABEL_COPY_B_TO_DEST_AND_APPEND_A;
    }

    while (1) {
        // Do the straitforward merging until one run wins consistently.
        uint32_t countA = 0;   // number of times run A won
        uint32_t countB = 0;   // number of times run B won

        // one-pair-at-a-time mode
        do {
            if (comp(*cursorB, *cursorA)) {     // Current elem of B is less than current elem of A
                *cursorDest = *cursorB;
                ++cursorDest;
                ++cursorB;
                --lengthB;
                countA = 0;
                ++countB;

                if (lengthB == 0) {
                    goto LABEL_COPY_MERGE_AREA_TO_DEST;
                }
            } else {                            // Current elem of A less than or equal to current elem of B
                *cursorDest = *cursorA;
                ++cursorDest;
                ++cursorA;
                --lengthA;
                ++countA;
                countB = 0;

                if (lengthA == 1) {
                    goto LABEL_COPY_B_TO_DEST_AND_APPEND_A;
                }
            }
        } while ((countA | countB) < minGallop);  // if countA > 0 then countB == 0, vice versa

        // Switch to the galloping mode and continue galloping until neither run appears to be winning consistently any more.
        RandomAccessIterator p;
        do {
            assert(lengthA > 1 && lengthB > 0);

            // Find the current element of B (pointed by cursorB)'s position in range [cursorA, lastA) in gallop way.
            p = GallopRight(cursorA, lastA, cursorA, *cursorB, comp);
            countA = distance(cursorA, p);
            if (countA != 0) {
                std::copy(cursorA, p, cursorDest);
                cursorDest += countA;
                cursorA += countA;
                lengthA -= countA;

                if (lengthA == 0) {
                    // All B's elems must have been merged since the last element of A is greater than all elems of B.
                    assert(lengthB == 0);
                    return;
                }

                if (lengthA == 1) {
                    goto LABEL_COPY_B_TO_DEST_AND_APPEND_A;
                }
            }
            *cursorDest = *cursorB;
            ++cursorDest;
            ++cursorB;
            if (--lengthB == 0) {
                goto LABEL_COPY_MERGE_AREA_TO_DEST;
            }

            // Find the current element of A (pointed by cursor A)'s position in range [cursorB, lastB) in gallop way.
            p = GallopLeft(cursorB, lastB, cursorB, *cursorA, comp);
            countB = distance(cursorB, p);
            if (countB != 0) {
                std::copy(cursorB, p, cursorDest);
                cursorDest += countB;
                cursorB += countB;
                lengthB -= countB;

                if (lengthB == 0) {
                    assert(lengthA > 0);
                    goto LABEL_COPY_MERGE_AREA_TO_DEST;
                }
            }
            *cursorDest = *cursorA;
            ++cursorDest;
            ++cursorA;
            --lengthA;

            if (lengthA == 1) {
                goto LABEL_COPY_B_TO_DEST_AND_APPEND_A;
            }

            // Decrease the minGallop until it reaches 1.
            // The computation of minGallop is heuristic.
            // The longer we stay in galloping mode this time, the eariler we switch to the gallop mode next time.
            minGallop -= (minGallop > 1);
        } while (countA >= kMinGallop || countB >= kMinGallop);

        ++minGallop;  // penalize for leaving gallop mode.
    } // end of while (1)

    state.mMinGallop = minGallop;

LABEL_COPY_MERGE_AREA_TO_DEST:
    assert(lengthA > 0 && lengthB == 0);
    std::copy(cursorA, cursorA + lengthA, cursorDest);
    return;

LABEL_COPY_B_TO_DEST_AND_APPEND_A:
    assert(lengthA == 1 && lengthB > 0);
    std::copy(cursorB, cursorB + lengthB, cursorDest);
    cursorDest += lengthB;
    *cursorDest = *cursorA;
    return;
}

template <typename RandomAccessIterator, typename Compare>
void TimSortImpl::MergeHigh(
        TimSortImpl::MergeState<RandomAccessIterator> &state, RandomAccessIterator firstA, RandomAccessIterator lastA,
        RandomAccessIterator firstB, RandomAccessIterator lastB, Compare comp)
{
    // The number of left elems have not been merged yet. Init to the input two arrays' size.
    typename RandomAccessIterator::difference_type lengthA = distance(firstA, lastA);
    typename RandomAccessIterator::difference_type lengthB = distance(firstB, lastB);

    assert(0 < lengthB && lengthA >= lengthB &&   // A must be larger than B AND
           lastA == firstB &&                     // A and B are adjacent AND
           comp(*firstB, *firstA) &&              // first_elem_of_A > first_elem_of_B AND
           comp(*(lastB - 1), *(lastA - 1)));     // last_elem_of_A > last_elem_of_B

    state.EnsureMergeAreaSize(lengthB);

    // Copy run B to temprary array
    std::copy(firstB, lastB, state.mMergeArea.begin());

    // Merge the two arrays from RIGHT to LEFT
    //                 A                 original  B              merge area (now contains B)
    //   +----------------------------+---------------+               +-----------------+
    //    ^                          ^               ^                 ^               ^
    //  firstA                    cursorA        cursorDest          firsB           cursorB

    RandomAccessIterator cursorA = lastA - 1;
    firstB = state.mMergeArea.begin();
    RandomAccessIterator cursorB = state.mMergeArea.begin() + lengthB - 1;
    RandomAccessIterator cursorDest = lastB - 1;

    // Move the last element of A and deal with degenerate 
    *cursorDest = *cursorA;
    --cursorDest;
    --cursorA;
    --lengthA;

    // Use local one for performance.
    size_t minGallop = state.mMinGallop;

    if (lengthA == 0) {
        goto LABEL_COPY_MERGE_AREA_TO_DEST;
    }

    if (lengthB == 1) {
        goto LABEL_COPY_A_TO_DEST_AND_PREPEND_B;
    }

    while (1) {
        // Do the straitforward merging until one run wins consistently.
        size_t countA = 0;
        size_t countB = 0;

        // one-pair-a-time mode
        do {
            assert(lengthA > 0 || lengthB > 1);

            if (comp(*cursorB, *cursorA)) {
                *cursorDest = *cursorA;
                --cursorDest;
                --cursorA;
                --lengthA;
                ++countA;
                countB = 0;

                if (lengthA == 0) {
                    goto LABEL_COPY_MERGE_AREA_TO_DEST;
                }
            } else {
                *cursorDest = *cursorB;
                --cursorDest;
                --cursorB;
                --lengthB;
                countA = 0;
                ++countB;

                if (lengthB == 1) {
                    goto LABEL_COPY_A_TO_DEST_AND_PREPEND_B;
                }
            }
        } while ((countA | countB) < minGallop);

        // Switch to the galloping mode and continue galloping until neither run appears to be winning consistently any more.
        RandomAccessIterator p;
        do {
            assert(lengthA > 0 && lengthB > 1);

            // Find the current element of B (pointed by cursorB)'s position in range [firstA, cursorA + 1) in gallop way.
            p = GallopRight(firstA, cursorA + 1, cursorA, *cursorB, comp);
            countA = distance(p, cursorA + 1);
            if (countA != 0) {
                std::copy_backward(p, cursorA + 1, cursorDest + 1);
                cursorDest -= countA;
                cursorA -= countA;
                lengthA -= countA;

                if (lengthA == 0) {
                    // Some B's elements must be left since firstA > firstB.
                    assert(lengthB > 0);
                    goto LABEL_COPY_MERGE_AREA_TO_DEST;
                }
            }
            *cursorDest = *cursorB;
            --cursorDest;
            --cursorB;
            --lengthB;

            if (lengthB == 1) {
                goto LABEL_COPY_A_TO_DEST_AND_PREPEND_B;
            }

            // Find the current element of A (pointed by cursor A)'s position in range [cursorB, lastB) in gallop way.
            p = GallopLeft(firstB, cursorB + 1, cursorB, *cursorA, comp);
            countB = distance(p, cursorB + 1);
            if (countB != 0) {
                std::copy_backward(p, cursorB + 1, cursorDest + 1);
                cursorDest -= countB;
                cursorB -= countB;
                lengthB -= countB;

                if (lengthB == 0) {
                    // A must be empty since firstA > firstB
                    assert(lengthA == 0);
                    return;
                }
                if (lengthB == 1) {
                    goto LABEL_COPY_A_TO_DEST_AND_PREPEND_B;
                }
            }
            *cursorDest = *cursorA;
            --cursorDest;
            --cursorA;
            --lengthA;

            if (lengthA == 0) {
                goto LABEL_COPY_MERGE_AREA_TO_DEST;
            }

            // Decrease the minGallop until it reaches 1.
            // The computation of minGallop is heuristic.
            // The longer we stay in galloping mode this time, the eariler we switch to the gallop mode next time.
            minGallop -= (minGallop > 1);
        } while (countA >= kMinGallop || countB >= kMinGallop);

        ++minGallop;  // penalize for leaving gallop mode.
    } // end of while (1)

    state.mMinGallop = minGallop;

LABEL_COPY_MERGE_AREA_TO_DEST:
    assert(lengthA == 0 && lengthB > 0);
    std::copy_backward(firstB, cursorB + 1, cursorDest + 1);
    return;

LABEL_COPY_A_TO_DEST_AND_PREPEND_B:
    assert(lengthB == 1 && lengthA > 0);
    std::copy_backward(firstA, cursorA + 1, cursorDest + 1);
    cursorDest -= lengthA;
    *cursorDest = *cursorB;
    return;
}

template <typename RandomAccessIterator, typename Compare>
RandomAccessIterator TimSortImpl::GallopLeft(
        RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator hint,
        const typename RandomAccessIterator::value_type &value, Compare comp)
{
    assert(first <= hint && hint < last);

    RandomAccessIterator begin;
    RandomAccessIterator end;

    if (comp(*hint, value) == false) {  // value <= hint
        // Exponential seraching from right to left to find the range value belongs to, where *begin < value <= *end
        // The range legths grows as power of 2. i.e. The seraching ranges are: 
        // [hint-1, hint), [hint-3, hint-1), [hint-7, hint-3) ... [hint-2^k-1, hint-2^(k-1)-1).
        size_t k;
        for (k = 1, begin = hint - 1, end = hint;
             begin > first && comp(*begin, value) == false; ++k) {
            end = begin;
            // Note: 1LL << k may be overflow then become negative. 
            //       But before the overflow, the loop has been terminated by condition begin > first
            //       since it's impossible to have a so huge array in reality (The array size is at least greater than (1 << 62)).
            begin = begin - (1LL << k);
        }
        begin = begin >= first ? begin : first;
    } else {                            // value > hint
        // Exponential seraching from left to right to find the range value belongs to, where *begin < value <= *end
        // The range legths grows as power of 2. i.e. The seraching ranges are: 
        // [hint-1, hint), [hint-3, hint-1), [hint-7, hint-3) ... [hint-2^k-1, hint-2^(k-1)-1).
        size_t k;
        for (k = 1, begin = hint, end = hint + 1;
             end < last && comp(*end, value); ++k) {
            begin = end;
            // XXX Should we need care about the overflow of (end + huge_integer) for 32-bits OS?
            end = end + (1LL << k);
        }
        end = end <= last ? end : last;
    }
    assert(first <= begin && begin <= end && end <= last);

    // Now binary search the value in the range [begin, end)
    // XXX: It's possible that begin and end are equal. Then we expect upper_bound return the begin iterator.
    //      It's OK for gcc version stl.
    return lower_bound(begin, end, value, comp);
}

template <typename RandomAccessIterator, typename Compare>
RandomAccessIterator TimSortImpl::GallopRight(
        RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator hint,
        const typename RandomAccessIterator::value_type &value, Compare comp)
{
    assert(first <= hint && hint < last);

    RandomAccessIterator begin;
    RandomAccessIterator end;
    if (comp(value, *hint)) {  // value < hint
        // Exponential seraching from right to left to find the range value belongs to, where *begin <= value < *end
        // The range legths grows as power of 2. i.e. The seraching ranges are: 
        // [hint-1, hint), [hint-3, hint-1), [hint-7, hint-3) ... [hint-2^k-1, hint-2^(k-1)-1).
        size_t k;
        for (k = 1, begin = hint - 1, end = hint;
             begin > first && comp(value, *begin); ++k) {
            end = begin;
            // Note: 1LL << k may be overflow then become negative. 
            //       But before the overflow, the loop has been terminated by condition begin > first
            //       since it's impossible to have a so huge array in reality (The array size is at least greater than (1 << 62)).
            begin = begin - (1LL << k);
        }
        begin = begin >= first ? begin : first;
    } else {                   // value >= hint
        // Exponential seraching from left to right to find the range value belongs to, where *begin <= value < *end
        // The range legths grows as power of 2. i.e. The seraching ranges are: 
        // [hint, hint+1), [hint+1, hint+3), [hint+3, hint+7) ... [hint+2^(k-1)-1, hint+2^k-1).
        size_t k;
        for (k = 1, begin = hint, end = hint + 1;
             end < last && comp(value, *end) == false; ++k) {
            begin = end;
            // XXX Should we need care about the overflow of (end + huge_integer) for 32-bits OS?
            end = end + (1LL << k);
        }
        end = end <= last ? end : last;
    }
    assert(first <= begin && begin <= end && end <= last);

    // Now binary search the value in the range [begin, end)
    // XXX: It's possible that begin and end are equal. Then we expect upper_bound return the begin iterator.
    //      It's OK for gcc version stl.
    return upper_bound(begin, end, value, comp);
}

template <typename RandomAccessIterator, typename Compare>
void TimSortImpl::Sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    assert(first <= last);

    size_t numElems = distance(first, last);
    size_t minRunLength = CalcMinRunLength(numElems);
    MergeState<RandomAccessIterator> mergeState(numElems);

    Run<RandomAccessIterator> run;
    RandomAccessIterator next = first;

    while (next < last) {
        run.first = next;
        run.last = DetectRunAndMakeAscending(next, last, comp);

        size_t numRemainElems = distance(next, last);
        size_t realRunLength = run.GetLength();
        if (realRunLength < minRunLength && realRunLength < numRemainElems) {
            // OK, we need boost the run length and sort it by insertion sort.
            realRunLength = minRunLength < numRemainElems ? minRunLength : numRemainElems;
            run.last = run.first + realRunLength;
            assert(run.last <= last);
            BinaryInsertionSort(run.first, run.last, comp);
        }

        // Push the run to the stack
        assert(mergeState.mNumRunInStack < kMaxMinRunLength);
        mergeState.mStack[mergeState.mNumRunInStack++] = run;

        TryMerge(mergeState, comp);

        // move to the next range
        next = run.last;
    }

    // Force merging all runs left in the stack
    if (mergeState.mNumRunInStack != 0) {
        ForceMerge(mergeState, comp);
    }
}

template <typename RandomAccessIterator>
inline void TimSort(RandomAccessIterator first, RandomAccessIterator last)
{
    TimSort(first, last, std::less<typename RandomAccessIterator::value_type>());
}

template <typename RandomAccessIterator, typename Compare>
inline void TimSort(RandomAccessIterator first, RandomAccessIterator last, Compare compare)
{
    TimSortImpl ts;
    ts.Sort(first, last, compare);
}

#endif
