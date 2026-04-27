#ifndef UNDOABLEACTION_H
#define UNDOABLEACTION_H

#include <string>

namespace MetaVisage {

class UndoableAction {
public:
    virtual ~UndoableAction() = default;
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    virtual std::string Description() const = 0;
};

} // namespace MetaVisage

#endif // UNDOABLEACTION_H
