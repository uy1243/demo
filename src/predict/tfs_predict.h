#pragma once
#include <string>
#include <vector>
#include "timesfm.grpc.pb.h"

class TFPredictor {
public:
    TFPredictor(const std::string& addr);
    bool Predict(
        const std::vector<double>& history,
        int horizon,
        const std::string& instr,
        std::vector<double>& out_mean,
        std::vector<double>& out_lower,
        std::vector<double>& out_upper
    );

private:
    std::unique_ptr<TimesFMService::Stub> stub_;
};