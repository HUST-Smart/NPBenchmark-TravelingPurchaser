#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <limits>
#include <set>
#include <vector>

#include "../Solver/PbReader.h"
#include "../Solver/TravelingPurchaser.pb.h"


using namespace std;
using namespace pb;


int main(int argc, char *argv[]) {
    enum CheckerFlag {
        IoError = 0x0,
        FormatError = 0x1,
        DemandUnsatisfiedError = 0x2,
        NotEnoughSupplyError = 0x4,
        NonExistingEdgeError = 0x8,
        NotSimplePathError = 0x10
    };

    string inputPath;
    string outputPath;

    if (argc > 1) {
        inputPath = argv[1];
    } else {
        cerr << "input path: " << flush;
        cin >> inputPath;
    }

    if (argc > 2) {
        outputPath = argv[2];
    } else {
        cerr << "output path: " << flush;
        cin >> outputPath;
    }

    pb::TravelingPurchaser::Input input;
    if (!load(inputPath, input)) { return ~CheckerFlag::IoError; }

    pb::TravelingPurchaser::Output output;
    ifstream ifs(outputPath);
    if (!ifs.is_open()) { return ~CheckerFlag::IoError; }
    string submission;
    getline(ifs, submission); // skip the first line.
    ostringstream oss;
    oss << ifs.rdbuf();
    jsonToProtobuf(oss.str(), output);

    if (output.path().empty()) { return ~CheckerFlag::FormatError; }

    int nodeNum = input.graph().nodes().size();
    auto pathBegin = output.path().begin() + ((output.path(0).node() == input.src()) ? 1 : 0);
    if (output.path().rbegin()->node() != input.dst()) {
        output.mutable_path()->Add()->set_node(input.dst());
    }
    for (auto n = pathBegin; n != output.path().end(); ++n) {
        if (n->node() >= nodeNum) { return ~CheckerFlag::FormatError; }
    }

    vector<vector<int>> adjMat(nodeNum, vector<int>(nodeNum, -1));
    if (input.graph().edges().empty()) {
        for (int i = 0; i < nodeNum; ++i) {
            for (int j = 0; j < nodeNum; ++j) {
                adjMat[i][j] = lround(hypot(input.graph().nodes(i).x() - input.graph().nodes(j).x(),
                    input.graph().nodes(i).y() - input.graph().nodes(j).y()));
            }
        }
    } else {
        for (auto e = input.graph().edges().begin(); e != input.graph().edges().end(); ++e) {
            adjMat[e->src()][e->dst()] = e->cost();
        }
    }

    // check solution.
    int error = 0;
    // check constraints.
    int prevNode = input.src();
    vector<bool> isVisited(nodeNum, false);
    vector<int> restDemands(input.demands().begin(), input.demands().end());
    for (auto n = pathBegin; n != output.path().end(); prevNode = n->node(), ++n) {
        if (adjMat[prevNode][n->node()] < 0) { return ~CheckerFlag::NonExistingEdgeError; }
        if (isVisited[n->node()]) {  error |= CheckerFlag::NotSimplePathError; }
        isVisited[n->node()] = true;
        auto &supplies(input.graph().nodes(n->node()).supplies());
        for (auto p = n->quantities().begin(); p != n->quantities().end(); ++p) {
            if (p->second <= 0) { cerr << "[warning] purchasing non-positive quantity." << endl; continue; }
            auto supply = supplies.find(p->first);
            if (supply == supplies.end()) { return ~CheckerFlag::NotEnoughSupplyError; }
            if (supply->second.quantity() < p->second) { error |= CheckerFlag::NotEnoughSupplyError; }
            restDemands[p->first] -= p->second;
        }
    }
    for (auto d = restDemands.begin(); d != restDemands.end(); ++d) {
        if (*d > 0) { error |= CheckerFlag::DemandUnsatisfiedError; }
        if (*d < 0) { cerr << "[warning] purchasing more than demand." << endl; }
    }
    // check objective.
    int cost = 0;
    prevNode = input.src();
    for (auto n = pathBegin; n != output.path().end(); prevNode = n->node(), ++n) {
        // routing cost.
        cost += adjMat[prevNode][n->node()];
        // purchase cost.
        for (auto p = n->quantities().begin(); p != n->quantities().end(); ++p) {
            cost += input.graph().nodes(n->node()).supplies().at(p->first).price() * p->second;
        }
    }

    int returnCode = (error == 0) ? cost : ~error;
    cout << ((error == 0) ? cost : returnCode) << endl;
    return returnCode;
}
