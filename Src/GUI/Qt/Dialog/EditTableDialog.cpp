#include "EditTableDialog.h"
#include "ui_EditTableDialog.h"

#include "PhysValueInput.h"

EditTableDialog::EditTableDialog(QString &par_TableString, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditTableDialog)
{
    ui->setupUi(this);

    m_DelegateEditor = new EditTableDelegateEditor;
    ui->ValuesTableView->setItemDelegate(m_DelegateEditor);

    m_Model = new EditTableModel(par_TableString);
    ui->ValuesTableView->setModel(m_Model);
    ui->SizeSinBox->setValue(m_Model->columnCount());
    ui->SizeSinBox->setMinimum(1);
    ui->SizeSinBox->setMaximum(100);
}

EditTableDialog::~EditTableDialog()
{
    delete ui;
    delete m_Model;
    delete m_DelegateEditor;
}

QString EditTableDialog::GetModifiedTableString()
{
    return m_Model->GetModifiedTableString();
}

EditTableModel::EditTableModel(QString &par_TableString, QObject *arg_parent)
{
    QStringList TableStrings = par_TableString.split(':');
    int Size = TableStrings.count();
    for (int x = 0; x < Size; x++) {
        QStringList ValuePairs = TableStrings.at(x).split('/');
        XYValue Value;
        if (ValuePairs.count() == 2) {
            bool Ok;
            Value.m_Phys = ValuePairs.at(0).toDouble(&Ok);
            if (!Ok) Value.m_Phys = 0.0;
            Value.m_Raw = ValuePairs.at(1).toDouble(&Ok);
            if (!Ok) Value.m_Raw = 0.0;
        } else {
            Value.m_Phys = 0.0;
            Value.m_Raw = 0.0;
        }
        m_Values.append(Value);
    }
}

EditTableModel::~EditTableModel()
{

}

int EditTableModel::rowCount(const QModelIndex &arg_parent) const
{
    return 2;
}

int EditTableModel::columnCount(const QModelIndex &arg_parent) const
{
    return m_Values.size();
}

QVariant EditTableModel::data(const QModelIndex &arg_index, int arg_role) const
{
    switch (arg_role) {
    case Qt::DisplayRole:
    {
        int Column = arg_index.column();
        if (arg_index.row()) {
            return QString().number(m_Values.at(Column).m_Raw);
        } else {
            return QString().number(m_Values.at(Column).m_Phys);
        }
        break;
    }
    case Qt::UserRole + 1:
    {
        int Column = arg_index.column();
        if (arg_index.row()) {
            return m_Values.at(Column).m_Raw;
        } else {
            return m_Values.at(Column).m_Phys;
        }
        break;
    }
    default:
        break;
    }
    return QVariant();
}

bool EditTableModel::setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role)
{
    switch (arg_role) {
    case Qt::UserRole + 1:
        int Column = arg_index.column();
        XYValue Value = m_Values.at(Column);
        if (arg_index.row()) {
            Value.m_Raw = arg_value.toDouble();
        } else {
            Value.m_Phys = arg_value.toDouble();
        }
        m_Values.replace(Column, Value);
        break;
    }
    return true;
}

QVariant EditTableModel::headerData(int arg_section, Qt::Orientation arg_orientation, int arg_role) const
{
    switch (arg_role) {
    case Qt::DisplayRole:
        if (arg_orientation == Qt::Horizontal) {
            return QString().number(arg_section);
        } else {
            if (arg_section == 0) {
                return QString("Phys");
            } else {
                return QString("Raw");
            }
        }
        break;
    default:
        break;
    }
    return QVariant();
}

Qt::ItemFlags EditTableModel::flags(const QModelIndex &arg_index) const
{
    return  Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void EditTableModel::ResizeTo(int par_Size)
{
    int OldSize = m_Values.size();
    if (OldSize > par_Size) {
        int First = par_Size;
        int Last = OldSize - 1;
        this->beginRemoveColumns(QModelIndex(), First, Last);
        for (int x = First; x <= Last; x++) m_Values.removeLast();
        this->endRemoveColumns();
    } else if (OldSize < par_Size) {
        int First = OldSize;
        int Last = par_Size - 1;
        this->beginInsertColumns(QModelIndex(), First, Last);
        XYValue Value;
        Value.m_Phys = 0.0;
        Value.m_Raw = 0.0;
        for (int x = First; x <= Last; x++) m_Values.append(Value);
        this->endInsertColumns();
    }
}

QString EditTableModel::GetModifiedTableString()
{
    QString Ret;
    for (int x = 0; x < m_Values.size(); x++) {
        XYValue Value = m_Values.at(x);
        if (x > 0) Ret.append(QString(":"));
        Ret.append(QString("%1").arg(Value.m_Phys));
        Ret.append(QString("/"));
        Ret.append(QString("%1").arg(Value.m_Raw));
    }
    return Ret;
}

void EditTableDialog::on_SizeSinBox_valueChanged(int arg1)
{
    if (m_Model != nullptr) {
        m_Model->ResizeTo(arg1);
    }
}


EditTableDelegateEditor::EditTableDelegateEditor(QObject *parent)
{

}

EditTableDelegateEditor::~EditTableDelegateEditor()
{

}

QWidget *EditTableDelegateEditor::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    PhysValueInput *editor = new PhysValueInput(parent);
    editor->SetDisplayRawValue(true);
    editor->SetDisplayPhysValue(false);
    connect(editor, SIGNAL(LeaveEditor()), this, SLOT(commitAndCloseEditor()));
    return editor;
}

void EditTableDelegateEditor::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    PhysValueInput *Edit = static_cast<PhysValueInput*>(editor);
    bool Ok;
    double Value = index.model()->data(index, Qt::UserRole + 1).toDouble(&Ok);
    if (Ok) {
        Edit->SetRawValue(Value);
    }
}

void EditTableDelegateEditor::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    PhysValueInput *Edit = static_cast<PhysValueInput*>(editor);
    bool Ok;
    double Value = Edit->GetDoubleRawValue(&Ok);
    if (Ok) {
        model->setData(index, Value, Qt::UserRole + 1);
    }
}

void EditTableDelegateEditor::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

void EditTableDelegateEditor::commitAndCloseEditor()
{
    PhysValueInput *editor = qobject_cast<PhysValueInput *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}
