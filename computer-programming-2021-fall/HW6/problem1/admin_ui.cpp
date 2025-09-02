#include "admin_ui.h"

AdminUI::AdminUI(ShoppingDB &db, std::ostream &os) : UI(db, os) {}

void AdminUI::add_product(std::string name, int price)
{
    if (price > 0)
    {
        db.add_product(name, price);
        os << "ADMIN_UI: " << name << " is added to the database." << std::endl;
    }
    else
    {
        os << "ADMIN_UI: Invalid price." << std::endl;
    }
}

void AdminUI::edit_product(std::string name, int price)
{
    if (db.edit_product(name, price))
    {
        os << "ADMIN_UI: " << name << " is modified from the database." << std::endl;
    }
    else
    {
        if (!db.find_product(name))
        {
            os << "ADMIN_UI: Invalid product name." << std::endl;
        }
        else
        {
            os << "ADMIN_UI: Invalid price." << std::endl;
        }
    }
}

void AdminUI::list_products()
{
    std::vector<Product *> products = db.get_products();
    os << "ADMIN_UI: Products: [";
    if (products.size() > 0)
    {
        for (auto it = products.begin(); it < products.end() - 1; it++)
        {
            os << "(" << (*it)->name << ", " << (*it)->price << ")"
               << ", ";
        }
        os << "(" << (*(products.end() - 1))->name << ", " << (*(products.end() - 1))->price << ")";
    }
    os << "]" << std::endl;
}
