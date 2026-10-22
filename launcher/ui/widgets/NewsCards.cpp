// SPDX-License-Identifier: GPL-3.0-only

#include "NewsCards.h"

#include <QEvent>
#include <functional>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTextDocumentFragment>
#include <QVBoxLayout>

namespace {
constexpr int MaxCards = 3;

// accent colors cycled through the cards, like the coloured headers in the mockup
const char* const Accents[] = { "#6c4cf0", "#3c8527", "#c27c0e" };

class NewsCard : public QFrame {
   public:
    NewsCard(const NewsEntryPtr& entry, const QString& accent, std::function<void()> onClick, QWidget* parent)
        : QFrame(parent), m_onClick(std::move(onClick))
    {
        setObjectName("newsCard");
        setCursor(Qt::PointingHandCursor);
        setMinimumWidth(200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setToolTip(entry->link);

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 14);
        layout->setSpacing(8);

        auto band = new QFrame(this);
        band->setFixedHeight(6);
        band->setStyleSheet(QString("background-color: %1; border-top-left-radius: 12px; border-top-right-radius: 12px;").arg(accent));
        layout->addWidget(band);

        auto title = new QLabel(entry->title, this);
        title->setObjectName("newsTitle");
        title->setWordWrap(true);
        title->setContentsMargins(16, 4, 16, 0);
        layout->addWidget(title);

        // content is HTML: keep only the text and cut it to a short teaser
        auto text = QTextDocumentFragment::fromHtml(entry->content).toPlainText().simplified();
        if (text.size() > 110)
            text = text.left(110).trimmed() + "…";
        auto teaser = new QLabel(text, this);
        teaser->setObjectName("newsTeaser");
        teaser->setWordWrap(true);
        teaser->setContentsMargins(16, 0, 16, 0);
        layout->addWidget(teaser);
        layout->addStretch(1);
    }

   protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
            m_onClick();
        QFrame::mouseReleaseEvent(event);
    }

   private:
    std::function<void()> m_onClick;
};
}  // namespace

NewsCards::NewsCards(QWidget* parent) : QFrame(parent)
{
    setObjectName("newsCards");

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 20);
    layout->setSpacing(12);

    auto header = new QHBoxLayout();
    m_header = new QLabel(this);
    m_header->setObjectName("newsHeader");
    header->addWidget(m_header);
    header->addStretch(1);
    m_moreButton = new QPushButton(this);
    m_moreButton->setObjectName("newsMoreButton");
    m_moreButton->setCursor(Qt::PointingHandCursor);
    m_moreButton->setFlat(true);
    header->addWidget(m_moreButton);
    layout->addLayout(header);

    m_row = new QHBoxLayout();
    m_row->setSpacing(14);
    layout->addLayout(m_row);

    connect(m_moreButton, &QPushButton::clicked, this, &NewsCards::moreClicked);

    setStyleSheet(
        "#newsCards { background-color: #1d1a26; border: 1px solid #2c2838; border-radius: 18px; }"
        "#newsHeader { color: white; font-size: 16px; font-weight: 700; }"
        "#newsMoreButton { color: rgba(255,255,255,70%); background: transparent; border: none; font-size: 13px; }"
        "#newsMoreButton:hover { color: white; }"
        "#newsCard { background-color: #14121a; border: 1px solid #2c2838; border-radius: 12px; }"
        "#newsCard:hover { border-color: #6c4cf0; }"
        "#newsTitle { color: white; font-size: 14px; font-weight: 600; }"
        "#newsTeaser, #newsEmpty { color: rgba(255,255,255,60%); font-size: 12px; }");

    rebuild();
}

void NewsCards::setEntries(const QList<NewsEntryPtr>& entries)
{
    m_entries = entries;
    rebuild();
}

void NewsCards::setLoading(bool loading)
{
    m_loading = loading;
    rebuild();
}

void NewsCards::rebuild()
{
    m_header->setText(tr("News"));
    m_moreButton->setText(tr("All news →"));
    m_moreButton->setVisible(!m_entries.isEmpty());

    while (auto item = m_row->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    if (m_entries.isEmpty()) {
        auto empty = new QLabel(m_loading ? tr("Loading news...") : tr("No news available."), this);
        empty->setObjectName("newsEmpty");
        m_row->addWidget(empty);
        m_row->addStretch(1);
        return;
    }

    for (int i = 0; i < std::min<int>(MaxCards, m_entries.size()); i++) {
        auto entry = m_entries[i];
        m_row->addWidget(new NewsCard(entry, Accents[i % std::size(Accents)], [this, entry] { emit entryClicked(entry); }, this));
    }
}

void NewsCards::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        rebuild();
    QFrame::changeEvent(event);
}
