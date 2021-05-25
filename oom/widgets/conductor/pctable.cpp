#include "pctable.h"
#include "pctablemodel.h"
#include <QTableView>
#include <QHeaderView>
#include <QModelIndex>
#include <QDrag>
#include <QPainter>
#include <QPoint>
#include <QBrush>
#include <QPen>
#include <QMimeData>

ProgramChangeTable::ProgramChangeTable(QWidget *parent) : QTableView(parent)
{
	setDragEnabled(true);
	setAcceptDrops(true);
	setCornerButtonEnabled(false);
	verticalHeader()->hide();
	//setAutoFillBackground(true);
	setTextElideMode(Qt::ElideNone);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setAlternatingRowColors(true);
	setWordWrap(false);
	setShowGrid(false);
}

void ProgramChangeTable::dropEvent(QDropEvent *evt)
{
	if (evt->mimeData()->hasText())
	{
		evt->setDropAction(Qt::MoveAction);
		ProgramChangeTableModel* m = (ProgramChangeTableModel*) model();
		QRect r = frameRect();
		QModelIndex index = indexAt(evt->pos());
		QString t = evt->mimeData()->text();
		int srow = t.toInt();
		int drow = index.row();
		if (drow != -1 && drow != srow/* && r.contains(evt->pos())*/)
		{
			QList<QStandardItem*> dragItems = m->takeRow(srow);
			m->insertRow(index.row(), dragItems);
			emit rowOrderChanged();
			resizeRowsToContents();
		}
	}
	else
	{
		evt->ignore();
	}
}

void ProgramChangeTable::mousePressEvent(QMouseEvent *evt)
{
	QModelIndex modidx = indexAt(evt->pos());
	QRect arect = visualRect(modidx);
	arect.setWidth(20);
	if (evt->button() == Qt::LeftButton && !arect.contains(evt->pos()) && ((QInputEvent*)evt)->modifiers() & Qt::ShiftModifier)
	{
		//printf("Mouse Press Event fired\n");
		QTableView::mousePressEvent(evt);
		QModelIndex index = currentIndex();
		QString plainText = QString::number(index.row());
		QMimeData *mimeData = new QMimeData;
		mimeData->setText(plainText);
		QDrag* drag = new QDrag(this);
		drag->setMimeData(mimeData);
		drag->setHotSpot(evt->pos() - rect().topLeft());
		drag->start(Qt::MoveAction);
	}
	else
	{
		QTableView::mousePressEvent(evt);
	}
}

void ProgramChangeTable::dragEnterEvent(QDragEnterEvent* evt)
{
	if (evt->mimeData()->hasText())
	{
		evt->acceptProposedAction();
		//printf("dragEnterEvent fired\n");
	}
	else
		evt->ignore();
}

void ProgramChangeTable::dragMoveEvent(QDragMoveEvent* evt)
{
	dropSite = evt->answerRect();

	ProgramChangeTableModel* m = (ProgramChangeTableModel*) model();
	m->emit_layoutChanged();
}

/*
void ProgramChangeTable::paintEvent(QPaintEvent* event)
{
	QTableView::paintEvent(event);
	QPainter painter(viewport());
	int x, y, w, h;
	dropSite.getRect(&x, &y, &w, &h);
	QPoint point(x, y);
	QModelIndex modidx = indexAt(point);
	QRect arect = visualRect(modidx);
	int b = arect.y();
	QBrush brush(Qt::black, Qt::Dense4Pattern);
	QPen pen;
	pen.setWidth(2);
	pen.setBrush(brush);
	painter.setPen(pen);
	painter.drawLine(0, b, width() - 40, b);
	event->accept();
}
*/

QList<int> ProgramChangeTable::getSelectedRows()
{
	QList<int> rv;
	QItemSelectionModel* smodel = selectionModel();
	if (smodel->hasSelection())
	{
		QModelIndexList indexes = smodel->selectedRows();
		QList<QModelIndex>::const_iterator id;
		for (id = indexes.constBegin(); id != indexes.constEnd(); ++id)
		{
			int row = (*id).row();
			rv.append(row);
		}
	}
	return rv;
}

void HTMLDelegate::paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex &index) const/*{{{*/
{
	//printf("HTMLDelegate::paint() called\n");
	QStyleOptionViewItem options = option;
	initStyleOption(&options, index);

	painter->save();

	QTextDocument doc;
	doc.setHtml(index.data().toString());

	// Call this to get the focus rect and selection background.
	options.text = "";
	options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

	// Draw using our rich text document. 
	painter->translate(options.rect.left(), options.rect.top());
	QRect clip(0, 0, options.rect.width(), options.rect.height());
	doc.drawContents(painter, clip);

	painter->restore();
}

QSize HTMLDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QStyleOptionViewItem options = option;
	initStyleOption(&options, index);

	QTextDocument doc;
	doc.setHtml(options.text);
	doc.setTextWidth(options.rect.width());
	return QSize(doc.idealWidth(), doc.size().height());
}/*}}}*/
