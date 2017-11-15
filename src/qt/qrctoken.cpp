#include "qrctoken.h"
#include "ui_qrctoken.h"
#include "tokenitemmodel.h"
#include "walletmodel.h"

#include <QPainter>
#include <QAbstractItemDelegate>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QActionGroup>

#define DECORATION_SIZE 54
#define SYMBOL_WIDTH 80
#define MARGIN 5

class TokenViewDelegate : public QAbstractItemDelegate
{
public:

    TokenViewDelegate(QObject *parent) :
        QAbstractItemDelegate(parent)
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const
    {
        painter->save();

        QIcon tokenIcon;
        QString tokenSymbol = index.data(TokenItemModel::SymbolRole).toString();
        QString tokenBalance = index.data(TokenItemModel::BalanceRole).toString();

        QRect mainRect = option.rect;
        mainRect.setWidth(option.rect.width());
        QColor rowColor = index.row() % 2 ? QColor("#ededed") : QColor("#e3e3e3");
        painter->fillRect(mainRect, rowColor);

        bool selected = option.state & QStyle::State_Selected;
        if(selected)
        {
            painter->fillRect(mainRect,QColor("#cbcbcb"));
        }

        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        tokenIcon.paint(painter, decorationRect);

        QColor foreground = option.palette.color(QPalette::Text);
        painter->setPen(foreground);
        QRect tokenSymbolRect(decorationRect.right() + MARGIN, mainRect.top(), SYMBOL_WIDTH, DECORATION_SIZE);
        painter->drawText(tokenSymbolRect, Qt::AlignLeft|Qt::AlignVCenter, tokenSymbol);

        int amountWidth = (mainRect.width() - decorationRect.width() - 2 * MARGIN - tokenSymbolRect.width());
        QRect tokenBalanceRect(tokenSymbolRect.right(), mainRect.top(), amountWidth, DECORATION_SIZE);
        painter->drawText(tokenBalanceRect, Qt::AlignRight|Qt::AlignVCenter, tokenBalance);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }
};

QRCToken::QRCToken(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QRCToken),
    m_model(0),
    m_clientModel(0),
    m_tokenModel(0),
    m_tokenDelegate(0)
{
    ui->setupUi(this);

    m_sendTokenPage = new SendTokenPage(this);
    m_receiveTokenPage = new ReceiveTokenPage(this);
    m_addTokenPage = new AddTokenPage(this);
    m_tokenDelegate = new TokenViewDelegate(this);

    ui->stackedWidget->addWidget(m_sendTokenPage);
    ui->stackedWidget->addWidget(m_receiveTokenPage);
    ui->stackedWidget->addWidget(m_addTokenPage);

    ui->tokensList->setItemDelegate(m_tokenDelegate);

    QActionGroup *actionGroup = new QActionGroup(this);
    m_sendAction = new QAction(tr("Send"), actionGroup);
    m_receiveAction = new QAction(tr("Receive"), actionGroup);
    m_addTokenAction = new QAction(tr("AddToken"), actionGroup);
    actionGroup->setExclusive(true);

    m_sendAction->setCheckable(true);
    m_receiveAction->setCheckable(true);
    m_addTokenAction->setCheckable(true);

    ui->sendButton->setDefaultAction(m_sendAction);
    ui->receiveButton->setDefaultAction(m_receiveAction);
    ui->addTokenButton->setDefaultAction(m_addTokenAction);

    connect(m_sendAction, SIGNAL(triggered()), this, SLOT(on_goToSendTokenPage()));
    connect(m_receiveAction, SIGNAL(triggered()), this, SLOT(on_goToReceiveTokenPage()));
    connect(m_addTokenAction, SIGNAL(triggered()), this, SLOT(on_goToAddTokenPage()));
    connect(ui->tokensList, SIGNAL(clicked(QModelIndex)), this, SLOT(on_currentTokenChanged(QModelIndex)));

    on_goToSendTokenPage();
}

QRCToken::~QRCToken()
{
    delete ui;
}

void QRCToken::setModel(WalletModel *_model)
{
    m_model = _model;
    m_addTokenPage->setModel(m_model);
    m_sendTokenPage->setModel(m_model);
    m_tokenModel = m_model->getTokenItemModel();
    ui->tokensList->setModel(m_tokenModel);
    connect(m_tokenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), SLOT(on_dataChanged(QModelIndex,QModelIndex,QVector<int>)));
}

void QRCToken::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    m_sendTokenPage->setClientModel(_clientModel);
    m_addTokenPage->setClientModel(_clientModel);
}

void QRCToken::on_goToSendTokenPage()
{
    m_sendAction->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);
}

void QRCToken::on_goToReceiveTokenPage()
{
    m_receiveAction->setChecked(true);
    ui->stackedWidget->setCurrentIndex(1);
}

void QRCToken::on_goToAddTokenPage()
{
    m_addTokenAction->setChecked(true);
    ui->stackedWidget->setCurrentIndex(2);
}

void QRCToken::on_currentTokenChanged(QModelIndex index)
{
    if(m_tokenModel)
    {
        m_selectedTokenHash = m_tokenModel->data(index, TokenItemModel::Hash).toString();
        std::string address = m_tokenModel->data(index, TokenItemModel::AddressRole).toString().toStdString();
        std::string symbol = m_tokenModel->data(index, TokenItemModel::SymbolRole).toString().toStdString();
        std::string sender = m_tokenModel->data(index, TokenItemModel::SenderRole).toString().toStdString();
        int8_t decimals = m_tokenModel->data(index, TokenItemModel::DecimalsRole).toInt();
        std::string balance = m_tokenModel->data(index, TokenItemModel::RawBalanceRole).toString().toStdString();
        m_sendTokenPage->setTokenData(address, sender, symbol, decimals, balance);
    }
}

void QRCToken::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(m_tokenModel)
    {
        QString tokenHash = m_tokenModel->data(topLeft, TokenItemModel::Hash).toString();
        if(tokenHash == m_selectedTokenHash)
        {
            on_currentTokenChanged(topLeft);
        }
    }
}