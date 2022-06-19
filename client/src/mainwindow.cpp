/**
 * client/src/mainwindow.cpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2019-2022 Reinder Feenstra
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mainwindow.hpp"
#include <QMenuBar>
#include <QStatusBar>
#include <QMdiSubWindow>
#include <QVBoxLayout>
#include <QToolBar>
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QApplication>
#include <QSplitter>
#include <QToolButton>
#include <QFileDialog>
#include <QDateTime>
#include <QSaveFile>
#include <traintastic/set/worldstate.hpp>
#include "mdiarea.hpp"
#include "dialog/connectdialog.hpp"
#include "settings/settingsdialog.hpp"
#include "dialog/worldlistdialog.hpp"
#include "network/connection.hpp"
#include "network/object.hpp"
#include "network/property.hpp"
#include "network/method.hpp"
#include "subwindow/objectsubwindow.hpp"
#include "subwindow/boardsubwindow.hpp"
#include "subwindow/throttlesubwindow.hpp"
#include "widget/serverlogwidget.hpp"
#include "utils/menu.hpp"
#include "theme/theme.hpp"


#include <QDesktopServices>
#include <traintastic/locale/locale.hpp>
#include <traintastic/copyright.hpp>
#include <version.hpp>

#define SETTING_PREFIX "mainwindow/"
#define SETTING_GEOMETRY SETTING_PREFIX "geometry"
#define SETTING_WINDOWSTATE SETTING_PREFIX "windowstate"
#define SETTING_VIEW_TOOLBAR SETTING_PREFIX "view_toolbar"

static const QLatin1String dotCTW{".ctw"};

inline static void setWorldMute(const ObjectPtr& world, bool value)
{
  if(Q_LIKELY(world))
    world->setPropertyValue("mute", value);
}

inline static void setWorldNoSmoke(const ObjectPtr& world, bool value)
{
  if(Q_LIKELY(world))
    world->setPropertyValue("no_smoke", value);
}

template<class... Args>
static SubWindow* createSubWindow(SubWindowType type, Args... args)
{
  switch(type)
  {
    case SubWindowType::Object:
      return ObjectSubWindow::create(args...);

    case SubWindowType::Board:
      return BoardSubWindow::create(args...);

    case SubWindowType::Throttle:
      return ThrottleSubWindow::create(args...);
  }
  return nullptr;
}

MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  m_splitter{new QSplitter(Qt::Vertical, this)},
  m_mdiArea{new MdiArea(m_splitter)},
  m_serverLog{nullptr},
  m_toolbar{new QToolBar(this)}
{
  instance = this;

  setWindowIcon(Theme::getIcon("appicon"));
  updateWindowTitle();

  QMenu* menu;

  m_mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  // build the main menu
  {
    setMenuBar(new QMenuBar());

    menu = menuBar()->addMenu(Locale::tr("qtapp.mainmenu:file"));
    m_actionConnectToServer = menu->addAction(Locale::tr("qtapp.mainmenu:connect_to_server") + "...", this, [this](){ MainWindow::connectToServer(); });
    m_actionDisconnectFromServer = menu->addAction(Locale::tr("qtapp.mainmenu:disconnect_from_server"), this, &MainWindow::disconnectFromServer);
    menu->addSeparator();
    m_actionNewWorld = menu->addAction(Theme::getIcon("world_new"), Locale::tr("qtapp.mainmenu:new_world"),
      [this]()
      {
        if(m_connection)
          if(const ObjectPtr& traintastic = m_connection->traintastic())
            if(Method* method = traintastic->getMethod("new_world"))
              method->call();
      });
    m_actionNewWorld->setShortcut(QKeySequence::New);
    m_actionLoadWorld = menu->addAction(Theme::getIcon("world_load"), Locale::tr("qtapp.mainmenu:load_world") + "...", this, &MainWindow::loadWorld);
    m_actionLoadWorld->setShortcut(QKeySequence::Open);
    m_actionSaveWorld = menu->addAction(Theme::getIcon("world_save"), Locale::tr("qtapp.mainmenu:save_world"),
      [this]()
      {
        if(m_world)
          if(Method* method = m_world->getMethod("save"))
            method->call();
      });
    m_actionSaveWorld->setShortcut(QKeySequence::Save);
    m_actionCloseWorld = menu->addAction(Theme::getIcon("world_close"), Locale::tr("qtapp.mainmenu:close_world"),
      [this]()
      {
        if(m_connection)
          if(const ObjectPtr& traintastic = m_connection->traintastic())
            traintastic->callMethod("close_world");
      });
    m_actionCloseWorld->setShortcut(QKeySequence::Close);
    menu->addSeparator();
    m_actionImportWorld = menu->addAction(Theme::getIcon("world_import"), Locale::tr("qtapp.mainmenu:import_world") + "...",
      [this]()
      {
        QSettings settings;
        settings.beginGroup("import");
        const QString pathKey{"path"};

        const QString filename = QFileDialog::getOpenFileName(
            this,
            Locale::tr("qtapp.mainmenu:import_world"),
            settings.value(pathKey, QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first()).toString(),
            Locale::tr("qtapp:traintastic_world").append(" (*.ctw)"));

        if(!filename.isEmpty())
        {
          settings.setValue(pathKey, QFileInfo(filename).absolutePath());

          QFile file(filename);
          if(file.open(QIODevice::ReadOnly))
          {
            auto request = Message::newRequest(Message::Command::ImportWorld);
            request->write(file.readAll());
            m_connection->send(request,
              [this](const std::shared_ptr<Message>& response)
              {
                if(response->isResponse() && response->isError())
                {
                  QMessageBox::critical(
                    this,
                    Locale::tr("qtapp:import_world_failed"),
                    Locale::tr("qtapp.error:server_error_x").arg(static_cast<std::underlying_type_t<Message::ErrorCode>>(response->errorCode())));
                }
              });
          }
          else
          {
            QMessageBox::critical(
              this,
              Locale::tr("qtapp:import_world_failed"),
              Locale::tr("qtapp.error:cant_read_from_file_x").arg(filename));
          }
        }
      });
    m_actionExportWorld = menu->addAction(Theme::getIcon("world_export"), Locale::tr("qtapp.mainmenu:export_world") + "...",
      [this]()
      {
        if(m_world)
        {
          assert(m_world->getProperty("name"));

          QSettings settings;
          settings.beginGroup("export");
          const QString pathKey{"path"};

          QString filename =
            settings.value(pathKey, QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString()
              .append(QDir::separator())
              .append(m_world->getProperty("name")->toString())
              .append(QDateTime::currentDateTime().toString(" yyyy-MM-dd"))
              .append(dotCTW);

          filename = QFileDialog::getSaveFileName(
            this,
            Locale::tr("qtapp.mainmenu:export_world"),
            filename,
            Locale::tr("qtapp:traintastic_world").append(" (*.ctw)"));

          if(!filename.isEmpty())
          {
            settings.setValue(pathKey, QFileInfo(filename).absolutePath());

            if(!filename.endsWith(dotCTW))
              filename.append(dotCTW);

            auto request = Message::newRequest(Message::Command::ExportWorld);
            m_connection->send(request,
              [this, filename](const std::shared_ptr<Message>& response)
              {
                if(response->isResponse() && !response->isError())
                {
                  QByteArray worldData;
                  response->read(worldData);

                  bool success = false;
                  QSaveFile file(filename);
                  if(file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                  {
                    const auto* p = worldData.constData();
                    const auto* end = p + worldData.size();
                    while(p < end)
                    {
                      qint64 r = file.write(p, end - p);
                      if(r < 0)
                        break;
                      p += r;
                    }

                    success = file.commit();
                  }

                  if(!success)
                  {
                    QMessageBox::critical(
                      this,
                      Locale::tr("qtapp:export_world_failed"),
                      Locale::tr("qtapp.error:cant_write_to_file_x").arg(filename));
                  }
                }
                else
                {
                  QMessageBox::critical(
                    this,
                    Locale::tr("qtapp:export_world_failed"),
                    Locale::tr("qtapp.error:server_error_x").arg(static_cast<std::underlying_type_t<Message::ErrorCode>>(response->errorCode())));
                }
              });
          }
        }
      });
    menu->addSeparator();
    menu->addAction(Locale::tr("qtapp.mainmenu:quit"), this, &MainWindow::close)->setShortcut(QKeySequence::Quit);

    menu = menuBar()->addMenu(Locale::tr("qtapp.mainmenu:view"));
    m_actionFullScreen = menu->addAction(Locale::tr("qtapp.mainmenu:fullscreen"), this, &MainWindow::toggleFullScreen);
    m_actionFullScreen->setCheckable(true);
    m_actionFullScreen->setShortcut(Qt::Key_F11);
    m_actionViewToolbar = menu->addAction(Locale::tr("qtapp.mainmenu:toolbar"), m_toolbar, &QToolBar::setVisible);
    m_actionViewToolbar->setCheckable(true);
    m_actionServerLog = menu->addAction(Locale::tr("qtapp.mainmenu:server_log") + "...", this, &MainWindow::toggleServerLog);
    m_actionServerLog->setCheckable(true);
    m_actionServerLog->setShortcut(Qt::Key_F12);

    m_menuWorld = menuBar()->addMenu(Locale::tr("qtapp.mainmenu:world"));
    m_menuConnection = m_menuWorld->addMenu(Locale::tr("qtapp.mainmenu:connection"));
    m_worldOnlineAction = m_menuConnection->addAction(Theme::getIcon("online"), Locale::tr("world:online"),
      [this]()
      {
        if(Q_LIKELY(m_world))
          m_world->callMethod("online");
      });
    m_worldOfflineAction = m_menuConnection->addAction(Theme::getIcon("offline"), Locale::tr("world:offline"),
      [this]()
      {
        if(Q_LIKELY(m_world))
          m_world->callMethod("offline");
      });
    m_menuPower = m_menuWorld->addMenu(Locale::tr("qtapp.mainmenu:power"));
    m_worldPowerOnAction = m_menuPower->addAction(Theme::getIcon("power_on"), Locale::tr("world:power_on"),
      [this]()
      {
        if(Q_LIKELY(m_world))
          m_world->callMethod("power_on");
      });
    m_worldPowerOffAction = m_menuPower->addAction(Theme::getIcon("power_off"), Locale::tr("world:power_off"),
      [this]()
      {
        if(Q_LIKELY(m_world))
          m_world->callMethod("power_off");
      });
    m_menuWorld->addSeparator();
    m_worldStopAction = m_menuWorld->addAction(Theme::getIcon("stop"), Locale::tr("world:stop"),
      [this]()
      {
        if(Q_LIKELY(m_world))
        {
          m_world->callMethod("stop");
          if(AbstractProperty* p = m_world->getProperty("state"))
            m_worldStopAction->setChecked(!contains(p->toSet<WorldState>(), WorldState::Run)); // make sure checked matches current world state
        }
      });
    m_worldStopAction->setCheckable(true);
    m_worldRunAction = m_menuWorld->addAction(Theme::getIcon("run"), Locale::tr("world:run"),
      [this]()
      {
        if(Q_LIKELY(m_world))
        {
          m_world->callMethod("run");
          if(AbstractProperty* p = m_world->getProperty("state"))
            m_worldRunAction->setChecked(contains(p->toSet<WorldState>(), WorldState::Run)); // make sure checked matches current world state
        }
      });
    m_worldRunAction->setCheckable(true);
    m_menuWorld->addSeparator();
    m_worldMuteMenuAction = m_menuWorld->addAction(Theme::getIcon("mute"), Locale::tr("world:mute"),
      [this](bool checked)
      {
        setWorldMute(m_world, checked);
      });
    m_worldMuteMenuAction->setCheckable(true);
    m_worldNoSmokeMenuAction = m_menuWorld->addAction(Theme::getIcon("no_smoke"), Locale::tr("world:no_smoke"),
      [this](bool checked)
      {
        setWorldNoSmoke(m_world, checked);
      });
    m_worldNoSmokeMenuAction->setCheckable(true);
    m_worldEditAction = m_menuWorld->addAction(Theme::getIcon("edit"), Locale::tr("world:edit"),
      [this](bool checked)
      {
        if(m_world)
          if(AbstractProperty* property = m_world->getProperty("edit"))
            property->setValueBool(checked);
      });
    m_worldEditAction->setCheckable(true);
    m_menuWorld->addSeparator();
    m_worldSimulationAction = m_menuWorld->addAction(Theme::getIcon("simulation"), Locale::tr("world:simulation"),
      [this](bool checked)
      {
        if(m_world)
          if(AbstractProperty* property = m_world->getProperty("simulation"))
            property->setValueBool(checked);
      });
    m_worldSimulationAction->setCheckable(true);
    m_menuWorld->addSeparator();
    m_menuWorld->addAction(Theme::getIcon("world"), Locale::tr("qtapp.mainmenu:world_properties"), [this](){ showObject("world", Locale::tr("qtapp.mainmenu:world_properties")); });

    m_menuObjects = menuBar()->addMenu(Locale::tr("qtapp.mainmenu:objects"));
    menu = m_menuObjects->addMenu(Theme::getIcon("hardware"), Locale::tr("qtapp.mainmenu:hardware"));
    menu->addAction(Locale::tr("world:interfaces") + "...", [this](){ showObject("world.interfaces", Locale::tr("world:interfaces")); });
    menu->addAction(Locale::tr("world:decoders") + "...", [this](){ showObject("world.decoders", Locale::tr("world:decoders")); });
    menu->addAction(Locale::tr("world:inputs") + "...", [this](){ showObject("world.inputs", Locale::tr("world:inputs")); });
    menu->addAction(Locale::tr("world:outputs") + "...", [this](){ showObject("world.outputs", Locale::tr("world:outputs")); });
    m_menuObjects->addAction(Locale::tr("world:boards") + "...", [this](){ showObject("world.boards", Locale::tr("world:boards")); });
    m_actionLuaScript = m_menuObjects->addAction(Theme::getIcon("lua"), Locale::tr("world:lua_scripts") + "...", [this](){ showObject("world.lua_scripts", Locale::tr("world:lua_scripts")); });

    menu = menuBar()->addMenu(Locale::tr("qtapp.mainmenu:tools"));
    menu->addAction(Locale::tr("qtapp.mainmenu:settings") + "...",
      [this]()
      {
        std::unique_ptr<SettingsDialog> d = std::make_unique<SettingsDialog>(this);
        d->exec();
      });
    menu->addSeparator();
    m_menuServer = menu->addMenu(Locale::tr("qtapp.mainmenu:server"));
    m_actionServerSettings = m_menuServer->addAction(Locale::tr("qtapp.mainmenu:server_settings") + "...", this,
      [this]()
      {
        showObject("traintastic.settings", Locale::tr("qtapp.mainmenu:server_settings"));
      });
    m_menuServer->addSeparator();
    m_actionServerRestart = m_menuServer->addAction(Locale::tr("qtapp.mainmenu:restart_server"), this,
      [this]()
      {
        if(QMessageBox::question(this, Locale::tr("qtapp.mainmenu:restart_server"), Locale::tr("qtapp.mainmenu:restart_server_question"), Locale::tr("qtapp.message_box:yes"), Locale::tr("qtapp.message_box:no"), "", 0, 1) != 0)
          return;

        if(m_connection)
          if(const ObjectPtr& traintastic = m_connection->traintastic())
            if(Method* method = traintastic->getMethod("restart"))
              method->call();
      });
    m_actionServerShutdown = m_menuServer->addAction(Locale::tr("qtapp.mainmenu:shutdown_server"), this,
      [this]()
      {
        if(QMessageBox::question(this, Locale::tr("qtapp.mainmenu:shutdown_server"), Locale::tr("qtapp.mainmenu:shutdown_server_question"), Locale::tr("qtapp.message_box:yes"), Locale::tr("qtapp.message_box:no"), "", 0, 1) != 0)
          return;

        if(m_connection)
          if(const ObjectPtr& traintastic = m_connection->traintastic())
            if(Method* method = traintastic->getMethod("shutdown"))
              method->call();
      });

    menu = menuBar()->addMenu(Locale::tr("qtapp.mainmenu:help"));
    menu->addAction(Locale::tr("qtapp.mainmenu:help"), [](){ QDesktopServices::openUrl(QString("https://traintastic.org/manual?version=" TRAINTASTIC_VERSION_FULL)); })->setShortcut(QKeySequence::HelpContents);
    //menu->addSeparator();
    //menu->addAction(Locale::tr("qtapp.mainmenu:about_qt") + "...", qApp, &QApplication::aboutQt);
    menu->addAction(Locale::tr("qtapp.mainmenu:about") + "...", this, &MainWindow::showAbout);
  }

  // Online/offline:
  m_worldOnlineOfflineToolButton = new QToolButton(this);
  m_worldOnlineOfflineToolButton->setIcon(Theme::getIcon("offline"));
  m_worldOnlineOfflineToolButton->setToolTip(Locale::tr("qtapp:toggle_offline_online"));
  m_worldOnlineOfflineToolButton->setPopupMode(QToolButton::MenuButtonPopup);
  connect(m_worldOnlineOfflineToolButton, &QToolButton::clicked, this,
    [this]()
    {
      if(Q_LIKELY(m_world))
        if(auto* state = m_world->getProperty("state"))
          m_world->callMethod(contains(state->toSet<WorldState>(), WorldState::Online) ? "offline" : "online");
    });
  m_worldOnlineOfflineToolButton->setMenu(createMenu(this, {m_worldOnlineAction, m_worldOfflineAction}));
  m_toolbar->addWidget(m_worldOnlineOfflineToolButton);

  // Power on/off:
  m_worldPowerOnOffToolButton = new QToolButton(this);
  m_worldPowerOnOffToolButton->setIcon(Theme::getIcon("power_off"));
  m_worldPowerOnOffToolButton->setToolTip(Locale::tr("qtapp:toggle_power"));
  m_worldPowerOnOffToolButton->setPopupMode(QToolButton::MenuButtonPopup);
  connect(m_worldPowerOnOffToolButton, &QToolButton::clicked, this,
    [this]()
    {
      if(Q_LIKELY(m_world))
        if(auto* state = m_world->getProperty("state"))
          m_world->callMethod(contains(state->toSet<WorldState>(), WorldState::PowerOn) ? "power_off" : "power_on");
    });
  m_worldPowerOnOffToolButton->setMenu(createMenu(this, {m_worldPowerOnAction, m_worldPowerOffAction}));
  m_toolbar->addWidget(m_worldPowerOnOffToolButton);

  m_toolbar->addSeparator();
  m_toolbar->addAction(m_worldStopAction);
  m_toolbar->addAction(m_worldRunAction);
  m_toolbar->addSeparator();
  m_worldMuteToolbarAction = m_toolbar->addAction(Theme::getIcon("unmute", "mute"), Locale::tr("qtapp:toggle_mute"),
    [this](bool checked)
    {
      setWorldMute(m_world, checked);
    });
  m_worldMuteToolbarAction->setCheckable(true);
  m_worldNoSmokeToolbarAction = m_toolbar->addAction(Theme::getIcon("smoke", "no_smoke"), Locale::tr("qtapp:toggle_smoke"),
    [this](bool checked)
    {
      setWorldNoSmoke(m_world, checked);
    });
  m_worldNoSmokeToolbarAction->setCheckable(true);

  QWidget* spacer = new QWidget(this);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  spacer->show();
  m_toolbar->addWidget(spacer);

  m_toolbar->addAction(m_worldEditAction);

  QVBoxLayout* l = new QVBoxLayout();
  l->setMargin(0);
  l->addWidget(m_toolbar);
  l->addWidget(m_mdiArea);

  QWidget* w = new QWidget();
  w->setLayout(l);
  m_splitter->addWidget(w);
  m_splitter->setCollapsible(0, false);

  setCentralWidget(m_splitter);
  setStatusBar(new QStatusBar());

  QSettings settings;
  if(settings.contains(SETTING_GEOMETRY))
    restoreGeometry(settings.value(SETTING_GEOMETRY).toByteArray());
  if(settings.contains(SETTING_WINDOWSTATE))
    setWindowState(static_cast<Qt::WindowState>(settings.value(SETTING_WINDOWSTATE).toInt()));
  m_actionFullScreen->setChecked(isFullScreen());

  m_actionViewToolbar->setChecked(settings.value(SETTING_VIEW_TOOLBAR, true).toBool());
  m_toolbar->setVisible(m_actionViewToolbar->isChecked());

  connectionStateChanged();
}

MainWindow::~MainWindow()
{
  for(SubWindow* window : m_subWindows)
    disconnect(window, &QMdiSubWindow::destroyed, nullptr, nullptr);
}

void MainWindow::connectToServer(const QString& url)
{
  if(m_connection)
    return;

  std::unique_ptr<ConnectDialog> d = std::make_unique<ConnectDialog>(this, url);
  if(d->exec() == QDialog::Accepted)
  {
    m_connection = d->connection();
    connect(m_connection.get(), &Connection::worldChanged, this,
      [this]()
      {
        worldChanged();
        updateActions();
      });
    connect(m_connection.get(), &Connection::stateChanged, this, &MainWindow::connectionStateChanged);
    worldChanged();
    connectionStateChanged();
  }
}

void MainWindow::disconnectFromServer()
{
  if(m_connection)
    m_connection->disconnectFromHost();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  QSettings settings;
  settings.setValue(SETTING_GEOMETRY, saveGeometry());
  settings.setValue(SETTING_WINDOWSTATE, static_cast<int>(windowState()));
  settings.setValue(SETTING_VIEW_TOOLBAR, m_toolbar->isVisible());
  QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent* event)
{
  if(event->type() == QEvent::WindowStateChange)
    m_actionFullScreen->setChecked(isFullScreen());
}

void MainWindow::worldChanged()
{
  if(m_world)
    m_mdiArea->closeAllSubWindows();

  if(m_connection)
    m_world = m_connection->world();
  else
    m_world.reset();

  if(m_world)
  {
    if(auto* state = m_world->getProperty("state"))
      connect(state, &AbstractProperty::valueChangedInt64, this, &MainWindow::worldStateChanged);

    if(AbstractProperty* simulation = m_world->getProperty("simulation"))
    {
      connect(simulation, &AbstractProperty::attributeChanged,
        [this](AttributeName attribute, const QVariant& value)
        {
          if(attribute == AttributeName::Enabled)
            m_worldSimulationAction->setEnabled(value.toBool());
        });

      m_worldSimulationAction->setEnabled(simulation->getAttributeBool(AttributeName::Enabled, true));
    }
  }

  updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
  QString title = "Traintastic v" TRAINTASTIC_VERSION_FULL;
  if(m_world)
    title = m_world->getProperty("name")->toString() + " - " + title;
  setWindowTitle(title);
}

void MainWindow::loadWorld()
{
  if(!m_connection)
    return;

  std::unique_ptr<WorldListDialog> d = std::make_unique<WorldListDialog>(m_connection, this);
  if(d->exec() == QDialog::Accepted)
  {
    Method* method = m_connection->traintastic()->getMethod("load_world");
    if(Q_LIKELY(method))
      method->call(d->uuid());
  }
}

void MainWindow::toggleFullScreen()
{
  const bool fullScreen = qobject_cast<QAction*>(sender())->isChecked();
  if(fullScreen == isFullScreen())
    return;

  if(fullScreen)
  {
    m_beforeFullScreenGeometry = saveGeometry();
    showFullScreen();
  }
  else
  {
    showNormal(); // required to exit fullscreen mode
    if(!m_beforeFullScreenGeometry.isEmpty())
      restoreGeometry(m_beforeFullScreenGeometry);
    else
      showMaximized();
  }
}

void MainWindow::toggleServerLog()
{
  if(m_serverLog)
  {
    delete m_serverLog;
    m_serverLog = nullptr;
    m_actionServerLog->setChecked(false);
  }
  else
  {
    m_serverLog = new ServerLogWidget(connection(), m_splitter);
    m_splitter->addWidget(m_serverLog);
    m_actionServerLog->setChecked(true);
  }
}

void MainWindow::showObject(const ObjectPtr& object, SubWindowType type)
{
  QString windowId;
  if(auto* property = object->getProperty("id"))
    windowId = toString(type).append("/").append(property->toString());
  if(windowId.isEmpty() || !m_subWindows.contains(windowId))
  {
    SubWindow* window = createSubWindow(type, object);
    if(!window)
      return;
    if(!windowId.isEmpty())
      m_subWindows[windowId] = window;
    m_mdiArea->addSubWindow(window);
    window->setAttribute(Qt::WA_DeleteOnClose);
    connect(window, &QMdiSubWindow::destroyed, this,
      [this, windowId](QObject*)
      {
        m_subWindows.remove(windowId);
      });
    window->show();
  }
  else
    m_mdiArea->setActiveSubWindow(m_subWindows[windowId]);
}

void MainWindow::showObject(const QString& id, const QString& title, SubWindowType type)
{
  const QString windowId{toString(type).append("/").append(id)};
  if(!m_subWindows.contains(windowId))
  {
    SubWindow* window = createSubWindow(type, m_connection, id);
    if(!window)
      return;
    if(!title.isEmpty())
      window->setWindowTitle(title);
    m_subWindows[windowId] = window;
    m_mdiArea->addSubWindow(window);
    window->setAttribute(Qt::WA_DeleteOnClose);
    connect(window, &QMdiSubWindow::destroyed, this,
      [this, windowId](QObject*)
      {
        m_subWindows.remove(windowId);
      });
    window->show();
  }
  else
    m_mdiArea->setActiveSubWindow(m_subWindows[windowId]);
}

void MainWindow::showAbout()
{
  QMessageBox::about(this, tr("About Traintastic"),
    "<h2>Traintastic v" TRAINTASTIC_VERSION " <small>"
#ifdef TRAINTASTIC_VERSION_EXTRA
    + QString(TRAINTASTIC_VERSION_EXTRA).mid(1) +
#else
    TRAINTASTIC_CODENAME
#endif
    "</small></h2>"
    "<p>" + QString(TRAINTASTIC_COPYRIGHT).replace("(c)", "&copy;") + "</p>"
    "<p>This program is free software; you can redistribute it and/or"
    " modify it under the terms of the GNU General Public License"
    " as published by the Free Software Foundation; either version 2"
    " of the License, or (at your option) any later version.</p>"
    "<p>This program is distributed in the hope that it will be useful,"
    " but WITHOUT ANY WARRANTY; without even the implied warranty of"
    " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the"
    " GNU General Public License for more details.</p>"
    "<p><a href=\"https://traintastic.org\">traintastic.org</a></p>");
}

void MainWindow::connectionStateChanged()
{
  const bool connected = m_connection && m_connection->state() == Connection::State::Connected;

  m_mdiArea->setEnabled(connected);

  if(connected && m_actionServerLog->isChecked() && !m_serverLog)
  {
    m_serverLog = new ServerLogWidget(m_connection, m_splitter);
    m_splitter->addWidget(m_serverLog);
  }

  if(m_connection && m_connection->state() == Connection::State::Disconnected)
  {
    m_connection.reset();
    if(m_serverLog)
    {
      delete m_serverLog;
      m_serverLog = nullptr;
    }
    worldChanged();
  }

  updateActions();

  //if(client.state() == Client::State::SocketError)
  //  statusBar()->showMessage(client.errorString());
}

void MainWindow::updateActions()
{
  const bool connected = m_connection && m_connection->state() == Connection::State::Connected;
  const bool haveWorld = connected && m_connection->world();


  m_actionConnectToServer->setEnabled(!m_connection);
  m_actionConnectToServer->setVisible(!connected);
  m_actionDisconnectFromServer->setVisible(connected);
  m_actionNewWorld->setEnabled(connected);
  m_actionLoadWorld->setEnabled(connected);
  m_actionSaveWorld->setEnabled(haveWorld);
  m_actionCloseWorld->setEnabled(haveWorld);
  m_actionImportWorld->setEnabled(connected);
  m_actionExportWorld->setEnabled(haveWorld);

  m_actionServerLog->setEnabled(connected);
  m_menuServer->setEnabled(connected);
  m_actionServerSettings->setEnabled(connected);
  if(connected)
  {
    Method* m;
    m = m_connection->traintastic()->getMethod("restart");
    m_actionServerRestart->setEnabled(m && m->getAttributeBool(AttributeName::Enabled, false));
    m = m_connection->traintastic()->getMethod("shutdown");
    m_actionServerShutdown->setEnabled(m && m->getAttributeBool(AttributeName::Enabled, false));
  }

  setMenuEnabled(m_menuWorld, haveWorld);
  m_worldOnlineOfflineToolButton->setEnabled(haveWorld);
  m_worldPowerOnOffToolButton->setEnabled(haveWorld);
  m_worldMuteToolbarAction->setEnabled(haveWorld);
  m_worldNoSmokeToolbarAction->setEnabled(haveWorld);
  worldStateChanged(haveWorld ? m_connection->world()->getProperty("state")->toInt64() : 0);

  setMenuEnabled(m_menuObjects, haveWorld);
  if(haveWorld && !m_connection->world()->hasProperty("lua_scripts"))
    m_actionLuaScript->setEnabled(false);

  if(connected && !haveWorld)
  {
    m_mdiArea->addBackgroundAction(m_actionNewWorld);
    m_mdiArea->addBackgroundAction(m_actionLoadWorld);
  }
  else
  {
    m_mdiArea->removeBackgroundAction(m_actionNewWorld);
    m_mdiArea->removeBackgroundAction(m_actionLoadWorld);
  }
}

void MainWindow::worldStateChanged(int64_t value)
{
  const WorldState state = static_cast<WorldState>(value);

  QIcon connectionIcon = Theme::getIcon(contains(state, WorldState::Online) ? "online" : "offline");
  QIcon powerIcon = Theme::getIcon(contains(state, WorldState::PowerOn) ? "power_on" : "power_off");

  m_menuConnection->setIcon(connectionIcon);
  m_worldOnlineOfflineToolButton->setIcon(connectionIcon);
  m_menuPower->setIcon(powerIcon);
  m_worldPowerOnOffToolButton->setIcon(powerIcon);
  m_worldStopAction->setChecked(!contains(state, WorldState::Run));
  m_worldRunAction->setChecked(contains(state, WorldState::Run));
  m_worldMuteMenuAction->setChecked(contains(state, WorldState::Mute));
  m_worldMuteToolbarAction->setChecked(contains(state, WorldState::Mute));
  m_worldNoSmokeMenuAction->setChecked(contains(state, WorldState::NoSmoke));
  m_worldNoSmokeToolbarAction->setChecked(contains(state, WorldState::NoSmoke));
  m_worldEditAction->setChecked(contains(state, WorldState::Edit));
  m_worldSimulationAction->setChecked(contains(state, WorldState::Simulation));
}
