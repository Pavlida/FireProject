#ifndef USERIMAGE_H
#define USERIMAGE_H
#include <QFrame>
#include <QMouseEvent>
#include <QSize>
#include <QPoint>
#include <QPainter>
#include <QMessageBox>


#include <cmath>
#include <map>
#include <random>


#include "cell.h"

class UserImage : public QFrame
{
    Q_OBJECT

public:
    UserImage();
    ~UserImage();


    std::pair<uchar, uchar>* source; //ignition point
    std::pair<uchar, uchar> cellAt(QPointF p) const;
    void setFrame(int f);
    uchar getFrame() {return currentFrame;}
    bool isRenderComplete() {return renderComplete;}
    void reset();

    void renderScenario();

    // EVENTS
protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    bool renderComplete = false;
    void paintFrame();

    // CELLS INFO
    double cellSize[2];
    Cell cells[100][100][200]; //
    uchar currentFrame = 0;

    // MAP EDITING
    bool draw = false;
    uchar brushSize = 0;
    uchar brushColor = Types::Broadleaves;
    void paintBrush(QPoint p);
    QPoint* drawCursor = nullptr; // position of cursor in case we need to paint ot

public slots:
    void setBrushSize(int s) { brushSize = static_cast<uchar>(s - 1); }
    void setBrushColor(int c){ brushColor = static_cast<uchar>(abs(c) - 2);} // weird ids lead to having to do this way
    void setWindDir(int d)
    {
        int angle = 270 - d;
        if (angle < 0)
            angle += 360;
        windDirection = angle;
    }
    void setWindSpeed(double s) {windSpeed = s;}

    void returnToEditing();

signals:
    void pressedCoords(QString coords);
    void editingStage();
    void viewingStage();

private: //RENDERING
    double windDirection = 270, windSpeed = 0;

    double** distances;
    void createDistances();

    std::map<std::pair<uchar, uchar>, uchar> igniteNeighbours(const uchar& row, const uchar& col, double* wind);
    void noiseWind();
    double windInfluence(uchar dir);

};
#endif // USERIMAGE_H
