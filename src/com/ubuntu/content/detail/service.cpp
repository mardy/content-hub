/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "service.h"

#include "peer_registry.h"
#include "transfer.h"
#include "transferadaptor.h"

#include <com/ubuntu/content/peer.h>
#include <com/ubuntu/content/type.h>

#include <QCache>
#include <QCoreApplication>
#include <QDebug>
#include <QSharedPointer>
#include <QUuid>

#include <cassert>

namespace {
    /* sanitize the dbus object path */
    QString sanitize_path(const QString& path)
    {
        QString sanitized = path;

        for (int i = 0; i < sanitized.length(); ++i)
        {
            if ( !( sanitized[i].isLetter() || sanitized[i].isDigit()))
                sanitized[i] = QLatin1Char('_');
        }
        return sanitized;
    }
}

namespace cucd = com::ubuntu::content::detail;
namespace cuc = com::ubuntu::content;

struct cucd::Service::Private : public QObject
{
    Private(QDBusConnection connection,
            const QSharedPointer<cucd::PeerRegistry>& registry, 
            QObject* parent)
            : QObject(parent),
              connection(connection),
              registry(registry)
    {
    }

    QDBusConnection connection;
    QSharedPointer<cucd::PeerRegistry> registry;
    QSet<cucd::Transfer*> active_transfers;
    QMap<QString, QDBusObjectPath> registered_handlers;
};

cucd::Service::Service(QDBusConnection connection, const QSharedPointer<cucd::PeerRegistry>& peer_registry, QObject* parent)
        : QObject(parent),
          d(new Private{connection, peer_registry, this})
{
    assert(!peer_registry.isNull());
}

cucd::Service::~Service() {}

void cucd::Service::Quit()
{
    QCoreApplication::instance()->quit();
}

QStringList cucd::Service::KnownPeersForType(const QString& type_id)
{
    QStringList result;
    
    d->registry->enumerate_known_peers_for_type(
        Type(type_id),
        [&result](const Peer& peer)
        {
            result.append(peer.id());
        });
    
    return result;
}

QString cucd::Service::DefaultPeerForType(const QString& type_id)
{
    auto peer = d->registry->default_peer_for_type(Type(type_id));

    return peer.id();
}

QDBusObjectPath cucd::Service::CreateImportForTypeFromPeer(const QString& /*type_id*/, const QString& peer_id)
{
    static size_t import_counter{0}; import_counter++;

    static const QString importer_path_pattern{"/transfers/%1/import/%2"};
    static const QString exporter_path_pattern{"/transfers/%1/export/%2"};

    QUuid uuid{QUuid::createUuid()};

    QString destination = importer_path_pattern
            .arg("ThisShouldBeTheAppIdDeterminedFromThePidOfTheCallingProcess")
            .arg(import_counter);


    QString source = exporter_path_pattern
            .arg(sanitize_path(peer_id))
            .arg(import_counter);

    auto transfer = new cucd::Transfer(this);
    new TransferAdaptor(transfer);
    d->active_transfers.insert(transfer);

    if (not d->connection.registerObject(destination, transfer))
        qDebug() << "Problem registering object for path: " << destination;
    d->connection.registerObject(source, transfer);

    qDebug() << "Created transfer " << source << " -> " << destination;

    return QDBusObjectPath{destination};
}

void cucd::Service::RegisterImportExportHandler(const QString& /*type_id*/, const QString& peer_id, const QDBusObjectPath& handler)
{
    qDebug() << Q_FUNC_INFO << peer_id << ":" << peer_id << ":" << handler.path();
    d->registered_handlers.insert(peer_id, handler);
    qDebug() << Q_FUNC_INFO << "REGISTERED HANDLERS:" << d->registered_handlers.count();
}
