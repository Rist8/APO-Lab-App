#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

// ClickableLabel.h
class ClickableLabel : public QLabel {
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget* parent = nullptr) : QLabel(parent) {}

signals:
    void mouseClicked(QMouseEvent* event);

protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit mouseClicked(event);
    }
};


#endif // CLICKABLELABEL_H
