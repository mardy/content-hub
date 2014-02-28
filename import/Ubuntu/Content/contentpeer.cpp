/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "contenthandler.h"
#include "contenthub.h"
#include "contenticonprovider.h"
#include "contentpeer.h"
#include "contenttype.h"

#include <com/ubuntu/content/peer.h>
#include <QDebug>
#include <QIcon>

/*!
 * \qmltype ContentPeer
 * \instantiates ContentPeer
 * \inqmlmodule Ubuntu.Content 0.1
 * \brief An application that can export or import a ContentType
 *
 * A ContentPeer is an application that is registered in the ContentHub as
 * a source or destination of a ContentType
 *
 * See documentation for ContentHub
 */

namespace cuc = com::ubuntu::content;

ContentPeer::ContentPeer(QObject *parent)
    : QObject(parent),
      m_peer(0),
      m_handler(ContentHandler::Source),
      m_contentType(ContentType::Unknown),
      m_selectionType(ContentTransfer::Single),
      m_explicit_app(false)
{
    qDebug() << Q_FUNC_INFO;
    m_hub = cuc::Hub::Client::instance();
}

/*!
 * \qmlproperty string ContentPeer::name
 *
 * Returns user friendly name of the peer
 */
QString ContentPeer::name()
{
    qDebug() << Q_FUNC_INFO;
    return m_peer.name();
}

/*!
 * \qmlproperty string ContentPeer::appId
 *
 * Returns the Application id
 */
const QString &ContentPeer::appId() const
{
    qDebug() << Q_FUNC_INFO;
    return m_peer.id();
}

/*!
 * \brief ContentPeer::setAppId
 *
 * Sets the Application id
 */
void ContentPeer::setAppId(const QString& appId)
{
    qDebug() << Q_FUNC_INFO << appId;
    // FIXME: Not sure if it's a good idea to be able to change the peer
    this->setPeer(cuc::Peer{appId});
    m_explicit_app = true;
}

QImage &ContentPeer::icon()
{
    qDebug() << Q_FUNC_INFO;
    return m_icon;
}

/*!
 * \brief ContentPeer::peer
 * \internal
 */
const com::ubuntu::content::Peer &ContentPeer::peer() const
{
    return m_peer;
}

/*!
 * \brief ContentPeer::setPeer
 * \internal
 */
void ContentPeer::setPeer(const cuc::Peer &peer)
{
    qDebug() << Q_FUNC_INFO;
    m_peer = peer;
    if (peer.icon().isNull())
    {
        if (QIcon::hasThemeIcon(peer.iconName().toUtf8()))
            m_icon = QIcon::fromTheme(peer.iconName().toUtf8()).pixmap(256).toImage();
    } else
        m_icon = peer.icon();
    ContentIconProvider *iconProvider = ContentIconProvider::instance();
    iconProvider->addImage(appId(), m_icon);

    Q_EMIT nameChanged();
    Q_EMIT appIdChanged();
}

/*!
 * \qmlproperty int ContentPeer::handler
 *
 * Returns the ContentHandler 
 */
ContentHandler::Handler ContentPeer::handler() {
    qDebug() << Q_FUNC_INFO;
    return m_handler;
}

/*!
 * \brief ContentPeer::setHandler
 * \internal
 */
void ContentPeer::setHandler(ContentHandler::Handler handler)
{   
    qDebug() << Q_FUNC_INFO;
    m_handler = handler;

    Q_EMIT handlerChanged();
}

/*!
 * \qmlproperty int ContentPeer::contentType
 *
 * Returns the ContentType
 */
ContentType::Type ContentPeer::contentType() 
{
    qDebug() << Q_FUNC_INFO;
    return m_contentType;
}

/*!
 * \brief ContentPeer::setContentType
 * \internal
 */
void ContentPeer::setContentType(ContentType::Type contentType)
{   
    qDebug() << Q_FUNC_INFO;
    m_contentType = contentType;

    if(!m_explicit_app) {
        const cuc::Type &hubType = ContentType::contentType2HubType(m_contentType);
        setPeer(m_hub->default_source_for_type(hubType));
    }

    Q_EMIT contentTypeChanged();
}

/*!
 * \qmlproperty int ContentPeer::selectionType
 *
 * Returns the ContentTransfer selection type
 */
ContentTransfer::SelectionType ContentPeer::selectionType()
{
    qDebug() << Q_FUNC_INFO;
    return m_selectionType;
}

/*!
 * \brief ContentPeer::setSelectionType
 * \internal
 */
void ContentPeer::setSelectionType(ContentTransfer::SelectionType selectionType)
{
    qDebug() << Q_FUNC_INFO;
    m_selectionType = selectionType;

    Q_EMIT selectionTypeChanged();
}

/*!
 * \qmlmethod ContentPeer::request()
 * \overload ContentPeer::request(ContentStore)
 *
 * \brief Request to import data from this \a ContentPeer
 */
ContentTransfer *ContentPeer::request()
{   
    qDebug() << Q_FUNC_INFO;
    return request(nullptr);
}

/*!
 * \qmlmethod ContentPeer::request(ContentStore)
 *
 * \brief Request to import data from this \a ContentPeer and use
 * a \a ContentStore for permanent storage
 */
ContentTransfer *ContentPeer::request(ContentStore *store)
{
    qDebug() << Q_FUNC_INFO;
    ContentHub *contentHub = ContentHub::instance();
    ContentTransfer *qmlTransfer = NULL;
    if(m_handler == ContentHandler::Source) {
        qmlTransfer = contentHub->importContent(m_peer);
    } else if (m_handler == ContentHandler::Destination) {
        qmlTransfer = contentHub->exportContent(m_peer);
    } else if (m_handler == ContentHandler::Share) {
        qmlTransfer = contentHub->shareContent(m_peer);
    }

    qmlTransfer->setSelectionType(m_selectionType);
    if(store) {
        store->updateStore(m_contentType);
        qmlTransfer->setStore(store);
    }
    
    /* We only need to start it for import requests */
    if (m_handler == ContentHandler::Source)
        qmlTransfer->start();

    return qmlTransfer;
}

