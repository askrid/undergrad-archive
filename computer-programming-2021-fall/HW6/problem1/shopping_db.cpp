#include "shopping_db.h"

ShoppingDB::ShoppingDB()
{
}

void ShoppingDB::add_product(std::string name, int price)
{
    products.push_back(new Product(name, price));
}

bool ShoppingDB::edit_product(std::string name, int price)
{
    if (price > 0)
    {
        Product *prod = find_product(name);
        if (prod)
        {
            prod->price = price;
            return true;
        }
    }
    return false;
}

Product *ShoppingDB::find_product(std::string name)
{
    auto it = std::find_if(products.begin(), products.end(), [&name](const Product *prod)
                           { return prod->name == name; });
    if (it != products.end())
    {
        return *it;
    }
    else
    {
        return nullptr;
    }
}

std::vector<Product *> ShoppingDB::get_products()
{
    return products;
}

void ShoppingDB::add_user(std::string name, std::string password, bool premium)
{
    if (premium)
    {
        users.push_back(new PremiumUser(name, password));
    }
    else
    {
        users.push_back(new NormalUser(name, password));
    }
}

User *ShoppingDB::find_user(std::string name)
{
    auto it = std::find_if(users.begin(), users.end(), [&name](const User *user)
                           { return user->name == name; });
    if (it != users.end())
    {
        return *it;
    }
    else
    {
        return nullptr;
    }
}

std::vector<User *> ShoppingDB::get_users()
{
    return users;
}