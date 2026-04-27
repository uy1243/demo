#include "NewsClient.h"
#include <grpcpp/grpcpp.h>

using namespace grpc;

bool NewsClient::GetInstrumentSentiment(const std::string& instr, int minutes, float& avg_sentiment, std::vector<NewsInfo>& news) {
    ClientContext ctx;
    news_analyze::InstrumentReq req;
    news_analyze::InstrumentResp resp;

    req.set_instrument(instr);
    req.set_minutes(minutes);

    Status status = stub_->GetInstrumentSentiment(&ctx, req, &resp);
    if (!status.ok() || !resp.success()) return false;

    avg_sentiment = resp.avg_sentiment();
    for (auto& n : resp.news()) {
        news.push_back({n.title(), n.summary(), n.sentiment(), n.publish_time()});
    }
    return true;
}