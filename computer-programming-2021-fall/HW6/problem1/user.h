#ifndef PROBLEM1_USER_H
#define PROBLEM1_USER_H

#include <string>
#include <vector>
#include <cmath>
#include "product.h"

class User
{
public:
    User(std::string name, std::string password);
    const std::string name;
    bool authenticate(std::string password);
    void add_purchase_history(Product *product);
    void add_to_cart(Product *product);
    std::vector<Product *> get_cart();
    virtual int get_cart_price();
    void clear_cart();
    std::vector<Product *> get_history();

private:
    std::string password;
    std::vector<Product *> history;
    std::vector<Product *> cart;
};

class NormalUser : public User
{
public:
    NormalUser(std::string name, std::string password);
};

class PremiumUser : public User
{
public:
    PremiumUser(std::string name, std::string password);
    int get_cart_price();
};

#endif // PROBLEM1_USER_H
