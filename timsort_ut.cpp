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


#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include "timsort.h"

using namespace std;

template <typename T>
string ToString(T t)
{
    stringstream s;
    s << t;
    return s.str();
}

template <typename T>
void PrintVector(const vector<T> &v)
{
    for (size_t i = 0; i < v.size(); ++i) {
        cout << v[i] << endl;
    }
}

struct TestState
{
    bool mIsFail;
    string mMsg;

    TestState() : mIsFail(false) {}
};


struct TimSortUT
{
    static TestState TestInsertionSort();
    static TestState TestCalcMinRunLength();
    static TestState TestGallop();
    static TestState TestMerge(bool isTestMergeLow);
    static TestState TestTryMerge();
    static TestState TestTimSort();
    
    // Helper function
    static size_t CalcMinRunLength(size_t aSize);
};

TestState TimSortUT::TestInsertionSort()
{
    const size_t kNumElems = 100000;
    TestState state;
    state.mMsg = "TestInsertionSort\t PASS";

    vector<int> v;
    v.reserve(kNumElems);
    for (size_t i = 0; i < kNumElems; ++i) {
        v.push_back(rand());
    }

    TimSortImpl::BinaryInsertionSort(v.begin(), v.end(), less<int>());

    vector<int>::iterator i = v.begin();
    vector<int>::iterator j = i + 1;
    while (j != v.end()) {
        if (*i > *j) {
            state.mIsFail = true;
            state.mMsg = "TestInsertionSortSimple\t FAIL!";
            break;
        }
        ++i;
        ++j;
    }

    return state;
}

// The widely-used way to get the min run length
size_t TimSortUT::CalcMinRunLength(size_t aSize)
{
    size_t sBumper = 0;
    size_t sMinRun;

    assert(aSize >= 0);

    sMinRun = aSize;

    while (sMinRun >= TimSortImpl::kMaxMinRunLength) {
        sBumper |= (sMinRun & 1);
        sMinRun >>= 1;
    }

    return sMinRun + sBumper;
}

TestState TimSortUT::TestCalcMinRunLength()
{
    TestState state;
    state.mMsg = "TestCalcMinRunLength\t PASS";
    size_t result = 0;
    size_t gold = 0;
    for (size_t i = 0; i < 10000000; ++i) {
        size_t n = (rand() % (RAND_MAX - 1)) + 1;
        result = TimSortImpl::CalcMinRunLength(n);
        gold = CalcMinRunLength(n);
        if (result != gold) {
            state.mIsFail = true;
            state.mMsg = 
                "TestCalcMinRunLength\t Fail! \nInput n: " + ToString(n) + 
                ", gold: " + ToString(gold) + ", result: " + ToString(result);
        }
    }

    result = TimSortImpl::CalcMinRunLength(1984);
    gold = CalcMinRunLength(1984);
    if (result != gold) {
        state.mIsFail = true;
        state.mMsg = 
            "TestCalcMinRunLength\t FAIL! \nInput n: 1984, gold" +
            ToString(gold) + ", result: " + ToString(result);
    }

    result = TimSortImpl::CalcMinRunLength(TimSortImpl::kMaxMinRunLength);
    gold = CalcMinRunLength(TimSortImpl::kMaxMinRunLength);
    if (result != gold) {
        state.mIsFail = true;
        state.mMsg = 
            "TestCalcMinRunLength\t FAIL! \nInput n: " + ToString(TimSortImpl::kMaxMinRunLength) +
            ", gold: " + ToString(gold) + ", result: " + ToString(result);
    }

    return state;
}

TestState TimSortUT::TestGallop()
{
    const size_t kNumElems = 1000000;
    // The value should be far less than kNumElems to produce enough equal elements
    const size_t valueSpace = 1000;
    TestState state;
    state.mMsg = "TestGallop\t PASS!";

    // prepare data
    vector<int> v;
    v.reserve(kNumElems);
    for (size_t i = 0; i < kNumElems; ++i) {
        v.push_back(rand() % valueSpace); 
    }

    sort(v.begin(), v.end());

    vector<int>::iterator result;
    vector<int>::iterator gold;
    vector<int>::iterator hint;
    int key;
    for (size_t i = 0; i < 100000; ++i) {
        key = rand() % valueSpace;
        hint = v.begin() + rand() % v.size();
        result = TimSortImpl::GallopLeft(v.begin(), v.end(), hint, key, less<int>()); 
        gold = lower_bound(v.begin(), v.end(), key);
        if (result != gold) {
            state.mIsFail = true;
            state.mMsg = "Test GallopLeft\t FAIL!";
            return state;
        }

        result = TimSortImpl::GallopRight(v.begin(), v.end(), hint, key, less<int>()); 
        gold = upper_bound(v.begin(), v.end(), key);
        if (result != gold) {
            state.mIsFail = true;
            state.mMsg = "Test GallopRight\t FAIL!";
            return state;
        }
    }

    // test boundary
    v.clear();
    v.push_back(1);
    v.push_back(2);
    v.push_back(2);
    v.push_back(10);
    hint = v.begin();
    for (key = 0; key < 3; ++key) {
        result = TimSortImpl::GallopLeft(v.begin(), v.end(), hint, key, less<int>()); 
        gold = lower_bound(v.begin(), v.end(), key);
        if (result != gold) {
            state.mIsFail = true;
            state.mMsg = "Test GallopLeft\t FAIL!";
            return state;
        }
    }

    for (key = 0; key < 3; ++key) {
        result = TimSortImpl::GallopRight(v.begin(), v.end(), hint, key, less<int>()); 
        gold = upper_bound(v.begin(), v.end(), key);
        if (result != gold) {
            state.mIsFail = true;
            state.mMsg = "Test GallopRight\t FAIL!";
            return state;
        }
    }

    return state;
}

TestState TimSortUT::TestMerge(bool isTestMergeLow)
{
    const size_t kNumElems = 1000000;
    TestState state;
    string testName = isTestMergeLow ? "TestMergeLow" : "TestMergeHigh";
    state.mMsg = testName + "\t PASS!";

    // prepare data
    // The vector divide two parts: A and B.
    vector<int> v;
    v.reserve(kNumElems);
    for (size_t i = 0; i < kNumElems; ++i) {
        v.push_back(rand());
    }
    size_t pivot;
    do {
        pivot = rand() % v.size() / 2;
    } while (pivot == 0);

    if (isTestMergeLow == false) {
        // Make pivot > v.size()/2;
        pivot = v.size() - pivot;
    }


    vector<int>::iterator minA;
    vector<int>::iterator minB;
    vector<int>::iterator maxA;
    vector<int>::iterator maxB;
    do {
        minA = min_element(v.begin(), v.begin() + pivot);
        minB = min_element(v.begin() + pivot, v.end());
        maxA = max_element(v.begin(), v.begin() + pivot);
        maxB = max_element(v.begin() + pivot, v.end());
        if (*minA < *minB) swap(*minA, *minB);
        else if (*minA == *minB) ++(*minA);

        if (*maxA < *maxB) swap(*maxA, *maxB);
        else if (*maxA == *maxB) ++(*maxA);
    } while (*minA <= *minB || *maxA <= *maxB);

    sort(v.begin(), v.begin() + pivot);
    sort(v.begin() + pivot, v.end());

    //cout << "pivot: " << pivot << endl;
    //PrintVector(v);

    TimSortImpl::MergeState<vector<int>::iterator> mergeState(kNumElems);
    if (isTestMergeLow) {
        TimSortImpl::MergeLow(mergeState, v.begin(), v.begin() + pivot, v.begin() + pivot, v.end(), less<int>());
    } else {
        TimSortImpl::MergeHigh(mergeState, v.begin(), v.begin() + pivot, v.begin() + pivot, v.end(), less<int>());
    }

    vector<int>::iterator i = v.begin();
    vector<int>::iterator j = i + 1;
    while (j != v.end()) {
        if (*i > *j) {
            state.mIsFail = true;
            state.mMsg = testName + "\t FAIL!";
            break;
        }
        ++i;
        ++j;
    }

    //cout << "--------------------------" << endl;
    //PrintVector(v);

    return state;
}

TestState TimSortUT::TestTryMerge()
{
    const size_t kNumElems = 1000000;
    TestState state;
    string testName = "TestTryMerge";
    state.mMsg = testName + "\t PASS!";

    // prepare data
    vector<int> v;
    v.reserve(kNumElems);
    TimSortImpl::MergeState<vector<int>::iterator> mergeState(kNumElems);
    vector<int>::iterator start = v.begin();
    size_t step = 0;
    size_t totalElems = 0;
    while (totalElems < kNumElems) {
        step = rand() % kNumElems;
        if (totalElems + step > kNumElems) {
            step = kNumElems - totalElems;
        }
        for (size_t i = 0; i < step; ++i) {
            v.push_back(rand());
        }
        sort(start, start + step);
        TimSortImpl::Run<vector<int>::iterator> run;
        run.first = start;
        run.last = start + step;
        mergeState.mStack[mergeState.mNumRunInStack++] = run;

        start = start + step;
        totalElems += step;
    }
    TimSortImpl::TryMerge(mergeState, less<int>());

    vector<int>::iterator i = v.begin();
    vector<int>::iterator j = i + 1;
    while (j != v.end()) {
        if (*i > *j) {
            state.mIsFail = true;
            state.mMsg = testName + "\t FAIL!";
            break;
        }
        ++i;
        ++j;
    }

    //cout << "--------------------------" << endl;
    //PrintVector(v);

    return state;
}

TestState TimSortUT::TestTimSort()
{
    const size_t kNumElems = 1000000;
    TestState state;
    state.mMsg = "TestTimSort\t PASS!";

    vector<int> v;
    v.reserve(kNumElems);
    for (size_t i = 0; i < kNumElems; ++i) {
        v.push_back(rand());
    }

    //sort(v.begin(), v.end());
    TimSort(v.begin(), v.end());

    for (size_t i = 0, j = 1; j < v.size(); ++i, ++j) {
        if (v[i] > v[j]) {
            state.mIsFail = true;
            state.mMsg = 
                "TestTimSort FAIL! <" + ToString(i) + ", " + ToString(j) + "> = <" +
                ToString(v[i]) + ", " + ToString(v[j]) + ">";
            break;
        }
    }

    return state;
}

void PrintFailureMsg(const TestState &state)
{
    if (state.mMsg.empty() == false) {
        cerr << state.mMsg << endl;
    }
}

int main()
{
    srand(time(NULL));
    TestState state;

    //state = TimSortUT::TestInsertionSort();
    PrintFailureMsg(state);

    //state = TimSortUT::TestCalcMinRunLength();
    PrintFailureMsg(state);

    //state = TimSortUT::TestGallop();
    PrintFailureMsg(state);

    //state = TimSortUT::TestMerge(true);
    PrintFailureMsg(state);

    //state = TimSortUT::TestMerge(false);
    PrintFailureMsg(state);

    //state = TimSortUT::TestTryMerge();
    PrintFailureMsg(state);

    state = TimSortUT::TestTimSort();
    PrintFailureMsg(state);

    return 0;
}
