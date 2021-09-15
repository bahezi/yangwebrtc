#include "yangrecordthread.h"
#include <QDebug>
#include <QMapIterator>
//#include "../yangutil/yangmeetingtype.h"
YangRecordThread::YangRecordThread()
{
    // m_ys=nullptr;
    m_isLoop=0;
    m_video=nullptr;
    m_videoBuffer=nullptr;
  //  delState=0;
    m_bgColor={0,0,0};
    m_textColor={0,0,255};

    m_videoPlayNum=5;
    m_sid=1;
    showType=1;

    m_isStart=0;
}

YangRecordThread::~YangRecordThread(){
    // m_ys=nullptr;
    // m_vb=NULL;
      m_video=nullptr;
      m_videoBuffer=nullptr;
  //  m_vb=NULL;

   stopAll();
}
void YangRecordThread::stopAll(){
    if(m_isLoop){
        m_isLoop=0;
        while (m_isStart) {
            QThread::msleep(1);
        }
    }
     closeAll();
}
void YangRecordThread::initPara(YangContext *pini){
   // m_ini=pini;
    m_para=pini;


    m_videoPlayNum=pini->video.videoPlayCacheNum;
}
void YangRecordThread::closeAll(){
    //clearRender();
}


void YangRecordThread::render(){


        if(m_videoBuffer&&m_videoBuffer->m_size>0){

                uint8_t* t_vb=m_videoBuffer->getVideoRef(&m_frame);

                    if(t_vb&&m_video){
                        m_video->PlayOneFrame(t_vb,m_videoBuffer->m_width,m_videoBuffer->m_height);
                    }

                    t_vb=NULL;

        }



}

void YangRecordThread::run(){
    // init();

    m_isLoop=1;
    m_isStart=1;

    while(m_isLoop){

        QThread::msleep(20); 
        render();
    }
    m_isStart=0;
   // closeAll();
}