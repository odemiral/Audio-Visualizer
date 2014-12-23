/* Written By Onur Demiralay 
 * This class takes the FFT of a given file and contains methods that are related to to FFT.
 * Methods are explained in detail in .cpp file
*/
#pragma once

#include <math.h>
#include <vector>
#include <limits>


#include <iostream>
using namespace std;




class MyFFT
{

public:
	MyFFT(int FFTSize);
	~MyFFT(void);

	void powerSpectrum(int windowSize, float *data, int windowingFunc);

	float* getMagnitude() const;
	float* getPower() const;
	float* getPhase() const;
	float* getDecibels() const;

	float getMinDB() const;
	float getMaxDB() const;

	float getMinMAG() const;
	float getMaxMAG() const;
private:

	bool isPowerOfTwo(int num) const;
	int numOfRequiredBits(int size) const;
	int reverseBits(int bits, int bitSize) const;
	inline int fastRevBits(int bits) const;

	void createLookUpTable(int bitSize);
	void createTrigTables(int bitSize);

	//FFT functions
	void complexFFT(int numOfSamples);
	void realFFT(int numOfSamples);

	//this is the function which selects  which windowing function to be applied on data.
	void applySelectedWinFunction(int windowingFunc, int windowSize); 

	//windowing functions
	void applyHanningFunc(int windowSize);
	void applyBlackmanHarrisFunc(int windowSize);
	void applyHammingFunc(int windowSize);
	void applyFlatTopFunc(int windowSize);


	//size of the FFT must be 2^N (radix-2)
	int m_FFTSize;





	//using vectors
	vector<float> realOutput;
	vector<float> imagOutput;

	vector<float> realInput;
	vector<float> imagInput;

	//will be needed in realFFT
	vector<float> tempRealInput;
	vector<float> tempImagInput;

	//tables
	vector<int> reverseBitsTable;
	vector<float> cosTable;
	vector<float> sinTable;

	//information needed by testApp
	float* m_magnitude;
	float* m_power;
	float* m_phase;
	float* m_decibels;

	//values needed to normalize the spectrum.
	//currently they're not used. 
	float m_minDB;
	float m_maxDB;
	float m_minMAG;
	float m_maxMAG;


};

