#ifndef SRC_YANGPLAYER_SRC_YANGRTCRECEIVE_H_
#define SRC_YANGPLAYER_SRC_YANGRTCRECEIVE_H_
#include <yangutil/yangavinfotype.h>
#include <string>
#include "yangutil/buffer/YangAudioEncoderBuffer.h"
#include "yangutil/buffer/YangVideoDecoderBuffer.h"
#include "yangwebrtc/YangRtcHandle.h"
#include "yangutil/sys/YangThread.h"

using namespace std;
class YangRtcReceive : public YangThread, public YangReceiveCallback{
public:
	YangRtcReceive();
	virtual ~YangRtcReceive();
	void receiveAudio(YangFrame* audioFrame);
	void receiveVideo(YangFrame* videoFrame);
    int32_t init(int32_t puid,string localIp,int32_t localPort, string server, int32_t pport,string app,	string stream);
    void setBuffer(YangAudioEncoderBuffer *al,YangVideoDecoderBuffer *vl);
    void disConnect();
    void play(char* pserverStr,char *streamName);
    YangRtcHandle *m_recv;
  	int32_t isReceived; //,isHandled;
	int32_t isReceiveConvert; //,isHandleAllInvoke;
	int32_t m_isStart;
	void stop();
protected:
	void run();
	void startLoop();



private:
	pthread_mutex_t m_lock;
	pthread_cond_t m_cond_mess;
	YangStreamConfig m_conf;
	int32_t m_waitState;
	int32_t m_headLen;
	YangAudioEncoderBuffer *m_out_audioBuffer;
	YangVideoDecoderBuffer *m_out_videoBuffer;

    //uint8_t* m_keyBuf;
};

#endif /* SRC_YANGPLAYER_SRC_YANGRTCRECEIVE_H_ */