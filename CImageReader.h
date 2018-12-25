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
#include <QThread>

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

class CImageReader : public node::ObjectWrap
{
public:
	static void Init(v8::Local<v8::Object> exports);
	static void New(const FunctionCallbackInfo<Value>& args);

	void readImage();
	static CImageReader* getObj() { return m_pInstace; };
private:
	explicit CImageReader(const char* strImage = "", const char* strPreview = "", float nRatio = 0.6);
	~CImageReader();

	QList<QColor> getColorList();
	int getWigth();
	int getHeight();

	bool compareColor(QColor color);
	bool readImageFile();
	bool readCdrPerviewFile();

private:
	//µ¼³öº¯Êý
	static void readFile(const FunctionCallbackInfo<Value>& args);
	static void checkColor(const FunctionCallbackInfo<Value>& args);
	static void MD5(const FunctionCallbackInfo<Value>& args);
	static void colorCount(const FunctionCallbackInfo<Value>& args);
	static void colorAt(const FunctionCallbackInfo<Value>& args);
	static void ImageWidth(const FunctionCallbackInfo<Value>& args);
	static void ImageHeigth(const FunctionCallbackInfo<Value>& args);
private:
	static v8::Persistent<v8::Function> constructor;
	static CImageReader *m_pInstace;

	QString m_strImage;
	QImage m_imgImage;
	QString m_strPreview;
	QImage m_imgPreview;
	QList<QColor> m_lstColor;

	int m_nHeight;
	int m_nWight;
	float m_nRatio;

	int m_nColorCount;
	bool readPPT();
};

#endif // QTADDONS_H
