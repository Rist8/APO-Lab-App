#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include "previewdialogbase.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMap>
#include <QVariant>
#include <QWidget>
#include <qcombobox.h>
#include <qspinbox.h>

class InputDialog : public PreviewDialogBase {
    Q_OBJECT

public:
    explicit InputDialog(QWidget *parent = nullptr);

    // Add input fields by label and widget
    void addInput(const QString &label, QSpinBox *spinBox);

    void addInput(const QString &label, QComboBox *comboBox);

    // Retrieve the value by label (returns QVariant)
    QVariant getValue(const QString &label) const;

    QCheckBox* getPreviewCheckBox() const { return previewCheckBox; }

private slots:
    void onPreviewToggled(bool);

private:
    QVBoxLayout *mainLayout;
    QFormLayout *formLayout;
    QDialogButtonBox *buttonBox;
    QCheckBox *previewCheckBox;

    // Store widgets with labels for easy value access
    QMap<QString, QWidget*> inputWidgets;
};

#endif // INPUTDIALOG_H
