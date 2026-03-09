#include "CodeHighlighter.h"
#include <QFileInfo>

CodeHighlighter::CodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
}

void CodeHighlighter::setupFormats()
{
    m_keywordFmt.setForeground(QColor("#d73a49"));
    m_keywordFmt.setFontWeight(QFont::Bold);

    m_typeFmt.setForeground(QColor("#6f42c1"));

    m_stringFmt.setForeground(QColor("#032f62"));

    m_commentFmt.setForeground(QColor("#6a737d"));
    m_commentFmt.setFontItalic(true);

    m_numberFmt.setForeground(QColor("#005cc5"));

    m_functionFmt.setForeground(QColor("#6f42c1"));

    m_preprocessorFmt.setForeground(QColor("#22863a"));

    m_operatorFmt.setForeground(QColor("#d73a49"));
}

void CodeHighlighter::applyThemeColors(const QColor &keyword, const QColor &type, const QColor &string,
                                        const QColor &comment, const QColor &number, const QColor &function,
                                        const QColor &preprocessor, const QColor &operatorColor)
{
    m_keywordFmt.setForeground(keyword);
    m_typeFmt.setForeground(type);
    m_stringFmt.setForeground(string);
    m_commentFmt.setForeground(comment);
    m_numberFmt.setForeground(number);
    m_functionFmt.setForeground(function);
    m_preprocessorFmt.setForeground(preprocessor);
    m_operatorFmt.setForeground(operatorColor);

    // Rebuild rules with new colors and rehighlight
    m_rules.clear();
    m_hasMultiLineComment = false;
    buildRules();
    rehighlight();
}

void CodeHighlighter::setLanguage(Language lang)
{
    if (m_language == lang) return;
    m_language = lang;
    m_rules.clear();
    m_hasMultiLineComment = false;
    buildRules();
    rehighlight();
}

void CodeHighlighter::buildRules()
{
    switch (m_language) {
    case Cpp: case CSharp: addCppRules(); break;
    case Python: addPythonRules(); break;
    case JavaScript: case TypeScript: addJavaScriptRules(); break;
    case Java: case Kotlin: addJavaRules(); break;
    case Rust: addRustRules(); break;
    case Go: addGoRules(); break;
    case Shell: addShellRules(); break;
    case SQL: addSqlRules(); break;
    case HTML: case Xml: addHtmlRules(); break;
    case CSS: addCssRules(); break;
    case Ruby: case Perl: addRubyRules(); break;
    case Lua: addLuaRules(); break;
    case Json: addJsonRules(); break;
    case Yaml: case Toml: case Ini: addYamlRules(); break;
    case CMake: case Makefile: addCMakeRules(); break;
    case PHP: addJavaScriptRules(); break;
    case Haskell: case Swift: addGenericRules(); break;
    case None: break;
    }
}

void CodeHighlighter::addCppRules()
{
    // Keywords
    QStringList keywords = {
        "auto","break","case","catch","class","const","constexpr","continue",
        "default","delete","do","else","enum","explicit","export","extern",
        "false","for","friend","goto","if","inline","mutable","namespace",
        "new","noexcept","nullptr","operator","private","protected","public",
        "register","return","sizeof","static","static_assert","static_cast",
        "struct","switch","template","this","throw","true","try","typedef",
        "typeid","typename","union","using","virtual","void","volatile","while",
        "override","final","concept","requires","co_await","co_return","co_yield",
        "consteval","constinit","char8_t","module","import"
    };
    QString kwPattern = "\\b(" + keywords.join('|') + ")\\b";
    m_rules.append({QRegularExpression(kwPattern), m_keywordFmt});

    // Types
    QStringList types = {
        "int","char","float","double","bool","long","short","unsigned","signed",
        "size_t","int8_t","int16_t","int32_t","int64_t","uint8_t","uint16_t",
        "uint32_t","uint64_t","string","wstring","vector","map","set","list",
        "array","pair","tuple","unique_ptr","shared_ptr","weak_ptr","optional",
        "variant","any","QString","QVector","QList","QMap","QSet","QHash",
        "QWidget","QObject","QMainWindow","QApplication","qint64","qsizetype"
    };
    QString typePattern = "\\b(" + types.join('|') + ")\\b";
    m_rules.append({QRegularExpression(typePattern), m_typeFmt});

    // Preprocessor
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*#[a-zA-Z_]+")), m_preprocessorFmt});

    // Numbers
    m_rules.append({QRegularExpression(QStringLiteral("\\b(0[xX][0-9a-fA-F]+|0[bB][01]+|\\d+\\.?\\d*([eE][+-]?\\d+)?[fFlLuU]*)\\b")), m_numberFmt});

    // Strings
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("R\"\\([^)]*\\)\"")), m_stringFmt});

    // Functions
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()")), m_functionFmt});

    // Single-line comment
    m_rules.append({QRegularExpression(QStringLiteral("//[^\n]*")), m_commentFmt});

    // Multi-line comment
    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeHighlighter::addPythonRules()
{
    QStringList keywords = {
        "and","as","assert","async","await","break","class","continue","def",
        "del","elif","else","except","False","finally","for","from","global",
        "if","import","in","is","lambda","None","nonlocal","not","or","pass",
        "raise","return","True","try","while","with","yield"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    QStringList builtins = {
        "int","float","str","bool","list","dict","tuple","set","frozenset",
        "bytes","bytearray","range","print","len","type","isinstance","input",
        "open","map","filter","zip","enumerate","sorted","reversed","super",
        "property","classmethod","staticmethod","Exception","ValueError","TypeError"
    };
    m_rules.append({QRegularExpression("\\b(" + builtins.join('|') + ")\\b"), m_typeFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("@[a-zA-Z_][a-zA-Z0-9_]*")), m_preprocessorFmt});

    // Strings (single and double, including triple-quoted handled in highlightBlock)
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("[fFrRbBuU]?\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("[fFrRbBuU]?'([^'\\\\]|\\\\.)*'")), m_stringFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()")), m_functionFmt});
    m_rules.append({QRegularExpression(QStringLiteral("#[^\n]*")), m_commentFmt});

    // Triple-quote multi-line strings
    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("(\"\"\"|\\'\\'\\')"));
    m_commentEnd = QRegularExpression(QStringLiteral("(\"\"\"|\\'\\'\\')"));
}

void CodeHighlighter::addJavaScriptRules()
{
    QStringList keywords = {
        "async","await","break","case","catch","class","const","continue",
        "debugger","default","delete","do","else","export","extends","false",
        "finally","for","function","if","import","in","instanceof","let",
        "new","null","of","return","static","super","switch","this","throw",
        "true","try","typeof","undefined","var","void","while","with","yield"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    QStringList types = {
        "Array","Boolean","Date","Error","Function","JSON","Map","Math",
        "Number","Object","Promise","Proxy","RegExp","Set","String","Symbol",
        "WeakMap","WeakSet","console","document","window","global","process"
    };
    m_rules.append({QRegularExpression("\\b(" + types.join('|') + ")\\b"), m_typeFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("`[^`]*`")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_$][a-zA-Z0-9_$]*)\\s*(?=\\()")), m_functionFmt});
    m_rules.append({QRegularExpression(QStringLiteral("//[^\n]*")), m_commentFmt});

    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeHighlighter::addJavaRules()
{
    QStringList keywords = {
        "abstract","assert","break","case","catch","class","continue","default",
        "do","else","enum","extends","false","final","finally","for","if",
        "implements","import","instanceof","interface","native","new","null",
        "package","private","protected","public","return","static","strictfp",
        "super","switch","synchronized","this","throw","throws","transient",
        "true","try","void","volatile","while","record","sealed","permits","var"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    QStringList types = {
        "boolean","byte","char","double","float","int","long","short",
        "String","Integer","Long","Double","Float","Boolean","Character",
        "Object","List","Map","Set","ArrayList","HashMap","HashSet",
        "Optional","Stream","Consumer","Function","Predicate","Supplier"
    };
    m_rules.append({QRegularExpression("\\b(" + types.join('|') + ")\\b"), m_typeFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?[fFdDlL]?\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("@[a-zA-Z_][a-zA-Z0-9_]*")), m_preprocessorFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()")), m_functionFmt});
    m_rules.append({QRegularExpression(QStringLiteral("//[^\n]*")), m_commentFmt});

    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeHighlighter::addRustRules()
{
    QStringList keywords = {
        "as","async","await","break","const","continue","crate","dyn","else",
        "enum","extern","false","fn","for","if","impl","in","let","loop",
        "match","mod","move","mut","pub","ref","return","self","Self","static",
        "struct","super","trait","true","type","union","unsafe","use","where","while"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    QStringList types = {
        "i8","i16","i32","i64","i128","isize","u8","u16","u32","u64","u128",
        "usize","f32","f64","bool","char","str","String","Vec","Box","Rc","Arc",
        "Option","Result","Some","None","Ok","Err","HashMap","HashSet","BTreeMap"
    };
    m_rules.append({QRegularExpression("\\b(" + types.join('|') + ")\\b"), m_typeFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?(_[iu]\\d+|_[fu]\\d+|_usize|_isize)?\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("#!?\\[.*\\]")), m_preprocessorFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()")), m_functionFmt});
    m_rules.append({QRegularExpression(QStringLiteral("//[^\n]*")), m_commentFmt});

    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeHighlighter::addGoRules()
{
    QStringList keywords = {
        "break","case","chan","const","continue","default","defer","else",
        "fallthrough","for","func","go","goto","if","import","interface",
        "map","package","range","return","select","struct","switch","type","var",
        "true","false","nil","iota"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    QStringList types = {
        "bool","byte","complex64","complex128","error","float32","float64",
        "int","int8","int16","int32","int64","rune","string","uint","uint8",
        "uint16","uint32","uint64","uintptr"
    };
    m_rules.append({QRegularExpression("\\b(" + types.join('|') + ")\\b"), m_typeFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("`[^`]*`")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()")), m_functionFmt});
    m_rules.append({QRegularExpression(QStringLiteral("//[^\n]*")), m_commentFmt});

    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeHighlighter::addShellRules()
{
    QStringList keywords = {
        "if","then","else","elif","fi","for","while","do","done","case","esac",
        "in","function","return","exit","local","export","source","alias","unalias",
        "set","unset","shift","break","continue","readonly","declare","typeset",
        "select","until","trap","eval","exec"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    QStringList builtins = {
        "echo","printf","cd","pwd","ls","cp","mv","rm","mkdir","rmdir",
        "cat","grep","sed","awk","find","sort","head","tail","wc","chmod",
        "chown","test","read","true","false"
    };
    m_rules.append({QRegularExpression("\\b(" + builtins.join('|') + ")\\b"), m_typeFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\$[a-zA-Z_][a-zA-Z0-9_]*")), m_preprocessorFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\$\\{[^}]+\\}")), m_preprocessorFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'[^']*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("#[^\n]*")), m_commentFmt});
}

void CodeHighlighter::addSqlRules()
{
    QStringList keywords = {
        "SELECT","FROM","WHERE","INSERT","INTO","VALUES","UPDATE","SET","DELETE",
        "CREATE","DROP","ALTER","TABLE","INDEX","VIEW","DATABASE","GRANT","REVOKE",
        "JOIN","INNER","LEFT","RIGHT","OUTER","FULL","CROSS","ON","AND","OR","NOT",
        "IN","BETWEEN","LIKE","IS","NULL","AS","ORDER","BY","GROUP","HAVING","LIMIT",
        "OFFSET","UNION","ALL","DISTINCT","EXISTS","CASE","WHEN","THEN","ELSE","END",
        "BEGIN","COMMIT","ROLLBACK","TRANSACTION","PRIMARY","KEY","FOREIGN","REFERENCES",
        "CONSTRAINT","DEFAULT","CHECK","UNIQUE","AUTO_INCREMENT","CASCADE",
        "select","from","where","insert","into","values","update","set","delete",
        "create","drop","alter","table","join","inner","left","right","on","and","or",
        "not","in","between","like","is","null","as","order","by","group","having",
        "limit","offset","union","distinct","exists","case","when","then","else","end"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    QStringList types = {
        "INT","INTEGER","BIGINT","SMALLINT","TINYINT","FLOAT","DOUBLE","DECIMAL",
        "NUMERIC","VARCHAR","CHAR","TEXT","BLOB","DATE","DATETIME","TIMESTAMP",
        "BOOLEAN","SERIAL","UUID"
    };
    m_rules.append({QRegularExpression("\\b(" + types.join('|') + ")\\b", QRegularExpression::CaseInsensitiveOption), m_typeFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("--[^\n]*")), m_commentFmt});

    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeHighlighter::addHtmlRules()
{
    // Tags
    m_rules.append({QRegularExpression(QStringLiteral("</?[a-zA-Z][a-zA-Z0-9]*")), m_keywordFmt});
    m_rules.append({QRegularExpression(QStringLiteral("/?>|/>")), m_keywordFmt});
    // Attributes
    m_rules.append({QRegularExpression(QStringLiteral("\\b[a-zA-Z-]+(?=\\s*=)")), m_typeFmt});
    // Strings
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    // Entities
    m_rules.append({QRegularExpression(QStringLiteral("&[a-zA-Z]+;")), m_numberFmt});
    // Comments
    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("<!--"));
    m_commentEnd = QRegularExpression(QStringLiteral("-->"));
}

void CodeHighlighter::addCssRules()
{
    // Properties
    m_rules.append({QRegularExpression(QStringLiteral("[a-z-]+(?=\\s*:)")), m_keywordFmt});
    // Selectors
    m_rules.append({QRegularExpression(QStringLiteral("[.#][a-zA-Z_][a-zA-Z0-9_-]*")), m_typeFmt});
    // Values
    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*(px|em|rem|%|vh|vw|pt|cm|mm|in)?\\b")), m_numberFmt});
    // Colors
    m_rules.append({QRegularExpression(QStringLiteral("#[0-9a-fA-F]{3,8}\\b")), m_numberFmt});
    // Strings
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    // Functions
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z-]+)\\s*(?=\\()")), m_functionFmt});
    // Comments
    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
}

void CodeHighlighter::addRubyRules()
{
    QStringList keywords = {
        "alias","and","begin","break","case","class","def","defined","do",
        "else","elsif","end","ensure","false","for","if","in","module",
        "next","nil","not","or","redo","rescue","retry","return","self",
        "super","then","true","undef","unless","until","when","while","yield",
        "require","require_relative","include","extend","attr_reader","attr_writer","attr_accessor"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    m_rules.append({QRegularExpression(QStringLiteral(":[a-zA-Z_][a-zA-Z0-9_]*")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("@[a-zA-Z_][a-zA-Z0-9_]*")), m_preprocessorFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\$[a-zA-Z_][a-zA-Z0-9_]*")), m_preprocessorFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("#[^\n]*")), m_commentFmt});

    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("^=begin"));
    m_commentEnd = QRegularExpression(QStringLiteral("^=end"));
}

void CodeHighlighter::addLuaRules()
{
    QStringList keywords = {
        "and","break","do","else","elseif","end","false","for","function",
        "goto","if","in","local","nil","not","or","repeat","return","then",
        "true","until","while"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b"), m_keywordFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()")), m_functionFmt});
    m_rules.append({QRegularExpression(QStringLiteral("--[^\n]*")), m_commentFmt});

    m_hasMultiLineComment = true;
    m_commentStart = QRegularExpression(QStringLiteral("--\\[\\["));
    m_commentEnd = QRegularExpression(QStringLiteral("\\]\\]"));
}

void CodeHighlighter::addJsonRules()
{
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"\\s*(?=:)")), m_keywordFmt});
    m_rules.append({QRegularExpression(QStringLiteral(":\\s*\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b(true|false|null)\\b")), m_keywordFmt});
    m_rules.append({QRegularExpression(QStringLiteral("-?\\b\\d+\\.?\\d*([eE][+-]?\\d+)?\\b")), m_numberFmt});
}

void CodeHighlighter::addYamlRules()
{
    m_rules.append({QRegularExpression(QStringLiteral("^[a-zA-Z_][a-zA-Z0-9_.-]*(?=\\s*:)")), m_keywordFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b(true|false|yes|no|null|~)\\b"), QRegularExpression::CaseInsensitiveOption), m_typeFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'[^']*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("#[^\n]*")), m_commentFmt});
}

void CodeHighlighter::addCMakeRules()
{
    QStringList keywords = {
        "cmake_minimum_required","project","add_executable","add_library",
        "target_link_libraries","find_package","include_directories",
        "set","option","if","else","elseif","endif","foreach","endforeach",
        "while","endwhile","function","endfunction","macro","endmacro",
        "message","install","add_subdirectory","include","configure_file",
        "target_include_directories","target_compile_definitions",
        "target_compile_options","add_custom_command","add_custom_target"
    };
    m_rules.append({QRegularExpression("\\b(" + keywords.join('|') + ")\\b", QRegularExpression::CaseInsensitiveOption), m_keywordFmt});

    m_rules.append({QRegularExpression(QStringLiteral("\\$\\{[^}]+\\}")), m_preprocessorFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b(ON|OFF|TRUE|FALSE|REQUIRED|PRIVATE|PUBLIC|INTERFACE)\\b")), m_typeFmt});
    m_rules.append({QRegularExpression(QStringLiteral("#[^\n]*")), m_commentFmt});
}

void CodeHighlighter::addGenericRules()
{
    m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*([eE][+-]?\\d+)?\\b")), m_numberFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFmt});
    m_rules.append({QRegularExpression(QStringLiteral("//[^\n]*")), m_commentFmt});
    m_rules.append({QRegularExpression(QStringLiteral("#[^\n]*")), m_commentFmt});
    m_rules.append({QRegularExpression(QStringLiteral("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()")), m_functionFmt});
}

void CodeHighlighter::highlightBlock(const QString &text)
{
    // Apply inline rules
    for (const auto &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Multi-line comment/string handling
    if (!m_hasMultiLineComment) {
        setCurrentBlockState(0);
        return;
    }

    int prevState = previousBlockState();
    if (prevState == -1) prevState = 0;

    int startIndex = 0;
    if (prevState != 1) {
        auto m = m_commentStart.match(text);
        startIndex = m.hasMatch() ? m.capturedStart() : -1;
    }

    while (startIndex >= 0) {
        int searchFrom = (prevState == 1) ? 0 : startIndex + 1;
        auto endMatch = m_commentEnd.match(text, searchFrom);
        int commentLength;

        if (endMatch.hasMatch()) {
            commentLength = endMatch.capturedEnd() - startIndex;
            setCurrentBlockState(0);
        } else {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }

        setFormat(startIndex, commentLength, m_commentFmt);

        if (currentBlockState() == 1) break;

        auto nextStart = m_commentStart.match(text, startIndex + commentLength);
        startIndex = nextStart.hasMatch() ? nextStart.capturedStart() : -1;
    }

    if (prevState == 1 && startIndex < 0) {
        // Entire block is inside comment
        setFormat(0, text.length(), m_commentFmt);
        auto endMatch = m_commentEnd.match(text);
        if (endMatch.hasMatch()) {
            setFormat(0, endMatch.capturedEnd(), m_commentFmt);
            setCurrentBlockState(0);
        } else {
            setCurrentBlockState(1);
        }
    }
}

CodeHighlighter::Language CodeHighlighter::detectLanguage(const QString &filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    QString base = QFileInfo(filePath).fileName().toLower();

    if (ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" ||
        ext == "h" || ext == "hpp" || ext == "hxx" || ext == "hh")
        return Cpp;
    if (ext == "cs") return CSharp;
    if (ext == "py" || ext == "pyw" || ext == "pyi") return Python;
    if (ext == "js" || ext == "mjs" || ext == "cjs") return JavaScript;
    if (ext == "ts" || ext == "tsx") return TypeScript;
    if (ext == "jsx") return JavaScript;
    if (ext == "java") return Java;
    if (ext == "kt" || ext == "kts") return Kotlin;
    if (ext == "rs") return Rust;
    if (ext == "go") return Go;
    if (ext == "sh" || ext == "bash" || ext == "zsh") return Shell;
    if (ext == "sql") return SQL;
    if (ext == "html" || ext == "htm" || ext == "xhtml") return HTML;
    if (ext == "xml" || ext == "svg" || ext == "xsl") return Xml;
    if (ext == "css" || ext == "scss" || ext == "less") return CSS;
    if (ext == "rb" || ext == "rake") return Ruby;
    if (ext == "php") return PHP;
    if (ext == "lua") return Lua;
    if (ext == "pl" || ext == "pm") return Perl;
    if (ext == "hs" || ext == "lhs") return Haskell;
    if (ext == "swift") return Swift;
    if (ext == "json") return Json;
    if (ext == "yaml" || ext == "yml") return Yaml;
    if (ext == "toml") return Toml;
    if (ext == "ini" || ext == "cfg" || ext == "conf") return Ini;
    if (base == "cmakelists.txt" || ext == "cmake") return CMake;
    if (base == "makefile" || base == "gnumakefile" || ext == "mk") return Makefile;
    if (base == "dockerfile") return Shell;
    if (base == ".bashrc" || base == ".zshrc" || base == ".profile") return Shell;

    return None;
}

QString CodeHighlighter::languageName(Language lang)
{
    switch (lang) {
    case None: return "Plain Text";
    case Cpp: return "C/C++";
    case CSharp: return "C#";
    case Python: return "Python";
    case JavaScript: return "JavaScript";
    case TypeScript: return "TypeScript";
    case Java: return "Java";
    case Kotlin: return "Kotlin";
    case Rust: return "Rust";
    case Go: return "Go";
    case Shell: return "Shell";
    case SQL: return "SQL";
    case HTML: return "HTML";
    case Xml: return "XML";
    case CSS: return "CSS";
    case Ruby: return "Ruby";
    case PHP: return "PHP";
    case Lua: return "Lua";
    case Perl: return "Perl";
    case Haskell: return "Haskell";
    case Swift: return "Swift";
    case Json: return "JSON";
    case Yaml: return "YAML";
    case Toml: return "TOML";
    case Ini: return "INI";
    case CMake: return "CMake";
    case Makefile: return "Makefile";
    }
    return "Plain Text";
}
