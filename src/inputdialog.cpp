#include "inputdialog.h"
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>

InputDialog::InputDialog(QWidget *parent) : PreviewDialogBase(parent) {
    mainLayout = new QVBoxLayout(this);
    formLayout = new QFormLayout;
    previewCheckBox = new QCheckBox("Preview");

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(previewCheckBox);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(previewCheckBox, &QCheckBox::toggled, this, &InputDialog::onPreviewToggled);
}

void InputDialog::addInput(const QString &label, QSpinBox *spinBox) {
    formLayout->addRow(label, spinBox);
    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
    inputWidgets[label] = spinBox;
}

void InputDialog::addInput(const QString &label, QComboBox *comboBox) {
    formLayout->addRow(label, comboBox);
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewDialogBase::previewRequested);
    inputWidgets[label] = comboBox;
}

QVariant InputDialog::getValue(const QString &label) const {
    QWidget* widget = inputWidgets.value(label, nullptr);
    if (!widget) return QVariant();

    if (auto spin = qobject_cast<QSpinBox*>(widget)) return spin->value();
    if (auto combo = qobject_cast<QComboBox*>(widget)) return combo->currentText();
    if (auto lineEdit = qobject_cast<QLineEdit*>(widget)) return lineEdit->text();

    return QVariant(); // Unknown widget
}

void InputDialog::onPreviewToggled(bool) {
    emit previewRequested();
}
