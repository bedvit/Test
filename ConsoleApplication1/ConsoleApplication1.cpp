#include "pch.h"
#include <string>
#include <ctime>
#include <algorithm>
#include <functional>
#include <iostream>
//#include <fstream>
#include <iterator>




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
    std::cin.get();
	return 0;
}