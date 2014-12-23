#include "ReadSound.h"


ReadSound::ReadSound(const char *fileName)
{
	infile = 0;
	infile = sf_open(fileName,SFM_READ,&sfinfo); //loads the file into infile and updates sfinfo
	if (infile == NULL) {
		printf("Failed to open the file\n");
		exit(1);
	}

	m_isFinished = false;
	
	m_sampleRate = sfinfo.samplerate;


	m_frames = sfinfo.frames;
	m_totalArrSize = sfinfo.frames * sfinfo.channels;
	m_framesLeft = m_totalArrSize; //frames left to process, since none processes its all the frames in given file.

	cout << m_framesLeft << endl;
	//system("pause");

	m_5MB = 5242880 / sizeof(float); // modelSize, 5mb 2.5mb left +  2.5mb right
	cout << "arraySize: " << m_5MB << endl;

	//allocate space for sample data.
	float *samplesBoth;
	if(m_framesLeft - m_5MB > 0) { //more than 5mb of data, which means there will be a next cycle, so threads required!
		m_needThread = true;
		samplesBoth = new float[m_5MB];
		m_channelSize = m_5MB / 2;
		m_left = new float[m_channelSize];
		m_right = new float[m_channelSize];
		m_framesLeft -= m_5MB;
	} else { // less than 5mb of data required to play the data there won't be a next cycle,  so don't start threaded function!.
		m_needThread = false;
		samplesBoth = new float[m_framesLeft];
		m_channelSize = m_framesLeft / 2;
		m_left = new float[m_channelSize];
		m_right = new float[m_channelSize];
		m_framesLeft = 0;
	}

	//read data 
	sf_read_float (infile, samplesBoth, m_channelSize * 2); 

	ripChannels(samplesBoth);

	delete[] samplesBoth;

	//don't forget 	sf_close(infile);

}


/* Check if its is less than 5mb, if it is read whatever there is,
 * if its  >= 5mb then read 5mb 
 * substract 5mb from the source 
 * read until source = 0mb
 * if false then no more data left to read.
 *
 * this function is used after the first allocation.
*/

//Depricated function! threadedFunction used instead.
bool ReadSound::readSamples()
{
	//first free left and right channels
	delete[] m_left;
	delete[] m_right;

	//samples of both channels
	float *samplesBoth;
	//int size = 0;

	//no more frames left..
	if(m_framesLeft == 0) {
		return false;
	}
	//everytime right after allocating space for channels update m_currentFrame, and m_framesLeft.

	if((m_framesLeft - m_5MB) < 0) { 	//less than 5mb to read just read whatever left.
		cout << "LESS THAN 5MB TO READ!: " << m_framesLeft << endl;
		system("pause");
		samplesBoth = new float [m_framesLeft];
		m_channelSize = m_framesLeft / 2;
		m_left = new float[m_channelSize];
		m_right = new float[m_channelSize];
		m_framesLeft = 0; //last batch, no more frames left, next time you enter this function it will return false.
	} else { //else if ((m_framesLeft - m_5MB) > 0) { // more than 5mb to read just read 5mb
		cout << "MORE THAN 5MB TO READ! : " << m_framesLeft << endl;
		m_framesLeft -= m_5MB;
		samplesBoth = new float [m_5MB];
		m_channelSize = m_5MB / 2;
		m_left = new float[m_channelSize];
		m_right = new float[m_channelSize];
	}

	//read size time sample into samplesBoth, sample data for channels are interleaved.
	sf_read_float (infile, samplesBoth, m_channelSize * 2); 

	ripChannels(samplesBoth); // 
	delete[] samplesBoth;
	return true; 
}

/* assign proper data to specified channels by ripping it from source (they're interleaved) */
void ReadSound::ripChannels(float *samplesBoth)
{

	int pos = 0;
	//rip sample data to its corresponding channels.
	for(int i = 0; i < m_channelSize * 2; i += 2) {
		//left = i = 0, 2,4,6
		m_left[pos] = samplesBoth[i];

		//right = i + 1 = 1,3,5,7,
		m_right[pos] = samplesBoth[i + 1];
		pos++;
	}
}

/* Getter functions */
float* ReadSound::getLeft() const
{
	return m_left;
}

float* ReadSound::getRight() const
{
	return m_right;
}

long ReadSound::getChannelSize() const
{
	return m_channelSize;
}

int ReadSound::getSampleRate() const
{
	return m_sampleRate;
}

int ReadSound::getDuration() const
{
	return m_frames / m_sampleRate;
}

/* returns if there is anything else to read. */
bool ReadSound::isFinished() const
{
	// cout << "isFinished: " << m_isFinished << endl;
	return m_isFinished;
}

bool ReadSound::needThread() const
{
	return m_needThread;
}


//start the thread.
void ReadSound::start(){
	startThread(false, true);   // blocking, verbose
}

//stop the thread
void ReadSound::stop(){
	stopThread();
}


//read left and right and put the data into tempLeft and tempRight,
//this function doesn't alter m_left or m_right,

void ReadSound::threadedFunction()
{
	cout << "STARTING THREADED FUNCTION!" << endl;
	//samples of both channels
	float *samplesBoth;
	//int size = 0;

	//no more frames left..
	if(m_framesLeft == 0) {
		m_isFinished = true;
		return;
	}
	//everytime right after allocating space for channels update m_currentFrame, and m_framesLeft.

	if((m_framesLeft - m_5MB) < 0) { 	//less than 5mb to read just read whatever left.
		cout << "LESS THAN 5MB TO READ!: " << m_framesLeft << endl;
		//system("pause");
		samplesBoth = new float [m_framesLeft];
		m_channelSize = m_framesLeft / 2;

		m_tempLeft = new float[m_channelSize];
		m_tempRight = new float[m_channelSize];

		m_framesLeft = 0; //last batch, no more frames left, next time you enter this function it will return false.
	} else { //else if ((m_framesLeft - m_5MB) > 0) { // more than 5mb to read just read 5mb
		cout << "MORE THAN 5MB TO READ! : " << m_framesLeft << endl;
		m_framesLeft -= m_5MB;
		samplesBoth = new float [m_5MB];
		m_channelSize = m_5MB / 2;

		m_tempLeft = new float[m_channelSize];
		m_tempRight = new float[m_channelSize];
	}

	//read size time sample into samplesBoth, sample data for channels are interleaved.
	sf_read_float (infile, samplesBoth, m_channelSize * 2); 

	ripChannelsTEMP(samplesBoth); 
	delete[] samplesBoth;
}

/* clean up the memory used by old threads, and initialize sound channel arrays to current left and right */
void ReadSound::initChannels()
{
	delete[] m_left;
	delete[] m_right;

	m_left = m_tempLeft;
	m_right = m_tempRight;

}

//rip channels for the temp array
void ReadSound::ripChannelsTEMP(float *samplesBoth)
{
	int pos = 0;
	//rip sample data to its corresponding channels.
	for(int i = 0; i < m_channelSize * 2; i += 2) {
		//left = i = 0, 2,4,6
		m_tempLeft[pos] = samplesBoth[i];

		//right = i + 1 = 1,3,5,7,
		m_tempRight[pos] = samplesBoth[i + 1];
		pos++;
	}
}