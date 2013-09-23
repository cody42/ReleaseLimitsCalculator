#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <exception>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSpacerItem>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qfile.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	this->rules = new RuleVector();

	ReleaseLimitsRuleBuilder ruleBuilder;

	//load rules from file
	QFile *ruleFile = new QFile("rules.json");
	if(!ruleFile->open(QIODevice::ReadOnly)) {
		QMessageBox::critical(this, "Error", "The configuration file rules.json was not found.");
		qApp->quit();
	}
	QJsonParseError jerr;
	QJsonDocument rulesJsonDoc = QJsonDocument::fromJson(ruleFile->readAll(), &jerr);
	ruleFile->close();
	delete ruleFile;
	if(jerr.error != QJsonParseError::NoError) {
		QMessageBox::critical(this, "Error",
			QString("Error while parising the configuration file rules.json.\n%1").arg(jerr.errorString()));
		qApp->quit();
	}
	if(!rulesJsonDoc.isArray()) {
		QMessageBox::critical(this, "Error",
			"Error while parising the configuration file rules.json.\nTop level element is not an array.");
		qApp->quit();
	}
	QJsonArray arr = rulesJsonDoc.array();
	for(auto it = arr.begin(); it != arr.end(); ++it) {
		if(!(*it).isObject()) {
			QMessageBox::warning(this, "Erroneous Configuration",
				QString("The configuration file rules.json has errors.\n"
				"Skipping rule #%1.\nArray element is not an object.").arg(it - arr.begin()));
			break;
		}
		try {
			this->rules->push_back(ruleBuilder.createFromJson((*it).toObject()));
		} catch (ReleaseLimitsRuleBuilder::json_error &e) {
			QMessageBox::warning(this, "Erroneous Configuration",
				QString("The configuration file rules.json has errors.\n"
				"Skipping rule #%1.\n%2").arg(it - arr.begin()).arg(e.qwhat()));
			ruleBuilder.reset();
		}
	}

	{
		auto it = this->rules->begin();
		while(it != this->rules->end()) {
			this->ui->boxResults->layout()->addWidget(*it);
			++it;
			if(it != this->rules->end()) {
				this->ui->boxResults->layout()->addItem(new QSpacerItem(0,10, QSizePolicy::Minimum, QSizePolicy::Minimum));
			}
		}
	}
	this->ui->boxResults->layout()->addItem(new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::Expanding));

	QObject::connect(this->ui->btnCalculate, SIGNAL(clicked()), this, SLOT(calculateReleaseLimits()));
	QObject::connect(this->ui->btnClear, SIGNAL(clicked()), this, SLOT(clearAll()));
	
	QObject::connect(this->ui->actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
	QObject::connect(this->ui->actionInfo, SIGNAL(triggered()), this, SLOT(displayInfo()));
	QObject::connect(this->ui->actionAbout, SIGNAL(triggered()), this, SLOT(displayAbout()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::calculateReleaseLimits() {
	try {
		bool no_error;

		if(this->ui->editDeclaredContent->text().isEmpty()) {
			throw std::runtime_error("The field 'declared value' must not be left blank.");
		}

		QString declaredStringValue = this->ui->editDeclaredContent->text();
		declaredStringValue.replace(',', '.');
		float declaredValue = declaredStringValue.toFloat(&no_error);
		if(!no_error) {
			throw std::runtime_error("The declared value has to be a number.\nPoint and comma may be used as decimal separator.");
		}

		QString densityStringValue = this->ui->editDensity->text();
		densityStringValue.replace(',', '.');
		float density = densityStringValue.toFloat(&no_error);
		if(!no_error && !densityStringValue.isEmpty()) {
			QMessageBox::warning(this, "Invalid Values", "The provided density could not be interpredet as number and will be ignored.\n\nPlease enter a valid number or leave the field blank. Point and comma may be used as decimal separator.");
		}

		bool percentWW = this->ui->rPercentWW->isChecked();
#ifndef NDEBUG
		if(percentWW && this->ui->rGrammsPerLiter->isChecked()) throw std::logic_error("Radio buttons 'g/l' and '% w/w' are checked simultaniously.");
#endif
		bool homogenous = this->ui->rHomogenous->isChecked();
#ifndef NDEBUG
		if(homogenous && this->ui->rHeterogenous->isChecked()) throw std::logic_error("Radio buttons 'homogenous' and 'heterogenous' are checked simultaniously.");
#endif

		for(auto it = this->rules->begin(); it != this->rules->end(); ++it) {
			(*it)->update(declaredValue, density, percentWW, homogenous);
		}
	} catch(std::runtime_error &e) {
		QMessageBox::critical(this, "Invalid Values", e.what());
	} catch(std::logic_error &e) {
		QString msg("The program encountered an internal error.\n\nError-Message:\n%1");
		msg.arg(e.what());
		QMessageBox::critical(this, "Error", msg);
		qApp->quit();
	}
}

void MainWindow::clearAll() {
	this->ui->editDeclaredContent->clear();
	this->ui->editDensity->clear();
	this->ui->rGrammsPerLiter->setChecked(true);
	this->ui->rHomogenous->setChecked(true);
	for(auto it = this->rules->begin(); it != this->rules->end(); ++it) {
		(*it)->reset();
	}
}

void MainWindow::displayInfo() {
	QString info;

	{
		auto it = this->rules->begin();
		while(it != this->rules->end()) {
			info.append((*it)->getInfo());
			++it;
			if(it != this->rules->end()) {
				info.append("\n");
			}
		}
	}

	QMessageBox::information(this, "Info",
		info.arg(QChar(0xB1)).arg(QChar(0xB0)));
}
void MainWindow::displayAbout() {
	QString msg = QString("Written by David Korzeniewski, %1 2013\n\n"
		"Released under GNU GPLv3\nwww.gnu.org/licenses/gpl-3.0.html\n\n"
		"Source code available at:\nhttps://github.com/cody42/ReleaseLimitsCalculator").arg(QChar(0xA9));
	QMessageBox::about(this, "About Release Limits Calculator", msg);
}
