#ifndef IMAGEOPERATION_H
#define IMAGEOPERATION_H

#include <QObject>
#include <QAction>
#include <QMenu>
#include <functional>

class ImageViewer;

class ImageOperation : public QObject {
    Q_OBJECT

public:
    enum ImageType {
        Grayscale = 0x1,
        Color     = 0x2,
        Binary    = 0x4,
        All       = Grayscale | Color | Binary,
        None      = 0x8
    };
    Q_DECLARE_FLAGS(ImageTypes, ImageType)

    ImageOperation(const QString &name,
                   ImageViewer *parentViewer,
                   QMenu *parentMenu,
                   ImageTypes supportedTypes,
                   std::function<void()> slotFunction,
                   bool checkable = false,
                   QKeySequence keyBind = QKeySequence());

    void updateActionState(ImageType currentImageType);
    QAction* getAction() const;

private:
    QString name;
    QAction *action;
    QMenu *menu;
    ImageTypes allowedTypes;
    std::function<void()> operationFunc;
    QKeySequence bind;
    ImageViewer *viewer;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ImageOperation::ImageTypes)

#endif // IMAGEOPERATION_H
