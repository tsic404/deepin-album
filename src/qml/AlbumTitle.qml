import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import Qt.labs.folderlistmodel 2.11
import org.deepin.dtk 1.0

Rectangle {

    anchors.top : root.top
    anchors.left : leftSidebar.right
    anchors.leftMargin: 0
    width: leftSidebar.visible ? parent.width-leftSidebar.width : root.width
    height: 50
    //        color:titlecontrol.ColorSelector.backgroundColor


//    MouseArea { //为窗口添加鼠标事件
//        anchors.fill: parent
//        acceptedButtons: Qt.LeftButton //只处理鼠标左键
//        property point clickPos: "0,0"
//        onPressed: { //接收鼠标按下事件
//            clickPos  = Qt.point(mouse.x,mouse.y)
//            sigTitlePress()
//        }
//        onPositionChanged: { //鼠标按下后改变位置
//            //鼠标偏移量
//            var delta = Qt.point(mouse.x-clickPos.x, mouse.y-clickPos.y)

//            //如果mainwindow继承自QWidget,用setPos
//            root.setX(root.x+delta.x)
//            root.setY(root.y+delta.y)
//        }
//    }



    TitleBar {
        id : title
        anchors.fill: parent
        width: parent.width
        aboutDialog: AboutDialog {
            icon: "deepin-album"
            width: 400
            modality: Qt.NonModal
            version: qsTr(String("Version: %1").arg(Qt.application.version))
            description: qsTr("Album is a fashion manager for viewing and organizing photos and videos.")
            productName: qsTr("deepin-album")
            websiteName: DTK.deepinWebsiteName
            websiteLink: DTK.deepinWebsitelLink
            license: qsTr(String("%1 is released under %2").arg(productName).arg("GPLV3"))
        }
        ActionButton {
            visible: leftSidebar.visible ? false : true
            id: appTitleIcon
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 0 : 50
            height : 50
            icon {
                name: "deepin-album"
                width: 36
                height: 36
            }
        }

        ActionButton {
            visible: leftSidebar.visible ? false : true
            id: showHideleftSidebarButton
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: appTitleIcon.right
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 0 : 50
            height : 50
            icon {
                name: "topleft"
                width: 36
                height: 36
            }
            onClicked :{
                leftSidebar.visible = !leftSidebar.visible
            }
        }

        ActionButton {
            id: range1Button
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: showHideleftSidebarButton.right
            anchors.leftMargin: 0
            width:50
            height:50
            icon {
                name: publisher.getLoadMode() == 0 ? "range1" : "range2"
                width: 36
                height: 36
            }
            onClicked: {
                //1.图片推送器切换
                publisher.switchLoadMode()

                //切换图标
                if(icon.name == "range1"){
                    icon.name = "range2"
                }else{
                    icon.name = "range1"
                }

                //2.发送全局信号，所有的缩略图强制刷新
                global.sigThumbnailStateChange()
            }
        }
        ButtonBox {

            anchors.top: parent.top
            anchors.topMargin: 7
            anchors.left: range1Button.right
            anchors.leftMargin: 0
            height:36

            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("Year") ;
                checked: true
            }
            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("Month")
            }
            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("Day")
            }
            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("All items")
            }
        }
        SearchEdit{
            placeholder: qsTr("Search")
            width: 240
            anchors.top: parent.top
            anchors.topMargin: 7
            anchors.left: parent.left
            anchors.leftMargin: ( parent.width - width )/2
        }

        ActionButton {

            visible: global.selectedPaths.length === 0
            id: titleImportBtn
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 4 * parent.height
            width: 50
            height: 50
            icon {
                name: "import"
                width: 36
                height: 36
            }
            onClicked :{
                importDialog.open()
            }
        }
        ActionButton {
            id: titleCollectionBtn
            property bool canFavorite: albumControl.canFavorite(global.selectedPaths,global.bRefreshFlag)
            visible: !titleImportBtn.visible
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right: titleRotateBtn.visible ? titleRotateBtn.left : (titleTrashBtn.visible ? titleTrashBtn.left : parent.right)
            anchors.rightMargin: (!titleRotateBtn.visible && !titleTrashBtn.visible) ? 4 * parent.height : 0
            width: 50
            height: 50
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: canFavorite ? qsTr("Favorite") : qsTr("Unfavorite")
            icon {
                name: canFavorite ? "toolbar-collection2" : "toolbar-collection"
                width: 36
                height: 36
            }
            onClicked: {
                if (canFavorite)
                    albumControl.insertIntoAlbum(0, global.selectedPaths)
                else
                    albumControl.removeFromAlbum(0, global.selectedPaths)

                global.bRefreshFlag = !global.bRefreshFlag
            }
        }

        ActionButton {
            id: titleRotateBtn
            visible: (titleImportBtn.visible ? false : true) && fileControl.isRotatable(global.selectedPaths)
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right:  titleTrashBtn.visible ? titleTrashBtn.left : parent.right
            anchors.rightMargin: titleTrashBtn.visible ? 0 : 4 * parent.height
            width: 50
            height: 50
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Rotate")
            icon {
                name: "felete"
                width: 36
                height: 36
            }
        }
        ActionButton {
            id: titleTrashBtn
            visible: (titleImportBtn.visible ? false : true) && fileControl.isCanDelete(global.selectedPaths)
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 4 * parent.height
            width: 50
            height: 50
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete")
            icon {
                name: "delete"
                width: 36
                height: 36
            }
        }
    }

}
