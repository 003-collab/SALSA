/*
    Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
    at Austin (ARL:UT).

    SALSA is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License version 3 (GPL-3.0-only) as published
    by the Free Software Foundation.

    SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
    A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
    SALSA.  If not, see https://www.gnu.org/licenses/.
*/

#include "LSADoubleSpinBox.hpp"

double LSADoubleSpinBox::valueFromText(const QString &text) const
{
    // cleanText removes prefix, suffix, whitespaces
    return this->cleanText().toDouble();
}

// Pretty much reimplement the behaviour of QDoubleSpinBoxPrivate::validateAndInterpret, except for when the
// number of decimals is more than this.decimals
// https://code.woboq.org/qt5/qtbase/src/widgets/widgets/qspinbox.cpp.html#_ZNK21QDoubleSpinBoxPrivate20validateAndInterpretER7QStringRiRN10QValidator5StateE
// Takes the `text` and decides if the user is actually allowed to enter it.  `pos` is the cursor position.
// The returned `QValidator::State` has three options:
// `Accepted` - Let the user enter the text
// `Intermediate` - Maybe the text will be good after the next keypress (pressing 4 when value must be [10, 50]).
// `Invalid` - Don't let the user enter the text
QValidator::State LSADoubleSpinBox::validate(QString &text, int &pos) const
{
    auto min = minimum();
    auto max = maximum();

    QValidator::State state;
    auto locality = locale();
    QString copy = text;

    // Remove and prefix
    copy.remove(prefix());

    // The suffix will keep the value from being acceptable, remove it for this.
    auto suff = suffix();
    if (!suff.isEmpty())
    {
        auto idx = copy.lastIndexOf(suff);
        if(idx != -1)
        {
            copy = copy.left(idx);
        }
    }

    int len = copy.size();
    // default the value to min
    double num = min;

    const bool plus = max >= 0;
    const bool minus = min <= 0;

    // Some edge cases when there are 0,1, or 2 characters
    switch (len)
    {
        case 0: // Empty text
            state = max != min ? QValidator::Intermediate : QValidator::Invalid;
            goto end;
        case 1: // One character
            // If first char is `.`, `+`, or `-`...
            if (copy.at(0) == locality.decimalPoint()
                || (plus && copy.at(0) == QLatin1Char('+'))
                || (minus && copy.at(0) == QLatin1Char('-')))
            {
                state = QValidator::Intermediate;
                goto end;
            }
            break;
        case 2:
            // If the second char is `.`, AND fist char is `-` or `+`
            if (copy.at(1) == locality.decimalPoint()
                && ((plus && copy.at(0) == QLatin1Char('+')) || (minus && copy.at(0) == QLatin1Char('-'))))
            {
                state = QValidator::Intermediate;
                goto end;
            }
            break;
        default: break;
    }

    // Can't have a groupSeperator (delimeter in list of values)
    if (copy.at(0) == locality.groupSeparator())
    {
        state = QValidator::Invalid;
        goto end;
    }
    else if (len > 1)
    {
        const int dec = copy.indexOf(locality.decimalPoint());
        // If we found the decimalPoint (`.` here in the U.S.)
        if (dec != -1)
        {
            // typing a delimiter when you are on the delimiter
            // should be treated as typing right arrow
            if (dec + 1 < copy.size() && copy.at(dec + 1) == locality.decimalPoint() && pos == dec + 1)
            {
                copy.remove(dec + 1, 1);
            }
            // Make sure there are no spaces or delimeters after the decimal and before the end of the text
            for (int i=dec + 1; i<copy.size(); ++i)
            {
                if (copy.at(i).isSpace() || copy.at(i) == locality.groupSeparator())
                {
                    state = QValidator::Invalid;
                    goto end;
                }
            }
        }
        else
        {
            // Check if there is a delimeter-space or space-delimeter at the end of the text
            const QChar last = copy.at(len - 1);
            const QChar secondLast = copy.at(len - 2);
            if ((last == locality.groupSeparator() || last.isSpace())
                && (secondLast == locality.groupSeparator() || secondLast.isSpace()))
            {
                state = QValidator::Invalid;
                goto end;
            }
            // See if there is a space before the last digit (while spaces aren't valid)
            else if (last.isSpace() && (!locality.groupSeparator().isSpace() || secondLast.isSpace()))
            {
                state = QValidator::Invalid;
                goto end;
            }
        }
    }

    {
        // See if we can make the text into a double
        bool ok = false;
        num = locality.toDouble(copy, &ok);
        if (!ok)
        {
            // If the separator is printable...
            if (locality.groupSeparator().isPrint())
            {
                if (max < 1000 && min > -1000 && copy.contains(locality.groupSeparator()))
                {
                    state = QValidator::Invalid;
                    goto end;
                }
                const int len = copy.size();
                // Make sure there aren't two Separators in a row
                for (int i=0; i<len- 1; ++i)
                {
                    if (copy.at(i) == locality.groupSeparator() && copy.at(i + 1) == locality.groupSeparator())
                    {
                        state = QValidator::Invalid;
                        goto end;
                    }
                }
                // Delete the separator and see if we can convert to a double again.
                QString copy2 = copy;
                copy2.remove(locality.groupSeparator());
                num = locality.toDouble(copy2, &ok);
                if (!ok)
                {
                    state = QValidator::Invalid;
                    goto end;
                }
            }
        }
        if (!ok)
        {
            state = QValidator::Invalid;
        }
        else if (num >= min && num <= max)
        {
            state = QValidator::Acceptable;
        }
        else if (max == min)
        { // when max and min is the same the only non-Invalid input is max (or min)
            state = QValidator::Invalid;
        }
        else
        {
            if ((num >= 0 && num > max) || (num < 0 && num < min))
            {
                state = QValidator::Invalid;
            }
            else
            {
                state = QValidator::Intermediate;
            }
        }
    }
end:

    // Put back the prefix and suffix on the text
    text = prefix() + copy + suffix();
    return state;
}
