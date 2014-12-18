#include "ofxRSSDK.h"
#include "ofMain.h"

//--------------------------------------------------------------------
ofxRSSDK::ofxRSSDK() 
{
	ofLog(OF_LOG_VERBOSE, "ofxRSSDK: Creating ofxRSSDK");

	bUseTexture = true;
	bGrabVideo = true;

	// set defaults
	bGrabberInited = false;

	bNeedsUpdate = false;
	bUpdateTex = false;
	bIsFrameNew = false;

	bIsVideoInfrared = false;
	videoBytesPerPixel = 3;

	kinectDevice = NULL;

	targetTiltAngleDeg = 0;
	currentTiltAngleDeg = 0;
	bTiltNeedsApplying = false;

	currentLed = -1;
	bLedNeedsApplying = false;

	lastDeviceId = -1;
	tryCount = 0;
	timeSinceOpen = 0;
	bGotData = false;

	bUseRegistration = false;
	bNearWhite = true;

	setDepthClipping();
}

//--------------------------------------------------------------------
ofxRSSDK::~ofxRSSDK()
{
	close();
	clear();
}

//--------------------------------------------------------------------
bool ofxRSSDK::init()
{
	if (isConnected()) {
		ofLog(OF_LOG_WARNING, "ofxRSSDK: Do not call init while ofxRSSDK is running!");
		return false;
	}

	clear();
	videoBytesPerPixel = 4;

	// allocate
	depthPixelsRaw.allocate(width, height, 1);
	depthPixelsRawBack.allocate(width, height, 1);

	videoPixels.allocate(width, height, videoBytesPerPixel);
	videoPixelsBack.allocate(width, height, videoBytesPerPixel);

	depthPixels.allocate(width, height, 1);
	distancePixels.allocate(width, height, 1);

	// set
	depthPixelsRaw.set(0);
	depthPixelsRawBack.set(0);

	videoPixels.set(0);
	videoPixelsBack.set(0);

	depthPixels.set(0);
	distancePixels.set(0);

	if (bUseTexture)
	{
		depthTex.allocate(width, height, GL_LUMINANCE);
		videoTex.allocate(width, height, GL_BGRA);
	}

	bGrabberInited = true;
	mRealSenseDevice = PXCSenseManager::CreateInstance();
	if (!mRealSenseDevice)
		bGrabberInited = false;

	bGrabberInited = mRealSenseDevice->EnableStream(PXCCapture::STREAM_TYPE_COLOR, width, height, 30) >= PXC_STATUS_NO_ERROR;
	bGrabberInited = mRealSenseDevice->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, width, height, 30) >= PXC_STATUS_NO_ERROR;
	bGrabberInited = mRealSenseDevice->Init() >= PXC_STATUS_NO_ERROR;

	return bGrabberInited;
}

//---------------------------------------------------------------------------
void ofxRSSDK::clear()
{
	if (isConnected()) {
		ofLog(OF_LOG_WARNING, "ofxRSSDK: Do not call clear while ofxRSSDK is running!");
		return;
	}

	depthPixelsRaw.clear();
	depthPixelsRawBack.clear();

	videoPixels.clear();
	videoPixelsBack.clear();

	depthPixels.clear();
	distancePixels.clear();

	depthTex.clear();
	videoTex.clear();

	bGrabberInited = false;
}

//--------------------------------------------------------------------
void ofxRSSDK::setRegistration(bool bUseRegistration) {
	this->bUseRegistration = bUseRegistration;
}

//---------------------------------------------------------------------------
void ofxRSSDK::close()
{
	bIsFrameNew = false;
	bNeedsUpdate = false;
	bUpdateTex = false;
}

//---------------------------------------------------------------------------
bool ofxRSSDK::isConnected()
{
	return mRealSenseDevice->IsConnected();
}

//--------------------------------------------------------------------
bool ofxRSSDK::isFrameNew()
{
	return bIsFrameNew;
}

//----------------------------------------------------------
void ofxRSSDK::update()
{
	if (!bGrabberInited)
	{
		return;
	}


	depthPixelsRaw = depthPixelsRawBack;
	videoPixels = videoPixelsBack;


	updateDepthPixels();


	depthTex.loadData(depthPixels.getPixels(), width, height, GL_LUMINANCE);
	videoTex.loadData(videoPixels.getPixels(), width, height, bIsVideoInfrared ? GL_LUMINANCE : GL_RGB);
}

//------------------------------------
float ofxRSSDK::getDistanceAt(int x, int y) {
	return depthPixelsRaw[y * width + x];
}

//------------------------------------
float ofxRSSDK::getDistanceAt(const ofPoint & p) {
	return getDistanceAt(p.x, p.y);
}

//------------------------------------
ofVec3f ofxRSSDK::getWorldCoordinateAt(int x, int y) {
	return getWorldCoordinateAt(x, y, getDistanceAt(x, y));
}

//------------------------------------
ofVec3f ofxRSSDK::getWorldCoordinateAt(float cx, float cy, float wz)
{
	double wx, wy;
	//freenect_camera_to_world(kinectDevice, cx, cy, wz, &wx, &wy);
	return ofVec3f(wx, wy, wz);
}

//------------------------------------
ofColor ofxRSSDK::getColorAt(int x, int y)
{
	int index = (y * width + x) * videoBytesPerPixel;
	ofColor c;
	c.r = videoPixels[index + 0];
	c.g = videoPixels[index + (videoBytesPerPixel - 1) / 2];
	c.b = videoPixels[index + (videoBytesPerPixel - 1)];
	c.a = 255;

	return c;
}

//------------------------------------
ofColor ofxRSSDK::getColorAt(const ofPoint & p) {
	return getColorAt(p.x, p.y);
}

//---------------------------------------------------------------------------
unsigned char * ofxRSSDK::getPixels() {
	return videoPixels.getPixels();
}

//---------------------------------------------------------------------------
unsigned char * ofxRSSDK::getDepthPixels() {
	return depthPixels.getPixels();
}

//---------------------------------------------------------------------------
unsigned short * ofxRSSDK::getRawDepthPixels() {
	return depthPixelsRaw.getPixels();
}

//---------------------------------------------------------------------------
float* ofxRSSDK::getDistancePixels() {
	return distancePixels.getPixels();
}

ofPixels & ofxRSSDK::getPixelsRef(){
	return videoPixels;
}

ofPixels & ofxRSSDK::getDepthPixelsRef(){
	return depthPixels;
}

ofShortPixels & ofxRSSDK::getRawDepthPixelsRef(){
	return depthPixelsRaw;
}

ofFloatPixels & ofxRSSDK::getDistancePixelsRef()
{
	return distancePixels;
}

//------------------------------------
ofTexture& ofxRSSDK::getTextureReference()
{
	if (!videoTex.bAllocated()){
		ofLog(OF_LOG_WARNING, "ofxRSSDK: Video texture is not allocated");
	}
	return videoTex;
}

//---------------------------------------------------------------------------
ofTexture& ofxRSSDK::getDepthTextureReference()
{
	if (!depthTex.bAllocated()){
		ofLog(OF_LOG_WARNING, "ofxRSSDK: Depth texture is not allocated");
	}
	return depthTex;
}

//---------------------------------------------------------------------------
void ofxRSSDK::enableDepthNearValueWhite(bool bEnabled) {
	bNearWhite = bEnabled;
	updateDepthLookupTable();
}

//---------------------------------------------------------------------------
bool ofxRSSDK::isDepthNearValueWhite() {
	return bNearWhite;
}

//---------------------------------------------------------------------------
void ofxRSSDK::setDepthClipping(float nearClip, float farClip) {
	nearClipping = nearClip;
	farClipping = farClip;
	updateDepthLookupTable();
}

//---------------------------------------------------------------------------
float ofxRSSDK::getNearClipping() {
	return nearClipping;
}

//---------------------------------------------------------------------------
float ofxRSSDK::getFarClipping() {
	return farClipping;
}

//------------------------------------
void ofxRSSDK::setUseTexture(bool bUse){
	bUseTexture = bUse;
}

//----------------------------------------------------------
void ofxRSSDK::draw(float _x, float _y, float _w, float _h) {
	if (bUseTexture && bGrabVideo) {
		videoTex.draw(_x, _y, _w, _h);
	}
}

//----------------------------------------------------------
void ofxRSSDK::draw(float _x, float _y) {
	draw(_x, _y, (float)width, (float)height);
}

//----------------------------------------------------------
void ofxRSSDK::draw(const ofPoint & point) {
	draw(point.x, point.y);
}

//----------------------------------------------------------
void ofxRSSDK::draw(const ofRectangle & rect) {
	draw(rect.x, rect.y, rect.width, rect.height);
}

//----------------------------------------------------------
void ofxRSSDK::drawDepth(float _x, float _y, float _w, float _h) {
	if (bUseTexture) {
		depthTex.draw(_x, _y, _w, _h);
	}
}

//---------------------------------------------------------------------------
void ofxRSSDK::drawDepth(float _x, float _y) {
	drawDepth(_x, _y, (float)width, (float)height);
}

//----------------------------------------------------------
void ofxRSSDK::drawDepth(const ofPoint & point) {
	drawDepth(point.x, point.y);
}

//----------------------------------------------------------
void ofxRSSDK::drawDepth(const ofRectangle & rect) {
	drawDepth(rect.x, rect.y, rect.width, rect.height);
}

//----------------------------------------------------------
float ofxRSSDK::getHeight() {
	return (float)height;
}

//---------------------------------------------------------------------------
float ofxRSSDK::getWidth() {
	return (float)width;
}

//---------------------------------------------------------------------------
void ofxRSSDK::updateDepthLookupTable() {
	unsigned char nearColor = bNearWhite ? 255 : 0;
	unsigned char farColor = bNearWhite ? 0 : 255;
	unsigned int maxDepthLevels = 10000;
	depthLookupTable.resize(maxDepthLevels);
	depthLookupTable[0] = 0;
	for (int i = 1; i < maxDepthLevels; i++) {
		depthLookupTable[i] = ofMap(i, nearClipping, farClipping, nearColor, farColor, true);
	}
}

//----------------------------------------------------------
void ofxRSSDK::updateDepthPixels() {
	int n = width * height;
	for (int i = 0; i < n; i++) {
		distancePixels[i] = depthPixelsRaw[i];
	}
	for (int i = 0; i < n; i++) {
		depthPixels[i] = depthLookupTable[depthPixelsRaw[i]];
	}
}

