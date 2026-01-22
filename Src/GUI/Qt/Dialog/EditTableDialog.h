#ifndef EDITTABLEDIALOG_H
#define EDITTABLEDIALOG_H

#include <QAbstractItemModel>
#include <QItemDelegate>
#include <QDialog>

class EditTableModel : public QAbstractItemModel //QAbstractTableModel
{
    Q_OBJECT

private:
    class XYValue {
    public:
        double m_Phys;
        double m_Raw;
    };
    QList<XYValue> m_Values;

public:
    EditTableModel(QString &par_TableString, QObject *arg_parent = nullptr);
    ~EditTableModel() Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE
    { Q_UNUSED(parent) return createIndex (row, column); }
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE
    { Q_UNUSED(index) return QModelIndex();}

    QVariant data(const QModelIndex &arg_index, int arg_role) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role) Q_DECL_OVERRIDE;
    QVariant headerData(int arg_section, Qt::Orientation arg_orientation, int arg_role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &arg_index) const Q_DECL_OVERRIDE;

    void ResizeTo(int par_Size);
    QString GetModifiedTableString();
};


class EditTableDelegateEditor : public QItemDelegate
{
    Q_OBJECT
public:
    explicit EditTableDelegateEditor(QObject *parent = nullptr);
    ~EditTableDelegateEditor();
protected:
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget * editor, const QModelIndex & index) const;
    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const;
    void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const;

signals:

public slots:
    void commitAndCloseEditor();

};


namespace Ui {
class EditTableDialog;
}

class EditTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditTableDialog(QString &par_TableString,
                             QWidget *parent = nullptr);
    ~EditTableDialog();
    QString GetModifiedTableString();

private slots:
    void on_SizeSinBox_valueChanged(int arg1);

private:
    Ui::EditTableDialog *ui;
    EditTableDelegateEditor *m_DelegateEditor;
    EditTableModel *m_Model;
};

#endif // EDITTABLEDIALOG_H
