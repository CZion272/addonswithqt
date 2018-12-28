#ifndef QTADDONS_H
#define QTADDONS_H

#include <uv.h>
//#include <Windows.h>
#include <stdio.h>
#include "quazip/quazipfile.h"
#include "quazip/quazip.h"
#include <QIODevice>
#include <QImageReader>
#include <node.h>
#include <QFileInfo>
#include <node_object_wrap.h>
#include <MagickCore/MagickCore.h>
#include <direct.h>

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

enum FILETYPE
{
	TYPE_NORMAL,
	TYPE_CDR,
	TYPE_OFFICE
};

struct ShareData
{
	uv_work_t request;
	Isolate * isolate;
	Persistent<Function> js_callback;
};

class CImageReader : public node::ObjectWrap
{

public:
	static void Init(v8::Local<v8::Object> exports);
	static void New(const FunctionCallbackInfo<Value>& args);
	static void realease();

	void readImage();
	static CImageReader* getObj() { return m_pInstace; };
private:
	explicit CImageReader(const char* strImage = "", const char* strPreview = "");
	~CImageReader();

	QList<QColor> getColorList();
	int getWigth();
	int getHeight();

	bool compareColor(QColor color);

	bool readImageFile();
	bool readCdrPerviewFile();
	bool readPPT();

private:
	//导出函数

	//设置默认值，当文件二次读取时直接设置
	static void setDefaultColorList(const FunctionCallbackInfo<Value>& args);
	static void setDefaultImageSize(const FunctionCallbackInfo<Value>& args);
	static void setDefaultMD5(const FunctionCallbackInfo<Value>& args);

	//首次读取文件
	static void setPreviewSize(const FunctionCallbackInfo<Value>& args);
	static void setMiddleFile(const FunctionCallbackInfo<Value>& args);//CDR AI PPT等无法直接预览的格式需要
	static void readFile(const FunctionCallbackInfo<Value>& args);
	static void cancel(const FunctionCallbackInfo<Value>& args);

	//获取文件属性
	static void checkColor(const FunctionCallbackInfo<Value>& args);
	static void MD5(const FunctionCallbackInfo<Value>& args);
	static void colorCount(const FunctionCallbackInfo<Value>& args);
	static void colorAt(const FunctionCallbackInfo<Value>& args);
	static void imageWidth(const FunctionCallbackInfo<Value>& args);
	static void imageHeight(const FunctionCallbackInfo<Value>& args);
private:
	static v8::Persistent<v8::Function> m_pConstructor;
	static CImageReader *m_pInstace;

	QString m_strImage;
	QImage m_imgImage;
	QString m_strPreview;
	QImage m_imgPreview;
	QList<QColor> m_lstColor;
	QString m_strMd5;
	QString m_strMiddleFile;
	QString m_strSufix;

	int m_nHeight;
	int m_nWight;
	int m_nScaleHeight;
	int m_nScaleWight;
	float m_fRatio;

	int m_nColorCount;
	ShareData * m_pReqData;

};

#endif // QTADDONS_H
