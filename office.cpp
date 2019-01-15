#include "office.h"
#include "opc/opc.h"
#include <QDebug>

//pptµ¼³öÔ¤ÀÀÍ¼Æ¬
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

void PPT2Image(QString strPPT, QString strSave)
{
	opcInitLibrary();
	opcContainer *c = opcContainerOpen(_X(strPPT.toStdString().c_str()), OPC_OPEN_READ_ONLY, NULL, NULL);

	const char * chSavePath = strSave.toStdString().c_str();

	if (NULL != c)
	{
		for (opcPart part = opcPartGetFirst(c); OPC_PART_INVALID != part; part = opcPartGetNext(c, part))
		{
			const xmlChar *type = opcPartGetType(c, part);
			if (xmlStrcmp(type, _X("image/jpeg")) == 0)
			{
				extract(c, part, chSavePath);
			}
			else if (xmlStrcmp(type, _X("image/png")) == 0)
			{
				extract(c, part, chSavePath);
			}
			else
			{
				printf("skipped %s of type %s\n", part, type);
			}
		}
		opcContainerClose(c, OPC_CLOSE_NOW);
	}
	opcFreeLibrary();
}