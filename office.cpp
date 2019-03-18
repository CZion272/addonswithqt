#include "office.h"

#using "../vcruntimelibrary/Microsoft.Office.Interop.PowerPoint.dll" 
#using "../vcruntimelibrary/Microsoft.Office.Core.dll" 


using namespace Microsoft;
using namespace Microsoft::Office;
using namespace Microsoft::Office::Interop;
using namespace Microsoft::Office::Interop::PowerPoint;
using namespace Microsoft::Office::Core;
using namespace Microsoft::Win32;
using namespace System;

bool PPT2Image(const char* chPPT, const char* chSave)
{
    ApplicationClass ^pptApp = gcnew ApplicationClass();
    
    String^ strFile = System::Runtime::InteropServices::Marshal::PtrToStringAnsi((IntPtr)(char*)chPPT);
    String^ strSave = System::Runtime::InteropServices::Marshal::PtrToStringAnsi((IntPtr)(char*)chSave);
    Presentation ^ppt = pptApp->Presentations->Open(strFile, MsoTriState::msoFalse, MsoTriState::msoFalse, MsoTriState::msoFalse);
    if (!ppt)
    {
        pptApp->Quit();
        return false;
    }
    Slide ^silde = ppt->Slides[1];
    if (!silde)
    {
        ppt->Close();
        pptApp->Quit();
        return false;
    }
    silde->Export(strSave, "PNG", 960, 600);
    ppt->Close();
    pptApp->Quit();
    return true;
}