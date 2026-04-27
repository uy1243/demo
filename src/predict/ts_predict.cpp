#include "TFPredictor.h"
#include <grpcpp/grpcpp.h>

using grpc::Channel;
using grpc::ClientContext;

TFPredictor::TFPredictor(const std::string& addr) {
    stub_ = TimesFMService::NewStub(
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials())
    );
}

bool TFPredictor::Predict(
    const std::vector<double>& history,
    int horizon,
    const std::string& instr,
    std::vector<double>& out_mean,
    std::vector<double>& out_lower,
    std::vector<double>& out_upper
) {
    ClientContext ctx;
    PredictRequest req;
    PredictResponse resp;

    for (double v : history) req.add_history(v);
    req.set_horizon(horizon);
    req.set_instrument(instr);

    auto status = stub_->Predict(&ctx, req, &resp);
    if (!status.ok() || !resp.success()) return false;

    out_mean.assign(resp.mean().begin(), resp.mean().end());
    out_lower.assign(resp.lower().begin(), resp.lower().end());
    out_upper.assign(resp.upper().begin(), resp.upper().end());
    return true;
}