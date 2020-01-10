#include "ofApp.h"

void ofApp::setup() 
{
	ofSetVerticalSync(false);
	wai = make_unique<ofxWrnchAI>("");
	//vid.load("face.mov");
	//vid.setLoopState(OF_LOOP_NORMAL);
	//vid.play();
	vid.setDeviceID(0);
	vid.setDesiredFrameRate(60);
	vid.initGrabber(1280, 720);
	ofSetWindowShape(vid.getWidth(), vid.getHeight());
}
void ofApp::update() 
{
	vid.update();

	persons.clear();
	wai->update(vid.getPixels());
	//persons = wai->get_structured_data(vid.getWidth(), vid.getHeight());
}
void ofApp::draw() 
{
	vid.draw(0, 0);
	wai->debug_draw(vid.getWidth(), vid.getHeight());

	//for (auto p : persons)
	//{
	//	ofPushStyle();
	//	// body
	//	ofSetColor(ofColor::yellow);
	//	for (auto b : p.body.bones)
	//		ofDrawLine(b.from.p, b.to.p);
	//	ofSetColor(ofColor::green);
	//	for (auto j : p.body.joints)
	//	{
	//		ofDrawCircle(j.p, 3);
	//		ofDrawBitmapString(j.name, j.p);
	//	}
	//	// hand
	//	for (auto h : p.hands)
	//	{
	//		if (h.left_or_right == 0)
	//			ofSetColor(ofColor::magenta);
	//		else
	//			ofSetColor(ofColor::cyan);
	//		for (auto b : h.bones)
	//			ofDrawLine(b.from.p, b.to.p);
	//		if (h.left_or_right == 0)
	//			ofSetColor(ofColor::cyan);
	//		else
	//			ofSetColor(ofColor::magenta);
	//		for (auto j : h.joints)
	//		{
	//			ofDrawCircle(j.p, 3);
	//			ofDrawBitmapString(j.name, j.p);
	//		}
	//	}
	//	// face
	//	ofSetColor(ofColor::red);
	//	for (auto b : p.face.bones)
	//		ofDrawLine(b.from.p, b.to.p);
	//	ofSetColor(ofColor::blue);
	//	for (auto j : p.face.joints)
	//	{
	//		ofDrawCircle(j.p, 3);
	//		ofDrawBitmapString(j.name, j.p);
	//	}
	//	ofPopStyle();
	//}

	ofDrawBitmapStringHighlight("fps:" + ofToString(ofGetFrameRate(), 2), 10, ofGetHeight() - 20);
}
void ofApp::keyPressed(int key) {}
void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}
void ofApp::mouseDragged(int x, int y, int button) {}
void ofApp::mousePressed(int x, int y, int button) {}
void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::gotMessage(ofMessage msg) {}
void ofApp::dragEvent(ofDragInfo dragInfo) {}