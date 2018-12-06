#include "CImageReader.h"
#include <QDebug>
#include "qmath.h"

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

using namespace std;
Persistent<Function> CImageReader::constructor;

static QColor gWhite(Qt::white);

CImageReader::CImageReader(const char* image, const char* preview, float nRatio) :
	m_strImage(image),
	m_strPreview(preview),
	m_nHeight(0),
	m_nWight(0),
	m_nRatio(nRatio)
{
	qDebug() << m_strImage << m_strPreview << image << preview;
	readImage(m_strImage);
}

CImageReader::~CImageReader()
{
}

void CImageReader::Init(Local<Object> exports)
{
	Isolate* isolate = exports->GetIsolate();

	// 准备构造函数模版
	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
	tpl->SetClassName(String::NewFromUtf8(isolate, "CImageReader"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// 原型
	NODE_SET_PROTOTYPE_METHOD(tpl, "colorAt", colorAt);
	NODE_SET_PROTOTYPE_METHOD(tpl, "ImageWidth", ImageWidth);
	NODE_SET_PROTOTYPE_METHOD(tpl, "ImageHeigth", ImageHeigth);

	constructor.Reset(isolate, tpl->GetFunction());
	exports->Set(String::NewFromUtf8(isolate, "CImageReader"),
		tpl->GetFunction());
}

void CImageReader::New(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();

	// 像构造函数一样调用：`new MyObject(...)`
	v8::String::Utf8Value str(args[0]->ToString());
	const char *chImage = *str;
	v8::String::Utf8Value str1(args[1]->ToString());
	const char *chImage1 = *str1;
	printf(chImage);
	CImageReader* obj = new CImageReader(chImage, chImage1);
	obj->Wrap(args.This());
	args.GetReturnValue().Set(args.This());

}

static double gR = 100;
static double gAngle = 30;
static double gh = gR * qCos((float)gAngle / 180 * M_PI);
static double gr = gR * qSin((float)gAngle / 180 * M_PI);

bool CImageReader::compareColor(QColor color)
{
	int h = 0, s = 0, v = 0;
	QColor hsvColor = color.toHsv();
	hsvColor.getHsv(&h, &s, &v);
	if (gWhite == color)
	{
		return false;
	}
	if (m_lstColor.count() == 0)
	{
		return true;
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
		if ((int)rCom < 500000)
		{
			return false;
		}
	}
	return true;
}

void CImageReader::creatColorList()
{
	//如果图片存在颜色列表，只读取颜色列表中的色彩值
	if (m_imgPreview.colorCount() > 0)
	{
		for (int i = 0; i < m_imgPreview.colorCount(); i++)
		{
			m_lstColor.append(m_imgPreview.color(i));
		}
		return;
	}
	//遍历图片所有像素获取5种差距较大的颜色
	for (int i = 0; i < m_imgPreview.width(); i++)
	{
		for (int j = 0; j < m_imgPreview.height(); j++)
		{
			QColor color(m_imgPreview.pixelColor(i, j));

			if (compareColor(color))
			{
				m_lstColor.append(color);
				if (m_lstColor.count() >= 5)
				{
					return;
				}
			}
		}
	}
}

bool CImageReader::readCdrPerviewFile(QString strZip)
{
	QuaZip zip(strZip);
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
			img = img.scaled(img.width()*m_nRatio, img.height()*m_nRatio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			img.save(m_strPreview);
			// do something cool with file here
			file.close(); // do not forget to close!
			return true;
		}
	}
	return false;
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

void CImageReader::readImage(QString strImage)
{
	m_strImage = strImage;
	QFileInfo file(m_strImage);

	bool bReadSave = false;
	FILETYPE type;
	if (file.suffix() == "cdr")
	{
		type = TYPE_CDR;
	}
	else
	{

	}
	switch (type)
	{
	case CImageReader::TYPE_PNG:
		break;
	case CImageReader::TYPE_JPG:
		break;
	case CImageReader::TYPE_GIF:
		break;
	case CImageReader::TYPE_BIT:
		break;
	case CImageReader::TYPE_PBM:
		break;
	case CImageReader::TYPE_PGM:
		break;
	case CImageReader::TYPE_PPM:
		break;
	case CImageReader::TYPE_XBM:
		break;
	case CImageReader::TYPE_XPM:
		break;
	case CImageReader::TYPE_SVG:
		break;
	case CImageReader::TYPE_CDR:
	{
		if (readCdrPerviewFile(m_strImage))
		{
			bReadSave = m_imgPreview.load(m_strPreview);
		}
	}
	break;
	case CImageReader::TYPE_AI:
		break;
	case CImageReader::TYPE_TIFF:
		break;
	case CImageReader::TYPE_PSD:
		break;
	default:
		break;
	}
	if (bReadSave)
	{
		creatColorList();
	}
}

void CImageReader::colorAt(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate = args.GetIsolate();
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

