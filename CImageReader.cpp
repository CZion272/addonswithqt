#include "CImageReader.h"

using namespace std;
Persistent<Function> CImageReader::m_pConstructor;

CImageReader::CImageReader(const char* image, const char* preview) :
    m_strImage(image),
    m_strPreview(preview),
    m_pImageObj(NULL)
{
    if (getenv("MAGICK_GHOSTSCRIPT_PATH") == NULL)
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
    if (m_pImageObj != NULL)
    {
        delete m_pImageObj;
        m_pImageObj = NULL;
    }
}

void CImageReader::Init(Local<Object> exports)
{
    Isolate* isolate = exports->GetIsolate();

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "CImageReader"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "setDefaultColorList", setDefaultColorList);
    NODE_SET_PROTOTYPE_METHOD(tpl, "setDefaultImageSize", setDefaultImageSize);
    NODE_SET_PROTOTYPE_METHOD(tpl, "setDefaultMD5", setDefaultMD5);

    NODE_SET_PROTOTYPE_METHOD(tpl, "setMiddleFile", setMiddleFile);
    NODE_SET_PROTOTYPE_METHOD(tpl, "setPreviewSize", setPreviewSize);
    NODE_SET_PROTOTYPE_METHOD(tpl, "readFile", readFile);
    NODE_SET_PROTOTYPE_METHOD(tpl, "pingFileInfo", pingFileInfo);
    NODE_SET_PROTOTYPE_METHOD(tpl, "cancel", cancel);
    NODE_SET_PROTOTYPE_METHOD(tpl, "Release", Release);

    NODE_SET_PROTOTYPE_METHOD(tpl, "compareColor", compareColor);
    NODE_SET_PROTOTYPE_METHOD(tpl, "colorCount", colorCount);
    NODE_SET_PROTOTYPE_METHOD(tpl, "colorAt", colorAt);
    NODE_SET_PROTOTYPE_METHOD(tpl, "imageWidth", imageWidth);
    NODE_SET_PROTOTYPE_METHOD(tpl, "imageHeight", imageHeight);
    NODE_SET_PROTOTYPE_METHOD(tpl, "MD5", MD5);

    m_pConstructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "CImageReader"),
        tpl->GetFunction());
}

void CImageReader::readImageWorkerCb(uv_work_t * req)
{
    CImageReader * obj = static_cast<CImageReader *>(req->data);
    Isolate * isolate = obj->isolate;
    obj->m_pImageObj->readImage();
}

void CImageReader::afterReadImageWorkerCb(uv_work_t * req, int status)
{
    CImageReader * obj = static_cast<CImageReader *>(req->data);
    if (status == UV_ECANCELED)
    {
        return;
    }

    Isolate * isolate = obj->isolate;
    HandleScope scope(isolate);
    Local<Function> js_callback = Local<Function>::New(isolate, obj->js_callback);
    Local<Value> error =
        v8::Exception::TypeError(
            String::NewFromUtf8(isolate,
                obj->m_pImageObj->getLastError().toStdString().c_str()));
    if (!obj->m_pImageObj->getLastError().isEmpty())
    {
        js_callback->Call(isolate->GetCurrentContext()->Global(), 1, &error);
    }
    else
    {
        js_callback->Call(isolate->GetCurrentContext()->Global(), 0, 0);
    }
}

void CImageReader::New(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (
        !args[0]->IsString()
        || !args[1]->IsString()
        )
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        args.GetReturnValue().Set(FALSE);
        return;
    }
    v8::String::Utf8Value str(args[0]->ToString());
    const char *chImage = *str;
    v8::String::Utf8Value str1(args[1]->ToString());
    const char *chImage1 = *str1;

    CImageReader *obj = new CImageReader(chImage, chImage1);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
}

void CImageReader::Release(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();

    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    if (obj->m_pImageObj)
    {
        delete obj->m_pImageObj;
        obj->m_pImageObj = NULL;
    }
    args.This().Clear();
}

void CImageReader::setDefaultColorList(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsString() || !args[1]->IsString())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        return;
    }
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

    v8::String::Utf8Value str(args[0]->ToString());

    v8::String::Utf8Value strSplit(args[1]->ToString());

    QString strColor(*str);

    QStringList lstColor = strColor.split(*strSplit);
    QList<QColor> clr;
    foreach(QString strC, lstColor)
    {
        clr.append(QColor(strC));
    }
    obj->m_pImageObj->setDefaultColorList(clr);
}

void CImageReader::setDefaultImageSize(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsNumber() || !args[1]->IsNumber())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        return;
    }
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setDefaultSize(args[0]->IntegerValue(), args[1]->IntegerValue());
    }
    else
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "No Image Object")));
        return;
    }
}

void CImageReader::setDefaultMD5(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsString())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        return;
    }
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

    v8::String::Utf8Value str(args[0]->ToString());
    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setDefalutMD5(*str);
    }
}

void CImageReader::readFile(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsFunction())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        return;
    }
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

    QFileInfo info(obj->m_strImage);
    if (!info.isFile())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Invalid file")));
        args.GetReturnValue().Set(false);
        return;
    }

    HandleScope scope(isolate);
    obj->request.data = obj;
    obj->isolate = isolate;
    obj->js_callback.Reset(isolate, Local<Function>::Cast(args[0]));

    uv_queue_work(uv_default_loop(), &(obj->request), readImageWorkerCb, afterReadImageWorkerCb);

    args.GetReturnValue().Set(true);
}

void CImageReader::pingFileInfo(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    QFileInfo info(obj->m_strImage);
    if (!info.isFile())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Invalid file or video")));
        args.GetReturnValue().Set(false);
        return;
    }
    bool b = false;
    if (obj->m_pImageObj)
    {
        b = obj->m_pImageObj->pingImageFile();
    }
    args.GetReturnValue().Set(b);
}

void CImageReader::cancel(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    uv_work_s* w = &obj->request;
    int n = uv_cancel((uv_req_s*)w);
    if (n == 0)
    {
        args.GetReturnValue().Set(true);
        return;
    }
    args.GetReturnValue().Set(false);
}

void CImageReader::setPreviewSize(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsNumber() || !args[1]->IsNumber())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        args.GetReturnValue().Set(FALSE);
        return;
    }
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setScallSize(args[0]->IntegerValue(), args[1]->IntegerValue());
    }
}

void CImageReader::setMiddleFile(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsString())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        args.GetReturnValue().Set(FALSE);
        return;
    }
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

    v8::String::Utf8Value str(args[0]->ToString());
    if (obj->m_pImageObj)
    {
        obj->m_pImageObj->setMiddleFile(*str);
    }
}

void CImageReader::compareColor(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsString())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        args.GetReturnValue().Set(FALSE);
        return;
    }
    v8::String::Utf8Value str(args[0]->ToString());
    QColor color(*str);
    if (color.isValid())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        args.GetReturnValue().Set(FALSE);
        return;
    }
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    bool bCompare = false;
    if (obj->m_pImageObj)
    {
        bCompare = obj->m_pImageObj->compareColorEx(color);
    }
    args.GetReturnValue().Set(!bCompare);
}

void CImageReader::MD5(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    QFile file(obj->m_strImage);
    if (!file.open(QFile::ReadOnly))
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "File open failed")));
        args.GetReturnValue().Set(FALSE);
    }
    QString strMD5 = "";
    if (obj->m_pImageObj)
    {
        strMD5 = obj->m_pImageObj->MD5();
    }
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, strMD5.toLatin1().constData()));
}

void CImageReader::colorCount(const FunctionCallbackInfo<Value>& args)
{
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    int nCount = 0;
    if (obj->m_pImageObj)
    {
        nCount = obj->m_pImageObj->getColorList().count();
    }
    args.GetReturnValue().Set(nCount);
}

//static node.jsʹ��
void CImageReader::colorAt(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    if (!args[0]->IsNumber())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, "Parameter error")));
        args.GetReturnValue().Set(FALSE);
        return;
    }
    int i = args[0]->NumberValue();

    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    QList<QColor> lstColor;
    if (obj->m_pImageObj)
    {
        lstColor = obj->m_pImageObj->getColorList();
    }

    if (i >= lstColor.count())
    {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, ""));
        return;
    }
    QColor color = lstColor.at(i);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, color.name().toLatin1().constData()));
    return;
}

void CImageReader::imageWidth(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    int nWigth = 0;
    if (obj->m_pImageObj)
    {
        nWigth = obj->m_pImageObj->getWigth();
    }
    args.GetReturnValue().Set(nWigth);
}

void CImageReader::imageHeight(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
    int nHeight = 0;
    if (obj->m_pImageObj)
    {
        nHeight = obj->m_pImageObj->getHeight();
    }
    args.GetReturnValue().Set(nHeight);
}
