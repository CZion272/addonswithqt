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
#include <MagickCore/MagickCore.h>
#include <direct.h>
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

enum FILETYPE
{
	TYPE_NORMAL,
	TYPE_CDR,
	TYPE_OFFICE
};

class CImageReader;

struct ShareData
{
	uv_work_t request;
	Isolate * isolate;
	Persistent<Function> js_callback;
	CImageReader* obj;

	uv_sem_t m_pSemThis = NULL;
	uv_sem_t m_pSemLast = NULL;
};

class CImageReader : public node::ObjectWrap
{

public:
	static void Init(v8::Local<v8::Object> exports);
	static void New(const FunctionCallbackInfo<Value>& args);
	void readImage();
private:
	explicit CImageReader(const char* strImage = "", const char* strPreview = "");
	~CImageReader();

	QList<QColor> getColorList();
	int getWigth();
	int getHeight();

	bool compareColorEx(QColor color, int nDiff = 10000);

	bool pingImageFile();
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

	static void release(const FunctionCallbackInfo<Value>& args);

private:
	static v8::Persistent<v8::Function> m_pConstructor;

	QString m_strImage;
	QImage m_imgImage;
	QString m_strPreview;
	QImage m_imgPreview;
	QList<QColor> m_lstColor;
	QString m_strMd5;
	QString m_strMiddleFile;
	QString m_strSufix;
	QString m_strImageMgickError;

	int m_nHeight;
	int m_nWight;
	int m_nScaleHeight;
	int m_nScaleWight;
	float m_fRatio;

	int m_nColorCount;
	ShareData * m_pReqData;
};

#endif // QTADDONS_H
