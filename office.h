#include <QString>
#include <QIODevice>
#include <stdio.h>
#include <Windows.h>
#include <QDebug>
#include <iostream>

using namespace std;

extern __declspec(dllexport) void PPT2Image(const char* wcPPT, const char* wcSave);
