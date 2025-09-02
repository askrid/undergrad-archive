#include "monster.h"

int Monster::num_monsters = 0;
Monster::Monster(string name, hp_t hp, hp_t damage, speed_t speed, MonsterType type, MonsterType critical_to)
        : name(name), hp(hp), damage(damage), speed(speed), type(type), critical_to(critical_to) {
            id = num_monsters++;
        }

WaterMon::WaterMon() : Monster("Squirtle", 500, 10, 30, waterMon, fireMon) {}
FireMon::FireMon() : Monster("Charmander", 300, 20, 60, fireMon, grassMon) {}
GrassMon::GrassMon() : Monster("Bulbasaur", 600, 10, 50, grassMon, waterMon) {}

hp_t Monster::get_hp() const {
    return hp;
}

MonsterType Monster::get_type() const {
    return type;
}

void Monster::decrease_health(hp_t attack_damage) {
    hp -= attack_damage;
}

void Monster::attack(Monster *attacked_monster) {
    if (attacked_monster->get_type() == critical_to) {
        critical_attack(attacked_monster);
    } else {
        attacked_monster->decrease_health(damage);
    }
}

void Monster::critical_attack(Monster *attacked_monster) const {
    attacked_monster->decrease_health(2 * damage);
}

speed_t Monster::get_speed() const {
    return speed;
}

void WaterMon::critical_attack(Monster *attacked_monster) const {
    attacked_monster->decrease_health(damage * damage / 2);
}

void FireMon::critical_attack(Monster *attacked_monster) const {
    attacked_monster->decrease_health((int) (10 * std::rand()) / RAND_MAX);
}

void GrassMon::critical_attack(Monster *attacked_monster) const {
    attacked_monster->decrease_health(3 * damage);
}

ostream& operator<< (ostream& os, const Monster& monster) {
    return os << monster.name;
}