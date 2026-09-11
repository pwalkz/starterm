#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
#include <QCloseEvent>
#include <QKeySequence>
#include <QAction>
#include <QTabBar>
#include <QMenu>
#include <QSettings>
#include <qtermwidget.h>

class TerminalWindow : public QMainWindow
{
public:
    TerminalWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        // Central widget with tabs
        QWidget *central = new QWidget(this);
        setCentralWidget(central);

        QVBoxLayout *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Tab widget
        tabWidget = new QTabWidget(this);
        tabWidget->setTabsClosable(true);
        tabWidget->setMovable(true);
        tabWidget->setDocumentMode(true);

        mainLayout->addWidget(tabWidget);

        // Setup UI
        createMenu();

        // Add first tab
        addNewTab();

        // Load saved theme
        loadThemePreference();

        // Status bar
        statusBar()->showMessage("Ready");

        setWindowTitle("Star Term - Terminal Emulator");
        resize(800, 600);

        // Connect signals
        connect(tabWidget, &QTabWidget::tabCloseRequested,
                this, &TerminalWindow::closeTab);
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        // Close all terminals without asking
        for (int i = 0; i < tabWidget->count(); i++) {
            QWidget *widget = tabWidget->widget(i);
            if (widget) {
                // QTermWidget will clean up properly
            }
        }
        event->accept();  // Accept the close event immediately
    }

private slots:
    void addNewTab()
    {
        QTermWidget *terminal = new QTermWidget(this);
        terminal->setShellProgram("/bin/bash");
        terminal->startShellProgram();
        terminal->setTerminalFont(QFont("JetBrains Mono", 11));

        // Apply saved theme to new tab
        QString savedTheme = loadThemeFromSettings();
        if (!savedTheme.isEmpty()) {
            terminal->setColorScheme(savedTheme);
        } else {
            terminal->setColorScheme("Tango");
        }

        terminal->setScrollBarPosition(QTermWidget::ScrollBarRight);
        terminal->setHistorySize(10000);

        int index = tabWidget->addTab(terminal, QString("Terminal %1").arg(tabWidget->count() + 1));
        tabWidget->setCurrentIndex(index);

        statusBar()->showMessage(QString("New tab created (Total: %1)").arg(tabWidget->count()), 2000);
    }

    void closeTab(int index)
    {
        if (tabWidget->count() > 1) {
            QWidget *widget = tabWidget->widget(index);
            tabWidget->removeTab(index);
            delete widget;
            statusBar()->showMessage(QString("Tab closed (Remaining: %1)").arg(tabWidget->count()), 2000);
        } else {
            // Close the last tab - exit the application
            close();
        }
    }

    void closeCurrentTab()
    {
        int currentIndex = tabWidget->currentIndex();
        if (currentIndex >= 0) {
            closeTab(currentIndex);
        }
    }

    void nextTab()
    {
        int currentIndex = tabWidget->currentIndex();
        if (currentIndex < tabWidget->count() - 1) {
            tabWidget->setCurrentIndex(currentIndex + 1);
        } else if (tabWidget->count() > 0) {
            tabWidget->setCurrentIndex(0);
        }
    }

    void previousTab()
    {
        int currentIndex = tabWidget->currentIndex();
        if (currentIndex > 0) {
            tabWidget->setCurrentIndex(currentIndex - 1);
        } else if (tabWidget->count() > 0) {
            tabWidget->setCurrentIndex(tabWidget->count() - 1);
        }
    }

    void copyText()
    {
        QTermWidget *terminal = getCurrentTerminal();
        if (terminal) {
            terminal->copyClipboard();
            statusBar()->showMessage("Copied to clipboard", 2000);
        }
    }

    void pasteText()
    {
        QTermWidget *terminal = getCurrentTerminal();
        if (terminal) {
            terminal->pasteClipboard();
            statusBar()->showMessage("Pasted from clipboard", 2000);
        }
    }

    void zoomIn()
    {
        QTermWidget *terminal = getCurrentTerminal();
        if (terminal) {
            terminal->zoomIn();
            statusBar()->showMessage("Zoomed in", 1000);
        }
    }

    void zoomOut()
    {
        QTermWidget *terminal = getCurrentTerminal();
        if (terminal) {
            terminal->zoomOut();
            statusBar()->showMessage("Zoomed out", 1000);
        }
    }

    void setTheme(const QString &themeName)
    {
        QTermWidget *terminal = getCurrentTerminal();
        if (terminal) {
            terminal->setColorScheme(themeName);
            saveThemeToSettings(themeName);
            updateThemeMenuChecks(themeName);
            statusBar()->showMessage("Theme changed to: " + themeName, 2000);
        }
    }

    void setAllTabsTheme(const QString &themeName)
    {
        for (int i = 0; i < tabWidget->count(); i++) {
            QTermWidget *terminal = qobject_cast<QTermWidget*>(tabWidget->widget(i));
            if (terminal) {
                terminal->setColorScheme(themeName);
            }
        }
        saveThemeToSettings(themeName);
        updateThemeMenuChecks(themeName);
        statusBar()->showMessage("All tabs theme changed to: " + themeName, 2000);
    }

    void showAbout()
    {
        QMessageBox::about(this, "About Starterm",
            "<h2>Starterm - Terminal Emulator</h2>"
            "<p>Version 1.0.0</p>"
            "<p>A lightweight terminal emulator built with Qt and QTermWidget.</p>"
            "<p>Features:</p>"
            "<ul>"
            "<li>Multiple tabs</li>"
            "<li>Copy & Paste support</li>"
            "<li>Zoom in/out</li>"
            "<li>Multiple color themes</li>"
            "<li>Keyboard shortcuts</li>"
            "<li>Remembers your theme preference</li>"
            "</ul>"
            "<p>Built with Qt 5/6 and QTermWidget</p>"
            "<p>Peter Walker &copy; 2026 Starterm Project</p>");
    }

private:
    QTabWidget *tabWidget;
    QMap<QString, QAction*> themeActions;
    QMap<QString, QAction*> themeAllActions;

    QTermWidget* getCurrentTerminal()
    {
        return qobject_cast<QTermWidget*>(tabWidget->currentWidget());
    }

    void saveThemeToSettings(const QString &themeName)
    {
        QSettings settings("Starterm", "Terminal");
        settings.setValue("theme", themeName);
    }

    QString loadThemeFromSettings()
    {
        QSettings settings("Starterm", "Terminal");
        return settings.value("theme", "Borland").toString();
    }

    void loadThemePreference()
    {
        QString savedTheme = loadThemeFromSettings();
        if (!savedTheme.isEmpty()) {
            // Apply to all existing tabs
            for (int i = 0; i < tabWidget->count(); i++) {
                QTermWidget *terminal = qobject_cast<QTermWidget*>(tabWidget->widget(i));
                if (terminal) {
                    terminal->setColorScheme(savedTheme);
                }
            }
            updateThemeMenuChecks(savedTheme);
            statusBar()->showMessage("Loaded theme: " + savedTheme, 2000);
        }
    }

    void updateThemeMenuChecks(const QString &selectedTheme)
    {
        // Update theme actions
        for (auto it = themeActions.begin(); it != themeActions.end(); ++it) {
            it.value()->setChecked(it.key() == selectedTheme);
        }

        // Update theme all actions
        for (auto it = themeAllActions.begin(); it != themeAllActions.end(); ++it) {
            it.value()->setChecked(it.key() == selectedTheme);
        }
    }

    void createMenu()
    {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);

        // File menu
        QMenu *fileMenu = menuBar->addMenu("&File");

        QAction *newTabAction = new QAction("&New Tab", this);
        newTabAction->setShortcut(QKeySequence::AddTab);
        connect(newTabAction, &QAction::triggered, this, &TerminalWindow::addNewTab);
        fileMenu->addAction(newTabAction);

        QAction *closeTabAction = new QAction("&Close Tab", this);
        closeTabAction->setShortcut(QKeySequence::Close);
        connect(closeTabAction, &QAction::triggered, this, &TerminalWindow::closeCurrentTab);
        fileMenu->addAction(closeTabAction);

        fileMenu->addSeparator();

        QAction *quitAction = new QAction("&Quit", this);
        quitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
        connect(quitAction, &QAction::triggered, this, &TerminalWindow::close);
        fileMenu->addAction(quitAction);

        // View menu
        QMenu *viewMenu = menuBar->addMenu("&View");

        QAction *zoomInAction = new QAction("Zoom &In", this);
        zoomInAction->setShortcut(QKeySequence::ZoomIn);
        connect(zoomInAction, &QAction::triggered, this, &TerminalWindow::zoomIn);
        viewMenu->addAction(zoomInAction);

        QAction *zoomOutAction = new QAction("Zoom &Out", this);
        zoomOutAction->setShortcut(QKeySequence::ZoomOut);
        connect(zoomOutAction, &QAction::triggered, this, &TerminalWindow::zoomOut);
        viewMenu->addAction(zoomOutAction);

        viewMenu->addSeparator();

        QAction *nextTabAction = new QAction("Next Tab", this);
        nextTabAction->setShortcut(QKeySequence::NextChild);
        connect(nextTabAction, &QAction::triggered, this, &TerminalWindow::nextTab);
        viewMenu->addAction(nextTabAction);

        QAction *prevTabAction = new QAction("Previous Tab", this);
        prevTabAction->setShortcut(QKeySequence::PreviousChild);
        connect(prevTabAction, &QAction::triggered, this, &TerminalWindow::previousTab);
        viewMenu->addAction(prevTabAction);

        // Theme menu
        QMenu *themeMenu = menuBar->addMenu("&Themes");
        QStringList themes = {"Tango", "Borland", "GreenOnBlack", "Solarized",
                              "SolarizedLight", "DarkPastels", "Ubuntu", "WhiteOnBlack"};

        // Load saved theme to check which one should be checked
        QString savedTheme = loadThemeFromSettings();

        for (const QString &theme : themes) {
            QAction *themeAction = new QAction(theme, this);
            themeAction->setCheckable(true);
            themeAction->setChecked(theme == savedTheme);
            connect(themeAction, &QAction::triggered, [this, theme]() {
                setTheme(theme);
            });
            themeMenu->addAction(themeAction);
            themeActions[theme] = themeAction;
        }

        // Theme menu - apply to all tabs
        QMenu *themeAllMenu = menuBar->addMenu("&All Tabs");
        for (const QString &theme : themes) {
            QAction *themeAction = new QAction(theme, this);
            themeAction->setCheckable(true);
            themeAction->setChecked(theme == savedTheme);
            connect(themeAction, &QAction::triggered, [this, theme]() {
                setAllTabsTheme(theme);
            });
            themeAllMenu->addAction(themeAction);
            themeAllActions[theme] = themeAction;
        }

        // Help menu
        QMenu *helpMenu = menuBar->addMenu("&Help");

        QAction *aboutAction = new QAction("About", this);
        aboutAction->setShortcut(QKeySequence::HelpContents);
        connect(aboutAction, &QAction::triggered, this, &TerminalWindow::showAbout);
        helpMenu->addAction(aboutAction);

        QAction *aboutQtAction = new QAction("About &Qt", this);
        connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
        helpMenu->addAction(aboutQtAction);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Starterm");
    app.setApplicationName("Terminal");

    TerminalWindow window;
    window.setWindowIcon(QIcon(":/icons/starterm.png"));
    window.show();

    return app.exec();
}
