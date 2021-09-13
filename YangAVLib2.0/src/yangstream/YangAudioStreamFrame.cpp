#include <stdio.h>
#include <memory.h>
#include <yangstream/YangAudioStreamFrame.h>
YangAudioStreamFrame::YangAudioStreamFrame() {
	m_audioBufs = new uint8_t[1024 * 8];
	m_audioBuffer = m_audioBufs + 20;
	m_audioHeaderLen = 1;
	atime = 0;
	atime1 = 0;
	perSt = 0;
	m_src = NULL;
	m_srcLen = 0;
	m_transType = 0;
	m_frametype=1;
	m_audioLen=0;
	m_audioType=Yang_AED_AAC;
}

YangAudioStreamFrame::~YangAudioStreamFrame() {
	m_audioBuffer = NULL;
	delete[] m_audioBufs;
	m_audioBufs = NULL;

	m_src = NULL;
}
void YangAudioStreamFrame::init(int32_t ptranstype, int32_t sample, int32_t channel,
		YangAudioEncDecType audioType) {
	m_transType = ptranstype;
	m_audioType=audioType;
	if (audioType == Yang_AED_MP3)
		perSt = 1152 * 1000 / sample;
	if (audioType == Yang_AED_AAC) {
		m_audioHeaderLen = 2;
		m_audioBuffer[0] = 0xaf;
		m_audioBuffer[1] = 0x01;
		perSt = 1024 * 1000 / sample;
	} else {
		m_audioHeaderLen = 1;
		m_audioBuffer[0] = 0xbe;

		if (audioType != Yang_AED_MP3) {
			int32_t sam = 320;
			perSt = sam * 1000 / 16000;
		}
	}
}

void YangAudioStreamFrame::setFrametype(int32_t frametype){
	m_frametype=frametype;
}
void YangAudioStreamFrame::setAudioData(YangFrame* audioFrame) {

	if (m_transType == 2) {
		m_src = audioFrame->payload;
		m_srcLen = audioFrame->nb;
		m_audioLen = audioFrame->nb;

	} else {
		memcpy(m_audioBuffer + m_audioHeaderLen, audioFrame->payload, audioFrame->nb);
		m_audioLen=audioFrame->nb + m_audioHeaderLen;

	}

	atime1 += perSt;		//10240*t_frames/441;
	atime = (int) atime1;

}
void YangAudioStreamFrame::setAudioMetaData(uint8_t* p,int32_t plen){
	if (m_transType == 2) {
		m_src = p;
		m_srcLen = plen;
		m_audioLen = plen;

	} else {
		memcpy(m_audioBuffer + m_audioHeaderLen, p, plen);
		m_audioLen=plen + m_audioHeaderLen;
	}
}
uint8_t* YangAudioStreamFrame::getAudioData() {
	return m_transType == 2 ? m_src : m_audioBuffer;
}
int64_t YangAudioStreamFrame::getRtmpTimestamp() {
	return atime;
}
int64_t YangAudioStreamFrame::getTsTimestamp() {
	return atime * 90;
}

int64_t YangAudioStreamFrame::getTimestamp() {
	return m_transType==0 ? atime : atime * 90;
}
int32_t YangAudioStreamFrame::getAudioLen(){
	return m_audioLen;
}
int32_t YangAudioStreamFrame::getFrametype(){
	return m_frametype;
}
YangAudioEncDecType YangAudioStreamFrame::getAudioType(){
	return m_audioType;
}
