// SPDX-License-Identifier: GPL-3.0-only

#include "AccountCard.h"

#include <QEvent>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr int SkinScale = 6;
}

QImage AccountCard::renderSkinFront(const QImage& skin, bool slim)
{
    const bool legacy = skin.height() < 64;  // 64x32 skins have no separate left limbs or overlays
    const int armWidth = slim ? 3 : 4;

    QImage out(16, 32, QImage::Format_ARGB32);
    out.fill(Qt::transparent);
    QPainter p(&out);

    // draws a part of the texture, mirrored when a legacy skin reuses the right limb for the left one
    auto part = [&](int sx, int sy, int w, int h, int dx, int dy, bool mirror = false) {
        auto piece = skin.copy(sx, sy, w, h);
        if (mirror)
            piece = piece.flipped(Qt::Horizontal);
        p.drawImage(dx, dy, piece);
    };

    // base layer: head, body, right arm/leg (viewer's left), left arm/leg (viewer's right)
    part(8, 8, 8, 8, 4, 0);
    part(20, 20, 8, 12, 4, 8);
    part(44, 20, armWidth, 12, 4 - armWidth, 8);
    part(4, 20, 4, 12, 4, 20);
    if (legacy) {
        part(44, 20, armWidth, 12, 12, 8, true);
        part(4, 20, 4, 12, 8, 20, true);
    } else {
        part(36, 52, armWidth, 12, 12, 8);
        part(20, 52, 4, 12, 8, 20);
    }

    // second layer: hat, jacket, sleeves, pants
    part(40, 8, 8, 8, 4, 0);
    if (!legacy) {
        part(20, 36, 8, 12, 4, 8);
        part(44, 36, armWidth, 12, 4 - armWidth, 8);
        part(52, 52, armWidth, 12, 12, 8);
        part(4, 36, 4, 12, 4, 20);
        part(4, 52, 4, 12, 8, 20);
    }
    return out;
}

AccountCard::AccountCard(QWidget* parent) : QFrame(parent)
{
    setObjectName("accountCard");
    setFixedWidth(260);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 28, 24, 24);
    layout->setSpacing(10);

    m_skin = new QLabel(this);
    m_skin->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    m_skin->setMinimumHeight(32 * SkinScale);
    layout->addWidget(m_skin);

    auto underline = new QFrame(this);
    underline->setObjectName("accountUnderline");
    underline->setFixedSize(140, 5);
    layout->addWidget(underline, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    m_name = new QLabel(this);
    m_name->setObjectName("accountName");
    m_name->setAlignment(Qt::AlignCenter);
    m_name->setWordWrap(true);
    layout->addWidget(m_name);

    m_type = new QLabel(this);
    m_type->setObjectName("accountType");
    m_type->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_type);

    layout->addStretch(1);

    m_manageButton = new QPushButton(this);
    m_manageButton->setObjectName("accountManageButton");
    m_manageButton->setCursor(Qt::PointingHandCursor);
    m_manageButton->setMinimumHeight(38);
    layout->addWidget(m_manageButton);

    m_switchButton = new QPushButton(this);
    m_switchButton->setObjectName("accountSwitchButton");
    m_switchButton->setCursor(Qt::PointingHandCursor);
    m_switchButton->setMinimumHeight(38);
    layout->addWidget(m_switchButton);

    connect(m_manageButton, &QPushButton::clicked, this, &AccountCard::manageClicked);

    setStyleSheet(
        "#accountCard { background-color: #1d1a26; border: 1px solid #2c2838; border-radius: 18px; }"
        "#accountUnderline { background-color: #6c4cf0; border-radius: 2px; }"
        "#accountName { color: white; font-size: 20px; font-weight: 700; }"
        "#accountType { color: rgba(255,255,255,55%); font-size: 12px; }"
        "#accountManageButton { color: white; background-color: #6c4cf0; border: none; border-radius: 8px;"
        " font-size: 13px; font-weight: 600; }"
        "#accountManageButton:hover { background-color: #7d60f5; }"
        "#accountSwitchButton { color: white; background-color: #14121a; border: 1px solid #2c2838; border-radius: 8px;"
        " font-size: 13px; }"
        "#accountSwitchButton:hover { background-color: #26222f; }"
        "#accountSwitchButton::menu-indicator { subcontrol-position: right center; right: 12px; }");

    refresh();
}

void AccountCard::setAccount(MinecraftAccountPtr account)
{
    m_account = account;
    refresh();
}

void AccountCard::setSwitchMenu(QMenu* menu)
{
    m_switchButton->setMenu(menu);
}

void AccountCard::refresh()
{
    const bool hasAccount = m_account && !m_account->profileName().isEmpty();

    QImage body;
    if (hasAccount) {
        const auto& skin = m_account->accountData()->minecraftProfile.skin;
        QImage texture;
        if (texture.loadFromData(skin.data, "PNG") && texture.width() == 64)
            body = renderSkinFront(texture, skin.variant.compare("SLIM", Qt::CaseInsensitive) == 0);
    }
    if (!body.isNull()) {
        // nearest-neighbour upscale keeps the pixel-art look
        m_skin->setPixmap(QPixmap::fromImage(body.scaled(body.size() * SkinScale, Qt::KeepAspectRatio, Qt::FastTransformation)));
    } else {
        m_skin->setPixmap(QIcon(":/icons/multimc/scalable/instances/steve.svg").pixmap(128, 128));
    }

    m_switchButton->setVisible(hasAccount);
    if (hasAccount) {
        QString type;
        switch (m_account->accountType()) {
            case AccountType::MSA:
                type = tr("Microsoft account");
                break;
            case AccountType::Offline:
                type = tr("Offline account");
                break;
            case AccountType::Elyby:
                type = tr("Ely.by account");
                break;
            default:
                type = tr("Custom account");
                break;
        }
        m_name->setText(m_account->profileName());
        m_type->setText(m_account->isInUse() ? tr("%1 · in use").arg(type) : type);
        m_manageButton->setText(tr("Manage accounts"));
        m_switchButton->setText(tr("Switch account"));
    } else {
        m_name->setText(tr("No account"));
        m_type->setText(tr("Add a Microsoft account to play online"));
        m_type->setWordWrap(true);
        m_manageButton->setText(tr("Add account"));
    }
}

void AccountCard::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        refresh();
    QFrame::changeEvent(event);
}
