#include "largeScale.h"
#include "TwoStep.h"
#include "parser.h"
#include "utility.h"
#include <stdio.h>
#include <stdlib.h>

extern Abc_Frame_t * pAbc;
extern LargeScale lg;
int main (int argc, char *argv[]){
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();
    if(argc != 3){
        cout << "[main] ERROR: wrong argument number!\n";
        exit(1);
    }
    InputStructure input = parseInput(argv[1]);
//    lg = LargeScale(input, argv[2], 1);
//    int lgMatch = lg.start();
//    if(lgMatch == -1){
//        return 0;
//    }
    ts = TwoStep(input, argv[2], 0);
    int tsMatch = ts.start();
    if(tsMatch == -1){
        return 0;
    }
    TwoStep ts1 = TwoStep(input, argv[2], 1);
    tsMatch = ts1.start();
    if(tsMatch == -1){
        return 0;
    }
    TwoStep ts2 = TwoStep(input, argv[2], 2);
    Abc_Stop();
    cout << "Optimize:" << allOptimize / 1000000 << endl;
    printStatistic();
}
