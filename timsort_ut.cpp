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
    static TestState TestTimSort();
    
    // Helper function
    static size_t CalcMinRunLength(size_t aSize);
};

TestState TimSortUT::TestInsertionSort()
{
    const size_t kNumElems = 100000;
    TestState state;
    state.mMsg = "TestInsertionSort PASS";

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
            state.mMsg = "TestInsertionSortSimple FAIL!";
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
                "TestCalcMinRunLength Fail! Input n: " + ToString(n) + 
                ", gold: " + ToString(gold) + ", result: " + ToString(result);
        }
    }

    result = TimSortImpl::CalcMinRunLength(1984);
    gold = CalcMinRunLength(1984);
    if (result != gold) {
        state.mIsFail = true;
        state.mMsg = 
            "TestCalcMinRunLength FAIL! Input n: 1984, gold" +
            ToString(gold) + ", result: " + ToString(result);
    }

    result = TimSortImpl::CalcMinRunLength(TimSortImpl::kMaxMinRunLength);
    gold = CalcMinRunLength(TimSortImpl::kMaxMinRunLength);
    if (result != gold) {
        state.mIsFail = true;
        state.mMsg = 
            "TestCalcMinRunLength FAIL! Input n: " + ToString(TimSortImpl::kMaxMinRunLength) +
            ", gold: " + ToString(gold) + ", result: " + ToString(result);
    }

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

    /*
    for (size_t i = 0; i < v.size(); ++i) {
        cout << v[i] << endl;
    }
    */

    TimSort(v.begin(), v.end());

    for (size_t i = 0, j = 1; j < v.size(); ++i, ++j) {
        if (v[i] > v[j]) {
            state.mIsFail = true;
            state.mMsg = 
                "TestTimSortSimple FAIL! <" + ToString(i) + ", " + ToString(j) + "> = <" +
                ToString(v[i]) + ", " + ToString(v[j]) + ">";
            break;
        }
    }

    return state;
}

void PrintFailureMsg(const TestState &state)
{
    if (state.mIsFail) {
        cerr << state.mMsg << endl;
    };
}

int main()
{
    srand(time(NULL));
    TestState state;

    state = TimSortUT::TestInsertionSort();
    PrintFailureMsg(state);

    state = TimSortUT::TestCalcMinRunLength();
    PrintFailureMsg(state);

    state = TimSortUT::TestTimSort();
    PrintFailureMsg(state);

    return 0;
}
