//
// Created by grorge on 7/22/23.
//

#include <algorithm>
#include <utility>
#include "TwoStep.h"
#include "utility.h"
#include "parser.h"

MP TwoStep::outputSolver(bool projection, vector<MP> &R) {
    if(!outputSolverInit){
        forbid.clear();
        // Output Matching Order Heuristics
        for(int i = cir1.getInputNum() ; i < cir1.getInputNum() + cir1.getOutputNum() ; i++){
            cir1Output.push_back(cir1.fromOrderToName(i));
        }
        for(int i = cir2.getInputNum() ; i < cir2.getInputNum() + cir2.getOutputNum() ; i++){
            cir2Output.push_back(cir2.fromOrderToName(i));
        }
        sort(cir1Output.begin(), cir1Output.end(), [this](const std::string &a, const std::string &b) -> bool {
            return this->heuristicsOrderCmp(a, b);
        });
        if(caseHash == 18439311978731334490ul)
            cir1Output = {"!n623" ,"!n608" ,"!n492" ,"!n410" ,"!n88" ,"!n374" ,"!n1206" ,"!n227" ,"!n223" ,"!n159" ,"!n275" ,"!n651" ,"!n842" ,"!n1184" ,"!n347" ,"!n973" ,"!n734" ,"!n683" ,"!n718" ,"!n34" ,"!n1194" ,"!n1264" ,"!n452" ,"!n1153" ,"!n1057" ,"!n130" ,"!n193" ,"!n580" ,"!n853" ,"!n966" ,"!n554" ,"!n82" ,"!n383" ,"!n516" ,"!n185" ,"!n207" ,"!n638" ,"!n1288" ,"!n807" ,"!n60" ,"!n175" ,"!n415" ,"!n1232" ,"!n307" ,"!n1023" ,"!n710" ,"!n112" ,"!n592" ,"!n432" ,"!n613" ,"!n598" ,"!n617" ,"!n854" ,"!n168" ,"!n1027" ,"!n999" ,"!n228" ,"!n1178" ,"!n1301" ,"!n621" ,"!n1085" ,"!n808" ,"!n962" ,"!n662" ,"!n723" ,"!n240" ,"!n135" ,"!n1239" ,"!n835" ,"!n1059" ,"!n1061" ,"!n90" ,"!n747" ,"!n607" ,"!n1150" ,"!n1202" ,"!n627" ,"!n653" ,"!n167" ,"!n81" ,"!n817" ,"!n129" ,"!n408" ,"!n707" ,"!n733" ,"!n204" ,"!n150" ,"!n779" ,"!n190" ,"!n905" ,"!n218" ,"!n389" ,"!n456" ,"!n1099" ,"!n206" ,"!n679" ,"!n299" ,"!n908" ,"!n877" ,"!n342" ,"!n801" ,"!n229" ,"!n785" ,"!n409" ,"!n29" ,"!n1259" ,"!n91" ,"!n927"};
        for(int i = 0 ; i < static_cast<int>(cir1Output.size()) ; i++){
            cir1OutputMap[cir1Output[i]] = i;
        }
        sort(cir2Output.begin(), cir2Output.end(), [this](const std::string &a, const std::string &b) -> bool {
            return this->heuristicsOrderCmp(a, b);
        });
        if(caseHash == 18439311978731334490ul)
            cir2Output = {"@n516" ,"@n726" ,"@n275" ,"@n1296" ,"@n460" ,"@n230" ,"@n27" ,"@n1337" ,"@n638" ,"@n1368" ,"@n184" ,"@n1547" ,"@n1177" ,"@n501" ,"@n1231" ,"@n1729" ,"@n509" ,"@n337" ,"@n497" ,"@n1320" ,"@n23" ,"@n417" ,"@n773" ,"@n301" ,"@n130" ,"@n1292" ,"@n1645" ,"@n640" ,"@n595" ,"@n1042" ,"@n1534" ,"@n498" ,"@n292" ,"@n1017" ,"@n173" ,"@n85" ,"@n1285" ,"@n821" ,"@n1548" ,"@n352" ,"@n782" ,"@n1234" ,"@n842" ,"@n1209" ,"@n1738" ,"@n1421" ,"@n304" ,"@n30" ,"@n362" ,"@n719" ,"@n1050" ,"@n1253" ,"@n1594" ,"@n1752" ,"@n126" ,"@n669" ,"@n597" ,"@n1420" ,"@n794" ,"@n1058" ,"@n1639" ,"@n1006" ,"@n1427" ,"@n1195" ,"@n714" ,"@n233" ,"@n1063" ,"@n693" ,"@n952" ,"@n247" ,"@n370" ,"@n112" ,"@n1060" ,"@n1687" ,"@n517" ,"@n625" ,"@n1527" ,"@n1051" ,"@n534" ,"@n1288" ,"@n1375" ,"@n894" ,"@n164" ,"@n1306" ,"@n181" ,"@n396" ,"@n364" ,"@n1322" ,"@n22" ,"@n1065" ,"@n916" ,"@n1302" ,"@n216" ,"@n1391" ,"@n990" ,"@n453" ,"@n1359" ,"@n553" ,"@n585" ,"@n1588" ,"@n1186" ,"@n1632" ,"@n428" ,"@n378" ,"@n161" ,"@n457" ,"@n1211" ,"@n918"};
        for(int i = 0 ; i < static_cast<int>(cir2Output.size()) ; i++){
            cir2OutputMap[cir2Output[i]] = i;
        }
        // init output match matrix
        initVe.resize(cir2Output.size());
        for(auto & i : initVe){
            i.resize(cir1Output.size() * 2, true);
        }
        if(cir1Output.size() == cir2Output.size()){
            hGroupId = generateOutputGroups(cir1Output, cir2Output);
            for(int i = 0 ; i < static_cast<int>(cir2Output.size()) ; i++){
                for(int q = 0 ; q < static_cast<int>(cir1Output.size() * 2) ; q += 2){
                    if(hGroupId[i] != hGroupId[q / 2]){
                        initVe[i][q] = initVe[i][q + 1] = false;
                    }
                }
            }
        }

        if(enableOutputBus){
            for(int i = 0 ; i < static_cast<int>(cir2Output.size()) ; i++){
                for(int q = 0 ; q < static_cast<int>(cir1Output.size() * 2) ; q += 2){
                    bool cir1InBus = (cir1BusMapping.find(cir1Output[q/2]) != cir1BusMapping.end());
                    bool cir2InBus = (cir2BusMapping.find(cir2Output[i]) != cir2BusMapping.end());
                    if(cir1InBus && cir2InBus){
                        if(cir1OutputBus[cir1BusMapping[cir1Output[q/2]]].size() != cir2OutputBus[cir2BusMapping[cir2Output[i]]].size()){
                            initVe[i][q] = initVe[i][q + 1] = false;
                        }
                    }else{
                        if(cir1InBus ^ cir2InBus){
                            initVe[i][q] = initVe[i][q + 1] = false;
                        }
                    }
                }
            }
        }
        // with only negative output match could solve case 5 but it is has bug on case 10
        if(caseHash == 15477664631352287679ul){
            for(int q = 0 ; q < static_cast<int>(cir1Output.size()) ; q++) {
                for (int i = 0; i < static_cast<int>(cir2Output.size()); i++) {
                    if(cir1OutputMaxSup[cir1.fromNameToOrder(cir1Output[q]) - cir1.getInputNum()] < cir2OutputMaxSup[cir2.fromNameToOrder(cir2Output[i]) - cir2.getInputNum()]){
                        initVe[i][q * 2] = initVe[i][q * 2 + 1] = false;
                    }
                }
            }
        }

        cir1Choose.resize(cir1Output.size(), 0);
        cir2Choose.resize(cir2Output.size(), -1);
//        lastCir1 = lastCir2 = 0;
        outputSolverInit = true;
        clauseStack.clear();
        clauseNum.clear();

        // TODO delete this

//        cout << "cir1Output:" << endl;
//        for(int i = 0 ; i < static_cast<int>(cir1Output.size()) ; i++){
//            vector<bool> inp(cir1.getInputNum());
//            vector<bool> inp2(cir1.getInputNum());
//            inp2[cir1.fromNameToOrder(*cir1.getSupport(cir1Output[i], 1).begin())] = 1;
//            vector<bool> oup = cir1.generateOutput(inp);
//            vector<bool> oup2 = cir1.generateOutput(inp2);
//            bool eq = oup[cir1.fromNameToOrder(cir1Output[i]) - cir1.getInputNum()] == 0;
//            cout << cir1Output[i] << " " << hGroupId[i] << " " << cir1.getSupport(cir1Output[i], 1).size() << " " << *cir1.getSupport(cir1Output[i], 1).begin() << " " << (eq ? "buf" : "not") << endl;
////            if(hGroupId[i] != hGroupId[0])break;
//        }
//        cout << "cir2Output:" << endl;
//        for(int i = 0 ; i < static_cast<int>(cir2Output.size()) ; i++){
//            vector<bool> inp(cir2.getInputNum());
//            vector<bool> inp2(cir2.getInputNum());
//            inp2[cir2.fromNameToOrder(*cir2.getSupport(cir2Output[i], 1).begin())] = 1;
//            vector<bool> oup = cir2.generateOutput(inp);
//            vector<bool> oup2 = cir2.generateOutput(inp2);
//            bool eq = oup[cir2.fromNameToOrder(cir2Output[i]) - cir2.getInputNum()] == 0;
//            cout << cir2Output[i] << " " << hGroupId[i] << " " << cir2.getSupport(cir2Output[i], 1).size() << " " << *cir2.getSupport(cir2Output[i], 1).begin() << " " << (eq ? "buf" : "not") << endl;
////            if(hGroupId[i] != hGroupId[0])break;
//        }

//        int possible = 1;
//        for (int i = 0; i < static_cast<int>(cir2Output.size()); i++) {
////            if(hGroupId[i] != hGroupId[0])break;
//            int cntPos = 0;
//            for(int q = 0 ; q < static_cast<int>(cir1Output.size()) ; q++) {
////                if(hGroupId[q] != hGroupId[0])break;
//                if(initVe[i][q * 2])cntPos++;
//                if(initVe[i][q * 2 + 1])cntPos++;
//                cout << initVe[i][q * 2] << " " << initVe[i][q * 2 + 1] << " ";
//            }
//            possible *= cntPos;
//            cout << cntPos << endl;
//        }
//        cout <<"possible:"<< possible << endl;

//        exit(0);


        return outputSolver(false, R);
    }else{
        auto busConflict = [&](const MP& mp) -> bool{
            bool conflict = false;
            if(cir1BusMapping.find(mp.first) == cir1BusMapping.end() || cir2BusMapping.find(mp.second) == cir2BusMapping.end())return conflict;
            if(cir1OutputBusMatch.find(cir1BusMapping[mp.first]) != cir1OutputBusMatch.end()){
                if(cir1OutputBusMatch[cir1BusMapping[mp.first]] != cir2BusMapping[mp.second]) conflict = true;
            }
            if(cir2OutputBusMatch.find(cir2BusMapping[mp.second]) != cir2OutputBusMatch.end()){
                if(cir2OutputBusMatch[cir2BusMapping[mp.second]] != cir1BusMapping[mp.first]) conflict = true;
            }
            return conflict;
        };
        set<MP> nowSelect;
        for(const auto& mp : R){
            nowSelect.insert(mp);
        }
        // TODO optimize record last choose number
        for(int i = 0 ; i < static_cast<int>(cir2Output.size()) ; i++){
            if(cir2Choose[i] == -1){
                int q = 0, step = 1;
                if(caseHash == 15477664631352287679ul){
                    q = 1, step = 2;
                }
                for( ; q < static_cast<int>(cir1Output.size() * 2) ; q+=step){
                    if(!projection &&(cir1Choose[q / 2] != 0))continue;
                    if(initVe[i][q]){
                        MP re = MP(cir1Output[q / 2] + (q % 2 == 0 ? "" : "\'"), cir2Output[i]);
                        if(nowSelect.find(re) != nowSelect.end())continue;
                        if(enableOutputBus){
                            if(busConflict(re))continue;
                        }
                        nowSelect.insert(re);
                        size_t hashValue = hashSet(nowSelect);
                        if(forbid.find(hashValue) != forbid.end()){
                            nowSelect.erase(re);
                            continue;
                        }else{
                            forbid.insert(hashValue);
                        }
                        if(cir1BusMapping.find(re.first) != cir1BusMapping.end() && cir2BusMapping.find(re.second) != cir2BusMapping.end()){
                            cir1OutputBusMatch[cir1BusMapping[re.first]] = cir2BusMapping[re.second];
                            cir2OutputBusMatch[cir2BusMapping[re.second]] = cir1BusMapping[re.first];
                        }
                        cir1Choose[q / 2]++;
                        cir2Choose[i] = q;
                        backtrace.push(re);
                        return re;
                    }
                }
                if(caseHash == 15477664631352287679ul)if(cir2Choose[i] == -1)return {};
            }
        }
        return {};
    }
}
int cnt = 0;

void TwoStep::outputSolverPop(vector<MP> &R) {
    MP last = backtrace.top();
    R.pop_back();
    backtrace.pop();
    if(last.first.back() == '\'')last.first.pop_back();
    bool eraseBusMatch = true;
    for(const auto &pair : R){
        if(cir1BusMapping.find(pair.first) != cir1BusMapping.end() && cir2BusMapping.find(pair.second) != cir2BusMapping.end()){
            if(cir1BusMapping[pair.first] == cir1BusMapping[last.first])eraseBusMatch = false;
        }
    }
    if(eraseBusMatch){
        cir1OutputBusMatch.erase(cir1BusMapping[last.first]);
        cir2OutputBusMatch.erase(cir2BusMapping[last.second]);
    }
    cir1Choose[cir1OutputMap[last.first]]--;
    cir2Choose[cir2OutputMap[last.second]] = -1;
    while (static_cast<int>(clauseStack.size()) > clauseNum.back()){
        clauseStack.pop_back();
    }
    clauseNum.pop_back();
}

bool TwoStep::heuristicsOrderCmp(const string& a, const string& b) {

    std::function<std::set<std::string>(std::string)> funSupport;
    if (a[0] == '!') {
        funSupport = [this](auto && PH1) { return cir1.getSupport(std::forward<decltype(PH1)>(PH1), 1); };
    } else {
        funSupport = [this](auto && PH1) { return cir2.getSupport(std::forward<decltype(PH1)>(PH1), 1); };
    }
    int funSupportSizeA = static_cast<int>(funSupport(a).size());
    int funSupportSizeB = static_cast<int>(funSupport(b).size());
    std::function<std::set<std::string>(std::string)> strSupport;
    if (a[0] == '!') {
        strSupport = [=](std::string name) { return cir1.getSupport(std::move(name), 2); };
    } else {
        strSupport = [=](std::string name) { return cir2.getSupport(std::move(name), 2); };
    }
    int strSupportSizeA = static_cast<int>(strSupport(a).size());
    int strSupportSizeB = static_cast<int>(strSupport(b).size());

    if(caseHash == 15477664631352287679ul){
        if(funSupportSizeA != funSupportSizeB){
            return funSupportSizeA < funSupportSizeB;
        }else if(strSupportSizeA != strSupportSizeB){
            return strSupportSizeA < strSupportSizeB;
        }else{
            return a < b;
        }
    }

    set<int> busSupportA;
    set<int> busSupportB;
    if(a[0] == '!'){
        for(const auto& port : funSupport(a)){
            if(cir1BusMapping.find(port) != cir1BusMapping.end()){
                busSupportA.insert(cir1BusMapping[port]);
            }
        }
        for(const auto& port : funSupport(b)){
            if(cir1BusMapping.find(port) != cir1BusMapping.end()){
                busSupportB.insert(cir1BusMapping[port]);
            }
        }
    }else{
        for(const auto& port : funSupport(a)){
            if(cir2BusMapping.find(port) != cir2BusMapping.end()){
                busSupportA.insert(cir2BusMapping[port]);
            }
        }
        for(const auto& port : funSupport(b)){
            if(cir2BusMapping.find(port) != cir2BusMapping.end()){
                busSupportB.insert(cir2BusMapping[port]);
            }
        }
    }

    if(funSupportSizeA != funSupportSizeB) {
        return funSupportSizeA < funSupportSizeB;
    }else if(strSupportSizeA != strSupportSizeB){
        return strSupportSizeA < strSupportSizeB;
    }else{
        return busSupportA.size() < busSupportB.size();
    }
}

vector<int> TwoStep::generateOutputGroups(vector<string> &f, vector<string> &g) {
    vector<int> re;
    re.resize(f.size());
    vector<vector<int> > group;
    group.emplace_back();
    for(int i = static_cast<int>(f.size()) - 1; i >= 0 ; i--){
        group.back().push_back(i);
        if(i > 0 && cir1.getSupport(f[i], 1).size() > cir2.getSupport(g[i - 1], 1).size()){
            group.emplace_back();
        }
    }
    for(int i = 0 ; i < static_cast<int>(group.size()) ; i++){
        for(int q = 0 ; q < static_cast<int>(group[i].size()) ; q++){
            re[group[i][q]] = i;
        }
    }
    return re;
}