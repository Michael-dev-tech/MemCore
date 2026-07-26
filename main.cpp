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
#include <QVariantAnimation>
#include <QPainter>
#include <QDialog>
#include <QShortcut>
#include <QSettings>
#include <QStackedWidget>
#include <QComboBox>
#include <QScrollArea>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPointer>
#include <QListView>
#include <QMenu>
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
#include <hunspell/hunspell.hxx>

namespace fs = std::filesystem;

// --- CRIPTARE ---
QByteArray encryptAES(const QByteArray &plaintext, const unsigned char* key) {
    unsigned char iv[16]; RAND_bytes(iv, sizeof(iv)); 
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    
    QByteArray ciphertext; 
    ciphertext.resize(plaintext.length() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int outlen1 = 0, outlen2 = 0;
    
    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &outlen1, reinterpret_cast<const unsigned char*>(plaintext.constData()), plaintext.length());
    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + outlen1, &outlen2);
    ciphertext.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    
    QByteArray result; 
    result.reserve(8 + 16 + ciphertext.length()); 
    result.append("ORBENC__", 8); 
    result.append(reinterpret_cast<const char*>(iv), 16); 
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
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    
    QByteArray plaintext; 
    plaintext.resize(ciphertext.length());
    int outlen1 = 0, outlen2 = 0;
    
    if (1 != EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &outlen1, reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.length())) {
        EVP_CIPHER_CTX_free(ctx); return {};
    }
    if (1 != EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + outlen1, &outlen2)) {
        EVP_CIPHER_CTX_free(ctx); return {}; 
    }
    plaintext.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

// --- CLASA PENTRU DRAG WINDOW ---
class WindowDragger : public QObject {
    QWidget *target;
    QPoint dragPos;
public:
    explicit WindowDragger(QWidget *target) : QObject(target), target(target) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                dragPos = me->globalPosition().toPoint() - target->frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->buttons() & Qt::LeftButton) {
                target->move(me->globalPosition().toPoint() - dragPos);
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

// --- CUSTOM TOGGLE SWITCH ---
class ToggleSwitch : public QAbstractButton {
    double _thumbPos = 0.0;
    QVariantAnimation *anim;
public:
    QColor bgOn = QColor("#8b5cf6");
    QColor bgOff = QColor("#d4d4d8");
    QColor thumbColor = QColor("#ffffff");

    explicit ToggleSwitch(QWidget* parent = nullptr) : QAbstractButton(parent) {
        setFixedSize(44, 24);
        setCursor(Qt::PointingHandCursor);
        setCheckable(true);
        
        anim = new QVariantAnimation(this);
        anim->setDuration(150);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        
        connect(anim, &QVariantAnimation::valueChanged, [this](const QVariant& val) {
            _thumbPos = val.toDouble();
            update();
        });
        
        connect(this, &QAbstractButton::toggled, [this](bool checked) {
            anim->stop();
            anim->setStartValue(_thumbPos);
            anim->setEndValue(checked ? 1.0 : 0.0);
            anim->start();
        });
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        
        int r = bgOff.red() + _thumbPos * (bgOn.red() - bgOff.red());
        int g = bgOff.green() + _thumbPos * (bgOn.green() - bgOff.green());
        int b = bgOff.blue() + _thumbPos * (bgOn.blue() - bgOff.blue());
        
        p.setBrush(QColor(r, g, b));
        p.drawRoundedRect(0, 0, width(), height(), height() / 2.0, height() / 2.0);
        
        p.setBrush(thumbColor);
        int margin = 3;
        int thumbRadius = height() - margin * 2;
        double xPos = margin + _thumbPos * (width() - thumbRadius - margin * 2);
        
        p.setPen(QPen(QColor(0, 0, 0, 40), 1));
        p.drawEllipse(QRectF(xPos, margin + 0.5, thumbRadius, thumbRadius));
        
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(xPos, margin, thumbRadius, thumbRadius));
    }
};

// --- MENIUL SETTINGS ---
class SettingsDialog : public QDialog {
    QListWidget *categoryList;
    QStackedWidget *stack;
    std::vector<ToggleSwitch*> toggleSwitches;
public:
    SettingsDialog(QWidget *parent, const std::function<void(QString)>& onThemeChange, const std::function<void(bool)>& onSpellcheckChange) : QDialog(parent) {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        resize(800, 600);
        
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0,0,0,0);
        mainLayout->setSpacing(0);
        
        auto *header = new QWidget();
        header->setObjectName("dialogHeader");
        header->setFixedHeight(45);
        auto *hLayout = new QHBoxLayout(header);
        hLayout->setContentsMargins(20, 0, 10, 0);
        
        auto *titleLabel = new QLabel("Settings");
        titleLabel->setObjectName("dialogTitle");
        
        auto *closeBtn = new QPushButton("✕");
        closeBtn->setObjectName("closeBtn");
        closeBtn->setFixedSize(30, 30);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        
        hLayout->addWidget(titleLabel); hLayout->addStretch(); hLayout->addWidget(closeBtn);
        header->installEventFilter(new WindowDragger(this));
        mainLayout->addWidget(header);
        
        auto *body = new QWidget();
        auto *bodyLayout = new QHBoxLayout(body);
        bodyLayout->setContentsMargins(0,0,0,0);
        bodyLayout->setSpacing(0);
        
        categoryList = new QListWidget();
        categoryList->setFixedWidth(220);
        categoryList->setObjectName("settingsSidebar");
        
        stack = new QStackedWidget();
        stack->setObjectName("settingsStack");
        
        bodyLayout->addWidget(categoryList);
        bodyLayout->addWidget(stack);
        mainLayout->addWidget(body);
        
        auto createRow = [](const QString& t, const QString& d, QWidget* ctrl) {
            auto *w = new QWidget(); w->setObjectName("settingRow");
            auto *l = new QHBoxLayout(w); l->setContentsMargins(0, 20, 0, 20);
            auto *tl = new QVBoxLayout();
            auto *lt = new QLabel(t); lt->setObjectName("rowTitle");
            auto *ld = new QLabel(d); ld->setObjectName("rowDesc"); ld->setWordWrap(true);
            tl->addWidget(lt); tl->addWidget(ld);
            l->addLayout(tl); l->addStretch(); l->addWidget(ctrl);
            return w;
        };

        auto addPage = [&](const QString& name, QWidget* content) {
            categoryList->addItem(name);
            auto *sa = new QScrollArea(); sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame); sa->setObjectName("settingsScroll");
            
            auto *container = new QWidget(); container->setObjectName("settingsPageContainer");
            auto *v = new QVBoxLayout(container); v->setContentsMargins(50, 40, 50, 40); v->setAlignment(Qt::AlignTop);
            
            auto *h1 = new QLabel(name); h1->setObjectName("pageTitle");
            v->addWidget(h1); v->addWidget(content); v->addStretch();
            
            sa->setWidget(container); stack->addWidget(sa);
        };
        
        auto *pgGeneral = new QWidget(); auto *lGen = new QVBoxLayout(pgGeneral); lGen->setContentsMargins(0,0,0,0);
        auto *cb1 = new ToggleSwitch(); cb1->setChecked(true); toggleSwitches.push_back(cb1);
        lGen->addWidget(createRow("Automatic Updates", "Keep the application up to date silently.", cb1));

        auto *pgEditor = new QWidget(); auto *lEd = new QVBoxLayout(pgEditor); lEd->setContentsMargins(0,0,0,0);
        
        QSettings s("Orbit", "EnterpriseEdition");
        auto *cbSpell = new ToggleSwitch(); 
        cbSpell->setChecked(s.value("spellcheck", true).toBool());
        connect(cbSpell, &QAbstractButton::toggled, [onSpellcheckChange](bool checked){
            QSettings("Orbit", "EnterpriseEdition").setValue("spellcheck", checked);
            onSpellcheckChange(checked);
        });
        toggleSwitches.push_back(cbSpell);
        lEd->addWidget(createRow("Spellcheck", "Highlight typos and misspelled words.", cbSpell));
        
        auto *cbLine = new ToggleSwitch(); cbLine->setChecked(true); toggleSwitches.push_back(cbLine);
        lEd->addWidget(createRow("Readable line length", "Limit the width of the text to make it easier to read.", cbLine));

        auto *pgFiles = new QWidget(); auto *lFiles = new QVBoxLayout(pgFiles); lFiles->setContentsMargins(0,0,0,0);
        auto *comboOpen = new QComboBox(); 
        comboOpen->setView(new QListView()); 
        comboOpen->addItems({"Last opened", "Welcome note"});
        lFiles->addWidget(createRow("Default file to open", "Choose which file to open when the app starts.", comboOpen));
        auto *chkWiki = new ToggleSwitch(); chkWiki->setChecked(true); toggleSwitches.push_back(chkWiki);
        lFiles->addWidget(createRow("Use [[Wikilinks]]", "Auto-generate Wikilinks instead of standard Markdown.", chkWiki));

        auto *pgApp = new QWidget(); auto *lApp = new QVBoxLayout(pgApp); lApp->setContentsMargins(0,0,0,0);
        auto *comboTheme = new QComboBox(); 
        comboTheme->setView(new QListView()); 
        comboTheme->addItems({"Dark", "Light", "Solarized Light"});
        
        QString currentTheme = s.value("theme", "Light").toString(); 
        comboTheme->setCurrentText(currentTheme);
        
        connect(comboTheme, &QComboBox::currentTextChanged, [this, onThemeChange](const QString &t){ applyStyles(t); onThemeChange(t); });
        lApp->addWidget(createRow("Base theme", "Choose Dark, Light, or the warm Solarized Light mode.", comboTheme));

        auto *lblHotkeys = new QLabel("Manage your keyboard shortcuts."); lblHotkeys->setObjectName("rowDesc");
        
        addPage("General", pgGeneral);
        addPage("Editor", pgEditor);
        addPage("Files and links", pgFiles);
        addPage("Appearance", pgApp);
        addPage("Hotkeys", lblHotkeys);
        
        connect(categoryList, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);
        categoryList->setCurrentRow(1); 
        
        applyStyles(currentTheme);
    }

    void applyStyles(const QString& theme) {
        QString bgMain, bgSide, textCol, textMuted, borderCol, accent, toggleBg;
        if (theme == "Solarized Light") {
            bgMain = "#fdf6e3"; bgSide = "#eee8d5"; textCol = "#073642"; textMuted = "#586e75"; borderCol = "#e0d8c3"; accent = "#b58900"; toggleBg = "#d5c4a1";
        } else if (theme == "Light") {
            bgMain = "#ffffff"; bgSide = "#f4f4f5"; textCol = "#18181b"; textMuted = "#71717a"; borderCol = "#e4e4e7"; accent = "#8b5cf6"; toggleBg = "#d4d4d8";
        } else { // Dark
            bgMain = "#18181b"; bgSide = "#09090b"; textCol = "#e4e4e7"; textMuted = "#a1a1aa"; borderCol = "#27272a"; accent = "#8b5cf6"; toggleBg = "#3f3f46";
        }

        this->setStyleSheet(QString(
            "QDialog { background-color: %1; border: 1px solid %5; border-radius: 12px; font-family: \"Inter\", -apple-system, BlinkMacSystemFont, \"SF Pro Display\", \"Segoe UI\", \"Roboto\", \"Helvetica Neue\", sans-serif; }"
            "QWidget#dialogHeader { background-color: %2; border-bottom: 1px solid %5; border-top-left-radius: 12px; border-top-right-radius: 12px; }"
            "QLabel#dialogTitle { color: %3; font-size: 14px; font-weight: 700; border: none; }"
            "QPushButton#closeBtn { background: transparent; color: %4; border: none; font-size: 16px; font-weight: bold; }"
            "QPushButton#closeBtn:hover { color: #ef4444; }"
            
            "QListWidget#settingsSidebar { background-color: %2; border-right: 1px solid %5; padding: 20px 10px; outline: none; border-bottom-left-radius: 12px; }"
            "QListWidget#settingsSidebar::item { padding: 10px 15px; color: %4; border-radius: 6px; margin-bottom: 4px; font-size: 13px; font-weight: 500; }"
            "QListWidget#settingsSidebar::item:selected { background-color: rgba(139, 92, 246, 0.15); color: %6; font-weight: 700; }"
            "QListWidget#settingsSidebar::item:hover:!selected { background-color: rgba(128,128,128,0.1); }"
            
            "QStackedWidget#settingsStack { background-color: %1; border-bottom-right-radius: 12px; }"
            "QScrollArea#settingsScroll { background: transparent; border: none; }"
            "QWidget#settingsPageContainer { background: transparent; }"
            "QWidget#settingRow { border-bottom: 1px solid %5; }"
            "QLabel#pageTitle { color: %3; font-size: 26px; font-weight: 800; margin-bottom: 25px; border: none; }"
            "QLabel#rowTitle { color: %3; font-size: 15px; font-weight: 600; border: none; }"
            "QLabel#rowDesc { color: %4; font-size: 13px; border: none; margin-top: 4px; }"
            
            "QComboBox { background-color: %1; color: %3; border: 1px solid %5; padding: 8px 15px; border-radius: 6px; outline: none; font-size: 13px; font-weight: 500; }"
            "QComboBox::drop-down { border: none; background: transparent; width: 20px; }"
            "QComboBox::down-arrow { image: none; }" 
            "QComboBox QAbstractItemView { background-color: %1; color: %3; selection-background-color: %6; selection-color: #ffffff; border: 1px solid %5; outline: none; border-radius: 0px; padding: 0px; margin: 0px; }"
            "QComboBox QAbstractItemView::item { min-height: 28px; padding-left: 8px; border: none; background-color: %1; }"
            "QComboBox QAbstractItemView::item:selected { background-color: %6; color: #ffffff; }"
        ).arg(bgMain, bgSide, textCol, textMuted, borderCol, accent, toggleBg));

        for (auto* sw : toggleSwitches) {
            sw->bgOn = QColor(accent);
            sw->bgOff = QColor(toggleBg);
            sw->update();
        }
    }
};

// --- COMMAND PALETTE ---
class CommandPalette : public QDialog {
public:
    QLineEdit *searchBox; 
    QListWidget *listWidget;
    std::unordered_map<QString, std::function<void()>> commands;
    
    explicit CommandPalette(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
        setFixedSize(450, 350);
        
        auto *layout = new QVBoxLayout(this);
        searchBox = new QLineEdit(this);
        searchBox->setPlaceholderText("Type a command...");
        listWidget = new QListWidget(this);
        
        layout->addWidget(searchBox); 
        layout->addWidget(listWidget);
        connect(searchBox, &QLineEdit::textChanged, this, &CommandPalette::filterCommands);
        connect(listWidget, &QListWidget::itemActivated, this, &CommandPalette::executeCommand);
        connect(searchBox, &QLineEdit::returnPressed, [this]() {
            if(listWidget->count() > 0) { listWidget->setCurrentRow(0); executeCommand(listWidget->item(0)); }
        });
    }

    void applyStyles(const QString& t) {
        QString bgMain = (t == "Solarized Light") ? "#fdf6e3" : ((t == "Light") ? "#ffffff" : "#18181b");
        QString textCol = (t == "Solarized Light") ? "#073642" : ((t == "Light") ? "#18181b" : "#e4e4e7");
        QString borderCol = (t == "Solarized Light") ? "#e0d8c3" : ((t == "Light") ? "#e4e4e7" : "#27272a");
        QString inputBg = (t == "Solarized Light") ? "#eee8d5" : ((t == "Light") ? "#f4f4f5" : "#09090b");
        QString highlight = (t == "Solarized Light") ? "#b58900" : "#8b5cf6";
        
        setStyleSheet(QString("QDialog { background-color: %1; color: %2; border: 1px solid %3; border-radius: 12px; font-family: \"Inter\", -apple-system, BlinkMacSystemFont, \"SF Pro Display\", \"Segoe UI\", \"Roboto\", \"Helvetica Neue\", sans-serif; }").arg(bgMain, textCol, borderCol));
        searchBox->setStyleSheet(QString("background-color: %1; border: 1px solid %2; padding: 12px; font-size: 15px; border-radius: 6px; color: %3; outline: none;").arg(inputBg, borderCol, textCol));
        listWidget->setStyleSheet(QString(
            "QListWidget { border: none; font-size: 14px; outline: none; background: transparent; color: %1; margin-top: 10px; }"
            "QListWidget::item { padding: 10px; border-radius: 6px; }"
            "QListWidget::item:selected { background-color: %2; color: white; font-weight: bold; }"
        ).arg(textCol, highlight));
    }

    void addCommand(const QString& name, const std::function<void()>& func) { 
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

// --- SYNTAX HIGHLIGHTER ---
class MarkdownHighlighter : public QSyntaxHighlighter {
    QString currentTheme;
public:
    std::shared_ptr<Hunspell> hunspell;
    std::shared_ptr<bool> spellcheckEnabled;
    
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent), currentTheme("Light") {}
    void setTheme(const QString& theme) { currentTheme = theme; rehighlight(); }
protected:
    void highlightBlock(const QString &text) override {
        QColor linkCol = (currentTheme == "Light") ? QColor("#7c3aed") : (currentTheme == "Solarized Light" ? QColor("#b58900") : QColor("#a78bfa"));
        QColor tagCol = (currentTheme == "Light") ? QColor("#71717a") : (currentTheme == "Solarized Light" ? QColor("#93a1a1") : QColor("#a1a1aa"));
        QColor headerCol = (currentTheme == "Light") ? QColor("#18181b") : (currentTheme == "Solarized Light" ? QColor("#073642") : QColor("#e4e4e7"));
        QColor yamlCol = (currentTheme == "Light") ? QColor("#a1a1aa") : (currentTheme == "Solarized Light" ? QColor("#d5c4a1") : QColor("#71717a"));
        
        QTextCharFormat linkFormat; linkFormat.setForeground(linkCol); 
        QTextCharFormat tagFormat; tagFormat.setForeground(tagCol); 
        QTextCharFormat headerFormat; headerFormat.setFontWeight(QFont::Bold); headerFormat.setForeground(headerCol);
        QTextCharFormat yamlFormat; yamlFormat.setForeground(yamlCol); yamlFormat.setFontItalic(true);
        QTextCharFormat doneFormat; doneFormat.setForeground(yamlCol); doneFormat.setFontStrikeOut(true);

        static const QRegularExpression yamlRegex("^---[\\s\\S]*?---");
        static const QRegularExpression linkRegex("\\[\\[(.*?)\\]\\]");
        static const QRegularExpression tagRegex("(?:^|\\s)(#[a-zA-Z_][a-zA-Z0-9_-]*)");
        static const QRegularExpression headerRegex("^#+\\s.*");
        static const QRegularExpression todoRegex("^(?:\\s*)-\\s\\[\\s\\](.*)");
        static const QRegularExpression doneRegex("^(?:\\s*)-\\s\\[[xX]\\](.*)");

        auto matchAndFormat = [&](const QRegularExpression& regex, const QTextCharFormat& format) {
            auto i = regex.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                setFormat(match.capturedStart(), match.capturedLength(), format);
            }
        };

        matchAndFormat(yamlRegex, yamlFormat);
        matchAndFormat(linkRegex, linkFormat);
        matchAndFormat(tagRegex, tagFormat);
        matchAndFormat(headerRegex, headerFormat);
        matchAndFormat(todoRegex, yamlFormat);
        matchAndFormat(doneRegex, doneFormat);

        // -- LOGICA PENTRU SPELLCHECK --
        if (spellcheckEnabled && *spellcheckEnabled && hunspell) {
            static const QRegularExpression wordRegex("\\b[a-zA-Z']{2,}\\b");
            auto i = wordRegex.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                if (hunspell->spell(match.captured().toStdString()) == 0) {
                    QTextCharFormat fmt = format(match.capturedStart());
                    fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
                    fmt.setUnderlineColor(QColor("#ef4444"));
                    setFormat(match.capturedStart(), match.capturedLength(), fmt);
                }
            }
        }
    }
};

// --- EDITOR ---
class MarkdownEditor : public QTextEdit {
public:
    std::string vaultPath;
    MarkdownHighlighter* highlighter;
    
    explicit MarkdownEditor(QWidget *parent = nullptr) : QTextEdit(parent) {
        setAcceptRichText(false); 
        highlighter = new MarkdownHighlighter(document());
    }
protected:
    void keyPressEvent(QKeyEvent *e) override {
        // SCURTĂTURI PENTRU BOLD ȘI ITALIC
        if (e->modifiers() == Qt::ControlModifier) {
            if (e->key() == Qt::Key_B) {
                QTextCursor c = textCursor();
                if (c.hasSelection()) { QString text = c.selectedText(); c.insertText("**" + text + "**"); }
                return;
            }
            if (e->key() == Qt::Key_I) {
                QTextCursor c = textCursor();
                if (c.hasSelection()) { QString text = c.selectedText(); c.insertText("*" + text + "*"); }
                return;
            }
        }
        QTextEdit::keyPressEvent(e);
    }

    void insertFromMimeData(const QMimeData *source) override {
        if (source->hasUrls()) {
            for (const QUrl &url : source->urls()) {
                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile(); 
                    QFileInfo fileInfo(filePath); 
                    QString suffix = fileInfo.suffix().toLower();
                    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "gif") {
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

struct TabData {
    QPointer<MarkdownEditor> editor;
    QPointer<QTextBrowser> preview;
};

// --- GRAF NODURI ---
enum NodeType { FILE_NODE, TAG_NODE };
class Edge;

class Node : public QGraphicsEllipseItem {
public:
    std::function<void(QString)> onDoubleClick; 

    Node(const QString& name, NodeType type = FILE_NODE) : QGraphicsEllipseItem(-18, -18, 36, 36), name(name), defaultRadius(18) {
        setFlag(ItemIsMovable); setFlag(ItemSendsGeometryChanges);
        setAcceptHoverEvents(true); 
        if (type == TAG_NODE) { setBrush(QColor("#a1a1aa")); setRect(-14, -14, 28, 28); defaultRadius = 14; } 
        else { setBrush(QColor("#8b5cf6")); }
        setPen(QPen(QColor(128, 128, 128, 40), 1)); 
        textItem = new QGraphicsTextItem(name, this);
        QFont modernFont("Inter", type == TAG_NODE ? 10 : 11);
        if (type == TAG_NODE) modernFont.setBold(true);
        textItem->setFont(modernFont);
        textItem->setPos(-textItem->boundingRect().width() / 2, (type == TAG_NODE) ? 16 : 22);
    }
    QString name; std::vector<Edge*> edges; QPointF velocity; double defaultRadius;
    void setTextColor(const QColor& color) { textItem->setDefaultTextColor(color); }
    
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override {
        setRect(-defaultRadius - 2, -defaultRadius - 2, (defaultRadius + 2) * 2, (defaultRadius + 2) * 2);
        setPen(QPen(QColor(139, 92, 246, 200), 3)); 
        QGraphicsEllipseItem::hoverEnterEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override {
        setRect(-defaultRadius, -defaultRadius, defaultRadius * 2, defaultRadius * 2);
        setPen(QPen(QColor(128, 128, 128, 40), 1)); 
        QGraphicsEllipseItem::hoverLeaveEvent(event);
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override {
        if (onDoubleClick) onDoubleClick(name);
        QGraphicsEllipseItem::mouseDoubleClickEvent(event);
    }

private:
    QGraphicsTextItem* textItem;
};

class Edge : public QGraphicsLineItem {
public:
    Edge(Node* source, Node* dest) : source(source), dest(dest) {
        setPen(QPen(QColor(128, 128, 128, 100), 1.0)); setZValue(-1); adjust();
    }
    Node* source; Node* dest;
    void adjust() { if (source && dest) setLine(QLineF(source->pos(), dest->pos())); }
};

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) {
        for (Edge* edge : edges) edge->adjust(); 
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

// --- GRAF VIEW CUSTOM (PENTRU ZOOM) ---
class GraphView : public QGraphicsView {
public:
    GraphView(QGraphicsScene* scene) : QGraphicsView(scene) {
        setRenderHint(QPainter::Antialiasing);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setStyleSheet("background: transparent; border: none; outline: none;");
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            const double scaleFactor = 1.15;
            if (event->angleDelta().y() > 0) scale(scaleFactor, scaleFactor);
            else scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        } else {
            QGraphicsView::wheelEvent(event);
        }
    }
};

// --- MAIN APP ---
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
            keyFile.write(reinterpret_cast<const char*>(global_aes_key), 32);
            keyFile.close();
        }
    }

    std::string vaultPath = "../VaultTest";
    std::regex linkPattern(R"(\[\[(.*?)\]\])");
    std::regex tagPattern(R"((?:^|\s)#([a-zA-Z_][a-zA-Z0-9_-]*))");
    if (!fs::exists(vaultPath)) fs::create_directory(vaultPath);

    // Initializare Hunspell Portabil
    std::shared_ptr<Hunspell> globalHunspell;
    QString dictBase = QCoreApplication::applicationDirPath() + "/dict/en_US";
    
    if (QFile::exists(dictBase + ".aff") && QFile::exists(dictBase + ".dic")) {
        globalHunspell = std::make_shared<Hunspell>((dictBase + ".aff").toStdString().c_str(), (dictBase + ".dic").toStdString().c_str());
    } else {
        QStringList fallbacks = {"/usr/share/myspell/en_US", "/usr/share/hunspell/en_US"};
        for (const auto& p : fallbacks) {
            if (QFile::exists(p + ".aff") && QFile::exists(p + ".dic")) {
                globalHunspell = std::make_shared<Hunspell>((p + ".aff").toStdString().c_str(), (p + ".dic").toStdString().c_str());
                break;
            }
        }
    }
    auto isSpellcheckEnabled = std::make_shared<bool>(QSettings("Orbit", "EnterpriseEdition").value("spellcheck", true).toBool());

    // --- TEME CU EFECT DE LIQUID GLASS ---
    QString themeDark = R"(
        * { font-family: "Inter", -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", "Roboto", "Helvetica Neue", sans-serif; outline: 0; }
        QMainWindow { background-color: #09090b; }
        QWidget#ribbon { background-color: #09090b; }
        QPushButton#ribbonBtn { background: transparent; color: #71717a; border: none; font-size: 18px; border-radius: 10px; margin: 2px; }
        QPushButton#ribbonBtn:hover { background-color: #27272a; color: #ffffff; }
        QWidget#sidebar { background-color: #0e0e11; }
        QLineEdit { background-color: #18181b; color: #e4e4e7; border: 1px solid #27272a; padding: 10px 14px; border-radius: 8px; font-size: 13px; }
        QLineEdit:focus { border: 1px solid #8b5cf6; background-color: #1e1e24; }
        QPushButton#newNoteBtn { background: transparent; color: #a1a1aa; border: 1px dashed #27272a; border-radius: 8px; font-size: 18px; font-weight: bold; }
        QPushButton#newNoteBtn:hover { background-color: #27272a; color: #ffffff; border-style: solid; }
        QTreeWidget, QListWidget { background: transparent; color: #a1a1aa; border: none; font-size: 14px; }
        QTreeWidget::item { padding: 10px 12px; border-radius: 8px; margin: 2px 12px; }
        QTreeWidget::item:selected { background-color: rgba(139, 92, 246, 0.2); color: #c4b5fd; font-weight: 600; }
        QTreeWidget::item:hover:!selected { background-color: rgba(255, 255, 255, 0.04); color: #e4e4e7; }
        QWidget#header { background-color: #09090b; }
        QTabBar::tab { background: transparent; color: #71717a; padding: 12px 24px; border: none; font-size: 13px; font-weight: 500; margin: 6px 4px 0px 4px; border-radius: 8px; }
        QTabBar::tab:selected { background-color: #27272a; color: #ffffff; }
        QTabBar::tab:hover:!selected { background-color: rgba(255, 255, 255, 0.05); color: #e4e4e7; }
        QTabBar::close-button { margin-right: 4px; }
        QPushButton#winCtrl { background: transparent; color: #71717a; border: none; font-size: 12px; border-radius: 6px; margin: 4px; }
        QPushButton#winCtrl:hover { background-color: #27272a; color: #ffffff; }
        QPushButton#winCtrlClose { background: transparent; color: #71717a; border: none; font-size: 12px; border-radius: 6px; margin: 4px; }
        QPushButton#winCtrlClose:hover { background-color: #ef4444; color: white; }
        QWidget#workspace, QStackedWidget { background-color: #0e0e11; }
        QTextEdit, QTextBrowser { background-color: rgba(39, 39, 42, 0.4); color: #e4e4e7; border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 16px; padding: 30px 40px; font-size: 16px; line-height: 1.8; }
        QSplitter::handle { background-color: transparent; width: 0px; }
        QScrollBar:vertical { background: transparent; width: 12px; margin: 0px; }
        QScrollBar::handle:vertical { background: rgba(255, 255, 255, 0.1); border-radius: 6px; min-height: 30px; margin: 2px; }
        QScrollBar::handle:vertical:hover { background: rgba(255, 255, 255, 0.2); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
        QMenu { background-color: #1e1e24; color: #e4e4e7; border: 1px solid #27272a; border-radius: 8px; padding: 6px; }
        QMenu::item { padding: 8px 24px; border-radius: 6px; }
        QMenu::item:selected { background-color: #27272a; }
    )";

    QString themeLight = R"(
        * { font-family: "Inter", -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", "Roboto", "Helvetica Neue", sans-serif; outline: 0; }
        QMainWindow { background-color: #f4f4f5; }
        QWidget#ribbon { background-color: #ffffff; }
        QPushButton#ribbonBtn { background: transparent; color: #71717a; border: none; font-size: 18px; border-radius: 10px; margin: 2px; }
        QPushButton#ribbonBtn:hover { background-color: #f4f4f5; color: #18181b; }
        QWidget#sidebar { background-color: #fafafa; }
        QLineEdit { background-color: #ffffff; color: #18181b; border: 1px solid #e4e4e7; padding: 10px 14px; border-radius: 8px; font-size: 13px; }
        QLineEdit:focus { border: 1px solid #8b5cf6; background-color: #ffffff; }
        QPushButton#newNoteBtn { background: transparent; color: #a1a1aa; border: 1px dashed #e4e4e7; border-radius: 8px; font-size: 18px; font-weight: bold; }
        QPushButton#newNoteBtn:hover { background-color: #f4f4f5; color: #18181b; border-style: solid; }
        QTreeWidget, QListWidget { background: transparent; color: #52525b; border: none; font-size: 14px; }
        QTreeWidget::item { padding: 10px 12px; border-radius: 8px; margin: 2px 12px; }
        QTreeWidget::item:selected { background-color: rgba(139, 92, 246, 0.1); color: #7c3aed; font-weight: 600; }
        QTreeWidget::item:hover:!selected { background-color: rgba(0, 0, 0, 0.03); color: #18181b; }
        QWidget#header { background-color: #ffffff; }
        QTabBar::tab { background: transparent; color: #a1a1aa; padding: 12px 24px; border: none; font-size: 13px; font-weight: 500; margin: 6px 4px 0px 4px; border-radius: 8px; }
        QTabBar::tab:selected { background-color: #f4f4f5; color: #18181b; }
        QTabBar::tab:hover:!selected { background-color: #fafafa; color: #18181b; }
        QTabBar::close-button { margin-right: 4px; }
        QPushButton#winCtrl { background: transparent; color: #a1a1aa; border: none; font-size: 12px; border-radius: 6px; margin: 4px; }
        QPushButton#winCtrl:hover { background-color: #f4f4f5; color: #18181b; }
        QPushButton#winCtrlClose { background: transparent; color: #a1a1aa; border: none; font-size: 12px; border-radius: 6px; margin: 4px; }
        QPushButton#winCtrlClose:hover { background-color: #ef4444; color: white; }
        QWidget#workspace, QStackedWidget { background-color: #f4f4f5; }
        QTextEdit, QTextBrowser { background-color: rgba(255, 255, 255, 0.7); color: #18181b; border: 1px solid rgba(255, 255, 255, 1.0); border-radius: 16px; padding: 30px 40px; font-size: 16px; line-height: 1.8; }
        QSplitter::handle { background-color: transparent; width: 0px; }
        QScrollBar:vertical { background: transparent; width: 12px; margin: 0px; }
        QScrollBar::handle:vertical { background: rgba(0, 0, 0, 0.1); border-radius: 6px; min-height: 30px; margin: 2px; }
        QScrollBar::handle:vertical:hover { background: rgba(0, 0, 0, 0.2); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
        QMenu { background-color: #ffffff; color: #18181b; border: 1px solid #e4e4e7; border-radius: 8px; padding: 6px; }
        QMenu::item { padding: 8px 24px; border-radius: 6px; }
        QMenu::item:selected { background-color: #f4f4f5; }
    )";

    QString themeSolarizedLight = R"(
        * { font-family: "Inter", -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", "Roboto", "Helvetica Neue", sans-serif; outline: 0; }
        QMainWindow { background-color: #eee8d5; }
        QWidget#ribbon { background-color: #fdf6e3; }
        QPushButton#ribbonBtn { background: transparent; color: #93a1a1; border: none; font-size: 18px; border-radius: 10px; margin: 2px; }
        QPushButton#ribbonBtn:hover { background-color: #eee8d5; color: #073642; }
        QWidget#sidebar { background-color: #f5efdc; }
        QLineEdit { background-color: #fdf6e3; color: #073642; border: 1px solid #e0d8c3; padding: 10px 14px; border-radius: 8px; font-size: 13px; }
        QLineEdit:focus { border: 1px solid #b58900; background-color: #fdf6e3; }
        QPushButton#newNoteBtn { background: transparent; color: #93a1a1; border: 1px dashed #e0d8c3; border-radius: 8px; font-size: 18px; font-weight: bold; }
        QPushButton#newNoteBtn:hover { background-color: #eee8d5; color: #073642; border-style: solid; }
        QTreeWidget, QListWidget { background: transparent; color: #586e75; border: none; font-size: 14px; }
        QTreeWidget::item { padding: 10px 12px; border-radius: 8px; margin: 2px 12px; }
        QTreeWidget::item:selected { background-color: rgba(181, 137, 0, 0.15); color: #b58900; font-weight: 600; }
        QTreeWidget::item:hover:!selected { background-color: rgba(0, 0, 0, 0.03); color: #073642; }
        QWidget#header { background-color: #fdf6e3; }
        QTabBar::tab { background: transparent; color: #93a1a1; padding: 12px 24px; border: none; font-size: 13px; font-weight: 500; margin: 6px 4px 0px 4px; border-radius: 8px; }
        QTabBar::tab:selected { background-color: #eee8d5; color: #073642; }
        QTabBar::tab:hover:!selected { background-color: rgba(0,0,0,0.02); color: #073642; }
        QTabBar::close-button { margin-right: 4px; }
        QPushButton#winCtrl { background: transparent; color: #93a1a1; border: none; font-size: 12px; border-radius: 6px; margin: 4px; }
        QPushButton#winCtrl:hover { background-color: #eee8d5; color: #073642; }
        QPushButton#winCtrlClose { background: transparent; color: #93a1a1; border: none; font-size: 12px; border-radius: 6px; margin: 4px; }
        QPushButton#winCtrlClose:hover { background-color: #ef4444; color: white; }
        QWidget#workspace, QStackedWidget { background-color: #eee8d5; }
        QTextEdit, QTextBrowser { background-color: rgba(253, 246, 227, 0.6); color: #073642; border: 1px solid rgba(253, 246, 227, 0.9); border-radius: 16px; padding: 30px 40px; font-size: 16px; line-height: 1.8; }
        QSplitter::handle { background-color: transparent; width: 0px; }
        QScrollBar:vertical { background: transparent; width: 12px; margin: 0px; }
        QScrollBar::handle:vertical { background: rgba(0, 0, 0, 0.1); border-radius: 6px; min-height: 30px; margin: 2px; }
        QScrollBar::handle:vertical:hover { background: rgba(0, 0, 0, 0.2); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
        QMenu { background-color: #fdf6e3; color: #073642; border: 1px solid #e0d8c3; border-radius: 8px; padding: 6px; }
        QMenu::item { padding: 8px 24px; border-radius: 6px; }
        QMenu::item:selected { background-color: #eee8d5; }
    )";

    QMainWindow window;
    window.setWindowTitle("Orbit");
    window.resize(1300, 800); 
    window.setWindowFlags(Qt::FramelessWindowHint);

    auto *central = new QWidget(&window);
    window.setCentralWidget(central);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    auto *ribbon = new QWidget();
    ribbon->setObjectName("ribbon");
    ribbon->setFixedWidth(60);
    auto *ribbonLayout = new QVBoxLayout(ribbon);
    ribbonLayout->setContentsMargins(8, 15, 8, 15);
    ribbonLayout->setSpacing(12);
    ribbon->installEventFilter(new WindowDragger(&window));

    auto *rBtnFiles = new QPushButton("◫"); rBtnFiles->setObjectName("ribbonBtn"); rBtnFiles->setFixedSize(44, 44);
    auto *rBtnSearch = new QPushButton("🔍"); rBtnSearch->setObjectName("ribbonBtn"); rBtnSearch->setFixedSize(44, 44);
    auto *rBtnGraph = new QPushButton("🕸️"); rBtnGraph->setObjectName("ribbonBtn"); rBtnGraph->setFixedSize(44, 44);
    ribbonLayout->addWidget(rBtnFiles); ribbonLayout->addWidget(rBtnSearch); ribbonLayout->addWidget(rBtnGraph);
    ribbonLayout->addStretch();
    
    auto *rBtnSettings = new QPushButton("⚙️"); rBtnSettings->setObjectName("ribbonBtn"); rBtnSettings->setFixedSize(44, 44);
    ribbonLayout->addWidget(rBtnSettings);
    rootLayout->addWidget(ribbon);

    auto *mainSplitter = new QSplitter(Qt::Horizontal);
    rootLayout->addWidget(mainSplitter);

    auto *sidebar = new QWidget();
    sidebar->setObjectName("sidebar");
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    
    auto *sidebarDrag = new QWidget();
    sidebarDrag->setFixedHeight(45);
    sidebarDrag->installEventFilter(new WindowDragger(&window));
    sidebarLayout->addWidget(sidebarDrag);

    auto *searchArea = new QWidget();
    auto *searchH = new QHBoxLayout(searchArea);
    searchH->setContentsMargins(20, 0, 20, 15);
    auto *searchBox = new QLineEdit();
    searchBox->setPlaceholderText("Search notes...");
    auto *btnNewNote = new QPushButton("+");
    btnNewNote->setObjectName("newNoteBtn");
    btnNewNote->setFixedSize(36, 36);
    searchH->addWidget(searchBox); searchH->addWidget(btnNewNote);
    sidebarLayout->addWidget(searchArea);

    auto *treeWidget = new QTreeWidget();
    treeWidget->setHeaderHidden(true); 
    sidebarLayout->addWidget(treeWidget);

    auto *vaultName = new QLabel(" Orbit Vault");
    vaultName->setStyleSheet("padding: 20px; font-weight: bold; font-size: 12px; opacity: 0.5;");
    sidebarLayout->addWidget(vaultName);
    mainSplitter->addWidget(sidebar);

    auto *workspace = new QWidget();
    workspace->setObjectName("workspace");
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0,0,0,0);
    workspaceLayout->setSpacing(0);

    auto *header = new QWidget();
    header->setObjectName("header");
    header->setFixedHeight(45);
    header->installEventFilter(new WindowDragger(&window)); 
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0,0,0,0);
    headerLayout->setSpacing(0);

    auto *topTabs = new QTabBar();
    topTabs->setTabsClosable(true);
    topTabs->setDrawBase(false);
    topTabs->setUsesScrollButtons(false);
    
    auto *btnMin = new QPushButton("—"); btnMin->setObjectName("winCtrl"); btnMin->setFixedSize(45, 35);
    auto *btnMax = new QPushButton("◻"); btnMax->setObjectName("winCtrl"); btnMax->setFixedSize(45, 35);
    auto *btnClose = new QPushButton("✕"); btnClose->setObjectName("winCtrlClose"); btnClose->setFixedSize(45, 35);
    
    QObject::connect(btnMin, &QPushButton::clicked, &window, &QMainWindow::showMinimized);
    QObject::connect(btnMax, &QPushButton::clicked, [&]() {
        if (window.isMaximized()) window.showNormal(); else window.showMaximized();
    });
    QObject::connect(btnClose, &QPushButton::clicked, &window, &QMainWindow::close);

    headerLayout->addWidget(topTabs);
    headerLayout->addStretch();
    headerLayout->addWidget(btnMin);
    headerLayout->addWidget(btnMax);
    headerLayout->addWidget(btnClose);
    workspaceLayout->addWidget(header);

    auto *tabContent = new QStackedWidget();
    tabContent->setStyleSheet("background: transparent;");
    workspaceLayout->addWidget(tabContent);

    auto *statusW = new QWidget();
    statusW->setFixedHeight(30);
    auto *statusLayout = new QHBoxLayout(statusW);
    statusLayout->setContentsMargins(15,0,15,0);
    auto *statsLabel = new QLabel("Ready");
    statsLabel->setStyleSheet("font-size: 12px; opacity: 0.5; font-weight: 500;");
    statusLayout->addStretch(); statusLayout->addWidget(statsLabel);
    workspaceLayout->addWidget(statusW);

    mainSplitter->addWidget(workspace);
    mainSplitter->setSizes(QList<int>() << 300 << 1000);

    auto *palette = new CommandPalette(&window);
    auto activeTabs = std::make_shared<std::vector<TabData>>();

    // --- VARIABILE GLOBALE ---
    auto physicsTimer = std::make_shared<QTimer>();
    auto *graphScene = new QGraphicsScene();
    auto activeNodes = std::make_shared<std::vector<Node*>>();
    auto isPhysicsEnabled = std::make_shared<bool>(true);
    auto currentGraphTextColor = std::make_shared<QString>("#18181b"); 
    auto currentThemeName = std::make_shared<QString>("Light");
    auto globalGraph = std::make_shared<std::unordered_map<std::string, std::vector<std::string>>>();
    auto globalFileContents = std::make_shared<std::unordered_map<std::string, QString>>(); 

    auto formatPreviewMarkdown = [](QString md) -> QString {
        QRegularExpression yamlRegex("^---\\n([\\s\\S]*?)\\n---"); return md.replace(yamlRegex, ""); 
    };

    auto updateStats = [statsLabel](QTextEdit* editor) {
        if (!editor) { statsLabel->setText("Graph View Active"); return; }
        QString text = editor->toPlainText();
        int words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
        if (text.trimmed().isEmpty()) words = 0;
        statsLabel->setText(QString("%1 words  •  %2 characters").arg(words).arg(text.length()));
    };

    auto *saveTimer = new QTimer(&window);
    saveTimer->setSingleShot(true);
    saveTimer->setInterval(500); 
    auto currentSavePath = std::make_shared<std::string>("");
    auto currentSaveText = std::make_shared<QString>("");

    QObject::connect(saveTimer, &QTimer::timeout, [currentSavePath, currentSaveText, &global_aes_key]() {
        if (currentSavePath->empty() || currentSaveText->isEmpty()) return;
        QSaveFile sFile(QString::fromStdString(*currentSavePath));
        if (sFile.open(QIODevice::WriteOnly)) {
            sFile.write(encryptAES(currentSaveText->toUtf8(), global_aes_key)); 
            sFile.commit(); 
        }
    });

    auto applyThemeGlobal = [&](const QString& themeName) {
        *currentThemeName = themeName;
        if (themeName == "Dark") { app.setStyleSheet(themeDark); *currentGraphTextColor = "#e4e4e7"; }
        else if (themeName == "Light") { app.setStyleSheet(themeLight); *currentGraphTextColor = "#18181b"; }
        else if (themeName == "Solarized Light") { app.setStyleSheet(themeSolarizedLight); *currentGraphTextColor = "#073642"; }
        
        palette->applyStyles(themeName); 
        for (Node* n : *activeNodes) {
            if (n) n->setTextColor(QColor(*currentGraphTextColor));
        }
        
        QString css = "";
        if (themeName == "Dark") css = R"( body { font-family: "Inter", -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", "Roboto", "Helvetica Neue", sans-serif; font-size: 16px; color: #e4e4e7; line-height: 1.8; } h1, h2, h3 { color: #ffffff; margin-top: 1.2em; font-weight: 800; } h1 { padding-bottom: 0.2em; border-bottom: 1px solid rgba(255,255,255,0.05); } a { color: #a78bfa; text-decoration: none; } hr { border: 0; border-top: 1px solid rgba(255,255,255,0.05); } blockquote { border-left: 3px solid #8b5cf6; padding-left: 1em; color: #a1a1aa; font-style: italic; } code { background-color: #27272a; padding: 0.2em 0.4em; border-radius: 4px; color: #a78bfa; } )";
        else if (themeName == "Light") css = R"( body { font-family: "Inter", -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", "Roboto", "Helvetica Neue", sans-serif; font-size: 16px; color: #18181b; line-height: 1.8; } h1, h2, h3 { color: #000000; margin-top: 1.2em; font-weight: 800; } h1 { padding-bottom: 0.2em; border-bottom: 1px solid rgba(0,0,0,0.05); } a { color: #8b5cf6; text-decoration: none; } hr { border: 0; border-top: 1px solid rgba(0,0,0,0.05); } blockquote { border-left: 3px solid #8b5cf6; padding-left: 1em; color: #71717a; font-style: italic; } code { background-color: #f4f4f5; padding: 0.2em 0.4em; border-radius: 4px; color: #7c3aed; } )";
        else if (themeName == "Solarized Light") css = R"( body { font-family: "Inter", -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", "Roboto", "Helvetica Neue", sans-serif; font-size: 16px; color: #073642; line-height: 1.8; } h1, h2, h3 { color: #002b36; margin-top: 1.2em; font-weight: 800; } h1 { padding-bottom: 0.2em; border-bottom: 1px solid rgba(0,0,0,0.05); } a { color: #b58900; text-decoration: none; } hr { border: 0; border-top: 1px solid rgba(0,0,0,0.05); } blockquote { border-left: 3px solid #b58900; padding-left: 1em; color: #586e75; font-style: italic; } code { background-color: #eee8d5; padding: 0.2em 0.4em; border-radius: 4px; color: #cb4b16; } )";

        for (auto& tab : *activeTabs) {
            if (tab.editor && tab.preview) {
                tab.preview->document()->setDefaultStyleSheet(css);
                tab.preview->setMarkdown(formatPreviewMarkdown(tab.editor->toPlainText()));
                if (tab.editor->highlighter) {
                    tab.editor->highlighter->setTheme(themeName);
                }
            }
        }
        QSettings s("Orbit", "EnterpriseEdition"); s.setValue("theme", themeName);
    };

    auto onSpellcheckChange = [&](bool enabled) {
        *isSpellcheckEnabled = enabled;
        for (auto& tab : *activeTabs) {
            if (tab.editor && tab.editor->highlighter) {
                tab.editor->highlighter->rehighlight();
            }
        }
    };

    auto openNoteInTab = [&](const std::string& fullPath, const QString& tabName) {
        for (int i = 0; i < topTabs->count(); ++i) {
            if (tabContent->widget(i)->property("file_path").toString().toStdString() == fullPath) {
                topTabs->setCurrentIndex(i); return;
            }
        }
        QFile file(QString::fromStdString(fullPath));
        if (!file.open(QIODevice::ReadOnly)) return;
        QString text = QString::fromUtf8(decryptAES(file.readAll(), global_aes_key));

        auto *textSplitter = new QSplitter(Qt::Horizontal);
        textSplitter->setProperty("file_path", QString::fromStdString(fullPath)); 

        auto *textEdit = new MarkdownEditor();
        textEdit->vaultPath = vaultPath;
        textEdit->highlighter->hunspell = globalHunspell;
        textEdit->highlighter->spellcheckEnabled = isSpellcheckEnabled;
        textEdit->setPlainText(text);

        auto *markdownPreview = new QTextBrowser();
        markdownPreview->setOpenExternalLinks(true); 
        markdownPreview->document()->setBaseUrl(QUrl::fromLocalFile(QDir(QString::fromStdString(vaultPath)).absolutePath() + "/"));
        
        // Wrap în containere pentru a obține spațierea efectului Liquid Glass
        auto *editorContainer = new QWidget();
        editorContainer->setStyleSheet("background: transparent;");
        auto *eLayout = new QVBoxLayout(editorContainer);
        eLayout->setContentsMargins(20, 20, 10, 20); 
        eLayout->addWidget(textEdit);

        auto *previewContainer = new QWidget();
        previewContainer->setStyleSheet("background: transparent;");
        auto *pLayout = new QVBoxLayout(previewContainer);
        pLayout->setContentsMargins(10, 20, 20, 20);
        pLayout->addWidget(markdownPreview);

        textSplitter->addWidget(editorContainer);
        textSplitter->addWidget(previewContainer);
        textSplitter->setSizes(QList<int>() << 500 << 500); 

        activeTabs->push_back({textEdit, markdownPreview});

        int newIndex = tabContent->addWidget(textSplitter);
        topTabs->addTab(tabName); topTabs->setCurrentIndex(newIndex);
        
        applyThemeGlobal(*currentThemeName); 
        
        QScrollBar *ls = textEdit->verticalScrollBar(); QScrollBar *rs = markdownPreview->verticalScrollBar();
        auto *anim = new QPropertyAnimation(rs, "value", textSplitter);
        anim->setDuration(150); anim->setEasingCurve(QEasingCurve::OutCubic); 
        QObject::connect(ls, &QScrollBar::valueChanged, [ls, rs, anim](int v) {
            if (ls->maximum() > 0) {
                int t = ((double)v / ls->maximum()) * rs->maximum();
                if (anim->state() == QAbstractAnimation::Running) anim->stop();
                anim->setStartValue(rs->value()); anim->setEndValue(t); anim->start();
            }
        });

        auto isProg = std::make_shared<bool>(false);
        QObject::connect(textEdit, &QTextEdit::textChanged, [=]() {
            if (!*isProg) {
                *currentSavePath = fullPath;
                *currentSaveText = textEdit->toPlainText();
                saveTimer->start(); 
                
                markdownPreview->setMarkdown(formatPreviewMarkdown(*currentSaveText));
                
                auto *splitter = qobject_cast<QSplitter*>(tabContent->currentWidget());
                if (splitter && splitter->count() > 0) {
                    updateStats(textEdit);
                }
            }
        });
    };

    auto reloadSystem = [&](bool restoreSelection = false) {
        QString selectedPath = "";
        if (restoreSelection && treeWidget->currentItem()) {
            selectedPath = treeWidget->currentItem()->data(0, Qt::UserRole).toString();
        }
        
        activeNodes->clear(); treeWidget->clear(); graphScene->clear(); globalGraph->clear(); globalFileContents->clear();
        std::unordered_map<std::string, Node*> nodesMap;
        if (fs::exists(vaultPath)) {
            for (const auto& entry : fs::recursive_directory_iterator(vaultPath)) {
                if (entry.path().filename().string().rfind(".", 0) == 0) continue; 
                if (entry.is_regular_file() && entry.path().extension() == ".md") {
                    std::string relPath = fs::relative(entry.path(), vaultPath).string();
                    (*globalGraph)[relPath] = std::vector<std::string>();
                    QFile file(QString::fromStdString(entry.path().string()));
                    if (file.open(QIODevice::ReadOnly)) {
                        QString qText = QString::fromUtf8(decryptAES(file.readAll(), global_aes_key));
                        (*globalFileContents)[relPath] = qText; 
                        std::string line; std::istringstream stream(qText.toStdString());
                        while (std::getline(stream, line)) {
                            std::smatch match; std::string temp = line;
                            while (std::regex_search(temp, match, linkPattern)) {
                                std::string link = match[1].str(); if (link.length() >= 3 && link.substr(link.length() - 3) == ".md") link = link.substr(0, link.length() - 3);
                                (*globalGraph)[relPath].push_back(link); temp = match.suffix().str();
                            }
                        }
                    }
                    auto *noteItem = new QTreeWidgetItem(treeWidget);
                    noteItem->setText(0, QString::fromStdString(entry.path().filename().string())); 
                    noteItem->setData(0, Qt::UserRole, QString::fromStdString(relPath)); 
                    
                    if (restoreSelection && QString::fromStdString(relPath) == selectedPath) {
                        treeWidget->setCurrentItem(noteItem);
                    }
                }
            }
        }
        for (const auto& pair : *globalGraph) {
            std::string nName = std::filesystem::path(pair.first).filename().string();
            if (nodesMap.find(nName) == nodesMap.end()) { 
                Node* n = new Node(QString::fromStdString(nName), FILE_NODE); 
                n->onDoubleClick = [=](QString name) {
                    openNoteInTab(vaultPath + "/" + name.toStdString(), name);
                };
                nodesMap[nName] = n; graphScene->addItem(n); activeNodes->push_back(n); n->setPos(rand() % 500 - 250, rand() % 500 - 250); 
            }
            for(const auto& link : pair.second) {
                if (nodesMap.find(link) == nodesMap.end()) { 
                    Node* n = new Node(QString::fromStdString(link), FILE_NODE); 
                    n->onDoubleClick = [=](QString name) {
                        openNoteInTab(vaultPath + "/" + name.toStdString(), name);
                    };
                    nodesMap[link] = n; graphScene->addItem(n); activeNodes->push_back(n); n->setPos(rand() % 500 - 250, rand() % 500 - 250); 
                }
            }
        }
        for (const auto& pair : *globalGraph) {
            Node* sNode = nodesMap[std::filesystem::path(pair.first).filename().string()];
            for (const std::string& link : pair.second) { Node* dNode = nodesMap[link]; Edge* edge = new Edge(sNode, dNode); graphScene->addItem(edge); sNode->edges.push_back(edge); dNode->edges.push_back(edge); }
        }
        for (Node* n : *activeNodes) {
            if (n) n->setTextColor(QColor(*currentGraphTextColor));
        }
        physicsTimer->start(16); 
    };

    auto deleteNote = [&](QTreeWidgetItem* item) {
        if (!item) return;
        auto reply = QMessageBox::question(&window, "Delete Note", "Are you sure you want to delete '" + item->text(0) + "'?", QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QString relPath = item->data(0, Qt::UserRole).toString();
            std::string fullPath = vaultPath + "/" + relPath.toStdString();
            QFile::remove(QString::fromStdString(fullPath));
            
            for (int i = 0; i < topTabs->count(); ++i) {
                if (tabContent->widget(i)->property("file_path").toString() == QString::fromStdString(fullPath)) {
                    QWidget* w = tabContent->widget(i);
                    if (topTabs->currentIndex() == i) {
                        if (i > 0) topTabs->setCurrentIndex(i - 1);
                        else if (topTabs->count() > 1) topTabs->setCurrentIndex(1);
                    }
                    topTabs->removeTab(i);
                    tabContent->removeWidget(w);
                    w->deleteLater();
                    break;
                }
            }
            reloadSystem(false);
        }
    };

    auto renameNote = [&](QTreeWidgetItem* item) {
        if (!item) return;
        QString oldName = item->text(0);
        QString oldBase = QFileInfo(oldName).completeBaseName();
        QString newBase = QInputDialog::getText(&window, "Rename Note", "New name:", QLineEdit::Normal, oldBase);
        
        if (!newBase.isEmpty() && newBase != oldBase) {
            QString newName = newBase + ".md";
            QString oldRelPath = item->data(0, Qt::UserRole).toString();
            std::string oldFullPath = vaultPath + "/" + oldRelPath.toStdString();
            std::string newFullPath = vaultPath + "/" + newName.toStdString();
            
            QFile::rename(QString::fromStdString(oldFullPath), QString::fromStdString(newFullPath));
            
            for (int i = 0; i < topTabs->count(); ++i) {
                if (tabContent->widget(i)->property("file_path").toString() == QString::fromStdString(oldFullPath)) {
                    tabContent->widget(i)->setProperty("file_path", QString::fromStdString(newFullPath));
                    topTabs->setTabText(i, newName);
                    break;
                }
            }
            reloadSystem(true);
        }
    };

    auto createNewNote = [&]() {
        QString text = QInputDialog::getText(nullptr, "New Note", "Name:", QLineEdit::Normal, "");
        if (!text.isEmpty()) { 
            std::string fullPath = vaultPath + "/" + text.toStdString() + ".md";
            QFile file(QString::fromStdString(fullPath));
            if (file.open(QIODevice::WriteOnly)) file.write(encryptAES(QByteArray(""), global_aes_key));
            reloadSystem(true);
            openNoteInTab(fullPath, text + ".md");
        }
    };

    auto *actionNew = new QAction("New Note", &window); 
    actionNew->setShortcut(QKeySequence("Ctrl+N")); 
    window.addAction(actionNew);

    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(treeWidget, &QTreeWidget::customContextMenuRequested, [&](const QPoint &pos) {
        QTreeWidgetItem *item = treeWidget->itemAt(pos);
        if (!item) return;
        QMenu menu(treeWidget);
        QAction *renameAct = menu.addAction("Rename (F2)");
        QAction *deleteAct = menu.addAction("Delete (Del)");
        QAction *selected = menu.exec(treeWidget->viewport()->mapToGlobal(pos));
        
        if (selected == deleteAct) deleteNote(item);
        else if (selected == renameAct) renameNote(item);
    });

    auto *delShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), treeWidget);
    delShortcut->setContext(Qt::WidgetShortcut);
    QObject::connect(delShortcut, &QShortcut::activated, [&]() { deleteNote(treeWidget->currentItem()); });
    
    auto *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), treeWidget);
    renameShortcut->setContext(Qt::WidgetShortcut);
    QObject::connect(renameShortcut, &QShortcut::activated, [&]() { renameNote(treeWidget->currentItem()); });

    QObject::connect(physicsTimer.get(), &QTimer::timeout, [activeNodes, isPhysicsEnabled]() {
        if (!(*isPhysicsEnabled)) return; 
        bool needsUpdate = false;
        
        for (Node* n1 : *activeNodes) {
            if (n1->scene() && n1->scene()->mouseGrabberItem() == n1) { n1->velocity = QPointF(0, 0); needsUpdate = true; continue; }
            QPointF force(0, 0);
            
            for (Node* n2 : *activeNodes) {
                if (n1 == n2) continue; 
                QPointF d = n1->pos() - n2->pos();
                double distSq = d.x()*d.x() + d.y()*d.y();
                if (distSq > 0 && distSq < 400000) { 
                    double dist = std::sqrt(distSq);
                    force += (d / dist) * (5000.0 / distSq);
                }
            }
            for (Edge* e : n1->edges) {
                Node* n2 = (e->source == n1) ? e->dest : e->source; 
                QPointF d = n2->pos() - n1->pos();
                double distSq = d.x()*d.x() + d.y()*d.y();
                if (distSq > 0) {
                    double dist = std::sqrt(distSq); 
                    force += (d / dist) * ((dist - 180.0) * 0.04); 
                }
            }
            QPointF dCenter = QPointF(0,0) - n1->pos(); 
            double distCenterSq = dCenter.x()*dCenter.x() + dCenter.y()*dCenter.y();
            if (distCenterSq > 0) {
                double distCenter = std::sqrt(distCenterSq);
                force += (dCenter / distCenter) * (distCenter * 0.010);
            }
            
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

    QObject::connect(topTabs, &QTabBar::currentChanged, tabContent, &QStackedWidget::setCurrentIndex);
    QObject::connect(topTabs, &QTabBar::tabCloseRequested, [&](int index) {
        QWidget* widget = tabContent->widget(index);
        if (topTabs->currentIndex() == index) {
            if (index > 0) topTabs->setCurrentIndex(index - 1);
            else if (topTabs->count() > 1) topTabs->setCurrentIndex(1);
        }
        topTabs->removeTab(index); 
        tabContent->removeWidget(widget); 
        widget->deleteLater();
    });
    
    QObject::connect(topTabs, &QTabBar::currentChanged, [&](int index) {
        if (index == -1) { updateStats(nullptr); return; }
        QWidget* currentWidget = tabContent->widget(index);
        if (dynamic_cast<GraphView*>(currentWidget)) {
            updateStats(nullptr); 
            return;
        }
        for (auto& tab : *activeTabs) {
            // tab.editor este acum în interiorul unui container
            if (tab.editor && tab.editor->parentWidget()->parentWidget() == currentWidget) {
                updateStats(tab.editor);
                break;
            }
        }
    });

    QObject::connect(rBtnFiles, &QPushButton::clicked, [sidebar](){ sidebar->setVisible(!sidebar->isVisible()); });
    QObject::connect(rBtnSearch, &QPushButton::clicked, [searchBox, sidebar](){ if(!sidebar->isVisible()) sidebar->setVisible(true); searchBox->setFocus(); });
    
    QObject::connect(rBtnGraph, &QPushButton::clicked, [&](){ 
        for (int i = 0; i < tabContent->count(); ++i) {
            if (dynamic_cast<GraphView*>(tabContent->widget(i))) {
                topTabs->setCurrentIndex(i); 
                return;
            }
        }
        auto *newGraphView = new GraphView(graphScene);
        int newIndex = tabContent->addWidget(newGraphView);
        topTabs->addTab("Graph view");
        topTabs->setCurrentIndex(newIndex);
    });
    
    QObject::connect(rBtnSettings, &QPushButton::clicked, [&](){ SettingsDialog d(&window, applyThemeGlobal, onSpellcheckChange); d.exec(); });
    QObject::connect(btnNewNote, &QPushButton::clicked, createNewNote);
    QObject::connect(actionNew, &QAction::triggered, createNewNote);

    reloadSystem(false);

    QObject::connect(searchBox, &QLineEdit::textChanged, [&](const QString &text) {
        for (QTreeWidgetItemIterator it(treeWidget); *it; ++it) {
            QString relPath = (*it)->data(0, Qt::UserRole).toString(); bool match = (*it)->text(0).contains(text, Qt::CaseInsensitive);
            if (!match && globalFileContents->find(relPath.toStdString()) != globalFileContents->end()) match = (*globalFileContents)[relPath.toStdString()].contains(text, Qt::CaseInsensitive);
            (*it)->setHidden(!match);
        }
    });

    QObject::connect(treeWidget, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int) {
        openNoteInTab(vaultPath + "/" + item->data(0, Qt::UserRole).toString().toStdString(), item->text(0));
    });

    bool hasNotes = false; QTreeWidgetItemIterator it(treeWidget);
    while (*it) {
        if (!(*it)->data(0, Qt::UserRole).toString().isEmpty()) {
            hasNotes = true; openNoteInTab(vaultPath + "/" + (*it)->data(0, Qt::UserRole).toString().toStdString(), (*it)->text(0)); break; 
        } ++it;
    }

    if (!hasNotes) {
        std::string welcomeFullPath = vaultPath + "/Welcome.md"; QFile file(QString::fromStdString(welcomeFullPath));
        if (file.open(QIODevice::WriteOnly)) file.write(encryptAES(QString("# ✨ Welcome to Orbit\n\nThis is your new *vault*.\n\nMake a note of something, [[create a link]], sau explorează Graph Map din stânga.\n").toUtf8(), global_aes_key));
        reloadSystem(false); openNoteInTab(welcomeFullPath, "Welcome.md");
    }

    auto *initialGraphView = new GraphView(graphScene);
    tabContent->insertWidget(0, initialGraphView);
    topTabs->insertTab(0, "Graph view");
    if (!hasNotes) topTabs->setCurrentIndex(0);

    palette->addCommand("Settings: Open Settings", [&]() { SettingsDialog d(&window, applyThemeGlobal, onSpellcheckChange); d.exec(); });
    palette->addCommand("File: New Note", [&]() { actionNew->trigger(); });
    auto *actionPalette = new QAction("Command Palette", &window); actionPalette->setShortcut(QKeySequence("Ctrl+P")); window.addAction(actionPalette);
    QObject::connect(actionPalette, &QAction::triggered, [palette, &window]() {
        palette->searchBox->clear(); palette->filterCommands(""); QPoint center = window.geometry().center() - palette->rect().center(); palette->move(center); palette->show(); palette->searchBox->setFocus();
    });

    QSettings settings("Orbit", "EnterpriseEdition");
    applyThemeGlobal(settings.value("theme", "Light").toString());

    window.show();
    return app.exec();
}