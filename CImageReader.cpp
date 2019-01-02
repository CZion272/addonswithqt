#include "CImageReader.h"
#include <QDebug>
#include "qmath.h"
#include <QImageWriter>
#include <QBuffer>
#include <QTimer>
#include <QCryptographicHash>
#include <QTemporaryFile>
#include "opc/opc.h"
#include <QThread>

using namespace std;
Persistent<Function> CImageReader::m_pConstructor;

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

uv_sem_t g_pSemSave = NULL;

CImageReader::CImageReader(const char* image, const char* preview) :
	m_strImage(image),
	m_strPreview(preview),
	m_nHeight(0),
	m_nWight(0),
	m_nScaleWight(0),
	m_nScaleHeight(0),
	m_fRatio(0),
	m_strMiddleFile(""),
	m_nColorCount(0)
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
	m_strSufix = QFileInfo(m_strImage).suffix().toLower();
}

CImageReader::~CImageReader()
{
	delete m_pReqData;
}

void CImageReader::Init(Local<Object> exports)
{
	Isolate* isolate = exports->GetIsolate();

	// 准备构造函数模版
	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
	tpl->SetClassName(String::NewFromUtf8(isolate, "CImageReader"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// 原型
	NODE_SET_PROTOTYPE_METHOD(tpl, "setDefaultColorList", setDefaultColorList);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setDefaultImageSize", setDefaultImageSize);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setDefaultMD5", setDefaultMD5);

	NODE_SET_PROTOTYPE_METHOD(tpl, "setMiddleFile", setMiddleFile);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setPreviewSize", setPreviewSize);
	NODE_SET_PROTOTYPE_METHOD(tpl, "readFile", readFile);
	NODE_SET_PROTOTYPE_METHOD(tpl, "cancel", cancel);

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
	ShareData * my_data = static_cast<ShareData *>(req->data);
	if (my_data->m_pSemLast)
	{
		uv_sem_wait(&my_data->m_pSemLast);
	}
	my_data->obj->readImage();
}

void CImageReader::afterReadImageWorkerCb(uv_work_t * req, int status)
{
	ShareData * my_data = static_cast<ShareData *>(req->data);
	if (status == UV_ECANCELED)
	{
		delete my_data;
		return;
	}

	if (my_data->m_pSemThis)
	{
		uv_sem_post(&my_data->m_pSemThis);
	}

	Isolate * isolate = my_data->isolate;
	HandleScope scope(isolate);
	Local<Function> js_callback = Local<Function>::New(isolate, my_data->js_callback);
	js_callback->Call(isolate->GetCurrentContext()->Global(), 0, NULL);

	if (my_data->m_pSemThis)
	{
		uv_sem_destroy(&my_data->m_pSemThis);
		g_pSemSave = NULL;
	}

	delete my_data;
	m_pConstructor.Reset();
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
			String::NewFromUtf8(isolate, "参数错误")));
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

bool CImageReader::compareColorEx(QColor color)
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

void CImageReader::readImage()
{
	bool bReadSave = false;
	FILETYPE type = TYPE_NORMAL;
	if (m_strSufix == "cdr")
	{
		type = TYPE_CDR;
	}
	else if (
		m_strSufix == "ppt"
		|| m_strSufix == "pptx"
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

bool CImageReader::readImageFile()
{
	ExceptionInfo *exception;
	exception = AcquireExceptionInfo();

	ImageInfo *imageInfo, *thumbnailsInfo;
	imageInfo = CloneImageInfo(NULL);
	thumbnailsInfo = CloneImageInfo(NULL);

	strcpy(imageInfo->filename, m_strImage.toStdString().c_str());
	strcpy(thumbnailsInfo->filename, m_strPreview.toStdString().c_str());
	Image *images = ReadImage(imageInfo, exception);

	if (images == NULL)
	{
		QImage qImag(m_strImage);
		if (!qImag.isNull())
		{
			QFileInfo fInfo(imageInfo->filename);
			QString tempFile(fInfo.baseName() + "." + fInfo.suffix());
			qImag.save(fInfo.baseName() + "." + fInfo.suffix());
			strcpy(imageInfo->filename, tempFile.toStdString().c_str());
			images = ReadImage(imageInfo, exception);
			QFile::remove(tempFile);
		}
		else
		{
			return false;
		}
	}
	Image *thumbnails = NewImageList();

	Image *image = NULL;
	if ((image = RemoveFirstImageFromList(&images)) != (Image *)NULL)
	{
		m_nHeight = image->magick_rows;
		m_nWight = image->magick_columns;

		if (m_nHeight == 0 || m_nWight == 0)
		{
			return false;
		}

		if (m_nHeight > m_nWight)
		{
			if (m_nScaleHeight != 0)
			{
				m_fRatio = (float)m_nScaleHeight / (float)m_nHeight;
			}
			else
			{
				m_fRatio = (float)m_nScaleWight / (float)m_nWight;
			}
		}
		else
		{
			if (m_nScaleWight != 0)
			{
				m_fRatio = (float)m_nScaleWight / (float)m_nWight;
			}
			else
			{
				m_fRatio = (float)m_nScaleHeight / (float)m_nHeight;
			}
		}
		if (m_fRatio == 0.00)
		{
			m_fRatio = 1.0;
		}

		if (m_strSufix == "gif")
		{
			thumbnails = ThumbnailImage(image, m_nWight * m_fRatio, m_nHeight * m_fRatio, exception);
		}
		else
		{
			thumbnails = ScaleImage(image, m_nWight * m_fRatio, m_nHeight * m_fRatio, exception);
		}
		if (thumbnails == (Image *)NULL)
			MagickError(exception->severity, exception->reason, exception->description);

		if (!m_strMiddleFile.isEmpty())
		{
			if (//ppt和cdr均已保存过中间文件
				m_strSufix != "ppt"
				|| m_strSufix != "pptx"
				|| m_strSufix != "cdr"
				)
			{
				(void)strcpy(image->filename, m_strMiddleFile.toStdString().c_str());
				WriteImage(imageInfo, image, exception);
			}
		}
		image = DestroyImage(image);
	}


	//压缩图片颜色，操作时间比较长，可以有效缩小缩略图大小
	//SetImageColorspace(thumbnails, RGBColorspace, exception);
	//CompressImageColormap(thumbnails, exception);
	//quantize->colorspace = RGBColorspace;
	//quantize->number_colors = 20;
	//QuantizeImage(quantize, thumbnails, exception);

	size_t nColors = 0;
	//创建颜色直方图，排序
	if (thumbnails == NULL)
	{
		images = DestroyImage(images);
		return false;
	}

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

		p++;
	}
	/*
	  Write the image thumbnail.
	*/
	(void)strcpy(thumbnails->filename, m_strPreview.toStdString().c_str());
	bool b = WriteImage(thumbnailsInfo, thumbnails, exception);

	images = DestroyImageList(images);

	thumbnails = DestroyImageList(thumbnails);

	imageInfo = DestroyImageInfo(imageInfo);

	thumbnailsInfo = DestroyImageInfo(thumbnailsInfo);

	exception = DestroyExceptionInfo(exception);
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
	QString strFileToSave = m_strMiddleFile.isEmpty() ? m_strPreview : m_strMiddleFile;
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
			img.save(strFileToSave);
			// do something cool with file here
			file.close(); // do not forget to close!
			m_strImage = strFileToSave;
			return readImageFile();
		}
	}
	return false;
}

bool CImageReader::readPPT()
{
	opcInitLibrary();
	opcContainer *c = opcContainerOpen(_X(m_strImage.toStdString().c_str()), OPC_OPEN_READ_ONLY, NULL, NULL);

	const char * path = m_strMiddleFile.isEmpty() ? m_strPreview.toStdString().c_str() : m_strMiddleFile.toStdString().c_str();

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
	m_strImage = path;

	return readImageFile();
}

void CImageReader::setDefaultColorList(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsString() || !args[1]->IsString())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		return;
	}
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	v8::String::Utf8Value str(args[0]->ToString());

	v8::String::Utf8Value strSplit(args[1]->ToString());

	QString strColor(*str);

	QStringList lstColor = strColor.split(*strSplit);

	foreach(QString strC, lstColor)
	{
		obj->m_lstColor.append(QColor(strC));
	}
}

void CImageReader::setDefaultImageSize(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsNumber() || !args[1]->IsNumber())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		return;
	}
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	obj->m_nWight = args[0]->IntegerValue();
	obj->m_nHeight = args[1]->IntegerValue();
}

void CImageReader::setDefaultMD5(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsString())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		return;
	}
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	v8::String::Utf8Value str(args[0]->ToString());

	obj->m_strMd5 = *str;
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
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	QFileInfo info(obj->m_strImage);
	if (!info.isFile())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "无效文件")));
		args.GetReturnValue().Set(false);
		return;
	}

	HandleScope scope(isolate);
	ShareData * pReqData = new ShareData;
	pReqData->request.data = pReqData;
	pReqData->isolate = isolate;
	pReqData->js_callback.Reset(isolate, Local<Function>::Cast(args[0]));
	pReqData->obj = obj;
	obj->m_pReqData = pReqData;
	//libuv线程池执行读取，异步回调
	uv_sem_t sem;
	uv_sem_init(&sem, 0);
	pReqData->m_pSemLast = g_pSemSave;
	g_pSemSave = sem;
	pReqData->m_pSemThis = sem;
	uv_queue_work(uv_default_loop(), &(pReqData->request), readImageWorkerCb, afterReadImageWorkerCb);

	args.GetReturnValue().Set(true);
}

void CImageReader::cancel(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
	uv_work_s* w = &obj->m_pReqData->request;
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
			String::NewFromUtf8(isolate, "参数错误")));
		args.GetReturnValue().Set(FALSE);
		return;
	}
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());
	obj->m_nScaleWight = args[0]->IntegerValue();
	obj->m_nScaleHeight = args[1]->IntegerValue();
}

void CImageReader::setMiddleFile(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsString())
	{
		isolate->ThrowException(v8::Exception::TypeError(
			String::NewFromUtf8(isolate, "参数错误")));
		args.GetReturnValue().Set(FALSE);
		return;
	}
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	v8::String::Utf8Value str(args[0]->ToString());
	obj->m_strMiddleFile = *str;
}

void CImageReader::compareColor(const FunctionCallbackInfo<Value>& args)
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
	bool bCompare = obj->compareColorEx(color);
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

	obj->m_strMd5 = md5;
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

void CImageReader::imageWidth(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	args.GetReturnValue().Set(obj->getWigth());
}

void CImageReader::imageHeight(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
	CImageReader* obj = ObjectWrap::Unwrap<CImageReader>(args.Holder());

	args.GetReturnValue().Set(obj->getHeight());
}
