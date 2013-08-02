// Standard includes
#include <iostream>
#include <sstream>

// ROOT includes
#include <TROOT.h>


// Standard namespaces
using namespace std;


int main()
{
    // Declare value
    int target = 0;
    cout << target << endl;

    // Set value
    int *target_p = &target;
    stringstream ss;
    ss << "*(int *)(" << target_p << ") = 5";
    cout << ss.str() << endl;
    cout << gROOT->ProcessLine(ss.str().c_str()) << endl;



    cout << target << endl;

    return 0;
}
