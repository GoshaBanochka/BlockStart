// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QFrame>

#include "news/NewsEntry.h"

class QHBoxLayout;
class QLabel;
class QPushButton;

// Row of cards with the latest launcher news on the home page.
class NewsCards : public QFrame {
    Q_OBJECT
   public:
    explicit NewsCards(QWidget* parent = nullptr);

    void setEntries(const QList<NewsEntryPtr>& entries);
    void setLoading(bool loading);

   signals:
    void entryClicked(NewsEntryPtr entry);
    void moreClicked();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void rebuild();

    QList<NewsEntryPtr> m_entries;
    bool m_loading = false;
    QLabel* m_header = nullptr;
    QPushButton* m_moreButton = nullptr;
    QHBoxLayout* m_row = nullptr;
};
