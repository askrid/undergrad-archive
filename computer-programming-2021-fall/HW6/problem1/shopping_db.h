#ifndef PROBLEM1_SHOPPING_DB_H
#define PROBLEM1_SHOPPING_DB_H

#include <string>
#include <vector>
#include <algorithm>
#include "user.h"
#include "product.h"

class ShoppingDB
{
public:
    ShoppingDB();
    void add_product(std::string name, int price);
    bool edit_product(std::string name, int price);
    Product *find_product(std::string name);
    std::vector<Product *> get_products();
    void add_user(std::string name, std::string password, bool premium);
    User *find_user(std::string name);
    std::vector<User *> get_users();
private:
    std::vector<User *> users;
    std::vector<Product *> products;
};

#endif // PROBLEM1_SHOPPING_DB_H
