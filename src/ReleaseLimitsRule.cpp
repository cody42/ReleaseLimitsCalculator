#include "ReleaseLimitsRule.h"

#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>

OutputValueWidget::OutputValueWidget(const QString& title, QWidget *parent) 
	: QWidget(parent) {
	mainLayout = new QGridLayout();
	
	labelTitle = new QLabel(title);
	labelUnitGL = new QLabel("g/l");
	labelUnitWW = new QLabel("% w/w");
	editValueGL = new QLineEdit();
	editValueGL->setReadOnly(true);
	editValueGL->setFocusPolicy(Qt::FocusPolicy::NoFocus);
	editValueWW = new QLineEdit();
	editValueWW->setReadOnly(true);
	editValueWW->setFocusPolicy(Qt::FocusPolicy::NoFocus);
	
	mainLayout->addWidget(editValueGL, 0, 0);
	mainLayout->addWidget(editValueWW, 1, 0);

	mainLayout->addWidget(labelUnitGL, 0, 1);
	mainLayout->addWidget(labelUnitWW, 1, 1);

	mainLayout->addWidget(labelTitle, 3, 0, 1, 2);

	this->setLayout(mainLayout);
}
OutputValueWidget::~OutputValueWidget(void) {
}

void OutputValueWidget::setGL(float value) {
	QString string;
	string.setNum(value, 'f', 2);
	this->editValueGL->setText(string);
}
void OutputValueWidget::setWW(float value) {
	QString string;
	string.setNum(value, 'f', 2);
	this->editValueWW->setText(string);
}
void OutputValueWidget::reset() {
	this->editValueGL->clear();
	this->editValueWW->clear();
}

ReleaseLimitsRule::ReleaseLimitsRule(const QString &name,
									 const QString &info,
									 OutputValueWidgetVector *outputWidgets,
									 const ToleranceFunction &f,
									 QWidget *parent)
	:QGroupBox(parent), outputWidgets(outputWidgets), calculateValue(f), info(new QString(info)) {
		
	QFont font = this->font();
	QFont bigFont = font;
	font.setPointSize(8);
	bigFont.setPointSize(11);
	this->setFont(bigFont);

	mainLayout = new QHBoxLayout();
	for(auto it = this->outputWidgets->begin(); it != this->outputWidgets->end(); ++it) {
		(*it)->setFont(font);
		mainLayout->addWidget(*it);
	}

	this->setLayout(mainLayout);
	this->setTitle(name);
	calculateValue = f;
}


ReleaseLimitsRule::~ReleaseLimitsRule(void) {
	delete this->outputWidgets;
	delete this->info;
}

void ReleaseLimitsRule::update(float declared, float density, bool percentWW, bool homogenous) {
	//this->outputWidgets->at(0)->setWW(declared);
	//this->outputWidgets->at(0)->setGL(density);
	if(percentWW) {
		std::vector<float> valuesWW = this->calculateValue(declared*10.f, homogenous);

		for(size_t i = 0; i < this->outputWidgets->size(); ++i) {
			this->outputWidgets->at(i)->reset();
			if(i < valuesWW.size()) {
				this->outputWidgets->at(i)->setWW(valuesWW[i]/10.f);
			}
		}
	} else {
		std::vector<float> valuesGL = this->calculateValue(declared, homogenous);
		
		for(size_t i = 0; i < this->outputWidgets->size(); ++i) {
			this->outputWidgets->at(i)->reset();
			if(i < valuesGL.size()) {
				this->outputWidgets->at(i)->setGL(valuesGL[i]);
				if(density > 0) {
					this->outputWidgets->at(i)->setWW(valuesGL[i]/density);
				}
			}
		}
	}
}

void ReleaseLimitsRule::reset() {
	for(auto it = this->outputWidgets->begin(); it != this->outputWidgets->end(); ++it) {
		(*it)->reset();
	}
}

enum TriState : char {
	FALSE = 0,
	TRUE = 1,
	DC = -1 //don't care
};

struct RuleLimit {
	bool catch_all;
	float lte;
	float factor;
	float absolute;
	TriState homogenous;
};

typedef std::vector<RuleLimit> LimitsVector;

ReleaseLimitsRule* ReleaseLimitsRuleBuilder::createFromJson(QJsonObject &obj) {
	if(!obj["name"].isString()) {
		throw json_error("Key \"name\" is not a string or does not exist.");
	}
	this->name(obj["name"].toString());

	if(!obj["outputs"].isArray()) {
		throw json_error("Key \"outputs\" is not an array or does not exist.");
	}
	std::vector<float> outputs;
	QJsonArray joutputs = obj["outputs"].toArray();
	for(auto it = joutputs.begin(); it != joutputs.end(); ++it) {
		if(!(*it).isObject()) {
			throw json_error(QString("Output #%1 is not an object.")
				.arg(it - joutputs.begin()));
		}
		QJsonObject output = (*it).toObject();
		if(!output["title"].isString()) {
			throw json_error(QString("Key \"title\" on output #%1 is missing or not a string.")
				.arg(it - joutputs.begin()));
		}
		if(!output["offset"].isDouble()) {
			throw json_error(QString("Key \"offset\" on output #%1 is missing or not a string.")
				.arg(it - joutputs.begin()));
		}
		this->addValue(output["title"].toString());
		outputs.push_back(static_cast<float>(output["offset"].toDouble()));
	}

	if(!obj["limits"].isArray()) {
		throw json_error("Key \"limits\" is not an array or does not exist.");
	}
	LimitsVector limits;
	QJsonArray jlimits = obj["limits"].toArray();
	for(auto it = jlimits.begin(); it != jlimits.end(); ++it) {
		if(!(*it).isObject()) {
			throw json_error(QString("Elemet #%1 of \"limits\" is not an object.")
				.arg(it - jlimits.begin()));
		}
		QJsonObject jlimit = (*it).toObject();
		RuleLimit limit;
		if(!(jlimit["absolute"].isDouble() || jlimit["percent"].isDouble())) {
			throw json_error(QString("Elemet #%1 of \"limits\" has neither a \"percent\" nor an \"absolute\" value or neither is a double.")
				.arg(it - jlimits.begin()));
		}
		limit.factor = jlimit["percent"].isDouble() ? static_cast<float>(jlimit["percent"].toDouble()) / 100.f : 0.f;
		limit.absolute = jlimit["absolute"].isDouble() ? static_cast<float>(jlimit["absolute"].toDouble()) : 0.f;

		if(!jlimit["lte"].isDouble()) {
			limit.catch_all = true;
			limit.lte = 0.f;
		} else {
			limit.catch_all = false;
			limit.lte = static_cast<float>(jlimit["lte"].toDouble());
		}

		limit.homogenous = TriState::DC;
		if(jlimit["homogenous"].isBool() && jlimit["homogenous"].toBool()) {
			limit.homogenous = TriState::TRUE;
		}
		if(jlimit["heterogenous"].isBool() && jlimit["heterogenous"].toBool()) {
			limit.homogenous = limit.homogenous == TriState::TRUE ? TriState::DC : TriState::FALSE;
		}

		limits.push_back(limit);
	}

	if(obj["info"].isString()) {
		this->info(obj["info"].toString());
	}

	return this->create([outputs, limits](float declared, bool homogenous) {
		std::vector<float> values(outputs.size(), declared);

		for(auto it = limits.begin(); it != limits.end(); ++it) {
			if(it->catch_all ||
				(declared <= it->lte && 
					(it->homogenous == TriState::DC
					|| (homogenous && it->homogenous == TriState::TRUE)
					|| (!homogenous && it->homogenous == TriState::FALSE))) 
				) {
				float tolerance = it->absolute + declared * it->factor;
				for(size_t i = 0; i < values.size(); ++i) {
					values[i] += tolerance * outputs[i];
				}
				break;
			}
		}

		return values;
	});
}