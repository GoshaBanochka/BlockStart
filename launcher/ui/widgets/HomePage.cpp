// SPDX-License-Identifier: GPL-3.0-only

#include "HomePage.h"

#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "AccountCard.h"
#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "settings/SettingsObject.h"
#include "BaseInstance.h"
#include "MMCTime.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

// Rounded card that paints the newest instance screenshot (if any) under a dark gradient,
// so white text stays readable whatever the picture is.
class HeroCard : public QWidget {
   public:
    using QWidget::QWidget;

    void setBackground(const QPixmap& pixmap)
    {
        m_background = pixmap;
        update();
    }

   protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QPainterPath clip;
        clip.addRoundedRect(rect(), 18, 18);
        p.setClipPath(clip);

        QLinearGradient base(rect().topLeft(), rect().bottomRight());
        base.setColorAt(0, QColor(0x2a, 0x22, 0x3f));
        base.setColorAt(1, QColor(0x14, 0x12, 0x1c));
        p.fillRect(rect(), base);

        if (!m_background.isNull()) {
            // "cover" scaling: fill the card, crop what does not fit
            auto scaled = m_background.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.drawPixmap((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);
        }

        QLinearGradient shade(rect().topLeft(), rect().topRight());
        shade.setColorAt(0, QColor(12, 10, 18, 235));
        shade.setColorAt(0.55, QColor(12, 10, 18, 170));
        shade.setColorAt(1, QColor(12, 10, 18, m_background.isNull() ? 0 : 90));
        p.fillRect(rect(), shade);

        p.setClipping(false);
        p.setPen(QPen(QColor(255, 255, 255, 25), 1));
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 18, 18);
    }

   private:
    QPixmap m_background;
};

namespace {
// display name of the mod loader component, empty for vanilla
QString loaderName(PackProfile* profile, QString& version)
{
    static const QList<QPair<QString, QString>> loaders = {
        { "net.neoforged", "NeoForge" },          { "net.minecraftforge", "Forge" },
        { "net.fabricmc.fabric-loader", "Fabric" }, { "org.quiltmc.quilt-loader", "Quilt" },
        { "com.mumfrey.liteloader", "LiteLoader" },
    };
    for (auto& [uid, name] : loaders) {
        version = profile->getComponentVersion(uid);
        if (!version.isEmpty())
            return name;
    }
    return {};
}
}  // namespace

HomePage::HomePage(QWidget* parent) : QWidget(parent)
{
    setObjectName("homePage");
    setAttribute(Qt::WA_StyledBackground);

    auto outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);

    outer->setSpacing(18);

    auto top = new QHBoxLayout();
    top->setSpacing(18);
    m_card = new HeroCard(this);
    m_card->setObjectName("heroCard");
    m_card->setMinimumHeight(340);
    top->addWidget(m_card, 1);
    m_accountCard = new AccountCard(this);
    top->addWidget(m_accountCard);
    outer->addLayout(top, 1);

    auto cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(44, 40, 44, 40);
    cardLayout->setSpacing(10);

    auto header = new QHBoxLayout();
    header->setSpacing(16);
    m_icon = new QLabel(m_card);
    m_icon->setFixedSize(64, 64);
    header->addWidget(m_icon, 0, Qt::AlignTop);

    auto titles = new QVBoxLayout();
    titles->setSpacing(2);
    m_overline = new QLabel(m_card);
    m_overline->setObjectName("heroOverline");
    m_title = new QLabel(m_card);
    m_title->setObjectName("heroTitle");
    m_title->setWordWrap(true);
    titles->addWidget(m_overline);
    titles->addWidget(m_title);
    header->addLayout(titles, 1);
    cardLayout->addLayout(header);

    m_badge = new QLabel(m_card);
    m_badge->setObjectName("heroBadge");
    cardLayout->addWidget(m_badge, 0, Qt::AlignLeft);

    m_description = new QLabel(m_card);
    m_description->setObjectName("heroDescription");
    m_description->setWordWrap(true);
    m_description->setMaximumWidth(560);
    m_description->setTextFormat(Qt::PlainText);
    // no alignment flag here: it would break height-for-width and clip the wrapped text
    cardLayout->addWidget(m_description);

    cardLayout->addStretch(1);

    auto buttons = new QHBoxLayout();
    buttons->setSpacing(12);

    // two-line button: big action text and a small status line under it
    m_playButton = new QPushButton(m_card);
    m_playButton->setObjectName("heroPlayButton");
    m_playButton->setCursor(Qt::PointingHandCursor);
    m_playButton->setMinimumSize(240, 64);
    auto playLayout = new QVBoxLayout(m_playButton);
    playLayout->setContentsMargins(24, 8, 24, 8);
    playLayout->setSpacing(0);
    m_playTitle = new QLabel(m_playButton);
    m_playTitle->setObjectName("heroPlayTitle");
    m_playSubtitle = new QLabel(m_playButton);
    m_playSubtitle->setObjectName("heroPlaySubtitle");
    for (auto label : { m_playTitle, m_playSubtitle }) {
        label->setAlignment(Qt::AlignCenter);
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        playLayout->addWidget(label);
    }
    buttons->addWidget(m_playButton);

    m_secondaryButton = new QPushButton(m_card);
    m_secondaryButton->setObjectName("heroSecondaryButton");
    m_secondaryButton->setCursor(Qt::PointingHandCursor);
    m_secondaryButton->setMinimumHeight(64);
    buttons->addWidget(m_secondaryButton);

    m_switchButton = new QPushButton(m_card);
    m_switchButton->setObjectName("heroLinkButton");
    m_switchButton->setCursor(Qt::PointingHandCursor);
    m_switchButton->setFlat(true);
    buttons->addWidget(m_switchButton);
    buttons->addStretch(1);
    cardLayout->addLayout(buttons);

    connect(m_playButton, &QPushButton::clicked, this, [this] {
        if (m_instance)
            emit playClicked();
        else
            emit createInstanceClicked();
    });
    connect(m_secondaryButton, &QPushButton::clicked, this, [this] {
        if (m_instance)
            emit editClicked();
        else
            emit chooseInstanceClicked();
    });
    connect(m_switchButton, &QPushButton::clicked, this, &HomePage::chooseInstanceClicked);

    setStyleSheet(
        "#homePage { background-color: #121018; }"
        "#heroOverline { color: rgba(255,255,255,65%); font-size: 14px; font-weight: 600; }"
        "#heroTitle { color: white; font-size: 34px; font-weight: 800; }"
        "#heroBadge { color: white; background-color: #3c8527; border-radius: 9px; padding: 3px 14px;"
        " font-size: 12px; font-weight: 600; }"
        "#heroDescription { color: rgba(255,255,255,80%); font-size: 13px; }"
        "#heroPlayButton { background-color: #4caf1f; border: none; border-radius: 8px; }"
        "#heroPlayButton:hover { background-color: #5cc22a; }"
        "#heroPlayButton:pressed { background-color: #3d8f18; }"
        "#heroPlayButton[running=\"true\"] { background-color: #c62f26; }"
        "#heroPlayButton[running=\"true\"]:hover { background-color: #db4036; }"
        "#heroPlayButton:disabled { background-color: #5b5866; }"
        "#heroPlayTitle { color: white; font-size: 20px; font-weight: 800; }"
        "#heroPlaySubtitle { color: rgba(255,255,255,85%); font-size: 11px; }"
        "#heroSecondaryButton { color: white; background-color: rgba(255,255,255,10%);"
        " border: 1px solid rgba(255,255,255,18%); border-radius: 8px; padding: 0 22px; font-size: 14px; font-weight: 600; }"
        "#heroSecondaryButton:hover { background-color: rgba(255,255,255,18%); }"
        "#heroLinkButton { color: rgba(255,255,255,75%); background: transparent; border: none; font-size: 13px;"
        " text-decoration: underline; padding: 0 8px; }"
        "#heroLinkButton:hover { color: white; }");

    refresh();
}

void HomePage::setInstance(BaseInstance* instance)
{
    m_instance = instance;
    loadBackground();
    refresh();
}

void HomePage::loadBackground()
{
    // newest screenshot of the instance makes a nice personal background
    QString path;
    if (m_instance) {
        QDir dir(FS::PathCombine(m_instance->gameRoot(), "screenshots"));
        auto files = dir.entryInfoList({ "*.png", "*.jpg" }, QDir::Files, QDir::Time);
        if (!files.isEmpty())
            path = files.first().absoluteFilePath();
    }
    if (path == m_backgroundPath)
        return;
    m_backgroundPath = path;
    m_card->setBackground(path.isEmpty() ? QPixmap() : QPixmap(path));
}

void HomePage::refresh()
{
    const bool hasInstance = m_instance != nullptr;
    m_icon->setVisible(hasInstance);
    m_badge->setVisible(hasInstance);
    m_switchButton->setVisible(hasInstance);

    if (!hasInstance) {
        m_overline->setText(BuildConfig.LAUNCHER_DISPLAYNAME);
        m_title->setText(tr("No instance selected"));
        m_description->setText(tr("An instance is a separate copy of Minecraft with its own version, mods and worlds. "
                                  "Create a new one or pick an existing instance to start playing."));
        m_playTitle->setText(tr("CREATE INSTANCE"));
        m_playSubtitle->setText(tr("Choose a version and mods"));
        m_playButton->setEnabled(true);
        m_playButton->setProperty("running", false);
        m_secondaryButton->setText(tr("Choose instance"));
    } else {
        m_icon->setPixmap(APPLICATION->icons()->getIcon(m_instance->iconKey()).pixmap(64, 64));
        m_title->setText(m_instance->name());

        QString mcVersion;
        QString badge = tr("Vanilla");
        if (auto mc = dynamic_cast<MinecraftInstance*>(m_instance)) {
            auto profile = mc->getPackProfile();
            mcVersion = profile->getComponentVersion("net.minecraft");
            QString loaderVersion;
            auto loader = loaderName(profile, loaderVersion);
            if (!loader.isEmpty())
                badge = QString("%1 %2").arg(loader, loaderVersion);
        }
        m_overline->setText(mcVersion.isEmpty() ? m_instance->typeName() : tr("Minecraft %1").arg(mcVersion));
        m_badge->setText(badge);

        auto notes = m_instance->notes().trimmed();
        if (notes.isEmpty()) {
            if (m_instance->totalTimePlayed() > 0) {
                const bool noDays = APPLICATION->settings()->get("ShowGameTimeWithoutDays").toBool();
                notes = tr("Last played: %1\nTotal time played: %2")
                            .arg(QLocale().toString(QDateTime::fromMSecsSinceEpoch(m_instance->lastLaunch()), QLocale::ShortFormat),
                                 Time::prettifyDuration(m_instance->totalTimePlayed(), noDays));
            } else {
                notes = tr("You have not played this instance yet. Press Play to download the game files and start.");
            }
        } else if (notes.size() > 320) {
            notes = notes.left(320).trimmed() + "…";
        }
        m_description->setText(notes);

        const bool running = m_instance->isRunning();
        m_playTitle->setText(running ? tr("STOP") : (mcVersion.isEmpty() ? tr("PLAY") : tr("PLAY %1").arg(mcVersion)));
        if (running)
            m_playSubtitle->setText(tr("The game is running"));
        else if (m_instance->canLaunch())
            m_playSubtitle->setText(tr("Ready to launch"));
        else
            m_playSubtitle->setText(tr("Can't be launched"));
        m_playButton->setEnabled(running || m_instance->canLaunch());
        m_playButton->setProperty("running", running);
        m_secondaryButton->setText(tr("Mods and settings"));
        m_switchButton->setText(tr("Switch instance"));
    }

    // re-apply the stylesheet so the [running] selector picks up the new state
    m_playButton->style()->unpolish(m_playButton);
    m_playButton->style()->polish(m_playButton);
}

void HomePage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        refresh();
    QWidget::changeEvent(event);
}
