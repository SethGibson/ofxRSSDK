#ifndef __OFAPP_H__
#define __OFAPP_H__
#ifdef _DEBUG
#pragma comment(lib, "libpxcmd_d.lib")
#else
#pragma comment(lib, "libpxcmd.lib")
#endif
#include <vector>
#include "ofMain.h"
#include "ofxRSSDK.h"

using namespace std;
class ofApp : public ofBaseApp
{
public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

private:
	void setupCamera();
	int mWidth, mHeight;
	ofxRSSDK mRSSDK;
	
	vector<ofVec3f> mCloudPoints;
	ofMesh mCloudMesh;

	ofEasyCam mCamera;

};

#endif
