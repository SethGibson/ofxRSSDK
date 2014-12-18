#ifndef __OFX_RSSDK_H__
#define __OFX_RSSDK_H__

#include "ofMain.h"
#include "pxcsensemanager.h"
#include "ofxBase3DVideo.h"


class ofxRSSDKContext;

/// \class ofxRSSDK
///
/// Base3dVideo implementation of RealSense SDK
/// based on ofxKinect
///
class ofxRSSDK : public ofxBase3DVideo, protected ofThread {

public:

	ofxRSSDK();
	virtual ~ofxRSSDK();

	bool init();
	void clear();
	void setRegistration(bool bUseRegistration = false);
	void close();

	bool isConnected();
	bool isFrameNew();
	void update();

	float getDistanceAt(int x, int y);
	float getDistanceAt(const ofPoint & p);

	ofVec3f getWorldCoordinateAt(int cx, int cy);
	ofVec3f getWorldCoordinateAt(float cx, float cy, float wz);

	ofColor getColorAt(int x, int y);
	ofColor getColorAt(const ofPoint & p);

	unsigned char* getPixels();

	unsigned char* getDepthPixels();       ///< grayscale values
	unsigned short* getRawDepthPixels();   ///< raw 11 bit values
	float* getDistancePixels();
	ofFloatPixels & getDistancePixelsRef();

	ofPixels & getPixelsRef();
	ofPixels & getDepthPixelsRef();       	///< grayscale values
	ofShortPixels & getRawDepthPixelsRef();	///< raw 11 bit values

	ofTexture& getTextureReference();
	ofTexture& getDepthTextureReference();

	void enableDepthNearValueWhite(bool bEnabled = true);
	bool isDepthNearValueWhite();

	void setDepthClipping(float nearClip = 500, float farClip = 4000);
	float getNearClipping();
	float getFarClipping();

	void setUseTexture(bool bUse);

	void draw(float x, float y, float w, float h);
	void draw(float x, float y);
	void draw(const ofPoint& point);
	void draw(const ofRectangle& rect);

	void drawDepth(float x, float y, float w, float h);
	void drawDepth(float x, float y);
	void drawDepth(const ofPoint& point);
	void drawDepth(const ofRectangle& rect);

	const static int width = 640;
	const static int height = 480;
	float getHeight();
	float getWidth();

protected:
	bool bUseTexture;
	ofTexture depthTex; ///< the depth texture
	ofTexture videoTex; ///< the RGB texture
	bool bGrabberInited;

	ofPixels videoPixels;
	ofPixels depthPixels;
	ofShortPixels depthPixelsRaw;
	ofFloatPixels distancePixels;

	ofPoint rawAccel;
	ofPoint mksAccel;

	float targetTiltAngleDeg;
	float currentTiltAngleDeg;
	bool bTiltNeedsApplying;

	int currentLed;
	bool bLedNeedsApplying;

	// for auto connect tries
	float timeSinceOpen;
	int lastDeviceId;
	bool bGotData;
	int tryCount;

private:
	PXCSenseManager* mRealSenseDevice;      ///< kinect device handle

	vector<unsigned char> depthLookupTable;
	void updateDepthLookupTable();
	void updateDepthPixels();

	bool bIsFrameNew;
	bool bNeedsUpdate;
	bool bUpdateTex;
	bool bGrabVideo;
	bool bUseRegistration;
	bool bNearWhite;

	float nearClipping, farClipping;

	bool bIsVideoInfrared;  ///< is the video image infrared or RGB?
	int videoBytesPerPixel; ///< how many bytes per pixel in the video image
};
#endif