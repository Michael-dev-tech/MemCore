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
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace fs = std::filesystem;

enum NodeType { FILE_NODE, TAG_NODE };

class Edge;

class Node : public QGraphicsEllipseItem {
public:
    Node(const QString& name, NodeType type = FILE_NODE) : QGraphicsEllipseItem(-18, -18, 36, 36), name(name) {
        setFlag(ItemIsMovable); 
        setFlag(ItemSendsGeometryChanges);
        
        if (type == TAG_NODE) {
            setBrush(QColor("#0ea5e9")); 
            setRect(-14, -14, 28, 28); 
        } else {
            setBrush(QColor("#8b5cf6")); 
        }
        setPen(QPen(QColor(128, 128, 128, 60), 2));
        
        textItem = new QGraphicsTextItem(name, this);
        textItem->setDefaultTextColor(QColor("#f4f4f5")); 
        
        QFont modernFont("-apple-system", type == TAG_NODE ? 10 : 11);
        if (type == TAG_NODE) modernFont.setBold(true);
        modernFont.setStyleHint(QFont::SansSerif);
        textItem->setFont(modernFont);
        
        int yOffset = (type == TAG_NODE) ? 16 : 22;
        textItem->setPos(-textItem->boundingRect().width() / 2, yOffset);
    }
    
    QString name;
    std::vector<Edge*> edges;
    QPointF velocity; 

    void setTextColor(const QColor& color) {
        textItem->setDefaultTextColor(color);
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
private:
    QGraphicsTextItem* textItem;
};

class Edge : public QGraphicsLineItem {
public:
    Edge(Node* source, Node* dest) : source(source), dest(dest) {
        setPen(QPen(QColor("#9ca3af"), 1.5)); 
        setZValue(-1); 
        adjust();
    }
    Node* source;
    Node* dest;
    void adjust() {
        if (!source || !dest) return;
        setLine(QLineF(source->pos(), dest->pos()));
    }
};

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) {
        for (Edge* edge : edges) edge->adjust(); 
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

class MarkdownHighlighter : public QSyntaxHighlighter {
public:
    MarkdownHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {}
protected:
    void highlightBlock(const QString &text) override {
        QTextCharFormat linkFormat;
        linkFormat.setForeground(QColor("#8b5cf6")); 
        linkFormat.setFontWeight(QFont::Bold);

        QTextCharFormat tagFormat;
        tagFormat.setForeground(QColor("#0ea5e9")); 
        
        QTextCharFormat headerFormat;
        headerFormat.setFontWeight(QFont::Bold);

        QRegularExpression linkRegex("\\[\\[(.*?)\\]\\]");
        QRegularExpressionMatchIterator i = linkRegex.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), linkFormat);
        }

        QRegularExpression tagRegex("(?:^|\\s)(#[a-zA-Z_][a-zA-Z0-9_-]*)");
        i = tagRegex.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(1), match.capturedLength(1), tagFormat);
        }
        
        QRegularExpression headerRegex("^#+\\s.*");
        i = headerRegex.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), headerFormat);
        }
    }
};

int main(int argc, char *argv[]) {
    std::string vaultPath = "../VaultTest";
    std::regex linkPattern(R"(\[\[(.*?)\]\])");
    std::regex tagPattern(R"((?:^|\s)#([a-zA-Z_][a-zA-Z0-9_-]*))");

    if (!fs::exists(vaultPath)) fs::create_directory(vaultPath);

    QApplication app(argc, argv);
    
    QString themeDark = R"(
        * { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
        QMainWindow, QGraphicsView { background-color: #1e1e1e; border: none; }
        QMenuBar { background-color: #18181b; color: #e4e4e7; border-bottom: 1px solid #2d2d2d; padding: 2px; }
        QMenuBar::item:selected { background-color: #27272a; color: #ffffff; }
        QMenu { background-color: #1e1e1e; color: #e4e4e7; border: 1px solid #3f3f46; border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 6px 30px 6px 12px; border-radius: 4px; }
        QMenu::item:selected { background-color: #0ea5e9; color: #ffffff; }
        QSplitter::handle { background-color: #2d2d2d; }
        QWidget#leftContainer { background-color: #18181b; } 
        QTreeWidget, QListWidget { background-color: transparent; color: #d4d4d8; border: none; font-size: 13px; outline: none; }
        QTreeWidget::item, QListWidget::item { padding: 6px 8px; border-radius: 6px; margin: 2px 8px; }
        QTreeWidget::item:selected, QListWidget::item:selected { background-color: #27272a; color: #ffffff; font-weight: 600; }
        QTreeWidget::item:hover:!selected, QListWidget::item:hover:!selected { background-color: #27272a; color: #f4f4f5; }
        QTextEdit, QTextBrowser { background-color: #1e1e1e; color: #e4e4e7; border: none; padding: 30px 40px; font-size: 15px; line-height: 1.6; }
        QTextBrowser { border-left: 1px solid #2d2d2d; }
        QLineEdit { background-color: #000000; color: #f4f4f5; border: 1px solid #3f3f46; padding: 8px 12px; border-radius: 8px; font-size: 13px; margin: 12px 12px 8px 12px; }
        QLineEdit:focus { border: 1px solid #0a84ff; background-color: #18181b; }
        QTabWidget::pane { border: none; background-color: #1e1e1e; border-top: 1px solid #2d2d2d; }
        QTabBar::tab { background-color: transparent; color: #a1a1aa; padding: 10px 20px; font-size: 13px; font-weight: 600; border-bottom: 2px solid transparent; }
        QTabBar::tab:selected { color: #ffffff; border-bottom: 2px solid #0a84ff; }
        QTabBar::tab:hover:!selected { color: #d4d4d8; background-color: #27272a; }
        QPushButton#btnFocus { background-color: transparent; color: #0a84ff; border: none; padding: 6px 12px; margin: 4px; font-size: 13px; }
        QPushButton#btnFocus:hover { background-color: #27272a; color: #60a5fa; border-radius: 6px; }
        QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: #52525b; border-radius: 4px; }
        QScrollBar:horizontal { border: none; background: transparent; height: 8px; margin: 0px; }
        QScrollBar::handle:horizontal { background: #52525b; border-radius: 4px; }
    )";

    QString themeLight = R"(
        * { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
        QMainWindow, QGraphicsView { background-color: #ffffff; border: none; }
        QMenuBar { background-color: #f3f4f6; color: #111827; border-bottom: 1px solid #e5e7eb; padding: 2px; }
        QMenuBar::item:selected { background-color: #e5e7eb; color: #000000; }
        QMenu { background-color: #ffffff; color: #111827; border: 1px solid #d1d5db; border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 6px 30px 6px 12px; border-radius: 4px; }
        QMenu::item:selected { background-color: #0ea5e9; color: #ffffff; }
        QSplitter::handle { background-color: #e5e7eb; }
        QWidget#leftContainer { background-color: #f9fafb; } 
        QTreeWidget, QListWidget { background-color: transparent; color: #374151; border: none; font-size: 13px; outline: none; }
        QTreeWidget::item, QListWidget::item { padding: 6px 8px; border-radius: 6px; margin: 2px 8px; }
        QTreeWidget::item:selected, QListWidget::item:selected { background-color: #e5e7eb; color: #111827; font-weight: 600; }
        QTreeWidget::item:hover:!selected, QListWidget::item:hover:!selected { background-color: #f3f4f6; color: #111827; }
        QTextEdit, QTextBrowser { background-color: #ffffff; color: #111827; border: none; padding: 30px 40px; font-size: 15px; line-height: 1.6; }
        QTextBrowser { border-left: 1px solid #e5e7eb; }
        QLineEdit { background-color: #ffffff; color: #111827; border: 1px solid #d1d5db; padding: 8px 12px; border-radius: 8px; font-size: 13px; margin: 12px 12px 8px 12px; }
        QLineEdit:focus { border: 1px solid #0ea5e9; background-color: #ffffff; }
        QTabWidget::pane { border: none; background-color: #ffffff; border-top: 1px solid #e5e7eb; }
        QTabBar::tab { background-color: transparent; color: #6b7280; padding: 10px 20px; font-size: 13px; font-weight: 600; border-bottom: 2px solid transparent; }
        QTabBar::tab:selected { color: #111827; border-bottom: 2px solid #0ea5e9; }
        QTabBar::tab:hover:!selected { color: #374151; background-color: #f3f4f6; }
        QPushButton#btnFocus { background-color: transparent; color: #0ea5e9; border: none; padding: 6px 12px; margin: 4px; font-size: 13px; }
        QPushButton#btnFocus:hover { background-color: #f3f4f6; color: #0284c7; border-radius: 6px; }
        QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: #d1d5db; border-radius: 4px; }
        QScrollBar:horizontal { border: none; background: transparent; height: 8px; margin: 0px; }
        QScrollBar::handle:horizontal { background: #d1d5db; border-radius: 4px; }
    )";

    QString themeSolarized = R"(
        * { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
        QMainWindow, QGraphicsView { background-color: #fdf6e3; border: none; }
        QMenuBar { background-color: #eee8d5; color: #586e75; border-bottom: 1px solid #e0d8c0; padding: 2px; }
        QMenuBar::item:selected { background-color: #e0d8c0; color: #073642; }
        QMenu { background-color: #fdf6e3; color: #586e75; border: 1px solid #d3cbb5; border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 6px 30px 6px 12px; border-radius: 4px; }
        QMenu::item:selected { background-color: #268bd2; color: #fdf6e3; }
        QSplitter::handle { background-color: #e0d8c0; }
        QWidget#leftContainer { background-color: #eee8d5; } 
        QTreeWidget, QListWidget { background-color: transparent; color: #657b83; border: none; font-size: 13px; outline: none; }
        QTreeWidget::item, QListWidget::item { padding: 6px 8px; border-radius: 6px; margin: 2px 8px; }
        QTreeWidget::item:selected, QListWidget::item:selected { background-color: #e0d8c0; color: #073642; font-weight: 600; }
        QTreeWidget::item:hover:!selected, QListWidget::item:hover:!selected { background-color: #e6dfca; color: #586e75; }
        QTextEdit, QTextBrowser { background-color: #fdf6e3; color: #657b83; border: none; padding: 30px 40px; font-size: 15px; line-height: 1.6; }
        QTextBrowser { border-left: 1px solid #e0d8c0; }
        QLineEdit { background-color: #fdf6e3; color: #586e75; border: 1px solid #d3cbb5; padding: 8px 12px; border-radius: 8px; font-size: 13px; margin: 12px 12px 8px 12px; }
        QLineEdit:focus { border: 1px solid #268bd2; background-color: #fdf6e3; }
        QTabWidget::pane { border: none; background-color: #fdf6e3; border-top: 1px solid #e0d8c0; }
        QTabBar::tab { background-color: transparent; color: #93a1a1; padding: 10px 20px; font-size: 13px; font-weight: 600; border-bottom: 2px solid transparent; }
        QTabBar::tab:selected { color: #073642; border-bottom: 2px solid #268bd2; }
        QTabBar::tab:hover:!selected { color: #586e75; background-color: #eee8d5; }
        QPushButton#btnFocus { background-color: transparent; color: #268bd2; border: none; padding: 6px 12px; margin: 4px; font-size: 13px; }
        QPushButton#btnFocus:hover { background-color: #eee8d5; color: #2aa198; border-radius: 6px; }
        QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: #d3cbb5; border-radius: 4px; }
        QScrollBar:horizontal { border: none; background: transparent; height: 8px; margin: 0px; }
        QScrollBar::handle:horizontal { background: #d3cbb5; border-radius: 4px; }
    )";

    app.setStyleSheet(themeDark); 

    QMainWindow window;
    window.setWindowTitle("MemCore - Pro Edition");
    window.resize(1300, 800); 

    QMenuBar *menuBar = new QMenuBar(&window);
    menuBar->setNativeMenuBar(false); 
    window.setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu("&File");
    QAction *actionNew = fileMenu->addAction("New Note\tCtrl+N");
    QAction *actionRename = fileMenu->addAction("Rename Note\tF2");
    QAction *actionDelete = fileMenu->addAction("Delete Note");
    fileMenu->addSeparator();
    QAction *actionOpenVault = fileMenu->addAction("Open Vault Folder...");
    fileMenu->addSeparator();
    QAction *actionExportPDF = fileMenu->addAction("Export as PDF...");
    fileMenu->addSeparator();
    QAction *actionExit = fileMenu->addAction("Exit\tCtrl+Q");

    QMenu *editMenu = menuBar->addMenu("&Edit");
    QAction *actionInsertDate = editMenu->addAction("Insert Date & Time");
    QAction *actionClearEditor = editMenu->addAction("Clear Editor");

    QMenu *viewMenu = menuBar->addMenu("&View");
    QAction *actionFocus = viewMenu->addAction("Toggle Sidebar (Focus Mode)");
    QAction *actionRefresh = viewMenu->addAction("Refresh Vault\tF5");
    viewMenu->addSeparator();
    
    QMenu *themeMenu = viewMenu->addMenu("Themes");
    QAction *actionThemeDark = themeMenu->addAction("Dark Mode");
    QAction *actionThemeLight = themeMenu->addAction("Light Mode");
    QAction *actionThemeSolarized = themeMenu->addAction("Solarized Light");

    viewMenu->addSeparator();
    QAction *actionTogglePhysics = viewMenu->addAction("Toggle Graph Physics");

    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *actionCheatsheet = helpMenu->addAction("Markdown Cheatsheet");
    QAction *actionAbout = helpMenu->addAction("About MemCore");

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, &window);
    mainSplitter->setHandleWidth(1); 

    QWidget *leftContainer = new QWidget(mainSplitter);
    leftContainer->setObjectName("leftContainer");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLineEdit *searchBox = new QLineEdit(leftContainer);
    searchBox->setPlaceholderText("Search or #tag...");
    leftLayout->addWidget(searchBox);

    QSplitter *leftSplitter = new QSplitter(Qt::Vertical, leftContainer);
    leftSplitter->setHandleWidth(1);
    
    QTreeWidget *treeWidget = new QTreeWidget(leftSplitter);
    treeWidget->setHeaderHidden(true); 

    QWidget *backlinksContainer = new QWidget(leftSplitter);
    QVBoxLayout *backlinksLayout = new QVBoxLayout(backlinksContainer);
    backlinksLayout->setContentsMargins(12, 12, 12, 12);
    
    QHBoxLayout *backlinksHeaderLayout = new QHBoxLayout();
    QLabel *backlinksLabel = new QLabel("BACKLINKS", backlinksContainer);
    backlinksLabel->setStyleSheet("color: #71717a; font-weight: 700; padding: 4px; font-size: 11px; letter-spacing: 1px;");
    QPushButton *btnRefreshSmall = new QPushButton("⟳", backlinksContainer);
    btnRefreshSmall->setStyleSheet("border: none; background: transparent; color: #a1a1aa; font-size: 16px; padding: 0px; margin: 0px;");
    btnRefreshSmall->setCursor(Qt::PointingHandCursor);
    backlinksHeaderLayout->addWidget(backlinksLabel);
    backlinksHeaderLayout->addStretch();
    backlinksHeaderLayout->addWidget(btnRefreshSmall);
    
    QListWidget *backlinksList = new QListWidget(backlinksContainer);
    backlinksLayout->addLayout(backlinksHeaderLayout);
    backlinksLayout->addWidget(backlinksList);

    leftSplitter->setSizes(QList<int>() << 600 << 200);
    leftLayout->addWidget(leftSplitter);

    QTabWidget *rightTabs = new QTabWidget(mainSplitter);
    
    QPushButton *btnFocus = new QPushButton("Toggle Sidebar", rightTabs);
    btnFocus->setObjectName("btnFocus");
    rightTabs->setCornerWidget(btnFocus, Qt::TopRightCorner);

    QSplitter *textSplitter = new QSplitter(Qt::Horizontal, rightTabs);
    textSplitter->setHandleWidth(1);
    
    QTextEdit *textEdit = new QTextEdit(textSplitter);
    textEdit->setPlaceholderText("Start typing your note here...");
    
    MarkdownHighlighter *highlighter = new MarkdownHighlighter(textEdit->document());
    
    QTextBrowser *markdownPreview = new QTextBrowser(textSplitter);
    markdownPreview->setOpenExternalLinks(true); 
    markdownPreview->setSearchPaths(QStringList() << QString::fromStdString(vaultPath));
    
    textSplitter->setSizes(QList<int>() << 500 << 500); 
    rightTabs->addTab(textSplitter, "Document");

    QGraphicsScene *graphScene = new QGraphicsScene();
    graphScene->setSceneRect(-2000, -2000, 4000, 4000); 
    QGraphicsView *graphView = new QGraphicsView(graphScene);
    graphView->setRenderHint(QPainter::Antialiasing);
    graphView->setDragMode(QGraphicsView::ScrollHandDrag); 
    rightTabs->addTab(graphView, "Graph Map");

    mainSplitter->setSizes(QList<int>() << 280 << 1020); 
    window.setCentralWidget(mainSplitter);

    auto currentFile = std::make_shared<std::string>("");
    auto isProgrammaticChange = std::make_shared<bool>(false);
    auto activeNodes = std::make_shared<std::vector<Node*>>();
    auto isPhysicsEnabled = std::make_shared<bool>(true);
    
    auto globalGraph = std::make_shared<std::unordered_map<std::string, std::vector<std::string>>>();
    auto globalTags = std::make_shared<std::unordered_map<std::string, std::vector<std::string>>>(); 

    std::shared_ptr<QTimer> physicsTimer = std::make_shared<QTimer>();
    QObject::connect(physicsTimer.get(), &QTimer::timeout, [activeNodes, isPhysicsEnabled]() {
        if (!(*isPhysicsEnabled)) return; 

        bool needsUpdate = false;
        for (Node* n1 : *activeNodes) {
            if (n1->scene() && n1->scene()->mouseGrabberItem() == n1) {
                n1->velocity = QPointF(0, 0);
                needsUpdate = true;
                continue;
            }
            QPointF force(0, 0);
            for (Node* n2 : *activeNodes) {
                if (n1 == n2) continue;
                QPointF d = n1->pos() - n2->pos();
                double dist = std::max(1.0, std::sqrt(d.x()*d.x() + d.y()*d.y()));
                double repulse = 5000.0 / (dist * dist);
                force += (d / dist) * repulse;
            }
            for (Edge* e : n1->edges) {
                Node* n2 = (e->source == n1) ? e->dest : e->source;
                QPointF d = n2->pos() - n1->pos();
                double dist = std::max(1.0, std::sqrt(d.x()*d.x() + d.y()*d.y()));
                double attract = (dist - 180.0) * 0.04; 
                force += (d / dist) * attract;
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
        // --- PREZERVARE STARE CURENTĂ ---
        QString selectedFileName = "";
        if (treeWidget->currentItem() && treeWidget->currentItem()->parent() == nullptr) {
            selectedFileName = treeWidget->currentItem()->text(0);
        }

        activeNodes->clear();
        treeWidget->clear();
        graphScene->clear();
        globalGraph->clear();
        globalTags->clear();
        std::unordered_map<std::string, Node*> nodesMap;

        if (fs::exists(vaultPath) && fs::is_directory(vaultPath)) {
            for (const auto& entry : fs::directory_iterator(vaultPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".md") {
                    std::string filename = entry.path().filename().string();
                    (*globalGraph)[filename] = std::vector<std::string>();
                    (*globalTags)[filename] = std::vector<std::string>();
                    
                    std::ifstream file(entry.path());
                    if (file.is_open()) {
                        std::string line;
                        while (std::getline(file, line)) {
                            std::smatch match;
                            std::string searchAreaLinks = line;
                            while (std::regex_search(searchAreaLinks, match, linkPattern)) {
                                std::string linkName = match[1].str();
                                if (linkName.length() >= 3 && linkName.substr(linkName.length() - 3) == ".md") linkName = linkName.substr(0, linkName.length() - 3);
                                (*globalGraph)[filename].push_back(linkName);
                                searchAreaLinks = match.suffix().str();
                            }
                            std::string searchAreaTags = line;
                            while (std::regex_search(searchAreaTags, match, tagPattern)) {
                                std::string tag = "#" + match[1].str();
                                if (std::find((*globalTags)[filename].begin(), (*globalTags)[filename].end(), tag) == (*globalTags)[filename].end()) {
                                    (*globalTags)[filename].push_back(tag);
                                }
                                searchAreaTags = match.suffix().str();
                            }
                        }
                    }

                    QTreeWidgetItem *noteItem = new QTreeWidgetItem(treeWidget);
                    noteItem->setText(0, QString::fromStdString(filename));
                    
                    for (const std::string& link : (*globalGraph)[filename]) {
                        QTreeWidgetItem *childItem = new QTreeWidgetItem(noteItem);
                        childItem->setText(0, QString::fromStdString("↗ " + link));
                        childItem->setForeground(0, QBrush(QColor("#a1a1aa"))); 
                    }
                    for (const std::string& tag : (*globalTags)[filename]) {
                        QTreeWidgetItem *childItem = new QTreeWidgetItem(noteItem);
                        childItem->setText(0, QString::fromStdString("🏷️ " + tag));
                        childItem->setForeground(0, QBrush(QColor("#0ea5e9"))); 
                    }
                }
            }
        }
        treeWidget->expandAll();

        // --- RESTAURARE STARE ---
        if (!selectedFileName.isEmpty()) {
            QList<QTreeWidgetItem*> items = treeWidget->findItems(selectedFileName, Qt::MatchExactly, 0);
            if (!items.isEmpty()) {
                treeWidget->setCurrentItem(items.first());
                
                // Actualizare tăcută a backlink-urilor
                backlinksList->clear();
                std::string targetLink = selectedFileName.toStdString();
                if (targetLink.length() >= 3 && targetLink.substr(targetLink.length() - 3) == ".md") {
                    targetLink = targetLink.substr(0, targetLink.length() - 3);
                }
                for (const auto& pair : *globalGraph) {
                    for (const std::string& link : pair.second) {
                        if (link == targetLink) {
                            backlinksList->addItem(QString::fromStdString(pair.first));
                            break;
                        }
                    }
                }
            }
        }

        for (const auto& pair : *globalGraph) {
            if (nodesMap.find(pair.first) == nodesMap.end()) {
                Node* n = new Node(QString::fromStdString(pair.first), FILE_NODE);
                nodesMap[pair.first] = n;
                graphScene->addItem(n);
                activeNodes->push_back(n);
                n->setPos(rand() % 500 - 250, rand() % 500 - 250); 
            }
            for(const auto& link : pair.second) {
                std::string linkFile = link + ".md";
                if (nodesMap.find(linkFile) == nodesMap.end()) {
                     Node* n = new Node(QString::fromStdString(linkFile), FILE_NODE);
                     nodesMap[linkFile] = n;
                     graphScene->addItem(n);
                     activeNodes->push_back(n);
                     n->setPos(rand() % 500 - 250, rand() % 500 - 250);
                }
            }
        }
        for (const auto& pair : *globalTags) {
            for (const std::string& tag : pair.second) {
                if (nodesMap.find(tag) == nodesMap.end()) {
                    Node* n = new Node(QString::fromStdString(tag), TAG_NODE);
                    nodesMap[tag] = n;
                    graphScene->addItem(n);
                    activeNodes->push_back(n);
                    n->setPos(rand() % 500 - 250, rand() % 500 - 250); 
                }
            }
        }

        for (const auto& pair : *globalGraph) {
            Node* sourceNode = nodesMap[pair.first];
            for (const std::string& link : pair.second) {
                Node* destNode = nodesMap[link + ".md"];
                Edge* edge = new Edge(sourceNode, destNode);
                graphScene->addItem(edge);
                sourceNode->edges.push_back(edge);
                destNode->edges.push_back(edge);
            }
        }
        for (const auto& pair : *globalTags) {
            Node* sourceNode = nodesMap[pair.first];
            for (const std::string& tag : pair.second) {
                Node* destNode = nodesMap[tag];
                Edge* edge = new Edge(sourceNode, destNode);
                graphScene->addItem(edge);
                sourceNode->edges.push_back(edge);
                destNode->edges.push_back(edge);
            }
        }
        physicsTimer->start(16); 
    };

    reloadSystem();

    auto updateGraphTextColor = [activeNodes](const QString& hexColor) {
        for (Node* n : *activeNodes) n->setTextColor(QColor(hexColor));
    };

    QObject::connect(actionThemeDark, &QAction::triggered, [&]() {
        app.setStyleSheet(themeDark);
        updateGraphTextColor("#f4f4f5"); 
    });

    QObject::connect(actionThemeLight, &QAction::triggered, [&]() {
        app.setStyleSheet(themeLight);
        updateGraphTextColor("#111827"); 
    });

    QObject::connect(actionThemeSolarized, &QAction::triggered, [&]() {
        app.setStyleSheet(themeSolarized);
        updateGraphTextColor("#657b83"); 
    });

    QObject::connect(actionNew, &QAction::triggered, [&]() {
        bool ok;
        QString text = QInputDialog::getText(nullptr, "New Note", "Note name (without .md):", QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            std::ofstream file(vaultPath + "/" + text.toStdString() + ".md");
            reloadSystem();
        }
    });

    QObject::connect(actionRename, &QAction::triggered, [&]() {
        QTreeWidgetItem *item = treeWidget->currentItem();
        if (!item || item->parent() != nullptr) {
            QMessageBox::warning(nullptr, "Select Note", "Please select a main file from the left sidebar to rename.");
            return;
        }
        std::string oldName = item->text(0).toStdString();
        std::string nameWithoutExt = oldName.length() >= 3 ? oldName.substr(0, oldName.length() - 3) : oldName;
        
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Rename Note", "New note name (without .md):", QLineEdit::Normal, QString::fromStdString(nameWithoutExt), &ok);
        if (ok && !newName.isEmpty()) {
            std::string oldPath = vaultPath + "/" + oldName;
            std::string newPath = vaultPath + "/" + newName.toStdString() + ".md";
            fs::rename(oldPath, newPath);
            if (*currentFile == oldPath) *currentFile = newPath;
            reloadSystem();
        }
    });

    QObject::connect(actionDelete, &QAction::triggered, [&]() {
        QTreeWidgetItem *item = treeWidget->currentItem();
        if (!item || item->parent() != nullptr) {
            QMessageBox::warning(nullptr, "Select Note", "Please select a main file from the left sidebar to delete.");
            return;
        }
        auto reply = QMessageBox::question(nullptr, "Delete Note", "Are you sure you want to permanently delete '" + item->text(0) + "'?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            fs::remove(vaultPath + "/" + item->text(0).toStdString());
            *currentFile = "";
            textEdit->clear();
            markdownPreview->clear();
            backlinksList->clear();
            reloadSystem();
        }
    });

    QObject::connect(actionOpenVault, &QAction::triggered, [&]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QDir(QString::fromStdString(vaultPath)).absolutePath()));
    });

    QObject::connect(actionExportPDF, &QAction::triggered, [&]() {
        if (textEdit->toPlainText().isEmpty()) {
            QMessageBox::warning(nullptr, "Notice", "There is nothing to export! Open a note first.");
            return;
        }
        QString fileName = QFileDialog::getSaveFileName(nullptr, "Export PDF", "", "PDF Files (*.pdf)");
        if (!fileName.isEmpty()) {
            QPdfWriter pdfWriter(fileName);
            pdfWriter.setResolution(300);
            markdownPreview->document()->print(&pdfWriter);
            QMessageBox::information(nullptr, "Success", "Note exported successfully as PDF!");
        }
    });

    QObject::connect(actionInsertDate, &QAction::triggered, [&]() {
        QString elegantDateTime = QDateTime::currentDateTime().toString("MMMM d, yyyy • hh:mm AP");
        textEdit->insertPlainText(elegantDateTime + "\n");
    });

    QObject::connect(actionClearEditor, &QAction::triggered, [&]() {
        textEdit->clear();
    });

    QObject::connect(actionTogglePhysics, &QAction::triggered, [&]() {
        *isPhysicsEnabled = !(*isPhysicsEnabled);
    });

    QObject::connect(actionCheatsheet, &QAction::triggered, [&]() {
        QMessageBox::information(nullptr, "Markdown Cheatsheet", 
            "<b>Basic Markdown Syntax:</b><br><br>"
            "<b># Heading 1</b><br>"
            "<b>## Heading 2</b><br>"
            "**Bold Text**<br>"
            "*Italic Text*<br>"
            "[[Link to another note]]<br>"
            "#tag_name<br>"
            "![Image Alt](image.jpg)<br>"
            "- Bullet List item<br>");
    });

    QObject::connect(actionAbout, &QAction::triggered, [&]() {
        QMessageBox::about(nullptr, "About MemCore", 
            "<div style='text-align: center; font-family: -apple-system, sans-serif;'>"
            "<h2 style='margin-bottom: 5px; font-weight: bold;'>MemCore</h2>"
            "<p style='color: gray; margin-top: 0px; font-size: 13px;'>Version 1.0 (Pro Edition)</p>"
            "<p style='margin: 15px 0px; line-height: 1.4;'>"
            "A meticulously crafted Second Brain.<br>"
            "Focus on your thoughts, we handle the connections."
            "</p>"
            "<p style='font-size: 11px; color: #888888; margin-top: 20px;'>"
            "© 2026 MemCore Technologies. All rights reserved."
            "</p>"
            "</div>"
        );
    });

    QObject::connect(actionExit, &QAction::triggered, &app, &QApplication::quit);

    bool isFocusMode = false;
    auto toggleFocusMode = [&]() {
        isFocusMode = !isFocusMode;
        leftContainer->setVisible(!isFocusMode); 
        btnFocus->setText(isFocusMode ? "Show Sidebar" : "Toggle Sidebar");
    };
    QObject::connect(btnFocus, &QPushButton::clicked, toggleFocusMode);
    QObject::connect(actionFocus, &QAction::triggered, toggleFocusMode);

    QObject::connect(searchBox, &QLineEdit::textChanged, [treeWidget](const QString &text) {
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = treeWidget->topLevelItem(i);
            bool match = item->text(0).contains(text, Qt::CaseInsensitive);
            if (!match) {
                for (int j = 0; j < item->childCount(); ++j) {
                    if (item->child(j)->text(0).contains(text, Qt::CaseInsensitive)) {
                        match = true; break;
                    }
                }
            }
            item->setHidden(!match);
        }
    });

    QObject::connect(actionRefresh, &QAction::triggered, reloadSystem);
    QObject::connect(btnRefreshSmall, &QPushButton::clicked, reloadSystem);

    // --- NOU: MECANISME DE AUTO-UPDATE PENTRU GRAF ---

    // 1. Refresh automat la schimbarea pe tabul de Graf
    QObject::connect(rightTabs, &QTabWidget::currentChanged, [&](int index) {
        if (index == 1) { // Index 1 este "Graph Map"
            reloadSystem();
        }
    });

    // 2. Debounce Timer (actualizează automat la 1.5 secunde după ce ai terminat de scris)
    std::shared_ptr<QTimer> autoRefreshTimer = std::make_shared<QTimer>();
    autoRefreshTimer->setSingleShot(true);
    autoRefreshTimer->setInterval(1500); 
    QObject::connect(autoRefreshTimer.get(), &QTimer::timeout, reloadSystem);

    QObject::connect(textEdit, &QTextEdit::textChanged, [&]() {
        if (!*isProgrammaticChange && !currentFile->empty()) {
            std::ofstream file(*currentFile);
            if (file.is_open()) file << textEdit->toPlainText().toStdString();
            markdownPreview->setMarkdown(textEdit->toPlainText());
            
            // Repornim cronometrul la fiecare literă tastată
            autoRefreshTimer->start(); 
        }
    });

    QObject::connect(treeWidget, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int column) {
        std::string itemName = item->text(0).toStdString();
        
        if (item->parent() != nullptr) {
            QString childText = item->text(0);
            if (childText.startsWith("🏷️ ")) {
                searchBox->setText(childText.replace("🏷️ ", ""));
            }
            return; 
        }

        backlinksList->clear();
        std::string targetLink = itemName;
        if (targetLink.length() >= 3 && targetLink.substr(targetLink.length() - 3) == ".md") {
            targetLink = targetLink.substr(0, targetLink.length() - 3);
        }
        
        for (const auto& pair : *globalGraph) {
            for (const std::string& link : pair.second) {
                if (link == targetLink) {
                    backlinksList->addItem(QString::fromStdString(pair.first));
                    break;
                }
            }
        }

        std::string fullPath = vaultPath + "/" + itemName;
        std::ifstream file(fullPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            *isProgrammaticChange = true;
            QString text = QString::fromStdString(buffer.str());
            
            textEdit->setPlainText(text);
            markdownPreview->setMarkdown(text); 
            
            *currentFile = fullPath;
            *isProgrammaticChange = false;
            rightTabs->setCurrentIndex(0);
        }
    });

    QObject::connect(backlinksList, &QListWidget::itemClicked, [&](QListWidgetItem *item) {
        QList<QTreeWidgetItem*> foundItems = treeWidget->findItems(item->text(), Qt::MatchExactly, 0);
        if (!foundItems.isEmpty()) {
            treeWidget->setCurrentItem(foundItems.first());
            emit treeWidget->itemClicked(foundItems.first(), 0);
        }
    });

    window.show();
    return app.exec();
}