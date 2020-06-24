﻿// ConsoleApplication1.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//




//#define MAX(a,b) \
//    ({ __typeof__ (a) _a = (a); \
//     __typeof__ (b) _b = (b); \
//     _a > _b ? _a : _b; })

#include "pch.h"
#include <stdio.h>
#include <string.h>



#define max(a,b) (((a) > (b)) ? (a) : (b))



#define XSIZE   500
#define ASIZE   256
void preBmBc(char *x, int m, int bmBc[]) {
	int i;

	for (i = 0; i < ASIZE; ++i)
		bmBc[i] = m;
	for (i = 0; i < m - 1; ++i)
		bmBc[x[i]] = m - i - 1;
}


void suffixes(char *x, int m, int *suff) {
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

void preBmGs(char *x, int m, int bmGs[]) {
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


int BM(char *x, int m, char *y, int n) {
	int i, j, bmGs[XSIZE], bmBc[ASIZE];

	/* Preprocessing */
	preBmGs(x, m, bmGs);
	preBmBc(x, m, bmBc);

	puts("bmBc:");
	for (i = 0; i < 256; ++i)
		if (bmBc[i] != m)
			printf("%c ", i);
	puts("");
	for (i = 0; i < 256; ++i)
		if (bmBc[i] != m)
			printf("%d ", bmBc[i]);
	puts("");
	puts("BmGs:");
	for (i = 0; i < m; ++i)
		printf("%d ", i);
	puts("");
	for (i = 0; i < m; ++i)
		printf("%d ", bmGs[i]);
	puts("");
	puts("matched index:");

	/* Searching */
	j = 0;
	while (j <= n - m) {
		for (i = m - 1; i >= 0 && x[i] == y[i + j]; --i);
		if (i < 0) {
			printf("%d\n", j);
			return j;
			j += bmGs[0];
		}
		else
			j += max(bmGs[i], bmBc[y[i + j]] - m + 1 + i);
	}
	return j;
}

int main() {
	char x[] = "gcagagag";
	char y[] = "gcatcgcagagagtatacagtacggcagagag";
	int i = 	BM(x, strlen(x), y, strlen(y));
	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
