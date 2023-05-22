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
	
private:

	struct TreeNode
	{
		size_t freq;
		size_t nodeCnt;
		DWORD val;
		TreeNode* pLeft;
		TreeNode* pRight;
		TreeNode(size_t freq, const DWORD& val);
		TreeNode(size_t freq, TreeNode* pLeft, TreeNode* pRight);
		~TreeNode();
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

	void _bfs_getLengthCountOnly(std::vector<int>& lengthCountTable);

	void _bfs(std::vector<TableEntry>& table);

	void _limitCodeLength(std::vector<int>& lengthCountTable, int limitLength);
public:
	IntHuffman();

	~IntHuffman();

	void setFrequencyMap(const size_t* freqMap, const int count);

	void buildTree();

	void getTable(std::vector<TableEntry>& table);


	//���ݷ�ʽ���������ű�canonicalTable��ȡ��ʽ�����������bitStringTable
	//���ص�bitStringTable��, Ԫ��bitStringTable[i][j]ǡ��Ϊ����canonicalTable[i][j]��Ӧ�ı���
	static void getCanonicalCodes(const HuffmanTable& canonicalTable, std::vector<std::vector<BitString>>& bitStringTable);

	//getCanonicalTable:���ݱ��볤�ȼ�������lengthCountTable��ȡ��ʽ���������ű�canonicalTable
	//��Ľṹ:��ά����T
	//T[i]: һά����,�����д洢��Ӧ���볤��Ϊi�ķ���
	//T[i][j]: ����,��ʾһ������
 	void getCanonicalTable(std::vector<int>& lengthCountTable, HuffmanTable& canonicalTable);

	void getCanonicalCodes(const std::vector<int>& canonicalTable, std::vector<BitString>& bitStrings);

	void getLengthCountTable(std::vector<int>& table,int lengthLimit);

};
#endif // Huffman_h__
