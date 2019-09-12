#include "thumbnaillistview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/signalmanager.h"
#include <QDebug>

namespace {
const int ITEM_SPACING = 5;
const int BASE_HEIGHT = 100;
const int LEFT_MARGIN = 12;
const int RIGHT_MARGIN = 8;

const QString IMAGE_DEFAULTTYPE = "All pics";
const QString FAVORITES_ALBUM = "My favorite";
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

QString ss(const QString &text)
{
    QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text).toString();
    str.replace(" ", "");
    return str;
}
}  //namespace

ThumbnailListView::ThumbnailListView(QString imgtype)
    : m_model(new QStandardItemModel(this))
{
    m_imageType = imgtype;

    m_iDefaultWidth = 0;
    m_iBaseHeight = BASE_HEIGHT;

    setViewportMargins(LEFT_MARGIN, 0, RIGHT_MARGIN, 0);
    setIconSize(QSize(400, 400));
    setResizeMode(QListView::Adjust);
    setViewMode(QListView::IconMode);
    setFlow(QListView::LeftToRight);
    setSpacing(ITEM_SPACING);
    setDragEnabled(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
//    setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_delegate = new ThumbnailDelegate();
    m_delegate->m_imageTypeStr = m_imageType;

    setItemDelegate(m_delegate);
    setModel(m_model);

    m_pMenu = new DMenu();

    initConnections();
}

ThumbnailListView::~ThumbnailListView()
{

}

void ThumbnailListView::initConnections()
{
    connect(this, &QListView::customContextMenuRequested, this, &ThumbnailListView::onShowMenu);
    connect(m_pMenu, &DMenu::triggered, this, &ThumbnailListView::onMenuItemClicked);
	connect(this,&ThumbnailListView::doubleClicked,this,[=](const QModelIndex &index){
        qDebug()<<"index is "<<index.row();
        emit hideExtensionPanel();
        emit openImage(index.row());
    });
    connect(this,&ThumbnailListView::clicked,this,[=](){
            emit hideExtensionPanel();
    });
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &ThumbnailListView::onPixMapScale);
    connect(m_delegate, &ThumbnailDelegate::sigCancelFavorite, this, &ThumbnailListView::onCancelFavorite);
}

void ThumbnailListView::calBasePixMapWandH()
{
    for(int i = 0; i < m_ItemList.length(); i++)
    {
        m_ItemList[i].width = m_ItemList[i].width * m_iBaseHeight / m_ItemList[i].height;
        m_ItemList[i].height = m_iBaseHeight;
    }
}

void ThumbnailListView::calWidgetItemWandH()
{
    int i_baseWidth = 0;
    int i_totalwidth = width() - 36;

    QList<int> rowWidthList;
    QList<ItemInfo> itemInfoList;

    m_gridItem.clear();

    for(int i = 0; i< m_ItemList.length(); i++)
    {
        if ((i_baseWidth + m_ItemList[i].width) <= i_totalwidth)
        {
            i_baseWidth = i_baseWidth + m_ItemList[i].width + ITEM_SPACING;
            itemInfoList<<m_ItemList[i];

            if (i == m_ItemList.length() -1 )
            {
                i_baseWidth -= ITEM_SPACING;
                rowWidthList<<i_baseWidth;
                m_gridItem<<itemInfoList;
            }
        }
        else
        {
            i_baseWidth -= ITEM_SPACING;
            rowWidthList<<i_baseWidth;
            i_baseWidth = m_ItemList[i].width;

            m_gridItem<<itemInfoList;
            itemInfoList.clear();
            itemInfoList<<m_ItemList[i];

            if (i == m_ItemList.length() -1 )
            {
                rowWidthList<<i_baseWidth;
                m_gridItem<<itemInfoList;
            }
        }
    }

    for(int i = 0; i < rowWidthList.length(); i++)
    {
        if (i == rowWidthList.length() - 1)
        {
            break;
        }

        int rowWidth = 0;
        for(int j = 0; j < m_gridItem[i].length(); j++)
        {
            m_gridItem[i][j].width = m_gridItem[i][j].width * i_totalwidth / rowWidthList[i];
            m_gridItem[i][j].height = m_gridItem[i][j].height * i_totalwidth / rowWidthList[i];

            rowWidth = rowWidth + m_gridItem[i][j].width + ITEM_SPACING;
        }

        rowWidthList[i] = rowWidth - ITEM_SPACING;
    }


    if (1 < rowWidthList.length() && rowWidthList[0] < i_totalwidth)
    {
        m_gridItem[0][0].width = m_gridItem[0][0].width + i_totalwidth - rowWidthList[0];
    }

    if (0 < m_gridItem.length())
    {
        m_height = m_gridItem[0][0].height;
    }
}

void ThumbnailListView::addThumbnailView()
{
    m_model->clear();
    for(int i = 0; i < m_gridItem.length(); i++)
    {
        for(int j = 0; j < m_gridItem[i].length(); j++)
        {
            QStandardItem *item = new QStandardItem;

            QVariantList datas;
            datas.append(QVariant(m_gridItem[i][j].name));
            datas.append(QVariant(m_gridItem[i][j].path));
            datas.append(QVariant(m_gridItem[i][j].width));
            datas.append(QVariant(m_gridItem[i][j].height));

            item->setData(QVariant(datas), Qt::DisplayRole);
            item->setData(QVariant(QSize(m_gridItem[i][j].width, m_gridItem[i][j].height)), Qt::SizeHintRole);
            m_model->appendRow(item);
        }
    }
}

void ThumbnailListView::updateThumbnailView()
{
    int index = 0;
    for(int i = 0; i < m_gridItem.length(); i++)
    {
        for(int j = 0; j < m_gridItem[i].length(); j++)
        {
            QSize picSize(m_gridItem[i][j].width, m_gridItem[i][j].height);

            m_model->item(index, 0)->setSizeHint(picSize);
            index++;
        }
    }
}

void ThumbnailListView::insertThumbnails(const QList<ItemInfo> &itemList)
{
    m_ItemList = itemList;

    for(int i = 0; i < m_ItemList.length(); i++)
    {
        QPixmap pixmap;
        pixmap.load(m_ItemList[i].path);
        m_ItemList[i].width = pixmap.width();
        m_ItemList[i].height = pixmap.height();
    }

    calBasePixMapWandH();

    if (0 != m_iDefaultWidth)
    {
        calWidgetItemWandH();
        addThumbnailView();
    }
}

void ThumbnailListView::onShowMenu(const QPoint &pos)
{
    if (!this->indexAt(pos).isValid())
    {
        return;
    }

    updateMenuContents();
    m_pMenu->popup(QCursor::pos());
}

void ThumbnailListView::updateMenuContents()
{
    m_pMenu->clear();
    appendAction(IdView, tr("View"), ss("View"));
    appendAction(IdFullScreen, tr("Fullscreen"), ss("Fullscreen"));
    appendAction(IdStartSlideShow, tr("Slide show"), ss("Slide show"));
    QMenu *am = createAlbumMenu();
    if (am) {
        m_pMenu->addMenu(am);
    }
    m_pMenu->addSeparator();
    appendAction(IdExport, tr("export"), ss("export"));
    appendAction(IdCopyToClipboard, tr("Copy to clipboard"), ss("Copy to clipboard"));
    appendAction(IdMoveToTrash, tr("Throw to trash"), ss("Throw to trash"));
    m_pMenu->addSeparator();
    appendAction(IdAddToFavorites, tr("Favorite"), ss("Favorite"));
//    appendAction(IdRemoveFromFavorites, tr("Unfavorite"), ss("Unfavorite"));
    m_pMenu->addSeparator();
    appendAction(IdRotateClockwise, tr("Rotate clockwise"), ss("Rotate clockwise"));
    appendAction(IdRotateCounterclockwise, tr("Rotate counterclockwise"), ss("Rotate counterclockwise"));
    m_pMenu->addSeparator();
    appendAction(IdSetAsWallpaper, tr("Set as wallpaper"), ss("Set as wallpaper"));
    appendAction(IdDisplayInFileManager, tr("Display in file manager"), ss("Display in file manager"));
    appendAction(IdImageInfo, tr("Image info"), ss("Image info"));
}

void ThumbnailListView::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_pMenu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_pMenu->addAction(ac);
}

QMenu *ThumbnailListView::createAlbumMenu()
{
    QMenu *am = new QMenu(tr("Add to album"));

    QStringList albums = DBManager::instance()->getAllAlbumNames();
    albums.removeAll(FAVORITES_ALBUM);

    QAction *ac = new QAction(am);
    ac->setProperty("MenuID", IdAddToAlbum);
    ac->setText(tr("Add to new album"));
    ac->setData(QString("Add to new album"));
    am->addAction(ac);
    am->addSeparator();
    for (QString album : albums) {
        QAction *ac = new QAction(am);
        ac->setProperty("MenuID", IdAddToAlbum);
        ac->setText(fontMetrics().elidedText(QString(album).replace("&", "&&"), Qt::ElideMiddle, 200));
        ac->setData(album);
        am->addAction(ac);
    }

    return am;
}

void ThumbnailListView::onMenuItemClicked(QAction *action)
{
    QStringList paths = selectedPaths();
    paths.removeAll(QString(""));
    if (paths.isEmpty()) {
        return;
    }

//    const QStringList viewPaths = (paths.length() == 1) ? albumPaths() : paths;
    const QString path = paths.first();

    const int id = action->property("MenuID").toInt();
    switch (MenuItemId(id)) {
    case IdView:
        emit menuOpenImage(path,paths,false);
        break;
    case IdFullScreen:
        emit menuOpenImage(path,paths,true);
        break;
//    case IdStartSlideShow:
//        emit startSlideShow(viewPaths, path);
//        break;
//    case IdPrint: {
//        showPrintDialog(paths);
//        break;
//    }
    case IdAddToAlbum: {
        const QString album = action->data().toString();
        if (album != "Add to new album") {
           DBManager::instance()->insertIntoAlbum(album, paths);
           emit dApp->signalM->sigMenuAddToAlbum();
        }
        else {
            emit dApp->signalM->createAlbum(paths);
        }
        break;
    }
//    case IdCopy:
//        utils::base::copyImageToClipboard(paths);
//        break;
    case IdCopyToClipboard:
        utils::base::copyOneImageToClipboard(path);
        break;
    case IdMoveToTrash:
    {
        DBImgInfoList infos;
        for(auto path : paths)
        {
            DBImgInfo info;
            info = DBManager::instance()->getInfoByPath(path);
            infos<<info;
        }

        DBManager::instance()->insertTrashImgInfos(infos);
        DBManager::instance()->removeImgInfos(paths);
    }
        break;
    case IdAddToFavorites:
        DBManager::instance()->insertIntoAlbum(FAVORITES_ALBUM, paths);
        emit dApp->signalM->sigMenuAddToAlbum();
        break;
//    case IdRemoveFromFavorites:
//        DBManager::instance()->removeFromAlbum(FAVORITES_ALBUM_NAME, paths);
//        break;
//    case IdRemoveFromAlbum:
//        m_view->removeItems(paths);
//        DBManager::instance()->removeFromAlbum(m_album, paths);
//        break;
    case IdRotateClockwise:
    {
        for(QString path : paths)
        {
            utils::image::rotate(path, 90);
        }

        emit dApp->signalM->sigPixMapRotate();
    }
        break;
    case IdRotateCounterclockwise:
    {
        for(QString path : paths)
        {
            utils::image::rotate(path, -90);
        }

        emit dApp->signalM->sigPixMapRotate();
    }
        break;
    case IdSetAsWallpaper:
        dApp->wpSetter->setWallpaper(path);
        break;
    case IdDisplayInFileManager:
        utils::base::showInFileManager(path);
        break;
    case IdImageInfo:
        emit showImageInfo(path);
        break;
    case IdExport:
//        emit exportImage(path,paths);
        emit dApp->signalM->exportImage(paths);
//        Exporter::instance()->exportImage(paths);
        break;
//    default:
//        break;
    }

//    updateMenuContents();
}

//void ThumbnailListView::onShowImageInfo(const QString &path)
//{
//    if (m_infoShowingList.indexOf(path) != -1)
//        return;
//    else
//        m_infoShowingList << path;

//    ImgInfoDialog *info = new ImgInfoDialog(path);
//    info->move((width() - info->width()) / 2 +
//               mapToGlobal(QPoint(0, 0)).x(),
//               (window()->height() - info->sizeHint().height()) / 2 +
//               mapToGlobal(QPoint(0, 0)).y());
//    info->show();
//    info->setWindowState(Qt::WindowActive);
//    connect(info, &ImgInfoDialog::closed, this, [=] {
//        info->deleteLater();
//        m_infoShowingList.removeAll(path);
//    });
//}

QStringList ThumbnailListView::selectedPaths()
{
    QStringList paths;
    for (QModelIndex index : selectionModel()->selectedIndexes()) {
        const QVariantList datas =
                index.model()->data(index, Qt::DisplayRole).toList();
        if (datas.length() == 4) {
            paths << datas[1].toString();
        }
    }

    return paths;
}

void ThumbnailListView::onPixMapScale(int value)
{
    switch(value)
    {
    case 0:
        m_iBaseHeight = 80;
        break;
    case 1:
        m_iBaseHeight = 90;
        break;
    case 2:
        m_iBaseHeight = 100;
        break;
    case 3:
        m_iBaseHeight = 110;
        break;
    case 4:
        m_iBaseHeight = 120;
        break;
    }

    calBasePixMapWandH();
    calWidgetItemWandH();
    addThumbnailView();
}

void ThumbnailListView::onCancelFavorite(const QModelIndex &index)
{
    QStringList str;
    QVariantList datas = index.model()->data(index, Qt::DisplayRole).toList();

    if (datas.length() >= 2) {
        str<<datas[1].toString();
    }

    DBManager::instance()->removeFromAlbum(FAVORITES_ALBUM, str);

    m_model->removeRow(index.row());
}

void ThumbnailListView::resizeEvent(QResizeEvent *e)
{
    if (0 == m_iDefaultWidth)
    {
        calWidgetItemWandH();
        addThumbnailView();
    }
    else
    {
        calWidgetItemWandH();
        updateThumbnailView();
    }
    emit loadend((m_height)*m_gridItem.size()+15);

    m_iDefaultWidth = width();

    QListView::resizeEvent(e);
}