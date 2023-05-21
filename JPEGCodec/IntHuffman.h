/*
* Huffman.h
* Written by kiminouso, 2023/05/03
*/
#pragma once
#ifndef Huffman_h__
#define Huffman_h__
#define _CRT_SECURE_NO_WARNINGS
#include <queue>
#include <vector>
#include "typedef.h"
#include "Heap.h"
#include "BitString.h"


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
			:freq(freq), nodeCnt(pLeft->nodeCnt + pRight->nodeCnt), pLeft(pLeft), pRight(pRight),val(-1) {}
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
	size_t* _freqMap;
	DWORD _count;
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
	IntHuffman() : _count(0),_freqMap(nullptr){
	}
	~IntHuffman() {
		delete[] _freqMap;
		while (_freqHeap.size()!=0) {
			delete _freqHeap.top();
			_freqHeap.pop();
		}
	}
	void setFrequencyMap(const size_t* freqMap, const int count) {
		_count = count;
		if (_freqMap != nullptr) {
			delete[] _freqMap;
		} else {
			_freqMap = new size_t[count];
		}
		memcpy(_freqMap, freqMap, count * sizeof(size_t));
	}
	void buildTree() {
		_freqHeap.clear();
		for (size_t i = 0; i < _count;++i) {
			if (_freqMap[i] > 0) {
				_freqHeap.push(new TreeNode(_freqMap[i], (DWORD)i));
			}
		}
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

	void getCanonicalCodes(const std::vector<CanonicalTableEntry> &canonicalTable, std::vector<BitString> &bitStrings) {
		bitStrings.resize(canonicalTable.size());
		bitStrings[0].setLength(canonicalTable[0].codeLength);
		bitStrings[0].setAll0();
		size_t count = canonicalTable.size();
		for (size_t i = 1; i < count; ++i) {
			int lengthDiff = canonicalTable[i].codeLength - canonicalTable[i - 1].codeLength;
			switch (lengthDiff) {
			case 0:
				bitStrings[i] = bitStrings[i - 1] + 1;
				break;
			default:
				bitStrings[i] = bitStrings[i - 1] + 1;
				for (int j = 0; j < lengthDiff; ++j) {
					bitStrings[i].push_back(0);
				}
				break;
			}
		}
	}

	//根据范式哈夫曼符号表canonicalTable获取范式哈夫曼编码表bitStringTable
	//返回的bitStringTable中, 元素bitStringTable[i][j]恰好为符号canonicalTable[i][j]对应的编码
	void getCanonicalCodes(const std::vector<std::vector<DWORD>>& canonicalTable, std::vector<std::vector<BitString>>& bitStringTable) {
		size_t count = canonicalTable.size();
		size_t i = 0;
		bitStringTable.resize(canonicalTable.size());
		bitStringTable[0].push_back(BitString());
		int pushedCnt = 0;
		while (1) {
			size_t lastLen = i;
			int lengthDiff;
			while (canonicalTable[i].size() - pushedCnt == 0) {
				++i;
				if (i == count) {
					return;
				}
				pushedCnt = 0;
			}
			lengthDiff = (int)(i - lastLen);
			const BitString& last = bitStringTable[lastLen].back();
			BitString curr = last;
			curr = last + 1;
			if (lengthDiff > 0) {
				for (int j = 0; j < lengthDiff; ++j) {
					curr.push_back(0);
				}
			}
			bitStringTable[i].push_back(curr);
			++pushedCnt;
		}
	}

	//getCanonicalTable:根据编码长度计数向量lengthCountTable获取范式哈夫曼符号表canonicalTable
	//表的结构:二维向量T
	//T[i]: 一维向量,向量中存储对应编码长度为i的符号
	//T[i][j]: 整型,表示一个符号
 	void getCanonicalTable(std::vector<int>& lengthCountTable, std::vector<std::vector<DWORD>>& canonicalTable) {
		struct Entry {
			size_t freq;
			DWORD symbol;
		};
		std::vector<Entry> rankedSymbols;
		for (DWORD i = 0; i < _count; ++i) {
			if (_freqMap[i] != 0) {
				rankedSymbols.push_back({ _freqMap[i],i });
			}
		}
		std::sort(rankedSymbols.begin(), rankedSymbols.end(), [](const Entry& lft, const Entry& rgt) {
			if (lft.freq > rgt.freq) {
				return true;
			} else if (lft.freq == rgt.freq) {
				return lft.symbol < rgt.symbol;
			} else {
				return false;
			}
		});
		size_t lengthCount = lengthCountTable.size();
		size_t pushedCount = 0;
		std::vector<Entry>::iterator rankedSymbolsIter = rankedSymbols.begin();
		canonicalTable.resize(lengthCount);
		for (size_t i = 0; i < lengthCount;) {
			if (lengthCountTable[i] - pushedCount == 0) {
				pushedCount = 0;
				++i;
			} else {
				canonicalTable[i].push_back(rankedSymbolsIter->symbol);
				++rankedSymbolsIter;
				++pushedCount;
			}
		}
	}

	void getCanonicalCodes(const std::vector<int>& canonicalTable, std::vector<BitString>& bitStrings) {
		size_t count = canonicalTable.size();
		size_t i = 0;
		bitStrings.push_back(BitString());
		size_t thisLenCnt = 0;
		while (1) {
			size_t lastLen = i;
			int lengthDiff;
			while (canonicalTable[i] - thisLenCnt == 0) {
				++i;
				if (i == count) {
					return;
				}
				thisLenCnt = 0;
			}
			lengthDiff = i - lastLen;
			const BitString& last = bitStrings.back();
			BitString curr = last;
			curr = last + 1;
			if (lengthDiff > 0) {
				for (int j = 0; j < lengthDiff; ++j) {
					curr.push_back(0);
				}
			}
			bitStrings.push_back(curr);
			++thisLenCnt;
		}
	}

	void getCanonicalTable(std::vector<CanonicalTableEntry>& table) {
		TreeNode* tree = _freqHeap.top();
		BitString nodeBits;
		_bfs_getLengthOnly(table);
	}

	void getLengthCountTable(std::vector<int>& table,int lengthLimit) {
		TreeNode* tree = _freqHeap.top();
		BitString nodeBits;
		_bfs_getLengthCountOnly(table);
		_limitCodeLength(table, lengthLimit);
	}

	void _limitCodeLength(std::vector<int>& lengthCountTable,int limitLength) {
		size_t tableSize = lengthCountTable.size();
		for (size_t i = tableSize - 1; i > limitLength;) {
			if (lengthCountTable[i] > 0) {
				size_t j = i - 2;
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
