#include "MyFFT.h"

#ifndef M_PI
#define	M_PI		3.14159265358979323846  //PI CONSTANT sometimes defined in math.h if not defined here.
#endif



/*constructor, allocates space for vectors, arrays initializes values, creates trig tables and reverse bit table.*/
MyFFT::MyFFT(int FFTSize) 
{
	m_FFTSize = FFTSize;
	realOutput.resize(m_FFTSize);
	imagOutput.resize(m_FFTSize);
	realInput.resize(m_FFTSize);
	imagInput.resize(m_FFTSize);

	m_magnitude = new float[m_FFTSize];
	m_power = new float[m_FFTSize];
	m_phase =  new float[m_FFTSize];
	m_decibels =  new float[m_FFTSize];



	//half the FFT Size, will be needed in RealFFT method.
	tempRealInput.resize(m_FFTSize / 2);
	tempImagInput.resize(m_FFTSize / 2);



	//make sure the size is power of two.
	if (!isPowerOfTwo(m_FFTSize)) {
		cout << m_FFTSize << " is not power of 2!" << endl;
		exit(1);
	}

	//create the tables.
	int numOfBits = numOfRequiredBits(m_FFTSize);
	createLookUpTable(numOfBits);
	createTrigTables(numOfBits);
}


MyFFT::~MyFFT(void)
{
	delete[] m_magnitude;
	delete[] m_power;
	delete[] m_decibels;
	delete[] m_phase;
}

/*a fast bitwise operation trick to determine if given number is the power of two. */
bool MyFFT::isPowerOfTwo(int num) const
{
	if( (num & (num - 1)) || num < 2) {
		return false;
	}
	return true;
}

/* Given bits and the size of the bit, it returns the reverse of that bit.
 * this function is used to create the lookup table.
 * idea behind this method can be found here: http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
*/
int MyFFT::reverseBits(int bits, int bitSize) const
{
	int revBits = 0;
	while(bitSize != 0) { //bitsize--
		revBits <<= 1;
		revBits |= bits & 1;
		bits >>= 1;
		bitSize--;
	}
	return revBits;
}


/* Creates a lookup table for reverse bits (an array which has the reverse bits value for every number that bit can hold.
 * since in fft buffer size will be between 256 to 8192 (8 to 13 bits) size of this table won't be noticable.
*/
void MyFFT::createLookUpTable(int bitSize)
{
	int size = pow(2.0,(double)bitSize); // or 1 << bitSize;

	cout << "Table Size: " << size << endl;

	reverseBitsTable.resize(size);

	//reverseBitsTable = new int[size];

	for (int i = 0; i < size; i++) {
		int bits = i;
		reverseBitsTable[i] = reverseBits(bits,bitSize);
	}
}

/* After creating the table, bit reverse lookup in O(1) time..
 * if the bitSize is different than the tableSize then use >> (tableSize - bitSize);
*/
inline int MyFFT::fastRevBits(int bits) const
{
	return reverseBitsTable[bits]; // >> (tableSize - bitSize);
}


/* Generates trigonometric lookup tables
 * Accuracy of FFT is very sensitive to trigonometric erros, be very careful when you estimate trig values rather than computing them
*/
void MyFFT::createTrigTables(int bitSize)
{
	int size = pow(2.0,(double)bitSize); // or 1 << bitSize;

	sinTable.resize(size);
	cosTable.resize(size);

	//0 won't ever be used but set to 0 just to be safe.
	sinTable[0] = 0;
	cosTable[0] = 0;

	for( int i = 1; i < size; i++ )
	{
		sinTable[i] = sin( -M_PI / i );
		cosTable[i] = cos( -M_PI / i );
	}
}



/* Finds the required number of bits for given size. */
int MyFFT::numOfRequiredBits(int size) const
{
	for (int i = 0;; i++) {
		if (size & (1 << i)) {
			return i;
		}
	}
}


/*
 * The Danielson Lanczos Algorithm
 * Radix-2
 * FFT on N/2 samples, the function uses precomputed trigonometric tables and bit reversal tables to speed up the process.
 * FFT LOOPS:
 * loop1 -> loop for FFT Stages (logN stages)
 * loop2 -> loop for each sub-dft
 * loop3 -> loop for each butterfly (additions/substractions & twiddle factors)
*/
void MyFFT::complexFFT(int numOfSamples)
{
	//numOfBits required to store indices.
	int numOfBits = numOfRequiredBits(numOfSamples); 

	//check if the new samples pow of 2 
	// can be avoided since constructor will check at the beginning but assume the program might feed different numOfSamples than the original size assigned in constructor
	if (!isPowerOfTwo(numOfSamples)) {
		cout << numOfSamples << " is not power of 2!" << endl;
		exit(1);
	}


	//creates m_realOutput and m_imagOutput in reversed Bit Order, 
	for (int i = 0; i < numOfSamples; i++) {
		int j = fastRevBits(i);
		realOutput[j] = tempRealInput[i];
		imagOutput[j] = tempImagInput[i]; 	
	}

	//Compute FFT
	for(int halfSize = 1; halfSize < numOfSamples; halfSize *= 2) {

		//default phase values.
		float phaseShiftCurrentReal = 1;
		float phaseShiftCurrentImag = 0;

		float phaseShiftReal = cosTable[halfSize];
		float phaseShiftImag = sinTable[halfSize];

		for (int fftSubStep = 0; fftSubStep < halfSize; fftSubStep++) {
			for (int k = fftSubStep; k < numOfSamples; k += 2 * halfSize) {

				int j = k + halfSize;


				float tempr =  phaseShiftCurrentReal * realOutput[j] - phaseShiftCurrentImag * imagOutput[j];
				float tempi =  phaseShiftCurrentReal * imagOutput[j] + phaseShiftCurrentImag * realOutput[j];

				realOutput[j] = realOutput[k] - tempr;
				imagOutput[j] = imagOutput[k] - tempi;

				realOutput[k] += tempr;
				imagOutput[k] += tempi;

			}

			//trigonometric recurrence
			float tempPhaseReal = phaseShiftCurrentReal; 
			phaseShiftCurrentReal = (phaseShiftCurrentReal * phaseShiftReal) - (phaseShiftCurrentImag * phaseShiftImag);
			phaseShiftCurrentImag = (phaseShiftCurrentImag * phaseShiftReal) + (tempPhaseReal * phaseShiftImag);
		}
	}

}




/* 
 * Splits the data to half and into 2 parts real and imaginary, in real Input sequence 2 * nth cell will represent the realOutput and 2 * nth + 1 will represent imaginaryOutput,
 * treats the real half data as a complex data (will be used by complexFFT function)
 * Due to this symetrical property the spectrum will be double sided, (i.e. first half = second half) so plot only first half of the spectrum, 
 * then, in testApp.h draw only quarter of the spectrum, this will be less accurate but much more faster.
 *
 * I found this method in a book called "Numerical Recipes In C" 
 * 
 * The Idea described in more details in section "FFT of Single Real Function" page 512 http://astronu.jinr.ru/wiki/upload/d/d6/NumericalRecipesinC.pdf
 * 
 * 
 * More resources on Real FFT
 * http://www.katjaas.nl/realFFT/realFFT.html
 * http://www.engineeringproductivitytools.com/stuff/T0001/PT10.HTM
*/
void MyFFT::realFFT(int numOfSamples) //float *m_realOutput, float *m_imagOutput --> m_realOutput
{
	int half = numOfSamples / 2;

	float theta = M_PI / half;

	//use only  first half
	for (int n = 0; n < half; n++) {
		tempRealInput[n] = realInput[2 * n];
		tempImagInput[n] = realInput[2 * n + 1];
	}


	complexFFT(half);


	float phaseTemp = float (sin(0.5 * theta));

	float phaseShiftReal = -2.0 * phaseTemp * phaseTemp;
	float phaseShiftImag = float (sin(theta));
	float phaseShiftCurrentReal = 1.0 + phaseShiftReal;
	float phaseShiftCurrentImag = phaseShiftImag;



	float h1r, h1i, h2r, h2i; //extract information on real imaginary parts 

	//iterate throught the first half / 2
	for (int i = 1; i < half / 2; i++) {

		int currentTail = half - i; //current tail(when i progress current taill will shift right eventually they will meet at the middle freq

		//transforms are seperated from realOut and ImaginaryOut
		h1r = 0.5 * (realOutput[i] + realOutput[currentTail]); 
		h1i = 0.5 * (imagOutput[i] - imagOutput[currentTail]);
		h2r = 0.5 * (imagOutput[i] + imagOutput[currentTail]);
		h2i = -0.5 * (realOutput[i] - realOutput[currentTail]);

		//recombined to generate the "real" transform of the original data.
		realOutput[i] = h1r + phaseShiftCurrentReal * h2r - phaseShiftCurrentImag * h2i;
		imagOutput[i] = h1i + phaseShiftCurrentReal * h2i + phaseShiftCurrentImag * h2r;
		realOutput[currentTail] = h1r - phaseShiftCurrentReal * h2r + phaseShiftCurrentImag * h2i;
		imagOutput[currentTail] = -h1i + phaseShiftCurrentReal * h2i + phaseShiftCurrentImag * h2r;

		//trig recurrence
		phaseTemp = phaseShiftCurrentReal;
		phaseShiftCurrentReal = phaseShiftCurrentReal * phaseShiftReal - phaseShiftCurrentImag * phaseShiftImag;   // phaseShiftCurrentReal * phaseShiftReal - phaseShiftCurrentImag * phaseShiftImag + phaseShiftCurrentReal;
		phaseShiftCurrentImag = phaseShiftCurrentImag * phaseShiftReal + phaseTemp * phaseShiftImag;  // phaseShiftCurrentImag * phaseShiftReal + phaseTemp * phaseShiftImag + phaseShiftCurrentImag;
	}

	//finally, put the last piece.
	h1r = realOutput[0];
	realOutput[0] = h1r + imagOutput[0];
	imagOutput[0] = h1r - imagOutput[0];
}


/* About Windowing Functions
 * Frequencies will eventually leak into surrounding bins, to prevent this windowing function is used, 
 * windowing function provides frequency isolation in the frequency domain however, it causes information loss in time domain,
 * most suitable windowing function for the realInput to generate the power spectrum.
 * formula is derived from http://en.wikipedia.org/wiki/Window_function#Hann_.28Hanning.29_window
*/
void MyFFT::applyHanningFunc(int windowSize)
{
	for (int i = 0; i < windowSize; i++){
		realInput[i] *= 0.50 - 0.50 * cos(2 * M_PI * i / (windowSize - 1));
	}
}


/* Another windowing function particularly useful on low resolution (when frequency bin is much larger, i.e. smaller fft size) */
void MyFFT::applyBlackmanHarrisFunc(int windowSize)
{
	float a0 = 0.35875;
	float a1 = 0.48829;
	float a2 = 0.14128;
	float a3 = 0.01168;

	for(int i = 0; i < windowSize; i++) {
		realInput[i] *= a0 - (a1 * cos(2 * M_PI * i / (windowSize - 1))) + (a2 * cos(4 * M_PI * i / (windowSize - 1))) + (a3 * cos(6 * M_PI * i / (windowSize - 1)));
	}

}



void MyFFT::applyHammingFunc(int windowSize)
{
	for (int i = 0; i < windowSize; i++) {
		realInput[i] *= 0.54 - 0.46 * cos(2 * M_PI * i / (windowSize - 1));
	}
}


void MyFFT::applyFlatTopFunc(int windowSize)
{
	float a0 = 1;
	float a1 = 1.93;
	float a2 = 1.29;
	float a3 = 0.388;
	float a4 = 0.032;

	for(int i = 0; i < windowSize; i++) {
		realInput[i] *= a0 
			- (a1 * cos(2 * M_PI * i / (windowSize - 1))) 
			+ (a2 * cos(4 * M_PI * i / (windowSize - 1))) 
			+ (a3 * cos(6 * M_PI * i / (windowSize - 1)))
			+ (a4 * cos(8 * M_PI * i / (windowSize - 1)));
	}

}


/* given int windowingFunc determine which windowing function to apply, 
 * this function simplifies the use of windowing functions and allows user to apply them on the fly in the event loop.
*/
void MyFFT::applySelectedWinFunction(int windowingFunc, int windowSize)
{
	if (windowingFunc == 1) {
		applyHanningFunc(windowSize);
	} else if (windowingFunc == 2) {
		applyBlackmanHarrisFunc(windowSize);
	} else if (windowingFunc == 3) {
		applyHammingFunc(windowSize);
	} else if (windowingFunc == 4) {
		applyFlatTopFunc(windowSize);
	}
	//add more windowing functions.
}

/* Second half ignored since they have complex conjugate symetry with the first half
 * this is the main method which will be called by testApp class, it applies a windowing function(currently Hanning but for lower resolutions Blackman-Harris produces better results.
 * takes the fft  and calculates power, magnitude phase and decibel 
 * then testApp uses these values to visualize sounds
 * windowingFunc determines which windowing function to be used
 * 0 -> No windowing Function applied
 * 1 -> Hanning Function
 * 2 -> Blackman-Harris function
 * 3 -> Hamming Function
 * 4 -> Flat Top Function
*/
void MyFFT::powerSpectrum(int windowSize, float *data, int windowingFunc) {


	//will be used in normalization process..
	m_minDB = m_minMAG = std::numeric_limits<float>::max();
	m_maxDB = m_maxMAG = std::numeric_limits<float>::min();

	float bitRange = windowSize * 2; //windowSize always pow of 2, this is the bit range (for 1024, range is -1024 to 1024 so 2048)

	// calculate the dynamic range for db 
	float dynamicRange = fabs(20.0f * log10f(1.0f / bitRange));

	//cout << dynamicRange << endl;

	int half = windowSize / 2; 



	for (int i = 0; i < windowSize; i++) {
		realInput[i] = data[i];
	}

	//apply the window function before FFT.
	applySelectedWinFunction(windowingFunc,windowSize);
	realFFT(windowSize);

	//calculate the magnitude of first half of the FFT output bins.
	for (int i = 0 ; i < half; i++) {

		//calculate power  = i^2 + r^2
		m_power[i] = realOutput[i]*realOutput[i] + imagOutput[i]*imagOutput[i];

		//find magnitude = 2 * sqrt(power)
		m_magnitude[i] = sqrt(m_power[i]);

		if(m_magnitude[i] > m_maxMAG) {
			m_maxMAG = m_magnitude[i];
		}

		if(m_magnitude[i] < m_minMAG) {
			m_minMAG = m_magnitude[i];
		}

		//find phase = arctangent(i,r)
		//not used by the current spectrums, but calculated just incase needed later.
		m_phase[i] = atan2(imagOutput[i], realOutput[i]);

		//cout << "magnitude: " << m_magnitude[i] << " phase: " << m_phase[i] << endl;
		//cout << "normalization factor: " << avg << endl;

		//find decibels = 20 * log10(magnitude)
		//then add the dynamic range for scaling, its to make better looking spectrum.



		m_decibels[i] = dynamicRange + 20 *log10f(m_magnitude[i]); 

		//m_decibels[i] = (m_decibels[i] / dynamicRange) * 100;


		//no longer in use.
		if(m_decibels[i] > m_maxDB) {
			m_maxDB = m_decibels[i];
		}

		if(m_decibels[i] < m_minDB) {
			m_minDB = m_decibels[i];
		}


	}

	//empty the values for the next cycle.
	for (int i = 0; i < m_FFTSize; i++) {
		imagOutput[i] = realOutput[i] = 0;
	}
}


//Getter functions 
float* MyFFT::getMagnitude() const
{
	return m_magnitude;
}

float* MyFFT::getPower() const
{
	return m_power;
}
float* MyFFT::getDecibels() const
{
	return m_decibels;
}
float* MyFFT::getPhase() const
{
	return m_phase;
}

float MyFFT::getMinDB() const
{
	return m_minDB;
}

float MyFFT::getMaxDB() const
{
	return m_maxDB;
}

float MyFFT::getMinMAG() const
{
	return m_minMAG;
}

float MyFFT::getMaxMAG() const
{
	return m_maxMAG;
}




