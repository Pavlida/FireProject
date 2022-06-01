#include "cell.h"

Cell::Cell()
{

}

QColor Cell::getColor() const
{
    if (alreadyBurnt)
        return Qt::gray;

    if (isBurning)
        return Qt::red;

    switch(type)
    {
        case Broadleaves:
            return QColor(26, 67, 1);
        case Shrubs:
            return QColor(115, 169, 66);
        case Grassland:
            return QColor(170, 213, 118);
        case FPConifers:
            return QColor(36, 85, 1);
        case AFAreas:
            return QColor(83, 141, 34);
        case NFPForest:
            return QColor(20, 54, 1);
        default:
            return Qt::transparent;
    }
}
