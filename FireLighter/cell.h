#ifndef CELL_H
#define CELL_H
#include <QColor>

enum Types
{
    Broadleaves,
    Shrubs,
    Grassland,
    FPConifers,
    AFAreas,
    NFPForest,
};

class UserImage;

class Cell
{
public:

    enum Types
    {

    };

    Cell();
    QColor getColor() const;
    uchar type = Broadleaves;

    bool isBurning = false, alreadyBurnt = false;

private:
    friend UserImage;

    constexpr static double fireSpreadProb[6][6] ={
        {0.3, 0.375, 0.25, 0.275, 0.25, 0.25},
        {0.375, 0.375, 0.35, 0.4, 0.3, 0.375},
        {0.45, 0.475, 0.475, 0.475, 0.375, 0.475},
        {0.225, 0.325, 0.25, 0.35, 0.2, 0.35},
        {0.25, 0.25, 0.3, 0.475, 0.35, 0.25},
        {0.075, 0.1, 0.075, 0.275, 0.075, 0.075}};

    constexpr static uchar nominalSpreadV[6] =
        {100, 140, 120, 200, 120, 60};
};

#endif // CELL_H
