/*
 Copyright (c) 2000,2001 Matthias Elter <elter@kde.org>
 Copyright (c) 2009 Ben Cooksley <ben@eclipse.endoftheinternet.org>

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

#include "CategoryList.h"

#include "MenuItem.h"

#include <QFile>
#include <QModelIndex>
#include <QTextStream>

#include <KDebug>
#include <KLocale>
#include <KCursor>
#include <KHTMLPart>
#include <KApplication>
#include <KCModuleInfo>
#include <KStandardDirs>
#include <KGlobalSettings>

static const char kcc_infotext[]= I18N_NOOP("KDE Control Center");
static const char title_infotext[]= I18N_NOOP("Configure your system");
static const char intro_infotext[]= I18N_NOOP("Welcome to the \"KDE Control Center\", "
    "a central place to configure your computer system.");

class CategoryList::Private {
public:
    Private() {}

    KHTMLPart * categoryView;
    QModelIndex categoryMenu;
    QAbstractItemModel * itemModel;
    QMap<QString, QModelIndex> itemMap;
};

CategoryList::CategoryList( QWidget *parent, QAbstractItemModel *model )
    : KHBox(parent), d( new Private() )
{
    setMinimumSize( 400, 400 );
    d->itemModel = model;

    // set what's this help
    this->setWhatsThis( i18n( intro_infotext ) );
    d->categoryView = new KHTMLPart( this );
    d->categoryView->widget()->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    connect( d->categoryView->browserExtension(),
             SIGNAL( openUrlRequest( const KUrl&,
                                     const KParts::OpenUrlArguments&,
                                     const KParts::BrowserArguments& ) ),
             this, SLOT(slotModuleLinkClicked( const KUrl& ) ) );
}

CategoryList::~CategoryList()
{
    delete d;
}

void CategoryList::updatePixmap()
{
    QString content;
    QString moduleName;
    KIconLoader * iconL = KIconLoader::global();
    d->itemMap.clear();

    QString templatePath = KStandardDirs::locate( "data", "kcontrol4/classic/main.html" );
    QFile templateFile( templatePath );
    templateFile.open( QIODevice::ReadOnly );
    QTextStream templateText( &templateFile );
    QString templateString = templateText.readAll();
    templateString = templateString.arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage.css" ) );
    if ( kapp->layoutDirection() == Qt::RightToLeft ) {
        templateString = templateString.arg( "@import \"%1\";" ).arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage_rtl.css" ) );
    } else {
        templateString = templateString.arg( QString() );
    }
    templateString = templateString.arg( i18n( kcc_infotext ) );
    templateString = templateString.arg( i18n( title_infotext ) );
    templateString = templateString.arg( i18n( intro_infotext ) );
    if ( d->categoryMenu.isValid() ) {
        moduleName = d->itemModel->data( d->categoryMenu, Qt::UserRole ).value<MenuItem*>()->service()->name();
    }
    content += "<div id=\"tableTitle\">" + moduleName + "</div>";
    content += "<table class=\"kc_table\">\n";
    for( int done = 0;  d->itemModel->rowCount( d->categoryMenu ) > done; done = 1 + done ) {
        QModelIndex childIndex = d->itemModel->index( done, 0, d->categoryMenu );
        MenuItem *childItem = d->itemModel->data( childIndex, Qt::UserRole ).value<MenuItem*>();
        content += "<tr><td class=\"kc_leftcol\"><img src=\"%1\"></td><td class=\"kc_middlecol\">";
        QString szName = childItem->service()->name();
        QString szComment = childItem->service()->comment();
        content += "<a href=\"%2\">" + szName + "</a></td><td class=\"kc_rightcol\">" + szComment;
        QString linkURL( "kcm://" + childItem->item().fileName() );
        KUrl link( linkURL );
        content = content.arg( iconL->iconPath(childItem->service()->icon(), - KIconLoader::SizeSmallMedium ) );
        content = content.arg( link.url() );
        d->itemMap.insert( linkURL, childIndex );
        content += "</td></tr>\n";
    }
    content += "</table>";
    d->categoryView->begin( KUrl( templatePath ) );
    d->categoryView->write( templateString.arg( content ) );
    d->categoryView->end();
}

void CategoryList::changeModule( QModelIndex newItem )
{
    d->categoryMenu = newItem;
    updatePixmap();
}

void CategoryList::slotModuleLinkClicked( const KUrl& moduleName ) 
{
    QModelIndex module = d->itemMap.value( moduleName.url() );
    kWarning() << "Link name: " + moduleName.url();
    emit moduleSelected( module );
}

#include "CategoryList.moc"
