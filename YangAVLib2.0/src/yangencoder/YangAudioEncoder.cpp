#include "yangencoder/YangAudioEncoder.h"

#include <stdio.h>
#include "unistd.h"

YangAudioEncoder::YangAudioEncoder() {
	m_isInit=0;
	m_uid=-1;
	memset(&m_audioInfo,0,sizeof(YangAudioInfo));

}
YangAudioEncoder::~YangAudioEncoder(void) {
	//m_ini = NULL;
}

void YangAudioEncoder::setAudioPara(YangAudioInfo *pap){
	memcpy(&m_audioInfo,pap,sizeof(YangAudioInfo));
}


