#include <iostream>
#include "Point.h"

#ifndef GRID_H
#define GRID_H

class Grid {
    int **grid;
    int row, column;
    static int counter;
public:
    Grid(int row_value, int column_value);

    void initialize_with_zeros();

    int getRow() const;

    int getColumn() const;

    int getAt(int r, int c) const;

    void setAt(int r, int c, int v);

    void printGrid();

    void mark_point(Point p);

    Grid(Grid const &g);

     ~Grid();
};

#endif
