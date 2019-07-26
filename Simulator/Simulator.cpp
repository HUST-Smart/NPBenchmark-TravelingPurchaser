#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <random>

#include <cstring>

#include "Simulator.h"
#include "ThreadPool.h"
#include "ShortestPassWithMustPassNodes.pb.h"


using namespace std;


namespace szx {

// EXTEND[szx][5]: read it from InstanceList.txt.
static const vector<String> instList({
    "ssppmpn000.n200e990p20",
    "ssppmpn001.n200e990p40",
    "ssppmpn002.n300e995p40",
    "ssppmpn003.n500e2986p100",
    "ssppmpn004.n1000e3995p100",
    "ssppmpn005.n2000e39832p100",
    "ssppmpn006.n300e844p20",
    "ssppmpn007.n500e2000p22",
    "ssppmpn008.n2000e27632p85",
    "ssppmpn009.n2000e39832p100",
    "ssppmpn010.n500e5990p199",
    "ssppmpn011.n1000e5995p199",
    "ssppmpn012.n500e4396p50",
    "ssppmpn013.n1000e18281p200",
    "ssppmpn014.n1000e18293p99",
    "ssppmpn015.n2000e37052p400",
});

void Simulator::initDefaultEnvironment() {
    Solver::Environment env;
    env.save(Env::DefaultEnvPath());

    Solver::Configuration cfg;
    cfg.save(Env::DefaultCfgPath());
}

void Simulator::run(const Task &task) {
    String instanceName(task.instSet + task.instId + ".json");
    String solutionName(task.instSet + task.instId + ".json");

    char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
    char *argv[Cmd::MaxArgNum];
    for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
    strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

    int argc = ArgIndex::ArgStart;

    strcpy(argv[argc++], Cmd::InstancePathOption().c_str());
    strcpy(argv[argc++], (InstanceDir() + instanceName).c_str());

    System::makeSureDirExist(SolutionDir());
    strcpy(argv[argc++], Cmd::SolutionPathOption().c_str());
    strcpy(argv[argc++], (SolutionDir() + solutionName).c_str());

    if (!task.randSeed.empty()) {
        strcpy(argv[argc++], Cmd::RandSeedOption().c_str());
        strcpy(argv[argc++], task.randSeed.c_str());
    }

    if (!task.timeout.empty()) {
        strcpy(argv[argc++], Cmd::TimeoutOption().c_str());
        strcpy(argv[argc++], task.timeout.c_str());
    }

    if (!task.maxIter.empty()) {
        strcpy(argv[argc++], Cmd::MaxIterOption().c_str());
        strcpy(argv[argc++], task.maxIter.c_str());
    }

    if (!task.jobNum.empty()) {
        strcpy(argv[argc++], Cmd::JobNumOption().c_str());
        strcpy(argv[argc++], task.jobNum.c_str());
    }

    if (!task.runId.empty()) {
        strcpy(argv[argc++], Cmd::RunIdOption().c_str());
        strcpy(argv[argc++], task.runId.c_str());
    }

    if (!task.cfgPath.empty()) {
        strcpy(argv[argc++], Cmd::ConfigPathOption().c_str());
        strcpy(argv[argc++], task.cfgPath.c_str());
    }

    if (!task.logPath.empty()) {
        strcpy(argv[argc++], Cmd::LogPathOption().c_str());
        strcpy(argv[argc++], task.logPath.c_str());
    }

    Cmd::run(argc, argv);
}

void Simulator::run(const String &envPath) {
    char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
    char *argv[Cmd::MaxArgNum];
    for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
    strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

    int argc = ArgIndex::ArgStart;

    strcpy(argv[argc++], Cmd::EnvironmentPathOption().c_str());
    strcpy(argv[argc++], envPath.c_str());

    Cmd::run(argc, argv);
}

void Simulator::debug() {
    Task task;
    task.instSet = "";
    task.instId = "pmed1.n100e198p5";
    task.randSeed = "1500972793";
    //task.randSeed = to_string(RandSeed::generate());
    task.timeout = "180";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();
    task.runId = "0";

    run(task);
}

void Simulator::benchmark(int repeat) {
    Task task;
    task.instSet = "";
    //task.timeout = "180";
    //task.maxIter = "1000000000";
    task.timeout = "3600";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();

    random_device rd;
    mt19937 rgen(rd());
    for (int i = 0; i < repeat; ++i) {
        //shuffle(instList.begin(), instList.end(), rgen);
        for (auto inst = instList.begin(); inst != instList.end(); ++inst) {
            task.instId = *inst;
            task.randSeed = to_string(Random::generateSeed());
            task.runId = to_string(i);
            run(task);
        }
    }
}

void Simulator::parallelBenchmark(int repeat) {
    Task task;
    task.instSet = "";
    //task.timeout = "180";
    //task.maxIter = "1000000000";
    task.timeout = "3600";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();

    ThreadPool<> tp(4);

    random_device rd;
    mt19937 rgen(rd());
    for (int i = 0; i < repeat; ++i) {
        //shuffle(instList.begin(), instList.end(), rgen);
        for (auto inst = instList.begin(); inst != instList.end(); ++inst) {
            task.instId = *inst;
            task.randSeed = to_string(Random::generateSeed());
            task.runId = to_string(i);
            tp.push([=]() { run(task); });
            this_thread::sleep_for(1s);
        }
    }
}

void Simulator::generateInstance(const InstanceTrait &trait) {
    Random rand;

    Problem::Input input;

    // EXTEND[szx][5]: generate random instances.

    ostringstream path;
    path << InstanceDir() << "rand.n" << input.graph().nodes().size()
        << "e" << input.graph().edges().size() << "p" << input.demands().size() << ".json";
    save(path.str(), input);
}

void Simulator::convertSsppmpnInstance(const String &ssppmpnPath, int index) {
    Log(Log::Debug) << "converting " << ssppmpnPath << endl;

    pb::ShortestPassWithMustPassNodes oldInput;
    load(ssppmpnPath, oldInput);

    ID maxNodeId = max(oldInput.source(), oldInput.target());
    for (auto e = oldInput.edges().begin(); e != oldInput.edges().end(); ++e) {
        if (e->src() > maxNodeId) { maxNodeId = e->src(); }
        if (e->dst() > maxNodeId) { maxNodeId = e->dst(); }
    }
    ID nodeNum = maxNodeId + 1;

    Problem::Input input;
    input.set_src(oldInput.source());
    input.set_dst(oldInput.target());
    input.mutable_demands()->Resize(oldInput.mustpass().size(), 1);

    auto &graph(*input.mutable_graph());
    graph.mutable_nodes()->Reserve(nodeNum);
    for (ID n = 0; n < nodeNum; ++n) { graph.add_nodes(); }
    for (ID i = 0; i < oldInput.mustpass().size(); ++i) {
        auto &supplies(*graph.mutable_nodes(oldInput.mustpass(i))->mutable_supplies());
        supplies[i].set_quantity(1);
        supplies[i].set_price(0);
    }

    graph.mutable_edges()->Reserve(oldInput.edges().size());
    Arr2D<ID> edgeIndices(nodeNum, nodeNum, -1);
    for (auto e = oldInput.edges().begin(); e != oldInput.edges().end(); ++e) {
        if (edgeIndices.at(e->src(), e->dst()) < 0) {
            edgeIndices.at(e->src(), e->dst()) = graph.edges().size();
            auto &edge(*graph.add_edges());
            edge.set_src(e->src());
            edge.set_dst(e->dst());
            edge.set_cost(e->cost());
        } else {
            Log(Log::Warning) << "duplicated edge " << e->src() << "-" << e->dst() << endl;
            auto &edge(*graph.mutable_edges(edgeIndices.at(e->src(), e->dst())));
            if (e->cost() < edge.cost()) { edge.set_cost(e->cost()); }
        }
    }

    ostringstream path;
    path << InstanceDir() << "ssppmpn" << setfill('0') << setw(3) << index << ".n" << input.graph().nodes().size()
        << "e" << input.graph().edges().size() << "p" << oldInput.mustpass().size() << ".json";
    save(path.str(), input);
}

void Simulator::convertSsppmpnInstances() {
    int i = 0;
    convertSsppmpnInstance("Instance/SSPP-MPN/Hop.v200e990d20.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Hop.v200e990d40.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Hop.v300e995d40.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Hop.v500e2986d100.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Hop.v1000e3995d100.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Hop.v2000e39832d100.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Rand.v300e844d20.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Rand.v500e2000d22.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Rand.v2000e27742d85.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Rand.v2000e40000d100.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Clique.v500e5990d199.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Clique.v1000e5995d199.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Crafted.v500e4433d50.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Grid.v1000e18281d200.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Grid.v1000e18293d99.json", i++);
    convertSsppmpnInstance("Instance/SSPP-MPN/Grid.v2000e37052d400.json", i++);
}

}
