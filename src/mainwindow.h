#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <vector>

#include "ReleaseLimitsRule.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	typedef std::vector<ReleaseLimitsRule*> RuleVector;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
	void calculateReleaseLimits();
	void clearAll();

	void displayInfo();
	void displayAbout();
private:
    Ui::MainWindow *ui;
	RuleVector *rules;
};

#endif // MAINWINDOW_H
