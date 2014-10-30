#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <iomanip>
//user classes
#include "classes.h"
using namespace std;
extern int tran_cnt;
int main(int argc, char *argv[])
{
    if (argc!=8)
    {
        cout<<"Wrong input. Give bl_size,size,assoc,victim_size,l2 size,l2assoc,trace_fil"<<endl;
        exit(1);
    }
    //printing inputs
    if (!wb_dbg)
    {
    cout << "  ===== Simulator Configuration =====  " << endl;
    cout << "  BLOCKSIZE:                       " << argv[1] << endl;
    cout << "  L1_SIZE:                         " << argv[2]<< endl;
    cout << "  L1_ASSOC:                        " << argv[3] << endl;
    cout << "  Victim_Cache_SIZE:               " << argv[4] <<endl;
    cout << "  L2_SIZE:                         " << argv[5]<< endl;
    cout << "  L2_ASSOC:                        " << argv[6] << endl;
    cout << "  trace_file:                      " << argv[7]  <<endl;
    cout << "  ===================================  " << endl;
    }
    //Setting up the cache
    //Cache L2(8,514,2,0,0,NULL);
  
    Cache* L2 = new Cache(atoi(argv[1]),atoi(argv[5]),atoi(argv[6]),0,0,"L2",NULL,NULL);
    Cache* V = new Cache(atoi(argv[1]),atoi(argv[4]));
    Cache* L1 = new Cache(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),0,0,"L1",L2,V);
    V->nLevel(L2);

    if (Debug)
    {
        cout << L1 << endl;
        cout << V << endl;
        cout << L2 << endl;
    }

    ifstream trace(argv[7]);
    if (!trace)
    {
        cerr << "File not found, please check" << endl;
        exit(1);
    }
    
    while(trace)
    {
        tran_cnt++;
        if (Debug) cout << dec<<  tran_cnt << "." << endl;
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
        L1->request(addr,InTran.tranType());
        if (Debug)
        {
            cout << "L1 read " << dec << L1->getrdCnt() << " L1 read miss " << L1->getrdMCnt()
                << " L1 writes " <<L1->getwrCnt() << " L1 write misses " << L1->getwrMCnt() << endl;
            cout << "V read " << V->getrdCnt() << " V read miss " << V->getrdMCnt()
                << " V writes " <<V->getwrCnt() << " V write misses " << V->getwrMCnt() << endl;
            cout << "L2 read " << L2->getrdCnt() << " L2 read miss " << L2->getrdMCnt()
                << " L2 writes " <<L2->getwrCnt() << " L2 write misses " << L2->getwrMCnt() << endl;
            cout << "Victim write backs "<< V->getWB()<<endl;
            L1->print_contents();
            V->print_contents();
            L2->print_contents();

            cout << endl;
        }
    }
   
    if (!Debug && !wb_dbg)
    {
        L1->print_contents();
        V->print_contents();
        L2->print_contents();
    //Printing outputs
        cout << "  ====== Simulation results (raw) ======  " << endl;
        cout << "  a. number of L1 reads:" << setw(10) << right << dec <<L1->getrdCnt()<<endl;
        cout << "  b. number of L1 read misses:" << setw(10) << right << L1->getrdMCnt() - (V->getrdCnt() - V->getrdMCnt())<<endl;
        cout << "  c. number of L1 writes:" << setw(10) << right << L1->getwrCnt()<<endl;
        cout << "  d. number of L1 write misses:" << setw(10) << right << L1->getwrMCnt() - (V->getwrCnt() - V->getwrMCnt())<<endl;
        cout << "  e. L1 miss rate:" << setw(10) << right << fixed << setprecision(4) << L1->getMR()<<endl;
        cout << "  f. number of swaps:" << victim_swp <<endl; //Victim cache op
        cout << "  g. number of victim cache writeback:" << V->getWB() <<endl; //Victim cache op
        cout << "  h. number of L2 reads:" << setw(10) << right << dec <<L2->getrdCnt()<<endl;
        cout << "  i. number of L2 read misses:" << setw(10) << right << L2->getrdMCnt()<<endl;
        cout << "  j. number of L2 writes:" << setw(10) << right << L2->getwrCnt()<<endl;
        cout << "  k. number of L2 write misses:" << setw(10) << right << L2->getwrMCnt()<<endl;
        cout << "  l. L2 miss rate:" << setw(10) << right;
        if (L2->getAssoc() == 0)
            cout << static_cast<int>(L2->getMR()) << endl;
        else 
            cout << fixed << setprecision(4) << L2->getMR()<<endl;
        cout << "  m. number of L2 writebacks:" << setw(10) << right << L2->getWB()<<endl;
        if (L1->has_victim && !L2->getSize())
        {
            cout << "  n. total memory traffic: " <<setw(10) << right << V->getWB() + V->getrdMCnt() + V->getwrMCnt()  << endl;
        }
        else
            cout << "  n. total memory traffic: " <<setw(10) << right << L2->getTotMem() << endl;
    }

    if (!Debug && !wb_dbg)
    {
    cout << "  ==== Simulation results (performance) ==== " << endl;
    float l1_miss_p, l2_miss_p, l1_hit_t, l2_hit_t;
    l1_miss_p = 20 + 0.5*(static_cast<float>(L1->getBlSize())/16);
    l1_hit_t = 0.25 + 2.5*(static_cast<float>(L1->getSize())/(512*1024)) + 0.025*(static_cast<float>(L1->getBlSize())/16) + 0.025*static_cast<float>(L1->getAssoc());

    l2_miss_p = 20 + 0.5*(static_cast<float>(L2->getBlSize())/16);
    l2_hit_t = 2.5 + 2.5*(static_cast<float>(L2->getSize())/(512*1024)) + 0.025*(static_cast<float>(L2->getBlSize())/16) + 0.025*static_cast<float>(L2->getAssoc());

    float aat;
    if (L2->getAssoc() == 0)
        aat = l1_hit_t +static_cast<float>(L1->getMR())*(l1_miss_p); 
    else
        aat = l1_hit_t + static_cast<float>(L1->getMR())*(l2_hit_t + static_cast<float>(L2->getMR())*static_cast<float>(l2_miss_p));

    cout << "  1. average access time:" << setw(10) << right << dec <<aat<<" ns" <<endl;
    }

    if(Debug)
    {
    cout << "L1 " << L1->getrdHCnt() << " " << L1->getwrHCnt() << endl;
    cout << "V " << V->getrdHCnt() << " " << V->getwrHCnt() << endl;
    cout << "L2 " << L2->getrdHCnt() << " " << L2->getwrHCnt() << endl;
    }
    return 0;
}

