#pragma once
#ifndef QTADDONS_H
#define QTADDONS_H

#include <uv.h>
#include <stdio.h>
#include <node_api.h>
#include <QMap>
#include "ImageObjcet.h"
#include <QStringList>


#define DECLARE_NAPI_METHOD(name, func)                          \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

class CImageReader
{

public:
    static napi_value Init(napi_env env, napi_value exports);
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
private:
    explicit CImageReader(QString strImage = "", QString strPreview = "");
    ~CImageReader();
private:
    //导出函数
    static napi_value New(napi_env env, napi_callback_info info);

    static napi_value Release(napi_env env, napi_callback_info info);
    //设置默认值，当文件二次读取时直接设置
    static napi_value setDefaultColorList(napi_env env, napi_callback_info info);
    static napi_value setDefaultImageSize(napi_env env, napi_callback_info info);
    static napi_value setDefaultMD5(napi_env env, napi_callback_info info);

    //首次读取文件
    static napi_value setPreviewSize(napi_env env, napi_callback_info info);
    static napi_value setMiddleFile(napi_env env, napi_callback_info info);//CDR AI PPT等无法直接预览的格式需要
    static napi_value readFile(napi_env env, napi_callback_info info);
    static napi_value pingFileInfo(napi_env env, napi_callback_info info);
    static napi_value createPreviewFile(napi_env env, napi_callback_info info);
    static napi_value createColorMap(napi_env env, napi_callback_info info);
    //获取文件属性
    static napi_value compareColor(napi_env env, napi_callback_info info);
    static napi_value MD5(napi_env env, napi_callback_info info);
    static napi_value colorCount(napi_env env, napi_callback_info info);
    static napi_value colorAt(napi_env env, napi_callback_info info);
    static napi_value imageWidth(napi_env env, napi_callback_info info);
    static napi_value imageHeight(napi_env env, napi_callback_info info);
private:
    static napi_ref constructor;
    napi_env env_;
    napi_ref wrapper_;

    ImageObjcet *m_pImageObj;
    QString m_strImage;
    QString m_strPreview;
};

static napi_value intValue(napi_env env, int num);
static napi_value stringValue(napi_env env, QString str);
static napi_value boolenValue(napi_env env, bool b);


#endif // QTADDONS_H
