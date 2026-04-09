// C++AMP.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//https://learn.microsoft.com/en-us/previous-versions/cpp/parallel/amp/cpp-amp-overview?view=msvc-170

#include <iostream>
#include <vector>

#define _SILENCE_AMP_DEPRECATION_WARNINGS
#include <amp.h>
using namespace concurrency;

//#include <amprt.h>
////#pragma warning(suppress:4996)//только для следующей строки
////#pragma warning(default)//Сброс поведения предупреждений к его значению по умолчанию.
////#pragma warning(disable:4996)


void StandardMethod() {

    int aCPP[] = { 1, 2, 3, 4, 5 };
    int bCPP[] = { 6, 7, 8, 9, 10 };
    int sumCPP[5];

    for (int idx = 0; idx < 5; idx++)
    {
        sumCPP[idx] = aCPP[idx] + bCPP[idx];
    }

    for (int idx = 0; idx < 5; idx++)
    {
        std::cout << sumCPP[idx] << "\n";
    }
}


const int size = 5;

void CppAmpMethod() {
    int aCPP[] = { 1, 2, 3, 4, 5 };
    int bCPP[] = { 6, 7, 8, 9, 10 };
    int sumCPP[size];

    // Create C++ AMP objects.
    array_view<const int, 1> a(size, aCPP);
    array_view<const int, 1> b(size, bCPP);
    array_view<int, 1> sum(size, sumCPP);
    sum.discard_data();

    parallel_for_each(
        // Define the compute domain, which is the set of threads that are created.
        sum.extent,
        // Define the code to run on each thread on the accelerator.
        [=](index<1> idx) restrict(amp) {
            sum[idx] = a[idx] + b[idx];
        }
    );

    // Print the results. The expected output is "7, 9, 11, 13, 15".
    for (int i = 0; i < size; i++) {
        std::cout << sum[i] << "\n";
    }
}

int main()
{
    std::cout << "Hello World!\n";

    accelerator acc = accelerator(accelerator::default_accelerator);

    // Early out if the default accelerator doesn't support shared memory.
    if (!acc.supports_cpu_shared_memory)
    {
        std::cout << "The default accelerator does not support shared memory" << std::endl;
        return 1;
    }
    std::vector<accelerator> accs = acc.get_all();
    //std::for_each(accs.cbegin(), accs.cend(), [=, &a](const accelerator& a);
    StandardMethod();
    CppAmpMethod();
}


