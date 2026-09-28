//
// Created by grorge on 5/15/23.
//

#ifndef MULTI_BIT_LARGE_SCALE_BOOLEAN_MATCHING_AIG_H
#define MULTI_BIT_LARGE_SCALE_BOOLEAN_MATCHING_AIG_H
#include "aiger.h"
#include <cstring>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <iostream>
#include <utility>
#include <vector>
#include <fstream>

using namespace std;
class CNF;
class AIG {
    typedef pair<string,string> MP;
    struct Node{
        bool inv[3] = {}; // self, l, r if invert
        int l = -1, r = -1;
        bool isInput = false;
        bool isOutput = false;
        bool exist = false;
    };
    vector<Node> tree;
    string raw;

private:
    int MAXIndex, inputNum, outputNum, latchNum, andNum;
    unordered_map<string, int> nameMap; // verilog input name to AIG Node index
    unordered_map<int, string> inputNameMapInv; // AIG Node index to verilog input name
    unordered_map<int, vector<string> > outputNameMapInv; // AIG Node index to verilog input name
    vector<int> indexMap; // AIG input order to AIG Node index
    unordered_map<int, int> inputIndexMapInv; // AIG Node index to AIG input order
    unordered_map<int, vector<int> > outputIndexMapInv; // AIG Node index to AIG input order
    vector<string> orderToName; // AIG input order to verilog name
                                // verilog input name to order
    vector<bool> invMap; // AIG input order if is inverted
    unordered_set<string> inputPort;
    unordered_set<string> outputPort;
    unordered_map<string , set<string> > strSupport; // input Name to strSupport set search by recursive
    unordered_map<string , set<string> > abcStrSupport; // input Name to strSupport set search by abc
    unordered_map<string , set<string> > funSupport; // input Name to funSupport set
    unordered_map<string , vector<MP> > posSym;
    unordered_map<string , vector<MP> > negSym;
    bool symInit = false;
    void parseRaw();
    void recursiveFindSupport(int output, int now, vector<bool> &visit);
    map<string, set<string>> satFindSupport();
    bool recursiveGenerateOutput(int now, vector<int>& signal, vector<bool>& input);
    vector<string> zero, one;
    vector<pair<string,string>> wire; // <output name, input name>
    void recursiveFindStrSupport();
    void abcFindFunSupport();
    void abcFindStrSupport();
public:
    string cirName;
    AIG(){};
    AIG(const string &name, string cirName = "") : cirName(std::move(cirName)){
        readFromAIGFile(name);
    }
    const string &inputFromIndexToName(int index);
    const vector<string> &outputFromIndexToName(int idx);
    const string &fromOrderToName(int order);
    int fromOrderToIndex(int order) const;
    int fromNameToIndex(const string &name);
    int inputFromIndexToOrder(int idx);
    const vector<int> &outputFromIndexToOrder(int idx);
    int fromOrderToIndex(int order);
    int fromNameToOrder(string name);
    int getInputNum() const;
    int getOutputNum() const;
    int getMaxNum() const;
    bool isInput(int idx);
    bool isOutput(int idx);
    bool isInput(const string& name);
    bool isOutput(const string& name);
    bool portExist(const string& name);
    bool portIsNegative(int order);
    const set<string> & getSupport(const string &name, int supType); // 0: funSuppose 1: abcStrSuppose 2: recurStrSuppose
    vector<bool> generateOutput(vector<bool> input);
    const string &getRaw();
    void changeName(const string& oldName, const string& newName);
    void erasePort(const vector<string>& nameList);
    const vector<string> &getZero() const;
    const vector<string> &getOne() const;
    void addNegativeOutput();
    void addFloatInput(const vector<string>& name);
    void invertGate(const string &name);
    void copyOutput(const string &origin, const string &newName, bool negative);
    void exportInput(const string &from, const string &to, bool negative);
    void setConstant(const string &origin, int val);
    void writeToAIGFile(const string &fileName);
    void readFromAIGFile(const string &fileName);
    void modifyAIG();
    void Debug();
    void selfTest();
    void calSymmetry();
    void optimize();
    vector<vector<string>> getSymGroup(const vector<MP> &sym);
    vector<vector<string>> getNP3Sym(const string& output, bool positive, int fsg, int fsf);
    vector<vector<string>> getNPSym(bool positive);
    vector<vector<int> > getSymSign();
};

void solveMiter(AIG &cir1, AIG &cir2, CNF &miter, AIG &miterAIG);

#endif //MULTI_BIT_LARGE_SCALE_BOOLEAN_MATCHING_AIG_H
