#include <stdio.h>
#include <Windows.h>
#include <iostream>

using namespace std;

extern __declspec(dllexport) bool PPT2Image(const char* wcPPT, const char* wcSave);
