QT += core widgets
CONFIG += c++17 qt

TARGET = starterm
TEMPLATE = app

SOURCES += main.cpp

INCLUDEPATH += /usr/include/qtermwidget6
LIBS += -lqtermwidget6

# Add icon resources
RESOURCES += resources.qrc
