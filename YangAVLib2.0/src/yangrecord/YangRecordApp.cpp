/*
 * YangRecZbHandle.cpp
 *
 *  Created on: 2020年10月8日
 *      Author: yang
 */

#include <yangrecord/YangRecordApp.h>
#include <unistd.h>
YangRecordApp::YangRecordApp(YangAudioInfo *paudio,YangVideoInfo *pvideo,YangVideoEncInfo *penc) {
	m_audio=paudio;
		m_video=pvideo;
		m_enc=penc;
		m_video->videoCacheNum=m_video->videoCacheNum*2;
		m_audio->audioCacheNum=m_audio->audioCacheNum*2;
	m_cap=NULL;
	m_rec=NULL;

}

YangRecordApp::~YangRecordApp() {

	yang_delete(m_cap);
	yang_delete(m_rec);
	m_audio=NULL;
				m_video=NULL;
				m_enc=NULL;
}


void YangRecordApp::init(){
	if(!m_cap){
		m_cap=new YangRecordCapture(m_audio,m_video);
		m_cap->initAudio(NULL);
		m_cap->initVideo();
		m_cap->startAudioCapture();
		m_cap->startVideoCapture();
	}
	if(!m_rec){
		m_rec=new YangMp4FileApp(m_audio,m_video,m_enc);
		m_rec->init();
		m_rec->setInAudioBuffer(m_cap->getOutAudioBuffer());
		m_rec->setInVideoBuffer(m_cap->getOutVideoBuffer());
		//m_rec->setFileTimeLen(1);
	}
}
void YangRecordApp::pauseRecord(){
	if(m_cap)	m_cap->startPauseCaptureState();
	if(m_rec) m_rec->pauseRecord();
}
void YangRecordApp::resumeRecord(){
	if(m_rec) m_rec->resumeRecord();
	if(m_cap) m_cap->stopPauseCaptureState();

}
void YangRecordApp::recordFile(char* filename){
	m_rec->startRecordMp4(filename,1,1);
	sleep(1);
	m_cap->startVideoCaptureState();
	m_cap->startAudioCaptureState();
}
void YangRecordApp::stopRecord(){
	m_cap->stopAudioCaptureState();
	m_cap->stopVideoCaptureState();
	m_cap->m_audioCapture->stop();
	m_cap->m_videoCapture->stop();
	m_rec->m_enc->m_ae->stop();
	m_rec->m_enc->m_ve->stop();
	//m_rec->m_enc->
	sleep(1);

	m_rec->stopRecordMp4();
}
