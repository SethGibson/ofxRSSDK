#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	ofSetWindowShape(1280, 800);
	if (!mRSSDK.init())
	{
		ofLogError("Unable to create ofxRSSDK object");
	}

	mRSSDK.setCalcCameraPoints(true);

	mWidth = static_cast<int>(mRSSDK.getWidth());
	mHeight = static_cast<int>(mRSSDK.getHeight());

	setupCamera();
}

//--------------------------------------------------------------
void ofApp::update()
{
	mRSSDK.update();
	mCloudMesh.clear();

	mCloudMesh.setMode(OF_PRIMITIVE_POINTS);
	mCloudMesh.enableColors();
	for (auto p : mRSSDK.getWorldPoints())
	{
		mCloudMesh.addVertex(p);
		mCloudMesh.addColor(ofColor::white);
	}
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofClear(ofColor::black);
	ofSetColor(ofColor::white);

	mCamera.begin();
	mCloudMesh.draw(); 
	mCamera.end();
}

void ofApp::setupCamera()
{
	ofPoint cFOV = mRSSDK.getDepthFOV();
	mCamera.setFov(cFOV.y);
	mCamera.setAspectRatio(ofGetWindowWidth() / (float)ofGetWindowHeight());
	mCamera.setNearClip(100);
	mCamera.setFarClip(5000);

	mCamera.setGlobalPosition(ofVec3f(0, 0, 0));
	mCamera.lookAt(ofVec3f(0, 0, 100), ofVec3f(0, 1, 0));
	mCamera.setAutoDistance(true);
}

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
