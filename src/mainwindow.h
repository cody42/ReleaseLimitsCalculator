#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/qsettings.h>
#include <vector>

#include "ReleaseLimitsRule.h"
#include "SettingsDialog.h"

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
	void applySettings();

	void displayInfo();
	void displayAbout();
	void displaySettings();
protected:
	void displayRules(std::map<QString, bool> settings);
	void displayRules(QStringList hidden);
private:
    Ui::MainWindow *ui;
	RuleVector *rules;
	SettingsDialog *settingsDialog;
	QSettings *settings;
};

#endif // MAINWINDOW_H
