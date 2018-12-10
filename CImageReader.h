#ifndef QTADDONS_H
#define QTADDONS_H

#include <Windows.h>
#include <stdio.h>
#include "quazip/quazipfile.h"
#include "quazip/quazip.h"
#include <QIODevice>
#include <QImageReader>
#include <node.h>
#include <QFileInfo>
#include <node_object_wrap.h>

using v8::Exception;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

class CImageReader : public node::ObjectWrap 
{
	enum FILETYPE
	{
		TYPE_NORMAL,
		TYPE_CDR,
		TYPE_AI,
		TYPE_PSD
	};

public:
	static void Init(v8::Local<v8::Object> exports);
	static void New(const FunctionCallbackInfo<Value>& args);


private:
	explicit CImageReader(const char* strImage = "", const char* strPreview = "", float nRatio = 0.6);
	~CImageReader();

	bool compareColor(QColor color);
	void creatColorList();
	bool readImageFile(QString strPath);
	void readImage(QString strImage);
	bool readCdrPerviewFile(QString strZip);
	QList<QColor> getColorList();
	int getWigth();
	int getHeight();

private:
	static void colorAt(const FunctionCallbackInfo<Value>& args);
	static void ImageWidth(const FunctionCallbackInfo<Value>& args);
	static void ImageHeigth(const FunctionCallbackInfo<Value>& args);
private:
	static v8::Persistent<v8::Function> constructor;
	QString m_strImage;
	QImage m_imgImage;
	QString m_strPreview;
	QImage m_imgPreview;
	QList<QColor> m_lstColor;

	int m_nHeight;
	int m_nWight;
	float m_nRatio;
};

#endif // QTADDONS_H
