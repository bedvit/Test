// Test.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <fstream>
#include <string>

#include <iostream>
#include <fstream>
#include <cstdlib> // для использования exit()
#include <ctime>
#include <Windows.h>

//#include <iostream>
//#include <exception>
//#include <stdio.h>
int GetRowsCountCSVansi(PCTSTR path)
{
	HANDLE  hFile;     // дескриптор файла
	HANDLE  hEndRead;  // дескриптор события
	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу
	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты
	//char* buf = new char[nNumberOfBytesToRead]; //буфер CreateFile
	char* notalign = new char[nNumberOfBytesToRead + 512]; //память
	char* buf = notalign; //буфер
	if (size_t(buf) % 512) { buf += 512 - (size_t(buf) % 512); }

	char* bufWork = new char[nNumberOfBytesToRead + 1]; //буфер Рабочий
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	char* find;// указатель для поиска
	int strCount = 1; //счетчик строк

	// создаем события с автоматическим сбросом
	hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEndRead == NULL) { return GetLastError(); }

	// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	// открываем файл для чтения
	hFile = CreateFile(
		path,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,  // асинхронный ввод
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		delete[] notalign;
		delete[] bufWork;
		return -1;
	}
	// читаем данные из файла
	for (;;)
	{
		DWORD  dwBytesRead;
		DWORD  dwError;
		char* find = buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&dwBytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING:
				// асинхронный ввод-вывод все еще происходит 
				// сделаем кое-что пока он идет 
				break;
			case ERROR_HANDLE_EOF:
				// мы достигли конца файла 
				// закрываем дескрипторы
				CloseHandle(hFile);
				CloseHandle(hEndRead);
				delete[] notalign;
				delete[] bufWork;
				return strCount;
			default:
				// закрываем дескрипторы
				CloseHandle(hFile);
				CloseHandle(hEndRead);
				delete[] notalign;
				delete[] bufWork;
				return -1;
			}
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		find = bufWork; //буфер
		find = strstr(find, "\n");
		while (find != NULL)
		{
			find = find + 1;
			strCount++;
			find = strstr(find, "\n");
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEndRead, INFINITE);

		// проверим результат работы асинхронного чтения 
		// если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
		{
			// решаем что делать с кодом ошибки
			switch (dwError = GetLastError())
			{
			case ERROR_HANDLE_EOF:
			{
				// мы достигли конца файла в ходе асинхронной операции	
				// закрываем дескрипторы
				CloseHandle(hFile);
				CloseHandle(hEndRead);
				delete[] notalign;
				delete[] bufWork;
				return strCount;
			}
			// решаем что делать с другими случаями ошибок
			CloseHandle(hFile);
			CloseHandle(hEndRead);
			delete[] notalign;
			delete[] bufWork;
			return -1;
			}// конец процедуры switch (dwError = GetLastError())
		}

		//синхронно копируем данные из буфера в рабочий буфер, далее с ним работаем асинхронно
		memcpy(bufWork, buf, nNumberOfBytesToRead);

		// увеличиваем смещение в файле
		ovl.Offset += nNumberOfBytesToRead;// sizeof(n);
	}
}

int GetRowCSVansi(PCTSTR path, int strNum=1)
{
	HANDLE  hFile;     // дескриптор файла
	HANDLE  hEndRead;  // дескриптор события
	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу
	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты
	char* buf = new char[nNumberOfBytesToRead]; //буфер CreateFile
	char* bufWork = new char[nNumberOfBytesToRead + 1]; //буфер Рабочий
	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	char* find;// указатель для поиска
	char* findNext;// указатель для поиска следующий
	int strCount=1; //счетчик строк
	std::string strOut;
	DWORD  dwBytesReadWork;

	// создаем события с автоматическим сбросом
	hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEndRead == NULL) { return GetLastError(); }

	// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	// открываем файл для чтения
	hFile = CreateFile(
		path,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,  // асинхронный ввод
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		delete[] bufWork;
		delete[] buf;
		return -1;
	}
	// читаем данные из файла
	for (;;)
	{
		DWORD  dwBytesRead;
		DWORD  dwError;
		char* find= buf; //буфер
		// читаем одну запись
		if (!ReadFile(
			hFile,           // дескриптор файла
			buf,             // адрес буфера, куда читаем данные
			nNumberOfBytesToRead,// количество читаемых байтов
			&dwBytesRead,    // количество прочитанных байтов
			&ovl             // чтение асинхронное
		))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
			case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
			//case ERROR_HANDLE_EOF: {goto return0; } // мы достигли конца файла читалкой ReadFile
			default: {goto return1; }// другие ошибки
			}
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		if (ovl.Offset > 0)//если рабочий буфер заполнен
		{
			bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
			find = bufWork; //новый буфер
			do //считаем строки в буфере
			{
				if (strCount == strNum)
				{
					findNext = strstr(find, "\n");
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
				find = strstr(find, "\n");
				if (find != NULL)
				{
					find++;
					strCount++;
				}
			} while (find != NULL);

		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//

		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEndRead, INFINITE);

		// проверим результат работы асинхронного чтения // если возникла проблема ... 
		if (!GetOverlappedResult(hFile, &ovl,&dwBytesRead, FALSE))
		{
			switch (dwError = GetLastError())// решаем что делать с кодом ошибки
			{
				case ERROR_HANDLE_EOF: {goto return0; }// мы достигли конца файла в ходе асинхронной операции
				default: {goto return1; }// другие ошибки
			}// конец процедуры switch (dwError = GetLastError())
		}

		//синхронно копируем данные из буфера в рабочий буфер, далее с ним работаем асинхронно
		memcpy(bufWork, buf, nNumberOfBytesToRead);

		// увеличиваем смещение в файле
		dwBytesReadWork = dwBytesRead;
		ovl.Offset += nNumberOfBytesToRead;// sizeof(n);
	}

return0:
	// закрываем дескрипторы
	std::cout << "String Find " << strOut << std::endl;
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] buf;
	delete[] bufWork;
	return 0;
return1:
	CloseHandle(hFile);
	CloseHandle(hEndRead);
	delete[] buf;
	delete[] bufWork;
	return -1;
}

int FindInCSVansi(PCTSTR path, PCTSTR find)
{
HANDLE  hFile;     // дескриптор файла
HANDLE  hEndRead;  // дескриптор события
OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу
const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты
char* buf = new char[nNumberOfBytesToRead]; //буфер CreateFile
char* bufWork = new char[nNumberOfBytesToRead + 1]; //буфер Рабочий
bufWork[0] = '\0';//добавим нуль-терминатор
bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
char* find;// указатель для поиска
char* findNext;// указатель для поиска следующий
int strCount = 1; //счетчик строк
std::string strOut;
DWORD  dwBytesReadWork;

// создаем события с автоматическим сбросом
hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);
if (hEndRead == NULL) { return GetLastError(); }

// инициализируем структуру OVERLAPPED
ovl.Offset = 0;         // младшая часть смещения равна 0
ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

// открываем файл для чтения
hFile = CreateFile(
	path,   // имя файла
	GENERIC_READ,          // чтение из файла
	FILE_SHARE_READ,       // совместный доступ к файлу
	NULL,                  // защиты нет
	OPEN_EXISTING,         // открываем существующий файл
	FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,  // асинхронный ввод
	NULL                   // шаблона нет
);
// проверяем на успешное открытие
if (hFile == INVALID_HANDLE_VALUE)
{
	CloseHandle(hEndRead);
	delete[] bufWork;
	delete[] buf;
	return -1;
}
// читаем данные из файла
for (;;)
{
	DWORD  dwBytesRead;
	DWORD  dwError;
	char* find = buf; //буфер
	// читаем одну запись
	if (!ReadFile(
		hFile,           // дескриптор файла
		buf,             // адрес буфера, куда читаем данные
		nNumberOfBytesToRead,// количество читаемых байтов
		&dwBytesRead,    // количество прочитанных байтов
		&ovl             // чтение асинхронное
	))
	{
		switch (dwError = GetLastError())// решаем что делать с кодом ошибки
		{
			//эти ошибки смотрм после завершения асинхронной операции чтения, для возможности обработать рабочий буфер
		case ERROR_IO_PENDING: { break; }		 // асинхронный ввод-вывод все еще происходит // сделаем кое-что пока он идет 
		//case ERROR_HANDLE_EOF: {goto return0; } // мы достигли конца файла читалкой ReadFile
		default: {goto return1; }// другие ошибки
		}
	}
	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
	if (ovl.Offset > 0)//если рабочий буфер заполнен
	{
		bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
		find = bufWork; //новый буфер
		do //считаем строки в буфере
		{
			if (strCount == strNum)
			{
				findNext = strstr(find, "\n");
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
			find = strstr(find, "\n");
			if (find != NULL)
			{
				find++;
				strCount++;
			}
		} while (find != NULL);

	}
	//работаем асинхронно, выполняем код, пока ждем чтение с диска//

	// ждем, пока завершится асинхронная операция чтения
	WaitForSingleObject(hEndRead, INFINITE);

	// проверим результат работы асинхронного чтения // если возникла проблема ... 
	if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
	{
		switch (dwError = GetLastError())// решаем что делать с кодом ошибки
		{
		case ERROR_HANDLE_EOF: {goto return0; }// мы достигли конца файла в ходе асинхронной операции
		default: {goto return1; }// другие ошибки
		}// конец процедуры switch (dwError = GetLastError())
	}

	//синхронно копируем данные из буфера в рабочий буфер, далее с ним работаем асинхронно
	memcpy(bufWork, buf, nNumberOfBytesToRead);

	// увеличиваем смещение в файле
	dwBytesReadWork = dwBytesRead;
	ovl.Offset += nNumberOfBytesToRead;// sizeof(n);
}

return0:
// закрываем дескрипторы
std::cout << "String Find " << strOut << std::endl;
CloseHandle(hFile);
CloseHandle(hEndRead);
delete[] buf;
delete[] bufWork;
return 0;
return1:
CloseHandle(hFile);
CloseHandle(hEndRead);
delete[] buf;
delete[] bufWork;
return -1;
}
//#include <vector>
//BOOL ReadFromFileAsync2(PCTSTR path)
//{
//	BOOL bResult = FALSE;
//
//	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED /*| FILE_FLAG_NO_BUFFERING*/, NULL);
//	if (hFile == INVALID_HANDLE_VALUE)
//	{
//		_tprintf_s(TEXT("Error opening file: %s\n"), path);
//		return FALSE;
//	}
//	DWORD dwNumReads;
//	DWORD dwLineSize = 536870912; // size of each line, in bytes
//	std::vector<BYTE> bSecondLineBuf(dwLineSize);
//	std::vector<BYTE> bFourthLineBuf(dwLineSize);
//
//
//	OVERLAPPED oReadSecondLine = { 0 };
//	OVERLAPPED oReadFourthLine = { 0 };
//
//	oReadSecondLine.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//	if (!oReadSecondLine.hEvent)
//	{
//		_tprintf_s(TEXT("Error creating I/O event for reading second line\n"));
//		goto done;
//	}
//	oReadSecondLine.Offset = 0; // offset of second line
//
//	oReadFourthLine.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//	if (!oReadFourthLine.hEvent)
//	{
//		_tprintf_s(TEXT("Error creating I/O event for reading fourth line\n"));
//		goto done;
//	}
//	oReadFourthLine.Offset = 536870912; // offset of fourth line
//
//	if (!ReadFile(hFile, &bSecondLineBuf[0], dwLineSize, NULL, &oReadSecondLine))
//	{
//		if (GetLastError() != ERROR_IO_PENDING)
//		{
//			_tprintf_s(TEXT("Error starting I/O to read second line\n"));
//			goto done;
//		}
//	}
//
//	if (!ReadFile(hFile, &bFourthLineBuf[0], dwLineSize, NULL, &oReadFourthLine))
//	{
//		if (GetLastError() != ERROR_IO_PENDING)
//		{
//			_tprintf_s(TEXT("Error starting I/O to read fourth line\n"));
//			CancelIo(hFile);
//			goto done;
//		}
//	}
//
//	// perform some stuff asynchronously
//	_tprintf_s(TEXT("HEY\n"));
//
//	HANDLE hEvents[2];
//	hEvents[0] = oReadSecondLine.hEvent;
//	hEvents[1] = oReadFourthLine.hEvent;
//
//	OVERLAPPED* pOverlappeds[2];
//	pOverlappeds[0] = &oReadSecondLine;
//	pOverlappeds[1] = &oReadFourthLine;
//
//	BYTE* pBufs[2];
//	pBufs[0] = &bSecondLineBuf[0];
//	pBufs[1] = &bFourthLineBuf[0];
//
//	dwNumReads = _countof(hEvents);
//
//	do
//	{
//		DWORD dwWaitRes = WaitForMultipleObjects(dwNumReads, hEvents, FALSE, INFINITE);
//		if (dwWaitRes == WAIT_FAILED)
//		{
//			_tprintf_s(TEXT("Error waiting for I/O to finish\n"));
//			CancelIo(hFile);
//			goto done;
//		}
//
//		if ((dwWaitRes >= WAIT_OBJECT_0) && (dwWaitRes < (WAIT_OBJECT_0 + dwNumReads)))
//		{
//			DWORD dwIndex = dwWaitRes - WAIT_OBJECT_0;
//
//			_tprintf_s(TEXT("String that was read from file: "));
//
//			//for (int i = 0; i < pOverlappeds[dwIndex]->InternalHigh; ++i)
//			//	_tprintf_s(TEXT("%c"), (TCHAR)pBufs[dwIndex][i]);
//			//_tprintf_s(TEXT("\n"));
//
//			--dwNumReads;
//			if (dwNumReads == 0)
//				break;
//
//			if (dwIndex == 0)
//			{
//				hEvents[0] = hEvents[1];
//				pOverlappeds[0] = pOverlappeds[1];
//				pBufs[0] = pBufs[1];
//			}
//		}
//	} while (true);
//
//done:
//	if (oReadSecondLine.hEvent) CloseHandle(oReadSecondLine.hEvent);
//	if (oReadFourthLine.hEvent) CloseHandle(oReadFourthLine.hEvent);
//	CloseHandle(hFile);
//
//	return bResult;
//
//
////
////
////	BOOL bResult = FALSE;
////
////	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED /*| FILE_FLAG_NO_BUFFERING*/, NULL);
////	if (hFile == INVALID_HANDLE_VALUE)
////	{
////		_tprintf_s(TEXT("Error opening file: %s\n"), path);
////		return FALSE;
////	}
////	DWORD dwWaitRes;
////	DWORD dwLineSize = 250; // size of each line, in bytes
////	std::vector<BYTE> bSecondLineBuf(dwLineSize);
////	std::vector<BYTE> bFourthLineBuf(dwLineSize);
////
////	OVERLAPPED oReadSecondLine = { 0 };
////	OVERLAPPED oReadFourthLine = { 0 };
////
////	oReadSecondLine.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
////	if (!oReadSecondLine.hEvent)
////	{
////		_tprintf_s(TEXT("Error creating I/O event for reading second line\n"));
////		goto done;
////	}
////	oReadSecondLine.Offset = 250; // offset of second line
////
////	oReadFourthLine.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
////	if (!oReadFourthLine.hEvent)
////	{
////		_tprintf_s(TEXT("Error creating I/O event for reading fourth line\n"));
////		goto done;
////	}
////	oReadFourthLine.Offset = 500; // offset of fourth line
////
////	if (!ReadFile(hFile, &bSecondLineBuf[0], dwLineSize, NULL, &oReadSecondLine))
////	{
////		if (GetLastError() != ERROR_IO_PENDING)
////		{
////			_tprintf_s(TEXT("Error starting I/O to read second line\n"));
////			goto done;
////		}
////	}
////
////	if (!ReadFile(hFile, &bFourthLineBuf[0], dwLineSize, NULL, &oReadFourthLine))
////	{
////		if (GetLastError() != ERROR_IO_PENDING)
////		{
////			_tprintf_s(TEXT("Error starting I/O to read fourth line\n"));
////			CancelIo(hFile);
////			goto done;
////		}
////	}
////
////	// perform some stuff asynchronously
////	_tprintf_s(TEXT("HEY\n"));
////
////	HANDLE hEvents[2];
////	hEvents[0] = oReadSecondLine.hEvent;
////	hEvents[1] = oReadFourthLine.hEvent;
////
////	dwWaitRes = WaitForMultipleObjects(_countof(hEvents), hEvents, TRUE, INFINITE);
////	if (dwWaitRes == WAIT_FAILED)
////	{
////		_tprintf_s(TEXT("Error waiting for I/O to finish\n"));
////		CancelIo(hFile);
////		goto done;
////	}
////
////	_tprintf_s(TEXT("Strings that were read from file: "));
////
////	for (int i = 0; i < oReadSecondLine.InternalHigh; ++i)
////		_tprintf_s(TEXT("%c"), (TCHAR)&bSecondLineBuf[i]);
////	_tprintf_s(TEXT("\n"));
////
////	for (int i = 0; i < oReadFourthLine.InternalHigh; ++i)
////		_tprintf_s(TEXT("%c"), (TCHAR)&bFourthLineBuf[i]);
////	_tprintf_s(TEXT("\n"));
////
////done:
////	if (oReadSecondLine.hEvent) CloseHandle(oReadSecondLine.hEvent);
////	if (oReadFourthLine.hEvent) CloseHandle(oReadFourthLine.hEvent);
////	CloseHandle(hFile);
////
////	return bResult;
//}


int createfile() //создание файла для теста
{
	const int N = 4000000; // количество строк в файле
	std::ofstream fout("D:\\CSV_1_GB.csv");
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

int main() 
{
	//if (createfile() != 0) { return 1; }; //генератор данных в файле
	clock_t t1;
	clock_t t2;

//	///////////////////////////////////////////////
	//t1 = clock();
//	std::ifstream is;
//	std::filebuf * fb = is.rdbuf();
//	fb->open("C:\\CSV_1_GB.csv", std::ios::in | std::ios::binary);
//		while (is) 
//		{
//			std::string str;
//			std::getline(is, str);
//			size_t found = str.find("ASD123FGH456");
//			if (found != std::string::npos)	{ std::cout << "Find OK: " << std::endl; };
//		}
//	fb->close();
//	t2 = clock();
//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): std::getline + Find: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
//
//	///////////////////////////////////////////////
//	t1 = clock();
//	std::ifstream is2;
//	std::filebuf * fb2 = is2.rdbuf();
//	fb2->open("C:\\CSV_1_GB.csv", std::ios::in | std::ios::binary);
//	while (is2) 
//	{
//		std::string str;
//		std::getline(is2, str);
//	}
//	fb2->close();
//	t2 = clock();
//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): std::getline: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
//
//	///////////////////////////////////////////////
//	t1 = clock();
//	std::ifstream is3;
//	std::filebuf * fb3 = is3.rdbuf();
//	fb3->open("D:\\CSV_1_GB.csv", std::ios::in | std::ios::binary);
//	while (is3) 
//	{
//		std::string str;
//		std::getline(is3, str);
//	}
//	fb3->close();
//	t2 = clock();
//	printf("HDD WDC WD10EACS-00ZJB0 (1000 GB, SATA-II): std::getline: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
//
//	///////////////////////////////////////////////
//#pragma warning(disable : 4996)
//	t1 = clock();
//	char str[33000];
//	char* estr;
//	FILE* file;
//	file = fopen("C:\\CSV_1_GB.csv", "rb");
//	if (file != NULL)
//	{
//		while (1)
//		{
//			estr = fgets(str, sizeof(str), file);
//			if (estr == NULL) { break; }
//		}
//		fclose(file);
//	}
//	t2 = clock();
//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): fgets: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
//
//	///////////////////////////////////////////////
//#pragma warning(disable : 4996)
//	t1 = clock();
//	char str2[33000];
//	char* estr2;
//	FILE* file2;
//	file2 = fopen("D:\\CSV_1_GB.csv", "rb");
//	if (file2 != NULL)
//	{
//		while (1)
//		{
//			estr2 = fgets(str2, sizeof(str2), file2);
//			if (estr2 == NULL) { break; }
//		}
//		fclose(file2);
//	}
//	t2 = clock();
//	printf("HDD WDC WD10EACS-00ZJB0 (1000 GB, SATA-II): fgets: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
//
	//t1 = clock();
	////LPCTSTR p = (L"D:\\CSV_1_GB.csv");
	//if (ReadFromFileAsync(L"D:\\CSV_1_GB.csv")==0)
	//{
	//	t2 = clock();
	//	printf("HDD WDC WD10EACS-00ZJB0 (1000 GB, SATA-II): ReadFromFileAsync: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//}
	//int x;
	//t1 = clock();
	//if ((x= GetRowsCountCSVansi(L"D:\\CSV_0_GB.csv"))>-1)
	//{
	//	t2 = clock();
	//	printf("HDD WDC WD10EACS-00ZJB0 (1000 GB, SATA-II): GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	std::cout << "String Find " << x << std::endl;
	//}

	//t1 = clock();
	//if ((x = GetRowsCountCSVansi(L"C:\\CSV_1_GB.csv")) > -1)
	//{
	//	t2 = clock();
	//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	std::cout << "String Find " << x << std::endl;
	//}



	int x;
	t1 = clock();
	if ((x = GetRowCSVansi(L"D:\\CSV_0_GB.csv",2)) > -1)
	{
		t2 = clock();
		printf("HDD WDC WD10EACS-00ZJB0 (1000 GB, SATA-II): GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
		//std::cout << "String Find " << x << std::endl;
	}

	t1 = clock();
	if ((x = GetRowCSVansi(L"C:\\CSV_1_GB.csv", 4000000)) > -1)
	{
		t2 = clock();
		printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
		//std::cout << "String Find " << x << std::endl;
	}
	system("pause");
	return 0;
}

