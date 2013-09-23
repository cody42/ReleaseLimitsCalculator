#ifndef _RELEASELIMITSCALCULATOR_RELEASELIMITSRULE_H_
#define _RELEASELIMITSCALCULATOR_RELEASELIMITSRULE_H_

#include <QtWidgets/qwidget.h>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <qstring.h>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <qjsonobject.h>
#include <exception>

#include <vector>
#include <functional>

class OutputValueWidget :public QWidget {
	Q_OBJECT
public:

	OutputValueWidget(const QString& title, QWidget *parent = 0);
	virtual ~OutputValueWidget(void);

	void setGL(float value);
	void setWW(float value);
	void reset();
private:
	QGridLayout *mainLayout;
	QLabel *labelTitle;
	QLabel *labelUnitGL;
	QLabel *labelUnitWW;
	QLineEdit *editValueGL;
	QLineEdit *editValueWW;
};

class ReleaseLimitsRule : public QGroupBox {
	Q_OBJECT
public:
	/** Type of the calculation function
	The first parameter is the declared value. If the second value is true, use homogenous ruleset,
	else use heterogenous rule set
	*/
	typedef std::function<std::vector<float>(float, bool)> ToleranceFunction;
	typedef std::vector<OutputValueWidget*> OutputValueWidgetVector;

	ReleaseLimitsRule(const QString &name, const QString &info, OutputValueWidgetVector *outputWidgets, const ToleranceFunction &f, QWidget *parent = 0);
	virtual ~ReleaseLimitsRule(void);

	/** Calculate limit and set line edits accordingly
	\param declared The declared value
	\param density the density of the declared content. 0 means not density provided.
	\param percentWW if true the parameter declared has the unit % w/w, else it has the unit g/l
	\param homogenous if true calculate for homogenous else for heterogenous
	*/
	void update(float declared, float density, bool percentWW, bool homogenous);
	void reset();

	QString getInfo() {return *this->info;}
protected:
	QHBoxLayout *mainLayout;

	OutputValueWidgetVector *outputWidgets;
	ToleranceFunction calculateValue;
	const QString *info;
};

class ReleaseLimitsRuleBuilder {
public:
	ReleaseLimitsRuleBuilder(void) : outputWidgets(new ReleaseLimitsRule::OutputValueWidgetVector()){};
	~ReleaseLimitsRuleBuilder(void) {if(outputWidgets != nullptr) delete outputWidgets;};

	ReleaseLimitsRuleBuilder& reset(void) {
		if(outputWidgets != nullptr) delete outputWidgets;
		this->outputWidgets = new ReleaseLimitsRule::OutputValueWidgetVector();
		this->nameString = QString();
		this->infoString = QString();
		return *this;
	}
	
	ReleaseLimitsRuleBuilder& name(const QString & name) {
		this->nameString = name;
		return *this;
	}
	ReleaseLimitsRuleBuilder& info(const QString & info) {
		this->infoString = info;
		return *this;
	}

	ReleaseLimitsRuleBuilder& addValue(const QString & title) {
		OutputValueWidget* w = new OutputValueWidget(title);
		this->outputWidgets->push_back(w);

		return *this;
	};
	ReleaseLimitsRule* create(const ReleaseLimitsRule::ToleranceFunction &f) {
		ReleaseLimitsRule* instance = new ReleaseLimitsRule(this->nameString, this->infoString, this->outputWidgets, f);
		outputWidgets = nullptr;
		this->reset();
		return instance;
	};
	ReleaseLimitsRule* createFromJson(QJsonObject &obj);

	struct json_error : public std::exception {
		json_error(const char* msg) : msg (msg) {}
		json_error(QString msg) : msg (msg) {}
		QString msg;
		QString qwhat() {return this->msg;}
		const char* what() {return this->msg.toStdString().c_str();}
	};
private:
	ReleaseLimitsRule::OutputValueWidgetVector *outputWidgets;
	QString nameString;
	QString infoString;
};

#endif //_RELEASELIMITSCALCULATOR_RELEASELIMITSRULE_H_
