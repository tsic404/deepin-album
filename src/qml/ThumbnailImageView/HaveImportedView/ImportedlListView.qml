import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0

import "../../Control/ListView"
import "../../Control"
import "../../"
Item {
    id : importedListView
    signal rectSelTitleChanged(rect rt)
    signal sigUnSelectAll()
    signal sigListViewPressed(int x, int y)
    signal sigListViewReleased(int x, int y)
    property var selectedPaths: []
    property int filterComboOffsetY: 5
    property int spaceCtrlHeight: filterCombo.y + filterComboOffsetY
    property int importCheckboxHeight: 26
    property int listMargin: 10 // 已导入列表子项上、下边距
    property int rowSizeHint: (width - global.thumbnailListRightMargin) / global.cellBaseWidth
    property real realCellWidth : (width - global.thumbnailListRightMargin) / rowSizeHint

    property bool checkBoxClicked: false

    //view依赖的model管理器
    property ListModel importedListModel: ListModel {
        id: theModel
        property var selectedPathObj: {"id":0, "paths":[]}
        property var selectedPathObjs: []
        property var dayHeights: []
        function loadImportedInfos() {
            console.log("imported model has refreshed.. filterType:", filterCombo.currentIndex)
            theModel.clear()
            theModel.selectedPathObjs = []
            theModel.dayHeights = []
            // 从后台获取所有已导入数据,倒序
            var titleInfos = albumControl.getImportTimelinesTitleInfosReverse(filterCombo.currentIndex);
            console.log("imported model has refreshed.. filterType:", filterCombo.currentIndex, " done...")
            var tmpPath = []
            var i = 0
            var dayHeight = 0
            var listHeight = 0
            theView.listContentHeight = 0

            for (var j = 0; j < titleInfos.length; j++) {
                var listItem = titleInfos[j]

                for (var key in listItem) {
                    theModel.append({"title":key, "items":listItem[key]})
                    selectedPathObj = {"id": i, "paths":tmpPath}
                    theModel.selectedPathObjs.push(selectedPathObj)

                    // 计算每个日期列表高度
                    listHeight = Math.abs(Math.ceil(listItem[key].length / Math.floor(importedListView.width / realCellWidth)) * realCellWidth)
                    dayHeight = listHeight + listMargin * 2 + importCheckboxHeight + (i === 0 ? spaceCtrlHeight : 0)
                    dayHeights.push(dayHeight)
                    theView.listContentHeight += dayHeight
                    i++
                }
            }
        }
    }

    // 刷新已导入列表已选路径
    function updateSelectedPaths()
    {
        var tmpPaths = []
        for (var i = 0; i < theModel.selectedPathObjs.length; i++) {
            if (theModel.selectedPathObjs[i].paths.length > 0) {
                for (var j = 0; j < theModel.selectedPathObjs[i].paths.length; j++)
                    tmpPaths.push(theModel.selectedPathObjs[i].paths[j])
            }
        }

        selectedPaths = tmpPaths
        if (importedListView.visible) {
            global.selectedPaths = selectedPaths
        }
    }

    // 通知已导入视图标题栏区域，调整色差校正框选框大小
    Connections {
        target: rubberBand
        onRectSelChanged: {
            var pos1 = theView.contentItem.mapToItem(importedListView, rubberBand.left(), rubberBand.top())
            var pos2 = theView.contentItem.mapToItem(importedListView, rubberBand.right(), rubberBand.bottom())
            rectSelTitleChanged(albumControl.rect(pos1, pos2))
        }
    }

    //已导入列表本体
    ListView {
        id: theView
        clip: true
        interactive: false //禁用原有的交互逻辑，重新开始定制
        model: theModel
        width: parent.width
        height: parent.height
        delegate: importedListDelegate
        //鼠标正在按下状态
        property bool inPress: false
        //框选滚动方向
        property var scrollDirType: GlobalVar.RectScrollDirType.NoType
        property var listContentHeight
        property int rectSelScrollOffset: global.rectSelScrollStep
        signal dbClicked(string url)
        //激活滚动条
        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: true
        }

        MouseArea {
            // 鼠标区域需要包含空白区域，否则点击空白区域会拖动相册应用
            anchors.fill: parent.contentHeight > parent.height ? parent.contentItem : parent
            acceptedButtons: Qt.LeftButton //仅激活左键

            property bool ctrlPressed: false // 记录ctrl是否按下

            id: theMouseArea

            onPressed: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                var gPos = theMouseArea.mapToGlobal(mouse.x, mouse.y)
                sigListViewPressed(gPos.x, gPos.y)
                if (checkBoxClicked) {
                    mouse.accepted = false
                    return
                }

                //取消全选，初始化选中数据
                global.selectedPaths = []
                for (var i = 0; i != theModel.count; ++i) {
                    theModel.selectedPathObjs[i].paths.length = 0
                }

                ctrlPressed = Qt.ControlModifier & mouse.modifiers

                theView.scrollDirType = GlobalVar.RectScrollDirType.NoType
                parent.inPress = true
                rubberBand.x1 = mouse.x
                rubberBand.y1 = mouse.y
                rubberBand.x2 = mouse.x
                rubberBand.y2 = mouse.y
                mouse.accepted = true
            }
            onDoubleClicked: {
                if (global.selectedPaths.length > 0)
                    theView.dbClicked(global.selectedPaths[0])

                parent.inPress = false
                rubberBand.clearRect()

                mouse.accepted = true
            }

            onMouseXChanged: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                rubberBand.x2 = mouse.x

                mouse.accepted = true
            }
            onMouseYChanged: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                // 刷新矩形第二锚点，内部触发updateRect，保证y2值标记为矩形底部坐标
                rubberBand.y2 = mouse.y

                // 确定滚动延展方向（向上还是向下）
                var parentY = mapToItem(theView, mouse.x, mouse.y).y
                if (parentY > theView.height) {
                    // 选择框超出ListView底部，ListView准备向下滚动
                    if (parent.contentHeight > parent.height)
                        theView.scrollDirType = GlobalVar.RectScrollDirType.ToBottom
                } else if (parentY < 0) {
                    // 选择框超出ListView顶部，ListView准备向上滚动
                    theView.scrollDirType = GlobalVar.RectScrollDirType.ToTop
                } else {
                    if (rectScrollTimer.running)
                        rectScrollTimer.stop()
                }

                var gPos = theMouseArea.mapToGlobal(mouse.x, mouse.y)
                sigListViewReleased(gPos.x, gPos.y)

                mouse.accepted = true
            }
            onReleased: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                parent.inPress = false

                // ctrl按下，鼠标点击事件释放时，需要再发送一次框选改变信号，用来在鼠标释放时实现ctrl取消选中的功能
                if ((Qt.ControlModifier & mouse.modifiers) && rubberBand.width < 3 & rubberBand.height < 3) {
                    rubberBand.rectSelChanged()
                }

                ctrlPressed = false

                theView.scrollDirType = GlobalVar.RectScrollDirType.NoType
                rubberBand.clearRect()

                // 清除标题栏色差矫校正框选框
                rectSelTitleChanged(albumControl.rect(Qt.point(0, 0), Qt.point(0, 0)))

                mouse.accepted = true
            }
            onWheel: {
                // 滚动时，激活滚动条显示
                vbar.active = true
                var datla = wheel.angleDelta.y
                if (Qt.ControlModifier & wheel.modifiers) {
                    // 按住ctrl，缩放缩略图
                    var curValue = statusBar.sliderValue
                    if (datla > 0)
                        statusBar.setSliderWidgetValue(curValue + 1)
                    else
                        statusBar.setSliderWidgetValue(curValue - 1)
                } else {
                    // 正常滚动显示缩略图内容
                    if( datla > 0 ) {
                        vbar.decrease()
                    } else {
                        vbar.increase()
                    }
                }
            }

            //橡皮筋控件
            RubberBand {
                id: rubberBand
                visible: theView.inPress
            }

            Timer {
                id: rectScrollTimer
                interval: 100
                running: theView.scrollDirType !== GlobalVar.RectScrollDirType.NoType
                repeat: true
                onTriggered: {
                    // 选择框向下延展滚动
                    if (theView.scrollDirType === GlobalVar.RectScrollDirType.ToBottom) {
                        var newY2 = rubberBand.y2 + theView.rectSelScrollOffset
                        if (newY2 <= theView.listContentHeight) {
                            rubberBand.y2 = newY2
                            theView.contentY = theView.contentY + theView.rectSelScrollOffset + theView.originY
                        } else {
                            // 选择框底部最大值为内容区域底部
                            theView.contentY = theView.listContentHeight - theView.height
                            rubberBand.y2 = theView.listContentHeight
                            rectScrollTimer.stop()
                        }
                    } else if (theView.scrollDirType === GlobalVar.RectScrollDirType.ToTop) {
                        if (rubberBand.top() < 0) {
                            rectScrollTimer.stop()
                            return
                        }

                        // 矩形顶部向上延展
                        if (theView.contentY <= rubberBand.bottom() || rubberBand.bottom() === rubberBand.top()) {
                            var newTop = rubberBand.top() - theView.rectSelScrollOffset
                            if (newTop > 0) {
                                rubberBand.y2 = newTop
                                theView.contentY = theView.contentY - theView.rectSelScrollOffset + theView.originY
                            } else {
                                // 选择框顶部最小值为内容区域顶部
                                rubberBand.y2 = 0
                                theView.contentY = 0 + theView.originY

                                rectScrollTimer.stop()
                            }
                        } else {
                            // 矩形框底部向上收缩
                            var newBottom = rubberBand.bottom() - theView.rectSelScrollOffset
                            if (newBottom > rubberBand.top()) {
                                rubberBand.y2 = newBottom
                                theView.contentY = theView.contentY - theView.rectSelScrollOffset + theView.originY
                            } else {
                                var srcollOffset = Math.abs(rubberBand.y1 - rubberBand.y2)
                                rubberBand.y2 = rubberBand.y1
                                theView.contentY = theView.contentY - srcollOffset + theView.originY
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: global
        onSigPageUp: {
            if (visible) {
                vbar.active = true
                vbar.decrease()
            }
        }

        onSigPageDown: {
            if (visible) {
                vbar.active = true
                vbar.increase()
            }
        }

        //处理全选消息
        onSigSelectAll: {
            if (visible) {
                var paths = []
                for (var i = 0; i != theModel.count; ++i) {
                    var array = theModel.get(i).items
                    //清空
                    theModel.selectedPathObjs[i].paths.length = 0
                    for (var j = 0; j < array.count; ++j) {
                        paths.push(array.get(j).url)
                        theModel.selectedPathObjs[i].paths[j] = array.get(j).url
                    }
                }
                global.selectedPaths = paths
            }
        }
    }

    //已导入列表代理控件
    Component {
        id: importedListDelegate

        Control {
            id :importControl
            z: 2
            width: theView.width
            height: importedGridView.height + importedListView.listMargin * 2 + importedListView.importCheckboxHeight + spaceRect.height
            property string m_index: index
            property var theViewTitle: global.objIsEmpty(theModel.get(index)) ? "" : theModel.get(index).title //日期标题文本内容
            property var theViewItems: global.objIsEmpty(theModel.get(index)) ? "" : theModel.get(index).items //日期标题对应图片信息链表

            Rectangle {
                id: spaceRect
                width: parent.width
                height: index == 0 ? importedListView.spaceCtrlHeight : 0
            }

            CheckBox {
                id: importedCheckBox
                height: importedListView.importCheckboxHeight
                visible: selectedPaths.length > 0
                checked: importedGridView.haveSelectAll
                font: DTK.fontManager.t6
                anchors.top: (index == 0 ? spaceRect.bottom : spaceRect.top)
                //text: qsTr("Imported in %1   %2").arg(theViewTitle).arg(importedGridView.count() === 1 ? qsTr("1 item") : qsTr("%1 items").arg(importedGridView.count()))
                onClicked: {
                    if(checked) {
                        importedGridView.selectAll(true)
                    } else {
                        importedGridView.selectAll(false)
                    }
                }
                Connections {
                    target: importedListView
                    onSigListViewPressed: {
                        var object = importedCheckBox.mapFromGlobal(x,y)
                        if (importedCheckBox.contains(object)) {
                            checkBoxClicked = true
                            if (importedCheckBox.checkState === Qt.Checked) {
                                importedCheckBox.checkState = Qt.Unchecked
                                importedGridView.selectAll(false)
                            } else {
                                importedCheckBox.checkState = Qt.Checked
                                importedGridView.selectAll(true)
                            }
                        }
                    }

                    onSigListViewReleased: {
                        checkBoxClicked = false
                    }
                }
            }
            Label {
                anchors.left :importedCheckBox.visible ? importedCheckBox.right : parent.left
                anchors.top :importedCheckBox.top
                topPadding: 1
                font: DTK.fontManager.t6
                id: importedLabel
                text: qsTr("Imported on") + " " + theViewTitle + " " + (importedGridView.count() === 1 ? qsTr("1 item") : qsTr("%1 items").arg(importedGridView.count()))
            }

            //缩略图网格表
            ThumbnailListView {
                id: importedGridView
                viewTitle: theViewTitle
                anchors.left: parent.left
                anchors.top: importedCheckBox.bottom
                anchors.topMargin: importedListView.listMargin
                anchors.bottomMargin: importedListView.listMargin
                width: parent.width
                height: Math.abs(Math.ceil(importedGridView.count() / Math.floor((parent.width) / itemWidth)) * itemHeight)

                enableWheel: false

                // 装载数据
                thumbnailListModel: {
                    theViewItems
                }

                Connections {
                    target: rubberBand
                    onRectSelChanged: {
                        var pos1 = theMouseArea.mapToItem(importedGridView, rubberBand.left(), rubberBand.top())
                        var pos2 = theMouseArea.mapToItem(importedGridView, rubberBand.right(), rubberBand.bottom())
                        var rectsel = albumControl.rect(pos1, pos2)
                        var rectList = Qt.rect(0, 0, importedGridView.width, importedGridView.height)
                        var rect = albumControl.intersected(rectList, rectsel)
                        var bDetectMousePrees = albumControl.manhattanLength(pos1, pos2) < 3 // 识别此次框选事件是否为鼠标点击事件，以便在列表控件处理ctrl按键相关的操作
                        importedGridView.flushRectSel(rect.x, rect.y, rect.width, rect.height, theMouseArea.ctrlPressed, bDetectMousePrees, theView.inPress)
                    }
                }

                // 监听缩略图子控件选中状态，一旦改变，更新已导入视图所有选中路径
                Connections {
                    target: importedGridView
                    onSelectedChanged: {
                        theModel.selectedPathObjs[m_index].paths = importedGridView.selectedPaths
                        updateSelectedPaths()
                    }
                }

                Connections {
                    target: importedListView
                    onSigUnSelectAll: {
                        importedGridView.selectAll(false)
                    }
                }

                Connections {
                    target: theView
                    onDbClicked: {
                        var openPaths = importedGridView.allOriginUrls()
                        if (openPaths.indexOf(url) !== -1)
                            importedGridView.executeViewImage()
                    }
                }

                Component.onCompleted: {
                    importedGridView.initIsm(theModel.selectedPathObjs[index].paths)
                }
            }
        }
    }
}
