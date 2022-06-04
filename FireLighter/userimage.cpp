#include "userimage.h"
#include "iostream"
#include <iomanip>
#include <chrono>
enum WindDirection
{
    E,
    NE,
    N,
    NW,
    W,
    SW,
    S,
    SE
};

const double Cell::fireSpreadProb[6][6]; //redefinition here so it is seen in file
const uchar Cell::nominalSpreadV[6];

UserImage::UserImage()
{
    cellSize[0] = this->width() / 200.;
    cellSize[1] = this->height() / 100.; //let's have a standardized image of 200 x 100 cells

    for (uchar i = 0; i < 100; ++i)
    {
        for (uchar j = 0; j < 200; ++j)
        {
            cells[0][i][j] = Cell();
        }
    }

    installEventFilter(this);
}

UserImage::~UserImage()
{
    delete drawCursor;
}

void UserImage::paintFrame()
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    for (uchar i = 0; i < 100; ++i)
    {
        for (uchar j = 0; j < 200; ++j)
            painter.fillRect(j * cellSize[0], i * cellSize[1], cellSize[0] + 1, cellSize[1] + 1, cells[currentFrame][i][j].getColor()); // +1 to avoid weird gaps
    }

    if(drawCursor != nullptr) // cursor requires an update
    {
        paintBrush(*drawCursor);
    }
}

std::pair<uchar, uchar> UserImage::cellAt(QPointF p) const
{
    std::pair<uchar, uchar> xy;
    xy.first = static_cast<uchar>(p.x() / cellSize[0]);
    xy.second = static_cast<uchar>(p.y() / cellSize[1]);

    return xy;
}

void UserImage::reset()
{
    for (uchar i = 0; i < 100; ++i)
    {
        for (uchar j = 0; j < 200; ++j)
        {
            cells[0][i][j] = Cell();
        }
    }
    setFrame(0);
    renderComplete = false;
    emit editingStage();
}

void UserImage::setFrame(int f)
{
    currentFrame = f;
    repaint();
}

void UserImage::paintBrush(QPoint p)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    std::pair<uchar, uchar> xy = cellAt(p);

    QPointF topLeft;
    topLeft.setX(xy.first * cellSize[0]); // let's see how it goes with drawing outside borders
    topLeft.setY(xy.second * cellSize[1]);

    topLeft.setX(topLeft.x() - brushSize * cellSize[0]); // correction with the brush size
    topLeft.setY(topLeft.y() - brushSize * cellSize[1]);

    QSizeF size((2 * brushSize + 1) * cellSize[0], (2 * brushSize + 1)* cellSize[1]);

    painter.setBrush(Qt::transparent);

    painter.setPen(Qt::red); //let's use plain red

    painter.drawRect(QRectF(topLeft, size));
}

//EVENT HANDLING

bool UserImage::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
    { 
    case QEvent::Enter:
        setCursor(Qt::BlankCursor);
        setMouseTracking(true);
        break;
    case QEvent::Leave:
        setCursor(Qt::ArrowCursor);
        setMouseTracking(false);
        delete drawCursor;
        drawCursor = nullptr;
        draw = false;
        update();
        break;
    case QEvent::Paint:
        paintFrame();
    case QEvent::Resize:
    {
        cellSize[0] = this->width() / 200.;
        cellSize[1] = this->height() / 100.;
        break;
    }
    case QEvent::MouseMove:
        delete drawCursor;
        drawCursor = new QPoint(((QMouseEvent*)event)->pos());

        if(draw)
        {
            std::pair<uchar, uchar> from = cellAt(*drawCursor);
            for(short row = from.second - brushSize; row <= from.second + brushSize; ++row)
            {
                for(short col = from.first - brushSize; col <= from.first + brushSize; ++col)
                {
                    if (row < 0 || col < 0)
                        continue;
                    if (row >= 100 || col >= 200)
                        continue;

                    if(brushColor == 6)
                    {
                        if (source)
                            cells[0][source->first][source->second].isBurning = false;

                        cells[0][row][col].isBurning = true;
                        source = new std::pair<uchar, uchar>(row, col);
                    }
                    else
                        cells[0][row][col].type = brushColor;
                }
            }
        }

        update();
        break;
    case QEvent::MouseButtonPress:
    {
        if(renderComplete)
        {
            int Yes = QMessageBox::warning(this, tr("BzzLighter"),
                                           tr("The scenario is already rendered.\n"
                                              "Do you want to return to editing the landscape?"),
                                           QMessageBox::Yes,
                                           QMessageBox::No);
            if(Yes == QMessageBox::Yes)
            {
                renderComplete = false;
                cells[0][source->first][source->second].isBurning = false;
                delete source;
                currentFrame = 0;
                update();
                emit editingStage();
            }
            break;
        }
        draw = true;

        std::pair<uchar, uchar> from = cellAt(*drawCursor);

        for(short row = from.second - brushSize; row <= from.second + brushSize; ++row)
        {
            for(short col = from.first - brushSize; col <= from.first + brushSize; ++col)
            {
                if (row < 0 || col < 0)
                    continue;
                if (row >= 100 || col >= 200)
                    continue;

                if(brushColor == 6)
                {
                    if (source)
                    {
                        cells[0][source->first][source->second].isBurning = false;
                    }

                    cells[0][row][col].isBurning = true;
                    source = new std::pair<uchar, uchar>(row, col);
                }
                else
                    cells[0][row][col].type = brushColor;
            }
        }

        update();
        break;
    }
    case QEvent::MouseButtonRelease:
        draw = false;
    default:
        break;
    }

    return QFrame::eventFilter(obj, event);
}


//RENDERING
void UserImage::renderScenario()
{
    if(!source)
    {
        emit noIgnitionPoint();
        return;
    }

    createDistances(); // start by calculating the distances
    std::map<std::pair<uchar, uchar>, uchar> burning = {{*source, 1}}; // unordered sets cause an error so let's use maps with 3rd
                                                                       // argument being dt (transition time of fire)

    for(currentFrame = 0; currentFrame < 99; ++currentFrame)//time loop
    {
        std::cout << time(NULL) << std::endl;

        double windInfluences[8]; //calculate the wind influence coefficient first
        for(uchar i =0; i < 8; ++i)
        {
            windInfluences[i] = windInfluence(i);
        }

        std::vector<std::map<std::pair<uchar, uchar>, uchar>> adding;
        for(auto it = burning.begin(); it != burning.end();)
        {
            if(it->second == 1)
            {
                cells[currentFrame][it->first.first][it->first.second].isBurning = true;
                std::map<std::pair<uchar, uchar>, uchar> newBurn = igniteNeighbours(it->first.first, it->first.second, windInfluences);
                adding.push_back(newBurn);
            }
            if(it->second == 0)
            {
                cells[currentFrame][it->first.first][it->first.second].alreadyBurnt = true;
                it = burning.erase(it);
            }
            else if (it->second > 0)
            {
                it->second--;
                ++it;
            }
        }

        for(auto it = adding.begin(); it!=adding.end() ;++it)
        {
            for (auto ignited : *it)
            {
                if(burning.find(ignited.first) != burning.end())
                    burning[ignited.first] = std::min(burning[ignited.first], ignited.second);
                else
                    burning[ignited.first] = ignited.second;
            }
        }

        for (int i = 0; i < 100; ++i)
        {
            for (int j = 0; j < 200; ++j)
            {
                cells[currentFrame + 1][i][j] = cells[currentFrame][i][j]; // copy onto next frame
            }
        }

        noiseWind();
    }

    std::cout << "Render Complete\n";

    delete[] distances;
    currentFrame = 0;
    emit viewingStage();
    renderComplete = true;
}

std::map<std::pair<uchar, uchar>, uchar> UserImage::igniteNeighbours(const uchar& row, const uchar& col, double* wind)
{
    std::map<std::pair<uchar, uchar>, uchar> ignited;

    std::random_device rd;
    std::mt19937 gen(rd() + time(NULL) * row + col + currentFrame); //adding differernt seed changes to ger different results

    std::uniform_real_distribution<> probability(0, 1);

    double pComp, pReal;
    double nominalSpreadProbability;
    uchar nominalSpreadVelocity;

    //std::cout << std::fixed;
    //std::cout << std::setprecision(2);

//East:
    ++wind;

    if(col == 199 || cells[currentFrame][row][col + 1].isBurning || cells[currentFrame][row][col + 1].alreadyBurnt)
        goto NorthEast;

    pComp = probability(gen); //get new comparator to see if we ignite the point

    nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row][col + 1].type][cells[0][row][col].type];
    nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row][col + 1].type];

    pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

    //std::cout << "E: " << pComp ; // << " Wind: " << *wind;

    if(pComp < pReal)//ignition
        ignited[{row, col + 1}] = 1 + static_cast<uchar>(distances[row][col + 1] /
                ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1



NorthEast:
    ++wind;

    if(row == 0 || col == 199 || cells[currentFrame][row - 1][col + 1].isBurning || cells[currentFrame][row - 1][col + 1].alreadyBurnt)
        goto North;

    pComp = probability(gen); //get new comparator to see if we ignite the point

    nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row - 1][col + 1].type][cells[0][row][col].type];
    nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row - 1][col + 1].type];

    pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

    //std::cout << " NE: " << pComp ; // << " Wind: " << *wind;

    if(pComp < pReal)//ignition
        ignited[{row - 1, col + 1}] = 1 + static_cast<uchar>(distances[row - 1][col + 1] /
                ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1


North:
        if(row == 0 || cells[currentFrame][row - 1][col].isBurning || cells[currentFrame][row - 1][col].alreadyBurnt)
            goto NorthWest;

        pComp = probability(gen); //get new comparator to see if we ignite the point

        nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row - 1][col].type][cells[0][row][col].type];
        nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row - 1][col].type];

        pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

        //std::cout << " N: " << pComp ; // << " Wind: " << *wind;

        if(pComp < pReal)//ignition
            ignited[{row - 1, col}] = 1 + static_cast<uchar>(distances[row - 1][col] /
                    ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1

NorthWest:
    ++wind;

    if(row == 0 || col == 0 || cells[currentFrame][row - 1][col - 1].isBurning || cells[currentFrame][row - 1][col - 1].alreadyBurnt)
        goto West;

    pComp = probability(gen); //get new comparator to see if we ignite the point

    nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row - 1][col - 1].type][cells[0][row][col].type];
    nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row - 1][col - 1].type];

    pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

    //std::cout << " !!!NW: " << pComp ; // << " Wind: " << *wind;

    if(pComp < pReal)//ignition
        ignited[{row - 1, col - 1}] = 1 + static_cast<uchar>(distances[row - 1][col - 1] /
                ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1

West:
    ++wind;

    if(col == 0 || cells[currentFrame][row][col - 1].isBurning || cells[currentFrame][row][col - 1].alreadyBurnt)
        goto SouthWest;

    pComp = probability(gen); //get new comparator to see if we ignite the point

    nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row][col - 1].type][cells[0][row][col].type];
    nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row][col - 1].type];

    pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

    //std::cout << " W: " << pComp ; // << " Wind: " << *wind;


    if(pComp < pReal)//ignition
        ignited[{row, col - 1}] = 1 + static_cast<uchar>(distances[row][col - 1] /
                ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1

SouthWest:
    ++wind;

    if(row == 99 || col == 0 || cells[currentFrame][row + 1][col - 1].isBurning || cells[currentFrame][row + 1][col - 1].alreadyBurnt)
        goto South;

    pComp = probability(gen); //get new comparator to see if we ignite the point

    nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row + 1][col - 1].type][cells[0][row][col].type];
    nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row + 1][col - 1].type];

    pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

    //std::cout << " SW: " << pComp ; // << " Wind: " << *wind;

    if(pComp < pReal)//ignition
        ignited[{row + 1, col - 1}] = 1 + static_cast<uchar>(distances[row + 1][col - 1] /
                ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1


South:
    ++wind;

    if(row == 99 || cells[currentFrame][row + 1][col].isBurning || cells[currentFrame][row + 1][col].alreadyBurnt)
        goto SouthEast;

    pComp = probability(gen); //get new comparator to see if we ignite the point

    nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row + 1][col].type][cells[0][row][col].type];
    nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row + 1][col].type];

    pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

    //std::cout << " S: " << pComp ; // << " Wind: " << *wind;

    if(pComp < pReal)//ignition
        ignited[{row + 1, col}] = 1 + static_cast<uchar>(distances[row + 1][col] /
                ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1

SouthEast:
    ++wind;

    if(row == 99 || col == 199 || cells[currentFrame][row + 1][col + 1].isBurning || cells[currentFrame][row + 1][col + 1].alreadyBurnt)
        goto Completed;

    pComp = probability(gen); //get new comparator to see if we ignite the point

    nominalSpreadProbability = Cell::fireSpreadProb[cells[0][row + 1][col + 1].type][cells[0][row][col].type];
    nominalSpreadVelocity = Cell::nominalSpreadV[cells[0][row + 1][col + 1].type];

    pReal = (1 - pow( 1 - nominalSpreadProbability, *wind)) * 1.1;

    //std::cout << " !!!SE: " << pComp; // << " Wind: " << *wind;

    if(pComp < pReal)//ignitionn
        ignited[{row + 1, col + 1}] = 1 + static_cast<uchar>(distances[row + 1][col + 1] /
                ( (nominalSpreadVelocity * pow(M_E, 0.1783 * windSpeed)) * (pow(M_E, -0.014 * 0.1)) )); // Moisture for now will be 0.1

Completed:
    //std::cout << std::endl;
    return ignited;
}

void UserImage::noiseWind()
{
    std::random_device rd;
    std::mt19937 gen(rd() + time(NULL) + static_cast<ushort>(windDirection));

    std::uniform_real_distribution<> disSpeed(0.8, 1.2);
    std::uniform_real_distribution<> disAngle(-11.25, 11.25);

    windSpeed *= disSpeed(gen);
    windDirection += disAngle(gen);

    if(windSpeed > 100) // hard cap
        windSpeed = 100;

    if(windDirection >= 360) // keep inside 0 < x < 2pi
        windDirection -= 360;
    if(windDirection < 0)
        windDirection += 360;
}

double func(double x)
{
    if (x < 0.7)
        return pow(3, 10 * x - 6);
    return pow(10 * x - 6, 2);
}

double UserImage::windInfluence(uchar dir) // we make a similar function of wind influence based on the graph we have
{
    if (windSpeed == 0)
        return 1;
    double add = windSpeed / 100;
    double divAdd = 1 + func(1 - windSpeed / 100);
    return add + 4 / (0.5 * abs(dir * 45 - windDirection) + divAdd);
}



void UserImage::createDistances()
{
    distances = new double*[100];
    for (int i = 0; i < 100; ++i)
    {
        distances[i] = new double[200];
        for (int j = 0; j < 200; ++j)
        {
            distances[i][j] = sqrt(pow(source->first - i, 2) + pow(source->second - j, 2));
        }
    }
}

void UserImage::returnToEditing()
{
    renderComplete = false;

    if(source)
        cells[0][source->first][source->second].isBurning = false;
    delete source;

    currentFrame = 0;
    update();
    emit editingStage();
}

