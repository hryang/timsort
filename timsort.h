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
#include <cassert>

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

public:
    template <typename RandomAccessIterator>
    static void Sort(RandomAccessIterator first, RandomAccessIterator last);

    template <typename RandomAccessIterator, typename Compare>
    static void Sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp);

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
        // Maintain a stack for merging
        size_t mNumRunInStack;
        Run<RandomAccessIterator> mStack[TimSortImpl::kMaxMergeStackSize];

        MergeState() : mNumRunInStack(0) {};
    };

private:
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
        RandomAccessIterator k;

        for (k = i - 1; k >= j; --k) {
            *(k + 1) = *k;
        }

        *(k + 1) = value;
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

    std::vector<typename RandomAccessIterator::value_type> tmpStorage;
    tmpStorage.resize(state.mStack[stackPos].GetLength() + state.mStack[stackPos + 1].GetLength());
    std::merge(
        state.mStack[stackPos].first,     state.mStack[stackPos].last,
        state.mStack[stackPos + 1].first, state.mStack[stackPos + 1].last,
        tmpStorage.begin(), comp);
    std::copy(tmpStorage.begin(), tmpStorage.end(), state.mStack[stackPos].first);

    // Adjust the stack entries
    state.mStack[stackPos].last = state.mStack[stackPos + 1].last;
    // If we merge A and B, then we need adjust C to B's position.
    if (stackPos == state.mNumRunInStack - 3) {
        state.mStack[stackPos + 1] = state.mStack[stackPos + 2];
    }
    --state.mNumRunInStack;
}

template <typename RandomAccessIterator, typename Compare>
void TimSortImpl::Sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    assert(first <= last);

    size_t numElems = distance(first, last);
    size_t minRunLength = CalcMinRunLength(numElems);
    MergeState<RandomAccessIterator> mergeState;

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
