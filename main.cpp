#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDirIterator>
#include <QFontDialog>
#include <QFontInfo>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSizePolicy>
#include <QSet>
#include <QStringList>
#include <QAbstractItemView>
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

        // ----- Top command / history panel -----
        commandPanel = new QWidget(this);
        QHBoxLayout *panelLayout = new QHBoxLayout(commandPanel);
        panelLayout->setContentsMargins(6, 4, 6, 4);
        panelLayout->setSpacing(6);

        // --- Commands combo + Run ---
        QLabel *cmdLabel = new QLabel("Commands:", commandPanel);
        panelLayout->addWidget(cmdLabel);

        commandCombo = new QComboBox(commandPanel);
        commandCombo->setEditable(true);
        commandCombo->setInsertPolicy(QComboBox::NoInsert);
        commandCombo->setMaxVisibleItems(50);
        commandCombo->setMinimumContentsLength(10);
        commandCombo->setSizeAdjustPolicy(
            QComboBox::AdjustToMinimumContentsLengthWithIcon);
        commandCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        commandCombo->setToolTip("Pick a command, or type your own, then click Run…");
        commandCombo->view()->setMinimumWidth(360);
        panelLayout->addWidget(commandCombo, 1);

        runButton = new QPushButton("Run…", commandPanel);
        runButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        runButton->setToolTip("Send the selected command to the current tab");
        panelLayout->addWidget(runButton);

        // --- History group (label + combo + Run) ---
        historyGroup = new QWidget(commandPanel);
        QHBoxLayout *historyLayout = new QHBoxLayout(historyGroup);
        historyLayout->setContentsMargins(0, 0, 0, 0);
        historyLayout->setSpacing(6);

        QLabel *histLabel = new QLabel("History:", historyGroup);
        historyLayout->addWidget(histLabel);

        historyCombo = new QComboBox(historyGroup);
        historyCombo->setEditable(true);
        historyCombo->setInsertPolicy(QComboBox::NoInsert);
        historyCombo->setMaxVisibleItems(50);
        historyCombo->setMinimumContentsLength(10);
        historyCombo->setSizeAdjustPolicy(
            QComboBox::AdjustToMinimumContentsLengthWithIcon);
        historyCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        historyCombo->setToolTip("Recent commands from your shell history");
        historyCombo->view()->setMinimumWidth(360);
        historyLayout->addWidget(historyCombo, 1);

        historyRunButton = new QPushButton("Run…", historyGroup);
        historyRunButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        historyRunButton->setToolTip("Re-run the selected history entry");
        historyLayout->addWidget(historyRunButton);

        panelLayout->addWidget(historyGroup, 1);

        // Place the panel above the tab widget
        mainLayout->insertWidget(0, commandPanel);

        connect(runButton, &QPushButton::clicked,
                this, &TerminalWindow::runSelectedCommand);
        connect(historyRunButton, &QPushButton::clicked,
                this, &TerminalWindow::runSelectedHistory);

        // When the user *picks* an item (not types), collapse the line edit
        // down to just the command and move focus to the Run button.
        connect(commandCombo, QOverload<int>::of(&QComboBox::activated),
                this, &TerminalWindow::onCommandActivated);
        connect(historyCombo, QOverload<int>::of(&QComboBox::activated),
                this, &TerminalWindow::onHistoryActivated);

        // Enter inside the editable line edit runs the command too
        connect(commandCombo->lineEdit(), &QLineEdit::returnPressed,
                this, &TerminalWindow::runSelectedCommand);
        connect(historyCombo->lineEdit(), &QLineEdit::returnPressed,
                this, &TerminalWindow::runSelectedHistory);

        loadCommands();
        loadHistory();

        // Setup UI
        createMenu();

        // Apply saved visibility
        applyCommandPanelVisibility(loadShowCommandPanelFromSettings());
        applyHistoryPanelVisibility(loadShowHistoryPanelFromSettings());
        applyStatusBarVisibility(loadShowStatusBarFromSettings());
        updateTabBarVisibility();

        // Add first tab
        addNewTab();

        // Load saved theme
        loadThemePreference();

        // Load saved font (applies to all open tabs)
        loadFontPreference();

        // Status bar
        statusBar()->showMessage("Ready");

        setWindowTitle("Star Term - Terminal Emulator");
        resize(900, 600);

        // Connect signals
        connect(tabWidget, &QTabWidget::tabCloseRequested,
                this, &TerminalWindow::closeTab);

        connect(tabWidget, &QTabWidget::currentChanged,
                this, [this](int) { focusCurrentTerminal(); });
    }

    // Public so main() can call it after show()
    void focusCurrentTerminal()
    {
        if (QTermWidget *term = getCurrentTerminal()) {
            term->setFocus(Qt::OtherFocusReason);
        }
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        event->accept();
    }

    void changeEvent(QEvent *event) override
    {
        if (event->type() == QEvent::ActivationChange && isActiveWindow()) {
            focusCurrentTerminal();
        }
        QMainWindow::changeEvent(event);
    }

private slots:
    void addNewTab()
    {
        QTermWidget *terminal = new QTermWidget(this);
        terminal->setShellProgram("/bin/bash");
        terminal->startShellProgram();

        terminal->setTerminalFont(loadFontFromSettings());

        QString savedTheme = loadThemeFromSettings();
        if (savedTheme.isEmpty()) savedTheme = "Tango";
        applySchemeToTerminal(terminal, savedTheme);

        terminal->setScrollBarPosition(QTermWidget::ScrollBarRight);
        terminal->setHistorySize(10000);

        int index = tabWidget->addTab(terminal,
            QString("Terminal %1").arg(tabWidget->count() + 1));
        tabWidget->setCurrentIndex(index);
        focusCurrentTerminal();
        updateTabBarVisibility();

        statusBar()->showMessage(
            QString("New tab created (Total: %1)").arg(tabWidget->count()), 2000);
    }

    void closeTab(int index)
    {
        if (tabWidget->count() > 1) {
            QWidget *widget = tabWidget->widget(index);
            tabWidget->removeTab(index);
            delete widget;
            focusCurrentTerminal();
            updateTabBarVisibility();
            statusBar()->showMessage(
                QString("Tab closed (Remaining: %1)").arg(tabWidget->count()), 2000);
        } else {
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
        if (QTermWidget *terminal = getCurrentTerminal()) {
            terminal->copyClipboard();
            statusBar()->showMessage("Copied to clipboard", 2000);
        }
    }

    void pasteText()
    {
        if (QTermWidget *terminal = getCurrentTerminal()) {
            terminal->pasteClipboard();
            statusBar()->showMessage("Pasted from clipboard", 2000);
        }
    }

    // -------------------- View toggles --------------------

    void toggleHideTabs(bool on)
    {
        saveHideTabsToSettings(on);
        updateTabBarVisibility();
    }

    void toggleAutoHideTabs(bool on)
    {
        saveAutoHideTabsToSettings(on);
        updateTabBarVisibility();
    }

    void toggleHistoryPanel(bool visible)
    {
        applyHistoryPanelVisibility(visible);
        saveShowHistoryPanelToSettings(visible);
        focusCurrentTerminal();
    }

    void toggleStatusBar(bool visible)
    {
        applyStatusBarVisibility(visible);
        saveShowStatusBarToSettings(visible);
        focusCurrentTerminal();
    }

    // -------------------- Command / History panel --------------------

    void onCommandActivated(int index)
    {
        if (index < 0 || index >= commandCombo->count()) return;

        const QVariant userCmd = commandCombo->itemData(index, Qt::UserRole);
        const QString clean = userCmd.isValid()
                                ? userCmd.toString()
                                : commandCombo->itemText(index);

        commandCombo->setEditText(clean);

        const QString desc = commandCombo->itemData(index, Qt::ToolTipRole).toString();
        commandCombo->setToolTip(desc.isEmpty()
            ? "Pick a command, or type your own, then click Run…"
            : desc);

        // Move focus to the Run button so Enter/Space runs the command
        runButton->setFocus(Qt::OtherFocusReason);
        runButton->setDefault(true);
        historyRunButton->setDefault(false);
    }

    void onHistoryActivated(int index)
    {
        if (index < 0 || index >= historyCombo->count()) return;

        const QVariant userCmd = historyCombo->itemData(index, Qt::UserRole);
        const QString clean = userCmd.isValid()
                                ? userCmd.toString()
                                : historyCombo->itemText(index);

        historyCombo->setEditText(clean);
        historyCombo->setToolTip("History entry — will be re-run");

        // Move focus to the History Run button so Enter/Space runs the entry
        historyRunButton->setFocus(Qt::OtherFocusReason);
        historyRunButton->setDefault(true);
        runButton->setDefault(false);
    }

    void runSelectedCommand()
    {
        runFromCombo(commandCombo, "command");
    }

    void runSelectedHistory()
    {
        runFromCombo(historyCombo, "history entry");
    }

    void toggleCommandPanel(bool visible)
    {
        applyCommandPanelVisibility(visible);
        saveShowCommandPanelToSettings(visible);
        focusCurrentTerminal();
    }

    void reloadCommands()
    {
        loadCommands();
        statusBar()->showMessage(
            QString("Commands reloaded (%1 entries)").arg(commandCombo->count()), 2000);
    }

    void reloadHistory()
    {
        loadHistory();
        statusBar()->showMessage(
            QString("History reloaded (%1 entries)").arg(historyCombo->count()), 2000);
    }

    void reloadPanel()
    {
        loadCommands();
        loadHistory();
        statusBar()->showMessage(
            QString("Panel reloaded — commands: %1, history: %2")
                .arg(commandCombo->count())
                .arg(historyCombo->count()), 3000);
    }

    // -------------------- Font handling --------------------

    void chooseFont()
    {
        QFont current = loadFontFromSettings();
        bool ok = false;

        QFont chosen = QFontDialog::getFont(&ok, current, this,
                                            "Select Terminal Font",
                                            QFontDialog::MonospacedFonts);
        if (!ok) return;

        saveFontToSettings(chosen);
        applyFontToAllTabs(chosen);
        statusBar()->showMessage(
            QString("Font changed to %1 %2")
                .arg(chosen.family())
                .arg(chosen.pointSize()), 2000);
        focusCurrentTerminal();
    }

    void increaseFontSize()
    {
        QFont f = loadFontFromSettings();
        int size = f.pointSize();
        if (size <= 0) size = 11;
        f.setPointSize(size + 1);
        saveFontToSettings(f);
        applyFontToAllTabs(f);
        statusBar()->showMessage(QString("Font size: %1").arg(f.pointSize()), 1000);
        focusCurrentTerminal();
    }

    void decreaseFontSize()
    {
        QFont f = loadFontFromSettings();
        int size = f.pointSize();
        if (size <= 0) size = 11;
        size = qMax(4, size - 1);
        f.setPointSize(size);
        saveFontToSettings(f);
        applyFontToAllTabs(f);
        statusBar()->showMessage(QString("Font size: %1").arg(f.pointSize()), 1000);
        focusCurrentTerminal();
    }

    void resetFont()
    {
        QFont f("JetBrains Mono", 11);
        saveFontToSettings(f);
        applyFontToAllTabs(f);
        statusBar()->showMessage("Font reset to default", 2000);
        focusCurrentTerminal();
    }

    // -------------------- Theme handling --------------------

    // Applies the theme to ALL open tabs and saves the preference.
    void setTheme(const QString &themeName)
    {
        for (int i = 0; i < tabWidget->count(); i++) {
            QTermWidget *terminal = qobject_cast<QTermWidget*>(tabWidget->widget(i));
            if (terminal) {
                applySchemeToTerminal(terminal, themeName);
            }
        }
        saveThemeToSettings(themeName);
        updateThemeMenuChecks(themeName);
        statusBar()->showMessage("Theme changed to: " + themeName, 2000);
    }

    void reloadThemes()
    {
        if (!themeMenu) return;

        QString savedTheme = loadThemeFromSettings();

        themeMenu->clear();
        themeActions.clear();

        QStringList themes = discoverThemes();

        for (const QString &theme : themes) {
            QAction *themeAction = new QAction(theme, this);
            themeAction->setCheckable(true);
            themeAction->setChecked(theme == savedTheme);
            connect(themeAction, &QAction::triggered, this, [this, theme]() {
                setTheme(theme);
            });
            themeMenu->addAction(themeAction);
            themeActions[theme] = themeAction;
        }

        themeMenu->addSeparator();
        QAction *reloadAction = new QAction("&Reload Themes from Disk", this);
        connect(reloadAction, &QAction::triggered, this, &TerminalWindow::reloadThemes);
        themeMenu->addAction(reloadAction);

        statusBar()->showMessage(
            QString("Themes reloaded (%1 found)").arg(themes.size()), 2000);
    }

    void showAbout()
    {
        QMessageBox::about(this, "About Starterm",
            "<h2>Starterm - Terminal Emulator</h2>"
            "<p>Version 1.0.1</p>"
            "<p>A lightweight terminal emulator built with Qt and QTermWidget.</p>"
            "<p>Features:</p>"
            "<ul>"
            "<li>Multiple tabs (optionally hidden / auto-hidden)</li>"
            "<li>Copy & Paste support</li>"
            "<li>Multiple color themes (including user-supplied ones)</li>"
            "<li>Monospace-only font picker (name and size saved to your config)</li>"
            "<li>Run Command Panel with dropdowns for commands.txt and shell history</li>"
            "<li>Pick from a dropdown and press Enter to run</li>"
            "<li>Keyboard shortcuts</li>"
            "<li>Remembers theme, font, tabs, panel, and status bar preferences</li>"
            "</ul>"
            "<p>Built with Qt 5/6 and QTermWidget</p>"
            "<p>Peter Walker &copy; 2026 Starterm Project</p>");
    }

private:
    QTabWidget  *tabWidget;
    QWidget     *commandPanel    = nullptr;
    QWidget     *historyGroup    = nullptr;
    QComboBox   *commandCombo    = nullptr;
    QPushButton *runButton       = nullptr;
    QComboBox   *historyCombo    = nullptr;
    QPushButton *historyRunButton = nullptr;

    QMenu *themeMenu = nullptr;
    QMap<QString, QAction*> themeActions;

    QAction *showCommandPanelAction = nullptr;
    QAction *showHistoryPanelAction = nullptr;
    QAction *showStatusBarAction    = nullptr;
    QAction *hideTabsAction         = nullptr;
    QAction *autoHideTabsAction     = nullptr;

    QTermWidget* getCurrentTerminal()
    {
        return qobject_cast<QTermWidget*>(tabWidget->currentWidget());
    }

    // ------------------------------------------------------------------
    //  Tab bar visibility
    // ------------------------------------------------------------------

    void updateTabBarVisibility()
    {
        if (!tabWidget || !tabWidget->tabBar()) return;

        const bool hideTabs = loadHideTabsFromSettings();
        const bool autoHide = loadAutoHideTabsFromSettings();
        const int  tabCount = tabWidget->count();

        bool visible = true;
        if (hideTabs) {
            visible = false;
        } else if (autoHide && tabCount <= 1) {
            visible = false;
        }

        tabWidget->tabBar()->setVisible(visible);

        if (hideTabsAction)     hideTabsAction->setChecked(hideTabs);
        if (autoHideTabsAction) autoHideTabsAction->setChecked(autoHide);
    }

    void saveHideTabsToSettings(bool on)
    {
        QSettings settings("Starterm", "Terminal");
        settings.setValue("ui/hideTabs", on);
        settings.sync();
    }

    bool loadHideTabsFromSettings()
    {
        QSettings settings("Starterm", "Terminal");
        return settings.value("ui/hideTabs", false).toBool();
    }

    void saveAutoHideTabsToSettings(bool on)
    {
        QSettings settings("Starterm", "Terminal");
        settings.setValue("ui/autoHideTabs", on);
        settings.sync();
    }

    bool loadAutoHideTabsFromSettings()
    {
        QSettings settings("Starterm", "Terminal");
        return settings.value("ui/autoHideTabs", false).toBool();
    }

    // ------------------------------------------------------------------
    //  Status bar visibility
    // ------------------------------------------------------------------

    void applyStatusBarVisibility(bool visible)
    {
        if (!statusBar()) return;
        statusBar()->setVisible(visible);
        if (showStatusBarAction)
            showStatusBarAction->setChecked(visible);
    }

    void saveShowStatusBarToSettings(bool visible)
    {
        QSettings settings("Starterm", "Terminal");
        settings.setValue("ui/showStatusBar", visible);
        settings.sync();
    }

    bool loadShowStatusBarFromSettings()
    {
        QSettings settings("Starterm", "Terminal");
        return settings.value("ui/showStatusBar", true).toBool();
    }

    // ------------------------------------------------------------------
    //  History panel visibility
    // ------------------------------------------------------------------

    void applyHistoryPanelVisibility(bool visible)
    {
        if (!historyGroup) return;
        historyGroup->setVisible(visible);
        if (showHistoryPanelAction)
            showHistoryPanelAction->setChecked(visible);
    }

    void saveShowHistoryPanelToSettings(bool visible)
    {
        QSettings settings("Starterm", "Terminal");
        settings.setValue("ui/showHistoryPanel", visible);
        settings.sync();
    }

    bool loadShowHistoryPanelFromSettings()
    {
        QSettings settings("Starterm", "Terminal");
        return settings.value("ui/showHistoryPanel", true).toBool();
    }

    // ------------------------------------------------------------------
    //  Run helper (shared by both combos)
    // ------------------------------------------------------------------

    void runFromCombo(QComboBox *combo, const QString &what)
    {
        if (!combo) return;

        const QString visible = combo->lineEdit()
            ? combo->lineEdit()->text().trimmed()
            : combo->currentText().trimmed();

        if (visible.isEmpty()) {
            statusBar()->showMessage("Nothing selected in " + what, 2000);
            return;
        }

        QString cmd = visible;
        const int idx = combo->currentIndex();
        if (idx >= 0 && combo->itemText(idx) == visible) {
            const QVariant userCmd = combo->itemData(idx, Qt::UserRole);
            if (userCmd.isValid()) {
                cmd = userCmd.toString();
            }
        }

        cmd = cmd.trimmed();
        if (cmd.isEmpty()) {
            statusBar()->showMessage("Nothing to run in " + what, 2000);
            return;
        }

       /* if (!isCommandSafe(cmd)) {
            QMessageBox::warning(this, "Command Blocked",
                QString("For safety, this %1 will not be run from the panel:\n\n%2\n\n"
                        "You can still type it manually in the terminal if you wish.")
                    .arg(what, cmd));
            statusBar()->showMessage("Blocked for safety", 3000);
            return;
        }   */

        QTermWidget *terminal = getCurrentTerminal();
        if (!terminal) {
            statusBar()->showMessage("No active terminal", 2000);
            return;
        }

        terminal->sendText(cmd + "\n");
        focusCurrentTerminal();
        statusBar()->showMessage(QString("Ran %1: %2").arg(what, cmd), 2000);
    }

    // ------------------------------------------------------------------
    //  Command panel — commands.txt
    // ------------------------------------------------------------------

    void loadCommands()
    {
        commandCombo->clear();

        const QString path = QCoreApplication::applicationDirPath() + "/commands.txt";
        QFile file(path);

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();

                if (line.isEmpty() || line.startsWith('#'))
                    continue;

                QString cmd = line;
                QString desc;

                int sep = line.indexOf("::");
                if (sep >= 0) {
                    cmd  = line.left(sep).trimmed();
                    desc = line.mid(sep + 2).trimmed();
                }

                if (cmd.isEmpty())
                    continue;

                addCommandToCombo(cmd, desc);
            }
            file.close();
        } else {
            const QList<QPair<QString, QString>> defaults = {
                {"ls -lah",             "List files in long, human-readable format"},
                {"pwd",                 "Print working directory"},
                {"cd ..",               "Go up one directory"},
                {"cd ~",                "Go to home directory"},
                {"cat /etc/os-release", "Show OS information"},
                {"df -h",               "Show disk usage"},
                {"du -sh *",            "Show size of items in current dir"},
                {"free -h",             "Show memory usage"},
                {"uname -a",            "Show kernel and system info"},
                {"whoami",              "Show current user"},
                {"id",                  "Show user and group IDs"},
                {"uptime",              "Show system uptime and load"},
                {"ps aux | head -20",   "Show first 20 running processes"},
                {"top",                 "Live process viewer (press q to quit)"},
                {"ip a",                "Show network interfaces"},
                {"ping -c 4 1.1.1.1",   "Ping Cloudflare DNS 4 times"},
                {"history",             "Show recent shell history"},
                {"env",                 "Show environment variables"},
                {"which bash",          "Locate the bash binary"},
                {"man ls",              "Open the manual page for ls"},
            };
            for (const auto &p : defaults) {
                addCommandToCombo(p.first, p.second);
            }
            statusBar()->showMessage(
                "commands.txt not found — using built-in command list", 4000);
        }
    }

    void addCommandToCombo(const QString &cmd, const QString &desc)
    {
        commandCombo->addItem(cmd);

        const int idx = commandCombo->count() - 1;
        if (!desc.isEmpty()) {
            commandCombo->setItemData(idx, cmd,  Qt::UserRole);
            commandCombo->setItemData(idx, desc, Qt::ToolTipRole);
            commandCombo->setItemData(idx,
                QString("%1   —   %2").arg(cmd, desc),
                Qt::DisplayRole);
        }
    }

    // ------------------------------------------------------------------
    //  Command panel — shell history
    // ------------------------------------------------------------------

    QString historyFilePath() const
    {
        QSettings settings("Starterm", "Terminal");
        QString configured = settings.value("history/file").toString();
        if (!configured.isEmpty()) {
            if (configured.startsWith("~/")) {
                configured = QDir::homePath() + configured.mid(1);
            }
            return configured;
        }
        return QDir::homePath() + "/.bash_history";
    }

    int historyMaxEntries() const
    {
        QSettings settings("Starterm", "Terminal");
        int n = settings.value("history/maxEntries", 50).toInt();
        if (n <= 0) n = 50;
        return n;
    }

    void loadHistory()
    {
        historyCombo->clear();

        const QString path = historyFilePath();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            statusBar()->showMessage(
                QString("History file not found: %1").arg(path), 4000);
            return;
        }

        QTextStream in(&file);

        QStringList ordered;
        QSet<QString> seen;

        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.startsWith('#'))
                continue;
            if (line.startsWith(' '))
                continue;

            line = line.trimmed();
            if (line.isEmpty())
                continue;

            if (line.startsWith(": ")) {
                int semi = line.indexOf(';');
                if (semi >= 0)
                    line = line.mid(semi + 1).trimmed();
                if (line.isEmpty())
                    continue;
            }

            if (line.startsWith("- cmd:"))
                line = line.mid(6).trimmed();

            if (line.isEmpty())
                continue;

            if (seen.contains(line))
                continue;

            seen.insert(line);
            ordered.prepend(line);
        }

        file.close();

        const int cap = historyMaxEntries();
        if (ordered.size() > cap) {
            ordered = ordered.mid(0, cap);
        }

        for (const QString &cmd : ordered) {
            historyCombo->addItem(cmd);
            const int idx = historyCombo->count() - 1;
            historyCombo->setItemData(idx, cmd, Qt::UserRole);
            historyCombo->setItemData(idx, "History entry — will be re-run",
                                      Qt::ToolTipRole);
        }

        if (ordered.isEmpty()) {
            statusBar()->showMessage(
                QString("History file empty or unreadable: %1").arg(path), 3000);
        }
    }

    // ------------------------------------------------------------------
    //  Safety filter
    // ------------------------------------------------------------------

    bool isCommandSafe(const QString &cmd) const
    {
        static const QStringList blocked = {
            "rm -rf /", "rm -rf /*", "rm -rf ~", "rm -rf $home",
            "mkfs", "dd if=", "dd of=", "shutdown", "reboot", "poweroff",
            "init 0", "init 6", ":(){", ":() {", "> /dev/sda", "> /dev/nvme",
            "chmod -r 777 /", "chown -r", "mv /* ", "mv ~ ",
        };

        const QString lower = cmd.toLower();
        for (const QString &b : blocked) {
            if (lower.contains(b))
                return false;
        }

        static const QStringList privEsc = {
            "sudo ", "su ", "doas ", "pkexec "
        };
        for (const QString &p : privEsc) {
            if (lower.startsWith(p))
                return false;
        }

        return true;
    }

    // ------------------------------------------------------------------
    //  Command panel visibility
    // ------------------------------------------------------------------

    void applyCommandPanelVisibility(bool visible)
    {
        if (!commandPanel) return;
        commandPanel->setVisible(visible);
        if (showCommandPanelAction)
            showCommandPanelAction->setChecked(visible);
    }

    void saveShowCommandPanelToSettings(bool visible)
    {
        QSettings settings("Starterm", "Terminal");
        settings.setValue("ui/showCommandPanel", visible);
        settings.sync();
    }

    bool loadShowCommandPanelFromSettings()
    {
        QSettings settings("Starterm", "Terminal");
        return settings.value("ui/showCommandPanel", true).toBool();
    }

    // ------------------------------------------------------------------
    //  Font helpers
    // ------------------------------------------------------------------

    void saveFontToSettings(const QFont &font)
    {
        QSettings settings("Starterm", "Terminal");
        settings.setValue("font/family", font.family());
        settings.setValue("font/pointSize", font.pointSize());
        settings.setValue("font/weight", static_cast<int>(font.weight()));
        settings.setValue("font/italic", font.italic());
        settings.sync();
    }

    QFont loadFontFromSettings()
    {
        QSettings settings("Starterm", "Terminal");
        QFont font;
        font.setFamily(settings.value("font/family", "JetBrains Mono").toString());

        int size = settings.value("font/pointSize", 11).toInt();
        if (size <= 0) size = 11;
        font.setPointSize(size);

        int weight = settings.value("font/weight",
                                    static_cast<int>(QFont::Normal)).toInt();
        font.setWeight(static_cast<QFont::Weight>(weight));

        font.setItalic(settings.value("font/italic", false).toBool());

        QFontInfo info(font);
        if (!info.fixedPitch()) {
            font = QFont("JetBrains Mono", 11);
        }
        return font;
    }

    void applyFontToTerminal(QTermWidget *terminal, const QFont &font)
    {
        if (!terminal) return;
        terminal->setTerminalFont(font);
    }

    void applyFontToAllTabs(const QFont &font)
    {
        for (int i = 0; i < tabWidget->count(); i++) {
            QTermWidget *terminal = qobject_cast<QTermWidget*>(tabWidget->widget(i));
            applyFontToTerminal(terminal, font);
        }
    }

    void loadFontPreference()
    {
        QFont font = loadFontFromSettings();
        applyFontToAllTabs(font);
        statusBar()->showMessage(
            QString("Loaded font: %1 %2")
                .arg(font.family())
                .arg(font.pointSize()), 2000);
    }

    // ------------------------------------------------------------------
    //  Color scheme helpers
    // ------------------------------------------------------------------

    QString colorSchemesDir() const
    {
        return QCoreApplication::applicationDirPath() + "/color-schemes";
    }

    QStringList discoverThemes() const
    {
        QStringList themes;
        themes << "Tango" << "Borland" << "GreenOnBlack" << "Solarized"
               << "SolarizedLight" << "DarkPastels" << "Ubuntu" << "WhiteOnBlack";

        QDir dir(colorSchemesDir());
        if (dir.exists()) {
            QDirIterator it(dir.absolutePath(),
                            QStringList() << "*.colorscheme",
                            QDir::Files,
                            QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                QFileInfo fi(it.next());
                const QString base = fi.completeBaseName();
                if (!themes.contains(base, Qt::CaseInsensitive)) {
                    themes << base;
                }
            }
        }
        return themes;
    }

    void applySchemeToTerminal(QTermWidget *terminal, const QString &themeName)
    {
        if (!terminal) return;
        const QString candidate = QDir(colorSchemesDir())
                                  .filePath(themeName + ".colorscheme");
        if (QFileInfo::exists(candidate)) {
            terminal->setColorScheme(candidate);
        } else {
            terminal->setColorScheme(themeName);
        }
    }

    // ------------------------------------------------------------------
    //  Theme settings
    // ------------------------------------------------------------------

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
            for (int i = 0; i < tabWidget->count(); i++) {
                QTermWidget *terminal = qobject_cast<QTermWidget*>(tabWidget->widget(i));
                if (terminal) {
                    applySchemeToTerminal(terminal, savedTheme);
                }
            }
            updateThemeMenuChecks(savedTheme);
            statusBar()->showMessage("Loaded theme: " + savedTheme, 2000);
        }
    }

    void updateThemeMenuChecks(const QString &selectedTheme)
    {
        for (auto it = themeActions.begin(); it != themeActions.end(); ++it) {
            it.value()->setChecked(it.key() == selectedTheme);
        }
    }

    // ------------------------------------------------------------------
    //  Menu construction
    // ------------------------------------------------------------------

    void createMenu()
    {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);

        // File menu
        QMenu *fileMenu = menuBar->addMenu("&File");

        QAction *newTabAction = new QAction("&New Tab", this);
       // newTabAction->setShortcut(QKeySequence::AddTab);
        newTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
        connect(newTabAction, &QAction::triggered, this, &TerminalWindow::addNewTab);
        fileMenu->addAction(newTabAction);

        QAction *closeTabAction = new QAction("&Close Tab", this);
       // closeTabAction->setShortcut(QKeySequence::Close);
        closeTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
        connect(closeTabAction, &QAction::triggered, this, &TerminalWindow::closeCurrentTab);
        fileMenu->addAction(closeTabAction);

        fileMenu->addSeparator();

        QAction *reloadCommandsAction = new QAction("&Reload Commands", this);
        connect(reloadCommandsAction, &QAction::triggered,
                this, &TerminalWindow::reloadCommands);
        fileMenu->addAction(reloadCommandsAction);

        QAction *reloadHistoryAction = new QAction("Reload &History", this);
        connect(reloadHistoryAction, &QAction::triggered,
                this, &TerminalWindow::reloadHistory);
        fileMenu->addAction(reloadHistoryAction);

        QAction *reloadPanelAction = new QAction("Reload &Command && History Panel", this);
        reloadPanelAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
        connect(reloadPanelAction, &QAction::triggered,
                this, &TerminalWindow::reloadPanel);
        fileMenu->addAction(reloadPanelAction);

        fileMenu->addSeparator();

        QAction *quitAction = new QAction("&Quit", this);
        quitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
        connect(quitAction, &QAction::triggered, this, &TerminalWindow::close);
        fileMenu->addAction(quitAction);

        // View menu
        QMenu *viewMenu = menuBar->addMenu("&View");

        // --- Tabs submenu ---
        QMenu *tabsMenu = viewMenu->addMenu("&Tabs");

        hideTabsAction = new QAction("&Hide Tabs", this);
        hideTabsAction->setCheckable(true);
        hideTabsAction->setChecked(loadHideTabsFromSettings());
        connect(hideTabsAction, &QAction::toggled,
                this, &TerminalWindow::toggleHideTabs);
        tabsMenu->addAction(hideTabsAction);

        autoHideTabsAction = new QAction("&Auto Hide Tabs (if only one)", this);
        autoHideTabsAction->setCheckable(true);
        autoHideTabsAction->setChecked(loadAutoHideTabsFromSettings());
        connect(autoHideTabsAction, &QAction::toggled,
                this, &TerminalWindow::toggleAutoHideTabs);
        tabsMenu->addAction(autoHideTabsAction);

        // --- Command panel toggles ---
        showCommandPanelAction = new QAction("Show Run Command Panel", this);
        showCommandPanelAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
        showCommandPanelAction->setCheckable(true);
        showCommandPanelAction->setChecked(loadShowCommandPanelFromSettings());
        connect(showCommandPanelAction, &QAction::toggled,
                this, &TerminalWindow::toggleCommandPanel);
        viewMenu->addAction(showCommandPanelAction);

        showHistoryPanelAction = new QAction("Show &History Panel", this);
        showHistoryPanelAction->setCheckable(true);
        showHistoryPanelAction->setChecked(loadShowHistoryPanelFromSettings());
        connect(showHistoryPanelAction, &QAction::toggled,
                this, &TerminalWindow::toggleHistoryPanel);
        viewMenu->addAction(showHistoryPanelAction);

        showStatusBarAction = new QAction("Show &Status Bar", this);
        showStatusBarAction->setCheckable(true);
        showStatusBarAction->setChecked(loadShowStatusBarFromSettings());
        connect(showStatusBarAction, &QAction::toggled,
                this, &TerminalWindow::toggleStatusBar);
        viewMenu->addAction(showStatusBarAction);

        viewMenu->addSeparator();

        // Font submenu
        QMenu *fontMenu = viewMenu->addMenu("&Font");

        QAction *chooseFontAction = new QAction("&Choose Font...", this);
        connect(chooseFontAction, &QAction::triggered, this, &TerminalWindow::chooseFont);
        fontMenu->addAction(chooseFontAction);

        fontMenu->addSeparator();

        QAction *incFontAction = new QAction("&Increase Size", this);
        incFontAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
        connect(incFontAction, &QAction::triggered, this, &TerminalWindow::increaseFontSize);
        fontMenu->addAction(incFontAction);

        QAction *decFontAction = new QAction("&Decrease Size", this);
        decFontAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
        connect(decFontAction, &QAction::triggered, this, &TerminalWindow::decreaseFontSize);
        fontMenu->addAction(decFontAction);

        fontMenu->addSeparator();

        QAction *resetFontAction = new QAction("&Reset to Default", this);
        connect(resetFontAction, &QAction::triggered, this, &TerminalWindow::resetFont);
        fontMenu->addAction(resetFontAction);

        viewMenu->addSeparator();

        QAction *nextTabAction = new QAction("Next Tab", this);
        nextTabAction->setShortcut(QKeySequence::NextChild);
        connect(nextTabAction, &QAction::triggered, this, &TerminalWindow::nextTab);
        viewMenu->addAction(nextTabAction);

        QAction *prevTabAction = new QAction("Previous Tab", this);
        prevTabAction->setShortcut(QKeySequence::PreviousChild);
        connect(prevTabAction, &QAction::triggered, this, &TerminalWindow::previousTab);
        viewMenu->addAction(prevTabAction);

        // Themes menu (applies to all tabs)
        themeMenu = menuBar->addMenu("&Themes");

        QStringList themes = discoverThemes();
        QString savedTheme = loadThemeFromSettings();

        for (const QString &theme : themes) {
            QAction *themeAction = new QAction(theme, this);
            themeAction->setCheckable(true);
            themeAction->setChecked(theme == savedTheme);
            connect(themeAction, &QAction::triggered, this, [this, theme]() {
                setTheme(theme);
            });
            themeMenu->addAction(themeAction);
            themeActions[theme] = themeAction;
        }

        themeMenu->addSeparator();
        QAction *reloadAction = new QAction("&Reload Themes from Disk", this);
        connect(reloadAction, &QAction::triggered, this, &TerminalWindow::reloadThemes);
        themeMenu->addAction(reloadAction);

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

    // Give the current terminal keyboard focus so the user can type right away
    window.focusCurrentTerminal();

    return app.exec();
}
