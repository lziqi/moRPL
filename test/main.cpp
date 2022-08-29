
#include <iostream>
using namespace std;

int main()
{
    cout << "char " << sizeof(char) << endl;     // 1
    cout << "short " << sizeof(short) << endl;   // 2
    cout << "int " << sizeof(int) << endl;       // 4
    cout << "long " << sizeof(long) << endl;     // 4
    cout << "float " << sizeof(float) << endl;   // 4
    cout << "double " << sizeof(double) << endl; // 8
    return 0;
}