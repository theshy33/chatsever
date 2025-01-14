#include <iostream>
#include <thread>
using namespace std;

int main()
{
    //获取计算机核数
    int num_threads = thread::hardware_concurrency();
    cout << "The number of threads is: " << num_threads << endl;

    return 0;
}