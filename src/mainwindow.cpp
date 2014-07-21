#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <exception>
#include <QMessageBox>
#include <QSpacerItem>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qfile.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
	settingsDialog(nullptr)
{
	this->settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Cody-Films", "ReleaseLimitsCalculator");

    ui->setupUi(this);

	settings->beginGroup("MainWindow");
	resize(settings->value("size", this->size()).toSize());
	move(settings->value("pos", this->pos()).toPoint());
	settings->endGroup();

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
		bool ok;
		unsigned int precision = this->settings->value("precision", 2).toUInt(&ok);
		if(!ok) {
			precision = 2;
			this->settings->setValue("precision", 2);
		}
		for(auto it = this->rules->begin(); it != this->rules->end(); ++it) {
			(*it)->updatePrecision(precision);
		}
	}

	QStringList hidden = this->settings->value("hidden", "").toString().split(",");
	this->displayRules(hidden);

	QObject::connect(this->ui->btnCalculate, SIGNAL(clicked()), this, SLOT(calculateReleaseLimits()));
	QObject::connect(this->ui->btnClear, SIGNAL(clicked()), this, SLOT(clearAll()));
	
	QObject::connect(this->ui->actionSettings, SIGNAL(triggered()), this, SLOT(displaySettings()));
	QObject::connect(this->ui->actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
	QObject::connect(this->ui->actionInfo, SIGNAL(triggered()), this, SLOT(displayInfo()));
	QObject::connect(this->ui->actionAbout, SIGNAL(triggered()), this, SLOT(displayAbout()));
}

void MainWindow::displayRules(std::map<QString, bool> settings) {
	QStringList hidden;

	for(auto iter = settings.begin(); iter != settings.end(); ++iter) {
		if(!iter->second) {
			hidden.append(iter->first);
		}
	}
	this->displayRules(hidden);
}

void MainWindow::displayRules(QStringList hidden) {
	for(auto iter = this->rules->begin(); iter != this->rules->end(); ++iter) {
		this->ui->boxResults->layout()->removeWidget(*iter);
		(*iter)->setParent(nullptr);
	}

	QLayoutItem *child;
	while((child = this->ui->boxResults->layout()->takeAt(0)) != 0) {
		delete child;
	}

	auto it = this->rules->begin();
	bool first = true;
	while(it != this->rules->end()) {
		if(!hidden.contains((*it)->getName())) {
			if(!first) {
				this->ui->boxResults->layout()->addItem(new QSpacerItem(0,10, QSizePolicy::Minimum, QSizePolicy::Minimum));
			}
			this->ui->boxResults->layout()->addWidget(*it);
			first = false;
		}
		++it;
	}
	this->ui->boxResults->layout()->addItem(new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

MainWindow::~MainWindow() {
	settings->beginGroup("MainWindow");
	settings->setValue("size", size());
	settings->setValue("pos", pos());
	settings->endGroup();

	if(settingsDialog != nullptr) {
		delete settingsDialog;
	}
    delete ui;
	delete settings;
}

void MainWindow::calculateReleaseLimits() {
	try {
		bool no_error;

		if(this->ui->editDeclaredContent->text().isEmpty()) {
			throw std::runtime_error("The field 'declared value' must not be left blank.");
		}

		QString declaredStringValue = this->ui->editDeclaredContent->text();
		declaredStringValue.replace(',', '.');
		double declaredValue = declaredStringValue.toDouble(&no_error);
		if(!no_error) {
			throw std::runtime_error("The declared value has to be a number.\nPoint and comma may be used as decimal separator.");
		}

		QString densityStringValue = this->ui->editDensity->text();
		densityStringValue.replace(',', '.');
		double density = densityStringValue.toDouble(&no_error);
		if(!no_error) {
			this->ui->editDensity->setText("1.00");
			density = 1.f;
		}
		if(density <= 0) {
			throw std::runtime_error("The density must be a positive value.");
		}


		bool percentWW = this->ui->rPercentWW->isChecked();
#ifndef NDEBUG
		if(percentWW && this->ui->rGrammsPerLiter->isChecked()) throw std::logic_error("Radio buttons 'g/l' and '% w/w' are checked simultaniously.");
#endif
		bool homogenous = this->ui->rHomogenous->isChecked();
#ifndef NDEBUG
		if(homogenous && this->ui->rHeterogenous->isChecked()) throw std::logic_error("Radio buttons 'homogenous' and 'heterogenous' are checked simultaniously.");
#endif
		
		ratio declared = ratio(declaredValue, percentWW ? Unit::PERCENT_WW : Unit::g_per_l);
		for(auto it = this->rules->begin(); it != this->rules->end(); ++it) {
			(*it)->update(declared, density, homogenous);
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
	QString info("To calculate the release limits specify all values in the input area, then click 'Calculate Release Limits'. "
		"To update the calculation change any value in the input area, then click 'Calculate Release Limits' to update the output values. "
		"If the density is not specified, 1.00g/ml will be used!\n\n"
		"All calculations are performed at a precision of 6 to 9 digits. Output values are rounded to two decimal places.\n\n");

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
		info.arg(QChar(0xB1)).arg(QChar(0xB0)).arg(QChar(0x2265)));
}
void MainWindow::displayAbout() {
	QString msg = QString("Version 1.1 (2014-07-21)\n\n"
		"Written by David Korzeniewski, %1 2013\n\n"
		"Released under GNU GPLv3\nwww.gnu.org/licenses/gpl-3.0.html\n\n"
		"Source code available at:\nhttps://github.com/cody42/ReleaseLimitsCalculator").arg(QChar(0xA9));
	QMessageBox::about(this, "About Release Limits Calculator", msg);
}

void MainWindow::displaySettings() {
	std::map<QString, bool> currentSettings;
	
	QStringList hidden = this->settings->value("hidden", "").toString().split(",");

	for(auto it = this->rules->begin(); it != this->rules->end(); ++it) {
		currentSettings.insert(std::make_pair(QString((*it)->getName()), !hidden.contains((*it)->getName())));
	}

	
	bool ok;
	unsigned int precision = this->settings->value("precision", 2).toUInt(&ok);
	if(!ok) {
		precision = 2;
		this->settings->setValue("precision", 2);
	}
	for(auto it = this->rules->begin(); it != this->rules->end(); ++it) {
		(*it)->updatePrecision(precision);
	}
	
	this->settingsDialog = new SettingsDialog(std::move(currentSettings), precision);
	this->settingsDialog->setModal(true);
	this->settingsDialog->setWindowTitle("Settings");
	connect(this->settingsDialog, SIGNAL(accepted()), this, SLOT(applySettings()));
	this->settingsDialog->show();
}

void MainWindow::applySettings() {
	if(this->settingsDialog !=nullptr) {
		auto map = this->settingsDialog->getShowHideMap();
		QStringList hidden;

		for(auto iter = map.begin(); iter != map.end(); ++iter) {
			if(!iter->second) {
				hidden.append(iter->first);
			}
		}
		this->settings->setValue("hidden", hidden.join(','));
		this->displayRules(hidden);

		
		unsigned int precision = this->settingsDialog->getPrecisionSetting();
		this->settings->setValue("precision", precision);
		
		for(auto it = this->rules->begin(); it != this->rules->end(); ++it) {
			(*it)->updatePrecision(precision);
		}
		
		delete this->settingsDialog;
		this->settingsDialog = nullptr;
	}
}