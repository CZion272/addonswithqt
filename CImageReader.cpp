#include "CImageReader.h"
#include <QDebug>
#include "qmath.h"
#include <QImageWriter>
#include <QBuffer>
#include <QTimer>
#include <QCryptographicHash>
#include <direct.h>

#include "opc/opc.h"

#define MAX_COLORS 5

using namespace std;
Persistent<Function> CImageReader::constructor;

struct ShareData
{
	uv_work_t request;
	Isolate * isolate;
	Persistent<Function> js_callback;
};

static double gR = 100;
static double gAngle = 30;
static double gh = gR * qCos((float)gAngle / 180 * M_PI);
static double gr = gR * qSin((float)gAngle / 180 * M_PI);
//直方图排序
static int HistogramCompare(const void *x, const void *y)
{
	const PixelInfo
		*color_1,
		*color_2;

	color_1 = (const PixelInfo *)x;
	color_2 = (const PixelInfo *)y;
	return((int)color_2->count - (int)color_1->count);
}

CImageReader *CImageReader::m_pInstace = NULL;


//ppt导出预览图片
static void extract(opcContainer *c, opcPart p, const char *path)
{
	char filename[OPC_MAX_PATH];
	opc_uint32_t i = xmlStrlen(p);
	while (i > 0 && p[i] != '/')
	{
		i--;
	}
	if (p[i] == '/')
	{
		i++;
	}
	QString strType((char*)(p + i));
	if (!strType.contains("thumbnail"))
	{
		return;
	}
	strcpy(filename, path);
	FILE *out = fopen(filename, "wb");
	if (NULL != out)
	{
		opcContainerInputStream *stream = opcContainerOpenInputStream(c, p);
		if (NULL != stream)
		{
			opc_uint32_t  ret = 0;
			opc_uint8_t buf[100];
			while ((ret = opcContainerReadInputStream(stream, buf, sizeof(buf))) > 0)
			{
				fwrite(buf, sizeof(char), ret, out);
			}
			opcContainerCloseInputStream(stream);
		}
		fclose(out);
	}
}

CImageReader::CImageReader(const char* image, const char* preview, float nRatio) :
	m_strImage(image),
	m_strPreview(preview),
	m_nHeight(0),
	m_nWight(0),
	m_nRatio(nRatio)
{
	//PDF和AI文件必要加载ghostscprit库，查找并设置环境变量
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
}

CImageReader::~CImageReader()
{
	if (m_pInstace)
	{
		delete m_pInstace;
	}
}

void CImageReader::Init(Local<Object> exports)
{
	Isolate* isolate = exports->GetIsolate();

	// 准备构造函数模版
	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
	tpl->SetClassName(String::NewFromUtf8(isolate, "CImageReader"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// 原型
	NODE_SET_PROTOTYPE_METHOD(tpl, "checkColor", checkColor);
	NODE_SET_PROTOTYPE_METHOD(tpl, "colorCount", colorCount);
	NODE_SET_PROTOTYPE_METHOD(tpl, "colorAt", colorAt);
	NODE_SET_PROTOTYPE_METHOD(tpl, "ImageWidth", ImageWidth);
	NODE_SET_PROTOTYPE_METHOD(tpl, "ImageHeigth", ImageHeigth);
	NODE_SET_PROTOTYPE_METHOD(tpl, "MD5", MD5);
	NODE_SET_PROTOTYPE_METHOD(tpl, "readFile", readFile);

	constructor.Reset(isolate, tpl->GetFunction());
	exports->Set(String::NewFromUtf8(isolate, "CImageReader"),
		tpl->GetFunction());
}

void readImageWorkerCb(uv_work_t * req)
{
	CImageReader::getObj()->readImage();
}

void afterReadImageWorkerCb(uv_work_t * req, int status)
{
	ShareData * my_data = static_cast<ShareData *>(req->data);
	Isolate * isolate = my_data->isolate;
	HandleScope scope(isolate);
	Local<Function> js_callback = Local<Function>::New(isolate, my_data->js_callback);
	js_callback->Call(isolate->GetCurrentContext()->Global(), 0, NULL);
	delete my_data;
}

void CImageReader::New(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (
		!args[0]->IsString()
		|| !args[1]->IsString()
		|| !args[2]->IsNumber()
		)
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		args.GetReturnValue().Set(FALSE);
		return;
	}
	v8::String::Utf8Value str(args[0]->ToString());
	const char *chImage = *str;
	v8::String::Utf8Value str1(args[1]->ToString());
	const char *chImage1 = *str1;

	m_pInstace = new CImageReader(chImage, chImage1, args[2]->NumberValue());

	m_pInstace->Wrap(args.This());
	args.GetReturnValue().Set(args.This());
}

QList<QColor> CImageReader::getColorList()
{
	return m_lstColor;
}

int CImageReader::getWigth()
{
	return m_nWight;
}

int CImageReader::getHeight()
{
	return m_nHeight;
}

void CImageReader::readImage()
{
	QFileInfo file(m_strImage);

	bool bReadSave = false;
	FILETYPE type = TYPE_NORMAL;
	if (file.suffix().toLower() == "cdr")
	{
		type = TYPE_CDR;
	}
	else if (
		file.suffix().toLower() == "ppt"
		|| file.suffix().toLower() == "pptx"
		//|| file.suffix().toLower() == "doc"
		//|| file.suffix().toLower() == "docx"
		)
	{
		type = TYPE_OFFICE;
	}
	switch (type)
	{
	case TYPE_CDR:
	{
		bReadSave = readCdrPerviewFile();
	}
	case TYPE_OFFICE:
	{
		bReadSave = readPPT();
	}
	default:
		bReadSave = readImageFile();
		break;
	}
}

bool CImageReader::compareColor(QColor color)
{
	int h = 0, s = 0, v = 0;
	QColor hsvColor = color.toHsv();
	hsvColor.getHsv(&h, &s, &v);
	if (m_lstColor.count() == 0)
	{
		return false;
	}
	foreach(QColor colorCompare, m_lstColor)
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
		if ((int)rCom < 50000)
		{
			return true;
		}
	}
	return false;
}

bool CImageReader::readImageFile()
{
	ExceptionInfo *exception;
	exception = AcquireExceptionInfo();
	ImageInfo *imageInfo;
	imageInfo = CloneImageInfo(NULL);

	std::string strFile = m_strImage.toStdString();
	strcpy(imageInfo->filename, strFile.c_str());

	Image *images = ReadImage(imageInfo, exception);

	QuantizeInfo *quantize = AcquireQuantizeInfo(imageInfo);
	Image *thumbnails = NewImageList();
	Image *img = NULL;
	Image *resize_image = NULL;
	while ((img = RemoveFirstImageFromList(&images)) != (Image *)NULL)
	{
		m_nHeight = img->magick_rows;
		m_nWight = img->magick_columns;
		if (QFileInfo(img->filename).suffix() == "gif")
		{
			resize_image = ThumbnailImage(img, m_nWight * m_nRatio, m_nHeight * m_nRatio, exception);
		}
		else
		{
			resize_image = ScaleImage(img, m_nWight * m_nRatio, m_nHeight * m_nRatio, exception);
		}
		if (resize_image == (Image *)NULL)
			MagickError(exception->severity, exception->reason, exception->description);
		(void)AppendImageToList(&thumbnails, resize_image);
		DestroyImage(img);
	}
	/*
	  Write the image thumbnail.
	*/
	std::string strSave = m_strPreview.toStdString();
	(void)strcpy(thumbnails->filename, strSave.c_str());

	//压缩图片颜色，操作时间比较长，可以有效缩小缩略图大小
	//SetImageColorspace(thumbnails, RGBColorspace, exception);
	//CompressImageColormap(thumbnails, exception);
	//quantize->colorspace = RGBColorspace;
	//quantize->number_colors = 20;
	//QuantizeImage(quantize, thumbnails, exception);

	size_t nColors = 0;
	//创建颜色直方图，排序
	PixelInfo *p = GetImageHistogram(thumbnails, &nColors, exception);

	QMap<QString, int> mapColor;
	QList<QColor> lstColor;
	QList<int> lstColorNumber;

	qsort((void *)p, (size_t)nColors, sizeof(*p), HistogramCompare);

	PixelInfo pixel;
	GetPixelInfo(thumbnails, &pixel);
	m_nColorCount = nColors;
	for (int i = 0; i < (ssize_t)nColors; i++)
	{
		pixel = (*p);
		char color[MAX_PATH] = { 0 };
		char hex[MAX_PATH] = { 0 };
		GetColorTuple(&pixel, MagickTrue, hex);
		QString strHex(hex);
		strHex = strHex.left(13);

		if (m_lstColor.indexOf(QColor(strHex)) == -1)
		{
			m_lstColor.append(QColor(strHex));
		}

		//if (m_lstColor.count() == MAX_COLORS)
		//{
		//	break;
		//}
		p++;
	}

	bool b = WriteImage(imageInfo, thumbnails, exception);

	thumbnails = DestroyImageList(thumbnails);
	imageInfo = DestroyImageInfo(imageInfo);
	exception = DestroyExceptionInfo(exception);

	quantize = DestroyQuantizeInfo(quantize);
	return b;
}

bool CImageReader::readCdrPerviewFile()
{
	QuaZip zip(m_strImage);
	zip.open(QuaZip::mdUnzip);
	// first, we need some information about archive itself
	QString comment = zip.getComment();
	// and now we are going to access files inside it
	QuaZipFile file(&zip);
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
	{
		//"/previews/thumbnail.png" cdr预览图片保存位置
		if (zip.getCurrentFileName().contains("thumbnail"))
		{
			file.open(QIODevice::ReadOnly);

			QByteArray imgByte = file.readAll();
			QImage img;
			img.loadFromData(imgByte);
			m_nWight = img.width();
			m_nHeight = img.height();
			//img = img.scaled(img.width(), img.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
			img.save(m_strPreview);
			// do something cool with file here
			file.close(); // do not forget to close!
			m_strImage = m_strPreview;
			return readImageFile();
		}
	}
	return false;
}

bool CImageReader::readPPT()
{
	opcInitLibrary();
	opcContainer *c = opcContainerOpen(_X(m_strImage.toStdString().c_str()), OPC_OPEN_READ_ONLY, NULL, NULL);
	const char *path = m_strPreview.toStdString().c_str();
	if (NULL != c)
	{
		for (opcPart part = opcPartGetFirst(c); OPC_PART_INVALID != part; part = opcPartGetNext(c, part))
		{
			const xmlChar *type = opcPartGetType(c, part);
			if (xmlStrcmp(type, _X("image/jpeg")) == 0)
			{
				extract(c, part, path);
			}
			else if (xmlStrcmp(type, _X("image/png")) == 0)
			{
				extract(c, part, path);
			}
			else
			{
				printf("skipped %s of type %s\n", part, type);
			}
		}
		opcContainerClose(c, OPC_CLOSE_NOW);
	}
	opcFreeLibrary();
	m_strImage = m_strPreview;
	return readImageFile();	
}

void CImageReader::readFile(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsFunction())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		return;
	}
	HandleScope scope(isolate);
	ShareData * req_data = new ShareData;
	req_data->request.data = req_data;
	req_data->isolate = isolate;
	req_data->js_callback.Reset(isolate, Local<Function>::Cast(args[0]));
	//libuv线程池执行读取，异步回调
	uv_queue_work(uv_default_loop(), &(req_data->request), readImageWorkerCb, afterReadImageWorkerCb);
}

void CImageReader::checkColor(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsString())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		args.GetReturnValue().Set(FALSE);
		return;
	}
	v8::String::Utf8Value str(args[0]->ToString());
	QColor color(*str);
	if (color.isValid())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		args.GetReturnValue().Set(FALSE);
		return;
	}
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
	bool bCompare = obj->compareColor(color);
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
			String::NewFromUtf8(isolate, "文件打开失败")));
		args.GetReturnValue().Set(FALSE);
	}

	QByteArray ba = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5);

	file.close();
	QString md5 = ba.toHex();

	args.GetReturnValue().Set(String::NewFromUtf8(isolate, md5.toLatin1().constData()));
}

void CImageReader::colorCount(const FunctionCallbackInfo<Value>& args)
{
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
	args.GetReturnValue().Set(obj->m_nColorCount);
}

//static node.js使用
void CImageReader::colorAt(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsNumber())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		args.GetReturnValue().Set(FALSE);
		return;
	}
	int i = args[0]->NumberValue();

	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	QList<QColor> lstColor = obj->getColorList();
	if (i >= lstColor.count())
	{
		args.GetReturnValue().Set(String::NewFromUtf8(isolate, ""));
		return;
	}
	QColor color = lstColor.at(i);
	args.GetReturnValue().Set(String::NewFromUtf8(isolate, color.name().toLatin1().constData()));
	return;
}

void CImageReader::ImageWidth(const FunctionCallbackInfo<Value>& args)
{
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	args.GetReturnValue().Set(obj->getWigth());
}

void CImageReader::ImageHeigth(const FunctionCallbackInfo<Value>& args)
{
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	args.GetReturnValue().Set(obj->getHeight());
}
