// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "playlistutils.h"

#include <QString>

#include <numeric>

#include <ytmusic.h>

QString PlaylistUtils::artistsToString(const std::vector<meta::Artist> &artists)
{
    if (artists.size() > 0) {
        return std::accumulate(artists.begin() + 1, artists.end(),
                               QString::fromStdString(artists.front().name),
                               [](QString &string, const meta::Artist &artist) {
            return string.append(QStringLiteral(", %1").arg(QString::fromStdString(artist.name)));
        });
    }

    return {};
}
