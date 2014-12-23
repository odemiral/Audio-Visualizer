/* testApp class, the main class for openFrameWorks (OF)
   OpenGL is used to visualize the data, and RtAudio is used to playing the sound files.
*/

#include "testApp.h"


void testApp::setup(){

	//ofEnableSmoothing();

	m_isPressed = false; // no key has been pressed at this point.
	m_isFullScreen = false;
	m_isMoreOptions = false;
	m_height = ofGetWindowHeight();
	m_width = ofGetWindowWidth();
	m_spectogramXBoundary = 50;
	m_spectogramYBoundary = 200;

	ofBackground(222,223,223);


	m_windowingFunc = 1; //DEFAULT WINDOWING FUNCTION IS HANNING
	m_windowName = "Hanning";

}


/* setup values if user choses to use a microphone. */
void testApp::setupFFTForMic()
{
	m_channelAvg = new float[BUFFER_SIZE];
	m_currentLeft = new float[BUFFER_SIZE];
	m_currentRight = new float[BUFFER_SIZE];

	/*	
	0 output channels, 
	2 input channels
	44100 samples per second
	BUFFER_SIZE samples per buffer
	4 number of buffers (latency)
	*/
	ofSoundStreamSetup(0,2,this, 44100, BUFFER_SIZE, 4);

	testFFT = new MyFFT(BUFFER_SIZE);

	m_duration = ""; // this way when mic is used duration won't be displayed


}

/* setup values if user choses to use sound file  */
void testApp::setupFFTForSoundFile()
{
	m_sound = new ReadSound(m_filePath.c_str());
	//at this point m_left, m_right allocated in m_sound.
	m_left = m_sound->getLeft();
	m_right = m_sound->getRight();

	m_channelSize = m_sound->getChannelSize();
	cout << m_channelSize << endl;
	m_sampleRate = m_sound->getSampleRate();

	m_frequencySpacing = m_sampleRate / BUFFER_SIZE;
	cout << "sample Rate: " << m_sampleRate << endl;
	cout << "Frequency Spacing: " << m_frequencySpacing << " Hz" << endl;


	//calculate the duration of the sound file
	float mins = (float)m_sound->getDuration() / 60.0f;
	float secs = (mins - floor(mins)) * 60;
	m_duration = ofToString((int)mins) + ":" + ofToString(secs);
	m_currentSec = 0;

	m_pos = 0; //keeps track of the position of frequency data.

	//will hold average sample data value for left and right channel (by taking the mean avg)
	m_channelAvg = new float[BUFFER_SIZE];

	m_currentLeft = new float[BUFFER_SIZE];
	m_currentRight = new float[BUFFER_SIZE];

	testFFT = new MyFFT(BUFFER_SIZE);

	/*
	0 output channels, 
	2 input channels
	m_sampleRate samples per second
	BUFFER_SIZE samples per buffer
	4 number of buffers (latency)
	*/
	ofSoundStreamSetup(2,0,this, m_sampleRate,BUFFER_SIZE, 0); //44100

	//if needs threads which means there will be more data to be read start the threaded function
	//which reads the next cycle while processing the current one

	if(m_sound->needThread()) {
		m_sound->start(); //starts threaded function here.
	}
}

//--------------------------------------------------------------
//update the window positions and perform FFT on current frequency data.
void testApp::update(){
	m_width = ofGetWindowWidth();
	m_height = ofGetWindowHeight();

	m_spectogramWidth = m_width - m_spectogramXBoundary;
	m_spectogramHeight = m_height - m_spectogramYBoundary;

	if(m_isPressed == true) {
		//---------MY FFT-------------------
		//taking the mean for each channel then taking the FFT of the mean.
		for (int i = 0; i < BUFFER_SIZE; i++) {
			m_channelAvg[i] = (m_currentLeft[i] + m_currentRight[i]) / 2;
		}
		testFFT->powerSpectrum(BUFFER_SIZE,m_channelAvg,m_windowingFunc);
	}
}

//--------------------------------------------------------------
//Drawing Function of openFrameWorks (all the drawing must be done in here)

void testApp::draw(){


	if(!m_isPressed) {
		ofSetColor(165,42,42);
		ofDrawBitmapString("Press M to Use The Microphone", m_spectogramXBoundary - 10, m_height - 10);
		ofDrawBitmapString("Press F to Load a Sound File ", m_spectogramXBoundary + 250, m_height - 10);
		ofDrawBitmapString("Press ENTER to Enter Full Screen", m_spectogramXBoundary + 540, m_height - 10);


	} else {
		ofDrawBitmapString("Press ENTER to Enter Full Screen", m_spectogramXBoundary - 10, m_height - 10);
		ofDrawBitmapString("Current FPS: "  + ofToString(ofGetFrameRate()), m_spectogramWidth - 130, m_height - 10);
		if(m_duration != "") {
			float mins = (float)(m_currentSec / m_sampleRate) / 60.0f;
			int secs = (mins - floor(mins)) * 60;
			string duration = ofToString((int)mins) + ":" + ofToString(secs) + " / " + m_duration;
			ofDrawBitmapString("Position: " + duration, m_spectogramXBoundary + 270, m_height - 10);
		}

		ofDrawBitmapString("Press + To View More Options", m_spectogramXBoundary + 470, m_height - 10);

		ofDrawBitmapString("Current Window: " + m_windowName, m_spectogramXBoundary + 750, m_height - 10);

		//draw magnitude spectrum and ocilloscope.
		drawOcilloscope();
		drawAudioSpectrumMAG();

		//drawAudioSpectrumDB();
	}

	//show more options menu.
	if(m_isMoreOptions) {
		ofNoFill();
		int x = m_spectogramWidth  - 290;
		ofSetColor(165,42,42);
		ofRect(x , 20, 330, 150);
		ofDrawBitmapString("Press 0 to Apply No Window", x + 10, 40);
		ofDrawBitmapString("Press 1 to Apply Hanning Window", x + 10, 70);
		ofDrawBitmapString("Press 2 to Apply Blackman-Harris Window", x + 10, 100);
		ofDrawBitmapString("Press 3 to Apply Hamming Window", x + 10, 130);
		ofDrawBitmapString("Press 4 to Apply Flat Top Window", x + 10, 160);
	}

}



/*given thickness height and starting x,y coordinates, draws a bar(rectangle) */
inline void testApp::drawBar(int &x, int &y, int &thickness, float &height) const
{
	for(int i = 0; i < thickness; i++) {
		ofLine(x + i, y, x + i, y - height );
	}
}



/* Draws the audio spectrum magnitude/hz
 * it will display magnitude of the signal at the frequency of the given bin
 * magnitudes multiplied by 24 to have a nice graphical representation.
*/
void testApp::drawAudioSpectrumMAG()
{
	ofSetColor(165,42,42);

	m_magnitude = testFFT->getMagnitude();

	float minMAG =  testFFT->getMinMAG();
	float maxMAG = testFFT-> getMaxMAG();
	float range = abs(maxMAG) - abs(minMAG);
	float range2 = m_spectogramHeight - m_spectogramXBoundary;

	//cout << "minMAG: " << minMAG << " maxMAG: " << maxMAG << endl;
	//cout << "range: " << m_spectogramHeight << endl;

	//range min = m_spectogramXBoundary
	//range max = m_spectogramHeight;
	int x = m_spectogramXBoundary;
	//int y = m_spectogramHeight;
	int barDistance = 1;
	int quarter = BUFFER_SIZE /4;

	int thickness = (m_spectogramWidth - (quarter - 1) * barDistance) / quarter;

	if (thickness < 1) {
		thickness = 1;
	}

	for (int i = 1; i < quarter; i++) {
		float magnitude = m_magnitude[i] * 24.0f; // multiply by 20 to widen the range.
		//magnitude =  (magnitude - minMAG) / range;
		//magnitude =  (magnitude * range2) + m_spectogramXBoundary;
		drawBar(x,m_spectogramHeight,thickness,magnitude);
		x += thickness + barDistance;
	}
}

/* Draws audio spectrum db (in conversion dynamic range has been added to display more relevant results)
 * Decibel spectrum is not used, the table displayed is Magnitude spectrum.
*/
void testApp::drawAudioSpectrumDB()
{
	ofSetColor(165,42,42);

	m_decibels = testFFT->getDecibels(); //get the decibel array
	int x = m_spectogramXBoundary;
	int barDistance = 1;

	int quarter = BUFFER_SIZE /4;
	int thickness = (m_spectogramWidth - (quarter - 1) * barDistance) / quarter;

	if (thickness < 1) {
		thickness = 1;
	}

	for (int i = 0; i < quarter; i++) {
		float decibels = m_decibels[i] * 2.0f;
		drawBar(x,m_spectogramHeight,thickness,decibels);
		x += thickness + barDistance;
	}
}


/* Checks if a key is pressed. */
void testApp::keyPressed  (int key){ 
	if( (key == 'f' || key == 'F')  && !m_isPressed) { 
		ofFileDialogResult openFileResult= ofSystemLoadDialog("Select a FLAC or WAV"); 
		m_isPressed = openFileResult.bSuccess;
		m_filePath = openFileResult.getPath();

		//setup everything here.
		if(m_isPressed) {
			setupFFTForSoundFile();
		}
	}

	if( (key == 'm' || key == 'M') && !m_isPressed) {
		m_isPressed = true;
		setupFFTForMic();

	}
	if (key == OF_KEY_RETURN) {
		cout << "OF RETURN KEY!" << endl;
		if(m_isFullScreen) {
			cout << "NO LONGER IN FULL SCREEN" << endl;
			m_isFullScreen = false;
			ofSetFullscreen(m_isFullScreen);
		} else {
			cout << "OF RETURN KEY!" << endl;
			m_isFullScreen = true;
			ofSetFullscreen(m_isFullScreen);

		}
	}

	if (key == '+' && m_isPressed) {
		m_isMoreOptions = !m_isMoreOptions;
	}
	if (key == '0' && m_isPressed) {
		m_windowingFunc = 0;
		m_windowName = "No Window";
	}
	if (key == '1' && m_isPressed) {
		m_windowingFunc = 1;
		m_windowName = "Hanning";
	}
	if (key == '2' && m_isPressed) {
		m_windowingFunc = 2;
		m_windowName = "Blackman-Harris";
	}
	if (key == '3' && m_isPressed) {
		m_windowingFunc = 3;
		m_windowName = "Hamming";
	}
	if (key == '4' && m_isPressed) {
		m_windowingFunc = 4;
		m_windowName = "Flat Top";
	}
}

/* Draws the Ocilloscope */
void testApp::drawOcilloscope()
{
	ofNoFill();
	float width = m_spectogramWidth;
	float widthCoef = width / BUFFER_SIZE;

	// draw channelAvg (both channel):
	ofPushStyle();
	ofPushMatrix();
	ofTranslate(32, 170, 0);

	ofSetColor(165,42,42);
	ofDrawBitmapString("Both Channels", 4, m_spectogramHeight - m_spectogramYBoundary + 68);

	ofSetLineWidth(1);	
	ofRect(0, m_spectogramHeight - m_spectogramYBoundary + 50, width, 150);


	ofSetColor(165,42,42);
	ofSetLineWidth(3);

	ofBeginShape();
	for (int i = 0; i < BUFFER_SIZE; i++){
		ofVertex(i * widthCoef, (m_spectogramHeight - 75) - m_channelAvg[i]*75.0f);
	}
	ofEndShape(false);

	ofPopMatrix();
	ofPopStyle();
}



/* This function is called when system is feeding data to audio output 
 * overriding the function defined in ofBaseApp to initialize sound channels and play the sound file.
 * Both audioOut and audioIn functions will be called on a different thread than the one used by draw() & update() functions.
 * more info on audioOut can be found here: http://openframeworks.cc/documentation/sound/ofSoundStream.html
*/
void testApp::audioOut(float * output, int bufferSize, int nChannels)
{
	//how the loop should be:
	for (int i = 0; i < bufferSize; i++) {
		//if  everything in the array already been processed(read) then,
		//load more data into the array.
		if(m_channelSize < m_pos + i) {


			// thread is not needed, which means there is no next cycle if everything is processed then, exit the program.
			if(!m_sound->needThread()) {
				exitApp();
			}

			bool isFinished = m_sound->isFinished(); 
			//cout << isFinished << endl;
			//system("pause");
			//bool anyFrameLeft = m_sound->readSamplesTEMP();


			m_sound->initChannels();
			m_sound->start(); //run it again. this starts the threaded function, 


			//bool anyFrameLeft = m_sound.readSamples(); //definitely needs threads..
			if(!isFinished) {
				//IT WAS OUTSIDE ISFINISHED (BEFORE ISFINISHED)

				m_channelSize = m_sound->getChannelSize();
				m_left = m_sound->getLeft();
				m_right = m_sound->getRight();
				m_pos = 0;
			} else { //finished! nothing to play. exit the program.
				exitApp();
				ofExit();
				ofSoundStreamClose();
				m_sound->stop();
			}
		} else { //still data to be read.
			output[i * nChannels] = m_left[m_pos];       // left
			m_currentLeft[i] = output[i * nChannels];

			output[i * nChannels + 1] = m_right[m_pos];  //right
			m_currentRight[i] = output[i * nChannels + 1]; 
			m_pos++; //m_pos = m_pos + i;

			m_currentSec++; //used to display the current absolute position of the sound file.
		}
	}

}


/* Reads data from the microphone and initialize sound channels to visualize the data. */
void testApp::audioIn(float *input, int bufferSize, int nChannels)
{
	// samples are  interleaved.
	for (int i = 0; i < bufferSize; i++){
		m_currentLeft[i] = input[i * nChannels];
		m_currentRight[i] = input[i * nChannels + 1];
	}
}