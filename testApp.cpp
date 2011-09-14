#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

        vidGrabber.setVerbose(true);
        vidGrabber.initGrabber(320,240);

    colorImg.allocate(320,240);
	grayImage.allocate(320,240);
	grayBg.allocate(320,240);
	grayDiff.allocate(320,240);

	bLearnBakground = true;
	threshold = 80;
	catchButtons = false;

	numButtons = 0;

	for (int i=0; i < NUM_BUTTONS; i++) {
		reds[i] = (int) ofRandom(50, 255);
		greens[i] = (int) ofRandom(50, 255);
		blues[i] = (int) ofRandom(50, 255);
		isButtonOn[i] = false;
	}

		//audio setup
	int bufferSize		= 512;
	sampleRate 			= 44100;
	//phase, phase2, phase3 = 0.0f;
	phase = 0.0f; phase2 = 0.0f; phase3 = 0.0f; phase4 = 0.0f;
	phaseAdder = 0.0f; phaseAdder2 = 0.0f; phaseAdder3 = 0.0f; phaseAdder4 = 0.0f;
	phaseAdderTarget = 0.0f; phaseAdderTarget2 = 0.0f; phaseAdderTarget3 = 0.0f; phaseAdderTarget4 = 0.0f;
	volume				= 0.1f;
	bNoise 				= false;
	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);
	lAudio2.assign(bufferSize, 0.0);
	rAudio2.assign(bufferSize, 0.0);
	soundStream.setup(this, 2, 0, sampleRate, bufferSize, 4);

	targetFrequency = 440.0f;
	phaseAdderTarget = (targetFrequency / (float) sampleRate) * TWO_PI;
	//pythagorean 5th
	phaseAdderTarget2 = ((targetFrequency * 1.5f) / (float) sampleRate) * TWO_PI;
	//pythagorean major third (plus an octave)
	phaseAdderTarget3 = (((targetFrequency * 2.0f) * (81.0f / 64.0f)) / (float) sampleRate) * TWO_PI;
	//pythagorean major seventh (plus an octave)
	phaseAdderTarget4 = (((targetFrequency * 2.0f) * (243.0f / 128.0f)) / (float) sampleRate) * TWO_PI;
	pan = 0.5f;
	
	ofSetFrameRate(60);
}

//--------------------------------------------------------------
void testApp::update(){
	ofBackground(100,100,100);

    bool bNewFrame = false;

	vidGrabber.grabFrame();
	bNewFrame = vidGrabber.isFrameNew();

	if (bNewFrame) {

		colorImg.setFromPixels(vidGrabber.getPixels(), 320,240);

        grayImage = colorImg;
		if (bLearnBakground == true){
			grayBg = grayImage;		// the = sign copys the pixels from grayImage into grayBg (operator overloading)
			bLearnBakground = false;
		}

		// take the abs value of the difference between background and incoming and then threshold:
		grayDiff.absDiff(grayBg, grayImage);
		grayDiff.threshold(threshold);

		contourFinder.findContours(grayDiff, 20, (340*240)/3, MIN_BLOB_SIZE, false);
	}

}

//--------------------------------------------------------------
void testApp::draw(){

	// draw the incoming, the grayscale, the bg and the thresholded difference
	ofSetHexColor(0xffffff);
	//colorImg.draw(20,20);
	grayImage.draw(360,20);
	grayBg.draw(20,280);
	grayDiff.draw(360,280);

	// then draw the contours:

	ofFill();
	ofSetHexColor(0x333333);
	ofRect(20,20,320,240);
	ofSetHexColor(0xffffff);

	// we could draw the whole contour finder
	//contourFinder.draw(360,540);

	if (catchButtons) {
		numButtons = 0;
	}
	int buttonCounter = 0;

	// or, instead we can draw each blob individually,
	// this is how to get access to them:
    for (int i = 0; i < contourFinder.nBlobs; i++){
		if (isInLowerHalf(contourFinder.blobs[i].centroid)) {
	        contourFinder.blobs[i].draw(20,20);
			if (catchButtons && buttonCounter < NUM_BUTTONS) {
				ofRectangle rect = contourFinder.blobs[i].boundingRect;
				buttons[buttonCounter++] = rect;
			}
		}
    }
	if (catchButtons) {
		numButtons = buttonCounter;
		bLearnBakground = true;
	}

	catchButtons = false;

	for (int i=0; i < numButtons || i == NUM_BUTTONS; i++) {
		isButtonOn[i] = false;
		ofRectangle shiftedRect = buttons[i];
		shiftedRect.x+= 20;
		shiftedRect.y+= 20;
		ofFill();
		ofSetColor(reds[i], greens[i], blues[i]);
		ofRect(shiftedRect);

		//turn buttons on
		for (int j = 0; j < contourFinder.nBlobs; j++) {
			ofPoint centroid = contourFinder.blobs[j].centroid;
			int blobWidth = contourFinder.blobs[j].boundingRect.width;
			int blobHeight = contourFinder.blobs[j].boundingRect.height;
			ofPoint topLeft = ofPoint(contourFinder.blobs[j].boundingRect.x,
				contourFinder.blobs[j].boundingRect.x);
			ofPoint topRight = ofPoint(topLeft.x, topLeft.y + blobHeight);
			ofPoint bottomLeft = ofPoint(topLeft.x + blobWidth, topLeft.y);
			ofPoint bottomRight = ofPoint(topLeft.x + blobWidth, topLeft.y + blobHeight);

			bool isInside = 
				buttons[i].inside(centroid) ||
				buttons[i].inside(topLeft) ||
				buttons[i].inside(topRight) ||
				buttons[i].inside(bottomLeft) ||
				buttons[i].inside(bottomRight);

			if (isInside) {
				isButtonOn[i] = true;
				continue;
			}
		}
	}

	// finally, a report:
	ofSetHexColor(0xffffff);
	char reportStr[1024];
	sprintf(reportStr, "bg subtraction and blob detection\npress ' ' to capture bg\nthreshold %i (press: +/-)\nnum blobs found %i, fps: %f", threshold, contourFinder.nBlobs, ofGetFrameRate());
	ofDrawBitmapString(reportStr, 20, 600);

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

	switch (key){
		case ' ':
			bLearnBakground = true;
			break;
		case '+':
			threshold ++;
			if (threshold > 255) threshold = 255;
			break;
		case '-':
			threshold --;
			if (threshold < 0) threshold = 0;
			break;
		case 'c':
			catchButtons = true;
			break;
		case 'e':
			for (int i=0; i < NUM_BUTTONS; i++) isButtonOn[i] = false;
			numButtons = 0;
			break;
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

bool testApp::isInLowerHalf(ofPoint p) {
	int cutoff = 240 - 240 / 2;
	return p.y > cutoff? true : false;
}

void testApp::audioOut(float * output, int bufferSize, int nChannels){
	float leftScale = 1 - pan;
	float rightScale = pan;
	volume = 0.1f;

	// sin (n) seems to have trouble when n is very large, so we
	// keep phase in the range of 0-TWO_PI like this:
	while (phase > TWO_PI){
		phase -= TWO_PI;
	}

	phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
	phaseAdder2 = 0.95f * phaseAdder2 + 0.05f * phaseAdderTarget2;
	phaseAdder3 = 0.95f * phaseAdder3 + 0.05f * phaseAdderTarget3;
	phaseAdder4 = 0.95f * phaseAdder4 + 0.05f * phaseAdderTarget4;
	for (int i = 0; i < bufferSize; i++){
		phase += phaseAdder; phase2 += phaseAdder2; phase3 += phaseAdder3; phase4 += phaseAdder4;
		float sample = sin(phase);
		float sample2 = sin(phase2);
		float sample3 = sin(phase3);
		float sample4 = sin(phase4);

		float sample_left = 0.0f;
		float sample_right = 0.0f;
		if (isButtonOn[0]) sample_left += sample;
		if (isButtonOn[1]) sample_left += sample2;
		//if (isQuadrantOn[2]) sample_right += sample3;
		//if (isQuadrantOn[3]) sample_right += sample4;

		lAudio[i] = output[i*nChannels    ] = sample_left * volume * leftScale;
		rAudio[i] = output[i*nChannels + 1] = sample_right * volume * rightScale;
	}
}