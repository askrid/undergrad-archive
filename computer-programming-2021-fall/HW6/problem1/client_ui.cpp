#include <vector>
#include "client_ui.h"
#include "product.h"
#include "user.h"

ClientUI::ClientUI(ShoppingDB &db, std::ostream &os) : UI(db, os), current_user() {}

void ClientUI::signup(std::string username, std::string password, bool premium)
{
    db.add_user(username, password, premium);
    os << "CLIENT_UI: " << username << " is signed up." << std::endl;
}

void ClientUI::login(std::string username, std::string password)
{
    if (current_user)
    {
        os << "CLIENT_UI: Please logout first." << std::endl;
    }
    else
    {
        User *user = db.find_user(username);
        if (user && user->authenticate(password))
        {
            current_user = user;
            os << "CLIENT_UI: " << username << " is logged in." << std::endl;
        }
        else
        {
            os << "CLIENT_UI: Invalid username or password." << std::endl;
        }
    }
}

void ClientUI::logout()
{
    if (current_user)
    {
        os << "CLIENT_UI: " << current_user->name << " is logged out." << std::endl;
        current_user = nullptr;
    }
    else
    {
        os << "CLIENT_UI: There is no logged-in user." << std::endl;
    }
}

void ClientUI::add_to_cart(std::string product_name)
{
    if (current_user)
    {
        Product *product = db.find_product(product_name);
        if (product)
        {
            current_user->add_to_cart(product);
            os << "CLIENT_UI: " << product->name << " is added to the cart." << std::endl;
        }
        else
        {
            os << "CLIENT_UI: Invalid product name." << std::endl;
        }
    }
    else
    {
        os << "CLIENT_UI: Please login first." << std::endl;
    }
}

void ClientUI::list_cart_products()
{
    if (current_user)
    {
        std::vector<Product *> products = current_user->get_cart();
        os << "CLIENT_UI: Cart: [";
        if (products.size() > 0)
        {
            if (typeid(*current_user) == typeid(PremiumUser))
            {
                for (auto it = products.begin(); it < products.end() - 1; it++)
                {
                    os << "(" << (*it)->name << ", " << round((*it)->price * 0.9) << ")"
                       << ", ";
                }
                os << "(" << (*(products.end() - 1))->name << ", " << round((*(products.end() - 1))->price * 0.9) << ")";
            }
            else
            {
                for (auto it = products.begin(); it < products.end() - 1; it++)
                {
                    os << "(" << (*it)->name << ", " << (*it)->price << ")"
                       << ", ";
                }
                os << "(" << (*(products.end() - 1))->name << ", " << (*(products.end() - 1))->price << ")";
            }
        }
        os << "]" << std::endl;
    }
    else
    {
        os << "CLIENT_UI: Please login first." << std::endl;
    }
}

void ClientUI::buy_all_in_cart()
{
    if (current_user)
    {
        std::vector<Product *> cart = current_user->get_cart();
        for (auto it = cart.begin(); it < cart.end(); it++)
        {
            current_user->add_purchase_history(*it);
        }
        os << "CLIENT_UI: Cart purchase completed. Total price: " << current_user->get_cart_price() << "." << std::endl;
        current_user->clear_cart();
    }
    else
    {
        os << "CLIENT_UI: Please login first." << std::endl;
    }
}

void ClientUI::buy(std::string product_name)
{
    if (current_user)
    {
        Product *product = db.find_product(product_name);
        if (product)
        {
            current_user->add_purchase_history(product);
            int price = (typeid(*current_user) == typeid(PremiumUser)) ? round(product->price * 0.9) : product->price;
            os << "CLIENT_UI: Purchase completed. Price: " << price << "." << std::endl;
        }
        else
        {
            os << "CLIENT_UI: Invalid product name." << std::endl;
        }
    }
    else
    {
        os << "CLIENT_UI: Please login first." << std::endl;
    }
}

void ClientUI::recommend_products()
{
    if (current_user)
    {
        std::vector<Product *> recommended;
        std::vector<Product *> history = current_user->get_history();
        os << "CLIENT_UI: Recommended products: [";
        if (typeid(*current_user) == typeid(NormalUser) && history.size() > 0)
        {
            for (auto it = history.end() - 1; recommended.size() < 3 && it >= history.begin(); it--)
            {
                if (std::find(recommended.begin(), recommended.end(), *it) == recommended.end())
                {
                    recommended.push_back(*it);
                }
            }
            for (auto it = recommended.begin(); it < recommended.end() - 1; it++)
            {
                os << "(" << (*it)->name << ", " << (*it)->price << ")"
                   << ", ";
            }
            os << "(" << (*(recommended.end() - 1))->name << ", " << (*(recommended.end() - 1))->price << ")";
        }
        else if (typeid(*current_user) == typeid(PremiumUser))
        {
            std::vector<std::pair<User *, int>> sims;
            std::vector<User *> users = db.get_users();
            std::vector<Product *> products = db.get_products();
            for (auto it = users.begin(); it < users.end(); it++)
            {
                if (*it != current_user)
                {
                    std::pair<User *, int> sim;
                    sim.first = *it;
                    int sum = 0;

                    for (Product *product : products)
                    {
                        if (std::find(history.begin(), history.end(), product) != history.end())
                        {
                            std::vector<Product *> target_history = (*it)->get_history();
                            sum += std::count(target_history.begin(), target_history.end(), product);
                        }
                    }
                    sim.second = sum;
                    sims.push_back(sim);
                }
            }
            std::stable_sort(sims.begin(), sims.end(), [](std::pair<User *, int> const &a, std::pair<User *, int> const &b)
                             { return a.second > b.second; });

            for (auto it = sims.begin(); recommended.size() < 3 && it < sims.end(); it++)
            {
                if (it->first->get_history().size() == 0) continue;
                Product *product = *(it->first->get_history().end() - 1);
                if (std::find(recommended.begin(), recommended.end(), product) == recommended.end())
                {
                    recommended.push_back(product);
                }
            }
            for (auto it = recommended.begin(); it < recommended.end() - 1; it++)
            {
                os << "(" << (*it)->name << ", " << round((*it)->price * 0.9) << ")"
                   << ", ";
            }
            if (recommended.size() > 0)
                os << "(" << (*(recommended.end() - 1))->name << ", " << round((*(recommended.end() - 1))->price * 0.9) << ")";
        }
        os << "]" << std::endl;
    }
    else
    {
        os << "CLIENT_UI: Please login first." << std::endl;
    }
}
