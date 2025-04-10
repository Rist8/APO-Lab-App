#include "imageoperation.h"
#include "imageviewer.h"

ImageOperation::ImageOperation(const QString &name,
                               ImageViewer *parentViewer,
                               QMenu *parentMenu,
                               ImageTypes supportedTypes,
                               std::function<void()> slotFunction,
                               bool checkable)
    : QObject(parentViewer), name(name), menu(parentMenu), allowedTypes(supportedTypes), operationFunc(slotFunction), viewer(parentViewer) {

    action = new QAction(name, this);
    action->setCheckable(checkable);

    connect(action, &QAction::triggered, this, [this]() {
        if (operationFunc) operationFunc();
    });

    if (menu) menu->addAction(action);
}

void ImageOperation::updateActionState(ImageType currentImageType) {
    if ((allowedTypes & Grayscale) && currentImageType == Binary) {
        action->setEnabled(true);
    } else {
        action->setEnabled(allowedTypes & currentImageType);
    }
}

QAction* ImageOperation::getAction() const {
    return action;
}
