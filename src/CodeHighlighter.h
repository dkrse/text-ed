#pragma once
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

class CodeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    enum Language {
        None, Cpp, Python, JavaScript, Java, Rust, Go, Shell, SQL,
        HTML, CSS, Ruby, PHP, Lua, Perl, Haskell, Kotlin, Swift,
        TypeScript, CSharp, Xml, Json, Yaml, Toml, Ini, CMake, Makefile, Asm
    };

    explicit CodeHighlighter(QTextDocument *parent = nullptr);

    void applyThemeColors(const QColor &keyword, const QColor &type, const QColor &string,
                          const QColor &comment, const QColor &number, const QColor &function,
                          const QColor &preprocessor, const QColor &operatorColor);
    void setLanguage(Language lang);
    Language language() const { return m_language; }

    static Language detectLanguage(const QString &filePath);
    static QString languageName(Language lang);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void setupFormats();
    void buildRules();
    void addCppRules();
    void addPythonRules();
    void addJavaScriptRules();
    void addJavaRules();
    void addRustRules();
    void addGoRules();
    void addShellRules();
    void addSqlRules();
    void addHtmlRules();
    void addCssRules();
    void addRubyRules();
    void addLuaRules();
    void addGenericRules();
    void addJsonRules();
    void addYamlRules();
    void addCMakeRules();
    void addMakefileRules();
    void addAsmRules();

    Language m_language = None;
    QVector<Rule> m_rules;

    // Shared formats
    QTextCharFormat m_keywordFmt;
    QTextCharFormat m_typeFmt;
    QTextCharFormat m_stringFmt;
    QTextCharFormat m_commentFmt;
    QTextCharFormat m_numberFmt;
    QTextCharFormat m_functionFmt;
    QTextCharFormat m_preprocessorFmt;
    QTextCharFormat m_operatorFmt;

    // Multi-line comment state
    QRegularExpression m_commentStart;
    QRegularExpression m_commentEnd;
    bool m_hasMultiLineComment = false;
};
