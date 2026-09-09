#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QCloseEvent>
#include <qtermwidget.h>

class TerminalWindow : public QMainWindow
{
public:
    TerminalWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        QWidget *central = new QWidget(this);
        setCentralWidget(central);
        
        QVBoxLayout *layout = new QVBoxLayout(central);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        terminal = new QTermWidget(this);
        terminal->setShellProgram("/bin/bash");
        terminal->startShellProgram();
        terminal->setTerminalFont(QFont("Monospace", 11));
        terminal->setColorScheme("Tango");
        terminal->setScrollBarPosition(QTermWidget::ScrollBarRight);
        terminal->setHistorySize(10000);
        
        layout->addWidget(terminal);
        createMenu();
        
        setWindowTitle("Starterm - My Terminal");
        resize(900, 600);
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        // Cleanly close the terminal before closing window
        terminal->close();
        event->accept();
    }

private slots:
    void copyText() { terminal->copyClipboard(); }
    void pasteText() { terminal->pasteClipboard(); }
    void zoomIn() { terminal->zoomIn(); }
    void zoomOut() { terminal->zoomOut(); }
    void newTerminal() 
    { 
        terminal->startShellProgram(); 
    }
    void exitApplication() 
    { 
        close(); // This will trigger closeEvent
    }

private:
    QTermWidget *terminal;
    
    void createMenu()
    {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);
        
        QMenu *fileMenu = menuBar->addMenu("&File");
        QAction *newAction = new QAction("&New Tab", this);
        connect(newAction, &QAction::triggered, this, &TerminalWindow::newTerminal);
        fileMenu->addAction(newAction);
        fileMenu->addSeparator();
        QAction *exitAction = new QAction("E&xit", this);
        connect(exitAction, &QAction::triggered, this, &TerminalWindow::exitApplication);
        fileMenu->addAction(exitAction);
        
        QMenu *editMenu = menuBar->addMenu("&Edit");
        QAction *copyAction = new QAction("&Copy", this);
        connect(copyAction, &QAction::triggered, this, &TerminalWindow::copyText);
        editMenu->addAction(copyAction);
        QAction *pasteAction = new QAction("&Paste", this);
        connect(pasteAction, &QAction::triggered, this, &TerminalWindow::pasteText);
        editMenu->addAction(pasteAction);
        
        QMenu *viewMenu = menuBar->addMenu("&View");
        QAction *zoomInAction = new QAction("Zoom &In", this);
        connect(zoomInAction, &QAction::triggered, this, &TerminalWindow::zoomIn);
        viewMenu->addAction(zoomInAction);
        QAction *zoomOutAction = new QAction("Zoom &Out", this);
        connect(zoomOutAction, &QAction::triggered, this, &TerminalWindow::zoomOut);
        viewMenu->addAction(zoomOutAction);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    TerminalWindow window;
    window.show();
    
    return app.exec();
}
