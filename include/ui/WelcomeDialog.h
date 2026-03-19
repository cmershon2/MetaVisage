#ifndef WELCOMEDIALOG_H
#define WELCOMEDIALOG_H

#include <QDialog>
#include <QStringList>

namespace MetaVisage {

class WelcomeDialog : public QDialog {
    Q_OBJECT

public:
    enum Result {
        NewProject,
        OpenProject,
        OpenRecent,
        Cancelled
    };

    explicit WelcomeDialog(QWidget *parent = nullptr);
    ~WelcomeDialog();

    Result GetResult() const { return result_; }
    QString GetSelectedProjectPath() const { return selectedPath_; }

    static QStringList GetRecentProjects();
    static void AddRecentProject(const QString& path);

private:
    void SetupUI();
    void OnNewProject();
    void OnOpenProject();
    void OnRecentProjectClicked(const QString& path);

    Result result_;
    QString selectedPath_;
};

} // namespace MetaVisage

#endif // WELCOMEDIALOG_H
