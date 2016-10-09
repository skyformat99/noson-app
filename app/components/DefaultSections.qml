/*
 * Copyright (C) 2016
 *      Jean-Luc Barriere <jlbarriere68@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.4
import Ubuntu.Components 1.3

Sections {
    model: [player.queueInfo, currentZoneTag]
    selectedIndex: 0
    onSelectedIndexChanged: {
        if (selectedIndex == 1 && mainPageStack.currentPage.title !== i18n.tr("Zones")) {
            selectedIndex = 0
            mainPageStack.push(zonesPageLoader.item)
        }
    }
}
