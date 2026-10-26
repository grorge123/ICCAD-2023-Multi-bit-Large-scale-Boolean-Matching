//
// Created by grorge on 7/22/23.
//
#include <utility>
#include "TwoStep.h"
#include "CNF.h"
#include "utility.h"
#include "parser.h"
#include "largeScale.h"

static int ceil_log2(unsigned long long x)
{
    static const unsigned long long t[6] = {
            0xFFFFFFFF00000000ull,
            0x00000000FFFF0000ull,
            0x000000000000FF00ull,
            0x00000000000000F0ull,
            0x000000000000000Cull,
            0x0000000000000002ull
    };

    int y = (((x & (x - 1)) == 0) ? 0 : 1);
    int j = 32;
    int i = 0;

    for (i = 0; i < 6; i++) {
        int k = (((x & t[i]) == 0) ? 0 : j);
        y += k;
        x >>= k;
        j >>= 1;
    }

    return y;
}

bool TwoStep::generateClause(CNF &mappingSpace, AIG &cir1Reduce, AIG &cir2Reduce, const vector<MP> &R,
                             bool outputProjection) {
    int baseLength = (cir1Reduce.getInputNum() + 1) * 2;
    if(caseHash == 3423204434608620955){
        for (int i = 0; i < cir2Reduce.getInputNum(); i++) {
            vector<int> clause;
            clause.clear();
            for (int q = 0; q < baseLength; q++) {
                for (int k = q + 1; k < baseLength; k++) {
                    clause.push_back((i * baseLength + q + 1) * -1);
                    clause.push_back((i * baseLength + k + 1) * -1);
                    mappingSpace.addClause(clause);
                    clause.clear();
                }
            }
        }
        return true;
    }
    //Equation 3
    for (int i = 0; i < cir2Reduce.getInputNum(); i++) {
        vector<int> clause;
        clause.reserve(baseLength);
        for (int q = 0; q < baseLength; q++) {
            clause.push_back(i * baseLength + q + 1);
        }
        mappingSpace.addClause(clause);
//        clause.clear();
//        for (int q = 0; q < baseLength; q++) {
//            for (int k = q + 1; k < baseLength; k++) {
//                clause.push_back((i * baseLength + q + 1) * -1);
//                clause.push_back((i * baseLength + k + 1) * -1);
//                mappingSpace.addClause(clause);
//                clause.clear();
//            }
//        }
    }
    for (int i = 0; i < cir2Reduce.getInputNum(); i++) {
        int lastMax = mappingSpace.maxIdx;
        int g = ceil_log2(baseLength / 2);
        mappingSpace.maxIdx += g;
        for(int k = 0 ; k < baseLength / 2 ; k++){
            for(int q = 0 ; q < 2 ; q++){
                int in = k * 2 + q;
                int x = i * baseLength + in + 1;
                for(int b = 0; b < g ; b++){
                    vector<int> clause{-1 * x};
                    if( k & (1 << b)){
                        clause.push_back(lastMax + b + 1);
                    }else{
                        clause.push_back(-1 * (lastMax + b + 1));
                    }
                    mappingSpace.addClause(clause);
                }
            }
            mappingSpace.addClause({-1 * (i * baseLength + (k * 2) + 1), -1 * (i * baseLength + (k * 2 + 1) + 1)});
        }
    }
    // circuit 1 input must match
    for(int i = 0 ; i < cir1Reduce.getInputNum() ; i++){
        vector<int> clause;
        clause.reserve(2 * cir2Reduce.getInputNum());
        for(int q = 0 ; q < cir2Reduce.getInputNum() ; q++){
            clause.push_back(q * baseLength + i * 2 + 1);
            clause.push_back(q * baseLength + i * 2 + 1 + 1);
        }
        mappingSpace.addClause(clause);
        clause.clear();
    }
    if (cir1Reduce.getInputNum() == cir2Reduce.getInputNum()) {
        //disable match constant
        for (int i = 0; i < cir2Reduce.getInputNum(); i++) {
            vector<int> clause;
            clause.push_back(-1 * (i * baseLength + cir1Reduce.getInputNum() * 2 + 1));
            mappingSpace.addClause(clause);
            clause.clear();
            clause.push_back(-1 * (i * baseLength + cir1Reduce.getInputNum() * 2 + 1 + 1));
            mappingSpace.addClause(clause);
        }
        // disable input projection
//        for (int i = 0; i < cir1Reduce.getInputNum() ; i++) {
//            for(int u = 0 ; u < 2 ; u++){
//                for (int q = 0; q < cir2Reduce.getInputNum(); q++) {
//                    for(int d = 0 ; d < 2 ; d++){
//                        for (int k = q; k < cir2Reduce.getInputNum(); k++) {
//                            if(k == q && d <= u)continue;
//                            vector<int> clause;
//                            clause.push_back((q * baseLength + i * 2 + u + 1) * -1);
//                            clause.push_back((k * baseLength + i * 2 + d + 1) * -1);
//                            mappingSpace.addClause(clause);
//                        }
//                    }
//                }
//            }
//        }
        for(int i = 0 ; i < cir1Reduce.getInputNum() ; i++){
            int lastMax = mappingSpace.maxIdx;
            int g = ceil_log2(cir2Reduce.getInputNum());
            mappingSpace.maxIdx += g;
            int k = 0;
            for(int q = 0 ; q < cir2Reduce.getInputNum() ; q++){
                for(int u = 0 ; u < 2 ; u++){
                    for(int b = 0; b < g ; b++){
                        vector<int> clause{-1 * (q * baseLength + i * 2 + u + 1)};
                        if( k & (1 << b)){
                            clause.push_back(lastMax + b + 1);
                        }else{
                            clause.push_back(-1 * (lastMax + b + 1));
                        }
                        mappingSpace.addClause(clause);
                    }

                }
                mappingSpace.addClause({-1 * (q * baseLength + i * 2 + 1), -1 * (q * baseLength + i * 2 + 1 + 1)});
                k++;
            }
        }
        if(!outputProjection){
            LargeScale inputLg = LargeScale(cir1Reduce, cir2Reduce);
            auto eigenValue = inputLg.calculateEigenvalue();
            for (int i = 0; i < cir1Reduce.getInputNum(); i++) {
                for (int q = 0; q < cir2Reduce.getInputNum(); q++) {
                    if (eigenValue[cir1Reduce.fromOrderToName(i)] == eigenValue[cir2Reduce.fromOrderToName(q)])continue;
                    vector<int> clause;
                    clause.push_back(-1 * (q * baseLength + 2 * i + 1));
                    mappingSpace.addClause(clause);
                    clause.clear();
                    clause.push_back(-1 * (q * baseLength + 2 * i + 1 + 1));
                    mappingSpace.addClause(clause);
                }
            }
        }
//        Output Group Signature Heuristics
        for (int i = 0; i < cir1Reduce.getInputNum(); i++) {
            for (int q = 0; q < cir2Reduce.getInputNum(); q++) {
                bool equal = true;
                auto cir2Sup = cir2Reduce.getSupport(cir2Reduce.fromOrderToName(q), 1);
                auto cir1Sup = cir1Reduce.getSupport(cir1Reduce.fromOrderToName(i), 1);
                for(const auto& supName: cir1Sup){
                    bool find = false;
                    for(const auto& pair : R){
                        auto [gateName, negative] = analysisName(pair.first);
                        if(cir1.cirName + gateName == supName && cir2Sup.find(pair.second) != cir2Sup.end()){
                            find = true;
                            break;
                        }
                    }
                    if(!find){
                        equal = false;
                    }
                }
                if(equal)continue;

                vector<int> clause;
                clause.push_back(-1 * (q * baseLength + 2 * i + 1));
                mappingSpace.addClause(clause);
                clause.clear();
                clause.push_back(-1 * (q * baseLength + 2 * i + 1 + 1));
                mappingSpace.addClause(clause);
            }
        }
    }else if(cir1Reduce.getInputNum() < cir2Reduce.getInputNum()){
        if(caseHash == 18439311978731334490ul || caseHash == 12118911898328955962ul){
//            TODO this have bug in projection
            if(!outputProjection){
                for (int i = 0; i < cir1Reduce.getInputNum(); i++) {
                    for (int q = 0; q < cir2Reduce.getInputNum(); q++) {
                        if (cir1Reduce.getSupport(cir1Reduce.fromOrderToName(i), 1).size() <= cir2Reduce.getSupport(cir2Reduce.fromOrderToName(q), 1).size())continue;
                        vector<int> clause;
                        clause.push_back(-1 * (q * baseLength + 2 * i + 1));
                        mappingSpace.addClause(clause);
                        clause.clear();
                        clause.push_back(-1 * (q * baseLength + 2 * i + 1 + 1));
                        mappingSpace.addClause(clause);
                    }
                }
            }
//            Output Group Signature Heuristics
            for (int i = 0; i < cir1Reduce.getInputNum(); i++) {
                for (int q = 0; q < cir2Reduce.getInputNum(); q++) {
                    bool equal = true;
                    auto cir2Sup = cir2Reduce.getSupport(cir2Reduce.fromOrderToName(q), 1);
                    auto cir1Sup = cir1Reduce.getSupport(cir1Reduce.fromOrderToName(i), 1);
                    for(const auto& supName: cir1Sup){
                        bool find = false;
                        for(const auto& pair : R){
                            auto [gateName, negative] = analysisName(pair.first);
                            if(cir1.cirName + gateName == supName && cir2Sup.find(pair.second) != cir2Sup.end()){
                                find = true;
                                break;
                            }
                        }
                        if(!find){
                            equal = false;
                        }
                    }
                    if(cir1Sup.size() == cir2Sup.size()){
                        if(equal)continue;
                    }else{
                        if(equal && cir2Sup.size() <= 3)continue;
                    }

                    vector<int> clause;
                    clause.push_back(-1 * (q * baseLength + 2 * i + 1));
                    mappingSpace.addClause(clause);
                    clause.clear();
                    clause.push_back(-1 * (q * baseLength + 2 * i + 1 + 1));
                    mappingSpace.addClause(clause);
                }
            }
        }
    }else if(cir1Reduce.getInputNum() > cir2Reduce.getInputNum()){
        return false;
    }
    for(const auto &pair : R){
        auto [gateName, negation] = analysisName(pair.first);
        auto cir1Sup = cir1Reduce.getSupport(cir1.cirName + gateName, 1);
        auto cir2Sup = cir2Reduce.getSupport(pair.second, 1);
        for(auto &cir1Inp : cir1Sup){
            vector<int> clause;
            for(auto &cir2Inp : cir2Sup){
                clause.push_back(-1 * (cir2Reduce.fromNameToOrder(cir2Inp) * baseLength + 2 * cir1Reduce.fromNameToOrder(cir1Inp) + 1));
                clause.push_back(-1 * (cir2Reduce.fromNameToOrder(cir2Inp) * baseLength + 2 * cir1Reduce.fromNameToOrder(cir1Inp) + 1 + 1));
            }
            mappingSpace.addClause(clause);
        }
    }
    for (int i = 0; i < cir1Reduce.getInputNum(); i++) {
        for (int q = 0; q < cir2Reduce.getInputNum(); q++) {
            bool equal = true;
            auto cir2Sup = cir2Reduce.getSupport(cir2Reduce.fromOrderToName(q), 1);
            auto cir1Sup = cir1Reduce.getSupport(cir1Reduce.fromOrderToName(i), 1);
            for(const auto& supName: cir2Sup){
                int find = 0;
                for(const auto& pair : R){
                    auto [gateName, negative] = analysisName(pair.first);
                    if(pair.second == supName && cir1Sup.find(cir1Reduce.cirName + gateName) != cir1Sup.end()){
                        find++;
                    }
                }
                if(find > 3){
                    equal = false;
                }
            }
            if(equal)continue;

            vector<int> clause;
            clause.push_back(-1 * (q * baseLength + 2 * i + 1));
            mappingSpace.addClause(clause);
            clause.clear();
            clause.push_back(-1 * (q * baseLength + 2 * i + 1 + 1));
            mappingSpace.addClause(clause);
        }
    }
//    // symmetric constraint
//    int npO = 0;
//    auto cir1SymSign = cir1Reduce.getSymSign();
//    auto cir2SymSign = cir2Reduce.getSymSign();
//    for(const auto &pair : R){
//        auto cir2SupSZ = cir2Reduce.getSupport(pair.first, 1).size();
//        auto cir1SupSZ = cir1Reduce.getSupport(pair.second, 1).size();
//        if(cir2SupSZ == cir1SupSZ){
//            npO++;
//            for(int i = 0 ; i < cir1Reduce.getInputNum() ; i++){
//                for(int q = 0 ; q < cir2Reduce.getInputNum() ; q++){
//                    auto [gateName, negation] = analysisName(pair.first);
//                    if(cir1SymSign[i][(cir1Reduce.fromNameToOrder(cir1Reduce.cirName + gateName) - cir1Reduce.getInputNum()) * 2] != cir2SymSign[q][(cir2Reduce.fromNameToOrder(pair.second) - cir2Reduce.getInputNum()) * 2] ||
//                    cir1SymSign[i][(cir1Reduce.fromNameToOrder(cir1Reduce.cirName + gateName) - cir1Reduce.getInputNum()) * 2 + 1] != cir2SymSign[q][(cir2Reduce.fromNameToOrder(pair.second) - cir2Reduce.getInputNum()) * 2 + 1]){
//                        vector<int> clause;
//                        clause.push_back(-1 * (q * baseLength + 2 * i + 1));
//                        mappingSpace.addClause(clause);
//                        clause.clear();
//                        clause.push_back(-1 * (q * baseLength + 2 * i + 1 + 1));
//                        mappingSpace.addClause(clause);
//                    }
//                }
//            }
//        }
//    }
//    if(npO == static_cast<int>(R.size())){
//        auto cir1PosGroups = cir1Reduce.getNPSym(true);
//        auto cir2PosGroups = cir2Reduce.getNPSym(true);
//        if(cir1PosGroups.size() != cir2PosGroups.size()){
//            return false;
//        }
//        unordered_map<int, int> cir1CntMap;
//        unordered_map<int, int> cir2CntMap;
//        for(const auto & group : cir1PosGroups){
//            cir1CntMap[static_cast<int>(group.size())]++;
//        }
//        for(const auto & group : cir2PosGroups){
//            cir2CntMap[static_cast<int>(group.size())]++;
//        }
//        for(auto pair : cir1CntMap){
//            if(cir2CntMap[pair.first] != pair.second){
//                return false;
//            }
//        }
//        // TODO this have bug
////        for(const auto &group : cir1PosGroups){
////            vector<vector<int>> clauses;
////            clauses.resize(group.size());
////            for(const auto &group2 : cir2PosGroups){
////                if(group.size() == group2.size()){
////                    for(int i = 0 ; i < static_cast<int>(group.size()) ; i++){
////                        int order = cir1Reduce.fromNameToOrder(group[i]);
////                        int order2 = cir2Reduce.fromNameToOrder(group2[i]);
////                        clauses[i].push_back((order2 * baseLength + 2 * order + 1));
////                        clauses[i].push_back((order2 * baseLength + 2 * order + 1 + 1));
////                    }
////                }
////            }
////            for(const auto &clause : clauses){
////                mappingSpace.addClause(clause);
////            }
////        }
//    }
    return true;
}

void TwoStep::generateBusClause(CNF &mappingSpace, AIG &cir1Reduce, AIG &cir2Reduce, const vector<int> &cir1BusMatch,
                                const vector<int> &cir2BusMatch, const int lastMaxIdx, const vector<MP> &R) {
    int baseLength = (cir1Reduce.getInputNum() + 1) * 2;
    int busBaseLength = static_cast<int>(cir1BusMatch.size());
    int cir1BMS = static_cast<int>(cir1BusMatch.size());
    int cir2BMS = static_cast<int>(cir2BusMatch.size());
    map<int, int> cir1BusMatchInv, cir2BusMatchInv;
    for(int i = 0 ; i < static_cast<int>(cir1BusMatch.size()) ; i++){
        cir1BusMatchInv[cir1BusMatch[i]] = i;
    }
    for(int i = 0 ; i < static_cast<int>(cir2BusMatch.size()) ; i++){
        cir2BusMatchInv[cir2BusMatch[i]] = i;
    }
    vector<vector<string> > cir1InputBusCopy(cir1InputBus.size()), cir2InputBusCopy(cir2InputBus.size());
    for(auto choose : cir1BusMatch){
        for(const auto& port : cir1InputBus[choose]){
            if(cir1Reduce.portExist(port)){
                cir1InputBusCopy[choose].emplace_back(port);
            }
        }
    }
    for(auto choose : cir2BusMatch){
        for(const auto& port : cir2InputBus[choose]){
            if(cir2Reduce.portExist(port)){
                cir2InputBusCopy[choose].emplace_back(port);
            }
        }
    }
    // set 1 to 1
    if(cir1BusMatch.size() > cir2BusMatch.size()){
        for(int i = 0 ; i < cir2BMS ; i++){
            vector<int> clause;
            for(int q = 0 ; q < cir1BMS ; q++){
                clause.emplace_back(lastMaxIdx + busBaseLength * i + q + 1);
            }
            mappingSpace.addClause(clause);
        }
    }else{
        for(int i = 0 ; i < cir1BMS ; i++){
            vector<int> clause;
            for(int q = 0 ; q < cir2BMS ; q++){
                clause.emplace_back(lastMaxIdx + busBaseLength * q + i + 1);
            }
            mappingSpace.addClause(clause);
        }
    }
    // disable bus projection
    for(int i = 0 ; i < cir1BMS ; i++){
        for(int q = 0 ; q < cir2BMS ; q++){
            for(int k = q + 1 ; k < cir2BMS ; k++){
                vector<int> clause;
                clause.emplace_back(-1 * (lastMaxIdx + busBaseLength * q + i + 1));
                clause.emplace_back(-1 * (lastMaxIdx + busBaseLength * k + i + 1));
                mappingSpace.addClause(clause);
            }
        }
    }
    for(int i = 0 ; i < cir2BMS ; i++){
        for(int q = 0 ; q < cir1BMS ; q++){
            for(int k = q + 1 ; k < cir1BMS ; k++){
                vector<int> clause;
                clause.emplace_back(-1 * (lastMaxIdx + busBaseLength * i + q + 1));
                clause.emplace_back(-1 * (lastMaxIdx + busBaseLength * i + k + 1));
                mappingSpace.addClause(clause);
            }
        }
    }
    // A match B => Bus of A Match Bus of B
    for(int i = 0 ; i < cir2BMS ; i++){
        for(int q = 0 ; q < cir1BMS ; q++){
            for(const string& cir1Port : cir1InputBusCopy[cir1BusMatch[q]]){
                for(const string& cir2Port : cir2InputBusCopy[cir2BusMatch[i]]){
                    vector<int> clause;
                    clause.emplace_back(-1 * (baseLength * cir2Reduce.fromNameToOrder(cir2Port) + 2 * cir1Reduce.fromNameToOrder(cir1Port) + 1));
                    clause.emplace_back(lastMaxIdx + busBaseLength * i + q + 1);
                    mappingSpace.addClause(clause);
                    clause.clear();
                    clause.emplace_back(-1 * (baseLength * cir2Reduce.fromNameToOrder(cir2Port) + 2 * cir1Reduce.fromNameToOrder(cir1Port) + 1 + 1));
                    clause.emplace_back(lastMaxIdx + busBaseLength * i + q + 1);
                    mappingSpace.addClause(clause);
                }
            }
        }
    }
    // port of small size bus need to all match other bus
    for(int i = 0 ; i < cir2BMS ; i++) {
        for (int q = 0; q < cir1BMS; q++) {
            int nowBusIdx = -1 * (lastMaxIdx + busBaseLength * i + q + 1);
            //TODO use choose port to decide
            if(cir1InputBusCopy[cir1BusMatch[q]].size() < cir2InputBusCopy[cir2BusMatch[i]].size()){
                for(auto &cir1Name : cir1InputBusCopy[cir1BusMatch[q]]){
                    vector<int> clause = {nowBusIdx};
                    for(auto &cir2Name : cir2InputBusCopy[cir2BusMatch[i]]){
                        clause.emplace_back(baseLength * cir2Reduce.fromNameToOrder(cir2Name) + cir1Reduce.fromNameToOrder(cir1Name) * 2 + 1);
                        clause.emplace_back(baseLength * cir2Reduce.fromNameToOrder(cir2Name) + cir1Reduce.fromNameToOrder(cir1Name) * 2 + 1 + 1);
                    }
                    if(clause.size() > 2){
                        mappingSpace.addClause(clause);
                    }
                }
            }else{
                for(auto &cir2Name : cir2InputBusCopy[cir2BusMatch[i]]){
                    vector<int> clause = {nowBusIdx};
                    for(auto &cir1Name : cir1InputBusCopy[cir1BusMatch[q]]){
                        clause.emplace_back(baseLength * cir2Reduce.fromNameToOrder(cir2Name) + cir1Reduce.fromNameToOrder(cir1Name) * 2 + 1);
                        clause.emplace_back(baseLength * cir2Reduce.fromNameToOrder(cir2Name) + cir1Reduce.fromNameToOrder(cir1Name) * 2 + 1 + 1);
                    }
                    if(clause.size() > 2){
                        mappingSpace.addClause(clause);
                    }
                }
            }
        }
    }
    // pruning by output
    vector<vector<bool>> groupVE(cir2BusMatch.size(), vector<bool>(cir1BusMatch.size(), true));
    for(const auto& pair : R){
        auto [gateName, negation] = analysisName(pair.first);
        auto cir1Sup = cir1.getSupport(cir1.cirName + gateName, 1);
        auto cir2Sup = cir2.getSupport(pair.second, 1);
        set<int> cir1SupBus;
        set<int> cir2SupBus;
        for(const auto &cir1SupPort : cir1Sup){
            if(cir1BusMapping.find(cir1SupPort) != cir1BusMapping.end())
                cir1SupBus.insert(cir1BusMapping[cir1SupPort]);
        }
        for(const auto &cir2SupPort : cir2Sup){
            if(cir2BusMapping.find(cir2SupPort) != cir2BusMapping.end())
                cir2SupBus.insert(cir2BusMapping[cir2SupPort]);
        }
        for(auto supBus : cir1SupBus){
            for(int i = 0 ; i < cir2BMS ; i++){
                if(cir2SupBus.find(cir2BusMatch[i]) == cir2SupBus.end()){
                    groupVE[i][cir1BusMatchInv[supBus]] = false;
                }
            }
        }
    }
    for(int i = 0 ; i < cir1BMS ; i++){
        for(int q = 0 ; q < cir2BMS ; q++){
            if(!groupVE[q][i]){
                vector<int> clause;
                clause.emplace_back(-1 * (lastMaxIdx + busBaseLength * q + i + 1));
                mappingSpace.addClause(clause);
            }
        }
    }
    if(caseHash == 3423204434608620955) {
        for (int i = 0; i < static_cast<int>(cir1BusMatch.size()); i++) {
            for (int q = 0; q < static_cast<int>(cir2BusMatch.size()); q++) {
                if (cir1BusMatch[i] == 0 && cir2BusMatch[q] == 2)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 4 && cir2BusMatch[q] == 5)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 5 && cir2BusMatch[q] == 8)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 6 && cir2BusMatch[q] == 0)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 2 && cir2BusMatch[q] == 1)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 3 && cir2BusMatch[q] == 3)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 1 && cir2BusMatch[q] == 4)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 8 && cir2BusMatch[q] == 6)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
                if (cir1BusMatch[i] == 7 && cir2BusMatch[q] == 7)
                    mappingSpace.addClause({lastMaxIdx + busBaseLength * q + i + 1});
            }
        }
    }
}

vector<MP> TwoStep::inputSolver(vector<MP> &R, bool outputProjection) {
    //TODO define input solver stack recore input solver information
    startStatistic("initCNF");
    set<string> cir1ChoosePort;
    set<string> cir2ChoosePort;
    for (auto pair: R) {
        if (pair.first.back() == '\'') {
            pair.first.pop_back();
        }
        cir1ChoosePort.insert(pair.first);
        if (pair.second.back() == '\'') {
            pair.second.pop_back();
        }
        cir2ChoosePort.insert(pair.second);
    }
    vector<string> cir1Erase, cir2Erase;
    vector<string> cir1Constant, cir2Constant;
    auto eraseNonUsedPort = [](AIG &cir, set<string> &cirChoosePort,
                               vector<string> &cirConstant) -> vector<string> {
        set<string> cirErase;
        for (int i = cir.getInputNum(); i < cir.getInputNum() + cir.getOutputNum(); i++) {
            if (cirChoosePort.find(cir.fromOrderToName(i)) == cirChoosePort.end()) {
                cirErase.insert(cir.fromOrderToName(i));
            }
        }
        for (int order = 0; order < cir.getInputNum(); order++) {
            int eraseFlag = 0;
            for (const string &funSupportPort: cir.getSupport(cir.fromOrderToName(order), 1)) {
                if (cirErase.find(funSupportPort) == cirErase.end()) {
                    eraseFlag = 1;
                    break;
                }
            }
            if (eraseFlag == 0) {
                for (const string &strSupportPort: cir.getSupport(cir.fromOrderToName(order), 2)) {
                    if (cirErase.find(strSupportPort) == cirErase.end()) {
                        eraseFlag = 2;
                        break;
                    }
                }
            }
            if (eraseFlag == 0) {
                cirErase.insert(cir.fromOrderToName(order));
            } else if (eraseFlag == 2) {
                cirConstant.push_back(cir.fromOrderToName(order));
            }
        }
        vector<string> re;
        re.reserve(cirErase.size());
        for (const auto &erase: cirErase) {
            re.push_back(erase);
        }
        return re;
    };
    cir1Erase = eraseNonUsedPort(cir1, cir1ChoosePort, cir1Constant);
    cir2Erase = eraseNonUsedPort(cir2, cir2ChoosePort, cir2Constant);
    AIG cir1Reduce = cir1;
    AIG cir2Reduce = cir2;
    for (const auto &name: cir1Constant) {
        cir1Reduce.setConstant(name, 0);
    }
    for (const auto &name: cir2Constant) {
        cir2Reduce.setConstant(name, 0);
    }
    cir1Reduce.erasePort(cir1Erase);
    cir2Reduce.erasePort(cir2Erase);
    CNF mappingSpace;
    int baseLength = (cir1Reduce.getInputNum() + 1) * 2;
    mappingSpace.maxIdx = cir2Reduce.getInputNum() * baseLength;
    for (int i = 0; i < cir2Reduce.getInputNum(); i++) {
        for (int q = 0; q < cir1Reduce.getInputNum(); q++) {
            // CNF base 1
            mappingSpace.varMap[cir1Reduce.fromOrderToName(q) + "_" + cir2Reduce.fromOrderToName(i)] =
                    i * baseLength + q * 2 + 1;
            mappingSpace.varMap[cir1Reduce.fromOrderToName(q) + '\'' + "_" + cir2Reduce.fromOrderToName(i)] =
                    i * baseLength + q * 2 + 1 + 1;
        }
    }
    for (int i = 0; i < cir2Reduce.getInputNum(); i++) {
        mappingSpace.varMap["0_" + cir2Reduce.fromOrderToName(i)] =
                i * baseLength + cir1Reduce.getInputNum() * 2 + 1;
        mappingSpace.varMap["1_" + cir2Reduce.fromOrderToName(i)] =
                i * baseLength + cir1Reduce.getInputNum() * 2 + 1 + 1;
    }
    stopStatistic("initCNF");
    startStatistic("generateClause");
    if(!generateClause(mappingSpace, cir1Reduce, cir2Reduce, R, outputProjection)){
        if(verbose){
            cout << "w1 < w2: funSup pruning!" << endl;
        }
        return {};
    }
    stopStatistic("generateClause");
    // recover learning clause
    startStatistic("RecoverClause");
    for (const auto &clauses: clauseStack) {
        vector<int> clause;
        clause.reserve(clauses.size());
        for (const auto &key: clauses) {
#ifdef DBG
            if (mappingSpace.varMap.find(key) == mappingSpace.varMap.end()) {
                cout << "[TwoStep] Error: cant not recover " << key << endl;
                exit(1);
            }
#endif
            clause.emplace_back(mappingSpace.varMap[key]);
        }
        mappingSpace.addClause(clause);
        clause.clear();
    }
    stopStatistic("RecoverClause");
    startStatistic("FindInput");
    // set bus constraint
    auto [cir1BusPair, cir2BusPair]= generateBusMatchVector(cir1Reduce, cir2Reduce);
    auto cir1BusMatch = std::move(cir1BusPair.first);
    auto cir1BusCapacity = std::move(cir1BusPair.second);
    auto cir2BusMatch = std::move(cir2BusPair.first);
    auto cir2BusCapacity = std::move(cir2BusPair.second);
    int lastMaxIdx = mappingSpace.maxIdx;
    if(enableInputBus){

        mappingSpace.maxIdx += static_cast<int>(cir1BusMatch.size() * cir2BusMatch.size());
        generateBusClause(mappingSpace, cir1Reduce, cir2Reduce, cir1BusMatch, cir2BusMatch, lastMaxIdx,
                          R);
    }
    vector<MP> mapping;
    CNF miter = generateMiter(R, cir1Reduce, cir2Reduce);
    // TODO add unsatisfied
    if(miter.maxIdx == 0){
        if(verbose){
            cout << "Can not found possible match." << endl;
        }
        return {};
    }
    CNF miterCopy = miter;
    if(caseHash == 1948136321078351779ul) {
        for (int num = 0; num < 10000; num++) {
            miterCopy.solve();
            if (miterCopy.satisfiable) {
                vector<bool> cir1Input(cir1Reduce.getInputNum());
                vector<bool> cir2Input(cir2Reduce.getInputNum());
                for (int i = 0; i < cir1Reduce.getInputNum(); i++) {
                    string name = cir1Reduce.fromOrderToName(i);
#ifdef DBG
                    if (miterCopy.varMap.find(name) == miterCopy.varMap.end()) {
                        cout << "[TwoStepInput] Error: miter can not found cir1 after solve. " << name << endl;
                        exit(1);
                    }
#endif
                    cir1Input[i] = miterCopy.satisfiedInput[miterCopy.varMap[name] - 1];
                }
                for (int i = 0; i < cir2Reduce.getInputNum(); i++) {
                    string name = cir2Reduce.fromOrderToName(i);
#ifdef DBG
                    if (miterCopy.varMap.find(name) == miterCopy.varMap.end()) {
                        cout << "[TwoStepInput] Error: miter can not found cir1 after solve." << endl;
                        exit(1);
                    }
#endif
                    cir2Input[i] = miterCopy.satisfiedInput[miterCopy.varMap[name] - 1];
                }
                auto [cir1NRSet, cir2NRSet] = reduceSpace(mappingSpace, baseLength, cir1Reduce, cir2Reduce, mapping,
                                                          R, cir1Input, cir2Input);
                for (int idx = 0; idx < static_cast<int> (cir1NRSet.size()); idx++) {
                    const auto &cir1NonRedundant = cir1NRSet[idx];
                    const auto &cir2NonRedundant = cir2NRSet[idx];
                    vector<int> clause;
                    for (auto i: cir1NonRedundant) {
                        if (cir1Input[i]) {
                            clause.emplace_back(-1 * miterCopy.varMap[cir1Reduce.fromOrderToName(i)]);
                        } else {
                            clause.emplace_back(miterCopy.varMap[cir1Reduce.fromOrderToName(i)]);
                        }
                    }
                    for (auto i: cir2NonRedundant) {
                        if (cir2Input[i]) {
                            clause.emplace_back(-1 * miterCopy.varMap[cir2Reduce.fromOrderToName(i)]);
                        } else {
                            clause.emplace_back(miterCopy.varMap[cir2Reduce.fromOrderToName(i)]);
                        }
                    }
                    miterCopy.addClause(clause);
                }
            } else {
                break;
            }
        }
    }
    int failTime = 0;
    while (true) {
        startStatistic("solveMapping");
        mapping = solveMapping(mappingSpace, cir1Reduce, cir2Reduce, baseLength);
        stopStatistic("solveMapping");
        if (mapping.empty()){
            if(verbose)
                cout << "No mapping result." << endl;
            break;
        }
        startStatistic("solveMiter");
        auto counter = solveMiter(mapping, miter, cir1Reduce, cir2Reduce);
        stopStatistic("solveMiter");
//#ifdef DBG
//        if(((cir1Reduce.getInputNum() + 1) * 2) * cir2Reduce.getInputNum() + cir1BusMatch.size() * cir2BusMatch.size() != mappingSpace.satisfiedInput.size()){
//            cout << "[TwoStep] Error: SAT return wrong input number!" << " " << endl;
//            exit(1);
//        }
//#endif
        if (counter.first.empty()) {
            if (verbose) {
                cout << "Find Mapping: " << failTime << endl;
                printStatistic();
//                for (const auto &pair: mapping) {
//                    cout << pair.first << " " << pair.second << " ";
//                    auto [gateName, negation] = analysisName(pair.first);
//                    if(cir1BusMapping.find(cir1.cirName + gateName) != cir1BusMapping.end()){
//                        cout << cir1BusMapping[cir1.cirName + gateName] << " " ;
//                    }else{
//                        cout << "Not Found!!" << " " ;
//                    }
//                    if(cir2BusMapping.find(pair.second) != cir2BusMapping.end()){
//                        cout << cir2BusMapping[pair.second];
//                    }else{
//                        cout << "Not Found!!";
//                    }
//                    cout << endl;
//                }
//                cout << "BUS MATCH:" << endl;
//                for(int i = 0 ; i < static_cast<int>(cir1BusMatch.size()) ; i++){
//                    for(int q = 0 ; q < static_cast<int>(cir2BusMatch.size()) ; q++){
//                        if(mappingSpace.satisfiedInput[lastMaxIdx + cir1BusMatch.size() * q + i]){
//                            cout << cir1BusMatch[i] << " " << cir2BusMatch[q] << endl;
//                        }
//                    }
//                }
            }
            stopStatistic("FindInput");
            return mapping;
        }
        failTime++;
//                    if (verbose) {
//                        cout << "It is counterexample size:" << counter.second.size() << endl;
//                        recordMs();
//                    }
        startStatistic("reduceSpace");
        reduceSpace(mappingSpace, baseLength, cir1Reduce, cir2Reduce, mapping,
                    R, counter.first, counter.second);
        stopStatistic("reduceSpace");
        if(nowMs() - startMs > maxRunTime){
            stopStatistic("FindInput");
            return {};
        }
    }
    stopStatistic("FindInput");
    return {};
}

pair<pair<vector<int>, unordered_map<int, int>>, pair<vector<int>, unordered_map<int, int>>>
TwoStep::generateBusMatchVector(AIG &cir1, AIG &cir2) {
    unordered_map<int, int> cir1BusCapacity, cir2BusCapacity;
    if(enableInputBus){
        for(int i = 0 ; i < cir1.getInputNum() ; i++){
            if(cir1BusMapping.find(cir1.fromOrderToName(i)) == cir1BusMapping.end())continue;
            int busIdx = cir1BusMapping[cir1.fromOrderToName(i)];
//            if(verbose)
//                cout << "bus mapping:" << cir1.fromOrderToName(i) << " " << busIdx << endl;
            cir1BusCapacity[busIdx] = static_cast<int>(cir1InputBus[busIdx].size());
            cir1BusCapacity[busIdx] = 10;
        }
        for(int i = 0 ; i < cir2.getInputNum() ; i++){
            if(cir2BusMapping.find(cir2.fromOrderToName(i)) == cir2BusMapping.end())continue;
            int busIdx = cir2BusMapping[cir2.fromOrderToName(i)];
//            if(verbose)
//                cout << "bus mapping:" << cir2.fromOrderToName(i) << " " << busIdx << endl;
            cir2BusCapacity[busIdx] = static_cast<int>(cir2InputBus[busIdx].size());
            cir2BusCapacity[busIdx] = 10;
        }
    }

    vector<int> cir1Ve, cir2Ve;
    for(const auto &match : cir1BusCapacity){
        if(match.second > 0){
            cir1Ve.emplace_back(match.first);
        }
    }
    for(const auto &match : cir2BusCapacity){
        if(match.second > 0){
            cir2Ve.emplace_back(match.first);
        }
    }
    if(verbose){
        cout << "cir1Ve:" << endl;
        for(auto i : cir1Ve){
            cout << i << " " << cir1BusCapacity[i] << endl;
        }
        cout << "cir2Ve:" << endl;
        for(auto i : cir2Ve){
            cout << i << " " << cir2BusCapacity[i] << endl;
        }
    }
    return {{cir1Ve, cir1BusCapacity}, {cir2Ve, cir2BusCapacity}};
}

vector<MP> TwoStep::solveMapping(CNF &mappingSpace, AIG &cir1, AIG &cir2, const int baseLength) {
    vector<MP> re;
    mappingSpace.solve();
    if(mappingSpace.satisfiable){
        for(int i = 0 ; i < cir2.getInputNum() ; i++){
            for(int q = 0 ; q < (cir1.getInputNum() + 1) * 2 ; q++){
                if(mappingSpace.satisfiedInput[i * baseLength + q] > 0){
                    if(q >= cir1.getInputNum() * 2){
                        if(q == cir1.getInputNum() * 2 ){
                            re.emplace_back( "0",cir2.fromOrderToName(i));
                        }else{
                            re.emplace_back( "1",cir2.fromOrderToName(i));
                        }
                    }else{
                        re.emplace_back(cir1.fromOrderToName(q / 2) + (q % 2 == 0 ? "" : "\'"), cir2.fromOrderToName(i));
                    }
                }
            }
        }
    }
    return re;
}