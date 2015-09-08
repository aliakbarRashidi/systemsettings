/***************************************************************************
 *   Copyright (C) 2009 by Ben Cooksley <bcooksley@kde.org>                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#include "SettingsBase.h"
#include "BaseConfig.h"

#include <QTimer>
#include <QRadioButton>
#include <QVariantList>
#include <QLoggingCategory>
#include <QMenu>
#include <QMenuBar>
#include <QScreen>

#include <kaboutdata.h>
#include <KMessageBox>
#include <KConfigGroup>
#include <KCModuleInfo>
#include <KXMLGUIFactory>
#include <KStandardAction>
#include <KActionCollection>
#include <KServiceTypeTrader>
#include <KToolBar>
#include <kwindowconfig.h>

#include "BaseData.h"
#include "ModuleView.h"

SettingsBase::SettingsBase( QWidget * parent )
    : KXmlGuiWindow(parent)
{
    // Ensure delayed loading doesn't cause a crash
    activeView = 0;
    aboutDialog = 0;
    configDialog = 0;
    lostFound = 0;
    // Prepare the view area
    stackedWidget = new QStackedWidget( this );
    setWindowTitle(i18n("System Settings"));
    setWindowIcon(QIcon::fromTheme("preferences-system"));
    setCentralWidget(stackedWidget);
    setWindowFlags( windowFlags() | Qt::WindowContextHelpButtonHint );
    // Initialise search
    searchText = new KLineEdit( this );
    searchText->setClearButtonShown( true );
    searchText->setPlaceholderText( i18nc( "Search through a list of control modules", "Search" ) );
    searchText->setCompletionMode( KCompletion::CompletionPopup );

    spacerWidget = new QWidget( this );
    spacerWidget->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Maximum );
    // Initalise the window so we don't flicker
    initToolBar();
    // We can now launch the delayed loading safely
    QTimer::singleShot(0, this, SLOT(initApplication()));
}

SettingsBase::~SettingsBase()
{
    delete rootModule;
}

QSize SettingsBase::sizeHint() const
{
    qreal factor = qBound(1., QGuiApplication::primaryScreen()->physicalDotsPerInch()/96., 3.);

    return QSize(720*factor, 600*factor);
}

void SettingsBase::initApplication()
{
    // Prepare the menu of all modules
    categories = KServiceTypeTrader::self()->query("SystemSettingsCategory");
    modules = KServiceTypeTrader::self()->query("KCModule", "[X-KDE-System-Settings-Parent-Category] != ''");
    modules += KServiceTypeTrader::self()->query("SystemSettingsExternalApp");
    rootModule = new MenuItem( true, 0 );
    initMenuList(rootModule);
    // Handle lost+found modules...
    if (lostFound) {
        for (int i = 0; i < modules.size(); ++i) {
            const KService::Ptr entry = modules.at(i);
            MenuItem * infoItem = new MenuItem(false, lostFound);
            infoItem->setService( entry );
            qDebug() << "Added " << entry->name();
        }
    }

    // Prepare the Base Data
    BaseData::instance()->setMenuItem( rootModule );
    // Load all possible views
    const KService::List pluginObjects = KServiceTypeTrader::self()->query( "SystemSettingsView" );
    const int nbPlugins = pluginObjects.count();
    for( int pluginsDone = 0; pluginsDone < nbPlugins ; ++pluginsDone ) {
        KService::Ptr activeService = pluginObjects.at( pluginsDone );
        QString error;
        BaseMode * controller = activeService->createInstance<BaseMode>(this, QVariantList(), &error);
        if( error.isEmpty() ) {
            possibleViews.insert( activeService->library(), controller );
            controller->init( activeService );
            connect(controller, &BaseMode::changeToolBarItems, this, &SettingsBase::changeToolBar);
            connect(controller, &BaseMode::actionsChanged, this, &SettingsBase::updateViewActions);
            connect(searchText, &KLineEdit::textChanged, controller, &BaseMode::searchChanged);
            connect(controller, &BaseMode::viewChanged, this, &SettingsBase::viewChange);
        } else {
            qWarning() << "View load error: " + error;
        }
    }
    searchText->completionObject()->setIgnoreCase( true );
    searchText->completionObject()->setItems( BaseData::instance()->menuItem()->keywords() );
    changePlugin();
    activateWindow();
}

void SettingsBase::initToolBar()
{
    // Fill the toolbar with default actions
    // Exit is the very last action
    quitAction = actionCollection()->addAction( KStandardAction::Quit, "quit_action", this, SLOT(close()) );
    // Configure goes at the end
    configureAction = actionCollection()->addAction( KStandardAction::Preferences, this, SLOT(configShow()) );
    actionCollection()->setDefaultShortcut(configureAction, QKeySequence(Qt::CTRL + Qt::Key_M));
    configureAction->setText( i18n("Configure") );
    // Help after it
    initHelpMenu();
    configureAction->setIcon(QIcon::fromTheme("application-menu"));

    // Then a spacer so the search line-edit is kept separate
    spacerAction = new QWidgetAction( this );
    spacerAction->setDefaultWidget(spacerWidget);
    actionCollection()->addAction( "spacer", spacerAction );
    // Finally the search line-edit
    searchAction = new QWidgetAction( this );
    searchAction->setDefaultWidget(searchText);
    actionCollection()->setDefaultShortcut(searchAction, QKeySequence(Qt::CTRL + Qt::Key_F));
    connect( searchAction, SIGNAL(triggered(bool)), searchText, SLOT(setFocus()));
    actionCollection()->addAction( "searchText", searchAction );
    // Initialise the Window
    setupGUI(Save|Create,QString());
    menuBar()->hide();

    // Toolbar & Configuration
    helpActionMenu->setMenu( dynamic_cast<QMenu*>( factory()->container("help", this) ) );
    setMinimumSize(620,430);
    toolBar()->setMovable(false); // We don't allow any changes
    changeToolBar( BaseMode::Search | BaseMode::Configure );
}

void SettingsBase::initHelpMenu()
{
    helpActionMenu = new KActionMenu( QIcon::fromTheme("system-help"), i18n("Help"), this );
    helpActionMenu->setDelayed( false );
    actionCollection()->addAction( "help_toolbar_menu", helpActionMenu );
    // Add the custom actions
    aboutModuleAction = actionCollection()->addAction( KStandardAction::AboutApp, "help_about_module", this, SLOT(about()) );
    changeAboutMenu( 0, aboutModuleAction, i18n("About Active Module") );
    aboutViewAction = actionCollection()->addAction( KStandardAction::AboutApp, "help_about_view", this, SLOT(about()) );
}

void SettingsBase::initConfig()
{
    // Prepare dialog first
    configDialog = new KConfigDialog( this, "systemsettingsconfig", BaseConfig::self() );
    configDialog->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // Add our page
    QWidget * configPage = new QWidget( configDialog );
    configWidget.setupUi(configPage);
    QString iconName = KAboutData::applicationData().programIconName();
    configDialog->addPage( configPage, i18nc("General config for System Settings", "General"), iconName );
    QVBoxLayout * configLayout = new QVBoxLayout;
    // Get the list of modules
    foreach( BaseMode * mode, possibleViews ) {
        mode->addConfiguration( configDialog );
        QRadioButton * radioButton = new QRadioButton( mode->service()->name(), configWidget.GbViewStyle );
        radioButton->setIcon( QIcon::fromTheme(mode->service()->icon()) );
        configLayout->addWidget( radioButton );
        viewSelection.addButton( radioButton, possibleViews.values().indexOf(mode) );
    }
    configWidget.GbViewStyle->setLayout( configLayout );
    KWindowConfig::restoreWindowSize(configDialog->windowHandle(), KSharedConfig::openConfig()->group("ConfigDialog"));
    connect(configDialog, &KConfigDialog::accepted, this, &SettingsBase::configUpdated);
}


void SettingsBase::initMenuList(MenuItem * parent)
{
    // look for any categories inside this level, and recurse into them
    for (int i = 0; i < categories.size(); ++i) {
        const KService::Ptr entry = categories.at(i);
        const QString parentCategory = entry->property("X-KDE-System-Settings-Parent-Category").toString();
        const QString parentCategory2 = entry->property("X-KDE-System-Settings-Parent-Category-V2").toString();
        if ( parentCategory == parent->category() ||
             // V2 entries must not be empty if they want to become a proper category.
             ( !parentCategory2.isEmpty() && parentCategory2 == parent->category() ) ) {
            MenuItem * menuItem = new MenuItem(true, parent);
            menuItem->setService( entry );
            if( menuItem->category() == "lost-and-found" ) {
                lostFound = menuItem;
                continue;
            }
            initMenuList( menuItem );
        }
    }

    KService::List removeList;

    // scan for any modules at this level and add them
    for (int i = 0; i < modules.size(); ++i) {
        const KService::Ptr entry = modules.at(i);
        const QString category = entry->property("X-KDE-System-Settings-Parent-Category").toString();
        const QString category2 = entry->property("X-KDE-System-Settings-Parent-Category-V2").toString();
        if( !parent->category().isEmpty() && (category == parent->category() || category2 == parent->category()) ) {
            // Add the module info to the menu
            MenuItem * infoItem = new MenuItem(false, parent);
            infoItem->setService( entry );
            removeList.append( modules.at(i) );
        }
    }

    for (int i = 0; i < removeList.size(); ++i) {
        modules.removeOne( removeList.at(i) );
    }
    
    parent->sortChildrenByWeight();
}

void SettingsBase::configUpdated()
{
    KConfigGroup dialogConfig = KSharedConfig::openConfig()->group("ConfigDialog");
    KWindowConfig::saveWindowSize(configDialog->windowHandle(), dialogConfig);
    BaseConfig::setActiveView( possibleViews.keys().at(viewSelection.checkedId()) );
    BaseConfig::setShowToolTips( configWidget.ChTooltips->isChecked() );
    activeView->saveConfiguration();
    changePlugin();
}

void SettingsBase::configShow()
{
    // Initialise the configuration dialog if it hasn't already
    if( !configDialog ) {
        initConfig();
    }
    if( activeView && activeView->moduleView() && !activeView->moduleView()->resolveChanges() ) {
        return; // It shouldn't be triggering anyway, since the action is disabled
    }

    activeView->loadConfiguration();

    const QStringList pluginList = possibleViews.keys();
    const int configIndex = pluginList.indexOf( BaseConfig::activeView() );
    if( configIndex != -1 ) {
        viewSelection.button( configIndex )->setChecked(true);
    }
    configWidget.ChTooltips->setChecked( BaseConfig::showToolTips() );
    if( pluginList.isEmpty() ) {
        KMessageBox::error(this, i18n("System Settings was unable to find any views, and hence nothing is available to configure."), i18n("No views found"));
    } else {
        configDialog->show();
    }
}

bool SettingsBase::queryClose()
{
    bool changes = true;
    if( activeView ) {
        activeView->saveState();
        changes = activeView->moduleView()->resolveChanges();
    }
    BaseConfig::self()->save();
    return changes;
}

void SettingsBase::about()
{
    delete aboutDialog;
    aboutDialog = 0;

    const KAboutData * about = 0;
    if( sender() == aboutViewAction ) {
        about = activeView->aboutData();
    } else if( sender() == aboutModuleAction && activeView->moduleView() ) {
        about = activeView->moduleView()->aboutData();
    }

    if( about ) {
        aboutDialog = new KAboutApplicationDialog(*about, 0);
        aboutDialog->show();
    }
}

void SettingsBase::changePlugin()
{
    if( possibleViews.count() == 0 ) { // We should ensure we have a plugin available to choose
        KMessageBox::error(this, i18n("System Settings was unable to find any views, and hence has nothing to display."), i18n("No views found"));
        close();
        return; // Halt now!
    }

    if( activeView ) {
        activeView->saveState();
        activeView->leaveModuleView();
    }

    const QString viewToUse = BaseConfig::activeView();
    if( possibleViews.keys().contains(viewToUse) ) { // First the configuration entry
        activeView = possibleViews.value(viewToUse);
    }
    else { // Otherwise we activate the failsafe
        activeView = possibleViews.begin().value();
    }

    if( stackedWidget->indexOf(activeView->mainWidget()) == -1 ) {
        stackedWidget->addWidget(activeView->mainWidget());
    }

    // Handle the tooltips
    qDeleteAll( tooltipManagers );
    tooltipManagers.clear();
    if ( BaseConfig::showToolTips() ) {
        QList<QAbstractItemView*> theViews = activeView->views();
        foreach ( QAbstractItemView* view, theViews ) {
            tooltipManagers << new ToolTipManager( view );
        }
    }

    changeAboutMenu( activeView->aboutData(), aboutViewAction, i18n("About Active View") );
    viewChange(false);

    stackedWidget->setCurrentWidget(activeView->mainWidget());
    updateViewActions();

    activeView->giveFocus();
}

void SettingsBase::viewChange(bool state)
{
    KCModuleInfo * moduleInfo = activeView->moduleView()->activeModule();
    configureAction->setDisabled(state);
    if( moduleInfo ) {
        setCaption( moduleInfo->moduleName(), state );
    } else {
        setCaption( QString(), state );
    }
    changeAboutMenu( activeView->moduleView()->aboutData(), aboutModuleAction, i18n("About Active Module") );
}

void SettingsBase::updateViewActions()
{
    guiFactory()->unplugActionList( this, "viewActions" );
    guiFactory()->plugActionList( this, "viewActions", activeView->actionsList() );
}

void SettingsBase::changeToolBar( BaseMode::ToolBarItems toolbar )
{
    if( sender() != activeView ) {
        return;
    }
    guiFactory()->unplugActionList( this, "configure" );
    guiFactory()->unplugActionList( this, "search" );
    guiFactory()->unplugActionList( this, "quit" );
    if ( BaseMode::Search & toolbar ) {
        QList<QAction*> searchBarActions;
        searchBarActions << spacerAction << searchAction;
        guiFactory()->plugActionList( this, "search", searchBarActions );
    }
    if ( BaseMode::Configure & toolbar ) {
        QList<QAction*> configureBarActions;
        configureBarActions << configureAction;
        guiFactory()->plugActionList( this, "configure", configureBarActions );
    }
    if ( BaseMode::Quit & toolbar ) {
        QList<QAction*> quitBarActions;
        quitBarActions << quitAction;
        guiFactory()->plugActionList( this, "quit", quitBarActions );
    }
}

void SettingsBase::changeAboutMenu( const KAboutData * menuAbout, QAction * menuItem, QString fallback )
{
    if( !menuItem ) {
        return;
    }

    if( menuAbout ) {
        menuItem->setText( i18n( "About %1", menuAbout->displayName() ) );
        menuItem->setIcon( QIcon::fromTheme( menuAbout->programIconName() ) );
        menuItem->setEnabled(true);
    } else {
        menuItem->setText( fallback );
        menuItem->setIcon( QIcon::fromTheme( KAboutData::applicationData().programIconName() ) );
        menuItem->setEnabled(false);
    }
}

