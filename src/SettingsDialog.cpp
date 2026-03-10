#include "SettingsDialog.h"
#include "EditorTheme.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(const AppSettings &settings, QWidget *parent)
    : QDialog(parent), m_original(settings)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(450, 450);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(createEditorPage(), tr("Editor"));
    tabs->addTab(createAppearancePage(), tr("Appearance"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);
}

QWidget *SettingsDialog::createEditorPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    // Font
    auto *fontGroup = new QGroupBox(tr("Editor Font"));
    auto *fontLayout = new QFormLayout(fontGroup);

    m_editorFontCombo = new QFontComboBox;
    m_editorFontCombo->setCurrentFont(m_original.editorFont);
    fontLayout->addRow(tr("Font:"), m_editorFontCombo);

    m_editorFontSize = new QSpinBox;
    m_editorFontSize->setRange(6, 72);
    m_editorFontSize->setValue(m_original.editorFont.pointSize());
    m_editorFontSize->setSuffix(" pt");
    fontLayout->addRow(tr("Size:"), m_editorFontSize);

    layout->addWidget(fontGroup);

    // Behavior
    auto *behaviorGroup = new QGroupBox(tr("Behavior"));
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);

    m_syntaxHighlighting = new QCheckBox(tr("Syntax Highlighting"));
    m_syntaxHighlighting->setChecked(m_original.syntaxHighlighting);
    behaviorLayout->addWidget(m_syntaxHighlighting);

    m_lineNumbers = new QCheckBox(tr("Show Line Numbers"));
    m_lineNumbers->setChecked(m_original.lineNumbers);
    behaviorLayout->addWidget(m_lineNumbers);

    m_lineWrap = new QCheckBox(tr("Word Wrap"));
    m_lineWrap->setChecked(m_original.lineWrap);
    behaviorLayout->addWidget(m_lineWrap);

    m_highlightCurrentLine = new QCheckBox(tr("Highlight Current Line"));
    m_highlightCurrentLine->setChecked(m_original.highlightCurrentLine);
    behaviorLayout->addWidget(m_highlightCurrentLine);

    m_showWhitespace = new QCheckBox(tr("Show Whitespace"));
    m_showWhitespace->setChecked(m_original.showWhitespace);
    behaviorLayout->addWidget(m_showWhitespace);

    auto *tabLayout = new QFormLayout;
    m_tabWidth = new QSpinBox;
    m_tabWidth->setRange(1, 16);
    m_tabWidth->setValue(m_original.tabWidth);
    tabLayout->addRow(tr("Tab Width:"), m_tabWidth);
    behaviorLayout->addLayout(tabLayout);

    m_autoIndent = new QCheckBox(tr("Auto Indent"));
    m_autoIndent->setChecked(m_original.autoIndent);
    behaviorLayout->addWidget(m_autoIndent);

    m_bracketMatching = new QCheckBox(tr("Bracket Matching"));
    m_bracketMatching->setChecked(m_original.bracketMatching);
    behaviorLayout->addWidget(m_bracketMatching);

    m_autoSave = new QCheckBox(tr("Auto Save"));
    m_autoSave->setChecked(m_original.autoSave);
    behaviorLayout->addWidget(m_autoSave);

    auto *autoSaveIntervalLayout = new QFormLayout;
    m_autoSaveInterval = new QSpinBox;
    m_autoSaveInterval->setRange(5, 600);
    m_autoSaveInterval->setValue(m_original.autoSaveInterval);
    m_autoSaveInterval->setSuffix(tr(" s"));
    m_autoSaveInterval->setEnabled(m_original.autoSave);
    autoSaveIntervalLayout->addRow(tr("Interval:"), m_autoSaveInterval);
    behaviorLayout->addLayout(autoSaveIntervalLayout);
    connect(m_autoSave, &QCheckBox::toggled, m_autoSaveInterval, &QSpinBox::setEnabled);

    m_minimap = new QCheckBox(tr("Show Minimap"));
    m_minimap->setChecked(m_original.minimap);
    behaviorLayout->addWidget(m_minimap);

    layout->addWidget(behaviorGroup);
    layout->addStretch();

    return page;
}

QWidget *SettingsDialog::createAppearancePage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    // Theme
    auto *themeGroup = new QGroupBox(tr("Color Theme"));
    auto *themeLayout = new QFormLayout(themeGroup);

    m_themeCombo = new QComboBox;
    m_themeCombo->addItems(EditorTheme::themeNames());
    m_themeCombo->setCurrentText(m_original.themeName);
    themeLayout->addRow(tr("Theme:"), m_themeCombo);

    layout->addWidget(themeGroup);

    // GUI Font
    auto *fontGroup = new QGroupBox(tr("GUI Font"));
    auto *fontLayout = new QFormLayout(fontGroup);

    m_guiFontCombo = new QFontComboBox;
    m_guiFontCombo->setCurrentFont(m_original.guiFont);
    fontLayout->addRow(tr("Font:"), m_guiFontCombo);

    m_guiFontSize = new QSpinBox;
    m_guiFontSize->setRange(6, 72);
    m_guiFontSize->setValue(m_original.guiFont.pointSize() > 0 ? m_original.guiFont.pointSize() : 10);
    m_guiFontSize->setSuffix(" pt");
    fontLayout->addRow(tr("Size:"), m_guiFontSize);

    layout->addWidget(fontGroup);
    layout->addStretch();

    return page;
}

AppSettings SettingsDialog::settings() const
{
    AppSettings s;

    QFont ef = m_editorFontCombo->currentFont();
    ef.setPointSize(m_editorFontSize->value());
    s.editorFont = ef;

    QFont gf = m_guiFontCombo->currentFont();
    gf.setPointSize(m_guiFontSize->value());
    s.guiFont = gf;

    s.syntaxHighlighting = m_syntaxHighlighting->isChecked();
    s.lineNumbers = m_lineNumbers->isChecked();
    s.lineWrap = m_lineWrap->isChecked();
    s.highlightCurrentLine = m_highlightCurrentLine->isChecked();
    s.showWhitespace = m_showWhitespace->isChecked();
    s.tabWidth = m_tabWidth->value();
    s.themeName = m_themeCombo->currentText();
    s.autoIndent = m_autoIndent->isChecked();
    s.bracketMatching = m_bracketMatching->isChecked();
    s.autoSave = m_autoSave->isChecked();
    s.autoSaveInterval = m_autoSaveInterval->value();
    s.minimap = m_minimap->isChecked();

    return s;
}
