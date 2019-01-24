#pragma once
#include <QSize>
extern "C"
{
#include "libavcodec\avcodec.h"  
#include "libavformat\avformat.h"  
#include "libavutil\channel_layout.h"  
#include "libavutil\common.h"  
#include "libavutil\imgutils.h"  
#include "libswscale\swscale.h"   
#include "libavutil\imgutils.h"      
#include "libavutil\opt.h"         
#include "libavutil\mathematics.h"      
#include "libavutil\samplefmt.h"   
#include "libavfilter\avfilter.h"
};


QSize getVideoResolution(const char* chVideo);

bool makeVideoThumbnail(const char* chVideo, const char *chThumbnail);