#include "pch.h"
#include <string>
#include <ctime>
#include <algorithm>
#include <functional>
#include <iostream>
//#include <fstream>
#include <iterator>
#include <windows.h>
#include "combaseapi.h"




int main() 
{
    std::string s ("He\0llo",6);
    //std::string s2("llo");
    std::string s2("\0",1);
    std::cout << s.find(s2) << std::endl;
    char* c = (char*)s.c_str();
    std::string s3(c, 6);
    std::cout << s << std::endl;
    std::cout << c << std::endl;
    //std::cout << s << std::endl << c;
    std::cout << s3 << std::endl;

    GUID  guid1, guid2;
    if (CoCreateGuid(&guid1)) { throw - 1; }
    if (CoCreateGuid(&guid2)) { throw - 1; }
    LPOLESTR guidString1 = NULL, guidString2 = NULL;


    if (SUCCEEDED(StringFromCLSID(guid1, &guidString1))) { throw - 1; }
    if (SUCCEEDED(StringFromCLSID(guid2, &guidString2))) { throw - 1; }


    std::wstring guidString3(L"{CCCCCCCC-CCCC-CCCC-CCCC-CCCCCCCCCCCC}");//39 символов с нуль терминатором
    //wchar_t guidString4[39];//с нуль символом
    int res = StringFromGUID2(guid1, guidString3.data(), guidString3.length()+1);

    std::wcout << guidString1 << std::endl;
    std::wcout << guidString2 << std::endl;
    std::wcout << guidString3 << std::endl;

    CoTaskMemFree(guidString1);
    CoTaskMemFree(guidString2);

    std::cin.get();
	return 0;
}