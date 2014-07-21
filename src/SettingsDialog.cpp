#include "SettingsDialog.h"


SettingsDialog::SettingsDialog(std::map<QString, bool> &&rules, unsigned int precision, QWidget * parent) 
	: QDialog(parent) {
		
	this->rulesTabLayout = new QVBoxLayout();

	this->rulesBox = new QGroupBox("Show:");
	this->rulesLayout = new QVBoxLayout();
	for(auto it = rules.begin(); it != rules.end(); ++it) {
		QCheckBox *cb = new QCheckBox(it->first);
		cb->setChecked(it->second);
		this->rules.insert(std::make_pair(it->first, cb));
		this->rulesLayout->addWidget(cb);
	}
	this->rulesBox->setLayout(this->rulesLayout);
	this->rulesTabLayout->addWidget(this->rulesBox);

	//this->setLayout(this->mainLayout);
	
	this->rulesTab = new QTabWidget();
	this->rulesTab->setLayout(this->rulesTabLayout);

	this->precisionTab = new QTabWidget();
	this->precisionTabLayout = new QVBoxLayout();
	
	this->precisionSlider = new QSlider(Qt::Horizontal);
	this->precisionSlider->setMinimum(2);
	this->precisionSlider->setMaximum(6);
	this->precisionSlider->setTickPosition(QSlider::TicksBelow);
	this->precisionSlider->setTickInterval(1);
	this->precisionSlider->setValue(static_cast<int>(precision));
	this->precisionSlider->setTracking(true);
	this->precisionSpin = new QSpinBox();
	this->precisionSpin->setMinimum(2);
	this->precisionSpin->setMaximum(6);
	this->precisionSpin->setValue(static_cast<int>(precision));
	connect(this->precisionSpin, SIGNAL(valueChanged(int)), this->precisionSlider, SLOT(setValue(int)));
	connect(this->precisionSlider, SIGNAL(valueChanged(int)), this->precisionSpin, SLOT(setValue(int)));
	this->precisionTabLayout->addWidget(this->precisionSlider);
	this->precisionTabLayout->addWidget(this->precisionSpin);

	this->precisionTab->setLayout(this->precisionTabLayout);
	
	this->mainTabs = new QTabWidget(this);
	this->mainTabs->addTab(rulesTab, QString("Specifications"));
	this->mainTabs->addTab(precisionTab, QString("Precision (Decimal Places)"));

	this->mainLayout = new QVBoxLayout();
	this->mainLayout->addWidget(this->mainTabs);
	this->setLayout(this->mainLayout);
	
	this->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(this->buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(this->buttons, SIGNAL(rejected()), this, SLOT(reject()));

	this->mainLayout->addWidget(this->buttons);

	this->resize(300, 350);
}


SettingsDialog::~SettingsDialog(void) {
}

std::map<QString, bool> SettingsDialog::getShowHideMap() {
	std::map<QString, bool> settings;

	for(auto it = this->rules.begin(); it != this->rules.end(); ++it) {
		settings.insert(std::make_pair(it->first, it->second->isChecked()));
	}

	return std::move(settings);
}

unsigned int SettingsDialog::getPrecisionSetting() {
	return this->precisionSpin->value();
}