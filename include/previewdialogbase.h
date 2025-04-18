#ifndef PREVIEWDIALOGBASE_H
#define PREVIEWDIALOGBASE_H

#include <qdialog.h>
class PreviewDialogBase : public QDialog {
    Q_OBJECT
public:
    PreviewDialogBase(QWidget* parent) : QDialog(parent) { }
signals:
    void previewRequested();
};

#endif // PREVIEWDIALOGBASE_H
