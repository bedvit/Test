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

int main0() {
	const int N = 4000000; // количество строк
	std::string str = "Пятый; Тридацть четвертый; 44221100; BBB; CIFRAPOLE; POLEPVTRETYE; ODIN ODIN - TRI; CC; 01.01.2013; 01.01.2013; 8963; 2UTY39ADVGKR; СU707039; 40200М У0026034; -; 11; 2; 150; 1998; 1; -; 21; 1980; 1490; НОМЕР ПЯТЬ; -; ПОЛЕ ОДИН; ПОЛЕ ОДИН ПЯТЬ; -; -";
	std::string Out("");
	std::string needle = "3456789Пятый; Тридацть четвертый; 44221100; BBB";
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
	std::cout << "Position " << s - Out.c_str() << std::endl;

	t1 = clock();
	auto f = Out.find(needle);
	t2 = clock();
	printf("find: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	std::cout << "Position " << f << std::endl;
	std::cout << "Out.length() = " << Out.length() << std::endl;

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


int main() {
	const int N = 4000000; // количество строк
	std::string str = "Пятый; Тридацть четвертый; 44221100; BBB; CIFRAPOLE; POLEPVTRETYE; ODIN ODIN - TRI; CC; 01.01.2013; 01.01.2013; 8963; 2UTY39ADVGKR; СU707039; 40200М У0026034; -; 11; 2; 150; 1998; 1; -; 21; 1980; 1490; НОМЕР ПЯТЬ; -; ПОЛЕ ОДИН; ПОЛЕ ОДИН ПЯТЬ; -; -";
	std::string Out("");
	std::string needle = "3456789Пятый; Тридацть четвертый; 44221100; BBB";
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
	std::cout << "Position " << s - Out.c_str() << std::endl;

	t1 = clock();
	auto f = Out.find(needle);
	t2 = clock();
	printf("std::find: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки
	std::cout << "Position " << f << std::endl;
	std::cout << "Out.length() = " << Out.length() << std::endl;
	
	t1 = clock();
	auto sr = std::search(Out.begin(), Out.end(), needle.begin(), needle.end());
	t2 = clock();
	printf("std::search: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	auto bms = std::boyer_moore_searcher(needle.begin(), needle.end());
	t1 = clock();
	auto srb = std::search(Out.begin(), Out.end(), bms);
	t2 = clock();
	printf("std::boyer_moore_searcher: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	auto bmhs = std::boyer_moore_horspool_searcher(needle.begin(), needle.end());
	t1 = clock();
	auto srbs = std::search(Out.begin(), Out.end(), bmhs);
	t2 = clock();
	printf("std::boyer_moore_horspool_searcher: Time - %f\n", (t2 - t1 + .0) / CLOCKS_PER_SEC); // время отработки

	system("pause");
	return 0;
}