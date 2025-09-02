#include <iostream>
#include "Point.h"
#include "Grid.h"

void printNumberGrid(Grid g){
    for (int i=0; i<g.getRow(); i++) {
        for (int j=0; j<g.getColumn(); j++) {
            g.setAt(i, j, j + i*g.getColumn());
        }
    }

    g.printGrid();
}

int main() {
    Point p(1,3);
    Grid g(2,3);

    g.printGrid();

    Point p1(1,0);
    Point p2(0,1);
    Point p3(3,3);

    g.mark_point(p1);
    g.mark_point(p2);
    g.mark_point(p3);

    g.printGrid();

    return 0;
}
