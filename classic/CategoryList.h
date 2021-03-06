/*
  Copyright (c) 2000,2001 Matthias Elter <elter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef CATEGORYLIST_H
#define CATEGORYLIST_H

#include <QWidget>

class QModelIndex;
class QAbstractItemModel;


class CategoryList : public QWidget
{
    Q_OBJECT

public:
    explicit CategoryList( QWidget *parent, QAbstractItemModel *model );
    ~CategoryList() override;

    void changeModule( const QModelIndex &newItem);

Q_SIGNALS:
    void moduleSelected( const QModelIndex &itemSelected );

private Q_SLOTS:
    void slotModuleLinkClicked( const QUrl& moduleName );

private:
    void updatePixmap();

private:
    class Private;
    Private *const d;
};

#endif
