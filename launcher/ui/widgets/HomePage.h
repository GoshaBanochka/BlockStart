// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPixmap>
#include <QWidget>

class AccountCard;
class BaseInstance;
class NewsCards;
class QLabel;
class QPushButton;

// Big "hero" card for the selected instance: name, version, notes and a large Play button.
class HomePage : public QWidget {
    Q_OBJECT
   public:
    explicit HomePage(QWidget* parent = nullptr);

    // nullptr means no instance is selected
    void setInstance(BaseInstance* instance);
    // re-read running state, version and notes of the current instance
    void refresh();

    AccountCard* accountCard() const { return m_accountCard; }
    NewsCards* newsCards() const { return m_newsCards; }

   signals:
    void playClicked();
    void editClicked();
    void chooseInstanceClicked();
    void createInstanceClicked();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void loadBackground();

    BaseInstance* m_instance = nullptr;

    class HeroCard* m_card = nullptr;
    AccountCard* m_accountCard = nullptr;
    NewsCards* m_newsCards = nullptr;
    QLabel* m_icon = nullptr;
    QLabel* m_overline = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_badge = nullptr;
    QLabel* m_description = nullptr;

    QPushButton* m_playButton = nullptr;
    QLabel* m_playTitle = nullptr;
    QLabel* m_playSubtitle = nullptr;
    QPushButton* m_secondaryButton = nullptr;
    QPushButton* m_switchButton = nullptr;

    QString m_backgroundPath;
};
