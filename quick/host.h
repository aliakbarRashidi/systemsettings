/***************************************************************************
 *                                                                         *
 *   Copyright 2014 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef HOST_H
#define HOST_H

#include "Category.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QQmlListProperty>

class HostPrivate;
class MenuProxyModel;

class Host : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *categoriesModel READ categoriesModel CONSTANT)
    Q_PROPERTY(QQmlListProperty<Category> categories READ categories CONSTANT)

public:
    Host(MenuProxyModel *model, QObject *parent = 0);
    virtual ~Host();

    QQmlListProperty<Category> categories();

public Q_SLOTS:
    QAbstractItemModel *categoriesModel();
    Q_INVOKABLE void categoryClicked(int ix);
    Q_INVOKABLE void categoryNameClicked(const QString &cat);
    Q_INVOKABLE void moduleClicked(int ix);

Q_SIGNALS:
    void categoryChanged();

private:
    HostPrivate *d;
};

#endif // HOST_H
