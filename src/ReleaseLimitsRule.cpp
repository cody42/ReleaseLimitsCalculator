#include "ReleaseLimitsRule.h"

#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>

OutputValueWidget::OutputValueWidget(const QString& title, QWidget *parent) 
	: QWidget(parent)
	, precision(2) {
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

void OutputValueWidget::setGL(double value) {
	this->valueGL = std::make_pair(true, value);
	QString string;
	string.setNum(value, 'f', precision);
	this->editValueGL->setText(string);
}
void OutputValueWidget::setWW(double value) {
	this->valueWW = std::make_pair(true, value);
	QString string;
	string.setNum(value, 'f', precision);
	this->editValueWW->setText(string);
}

void OutputValueWidget::updatePrecision(unsigned int precision) {
	this->precision = precision;
	if(this->valueGL.first) {
		QString string;
		string.setNum(this->valueGL.second, 'f', precision);
		this->editValueGL->setText(string);
	}
	if(this->valueWW.first) {
		QString string;
		string.setNum(this->valueWW.second, 'f', precision);
		this->editValueWW->setText(string);
	}

}

void OutputValueWidget::reset() {
	this->editValueGL->clear();
	this->editValueWW->clear();
	this->valueGL.first = false;
	this->valueWW.first = false;
}

ReleaseLimitsRule::ReleaseLimitsRule(const QString &name,
									 const QString &info,
									 OutputValueWidgetVector *outputWidgets,
									 const ToleranceFunction &f,
									 QWidget *parent)
	:QGroupBox(parent), outputWidgets(outputWidgets), calculateValue(f), info(new QString(info)), name(name) {
		
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

void ReleaseLimitsRule::update(ratio declared, double density, bool homogenous) {
	//this->outputWidgets->at(0)->setWW(declared);
	//this->outputWidgets->at(0)->setGL(density);
	std::vector<ratio> values = this->calculateValue(declared, density, homogenous);

	for(size_t i = 0; i < this->outputWidgets->size(); ++i) {
		this->outputWidgets->at(i)->reset();
		if(i < values.size()) {
			this->outputWidgets->at(i)->setGL(values[i].g_l(density));
			this->outputWidgets->at(i)->setWW(values[i].w_w(density));
		}
	}
}

void ReleaseLimitsRule::updatePrecision(unsigned int precision) {
	for(size_t i = 0; i < this->outputWidgets->size(); ++i) {
		this->outputWidgets->at(i)->updatePrecision(precision);
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
	bool thresh_inclusive;
	double threshold;
	double factor[2];
	double absolute[2];
	TriState homogenous;
};

typedef std::vector<RuleLimit> LimitsVector;

ReleaseLimitsRule* ReleaseLimitsRuleBuilder::createFromJson(QJsonObject &obj) {
	if(!obj["name"].isString()) {
		throw json_error("Key \"name\" is not a string or does not exist.");
	}
	this->name(obj["name"].toString());
	
	if(!obj["unit"].isString()) {
		throw json_error("Key \"unit\" is not a string or does not exist.");
	}
	QString unit_string(obj["unit"].toString());
	Unit unit = Unit::INVALID;
	if(unit_string == "g/l") {
		unit = Unit::g_per_l;
	} else if(unit_string == "%w/w") {
		unit = Unit::PERCENT_WW;
	}
	if (unit == Unit::INVALID){
		throw json_error("\"unit\" must be exactly \"g/l\" or \"%w/w\".");
	}

	if(!obj["outputs"].isArray()) {
		throw json_error("Key \"outputs\" is not an array or does not exist.");
	}
	std::vector<double> outputs;
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
		outputs.push_back(output["offset"].toDouble());
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
		
		bool absIsValuePair = jlimit["absolute"].isObject()
			&& jlimit["absolute"].toObject()["+"].isDouble() && jlimit["absolute"].toObject()["-"].isDouble();
		bool perIsValuePair = jlimit["percent"].isObject()
				&& jlimit["percent"].toObject()["+"].isDouble() && jlimit["percent"].toObject()["-"].isDouble();

		if(jlimit["absolute"].isDouble() || jlimit["percent"].isDouble()) {
			limit.factor[0] = jlimit["percent"].isDouble() ? jlimit["percent"].toDouble() / 100.f : 0.f;
			limit.factor[1] = limit.factor[0];
			limit.absolute[0] = jlimit["absolute"].isDouble() ? jlimit["absolute"].toDouble() : 0.f;
			limit.absolute[1] = limit.absolute[0];
		} else if(absIsValuePair || perIsValuePair) {
			if(absIsValuePair) {
				limit.absolute[0] = jlimit["absolute"].toObject()["-"].toDouble();
				limit.absolute[1] = jlimit["absolute"].toObject()["+"].toDouble();
				limit.factor[0] = 0.f;
				limit.factor[1] = 0.f;
			}
			if(perIsValuePair) {
				limit.absolute[0] = 0.f;
				limit.absolute[1] = 0.f;
				limit.factor[0] = jlimit["percent"].toObject()["-"].toDouble() / 100.f;
				limit.factor[1] = jlimit["percent"].toObject()["+"].toDouble() / 100.f;
			}
		} else {
			throw json_error(QString("Elemet #%1 of \"limits\" has neither a \"percent\" nor an \"absolute\" value or value pair.")
				.arg(it - jlimits.begin()));
		}
		
		if(jlimit["lte"].isDouble()){
			limit.catch_all = false;
			limit.thresh_inclusive = true;
			limit.threshold = jlimit["lte"].toDouble();
		} else if(jlimit["lt"].isDouble()) {
			limit.catch_all = false;
			limit.thresh_inclusive = false;
			limit.threshold = jlimit["lt"].toDouble();
		} else {
			limit.catch_all = true;
			limit.thresh_inclusive = false;
			limit.threshold = 0.f;
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

	return this->create([outputs, limits, unit](ratio declared, double density, bool homogenous) {
		std::vector<ratio> values(outputs.size(), declared);


		for(auto it = limits.begin(); it != limits.end(); ++it) {
			if(it->catch_all ||
				( (declared.as(unit, density) < it->threshold ||
				  (declared.as(unit, density) == it->threshold && it->thresh_inclusive)) && 
					(it->homogenous == TriState::DC
					|| (homogenous && it->homogenous == TriState::TRUE)
					|| (!homogenous && it->homogenous == TriState::FALSE))) 
				) {
				double tolerance[2];
				tolerance[0] = it->absolute[0] + declared.as(unit, density) * it->factor[0];
				tolerance[1] = it->absolute[1] + declared.as(unit, density) * it->factor[1];
				for(size_t i = 0; i < values.size(); ++i) {
					if(outputs[i] < 0) {
						values[i] = ratio(declared.as(unit, density) + tolerance[0] * outputs[i], unit);
					} else {
						values[i] = ratio(declared.as(unit, density) + tolerance[1] * outputs[i], unit);
					}
				}
				break;
			}
		}

		return values;
	});
}