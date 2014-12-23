#ifndef _TEST_APP
#define _TEST_APP
#include "ofMain.h"

extern "C"
{
	#include "sndfile.h"
	#include <stdlib.h>	
	#include <stdio.h>
}

#include "ReadSound.h"
#include "MyFFT.h"


#define BUFFER_SIZE 1024  //SIZE OF THE FFT



class testApp : public ofSimpleApp{
	
	public:
		/* These are openFrameworks virtual functions to be overriden, they're all called inside the event loop.
		 * more detailed info on how they work can be found here: http://www.openframeworks.cc/tutorials/developers/001_how_openFrameworks_works.html
		*/
		void setup();
		void update();
		void draw();
		void keyPressed(int key);

		/* In order to play sound, frequency data must be feed to audioOut function.
		 * There are other alternatives to play a sound but feeding the frequency data is much more responsive & efficient than using a built-in media player (openframeworks has one which is based on OpenAL but very inefficient and uses a lot of memory)
		*/
		void audioOut(float *output, int bufferSize, int nChannels);

		/* Uses microphone to capture sound data */
		void audioIn(float *input, int bufferSize, int nChannels);



		void drawAudioSpectrumMAG();
		void drawAudioSpectrumDB();
		void drawOcilloscope();

	private:
		void setupFFTForSoundFile();
		void setupFFTForMic();


		inline void drawBar(int &x, int &y, int &thickness, float &height) const;



		//stores current window height
		int m_width;
		int m_height;

		string m_filePath; // path to the sound file.
		bool m_isPressed; //check if a key has been pressed.
		bool m_isFullScreen;

		int m_spectogramHeight;
		int m_spectogramWidth;
		int m_spectogramXBoundary; //X boundary for the spectogram starts from boundary to the spectogram height and width.
		int m_spectogramYBoundary;




		MyFFT *testFFT;
		ReadSound *m_sound; //a class which reads sound from a given file.

		float *m_left;
		float *m_right;

		//holds the average sample data for left and right channel.
		float *m_channelAvg;

		long m_pos; //position of the sound, in given m_left and m_right array

		string m_duration;
		int m_currentSec;


		long m_channelSize; //size of each sound channel array.

		int m_sampleRate; //sample rate of the given sound.
		int m_frequencySpacing; //m_sampleRate / BUFFER_SIZE

		int m_windowingFunc; // selects which windowing function to be applied.
		string m_windowName;
		bool m_isMoreOptions; //display box which shows detaild options for selecting windowing functions

		//current left & right sample data, size of the array is the BUFFER_SIZE;
		float *m_currentLeft;
		float *m_currentRight;

		//arrays which will be used to retrive the data from MyFFT class
		//currently only magnitude and decibels has their own spectrum graphs
		float *m_magnitude;
		float *m_decibels;
		float *m_phase;
		float *m_power;
};

#endif	

