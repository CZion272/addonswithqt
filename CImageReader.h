#ifndef QTADDONS_H
#define QTADDONS_H

#include <uv.h>
//#include <Windows.h>
#include <stdio.h>
#include <node.h>
#include "ImageObjcet.h"
#include <node_object_wrap.h>

using namespace v8;
using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;
using v8::HandleScope;

class CImageReader : public node::ObjectWrap
{

public:
	static void Init(v8::Local<v8::Object> exports);
	static void New(const FunctionCallbackInfo<Value>& args);
private:
	explicit CImageReader(const char* strImage = "", const char* strPreview = "");
	~CImageReader();

private:
	//导出函数

	static void Release(const FunctionCallbackInfo<Value>& args);
	//设置默认值，当文件二次读取时直接设置
	static void setDefaultColorList(const FunctionCallbackInfo<Value>& args);
	static void setDefaultImageSize(const FunctionCallbackInfo<Value>& args);
	static void setDefaultMD5(const FunctionCallbackInfo<Value>& args);

	//首次读取文件
	static void setPreviewSize(const FunctionCallbackInfo<Value>& args);
	static void setMiddleFile(const FunctionCallbackInfo<Value>& args);//CDR AI PPT等无法直接预览的格式需要
	static void readFile(const FunctionCallbackInfo<Value>& args);
	static void pingFileInfo(const FunctionCallbackInfo<Value>& args);
	static void cancel(const FunctionCallbackInfo<Value>& args);

	//获取文件属性
	static void compareColor(const FunctionCallbackInfo<Value>& args);
	static void MD5(const FunctionCallbackInfo<Value>& args);
	static void colorCount(const FunctionCallbackInfo<Value>& args);
	static void colorAt(const FunctionCallbackInfo<Value>& args);
	static void imageWidth(const FunctionCallbackInfo<Value>& args);
	static void imageHeight(const FunctionCallbackInfo<Value>& args);

	static void afterReadImageWorkerCb(uv_work_t * req, int status);
	static void readImageWorkerCb(uv_work_t * req);
private:
	static v8::Persistent<v8::Function> m_pConstructor;
	ImageObjcet *m_pImageObj;
	QString m_strImage;
	QString m_strPreview;

	uv_work_t request;
	Isolate * isolate;
	Persistent<Function> js_callback;
};

#endif // QTADDONS_H
