#include "knowledge_points.h"

#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace card_batch {

const std::vector<KnowledgePoint> kFixedPoints = {
    {1,  "股票种类与标识",
         "普通股与优先股的区别、A股/B股/H股/N股的概念、ST 与 *ST 股票的标识与含义"},
    {2,  "沪深北三大交易所",
         "上交所、深交所、北交所的成立时间、股票代码区间、板块定位差异"},
    {3,  "股票交易规则",
         "T+1 交收制度、涨跌幅限制（10%/20%/不设限）、集合竞价与连续竞价的时间段"},
    {4,  "债券基础",
         "国债/金融债/企业债/公司债/可转债的区别、到期收益率 YTM、票面利率与实际收益率"},
    {5,  "基金类型",
         "开放式与封闭式基金、ETF 与 LOF、基金净值 NAV 的计算、公募基金与私募基金"},
    {6,  "金融衍生品",
         "期货与期权的区别、保证金交易、杠杆倍数、多头与空头头寸"},
    {7,  "技术分析指标",
         "K 线（阳线/阴线/十字星）、均线 MA、MACD、KDJ、布林带 BOLL 的基本含义"},
    {8,  "基本面分析指标",
         "PE 市盈率、PB 市净率、ROE 净资产收益率、EPS 每股收益的计算与含义"},
    {9,  "证券公司业务",
         "经纪业务、投行业务、资产管理业务、自营业务、融资融券业务"},
    {10, "证券从业法规",
         "投资者适当性管理、内幕信息与内幕交易、操纵市场、从业人员禁止行为"},
};

std::vector<KnowledgePoint> loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("knowledge_points: cannot open " + path);
    }
    nlohmann::json j;
    in >> j;

    std::vector<KnowledgePoint> out;
    out.reserve(j.size());
    for (const auto& item : j) {
        out.push_back(KnowledgePoint{
            item.value("id", 0),
            item.value("topic", ""),
            item.value("description", ""),
        });
    }
    return out;
}

const std::vector<KnowledgePoint>& all() {
    return kFixedPoints;
}

const KnowledgePoint* findById(int id) {
    for (const auto& kp : kFixedPoints) {
        if (kp.id == id) return &kp;
    }
    return nullptr;
}

}  // namespace card_batch