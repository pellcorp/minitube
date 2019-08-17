/* $BEGIN_LICENSE

This file is part of Minitube.
Copyright 2009, Flavio Tordini <flavio.tordini@gmail.com>

Minitube is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Minitube is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Minitube.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

#include "searchview.h"
#include "channelsuggest.h"
#include "constants.h"
#include "fontutils.h"
#include "searchparams.h"
#include "ytsuggester.h"
#ifdef APP_MAC_SEARCHFIELD
#include "searchlineedit_mac.h"
#else
#include "searchlineedit.h"
#endif
#ifdef APP_MAC
#include "macutils.h"
#endif
#ifdef APP_EXTRA
#include "extra.h"
#endif
#ifdef APP_ACTIVATION
#include "activation.h"
#include "activationview.h"
#endif
#include "clickablelabel.h"
#include "iconutils.h"
#include "mainwindow.h"
#include "painterutils.h"

namespace {
const QString recentKeywordsKey = "recentKeywords";
const QString recentChannelsKey = "recentChannels";
} // namespace

SearchView::SearchView(QWidget *parent) : View(parent) {
    setBackgroundRole(QPalette::Base);
    setForegroundRole(QPalette::Text);
    setAutoFillBackground(true);

    const int padding = 30;

    QBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setMargin(padding);
    vLayout->setSpacing(0);

    // hidden message widget
    message = new QLabel(this);
    message->hide();
    vLayout->addWidget(message);

    vLayout->addStretch();

    QBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setAlignment(Qt::AlignCenter);

    vLayout->addLayout(hLayout);

    hLayout->addStretch();

    logo = new ClickableLabel();
    auto setLogoPixmap = [this] {
        logo->setPixmap(IconUtils::pixmap(":/images/app.png", logo->devicePixelRatioF()));
    };
    setLogoPixmap();
    connect(window()->windowHandle(), &QWindow::screenChanged, this, setLogoPixmap);
    connect(logo, &ClickableLabel::clicked, MainWindow::instance(), &MainWindow::visitSite);
    hLayout->addWidget(logo, 0, Qt::AlignTop);
    hLayout->addSpacing(padding);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    hLayout->addLayout(layout);

    QLabel *welcomeLabel = new QLabel();
    auto setupWelcomeLabel = [this, welcomeLabel] {
        QColor titleColor = palette().color(QPalette::WindowText);
        titleColor.setAlphaF(.75);
        int r, g, b, a;
        titleColor.getRgb(&r, &g, &b, &a);
        QString cssColor = QString::asprintf("rgba(%d,%d,%d,%d)", r, g, b, a);
        QString text =
                QString("<h1 style='font-weight:300;color:%1'>").arg(cssColor) +
                tr("Welcome to <a href='%1'>%2</a>")
                        .replace("<a ", "<a style='text-decoration:none; color:palette(text)' ")
                        .arg(Constants::WEBSITE, Constants::NAME) +
                "</h1>";
        welcomeLabel->setText(text);
    };
    setupWelcomeLabel();
    connect(qApp, &QGuiApplication::paletteChanged, this, setupWelcomeLabel);
    welcomeLabel->setOpenExternalLinks(true);
    welcomeLabel->setFont(FontUtils::light(welcomeLabel->font().pointSize()));
    layout->addWidget(welcomeLabel);

    layout->addSpacing(padding / 2);

    //: "Enter", as in "type". The whole phrase says: "Enter a keyword to start watching videos"
    // QLabel *tipLabel = new QLabel(tr("Enter"), this);
    QString tip;
    if (qApp->layoutDirection() == Qt::RightToLeft) {
        tip = tr("to start watching videos.") + " " + tr("a keyword") + " " + tr("Enter");
    } else {
        tip = tr("Enter") + " " + tr("a keyword") + " " + tr("to start watching videos.");
    }

    layout->addSpacing(padding / 2);

    QBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->setAlignment(Qt::AlignVCenter);

#ifdef APP_MAC_SEARCHFIELD
    SearchLineEditMac *slem = new SearchLineEditMac(this);
    queryEdit = slem;
    setFocusProxy(slem);
#else
    SearchLineEdit *sle = new SearchLineEdit(this);
    sle->setFont(FontUtils::medium());
    int tipWidth = sle->fontMetrics().size(Qt::TextSingleLine, tip).width();
    sle->setMinimumWidth(tipWidth + sle->fontMetrics().width('m') * 6);
    sle->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    queryEdit = sle;
#endif

    connect(queryEdit->toWidget(), SIGNAL(search(const QString &)), SLOT(watch(const QString &)));
    connect(queryEdit->toWidget(), SIGNAL(textChanged(const QString &)),
            SLOT(textChanged(const QString &)));
    connect(queryEdit->toWidget(), SIGNAL(textEdited(const QString &)),
            SLOT(textChanged(const QString &)));
    connect(queryEdit->toWidget(), SIGNAL(suggestionAccepted(Suggestion *)),
            SLOT(suggestionAccepted(Suggestion *)));
    queryEdit->setPlaceholderText(tip);

    youtubeSuggest = new YTSuggester(this);
    channelSuggest = new ChannelSuggest(this);
    connect(channelSuggest, SIGNAL(ready(QVector<Suggestion *>)),
            SLOT(onChannelSuggestions(QVector<Suggestion *>)));
    searchTypeChanged(0);

    searchLayout->addWidget(queryEdit->toWidget(), 0, Qt::AlignBaseline);

    layout->addLayout(searchLayout);

    layout->addSpacing(padding);

    QHBoxLayout *recentLayout = new QHBoxLayout();
    recentLayout->setMargin(0);
    recentLayout->setSpacing(10);

    recentKeywordsLayout = new QVBoxLayout();
    recentKeywordsLayout->setMargin(0);
    recentKeywordsLayout->setSpacing(0);
    recentKeywordsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    recentKeywordsLabel = new QLabel(tr("Recent keywords"));
    recentKeywordsLabel->setProperty("recentHeader", true);
    recentKeywordsLabel->hide();
    recentKeywordsLabel->setEnabled(false);
    recentKeywordsLayout->addWidget(recentKeywordsLabel);
    recentLayout->addLayout(recentKeywordsLayout);

    recentChannelsLayout = new QVBoxLayout();
    recentChannelsLayout->setMargin(0);
    recentChannelsLayout->setSpacing(0);
    recentChannelsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    recentChannelsLabel = new QLabel(tr("Recent channels"));
    recentChannelsLabel->setProperty("recentHeader", true);
    recentChannelsLabel->hide();
    recentChannelsLabel->setEnabled(false);
    recentChannelsLayout->addWidget(recentChannelsLabel);
    recentLayout->addLayout(recentChannelsLayout);

    layout->addLayout(recentLayout);

    hLayout->addStretch();

    vLayout->addStretch();

#ifdef APP_ACTIVATION
    if (!Activation::instance().isActivated())
        vLayout->addWidget(ActivationView::buyButton(tr("Get the full version")), 0,
                           Qt::AlignRight);
#endif
}

void SearchView::appear() {
    MainWindow *w = MainWindow::instance();
    w->showActionsInStatusBar(
            {w->getAction("manualplay"), w->getAction("safeSearch"), w->getAction("definition")},
            true);

    updateRecentKeywords();
    updateRecentChannels();

    queryEdit->selectAll();
    queryEdit->enableSuggest();
    QTimer::singleShot(0, queryEdit->toWidget(), SLOT(setFocus()));
}

void SearchView::disappear() {
    MainWindow *w = MainWindow::instance();
    w->showActionsInStatusBar(
            {w->getAction("manualplay"), w->getAction("safeSearch"), w->getAction("definition")},
            false);
}

void SearchView::updateRecentKeywords() {
    // load
    QSettings settings;
    const QStringList keywords = settings.value(recentKeywordsKey).toStringList();
    if (keywords == recentKeywords) return;
    recentKeywords = keywords;

    // cleanup
    QLayoutItem *item;
    while (recentKeywordsLayout->count() - 1 > recentKeywords.size() &&
           (item = recentKeywordsLayout->takeAt(1)) != nullptr) {
        item->widget()->close();
        delete item;
    }

    recentKeywordsLabel->setVisible(!keywords.isEmpty());
    MainWindow::instance()->getAction("clearRecentKeywords")->setEnabled(!keywords.isEmpty());

    const int maxDisplayLength = 25;

#ifdef APP_MAC
    QPalette p = palette();
    p.setColor(QPalette::Highlight, mac::accentColor());
#endif

    int counter = 1;
    for (const QString &keyword : keywords) {
        QString link = keyword;
        QString display = keyword;
        if (keyword.startsWith(QLatin1String("http://")) ||
            keyword.startsWith(QLatin1String("https://"))) {
            int separator = keyword.indexOf('|');
            if (separator > 0 && separator + 1 < keyword.length()) {
                link = keyword.left(separator);
                display = keyword.mid(separator + 1);
            }
        }
        bool needStatusTip = display.length() > maxDisplayLength;
        if (needStatusTip) {
            display.truncate(maxDisplayLength);
            display.append(QStringLiteral("\u2026"));
        }

        ClickableLabel *item;
        if (recentKeywordsLayout->count() - 1 >= counter) {
            item = qobject_cast<ClickableLabel *>(recentKeywordsLayout->itemAt(counter)->widget());

        } else {
            item = new ClickableLabel();
#ifdef APP_MAC
            item->setPalette(p);
#endif
            item->setAttribute(Qt::WA_DeleteOnClose);
            item->setProperty("recentItem", true);
            item->setFocusPolicy(Qt::TabFocus);
            connect(item, &ClickableLabel::hovered, this, [item, this](bool value) {
                item->setForegroundRole(value ? QPalette::Highlight : QPalette::WindowText);
                if (value) {
                    for (int i = 1; i < recentKeywordsLayout->count(); ++i) {
                        QWidget *w = recentKeywordsLayout->itemAt(i)->widget();
                        if (w != item) {
                            w->setForegroundRole(QPalette::WindowText);
                        }
                    }
                }
            });
            recentKeywordsLayout->addWidget(item);
        }

        item->setText(display);
        if (needStatusTip)
            item->setStatusTip(link);
        else
            item->setStatusTip(QString());

        disconnect(item, &ClickableLabel::clicked, nullptr, nullptr);
        connect(item, &ClickableLabel::clicked, this, [this, link]() { watchKeywords(link); });

        counter++;
    }
}

void SearchView::updateRecentChannels() {
    // load
    QSettings settings;
    const QStringList keywords = settings.value(recentChannelsKey).toStringList();
    if (keywords == recentChannels) return;
    recentChannels = keywords;

    // cleanup
    QLayoutItem *item;
    while ((item = recentChannelsLayout->takeAt(1)) != nullptr) {
        item->widget()->close();
        delete item;
    }

    recentChannelsLabel->setVisible(!keywords.isEmpty());

#ifdef APP_MAC
    QPalette p = palette();
    p.setColor(QPalette::Highlight, mac::accentColor());
#endif

    for (const QString &keyword : keywords) {
        QString link = keyword;
        QString display = keyword;
        int separator = keyword.indexOf('|');
        if (separator > 0 && separator + 1 < keyword.length()) {
            link = keyword.left(separator);
            display = keyword.mid(separator + 1);
        }

        ClickableLabel *item = new ClickableLabel(display);
#ifdef APP_MAC
        item->setPalette(p);
#endif
        item->setAttribute(Qt::WA_DeleteOnClose);
        item->setProperty("recentItem", true);
        item->setFocusPolicy(Qt::TabFocus);
        connect(item, &ClickableLabel::clicked, [this, link]() { watchChannel(link); });
        connect(item, &ClickableLabel::hovered, item, [item](bool value) {
            item->setForegroundRole(value ? QPalette::Highlight : QPalette::WindowText);
        });
        recentChannelsLayout->addWidget(item);
    }
}

void SearchView::watch() {
    QString query = queryEdit->text();
    watch(query);
}

void SearchView::textChanged(const QString &text) {
    lastChannelSuggestions.clear();
}

void SearchView::watch(const QString &query) {
    QString q = query.simplified();

    // check for empty query
    if (q.isEmpty()) {
        queryEdit->toWidget()->setFocus(Qt::OtherFocusReason);
        return;
    }

    SearchParams *searchParams = new SearchParams();
    searchParams->setKeywords(q);

    // go!
    emit search(searchParams);
}

void SearchView::watchChannel(const QString &channelId) {
    if (channelId.isEmpty()) {
        queryEdit->toWidget()->setFocus(Qt::OtherFocusReason);
        return;
    }

    QString id = channelId;

    // Fix old settings
    const QLatin1String uc("UC");
    if (!id.startsWith(uc)) id = uc + id;

    SearchParams *searchParams = new SearchParams();
    searchParams->setChannelId(id);
    searchParams->setSortBy(SearchParams::SortByNewest);

    // go!
    emit search(searchParams);
}

void SearchView::watchKeywords(const QString &query) {
    QString q = query.simplified();

    // check for empty query
    if (q.isEmpty()) {
        queryEdit->toWidget()->setFocus(Qt::OtherFocusReason);
        return;
    }

    queryEdit->setText(q);

    SearchParams *searchParams = new SearchParams();
    searchParams->setKeywords(q);

    // go!
    emit search(searchParams);
}

void SearchView::searchTypeChanged(int index) {
    if (index == 0) {
        queryEdit->setSuggester(youtubeSuggest);
    } else {
        queryEdit->setSuggester(channelSuggest);
    }
    queryEdit->selectAll();
    queryEdit->toWidget()->setFocus();
}

void SearchView::suggestionAccepted(Suggestion *suggestion) {
    if (suggestion->type == QLatin1String("channel")) {
        watchChannel(suggestion->userData);
    } else
        watch(suggestion->value);
}

void SearchView::onChannelSuggestions(const QVector<Suggestion *> &suggestions) {
    lastChannelSuggestions = suggestions;
}
