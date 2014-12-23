/* ReadSound Class, uses sndlibfile library to read supported audio formats (FLAC/WAV) 
 * Using sndlibfile alone is inefficient since it doesn't offer multi-thread support. 
 * This class uses threads to minimize memory consumption to fixed size (it's 5MB by default) rather than loading the whole file.
 * While main thread is feeding the data to the sound card, second thread loads the next cycle.
 * When the next cycle is needed, pointer swap will suffice.
*/

#pragma once

#include "ofMain.h"

extern "C"
{
	#include "sndfile.h"
}

#include <iostream>
#include <stdlib.h>	
#include <stdio.h>

using namespace std;

class ReadSound : public ofThread
{
public:
	ReadSound(){}
	
	ReadSound(const char* fileName);

	float* getLeft() const;
	float* getRight() const;
	long getChannelSize() const; // size of both m_left and m_right array.
	int getSampleRate() const;
	bool isFinished() const;

	int getDuration() const; //duration of the sound file, m_frames / m_sampleRate but in seconds

	bool needThread() const; //return if multithreading is needed.


	bool readSamples(); //after the construction, use this function to load more data.

	void start();
	void stop();

	void threadedFunction();
	void initChannels();
	

private:
	long m_currentFrame; //program loaded sound until currentFrame
	long m_frames; //frames in given file.
	long m_framesLeft; //frames left to process. this keeps track of the position of the music file.
	int m_sampleRate; //sample rate of the given file.
	
	long m_totalArrSize; //size of an array if all the sample data were to be loaded. (channels * frames)

	/* After extensive testing on my laptop, desktop I found out that 5mb, (2.5mb each channel) is a very good size for a temp array,
	 * if its less than this it refreshes too often and makes craking noises and if its larger than this it creates high latency which causes underflow.
	*/
	long m_5MB; //5mb arr size, just for model, real size of the array might be less than this, but never more than this.

	long m_channelSize; //actual array size for each channel, either (m_5MB / 2) or less.


	float *m_left; //left channel, holds sample data for the left channel
	float *m_right; //holds sample data for the right channel.

	SNDFILE *infile; //SNDFILE object, holds the file.
	SF_INFO sfinfo; //SF_INFO object, holds information about the sound file


	void ripChannels(float *samplesBoth);

	//Don't forget to clean up the memory!
	float *m_tempLeft;
	float *m_tempRight;
	void ripChannelsTEMP(float *samplesBoth);
	bool m_isFinished;

	bool m_needThread;


};

