#include <node.h>
#include <QDebug>
//#include <QWidget>
#include <QWindow>
#include "CImageReader.h"
#include "qmath.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return (TRUE);
}

namespace qtAddons
{

    static double gR = 100;
    static double gAngle = 30;
    static double gh = gR * qCos((float)gAngle / 180 * M_PI);
    static double gr = gR * qSin((float)gAngle / 180 * M_PI);

    using v8::Local;
    using v8::Object;
    bool compareColorEx(QColor color, QList<QColor> lstColor, int Diff = 500000)
    {
        int h = 0, s = 0, v = 0;
        QColor hsvColor = color.toHsv();
        hsvColor.getHsv(&h, &s, &v);
        if (lstColor.count() == 0)
        {
            return false;
        }
        foreach(QColor colorCompare, lstColor)
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
            if ((int)rCom < Diff)
            {
                return true;
            }
        }
        return false;
    }
    void hasColor(const FunctionCallbackInfo<Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        if (!args[0]->IsString() || !args[1]->IsString())
        {
            isolate->ThrowException(Exception::TypeError(
                String::NewFromUtf8(isolate, "Parameter error")));
            args.GetReturnValue().Set(false);
            return;
        }
        v8::String::Utf8Value str(args[0]->ToString());
        QString strColor = *str;

        v8::String::Utf8Value str1(args[1]->ToString());
        QString strColorList = *str1;

        QStringList lstStrColor = strColorList.split(",");
        QList<QColor> lstColor;
        foreach(QString str, lstStrColor)
        {
            lstColor.append(QColor(str));
        }
        args.GetReturnValue().Set(compareColorEx(QColor(strColor), lstColor));
    }

    void getColorList(const FunctionCallbackInfo<Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        if (!args[0]->IsString())
        {
            isolate->ThrowException(Exception::TypeError(
                String::NewFromUtf8(isolate, "Parameter error")));
            args.GetReturnValue().Set(false);
            return;
        }

        v8::String::Utf8Value str(args[0]->ToString());
        QFileInfo file(*str);
        if (!file.exists())
        {
            isolate->ThrowException(Exception::TypeError(
                String::NewFromUtf8(isolate, "No such file")));
            args.GetReturnValue().Set(false);
            return;
        }

        ImageInfo *imageInfo;
        ExceptionInfo *exception;
        imageInfo = CloneImageInfo(NULL);
        exception = AcquireExceptionInfo();
        strcpy(imageInfo->filename, QString(*str).toStdString().c_str());
        //if (m_strSufix == "psd")
        {
            imageInfo->number_scenes = 1;
        }
        Image *image = ReadImage(imageInfo, exception);

        Image *imgForColor = SampleImage(image, image->magick_columns * 0.1, image->rows * 0.1, exception);

        if (file.suffix().toLower() != "ai"
            || file.suffix().toLower() != "svg"
            || file.suffix().toLower() != "png")
        {
            QuantizeInfo *quantize = AcquireQuantizeInfo(imageInfo);
            quantize->number_colors = 10;
            CompressImageColormap(imgForColor, exception);
            QuantizeImage(quantize, imgForColor, exception);
        }
        size_t nColors;
        PixelInfo *p = GetImageHistogram(imgForColor, &nColors, exception);
        QMap<QString, int> mapColor;
        QList<QColor> lstColor;
        QList<int> lstColorNumber;

        qsort((void *)p, (size_t)nColors, sizeof(*p), HistogramCompare);
        PixelInfo pixel;
        GetPixelInfo(imgForColor, &pixel);

        int nDiff = nColors < 10 ? 5000 : 20000;
        for (int i = 0; i < (ssize_t)nColors; i++)
        {
            pixel = (*p);
            char hex[MAX_PATH] = { 0 };
            GetColorTuple(&pixel, MagickTrue, hex);
            QString strHex(hex);
            strHex = strHex.left(7);
            if (!compareColorEx(QColor(strHex), lstColor, nDiff))
            {
                //if (m_lstColor.indexOf(QColor(strHex)) == -1)
                {
                    lstColor.append(QColor(strHex));
                }
            }
            p++;
        }
        imgForColor = DestroyImage(imgForColor);
        image = DestroyImage(image);
        imageInfo = DestroyImageInfo(imageInfo);
        exception = DestroyExceptionInfo(exception);
        QString strBack;
        foreach(QColor clr, lstColor)
        {
            strBack += clr.name() + ";";
        }
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, strBack.toLatin1().constData()));
    }

    void InitAll(Local<Object> exports)
    {
        CImageReader::Init(exports);
        NODE_SET_METHOD(exports, "hasColor", hasColor);
        NODE_SET_METHOD(exports, "getColorList", getColorList);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}