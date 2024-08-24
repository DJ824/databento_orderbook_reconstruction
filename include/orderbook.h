//
// Created by Devang Jaiswal on 6/27/24.
//

#ifndef DATABENTO_ORDERBOOK_ORDERBOOK_H
#define DATABENTO_ORDERBOOK_ORDERBOOK_H

#include <cstdint>
#include <map>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <utility>
#include <iterator>
#include "order.h"
#include "limit.h"
#include "limit_pool.h"
#include "order_pool.h"
#include "../src/database.cpp"


class orderbook {
private:
    //std::unordered_map<std::pair<float, bool>, limit*> limit_lookup_;
    std::unordered_map<std::pair<float, bool>, limit*, boost::hash<std::pair<float, bool>>> limit_lookup_;
    order_pool order_pool_;

    //std::map<float, limit*>::iterator best_bid_it_;
    //std::map<float, limit*>::iterator best_offer_it_;
    uint64_t bid_count_;
    uint64_t ask_count_;
    //limit_pool limit_pool_;
    DatabaseManager &db_manager_;


public:
    orderbook(DatabaseManager& db_manager);
    ~orderbook();
    std::unordered_map<uint64_t , order* > order_lookup_;


    void add_limit_order(uint64_t id, float price, uint32_t size, bool side, uint64_t unix_time);
    //void remove_limit_order(order& target);
    void modify_order(uint64_t id, float new_price, uint32_t new_size, bool side, uint64_t unix_time);
    void trade_order(uint64_t id, float price, uint32_t size, bool side);
    void remove_order(uint64_t id, float price, uint32_t size, bool side);
    limit* get_best_bid();
    limit* get_best_ask();
    float get_best_bid_price();
    float get_best_ask_price();
    uint64_t get_count();
    bool contains_order(uint64_t id);
    order* get_order(uint64_t id);
    limit* get_or_insert_limit(bool side, float price);
    //void update_best_bid();
   // void update_best_offer();

    std::atomic<uint64_t> message_count_{0};
    std::map<float, limit *, std::less<>> offers_;
    std::map<float, limit *, std::greater<>> bids_;
};


#endif //DATABENTO_ORDERBOOK_ORDERBOOK_H
