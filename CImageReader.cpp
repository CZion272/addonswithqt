#include "CImageReader.h"
#include <QDebug>

using namespace std;
napi_ref CImageReader::constructor;

const char *errorMessage[] = {
    "No such file",
    "Parameter error",
    "env check error",
};

#define ERRORSTRING(env, code) \
{\
    napi_throw_error(env, QString::number(code).toLatin1().constData(), errorMessage[code]);\
}

static napi_value intValue(napi_env env, int num)
{
    napi_value rb;
    napi_create_int32(env, num, &rb);
    return rb;
}

static napi_value stringValue(napi_env env, QString str)
{
    napi_value rb;
    napi_create_string_latin1(env, str.toLatin1(), str.length(), &rb);
    return rb;
}

static napi_value boolenValue(napi_env env, bool b)
{
    napi_value rb;
    napi_get_boolean(env, b, &rb);
    return rb;
}

static bool checkNapiEnv(napi_env env, napi_callback_info info, size_t argc, napi_value *args, napi_value *jsthis)
{
    napi_status status;

    napi_value target;
    napi_get_new_target(env, info, &target);
    if (napi_get_new_target(env, info, &target) != napi_ok)
    {
        return false;
    }
    if (napi_get_cb_info(env, info, &argc, args, jsthis, nullptr))
    {
        return false;
    }
    return true;
}

static bool checkValueType(napi_env env, napi_value value, napi_valuetype type)
{
    napi_valuetype valueType;
    if (napi_typeof(env, value, &valueType) != napi_ok)
    {
        return false;
    }
    if (type != valueType)
    {
        return false;
    }
    return true;
}

static bool napiValueToString(napi_env env, napi_value value, QString &string)
{
    if (!checkValueType(env, value, napi_string))
    {
        return false;
    }
    size_t size;
    char *ch;
    if (napi_get_value_string_utf8(env, value, NULL, NAPI_AUTO_LENGTH, &size) != napi_ok)
    {
        return false;
    }
    ch = new char[size + 1];
    if (napi_get_value_string_utf8(env, value, ch, size + 1, &size) != napi_ok)
    {
        delete ch;
        return false;
    }
    string = QString(ch);
    delete ch;
    return true;
}

CImageReader::CImageReader(QString image, QString preview) :
    m_strImage(image),
    m_strPreview(preview),
    m_pImageObj(NULL),
    env_(nullptr),
    wrapper_(nullptr)
{
    //if (getenv("MAGICK_GHOSTSCRIPT_PATH") == NULL)
    {
        wchar_t wcEnvPath[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, wcEnvPath, MAX_PATH);
#ifdef WIN32
        QString strEnvPath = "MAGICK_GHOSTSCRIPT_PATH=";
        strEnvPath += QString::fromWCharArray(wcEnvPath);
#else// WIN32
        QString strEnvPath = QString::fromWCharArray(wcEnvPath);
#endif 
        int nPos = strEnvPath.lastIndexOf("\\");
        strEnvPath = strEnvPath.left(nPos);
        strEnvPath = strEnvPath.replace("\\", "\\\\");
        strEnvPath.toWCharArray(wcEnvPath);
#ifdef WIN32
        putenv(strEnvPath.toStdString().c_str());
#else// WIN32
        setenv("MAGICK_GHOSTSCRIPT_PATH", strEnvPath.toStdString().c_str(), 0);
#endif 
    }
    m_pImageObj = new ImageObjcet(image, preview);
}

CImageReader::~CImageReader()
{
    napi_delete_reference(env_, wrapper_);
    if (m_pImageObj != NULL)
    {
        delete m_pImageObj;
        m_pImageObj = NULL;
    }
}

napi_value CImageReader::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_property_descriptor properties[] = {
        //{ "value", 0, 0, GetValue, SetValue, 0, napi_default, 0 },
        DECLARE_NAPI_METHOD("setDefaultColorList", setDefaultColorList),
        DECLARE_NAPI_METHOD("setDefaultImageSize", setDefaultImageSize),
        DECLARE_NAPI_METHOD("setDefaultMD5", setDefaultMD5),
        DECLARE_NAPI_METHOD("setMiddleFile", setMiddleFile),
        DECLARE_NAPI_METHOD("setPreviewSize", setPreviewSize),
        DECLARE_NAPI_METHOD("readFile", readFile),
        DECLARE_NAPI_METHOD("createPreviewFile", createPreviewFile),
        DECLARE_NAPI_METHOD("createColorMap", createColorMap),
        DECLARE_NAPI_METHOD("pingFileInfo", pingFileInfo),
        DECLARE_NAPI_METHOD("Release", Release),
        DECLARE_NAPI_METHOD("compareColor", compareColor),
        DECLARE_NAPI_METHOD("colorCount", colorCount),
        DECLARE_NAPI_METHOD("colorAt", colorAt),
        DECLARE_NAPI_METHOD("imageWidth", imageWidth),
        DECLARE_NAPI_METHOD("imageHeight", imageHeight),
        DECLARE_NAPI_METHOD("MD5", MD5)
    };

    napi_value cons;
    status = napi_define_class(env, "CImageReader", NAPI_AUTO_LENGTH, New, nullptr, 16, properties, &cons);
    assert(status == napi_ok);

    status = napi_create_reference(env, cons, 1, &constructor);
    assert(status == napi_ok);

    status = napi_set_named_property(env, exports, "CImageReader", cons);
    assert(status == napi_ok);
    return exports;
}

void CImageReader::Destructor(napi_env env, void* nativeObject, void* /*finalize_hint*/)
{
    reinterpret_cast<CImageReader*>(nativeObject)->~CImageReader();
}


napi_value CImageReader::New(napi_env env, napi_callback_info info)
{
    napi_status status;

    size_t argc = 2;
    napi_value args[2];
    napi_value jsthis;

    napi_value target;
    if (napi_get_new_target(env, info, &target) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    if (!checkValueType(env, args[0], napi_string))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }
    if (!checkValueType(env, args[1], napi_string))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }

    QString strImage;
    QString strPreview;
    napiValueToString(env, args[0], strImage);
    napiValueToString(env, args[1], strPreview);

    CImageReader* obj = new CImageReader(strImage, strPreview);

    obj->env_ = env;
    status = napi_wrap(env,
        jsthis,
        reinterpret_cast<void*>(obj),
        CImageReader::Destructor,
        nullptr,  // finalize_hint
        &obj->wrapper_);
    assert(status == napi_ok);

    return jsthis;
}

napi_value CImageReader::Release(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }
    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    if (obj->m_pImageObj)
    {
        delete obj->m_pImageObj;
        obj->m_pImageObj = NULL;
    }
    return nullptr;
}

napi_value CImageReader::setDefaultColorList(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader* obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    if (!checkValueType(env, args[0], napi_string))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }

    QString strColor;

    napiValueToString(env, args[0], strColor);


    QStringList lstColor = strColor.split("*");
    QList<QColor> clr;
    foreach(QString strC, lstColor)
    {
        clr.append(QColor(strC));
    }
    obj->m_pImageObj->setDefaultColorList(clr);

    return nullptr;
}

napi_value CImageReader::setDefaultImageSize(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 2;
    napi_value args[2];
    CImageReader* obj;
    if (checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (checkValueType(env, args[0], napi_number))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }
    if (checkValueType(env, args[1], napi_number))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    int value[2];
    napi_get_value_int32(env, args[0], &value[0]);
    napi_get_value_int32(env, args[1], &value[1]);

    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setDefaultSize(value[0], value[1]);
    }
    else
    {
        ERRORSTRING(env, 1);
    }
    return nullptr;
}

napi_value CImageReader::setDefaultMD5(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader* obj;
    if (checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return boolenValue(env, false);;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    if (checkValueType(env, args[0], napi_string))
    {
        ERRORSTRING(env, 2);
        return boolenValue(env, false);;
    }

    QString strMD5;

    napiValueToString(env, args[0], strMD5);

    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setDefalutMD5(strMD5);
    }

    return jsthis;
}

napi_value CImageReader::readFile(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return boolenValue(env, false);
    }
    if (!checkValueType(env, args[0], napi_function))
    {
        ERRORSTRING(env, 2);
        return boolenValue(env, false);
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    napi_value global;
    status = napi_get_global(env, &global);
    napi_value argv[1] = { 0 };
    napi_value result;
    napi_value rb;
    napi_value cb = args[0];
    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->readImage();
    }
    if (!obj->m_pImageObj->getLastError().isEmpty())
    {
        argv[0] = stringValue(env, obj->m_pImageObj->getLastError());
        napi_call_function(env, global, cb, 1, argv, &result);
        return boolenValue(env, false);
    }
    else
    {
        napi_call_function(env, global, cb, 0, argv, &result);
        return boolenValue(env, true);
    }
}

napi_value CImageReader::pingFileInfo(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    napi_value global;
    status = napi_get_global(env, &global);
    napi_value rb;
    QFileInfo fInfo(obj->m_strImage);
    if (!fInfo.isFile())
    {
        ERRORSTRING(env, 1);
        return boolenValue(env, false);
    }
    bool b = false;
    if (obj->m_pImageObj)
    {
        b = obj->m_pImageObj->pingImageFile();
    }
    return boolenValue(env, b);
}

napi_value CImageReader::createPreviewFile(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (!checkValueType(env, args[0], napi_function))
    {
        ERRORSTRING(env, 2);
        return boolenValue(env, false);
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        ERRORSTRING(env, 3);
        return false;
    }
    napi_value global;
    status = napi_get_global(env, &global);
    napi_value argv[1];
    napi_value result;
    napi_value rb;
    napi_value cb = args[0];
    bool b = false;
    if (obj->m_pImageObj)
    {
        b = obj->m_pImageObj->createThumbnail();
    }
    if (!b)
    {
        argv[0] = stringValue(env, obj->m_pImageObj->getLastError());
        napi_call_function(env, global, cb, 1, argv, &result);
        return boolenValue(env, false);
    }
    else
    {
        napi_call_function(env, global, cb, 0, argv, &result);
    }
    return boolenValue(env, b);
}

napi_value CImageReader::createColorMap(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }
    napi_value global;
    status = napi_get_global(env, &global);
    napi_value argv[1];
    napi_value result;
    napi_value rb;
    napi_value cb = args[0];
    bool b = false;
    if (obj->m_pImageObj)
    {
        b = obj->m_pImageObj->createColorMap();
    }
    if (!b)
    {
        argv[0] = stringValue(env, obj->m_pImageObj->getLastError());
        napi_call_function(env, global, cb, 1, argv, &result);
    }
    else
    {
        napi_call_function(env, global, cb, 0, argv, &result);
    }
    return boolenValue(env, b);
}

napi_value CImageReader::setPreviewSize(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 2;
    napi_value args[2];

    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }
    if (!checkValueType(env, args[0], napi_number))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }

    if (!checkValueType(env, args[1], napi_number))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }
    int value[2];
    napi_get_value_int32(env, args[0], &value[0]);
    napi_get_value_int32(env, args[1], &value[1]);

    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setScallSize(value[0], value[1]);
    }
    return nullptr;
}

napi_value CImageReader::setMiddleFile(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }
    if (!checkValueType(env, args[0], napi_string))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }
    QString strMid;
    napiValueToString(env, args[0], strMid);
    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setMiddleFile(strMid);
    }
    return nullptr;
}

napi_value CImageReader::compareColor(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];

    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return boolenValue(env, false);
    }
    if (!checkValueType(env, args[0], napi_string))
    {
        ERRORSTRING(env, 2);
        return boolenValue(env, false);
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }

    QString strColor;
    napiValueToString(env, args[0], strColor);

    napi_value rb;
    bool bCompare = false;
    QColor color(strColor);
    if (color.isValid())
    {
        ERRORSTRING(env, 2);
        return boolenValue(env, bCompare);
    }
    if (obj->m_pImageObj)
    {
        bCompare = obj->m_pImageObj->compareColorEx(color);
    }
    return boolenValue(env, bCompare);
}

napi_value CImageReader::MD5(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];

    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }
    QFile file(obj->m_strImage);
    if (!file.exists())
    {
        ERRORSTRING(env, 1);
        return nullptr;
    }
    QString strMD5 = "";
    if (obj->m_pImageObj)
    {
        strMD5 = obj->m_pImageObj->MD5();
    }
    file.close();
    return stringValue(env, strMD5);
}

napi_value CImageReader::colorCount(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];

    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }
    int nCount = 0;
    if (obj->m_pImageObj)
    {
        nCount = obj->m_pImageObj->getColorList().count();
    }
    return intValue(env, nCount);
}

napi_value CImageReader::colorAt(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];

    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }
    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }

    if (!checkValueType(env, args[0], napi_number))
    {
        ERRORSTRING(env, 2);
        return nullptr;
    }
    int value;
    size_t size;
    napi_get_value_int32(env, args[0], &value);

    QList<QColor> lstColor;
    if (obj->m_pImageObj)
    {
        lstColor = obj->m_pImageObj->getColorList();
    }

    if (value >= lstColor.count())
    {
        return nullptr;
    }
    QColor color = lstColor.at(value);
    return stringValue(env, color.name());
}

napi_value CImageReader::imageWidth(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }
    int nWigth = 0;
    if (obj->m_pImageObj)
    {
        nWigth = obj->m_pImageObj->getWigth();
    }

    return intValue(env, nWigth);
}

napi_value CImageReader::imageHeight(napi_env env, napi_callback_info info)
{
    napi_status status;

    napi_value jsthis = NULL;
    size_t argc = 1;
    napi_value args[1];
    CImageReader *obj;
    if (!checkNapiEnv(env, info, argc, args, &jsthis))
    {
        ERRORSTRING(env, 3);
        return nullptr;
    }

    if (napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj)) != napi_ok)
    {
        return false;
    }
    int nHeight = 0;
    if (obj->m_pImageObj)
    {
        nHeight = obj->m_pImageObj->getHeight();
    }

    return intValue(env, nHeight);
}
