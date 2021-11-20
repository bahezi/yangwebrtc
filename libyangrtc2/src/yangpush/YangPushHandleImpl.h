﻿#ifndef YANGPUSH_YANGPUSHHANDLEIMPL_H_
#define YANGPUSH_YANGPUSHHANDLEIMPL_H_
#include <yangpush/YangPushPublish.h>
#include <yangpush/YangPushHandle.h>
#include <yangpush/YangRtcPublish.h>
#include <yangutil/sys/YangUrl.h>

class YangPushHandleImpl :public YangPushHandle{
public:
	YangPushHandleImpl(bool hasAudio,bool initVideo,int pvideotype,YangVideoInfo* pvideo,YangContext* pcontext,YangSysMessageI* pmessage);
	virtual ~YangPushHandleImpl();
	void init();
	void startCapture();
    int publish(string url,string localIp,int32_t localport);
	void setScreenVideoInfo(YangVideoInfo* pvideo);
	void setScreenInterval(int32_t pinterval);
	YangVideoBuffer* getPreVideoBuffer();

	void disconnect();
	void switchToCamera(bool pisinit);
	void switchToScreen(bool pisinit);
	void setDrawmouse(bool isDraw);

private:
    void startCamera();
    void startScreen();
    void stopCamera();
    void stopScreen();
	void stopPublish();

private:
	bool m_hasAudio;
	int m_videoState;
	bool m_isInit;

	YangPushPublish* m_cap;
	YangRtcPublish* m_pub;
	YangContext* m_context;
	YangUrlData m_url;
	YangSysMessageI* m_message;

	YangVideoInfo* m_screenInfo;



};

#endif /* YANGPUSH_YANGPUSHHANDLEIMPL_H_ */
