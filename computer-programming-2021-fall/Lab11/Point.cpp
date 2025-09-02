#include "Point.h"

Point::Point(int x_value, int y_value) {
         x = x_value;
         y = y_value;
     }

int Point::getX() const {return x;}

int Point::getY() const {return y;}