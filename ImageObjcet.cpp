#include "ImageObjcet.h"
#include <QDebug>
#include "qmath.h"
#include <QImageWriter>
#include <QBuffer>
#include <QTimer>
#include <QCryptographicHash>
#include <QTemporaryFile>
#include <QThread>
#include <QFile>
#include <QProcess>
#include "office.h"
#include "video.h"


static double gR = 100;
static double gAngle = 30;
static double gh = gR * qCos((float)gAngle / 180 * M_PI);
static double gr = gR * qSin((float)gAngle / 180 * M_PI);

static QString m_strFFMpeg;


static int HistogramCompare(const void *x, const void *y)
{
	const PixelInfo
		*color_1,
		*color_2;

	color_1 = (const PixelInfo *)x;
	color_2 = (const PixelInfo *)y;
	return((int)color_2->count - (int)color_1->count);
}

ImageObjcet::ImageObjcet(const char* image, const char* preview) :
	m_strImage(image),
	m_strPreview(preview),
	m_nHeight(0),
	m_nWight(0),
	m_nScaleWight(0),
	m_nScaleHeight(0),
	m_fRatio(0),
	m_strMiddleFile(""),
	m_nColorCount(0),
	m_strImageMgickError("")
{
	m_strSufix = QFileInfo(m_strImage).suffix().toLower();
}


ImageObjcet::~ImageObjcet()
{
}

QString ImageObjcet::MD5()
{
	QFile file(m_strImage);
	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray ba = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5);

		file.close();
		QString md5 = ba.toHex();
		m_strMd5 = md5;
	}

	return m_strMd5;
}

void ImageObjcet::setDefalutMD5(QString strMD5)
{
	m_strMd5 = strMD5;
}

void ImageObjcet::setScallSize(int nWight, int nHeight)
{
	m_nScaleWight = nWight;
	m_nScaleHeight = nHeight;
}

void ImageObjcet::setMiddleFile(QString strPath)
{
	m_strMiddleFile = strPath;
}

bool ImageObjcet::pingImageFile()
{
	ExceptionInfo *exception;

	exception = AcquireExceptionInfo();
	ImageInfo *imageInfo;
	imageInfo = CloneImageInfo(NULL);
	strcpy(imageInfo->filename, m_strImage.toStdString().c_str());
	imageInfo->number_scenes = 1;
	Image *images = PingImage(imageInfo, exception);
	if (images == NULL)
	{
		if (m_strFFMpeg.isEmpty())
		{
			return false;
		}

		QSize size = getVideoResolution(m_strImage.toLocal8Bit().constData());
		if (size.isEmpty())
		{
			return false;
		}

		m_nHeight = size.height();
		m_nWight = size.width();
	}
	else
	{
		m_nHeight = images->magick_rows;
		m_nWight = images->magick_columns;
		images = DestroyImage(images);
	}

	imageInfo = DestroyImageInfo(imageInfo);
	exception = DestroyExceptionInfo(exception);
	return true;
}

void ImageObjcet::readImage()
{
	bool bReadSave = false;
	FILETYPE type = TYPE_NORMAL;
	if (m_strSufix == "cdr")
	{
		type = TYPE_CDR;
	}
	else if (
		m_strSufix == "ppt"
		|| m_strSufix == "pptx"
		)
	{
		type = TYPE_OFFICE;
	}
	switch (type)
	{
	case TYPE_CDR:
	{
		bReadSave = readCdrPerviewFile();
	}
	case TYPE_OFFICE:
	{
		bReadSave = readPPT();
	}
	default:
		bReadSave = readImageFile();
		break;
	}

	if (!bReadSave)
	{
		bReadSave = readVideo();
	}
	if (bReadSave)
	{
		m_strImageMgickError = "";
	}
}

bool ImageObjcet::readImageFile()
{
	size_t number_formats;

	ExceptionInfo *exception;
	exception = AcquireExceptionInfo();

	ImageInfo *imageInfo;
	ImageInfo *thumbnailsInfo;

	imageInfo = AcquireImageInfo();
	thumbnailsInfo = CloneImageInfo(NULL);

	Image *images;
	Image *thumbnails;

#ifdef DEBUG
	strcpy(imageInfo->filename, "0.svg");
	strcpy(thumbnailsInfo->filename, "0.png");
#else
	strcpy(imageInfo->filename, m_strImage.toStdString().c_str());
	strcpy(thumbnailsInfo->filename, m_strPreview.toStdString().c_str());
#endif // DEBUG
	//if (m_strSufix == "psd")
	{
		imageInfo->number_scenes = 1;
	}
	images = ReadImage(imageInfo, exception);
	bool bTemp = false;
	QString tempFile;

	if (images == NULL)
	{
		m_strImageMgickError = exception->reason;
		return false;
	}

	thumbnails = NewImageList();

	//if ((image = RemoveFirstImageFromList(&images)) != (Image *)NULL)
	{
		m_nHeight = images->magick_rows;
		m_nWight = images->magick_columns;

		if (m_nHeight == 0 || m_nWight == 0)
		{
			return false;
		}

		if (m_nHeight > m_nWight)
		{
			if (m_nScaleHeight != 0)
			{
				m_fRatio = (float)m_nScaleHeight / (float)m_nHeight;
			}
			else
			{
				m_fRatio = (float)m_nScaleWight / (float)m_nWight;
			}
		}
		else
		{
			if (m_nScaleWight != 0)
			{
				m_fRatio = (float)m_nScaleWight / (float)m_nWight;
			}
			else
			{
				m_fRatio = (float)m_nScaleHeight / (float)m_nHeight;
			}
		}
		if (m_fRatio == 0.00)
		{
			m_fRatio = 1.0;
		}

		thumbnails = ThumbnailImage(images, m_nWight * m_fRatio, m_nHeight * m_fRatio, exception);
		if (thumbnails == NULL)
		{
			m_strImageMgickError = exception->reason;
			images = DestroyImageList(images);
			thumbnails = DestroyImageList(thumbnails);
			imageInfo = DestroyImageInfo(imageInfo);
			thumbnailsInfo = DestroyImageInfo(thumbnailsInfo);
			exception = DestroyExceptionInfo(exception);
			return false;
		}


		if (!m_strMiddleFile.isEmpty())
		{
			if (
				m_strSufix != "cdr"
				)
			{
				Image *image = NewImageList();
				ImageInfo *midInfo = CloneImageInfo(imageInfo);
				midInfo->quality = 10;

				image = ThumbnailImage(images, m_nWight, m_nHeight, exception);

				(void)strcpy(image->filename, m_strMiddleFile.toStdString().c_str());
				(void)strcpy(midInfo->filename, m_strMiddleFile.toStdString().c_str());

				WriteImage(midInfo, image, exception);

				image = DestroyImage(image);
				midInfo = DestroyImageInfo(midInfo);
			}
		}
	}

	size_t nColors = 0;
	/*
	  Write the image thumbnail.
	*/
	(void)strcpy(thumbnails->filename, m_strPreview.toStdString().c_str());
	thumbnailsInfo->quality = 10;
	bool b = WriteImage(thumbnailsInfo, thumbnails, exception);

	if (m_strSufix != "ai"
		|| m_strSufix != "svg"
		|| m_strSufix != "png")
	{
		QuantizeInfo *quantize = AcquireQuantizeInfo(thumbnailsInfo);
		quantize->number_colors = 10;
		CompressImageColormap(thumbnails, exception);
		QuantizeImage(quantize, thumbnails, exception);
	}
	PixelInfo *p = GetImageHistogram(thumbnails, &nColors, exception);
	PixelInfo *pCopy = ClonePixelInfo(p);
	QMap<QString, int> mapColor;
	QList<QColor> lstColor;
	QList<int> lstColorNumber;

	qsort((void *)p, (size_t)nColors, sizeof(*p), HistogramCompare);
	PixelInfo pixel;
	GetPixelInfo(thumbnails, &pixel);
	m_nColorCount = nColors;
	int nDiff = nColors < 10 ? 5000 : 20000;
	for (int i = 0; i < (ssize_t)nColors; i++)
	{
		pixel = (*p);
		char hex[MAX_PATH] = { 0 };
		GetColorTuple(&pixel, MagickTrue, hex);
		QString strHex(hex);
		strHex = strHex.left(7);
		if (!compareColorEx(QColor(strHex), nDiff))
		{
			//if (m_lstColor.indexOf(QColor(strHex)) == -1)
			{
				m_lstColor.append(QColor(strHex));
			}
		}
		p++;
	}

	images = DestroyImageList(images);
	thumbnails = DestroyImageList(thumbnails);
	imageInfo = DestroyImageInfo(imageInfo);
	thumbnailsInfo = DestroyImageInfo(thumbnailsInfo);
	m_strImageMgickError = exception->reason;
	exception = DestroyExceptionInfo(exception);
	if (bTemp)
	{
		QFile::remove(tempFile);
	}
	return b;
}

bool ImageObjcet::readCdrPerviewFile()
{
	QuaZip zip(m_strImage);
	zip.open(QuaZip::mdUnzip);
	// first, we need some information about archive itself
	QString comment = zip.getComment();
	// and now we are going to access files inside it
	QuaZipFile file(&zip);
	QString strFileToSave = m_strMiddleFile.isEmpty() ? m_strPreview : m_strMiddleFile;
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
	{
		if (zip.getCurrentFileName().contains("thumbnail"))
		{
			file.open(QIODevice::ReadOnly);

			QByteArray imgByte = file.readAll();
			QImage img;
			img.loadFromData(imgByte);
			m_nWight = img.width();
			m_nHeight = img.height();
			//img = img.scaled(img.width(), img.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
			img.save(strFileToSave);
			// do something cool with file here
			file.close(); // do not forget to close!
			m_strImage = strFileToSave;
			return readImageFile();
		}
	}
	m_strImageMgickError = "invaild cdr file";
	return false;
}

bool ImageObjcet::readPPT()
{
	bool b = false;
	PPT2Image(m_strImage.toLocal8Bit(), "tempppt.png");
	if (QFile("tempppt.png").exists())
	{
		m_strImage = "tempppt.png";
		b = readImageFile();
		QFile("tempppt.png").remove();
	}
	return b;
}

bool ImageObjcet::readVideo()
{
	makeVideoThumbnail(m_strImage.toLocal8Bit(), "temp.jpg");
	bool b = false;
	if (QFile("temp.jpg").exists())
	{
		m_strImage = "temp.jpg";
		b = readImageFile();
		QFile("temp.jpg").remove();
	}
	return b;
}

QList<QColor> ImageObjcet::getColorList()
{
	return m_lstColor;
}

void ImageObjcet::setDefaultColorList(QList<QColor> lstColor)
{
	m_lstColor = lstColor;
}

int ImageObjcet::getWigth()
{
	return m_nWight;
}

int ImageObjcet::getHeight()
{
	return m_nHeight;
}

void ImageObjcet::setDefaultSize(int nWigth, int nHeight)
{
	m_nWight = nWigth;
	m_nHeight = nHeight;
}

bool ImageObjcet::compareColorEx(QColor color, int nDiff /*= 10000*/)
{
	int h = 0, s = 0, v = 0;
	QColor hsvColor = color.toHsv();
	hsvColor.getHsv(&h, &s, &v);
	if (m_lstColor.count() == 0)
	{
		return false;
	}
	foreach(QColor colorCompare, m_lstColor)
	{
		int hc = 0, sc = 0, vc = 0;
		hsvColor = colorCompare.toHsv();
		hsvColor.getHsv(&hc, &sc, &vc);

		double x1 = gr * v * s * qCos((float)h / 180 * M_PI);
		double y1 = gr * v * s * qSin((float)h / 180 * M_PI);
		double z1 = gh * (1 - v);

		double x2 = gr * vc * sc * qCos((float)hc / 180 * M_PI);
		double y2 = gr * vc * sc * qSin((float)hc / 180 * M_PI);
		double z2 = gh * (1 - vc);

		double dx = x1 - x2;
		double dy = y1 - y2;
		double dz = z1 - z2;

		qreal rCom = qSqrt((dx * dx + dy * dy + dz * dz));
		if ((int)rCom < nDiff)
		{
			return true;
		}
	}
	return false;
}

QString ImageObjcet::getLastError()
{
	return m_strImageMgickError;
}

void ImageObjcet::setFFMpeg(QString str)
{
	m_strFFMpeg = str;
}
