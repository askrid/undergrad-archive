#include <iostream>

#ifndef POINT_H
#define POINT_H

class Point {
    int x, y;
public:
     Point(int x_value, int y_value);

     int getX() const;

     int getY() const;
};

#endif
