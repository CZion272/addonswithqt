#pragma once

#include <MagickCore/MagickCore.h>
#include "quazip/quazipfile.h"
#include "quazip/quazip.h"
#include <QIODevice>
#include <QImageReader>
#include <QFileInfo>
#include <direct.h>

enum FILETYPE
{
	TYPE_NORMAL,
	TYPE_CDR,
	TYPE_OFFICE
};

class ImageObjcet
{
public:
	ImageObjcet(const char* image, const char* preview);
	~ImageObjcet();
public:
	QString MD5();
	void setDefalutMD5(QString strMD5);

	void setScallSize(int nWight, int nHeight);
	void setMiddleFile(QString strPath);

	bool pingImageFile();
	void readImage();
	bool readImageFile();
	bool readCdrPerviewFile();
	bool readPPT();
	bool readVideo();

	QList<QColor> getColorList();
	void setDefaultColorList(QList<QColor> lstColor);
	int getWigth();
	int getHeight();
	void setDefaultSize(int nWigth, int nHeight);
	bool compareColorEx(QColor color, int nDiff = 10000);

	QString getLastError();
	static void setFFMpeg(QString str);
private:
	QString m_strImage;
	QImage m_imgImage;
	QString m_strPreview;
	QImage m_imgPreview;
	QList<QColor> m_lstColor;
	QString m_strMd5;
	QString m_strMiddleFile;
	QString m_strSufix;
	QString m_strImageMgickError;
	QString m_strVideo;

	int m_nHeight;
	int m_nWight;
	int m_nScaleHeight;
	int m_nScaleWight;
	float m_fRatio;

	int m_nColorCount;
};

