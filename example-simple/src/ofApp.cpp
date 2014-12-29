#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	ofSetWindowShape(1280, 960);
	if (!mRSSDK.init())
		return;

	int cW = static_cast<int>(mRSSDK.getWidth());
	int cH = static_cast<int>(mRSSDK.getHeight());
	mDepthColorTex.allocate(cW, cH, GL_RGB);
	mDepthColorPixels = new uint8_t[cW*cH * 3]{0};
	mNearColor = ofColor(255, 0, 0);
	mFarColor = ofColor(20, 40, 255);

}

//--------------------------------------------------------------
void ofApp::update()
{
	mRSSDK.update();
	depthToColor();
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofClear(ofColor::black);
	mRSSDK.draw(0, 0);
	ofDrawBitmapString("Raw Color", ofPoint(10, 20));
	ofPushMatrix();
	ofTranslate(640, 0);
	mRSSDK.drawDepth(0, 0);
	ofDrawBitmapString("Depth Pixels", ofPoint(10, 20));
	ofTranslate(-640, 480);
	mDepthColorTex.draw(0, 0);
	ofDrawBitmapString("Colored Depth", ofPoint(10, 20));
	ofPopMatrix();
}

void ofApp::depthToColor()
{
	int cWidth = mRSSDK.getWidth();
	int cHeight = mRSSDK.getHeight();
	uint16_t *cDepthMap = reinterpret_cast<uint16_t *>(mRSSDK.getRawDepthPixels());

	// Produce a cumulative histogram of depth values
	if (cDepthMap)
	{
		int histogram[256 * 256] = { 1 };
		for (int i = 0; i < cWidth * cHeight; ++i)
		{
			if (auto d = cDepthMap[i]) ++histogram[d];
		}
		for (int i = 1; i < 256 * 256; i++)
		{
			histogram[i] += histogram[i - 1];
		}
		
		//// Remap the cumulative histogram to the range 0..256
		for (int i = 1; i < 256 * 256; i++)
		{
			histogram[i] = (histogram[i] << 8) / histogram[256 * 256 - 1];
		}

		// Produce RGB image by using the histogram to interpolate between two colors
		for (int i = 0; i < cWidth * cHeight;i++)
		{
			if (uint16_t d = cDepthMap[i]) // For valid depth values (depth > 0)
			{
				auto t = histogram[d]; // Use the histogram entry (in the range of 0..256) to interpolate between nearColor and farColor
				mDepthColorPixels[i * 3] = ((256 - t) * mNearColor.r + t * mFarColor.r) >> 8;
				mDepthColorPixels[i * 3+1] = ((256 - t) * mNearColor.g + t * mFarColor.g) >> 8;
				mDepthColorPixels[i * 3+2] = ((256 - t) * mNearColor.b + t * mFarColor.b) >> 8;
			}
			else // Use black pixels for invalid values (depth == 0)
			{

				mDepthColorPixels[i * 3] = 0;
				mDepthColorPixels[i * 3+1] = 0;
				mDepthColorPixels[i * 3+2] = 0;
			}
		}

		mDepthColorTex.loadData(mDepthColorPixels, cWidth, cHeight, GL_RGB);
	}
}

void ofApp::exit()
{
	delete[] mDepthColorPixels;
}
/*
//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
*/