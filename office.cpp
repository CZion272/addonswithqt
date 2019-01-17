#include "office.h"
#include "opc/opc.h"

static inline std::string ws2s(const wstring& s)
{
	int
		len;

	std::string
		result;

	len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.length() + 1, 0, 0, 0, 0);
	result = std::string(len, '\0');
	(void)WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.length() + 1, &result[0],
		len, 0, 0);
	return result;
}


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

void PPT2Image(const char* chPPT, const char* chSave)
{
#ifdef WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	opcInitLibrary();
	opcContainer *c = opcContainerOpen(_X(chPPT), OPC_OPEN_READ_ONLY, NULL, NULL);
	if (NULL != c)
	{
		qDebug() << 123;
		for (opcPart part = opcPartGetFirst(c); OPC_PART_INVALID != part; part = opcPartGetNext(c, part))
		{
			const xmlChar *type = opcPartGetType(c, part);
			if (xmlStrcmp(type, _X("image/jpeg")) == 0)
			{
				extract(c, part, chSave);
			}
			else if (xmlStrcmp(type, _X("image/png")) == 0)
			{
				extract(c, part, chSave);
			}
			else
			{
				printf("skipped %s of type %s\n", part, type);
			}
		}
		opcContainerClose(c, OPC_CLOSE_NOW);
	}
#ifdef WIN32
	OPC_ASSERT(!_CrtDumpMemoryLeaks());
#endif
	opcFreeLibrary();
}