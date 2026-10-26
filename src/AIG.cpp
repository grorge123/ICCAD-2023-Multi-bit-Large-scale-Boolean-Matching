//
// Created by grorge on 5/15/23.
//

#include "AIG.h"
#include "abcTool.h"
#include "utility.h"
#include "aigtocnf.h"
#include "CNF.h"
#include <sstream>
#include <queue>
#include <unordered_set>
#include <filesystem>


void AIG::parseRaw() {
    stringstream ss;
    ss << raw;
    string tag;
    ss >> tag >> MAXIndex >> inputNum >> latchNum >> outputNum >> andNum;
#ifdef DBG
    if(tag != "aag" || latchNum != 0){
        cout << "ERROR: Can not parse AIG." << endl;
        exit(1);
    }
#endif
    tree.resize(MAXIndex+1);
    for(int i = 0 ; i < inputNum ; i++){
        int inpNum = 0;
        ss >> inpNum;
        tree[inpNum / 2].isInput = true;
        indexMap.push_back(inpNum / 2);
        inputIndexMapInv[inpNum / 2] = indexMap.size() - 1;
        invMap.push_back(inpNum % 2);
        tree[inpNum / 2].exist = true;
    }
    for(int i = 0 ; i < outputNum ; i++){
        int outNum = 0;
        ss >> outNum;

        indexMap.push_back(outNum / 2);
        invMap.push_back(outNum % 2);
        outputIndexMapInv[outNum / 2].push_back(indexMap.size() - 1);
        tree[outNum / 2].isOutput = true;
        tree[outNum / 2].exist = true;
    }
    for(int i = 0 ; i < andNum ; i++){
        int out = 0, l = 0, r = 0;
        ss >> out >> l >> r;
        int idx = out / 2;
        tree[idx].l = l / 2;
        tree[idx].r = r / 2;
        tree[idx].inv[0] = out % 2;
        tree[idx].inv[1] = l % 2;
        tree[idx].inv[2] = r % 2;
        tree[idx].exist = true;
    }
    int order = 0;
    while (!ss.eof() && order < inputNum + outputNum){
        string a, b;
        ss >> a;
        if(a != "c"){
            ss >> b;
        }else{
            break;
        }
        string portName = (b[0] != cirName[0] ? cirName + b : b);
        nameMap[portName] = indexMap[order];
        if(order < inputNum){
            inputNameMapInv[indexMap[order]] = portName;
            inputPort.insert(portName);
        }else{
            outputNameMapInv[indexMap[order]].push_back(portName);
            outputPort.insert(portName);
        }
        orderToName.push_back(portName);
        if(indexMap[order] == 0){
            if(invMap[order]){
                one.push_back(portName);
            }else{
                zero.push_back(portName);
            }
        }else if(order >= inputNum && tree[indexMap[order]].isInput){
            wire.emplace_back(portName, inputNameMapInv[indexMap[order]]);
        }
        order++;
    }

}

void AIG::recursiveFindStrSupport() {
    if(!strSupport.empty())return;
    for(int i = inputNum ; i < outputNum + inputNum; i++){
        int idx = indexMap[i];
        vector<bool> visit;
        if(idx == 0)continue;
        visit.resize(MAXIndex + 1);
        recursiveFindSupport(idx, idx, visit);
    }
}

map<string, set<string>> AIG::satFindSupport() {
    map<string, set<string> > re;
    for(int q = 0 ; q < inputNum + outputNum ; q++){
        string name = fromOrderToName(q);
        set<string> funSupport;
        vector<string> erase;
        AIG copy = *this;
        for(int i = inputNum ; i < inputNum + outputNum ; i++){
            if(fromOrderToName(i) != name){
                erase.emplace_back(fromOrderToName(i));
            }
        }
        copy.erasePort(erase);
        for(int i = 0 ; i < inputNum ; i++){
            CNF miter;
            AIG miterAIG;
            AIG copy1 = copy, copy2 = copy;
            copy1.setConstant(fromOrderToName(i), 0);
            copy2.setConstant(fromOrderToName(i), 1);
            solveMiter(copy1, copy2, miter, miterAIG);
            if(miter.satisfiable){
                funSupport.insert(fromOrderToName(i));
            }
        }
        re[name] = funSupport;
    }
    for(const auto& i : re){
        for(const auto& q : i.second){
            re[q].insert(i.first);
        }
    }
    return re;
}

void AIG::abcFindFunSupport() {
    if(!funSupport.empty())return;
    if(inputNum == 0) {
        funSupport = {};
        return;
    }
    ABCTool abcT(*this);
    funSupport = abcT.funSupport();
//#ifdef DBG
//    auto testFunSupport = satFindSupport();
//    for(const auto& i : testFunSupport){
//        if(i.second != funSupport[i.first]){
//            cout << "[AIG] Error: abc get wrong funSupport." << endl;
//            exit(1);
//        }
//    }
//#endif
}

void AIG::abcFindStrSupport() {
    if(!abcStrSupport.empty())return;
    if(inputNum == 0) {
        funSupport = {};
        return;
    }
    ABCTool abcT(*this);
    abcStrSupport = abcT.strSupport();
}


void AIG::recursiveFindSupport(int output, int now, vector<bool> &visit) {
    if(visit[now])return;
    visit[now] = true;
    if(now == 0)return;
    if(tree[now].isInput){
        for(const auto& outputPort : outputFromIndexToName(output)){
            strSupport[inputFromIndexToName(now)].insert(outputPort);
            strSupport[outputPort].insert(inputFromIndexToName(now));
        }
    }else{
        recursiveFindSupport(output, tree[now].l, visit);
        recursiveFindSupport(output, tree[now].r, visit);
    }
}

vector<bool> AIG::generateOutput(vector<bool> input) {
    vector<bool> output;
    vector<int> signal;
    signal.resize(MAXIndex + 1, -1);
    output.resize(outputNum);
    for(int i = inputNum ; i < outputNum + inputNum ; i++){
        signal[indexMap[i]] = recursiveGenerateOutput(indexMap[i], signal, input);
        output[i - inputNum] = (invMap[i] ? !signal[indexMap[i]] : signal[indexMap[i]]);
    }
    return output;
}

bool AIG::recursiveGenerateOutput(int now, vector<int> &signal, vector<bool> &input) {
    if(tree[now].isInput){
        return (invMap[inputIndexMapInv[now]] ? !input[inputIndexMapInv[now]] : input[inputIndexMapInv[now]]);
    }
    if(tree[now].l == -1 && tree[now].r == -1){
        return false;
    }
    if(signal[tree[now].l] == -1){
        signal[tree[now].l] = recursiveGenerateOutput(tree[now].l, signal, input);
    }
    if(signal[tree[now].r] == -1){
        signal[tree[now].r] = recursiveGenerateOutput(tree[now].r, signal, input);
    }
    bool lvalue = (tree[now].inv[1] ? !signal[tree[now].l] : signal[tree[now].l]);
    bool rvalue = (tree[now].inv[2] ? !signal[tree[now].r] : signal[tree[now].r]);
    return (tree[now].inv[0] ? !(lvalue & rvalue) : (lvalue & rvalue));
}

const set<string> & AIG::getSupport(const string &name, int supType) {
    if(supType == 1){
        abcFindFunSupport();
        return funSupport[name];
    }else if(supType == 2){
        abcFindStrSupport();
        return abcStrSupport[name];
    }else if(supType == 3){
        recursiveFindStrSupport();
        return strSupport[name];
    }
#ifdef DBG
    cout << "[AIG] Error: can not hanle supType!" << endl;
    exit(1);
#endif
}

int AIG::fromNameToIndex(const string &name) {
    return nameMap[name];
}

void AIG::Debug() {
    cout << "DEBUG:"<< (nameMap.find("@z") == nameMap.end()) << " " << nameMap.size() << endl;
    for(auto pair : nameMap){
        cout << "("<< pair.first << ", " << pair.second << ")" << " ";
    }
    cout << endl;
}

void AIG::selfTest() {
    for(const auto& i : nameMap){
        if(fromOrderToIndex(fromNameToOrder(i.first)) != i.second){
            cout << "[AIG] selfTest: nameMap failed." << endl;
            exit(1);
        }
    }
    for(const auto& i : inputNameMapInv){
        if(fromOrderToName(inputFromIndexToOrder(i.first)) != i.second){
            cout << "[AIG] selfTest: inputNameMapInv failed." << endl;
            exit(1);
        }
    }
    for(const auto& i : outputNameMapInv){
        for(const auto& q : i.second){
            bool safe = false;
            for(const auto& k : outputFromIndexToOrder(i.first)){
                if(fromOrderToName(k) == q){
                    safe = true;
                }
            }
            if(!safe){
                cout << "[AIG] selfTest: outputNameMapInv failed." << endl;
                exit(1);
            }
        }
    }
    for(int i = 0 ; i < static_cast<int>(indexMap.size()) ; i++){
        if(fromNameToIndex(fromOrderToName(i)) != indexMap[i]){
            cout << "[AIG] selfTest: index failed." << endl;
            exit(1);
        }
    }
    for(const auto& i : inputIndexMapInv){
        string name = inputFromIndexToName(i.first);
        if(fromNameToOrder(name) != i.second){
            cout << "[AIG] selfTest: inputIndexMapInv failed." << endl;
            exit(1);
        }
    }
    for(const auto& i : outputIndexMapInv){
        for(const auto& q : i.second){
            bool safe = false;
            for(const auto& k : outputFromIndexToName(i.first)){
                if(fromNameToOrder(k) == q){
                    safe = true;
                }
            }
            if(!safe){
                cout << "[AIG] selfTest: outputIndexMapInv failed." << endl;
                exit(1);
            }
        }
    }
    for(int i = 0 ; i < static_cast<int>(orderToName.size()) ; i++){
        bool safe = false;
        if(tree[fromOrderToIndex(i)].isInput && inputFromIndexToName(fromOrderToIndex(i)) == orderToName[i]){
            safe = true;
        }else{
            for(const auto& order : outputFromIndexToName(fromOrderToIndex(i))){
                if(order == orderToName[i]){
                    safe = true;
                }
            }
        }
        if(!safe){
            cout << "[AIG] selfTest: nameMap failed." << endl;
            exit(1);
        }
    }
    //TODO launch it
//    vector<bool> isInput, isOutput;
//    isInput.resize(MAXIndex + 1);
//    isOutput.resize(MAXIndex + 1);
//    for(int i = 0 ; i < inputNum ; i++){
//        isInput[indexMap[i]] = true;
//    }
//    for(int i = inputNum ; i < inputNum + outputNum ; i++){
//        isOutput[indexMap[i]] = true;
//    }
//    for(unsigned int idx = 1 ; idx < tree.size() ; idx++) {
//        if(tree[idx].isInput != isInput[idx]){
//            cout << "[AIG] selfTest: isInput failed." << endl;
//        }
//        if(tree[idx].isOutput != isOutput[idx]){
//            cout << "[AIG] selfTest: isInput failed." << endl;
//        }
//    }


}


int AIG::getInputNum() const {
    return inputNum;
}

const string &AIG::fromOrderToName(int order) {
#ifdef DBG
    if(static_cast<int>(orderToName.size()) <= order || order < 0){
        cout << "[AIG] Error: Can not transfer order(" << order << ") to name." << endl;
        exit(1);
    }
#endif
    return orderToName[order];
}

int AIG::getOutputNum() const {
    return outputNum;
}
int AIG::getMaxNum() const {
    return MAXIndex;
}
int AIG::inputFromIndexToOrder(int idx) {
#ifdef DBG
    if(inputIndexMapInv.find(idx) == inputIndexMapInv.end()){
        cout << "[AIG] Error: Can not transfer Index(" << idx << ") to order." << endl;
        exit(1);
    }
#endif
    return inputIndexMapInv[idx];
}

int AIG::fromOrderToIndex(int order) {
#ifdef DBG
    if(static_cast<int>(indexMap.size()) <= order || order < 0){
        cout << "[AIG] Error: Can not transfer order(" << order << ") to Index." << endl;
        exit(1);
    }
#endif
    return indexMap[order];
}

const string &AIG::getRaw() {
//    if(raw.size() != 0)return raw;
    raw = "aag " + to_string(MAXIndex) + " " + to_string(inputNum) + " 0 " + to_string(outputNum) + " " +
            to_string(andNum) + "\n";
    for(int i = 0 ; i < inputNum + outputNum ; i++){
        raw += (invMap[i] ? to_string(indexMap[i] * 2 + 1) : to_string(indexMap[i] * 2)) + "\n";
    }
    for(int i = 1 ; i <= MAXIndex ; i++){
        Node node = tree[i];
        if(node.exist && !node.isInput){
            raw += to_string(node.inv[0] ? i * 2 + 1 : i * 2) + " " + to_string((node.inv[1] ? node.l * 2 + 1 : node.l * 2))
                    + " " + to_string((node.inv[2] ? node.r * 2 + 1 : node.r * 2)) + "\n";
        }
    }
    for(int q = 0 ; q < inputNum ; q++){
        raw += "i" + to_string(q) + " " + orderToName[q] + "\n";
    }
    for(int q = inputNum ; q < inputNum + outputNum; q++){
        raw += "o" + to_string(q - inputNum) + " " + orderToName[q] + "\n";
    }
    raw += "c\n";

    return raw;
}
void AIG::changeName(const string& oldName, const string& newName) {
    modifyAIG();
    int tmpOrder = -1;
#ifdef DBG
    if(nameMap.find(oldName) == nameMap.end()){
        cout << "[AIG] Error: Can not change " << oldName << " to " << newName << endl;
        exit(1);
    }
#endif
    int tmpIdx = nameMap[oldName];
    nameMap[newName] = tmpIdx;
    if(tree[tmpIdx].isInput && inputNameMapInv[tmpIdx] == oldName){
        inputNameMapInv[tmpIdx] = newName;
        tmpOrder = inputIndexMapInv[tmpIdx];
    }else{
        for(auto &order : outputIndexMapInv[tmpIdx]){
            if(orderToName[order] == oldName){
                tmpOrder = order;
            }
        }
        for(auto &outputName : outputNameMapInv[tmpIdx]){
            if(outputName == oldName){
                outputName = newName;
                break;
            }
        }
    }
    if(inputPort.find(oldName) != inputPort.end()){
        inputPort.erase(oldName);
        inputPort.insert(newName);
    }else{
        outputPort.erase(oldName);
        outputPort.insert(newName);
    }
    orderToName[tmpOrder] = newName;
    nameMap.erase(oldName);
    selfTest();
}
//TODO check wire erase input but output still exist
void AIG::erasePort(const vector<string>& nameList) {
    selfTest();
    if(nameList.empty())return;
    modifyAIG();
    int removeAnd = 0;
    for(auto &name : nameList){
#ifdef DBG
        if(nameMap.find(name) == nameMap.end()){
            cout << "[AIG] ERROR: not found port" << endl;
        }
#endif
        int nodeIdx = nameMap[name];
        if(tree[nodeIdx].exist && tree[nodeIdx].isInput && name == inputNameMapInv[nodeIdx]){
            inputNum--;
            unsigned int inputOrder = min(inputIndexMapInv[nodeIdx], (int)orderToName.size() - 1);
            tree[nodeIdx].exist = false;
            nameMap.erase(name);
            inputNameMapInv.erase(nodeIdx);
            while (orderToName[inputOrder] != name){
#ifdef DBG
                if(inputOrder == 0){
                    cout << "[AIG] ERROR: Can not found input Name" << "(" << name << ")" << "from orderToName" << endl;
                    exit(1);
                }
#endif
                inputOrder--;
            }
            indexMap.erase(indexMap.begin() + inputOrder);
//            inputIndexMapInv.erase(nodeIdx);
            orderToName.erase(orderToName.begin() + inputOrder);
            invMap.erase(invMap.begin() + inputOrder);
            inputPort.erase(name);
        }else{
            outputNum--;
            unsigned int inputOrder = 0;
            for(auto &order : outputIndexMapInv[nodeIdx]){
                inputOrder = max((int)inputOrder, order);
            }
            inputOrder = min((unsigned int)orderToName.size() - 1, inputOrder);
            while (orderToName[inputOrder] != name){
#ifdef DBG
                if(inputOrder == 0){
                    cout << "[AIG] ERROR: Can not found input Name" << "(" << name << ")" << "from orderToName" << endl;
                    exit(1);
                }
#endif
                inputOrder--;
            }
            nameMap.erase(name);
            indexMap.erase(indexMap.begin() + inputOrder);
            orderToName.erase(orderToName.begin() + inputOrder);
            invMap.erase(invMap.begin() + inputOrder);
            for(auto it = outputNameMapInv[nodeIdx].begin() ; it != outputNameMapInv[nodeIdx].end() ; it++){
                if(*it == name){
                    outputNameMapInv[nodeIdx].erase(it);
                    break;
                }
            }
            if(outputNameMapInv[nodeIdx].empty()){
                outputNameMapInv.erase(nodeIdx);
            }
            outputPort.erase(name);
//            for(auto it = outputIndexMapInv[nodeIdx].begin() ; it != outputIndexMapInv[nodeIdx].end() ; it++){
//                if(*it == (int)inputOrder){
//                    outputIndexMapInv[nodeIdx].erase(it);
//                    break;
//                }
//            }
        }
    }
    inputIndexMapInv.clear();
    outputIndexMapInv.clear();
    for(unsigned int order = 0 ; order < indexMap.size() ; order++){
        if((int)order < inputNum){
            inputIndexMapInv[indexMap[order]] = order;
        }else{
            outputIndexMapInv[indexMap[order]].push_back(order);
        }
    }
    vector<bool> exist;
    exist.resize(tree.size());
    queue<int> existQueue;
    int maxIdx = 0;
    for(int order = 0 ; order < inputNum ; order++){
        if(tree[indexMap[order]].exist){
            maxIdx = max(maxIdx, indexMap[order]);
        }
    }
    for(int order = inputNum ; order < inputNum + outputNum ; order++){
        int idx = indexMap[order];
        if(idx != 0){
            maxIdx = max(maxIdx, idx);
            existQueue.push(idx);
        }
    }
    while (!existQueue.empty()){
        int now = existQueue.front();
        maxIdx = max(now, maxIdx);
        existQueue.pop();
        if(exist[now])continue;
        exist[now] = true;
#ifdef DBG
        if(!tree[now].exist && now != 0){
            cout << "[AIG] Error: output port need path " << now << " has been erased." << endl;
            exit(1);
        }
#endif
        if(!tree[now].isInput){
            if(now != 0){
                existQueue.push(tree[now].l);
                existQueue.push(tree[now].r);
            }
        }
    }
    for(unsigned int idx = 1 ; idx < tree.size() ; idx++) {
        tree[idx].isInput = false;
        tree[idx].isOutput = false;
    }
    for(int i = 0 ; i < inputNum ; i++){
        tree[indexMap[i]].isInput = true;
    }
    for(int i = inputNum ; i < inputNum + outputNum ; i++){
        tree[indexMap[i]].isOutput = true;
    }
    for(unsigned int idx = 1 ; idx < tree.size() ; idx++){
        if(tree[idx].isInput)continue;
        if(tree[idx].exist && !exist[idx]){
            removeAnd++;
        }else if( !tree[idx].exist && exist[idx]){
            removeAnd--;
        }
        tree[idx].exist = exist[idx];
    }
    andNum -= removeAnd;
    MAXIndex = maxIdx;
    while (static_cast<int>(tree.size()) > maxIdx + 1){
        tree.pop_back();
    }
    selfTest();
}

const string &AIG::inputFromIndexToName(int index) {
#ifdef DBG
    if(inputNameMapInv.find(index) == inputNameMapInv.end()){
        cout << "[AIG] Error: Can not transfer index (" << index << ") to name." <<  endl;
        exit(1);
    }
#endif
    return inputNameMapInv[index];
}

int AIG::fromNameToOrder(string name) {
    int idx = nameMap[name];
    if(tree[idx].isInput && inputNameMapInv[idx] == name){
        return inputIndexMapInv[idx];
    }else if(tree[idx].isOutput){
        for(auto order: outputIndexMapInv[idx]){
            if(orderToName[order] == name){
                return order;
            }
        }
    }
    if(name == "0")return -1;
#ifdef DBG
    cout << "[AIG] Error: Can not from Name(" << name << ") to find Order." << endl;
    exit(1);
#endif
}

int AIG::fromOrderToIndex(int order) const {
    return indexMap[order];
}

const vector<string> &AIG::getZero() const {
    return zero;
}

const vector<string> &AIG::getOne() const {
    return one;
}

bool AIG::isInput(int idx) {
    return tree[idx].isInput;
}
bool AIG::isOutput(int idx) {
    return tree[idx].isOutput;
}
bool AIG::isInput(const string& name) {
    return inputPort.find(name) != inputPort.end();
}
bool AIG::isOutput(const string& name) {
    return outputPort.find(name) != outputPort.end();
}

bool AIG::portExist(const string& name) {
    return nameMap.find(name) != nameMap.end();
}

const vector<string> &AIG::outputFromIndexToName(int idx) {
#ifdef DBG
    if(outputNameMapInv.find(idx) == outputNameMapInv.end()){
        cout << "[AIG] Error: Can not transfer output index " << idx << " to name."<< endl;
        exit(1);
    }
#endif
    return outputNameMapInv[idx];
}

const vector<int> &AIG::outputFromIndexToOrder(int idx) {
#ifdef DBG
    if(outputIndexMapInv.find(idx) == outputIndexMapInv.end()){
        cout << "[AIG] Error: Can not transfer output index " << idx << " to order." << endl;
        exit(1);
    }
#endif
    return outputIndexMapInv[idx];
}

void AIG::addNegativeOutput() {
    selfTest();
    modifyAIG();
    int oldNum = inputNum + outputNum;
    for(int i = inputNum ; i < oldNum ; i++){
        int newIndex = indexMap[i];
        indexMap.push_back(newIndex);
        int newOrder = indexMap.size() - 1;
        invMap.push_back(!invMap[i]);
        outputIndexMapInv[newIndex].push_back(newOrder);
        string newName = orderToName[i] + '\'';
        nameMap[newName] = indexMap[newOrder];
        outputNameMapInv[indexMap[newOrder]].push_back(newName);
        orderToName.push_back(newName);
        outputPort.insert(newName);
        outputNum++;
        if(indexMap[newOrder] == 0){
            if(invMap[newOrder]){
                one.push_back(newName);
            }else{
                zero.push_back(newName);
            }
        }else if(tree[indexMap[newOrder]].isInput){
            wire.emplace_back(newName, inputNameMapInv[indexMap[newOrder]]);
        }
    }
    selfTest();
}

void AIG::addFloatInput(const vector<string>& name) {
    selfTest();
    modifyAIG();
    indexMap.resize(indexMap.size() + name.size());
    orderToName.resize(orderToName.size() + name.size());
    invMap.resize(invMap.size() + name.size());
    for(auto i = inputNum + outputNum - 1 ; i >= inputNum ; i--){
        int originIdx = indexMap[i];
        indexMap[i + name.size()] = indexMap[i];
        for(auto &order : outputIndexMapInv[originIdx]){
            order += static_cast<int>(name.size());
        }
        orderToName[i + name.size()] = orderToName[i];
        invMap[i + name.size()] = invMap[i];
    }
    for(const auto & i : name){
        int newIdx = ++MAXIndex;
        inputPort.insert(i);
        nameMap[i] = newIdx;
        inputNameMapInv[newIdx] = i;
        indexMap[inputNum] = newIdx; // AIG input order to AIG Node index
        orderToName[inputNum] = i; // AIG input order to verilog name
        invMap[inputNum] = false; // AIG input order if is inverted
        inputNum++;
        Node no;
        no.isInput = no.exist = true;
        tree.push_back(no);
    }
    inputIndexMapInv.clear();
    outputIndexMapInv.clear();
    for(unsigned int order = 0 ; order < indexMap.size() ; order++){
        if((int)order < inputNum){
            inputIndexMapInv[indexMap[order]] = order;
        }else{
            outputIndexMapInv[indexMap[order]].push_back(order);
        }
    }
    selfTest();
}

void AIG::invertGate(const string &name) {
    modifyAIG();
#ifdef DBG
    if(nameMap.find(name) == nameMap.end()){
        cout << "[AIG] Error: invertGate can not find " << name << endl;
        exit(1);
    }
#endif
    int idx = nameMap[name];
    if(tree[idx].isInput && inputNameMapInv[idx] == name){
        for(auto &node : tree){
            if(node.l == idx){
                node.inv[1] = !node.inv[1];
            }
            if(node.r == idx){
                node.inv[2] = !node.inv[2];
            }
        }
        if(isOutput(idx)) {
            for (const auto &order: outputFromIndexToOrder(idx)) {
                invMap[order] = !invMap[order];
            }
        }
    }else{
        for(auto &order: outputIndexMapInv[idx]){
            if(orderToName[order] == name){
                invMap[order] = !invMap[order];
                break;
            }
        }
    }
    selfTest();
}

void AIG::copyOutput(const string &origin, const string &newName, const bool negative) {
    modifyAIG();
#ifdef DBG
    if(nameMap.find(origin) == nameMap.end()){
        cout << "[AIG] Error: Can not change " << origin << " to " << newName << endl;
        exit(1);
    }
#endif
    int originalIdx = nameMap[origin];
    outputPort.insert(newName);
    nameMap[newName] = originalIdx;
    indexMap.push_back(originalIdx);
    int newOrder = indexMap.size() - 1;
    int originalOrder = -1;
    for(const auto &order :outputIndexMapInv[originalIdx]){
        if(orderToName[order] == origin){
            originalOrder = order;
            break;
        }
    }
#ifdef DBG
    if(originalOrder == -1){
        cout << "[AIG] Error: Can not find origin output." << endl;
        exit(1);
    }
#endif

    invMap.push_back((negative ? !invMap[originalOrder] : invMap[originalOrder]));
    outputIndexMapInv[originalIdx].push_back(newOrder);
    nameMap[newName] = indexMap[newOrder];
    outputNameMapInv[indexMap[newOrder]].push_back(newName);
    orderToName.push_back(newName);
    outputNum++;
    if(indexMap[newOrder] == 0){
        if(invMap[newOrder]){
            one.push_back(newName);
        }else{
            zero.push_back(newName);
        }
    }else if(tree[indexMap[newOrder]].isInput){
        wire.emplace_back(newName, inputNameMapInv[indexMap[newOrder]]);
    }
    selfTest();
}

void AIG::exportInput(const string &from, const string &to, bool negative) {
    selfTest();
    modifyAIG();
#ifdef DBG
    if(nameMap.find(from) == nameMap.end() || nameMap.find(to) == nameMap.end()){
        cout << "[AIG] Error: Can not export " << from << " to " << to << endl;
        exit(1);
    }
#endif
    int fromIdx = nameMap[from];
    int toIdx = nameMap[to];
    for(auto &node : tree){
        if(node.l == toIdx){
            //assume input are all positive
#ifdef DBG
            if(tree[fromIdx].inv[0] != 0){
                cout << getRaw() << endl;
                cout << "[AIG]" << fromIdx << "(" << from << ")"<<"break assume;" << endl;
                exit(1);
            }
#endif
            node.l = fromIdx;
            if(negative)node.inv[1] = !node.inv[1];
        }
        if(node.r == toIdx){
#ifdef DBG
            if(tree[fromIdx].inv[1] != 0){
                cout << getRaw() << endl;
                cout << "[AIG]" << fromIdx << "(" << from << ")"<<"break assume;" << endl;
                exit(1);
            }
#endif
            node.r = fromIdx;
            if(negative)node.inv[2] = !node.inv[2];
        }
    }
    if(isOutput(toIdx)){
        for(const auto order : outputFromIndexToOrder(toIdx)){
            string name  = fromOrderToName(order);
            nameMap[name] = fromIdx;
            outputNameMapInv[fromIdx].push_back(name);
            indexMap[order] = fromIdx;
            outputIndexMapInv[fromIdx].push_back(order);
            if(negative)invMap[order] = !invMap[order];
        }
        tree[toIdx].isOutput = false;
        tree[fromIdx].isOutput = true;
    }
    outputIndexMapInv[toIdx].clear();
    outputNameMapInv[toIdx].clear();
    vector<string> ve;
    ve.emplace_back(to);
    erasePort(ve);
    selfTest();
}

void AIG::setConstant(const string &origin, int val) {
    selfTest();
    modifyAIG();
#ifdef DBG
    if(nameMap.find(origin) == nameMap.end()){
        cout << "[AIG] Error: setConstant can not find " << origin << endl;
        exit(1);
    }
#endif
    int idx = nameMap[origin];
#ifdef DBG
    if(idx == 0){
        cout << "[AIG] Error: break assume set constant to constant." << endl;
        exit(1);
    }
#endif
    for(auto &node : tree){
        if(node.l == idx){
            node.l = 0;
            if(val == 0){
                node.inv[1] = node.inv[1];
            }else{
                node.inv[1] = !node.inv[1];
            }
        }
        if(node.r == idx){
            node.r = 0;
            if(val == 0){
                node.inv[2] = node.inv[2];
            }else{
                node.inv[2] = !node.inv[2];
            }
        }
    }
    if(isOutput(idx)){
        for(const auto &name : outputFromIndexToName(idx)){
            int order = fromNameToOrder(name);
            nameMap[name] = 0;
            outputNameMapInv[0].push_back(name);
            indexMap[order] = 0;
            outputIndexMapInv[0].push_back(order);
            if(val == 0){
                invMap[order] = invMap[order];
            }else{
                invMap[order] = !invMap[order];
            }
            tree[0].isOutput = true;
        }
    }
    outputNameMapInv[idx].clear();
    outputIndexMapInv[idx].clear();
    vector<string> ve;
    ve.emplace_back(origin);
    inputPort.erase(origin);
    erasePort(ve);
    selfTest();
}

void AIG::writeToAIGFile(const string &fileName) {
    string aagFileName = fileName.substr(0, fileName.size() - 4) + ".aag";
    aiger *aig = aiger_init();
    FILE *fp = nullptr;
    fp = fopen(aagFileName.c_str(), "w+");
    if(fp == nullptr){
        cout << "[AIG:writeToAIGFile]ERROR: can not write file:" << aagFileName << endl;
        exit(1);
    }
    fputs(getRaw().c_str(), fp);
    fclose(fp);
    const char *err_msg = aiger_open_and_read_from_file(aig, aagFileName.c_str());
    if(err_msg != nullptr){
        cout << "[AIG:writeToAIGFile]ERROR: file:" << fileName << " msg:" << err_msg << endl;
        exit(1);
    }
    fp = fopen(fileName.c_str(), "w");
    if(fp == nullptr){
        cout << "[AIG:writeToAIGFile]ERROR: can not write file:" << fileName << endl;
        exit(1);
    }
    if(!aiger_write_to_file(aig, aiger_binary_mode, fp)){
        cout << "[AIG:writeToAIGFile]ERROR: can not write AIG to file:" << fileName << endl;
        exit(1);
    }
    fclose(fp);
    aiger_reset(aig);
}

void AIG::readFromAIGFile(const string &fileName) {
    aiger *input = aiger_init();
    const char *err_msg = aiger_open_and_read_from_file(input, fileName.c_str());
    if(err_msg != nullptr){
        cout << "[AIG]ERROR: " << err_msg << endl;
        exit(1);
    }
    ifstream ifs;
    ifs.open (fileName.c_str(), ios::binary );
    ifs.seekg (0, ios::end);
    long long length = ifs.tellg();
    if(length < 0){
        cout << "[AIG]ERROR: can not read file:" << fileName << endl;
        exit(1);
    }
    length = length * 8 + 1000;
    {
        // Free the serialization buffer before parseRaw() runs.
        vector<char> tmp(length);
        aiger_mode mode = aiger_ascii_mode;
        int err = aiger_write_to_string(input, mode, tmp.data(), tmp.size());
        if(err != 1){
            cout << "[AIG]ERROR: AIG write error" << endl;
            exit(1);
        }
        raw = tmp.data();
    }
    ifs.close();
    aiger_reset(input);
    parseRaw();
}

bool AIG::portIsNegative(int order) {
    return invMap[order];
}

void AIG::modifyAIG() {
    raw = "";
    funSupport.clear();
    strSupport.clear();
    abcStrSupport.clear();
    posSym.clear();
    negSym.clear();
    symInit = false;
}

void AIG::calSymmetry() {
    if(inputNum == 0) {
        symInit = true;
        return;
    }
    ABCTool abcT(*this);
    auto symPairMap = abcT.calSymmetry();
    for(const auto &output : symPairMap){
        int outputOrder = fromNameToOrder(output.first) - inputNum;
        bool type = false;
        for(const auto &pair : output.second){
            int xiOrder = fromNameToOrder(pair.first);
            int xjOrder = fromNameToOrder(pair.second);
            vector<bool> input1(inputNum);
            vector<bool> input2(inputNum);
            for(auto && it : input1){
                it = random();
            }
            input2 = input1;
            input1[xiOrder] = input1[xjOrder] = false;
            input2[xiOrder] = input2[xjOrder] = true;
            auto genOutput1 = generateOutput(input1);
            auto genOutput2 = generateOutput(input2);
            if(genOutput1[outputOrder] == genOutput2[outputOrder]){
                negSym[output.first].emplace_back(pair);
                type = true;
            }
            input1[xiOrder] = true;
            input1[xjOrder] = false;
            input2[xiOrder] = false;
            input2[xjOrder] = true;
            genOutput1 = generateOutput(input1);
            genOutput2 = generateOutput(input2);
            if(genOutput1[outputOrder] == genOutput2[outputOrder]){
                posSym[output.first].emplace_back(pair);
                type = true;
            }
#ifdef DBG
            if(!type){
                cout << "[AIG] Error: symmetric pair can not found type." << endl;
                exit(1);
            }
#endif
        }
    }
    symInit = true;
}

vector<vector<string>> AIG::getSymGroup(const vector<MP> &sym) {
    unordered_map<string, int> disMap;
    vector<string> disMapInv = {""};
    int cnt = 1;
    for(const auto &i : sym){
        if(disMap.find(i.first) == disMap.end()){
            disMap[i.first] = cnt++;
            disMapInv.emplace_back(i.first);
        }
        if(disMap.find(i.second) == disMap.end()){
            disMap[i.second] = cnt++;
            disMapInv.emplace_back(i.second);
        }
    }
    vector<int> fa;
    for(int i = 0 ; i < cnt ; i++){
        fa.emplace_back(i);
    }
    auto fifa = [&](auto && fifa, int a) -> int{
        if(fa[a] == a)return a;
        return fa[a] = fifa(fifa, fa[a]);
    };
    for(const auto &i : sym){
        if(fifa(fifa, disMap[i.first]) != fifa(fifa, disMap[i.second])){
            fa[fifa(fifa, disMap[i.first])] = fifa(fifa, disMap[i.second]);
        }
    }
    unordered_map<int, set<string> > disjointGroup;
    for(const auto &i : sym){
        if(disjointGroup[fifa(fifa, disMap[i.first])].find(i.first) == disjointGroup[fifa(fifa, disMap[i.first])].end()){
            disjointGroup[fifa(fifa, disMap[i.first])].insert(i.first);
        }
        if(disjointGroup[fifa(fifa, disMap[i.second])].find(i.second) == disjointGroup[fifa(fifa, disMap[i.second])].end()){
            disjointGroup[fifa(fifa, disMap[i.second])].insert(i.second);
        }
    }
    vector<vector<string> > re;
    for(const auto &group : disjointGroup){
        vector<string> tmp;
        for(const auto &i :group.second){
            tmp.emplace_back(i);
        }
        re.emplace_back(std::move(tmp));
    }
    return re;
}


vector<vector<string>> AIG::getNPSym(bool positive) {
    if(!symInit)calSymmetry();
    auto getAllExist = [](unordered_map<string, vector<MP>> Sym){
        auto isExist = [](set<MP> se, const MP& mp) -> bool{
            if(se.find(mp) != se.end() || se.find(MP(mp.second, mp.first)) != se.end())return true;
            return false;
        };
        set<MP> se;
        for(auto it = Sym.begin() ; it != Sym.end() ; ++it){
            if(it == Sym.begin()){
                for(auto &mp : it->second){
                    se.insert(mp);
                }
            }else{
                set<MP> newSe;
                for(auto &mp : it->second){
                    if(isExist(se, mp) && !isExist(newSe, mp)){
                        newSe.insert(mp);
                    }
                }
                se = std::move(newSe);
            }
        }
        vector<MP> re;
        for(const auto & mp: se){
            re.emplace_back(mp);
        }
        return re;
    };
    if(positive){

        return getSymGroup(getAllExist(posSym));
    }else{
        return getSymGroup(getAllExist(negSym));
    }
}

vector<vector<string>> AIG::getNP3Sym(const string& output, bool positive, int fsg = -1, int fsf = -1) {
    const vector<MP> &mpVe = (positive ? posSym[output] : negSym[output]);
    auto re = getSymGroup(mpVe);
    if(fsg != -1){
        for(auto it = re.begin() ; it != re.end() ; it++){
            if(fsg - static_cast<int>(it->size()) >= fsf){
                it = re.erase(it);
            }
        }
    }
    return re;
}


vector<vector<int> > AIG::getSymSign() {
    if(!symInit)calSymmetry();
    vector<vector<int> > re;
    re.resize(inputNum);
    for(auto &i : re){
        i.resize(2 * outputNum);
    }
    for(const auto &symVe : posSym){
        int order = fromNameToOrder(symVe.first) - inputNum;
        for(const auto &pair : symVe.second){
            re[fromNameToOrder(pair.first)][2 * order]++;
            re[fromNameToOrder(pair.second)][2 * order]++;
        }
    }
    for(const auto &symVe : negSym){
        int order = fromNameToOrder(symVe.first) - inputNum;
        for(const auto &pair : symVe.second){
            re[fromNameToOrder(pair.first)][2 * order + 1]++;
            re[fromNameToOrder(pair.second)][2 * order + 1]++;
        }
    }
    return re;
}

void AIG::optimize() {
    cout << "start optimize AIG: " << MAXIndex << " " << andNum << endl;
    startStatistic("optimizeAIG");
    const string folderPath = "./optimizeAIG/";
    if (!filesystem::exists(folderPath)) {
        try {
            filesystem::create_directory(folderPath);
        } catch (const std::exception& e) {
            cout << "[AIG] Error creating folder: " << e.what() << std::endl;
#ifdef DBG
            exit(1);
#endif
        }
    }
    const string origin = cirName + "origin.aig";
    const string final = cirName + "final.aig";
    writeToAIGFile(folderPath + origin);
    for(int i = 0 ; i < outputNum ; i++){
        string abcCmd = "read_aiger " + folderPath + origin + "; cone -O " + to_string(i) + "; write_aiger -s " + folderPath +
                 cirName + to_string(i) + ".aig;";
        exeAbcCmd(abcCmd, "AIG");
    }
    for(int i = 0 ; i < outputNum ; i++){
        int lastNum = INT32_MAX, repeat = 0;
        exeAbcCmd("read_aiger " + folderPath + cirName + to_string(i) + ".aig;", "AIG");
        while (repeat < 2){
            string abcCmd = resyn2;
            exeAbcCmd(abcCmd, "AIG");
            Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
//            cout << lastNum << " " << pNtk->nObjCounts[7] << endl;
            if(lastNum > pNtk->nObjCounts[7]){
                lastNum = pNtk->nObjCounts[7];
                repeat = 0;
            }else{
                repeat++;
            }
        }
        exeAbcCmd("write_aiger -s " + folderPath + cirName + to_string(i) + ".aig;", "AIG");
    }
    exeAbcCmd("read_aiger " + folderPath + cirName + "0" + ".aig;", "AIG");
    for(int i = 1 ; i < outputNum ; i++){
        exeAbcCmd("append " + folderPath + cirName + to_string(i) + ".aig;", "AIG");
    }
    exeAbcCmd("write_aiger -s " + folderPath + final, "AIG");
//    if(!exeAbcCmd("cec " + folderPath + origin, "AIG", "Networks are equivalent")){
//#ifdef DBG
//        cout << "[AIG] Error optimize ntk not equal." << endl;
//        exit(1);
//#endif
//    }else{
        tree.clear();
        nameMap.clear();
        inputNameMapInv.clear();
        outputNameMapInv.clear();
        indexMap.clear();
        inputIndexMapInv.clear();
        outputIndexMapInv.clear();
        orderToName.clear();
        invMap.clear();
        modifyAIG();
        readFromAIGFile(folderPath + final);
//    }
    stopStatistic("optimizeAIG");
    cout << "end optimize AIG: " << MAXIndex << " " << andNum << endl;
}

void solveMiter(AIG &cir1, AIG &cir2, CNF &miter, AIG &miterAIG) {
    string savePath1 = "AIGSave1.aig";
    string savePath2 = "AIGSave2.aig";
    cir1.writeToAIGFile(savePath1);
    cir2.writeToAIGFile(savePath2);

    //TODO optimize abc command
    string abcCmd = "miter " + savePath1 + " " + savePath2 + ";" + simplify + " write_aiger -s miter.aig;";
    exeAbcCmd(abcCmd, "AIG");
    char miterAIGFileName[]{"miter.aig"};
    char miterCNFFileName[]{"miter.cnf"};
    aigtocnf(miterAIGFileName, miterCNFFileName);
    miterAIG = AIG("miter.aig");
    miter = CNF(miterAIG);
    miter.solve();
}
