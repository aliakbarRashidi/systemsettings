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

#include "host.h"
#include "Category.h"
#include "MenuProxyModel.h"

#include <QDebug>

class HostPrivate
{
public:
    HostPrivate(Host *host)
        : q(host),
          categoriesModel(0) {
    }

    Host *q;

    QModelIndex currentCategory;
    MenuProxyModel *categoriesModel;
    QList<Category*> categories;
};

Host::Host(MenuProxyModel *model, QObject *parent) :
    QObject(parent),
    d(new HostPrivate(this))
{
    d->categoriesModel = model;

    categories();
}

Host::~Host()
{
    delete d;
}

QAbstractItemModel *Host::categoriesModel()
{
    return d->categoriesModel;
}

QQmlListProperty<Category> Host::categories()
{
//     return QQmlListProperty<ConfigCategory>(this, 0, ConfigModelPrivate::categories_append,
//                                             ConfigModelPrivate::categories_count,
//                                             ConfigModelPrivate::categories_at,
//                                             ConfigModelPrivate::categories_clear);
//
    if (!d->categories.count()) {
        const int n = d->categoriesModel->rowCount(d->currentCategory);
        qDebug() << "ROWS:" << n;
        for (int i = 0; i < n; i++) {
            QModelIndex index = d->categoriesModel->index(i, 0);
            Category *category = new Category(index, d->categoriesModel, this);
            d->categories.append(category);
            //QString c = d->categoriesModel->data(index, Qt::DisplayRole).toString();
            QString c = d->categoriesModel->data(index, Qt::DecorationRole).toString();
            qDebug() << " Cat from model: " << c << category->name();
        }

    }
    return QQmlListProperty<Category>(this, d->categories);
}

void Host::categoryClicked(int ix)
{
    qDebug() << "Category: " << ix;
    QModelIndex index = d->categoriesModel->index(ix, 0);
    QString c = d->categoriesModel->data(index, Qt::DisplayRole).toString();
    qDebug() << " Cat from model: " << c;
}

void Host::categoryNameClicked(const QString& cat)
{
    qDebug() << "Category: " << cat;
}

void Host::moduleClicked(int ix)
{
    qDebug() << "Module: " << ix;
}

#include "host.moc"
