#include "core/UndoStack.h"
#include "utils/Logger.h"

namespace MetaVisage {

UndoStack::UndoStack(int maxSize)
    : currentIndex_(0), maxSize_(maxSize) {
}

void UndoStack::Push(std::unique_ptr<UndoableAction> action) {
    // Clear any redo actions (everything after currentIndex_)
    if (currentIndex_ < static_cast<int>(actions_.size())) {
        actions_.erase(actions_.begin() + currentIndex_, actions_.end());
    }

    actions_.push_back(std::move(action));
    currentIndex_ = static_cast<int>(actions_.size());

    // Enforce max size by removing oldest actions
    while (static_cast<int>(actions_.size()) > maxSize_) {
        actions_.erase(actions_.begin());
        currentIndex_--;
    }

    MV_LOG_DEBUG(QString("Undo stack: pushed '%1' (size=%2, index=%3)")
        .arg(actions_.back()->Description()).arg(actions_.size()).arg(currentIndex_));
}

bool UndoStack::CanUndo() const {
    return currentIndex_ > 0;
}

bool UndoStack::CanRedo() const {
    return currentIndex_ < static_cast<int>(actions_.size());
}

void UndoStack::Undo() {
    if (!CanUndo()) return;

    currentIndex_--;
    MV_LOG_INFO(QString("Undo: %1").arg(actions_[currentIndex_]->Description()));
    actions_[currentIndex_]->Undo();
}

void UndoStack::Redo() {
    if (!CanRedo()) return;

    MV_LOG_INFO(QString("Redo: %1").arg(actions_[currentIndex_]->Description()));
    actions_[currentIndex_]->Redo();
    currentIndex_++;
}

void UndoStack::Clear() {
    actions_.clear();
    currentIndex_ = 0;
    MV_LOG_DEBUG("Undo stack cleared");
}

QString UndoStack::UndoDescription() const {
    if (!CanUndo()) return "";
    return actions_[currentIndex_ - 1]->Description();
}

QString UndoStack::RedoDescription() const {
    if (!CanRedo()) return "";
    return actions_[currentIndex_]->Description();
}

} // namespace MetaVisage
