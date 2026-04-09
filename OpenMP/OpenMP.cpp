// OpenMP.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.


#include <iostream>
//#include <stdio.h> 
#include <omp.h> 

// Определяем N как некоторую константу для длины массива 
#define N 100000

int main(int argc, char* argv[]) {
    int a[N];


#pragma omp parallel for 
    for (int i = 0; i < N; i++) { a[i] = 2 * i; }

    std::cout << a[N-1] << "\n";

    return 0;
}
