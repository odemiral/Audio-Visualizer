/* Written By Onur Demiralay 
 * For more information, please take a look at README.md
*/

#include "testApp.h"
#include "ofAppGlutWindow.h"


int main()
{
	ofAppGlutWindow window; // create a window
	
	//set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
	ofSetupOpenGL(&window, 1280, 768, OF_WINDOW);
	ofRunApp(new testApp()); // start the app
}
