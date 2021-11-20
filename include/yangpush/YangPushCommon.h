#ifndef INCLUDE_YANGPUSH_YANGPUSHCOMMON_H_
#define INCLUDE_YANGPUSH_YANGPUSHCOMMON_H_


#define Yang_VideoSrc_Camera 0
#define Yang_VideoSrc_Screen 1
#define Yang_VideoSrc_OutInterface 2
enum YangPushMessageType {
	YangM_Push_StartAudioCapture,
	YangM_Push_StartVideoCapture,
	YangM_Push_StartScreenCapture,
	YangM_Push_Publish_Start,
	YangM_Push_Publish_Stop,
	YangM_Push_SwitchToCamera,
	YangM_Push_SwitchToScreen
};

#endif /* INCLUDE_YANGPUSH_YANGPUSHCOMMON_H_ */
