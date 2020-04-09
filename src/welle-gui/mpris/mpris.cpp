/*
 *    Copyright (C) 2020
 *    tenzap (@github)
 *
 *    Copyright (C) 2019
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file was partly generated by qdbusxml2cpp version 0.8
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 */

#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#ifdef __unix__
    #include <unistd.h>
    #define GETPID getpid
#endif
#ifdef _WIN32
    #include <process.h>
    #define GETPID _getpid
#endif

#include "mpris.h"
#include "mpris_mp2.h"
#include "mpris_mp2_player.h"
#include "../gui_helper.h"

Mpris::Mpris(CRadioController *RadioController, QObject *parent)
    : QObject(parent)
    , radioController(RadioController)
{
    new MediaPlayer2Adaptor(this);
    new PlayerAdaptor(this);

    guiHelper = qobject_cast< CGUIHelper * >(parent);

    // Connect to Dbus for MPRIS2
    QDBusConnection connection = QDBusConnection::sessionBus();

    QString serviceName = "org.mpris.MediaPlayer2.welleio";
    bool registered = connection.registerService(serviceName);
    if (!registered)
        registered = connection.registerService(serviceName + ".instance" + QString::number(GETPID()));

    registered = connection.registerObject("/org/mpris/MediaPlayer2", this, QDBusConnection::ExportAdaptors);
    if (!registered)
        qDebug() << "MPRIS: DBus Object not registered.";

    connect(radioController, &CRadioController::autoServiceChanged, this, &Mpris::autoServiceChanged);
    connect(radioController, &CRadioController::autoChannelChanged, this, &Mpris::autoChannelChanged);
    connect(radioController, &CRadioController::titleChanged, this, &Mpris::metadataChanged);
    connect(radioController, &CRadioController::textChanged, this, &Mpris::metadataChanged);
    connect(radioController, &CRadioController::isPlayingChanged, this, &Mpris::isPlayingChanged);
    connect(radioController, &CRadioController::volumeChanged, this, &Mpris::volumeChanged);
    connect(radioController, &CRadioController::motReseted, this, &Mpris::motReseted);
    connect(guiHelper, &CGUIHelper::motChanged, this, &Mpris::motChanged);
}

Mpris::~Mpris()
{
    // Remove picture file
    deletePicFile();
}

void Mpris::setStationArray(QString serializedJson, QString listType, int index)
{
    if ((listType == "all" && index == 0) || (listType == "favorites" && index == 1)) {
        stationArray = QJsonDocument::fromJson(serializedJson.toUtf8()).array();
        //qDebug() << "MPRIS: Updating stations list using:" << listType;
    }
}

int Mpris::getCurrentStationIndex()
{
    quint32 sid = 0;

    // In case the station is not found, use the 1st in the list
    // Typically, it can happen when the user is playing a station
    // from the "All stations" (that is not in the Favorites) while displaying the favorites in the GUI
    int currentStationIndex = 0;

    for (int i=0; i<stationArray.size(); i++) {
        sid = stationArray.at(i).toObject().value("stationSId").toInt();
        if (sid == autoService) {
            currentStationIndex = i;
            break;
        }
    }
    qDebug() << "MPRIS: currentStationIndex" << currentStationIndex;

    return currentStationIndex;
}

bool Mpris::canQuit() const { return true; }

bool Mpris::canRaise() const { return true; }

bool Mpris::canSetFullscreen() const { return true; }

QString Mpris::desktopEntry() const { return "welle-io"; }

bool Mpris::fullscreen() const { return fullscreenState; }

void Mpris::setFullscreen(bool value)
{
    fullscreenState = value;
    EmitNotification("Fullscreen", fullscreenState);
    emit guiHelper->setFullScreen(fullscreenState);
}

void Mpris::setFullscreenState(bool value)
{
    fullscreenState = value;
    EmitNotification("Fullscreen", fullscreenState);
}

bool Mpris::hasTrackList() const { return false; }

QString Mpris::identity() const { return QCoreApplication::applicationName(); }

QStringList Mpris::supportedMimeTypes() const { return QStringList(); }

QStringList Mpris::supportedUriSchemes() const { return QStringList(); }

void Mpris::Quit() { QCoreApplication::quit(); }

void Mpris::Raise() { emit guiHelper->restoreWindow(); }

bool Mpris::canControl() const { return true; }

bool Mpris::canGoNext() const { return true; }

bool Mpris::canGoPrevious() const { return true; }

bool Mpris::canPause() const { return true; }

bool Mpris::canPlay() const { return true; }

bool Mpris::canSeek() const { return false; }

QString Mpris::loopStatus() const { return "None"; }

void Mpris::setLoopStatus(const QString &value) {}

double Mpris::maximumRate() const { return 1.0; }

void Mpris::autoChannelChanged(QString autoChannel) { this->autoChannel = autoChannel; }

void Mpris::autoServiceChanged(quint32 autoService)
{
    this->autoService = autoService;
    EmitNotification("Metadata", metadata());
}

void Mpris::isPlayingChanged(bool isPlaying)
{
    m_isPlaying = isPlaying;
    if (m_isPlaying)
        m_playbackStatus = "Playing";
    else
        m_playbackStatus = "Stopped";

    EmitNotification("PlaybackStatus", playbackStatus());
}

void Mpris::volumeChanged(qreal volume)
{
    EmitNotification("Volume", volume);
}

void Mpris::motChanged(QString pictureName, QString categoryTitle, int categoryId, int slideId)
{
    //qDebug() << "MPRIS: pictureName: " << pictureName;

    deletePicFile();

    QString file = pictureName.split("/").last();
    picPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
    QStringLiteral("/") +
    QString("welle-io.pic.%1.%2").arg(autoService).arg(file);

    QSize * size = new QSize(10,10);

    QPixmap pixmap = guiHelper->motImageProvider->requestPixmap(pictureName, size, QSize(100,100));
    pixmap.save(picPath);

    EmitNotification("Metadata", metadata());
}

void Mpris::motReseted()
{
    deletePicFile();
    picPath = "";
    EmitNotification("Metadata", metadata());
}

void Mpris::deletePicFile() { if (! picPath.isEmpty()) QFile::remove(picPath); }

void Mpris::metadataChanged() { EmitNotification("Metadata", metadata()); }

QVariantMap Mpris::metadata() const
{
    QString stationTitle = qvariant_cast< QString >(radioController->property("title"));
    QString stationText = qvariant_cast< QString >(radioController->property("text"));
    if (stationText.simplified().isEmpty())
        stationText = stationTitle;
    QString picPathAsUrl = QString::fromUtf8(QUrl::fromLocalFile(picPath).toEncoded());

    QVariantMap map;
    map.insert("mpris:trackid", "io.welle.welle-io.Service." + QString::number(autoService));
    if (!picPathAsUrl.isEmpty())
        map.insert("mpris:artUrl", picPathAsUrl);
    map.insert("xesam:album", stationTitle.simplified());
    map.insert("xesam:title", stationText.simplified());
    return map;
}

double Mpris::minimumRate() const { return 1.0; }

QString Mpris::playbackStatus() const { return m_playbackStatus; }

qlonglong Mpris::position() const { return 0; }

double Mpris::rate() const { return 1.0; }

void Mpris::setRate(double value) {}

bool Mpris::shuffle() const { return false; }

void Mpris::setShuffle(bool value) {}

double Mpris::volume() const { return (double) qvariant_cast< qreal >(radioController->property("volume")); }

void Mpris::setVolume(double value) { radioController->setVolume(value); }

void Mpris::Next()
{
    int next = std::min(stationArray.size()-1, getCurrentStationIndex()+1);
    QJsonObject obj = stationArray.at(next).toObject();

    QString autoChannel = obj.value("channelName").toString();
    quint32 autoService = obj.value("stationSId").toInt();
    QString title = obj.value("stationName").toString();
    if (autoService != this->autoService  || autoChannel != this->autoChannel)
        radioController->play(autoChannel, title, autoService);
}

void Mpris::OpenUri(const QString &Uri) {}

void Mpris::Pause() { Stop(); }

void Mpris::Play()
{
    this->autoChannel = qvariant_cast< QString >(radioController->property("autoChannel"));
    this->autoService = qvariant_cast< quint32 >(radioController->property("autoService"));
    QString title =  qvariant_cast< QString >(radioController->property("title"));
    radioController->play(this->autoChannel, title, this->autoService);
}

void Mpris::PlayPause()
{
    if (m_isPlaying)
        Stop();
    else
        Play();
}

void Mpris::Previous()
{
    int previous = std::max(0, getCurrentStationIndex()-1);
    QJsonObject obj = stationArray.at(previous).toObject();

    QString autoChannel = obj.value("channelName").toString();
    quint32 autoService = obj.value("stationSId").toInt();
    QString title = obj.value("stationName").toString();
    if (autoService != this->autoService || autoChannel != this->autoChannel)
        radioController->play(autoChannel, title, autoService);
}

void Mpris::Seek(qlonglong Offset) {}

void Mpris::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position) {}

void Mpris::Stop() { radioController->stop(); }

void Mpris::EmitNotification(const QString& name, const QVariant& val)
{
    EmitNotification(name, val, "org.mpris.MediaPlayer2.Player");
}

void Mpris::EmitNotification(const QString& name,
                             const QVariant& val,
                             const QString& mprisEntity)
{
    QDBusMessage msg = QDBusMessage::createSignal("/org/mpris/MediaPlayer2",
                                                  "org.freedesktop.DBus.Properties",
                                                  "PropertiesChanged");
    QVariantMap map;
    map.insert(name, val);
    QVariantList args = QVariantList() << mprisEntity << map << QStringList();
    msg.setArguments(args);
    QDBusConnection::sessionBus().send(msg);
}
