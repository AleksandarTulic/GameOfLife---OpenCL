#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

using std::cout;
using std::cin;
using std::endl;
using std::vector;
using std::pair;

	//parametri koji su globalni
	int* mat = nullptr;
	int width = -1;
	int height = -1;
	int oneRow = -1;
	int howMuch = 30;
	int imageSize = -1;

	int* compressImage(const unsigned char* a, const int& width, const int& height);
	unsigned char* decompressImage(const int* a, int width, int height);

	char* readKernelSource(const char* filename);
	static void readImage(const char* filename, unsigned char*& array, int& width, int& height);
	static void writeImage(const char* filename, const unsigned char* array, const int width, const int height);

	void GameOfLife(int n);
	void GameOfLife1(int n);
	void initMyStates(vector <pair<int, int> > arr);
	void copySubSegmentOfBoard(int x1, int y1, int x2, int y2);
	void initCopyStates(const std::string path, int x1, int y1, int x2, int y2);

	int getMaxWorkGroupSize(cl_device_id idDevice);
	void printAllInformation();

/*=====================================================*/
/*=====================================================*/
/*Ime i prezime: Aleksandar Tulic =====================*/
/*Indeks	   : 1179/18 ==============================*/
/*=====================================================*/
/*=====================================================*/