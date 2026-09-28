//
// Created by grorge on 6/28/23.
//
#include <sstream>
#include "CNF.h"
#include "aigtocnf.h"
#include "utility.h"
void CNF::readFromAIG(AIG &aig) {
    string fileName = "tmpCNFConstructor";
    string aigContent = aig.getRaw();
    ofstream ofs;
    ofs.open(fileName + ".aig");
    ofs << aigContent;
    ofs.close();
    aigtocnf((fileName + ".aig").c_str(), (fileName + ".cnf").c_str());
    // read another info
    readFromFile(fileName + ".cnf");

    ifstream ifs;
    ifs.open(fileName + ".cnf");
    string input;

    inv.resize(maxIdx);
    while (getline(ifs, input)){
        if(input[0] != 'c')break;
        stringstream ss;
        ss.clear();
        ss << input;
        ss >> input;
        int aigIdx = -1, cnfIdx = -1;
        string nodeName;
        ss >> nodeName >> aigIdx >> input >> cnfIdx;
        aigIdx /= 2;
        if(nodeName != "NaN"){
            int order = aig.fromNameToOrder(nodeName);
            if(aig.portIsNegative(order)){
                inv[cnfIdx];
            }
            varMap.insert(pair<string, int> (nodeName, cnfIdx));
        }
    }
    ifs.close();

}


void CNF::readFromFile(const string& inputPath) {
    ifstream ifs;
    ifs.open(inputPath);
    string input;
    while (getline(ifs, input)){
        if(input[0] == 'c')continue;
        stringstream ss;
        ss << input;
        if(input[0] == 'p'){
            ss >> input; // read p
            ss >> input; // read cnf
            ss >> maxIdx;
            int clauseNum;
            ss >> clauseNum;
        }else{
            vector<int> ve;
            int now;
            while (ss >> now){
                if(now == 0) break;
                ve.push_back(now);
            }
            clauses.push_back(ve);
        }
    }
    ifs.close();
}

string CNF::getRaw() {
    string raw;
    string saveSpace(10, ' ');
    raw += "p cnf " + to_string(maxIdx) + " " + to_string(clauses.size()) + saveSpace + '\n';
    for(const auto& clause : clauses){
        for(auto var : clause){
            raw += to_string(var) + " ";
        }
        raw += "0\n";
    }
    return raw;
}

void CNF::combine(const CNF &a) {
    change = 2;
    for(const auto& mapTable : a.varMap){
#ifdef DBG
        if(varMap.find(mapTable.first) != varMap.end()){
            cout << "[CNF] ERROR: Multiple mapping" << endl;
        }
#endif
        varMap[mapTable.first] = mapTable.second + maxIdx;
    }
    for(const auto& clause : clauses){
        vector<int> ve;
        for(auto var : clause){
            int newVar = (abs(var) + maxIdx ) * (var > 0 ? 1 : -1);
            ve.push_back(newVar);
        }
        clauses.push_back(ve);
    }
    maxIdx += a.maxIdx;
}

bool CNF::solve() {
    //TODO need optimize
    if(change == 0){
        return satisfiable;
    }else if(change == 2){
        delete solver;
        solver = new CaDiCaL::Solver();
        lastClauses = 0;
    }
    int setCnt = static_cast<int>(clauses.size()) - lastClauses;
    startStatistic("addClause");
    for(auto it = clauses.rbegin() ; it != clauses.rend() ; ++it){
        for(int number: *it){
            solver->add(number);
        }
        solver->add(0);
        --setCnt;
        if(!setCnt)break;
    }
    stopStatistic("addClause");
    startStatistic("CNF_optimize");
    solver->optimize(9);
    stopStatistic("CNF_optimize");
    startStatistic("solveCNF");
    int res = solver->solve();
    stopStatistic("solveCNF");
    satisfiedInput.clear();
    if (res == 10) {
        satisfiable = true;
        for (int i = 1; i <= maxIdx; ++i) {
            satisfiedInput.push_back(solver->val(i) > 0);
        }
    } else if (res == 20) {
        satisfiable = false;
    } else {
        cout << "[CNF] Error: The solver was unable to determine satisfiability." << endl;
    }
    if(change == 3){
        change = 1;
    }else{
        change = 0;
    }
    lastClauses = static_cast<int>(clauses.size());
    return satisfiable;
}

list<vector<int>>::iterator CNF::addClause(const vector<int> &clause) {
    if(change == 0)
        change = 1;
    clauses.emplace_back(clause);
    auto it = prev(clauses.end());
    return it;
}

const list<vector<int>> &CNF::getClauses() const {
    return clauses;
}

void CNF::eraseClause(list<vector<int>>::iterator it) {
    change = 2;
    clauses.erase(it);
}

const vector<int> &CNF::getClause(list<vector<int>>::iterator it) {
    return *it;
}

void CNF::addAssume(int lit) {
    if(change != 2){
        change = 3;
    }
    startStatistic("addAssume");
    solver->assume(lit);
    startStatistic("addAssume");
}

void CNF::copy(CNF &other) {
    other.change = change;
    other.lastClauses = lastClauses;
    other.clauses = clauses;
    other.satisfiable = satisfiable;
    other.satisfiedInput = satisfiedInput;
    other.inv = inv;
    other.varMap = varMap;
    other.maxIdx = maxIdx;
    solver->copy(*other.solver);
}

int CNF::getLastClauses() const {
    return max(lastClauses, static_cast<int>(clauses.size()));
}
