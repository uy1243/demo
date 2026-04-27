#pragma once
#include <string>
#include <vector>
#include "news_analyze.grpc.pb.h"

using grpc::Channel;

struct NewsInfo {
    std::string title;
    std::string summary;
    float sentiment;
    std::string publish_time;
};

class NewsClient {
public:
    NewsClient(std::shared_ptr<Channel> channel)
        : stub_(news_analyze::NewsAnalyzeService::NewStub(channel)) {}

    bool GetInstrumentSentiment(const std::string& instr, int minutes, float& avg_sentiment, std::vector<NewsInfo>& news);

private:
    std::unique_ptr<news_analyze::NewsAnalyzeService::Stub> stub_;
};