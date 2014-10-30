#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <iomanip>

using namespace std;

//struct req_op{
//    int index,tag;
//    bool dirty;
//};

//transaction class
class Transaction{
    private:
        int type,addr;
        
    public:
        void setType(int x) { type = x;}
        int tranType() { return type;}

        void setAddr(const string &x) 
        {
            istringstream adr_buf(x.substr(2));
            adr_buf>>hex>>addr;
        }
        int retAddr() { return addr; }

};

//Block Class
class Block{
    private:
        int block_id,tag,bl_size,cnt_block;
        bool valid,dirty;

    public:
        void BlkSize(int x) { bl_size = x; }
        int rdBlkSize() { return bl_size; };

        int getTag() { return tag;}
        void updateTag(int in_tag) { tag = in_tag;} 

        bool isValid() { return valid;}
        void updateValid(bool in) { valid = in;}

        bool getDirty() { return dirty;}
        void updateDirty(bool in) { dirty = in;}

        int getCnt() { return cnt_block; }
        void updateCnt(int x) { cnt_block = x;}
};

//Set Class
class Set{
    private:
        int set_id,cnt_set,num_blk;

    public:
        vector<Block> SetBlocks;
        //vector<int> cnt_blks;
        
        void SetSize(int x) { num_blk =x; }
        int rdSetSize() { return num_blk; }

        int getCntSet() { return cnt_set;}
        void updateCntSet(int x) { cnt_set = x;}

        void cnt_set_init() { cnt_set = 0;}

        void setup_Blocks(int x,int rep_pol)
        {
            SetBlocks.resize(num_blk);
            for (int i=0;i<num_blk;i++)
            {
                SetBlocks.at(i).BlkSize(x);
                SetBlocks.at(i).updateTag(0);
                SetBlocks.at(i).updateValid(false);
                SetBlocks.at(i).updateDirty(false);

                if (!rep_pol){ SetBlocks.at(i).updateCnt(i);}
                else { SetBlocks.at(i).updateCnt(0);}

            }
        }

        //int getIndex() { return index;}
        //void updateIndex(int in_index) { index = in_index;}
};

//Cache class
class Cache{
    private:
    int bl_size;
    int size;
    int assoc;
    int n_blk;
    int n_set;
    int rep_pol;
    int wr_pol;
    Cache *nextLevel;

    //Counters
    long rdCnt,rdMCnt, wrCnt, wrMCnt,WB,TotMem;
    float MR;

    public:
    vector<Set> CacheSets;
    int bl_offset_mask, index_mask, tag_mask;

    //Default constructor
    Cache()
    {
        bl_size = 4;
        size = 128;
        assoc = 1;
        rep_pol = 1;
        wr_pol = 1;
    }

    //Constructor for input
    Cache(int a,int b,int c,int d,int e, Cache *nL)
    {
        bl_size = a;
        size = b;
        assoc = c;
        rep_pol = d;
        wr_pol = e;
        n_blk = size / bl_size;
        n_set = n_blk / assoc;

        nextLevel = nL;

        getBlOffsetMask();
        getIndexMask();
        getTagMask();

        setup_Sets();
        make_Blocks();
    
        //resetting counters
        rdCnt=0;
        rdMCnt=0;
        wrCnt=0;
        wrMCnt=0;
        WB=0;
        TotMem=0;
    }

    int getSize() { return size;}
    int getBlSize() { return bl_size;}
    int getNumBlk() { return n_blk;}
    int getNumSet() { return n_set;}
    int getWrPol() { return wr_pol;}
    
    int getrdCnt() { return rdCnt;}
    void updaterdCnt() { rdCnt++;}
    
    int getwrCnt() { return wrCnt;}
    void updatewrCnt() { wrCnt++;}

    int getrdMCnt() { return rdMCnt;}
    void updaterdMCnt() { rdMCnt++;}

    int getwrMCnt() {return wrMCnt;}
    void updatewrMCnt() {wrMCnt++;}

    int getWB() { return WB;}
    void updateWB() { WB++;}//=bl_size;}

    int getTotMem() { return TotMem;}
    void updateTotMem() {TotMem++;}//=bl_size;}

    float getMR() 
    {
        MR = ((float)rdMCnt + wrMCnt)/(rdCnt + wrCnt);
        return MR;
    }

    void getBlOffsetMask()
    {
        //Mask needs to be for the log2(bl_size) number of bits,
        //Subtracting 1 will give us the corresponding mask
        //as the indices start fro 0 and end at bl_size - 1.
        bl_offset_mask = bl_size - 1;
    }

    void getIndexMask()
    {
        int x = n_set - 1;
        //Indices start from 0 thus subtracting 1
        
        index_mask = x*bl_size;
        //This is to shift the mask by block size, thus givin the index mask
    }

    void getTagMask()
    {
        tag_mask = 0xFFFFFFFF - (bl_offset_mask + index_mask);
    }

    int setup_Sets()
    {
        CacheSets.resize(n_set);
        for (int i=0;i<n_set;i++)
        {
            CacheSets.at(i).SetSize(assoc);
            if (rep_pol) { CacheSets.at(i).cnt_set_init();}
        }
        return 0;
    }

    void make_Blocks()
    {
        for (int i=0;i<n_set;i++)
        {
            CacheSets.at(i).setup_Blocks(bl_size,rep_pol);
        }
    }

    void updateBlock(int addr, int index, int tag, int k, int rw)
    {
        //Writeback, first, then eviction
        if (!wr_pol && CacheSets.at(index).SetBlocks.at(k).getDirty() &&
                CacheSets.at(index).SetBlocks.at(k).isValid())
        {
            updateWB();
            updateTotMem();
            if (nextLevel != NULL)
            {
                nextLevel->request(addr,1);
            }
        }

        CacheSets.at(index).SetBlocks.at(k).updateTag(tag);
        CacheSets.at(index).SetBlocks.at(k).updateValid(true);
        if (rw && !wr_pol)
        {
            CacheSets.at(index).SetBlocks.at(k).updateDirty(true);
        }
        else
        {
            CacheSets.at(index).SetBlocks.at(k).updateDirty(false);
        }

        if (!rep_pol) { LRU_cnt_update(index,k);}
        else 
        { 
            CacheSets.at(index).SetBlocks.at(k).updateCnt
                ( CacheSets.at(index).getCntSet() +1 );
        }
       // if (rep_pol)
       // {
       //     CacheSets.at(index).SetBlocks.at
    }

    void LRU_cnt_update(int index, int k)
    {
        int init = CacheSets.at(index).SetBlocks.at(k).getCnt();
                
        CacheSets.at(index).SetBlocks.at(k).updateCnt(0);
        for (int i=0;i<assoc;i++)
        {
            if (i==k) {}
            else
            {
                if (CacheSets.at(index).SetBlocks.at(i).getCnt() < init) 
                { 
                    CacheSets.at(index).SetBlocks.at(i).updateCnt(1 + 
                    CacheSets.at(index).SetBlocks.at(i).getCnt());
                }
            }
        }
    }

    bool checkHit(int index, int tag, int rw)
    {
        bool hit;
        //int k;
        for (int i=0;i<assoc;i++)
        {
            hit = (tag == CacheSets.at(index).SetBlocks.at(i).getTag() 
                    && CacheSets.at(index).SetBlocks.at(i).isValid() == true);
            //cout <<"in tag: " << tag << endl;
            //cout <<"tag: " << CacheSets.at(index).SetBlocks.at(i).getTag() << endl; 
            //cout <<"valid: " << CacheSets.at(index).SetBlocks.at(i).isValid()<<endl;
            //cout << hit << endl;
            if (hit)
            {
                if (!rep_pol)
                {
                    LRU_cnt_update(index,i);
                }
                else
                {
                    CacheSets.at(index).SetBlocks.at(i).updateCnt
                        (CacheSets.at(index).SetBlocks.at(i).getCnt() + 1);
                }

                if (rw && !wr_pol)
                {
                    CacheSets.at(index).SetBlocks.at(i).updateDirty(true);
                }

                if (rw && wr_pol)
                {
                    updateTotMem();
                }

                break;
            }

            
        }
        return hit;
    }

    void enterCache(int addr, int index,int tag, int rw)
    {
        int k=assoc;

        //Check if any blocks are empty
        for (int i=0;i<assoc;i++)
        {
            if (CacheSets.at(index).SetBlocks.at(i).isValid() == false)
            {
                k=i;
                break;
            }
        }

        //If it is not present, and any block is empty fill the block
        if (k<assoc)
        {
            updateBlock(addr,index,tag,k,rw);
            //Debug
            //cout << "k: "<< k << endl;
        }

        //If block is not empty, evict one of the blocks.
        if (k==assoc)
        {
            //LRU eviction code
            if (!rep_pol) 
            { 
                for (int i=0;i<assoc;i++)
                {
                    if (CacheSets.at(index).SetBlocks.at(i).getCnt() == (assoc - 1))
                    {
                        k=i;
                        break;
                    }
                }
            }

            //LFU Eviction Code
            else
            {
                int x=CacheSets.at(index).SetBlocks.at(0).getCnt();
                k=0;
                for (int i=1;i<assoc;i++)
                {
                    if (x > CacheSets.at(index).SetBlocks.at(i).getCnt())
                    {
                        x = CacheSets.at(index).SetBlocks.at(i).getCnt();
                        k=i;
                    }
                }
                CacheSets.at(index).updateCntSet(CacheSets.at(index).SetBlocks.at(k).getCnt());
            }

            //Debug
            //cout << "k: "<< k << endl;
            
            updateBlock(addr,index,tag,k,rw);
            }
        }

    void request(int addr,int rw)
    {
        int index = (addr & index_mask)/getBlSize();
        //Mask will only get the required bits,
        //division will effectively shift the bits to the right.
        //Division is better because there is no need of using log if 
        //we use division instead of shirt operations.
        
        int tag = (addr & tag_mask)/(getBlSize() * getNumSet());
        //Masking and then shiftingby (index + block offset) bits

        //cout <<"Index: " << dec << index << " "; 
        //cout << "Ip tag: " << hex << tag <<endl;
        //Check for Hit or Miss
        bool hit = checkHit(index,tag,rw);
        //cout << "Hit: "<<hit<<endl;
        
        //If it is a miss, check the type of transaction. 
        if (!hit)
        {
            if (!rw)
                updaterdMCnt();
            else
                updatewrMCnt();

        //If it is read, enter cache
        //If it is write, check if WBWA and then enterCache.
            if (!rw ||(rw && !getWrPol()))
            {
                updateTotMem();
                if (nextLevel != NULL)
                {
                    //Sending request to the next level
                    nextLevel->request(addr,0);
                }
                enterCache(addr,index,tag,rw);
            }
        //If it is write and WTNA
            else
            {
                updateTotMem();
                if (nextLevel != NULL)
                {
                    nextLevel->request(addr,1);
                }
            }

        }
    
        if (!rw) 
            updaterdCnt();
        else
            updatewrCnt();


    }

    void print_info(const string& x)
    {
        cout << "  ===== Simulator Configuration =====  " << endl;
        cout << "  L1_BLOCKSIZE:                    " << bl_size << endl;
        cout << "  L1_SIZE:                         " << size << endl;
        cout << "  L1_ASSOC:                        " << assoc << endl;
        cout << "  L1_REPLACEMENT_POLICY:           " << rep_pol <<endl;
        cout << "  L1_WRITE_POLICY:                 " << wr_pol << endl;
        cout << "  trace_file:                      " << x <<endl;
        cout << "  ===================================  " << endl <<endl ;
        cout << "===== L1 Contents =====" << endl;
    }

    void print_contents()
    {
        for (int i=0;i<n_set;i++)
        {
            cout << "set" << setw(4) << right << dec << i << ":";
            for (int j=0;j<assoc; j++)
            {
                cout << setw(8) << right << hex << CacheSets.at(i).SetBlocks.at(j).getTag() << " ";

                if (CacheSets.at(i).SetBlocks.at(j).getDirty())
                    cout << "D";
                else
                    cout << " ";
            }
            cout << endl;
        }
    }

    void print_raw_op()
    {
        cout << "  ====== Simulation results (raw) ======  " << endl;
        cout << "  a. number of L1 reads:" << setw(10) << right << dec <<getrdCnt()<<endl;
        cout << "  b. number of L1 read misses:" << setw(10) << right << getrdMCnt()<<endl;
        cout << "  c. number of L1 writes:" << setw(10) << right << getwrCnt()<<endl;
        cout << "  d. number of L1 write misses:" << setw(10) << right << getwrMCnt()<<endl;
        cout << "  e. L1 miss rate:" << setw(10) << right << fixed << setprecision(4) << getMR()<<endl;
        cout << "  f. number of writebacks from L1:" << setw(10) << right << getWB()<<endl;
        cout << "  g. total memory traffic: " <<setw(10) << right << getTotMem() << endl;
    }

    void print_perf_op()
    {
        cout << "  ==== Simulation results (performance) ==== " << endl;
        float miss_p, hit_t;
        miss_p = 20 + 0.5*((float)bl_size/16);
        hit_t = 0.25 + 2.5*((float)size/(512*1024)) + 0.025*((float)bl_size/16) + 0.025*assoc;

        float aat = hit_t + ((float)getMR()*miss_p);

        cout << "  1. average access time:" << setw(10) << right << dec <<aat<<" ns" <<endl;
    }
};

