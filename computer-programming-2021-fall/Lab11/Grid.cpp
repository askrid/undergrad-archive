#include "Grid.h"

int Grid::counter = 1;

Grid::Grid(int row_value, int column_value) {
    row = row_value;
    column = column_value;
    grid = new int*[row];

    for (int i=0; i<row; i++) {
        grid[i] = new int[column];
    }
    initialize_with_zeros();
}

void Grid::initialize_with_zeros() {
    for (int i=0; i<row; i++) {
        for (int j=0; j<column; j++) {
            grid[i][j] = 0;
        }
    }
}

int Grid::getRow() const { return row; }

int Grid::getColumn() const { return column; }

int Grid::getAt(int r, int c) const { return grid[r][c]; }

void Grid::setAt(int r, int c, int v) { grid[r][c] = v; }

void Grid::printGrid() {
    for (int i=0; i<row; i++) {
        for (int j=0; j<column; j++) {
            std::cout << grid[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

void Grid::mark_point(Point p) {
    if (p.getX() < row && p.getY() < column) {
        setAt(p.getX(), p.getY(), counter);
        counter++;
    }
}

Grid::Grid(Grid const &g) {
    row = g.getRow();
    column = g.getColumn();
    grid = new int*[row];

    for (int i=0; i<row; i++) {
        grid[i] = new int[column];
        for (int j=0; j<row; j++) {
            grid[i][j] = g.getAt(i, j);
        }
    }
}

Grid::~Grid() {
    
    for (int i=0; i<row; i++) {
        delete[] grid[i];
    }
    delete[] grid;

    std::cout << "Clean-up Grid" << std::endl;
}