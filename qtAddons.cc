#pragma once
#include <QDebug>
#include <QWindow>
#include "CImageReader.h"
#include "qmath.h"

namespace qtAddons
{

    static double gR = 100;
    static double gAngle = 30;
    static double gh = gR * qCos((float)gAngle / 180 * M_PI);
    static double gr = gR * qSin((float)gAngle / 180 * M_PI);

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
    napi_value hasColor(napi_env env, napi_callback_info info)
    {
        napi_status status;

        napi_value jsthis;
        size_t argc = 2;
        napi_value args[2];
        status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
        assert(status == napi_ok);

        napi_valuetype valuetype[2];
        status = napi_typeof(env, args[0], &valuetype[0]);
        assert(status == napi_ok);

        status = napi_typeof(env, args[1], &valuetype[1]);
        assert(status == napi_ok);

        if (valuetype[0] != napi_string && valuetype[1] != napi_string)
        {
            napi_throw_error(env, "1", "error string");
            return jsthis;
        }

        char value[2][1024] = { 0 };
        size_t size;
        status = napi_get_value_string_latin1(env, args[0], value[0], 1024, &size);
        assert(status == napi_ok);
        status = napi_get_value_string_latin1(env, args[1], value[1], 1024, &size);
        assert(status == napi_ok);

        QString strColor = value[0];

        QString strColorList = value[1];

        QStringList lstStrColor = strColorList.split(",");
        QList<QColor> lstColor;
        foreach(QString str, lstStrColor)
        {
            lstColor.append(QColor(str));
        }
        napi_value rb;
        status = napi_get_boolean(env, compareColorEx(QColor(strColor), lstColor), &rb);
        assert(status == napi_ok);
        return rb;
    }

    napi_value creatColorMap(napi_env env, napi_callback_info info)
    {
        __try
        {
            napi_status status;

            napi_value jsthis;
            size_t argc = 1;
            napi_value args[1];
            status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
            assert(status == napi_ok);

            napi_valuetype valuetype;
            status = napi_typeof(env, args[0], &valuetype);
            assert(status == napi_ok);
            if (valuetype != napi_string)
            {
                napi_throw_error(env, "1", "Parameter error");
                return jsthis;
            }

            char value[MAX_PATH] = { 0 };
            size_t size;
            status = napi_get_value_string_latin1(env, args[0], value, 1024, &size);
            assert(status == napi_ok);

            QFileInfo file(value);
            if (!file.exists())
            {
                napi_throw_error(env, "2", "No such file");
                return jsthis;
            }

            ImageInfo *imageInfo;
            ExceptionInfo *exception;
            imageInfo = CloneImageInfo(NULL);
            exception = AcquireExceptionInfo();
            strcpy(imageInfo->filename, QString(value).toStdString().c_str());
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
            napi_value rb;
            napi_create_string_latin1(env, strBack.toLatin1().constData(), strBack.size(), &rb);
            return rb;
        }
        __except (lpUnhandledExceptionFilter(GetExceptionInformation()))
        {
            napi_throw_error(env, "4", "WinDbg Failed");
            return false;
        }
    }

    napi_value InitAll(napi_env env, napi_value exports)
    {
        CImageReader::Init(env, exports);
        const int nPorperty = 2;
        napi_status status;
        napi_property_descriptor desc[nPorperty];
        desc[0] = DECLARE_NAPI_METHOD("hasColor", hasColor);
        desc[1] = DECLARE_NAPI_METHOD("creatColorMap", creatColorMap);
        status = napi_define_properties(env, exports, nPorperty, desc);
        assert(status == napi_ok);
        return exports;
    }

    NAPI_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}