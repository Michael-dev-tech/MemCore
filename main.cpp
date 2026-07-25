#include <QApplication>
#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QTextBrowser>
#include <QSplitter>
#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QScrollBar>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QPdfWriter>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QDir>
#include <QSaveFile>
#include <QMimeData>
#include <QFileInfo>
#include <QFile>
#include <QPropertyAnimation>
#include <QDialog>
#include <QShortcut>
#include <QStatusBar>
#include <QSettings>
#include <QListWidget>
#include <QStackedWidget>
#include <QCheckBox>
#include <set>
#include <filesystem>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <functional>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

namespace fs = std::filesystem;

QByteArray encryptAES(const QByteArray &plaintext, const unsigned char* key) {
    unsigned char iv[16];
    RAND_bytes(iv, sizeof(iv)); 

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    QByteArray ciphertext;
    ciphertext.resize(plaintext.length() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int outlen1 = 0, outlen2 = 0;

    EVP_EncryptUpdate(ctx, (unsigned char*)ciphertext.data(), &outlen1, (const unsigned char*)plaintext.constData(), plaintext.length());
    EVP_EncryptFinal_ex(ctx, (unsigned char*)ciphertext.data() + outlen1, &outlen2);
    ciphertext.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);

    QByteArray result;
    result.append("ORBENC__", 8); 
    result.append((const char*)iv, 16);
    result.append(ciphertext);
    return result.toBase64(); 
}

QByteArray decryptAES(const QByteArray &base64Text, const unsigned char* key) {
    QByteArray encrypted = QByteArray::fromBase64(base64Text);
    
    if (!encrypted.startsWith("ORBENC__")) return base64Text; 

    unsigned char iv[16];
    memcpy(iv, encrypted.constData() + 8, 16);
    QByteArray ciphertext = encrypted.mid(24);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    QByteArray plaintext;
    plaintext.resize(ciphertext.length());
    int outlen1 = 0, outlen2 = 0;

    if (1 != EVP_DecryptUpdate(ctx, (unsigned char*)plaintext.data(), &outlen1, (const unsigned char*)ciphertext.constData(), ciphertext.length())) {
        EVP_CIPHER_CTX_free(ctx); return QByteArray();
    }
    if (1 != EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext.data() + outlen1, &outlen2)) {
        EVP_CIPHER_CTX_free(ctx); return QByteArray(); 
    }
    plaintext.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

// --- NOU: Fereastra de Setări Stilizată (Obsidian Settings Modal) ---
class SettingsDialog : public QDialog {
public:
    SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Orbit Settings");
        resize(800, 550);
        setStyleSheet("background-color: #1e1e1e; color: #dcddde; font-family: -apple-system, sans-serif;");

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Meniul lateral din setări (Categorii)
        QListWidget *categoryList = new QListWidget(this);
        categoryList->setFixedWidth(220);
        categoryList->setStyleSheet(
            "QListWidget { background-color: #252525; border-right: 1px solid #2a2a2a; padding-top: 20px; outline: none; }"
            "QListWidget::item { padding: 10px 16px; color: #a1a1aa; border-radius: 4px; margin: 2px 10px; }"
            "QListWidget::item:selected { background-color: #8b5cf6; color: white; font-weight: 500; }"
            "QListWidget::item:hover:!selected { background-color: rgba(255,255,255,0.03); color: #e4e4e7; }"
        );

        categoryList->addItem("General");
        categoryList->addItem("Editor");
        categoryList->addItem("Files and links");
        categoryList->addItem("Appearance");
        categoryList->addItem("Hotkeys");
        categoryList->addItem("Core plugins");
        categoryList->addItem("Community plugins");

        // Panoul de conținut din dreapta
        QStackedWidget *stackedWidget = new QStackedWidget(this);
        stackedWidget->setStyleSheet("background-color: #1e1e1e; padding: 20px;");

        // Pagina General
        QWidget *generalPage = new QWidget();
        QVBoxLayout *genLayout = new QVBoxLayout(generalPage);
        genLayout->addWidget(new QLabel("<h2 style='color:#e4e4e7;'>General Settings</h2>"));
        genLayout->addWidget(new QLabel("Manage your application behavior and startup preferences."));
        genLayout->addStretch();

        // Pagina Appearance
        QWidget *appearancePage = new QWidget();
        QVBoxLayout *appLayout = new QVBoxLayout(appearancePage);
        appLayout->addWidget(new QLabel("<h2 style='color:#e4e4e7;'>Appearance</h2>"));
        appLayout->addWidget(new QLabel("Customize how Orbit looks on your device."));
        appLayout->addStretch();

        // Pagina Plugins (Core/Community)
        QWidget *pluginsPage = new QWidget();
        QVBoxLayout *plugLayout = new QVBoxLayout(pluginsPage);
        plugLayout->addWidget(new QLabel("<h2 style='color:#e4e4e7;'>Core Plugins</h2>"));
        plugLayout->addWidget(new QLabel("Enable or disable built-in features like Backlinks, Canvas, and Graph View."));
        plugLayout->addStretch();

        stackedWidget->addWidget(generalPage);
        stackedWidget->addWidget(appearancePage);
        stackedWidget->addWidget(new QWidget()); // Editor placeholder
        stackedWidget->addWidget(new QWidget()); // Files placeholder
        stackedWidget->addWidget(new QWidget()); // Hotkeys placeholder
        stackedWidget->addWidget(pluginsPage);
        stackedWidget->addWidget(new QWidget()); // Community plugins placeholder

        connect(categoryList, &QListWidget::currentRowChanged, stackedWidget, &QStackedWidget::setCurrentIndex);
        categoryList->setCurrentRow(0);

        mainLayout->addWidget(categoryList);
        mainLayout->addWidget(stackedWidget);
    }
};

enum NodeType { FILE_NODE, TAG_NODE };

class Edge;

class Node : public QGraphicsEllipseItem {
public:
    Node(const QString& name, NodeType type = FILE_NODE) : QGraphicsEllipseItem(-18, -18, 36, 36), name(name) {
        setFlag(ItemIsMovable); setFlag(ItemSendsGeometryChanges);
        if (type == TAG_NODE) { setBrush(QColor("#a1a1aa")); setRect(-14, -14, 28, 28); } 
        else { setBrush(QColor("#8b5cf6")); }
        setPen(QPen(QColor(128, 128, 128, 40), 1)); 
        textItem = new QGraphicsTextItem(name, this);
        textItem->setDefaultTextColor(QColor("#dcddde")); 
        QFont modernFont("-apple-system", type == TAG_NODE ? 10 : 11);
        if (type == TAG_NODE) modernFont.setBold(true);
        textItem->setFont(modernFont);
        textItem->setPos(-textItem->boundingRect().width() / 2, (type == TAG_NODE) ? 16 : 22);
    }
    QString name; std::vector<Edge*> edges; QPointF velocity; 
    void setTextColor(const QColor& color) { textItem->setDefaultTextColor(color); }
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
private:
    QGraphicsTextItem* textItem;
};

class Edge : public QGraphicsLineItem {
public:
    Edge(Node* source, Node* dest) : source(source), dest(dest) {
        setPen(QPen(QColor("#404040"), 1.0)); setZValue(-1); adjust();
    }
    Node* source; Node* dest;
    void adjust() { if (source && dest) setLine(QLineF(source->pos(), dest->pos())); }
};

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) for (Edge* edge : edges) edge->adjust(); 
    return QGraphicsEllipseItem::itemChange(change, value);
}

class CommandPalette : public QDialog {
public:
    QLineEdit *searchBox;
    QListWidget *listWidget;
    std::unordered_map<QString, std::function<void()>> commands;

    CommandPalette(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
        setFixedSize(400, 300);
        setStyleSheet("background-color: #252525; color: #dcddde; border: 1px solid #3a3a3a; border-radius: 8px;");
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        searchBox = new QLineEdit(this);
        searchBox->setPlaceholderText("Type a command...");
        searchBox->setStyleSheet("background-color: #1e1e1e; border: 1px solid #333; padding: 10px; font-size: 14px; border-radius: 4px;");
        
        listWidget = new QListWidget(this);
        listWidget->setStyleSheet("QListWidget { border: none; font-size: 13px; outline: none; } QListWidget::item { padding: 8px; border-radius: 4px; } QListWidget::item:selected { background-color: #8b5cf6; color: white; }");
        
        layout->addWidget(searchBox);
        layout->addWidget(listWidget);

        connect(searchBox, &QLineEdit::textChanged, this, &CommandPalette::filterCommands);
        connect(listWidget, &QListWidget::itemActivated, this, &CommandPalette::executeCommand);
        connect(searchBox, &QLineEdit::returnPressed, [this]() {
            if(listWidget->count() > 0) { listWidget->setCurrentRow(0); executeCommand(listWidget->item(0)); }
        });
    }

    void addCommand(const QString& name, std::function<void()> func) {
        commands[name] = func;
        listWidget->addItem(name);
    }

    void filterCommands(const QString& text) {
        for (int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem* item = listWidget->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    }

    void executeCommand(QListWidgetItem* item) {
        if (item) { commands[item->text()](); accept(); }
    }
};

class MarkdownEditor : public QTextEdit {
public:
    std::string vaultPath;
    MarkdownEditor(QWidget *parent = nullptr) : QTextEdit(parent) {}
protected:
    void insertFromMimeData(const QMimeData *source) override {
        if (source->hasUrls()) {
            for (const QUrl &url : source->urls()) {
                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile();
                    QFileInfo fileInfo(filePath);
                    QString suffix = fileInfo.suffix().toLower();
                    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "gif" || suffix == "webp") {
                        QDir vaultDir(QString::fromStdString(vaultPath));
                        if (!vaultDir.exists(".assets")) vaultDir.mkdir(".assets");
                        QString safeBaseName = fileInfo.baseName().replace(" ", "_");
                        QString newFileName = safeBaseName + "." + suffix;
                        QString destination = vaultDir.absoluteFilePath(".assets/" + newFileName);
                        int counter = 1;
                        while (QFile::exists(destination)) {
                            newFileName = safeBaseName + "_" + QString::number(counter) + "." + suffix;
                            destination = vaultDir.absoluteFilePath(".assets/" + newFileName);
                            counter++;
                        }
                        if (QFile::copy(filePath, destination)) {
                            textCursor().insertText(QString("\n![%1](.assets/%2)\n\n").arg(safeBaseName, newFileName));
                        }
                        continue; 
                    }
                }
            } return; 
        } QTextEdit::insertFromMimeData(source);
    }
};

class MarkdownHighlighter : public QSyntaxHighlighter {
public:
    MarkdownHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {}
protected:
    void highlightBlock(const QString &text) override {
        QTextCharFormat linkFormat; linkFormat.setForeground(QColor("#a78bfa")); 
        QTextCharFormat tagFormat; tagFormat.setForeground(QColor("#71717a")); 
        QTextCharFormat headerFormat; headerFormat.setFontWeight(QFont::Bold); headerFormat.setForeground(QColor("#e4e4e7"));
        QTextCharFormat yamlFormat; yamlFormat.setForeground(QColor("#52525b")); yamlFormat.setFontItalic(true);
        QTextCharFormat todoFormat; todoFormat.setForeground(QColor("#71717a")); 
        QTextCharFormat doneFormat; doneFormat.setForeground(QColor("#52525b")); doneFormat.setFontStrikeOut(true);

        QRegularExpression yamlRegex("^---[\\s\\S]*?---");
        QRegularExpressionMatchIterator i = yamlRegex.globalMatch(text);
        while (i.hasNext()) { QRegularExpressionMatch match = i.next(); setFormat(match.capturedStart(), match.capturedLength(), yamlFormat); }

        QRegularExpression linkRegex("\\[\\[(.*?)\\]\\]"); i = linkRegex.globalMatch(text);
        while (i.hasNext()) { QRegularExpressionMatch match = i.next(); setFormat(match.capturedStart(), match.capturedLength(), linkFormat); }

        QRegularExpression tagRegex("(?:^|\\s)(#[a-zA-Z_][a-zA-Z0-9_-]*)"); i = tagRegex.globalMatch(text);
        while (i.hasNext()) { QRegularExpressionMatch match = i.next(); setFormat(match.capturedStart(1), match.capturedLength(1), tagFormat); }
        
        QRegularExpression headerRegex("^#+\\s.*"); i = headerRegex.globalMatch(text);
        while (i.hasNext()) { QRegularExpressionMatch match = i.next(); setFormat(match.capturedStart(), match.capturedLength(), headerFormat); }

        QRegularExpression todoRegex("^(?:\\s*)-\\s\\[\\s\\](.*)"); i = todoRegex.globalMatch(text);
        while (i.hasNext()) { QRegularExpressionMatch match = i.next(); setFormat(match.capturedStart(), match.capturedLength(), todoFormat); }

        QRegularExpression doneRegex("^(?:\\s*)-\\s\\[[xX]\\](.*)"); i = doneRegex.globalMatch(text);
        while (i.hasNext()) { QRegularExpressionMatch match = i.next(); setFormat(match.capturedStart(), match.capturedLength(), doneFormat); }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    unsigned char global_aes_key[32];
    QString keyPath = ".orbit_key"; 
    
    QFile keyFile(keyPath);
    if (keyFile.exists()) {
        if (keyFile.open(QIODevice::ReadOnly)) {
            QByteArray keyData = keyFile.readAll();
            memcpy(global_aes_key, keyData.constData(), 32);
            keyFile.close();
        }
    } else {
        RAND_bytes(global_aes_key, sizeof(global_aes_key));
        if (keyFile.open(QIODevice::WriteOnly)) {
            keyFile.write((const char*)global_aes_key, 32);
            keyFile.close();
        }
    }

    std::string vaultPath = "../VaultTest";
    std::regex linkPattern(R"(\[\[(.*?)\]\])");
    std::regex tagPattern(R"((?:^|\s)#([a-zA-Z_][a-zA-Z0-9_-]*))");

    if (!fs::exists(vaultPath)) fs::create_directory(vaultPath);
    
    QString themeDark = R"(
        * { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; outline: 0; }
        QMainWindow, QGraphicsView { background-color: #1e1e1e; border: none; }
        QMenuBar { background-color: #1e1e1e; color: #a1a1aa; padding: 4px; border-bottom: 1px solid #2a2a2a; }
        QMenuBar::item:selected { background-color: rgba(255, 255, 255, 0.05); color: #e4e4e7; border-radius: 4px; }
        QMenu { background-color: #252525; color: #a1a1aa; border: 1px solid #333; border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 6px 30px 6px 12px; border-radius: 4px; }
        QMenu::item:selected { background-color: #8b5cf6; color: #ffffff; }
        QSplitter::handle { background-color: #2a2a2a; } 
        QWidget#leftContainer { background-color: #252525; border-right: 1px solid #2a2a2a; } 
        QTreeWidget, QListWidget { background-color: transparent; color: #a1a1aa; border: none; font-size: 13px; outline: none; }
        QTreeWidget::item { padding: 5px 8px; border-radius: 4px; margin: 1px 8px; }
        QTreeWidget::item:selected { background-color: rgba(139, 92, 246, 0.15); color: #e4e4e7; font-weight: 500; }
        QTreeWidget::item:hover:!selected { background-color: rgba(255, 255, 255, 0.03); color: #e4e4e7; }
        QTextEdit, QTextBrowser { background-color: #1e1e1e; color: #dcddde; border: none; padding: 40px 60px; font-size: 15px; line-height: 1.6; }
        QTextBrowser { border-left: 1px solid #2a2a2a; }
        QLineEdit { background-color: #1a1a1a; color: #dcddde; border: 1px solid #333; padding: 6px 10px; border-radius: 6px; margin: 12px 4px 12px 12px; }
        QLineEdit:focus { border: 1px solid #8b5cf6; }
        QPushButton#btnNewNote { background-color: transparent; color: #71717a; border: 1px solid #333; border-radius: 6px; font-weight: bold; font-size: 16px; margin: 12px 12px 12px 0px; padding: 4px 12px; }
        QPushButton#btnNewNote:hover { background-color: #333; color: #e4e4e7; }
        QTabWidget::pane { border: none; background-color: #1e1e1e; border-top: 1px solid #2a2a2a; }
        QTabBar::tab { background-color: #202020; color: #71717a; padding: 8px 24px; border-right: 1px solid #2a2a2a; border-bottom: 1px solid #2a2a2a; font-size: 12px;}
        QTabBar::tab:selected { background-color: #1e1e1e; color: #e4e4e7; border-bottom: none; }
        QTabBar::tab:hover:!selected { background-color: #252525; }
        QPushButton#btnFocus { background-color: transparent; color: #71717a; border: none; padding: 6px 12px; margin: 4px; font-size: 12px; }
        QPushButton#btnFocus:hover { color: #e4e4e7; }
        QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; }
        QScrollBar::handle:vertical { background: #3f3f46; border-radius: 3px; }
        QStatusBar { background: transparent; color: #71717a; font-size: 11px; border-top: 1px solid #2a2a2a; }
    )";

    QString themeLight = R"(
        * { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; outline: 0; }
        QMainWindow, QGraphicsView { background-color: #ffffff; border: none; }
        QMenuBar { background-color: #ffffff; color: #52525b; padding: 4px; border-bottom: 1px solid #e4e4e7; }
        QMenuBar::item:selected { background-color: rgba(0, 0, 0, 0.04); color: #18181b; border-radius: 4px; }
        QMenu { background-color: #ffffff; color: #52525b; border: 1px solid #e4e4e7; border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 6px 30px 6px 12px; border-radius: 4px; }
        QMenu::item:selected { background-color: #8b5cf6; color: #ffffff; }
        QSplitter::handle { background-color: #e4e4e7; } 
        QWidget#leftContainer { background-color: #fafafa; border-right: 1px solid #e4e4e7; } 
        QTreeWidget, QListWidget { background-color: transparent; color: #52525b; border: none; font-size: 13px; outline: none; }
        QTreeWidget::item { padding: 5px 8px; border-radius: 4px; margin: 1px 8px; }
        QTreeWidget::item:selected { background-color: rgba(139, 92, 246, 0.1); color: #18181b; font-weight: 500; }
        QTreeWidget::item:hover:!selected { background-color: rgba(0, 0, 0, 0.03); color: #18181b; }
        QTextEdit, QTextBrowser { background-color: #ffffff; color: #27272a; border: none; padding: 40px 60px; font-size: 15px; line-height: 1.6; }
        QTextBrowser { border-left: 1px solid #e4e4e7; }
        QLineEdit { background-color: #ffffff; color: #27272a; border: 1px solid #e4e4e7; padding: 6px 10px; border-radius: 6px; margin: 12px 4px 12px 12px; }
        QLineEdit:focus { border: 1px solid #8b5cf6; }
        QPushButton#btnNewNote { background-color: transparent; color: #a1a1aa; border: 1px solid #e4e4e7; border-radius: 6px; font-weight: bold; font-size: 16px; margin: 12px 12px 12px 0px; padding: 4px 12px; }
        QPushButton#btnNewNote:hover { background-color: #f4f4f5; color: #18181b; }
        QTabWidget::pane { border: none; background-color: #ffffff; border-top: 1px solid #e4e4e7; }
        QTabBar::tab { background-color: #fafafa; color: #a1a1aa; padding: 8px 24px; border-right: 1px solid #e4e4e7; border-bottom: 1px solid #e4e4e7; font-size: 12px;}
        QTabBar::tab:selected { background-color: #ffffff; color: #18181b; border-bottom: none; }
        QTabBar::tab:hover:!selected { background-color: #f4f4f5; }
        QPushButton#btnFocus { background-color: transparent; color: #a1a1aa; border: none; padding: 6px 12px; margin: 4px; font-size: 12px; }
        QPushButton#btnFocus:hover { color: #18181b; }
        QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; }
        QScrollBar::handle:vertical { background: #d4d4d8; border-radius: 3px; }
        QStatusBar { background: transparent; color: #a1a1aa; font-size: 11px; border-top: 1px solid #e4e4e7; }
    )";

    QMainWindow window;
    window.setWindowTitle("Orbit");
    window.resize(1300, 800); 

    QStatusBar *statusBar = window.statusBar();
    QLabel *statsLabel = new QLabel("Graph View Active", &window);
    statsLabel->setStyleSheet("padding-right: 20px;");
    statusBar->addPermanentWidget(statsLabel);

    QMenuBar *menuBar = new QMenuBar(&window);
    menuBar->setNativeMenuBar(false); 
    window.setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu("&File");
    QAction *actionNew = fileMenu->addAction("New Note");
    actionNew->setShortcut(QKeySequence("Ctrl+N"));
    
    QAction *actionDaily = fileMenu->addAction("Daily Note");
    actionDaily->setShortcut(QKeySequence("Ctrl+D"));
    
    QAction *actionRename = fileMenu->addAction("Rename Note");
    actionRename->setShortcut(QKeySequence("F2"));
    
    QAction *actionDelete = fileMenu->addAction("Delete Note");
    fileMenu->addSeparator();
    
    QAction *actionExportPDF = fileMenu->addAction("Export active tab as PDF...");
    fileMenu->addSeparator();
    
    QAction *actionExit = fileMenu->addAction("Exit");
    actionExit->setShortcut(QKeySequence("Ctrl+Q"));

    QMenu *viewMenu = menuBar->addMenu("&View");
    QAction *actionPalette = viewMenu->addAction("Command Palette");
    actionPalette->setShortcut(QKeySequence("Ctrl+P"));
    
    QAction *actionFocus = viewMenu->addAction("Toggle Sidebar (Focus Mode)");
    QAction *actionRefresh = viewMenu->addAction("Refresh Vault");
    actionRefresh->setShortcut(QKeySequence("F5"));
    viewMenu->addSeparator();
    
    QMenu *themeMenu = viewMenu->addMenu("Themes");
    QAction *actionThemeDark = themeMenu->addAction("Dark Mode");
    QAction *actionThemeLight = themeMenu->addAction("Light Mode");

    // --- NOU: Meniu Settings dedicat ---
    QMenu *settingsMenu = menuBar->addMenu("&Settings");
    QAction *actionOpenSettings = settingsMenu->addAction("Open Settings...");
    QObject::connect(actionOpenSettings, &QAction::triggered, [&]() {
        SettingsDialog settingsDlg(&window);
        settingsDlg.exec();
    });

    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *actionAbout = helpMenu->addAction("About Orbit");

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, &window);
    mainSplitter->setHandleWidth(1); 

    QWidget *leftContainer = new QWidget(mainSplitter);
    leftContainer->setObjectName("leftContainer");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *searchContainer = new QWidget(leftContainer);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    QLineEdit *searchBox = new QLineEdit(searchContainer);
    searchBox->setPlaceholderText("Global Search...");

    QPushButton *btnNewNote = new QPushButton("+", searchContainer);
    btnNewNote->setObjectName("btnNewNote");
    btnNewNote->setCursor(Qt::PointingHandCursor);
    btnNewNote->setToolTip("Create New Note (Ctrl+N)");
    QObject::connect(btnNewNote, &QPushButton::clicked, actionNew, &QAction::trigger);

    searchLayout->addWidget(searchBox);
    searchLayout->addWidget(btnNewNote);
    leftLayout->addWidget(searchContainer);

    QSplitter *leftSplitter = new QSplitter(Qt::Vertical, leftContainer);
    leftSplitter->setHandleWidth(1);
    
    QTreeWidget *treeWidget = new QTreeWidget(leftSplitter);
    treeWidget->setHeaderHidden(true); 

    QWidget *backlinksContainer = new QWidget(leftSplitter);
    QVBoxLayout *backlinksLayout = new QVBoxLayout(backlinksContainer);
    QLabel *backlinksLabel = new QLabel("BACKLINKS", backlinksContainer);
    backlinksLabel->setStyleSheet("color: #71717a; font-weight: 600; padding: 4px; font-size: 10px; letter-spacing: 1px;");
    QListWidget *backlinksList = new QListWidget(backlinksContainer);
    backlinksLayout->addWidget(backlinksLabel);
    backlinksLayout->addWidget(backlinksList);

    QWidget *tagsContainer = new QWidget(leftSplitter);
    QVBoxLayout *tagsLayout = new QVBoxLayout(tagsContainer);
    QLabel *tagsLabel = new QLabel("TAGS", tagsContainer);
    tagsLabel->setStyleSheet("color: #71717a; font-weight: 600; padding: 4px; font-size: 10px; letter-spacing: 1px;");
    QListWidget *tagsList = new QListWidget(tagsContainer);
    tagsLayout->addWidget(tagsLabel);
    tagsLayout->addWidget(tagsList);

    // --- NOU: Footer discret în stânga jos (Obsidian-Style Vault Name) ---
    QWidget *vaultFooter = new QWidget(leftContainer);
    QHBoxLayout *footerLayout = new QHBoxLayout(vaultFooter);
    footerLayout->setContentsMargins(12, 8, 12, 8);
    QLabel *vaultNameLabel = new QLabel("📁 Orbit Vault", vaultFooter);
    vaultNameLabel->setStyleSheet("color: #71717a; font-size: 11px; font-weight: 500;");
    QPushButton *btnSettingsFooter = new QPushButton("⚙️", vaultFooter);
    btnSettingsFooter->setFixedSize(24, 24);
    btnSettingsFooter->setStyleSheet("background: transparent; border: none; font-size: 12px;");
    QObject::connect(btnSettingsFooter, &QPushButton::clicked, [&]() {
        SettingsDialog settingsDlg(&window);
        settingsDlg.exec();
    });
    footerLayout->addWidget(vaultNameLabel);
    footerLayout->addStretch();
    footerLayout->addWidget(btnSettingsFooter);

    leftSplitter->setSizes(QList<int>() << 450 << 175 << 175);
    leftLayout->addWidget(leftSplitter);
    leftLayout->addWidget(vaultFooter); // Adăugat jos în stânga

    QTabWidget *rightTabs = new QTabWidget(mainSplitter);
    rightTabs->setTabsClosable(true); 
    QPushButton *btnFocus = new QPushButton("Toggle Sidebar", rightTabs);
    btnFocus->setObjectName("btnFocus");
    rightTabs->setCornerWidget(btnFocus, Qt::TopRightCorner);

    QGraphicsScene *graphScene = new QGraphicsScene();
    QGraphicsView *graphView = new QGraphicsView(graphScene);
    graphView->setRenderHint(QPainter::Antialiasing); graphView->setDragMode(QGraphicsView::ScrollHandDrag); 
    
    rightTabs->addTab(graphView, "Graph Map");
    rightTabs->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr); 
    rightTabs->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr);

    mainSplitter->setSizes(QList<int>() << 260 << 1040); 
    window.setCentralWidget(mainSplitter);

    auto activeNodes = std::make_shared<std::vector<Node*>>();
    auto isPhysicsEnabled = std::make_shared<bool>(true);
    auto currentGraphTextColor = std::make_shared<QString>("#dcddde"); 
    auto globalGraph = std::make_shared<std::unordered_map<std::string, std::vector<std::string>>>();
    auto globalTags = std::make_shared<std::unordered_map<std::string, std::vector<std::string>>>(); 
    auto globalFileContents = std::make_shared<std::unordered_map<std::string, QString>>(); 

    auto formatPreviewMarkdown = [](QString md) -> QString {
        QRegularExpression yamlRegex("^---\\n([\\s\\S]*?)\\n---");
        return md.replace(yamlRegex, ""); 
    };

    auto updateStats = [statsLabel](QTextEdit* editor) {
        if (!editor) { statsLabel->setText("Graph View Active"); return; }
        QString text = editor->toPlainText();
        int chars = text.length();
        int words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
        if (text.trimmed().isEmpty()) words = 0;
        int readingTime = std::max(1, words / 200); 
        if (words == 0) readingTime = 0;
        statsLabel->setText(QString("%1 Words  •  %2 Characters  •  %3 min read").arg(words).arg(chars).arg(readingTime));
    };

    auto openNoteInTab = [&](const std::string& fullPath, const QString& tabName) {
        for (int i = 1; i < rightTabs->count(); ++i) {
            if (rightTabs->widget(i)->property("file_path").toString().toStdString() == fullPath) {
                rightTabs->setCurrentIndex(i);
                return;
            }
        }

        QFile file(QString::fromStdString(fullPath));
        if (!file.open(QIODevice::ReadOnly)) return;
        QByteArray raw = file.readAll();
        QString text = QString::fromUtf8(decryptAES(raw, global_aes_key));

        QSplitter *textSplitter = new QSplitter(Qt::Horizontal, rightTabs);
        textSplitter->setHandleWidth(1);
        textSplitter->setProperty("file_path", QString::fromStdString(fullPath)); 

        MarkdownEditor *textEdit = new MarkdownEditor(textSplitter);
        textEdit->vaultPath = vaultPath;
        new MarkdownHighlighter(textEdit->document());
        textEdit->setPlainText(text);

        QTextBrowser *markdownPreview = new QTextBrowser(textSplitter);
        markdownPreview->setOpenExternalLinks(true); 
        markdownPreview->document()->setBaseUrl(QUrl::fromLocalFile(QDir(QString::fromStdString(vaultPath)).absolutePath() + "/"));
        
        QString previewCSS = (*currentGraphTextColor == "#dcddde") ? 
            R"( body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; font-size: 15px; color: #dcddde; line-height: 1.6; } h1, h2, h3 { color: #e4e4e7; margin-top: 1.2em; margin-bottom: 0.5em; font-weight: 600; } h1 { border-bottom: 1px solid #2a2a2a; padding-bottom: 0.2em; } img { max-width: 100%; border-radius: 6px; } blockquote { border-left: 3px solid #8b5cf6; margin-left: 0; padding-left: 1em; color: #a1a1aa; font-style: italic; } code { background-color: #252525; padding: 0.2em 0.4em; border-radius: 4px; font-family: monospace; font-size: 13px; color: #a78bfa; } pre code { display: block; padding: 1em; background-color: #1a1a1a; overflow-x: auto; color: #dcddde; border: 1px solid #2a2a2a;} hr { border: 0; border-top: 1px solid #2a2a2a; margin: 2em 0; } a { color: #a78bfa; text-decoration: none; } a:hover { text-decoration: underline; } )" :
            R"( body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; font-size: 15px; color: #27272a; line-height: 1.6; } h1, h2, h3 { color: #18181b; margin-top: 1.2em; margin-bottom: 0.5em; font-weight: 600; } h1 { border-bottom: 1px solid #e4e4e7; padding-bottom: 0.2em; } img { max-width: 100%; border-radius: 6px; } blockquote { border-left: 3px solid #8b5cf6; margin-left: 0; padding-left: 1em; color: #71717a; font-style: italic; } code { background-color: #f4f4f5; padding: 0.2em 0.4em; border-radius: 4px; font-family: monospace; font-size: 13px; color: #7c3aed; } pre code { display: block; padding: 1em; background-color: #fafafa; overflow-x: auto; color: #27272a; border: 1px solid #e4e4e7;} hr { border: 0; border-top: 1px solid #e4e4e7; margin: 2em 0; } a { color: #7c3aed; text-decoration: none; } a:hover { text-decoration: underline; } )";
        
        markdownPreview->document()->setDefaultStyleSheet(previewCSS);
        markdownPreview->setMarkdown(formatPreviewMarkdown(text));

        QScrollBar *leftScroll = textEdit->verticalScrollBar();
        QScrollBar *rightScroll = markdownPreview->verticalScrollBar();
        QPropertyAnimation *smoothScrollAnim = new QPropertyAnimation(rightScroll, "value", textSplitter);
        smoothScrollAnim->setDuration(150); smoothScrollAnim->setEasingCurve(QEasingCurve::OutCubic); 
        QObject::connect(leftScroll, &QScrollBar::valueChanged, [leftScroll, rightScroll, smoothScrollAnim](int value) {
            if (leftScroll->maximum() > 0) {
                double ratio = (double)value / leftScroll->maximum();
                int targetValue = ratio * rightScroll->maximum();
                if (smoothScrollAnim->state() == QAbstractAnimation::Running) smoothScrollAnim->stop();
                smoothScrollAnim->setStartValue(rightScroll->value()); smoothScrollAnim->setEndValue(targetValue); smoothScrollAnim->start();
            }
        });

        textSplitter->setSizes(QList<int>() << 500 << 500); 
        int newIndex = rightTabs->addTab(textSplitter, tabName);
        rightTabs->setCurrentIndex(newIndex);

        auto isProgrammaticChange = std::make_shared<bool>(false);
        QObject::connect(textEdit, &QTextEdit::textChanged, [=, &global_aes_key]() {
            if (!*isProgrammaticChange) {
                QSaveFile sFile(QString::fromStdString(fullPath));
                if (sFile.open(QIODevice::WriteOnly)) {
                    QByteArray out = encryptAES(textEdit->toPlainText().toUtf8(), global_aes_key); 
                    sFile.write(out);
                    sFile.commit(); 
                }
                markdownPreview->setMarkdown(formatPreviewMarkdown(textEdit->toPlainText()));
                if (rightTabs->currentWidget() == textSplitter) updateStats(textEdit);
            }
        });
    };

    QObject::connect(rightTabs, &QTabWidget::tabCloseRequested, [&](int index) {
        if (index == 0) return; 
        QWidget* widget = rightTabs->widget(index);
        rightTabs->removeTab(index);
        widget->deleteLater();
    });

    QObject::connect(rightTabs, &QTabWidget::currentChanged, [&](int index) {
        if (index == 0) { updateStats(nullptr); return; }
        QTextEdit* editor = rightTabs->widget(index)->findChild<QTextEdit*>();
        updateStats(editor);
    });

    std::shared_ptr<QTimer> physicsTimer = std::make_shared<QTimer>();
    QObject::connect(physicsTimer.get(), &QTimer::timeout, [activeNodes, isPhysicsEnabled]() {
        if (!(*isPhysicsEnabled)) return; 
        bool needsUpdate = false;
        for (Node* n1 : *activeNodes) {
            if (n1->scene() && n1->scene()->mouseGrabberItem() == n1) {
                n1->velocity = QPointF(0, 0); needsUpdate = true; continue;
            }
            QPointF force(0, 0);
            for (Node* n2 : *activeNodes) {
                if (n1 == n2) continue;
                QPointF d = n1->pos() - n2->pos();
                double dist = std::max(1.0, std::sqrt(d.x()*d.x() + d.y()*d.y()));
                force += (d / dist) * (5000.0 / (dist * dist));
            }
            for (Edge* e : n1->edges) {
                Node* n2 = (e->source == n1) ? e->dest : e->source;
                QPointF d = n2->pos() - n1->pos();
                double dist = std::max(1.0, std::sqrt(d.x()*d.x() + d.y()*d.y()));
                force += (d / dist) * ((dist - 180.0) * 0.04); 
            }
            QPointF dCenter = QPointF(0,0) - n1->pos();
            double distCenter = std::max(1.0, std::sqrt(dCenter.x()*dCenter.x() + dCenter.y()*dCenter.y()));
            force += (dCenter / distCenter) * (distCenter * 0.010);
            n1->velocity = (n1->velocity + force) * 0.80; 
            
            if (std::abs(n1->velocity.x()) < 0.1) n1->velocity.setX(0);
            if (std::abs(n1->velocity.y()) < 0.1) n1->velocity.setY(0);
            if (n1->velocity.x() != 0 || n1->velocity.y() != 0) needsUpdate = true;
        }
        if (needsUpdate) {
            for (Node* n : *activeNodes) {
                if (n->scene() && n->scene()->mouseGrabberItem() != n) n->setPos(n->pos() + n->velocity);
            }
        }
    });

    auto reloadSystem = [&]() {
        activeNodes->clear(); treeWidget->clear(); graphScene->clear();
        globalGraph->clear(); globalTags->clear(); globalFileContents->clear();
        std::unordered_map<std::string, Node*> nodesMap;
        std::unordered_map<std::string, QTreeWidgetItem*> folderMap;
        std::set<std::string> uniqueTags; 

        if (fs::exists(vaultPath)) {
            for (const auto& entry : fs::recursive_directory_iterator(vaultPath)) {
                if (entry.path().filename().string().rfind(".", 0) == 0) continue; 

                if (entry.is_regular_file() && entry.path().extension() == ".md") {
                    std::string relPath = fs::relative(entry.path(), vaultPath).string();
                    std::string filename = entry.path().filename().string();
                    std::string folderPath = fs::relative(entry.path().parent_path(), vaultPath).string();

                    (*globalGraph)[relPath] = std::vector<std::string>();
                    (*globalTags)[relPath] = std::vector<std::string>();
                    
                    QFile file(QString::fromStdString(entry.path().string()));
                    if (file.open(QIODevice::ReadOnly)) {
                        QByteArray rawData = file.readAll();
                        QString qText = QString::fromUtf8(decryptAES(rawData, global_aes_key));
                        
                        (*globalFileContents)[relPath] = qText; 

                        std::string line;
                        std::istringstream stream(qText.toStdString());
                        while (std::getline(stream, line)) {
                            std::smatch match; std::string temp = line;
                            while (std::regex_search(temp, match, linkPattern)) {
                                std::string link = match[1].str();
                                if (link.length() >= 3 && link.substr(link.length() - 3) == ".md") link = link.substr(0, link.length() - 3);
                                (*globalGraph)[relPath].push_back(link);
                                temp = match.suffix().str();
                            }
                            temp = line;
                            while (std::regex_search(temp, match, tagPattern)) {
                                std::string tag = "#" + match[1].str();
                                if (std::find((*globalTags)[relPath].begin(), (*globalTags)[relPath].end(), tag) == (*globalTags)[relPath].end())
                                    (*globalTags)[relPath].push_back(tag);
                                uniqueTags.insert(tag); 
                                temp = match.suffix().str();
                            }
                        }
                    }

                    QTreeWidgetItem *noteItem = nullptr;
                    if (folderPath != ".") {
                        if (folderMap.find(folderPath) == folderMap.end()) {
                            QTreeWidgetItem *folderItem = new QTreeWidgetItem(treeWidget);
                            folderItem->setText(0, QString::fromStdString("📁 " + folderPath));
                            folderItem->setForeground(0, QBrush(QColor("#71717a"))); 
                            folderMap[folderPath] = folderItem;
                        }
                        noteItem = new QTreeWidgetItem(folderMap[folderPath]);
                    } else {
                        noteItem = new QTreeWidgetItem(treeWidget);
                    }

                    noteItem->setText(0, QString::fromStdString(filename));
                    noteItem->setData(0, Qt::UserRole, QString::fromStdString(relPath)); 
                    
                    for (const std::string& link : (*globalGraph)[relPath]) {
                        QTreeWidgetItem *c = new QTreeWidgetItem(noteItem);
                        c->setText(0, QString::fromStdString("↗ " + link)); c->setForeground(0, QBrush(QColor("#71717a"))); 
                    }
                    for (const std::string& tag : (*globalTags)[relPath]) {
                        QTreeWidgetItem *c = new QTreeWidgetItem(noteItem);
                        c->setText(0, QString::fromStdString("🏷️ " + tag)); c->setForeground(0, QBrush(QColor("#a78bfa"))); 
                    }
                }
            }
        }
        treeWidget->expandAll();

        tagsList->clear();
        for (const std::string& t : uniqueTags) {
            QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(t), tagsList);
            item->setForeground(QBrush(QColor("#a78bfa")));
        }

        for (const auto& pair : *globalGraph) {
            std::string nName = std::filesystem::path(pair.first).filename().string();
            if (nodesMap.find(nName) == nodesMap.end()) {
                Node* n = new Node(QString::fromStdString(nName), FILE_NODE);
                nodesMap[nName] = n; graphScene->addItem(n); activeNodes->push_back(n);
                n->setPos(rand() % 500 - 250, rand() % 500 - 250); 
            }
            for(const auto& link : pair.second) {
                if (nodesMap.find(link) == nodesMap.end()) {
                     Node* n = new Node(QString::fromStdString(link), FILE_NODE);
                     nodesMap[link] = n; graphScene->addItem(n); activeNodes->push_back(n);
                     n->setPos(rand() % 500 - 250, rand() % 500 - 250);
                }
            }
        }
        for (const auto& pair : *globalGraph) {
            Node* sNode = nodesMap[std::filesystem::path(pair.first).filename().string()];
            for (const std::string& link : pair.second) {
                Node* dNode = nodesMap[link];
                Edge* edge = new Edge(sNode, dNode);
                graphScene->addItem(edge); sNode->edges.push_back(edge); dNode->edges.push_back(edge);
            }
        }
        
        for (Node* n : *activeNodes) {
            n->setTextColor(QColor(*currentGraphTextColor));
        }
        
        physicsTimer->start(16); 
    };
    
    reloadSystem();

    auto applyThemeToAllTabs = [&](const QString& css) {
        for (int i = 1; i < rightTabs->count(); ++i) {
            QTextBrowser* preview = rightTabs->widget(i)->findChild<QTextBrowser*>();
            QTextEdit* editor = rightTabs->widget(i)->findChild<QTextEdit*>();
            if (preview && editor) {
                preview->document()->setDefaultStyleSheet(css);
                preview->setMarkdown(formatPreviewMarkdown(editor->toPlainText()));
            }
        }
    };

    QObject::connect(actionThemeDark, &QAction::triggered, [&]() {
        *currentGraphTextColor = "#dcddde"; 
        app.setStyleSheet(themeDark); 
        for (Node* n : *activeNodes) n->setTextColor(QColor(*currentGraphTextColor)); 
        
        QString darkPreviewCSS = R"( body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; font-size: 15px; color: #dcddde; line-height: 1.6; } h1, h2, h3 { color: #e4e4e7; margin-top: 1.2em; margin-bottom: 0.5em; font-weight: 600; } h1 { border-bottom: 1px solid #2a2a2a; padding-bottom: 0.2em; } img { max-width: 100%; border-radius: 6px; } blockquote { border-left: 3px solid #8b5cf6; margin-left: 0; padding-left: 1em; color: #a1a1aa; font-style: italic; } code { background-color: #252525; padding: 0.2em 0.4em; border-radius: 4px; font-family: monospace; font-size: 13px; color: #a78bfa; } pre code { display: block; padding: 1em; background-color: #1a1a1a; overflow-x: auto; color: #dcddde; border: 1px solid #2a2a2a;} hr { border: 0; border-top: 1px solid #2a2a2a; margin: 2em 0; } a { color: #a78bfa; text-decoration: none; } a:hover { text-decoration: underline; } )";
        applyThemeToAllTabs(darkPreviewCSS);

        QSettings settings("Orbit", "EnterpriseEdition");
        settings.setValue("theme", "dark");
    });
    
    QObject::connect(actionThemeLight, &QAction::triggered, [&]() {
        *currentGraphTextColor = "#18181b"; 
        app.setStyleSheet(themeLight); 
        for (Node* n : *activeNodes) n->setTextColor(QColor(*currentGraphTextColor)); 
        
        QString lightPreviewCSS = R"( body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; font-size: 15px; color: #27272a; line-height: 1.6; } h1, h2, h3 { color: #18181b; margin-top: 1.2em; margin-bottom: 0.5em; font-weight: 600; } h1 { border-bottom: 1px solid #e4e4e7; padding-bottom: 0.2em; } img { max-width: 100%; border-radius: 6px; } blockquote { border-left: 3px solid #8b5cf6; margin-left: 0; padding-left: 1em; color: #71717a; font-style: italic; } code { background-color: #f4f4f5; padding: 0.2em 0.4em; border-radius: 4px; font-family: monospace; font-size: 13px; color: #7c3aed; } pre code { display: block; padding: 1em; background-color: #fafafa; overflow-x: auto; color: #27272a; border: 1px solid #e4e4e7;} hr { border: 0; border-top: 1px solid #e4e4e7; margin: 2em 0; } a { color: #7c3aed; text-decoration: none; } a:hover { text-decoration: underline; } )";
        applyThemeToAllTabs(lightPreviewCSS);

        QSettings settings("Orbit", "EnterpriseEdition");
        settings.setValue("theme", "light");
    });

    QObject::connect(actionRename, &QAction::triggered, [&]() {
        QTreeWidgetItem *item = treeWidget->currentItem();
        if (!item || item->data(0, Qt::UserRole).toString().isEmpty()) return;
        QString relPath = item->data(0, Qt::UserRole).toString();
        std::string oldName = std::filesystem::path(relPath.toStdString()).filename().string();
        std::string nameWithoutExt = oldName.length() >= 3 ? oldName.substr(0, oldName.length() - 3) : oldName;
        
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Rename Note", "New note name (without .md):", QLineEdit::Normal, QString::fromStdString(nameWithoutExt), &ok);
        if (ok && !newName.isEmpty()) {
            std::string oldFullPath = vaultPath + "/" + relPath.toStdString();
            std::string newRelPath = std::filesystem::path(relPath.toStdString()).parent_path().string();
            if(newRelPath != "" && newRelPath != ".") newRelPath += "/"; else newRelPath = "";
            newRelPath += newName.toStdString() + ".md";
            std::string newFullPath = vaultPath + "/" + newRelPath;
            
            fs::rename(oldFullPath, newFullPath);
            
            for(int i = 1; i < rightTabs->count(); ++i) {
                if (rightTabs->widget(i)->property("file_path").toString().toStdString() == oldFullPath) {
                    rightTabs->widget(i)->setProperty("file_path", QString::fromStdString(newFullPath));
                    rightTabs->setTabText(i, newName + ".md");
                }
            }
            reloadSystem();
        }
    });

    QObject::connect(actionDelete, &QAction::triggered, [&]() {
        QTreeWidgetItem *item = treeWidget->currentItem();
        if (!item || item->data(0, Qt::UserRole).toString().isEmpty()) return;
        QString relPath = item->data(0, Qt::UserRole).toString();
        auto reply = QMessageBox::question(nullptr, "Delete Note", "Are you sure you want to permanently delete '" + item->text(0) + "'?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            std::string fullPath = vaultPath + "/" + relPath.toStdString();
            fs::remove(fullPath);
            
            for(int i = 1; i < rightTabs->count(); ++i) {
                if (rightTabs->widget(i)->property("file_path").toString().toStdString() == fullPath) {
                    QWidget* w = rightTabs->widget(i);
                    rightTabs->removeTab(i);
                    w->deleteLater();
                    break;
                }
            }
            reloadSystem();
        }
    });

    QAction *treeDeleteShortcut = new QAction(treeWidget);
    treeDeleteShortcut->setShortcuts({QKeySequence::Delete, QKeySequence("Backspace")});
    treeDeleteShortcut->setShortcutContext(Qt::WidgetShortcut);
    treeWidget->addAction(treeDeleteShortcut);
    QObject::connect(treeDeleteShortcut, &QAction::triggered, actionDelete, &QAction::trigger);

    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(treeWidget, &QTreeWidget::customContextMenuRequested, [&](const QPoint &pos) {
        QTreeWidgetItem *item = treeWidget->itemAt(pos);
        if (item && !item->data(0, Qt::UserRole).toString().isEmpty()) {
            treeWidget->setCurrentItem(item);
            QMenu contextMenu;
            contextMenu.addAction(actionRename);
            contextMenu.addAction(actionDelete);
            contextMenu.exec(treeWidget->viewport()->mapToGlobal(pos));
        }
    });

    QObject::connect(actionNew, &QAction::triggered, [&]() {
        QString text = QInputDialog::getText(nullptr, "New Note", "Name:", QLineEdit::Normal, "");
        if (!text.isEmpty()) { 
            std::string fullPath = vaultPath + "/" + text.toStdString() + ".md";
            QFile file(QString::fromStdString(fullPath));
            if (file.open(QIODevice::WriteOnly)) {
                QByteArray out = encryptAES(QByteArray(""), global_aes_key);
                file.write(out);
            }
            reloadSystem(); 
            openNoteInTab(fullPath, text + ".md");
        }
    });

    QObject::connect(actionDaily, &QAction::triggered, [&]() {
        QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString fullPath = QString::fromStdString(vaultPath) + "/" + today + ".md";
        if (!QFile::exists(fullPath)) {
            QFile file(fullPath);
            if (file.open(QIODevice::WriteOnly)) {
                QString initialText = "---\ndate: " + today + "\ntags: #daily\n---\n\n# Daily Note: " + today + "\n\n- [ ] My first task\n";
                QByteArray out = encryptAES(initialText.toUtf8(), global_aes_key);
                file.write(out);
            }
            reloadSystem();
        }
        openNoteInTab(fullPath.toStdString(), today + ".md");
    });

    QObject::connect(actionExportPDF, &QAction::triggered, [&]() {
        if (rightTabs->currentIndex() == 0) {
            QMessageBox::warning(nullptr, "Export PDF", "You cannot export the Graph Map to PDF. Please select a note tab.");
            return;
        }
        QTextBrowser* preview = rightTabs->currentWidget()->findChild<QTextBrowser*>();
        if (!preview) return;

        QString fileName = QFileDialog::getSaveFileName(nullptr, "Export PDF", "", "PDF Files (*.pdf)");
        if (!fileName.isEmpty()) {
            QPdfWriter pdfWriter(fileName); pdfWriter.setResolution(300);
            preview->document()->print(&pdfWriter);
        }
    });

    QObject::connect(tagsList, &QListWidget::itemClicked, [&](QListWidgetItem *item) {
        searchBox->setText(item->text()); 
    });

    CommandPalette *palette = new CommandPalette(&window);
    palette->addCommand("Change Theme: Dark Mode", [&]() { actionThemeDark->trigger(); });
    palette->addCommand("Change Theme: Light Mode", [&]() { actionThemeLight->trigger(); });
    palette->addCommand("File: New Note", [&]() { actionNew->trigger(); });
    palette->addCommand("File: Daily Note", [&]() { actionDaily->trigger(); });
    palette->addCommand("File: Export PDF", [&]() { actionExportPDF->trigger(); });
    palette->addCommand("View: Toggle Sidebar", [&]() { btnFocus->click(); });
    palette->addCommand("Settings: Open Settings", [&]() { SettingsDialog d(&window); d.exec(); });
    
    QObject::connect(actionPalette, &QAction::triggered, [palette, &window]() {
        palette->searchBox->clear(); palette->filterCommands("");
        QPoint center = window.geometry().center() - palette->rect().center();
        palette->move(center); palette->show(); palette->searchBox->setFocus();
    });

    QObject::connect(searchBox, &QLineEdit::textChanged, [&](const QString &text) {
        for (QTreeWidgetItemIterator it(treeWidget); *it; ++it) {
            QTreeWidgetItem *item = *it;
            if (item->parent() == nullptr || item->text(0).startsWith("📁")) { 
                item->setHidden(false); continue; 
            }
            if (item->childCount() == 0 && (item->text(0).startsWith("↗") || item->text(0).startsWith("🏷️"))) continue;

            QString relPath = item->data(0, Qt::UserRole).toString();
            bool match = item->text(0).contains(text, Qt::CaseInsensitive);
            if (!match && globalFileContents->find(relPath.toStdString()) != globalFileContents->end()) {
                match = (*globalFileContents)[relPath.toStdString()].contains(text, Qt::CaseInsensitive);
            }
            item->setHidden(!match);
            for (int j = 0; j < item->childCount(); ++j) item->child(j)->setHidden(!match);
        }
    });

    QObject::connect(treeWidget, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int column) {
        if (item->text(0).startsWith("📁 ")) return;
        if (item->parent() != nullptr && !item->data(0, Qt::UserRole).isValid()) return;

        QString relPath = item->data(0, Qt::UserRole).toString();
        if (relPath.isEmpty()) relPath = item->text(0);
        std::string fullPath = vaultPath + "/" + relPath.toStdString();
        QString fileName = item->text(0);
        
        openNoteInTab(fullPath, fileName);
    });

    bool hasNotes = false;
    QTreeWidgetItemIterator it(treeWidget);
    while (*it) {
        if (!(*it)->data(0, Qt::UserRole).toString().isEmpty()) {
            hasNotes = true;
            QString relPath = (*it)->data(0, Qt::UserRole).toString();
            std::string fullPath = vaultPath + "/" + relPath.toStdString();
            openNoteInTab(fullPath, (*it)->text(0));
            break; 
        }
        ++it;
    }

    if (!hasNotes) {
        std::string welcomeFullPath = vaultPath + "/Welcome.md";
        QFile file(QString::fromStdString(welcomeFullPath));
        if (file.open(QIODevice::WriteOnly)) {
            QString welcomeText = 
                "# ✨ Welcome to Orbit\n\n"
                "Orbit is your personal, secure second brain. It helps you capture thoughts, connect ideas, and stay productive.\n\n"
                "---\n\n"
                "### 🚀 Getting Started\n"
                "- [ ] **Write something:** Click anywhere and start typing.\n"
                "- [ ] **Create a new note:** Click the `+` button in the sidebar or press `Ctrl+N`.\n"
                "- [ ] **Connect thoughts:** Type `[[` to link to another note.\n"
                "- [ ] **Organize:** Use `#tags` to categorize your ideas.\n\n"
                "### 💡 Tips & Tricks\n"
                "> \"Your mind is for having ideas, not holding them.\" - David Allen\n\n"
                "* Press `Ctrl+P` to open the Command Palette.\n"
                "* Explore the **Graph Map** tab to see how your notes connect.\n"
                "* Your data is **100% locally encrypted** and secure.\n";

            QByteArray out = encryptAES(welcomeText.toUtf8(), global_aes_key);
            file.write(out);
        }
        reloadSystem();
        openNoteInTab(welcomeFullPath, "Welcome.md");
    }

    QSettings settings("Orbit", "EnterpriseEdition");
    QString savedTheme = settings.value("theme", "dark").toString();
    if (savedTheme == "light") {
        actionThemeLight->trigger();
    } else {
        actionThemeDark->trigger();
    }

    QObject::connect(actionExit, &QAction::triggered, &app, &QApplication::quit);
    window.show();
    return app.exec();
}