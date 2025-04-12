#include "imageoperation.h"
#include "imageviewer.h"

// ==========================================================================
// Group 11: Operation Management & Abstraction
// ==========================================================================
// Constructor: Creates a managed image operation, linking it to a menu action.
// ==========================================================================
ImageOperation::ImageOperation(const QString &name,
                               ImageViewer *parentViewer,
                               QMenu *parentMenu,
                               ImageTypes supportedTypes,
                               std::function<void()> slotFunction,
                               bool checkable,
                               QKeySequence keyBind)
    : QObject(parentViewer), name(name), menu(parentMenu),
    allowedTypes(supportedTypes), operationFunc(slotFunction), viewer(parentViewer),
    bind(keyBind){

    action = new QAction(name, this);
    action->setCheckable(checkable);

    if(action){
        action->setShortcut(bind);
    }

    connect(action, &QAction::triggered, this, [this]() {
        if (operationFunc) operationFunc();
    });

    if (menu) menu->addAction(action);
}

// ==========================================================================
// Group 11: Operation Management & Abstraction
// ==========================================================================
// Enables/disables the associated QAction based on the current image type
// and the operation's supported types. Allows Grayscale ops on Binary images.
// ==========================================================================
void ImageOperation::updateActionState(ImageType currentImageType) {
    bool isEnabled = false;
    // Check if the operation supports the current image type directly
    if (allowedTypes & currentImageType) {
        isEnabled = true;
    }
    // Special case: Allow operations supporting Grayscale to also work on Binary images
    else if ((allowedTypes & Grayscale) && currentImageType == Binary) {
        isEnabled = true;
    }
    action->setEnabled(isEnabled);
}


// ==========================================================================
// Group 11: Operation Management & Abstraction
// ==========================================================================
// Getter for the QAction associated with this operation.
// ==========================================================================
QAction* ImageOperation::getAction() const {
    return action;
}
