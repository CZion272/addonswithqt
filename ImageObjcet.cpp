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
#include <Dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

LONG WINAPI lpUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    HANDLE hFile = CreateFile("Error.dmp", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return EXCEPTION_EXECUTE_HANDLER;

    MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = ExceptionInfo;
    mdei.ClientPointers = NULL;
    MINIDUMP_CALLBACK_INFORMATION mci;
    mci.CallbackRoutine = NULL;
    mci.CallbackParam = 0;

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, NULL, &mci);

    CloseHandle(hFile);

    return EXCEPTION_EXECUTE_HANDLER;
}



static double gR = 100;
static double gAngle = 30;
static double gh = gR * qCos((float)gAngle / 180 * M_PI);
static double gr = gR * qSin((float)gAngle / 180 * M_PI);

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
    __try
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
    __except (lpUnhandledExceptionFilter(GetExceptionInformation()))
    {
        return "";
    }
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
    __try
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
    __except (lpUnhandledExceptionFilter(GetExceptionInformation()))
    {
        return false;
    }
}

void ImageObjcet::readImage()
{
    __try
    {
        bool bReadSave = false;
        FILETYPE type = TYPE_NORMAL;
        if (m_strSufix.contains("cdr"))
        {
            type = TYPE_CDR;
        }
        else if (m_strSufix.contains("ppt"))
        {
            type = TYPE_OFFICE;
        }

        switch (type)
        {
        case TYPE_CDR:
        {
            bReadSave = readCdrPerviewFile();
            break;
        }
        case TYPE_OFFICE:
        {
            bReadSave = readPPT();
            break;
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
    __except (lpUnhandledExceptionFilter(GetExceptionInformation()))
    {
        m_strImageMgickError == "error";
    }
}

bool ImageObjcet::readImageFile()
{
    m_pIMException = AcquireExceptionInfo();

    m_pIMImageInfo = CloneImageInfo(NULL);
    m_pIMThumbnailsInfo = CloneImageInfo(NULL);
    m_pIMThumbnails = NULL;

#ifdef DEBUG
    strcpy(m_pIMImageInfo->filename, "0.svg");
    strcpy(m_pIMThumbnailsInfo->filename, "0.png");
#else
    strcpy(m_pIMImageInfo->filename, m_strImage.toStdString().c_str());
    strcpy(m_pIMThumbnailsInfo->filename, m_strPreview.toStdString().c_str());
#endif // DEBUG
    //if (m_strSufix == "psd")
    {
        m_pIMImageInfo->number_scenes = 1;
    }
    m_pIMImages = ReadImage(m_pIMImageInfo, m_pIMException);
    bool bTemp = false;
    QString tempFile;

    if (m_pIMImages == NULL)
    {
        m_strImageMgickError = m_pIMException->reason;
        releaseIM();
        return false;
    }

    {
        m_nHeight = m_pIMImages->magick_rows;
        m_nWight = m_pIMImages->magick_columns;

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
        if (m_fRatio == 0.00 || m_fRatio > 1.0)
        {
            m_fRatio = 1.0;
        }

        if (m_pIMImages->compression == RLECompression)
        {
            m_pIMThumbnails = ThumbnailImage(m_pIMImages, m_nWight * m_fRatio, m_nHeight * m_fRatio, m_pIMException);
        }
        else
        {
            m_pIMThumbnails = SampleImage(m_pIMImages, m_nWight * m_fRatio, m_nHeight * m_fRatio, m_pIMException);
        }
        if (m_pIMThumbnails == NULL)
        {
            m_strImageMgickError = m_pIMException->reason;
            releaseIM();
            return false;
        }

        if (!m_strMiddleFile.isEmpty())
        {
            if (
                m_strSufix != "cdr"
                )
            {
                Image *image;
                ImageInfo *midInfo = CloneImageInfo(m_pIMImageInfo);
                midInfo->quality = 10;
                midInfo->interlace = PNGInterlace;
                if (m_pIMImages->compression == RLECompression)
                {
                    image = ThumbnailImage(m_pIMImages, m_nWight, m_nHeight, m_pIMException);
                }
                else
                {
                    image = SampleImage(m_pIMImages, m_nWight, m_nHeight, m_pIMException);
                }
                (void)strcpy(image->filename, m_strMiddleFile.toStdString().c_str());
                (void)strcpy(midInfo->filename, m_strMiddleFile.toStdString().c_str());

                WriteImage(midInfo, image, m_pIMException);

                image = DestroyImage(image);
                midInfo = DestroyImageInfo(midInfo);
            }
        }
    }

    size_t nColors = 0;
    /*
      Write the image thumbnail.
    */
    (void)strcpy(m_pIMThumbnails->filename, m_strPreview.toStdString().c_str());
    m_pIMThumbnailsInfo->quality = 10;
    bool b = WriteImage(m_pIMThumbnailsInfo, m_pIMThumbnails, m_pIMException);
    Image *imgForColor = SampleImage(m_pIMThumbnails, m_pIMThumbnails->magick_columns * 0.1, m_pIMThumbnails->rows * 0.1, m_pIMException);

    if (m_strSufix != "ai"
        || m_strSufix != "svg"
        || m_strSufix != "png")
    {
        QuantizeInfo *quantize = AcquireQuantizeInfo(m_pIMThumbnailsInfo);
        quantize->number_colors = 10;
        CompressImageColormap(imgForColor, m_pIMException);
        QuantizeImage(quantize, imgForColor, m_pIMException);
    }
    PixelInfo *p = GetImageHistogram(imgForColor, &nColors, m_pIMException);
    QMap<QString, int> mapColor;
    QList<QColor> lstColor;
    QList<int> lstColorNumber;

    qsort((void *)p, (size_t)nColors, sizeof(*p), HistogramCompare);
    PixelInfo pixel;
    GetPixelInfo(imgForColor, &pixel);
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
    imgForColor = DestroyImage(imgForColor);
    m_strImageMgickError = m_pIMException->reason;
    releaseIM();
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
            zip.close();
            return readImageFile();
        }
    }
    m_strImageMgickError = "invaild cdr file";
    return false;
}

bool ImageObjcet::readPPT()
{
    bool b = PPT2Image(m_strImage.toLocal8Bit(), m_strMiddleFile.isEmpty() ? m_strPreview.toLocal8Bit() : m_strMiddleFile.toLocal8Bit());
    if (b)
    {
        m_strImage = m_strMiddleFile.isEmpty() ? m_strPreview : m_strMiddleFile;
        m_strMiddleFile.clear();
        b = readImageFile();
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

void ImageObjcet::releaseIM()
{
    if (m_pIMThumbnails)
    {
        m_pIMThumbnails = DestroyImage(m_pIMThumbnails);
    }
    if (m_pIMImages)
    {
        m_pIMImages = DestroyImage(m_pIMImages);
    }
    if (m_pIMImageInfo)
    {
        m_pIMImageInfo = DestroyImageInfo(m_pIMImageInfo);
    }
    if (m_pIMThumbnailsInfo)
    {
        m_pIMThumbnailsInfo = DestroyImageInfo(m_pIMThumbnailsInfo);
    }
    if (m_pIMException)
    {
        m_pIMException = DestroyExceptionInfo(m_pIMException);
    }
    MagickCoreTerminus();
}
