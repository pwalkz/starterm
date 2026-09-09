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
        createToolBar();
        
        // Add first tab
        addNewTab();
        
        // Status bar
        statusBar()->showMessage("Ready");
        
        setWindowTitle("Starterm - Terminal Emulator");
        resize(800, 600);  // 1000,700
        
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
        // Force the terminal to report as a basic 8-color terminal
       // terminal->setEnvironment(QStringList() << "TERM=xterm");
        terminal->setShellProgram("/bin/bash");
        terminal->startShellProgram();
        terminal->setTerminalFont(QFont("JetBrains Mono", 11));
        terminal->setColorScheme("Borland");
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
        statusBar()->showMessage("All tabs theme changed to: " + themeName, 2000);
    }
    
    // Borland theme handler
    void setBorlandTheme()
    {
        QTermWidget *terminal = getCurrentTerminal();
        if (terminal) {
            terminal->setColorScheme("Borland");
            statusBar()->showMessage("Theme changed to: Borland", 2000);
        }
    }
    
    void setAllTabsBorlandTheme()
    {
        for (int i = 0; i < tabWidget->count(); i++) {
            QTermWidget *terminal = qobject_cast<QTermWidget*>(tabWidget->widget(i));
            if (terminal) {
                terminal->setColorScheme("Borland");
            }
        }
        statusBar()->showMessage("All tabs theme changed to: Borland", 2000);
    }

private:
    QTabWidget *tabWidget;
    
    QTermWidget* getCurrentTerminal()
    {
        return qobject_cast<QTermWidget*>(tabWidget->currentWidget());
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
        
        QAction *exitAction = new QAction("E&xit", this);
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, this, &TerminalWindow::close);
        fileMenu->addAction(exitAction);
        
        // Edit menu
        QMenu *editMenu = menuBar->addMenu("&Edit");
        
        QAction *copyAction = new QAction("&Copy", this);
        copyAction->setShortcut(QKeySequence::Copy);
        connect(copyAction, &QAction::triggered, this, &TerminalWindow::copyText);
        editMenu->addAction(copyAction);
        
        QAction *pasteAction = new QAction("&Paste", this);
        pasteAction->setShortcut(QKeySequence::Paste);
        connect(pasteAction, &QAction::triggered, this, &TerminalWindow::pasteText);
        editMenu->addAction(pasteAction);
        
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
        QStringList themes = {"Borland","Tango", "Linux Colors", "Solarized Light", 
                              "Solarized Dark", "Dark Pastels", "Ubuntu", "XTerm"};
        for (const QString &theme : themes) {
            QAction *themeAction = new QAction(theme, this);
            connect(themeAction, &QAction::triggered, [this, theme]() {
                setTheme(theme);
            });
            themeMenu->addAction(themeAction);
        }
        
        // Add Borland theme
    //.    QAction *borlandAction = new QAction("Borland", this);
     //.   connect(borlandAction, &QAction::triggered, this, &TerminalWindow::setBorlandTheme);
  //.      themeMenu->addAction(borlandAction);
        
        // Theme menu - apply to all tabs
        QMenu *themeAllMenu = menuBar->addMenu("&Themes (All Tabs)");
        for (const QString &theme : themes) {
            QAction *themeAction = new QAction(theme, this);
            connect(themeAction, &QAction::triggered, [this, theme]() {
                setAllTabsTheme(theme);
            });
            themeAllMenu->addAction(themeAction);
        }
        
        // Add Borland theme to all tabs
   //.     QAction *borlandAllAction = new QAction("Borland", this);
   //.     connect(borlandAllAction, &QAction::triggered, this, &TerminalWindow::setAllTabsBorlandTheme);
   //.     themeAllMenu->addAction(borlandAllAction);
    }
    
    void createToolBar()
    {
        QToolBar *toolBar = new QToolBar(this);
        toolBar->setMovable(false);
        addToolBar(toolBar);
        
        QAction *newTabAction = new QAction("+ New Tab", this);
        newTabAction->setToolTip("Open a new terminal tab (Ctrl+T)");
        connect(newTabAction, &QAction::triggered, this, &TerminalWindow::addNewTab);
        toolBar->addAction(newTabAction);
        
        toolBar->addSeparator();
        
        QAction *copyAction = new QAction("Copy", this);
        copyAction->setToolTip("Copy selected text (Ctrl+C)");
        connect(copyAction, &QAction::triggered, this, &TerminalWindow::copyText);
        toolBar->addAction(copyAction);
        
        QAction *pasteAction = new QAction("Paste", this);
        pasteAction->setToolTip("Paste text from clipboard (Ctrl+V)");
        connect(pasteAction, &QAction::triggered, this, &TerminalWindow::pasteText);
        toolBar->addAction(pasteAction);
        
        toolBar->addSeparator();
        
        QAction *zoomInAction = new QAction("Zoom In", this);
        zoomInAction->setToolTip("Zoom in (Ctrl+=)");
        connect(zoomInAction, &QAction::triggered, this, &TerminalWindow::zoomIn);
        toolBar->addAction(zoomInAction);
        
        QAction *zoomOutAction = new QAction("Zoom Out", this);
        zoomOutAction->setToolTip("Zoom out (Ctrl+-)");
        connect(zoomOutAction, &QAction::triggered, this, &TerminalWindow::zoomOut);
        toolBar->addAction(zoomOutAction);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    TerminalWindow window;
    window.setWindowIcon(QIcon(":/icons/starterm.png"));
    window.show();
    
    return app.exec();
}
