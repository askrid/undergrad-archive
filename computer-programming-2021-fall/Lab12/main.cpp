#include <iostream>
#include <ctime>
#include "player.h"

void round(Player &a, Player &b);

int main() {
    std::srand(std::time(nullptr));

    Player a, b;
    a.add_monster(fireMon);
    a.add_monster(fireMon);
    a.add_monster(waterMon);
    b.add_monster(waterMon);
    b.add_monster(grassMon);
    b.add_monster(grassMon);

    std::cout << "Game start! Player A: " << a.get_total_hp() << ", Player B: " << b.get_total_hp() << std::endl;

    for (int i = 1;; i++) {
        round(a, b);
        std::cout << "Round " << i << ": " << a.get_total_hp() << " " << b.get_total_hp() << std::endl;
        if (b.get_num_monsters() == 0) {
            std::cout << "Player a won the game!" << std::endl;
            break;
        } else if (a.get_num_monsters() == 0) {
            std::cout << "Player b won the game!" << std::endl;
            break;
        }
    }
    return 0;
}

void round(Player &a, Player &b) {
    Monster* a_monster = a.select_monster();
    Monster* b_monster = b.select_monster();

    if (a_monster->get_speed() > b_monster->get_speed()) {
        a_monster->attack(b_monster);
        if (b_monster->get_hp() <= 0) {
            std::cout << "Player b's monster " <<  *b_monster << " fainted!" << std::endl;
            b.delete_monster(b_monster);
        } else {
            b_monster->attack(a_monster);
            if (a_monster->get_hp() <= 0) {
                std::cout << "Player a's monster " <<  *a_monster << " fainted!" << std::endl;
                a.delete_monster(a_monster);
            }
        }
    } else {
        b_monster->attack(a_monster);
        if (a_monster->get_hp() <= 0) {
            std::cout << "Player a's monster " <<  *a_monster << " fainted!" << std::endl;
            a.delete_monster(a_monster);
        } else {
            a_monster->attack(b_monster);
            if (b_monster->get_hp() <= 0) {
                std::cout << "Player b's monster " <<  *b_monster << " fainted!" << std::endl;
                b.delete_monster(b_monster);
            }
        }
    }
}
