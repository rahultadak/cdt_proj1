#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <iomanip>
//user classes
#include "classes.h"

using namespace std;
int main(int argc, char *argv[])
{
    if (argc!=7)
    {
        cout<<"Wrong input. Give bl_size,size,assoc,rep_pol,wr_pol,trace_fil"<<endl;
        exit(1);
    }

    //Setting up the cache
    //Cache L2(8,514,2,0,0,NULL);
    Cache L1(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),NULL);
    L1.print_info(argv[6]);
    
    ifstream trace(argv[6]);
    if (!trace)
    {
        cerr << "File not found, please check" << endl;
        exit(1);
    }
    
    while(trace)
    {
        //taking input
        string strIn;
        //getting the line
        getline(trace, strIn);
        //breaking from the while loop if this is the last line of the file.
        if (trace.eof()) break;

        //Setting the type of transaction
        Transaction InTran;
        if (strIn[0] == 'r')
            InTran.setType(0);
        else if (strIn[0] == 'w')
            InTran.setType(1);

        InTran.setAddr(strIn);
        
        int addr = InTran.retAddr();
        L1.request(addr,InTran.tranType());
    }
    L1.print_contents();
    L1.print_raw_op();
    L1.print_perf_op();
    return 0;
}

