/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huangyu<huangyub@uniontech.com>
 *
 * Maintainer: huangyu<huangyub@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef STYLESCICPP_H
#define STYLESCICPP_H

#include "textedittabwidget/style/stylesci.h"

class StyleSciCpp : public StyleSci
{
public:
    StyleSciCpp(TextEdit *parent);
    virtual QMap<int, QString> keyWords() const override;
    virtual void setStyle() override;
    virtual int styleOffset() const override;
};

#endif // STYLESCICPP_H