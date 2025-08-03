//
// Created by Yinghao Qin on 07/12/2023.
//

#include <iostream>
#include <thread>
#include <filesystem>  // C++17

#include "case.h"
#include "CACO.h"
#include "BACO2.h"
#include "stats.h"

#define MAX_TRIALS 10

using namespace std;
namespace fs = std::filesystem;

// Enum for Algorithms
enum class Algorithm {BACO, CBACO};

const string DATA_PATH = "instances/";
const string STATS_PATH = "stats";

struct ProgramOptions {
    Algorithm algorithm{};
    std::string instanceName;

    // Global optional parameter
    int stp = 0;  // 0 = max-evals (default), 1 = max-time

    // Optional parameters for CBACO
    int isCan = 1;
    int isRA = 1;
    int representation = 1;

    static ProgramOptions parse(int argc, char* argv[]) {
        if (argc < 3) {
            throw std::invalid_argument("Usage: <algorithm> <instance> [stp] [isCan] [isRA] [representation]");
        }
        // stp represents the stopping criteria, 0 for max-evals, 1 for max-time

        ProgramOptions opts;
        std::string algorithmStr(argv[1]);
        if (algorithmStr == "baco") {
            opts.algorithm = Algorithm::BACO;
        } else if (algorithmStr == "cbaco") {
            opts.algorithm = Algorithm::CBACO;
        } else {
            throw std::invalid_argument("Unknown algorithm: " + algorithmStr);
        }

        opts.instanceName = argv[2];

        if (argc > 3) opts.stp = std::stoi(argv[3]);
        if (opts.algorithm == Algorithm::CBACO) {
            if (argc > 4) opts.isCan = std::stoi(argv[4]); // population initialization, 1 for "buildSolutionFromCandidates", otherwise "buildSolution"
            if (argc > 5) opts.isRA = std::stoi(argv[5]);  // whether using confidence-based selection strategy, 1 means yes, 0 means no
            if (argc > 6) opts.representation = std::stoi(argv[6]); // 1 represents order-split, 2 represents direct with local search
        }

        return opts;
    }
};

int main(int argc, char* argv[]) {
    // working directory setting
    fs::path current = fs::current_path();
    while (!fs::exists(current / "instances")) {
        if (current == current.root_path()) {
            std::cerr << "Failed to locate project root (missing 'instances' directory)" << std::endl;
            return 1;
        }
        current = current.parent_path();
    }

    fs::current_path(current);

    ProgramOptions opts;
    try {
        opts = ProgramOptions::parse(argc, argv);
    } catch (const std::exception& ex) {
        cerr << "Error parsing arguments: " << ex.what() << endl;
        return 1;
    }

    std::vector<double> perfOfTrials(MAX_TRIALS);
    std::vector<std::thread> threads;

    if (opts.algorithm == Algorithm::BACO) {
        auto thread_function_1 = [&](int run) {
            Case* instance = new Case(DATA_PATH + opts.instanceName, run);
            auto* baco = new BACO2(instance, run, opts.stp);
            baco->run();
            perfOfTrials[run - 1] = baco->gbestf;
            delete instance;
            delete baco;
        };
        for (int run = 2; run <= MAX_TRIALS; ++run) {
            threads.emplace_back(thread_function_1, run);
        }
        thread_function_1(1);
    }

    if (opts.algorithm == Algorithm::CBACO) {
        auto thread_function_2 = [&](int run) {
            Case* instance = new Case(DATA_PATH + opts.instanceName, run);
            int timer = (instance->customerNumber <= 100) ? 1 * 36
                      : (instance->customerNumber <= 915) ? 2 * 36
                      : 3 * 36;
            double afr = 0.8;

            CACO* caco = new CACO(instance, run, opts.stp, opts.isCan, opts.isRA, opts.representation, timer, afr);
            caco->run();
            perfOfTrials[run - 1] = caco->bestSolution->fit;
            delete caco;
            delete instance;
        };
        for (int run = 2; run <= MAX_TRIALS; ++run) {
            threads.emplace_back(thread_function_2, run);
        }
        thread_function_2(1);
    }

    for (auto& t : threads) t.join();

    string instancePrefix = opts.instanceName.substr(0, opts.instanceName.find_last_of('.'));
    string directoryPath;
    if (opts.algorithm == Algorithm::CBACO) {
        directoryPath = STATS_PATH + "/" + (opts.representation == 1 ? "cbaco-i" : "cbaco-d") + "/" + instancePrefix;
    } else {
        directoryPath = STATS_PATH + "/baco/" + instancePrefix;
    }

    string filepath = directoryPath + "/" + "stats." + instancePrefix + ".txt";
    StatsInterface::stats_for_multiple_trials(filepath, perfOfTrials);

    return 0;
}