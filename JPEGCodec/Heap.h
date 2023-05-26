/*
* Heap.h
* Written by kiminouso, 2023/04/03
*/
#pragma once
#ifndef Heap_h__
#define Heap_h__
#include <vector>

template<typename Elem_t, class Compare_Func_t, class Container_t = std::vector<typename Elem_t>>
class Heap
{
private:
	Compare_Func_t _cmpFunc;
	Container_t _heap;
	size_t _lastIndex() {
		return _heap.size() == 0 ? 0 : _heap.size() - 1;
	}
	size_t _topIndex() {
		return 0;
	}
	size_t _leftSon(size_t index) {
		return (index << 1) + 1;
	}
	size_t _rightSon(size_t index) {
		return (index << 1) + 2;
	}
	size_t _parent(size_t index) {
		return (index - 1) >> 1;
	}
	void _emerge(size_t index) {
		size_t currIndex = index;
		while (currIndex != _topIndex()) {
			size_t parentIndex = _parent(currIndex);
			Elem_t& curr = _heap[currIndex];
			Elem_t& par = _heap[parentIndex];
			if (_cmpFunc(par, curr)) {           //par > curr 
				std::swap(par, curr);
				currIndex = parentIndex;
			} else {
				break;
			}
		}
	}
	void _sink(size_t index) {
		size_t currIndex = index;
		while (currIndex != _lastIndex()) {
			size_t lsindex = _leftSon(currIndex);
			size_t rsindex = _rightSon(currIndex);
			size_t toSwapIndex = currIndex;

			//找左右儿子中更大/小的那个
			if (lsindex <= _lastIndex() && _cmpFunc(_heap[currIndex], _heap[lsindex])) {	  //判断是否有左儿子        
				toSwapIndex = lsindex;	//left_son < curr
			}

			if (rsindex <= _lastIndex() && _cmpFunc(_heap[toSwapIndex], _heap[rsindex])) {    //判断是否有右儿子
				toSwapIndex = rsindex;
			}

			if (toSwapIndex == currIndex) {
				break;
			} else {
				std::swap(_heap[currIndex], _heap[toSwapIndex]);
				currIndex = toSwapIndex;
			}
		}
	}


public:
	Heap() {}
	Heap(size_t initCapacity) {
		reserve(initCapacity);
	}

	void reserve(int newCapacity) {
		_heap.reserve(newCapacity);
	}

	size_t size() {
		return _heap.size();
	}
	size_t capacity() {
		return _heap.capacity();
	}
	const Elem_t& top() {
		return _heap[_topIndex()];
	}

	void push(const Elem_t& i) {
		_heap.push_back(i);
		_emerge(_lastIndex());
	}

	void pop() {
		std::swap(_heap[_topIndex()], _heap[_lastIndex()]);
		_heap.pop_back();
		_sink(_topIndex());
	}
	void clear() {
		_heap.clear();
	}
};
#endif // Heap_h__