#include "book_gui.h"
#include "interactive_plot.h"
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>

const int UPDATE_INTERVAL = 1000;

BookGui::BookGui(QWidget *parent)
        : QWidget(parent),
          m_price_plot(new InteractivePlot(this)),
          m_pnl_plot(new InteractivePlot(this)),
          m_update_timer(new QTimer(this)),
          m_horizontal_scroll_bar(new QScrollBar(Qt::Horizontal, this)),
          m_start_button(new QPushButton("Start", this)),
          m_stop_button(new QPushButton("Stop", this)),
          m_progress_bar(new QProgressBar(this)),
          m_trade_log_table(new QTableWidget(this)),
          m_button_layout(new QHBoxLayout()),
          m_auto_scroll(true),
          m_user_scrolling(false) {

    setup_plots();
    setup_scroll_bar();
    setup_buttons();
    setup_trade_log();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_button_layout->addWidget(m_start_button);
    m_button_layout->addWidget(m_stop_button);

    QPushButton *resetZoomButton = new QPushButton("Reset Zoom", this);
    connect(resetZoomButton, &QPushButton::clicked, this, &BookGui::reset_zoom);
    m_button_layout->addWidget(resetZoomButton);

    QHBoxLayout *chartLayout = new QHBoxLayout();
    chartLayout->addWidget(m_price_plot, 1);
    chartLayout->addWidget(m_pnl_plot, 1);

    mainLayout->addLayout(m_button_layout);
    mainLayout->addLayout(chartLayout, 7);
    mainLayout->addWidget(m_horizontal_scroll_bar);
    mainLayout->addWidget(m_trade_log_table, 3);
    mainLayout->addWidget(m_progress_bar);

    setLayout(mainLayout);

    connect(m_update_timer, &QTimer::timeout, this, &BookGui::update_plots);
    m_update_timer->start(UPDATE_INTERVAL);

    resize(1200, 800);
}

void BookGui::setup_plots() {
    m_bid_graph = m_price_plot->addGraph();
    m_ask_graph = m_price_plot->addGraph();

    m_bid_graph->setPen(QPen(QColor(4, 165, 229), 2));
    m_ask_graph->setPen(QPen(QColor(136, 57, 239), 2));

    m_price_plot->legend->setVisible(false);

    m_bid_graph->setAdaptiveSampling(true);
    m_ask_graph->setAdaptiveSampling(true);

    m_price_plot->setNotAntialiasedElements(QCP::aeAll);

    QSharedPointer<QCPAxisTickerDateTime> dateTimeTicker(new QCPAxisTickerDateTime);
    dateTimeTicker->setDateTimeFormat("hh:mm:ss");
    m_price_plot->xAxis->setTicker(dateTimeTicker);

    connect(m_price_plot, &InteractivePlot::userInteracted, this, &BookGui::on_user_interacted);

    m_pnl_graph = m_pnl_plot->addGraph();
    m_pnl_graph->setPen(QPen(QColor(0, 255, 0), 2));

    m_pnl_plot->legend->setVisible(false);

    m_pnl_graph->setAdaptiveSampling(true);

    m_pnl_plot->setNotAntialiasedElements(QCP::aeAll);

    m_pnl_plot->xAxis->setTicker(dateTimeTicker);

    connect(m_pnl_plot, &InteractivePlot::userInteracted, this, &BookGui::on_user_interacted);

    style_plot(m_price_plot);
    style_plot(m_pnl_plot);
}

void BookGui::style_plot(QCustomPlot *plot) {
    plot->setBackground(QBrush(Qt::black));
    plot->xAxis->setBasePen(QPen(Qt::white, 1));
    plot->yAxis->setBasePen(QPen(Qt::white, 1));
    plot->xAxis->setTickPen(QPen(Qt::white, 1));
    plot->yAxis->setTickPen(QPen(Qt::white, 1));
    plot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    plot->yAxis->setSubTickPen(QPen(Qt::white, 1));
    plot->xAxis->setTickLabelColor(Qt::white);
    plot->yAxis->setTickLabelColor(Qt::white);
    plot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    plot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    plot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridVisible(true);
    plot->yAxis->grid()->setSubGridVisible(true);
    plot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    plot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    plot->xAxis->setLabel("Time");
    plot->yAxis->setLabel("Price");
    plot->xAxis->setLabelColor(Qt::white);
    plot->yAxis->setLabelColor(Qt::white);
}

void BookGui::add_data_point(qint64 timestamp, double bestBid, double bestAsk, double pnl) {
    double timeInSeconds = timestamp / 1000.0;

    m_bid_graph->addData(timeInSeconds, bestBid);
    m_ask_graph->addData(timeInSeconds, bestAsk);
    m_pnl_graph->addData(timeInSeconds, pnl);

    if (m_bid_graph->data()->size() == 1) {
        m_price_plot->xAxis->setRange(timeInSeconds, timeInSeconds + 1.0);
        m_pnl_plot->xAxis->setRange(timeInSeconds, timeInSeconds + 1.0);

        m_price_plot->yAxis->setRange(bestBid - 1, bestAsk + 1);
        m_pnl_plot->yAxis->setRange(pnl - 1, pnl + 1);
    }

    update_scroll_bar();

    update_plots();
}

void BookGui::update_plots() {
    if (isVisible()) {
        update_price_plot();
        update_pnl_plot();
    }
}

void BookGui::update_price_plot() {
    if (m_bid_graph->data()->isEmpty() || m_ask_graph->data()->isEmpty()) return;

    if (m_auto_scroll) {
        m_price_plot->yAxis->rescale();
    }

    if (m_auto_scroll) {
        bool foundRange;
        QCPRange keyRange = m_bid_graph->data()->keyRange(foundRange);
        if (foundRange) {
            double keyStart = keyRange.lower;
            double keyEnd = keyRange.upper;

            if (keyEnd - keyStart < 1.0) {
                keyEnd = keyStart + 1.0;
            }

            m_price_plot->xAxis->setRange(keyStart, keyEnd);
        }
    }

    m_price_plot->replot();
}

void BookGui::update_pnl_plot() {
    if (m_pnl_graph->data()->isEmpty()) return;

    if (m_auto_scroll) {
        m_pnl_plot->yAxis->rescale();
    }

    if (m_auto_scroll) {
        bool foundRange;
        QCPRange keyRange = m_pnl_graph->data()->keyRange(foundRange);
        if (foundRange) {
            double keyStart = keyRange.lower;
            double keyEnd = keyRange.upper;

            if (keyEnd - keyStart < 1.0) {
                keyEnd = keyStart + 1.0;
            }

            m_pnl_plot->xAxis->setRange(keyStart, keyEnd);
        }
    }

    m_pnl_plot->replot();
}

void BookGui::setup_scroll_bar() {
    m_horizontal_scroll_bar->setOrientation(Qt::Horizontal);
    m_horizontal_scroll_bar->setFixedHeight(15);
    connect(m_horizontal_scroll_bar, &QScrollBar::valueChanged, this, &BookGui::handle_horizontal_scroll_bar_value_changes);
    connect(m_horizontal_scroll_bar, &QScrollBar::sliderPressed, [this]() {
        m_user_scrolling = true;
    });
    connect(m_horizontal_scroll_bar, &QScrollBar::sliderReleased, [this]() {
        m_user_scrolling = false;
    });
}

void BookGui::update_scroll_bar() {
    bool foundRange;
    QCPRange timeRange = m_bid_graph->data()->keyRange(foundRange);
    if (foundRange) {
        double minTime = timeRange.lower;
        double maxTime = timeRange.upper;

        int minTimeMs = static_cast<int>(minTime * 1000);
        int maxTimeMs = static_cast<int>(maxTime * 1000);

        int pageStepMs = maxTimeMs - minTimeMs;

        if (pageStepMs < 1000) {
            pageStepMs = 1000;
        }

        m_horizontal_scroll_bar->setRange(minTimeMs, maxTimeMs);
        m_horizontal_scroll_bar->setPageStep(pageStepMs);

        if (m_auto_scroll && !m_user_scrolling) {
            m_horizontal_scroll_bar->setValue(maxTimeMs - pageStepMs);
        }
    } else {
        m_horizontal_scroll_bar->setRange(0, 0);
    }
}

void BookGui::handle_horizontal_scroll_bar_value_changes(int value) {
    if (m_user_scrolling) {
        if (m_bid_graph->data()->isEmpty()) return;

        int pageStepMs = m_horizontal_scroll_bar->pageStep();

        double keyStart = value / 1000.0;
        double keyEnd = (value + pageStepMs) / 1000.0;

        m_price_plot->xAxis->setRange(keyStart, keyEnd);
        m_pnl_plot->xAxis->setRange(keyStart, keyEnd);

        m_price_plot->replot();
        m_pnl_plot->replot();

        m_auto_scroll = false;
    }
}

void BookGui::setup_buttons() {
    m_stop_button->setEnabled(false);
    connect(m_start_button, &QPushButton::clicked, this, &BookGui::handle_start_button_click);
    connect(m_stop_button, &QPushButton::clicked, this, &BookGui::handle_stop_button_click);

}

void BookGui::setup_trade_log() {
    m_trade_log_table->setColumnCount(4);
    m_trade_log_table->setHorizontalHeaderLabels({"Timestamp", "Side", "Price", "Size"});
    m_trade_log_table->verticalHeader()->setVisible(false);
    m_trade_log_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trade_log_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_trade_log_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_trade_log_table->setStyleSheet("QTableWidget { border: none; }");
    m_trade_log_table->horizontalHeader()->setStretchLastSection(true);
}

void BookGui::handle_start_button_click() {
    m_start_button->setEnabled(false);
    m_stop_button->setEnabled(true);
    m_auto_scroll = true;
    emit start_backtest();
}

void BookGui::handle_stop_button_click() {
    m_start_button->setEnabled(true);
    m_stop_button->setEnabled(false);
    emit stop_backtest();
}

void BookGui::update_progress(int progress) {
    m_progress_bar->setValue(progress);
}

void BookGui::log_trade(const QString& timestamp, bool is_buy, int32_t price, int size) {
    int row = m_trade_log_table->rowCount();
    m_trade_log_table->insertRow(row);
    m_trade_log_table->setItem(row, 0, new QTableWidgetItem(timestamp));
    m_trade_log_table->setItem(row, 1, new QTableWidgetItem(is_buy ? "Buy" : "Sell"));
    m_trade_log_table->setItem(row, 2, new QTableWidgetItem(QString::number(price)));
    m_trade_log_table->setItem(row, 3, new QTableWidgetItem(QString::number(size)));
    m_trade_log_table->scrollToBottom();
}

void BookGui::on_backtest_finished() {
    qDebug() << "Backtest finished";
    m_start_button->setEnabled(true);
    m_stop_button->setEnabled(false);
}

void BookGui::on_backtest_error(const QString& error_message) {
    QMessageBox::critical(this, "Backtest Error", error_message);
    m_start_button->setEnabled(true);
    m_stop_button->setEnabled(false);
}

void BookGui::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    int plotHeight = (height() - 150) * 0.7;
    m_price_plot->setGeometry(0, 50, width() / 2, plotHeight);
    m_pnl_plot->setGeometry(width() / 2, 50, width() / 2, plotHeight);
    m_horizontal_scroll_bar->setGeometry(0, 50 + plotHeight, width(), 15);
}

void BookGui::on_user_interacted() {
    m_auto_scroll = false;
}

void BookGui::reset_zoom() {
    m_auto_scroll = true;
    m_price_plot->rescaleAxes();
    m_pnl_plot->rescaleAxes();
    update_plots();
}
