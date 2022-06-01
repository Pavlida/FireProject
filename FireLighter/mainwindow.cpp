#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, UserImage* w)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , view(w)
{
    ui->setupUi(this);

    //color everything in a cortrect way =)
    ui->menubar->setStyleSheet("background-color: rgb(245, 194, 144);");

    view->setMinimumSize(QSize(200, 100));
    ui->mainVL->insertWidget(0, view);


    //play manager icons
    ui->PlayPayse->setIcon(QIcon("icons/PlayButton.png"));
    ui->StepB->setIcon(QIcon("icons/StepBackward.png"));
    ui->StepF->setIcon(QIcon("icons/StepForward.png"));

    connect(view, &UserImage::pressedCoords, this, &MainWindow::outputStatusBar); // outputing in status bar for testing

    //brush color selection
    brushColor = new QButtonGroup;
    brushColor->addButton(ui->BrushColor1);
    brushColor->addButton(ui->BrushColor2);
    brushColor->addButton(ui->BrushColor3);
    brushColor->addButton(ui->BrushColor4);
    brushColor->addButton(ui->BrushColor5);
    brushColor->addButton(ui->BrushColor6);
    brushColor->addButton(ui->SourceButton);

    connect(brushColor, &QButtonGroup::idClicked, view, &UserImage::setBrushColor);

    connect(view, &UserImage::editingStage, this, [this](){ // editing limits moving through timeline
        ui->timeSlider->setMaximum(0);
        ui->dial->setEnabled(true);
        ui->WindSpeed->setReadOnly(false);
    });
    connect(view, &UserImage::viewingStage, this, [this](){
        ui->timeSlider->setMaximum(99);
        ui->dial->setEnabled(false);
        ui->WindSpeed->setReadOnly(true);
    });

    connect(ui->SourceButton, &QPushButton::toggled, this, [this](bool checked){ (checked) ?
                    ui->BrushSize->setMaximum(1) : ui->BrushSize->setMaximum(5);}); // if checked we set max brush to 1 else we return to default
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::outputStatusBar(QString text)
{
    ui->statusbar->showMessage(text);
}

void MainWindow::on_PlayPayse_clicked()
{
    if(!view->source)
    {
        QMessageBox::information(this, tr("BeLighter"), tr("Please select an ignition point first"));
        return;
    }

    if(notRenderedError())
        return;

    if (isPlaying)
    {
        ui->PlayPayse->setIcon(QIcon("icons/PlayButton.png"));
    }
    else
    {
        ui->PlayPayse->setIcon(QIcon("icons/PauseButton.png"));
    }
    isPlaying = !isPlaying;
}

void MainWindow::on_RenderButton_clicked()
{
    view->renderScenario();
}

void MainWindow::on_timeSlider_sliderPressed()
{
    notRenderedError();
}

void MainWindow::on_timeSlider_valueChanged(int value)
{
    view->setFrame(value);
}

bool MainWindow::notRenderedError()
{
    if(!view->isRenderComplete())
    {
        bool No = QMessageBox::warning(this, tr("BeLighter"),
                                       tr("The scenario wasn't rendered yet.\n"
                                          "Do you want to do it now?"),
                                       QMessageBox::Yes,
                                       QMessageBox::No);
        if(!No)
            view->renderScenario();

        return true;
    }

    return false;
}

void MainWindow::on_BrushSize_valueChanged(int arg1)
{
    view->setBrushSize(arg1);
}

void MainWindow::on_ResetButton_clicked()
{
    view->reset();
}

void MainWindow::on_dial_valueChanged(int value)
{
    view->setWindDir(value);
}

void MainWindow::on_WindSpeed_valueChanged(double arg1)
{
    view->setWindSpeed(arg1);
}
