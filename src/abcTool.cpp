#include "abcTool.h"

//
// Created by grorge on 7/1/23.
//
FILE * ABCTool::stdoutSave(){
    cout.flush();
    FILE *saveStdout = stdout;
    string resultPath = "stdoutOutput.txt";
    stdout = fopen(resultPath.c_str(), "w");
    if(stdout == nullptr){
        stdout = saveStdout;
        cout << "[abcTool]ERROR: can not write file:" << resultPath << endl;
        exit(1);
    }
    return saveStdout;
}
void ABCTool::stdoutRecovery(FILE *saveStdout) {
    fflush(stdout);
    fclose(stdout);
    stdout = saveStdout;
}

void ABCTool::init(string &path){
    // ABC copies the file name, so the buffers only need to outlive the call.
    char cmdName[] = "read_aiger";
    vector<char> pathArr(path.begin(), path.end());
    pathArr.push_back('\0');
    char * argv[] = {cmdName, pathArr.data()};
    FILE * saveStdout = stdoutSave();
    IoCommandReadAiger(pAbc, 2, argv);
    stdoutRecovery(saveStdout);
    mainNtk = Abc_FrameReadNtk(pAbc);
}

void ABCTool::init(AIG &cir) {
    ofstream file;
    string filename = "abcToolAIG.aag";
    string filename2 = "abcToolAIG.aig";
    string content = cir.getRaw();

    file.open(filename);
    if (file.is_open()) {
        aiger *aig = aiger_init();
        FILE *fp = nullptr;
        fp = fopen(filename.c_str(), "w+");
        if(fp == nullptr){
            cout << "[abcTool]ERROR: can not write file:" << filename << endl;
            exit(1);
        }
        fputs(content.c_str(), fp);
        fclose(fp);
        const char *err_msg = aiger_open_and_read_from_file(aig, filename.c_str());
        if(err_msg != nullptr){
            cout << "[abcTool]ERROR: " << err_msg << endl;
            exit(1);
        }
        fp = fopen(filename2.c_str(), "w");
        if(fp == nullptr){
            cout << "[abcTool]ERROR: can not write file:" << filename2 << endl;
            exit(1);
        }
        if(!aiger_write_to_file(aig, aiger_binary_mode, fp)){
            cout << "[abcTool]ERROR: can not write AIG to file:" << filename2 << endl;
            exit(1);
        }
        fclose(fp);
        aiger_reset(aig);
        init(filename2);
    } else {
        cout << "[abcTool] Error: cant not open file." << endl;
        exit(1);
    }
}

unordered_map<string, set<string>> ABCTool::funSupport(){
    Abc_Obj_t * pNode = nullptr, * pNodeCi = nullptr;
    unordered_map<string, set<string> > re;
    int i = 0, v = 0;
    FILE * saveStdout = stdoutSave();
    Vec_Ptr_t * vSuppFun = Sim_ComputeFunSupp( mainNtk, 0 );
    stdoutRecovery(saveStdout);
    Abc_NtkForEachCo( mainNtk, pNode, i )
        Abc_NtkForEachCi( mainNtk, pNodeCi, v ){
            if(Sim_SuppFunHasVar( vSuppFun, i, v ) != 0){
                string outputName = Abc_ObjName(pNode);
                string inputName = Abc_ObjName(pNodeCi);
                if(outputName == inputName)continue;
                re[outputName].insert(inputName);
                re[inputName].insert(outputName);
            }
        }
    if ( vSuppFun )     Sim_UtilInfoFree( vSuppFun );
    return re;
}

unordered_map<string, set<string>> ABCTool::strSupport() {
    Abc_Obj_t * pNode = nullptr, * pNodeCi = nullptr;
    unordered_map<string, set<string> > re;
    int i = 0, v = 0;
    FILE * saveStdout = stdoutSave();
    Vec_Ptr_t * vSuppStr = Sim_ComputeStrSupp( mainNtk );
    stdoutRecovery(saveStdout);
    Abc_NtkForEachCo( mainNtk, pNode, i )
        Abc_NtkForEachCi( mainNtk, pNodeCi, v ){
            if(Sim_SuppStrHasVar( vSuppStr, pNode, v ) != 0){
                string outputName = Abc_ObjName(pNode);
                string inputName = Abc_ObjName(pNodeCi);

                if(outputName == inputName)continue;
                re[outputName].insert(inputName);
                re[inputName].insert(outputName);
            }
        }
    if ( vSuppStr )     Sim_UtilInfoFree( vSuppStr);
    return re;
}
static inline void * Vec_PtrEntryCopy( Vec_Ptr_t * p, int i )
{
    return p->pArray[i];
}
static inline Vec_Int_t * Vec_VecEntryIntCopy( Vec_Vec_t * p, int i )
{
    return (Vec_Int_t *)p->pArray[i];
}
unordered_map<string, vector<pair<string, string>>> ABCTool::calSymmetry() {
    Abc_Obj_t * pNode = nullptr;
    unordered_map<string, vector<pair<string, string>>> re;
    FILE * saveStdout = stdoutSave();
    Sym_Man_t *p = Sim_ComputeTwoVarSymms2(mainNtk, 0);
    vector<string> nameMap(p->nOutputs);
    int d = 0;
    Abc_NtkForEachCo( mainNtk, pNode, d ){
        nameMap[d] = Abc_ObjName(pNode);
    }
    for (int i = 0; i < p->nOutputs; i++ ){
        re[nameMap[i]] = calSymmetryPair((Extra_BitMat_t *) Vec_PtrEntryCopy(p->vMatrSymms, i), Vec_VecEntryIntCopy(p->vSupports, i), p);
    }
    Sym_ManStop( p );
    stdoutRecovery(saveStdout);
    return re;
}

vector<pair<string, string>> ABCTool::calSymmetryPair(Extra_BitMat_t *pMat, Vec_Int_t *vSupport, Sym_Man_t *p) {
    Abc_Obj_t * pNodeCi = nullptr;
    int v = 0;
    vector<string> nameMap(p->nInputs);
    Abc_NtkForEachCi( mainNtk, pNodeCi, v ){
        nameMap[v] = Abc_ObjName(pNodeCi);
    }
    vector<pair<string, string>> re;
    int i = 0, k = 0, Index1 = 0, Index2 = 0;
    Vec_IntForEachEntry( vSupport, i, Index1 )
    Vec_IntForEachEntryStart( vSupport, k, Index2, Index1+1 )
        if ( Extra_BitMatrixLookup1( pMat, i, k ) ){
            printf( "(%d,%d) ", i, k );
            re.emplace_back(nameMap[i], nameMap[k]);
        }
    return re;
}
