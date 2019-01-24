#include "video.h"
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <QDebug>
#include <QImage>
#include <QFile>
#include <Windows.h>

extern QSize getVideoResolution(const char* chVideo)
{
	AVFormatContext* m_inputAVFormatCxt = avformat_alloc_context();
	int m_videoStreamIndex;
	int m_coded_width, m_coded_height; //视频宽高
	int res = 0;
	if ((res = avformat_open_input(&m_inputAVFormatCxt, chVideo, 0, NULL)) < 0)
	{
		return QSize(0, 0);
	}
	if (avformat_find_stream_info(m_inputAVFormatCxt, 0) < 0)
	{
		return QSize(0, 0);
	}
	av_dump_format(m_inputAVFormatCxt, 0, chVideo, 0);
	for (int i = 0; i < m_inputAVFormatCxt->nb_streams; i++)
	{
		AVStream *in_stream = m_inputAVFormatCxt->streams[i];

		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videoStreamIndex = i;

			m_coded_width = in_stream->codecpar->width;
			m_coded_height = in_stream->codecpar->height;
			break;
		}
	}
	return QSize(m_coded_width, m_coded_height);
}

int writeFile(AVFrame* pFrame, int width, int height, int iIndex, const char* chThumbnail)
{
	AVFormatContext* pFormatCtx = avformat_alloc_context();

	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

	// 创建并初始化一个和该url相关的AVIOContext
	if (avio_open(&pFormatCtx->pb, chThumbnail, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("Couldn't open output file.");
		return -1;
	}

	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL)
	{
		return -1;
	}


	// 设置该stream的信息
	AVCodecContext* pCodecCtx = pAVStream->codec;

	pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		printf("Codec not found.");
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.");
		return -1;
	}


	//Write Header
	avformat_write_header(pFormatCtx, NULL);


	int y_size = pCodecCtx->width * pCodecCtx->height;


	//Encode
	// 给AVPacket分配足够大的空间
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);


	// 
	int got_picture = 0;
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
	if (ret < 0)
	{
		printf("Encode Error.\n");
		return -1;
	}
	printf("got_picture %d \n", got_picture);
	if (got_picture == 1)
	{
		//pkt.stream_index = pAVStream->index;
		ret = av_write_frame(pFormatCtx, &pkt);
	}
	
	av_packet_unref(&pkt);

	av_write_trailer(pFormatCtx);
	
	if (pAVStream)
	{
		avcodec_free_context(&pAVStream->codec);
	}

	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);


	return 0;
}

bool makeVideoThumbnail(const char* chVideo, const char *chThumbnail)
{
	int videoStream = -1;
	AVCodecContext *pCodecCtx;
	AVFormatContext *pFormatCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
	AVPacket packet;

	av_register_all();

	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, chVideo, NULL, NULL) != 0)
	{
		return false;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		return false;
	}

	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1)
	{
		return false;
	}

	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

	if (pCodec == NULL)
	{
		return false;
	}

	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		return false;
	}

	//为每帧图像分配内存
	pFrame = av_frame_alloc();

	int i = 0;

	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		avcodec_send_packet(pCodecCtx, &packet);
		if (packet.stream_index == videoStream)
		{
			avcodec_receive_frame(pCodecCtx, pFrame);
			writeFile(pFrame, pCodecCtx->width, pCodecCtx->height, i++, chThumbnail);
			break;
		}

		av_packet_unref(&packet);
	}

	av_frame_free(&pFrame);
	avformat_close_input(&pFormatCtx);

	return true;
}
