#pragma once
/*
* Huffman.h
* Written by kiminouso, 2023/05/03
*/
#pragma once
#ifndef Huffman_h__
#define Huffman_h__
#include "typedef.h"
#include "Heap.h"
#include "BitString.h"
#include <queue>

class ByteHuffman
{
public:
	struct Table {
		WORD val;
		BitString bits;
	};
private:

	struct HuffmanNode
	{
		size_t freq;
		size_t nodeCnt;
		WORD val;
		HuffmanNode* pLeft;
		HuffmanNode* pRight;
		HuffmanNode(size_t freq, const WORD& val)
			:freq(freq), nodeCnt(1), val(val), pLeft(nullptr), pRight(nullptr) {}
		HuffmanNode(size_t freq, HuffmanNode* pLeft, HuffmanNode* pRight)
			:freq(freq), nodeCnt(pLeft->nodeCnt + pRight->nodeCnt), pLeft(pLeft), pRight(pRight) {}
		~HuffmanNode() {
			delete pLeft;
			delete pRight;
		}
	};

	struct CmpFunc_t
	{
		bool operator()(const HuffmanNode* lft, const HuffmanNode* rgt) const {
			if (lft->freq > rgt->freq) {
				return true;
			} else if (lft->freq == rgt->freq) {
				return lft->val < rgt->val;
			} else {
				return false;
			}
		}
	};
	Heap<HuffmanNode*, CmpFunc_t, std::vector<HuffmanNode*>> _freqHeap;
	size_t _freqMap[256];
	int _count;
	void _bfs(Table* pTableBuf,  int& count) {
		std::queue<std::pair<HuffmanNode*, BitString>> nodeQueue;
		HuffmanNode* node;
		nodeQueue.push({ _freqHeap.top(),BitString()});
		while (!nodeQueue.empty()) {
			node = nodeQueue.front().first;
			BitString bitString = nodeQueue.front().second;
			nodeQueue.pop();
			bool isLeaf = true;
			if (node->pLeft != nullptr) {
				isLeaf = false;
				bitString.push_back(0);
				nodeQueue.push({ node->pLeft, bitString });
				bitString.pop_back();
			}
			if (node->pRight != nullptr) {
				isLeaf = false;
				bitString.push_back(1);
				nodeQueue.push({ node->pRight,bitString });
				bitString.pop_back();
			}
			if (isLeaf) {
				pTableBuf[count].bits = bitString;
				pTableBuf[count].val = node->val;
				count++;
			}
		}
	}
public:
	ByteHuffman() : _count(0){
		memset(&_freqMap, 0, sizeof(_freqMap));
	}
	~ByteHuffman() {
		delete _freqHeap.top();
	}
	void setFrequencyMap(const size_t (*freqMap)[256]) {
		memcpy(_freqMap, freqMap, sizeof(_freqMap));
	}
	void buildTree() {
		_freqHeap.clear();
		for (int i = 0; i < 256;++i) {
			if (_freqMap[i] > 0) {
				_freqHeap.push(new HuffmanNode(_freqMap[i], i));
			}
		}
		//补一个特殊的节点,以便后续删掉哈夫曼编码为全1的符号
		_freqHeap.push(new HuffmanNode(1, 256));
		_count = _freqHeap.size();
		while (_freqHeap.size() != 1) {
			HuffmanNode* left = _freqHeap.top();
			_freqHeap.pop();
			HuffmanNode* right = _freqHeap.top();
			_freqHeap.pop();
			_freqHeap.push(new HuffmanNode(left->freq + right->freq, left, right));
		}
	}
	void getTable(Table* tableBuf,int& count) {
		count = 0;
		HuffmanNode* tree = _freqHeap.top();
		BitString nodeBits;
		_bfs( tableBuf,count);
	}

	void getCanonicalTable(Table* tableBuf,int& count) {
		getTable(tableBuf,count);
		tableBuf[0].bits.setAll0();
		for (int i = 1; i < count; ++i) {
			int lengthDiff = tableBuf[i].bits.length() - tableBuf[i - 1].bits.length();
			switch (lengthDiff) {
			case 0:
				tableBuf[i].bits = tableBuf[i - 1].bits + 1;
				break;
			default:
				tableBuf[i].bits = tableBuf[i - 1].bits + 1;
				for (int j = 0; j < lengthDiff; ++j) {
					tableBuf[i].bits.push_back(0);
				}
				break;
			}
		}
	}
};
#endif // Huffman_h__
