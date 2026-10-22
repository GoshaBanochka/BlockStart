// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QFrame>

#include "minecraft/auth/MinecraftAccount.h"

class QLabel;
class QMenu;
class QPushButton;

// Home page card with the default account: full-body skin preview, name and account actions.
class AccountCard : public QFrame {
    Q_OBJECT
   public:
    explicit AccountCard(QWidget* parent = nullptr);

    // null means there is no default account
    void setAccount(MinecraftAccountPtr account);
    // menu used by the "switch account" button (the launcher's accounts menu)
    void setSwitchMenu(QMenu* menu);

    // front view of a Minecraft skin texture (64x64 or legacy 64x32) as a 16x32 image
    static QImage renderSkinFront(const QImage& skin, bool slim);

   signals:
    void manageClicked();

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void refresh();

    MinecraftAccountPtr m_account;
    QLabel* m_skin = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_type = nullptr;
    QPushButton* m_manageButton = nullptr;
    QPushButton* m_switchButton = nullptr;
};
