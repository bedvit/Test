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
	DWORD  dwBytesReadWork=0;
	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты
	bool ERR_HANDLE_EOF = false;
	size_t i = 0;
	//char* buf = new char[nNumberOfBytesToRead]; //память
	//char* bufWork = new char[nNumberOfBytesToRead + 1]; //буфер Рабочий

	char* notAlignBuf = new char[nNumberOfBytesToRead + 4096]; //память
	char* buf = notAlignBuf; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufBufWork = new char[nNumberOfBytesToRead + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufBufWork; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  


	bufWork[0] = '\0';//добавим нуль-терминатор
	bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
	char* find;// указатель для поиска
	size_t strCount = 1; //счетчик строк

	// создаем события с автоматическим сбросом
	hEndRead = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEndRead == NULL) { return GetLastError(); }

	// инициализируем структуру OVERLAPPED
	ovl.Offset = 0;         // младшая часть смещения равна 0
	ovl.OffsetHigh = 0;      // старшая часть смещения равна 0
	ovl.hEvent = hEndRead;   // событие для оповещения завершения чтения

	i = sizeof(ovl.Offset);
	// открываем файл для чтения
	hFile = CreateFile(
		path,   // имя файла
		GENERIC_READ,          // чтение из файла
		FILE_SHARE_READ,       // совместный доступ к файлу
		NULL,                  // защиты нет
		OPEN_EXISTING,         // открываем существующий файл
		FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,// ,// | FILE_FLAG_RANDOM_ACCESS,//FILE_FLAG_NO_BUFFERING,  // асинхронный ввод//отключаем системный буфер
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		delete[] notAlignBuf;
		delete[] notAlignBufBufWork;
		return -1;
	}
	//FlushFileBuffers(hFile); //Сбрасывает буферы указанного файла и вызывает запись всех буферизованных данных в файл.
	// читаем данные из файла
	for (;;)
	{
		DWORD  dwBytesRead;
		DWORD  dwError;
		find = buf; //буфер
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
				// закроем файл после обработки данных в рабочем буфере
				ERR_HANDLE_EOF = true;
				break;
			default:
				// закрываем дескрипторы
				CloseHandle(hFile);
				CloseHandle(hEndRead);
				delete[] notAlignBuf;
				delete[] notAlignBufBufWork;
				return -1;
			}
		}
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
		find = bufWork; //буфер
		find = strchr(find, '\n');
		while (find != NULL)
		{
			find = find + 1;
			strCount++;
			find = strchr(find, '\n');
			
		}
		//i=strlen(bufWork);
		//std::cout << i << std::endl;
		//работаем асинхронно, выполняем код, пока ждем чтение с диска//
		// проверим результат работы асинхронного чтения 
		if (ERR_HANDLE_EOF)
		{
			// мы достигли конца файла в ходе асинхронной операции	
			// закрываем дескрипторы
			CloseHandle(hFile);
			CloseHandle(hEndRead);
			delete[] notAlignBuf;
			delete[] notAlignBufBufWork;
			return strCount;
		}
		// ждем, пока завершится асинхронная операция чтения
		WaitForSingleObject(hEndRead, 1000);// INFINITE);

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
				delete[] notAlignBuf;
				delete[] notAlignBufBufWork;
				return strCount;
			}
			// решаем что делать с другими случаями ошибок
			default:
			CloseHandle(hFile);
			CloseHandle(hEndRead);
			delete[] notAlignBuf;
			delete[] notAlignBufBufWork;
			return -1;
			}// конец процедуры switch (dwError = GetLastError())
		}

		//синхронно копируем данные из буфера в рабочий буфер, далее с ним работаем асинхронно
		memcpy(bufWork, buf, nNumberOfBytesToRead);

		// увеличиваем смещение в файле
		dwBytesReadWork = dwBytesRead;//кол-во считанных байт
		ovl.Offset += nNumberOfBytesToRead;// sizeof(n);
	}
}

int GetRowCSVansi(PCTSTR path, int strNum)
{
	HANDLE  hFile;     // дескриптор файла
	HANDLE  hEndRead;  // дескриптор события
	OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу
	const DWORD  nNumberOfBytesToRead = 16777216;//8388608;//читаем в буфер байты
	char* buf = new char[nNumberOfBytesToRead]; //буфер CreateFile
	//char* notalign = new char[nNumberOfBytesToRead + 1 + 512]; //память
	//char* buf = notalign; //буфер
	//if (size_t(buf) % 512) { buf += 512 - (size_t(buf) % 512); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора 

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
		FILE_FLAG_OVERLAPPED,// | FILE_FLAG_NO_BUFFERING,  // асинхронный ввод
		NULL                   // шаблона нет
	);
	// проверяем на успешное открытие
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndRead);
		delete[] buf;
		delete[] bufWork;
		return -1;
	}
	// читаем данные из файла
	for (;;)
	{
		DWORD  dwBytesRead;
		DWORD  dwError;
		find= buf; //буфер
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
			case ERROR_HANDLE_EOF: {break; } // мы достигли конца файла читалкой ReadFile
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

int FindInCSVansi(PCTSTR path, const char* findStr, bool multiLine)
{
HANDLE  hFile;     // дескриптор файла
HANDLE  hEndRead;  // дескриптор события
OVERLAPPED  ovl;   // структура управления асинхронным доступом к файлу
const DWORD  nNumberOfBytesToRead = 33554432; //16777216;//8388608;//читаем в буфер байты
//char* buf = new char[nNumberOfBytesToRead]; //буфер CreateFile
//char* bufWork = new char[nNumberOfBytesToRead + 1]; //буфер Рабочий
	char* notAlignBuf = new char[nNumberOfBytesToRead + 4096]; //память
	char* buf = notAlignBuf; //буфер
	if (size_t(buf) % 4096) { buf += 4096 - (size_t(buf) % 4096); }//адрес принимающего буфера тоже должен быть выровнен по размеру сектора/страницы 

	char* notAlignBufBufWork = new char[nNumberOfBytesToRead + 4096 + 1]; //буфер Рабочий
	char* bufWork = notAlignBufBufWork; //буфер
	if (size_t(bufWork) % 4096) { bufWork += 4096 - (size_t(bufWork) % 4096); }//адрес рабочего буфера тоже выровнял по размеру сектора/страницы  

bufWork[0] = '\0';//добавим нуль-терминатор
bufWork[nNumberOfBytesToRead] = '\0';//добавим нуль-терминатор
char* find;// указатель для поиска
int strCount = 1; //счетчик строк
std::string strOut;
DWORD  dwBytesReadWork;
int findStatus = 0;
DWORD ignoreOffset=0;

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
	FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,  // асинхронный ввод//отключить системный буфер
	NULL                   // шаблона нет
);
// проверяем на успешное открытие
if (hFile == INVALID_HANDLE_VALUE)
{
	CloseHandle(hEndRead);
	delete[] notAlignBuf;
	delete[] notAlignBufBufWork;
	return -1;
}
// читаем данные из файла
for (;;)
{
	DWORD  dwBytesRead;
	DWORD  dwError;
	find = buf; //буфер
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
		case ERROR_HANDLE_EOF: { 
			break; 
		} // мы достигли конца файла читалкой ReadFile
		default: {goto return1; }// другие ошибки
		}
	}
	//работаем асинхронно, выполняем код, пока ждем чтение с диска//
	if (ovl.Offset!=ignoreOffset)//игнорируем первую итерацию и этерации прошедшие следующим буфером, когда нужно было вернутся в предыдущий буфер
	{
		bufWork[dwBytesReadWork] = '\0';//добавим нуль-терминатор
		find = bufWork; //новый буфер
	go_1:
		char* strStart = bufWork;
		char* strEnd = NULL;

		if (findStatus == 0)//goFind
		{
			find = strstr(find, findStr);
			if (find != NULL) //если нужная подстрока найдена
			{

				for (strStart = find; strStart >= bufWork; strStart--)
				{
					if (*strStart == '\n') { break; } //если нашли начало строки
				}
				strStart++;
				if (ovl.Offset == nNumberOfBytesToRead || strStart != bufWork)//если первый буфер или найдено начало строки
				{
					strEnd = strchr(find, '\n');
					if (strEnd != NULL) //если нашли конец строки
					{
						strOut = strOut + std::string(strStart, strEnd - strStart + 1);
						if (!multiLine)
						{
							goto return0;
						}
						else//поиск в следующей строке начинаем с конца предыдущей
						{
							find = strEnd++;
							findStatus = 0;
							goto go_1;
						}
					}
					else//если конец строки в следующем буфере или конец файла
					{
						strOut = strOut + std::string(strStart);
						findStatus = 1;
					}
				}
				else //если начало строки не найдено и это не первый буфер смотрим предыдущий буфер
				{
					findStatus = 2;
					ovl.Offset -= nNumberOfBytesToRead * 3;
					ignoreOffset = ovl.Offset + nNumberOfBytesToRead;
				}
			} //если нужная подстрока найдена
		}
		else if (findStatus == 1)//goBuf
		{
			strEnd = strchr(find, '\n');
			if (strEnd != NULL) //если нашли конец строки
			{
				strOut = strOut + std::string(find, strEnd - find+1);
				findStatus = 0;
				if (!multiLine)
				{
					goto return0;
				}
				else//поиск в следующей строке начинаем с конца предыдущей
				{
					find = strEnd++;
					findStatus = 0;
					goto go_1;
				}
			}
			else//если конец строки в следующем буфере или конец файла
			{
				strOut = strOut + std::string(find);
			}
		}
		else if (findStatus == 2)//goBackBuf
		{

			for (strStart = bufWork + dwBytesReadWork; strStart >= bufWork; strStart--)
			{
				if (*strStart == '\n') { break; }//если нашли начало строки
			}
			strStart++;
			if (ovl.Offset == nNumberOfBytesToRead || strStart != bufWork)//если первый буфер или найдено начало строки
			{
				find = strStart++;
				findStatus = 1;
				goto go_1;
			}
			else
			{
				ovl.Offset -= nNumberOfBytesToRead * 3;//если начало строки не найдлено смотрим предыдущий буфер
				ignoreOffset = ovl.Offset + nNumberOfBytesToRead;
			}

		}
	}
	//работаем асинхронно, выполняем код, пока ждем чтение с диска//

	// ждем, пока завершится асинхронная операция чтения
	WaitForSingleObject(hEndRead, INFINITE);

	// проверим результат работы асинхронного чтения // если возникла проблема ... 
	if (!GetOverlappedResult(hFile, &ovl, &dwBytesRead, FALSE))
	{
		switch (dwError = GetLastError())// решаем что делать с кодом ошибки
		{
		case ERROR_HANDLE_EOF: 
		{
			if (findStatus != 2) goto return0; break;//если конец файла. но мы должны вернутся найти начало строки.
		}// мы достигли конца файла в ходе асинхронной операции
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
delete[] notAlignBuf;
delete[] notAlignBufBufWork;
return 0;
return1:
CloseHandle(hFile);
CloseHandle(hEndRead);
delete[] notAlignBuf;
delete[] notAlignBufBufWork;
return -1;
}


int createfile() //создание файла для теста
{
	const int N = 4000000; // количество строк в файле
	std::ofstream fout("C:\\CSV_1_GB.csv");
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
	int x;

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
//
	//t1 = clock();
	//if ((x = GetRowsCountCSVansi(L"C:\\CSV_1_GB.csv")) > -1)
	//{
	//	t2 = clock();
	//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	std::cout << "String Find " << x << std::endl;
	//}
//
	//t1 = clock();
	//if ((x = GetRowsCountCSVansi(L"C:\\CSV_1_GB.csv")) > -1)
	//{
	//	t2 = clock();
	//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	std::cout << "String Find - " << x << std::endl;
	//}
//
	//t1 = clock();
	//if ((x = GetRowCSVansi(L"C:\\CSV_1_GB.csv", 4000000)) > -1)
	//{
	//	t2 = clock();
	//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): GetRowCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	//std::cout << "String Find " << x << std::endl;
	//}


	//t1 = clock();
	//if ((x = FindInCSVansi(L"D:\\CSV_0_GB.csv","0",1)) > -1)
	//{
	//	t2 = clock();
	//	printf("HDD WDC WD10EACS-00ZJB0 (1000 GB, SATA-II): GetRowsCountCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	//std::cout << "String Find " << x << std::endl;
	//}

	//	t1 = clock();
	//if ((x = FindInCSVansi(L"D:\\CSV_10_GB.csv","40000000",1)) > -1)
	//{
	//	t2 = clock();
	//	printf("HDD WDC WD10EACS-00ZJB0 (1000 GB, SATA-II): FindInCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	//std::cout << "String Find " << x << std::endl;
	//}

	//t1 = clock();
	//if ((x = FindInCSVansi(L"C:\\CSV_10_GB.csv","40000000",0)) > -1)
	//{
	//	t2 = clock();
	//	printf("SSD KINGSTON SV300S37A120G (120 GB, SATA-III): FindInCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//	//std::cout << "String Find " << x << std::endl;
	//}




t1 = clock();
if ((x = GetRowsCountCSVansi(L"C:\\CSV_10_GB.csv")) > -1)
{
	t2 = clock();
	printf("HDD 500 GB, 7200 RPM, SATA - III: FindInCSVansi: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	std::cout << "String Find " << x << std::endl;
}


	system("pause");
	return 0;
}


//
//extern "C" __declspec(dllexport) LPXLOPER12  WINAPI FindInCSV(char *  arg1, char *  arg2, wchar_t *  arg3)
//{
//	LPXLOPER12 OperOut = new XLOPER12;
//	OperOut->xltype = xltypeStr | xlbitDLLFree;
//	OperOut->val.str = new XCHAR[32767 + 2]; //+1 под размер + 1 под нуль-терминатор
//	OperOut->val.str[0] = 0;
//	OperOut->val.str[1] = '\0';
//	std::filebuf fb;
//	if (fb.open(arg1, std::ios::in | std::ios::binary))
//	{
//		int x = 0;
//		int lenStrOut = 0; //0й элемент - размер строки
//		int lenArg3 = wcslen(arg3);
//		bool overflow = false;
//		std::string str;
//		std::istream is(&fb);
//		XCHAR* strTmp = OperOut->val.str + 1;
//
//		while (is)
//		{
//			std::getline(is, str);
//			size_t found = str.find(arg2);
//			if (found != std::string::npos)
//			{
//				x++;
//				if (lenArg3 > 0 && x > 1)
//				{
//					if (lenStrOut + lenArg3 > 32767) { overflow = true;  goto end_; }
//					wcscpy(strTmp + lenStrOut, arg3);
//					lenStrOut = lenStrOut + lenArg3;
//
//					if (lenStrOut + str.length() > 32767) { overflow = true;  goto end_; }
//					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, strTmp + lenStrOut, str.length() + 1);
//					lenStrOut = lenStrOut + str.length();
//				}
//				else if (lenArg3 > 0)
//				{
//					if (lenStrOut + str.length() > 32767) { overflow = true;  goto end_; }
//					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, strTmp + lenStrOut, str.length() + 1);
//					lenStrOut = lenStrOut + str.length();
//				}
//				else
//				{
//					if (lenStrOut + str.length() > 32767) { overflow = true;  goto end_; }
//					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, strTmp + lenStrOut, str.length() + 1);
//					lenStrOut = lenStrOut + str.length();
//					goto end_;
//				}
//			};
//		}
//	end_:
//		if (overflow) {
//			wcscpy(strTmp, L"Qverflow. Max char 32767\0");
//			OperOut->val.str[0] = 24;
//		}
//		else
//		{
//			OperOut->val.str[0] = lenStrOut;
//		}
//		fb.close();
//	}
//	return OperOut;
//}
//
//extern "C" __declspec(dllexport) LPXLOPER12  WINAPI GetRowCSV(char *  arg1, int  arg2)
//{
//	LPXLOPER12 OperOut = new XLOPER12{ xltypeNil };
//
//	std::filebuf fb;
//	if (fb.open(arg1, std::ios::in | std::ios::binary))
//	{
//		int x = 0;
//		std::istream is(&fb);
//		while (is) {
//			std::string str;
//			std::getline(is, str);
//			if (++x == arg2)
//			{
//				fb.close();
//				OperOut->xltype = xltypeStr | xlbitDLLFree;
//				OperOut->val.str = new XCHAR[str.length() + 2];//+1 под размер+1 под нуль-терминатор
//				XCHAR* strTmp = new XCHAR[str.length() + 1]; //+1 под нуль - терминатор
//				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, strTmp, str.length() + 1);
//				for (int i = 0; i <= str.length(); i++)
//				{
//					OperOut->val.str[i + 1] = strTmp[i];
//				}
//				OperOut->val.str[0] = str.length();
//				delete[] strTmp;
//				return OperOut;
//			};
//		}
//		fb.close();
//	}
//	return OperOut;
//}