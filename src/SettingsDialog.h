#ifndef RLC_SETTINGS_DIALOG_H
#define RLC_SETTINGS_DIALOG_H

#include <QtWidgets/QDialog.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qgroupbox.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qtabwidget.h>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qspinbox.h>
#include <map>

class SettingsDialog : public QDialog {
	Q_OBJECT
public:
	SettingsDialog(std::map<QString, bool> &&rules, unsigned int precision, QWidget * parent = 0);
	virtual ~SettingsDialog(void);

	std::map<QString, bool> getShowHideMap();
	unsigned int getPrecisionSetting();
private:
	QTabWidget *mainTabs;
	QWidget *rulesTab;
	QVBoxLayout *mainLayout;
	QVBoxLayout *rulesLayout;
	QVBoxLayout *rulesTabLayout;
	QGroupBox *rulesBox;
	QDialogButtonBox *buttons;
	std::map<QString, QCheckBox*> rules;
	QWidget *precisionTab;
	QVBoxLayout *precisionTabLayout;
	QSlider *precisionSlider;
	QSpinBox *precisionSpin;
};



#endif //RLC_SETTINGS_DIALOG_H