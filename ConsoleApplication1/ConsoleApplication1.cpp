// ConsoleApplication1.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//




//#define MAX(a,b) \
//    ({ __typeof__ (a) _a = (a); \
//     __typeof__ (b) _b = (b); \
//     _a > _b ? _a : _b; })

//#include "pch.h"
//#include <stdio.h>
//#include <string.h>

//
//
//#define max(a,b) (((a) > (b)) ? (a) : (b))
//
//
//
//#define XSIZE   500
//#define ASIZE   256
//void preBmBc(char *x, int m, int bmBc[]) {
//	int i;
//
//	for (i = 0; i < ASIZE; ++i)
//		bmBc[i] = m;
//	for (i = 0; i < m - 1; ++i)
//		bmBc[x[i]] = m - i - 1;
//}
//
//
//void suffixes(char *x, int m, int *suff) {
//	int f, g, i;
//
//	suff[m - 1] = m;
//	g = m - 1;
//	for (i = m - 2; i >= 0; --i) {
//		if (i > g && suff[i + m - 1 - f] < i - g)
//			suff[i] = suff[i + m - 1 - f];
//		else {
//			if (i < g)
//				g = i;
//			f = i;
//			while (g >= 0 && x[g] == x[g + m - 1 - f])
//				--g;
//			suff[i] = f - g;
//		}
//	}
//}
//
//void preBmGs(char *x, int m, int bmGs[]) {
//	int i, j, suff[XSIZE];
//
//	suffixes(x, m, suff);
//
//	for (i = 0; i < m; ++i)
//		bmGs[i] = m;
//	j = 0;
//	for (i = m - 1; i >= 0; --i)
//		if (suff[i] == i + 1)
//			for (; j < m - 1 - i; ++j)
//				if (bmGs[j] == m)
//					bmGs[j] = m - 1 - i;
//	for (i = 0; i <= m - 2; ++i)
//		bmGs[m - 1 - suff[i]] = m - 1 - i;
//}
//
//
//int BM(char *x, int m, char *y, int n) {
//	int i, j, bmGs[XSIZE], bmBc[ASIZE];
//
//	/* Preprocessing */
//	preBmGs(x, m, bmGs);
//	preBmBc(x, m, bmBc);
//
//	puts("bmBc:");
//	for (i = 0; i < 256; ++i)
//		if (bmBc[i] != m)
//			printf("%c ", i);
//	puts("");
//	for (i = 0; i < 256; ++i)
//		if (bmBc[i] != m)
//			printf("%d ", bmBc[i]);
//	puts("");
//	puts("BmGs:");
//	for (i = 0; i < m; ++i)
//		printf("%d ", i);
//	puts("");
//	for (i = 0; i < m; ++i)
//		printf("%d ", bmGs[i]);
//	puts("");
//	puts("matched index:");
//
//	/* Searching */
//	j = 0;
//	while (j <= n - m) {
//		for (i = m - 1; i >= 0 && x[i] == y[i + j]; --i);
//		if (i < 0) {
//			printf("%d\n", j);
//			return j;
//			j += bmGs[0];
//		}
//		else
//			j += max(bmGs[i], bmBc[y[i + j]] - m + 1 + i);
//	}
//	return j;
//}
//
//int main() {
//	char x[] = "gcagagag";
//	char y[] = "gcatcgcagagagtatacagtacggcagagag";
//	int i = 	BM(x, strlen(x), y, strlen(y));
//	return 0;
//}
//
//// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
//// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"
//
//// Советы по началу работы 
////   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
////   2. В окне Team Explorer можно подключиться к системе управления версиями.
////   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
////   4. В окне "Список ошибок" можно просматривать ошибки.
////   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
////   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.

#include "pch.h"
#include <string>
#include <ctime>
#include <algorithm>
#include <functional>
#include <iostream>

#define XSIZE 47
#define ASIZE 256
#define max(a,b) (((a) > (b)) ? (a) : (b))

void preBmBc(unsigned char *x, int m, int bmBc[]) {
	int i;
	for (i = 0; i < ASIZE; ++i)
		bmBc[i] = m;
	for (i = 0; i < m - 1; ++i)
		bmBc[x[i]] = m - i - 1;
}


void suffixes(unsigned char *x, int m, int *suff) {
	int f, g, i;
	suff[m - 1] = m;
	g = m - 1;
	for (i = m - 2; i >= 0; --i) {
		if (i > g && suff[i + m - 1 - f] < i - g)
			suff[i] = suff[i + m - 1 - f];
		else {
			if (i < g)
				g = i;
			f = i;
			while (g >= 0 && x[g] == x[g + m - 1 - f])
				--g;
			suff[i] = f - g;
		}
	}
}

void preBmGs(unsigned char *x, int m, int bmGs[]) {
	int i, j, suff[XSIZE];
	suffixes(x, m, suff);
	for (i = 0; i < m; ++i)
		bmGs[i] = m;
	j = 0;
	for (i = m - 1; i >= 0; --i)
		if (suff[i] == i + 1)
			for (; j < m - 1 - i; ++j)
				if (bmGs[j] == m)
					bmGs[j] = m - 1 - i;
	for (i = 0; i <= m - 2; ++i)
		bmGs[m - 1 - suff[i]] = m - 1 - i;
}


int BM(unsigned char *x, int m, unsigned char *y, int n) {
	int i, j, bmGs[XSIZE], bmBc[ASIZE];
	/* Preprocessing */
	preBmGs(x, m, bmGs);
	preBmBc(x, m, bmBc);
	j = 0;
	while (j <= n - m) {
		for (i = m - 1; i >= 0 && x[i] == y[i + j]; --i);
		if (i < 0) {
			return j;
		}
		else
			j += max(bmGs[i], bmBc[y[i + j]] - m + 1 + i);
	}
	return -1;
}

int main() {
	const int N = 4000000; // количество строк
	std::string str = "Пятый; Тридацть четвертый; 44221100; BBB; CIFRAPOLE; POLEPVTRETYE; ODIN ODIN - TRI; CC; 01.01.2013; 01.01.2013; 8963; 2UTY39ADVGKR; СU707039; 40200М У0026034; -; 11; 2; 150; 1998; 1; -; 21; 1980; 1490; НОМЕР ПЯТЬ; -; ПОЛЕ ОДИН; ПОЛЕ ОДИН ПЯТЬ; -; -";
	std::string Out("");
	std::string needle = "4000000Пятый; Тридацть четвертый; 44221100; BBB";
	Out.reserve(1027000000);
	for (int i = 0; i < N; ++i)
	{
		Out.append(std::to_string(i + 1) + str + '\n');
	}

	clock_t t1;
	clock_t t2;

	t1 = clock();
	auto s = strstr(Out.c_str(), needle.c_str());
	t2 = clock();
	printf("strstr: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	t1 = clock();
	auto f = Out.find(needle);
	t2 = clock();
	printf("std::find: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//std::cout << f << std::endl;

	t1 = clock();
	auto sr = std::search(Out.begin(), Out.end(), needle.begin(), needle.end());
	t2 = clock();
	printf("std::search: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	
	t1 = clock();
	auto srb = std::search(Out.begin(), Out.end(), std::boyer_moore_searcher(needle.begin(), needle.end()));
	t2 = clock();
	printf("std::boyer_moore_searcher: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	t1 = clock();
	auto srbs = std::search(Out.begin(), Out.end(), std::boyer_moore_horspool_searcher(needle.begin(), needle.end()));
	t2 = clock();
	printf("std::boyer_moore_horspool_searcher: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	//выносим часть разовых расчетов для подстроки поиска//
	auto bms2 = std::boyer_moore_searcher(needle.begin(), needle.end());
	t1 = clock();
	auto srb2 = std::search(Out.begin(), Out.end(), bms2);
	t2 = clock();
	printf("std::boyer_moore_searcher(needle_no_time): Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	auto bmhs2 = std::boyer_moore_horspool_searcher(needle.begin(), needle.end());
	t1 = clock();
	auto srbs2 = std::search(Out.begin(), Out.end(), bmhs2);
	t2 = clock();
	printf("std::boyer_moore_horspool_searcher(needle_no_time): Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	
	t1 = clock();
	auto bm = BM((unsigned char*)needle.c_str(), needle.length(), (unsigned char*)Out.c_str(), Out.length());
	t2 = clock();
	printf("BM: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	//std::cout << bm << std::endl;


	system("pause");
	return 0;
}
