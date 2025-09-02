#include <algorithm>
#include "restaurant_app.h"

#include <iostream>
#include <iomanip>
#include <numeric>


void RestaurantApp::rate(string target, int rate) {
    auto ratings = find_restaurant(target);
    if (ratings == nullptr) {
        vector<int> ratings;
        ratings.push_back(rate);
        restaurants.insert(std::pair<string, shared_ptr<vector<int>>>(target, make_shared<vector<int>>(ratings)));
    } else {
        ratings->push_back(rate);
        std::sort(ratings->begin(), ratings->end());
    }
}


void RestaurantApp::list() {
    for (auto const& res : restaurants) {
        std::cout << res.first << " ";
    }
    std::cout << std::endl;
}


void RestaurantApp::show(string target) {
    auto ratings = find_restaurant(target);
    if (ratings == nullptr) {
        std::cout << target << " does not exist." << std::endl;
    } else {
        for (auto const& rate : *ratings) {
            std::cout << rate << " ";
        }
        std::cout << std::endl;
    }
}


void RestaurantApp::ave(string target) {
    auto ratings = find_restaurant(target);
    if (ratings == nullptr) {
        std::cout << target << " does not exist." << std::endl;
    } else {
        double average = std::accumulate(ratings->begin(), ratings->end(), 0.0) / ratings->size();
        std::cout << average << std::endl;
    }
}


void RestaurantApp::del(string target, int rate) {
    auto ratings = find_restaurant(target);
    if (ratings == nullptr) {
        std::cout << target << " does not exist." << std::endl;
    } else {
        ratings->erase(std::remove(ratings->begin(), ratings->end(), rate), ratings->end());
    }
}


void RestaurantApp::cheat(string target, int rate) {
    auto ratings = find_restaurant(target);
    if (ratings == nullptr) {
        std::cout << target << " does not exist." << std::endl;
    } else {
        ratings->erase(std::remove_if(ratings->begin(), ratings->end(), [&rate](const int& x) {return x < rate;}), ratings->end());
    }
}


shared_ptr<vector<int>> RestaurantApp::find_restaurant(string target) {
    auto it = restaurants.find(target);
    if (it == restaurants.end()) {
        return nullptr;
    }
    return restaurants[target];
}
