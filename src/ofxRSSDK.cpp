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

	mRealSenseDevice = NULL;

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

	bCalcCameraPoints = false;
	setDepthClipping();
}

//--------------------------------------------------------------------
ofxRSSDK::~ofxRSSDK()
{
	close();
	clear();
}

//--------------------------------------------------------------------
bool ofxRSSDK::init(bool pGrabVideo, bool pUseTexture)
{
	if (isConnected()) {
		ofLog(OF_LOG_WARNING, "ofxRSSDK: Do not call init while ofxRSSDK is running!");
		return false;
	}

	clear();
	bGrabVideo = pGrabVideo;
	bUseTexture = pUseTexture;
	videoBytesPerPixel = 4;

	// allocate
	depthPixelsRaw.allocate(width, height, 1);
	//depthPixelsRawBack.allocate(width, height, 1);

	videoPixels.allocate(width, height, videoBytesPerPixel);
	//videoPixelsBack.allocate(width, height, videoBytesPerPixel);

	depthPixels.allocate(width, height, 4);
	distancePixels.allocate(width, height, 1);

	// set
	depthPixelsRaw.set(0);
	//depthPixelsRawBack.set(0);

	videoPixels.set(0);
	//videoPixelsBack.set(0);

	depthPixels.set(0);
	distancePixels.set(0);

	if (bUseTexture)
	{
		depthTex.allocate(width, height, GL_RGBA);
		videoTex.allocate(width, height, GL_RGBA);
	}

	bGrabberInited = true;
	mRealSenseDevice = PXCSenseManager::CreateInstance();
	if (!mRealSenseDevice)
		bGrabberInited = false;

	if (bGrabVideo)
		bGrabberInited = mRealSenseDevice->EnableStream(PXCCapture::STREAM_TYPE_COLOR, width, height, 30) >= PXC_STATUS_NO_ERROR;
	bGrabberInited = mRealSenseDevice->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, width, height, 30) >= PXC_STATUS_NO_ERROR;
	bGrabberInited = mRealSenseDevice->Init() >= PXC_STATUS_NO_ERROR;

	mProjection = mRealSenseDevice->QueryCaptureManager()->QueryDevice()->CreateProjection();
	mDepthBuffer = new uint16_t[width*height];
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
	//depthPixelsRawBack.clear();

	videoPixels.clear();
	//videoPixelsBack.clear();

	depthPixels.clear();
	distancePixels.clear();

	depthTex.clear();
	videoTex.clear();

	mCameraPoints.clear();

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
	if (mRealSenseDevice)
		mRealSenseDevice->Close();
}

//---------------------------------------------------------------------------
bool ofxRSSDK::isConnected()
{
	if (mRealSenseDevice!=nullptr)
		return mRealSenseDevice->IsConnected();
	return false;
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


	if (mRealSenseDevice->AcquireFrame(false, 0) >= PXC_STATUS_NO_ERROR)
	{
		PXCCapture::Sample *cSample = mRealSenseDevice->QuerySample();
		if (!cSample)
			return;

		if (bGrabVideo)
		{
			PXCImage *cRgbImage = cSample->color;
			if (cRgbImage)
			{
				PXCImage::ImageData cRgbData;
				if (cRgbImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &cRgbData) >= PXC_STATUS_NO_ERROR)
				{
					uint8_t *cBuffer = cRgbData.planes[0];
					videoPixels.setFromPixels(cBuffer, width, height, 4);

					cRgbImage->ReleaseAccess(&cRgbData);
				}
			}
		}

		PXCImage *cDepthImage = cSample->depth;
		if (cDepthImage)
		{
			PXCImage::ImageData cDepthData;
			if (cDepthImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_DEPTH, &cDepthData) >= PXC_STATUS_NO_ERROR)
			{
				depthPixelsRaw.setFromPixels(reinterpret_cast<uint16_t *>(cDepthData.planes[0]), width, height, 1);
				memcpy(mDepthBuffer, reinterpret_cast<uint16_t *>(cDepthData.planes[0]), (size_t)(width*height*sizeof(uint16_t)));
				cDepthImage->ReleaseAccess(&cDepthData);
			}

			if (cDepthImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_DEPTH_F32, &cDepthData) >= PXC_STATUS_NO_ERROR)
			{
				distancePixels.setFromPixels(reinterpret_cast<float *>(cDepthData.planes[0]), width, height, 1);
				cDepthImage->ReleaseAccess(&cDepthData);
			}

			if (cDepthImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &cDepthData) >= PXC_STATUS_NO_ERROR)
			{
				depthPixels.setFromPixels(cDepthData.planes[0], width, height, 4);
				cDepthImage->ReleaseAccess(&cDepthData);
			}
		}

		mRealSenseDevice->ReleaseFrame();
	}

	//updateDepthPixels();
	depthTex.loadData(depthPixels.getPixels(), width, height, GL_RGBA);
	videoTex.loadData(videoPixels.getPixels(), width, height, GL_BGRA);
	if (bCalcCameraPoints)
		updateCameraPoints();
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
ofVec3f ofxRSSDK::getWorldCoordinateAt(int x, int y)
{
	return getWorldCoordinateAt(x, y, getDistanceAt(x, y));
}

//------------------------------------
ofVec3f ofxRSSDK::getWorldCoordinateAt(float cx, float cy, float wz)
{
	PXCPoint3DF32 cDepthPoint;
	cDepthPoint.x = cx; cDepthPoint.y = cy; cDepthPoint.z = wz;
	PXCPoint3DF32 cDepth[1]{cDepthPoint};
	PXCPoint3DF32 cCamera[1]{PXCPoint3DF32()};
	mProjection->ProjectDepthToCamera(1, cDepth, cCamera);
	
	return ofVec3f(cCamera[0].x, cCamera[0].y, cCamera[0].z);
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

ofPoint ofxRSSDK::getDepthFOV()
{
	PXCPointF32 cFOV = mRealSenseDevice->QueryCaptureManager()->QueryDevice()->QueryDepthFieldOfView();
	return ofPoint(cFOV.x, cFOV.y);
}

ofPoint ofxRSSDK::getColorFOV()
{
	PXCPointF32 cFOV = mRealSenseDevice->QueryCaptureManager()->QueryDevice()->QueryColorFieldOfView();
	return ofPoint(cFOV.x, cFOV.y);
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

void ofxRSSDK::updateCameraPoints()
{
	mCameraPoints.clear();
	vector<PXCPoint3DF32> cDepth, cCamera;
	for (int dy = 0; dy < height;++dy)
	{
		for (int dx = 0; dx < width; ++dx)
		{
			PXCPoint3DF32 cPoint;
			cPoint.x = dx; cPoint.y = dy; cPoint.z = (float)mDepthBuffer[dy*width + dx];
			cDepth.push_back(cPoint);
		}
	}

	//PXCPoint3DF32 *cCamera = new PXCPoint3DF32[cDepth.size()];
	cCamera.resize(cDepth.size());
	mProjection->ProjectDepthToCamera(cDepth.size(), &cDepth[0], &cCamera[0]);

	for (int i = 0; i < cDepth.size();++i)
	{
		PXCPoint3DF32 p = cCamera[i];
		mCameraPoints.push_back(ofVec3f(p.x, p.y, p.z));
	}
}

