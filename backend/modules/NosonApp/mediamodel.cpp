/*
 *      Copyright (C) 2015-2016 Jean-Luc Barriere
 *
 *  This file is part of Noson-App
 *
 *  Noson-App is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Noson is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "mediamodel.h"
#include "sonos.h"
#include "tools.h"
#include "../../lib/noson/noson/src/private/cppdef.h"

#include <cstdio> // for strncpy
#include <cctype> // for isdigit

#define LOAD_BULKSIZE       100
#define ROOT_DISPLAY_TYPE   SONOS::SMAPIItem::Editorial
#define ROOT_TAG            "root"
#define SEARCH_TAG          "SEARCH"

MediaItem::MediaItem(const SONOS::SMAPIItem& data)
: m_ptr(data.uriMetadata)
, m_valid(false)
, m_canQueue(false)
, m_canPlay(false)
, m_isContainer(false)
{
  m_id = QString::fromUtf8(data.item->GetObjectID().c_str());
  m_parent = QString::fromUtf8(data.item->GetParentID().c_str());
  m_title = QString::fromUtf8(data.item->GetValue("dc:title").c_str());
  m_normalized = normalizedString(m_title);
  m_description = QString::fromUtf8(data.item->GetValue("r:description").c_str());
  m_art = QString::fromUtf8(data.item->GetValue("upnp:albumArtURI").c_str());

  if (data.uriMetadata)
  {
    m_canPlay = true;
    m_canQueue = SONOS::System::CanQueueItem(data.uriMetadata);
    m_objectId = QString::fromUtf8(data.uriMetadata->GetObjectID().c_str());
  }
  std::string tmp;
  switch (data.item->subType())
  {
  case SONOS::DigitalItem::SubType_album:
    m_type = MediaType::album;
    tmp = data.item->GetValue("dc:creator");
    if (!tmp.empty())
      m_artist = QString::fromUtf8(tmp.c_str());
    else
      m_artist = QString::fromUtf8(data.item->GetValue("dc:contributor").c_str());
    m_album = QString::fromUtf8(data.item->GetValue("dc:title").c_str());
    break;
  case SONOS::DigitalItem::SubType_person:
    m_type = MediaType::person;
    m_artist = QString::fromUtf8(data.item->GetValue("dc:title").c_str());
    break;
  case SONOS::DigitalItem::SubType_genre:
    m_type = MediaType::genre;
    break;
  case SONOS::DigitalItem::SubType_playlistContainer:
    m_type = MediaType::playlist;
    break;
  case SONOS::DigitalItem::SubType_audioItem:
    m_type = MediaType::audioItem;
    tmp = data.item->GetValue("dc:creator");
    if (!tmp.empty())
      m_artist = QString::fromUtf8(tmp.c_str());
    else
      m_artist = QString::fromUtf8(data.item->GetValue("dc:contributor").c_str());
    m_album = QString::fromUtf8(data.item->GetValue("upnp:album").c_str());
    break;
  case SONOS::DigitalItem::SubType_storageFolder:
    m_type = MediaType::folder;
    break;
  default:
    m_type = MediaType::unknown;
  }
  m_displayType = data.displayType;
  m_isContainer = data.item->IsContainer();
  m_valid = true;
}

QVariant MediaItem::payload() const
{
  QVariant var;
  var.setValue<SONOS::DigitalItemPtr>(m_ptr);
  return var;
}

MediaModel::MediaModel(QObject* parent)
: QAbstractListModel(parent)
, m_smapi(0)
, m_nextIndex(0)
, m_totalCount(0)
, m_searching(false)
{
}

MediaModel::~MediaModel()
{
  clearData();
  qDeleteAll(m_items);
  m_items.clear();
  SAFE_DELETE(m_smapi);
}

void MediaModel::addItem(MediaItem* item)
{
  {
    SONOS::LockGuard lock(m_lock);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_items << item;
    endInsertRows();
  }
  emit countChanged();
}

int MediaModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  SONOS::LockGuard lock(m_lock);
  return m_items.count();
}

QVariant MediaModel::data(const QModelIndex& index, int role) const
{
  SONOS::LockGuard lock(m_lock);
  if (index.row() < 0 || index.row() >= m_items.count())
      return QVariant();

  const MediaItem* item = m_items[index.row()];
  switch (role)
  {
  case PayloadRole:
    return item->payload();
  case IdRole:
    return item->id();
  case TitleRole:
    return item->title();
  case DescriptionRole:
    return item->description();
  case ArtRole:
    return item->art();
  case NormalizedRole:
    return item->normalized();
  case TypeRole:
    return item->type();
  case CanQueueRole:
    return item->canQueue();
  case CanPlayRole:
    return item->canPlay();
  case ArtistRole:
    return item->artist();
  case AlbumRole:
    return item->album();
  case ParentRole:
    return item->parent();
  case ObjectIdRole:
    return item->objectId();
  case DisplayTypeRole:
    return item->displayType();
  case IsContainerRole:
    return item->isContainer();
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> MediaModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[PayloadRole] = "payload";
  roles[IdRole] = "id";
  roles[TitleRole] = "title";
  roles[DescriptionRole] = "description";
  roles[ArtRole] = "art";
  roles[NormalizedRole] = "normalized";
  roles[TypeRole] = "type";
  roles[CanQueueRole] = "canQueue";
  roles[CanPlayRole] = "canPlay";
  roles[ArtistRole] = "artist";
  roles[AlbumRole] = "album";
  roles[ParentRole] = "parent";
  roles[ObjectIdRole] = "objectId";
  roles[DisplayTypeRole] = "displayType";
  roles[IsContainerRole] = "isContainer";
  return roles;
}

QVariantMap MediaModel::get(int row)
{
  SONOS::LockGuard lock(m_lock);
  if (row < 0 || row >= m_items.count())
    return QVariantMap();
  const MediaItem* item = m_items[row];
  QVariantMap model;
  QHash<int, QByteArray> roles = roleNames();
  model[roles[PayloadRole]] = item->payload();
  model[roles[IdRole]] = item->id();
  model[roles[TitleRole]] = item->title();
  model[roles[DescriptionRole]] = item->description();
  model[roles[ArtRole]] = item->art();
  model[roles[NormalizedRole]] = item->normalized();
  model[roles[TypeRole]] = item->type();
  model[roles[CanQueueRole]] = item->canQueue();
  model[roles[CanPlayRole]] = item->canPlay();
  model[roles[ArtistRole]] = item->artist();
  model[roles[AlbumRole]] = item->album();
  model[roles[ParentRole]] = item->parent();
  model[roles[ObjectIdRole]] = item->objectId();
  model[roles[DisplayTypeRole]] = item->displayType();
  model[roles[IsContainerRole]] = item->isContainer();
  return model;
}

bool MediaModel::init(QObject* sonos, const QVariant& service, bool fill)
{
  Sonos* _sonos = reinterpret_cast<Sonos*> (sonos);
  if (!_sonos)
    return false;
  SONOS::PlayerPtr player = _sonos->getPlayer();
  if (!player)
    return false;
  SAFE_DELETE(m_smapi);
  m_smapi = new SONOS::SMAPI(player);
  SONOS::SMServicePtr msvc = service.value<SONOS::SMServicePtr>();
  if (!m_smapi || !m_smapi->Init(msvc, _sonos->getLocale().toUtf8().constData()))
    return false;
  // initialize auth for the service account
  SONOS::SMAccount::Credentials oa = msvc->GetAccount()->GetCredentials();
  m_auth.type = msvc->GetAccount()->GetType();
  m_auth.serialNum = msvc->GetAccount()->GetSerialNum();
  m_auth.key = oa.key;
  m_auth.token = oa.token;
  // initialize path to root
  m_path.clear();
  return ListModel::init(sonos, fill);
}

void MediaModel::clearData()
{
  SONOS::LockGuard lock(m_lock);
  qDeleteAll(m_data);
  m_data.clear();
}

bool MediaModel::loadData()
{
  setUpdateSignaled(false);
  SONOS::LockGuard lock(m_lock);
  if (!m_smapi)
  {
    emit loaded(false);
    return false;
  }

  clearData();
  m_dataState = ListModel::NoData;
  m_searching = false; // enable browse state
  m_nextIndex = m_totalCount = 0;
  SONOS::SMAPIMetadata meta;
  if (!m_smapi->GetMetadata(pathId().toUtf8().constData(), m_nextIndex, LOAD_BULKSIZE, false, meta))
  {
    emit totalCountChanged();
    if (m_smapi->AuthTokenExpired())
      emit authStatusChanged();
    emit loaded(false);
    return false;
  }
  m_totalCount = meta.TotalCount();
  m_nextIndex = meta.ItemCount();

  SONOS::SMAPIItemList list = meta.GetItems();
  for (SONOS::SMAPIItemList::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    MediaItem* item = new MediaItem(*it);
    if (item->isValid())
      m_data << item;
    else
    {
      delete item;
      // Also decrease total count
      if (m_totalCount > 0)
        --m_totalCount;
    }
  }
  emit totalCountChanged();
  m_dataState = ListModel::Loaded;
  emit loaded(true);
  return true;
}

QString MediaModel::pathName() const
{
  SONOS::LockGuard lock(m_lock);
  if (m_path.empty())
    return ROOT_TAG;
  else
    return m_path.top().title;
}

QString MediaModel::pathId() const
{
  SONOS::LockGuard lock(m_lock);
  if (m_path.empty())
    return ROOT_TAG;
  else
    return m_path.top().id;
}

int MediaModel::parentDisplayType() const
{
  SONOS::LockGuard lock(m_lock);
  if (m_path.empty())
    return ROOT_DISPLAY_TYPE;
  else
    return m_path.top().displayType;
}

int MediaModel::viewIndex() const
{
  SONOS::LockGuard lock(m_lock);
  if (m_path.empty())
    return 0;
  else
    return m_path.top().viewIndex;
}

QList<QString> MediaModel::listSearchCategories() const
{
  QList<QString> list;
  SONOS::LockGuard lock(m_lock);
  if (m_smapi)
  {
    SONOS::ElementList el = m_smapi->AvailableSearchCategories();
    for (SONOS::ElementList::const_iterator it = el.begin(); it != el.end(); ++it)
      list << QString::fromUtf8((*it)->GetKey().c_str());
  }
  return list;
}

bool MediaModel::isAuthExpired() const
{
  return (m_smapi ? m_smapi->AuthTokenExpired() : false);
}

int MediaModel::policyAuth() const
{
  return (m_smapi ? m_smapi->GetPolicyAuth() : 0);
}

int MediaModel::requestSessionId(const QString& user, const QString& password)
{
  if (m_smapi)
  {
    SONOS::SMOAKeyring::Credentials auth;
    if (m_smapi->GetSessionId(user.toUtf8().constData(), password.toUtf8().constData(), auth))
    {
      m_auth = auth;
      emit authStatusChanged();
      return 1; // succeeded
    }
  }
  return 0;
}

bool MediaModel::beginDeviceRegistration()
{
  if (m_smapi && m_smapi->AuthTokenExpired())
  {
    switch (policyAuth())
    {
    case 2:
      return m_smapi->GetDeviceLinkCode(m_regURL, m_linkCode);
    case 3:
      return m_smapi->GetAppLink(m_regURL, m_linkCode);
    default:
      break;
    }
  }
  return false;
}

int MediaModel::requestDeviceAuth()
{
  if (m_smapi)
  {
    SONOS::SMOAKeyring::Credentials auth;
    if (m_smapi->GetDeviceAuthToken(auth))
      return 0; // retry
    if (!auth.key.empty())
    {
      m_auth = auth;
      emit authStatusChanged();
      return 1; // succeeded
    }
  }
  // signal to reset existing registration instance
  emit authStatusChanged();
  return 2;
}

MediaAuth* MediaModel::getDeviceAuth()
{
  MediaAuth* _auth = new MediaAuth();
  _auth->resetAuth(m_auth);
  return _auth;
}

bool MediaModel::asyncLoad()
{
  if (m_provider)
  {
    m_provider->runModelLoader(this);
    return true;
  }
  return false;
}

bool MediaModel::loadMoreData()
{
  SONOS::LockGuard lock(m_lock);
  if (!m_smapi)
  {
    emit loadedMore(false);
    return false;
  }
  // At end return false
  if (m_nextIndex >= m_totalCount)
  {
    emit loadedMore(false);
    return false;
  }

  SONOS::SMAPIMetadata meta;
  // browse or search for next items depending of current state
  if ((!m_searching && !m_smapi->GetMetadata(pathId().toUtf8().constData(), m_nextIndex, LOAD_BULKSIZE, false, meta)) ||
      (m_searching && !m_smapi->Search(m_searchCategory, m_searchTerm, m_nextIndex, LOAD_BULKSIZE, meta)))
  {
    if (m_smapi->AuthTokenExpired())
      emit authStatusChanged();
    emit loaded(false);
    return false;
  }
  if (m_totalCount != meta.TotalCount())
  {
    m_totalCount = meta.TotalCount();
    emit totalCountChanged();
  }
  m_nextIndex += meta.ItemCount();

  SONOS::SMAPIItemList list= meta.GetItems();
  for (SONOS::SMAPIItemList::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    MediaItem* item = new MediaItem(*it);
    if (item->isValid())
      m_data << item;
    else
    {
      delete item;
      // Also decrease total count
      if (m_totalCount) {
        --m_totalCount;
        emit totalCountChanged();
      }
    }
  }
  m_dataState = ListModel::Loaded;
  emit loadedMore(true);
  return true;
}

bool MediaModel::asyncLoadMore()
{
  if (!m_provider)
    return false;
  m_provider->runCustomizedModelLoader(this, 1);
  return true;
}

bool MediaModel::loadChild(const QString& id, const QString& title, int displayType, int viewIndex /*= 0*/)
{
  if (id.isEmpty())
    return false;
  SONOS::LockGuard lock(m_lock);
  // save current view index for this path item
  if (!m_path.empty())
    m_path.top().viewIndex = viewIndex;
  m_path.push(Path(id, title, displayType));
  emit pathChanged();
  return loadData();
}

bool MediaModel::asyncLoadChild(const QString &id, const QString &title, int displayType, int viewIndex /*= 0*/)
{
  if (id.isEmpty())
    return false;
  {
    SONOS::LockGuard lock(m_lock);
    // save current view index for this path item
    if (!m_path.empty())
      m_path.top().viewIndex = viewIndex;
    m_path.push(Path(id, title, displayType));
    emit pathChanged();
  }
  return asyncLoad();
}

bool MediaModel::loadParent()
{
  SONOS::LockGuard lock(m_lock);
  if (!m_path.empty())
    m_path.pop();
  // reload current search else the parent item
  if (pathName() == SEARCH_TAG)
  {
    m_searching = true; // reset state before signal the change
    emit pathChanged();
    return search();
  }
  else
  {
    m_searching = false; // reset state before signal the change
    emit pathChanged();
    return loadData();
  }
}

bool MediaModel::asyncLoadParent()
{
  if (!m_provider)
    return false;
  m_provider->runCustomizedModelLoader(this, 2);
  return true;
}

bool MediaModel::loadSearch(const QString &category, const QString &term)
{
  SONOS::LockGuard lock(m_lock);
  m_searchCategory = category.toUtf8().constData();
  m_searchTerm = term.toUtf8().constData();
  m_searching = true; // enable search state
  m_path.clear();
  m_path.push(Path("", SEARCH_TAG, ROOT_DISPLAY_TYPE));
  emit pathChanged();
  return search();
}

bool MediaModel::asyncLoadSearch(const QString &category, const QString &term)
{
  {
    SONOS::LockGuard lock(m_lock);
    m_searchCategory = category.toUtf8().constData();
    m_searchTerm = term.toUtf8().constData();
    m_searching = true; // enable search state
    m_path.clear();
    m_path.push(Path("", SEARCH_TAG, ROOT_DISPLAY_TYPE));
    emit pathChanged();
  }
  if (!m_provider)
    return false;
  m_provider->runCustomizedModelLoader(this, 3);
  return true;
}

bool MediaModel::search()
{
  if (!m_smapi)
  {
    emit loaded(false);
    return false;
  }

  SONOS::SMAPIMetadata meta;
  if (!m_smapi->Search(m_searchCategory, m_searchTerm, 0, LOAD_BULKSIZE, meta))
  {
    emit totalCountChanged();
    if (m_smapi->AuthTokenExpired())
      emit authStatusChanged();
    emit loaded(false);
    return false;
  }
  clearData();
  m_dataState = ListModel::NoData;
  m_totalCount = meta.TotalCount();
  m_nextIndex = meta.ItemCount();
  SONOS::SMAPIItemList list = meta.GetItems();
  for (SONOS::SMAPIItemList::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    MediaItem* item = new MediaItem(*it);
    if (item->isValid())
      m_data << item;
    else
    {
      delete item;
      // Also decrease total count
      if (m_totalCount > 0)
        --m_totalCount;
    }
  }
  emit totalCountChanged();
  m_dataState = ListModel::Loaded;
  emit loaded(true);
  return true;
}

void MediaModel::resetModel()
{
  {
    SONOS::LockGuard lock(m_lock);
    if (m_dataState != ListModel::Loaded)
        return;
    beginResetModel();
    beginRemoveRows(QModelIndex(), 0, m_items.count()-1);
    qDeleteAll(m_items);
    m_items.clear();
    endRemoveRows();
    beginInsertRows(QModelIndex(), 0, m_data.count()-1);
    foreach (MediaItem* item, m_data)
        m_items << item;
    m_data.clear();
    m_dataState = ListModel::Synced;
    endInsertRows();
    endResetModel();
  }
  emit countChanged();
}

void MediaModel::appendModel()
{
  {
    SONOS::LockGuard lock(m_lock);
    if (m_dataState != ListModel::Loaded)
      return;
    int cnt = m_items.count();
    beginInsertRows(QModelIndex(), cnt, cnt + m_data.count()-1);
    foreach (MediaItem* item, m_data)
        m_items << item;
    m_data.clear();
    m_dataState = ListModel::Synced;
    endInsertRows();
  }
  emit countChanged();
}

bool MediaModel::customizedLoad(int id)
{
  switch (id)
  {
    case 0:
      return loadData();
    case 1:
      return loadMoreData();
    case 2:
      return loadParent();
    case 3:
      return search();
    default:
      return false;
  }
}

void MediaModel::handleDataUpdate()
{
  if (!updateSignaled())
  {
    setUpdateSignaled(true);
    dataUpdated();
  }
}
