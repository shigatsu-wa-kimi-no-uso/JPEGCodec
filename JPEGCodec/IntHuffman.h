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
#include <vector>

class IntHuffman
{
public:
	struct TableEntry {
		DWORD val;
		BitString bits;
	};
	struct CanonicalTableEntry {
		DWORD val;
		DWORD codeLength;
	};
private:

	struct TreeNode
	{
		size_t freq;
		size_t nodeCnt;
		DWORD val;
		TreeNode* pLeft;
		TreeNode* pRight;
		TreeNode(size_t freq, const DWORD& val)
			:freq(freq), nodeCnt(1), val(val), pLeft(nullptr), pRight(nullptr) {}
		TreeNode(size_t freq, TreeNode* pLeft, TreeNode* pRight)
			:freq(freq), nodeCnt(pLeft->nodeCnt + pRight->nodeCnt), pLeft(pLeft), pRight(pRight) {}
		~TreeNode() {
			delete pLeft;
			delete pRight;
		}
	};

	struct CmpFunc_t
	{
		bool operator()(const TreeNode* lft, const TreeNode* rgt) const {
			if (lft->freq > rgt->freq) {
				return true;
			} else if (lft->freq == rgt->freq) {
				return lft->val < rgt->val;
			} else {
				return false;
			}
		}
	};
	Heap<TreeNode*, CmpFunc_t, std::vector<TreeNode*>> _freqHeap;
	size_t _freqMap[256];
	int _count;
	void _bfs_getLengthCountOnly(std::vector<int>& lengthCountTable) {
		std::queue<std::pair<TreeNode*, DWORD>> nodeQueue;
		TreeNode* node;
		nodeQueue.push({ _freqHeap.top(),0 });
		while (!nodeQueue.empty()) {
			node = nodeQueue.front().first;
			DWORD codeLength = nodeQueue.front().second;
			nodeQueue.pop();
			bool isLeaf = true;
			if (node->pLeft != nullptr) {
				isLeaf = false;
				nodeQueue.push({ node->pLeft, codeLength + 1 });
			}
			if (node->pRight != nullptr) {
				isLeaf = false;
				nodeQueue.push({ node->pRight,codeLength + 1 });
			}
			if (isLeaf) {
				while (lengthCountTable.size() <= codeLength) {
					lengthCountTable.push_back(0);
				}
				lengthCountTable[codeLength]++;
			}
		}
	}
	void _bfs_getLengthOnly(std::vector<CanonicalTableEntry>& table) {
		std::queue<std::pair<TreeNode*, DWORD>> nodeQueue;
		TreeNode* node;
		nodeQueue.push({ _freqHeap.top(),0 });
		while (!nodeQueue.empty()) {
			node = nodeQueue.front().first;
			DWORD codeLength = nodeQueue.front().second;
			nodeQueue.pop();
			bool isLeaf = true;
			if (node->pLeft != nullptr) {
				isLeaf = false;
				nodeQueue.push({ node->pLeft, codeLength + 1 });
			}
			if (node->pRight != nullptr) {
				isLeaf = false;
				nodeQueue.push({ node->pRight,codeLength + 1 });
			}
			if (isLeaf) {
				table.push_back({ node->val,codeLength });
			}
		}
	}

	void _bfs(std::vector<TableEntry>& table) {
		std::queue<std::pair<TreeNode*, BitString>> nodeQueue;
		TreeNode* node;
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
				table.push_back({ node->val,bitString });
			}
		}
	}

public:
	IntHuffman() : _count(0){
		memset(&_freqMap, 0, sizeof(_freqMap));
	}
	~IntHuffman() {
		while (_freqHeap.size()!=0) {
			delete _freqHeap.top();
			_freqHeap.pop();
		}

	}
	void setFrequencyMap(const size_t (*freqMap)[256]) {
		memcpy(_freqMap, freqMap, sizeof(_freqMap));
	}
	void buildTree() {
		_freqHeap.clear();
		for (int i = 0; i < 256;++i) {
			if (_freqMap[i] > 0) {
				_freqHeap.push(new TreeNode(_freqMap[i], i));
			}
		}
		_count = _freqHeap.size();
		while (_freqHeap.size() != 1) {
			TreeNode* left = _freqHeap.top();
			_freqHeap.pop();
			TreeNode* right = _freqHeap.top();
			_freqHeap.pop();
			_freqHeap.push(new TreeNode(left->freq + right->freq, left, right));
		}
	}
	void getTable(std::vector<TableEntry>& table) {
		TreeNode* tree = _freqHeap.top();
		BitString nodeBits;
		_bfs( table);
	}

	void getCanonicalCodes(const std::vector<CanonicalTableEntry> &canonicalTable, std::vector<BitString> &bitStringBuf) {
		bitStringBuf.resize(canonicalTable.size());
		bitStringBuf[0].setLength(canonicalTable[0].codeLength);
		bitStringBuf[0].setAll0();
		int count = canonicalTable.size();
		for (int i = 1; i < count; ++i) {
			int lengthDiff = canonicalTable[i].codeLength - canonicalTable[i - 1].codeLength;
			switch (lengthDiff) {
			case 0:
				bitStringBuf[i] = bitStringBuf[i - 1] + 1;
				break;
			default:
				bitStringBuf[i] = bitStringBuf[i - 1] + 1;
				for (int j = 0; j < lengthDiff; ++j) {
					bitStringBuf[i].push_back(0);
				}
				break;
			}
		}
	}

	void getCanonicalCodes(const std::vector<int>& canonicalTable, std::vector<BitString>& bitStringBuf) {
		int count = canonicalTable.size();
		int i = 0;
		bitStringBuf.push_back(BitString());
		int thisLenCnt = 0;
		while (1) {
			int lastLen = i, lengthDiff;
			while (canonicalTable[i] - thisLenCnt == 0) {
				++i;
				if (i == count) {
					return;
				}
				thisLenCnt = 0;
			}
			lengthDiff = i - lastLen;
			const BitString& last = bitStringBuf.back();
			BitString curr = last;
			curr = last + 1;
			if (lengthDiff > 0) {
				for (int j = 0; j < lengthDiff; ++j) {
					curr.push_back(0);
				}
			}
			bitStringBuf.push_back(curr);
			++thisLenCnt;
		}
	}

	void getCanonicalTable(std::vector<CanonicalTableEntry>& table) {
		TreeNode* tree = _freqHeap.top();
		BitString nodeBits;
		_bfs_getLengthOnly(table);
	}

	void getCanonicalTable(std::vector<int>& table,int lengthLimit) {
		TreeNode* tree = _freqHeap.top();
		BitString nodeBits;
		_bfs_getLengthCountOnly(table);
		_limitCodeLength(table, lengthLimit);
	}

	void _limitCodeLength(std::vector<int>& lengthCountTable,int limitLength) {
		int tableSize = lengthCountTable.size();
		for (int i = tableSize - 1; i > limitLength;) {
			if (lengthCountTable[i] > 0) {
				int j = i - 2;
				while (lengthCountTable[j] == 0) {
					--j;
				}
				lengthCountTable[i] -= 2;
				lengthCountTable[i - 1] += 1;
				lengthCountTable[j] -= 1;
				lengthCountTable[j + 1] += 2;
			} else {
				--i;
			}
		}
	}
};
#endif // Huffman_h__
