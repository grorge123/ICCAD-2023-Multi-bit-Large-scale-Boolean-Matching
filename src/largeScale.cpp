//
// Created by Han Wen Tsao on 2023/5/19.
//

#include "largeScale.h"
#include "utility.h"
#include "aiger.h"
#include "CNF.h"
#include "aigtocnf.h"
#include <fstream>

using namespace std;
LargeScale lg;
void LargeScale::randomSimulation(int only) {
    int stopNum = 10;
    int noChangeNum = stopNum;
    while (noChangeNum > 0){
        int change = 0;
        pair<vector<bool>, vector<bool>> input = generateInput();
        vector<bool> output1 = cir1.generateOutput(input.first);
        vector<bool> output2 = cir2.generateOutput(input.second);
        vector<vector<bool>> outputVector1;
        vector<vector<bool>> outputVector2;
        vector<vector<bool> > st1Record1, st1Record2;
        if(!only || only == 1)change += cir1.simulationType1(output1, st1Record1);
        if(!only || only == 1)change += cir2.simulationType1(output2, st1Record2);
        dealRecord(st1Record1, st1Record2, false);
        for(unsigned int i = 0 ; i < input.first.size() ; i++){
            vector<bool> tmpInput;
            for(unsigned int q = 0 ; q < input.first.size() ; q++){
                if(i == q){
                    tmpInput.push_back(!input.first[q]);
                }else{
                    tmpInput.push_back(input.first[q]);
                }
            }
            vector<bool> tmpOutput1 = cir1.generateOutput(tmpInput);
            outputVector1.push_back(tmpOutput1);
        }
        for(unsigned int i = 0 ; i < input.second.size() ; i++){
            vector<bool> tmpInput;
            for(unsigned int q = 0 ; q < input.second.size() ; q++){
                if(i == q){
                    tmpInput.push_back(!input.second[q]);
                }else{
                    tmpInput.push_back(input.second[q]);
                }
            }
            vector<bool> tmpOutput2 = cir2.generateOutput(tmpInput);
            outputVector2.push_back(tmpOutput2);
        }
        vector<vector<int> > stRecord1, stRecord2;
        if(!only || only == 2)change += cir1.simulationType2(output1, outputVector1, stRecord1);
        if(!only || only == 2)change += cir2.simulationType2(output2, outputVector2, stRecord2);
        dealRecord(stRecord1, stRecord2, true);
        stRecord1.clear();
        stRecord2.clear();
        if(!only || only == 3)change += cir1.simulationType3(output1, outputVector1, stRecord1);
        if(!only || only == 3)change += cir2.simulationType3(output2, outputVector2, stRecord2);
        dealRecord(stRecord1, stRecord2, false);
        if(change == 0 )noChangeNum--;
        else noChangeNum = stopNum;
    }
}

pair<vector<bool>, vector<bool>>
LargeScale::generateInput() {
    vector<bool> input1, input2, set1, set2;
    input1.resize(cir1.getInputNum());
    set1.resize(cir1.getInputNum());
    input2.resize(cir2.getInputNum());
    set2.resize(cir2.getInputNum());
    std::uniform_int_distribution<> dis(0, 1);
    for(auto cluster1 : cir1.getInputClusters()){
        for(auto cluster2 : cir2.getInputClusters()){
            if(hashTable[cluster1[0]] == hashTable[cluster2[0]]){
                bool genBool = distribution(generator);
                for(const auto& port : cluster1){
                    int order = cir1.inputFromIndexToOrder(cir1.fromNameToIndex(port));
                    input1[order] = genBool;
                    set1[order] = true;
                }
                for(const auto& port : cluster2){
                    int order = cir2.inputFromIndexToOrder(cir2.fromNameToIndex(port));
                    input2[order] = genBool;
                    set2[order] = true;
                }
                break;
            }
        }
    }
    for(unsigned int i = 0 ; i < input1.size() ; i++){
        if(!set1[i])input1[i] = distribution(generator);
    }
    for(unsigned int i = 0 ; i < input2.size() ; i++){
        if(!set2[i])input2[i] = distribution(generator);
    }
    pair<vector<bool>, vector<bool>> result{input1, input2};
    return result;
}

int LargeScale::start() {
    vector<vector<int>> initialInputRecord1, initialOutputRecord1;
    cir1.initialRefinement(initialInputRecord1, initialOutputRecord1);
    vector<vector<int> > initialInputRecord2, initialOutputRecord2;
    cir2.initialRefinement(initialInputRecord2, initialOutputRecord2);
    dealRecord(initialInputRecord1, initialInputRecord2, true);
    dealRecord(initialOutputRecord1, initialOutputRecord2, false);
    int change = 0;
    do{
        vector<vector<set<size_t> > > dependencyInputRecord1, dependencyOutputRecord1;
        vector<vector<set<size_t> > > dependencyInputRecord2, dependencyOutputRecord2;
        change = 0;
        change += cir1.dependencyAnalysis(dependencyInputRecord1, dependencyOutputRecord1, hashTable);
        change += cir2.dependencyAnalysis(dependencyInputRecord2, dependencyOutputRecord2, hashTable);
        dealRecord(dependencyInputRecord1, dependencyInputRecord2, true);
        dealRecord(dependencyOutputRecord1, dependencyOutputRecord2, false);
    } while (change != 0);
    randomSimulation();
    vector<pair<string, string>> inputMatchPair = removeNonSingleton(cir1.getInputClusters(),
                                                                     cir2.getInputClusters());
    vector<pair<string, string>> outputMatchPair = removeNonSingleton(cir1.getOutputClusters(),
                                                                      cir2.getOutputClusters());
    for(auto &port : cir1.getZero()){
        if(port.back() == '\'')continue;
        for(auto &port2 : cir2.getZero()){
            outputMatchPair.emplace_back(port, port2);
        }
    }
    for(auto &port : cir1.getOne()){
        if(port.back() == '\'')continue;
        for(auto &port2 : cir2.getOne()){
            outputMatchPair.emplace_back(port, port2);
        }
    }
    cir1.erasePort(cir1.getZero());
    cir2.erasePort(cir2.getZero());
    cir1.erasePort(cir1.getOne());
    cir2.erasePort(cir2.getOne());

#ifdef INF
    cout << cir1.getZero().size() << ' ' << cir2.getZero().size() << ' ' << cir1.getOne().size() << ' ' << cir2.getOne().size() << endl;
    cout << "First matching:" << inputMatchPair.size() << ' ' << outputMatchPair.size() <<endl;
#endif
    removeNonSupport(inputMatchPair, outputMatchPair);
    removeNonMatch(inputMatchPair, outputMatchPair);
    SAT_Solver(inputMatchPair, outputMatchPair);
    OutputStructure outputStructure;
    int matchNumber = 0;
    for(auto &match : inputMatchPair){
        Group group;
        group.cir1 = match.first;
        group.cir2.push_back(match.second);
        group.invVector.push_back(false);
        outputStructure.inputGroups.push_back(group);
    }
    for(auto &match : outputMatchPair){
        Group group;
        group.cir1 = match.first;
        group.cir2.push_back(match.second);
        group.invVector.push_back(false);
        outputStructure.outputGroups.push_back(group);
        matchNumber += 2;
    }
#ifdef INF
    cout << "Large Scale matching port number: " << matchNumber << "(" << (float)matchNumber / (float)allOutputNumber * 100 << "%)" << endl;
#endif
    parseOutput(outputFilePath, outputStructure, matchNumber);
    if(matchNumber == allOutputNumber)return -1;
    return  matchNumber;
}

map<string, size_t> LargeScale::calculateEigenvalue() {
    vector<vector<int>> initialInputRecord1, initialOutputRecord1;
    cir1.initialRefinement(initialInputRecord1, initialOutputRecord1);
    vector<vector<int> > initialInputRecord2, initialOutputRecord2;
    cir2.initialRefinement(initialInputRecord2, initialOutputRecord2);
    dealRecord(initialInputRecord1, initialInputRecord2, true);
    dealRecord(initialOutputRecord1, initialOutputRecord2, false);
    return hashTable;
}



vector<pair<string, string>>
LargeScale::removeNonSingleton(const vector<vector<string>> &par1, const vector<vector<string>> &par2) {
    vector<pair<string, string> > result;
    vector<string> nonSingleton1;
    map<size_t, string>nonSingleton2;
    for(const auto & i : par1){
        if(i.size() == 1){
            nonSingleton1.push_back(i[0]);
        }
    }
    for(const auto & i : par2){
        if(i.size() == 1){
            nonSingleton2.insert(pair<size_t ,string>(hashTable[i[0]], i[0]));
        }
    }
    for(auto port : nonSingleton1){
        if(port.back() == '\'')continue;
        if(nonSingleton2.find(hashTable[port]) != nonSingleton2.end()){
            result.emplace_back(port, nonSingleton2[hashTable[port]]);
        }
    }
    return result;
}

void LargeScale::removeNonSupport(vector<pair<string, string>> &inputMatch, vector<pair<string, string>> &outputMatch) {
    int change = 0;
    set<string> inputMap1, outputMap1;
    set<string> inputMap2, outputMap2;
    for(auto &match: inputMatch){
        inputMap1.insert(match.first);
        inputMap2.insert(match.second);
    }
    for(auto &match: outputMatch){
        outputMap1.insert(match.first);
        outputMap2.insert(match.second);
    }
    do{
        change = 0;
        for(auto match = inputMatch.begin() ; match != inputMatch.end() ; ){
            bool flag1 = false, flag2 = false;
            auto supportSet1 = cir1.getSupport(match->first, supType);
            for(auto &supportVar : supportSet1){
                if(outputMap1.find(supportVar) != outputMap1.end()){
                    flag1 = true;
                    break;
                }
            }
            auto supportSet2 = cir2.getSupport(match->second, supType);
            for(auto &supportVar : supportSet2){
                if(outputMap2.find(supportVar) != outputMap2.end()){
                    flag2 = true;
                    break;
                }
            }
            if(!(flag1 || flag2)){
                change++;
                inputMap1.erase(match->first);
                inputMap2.erase(match->second);
                match = inputMatch.erase(match);
            }else{
                match++;
            }
        }
        for(auto match = outputMatch.begin() ; match != outputMatch.end() ; ){
            bool flag1 = false, flag2 = false;
            auto supportSet1 = cir1.getSupport(match->first, supType);
            for(auto &supportVar : supportSet1){
                if(inputMap1.find(supportVar) == inputMap1.end()){
                    flag1 = true;
                    break;
                }
            }
            auto supportSet2 = cir2.getSupport(match->second, supType);
            for(auto &supportVar : supportSet2){
                if(inputMap2.find(supportVar) == inputMap2.end()){
                    flag2 = true;
                    break;
                }
            }
            if(flag1 || flag2){
                change++;
                outputMap1.erase(match->first);
                outputMap2.erase(match->second);
                match = outputMatch.erase(match);

            }else{
                match++;
            }
        }
    } while (change != 0);
}

void LargeScale::SAT_Solver(vector<pair<string, string> > &inputMatch, vector<pair<string, string> > &outputMatch) {
    if(outputMatch.empty())return;


    AIG newAIG = cir2;
    for(auto &match : inputMatch){
        newAIG.changeName(match.second, match.first);
    }
    for(auto &match : outputMatch){
        newAIG.changeName(match.second, match.first);
    }
    CNF miter;
    AIG miterAIG;
    solveMiter(cir1, newAIG, miter, miterAIG);
    if(miter.satisfiable){
        // TODO Not test
        cout << "Not Implement" << endl;
        while (true);
        AIG miterAIG("miterAIG.aig");
        ifstream pf("miterAIG.cnf");
        string symbol;
        map<int, string> CNFToAIG;
        while (pf >> symbol){
            if(symbol != "c")break;
            int AIGIdx = 0, CNFIdx = 0;
            string tmp;
            pf >> AIGIdx >> tmp >> CNFIdx;
            //TODO deal AIGIdx == 0 or 1 and new output format
            CNFToAIG[CNFIdx] = miterAIG.inputFromIndexToName(AIGIdx / 2);
        }
        pf.close();
        vector<bool> inputVector1, inputVector2;
        inputVector1.resize(cir1.getInputNum());
        inputVector2.resize(cir1.getInputNum());
        for(int i = 0 ; i < static_cast<int>(miter.satisfiedInput.size()) ; i++){
            inputVector1[cir1.inputFromIndexToOrder(cir1.fromNameToIndex(CNFToAIG[i + 1]))] = miter.satisfiedInput[i];
            inputVector2[cir2.inputFromIndexToOrder(cir2.fromNameToIndex(CNFToAIG[i + 1]))] = miter.satisfiedInput[i];
        }
        vector<bool> outputVector1 = cir1.generateOutput(inputVector1);
        vector<bool> outputVector2 = cir2.generateOutput(inputVector2);
        vector<string> removeVector1, removeVector2;
        for(unsigned int i = 0 ; i < outputVector1.size() ; i++){
            if(outputVector1[i] != outputVector2[i]){
                removeVector1.push_back(cir1.fromOrderToName(cir1.getInputNum() + i));
                removeVector2.push_back(cir2.fromOrderToName(cir2.getInputNum() + i));
            }
        }
        for(auto remove : removeVector1){
            for(auto it = outputMatch.begin() ; it != outputMatch.end();){
                if(remove == it->first){
                    outputMatch.erase(it);
                    break;
                }
            }
        }
        cir1.erasePort(removeVector1);
        cir2.erasePort(removeVector2);
        return SAT_Solver(inputMatch, outputMatch);
    }else{
        return;
    }
}

void LargeScale::removeNonMatch(const vector<pair<string, string>> &inputMatch,
                                const vector<pair<string, string>> &outputMatch) {
    set<string> inputSet1, outputSet1;
    set<string> inputSet2, outputSet2;
    for(auto &matchPair : inputMatch){
        inputSet1.insert(matchPair.first);
        inputSet2.insert(matchPair.second);
    }
    for(auto &matchPair : outputMatch){
        outputSet1.insert(matchPair.first);
        outputSet2.insert(matchPair.second);
    }
    cir1.removeNonMatch(inputSet1, outputSet1);
    cir2.removeNonMatch(inputSet2, outputSet2);
}




