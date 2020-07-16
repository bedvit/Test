// Test.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <fstream>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <Windows.h>
#include <vector>
#include <exception>
#include <stdio.h>
#include <algorithm>
#include <functional>
#include <ppl.h>




size_t GetRowsCountCSVansi(PCTSTR file, bool fileFlagNoBuffering)
{
	// создаем события с автоматическим сбросом
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) { return GetLastError(); }

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;
	
	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEvent;   // событие для оповещения завершения чтения

	// открываем файл для чтения
	HANDLE hFile = CreateFile(
		file,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEvent);
		return -1;
	}
	//FlushFileBuffers(hFile); //Сбрасывает буферы указанного файла и вызывает запись всех буферизованных данных в файл.

	DWORD  bytesReadTotal = 0;
	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты

	char* notAlignBuf = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер
	char* buf = notAlignBuf + nNumberOfBytesToRead;; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufWork = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufWork + nNumberOfBytesToRead; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  

	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	char* find;// указатель для поиска
	size_t strCount = 1; //счетчик строк
	bool errHandleEOF = false;

	// читаем данные из файла
	for (;;)
	{
		DWORD  bytesRead;
		DWORD  error;
		find = buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&bytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (error = GetLastError())
			{
			case ERROR_IO_PENDING:
				// асинхронный ввод-вывод все еще происходит 
				// сделаем кое-что пока он идет 
				break;
			case ERROR_HANDLE_EOF:
				// мы достигли конца файла 
				// закроем файл после обработки данных в рабочем буфере
				errHandleEOF = true;
				break;
			default:
				// закрываем дескрипторы
				goto return1;
			}
		}

		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		bufWork[bytesReadTotal] = '\0';//добавим нуль-терминатор
		find = bufWork; //буфер
		find = strchr(find, '\n');
		while (find != NULL)
		{
			find = find + 1;
			strCount++;
			find = strchr(find, '\n');
		}
		if (errHandleEOF)
		{
			// мы достигли конца файла в ходе асинхронной операции	
			// закрываем дескрипторы
			goto return0;
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		// ждем, пока завершится асинхронная операция чтения// проверим результат работы асинхронного чтения 
		WaitForSingleObject(hEvent, INFINITE);// INFINITE);

		// если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl, &bytesRead, FALSE))
		{
			// решаем что делать с кодом ошибки
			switch (error = GetLastError())
			{
			case ERROR_HANDLE_EOF: 
			{
				// мы достигли конца файла в ходе асинхронной операции	
				// закрываем дескрипторы
				goto return0;
			}
			// решаем что делать с другими случаями ошибок
			default:
				goto return1;
			}// конец процедуры switch (error = GetLastError())
		}

		//меняем буфер
		char* bufTmp = bufWork;
		bufWork = buf;
		buf = bufTmp;

		// увеличиваем смещение в файле
		bytesReadTotal = bytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}
return0:
	// закрываем дескрипторы
	CloseHandle(hFile);
	CloseHandle(hEvent);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return strCount;
return1:
	CloseHandle(hFile);
	CloseHandle(hEvent);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return -1;
}

int GetRowCSVansi(PCTSTR file, int strNum, bool fileFlagNoBuffering)
{
		// создаем события с автоматическим сбросом
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) { return GetLastError(); }

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу
	// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEvent;   // событие для оповещения завершения чтения

	// открываем файл для чтения
	HANDLE hFile = CreateFile(
		file,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEvent);
		return -1;
	}

	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты

	char* notAlignBuf = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер
	char* buf = notAlignBuf + nNumberOfBytesToRead;; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufWork = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufWork + nNumberOfBytesToRead; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  

	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	char* find;// указатель для поиска
	char* findNext;// указатель для поиска следующий
	int strCount = 1; //счетчик строк
	std::string strOut;
	DWORD  bytesReadTotal;
	bool errHandleEOF = false;

	// читаем данные из файла
	for (;;)
	{
		DWORD  bytesRead;
		DWORD  error;
		find= buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&bytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (error = GetLastError())// решаем что делать с кодом ошибки
			{
			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			case ERROR_HANDLE_EOF: {errHandleEOF = true; break; } // мы достигли конца файла читалкой ReadFile
			default: {goto return1; }// другие ошибки
			}
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		if (ui.QuadPart > 0)//если рабочий буфер заполнен
		{
			bufWork[bytesReadTotal] = '\0';//добавим нуль-терминатор
			find = bufWork; //новый буфер
			do //считаем строки в буфере
			{
				if (strCount == strNum)
				{
					findNext = strchr(find, '\n');
					if (findNext == NULL)
					{
						strOut = strOut + std::string(find);
					}
					else
					{
						findNext++;
						strOut = strOut + std::string(find, size_t(findNext - find));
						goto return0;
					}
				}
				find = strchr(find, '\n');
				if (find != NULL)
				{
					find++;
					strCount++;
				}
			} while (find != NULL);

		}
		if (errHandleEOF){ goto return0; }
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEvent, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl,&bytesRead, FALSE))
		{
			switch (error = GetLastError())// решаем что делать с кодом ошибки
			{
				case ERROR_HANDLE_EOF: {goto return0; }// мы достигли конца файла в ходе асинхронной операции
				default: {goto return1; }// другие ошибки
			}// конец процедуры switch (error = GetLastError())
		}

		//меняем буфер
		char* bufTmp = bufWork;
		bufWork = buf;
		buf = bufTmp;

		// увеличиваем смещение в файле
		bytesReadTotal = bytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}

return0:
	// закрываем дескрипторы
	CloseHandle(hFile);
	CloseHandle(hEvent);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return 0;
return1:
	CloseHandle(hFile);
	CloseHandle(hEvent);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return -1;
}

//int FindRowsInCSVansi0(PCTSTR file, const char* findStr, bool multiLine, bool fileFlagNoBuffering)
//{
//
//// создаем события с автоматическим сбросом
//HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
//if (hEvent == NULL) { return GetLastError(); }
//
//_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
//ui.QuadPart = 0;
//
//OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
//ovl.Offset = 0;         // младшая часть смещения равна 0
//ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
//ovl.hEvent = hEvent;   // событие для оповещения завершения чтения
//
//// открываем файл для чтения
//HANDLE hFile = CreateFile(	// дескриптор файла
//	file,   // имя файла
//	GENERIC_READ,          // чтение из файла
//	FILE_SHARE_READ,       // совместный доступ к файлу
//	NULL,                  // защиты нет
//	OPEN_EXISTING,         // открываем существующий файл
//	FILE_FLAG_OVERLAPPED | (fileFlagNoBuffering ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
//	NULL                   // шаблона нет
//);
//// проверяем на успешное открытие
//if (hFile == INVALID_HANDLE_VALUE)
//{
//	CloseHandle(hEvent);
//	return -1;
//}
//
//const DWORD  nNumberOfBytesToRead = 16777216;//67108864;//33554432; //16777216;//8388608;//читаем в буфер байты
//char* notAlignBuf = new char[nNumberOfBytesToRead + 4096]; //буфер
//char* buf = notAlignBuf; //буфер
//if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 
//
//char* notAlignBufWork = new char[nNumberOfBytesToRead + 4096 + 1]; //буфер Рабочий
//char* bufWork = notAlignBufWork; //буфер
//if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  
//
//bufWork[0] = '\0';//добавим нуль-терминатор
//bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
//char* find;// указатель для поиска
//size_t strCount = 1; //счетчик строк
//size_t findStrLen = strlen(findStr); //счетчик строк
//std::string strOut; //итоговая строка
//DWORD bytesReadTotal;
//DWORD findStatus = 0; //статус поиска
//ULONGLONG ignoreOffset = 0; //для расчета холостых циклов
//bool errHandleEOF = false; //метка конца файла
//
//// читаем данные из файла
//for (;;)
//{
//	DWORD  bytesRead;
//	DWORD  error;
//	find = buf; //буфер
//	// читаем одну запись
//	if (!ReadFile(
//		hFile,           // дескриптор файла
//		buf,             // адрес буфера, куда читаем данные
//		nNumberOfBytesToRead,// количество читаемых байтов
//		&bytesRead,    // количество прочитанных байтов
//		&ovl             // чтение асинхронное
//	))
//	{
//		switch (error = GetLastError())// решаем что делать с кодом ошибки
//		{
//			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
//		case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
//		case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
//		default: {goto return1; }// другие ошибки
//		}
//	}
//	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
//	if (ui.QuadPart != ignoreOffset)//игнорируем первую итерацию и этерации прошедшие следующим буфером, когда нужно было вернутся в предыдущий буфер
//	{
//		bufWork[bytesReadTotal] = '\0';//добавим нуль-терминатор
//		find = bufWork; //новый буфер
//	go_1:
//		char* strStart = bufWork;
//		char* strEnd = NULL;
//
//		if (findStatus == 0)//goFind
//		{
//			find = strstr(find, findStr);
//			if (find != NULL) //если нужная подстрока найдена
//			{
//
//				for (strStart = find; strStart >= bufWork; strStart--)
//				{
//					if (*strStart == '\n') { break; } //если нашли начало строки
//				}
//				strStart++;
//				if (ui.QuadPart == nNumberOfBytesToRead || strStart != bufWork)//если первый буфер или найдено начало строки
//				{
//					strEnd = strchr(find, '\n');
//					if (strEnd != NULL) //если нашли конец строки
//					{
//						strOut = strOut + std::string(strStart, strEnd - strStart + 1);
//						if (!multiLine)
//						{
//							goto return0;
//						}
//						else//поиск в следующей строке начинаем с конца предыдущей
//						{
//							find = strEnd++;
//							findStatus = 0;
//							goto go_1;
//						}
//					}
//					else//если конец строки в следующем буфере или конец файла
//					{
//						strOut = strOut + std::string(strStart);
//						findStatus = 1;
//					}
//				}
//				else //если начало строки не найдено и это не первый буфер смотрим предыдущий буфер
//				{
//					findStatus = 2;
//					ui.QuadPart -= (nNumberOfBytesToRead) * 3;
//					ignoreOffset = ui.QuadPart + nNumberOfBytesToRead;
//				}
//			} //если нужная подстрока найдена
//		}
//		else if (findStatus == 1)//goBuf
//		{
//			strEnd = strchr(find, '\n');
//			if (strEnd != NULL) //если нашли конец строки
//			{
//				strOut = strOut + std::string(find, strEnd - find+1);
//				findStatus = 0;
//				if (!multiLine)
//				{
//					goto return0;
//				}
//				else//поиск в следующей строке начинаем с конца предыдущей
//				{
//					find = strEnd++;
//					findStatus = 0;
//					goto go_1;
//				}
//			}
//			else//если конец строки в следующем буфере или конец файла
//			{
//				strOut = strOut + std::string(find);
//			}
//		}
//		else if (findStatus == 2)//goBackBuf
//		{
//
//			for (strStart = bufWork + bytesReadTotal; strStart >= bufWork; strStart--)
//			{
//				if (*strStart == '\n') { break; }//если нашли начало строки
//			}
//			strStart++;
//			if (ui.QuadPart == nNumberOfBytesToRead || strStart != bufWork)//если первый буфер или найдено начало строки
//			{
//				find = strStart++;
//				findStatus = 1;
//				goto go_1;
//			}
//			else
//			{
//				ui.QuadPart -= nNumberOfBytesToRead * 3;//если начало строки не найдлено смотрим предыдущий буфер
//				ignoreOffset = ui.QuadPart + nNumberOfBytesToRead;
//			}
//		}
//	}
//	if (errHandleEOF) { if (findStatus != 2) goto return0; }
//	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
//
//	// ждем, пока завершится асинхронная операция чтения
//	WaitForSingleObject(hEvent, INFINITE);
//
//	// проверим результат работы асинхронного чтения // если возникла проблема ... 
//	if (!GetOverlappedResult(hFile, &ovl, &bytesRead, FALSE))
//	{
//		switch (error = GetLastError())// решаем что делать с кодом ошибки
//		{
//		case ERROR_HANDLE_EOF: 
//		{
//			if (findStatus != 2) goto return0; break;//если конец файла. но мы должны вернутся найти начало строки.
//		}// мы достигли конца файла в ходе асинхронной операции
//		default: {goto return1; }// другие ошибки
//		}// конец процедуры switch (error = GetLastError())
//	}
//
//	//меняем буфер
//	char* bufTmp = bufWork;
//	bufWork = buf;
//	buf = bufTmp;
//
//	// увеличиваем смещение в файле
//	bytesReadTotal = bytesRead;//кол-во считанных байт
//	ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
//	ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
//	ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
//}
//
//return0:
//// закрываем дескрипторы, освобождаем память
//std::cout << "String Find\n" << strOut << std::endl;
//CloseHandle(hFile);
//CloseHandle(hEvent);
//delete[] notAlignBuf;
//delete[] notAlignBufWork;
//return 0;
//return1:
//CloseHandle(hFile);
//CloseHandle(hEvent);
//delete[] notAlignBuf;
//delete[] notAlignBufWork;
//return -1;
//}

int createfile() //создание файла для теста
{
	const int N = 4000000*3; // количество строк в файле
	std::ofstream fout("C:\\CSV_3_GB.csv");
	std::string str = "Пятый; Тридацть четвертый; 44221100; BBB; CIFRAPOLE; POLEPVTRETYE; ODIN ODIN - TRI; CC; 01.01.2013; 01.01.2013; 8963; 2UTY39ADVGKR; СU707039; 40200М У0026034; -; 11; 2; 150; 1998; 1; -; 21; 1980; 1490; НОМЕР ПЯТЬ; -; ПОЛЕ ОДИН; ПОЛЕ ОДИН ПЯТЬ; -; -";
	if (!fout){	return 1; }

	for (int i = 0; i < N; ++i)
	{
		fout << i + 1;
		fout << str << std::endl;
	}
	fout.close();
	return 0;
}

#pragma warning(disable : 4146)
std::string XLAT(std::string s)
{
	clock_t t1;
	clock_t t2;

	size_t MAXSTR = 1024;
	const char* const NAMEFILE = "C:\\CSV_1_GB.csv";
	t1 = clock();
	std::ifstream is(NAMEFILE, std::ios::in | std::ios::binary);

	size_t size = 100000;
	std::string str; str.assign(size, ' ');

	//std::string s(findStr);
	std::streamoff offset = -s.size();

	std::string sOut;

	while (is)
	{
		is.read((char*)str.data(), size);
		is.seekg(offset, std::ios::cur);

		size_t found = str.find(s);
		if (found != std::string::npos)
		{
			//std::cout << "Find OK: ";

			if (size - found < MAXSTR) // +
			{
				is.seekg(MAXSTR - size, std::ios::cur);
				is.read((char*)str.data(), size);
				found -= MAXSTR;
			}
			else if (size - found > size - MAXSTR) // -
			{
				is.seekg(-MAXSTR - size, std::ios::cur);
				is.read((char*)str.data(), size);
				found += MAXSTR;
			}

			size_t s1 = str.rfind('\n', found);
			size_t s2 = str.find('\n', found);
			//std::cout << str.substr(s1, s2 - s1) << std::endl;
			sOut = sOut + str.substr(s1+1, s2 - s1);
			return sOut;
		};
	}
	is.close();
	//std::cout << sOut << std::endl;
	//printf("   read + Find + seekg     : Time = %f(школа)\n\n",
	//	(clock() - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	return sOut;
}

std::string FindRowsInCSVansi(PCTSTR file, char* findStr, bool multiLine)
{
	const DWORD  nNumberOfBytesToRead = 16777216;//67108864;//33554432; //16777216;//8388608;//читаем в буфер байты
	size_t findStrLen = strlen(findStr);
	if (findStrLen >= nNumberOfBytesToRead) { return ""; };

	// создаем события с автоматическим сбросом
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
	//if (hEvent == NULL) { return GetLastError(); }
	if (hEvent == NULL) {	return ""; 	}

	// открываем файл для чтения
	HANDLE hFile = CreateFile(	// дескриптор файла
		file,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS,//(fileFlagNoBuffering != 0 ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_RANDOM_ACCESS),// асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEvent);
		return "";
	}

	char* notAlignBuf = new char[nNumberOfBytesToRead *2 + 4096 + 1]; //буфер
	char* buf = notAlignBuf + nNumberOfBytesToRead;; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufWork = new char[nNumberOfBytesToRead * 2 + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufWork + nNumberOfBytesToRead; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  
	
	buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор

	DWORD bytesReadTotal = 0;
	DWORD findStatus = 0; //статус поиска
	bool errHandleEOF = false; //метка конца файла
	char* find;// указатель для поиска
	char* strStart;
	char* strEnd;
	char* bufWorkNew = bufWork;//буфер с учетом полной строки
	size_t strStartLen = 0;
	size_t strCount = 1; //счетчик строк
	std::string strOut; //итоговая строка

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEvent;   // событие для оповещения завершения чтения

	// читаем данные из файла

	for (;;)
	{
		DWORD  bytesRead;
		DWORD  error;
		find = buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&bytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (error = GetLastError())// решаем что делать с кодом ошибки
			{
			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
			default: {goto return1; }// другие ошибки
			}
		}

		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		bufWork[bytesReadTotal] = '\0';//добавим нуль-терминатор
	goNextFind: //если ищем вторую и последующие строки в этом же буфере
		if (findStatus == 0)//goFind
		{
			find = strstr(bufWorkNew, findStr);
			if (find != NULL) //если нужная подстрока найдена
			{
				for (strStart = find; strStart >= bufWorkNew; strStart--)
				{
					if (*strStart == '\n') { break; } //если нашли начало строки
				}
				strStart++; //не учитываем '\n'

				strEnd = strchr(find, '\n'); //ищем конец строки
				if (strEnd != NULL) //если нашли конец строки
				{
					strOut = strOut + std::string(strStart, strEnd - strStart + 1);
					if (!multiLine)
					{
						goto return0;
					}
					else//поиск в следующей строке начинаем с конца предыдущей
					{
						bufWorkNew = strEnd++;
						findStatus = 0;
						goto goNextFind;
					}
				}
				else//если конец строки в следующем буфере или конец файла
				{
					strOut = strOut + std::string(strStart);
					findStatus = 1;
				}
			} //если нужная подстрока найдена
		}
		else if (findStatus == 1)//goBuf
		{
			strEnd = strchr(bufWork, '\n');
			if (strEnd != NULL) //если нашли конец строки
			{
				strOut = strOut + std::string(bufWork, strEnd - bufWork + 1);
				findStatus = 0;
				if (!multiLine)
				{
					goto return0;
				}
				else//поиск в следующей строке начинаем с конца предыдущей
				{
					bufWorkNew = strEnd++;
					findStatus = 0;
					goto goNextFind;
				}
			}
			else//если конец строки в следующем буфере или конец файла
			{
				strOut = strOut + std::string(bufWork);
			}
		}

		//блок дозагрузки рабочего буфера межбуферной строкой
		char* bufWorkEnd = bufWork + bytesReadTotal; //конец буфера
		for (strStart = bufWorkEnd; strStart >= bufWork; strStart--) { if (*strStart == '\n') { break; } } //если нашли начало строки
		if (strStart < bufWork && ui.QuadPart>=nNumberOfBytesToRead) { goto return1; }; //если строка больше буфера (не найден разделитель), кроме первого буфера
		strStartLen = (bufWorkEnd - ++strStart); //размер буфера, который добавляем к рабочему
		//пишем начало строки в новый буфер
		bufWorkNew = buf - strStartLen;//bufWorkNew = bufWork - strStartLen;
		memcpy(bufWorkNew, strStart, strStartLen);
		//

		if (errHandleEOF) { goto return0; }
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEvent, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl, &bytesRead, FALSE))
		{
			switch (error = GetLastError())// решаем что делать с кодом ошибки
			{
			case ERROR_HANDLE_EOF:{ goto return0; break; }// мы достигли конца файла в ходе асинхронной операции
			default: {goto return1; }// другие ошибки
			}
		}

		//меняем буфер
		char* bufTmp = bufWork;
		bufWork = buf;
		buf = bufTmp;
		//

		// увеличиваем смещение в файле
		bytesReadTotal = bytesRead;//кол-во считанных байт
		ui.QuadPart += nNumberOfBytesToRead; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}

return0:
	CloseHandle(hFile);
	CloseHandle(hEvent);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return strOut;
return1:
	CloseHandle(hFile);
	CloseHandle(hEvent);
	delete[] notAlignBuf;
	delete[] notAlignBufWork;
	return "";
}



//bool CompareCharPtrEqual(char* lhs, char* rhs) {
//	if (lhs == NULL && rhs == NULL) return true;
//	if (lhs == NULL || rhs == NULL) return false;
//	return strcmp(lhs, rhs) == 0;
//}
//bool CompareCharPtrAscending(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return strcmp(lhs, rhs) < 0;//strcoll
//}
//bool CompareCharPtrDescending(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return 	strcmp(lhs, rhs) > 0;
//}
//bool CompareCharPtrAscendingLoc(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return strcoll(lhs, rhs) < 0;//strcoll
//}
//bool CompareCharPtrDescendingLoc(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return 	strcoll(lhs, rhs) > 0;
//}
bool CompareCharPtrEqual(std::pair <char*, size_t> lhs, std::pair <char*, size_t> rhs) {
	if (lhs.first == NULL && rhs.first == NULL) return true;
	if (lhs.first == NULL || rhs.first == NULL) return false;
	return strcmp(lhs.first, rhs.first) == 0;
}
bool CompareCharPtrAscending(std::pair <char*, size_t> lhs, std::pair <char*, size_t> rhs) {
	if (lhs.first == NULL) return false;
	if (rhs.first == NULL) return true;
	return strcmp(lhs.first, rhs.first) < 0;//strcoll
}
bool CompareCharPtrDescending(std::pair <char*, size_t> lhs, std::pair <char*, size_t> rhs) {
	if (lhs.first == NULL) return false;
	if (rhs.first == NULL) return true;
	return 	strcmp(lhs.first, rhs.first) > 0;
}
bool CompareCharPtrAscendingLoc(std::pair <char*, size_t> lhs, std::pair <char*, size_t> rhs) {
	if (lhs.first == NULL) return false;
	if (rhs.first == NULL) return true;
	return strcoll(lhs.first, rhs.first) < 0;//strcoll
}
bool CompareCharPtrDescendingLoc(std::pair <char*, size_t> lhs, std::pair <char*, size_t> rhs) {
	if (lhs.first == NULL) return false;
	if (rhs.first == NULL) return true;
	return 	strcoll(lhs.first, rhs.first) > 0;
}


size_t SortDeleteDuplicateRowsCSVansi(LPCWSTR FileIn, LPCWSTR FileOut, int HeaderRowsCount, int OnlySort, int SortOrder, int SetLocale, char* Locale)
{
	clock_t t1 = clock();
	if (SetLocale != 0) { if (setlocale(LC_COLLATE, Locale) == NULL) { return -1; } }//"ru-RU"

	std::unique_ptr<void, decltype(&CloseHandle)>CreateEventUniquePtr(CreateEvent(NULL, FALSE, FALSE, NULL), &CloseHandle);// дескриптор события// создаем события с автоматическим сбросом
	HANDLE hEvent = CreateEventUniquePtr.get();//используем умный указатель
	if (hEvent == INVALID_HANDLE_VALUE) { return -1; }

	std::unique_ptr<void, decltype(&CloseHandle)>CreateFileInUniquePtr(CreateFile(FileIn, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL), &CloseHandle);
	HANDLE hFileIn = CreateFileInUniquePtr.get();//используем умный указатель
	if (hFileIn == INVALID_HANDLE_VALUE) {
		return -1; 
	}

	LARGE_INTEGER li; //Представляет 64-разрядное целое число со знаком обединяя два 32-х разрядных
	li.QuadPart = 0;

	if (GetFileSizeEx(hFileIn, &li) == 0) {
		return -1;
	}; //узнаем размер файла
	size_t fileSize = li.QuadPart;
	const DWORD  bufSize = 16777216;// 16777216; // 4;//33554432; //16777216;//8388608;//читаем в буфер байты
	//std::pair <char*, size_t> p;
	//int fff = sizeof(size_t);
	//fff = sizeof(p);

	std::unique_ptr<char[]> bufUniquePtr(new char[fileSize + bufSize + 4096 + 1]); //4096 - выравнивание, 1 - '\0', bufSize - для последнего буфера(остаток может быть меньше, но буфер мы должны выдилить полный)
	char* buf = bufUniquePtr.get() + 4096 - (size_t(bufUniquePtr.get()) % 4096);
	
	DWORD error;
	DWORD bytesRead=0;
	size_t bytesReadTotal = 0;
	bool errHandleEOF = false; //метка конца файла
	char* find = buf;// указатель для поиска
	char* findPre = buf;// указатель для поиска
	char* bufNext = buf; //новый буфер
	std::pair <char*, size_t> p;
	std::vector<std::pair <char*, size_t>> charPtrArr;
	charPtrArr.reserve(10000000);

	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
	ui.QuadPart = 0;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEvent;   // событие для оповещения завершения чтения

	// читаем данные из файла
	for (;;)
	{
		if (!ReadFile(hFileIn, bufNext, bufSize, NULL, &ovl))
		{
			switch (error = GetLastError())// решаем что делать с кодом ошибки
			{
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
			default: {	
				return -1; 
			}// другие ошибки
			}
		}

		//	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		if (bytesReadTotal > 0)
		{	//ставим нуль терминаров  в конце буфера,ищем строки по разделитеkю. далее если в конце буфера символ поиска оставляем нуль терминатор, созраняем новую строку или возвращаем последний символ на место.
			char chTmp = bufNext[-1];
			bufNext[-1] = '\0'; //конец буфера

			find = strchr(find, '\n');
			while (find != NULL)
			{
				find[0] = '\0';
				charPtrArr.push_back({ findPre, ++find - findPre }); //длина с нуль-терминатором
				findPre = find;
				find = strchr(find, '\n');
			}

			if (chTmp == '\n') 
			{ 
				charPtrArr.push_back({ findPre, bufNext- findPre });
				findPre = bufNext;

			} //если последний символ
			else { bufNext[-1] = chTmp; }

			find = buf + bytesReadTotal;
		}

		if (errHandleEOF) { goto return0; }
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEvent, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFileIn, &ovl, &bytesRead, FALSE))
		{
			switch (error = GetLastError())// решаем что делать с кодом ошибки
			{
			case ERROR_HANDLE_EOF: { goto return0; break; }	// мы достигли конца файла в ходе асинхронной операции
			default: {	
				return -1; 
			}// другие ошибки
			}// конец процедуры switch (error = GetLastError())
		}

		// увеличиваем смещение в файле
		bytesReadTotal += bytesRead;//кол-во считанных байт нарастающим итогом
		bufNext += bytesRead; //добавляем смещение к указателю на буфер
		ui.QuadPart += bufSize; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
	}
//return1:
//	return RowsCountOut;

return0:
	//CreateFileInUniquePtr.get_deleter();
	bufNext[0] = '\0'; //конец буфера
	charPtrArr.push_back({ findPre, ++bufNext - findPre });
	clock_t t2 = clock();
	printf("Load: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//синхронная версия////////
	//find = buf;
	//find = strchr(find, '\n');
	//while (find != NULL)
	//{
	//	find[0] = '\0';
	//	charPtrArr.push_back(++find);
	//	find = strchr(find, '\n');
	//}
	///////////////////////////

	//проставляем длину строк
	//size_t i;
	//size_t strLenEnd = strlen(charPtrArr[charPtrArr.size() - 1].first);
	//for (i = 0; i < charPtrArr.size()-1; i++)
	//{
	//	charPtrArr[i].second = charPtrArr[i+1].first - charPtrArr[i].first;// +1; // + нуль терминатор
	//}
	//charPtrArr[i].second = strLenEnd + 1; // +1 нуль терминатор

	//работа с окончанием последней строки
	//size_t strLenEnd = strlen(charPtrArr[charPtrArr.size() - 1]);

	size_t RowsCountOut = charPtrArr.size();
	size_t strLenEnd = charPtrArr[RowsCountOut - 1].second;
	if (strLenEnd < 2) { charPtrArr.pop_back(); } // строка с учетом нуль терминатора, удаляем пустую строку, если строка не пустая проставляем окончание '\r'
	else {
		charPtrArr[RowsCountOut - 1].first[strLenEnd-1] = '\r';
		charPtrArr[RowsCountOut - 1].first[strLenEnd ] = '\0';
		charPtrArr[RowsCountOut - 1].second = strLenEnd+1;
	}

	clock_t t22 = clock();
	printf("Load array: Time - %f\n", (t22 - t2 + .0) / CLOCKS_PER_SEC); // время отработки

	if (SetLocale != 0)
	{
		if (SortOrder == 0) { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrAscendingLoc); } //по возрастанию  используем local - strcoll()
		else { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrDescendingLoc); } //по убыванию
	}
	else
	{
		if (SortOrder == 0) { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrAscending); } //по возрастанию   используем бинароное сравнение - strcmp()
		else { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrDescending); } //по убыванию
	}
	//https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/a9eac961-e77d-41a6-90a5-ce1a8b0cdb9c
	clock_t t3 = clock();
	printf("Sort: Time - %f\n", (t3 - t22 + .0) / CLOCKS_PER_SEC); // время отработки

	clock_t t33 = clock();
	if (OnlySort == 0) //если удаляем
	{
		std::vector<std::pair <char*, size_t>>::iterator it;
		it = std::unique(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrEqual);
		charPtrArr.resize(std::distance(charPtrArr.begin(), it));
		t33 = clock();
		printf("dell: Time - %f\n", (t33 - t3 + .0) / CLOCKS_PER_SEC); // время отработки
	}
	RowsCountOut = charPtrArr.size();


	////вар1
	//std::ofstream file2(FileOut, std::ios::out | std::ios::binary);
	//if (!file2.is_open()) {	return RowsCountOut;	}
	//std::ostream_iterator<char*> output_iterator(file2, "\n");// std::ostream_iterator<std::string> output_iterator(output_file, "\n");
	//std::copy(charPtrArr.begin(), charPtrArr.end(), output_iterator);
	//file2.close();

	////вар2
	std::unique_ptr<void, decltype(&CloseHandle)>CreateFileOutUniquePtr(CreateFile(FileOut, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL), &CloseHandle);
	HANDLE hFileOut = CreateFileOutUniquePtr.get();//используем умный указатель
	if (hFileOut == INVALID_HANDLE_VALUE) { 
		return -1;
	}

	std::unique_ptr<char[]> bufPtrWrite1(new char[bufSize + 4096 + 1]); //4096 - выравнивание, 1 - '\0', bufSize - для последнего буфера(остаток может быть меньше, но буфер мы должны выдилить полный)
	char* bufWrite1 = bufPtrWrite1.get() + 4096 - (size_t(bufPtrWrite1.get()) % 4096);
	bufWrite1[bufSize] = '\0';//добавим нуль-терминатор

	std::unique_ptr<char[]> bufPtrWrite2(new char[bufSize + 4096 + 1]); //4096 - выравнивание, 1 - '\0', bufSize - для последнего буфера(остаток может быть меньше, но буфер мы должны выдилить полный)
	char* bufWrite2 = bufPtrWrite2.get() + 4096 - (size_t(bufPtrWrite2.get()) % 4096);
	bufWrite2[bufSize] = '\0';//добавим нуль-терминатор

	//int strSize0 = 0;
	//clock_t t333 = clock();
	//for (int i=0;i< charPtrArr.size();i++)
	//{
	//	strSize0 += strlen(charPtrArr[i]) + 1; // + нуль терминатор
	//}
	//clock_t t444 = clock();
	//printf("strlen: Time - %f\n", (t444 - t333 + .0) / CLOCKS_PER_SEC); // время отработки
	//std::cout << strSize0 << std::endl;

	size_t strNum = 0; //номер строки
	size_t strSize = charPtrArr[0].second; //размер строки
	size_t bytesWriteTotal = strSize;// 0; //записано в буфер нарастающим для расчета
	DWORD bytesWriteOut1 = 0; //записано в первый буфер итого
	DWORD bytesWriteOut2 = 0; //записано во второй буфер итого
	//char* bufWrite2Tmp = bufWrite2 - strSize;
	char* strTmp = charPtrArr[0].first; //первая строка
	//bool dataEnd = false;
	

	ui.QuadPart = 0;
	//while (bytesWriteOut1 > 0)
	for (;;)
	{
		char* bufWrite2Tmp = bufWrite2 - strSize;
		//работаем асинхронно - заполяем 2й буфер, пока ждем запись на диск/в кеш
		for (;;)
		{
			if (bytesWriteTotal <= bufSize)
			{
				memcpy(bufWrite2 + bytesWriteTotal - strSize, strTmp, strSize);
				//bytesWriteTotal += strSize;
				bufWrite2[bytesWriteTotal-1] = '\n';

				if (++strNum >= RowsCountOut)
				{
					bytesWriteOut2 = bytesWriteTotal;// bufSize;// bytesWriteTotal;
					bytesWriteTotal = UINTMAX_MAX;
					break;
				}
				else 
				{ 
					strTmp = charPtrArr[strNum].first; 
					strSize = charPtrArr[strNum].second;
					bytesWriteTotal += strSize; //конец строки в накопительно в буфере
				}
			}
			else if(bytesWriteTotal == UINTMAX_MAX) { bytesWriteOut2 = 0;	break; }
			else
			{
				strSize = bufSize - (bytesWriteTotal - strSize);
				memcpy(bufWrite2 + bufSize - strSize, strTmp, strSize);
				bytesWriteOut2 = bufSize;// bytesWriteTotal + strSize; //весь буфер заполнен

				strTmp = strTmp + strSize; //указатель на оставшуюся строку
				strSize = bytesWriteTotal - bufSize;//размер оставшийся строки
				bytesWriteTotal = strSize;// размер накопленной строки в новом буфере
				break;
			}
		}	//пишем асинхронно

		//ждем, пока завершится асинхронная операция чтения

		if (bytesWriteOut1 > 0)
		{
			WaitForSingleObject(hEvent, INFINITE); //WaitForSingleObject

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
			if (!GetOverlappedResult(hFileOut, &ovl, &bytesRead, FALSE))
			{
				switch (error = GetLastError())// решаем что делать с кодом ошибки
				{
				case ERROR_HANDLE_EOF: { goto return0; break; }	// мы достигли конца файла в ходе асинхронной операции
				default: {
					return -1;
				}// другие ошибки
				}// конец процедуры switch (error = GetLastError())
			}
		}
		ui.QuadPart += bytesWriteOut1; //добавляем смещение к указателю на файл
		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово

		bytesWriteOut1 = bytesWriteOut2;
		if (bytesWriteOut1==0){ break; }//если буфер пустой

		//меняем буфер
		//char* bufWriteTmp = bufWrite1;
		//bufWrite1 = bufWrite2;
		//bufWrite2 = bufWriteTmp;
		std::swap(bufWrite1, bufWrite2);
		/////

		///пишем в файл асинхронно из 1го буфера
		if (!WriteFile(hFileOut, bufWrite1, bytesWriteOut1, NULL, &ovl))
		{
			switch (error = GetLastError())
			{
			case ERROR_IO_PENDING: { break; }// асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			default: {
				return -1;
			}// другие ошибки
			}
		}
	}
	
	clock_t t4 = clock();
	printf("save: Time - %f\n", (t4 - t33 + .0) / CLOCKS_PER_SEC); // время отработки
	return RowsCountOut;
}


int main111()
{
	DWORD t1 = GetTickCount();

	HANDLE hFile = CreateFile(TEXT("C:\\CSV_1_GB.csv"),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		0);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Error: CreateFile = INVALID_HANDLE_VALUE\n");
		return -1;
	}

	//LPVOID buf = 0;
	//DWORD size = 0;
	DWORD dw;

	DWORD size = GetFileSize(hFile, NULL);

	//buf = HeapAlloc(GetProcessHeap(), 0, size);
	char *buf = new char[size];

	if (!buf)
	{
		printf("Error: HeapAlloc = %d\n", GetLastError());
		return -1;
	}

	if (!ReadFile(hFile, buf, size, &dw, NULL))
	{
		printf("Error: ReadFile = %d\n", GetLastError());
		return -1;
	}

	DWORD t2 = GetTickCount();

	//printf("vector size = %d, loading time = %d ms.\n", size, t2 - t1);

	//(GetProcessHeap(), 0, buf);
	delete[] buf;
	CloseHandle(hFile);
	//system("pause");
	return 0;
}

int test()
{
//	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
//
//	HANDLE hFile = CreateFile(L"C:\\test2.txt", GENERIC_READ, FILE_SHARE_READ, 	NULL, 	OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS,	NULL 	);
//
//	if (hFile == INVALID_HANDLE_VALUE)
//	{
//		CloseHandle(hEvent);
//		return -1;
//	}

	std::unique_ptr<void, decltype(&CloseHandle)>createFileUptr(CreateFile(L"C:\\test3.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL), &CloseHandle);
	HANDLE hFile = createFileUptr.get();
	if (hFile == INVALID_HANDLE_VALUE){	return -1;	}


	////if (hEvent == NULL) { return -1; }
	////CloseHandle(hEvent);


	const DWORD  nNumberOfBytesToRead = 16777216;//33554432; //16777216;//8388608;//читаем в буфер байты
	//char* notAlignBuf = new char[ 4096 + 1 + nNumberOfBytesToRead]; //4096 - выравнивание, 1 - '\0', nNumberOfBytesToRead - для последнего буфера (остаток может быть меньше, но буфер мы должны выдилить полный)
	//char* buf = notAlignBuf; //буфер
	//if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	//std::unique_ptr<char[]> bufPtr = std::make_unique<char[]>(nNumberOfBytesToRead + 4096);
	//char* buf = bufPtr.get() + 4096 - (size_t(bufPtr.get()) % 4096);
	//char* buf2 = bufPtr.get() + nNumberOfBytesToRead % 4096; //выравнивание


	//std::unique_ptr<char[]> bufPtr(new char[nNumberOfBytesToRead + 4096]);//рабочий
	//char* buf = bufPtr.get() + 4096 - (size_t(bufPtr.get()) % 4096);
	//char* buf22 = buf + 5;

	std::string s;
	s.reserve( nNumberOfBytesToRead + 4096 );//resize//reserve
	std::cout << size_t(s.c_str()) << std::endl;
	char* buf = &s[4096 - size_t(s.c_str()) % 4096]; //выравнивание
	std::cout << size_t(buf) << std::endl;
	char* buf2 = buf + 1;
	std::cout << size_t(buf2) << std::endl;
	buf2[0]= 'e';
	buf2[1] = 'n';
	buf2[2] = 'd';
	buf2[3] = '\0';
	std::cout << buf2 << std::endl;

//buf[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	return 0;
}

int main()
{
	//0.53
	//_ULARGE_INTEGER 0.53
	//memcpy 0.76 (+0.23)
	//nNumberOfBytesToRead*2 0.76
	//strStart 0.77
	//strrchr 1.08
	//strStartLen+memcpy 0.78
	//find = strstr(bufWorkNew, findStr) 1,30
	//const char* findCh = "917939";//"4000000";// "917939";
	//2.69 - FILE_FLAG_NO_BUFFERING
	//3.33
	//-0.5 sec ассинхронность
	//0,52 просто ассинхронное чтение из кеша
	//2.420000 просто ассинхронное чтение FILE_FLAG_NO_BUFFERING
	//3.082000 просто ассинхронное чтение
	//0,76 выполнение самого кода поиска и работы с копией буфера
	//0,03 блок дозагрузки рабочего буфера межбуферной строкой
	//0,5 find
	//0.52-0,54 весь блок поиска + доп буфер


	//std::string sOut;
	//std::string sOut2;

	//if (createfile() != 0) { return 1; }; //генератор данных в файле
	clock_t t1;
	clock_t t2;
	////int x;
	////std::string st ("4000000");
	////char str[10];
	//for (int i = 1; i <= 1; i++)
	//{
	//test();
	////}
	char str[] = "20000000";
	char loc[] = "ru-RU";

	////t1 = clock();
	////main0();
	////t2 = clock();
	////printf("main0: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки


	t1 = clock();
	auto tt = SortDeleteDuplicateRowsCSVansi(L"C:\\CSV_1_GB.csv", L"C:\\test2.txt", 0, 0, 0 , 0, loc);//"C:\\file.txt" test.txt 111 C:\\CSV_1_GB.csv
	t2 = clock();
	printf("GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	std::cout << tt << std::endl;


	//for (int i = 1; i <= 4000000; i++)
	//{
	//	sOut = FindRowsInCSVansiNew(L"C:\\CSV_1_GB.csv", std::to_string(i).c_str(), 0, 0);
	//	sOut2 = XLAT(std::to_string(i));
	//	if (sOut != sOut2)
	//	{
	//		std::cout <<"find: "<<i<<"\nbedvit\n"<< sOut << std::endl;
	//		std::cout <<"XLAT\n"<<sOut2 << std::endl;
	//	}
	//}

	//std::cout << "\n\FILE_FLAG_NO_BUFFERING\n" << std::endl;
	//for (int i = 1; i <= 5; i++)
	//{
	//	//t1 = clock();
	//	//sOut = FindRowsInCSVansiNew(L"C:\\CSV_1_GB.csv", "4000000", 0, 1);
	//	//t2 = clock();
	//	//printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	t1 = clock();
	//	sOut2 = XLAT("4000000");
	//	t2 = clock();
	//	printf("XLAT: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	t1 = clock();
	//	sOut = FindRowsInCSVansi(L"C:\\CSV_1_GB.csv", str, 0, 0);
	//	t2 = clock();
	//	printf("bedvit: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки


	//}

	//кешировано
	//std::cout << "\n\FILE_FLAG_BUFFERING\n" << std::endl;
	//for (int i = 1; i <= 5; i++)
	//{
		//t1 = clock();
		//auto sOut = GetRowsCountCSVansi(L"C:\\test2.txt", 0); //C:\\CSV_5_GB.csv  test2.txt
		//t2 = clock();
		//printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
		//std::cout << sOut << std::endl;

	//	t1 = clock();
	//	sOut = GetRowCSVansi(L"C:\\CSV_1_GB.csv", 4000000, 0);
	//	t2 = clock();
	//	printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	t1 = clock();
	//	sOut = FindRowsInCSVansi(L"C:\\CSV_1_GB.csv", str, 0, 0);
	//	t2 = clock();
	//	printf("bedvit1: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	//std::cout << sOut << std::endl;
	//	t1 = clock();
	//	sOut2 = XLAT("4000000");
	//	t2 = clock();
	//	printf("XLAT: Time - %f\n\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//	//t1 = clock();
	//	//auto O = GetRowsCountCSVansi(L"C:\\CSV_1_GB.csv",0);
	//	//t2 = clock();
	//	//printf("GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	//std::cout << O << std::endl;
	//}
	/*int N = 1000000;
	auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < N; i++) {
		char* p = (char*)malloc(1024);
		free(p);
	}
	auto end = std::chrono::steady_clock::now();

	std::cout << "malloc + free: " << (end - start).count() << std::endl;

	start = std::chrono::steady_clock::now();
	for (int i = 0; i < N; i++) {
		char* p = new char[1024];
		delete[] p;
	}
	end = std::chrono::steady_clock::now();

	std::cout << "new[] + delete[]: " << (end - start).count() << std::endl;*/

	system("pause");
	return 0;
}


int main0000()
{
	const int N = 4000000; // количество строк
	std::string str = "Пятый; Тридацть четвертый; 44221100; BBB; CIFRAPOLE; POLEPVTRETYE; ODIN ODIN - TRI; CC; 01.01.2013; 01.01.2013; 8963; 2UTY39ADVGKR; СU707039; 40200М У0026034; -; 11; 2; 150; 1998; 1; -; 21; 1980; 1490; НОМЕР ПЯТЬ; -; ПОЛЕ ОДИН; ПОЛЕ ОДИН ПЯТЬ; -; -";
	std::vector <std::string> OutStr; //строки
	OutStr.reserve(N);
	for (int i = 0; i < N; ++i) 
	{
		std::string strTmp = (std::to_string(i + 1) + str +"\r\n");
		OutStr.push_back(strTmp);
	}
	std::vector <const char*> OutStrPtr(OutStr.size()); //укахатели
	for (int i = 0; i < OutStr.size(); ++i)
	{
		OutStrPtr[i] = OutStr[i].c_str();
	}

	clock_t t1;
	clock_t t2;
	std::wstring FileOut1 = L"C:\\test1.txt";
	std::wstring FileOut2 = L"C:\\test2.txt";

	t1 = clock();	//вар1
	std::ofstream fOut(FileOut1, std::ios::out | std::ios::binary);
	if (!fOut.is_open()) { return -1; }
	std::ostream_iterator<const char*> output_iterator(fOut, "");// std::ostream_iterator<std::string> output_iterator(output_file, "\n");
	std::copy(OutStrPtr.begin(), OutStrPtr.end(), output_iterator);
	fOut.close();
	t2 = clock();
	printf("std::ofstream: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); 



	std::string Out("");
	Out.reserve(1027000000);
	for (int i = 0; i < N; ++i)
	{
		Out.append(std::to_string(i + 1) + str + "\r\n");
	}

	t1 = clock();	//вар2
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);// дескриптор события
	if (hEvent == NULL) { return -1; }

	HANDLE hFile = CreateFile(FileOut2.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL);//FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS FILE_FLAG_NO_BUFFERING
	if (hFile == INVALID_HANDLE_VALUE){ CloseHandle(hEvent); return -1;}

	DWORD bytesWriteTotal;
	DWORD  error;

	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEvent;   // событие для оповещения завершения чтения

	int size = 16777216;// 16384;//		16777216; // 33554432;//	8388608	;// 4096;// 16777216

	//int Offset = 0;
	bytesWriteTotal = size;
	//for (int i = 0; i < OutStrPtr.size(); ++i)
	while (ovl.Offset < Out.size())
	{
		//auto gg = WriteFile(hFile, Out.c_str() + ovl.Offset, bytesWriteTotal, NULL, &ovl);
		if (!WriteFile(hFile, Out.c_str() + ovl.Offset, bytesWriteTotal, NULL, &ovl))
		{
			switch (error = GetLastError())
			{
			case ERROR_IO_PENDING: { break; }// асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			default: {
				CloseHandle(hEvent);
				CloseHandle(hFile);
				return -1;
			}// другие ошибки
			}
		}
		
		ovl.Offset += size;
		if (Out.size() - ovl.Offset < bytesWriteTotal) { bytesWriteTotal = Out.size() - ovl.Offset; }
		//ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEvent, INFINITE);
	}
		
	
	////ждем, пока завершится асинхронная операция чтения
	//WaitForSingleObject(hEvent, INFINITE);

	CloseHandle(hEvent);
	CloseHandle(hFile);

	t2 = clock();
	printf("WriteFile: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC);

	system("pause");
	return 0;
}

#pragma warning(disable : 4996)
int main333() {
	char str1[33000];
	char str2[33000];
	//char* estr;
	FILE* file1 = fopen("C:\\test2.txt", "r");
	FILE* file2 = fopen("C:\\test21.txt", "r");
	if (file1 != NULL && file2 != NULL)
	{
		while (1)
		{
			char* estr1 = fgets(str1, sizeof(str1), file1);
			char* estr2 = fgets(str2, sizeof(str2), file2);

			if (estr1 == NULL || estr2 == NULL) { break; }
			else if (strcmp(estr1, estr2) != 0)
			{
				int l1 = strlen(estr1);
				int l2 = strlen(estr2); 
				std::cout << "\ntest2\n" << estr1 <<"\nlen2\n" << l1 << std::endl;
				std::cout << "\ntest21\n" << estr2 << "\nlen21\n" << l2 << std::endl;
				//break; 
			}
			
		}
		fclose(file1);
		fclose(file2);
	}

	system("pause");
	return 0;
}












//
//
//
//
//
///////////////////////////////////////////////
//bool CompareCharPtrEqual(char* lhs, char* rhs) {
//	if (lhs == NULL && rhs == NULL) return true;
//	if (lhs == NULL || rhs == NULL) return false;
//	return strcmp(lhs, rhs) == 0;
//}
//bool CompareCharPtrAscending(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return strcmp(lhs, rhs) < 0;//strcoll
//}
//bool CompareCharPtrDescending(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return 	strcmp(lhs, rhs) > 0;
//}
//bool CompareCharPtrAscendingLoc(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return strcoll(lhs, rhs) < 0;//strcoll
//}
//bool CompareCharPtrDescendingLoc(char* lhs, char* rhs) {
//	if (lhs == NULL) return false;
//	if (rhs == NULL) return true;
//	return 	strcoll(lhs, rhs) > 0;
//}
//size_t SortDeleteDuplicateRowsCSVansi(LPCWSTR FileIn, LPCWSTR FileOut, int HeaderRowsCount, int OnlySort, int SortOrder, int SetLocale, char* Locale)
//{
//	clock_t t1 = clock();
//	if (SetLocale != 0) { if (setlocale(LC_COLLATE, Locale) == NULL) { return -1; } }//"ru-RU"
//
//	std::unique_ptr<void, decltype(&CloseHandle)>CreateEventUniquePtr(CreateEvent(NULL, FALSE, FALSE, NULL), &CloseHandle);// дескриптор события// создаем события с автоматическим сбросом
//	HANDLE hEvent = CreateEventUniquePtr.get();//используем умный указатель
//	if (hEvent == INVALID_HANDLE_VALUE) { return -1; }
//
//	std::unique_ptr<void, decltype(&CloseHandle)>CreateFileInUniquePtr(CreateFile(FileIn, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL), &CloseHandle);
//	HANDLE hFileIn = CreateFileInUniquePtr.get();//используем умный указатель
//	if (hFileIn == INVALID_HANDLE_VALUE) {
//		return -1;
//	}
//
//	LARGE_INTEGER li; //Представляет 64-разрядное целое число со знаком обединяя два 32-х разрядных
//	li.QuadPart = 0;
//
//	if (GetFileSizeEx(hFileIn, &li) == 0) {
//		return -1;
//	}; //узнаем размер файла
//	size_t fileSize = li.QuadPart;
//	const DWORD  bufSize = 16777216;// 16777216; // 4;//33554432; //16777216;//8388608;//читаем в буфер байты
//	//int fff = sizeof(bufSize);
//
//	std::unique_ptr<char[]> bufPtr(new char[fileSize + bufSize + 4096 + 1]); //4096 - выравнивание, 1 - '\0', bufSize - для последнего буфера(остаток может быть меньше, но буфер мы должны выдилить полный)
//	char* buf = bufPtr.get() + 4096 - (size_t(bufPtr.get()) % 4096);
//
//	DWORD error;
//	DWORD bytesRead = 0;
//	size_t bytesReadTotal = 0;
//	bool errHandleEOF = false; //метка конца файла
//	char* find = buf;// указатель для поиска
//	char* bufNext = buf;
//	std::vector<char*> charPtrArr;
//	charPtrArr.push_back(buf);
//
//	_ULARGE_INTEGER ui; //Представляет 64-разрядное целое число без знака обединяя два 32-х разрядных
//	ui.QuadPart = 0;
//
//	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу// инициализируем структуру OVERLAPPED
//	ovl.Offset = 0;         // младшая часть смещения равна 0
//	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
//	ovl.hEvent = hEvent;   // событие для оповещения завершения чтения
//
//	// читаем данные из файла
//	for (;;)
//	{
//		if (!ReadFile(hFileIn, bufNext, bufSize, NULL, &ovl))
//		{
//			switch (error = GetLastError())// решаем что делать с кодом ошибки
//			{
//			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
//			case ERROR_HANDLE_EOF: { errHandleEOF = true;	break; } // мы достигли конца файла читалкой ReadFile
//			default: {
//				return -1;
//			}// другие ошибки
//			}
//		}
//
//		//	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
//		if (bytesReadTotal > 0)
//		{	//ставим нуль терминаров  в конце буфера,ищем строки по разделитедю. далее если в конце буфера символ поиска оставляем нуль терминатор, созраняем новую строку или возвращаем последний символ на место.
//			char chTmp = bufNext[-1];
//			bufNext[-1] = '\0'; //конец буфера
//
//			find = strchr(find, '\n');
//			while (find != NULL)
//			{
//				find[0] = '\0';
//				charPtrArr.push_back(++find);
//				find = strchr(find, '\n');
//			}
//
//			if (chTmp == '\n') { charPtrArr.push_back(bufNext); } //если последний символ
//			else { bufNext[-1] = chTmp; }
//
//			find = buf + bytesReadTotal;
//		}
//
//
//		if (errHandleEOF) { goto return0; }
//		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
//
//		// ждем, пока завершится асинхронная операция чтения
//		WaitForSingleObject(hEvent, INFINITE);
//
//		// проверим результат работы асинхронного чтения // если возникла проблема ... 
//		if (!GetOverlappedResult(hFileIn, &ovl, &bytesRead, FALSE))
//		{
//			switch (error = GetLastError())// решаем что делать с кодом ошибки
//			{
//			case ERROR_HANDLE_EOF: { goto return0; break; }	// мы достигли конца файла в ходе асинхронной операции
//			default: {
//				return -1;
//			}// другие ошибки
//			}// конец процедуры switch (error = GetLastError())
//		}
//
//		// увеличиваем смещение в файле
//		bytesReadTotal += bytesRead;//кол-во считанных байт нарастающим итогом
//		bufNext += bytesRead; //добавляем смещение к указателю на буфер
//		ui.QuadPart += bufSize; //добавляем смещение к указателю на файл
//		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
//		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
//	}
//	//return1:
//	//	return RowsCountOut;
//
//return0:
//	//CreateFileInUniquePtr.get_deleter();
//	clock_t t2 = clock();
//	printf("Load: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
//	bufNext[0] = '\0'; //конец буфера
//
//	//синхронная версия////////
//	//find = buf;
//	//find = strchr(find, '\n');
//	//while (find != NULL)
//	//{
//	//	find[0] = '\0';
//	//	charPtrArr.push_back(++find);
//	//	find = strchr(find, '\n');
//	//}
//	///////////////////////////
//
//	//работа с окончанием последней строки
//	size_t strLenEnd = strlen(charPtrArr[charPtrArr.size() - 1]);
//	if (strLenEnd == 0) { charPtrArr.pop_back(); } //удаляем пустую строку, если строка не пустая проставляем окончание '\r'
//	else {
//		charPtrArr[charPtrArr.size() - 1][strLenEnd] = '\r';
//		charPtrArr[charPtrArr.size() - 1][strLenEnd + 1] = '\0';
//	}
//
//	clock_t t22 = clock();
//	printf("Load array: Time - %f\n", (t22 - t2 + .0) / CLOCKS_PER_SEC); // время отработки
//
//	if (SetLocale != 0)
//	{
//		if (SortOrder == 0) { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrAscendingLoc); } //по возрастанию  используем local - strcoll()
//		else { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrDescendingLoc); } //по убыванию
//	}
//	else
//	{
//		if (SortOrder == 0) { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrAscending); } //по возрастанию   используем бинароное сравнение - strcmp()
//		else { concurrency::parallel_buffered_sort(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrDescending); } //по убыванию
//	}
//	//https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/a9eac961-e77d-41a6-90a5-ce1a8b0cdb9c
//	clock_t t3 = clock();
//	printf("Sort: Time - %f\n", (t3 - t22 + .0) / CLOCKS_PER_SEC); // время отработки
//
//	clock_t t33 = clock();
//	if (OnlySort == 0) //если удаляем
//	{
//		std::vector<char*>::iterator it;
//		it = std::unique(charPtrArr.begin() + HeaderRowsCount, charPtrArr.end(), CompareCharPtrEqual);
//		charPtrArr.resize(std::distance(charPtrArr.begin(), it));
//		t33 = clock();
//		printf("dell: Time - %f\n", (t33 - t3 + .0) / CLOCKS_PER_SEC); // время отработки
//	}
//	size_t RowsCountOut = charPtrArr.size();
//
//
//	////вар1
//	//std::ofstream file2(FileOut, std::ios::out | std::ios::binary);
//	//if (!file2.is_open()) {	return RowsCountOut;	}
//	//std::ostream_iterator<char*> output_iterator(file2, "\n");// std::ostream_iterator<std::string> output_iterator(output_file, "\n");
//	//std::copy(charPtrArr.begin(), charPtrArr.end(), output_iterator);
//	//file2.close();
//
//	////вар2
//	std::unique_ptr<void, decltype(&CloseHandle)>CreateFileOutUniquePtr(CreateFile(FileOut, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, NULL), &CloseHandle);
//	HANDLE hFileOut = CreateFileOutUniquePtr.get();//используем умный указатель
//	if (hFileOut == INVALID_HANDLE_VALUE) {
//		return -1;
//	}
//
//	std::unique_ptr<char[]> bufPtrWrite1(new char[bufSize + 4096 + 1]); //4096 - выравнивание, 1 - '\0', bufSize - для последнего буфера(остаток может быть меньше, но буфер мы должны выдилить полный)
//	char* bufWrite1 = bufPtrWrite1.get() + 4096 - (size_t(bufPtr.get()) % 4096);
//	bufWrite1[bufSize] = '\0';//добавим нуль-терминатор
//
//	std::unique_ptr<char[]> bufPtrWrite2(new char[bufSize + 4096 + 1]); //4096 - выравнивание, 1 - '\0', bufSize - для последнего буфера(остаток может быть меньше, но буфер мы должны выдилить полный)
//	char* bufWrite2 = bufPtrWrite2.get() + 4096 - (size_t(bufPtr.get()) % 4096);
//	bufWrite2[bufSize] = '\0';//добавим нуль-терминатор
//
//	DWORD bytesWriteOut1 = 0; //записано в первый буфер итого
//	DWORD bytesWriteOut2 = 0; //записано во второй буфер итого
//	DWORD bytesWriteTotal = 0; //записано в буфер нарастающим для расчета
//	size_t strNum = 0; //номер строки
//	size_t strSize = 0; //размер строки
//	char* strTmp = charPtrArr[0]; //первая строка
//	bool dataEnd = false;
//
//	ui.QuadPart = 0;
//	//while (bytesWriteOut1 > 0)
//	for (;;)
//	{
//		//работаем асинхронно - заполяем 2й буфер, пока ждем запись на диск/в кеш
//		for (;;)
//		{
//			if (dataEnd)
//			{
//				bytesWriteOut2 = 0;
//				break;
//			}
//			strSize = strlen(strTmp) + 1; // + нуль терминатор
//
//			if (bytesWriteTotal + strSize <= bufSize)
//			{
//				memcpy(bufWrite2 + bytesWriteTotal, strTmp, strSize);
//				bytesWriteTotal += strSize;
//				bufWrite2[bytesWriteTotal - 1] = '\n';
//				//bufWrite2[bytesWriteTotal] = EOF;// '\0';
//				if (++strNum >= charPtrArr.size())
//				{
//					bytesWriteOut2 = bytesWriteTotal;// bufSize;// bytesWriteTotal;
//					dataEnd = true;
//					break;
//				}
//				else { strTmp = charPtrArr[strNum]; }
//			}
//			else
//			{
//				strSize = bufSize - bytesWriteTotal;
//				memcpy(bufWrite2 + bytesWriteTotal, strTmp, strSize);
//				bytesWriteOut2 = bytesWriteTotal + strSize;
//				bytesWriteTotal = 0;
//				strTmp = strTmp + strSize;
//				break;
//			}
//		}	//пишем асинхронно
//
//		//ждем, пока завершится асинхронная операция чтения
//		if (bytesWriteOut1 > 0)
//		{
//			WaitForSingleObject(hEvent, INFINITE); //WaitForSingleObject
//
//		// проверим результат работы асинхронного чтения // если возникла проблема ... 
//			if (!GetOverlappedResult(hFileOut, &ovl, &bytesRead, FALSE))
//			{
//				switch (error = GetLastError())// решаем что делать с кодом ошибки
//				{
//				case ERROR_HANDLE_EOF: { goto return0; break; }	// мы достигли конца файла в ходе асинхронной операции
//				default: {
//					return -1;
//				}// другие ошибки
//				}// конец процедуры switch (error = GetLastError())
//			}
//		}
//		ui.QuadPart += bytesWriteOut1; //добавляем смещение к указателю на файл
//		ovl.Offset = ui.LowPart;// вносим смещение в младшее слово
//		ovl.OffsetHigh = ui.HighPart;// вносим смещение в старшеее слово
//
//		bytesWriteOut1 = bytesWriteOut2;
//		if (bytesWriteOut1 == 0) { break; }//если буфер пустой
//
//		//меняем буфер
//		//char* bufWriteTmp = bufWrite1;
//		//bufWrite1 = bufWrite2;
//		//bufWrite2 = bufWriteTmp;
//		std::swap(bufWrite1, bufWrite2);
//		/////
//
//		///пишем в файл асинхронно из 1го буфера
//		if (!WriteFile(hFileOut, bufWrite1, bytesWriteOut1, NULL, &ovl))
//		{
//			switch (error = GetLastError())
//			{
//			case ERROR_IO_PENDING: { break; }// асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
//			default: {
//				return -1;
//			}// другие ошибки
//			}
//		}
//	}
//
//	clock_t t4 = clock();
//	printf("save: Time - %f\n", (t4 - t33 + .0) / CLOCKS_PER_SEC); // время отработки
//	return RowsCountOut;
//}