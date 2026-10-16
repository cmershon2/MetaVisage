#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "core/UndoableAction.h"
#include <memory>
#include <string>
#include <vector>

namespace MetaVisage {

class UndoStack {
public:
    explicit UndoStack(int maxSize = 100);

    void Push(std::unique_ptr<UndoableAction> action);
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();
    void Clear();

    std::string UndoDescription() const;
    std::string RedoDescription() const;
    int Size() const { return static_cast<int>(actions_.size()); }

private:
    std::vector<std::unique_ptr<UndoableAction>> actions_;
    int currentIndex_; // Points past the last executed action
    int maxSize_;
};

} // namespace MetaVisage

#endif // UNDOSTACK_H
