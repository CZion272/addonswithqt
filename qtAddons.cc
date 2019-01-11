#include <node.h>
#include <QDebug>
#include "CImageReader.h"
#include "qmath.h"

namespace qtAddons
{

	static double gR = 100;
	static double gAngle = 30;
	static double gh = gR * qCos((float)gAngle / 180 * M_PI);
	static double gr = gR * qSin((float)gAngle / 180 * M_PI);

	using v8::Local;
	using v8::Object;
	bool compareColorEx(QColor color, QList<QColor> lstColor)
	{
		int h = 0, s = 0, v = 0;
		QColor hsvColor = color.toHsv();
		hsvColor.getHsv(&h, &s, &v);
		if (lstColor.count() == 0)
		{
			return false;
		}
		foreach(QColor colorCompare, lstColor)
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
			if ((int)rCom < 10000)
			{
				return true;
			}
		}
		return false;
	}
	void hasColor(const FunctionCallbackInfo<Value>& args)
	{
		Isolate* isolate = args.GetIsolate();
		if (!args[0]->IsString() || !args[1]->IsString())
		{
			isolate->ThrowException(Exception::TypeError(
				String::NewFromUtf8(isolate, "²ÎÊý´íÎó")));
			return;
		}
		int i = 1;
		v8::String::Utf8Value str(args[0]->ToString());
		QString strColor = *str;
		QList<QColor> lstColor;
		while (args[i]->IsString())
		{
			v8::String::Utf8Value strL(args[i]->ToString());
			QString strColorList = *strL;
			lstColor.append(QColor(strColorList));
			i++;
		}

		args.GetReturnValue().Set(compareColorEx(QColor(strColor), lstColor));
	}

	void InitAll(Local<Object> exports)
	{
		CImageReader::Init(exports);
		NODE_SET_METHOD(exports, "hasColor", hasColor);
	}

	NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}