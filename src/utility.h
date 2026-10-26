//
// Created by grorge on 4/14/23.
//

#include <string.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <ctime>
#include <iomanip>

using namespace std;

#ifndef JUDGE_UTILITY_H
#define JUDGE_UTILITY_H

ifstream* readFile(string fileName);

#define debug(...) do{\
    fprintf(stderr,"%s - %d (%s) = ",__PRETTY_FUNCTION__,__LINE__,#__VA_ARGS__);\
    _do(__VA_ARGS__);\
}while(0)
template<typename T>void _do(T &&_x){cerr<<_x<<endl;}
template<typename T,typename ...S> void _do(T &&_x,S &&..._t){cerr<<_x<<" ,";_do(_t...);}
template<typename _a,typename _b> ostream& operator << (ostream &_s,const pair<_a,_b> &_p){return _s<<"("<<_p.X<<","<<_p.Y<<")";}
template<typename It> ostream& _OUTC(ostream &_s,It _ita,It _itb)
{
    _s<<"{";
    for(It _it=_ita;_it!=_itb;_it++)
    {
        _s<<(_it==_ita?"":",")<<*_it;
    }
    _s<<"}";
    return _s;
}
template<typename _a> ostream &operator << (ostream &_s,vector<_a> &_c){return _OUTC(_s,ALL(_c));}
template<typename _a> ostream &operator << (ostream &_s,set<_a> &_c){return _OUTC(_s,ALL(_c));}
template<typename _a,typename _b> ostream &operator << (ostream &_s,map<_a,_b> &_c){return _OUTC(_s,ALL(_c));}
template<typename _t> void pary(_t _a,_t _b){_OUTC(cerr,_a,_b);cerr<<endl;}

string getNowTime();
string formatTime(time_t inputTime);
long long int nowTime();
extern long long  int allOptimize;
extern long long  int lastOptimize;
void startOptimize();
void stopOptimize();
void startStatistic(const string& name);
void stopStatistic(const string& name);
void printStatistic();

#if defined(ABC_NAMESPACE)
namespace ABC_NAMESPACE
{
#elif defined(__cplusplus)
extern "C"
{
#endif

// procedures to start and stop the ABC framework
// (should be called before and after the ABC procedures are called)
void   Abc_Start();
void   Abc_Stop();

// procedures to get the ABC framework and execute commands in it
typedef struct Abc_Frame_t_ Abc_Frame_t;

Abc_Frame_t * Abc_FrameGetGlobalFrame();
int Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );

#if defined(ABC_NAMESPACE)
}
using namespace ABC_NAMESPACE;
#elif defined(__cplusplus)
}
#endif


template <typename T>
int compareSets(const std::set<T>& set1, const std::set<T>& set2) {
    // 獲取較小的 set 大小
    size_t minSize = std::min(set1.size(), set2.size());

    // 建立兩個迭代器
    auto it1 = set1.begin();
    auto it2 = set2.begin();

    // 逐個比較元素
    for (size_t i = 0; i < minSize; ++i) {
        if (*it1 < *it2) {
            return -1;  // set1 比 set2 小
        } else if (*it1 > *it2) {
            return 1;   // set1 比 set2 大
        }
        ++it1;
        ++it2;
    }

    // 如果兩個 set 的前 minSize 個元素相等，則根據大小判斷兩個 set 的關係
    if (set1.size() < set2.size()) {
        return -1;  // set1 比 set2 小
    } else if (set1.size() > set2.size()) {
        return 1;   // set1 比 set2 大
    }

    return 0;       // 兩個 set 相等
}

template <typename T>
int compareVectors(const std::vector<T>& vector1, const std::vector<T>& vector2) {
    // 獲取較小的向量大小
    size_t minSize = std::min(vector1.size(), vector2.size());

    // 逐個比較元素
    for (size_t i = 0; i < minSize; ++i) {
        if (vector1[i] < vector2[i]) {
            return -1;  // vector1 比 vector2 小
        } else if (vector1[i] > vector2[i]) {
            return 1;   // vector1 比 vector2 大
        }
        // 如果元素相等，繼續比較下一個元素
    }

    // 如果兩個向量的前 minSize 個元素相等，則根據大小判斷兩個向量的關係
    if (vector1.size() < vector2.size()) {
        return -1;  // vector1 比 vector2 小
    } else if (vector1.size() > vector2.size()) {
        return 1;   // vector1 比 vector2 大
    }

    return 0;       // 兩個向量相等
}
bool exeAbcCmd(const string &abcCmd, const string &fromFun, const string& keyword = "");
extern Abc_Frame_t * pAbc;
static string resyn3 = "balance; resub; resub -K 6; balance; resub -z; resub -z -K 6; balance; resub -z -K 5; balance;";
static string resyn2 = "balance; rewrite; refactor; balance; rewrite ; rewrite -z ; balance; refactor -z; rewrite -z ;balance;";
static string compress2rs = "balance -l; resub -K 6 -l; rewrite -l; resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l;";
//static string simplify = compress2rs + compress2rs + compress2rs;
static string simplify = "";
#endif //JUDGE_UTILITY_H
