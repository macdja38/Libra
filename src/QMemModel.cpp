#include "QMemModel.hpp"

QMemModel::QMemModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

QVariant QMemModel::data(const QModelIndex &index, int role) const {
    
    //handle out of bounds
    if(index.row() >= mRows)
        return QVariant();
    if(index.column() >= mColumns)
        return QVariant();
    
    if(role == Qt::DisplayRole) {
        return QString("%1").arg((int)(mData[index.row() * 0x10 + index.column()]) & 0xFF, 2, 16, QChar('0')).toUpper();
    }
    return QVariant();
}

QVariant QMemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if(orientation == Qt::Horizontal) {
            return QString("%1").arg(section, 0, 16).toUpper();
        } else if(orientation == Qt::Vertical) {
            return QString("0x" + QString("%1").arg(section * 0x10, 4, 16, QChar('0')).toUpper());
        }
    }
    return QVariant();
}

bool QMemModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if(index.row() >= mRows)
        return false;
    if(index.column() >= mColumns)
        return false;
    
    if(role == Qt::EditRole) {
        mData[index.row() * 0x10 + index.column()] = value.toChar().toAscii();
    }
    return true;
}

void QMemModel::update() {
    emit dataChanged(this->createIndex(0,0), this->createIndex(mRows - 1, mColumns - 1));
}

void QMemModel::setItem(int row, int col, char data, bool bUpdate) {
    if(row >= mRows)
        return;
    if(col >= mColumns)
        return;
    
    mData[row*0x10 + col] = data;
    if(bUpdate)
        update();
}

void QMemModel::copyData(const unsigned char *data) {
    memcpy(mData, data, sizeof(mData));
}
