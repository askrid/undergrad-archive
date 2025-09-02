#include "user.h"

User::User(std::string name, std::string password) : name(name), password(password)
{
}

NormalUser::NormalUser(std::string name, std::string password) : User(name, password)
{
}

PremiumUser::PremiumUser(std::string name, std::string password) : User(name, password)
{
}

bool User::authenticate(std::string password)
{
    return this->password == password;
}

void User::add_purchase_history(Product *product)
{
    history.push_back(product);
}

void User::add_to_cart(Product *product)
{
    cart.push_back(product);
}

std::vector<Product *> User::get_cart()
{
    return cart;
}

int User::get_cart_price()
{
    int sum = 0;
    for (auto it = cart.begin(); it < cart.end(); it++)
    {
        sum += (*it)->price;
    }
    return sum;
}

int PremiumUser::get_cart_price()
{
    int sum = 0;
    std::vector<Product *> cart = this->get_cart();
    for (auto it = cart.begin(); it < cart.end(); it++)
    {
        sum += round((*it)->price * 0.9);
    }
    return sum;
}

void User::clear_cart()
{
    cart.clear();
}

std::vector<Product *> User::get_history()
{
    return history;
}