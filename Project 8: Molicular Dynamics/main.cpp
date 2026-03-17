#include <algorithm>
#include <chrono>
#include <iomanip>
#include <set>

#include "processing.h"

static std::vector<int> buildThreadSweep(int maxThreads) {
    std::set<int> threadSet;
    threadSet.insert(1);
    for (int t = 2; t <= maxThreads; t *= 2) {
        threadSet.insert(t);
    }
    threadSet.insert(maxThreads);
    return std::vector<int>(threadSet.begin(), threadSet.end());
}

static std::vector<double> buildSkinSweep(double baseSkin) {
    std::set<double> skinSet = {0.2, 0.4, 0.8, baseSkin};
    return std::vector<double>(skinSet.begin(), skinSet.end());
}

static std::string toFixed(double value, int precision = 3) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

static void runBenchmark(const std::map<std::string, std::string>& baseConfig) {
    const size_t baseTimeSteps = std::stoull(baseConfig.at("timeSteps"));
    const size_t baseStepSkip = static_cast<size_t>(std::stoull(baseConfig.at("stepSkip")));
    const size_t targetSampleCount = std::max<size_t>(1, (baseTimeSteps + baseStepSkip - 1) / baseStepSkip);
    const double baseSkin = (baseConfig.count("neighborSkin") != 0) ? std::stod(baseConfig.at("neighborSkin")) : 0.4;

    const std::vector<double> stepScales = {0.5, 1.0, 2.0};
    const std::vector<int> threadSweep = buildThreadSweep(std::max(1, omp_get_max_threads()));
    const std::vector<double> skinSweep = buildSkinSweep(baseSkin);

    std::cout << "benchmark_mode=on\n";
    std::cout << "threads,neighborSkin,timeSteps,numParticles,seconds,steps_per_second\n";

    for (int threads : threadSweep) {
        omp_set_num_threads(threads);

        for (double skin : skinSweep) {
            for (double scale : stepScales) {
                std::map<std::string, std::string> cfg = baseConfig;
                const size_t scaledSteps = std::max<size_t>(100, static_cast<size_t>(std::llround(static_cast<double>(baseTimeSteps) * scale)));
                const size_t scaledSkip = std::max<size_t>(1, (scaledSteps + targetSampleCount - 1) / targetSampleCount);

                cfg["timeSteps"] = std::to_string(scaledSteps);
                cfg["stepSkip"] = std::to_string(scaledSkip);
                cfg["neighborSkin"] = toFixed(skin, 3);
                cfg["showProgress"] = "0";

                auto t0 = std::chrono::steady_clock::now();
                MolucularSystem system(cfg);
                system.runSimulation();
                auto t1 = std::chrono::steady_clock::now();

                double seconds = std::chrono::duration<double>(t1 - t0).count();
                double stepsPerSecond = static_cast<double>(system.timeSteps) / std::max(1e-12, seconds);

                std::cout << threads << "," << toFixed(skin, 3) << "," << system.timeSteps << "," << system.numParticles << "," << toFixed(seconds, 6)
                          << "," << toFixed(stepsPerSecond, 2) << "\n";
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Example usage of the MolucularSystem class

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file> [--benchmark]\n";
        return 1;
    }

    try {
        std::map<std::string, std::string> config = parseConfigFile(argv[1]);

        const bool benchmarkMode = (argc >= 3 && std::string(argv[2]) == "--benchmark");
        if (benchmarkMode) {
            runBenchmark(config);
            return 0;
        }

        MolucularSystem system(config);
        std::cout << "Running simulation with " << system.numParticles << " particles for " << system.timeSteps << " time steps on "
                  << omp_get_max_threads() << " threads." << std::endl;
        system.runSimulation();
        std::cout << "Simulation complete. Saving results..." << std::endl;

        // system.save();

        system.binSave();  // Save results in binary format for efficient loading in Python

        std::cout << "Results saved to NPZ file." << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}