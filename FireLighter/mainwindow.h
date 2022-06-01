#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QMessageBox>
#include <QButtonGroup>
#include "userimage.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, UserImage* w = nullptr);
    ~MainWindow();

private slots:
    void outputStatusBar(QString text);

    void on_PlayPayse_clicked();

    void on_RenderButton_clicked();

    void on_timeSlider_sliderPressed();

    void on_timeSlider_valueChanged(int value);

    void on_BrushSize_valueChanged(int arg1);

    void on_ResetButton_clicked();

    void on_dial_valueChanged(int value);

    void on_WindSpeed_valueChanged(double arg1);

private:
    Ui::MainWindow *ui;
    UserImage* view;
    QButtonGroup* brushColor;
    bool isPlaying = false;

    bool notRenderedError();
};
#endif // MAINWINDOW_H
