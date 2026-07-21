#pragma once
#include <string>
#include <vector>

namespace card_batch {

struct KnowledgePoint {
    int id;
    std::string topic;
    std::string description;
};

extern const std::vector<KnowledgePoint> kFixedPoints;
std::vector<KnowledgePoint> loadFromFile(const std::string& path);
const std::vector<KnowledgePoint>& all();
const KnowledgePoint* findById(int id);

}  // namespace card_batch