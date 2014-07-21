QT += core gui widgets

RESOURCES += \
    untitled.qrc

SOURCES += \
    ReleaseLimitsRule.cpp \
    mainwindow.cpp \
    main.cpp \
    SettingsDialog.cpp

FORMS += \
    mainwindow.ui

OTHER_FILES += \
    rules.json

HEADERS += \
    ReleaseLimitsRule.h \
    mainwindow.h \
    SettingsDialog.h
